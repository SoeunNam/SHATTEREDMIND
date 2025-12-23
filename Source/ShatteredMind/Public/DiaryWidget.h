#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "DiaryWidget.generated.h"

UCLASS()
class SHATTEREDMIND_API UDiaryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 기존: 단일 텍스트 (있어도 무방)
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* DiaryText;

    // 새로 추가: 왼쪽/오른쪽 페이지 텍스트
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* LeftPageText;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* RightPageText;

public:
    // ? 기존 코드들과의 호환을 위해 단일 페이지 업데이트 함수 복원
    UFUNCTION(BlueprintCallable)
    void UpdateDiaryText(const FString& NewText)
    {
        if (DiaryText)
        {
            DiaryText->SetText(FText::FromString(NewText));
        }
        else if (LeftPageText) // DiaryText 없으면 왼쪽 페이지에 표시
        {
            LeftPageText->SetText(FText::FromString(NewText));
        }
    }

    // 왼쪽+오른쪽 페이지를 동시에 업데이트
    UFUNCTION(BlueprintCallable)
    void UpdateDiaryPages(const FString& LeftText, const FString& RightText)
    {
        if (LeftPageText)
            LeftPageText->SetText(FText::FromString(LeftText));

        if (RightPageText)
            RightPageText->SetText(FText::FromString(RightText));

        // 폴백(Left/Right 위젯이 없을 경우)
        if (!LeftPageText && !RightPageText && DiaryText)
        {
            const FString Merged = LeftText + TEXT("\n\n? ? ?\n\n") + RightText;
            DiaryText->SetText(FText::FromString(Merged));
        }
    }
};
