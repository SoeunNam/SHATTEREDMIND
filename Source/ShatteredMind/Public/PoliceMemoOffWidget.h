// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PoliceMemoOffWidget.generated.h"

/**
 *
 */
UCLASS()
class SHATTEREDMIND_API UPoliceMemoOffWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void SetUpdateMarkVisible(bool bVisible);

protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TextBlock_UpdateMark;
};
