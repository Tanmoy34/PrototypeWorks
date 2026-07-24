#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <sapi.h>
#include "Windows/HideWindowsPlatformTypes.h"

#include "SpeechRecognizerComponent.generated.h"

UCLASS(ClassGroup=(Custom), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class COOPADVENTURE_API USpeechRecognizerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	USpeechRecognizerComponent();

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:

	// Microsoft Speech API Objects
	ISpRecognizer* Recognizer = nullptr;

	ISpRecoContext* Context = nullptr;

	ISpRecoGrammar* Grammar = nullptr;

	// Functions
	bool InitializeSpeech();

	bool CreateRecognizer();

	bool CreateRecognitionContext();

	bool CreateGrammar();

	void ShutdownSpeech();

	void PollSpeech();
};