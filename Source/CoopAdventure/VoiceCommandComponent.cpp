// Fill out your copyright notice in the Description page of Project Settings.

#include "VoiceCommandComponent.h"
#include "CoopAdventureCharacter.h"
#include "NPCCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

// No vosk_api.h include and no link-time dependency on libvosk.lib -
// everything is resolved at runtime via GetProcAddress in LoadVoskDll(),
// so a missing/misplaced Vosk DLL can never stop CoopAdventure.dll itself
// from loading.

UVoiceCommandComponent::UVoiceCommandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UVoiceCommandComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only the locally controlled player needs a live mic + recognizer.
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (!OwnerPawn->IsLocallyControlled())
		{
			return;
		}
	}

	if (!InitializeVosk())
	{
		UE_LOG(LogTemp, Error, TEXT("VoiceCommandComponent: Vosk initialization failed. Check ModelDirectory."));
	}
}

void UVoiceCommandComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownVosk();
	Super::EndPlay(EndPlayReason);
}

bool UVoiceCommandComponent::LoadVoskDll()
{
	if (VoskDllHandle)
	{
		return true; // already loaded
	}

	// NOTE: adjust "vosk-win64-0.3.45" below if your folder name differs -
	// keep this in sync with the folder path used in CoopAdventure.Build.cs.
	const FString VoskDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("ThirdParty"), TEXT("Vosk"), TEXT("vosk-win64-0.3.45"));
	const FString DllPath = FPaths::Combine(VoskDir, TEXT("libvosk.dll"));

	if (!FPaths::FileExists(DllPath))
	{
		UE_LOG(LogTemp, Error, TEXT("VoiceCommandComponent: libvosk.dll not found at %s"), *DllPath);
		return false;
	}

	// Load libvosk.dll's own dependencies first, individually, so if one of
	// them is the actual problem we find out which one instead of just
	// getting a generic failure on the main DLL.
	auto TryLoadDependency = [&VoskDir](const TCHAR* DllName) -> bool
	{
		const FString DepPath = FPaths::Combine(VoskDir, DllName);
		void* Handle = FPlatformProcess::GetDllHandle(*DepPath);
		if (!Handle)
		{
			const uint32 ErrorCode = FPlatformMisc::GetLastError();
			TCHAR ErrorBuffer[1024];
			FPlatformMisc::GetSystemErrorMessage(ErrorBuffer, 1024, ErrorCode);
			UE_LOG(LogTemp, Error, TEXT("VoiceCommandComponent: failed to load dependency %s - Win32 error %u: %s"), DllName, ErrorCode, ErrorBuffer);
			return false;
		}
		// Intentionally leaked here - stays resident for the process, freed
		// implicitly on exit. We only needed to confirm/force it's loaded.
		return true;
	};

	TryLoadDependency(TEXT("libwinpthread-1.dll"));
	TryLoadDependency(TEXT("libgcc_s_seh-1.dll"));
	TryLoadDependency(TEXT("libstdc++-6.dll"));

	// Loading libvosk.dll by its full path means Windows searches that same
	// folder for its own dependencies, which is how it resolves them
	// without needing PATH changes.
	VoskDllHandle = FPlatformProcess::GetDllHandle(*DllPath);

	if (!VoskDllHandle)
	{
		const uint32 ErrorCode = FPlatformMisc::GetLastError();
		TCHAR ErrorBuffer[1024];
		FPlatformMisc::GetSystemErrorMessage(ErrorBuffer, 1024, ErrorCode);
		UE_LOG(LogTemp, Error, TEXT("VoiceCommandComponent: failed to load libvosk.dll from %s - Win32 error %u: %s"), *DllPath, ErrorCode, ErrorBuffer);
		return false;
	}

	// Resolve each function we use. If any are missing, this is the wrong
	// DLL (e.g. a different Vosk version with a renamed export) - fail
	// cleanly rather than crash on first call.
	p_vosk_model_new = (PFN_vosk_model_new)FPlatformProcess::GetDllExport(VoskDllHandle, TEXT("vosk_model_new"));
	p_vosk_model_free = (PFN_vosk_model_free)FPlatformProcess::GetDllExport(VoskDllHandle, TEXT("vosk_model_free"));
	p_vosk_recognizer_new_grm = (PFN_vosk_recognizer_new_grm)FPlatformProcess::GetDllExport(VoskDllHandle, TEXT("vosk_recognizer_new_grm"));
	p_vosk_recognizer_new = (PFN_vosk_recognizer_new)FPlatformProcess::GetDllExport(VoskDllHandle, TEXT("vosk_recognizer_new"));
	p_vosk_recognizer_free = (PFN_vosk_recognizer_free)FPlatformProcess::GetDllExport(VoskDllHandle, TEXT("vosk_recognizer_free"));
	p_vosk_recognizer_accept_waveform_s = (PFN_vosk_recognizer_accept_waveform_s)FPlatformProcess::GetDllExport(VoskDllHandle, TEXT("vosk_recognizer_accept_waveform_s"));
	p_vosk_recognizer_result = (PFN_vosk_recognizer_result)FPlatformProcess::GetDllExport(VoskDllHandle, TEXT("vosk_recognizer_result"));
	p_vosk_recognizer_final_result = (PFN_vosk_recognizer_final_result)FPlatformProcess::GetDllExport(VoskDllHandle, TEXT("vosk_recognizer_final_result"));
	p_vosk_recognizer_partial_result = (PFN_vosk_recognizer_partial_result)FPlatformProcess::GetDllExport(VoskDllHandle, TEXT("vosk_recognizer_partial_result"));
	p_vosk_set_log_level = (PFN_vosk_set_log_level)FPlatformProcess::GetDllExport(VoskDllHandle, TEXT("vosk_set_log_level"));

	if (!p_vosk_model_new || !p_vosk_model_free || !p_vosk_recognizer_new_grm || !p_vosk_recognizer_new || !p_vosk_recognizer_free ||
		!p_vosk_recognizer_accept_waveform_s || !p_vosk_recognizer_result || !p_vosk_recognizer_final_result ||
		!p_vosk_recognizer_partial_result || !p_vosk_set_log_level)
	{
		UE_LOG(LogTemp, Error, TEXT("VoiceCommandComponent: libvosk.dll loaded but one or more expected exports were missing - is this the right Vosk version?"));
		return false;
	}

	return true;
}

