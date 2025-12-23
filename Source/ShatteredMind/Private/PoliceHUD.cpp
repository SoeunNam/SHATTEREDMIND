// Fill out your copyright notice in the Description page of Project Settings.


#include "PoliceHUD.h"
#include "MainMenu.h"
#include "InteractionWidget.h"

APoliceHUD::APoliceHUD()
{
}

void APoliceHUD::BeginPlay()
{
	Super::BeginPlay();

	//메인 메뉴 위젯 생성 및 초기 설정
	if (MainMenuClass)
	{
		//MainMenuClass를 기반으로 메인 메뉴 위젯 생성
		MainMenuWidget = CreateWidget<UMainMenu>(GetWorld(), MainMenuClass);
		//
		MainMenuWidget->AddToViewport(5);
		//시작 시 메뉴는 보이지 않도록 Collapsed 상태
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (InteractionWidgetClass)
	{
		//상호작용 위젯 생성 및 초기 설정
		InteractionWidget = CreateWidget<UInteractionWidget>(GetWorld(), InteractionWidgetClass);
		//뷰포트에 추가, 메뉴보다 뒤에 배치
		InteractionWidget->AddToViewport(-1);
		//시작 시 상호작용 위젯은 보이지 않도록 Collapsed 상태
		InteractionWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}


void APoliceHUD::DisplayMenu()
{
	//메인메뉴 표시
	if (MainMenuWidget)
	{
		bIsMenuVisible = true;
		MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void APoliceHUD::OffMenu()
{
	//메인 메뉴 숨김
	if (MainMenuWidget)
	{
		bIsMenuVisible = false;
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void APoliceHUD::ShowInteractionWidget()
{
	//상호작용 위젯 표시
	if (InteractionWidget)
	{
		InteractionWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void APoliceHUD::HideInteractionWidget()
{
	//상호작용 위젯 숨김
	if (InteractionWidget)
	{
		InteractionWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void APoliceHUD::UpdateInteractionWidget(const FInteractableData* _InteractableData)
{
	//상호작용 대상 정보를 받아 위젯 업데이트
	if (InteractionWidget)
	{
		//위젯이 숨겨져있으면 먼저 표시
		if (InteractionWidget->GetVisibility() == ESlateVisibility::Collapsed)
		{
			InteractionWidget->SetVisibility(ESlateVisibility::Visible);
		}
		//상호작용 대상 정보로 위젯 내용 갱신
		InteractionWidget->UpdateWidget(_InteractableData);
	}
}

