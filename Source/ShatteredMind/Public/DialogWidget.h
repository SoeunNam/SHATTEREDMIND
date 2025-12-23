// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogTypes.h"
#include "Components/RichTextBlock.h"
#include "DialogWidget.generated.h"

UCLASS()
class SHATTEREDMIND_API UDialogWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // NPC로부터 대화 데이터를 받아서 화면에 갱신
    UFUNCTION(BlueprintCallable, Category = "Dialog")
    void UpdateDialog(const FDialogLine& DialogLine);

protected:
    // 블루프린트에서 바인딩할 텍스트 위젯
    UPROPERTY(meta = (BindWidget))
    class URichTextBlock* SpeakerText;

    UPROPERTY(meta = (BindWidget))
    class URichTextBlock* LineText;
};