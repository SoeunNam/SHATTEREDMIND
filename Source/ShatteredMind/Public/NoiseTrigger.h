// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PoliceMonologueWidget.h"
#include "Engine/TriggerBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Internationalization/Text.h"
#include "NoiseTrigger.generated.h"

/**
 *
 */
UCLASS()
class SHATTEREDMIND_API ANoiseTrigger : public ATriggerBox
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> NoiseWidgetClass;

    ANoiseTrigger();

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class UPoliceMonologueWidget> PoliceMonologueWidgetClass;

    void ShowPoliceMonologue();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

    UFUNCTION()
    void HideNoiseWidget();

private:
    UPROPERTY()
    UUserWidget* NoiseWidget;

    // 한 번만 실행 여부 체크
    bool bHasTriggered = false;

};