bool UVoiceCommandComponent::InitializeVosk()
{
	if (!LoadVoskDll())
	{
		return false;
	}

	if (ModelDirectory.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("VoiceCommandComponent: ModelDirectory is empty - set it in the Details panel to your unzipped Vosk model folder."));
		return false;
	}

	// Defensive: strip stray leading/trailing quote characters and
	// whitespace. Windows Explorer's "Copy as path" wraps paths in quotes,
	// and a quote pasted straight into this field silently breaks
	// vosk_model_new (it just looks for a folder that can't exist).
	FString CleanModelDirectory = ModelDirectory.TrimStartAndEnd();
	CleanModelDirectory.RemoveFromStart(TEXT("\""));
	CleanModelDirectory.RemoveFromEnd(TEXT("\""));

	// If someone pastes an absolute Windows path in here (starts with a
	// drive letter like "D:", or is already rooted), respect it as-is for
	// backwards compatibility - but the normal, portable case is a
	// project-relative path, resolved against wherever THIS machine's
	// project actually lives rather than trusting a string that was only
	// ever true on the machine that saved the Blueprint.
	FString ResolvedModelDirectory = FPaths::IsRelative(CleanModelDirectory)
		? FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), CleanModelDirectory))
		: CleanModelDirectory;

	p_vosk_set_log_level(0); // was -1 (silent) - turned up so Vosk prints its own diagnostic if the model fails to load

	UE_LOG(LogTemp, Log, TEXT("VoiceCommandComponent: loading Vosk model from \"%s\" (ModelDirectory=\"%s\")"), *ResolvedModelDirectory, *CleanModelDirectory);

	Model = p_vosk_model_new(TCHAR_TO_UTF8(*ResolvedModelDirectory));
	if (!Model)
	{
		UE_LOG(LogTemp, Error, TEXT("VoiceCommandComponent: failed to load Vosk model at %s"), *ResolvedModelDirectory);
		return false;
	}

	const FString Grammar = BuildGrammarJson();
	UE_LOG(LogTemp, Log, TEXT("VoiceCommandComponent: grammar = %s"), *Grammar);

	// If this only ever prints ["[unk]"] with no real words, the NPC's
	// VoiceCommands list was empty when this ran (e.g. no NPC found in the
	// level yet) - the recognizer then has nothing to match against and
	// will never return real text, regardless of mic quality.
	if (Grammar == TEXT("[\"[unk]\"]"))
	{
		UE_LOG(LogTemp, Warning, TEXT("VoiceCommandComponent: grammar has no real command words - is there an NPCCharacter placed in the level with VoiceCommands filled in?"));
	}

	// Restricting to a grammar (instead of vosk_recognizer_new, which does
	// open dictation) is the key accuracy improvement over the old SAPI
	// setup - the recognizer only ever has to choose between your command
	// words, not the entire English language.
	Recognizer = bUseOpenDictationForDebug
		? p_vosk_recognizer_new(Model, TargetSampleRate)
		: p_vosk_recognizer_new_grm(Model, TargetSampleRate, TCHAR_TO_UTF8(*Grammar));

	if (bUseOpenDictationForDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoiceCommandComponent: bUseOpenDictationForDebug is ON - grammar restriction is bypassed, commands will NOT trigger. Turn this off once you've confirmed Vosk can hear you."));
	}

	if (!Recognizer)
	{
		UE_LOG(LogTemp, Error, TEXT("VoiceCommandComponent: failed to create Vosk recognizer."));
		return false;
	}

	return true;
}

