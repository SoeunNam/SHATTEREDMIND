// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionInterface.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "InteractionWidget.generated.h"

// 플레이어 클래스 전방 선언
class APolice;
// 상호작용 대상의 정보를 담은 구조체 전방 선언
struct FInteractableData;
// UI 구성요소 전방 선언
class UTextBlock;
class UProgressBar;
/**
 * 플레이어와 상호작용할 때 정보를 표시하는 위젯 클래스
 */
UCLASS()
class SHATTEREDMIND_API UInteractionWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	// 상호작용 정보를 표시할 때 참조할 플레이어 객체
	UPROPERTY(VisibleAnywhere, Category = "InteractionWidget|PlayerReference")
	APolice* PlayerReference;

	// 상호작용 데이터로 위젯 갱신
	void UpdateWidget(const FInteractableData* _InteractableData);

protected:
	// 상호작용 대상의 이름을 표시하는 텍스트 블록
	//UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category = "InteractionWidget|InteractableData")
	//UTextBlock* NameText;
	
	// 상호작용 행동(예: 열기, 줍기 등)을 표시하는 텍스트 블록
	//UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category = "InteractionWidget|InteractableData")
	//UTextBlock* ActionText;
	/*
	// 상호작용 대상의 수량을 표시하는 텍스트 블록
	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category = "InteractionWidget|InteractableData")
	UTextBlock* QuantityText;*/

	// 상호작용에 사용할 키 입력을 표시하는 텍스트 블록
	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category = "InteractionWidget|InteractableData")
	UTextBlock* KeyPressText;

	// 상호작용 진행 상황을 표시하는 프로그레스 바
	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category = "InteractionWidget|InteractableData")
	UProgressBar* InteractionProgressBar;

	//현재 상호작용 지속된 시간(꾹 누르기)
	UPROPERTY(VisibleAnywhere, meta = (BindWidget), Category = "InteractionWidget|InteractableData")
	float CurrentInteractionDuration;

	UFUNCTION(Category = "Interaction|InteractableData")
	float UpdateInteractionProgress();

};
