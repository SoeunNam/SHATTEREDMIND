// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "InteractionInterface.h"

void UInteractionWidget::UpdateWidget(const FInteractableData* _InteractableData)
{
	// 상호작용 가능한 객체의 타입에 따라 위젯 UI를 업데이트
	switch (_InteractableData->InteractableType)
	{
	case EInteractableType::PickUp: // 픽업 가능한 아이템일 때
		// 키 입력 안내 텍스트 설정 (예: "Press")
		KeyPressText->SetText(FText::FromString("Press"));
		// 픽업은 진행 게이지가 필요 없으므로 프로그레스 바 숨김
		InteractionProgressBar->SetVisibility(ESlateVisibility::Collapsed);

		/* 수량 출력 필요 없을 듯
		if (_InteractableData->Quantity == 2)
		{
			QuantityText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			// "x{수량}" 형식으로 수량 텍스트를 갱신하고 표시
			QuantityText->SetText(FText::Format(NSLOCTEXT("InteractionWidget", "QuantityText", "x{0}"),
				_InteractableData->Quantity));
			QuantityText->SetVisibility(ESlateVisibility::Visible);
		}
		*/
		break;

	case EInteractableType::NPC:// NPC일 때 
		// 키 입력 안내 텍스트 설정 (예: "Press")
		KeyPressText->SetText(FText::FromString("Press"));
		// 픽업은 진행 게이지가 필요 없으므로 프로그레스 바 숨김
		InteractionProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		break;

	case EInteractableType::Door:// 문일 때
		// 키 입력 안내 텍스트 설정 (예: "Press")
		KeyPressText->SetText(FText::FromString("Press"));
		// 픽업은 진행 게이지가 필요 없으므로 프로그레스 바 숨김
		InteractionProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		break;

	default:;

	}
	// 상호작용 행동 텍스트와 이름 텍스트를 설정
	//ActionText->SetText(_InteractableData->Action);
	//NameText->SetText(_InteractableData->Name);
}

float UInteractionWidget::UpdateInteractionProgress()
{
	return 0.0f;
}
