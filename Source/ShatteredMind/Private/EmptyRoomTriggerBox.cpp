// Fill out your copyright notice in the Description page of Project Settings.


#include "EmptyRoomTriggerBox.h"
#include "Kismet/GameplayStatics.h"
#include "Police.h"
#include "Engine/TriggerBox.h"
#include "PoliceMonologueWidget.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

AEmptyRoomTriggerBox::AEmptyRoomTriggerBox()
{
	OnActorBeginOverlap.AddDynamic(this, &AEmptyRoomTriggerBox::OnOverlapBegin);
	//MonologueLine = FText::FromString(TEXT("이 방에는 아무 것도 없군."));
}

void AEmptyRoomTriggerBox::BeginPlay()
{
	Super::BeginPlay();
}

void AEmptyRoomTriggerBox::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	if (bWidgetShown) return; // 이미 한 번 보여줬으면 종료

	if (OtherActor && OtherActor->ActorHasTag("Player"))
	{
		ShowMonologue();
		bWidgetShown = true;
	}
}

void AEmptyRoomTriggerBox::ShowMonologue()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !MonologueWidgetClass) return;

	// 위젯 생성
	UPoliceMonologueWidget* Widget = CreateWidget<UPoliceMonologueWidget>(PC, MonologueWidgetClass);
	if (Widget)
	{
		Widget->AddToViewport();

		// 여기서 ShowLine 함수를 직접 만들 수 있음
		// 예: 단순히 텍스트만 설정
		Widget->MonologueText->SetText(MonologueLine);

		// 3초 후 자동 제거
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [Widget]()
			{
				if (Widget)
					Widget->RemoveFromParent();
			}, 3.f, false);
	}
}
