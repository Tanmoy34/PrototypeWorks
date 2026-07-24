// Fill out your copyright notice in the Description page of Project Settings.


#include "SpeechRecognizerComponent.h"

// Sets default values for this component's properties
USpeechRecognizerComponent::USpeechRecognizerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Default vocabulary. Overridable per-instance in the editor.
	CommandPhrases = {
		TEXT("move away"),
		TEXT("move aside"),
		TEXT("clear the path"),
		TEXT("please move away")
	};
}


// Called when the game starts
void USpeechRecognizerComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();

	APawn* Pawn = Cast<APawn>(Owner);

	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (!InitializeSpeech())
	{
		UE_LOG(LogTemp, Error, TEXT("Speech recognition failed to initialize; disabling component."));
		SetComponentTickEnabled(false);
	}
}

void USpeechRecognizerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownSpeech();
	Super::EndPlay(EndPlayReason);
}


// Called every frame
void USpeechRecognizerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	PollSpeech();
}

bool USpeechRecognizerComponent::InitializeSpeech()
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error, TEXT("COM Initialization Failed."));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("COM Initialized Successfully."));

	if (!CreateRecognizer())
	{
		return false;
	}
	if (!CreateRecognitionContext())
	{
		return false;
	}
	if (!CreateGrammar())
	{
		return false;
	}
	if (!BuildCommandGrammar())
	{
		return false;
	}
	if (!ActivateGrammar())
	{
		return false;
	}

	return true;
}

bool USpeechRecognizerComponent::CreateRecognizer()
{
	HRESULT hr = CoCreateInstance(
		CLSID_SpSharedRecognizer,
		nullptr,
		CLSCTX_ALL,
		IID_ISpRecognizer,
		reinterpret_cast<void**>(&Recognizer));

	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error,
			TEXT("Failed to create Speech Recognizer. HRESULT = 0x%08X"),
			hr);

		return false;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Speech Recognizer Created Successfully."));

	return true;
}

void USpeechRecognizerComponent::ShutdownSpeech()
{
	if (Grammar)
	{
		Grammar->Release();
		Grammar = nullptr;
	}
	if (Context)
	{
		Context->Release();
		Context = nullptr;
	}
	if (Recognizer)
	{
		Recognizer->Release();
		Recognizer = nullptr;
	}

	CoUninitialize();

	UE_LOG(LogTemp, Warning, TEXT("COM Shutdown."));
}
bool USpeechRecognizerComponent::CreateRecognitionContext()
{
	HRESULT hr = Recognizer->CreateRecoContext(&Context);

	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error,
			TEXT("Failed to create Recognition Context. HRESULT = 0x%08X"),
			hr);

		return false;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Recognition Context Created Successfully."));

	return true;
}

bool USpeechRecognizerComponent::CreateGrammar()
{
	HRESULT hr = Context->CreateGrammar(1, &Grammar);

	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error,
			TEXT("Failed to create Grammar. HRESULT = 0x%08X"),
			hr);

		return false;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Grammar Created Successfully."));

	return true;
}

bool USpeechRecognizerComponent::BuildCommandGrammar()
{
	if (!Grammar)
	{
		return false;
	}

	// SAPI doesn't hand back a separate "rule" object — GetRule() returns an
	// opaque SPSTATEHANDLE representing the rule's entry state, and the
	// grammar-editing calls (ClearRule/AddWordTransition/Commit) are methods
	// on ISpRecoGrammar itself, keyed off that handle.
	SPSTATEHANDLE InitialState = 0;

	HRESULT hr = Grammar->GetRule(
		CommandRuleName,
		0,
		SPRAF_TopLevel | SPRAF_Active,
		true,
		&InitialState);

	if (FAILED(hr) || !InitialState)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get grammar rule. HRESULT = 0x%08X"), hr);
		return false;
	}

	hr = Grammar->ClearRule(InitialState);

	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to clear grammar rule. HRESULT = 0x%08X"), hr);
		return false;
	}

	for (const FString& Phrase : CommandPhrases)
	{
		// hFromState = the rule's entry state, hToState = nullptr (the
		// rule's implicit exit state) attaches the whole phrase as one
		// transition per phrase.
		hr = Grammar->AddWordTransition(
			InitialState,
			nullptr,
			*Phrase,
			L" ",
			SPWT_LEXICAL,
			1.0f,
			nullptr);

		if (FAILED(hr))
		{
			UE_LOG(LogTemp, Error,
				TEXT("Failed to add command '%s' to grammar. HRESULT = 0x%08X"),
				*Phrase, hr);
			return false;
		}
	}

	hr = Grammar->Commit(0);

	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to commit grammar. HRESULT = 0x%08X"), hr);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Command Grammar Built Successfully (%d phrases)."), CommandPhrases.Num());

	return true;
}

bool USpeechRecognizerComponent::ActivateGrammar()
{
	if (!Context || !Grammar)
	{
		return false;
	}

	HRESULT hr = Context->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION));

	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to set recognition interest. HRESULT = 0x%08X"), hr);
		return false;
	}

	hr = Context->SetNotifyWin32Event();

	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to set notify event. HRESULT = 0x%08X"), hr);
		return false;
	}

	hr = Grammar->SetRuleState(CommandRuleName, nullptr, SPRS_ACTIVE);

	if (FAILED(hr))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to activate grammar rule. HRESULT = 0x%08X"), hr);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Grammar Activated Successfully."));

	return true;
}

void USpeechRecognizerComponent::PollSpeech()
{
	if (!Context)
	{
		return;
	}

	// SetNotifyWin32Event() means WaitForRecoNotifyEvent()/GetNotifyEventHandle()
	// is signaled whenever a queued event is ready; here we just drain the
	// queue non-blockingly every tick rather than waiting on the handle.
	SPEVENT Event;
	ULONG Fetched = 0;

	FMemory::Memzero(Event);

	while (SUCCEEDED(Context->GetEvents(1, &Event, &Fetched)) && Fetched > 0)
	{
		HandleRecognitionEvent(Event);
		FMemory::Memzero(Event);
	}
}

void USpeechRecognizerComponent::HandleRecognitionEvent(const SPEVENT& Event)
{
	if (Event.eEventId != SPEI_RECOGNITION)
	{
		return;
	}

	if (Event.elParamType != SPET_LPARAM_IS_OBJECT || Event.lParam == 0)
	{
		return;
	}

	ISpRecoResult* Result = reinterpret_cast<ISpRecoResult*>(Event.lParam);

	WCHAR* RecognizedText = nullptr;
	HRESULT hr = Result->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, true, &RecognizedText, nullptr);

	if (SUCCEEDED(hr) && RecognizedText)
	{
		FString Command(RecognizedText);

		UE_LOG(LogTemp, Warning, TEXT("Speech Recognized: %s"), *Command);

		OnSpeechRecognized.Broadcast(Command);

		CoTaskMemFree(RecognizedText);
	}

	Result->Release();
}