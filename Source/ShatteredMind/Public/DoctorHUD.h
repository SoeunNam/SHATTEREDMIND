//// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "DiaryWidget.h"
#include "DoctorHUD.generated.h"

class UDiaryWidget;

/**
 * 커스텀 HUD 클래스: DiaryWidget을 화면에 표시
 */
UCLASS()
class SHATTEREDMIND_API ADoctorHUD : public AHUD
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UDiaryWidget> DiaryWidgetClass;

    UPROPERTY()
    UDiaryWidget* DiaryWidget;

    // UI 열기
    void ShowDiary(const FString& Text, APlayerController* PC);



};