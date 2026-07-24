// Fill out your copyright notice in the Description page of Project Settings.


#include "SpeechRecognizerComponent.h"

// Sets default values for this component's properties
USpeechRecognizerComponent::USpeechRecognizerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USpeechRecognizerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!InitializeSpeech())
	{
		UE_LOG(LogTemp, Error, TEXT("Speech initialization failed."));
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

	// ...
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
	if (Recognizer)
	{
		Recognizer->Release();
		Recognizer = nullptr;
	}

	CoUninitialize();

	UE_LOG(LogTemp, Warning, TEXT("COM Shutdown."));
}

void USpeechRecognizerComponent::PollSpeech()
{
}