void UVoiceCommandComponent::ShutdownVosk()
{
	if (AudioCapture.IsStreamOpen())
	{
		AudioCapture.StopStream();
		AudioCapture.CloseStream();
	}

	if (Recognizer)
	{
		p_vosk_recognizer_free(Recognizer);
		Recognizer = nullptr;
	}

	if (Model)
	{
		p_vosk_model_free(Model);
		Model = nullptr;
	}

	if (VoskDllHandle)
	{
		FPlatformProcess::FreeDllHandle(VoskDllHandle);
		VoskDllHandle = nullptr;
	}
}

FString UVoiceCommandComponent::BuildGrammarJson() const
{
	// Pull the phrase list straight off the NPC so the grammar always
	// matches whatever the designer edited in the NPC's Details panel -
	// one editable source of truth for both UI and voice.
	TArray<AActor*> FoundNPCs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANPCCharacter::StaticClass(), FoundNPCs);

	TArray<FString> Words;
	if (FoundNPCs.Num() > 0)
	{
		if (ANPCCharacter* NPC = Cast<ANPCCharacter>(FoundNPCs[0]))
		{
			for (const FNPCVoiceCommand& Command : NPC->VoiceCommands)
			{
				for (const FString& Phrase : Command.TriggerPhrases)
				{
					Words.Add(Phrase);
				}
			}
		}
	}

	// [unk] lets Vosk fall back gracefully on noise instead of forcing a
	// wrong match to one of the real commands.
	FString Json = TEXT("[");
	for (const FString& Word : Words)
	{
		Json += FString::Printf(TEXT("\"%s\","), *Word.ToLower());
	}
	Json += TEXT("\"[unk]\"]");

	return Json;
}

