#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <sapi.h>
#include "Windows/HideWindowsPlatformTypes.h"

#include "SpeechRecognizerComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpeechRecognized,const FString&,Command);


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

	UPROPERTY(BlueprintAssignable)
	FOnSpeechRecognized OnSpeechRecognized;

	// The phrases the grammar will accept. Editable per-actor if a specific
	// NPC/pawn needs a different vocabulary.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speech")
	TArray<FString> CommandPhrases;

private:

	// Microsoft Speech API Objects
	ISpRecognizer* Recognizer = nullptr;

	ISpRecoContext* Context = nullptr;

	ISpRecoGrammar* Grammar = nullptr;

	static constexpr const WCHAR* CommandRuleName = L"CommandRule";

	// Functions
	bool InitializeSpeech();

	bool CreateRecognizer();

	bool CreateRecognitionContext();

	bool CreateGrammar();

	void ShutdownSpeech();

	void PollSpeech();

	bool BuildCommandGrammar();

	bool ActivateGrammar();

	void HandleRecognitionEvent(const SPEVENT& Event);
};