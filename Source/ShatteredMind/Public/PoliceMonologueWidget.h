// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PoliceMonologueWidget.generated.h"

/**
 *
 */
UCLASS()
class SHATTEREDMIND_API UPoliceMonologueWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    //대사 넣기
    UFUNCTION(BlueprintCallable)
    void SetMonologueText(const FString& NewText);

public:
    // BP에서 바인딩할 TextBlock
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* MonologueText;
};