void UVoiceCommandComponent::LogAvailableInputDevices()
{
	// A throwaway capture object just for enumeration - doesn't need to be
	// the same one used for actual listening, and works whether or not
	// Vosk itself has initialized.
	Audio::FAudioCapture TempCapture;

	TArray<Audio::FCaptureDeviceInfo> Devices;

	// NOTE: same version-sensitivity as elsewhere in this file - if
	// GetCaptureDevicesAvailable isn't the right name/signature on your
	// engine version, check AudioCaptureCore.h for the equivalent (it may
	// be GetInputDevicesAvailable or similar depending on UE5.x version).
	if (!TempCapture.GetCaptureDevicesAvailable(Devices))
	{
		UE_LOG(LogTemp, Warning, TEXT("VoiceCommandComponent: could not enumerate input devices."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("VoiceCommandComponent: available input devices:"));
	for (int32 Index = 0; Index < Devices.Num(); ++Index)
	{
		UE_LOG(LogTemp, Log, TEXT("  [%d] %s"), Index, *Devices[Index].DeviceName);
	}

	if (Devices.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("  (no input devices found - check Windows Sound settings > Input)"));
	}
}

int32 UVoiceCommandComponent::ResolvePreferredDeviceIndex() const
{
	if (PreferredInputDeviceNameContains.IsEmpty())
	{
		return PreferredInputDeviceIndex; // may still be -1 = OS default
	}

	// Fresh enumeration right before opening the stream - this is what
	// makes name matching robust against index-order surprises: we're
	// always looking the name up against the live list, not trusting a
	// number written down earlier.
	Audio::FAudioCapture TempCapture;
	TArray<Audio::FCaptureDeviceInfo> Devices;

	if (!TempCapture.GetCaptureDevicesAvailable(Devices))
	{
		UE_LOG(LogTemp, Warning, TEXT("VoiceCommandComponent: could not enumerate input devices to match \"%s\" - falling back to OS default."), *PreferredInputDeviceNameContains);
		return -1;
	}

	for (int32 Index = 0; Index < Devices.Num(); ++Index)
	{
		if (Devices[Index].DeviceName.Contains(PreferredInputDeviceNameContains))
		{
			UE_LOG(LogTemp, Log, TEXT("VoiceCommandComponent: matched \"%s\" -> [%d] %s"), *PreferredInputDeviceNameContains, Index, *Devices[Index].DeviceName);
			return Index;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("VoiceCommandComponent: no input device name contains \"%s\" - falling back to OS default. Available devices were logged above/via LogAvailableInputDevices."), *PreferredInputDeviceNameContains);
	return -1;
}

void UVoiceCommandComponent::StartListening()
{
	if (bIsListening) return;

	if (!Recognizer)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoiceCommandComponent: StartListening called but Vosk isn't initialized."));
		return;
	}

	{
		FScopeLock Lock(&AudioBufferLock);
		PendingAudioBuffer.Reset();
	}

	ResampleReadPos = 0.0;

	Audio::FAudioCaptureDeviceParams Params = Audio::FAudioCaptureDeviceParams();
	Params.DeviceIndex = ResolvePreferredDeviceIndex();

	// This engine version's callback hands back an untyped const void* -
	// the capture stream is opened below with default (float) format, so
	// it's safe to reinterpret it as const float* before use.
	Audio::FOnAudioCaptureFunction OnCapture = [this](const void* AudioData, int32 NumFrames, int32 NumChannels, int32 SampleRate, double StreamTime, bool bOverflow)
	{
		OnAudioCaptured(static_cast<const float*>(AudioData), NumFrames, NumChannels, SampleRate, StreamTime, bOverflow);
	};

	if (AudioCapture.OpenAudioCaptureStream(Params, MoveTemp(OnCapture), 1024))
	{
		AudioCapture.StartStream();
		bIsListening = true;

		// Log which mic Windows actually gave us - Params above didn't
		// specify a device, so this is whatever your OS default input
		// device is (Windows Settings > Sound > Input).
		// NOTE: like the capture callback earlier, this method's exact name
		// has moved around a bit across UE5.x (GetCaptureDeviceInfo vs
		// GetStreamInfo, etc.) - if this line doesn't compile, check
		// AudioCaptureCore.h for your engine version, or just delete this
		// block; it's diagnostic-only and not required for anything to work.
		Audio::FCaptureDeviceInfo DeviceInfo;
		if (AudioCapture.GetCaptureDeviceInfo(DeviceInfo))
		{
			UE_LOG(LogTemp, Log, TEXT("VoiceCommandComponent: listening on input device \"%s\""), *DeviceInfo.DeviceName);
		}

		if (ListenStartSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), ListenStartSound);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("VoiceCommandComponent: failed to open the mic capture stream."));
	}
}

void UVoiceCommandComponent::StopListening()
{
	if (!bIsListening) return;

	bIsListening = false;

	if (ListenStopSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), ListenStopSound);
	}

	if (AudioCapture.IsStreamOpen())
	{
		AudioCapture.StopStream();
		AudioCapture.CloseStream();
	}

	// Drain whatever's left, then ask Vosk for the final result.
	ProcessBufferedAudio();

	// Clear the live meter/partial-text debug lines now that we're done.
	if (bShowDebugOnScreen && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9001, 0.01f, FColor::Green, TEXT(""));
	}

	if (!Recognizer) return;

	const FString ResultJson = UTF8_TO_TCHAR(p_vosk_recognizer_final_result(Recognizer));

	// Always log the raw, unparsed response - this is the ground truth of
	// what Vosk actually returned, whether or not it happened to contain a
	// usable "text" field. Compare this against the grammar logged above.
	UE_LOG(LogTemp, Log, TEXT("VoiceCommandComponent: raw result = %s"), *ResultJson);

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResultJson);
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		FString RecognizedText;
		if (JsonObject->TryGetStringField(TEXT("text"), RecognizedText) && !RecognizedText.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("VoiceCommandComponent heard: \"%s\""), *RecognizedText);

			if (bShowDebugOnScreen && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(9002, 3.f, FColor::Cyan, FString::Printf(TEXT("Final: \"%s\""), *RecognizedText));
			}

			if (ACoopAdventureCharacter* Character = Cast<ACoopAdventureCharacter>(GetOwner()))
			{
				Character->IssueNPCVoiceCommand(RecognizedText);
			}
		}
		else if (bShowDebugOnScreen && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(9002, 2.f, FColor::Red, FString::Printf(TEXT("Final: (nothing recognized) raw=%s"), *ResultJson));
		}
	}
}

