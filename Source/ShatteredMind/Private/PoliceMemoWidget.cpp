// Fill out your copyright notice in the Description page of Project Settings.


#include "PoliceMemoWidget.h"

void UPoliceMemoWidget::UnlockMemo(int32 MemoIndex)
{
    switch (MemoIndex)
    {
    case 1: if (MemoText_1) MemoText_1->SetVisibility(ESlateVisibility::Visible); break;
    case 2: if (MemoText_2) MemoText_2->SetVisibility(ESlateVisibility::Visible); break;
    case 3: if (MemoText_3) MemoText_3->SetVisibility(ESlateVisibility::Visible); break;
    case 4: if (MemoText_4) MemoText_4->SetVisibility(ESlateVisibility::Visible); break;
    case 5: if (MemoText_5) MemoText_5->SetVisibility(ESlateVisibility::Visible); break;
    case 6: if (MemoText_6) MemoText_6->SetVisibility(ESlateVisibility::Visible); break;

    default: break;
    }
}

void UPoliceMemoWidget::InitializeMemoTexts()
{
    if (MemoText_1)
    {
        MemoText_1->SetText(FText::FromString(TEXT("▶ 범행 동기 파악. 연구실 책상 서랍?\n연구실은 복도 끝 왼쪽 방.\n\n")));
        MemoText_1->SetVisibility(ESlateVisibility::Visible);
    }
    if (MemoText_2)
    {
        MemoText_2->SetText(FText::FromString(TEXT("▶ 간호사\n의사는 이상적인 아버지같은 인격?\n\n")));
        MemoText_2->SetVisibility(ESlateVisibility::Visible);
    }
    if (MemoText_3)
    {
        MemoText_3->SetText(FText::FromString(TEXT("▶ 환자\n의사가 환자의 가족관계에 대해 관심을 가짐\n추가 특이사항 없음\n\n")));
        MemoText_3->SetVisibility(ESlateVisibility::Visible);
    }
    if (MemoText_4)
    {
        MemoText_4->SetText(FText::FromString(TEXT("▶ 신문에서 잘라낸 어떤 가족의 사진...?\n위에 빨간 펜으로 대충 쓴 글씨가 써있다.\n\n")));
        MemoText_4->SetVisibility(ESlateVisibility::Visible);
    }
    if (MemoText_5)
    {
        MemoText_5->SetText(FText::FromString(TEXT("▶ 환자들에 대한 애착...\n그럼에도 불구하고 범행?\n\n")));
        MemoText_5->SetVisibility(ESlateVisibility::Visible);
    }
    if (MemoText_6)
    {
        MemoText_6->SetText(FText::FromString(TEXT("▶ 서랍 자물쇠의 암호는...")));
        MemoText_6->SetVisibility(ESlateVisibility::Visible);
    }
}

void UPoliceMemoWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 이 시점에 BindWidget으로 바인딩된 MemoText_* 포인터들이 유효해야 함
    InitializeMemoTexts();
}
