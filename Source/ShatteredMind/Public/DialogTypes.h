// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialogTypes.generated.h"

/**
 * NPC의 대사 한 줄을 저장하는 구조체
 * - Speaker: 화자 이름 (예: 경찰, 환자)
 * - Line: 대사 내용 (예: "여긴 위험해요.")
 */
USTRUCT(BlueprintType)
struct FDialogLine
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
    FText SpeakerName; // 말하는 사람 이름

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
    FText LineText; // 실제 대사 문장

    //기본 생성자
    FDialogLine() {}

    // FText 기반 생성자
    FDialogLine(FText InSpeakerName, FText InLineText)
        : SpeakerName(InSpeakerName), LineText(InLineText)
    {
    }

    // FString 기반 생성자 ? 여기서 유니코드 문자열도 안전하게 FText로 변환
    FDialogLine(const FString& InSpeakerName, const FString& InLineText)
        : SpeakerName(FText::FromString(InSpeakerName)), LineText(FText::FromString(InLineText))
    {
    }

    // TCHAR* / TEXT() 문자열 기반 생성자
    FDialogLine(const TCHAR* InSpeakerName, const TCHAR* InLineText)
        : SpeakerName(FText::FromString(FString(InSpeakerName)))
        , LineText(FText::FromString(FString(InLineText)))
    {
    }
};