void UVoiceCommandComponent::OnAudioCaptured(const float* AudioData, int32 NumFrames, int32 NumChannels, int32 SampleRate, double StreamTime, bool bOverflow)
{
	// Runs on the audio capture thread - keep this cheap.
	FScopeLock Lock(&AudioBufferLock);

	if (NumFrames <= 0 || SampleRate <= 0) return;

	float PeakThisChunk = 0.f;

	// Linear-interpolation resample from the mic's native rate (commonly
	// 44100 or 48000 Hz) down to the 16000 Hz the Vosk model was created
	// expecting. Skipping this and feeding native-rate samples straight
	// through - which the very first version of this file did - plays
	// your voice back roughly 3x too fast internally as far as Vosk is
	// concerned: it's pure noise to the recognizer no matter how clearly
	// you speak, which is exactly what was happening before this fix.
	const double Ratio = (double)SampleRate / (double)TargetSampleRate;

	while (ResampleReadPos < (double)(NumFrames - 1))
	{
		const int32 Index = (int32)ResampleReadPos;
		const float Frac = (float)(ResampleReadPos - (double)Index);

		const float SampleA = AudioData[Index * NumChannels];
		const float SampleB = AudioData[FMath::Min(Index + 1, NumFrames - 1) * NumChannels];
		float Interpolated = FMath::Lerp(SampleA, SampleB, Frac);
		Interpolated = FMath::Clamp(Interpolated, -1.f, 1.f);

		PendingAudioBuffer.Add((int16)(Interpolated * 32767.f));
		PeakThisChunk = FMath::Max(PeakThisChunk, FMath::Abs(Interpolated));

		ResampleReadPos += Ratio;
	}

	// Carry the fractional remainder into the next chunk instead of
	// resetting to 0, so the resampling stays continuous across buffer
	// boundaries rather than clicking/gapping every callback.
	ResampleReadPos -= (double)NumFrames;

	// Smoothed rather than an instant snap, so the on-screen meter doesn't
	// flicker to zero between words - still responds fast enough to make
	// it obvious the mic is (or isn't) picking anything up.
	CurrentAudioLevel = FMath::Max(PeakThisChunk, CurrentAudioLevel * 0.85f);
}

void UVoiceCommandComponent::ProcessBufferedAudio()
{
	if (!Recognizer) return;

	TArray<int16> LocalBuffer;
	{
		FScopeLock Lock(&AudioBufferLock);
		if (PendingAudioBuffer.Num() == 0) return;
		LocalBuffer = MoveTemp(PendingAudioBuffer);
		PendingAudioBuffer.Reset();
	}

	p_vosk_recognizer_accept_waveform_s(Recognizer, LocalBuffer.GetData(), LocalBuffer.Num());
}

void UVoiceCommandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsListening) return;

	ProcessBufferedAudio();

	float Level = 0.f;
	{
		FScopeLock Lock(&AudioBufferLock);
		Level = CurrentAudioLevel;
	}

	FString PartialText;
	if (Recognizer && p_vosk_recognizer_partial_result)
	{
		const FString PartialJson = UTF8_TO_TCHAR(p_vosk_recognizer_partial_result(Recognizer));

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(PartialJson);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			JsonObject->TryGetStringField(TEXT("partial"), PartialText);
		}
	}

	OnVoiceCaptureUpdate.Broadcast(Level, PartialText);

	if (bShowDebugOnScreen && GEngine)
	{
		// Simple bar meter out of block characters - fills up as the level
		// rises, so you can see at a glance whether the mic is picking
		// anything up at all, independent of whether Vosk understood it.
		const int32 NumBars = FMath::Clamp(FMath::RoundToInt(Level * 20.f), 0, 20);
		FString Meter = TEXT("Mic [");
		for (int32 i = 0; i < 20; ++i)
		{
			Meter += (i < NumBars) ? TEXT("#") : TEXT("-");
		}
		Meter += TEXT("]");

		// Fixed keys so each message overwrites its own line every frame
		// instead of stacking up new ones.
		GEngine->AddOnScreenDebugMessage(9001, 0.1f, FColor::Green, Meter);
		GEngine->AddOnScreenDebugMessage(9002, 0.1f, FColor::Yellow,
			PartialText.IsEmpty() ? TEXT("Heard: ...") : FString::Printf(TEXT("Heard: %s"), *PartialText));
	}
}
