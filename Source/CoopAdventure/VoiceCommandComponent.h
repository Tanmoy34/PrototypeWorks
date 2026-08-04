// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AudioCaptureCore.h"
#include "VoiceCommandComponent.generated.h"

// Opaque Vosk types - we never dereference these, only pass pointers
// through function pointers loaded at runtime (see .cpp), so no real
// struct definition or vosk_api.h include is needed at all anymore.
struct VoskModel;
struct VoskRecognizer;

// Function pointer types matching Vosk's C API. Declared here (instead of
// linking against libvosk.lib) so that CoopAdventure.dll has NO link-time
// dependency on libvosk.dll whatsoever - it's loaded and resolved entirely
// at runtime via LoadVoskDll(). This means a missing/misplaced Vosk DLL can
// never prevent the game module itself from loading; at worst, voice
// commands silently fail to initialize while everything else works fine.
typedef VoskModel* (*PFN_vosk_model_new)(const char* model_path);
typedef void (*PFN_vosk_model_free)(VoskModel* model);
typedef VoskRecognizer* (*PFN_vosk_recognizer_new_grm)(VoskModel* model, float sample_rate, const char* grammar);
typedef void (*PFN_vosk_recognizer_free)(VoskRecognizer* recognizer);
typedef int (*PFN_vosk_recognizer_accept_waveform_s)(VoskRecognizer* recognizer, const int16* data, int length);
typedef const char* (*PFN_vosk_recognizer_result)(VoskRecognizer* recognizer);
typedef const char* (*PFN_vosk_recognizer_final_result)(VoskRecognizer* recognizer);
typedef const char* (*PFN_vosk_recognizer_partial_result)(VoskRecognizer* recognizer);
typedef void (*PFN_vosk_set_log_level)(int log_level);

/**
 * Free, fully-offline push-to-talk voice command recognizer using Vosk
 * (https://alphacephei.com/vosk/), restricted to a small grammar built
 * from ANPCCharacter's editable VoiceCommands list. Restricting the
 * vocabulary (instead of open dictation, which is what Windows SAPI was
 * doing) is what actually makes recognition reliable for short commands.
 *
 * Attach this to the player character. Bind a "V" Input Action:
 *   Started   -> StartListening()
 *   Completed -> StopListening()
 *
 * Setup required outside of this code (see README_VoiceCommands.md):
 *   1. Download the Vosk C API binaries + a small English model from
 *      https://alphacephei.com/vosk/models (vosk-model-small-en-us is fine
 *      for short commands and is ~40MB).
 *   2. Add "AudioCaptureCore" to your .Build.cs PublicDependencyModuleNames.
 *   3. Add the Vosk include path / lib / DLL to your .Build.cs and make
 *      sure the DLL ships alongside your .exe.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UVoiceCommandComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVoiceCommandComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Folder containing the unzipped Vosk model (e.g.
	// Content/../VoskModels/vosk-model-small-en-us-0.15). Set per-project.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Command")
	FString ModelDirectory;

	// Optional. Plays the instant V is pressed, so the player has a clear
	// cue that listening has started. Any short Sound Wave/Sound Cue works.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Command")
	class USoundBase* ListenStartSound = nullptr;

	// Optional. Plays the instant V is released.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Command")
	class USoundBase* ListenStopSound = nullptr;

	// Which input device to record from. -1 (default) uses whatever
	// Windows currently has set as the default recording device. If your
	// default mic doesn't work and you can't change it in Windows Sound
	// settings, use "Log Available Input Devices" below to find the index
	// of the mic you actually want, then set it here instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Command")
	int32 PreferredInputDeviceIndex = -1;

	// Click this in the Details panel (works even outside Play) to print
	// every input device Windows can see, with its index, to the Output
	// Log - e.g. "[0] Microphone Array (Realtek)". Use the index that
	// corresponds to the mic you want in PreferredInputDeviceIndex above.
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Voice Command")
	void LogAvailableInputDevices();

	// Show a live mic level meter and the recognized text on screen while
	// listening, via GEngine->AddOnScreenDebugMessage. Turn off if you'd
	// rather wire this up to your own UMG widget instead (see
	// OnPartialResult/OnAudioLevel below for that).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Command")
	bool bShowDebugOnScreen = true;

	// Call on Input Action "Started" (key down).
	UFUNCTION(BlueprintCallable, Category = "Voice Command")
	void StartListening();

	// Call on Input Action "Completed" (key up). Triggers final recognition
	// and, if any text came back, calls the owning character's
	// IssueNPCVoiceCommand().
	UFUNCTION(BlueprintCallable, Category = "Voice Command")
	void StopListening();

	UPROPERTY(BlueprintReadOnly, Category = "Voice Command")
	bool bIsListening = false;

	// Fires continuously while listening with the current mic level
	// (roughly 0-1) and whatever partial text Vosk has recognized so far.
	// Bind these in Blueprint if you want your own UMG display instead of
	// (or in addition to) the on-screen debug text.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVoiceCaptureUpdate, float, AudioLevel, const FString&, PartialText);
	UPROPERTY(BlueprintAssignable, Category = "Voice Command")
	FOnVoiceCaptureUpdate OnVoiceCaptureUpdate;

private:
	VoskModel* Model = nullptr;
	VoskRecognizer* Recognizer = nullptr;

	Audio::FAudioCapture AudioCapture;

	FCriticalSection AudioBufferLock;
	TArray<int16> PendingAudioBuffer;

	// Simple peak level of the most recent audio chunk, 0-1. Written on the
	// audio thread in OnAudioCaptured (under AudioBufferLock, same as the
	// buffer), read on the game thread in TickComponent - this is the
	// "is my mic actually picking anything up" indicator.
	float CurrentAudioLevel = 0.f;

	bool InitializeVosk();
	void ShutdownVosk();

	// Loads libvosk.dll from its real on-disk path and resolves each Vosk
	// function via GetProcAddress. No PublicAdditionalLibraries / no
	// PublicDelayLoadDLLs needed in the .Build.cs at all for this anymore.
	void* VoskDllHandle = nullptr;
	bool LoadVoskDll();

	PFN_vosk_model_new p_vosk_model_new = nullptr;
	PFN_vosk_model_free p_vosk_model_free = nullptr;
	PFN_vosk_recognizer_new_grm p_vosk_recognizer_new_grm = nullptr;
	PFN_vosk_recognizer_free p_vosk_recognizer_free = nullptr;
	PFN_vosk_recognizer_accept_waveform_s p_vosk_recognizer_accept_waveform_s = nullptr;
	PFN_vosk_recognizer_result p_vosk_recognizer_result = nullptr;
	PFN_vosk_recognizer_final_result p_vosk_recognizer_final_result = nullptr;
	PFN_vosk_recognizer_partial_result p_vosk_recognizer_partial_result = nullptr;
	PFN_vosk_set_log_level p_vosk_set_log_level = nullptr;

	// Bound to the audio capture stream; runs on an audio thread, so it
	// only ever appends to PendingAudioBuffer under the lock and returns.
	void OnAudioCaptured(const float* AudioData, int32 NumFrames, int32 NumChannels, int32 SampleRate, double StreamTime, bool bOverflow);

	// Called from Tick on the game thread: drains PendingAudioBuffer into
	// Vosk and checks for a final result once listening has stopped.
	void ProcessBufferedAudio();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Builds the Vosk grammar JSON array (e.g. ["stand","stay","wander","[unk]"])
	// from the owning ANPCCharacter's VoiceCommands so recognition is
	// restricted to just those words/phrases.
	FString BuildGrammarJson() const;
};
