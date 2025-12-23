// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionInterface.generated.h"

class APolice;
class ADoctor;

//상호작용 종류
UENUM()
enum class EInteractableType : uint8
{
	//아이템 조사
	PickUp UMETA(DisplayName = "PickUp"),
	//npc상호작용
	NPC UMETA(DisplayName = "NPC"),
	//문열고닫기
	Door UMETA(DisplayName = "Door")
};

USTRUCT()
struct FInteractableData
{
	GENERATED_BODY()

	//상호작용 대상 정보 초기화
	FInteractableData() :
		InteractableType(EInteractableType::PickUp),
		Name(FText::GetEmpty()),
		Action(FText::GetEmpty()),
		Quantity(0),
		InteractionDuration(0.0f)
	{
	};


	//타입
	UPROPERTY(EditInstanceOnly)
	EInteractableType InteractableType;
	//이름
	UPROPERTY(EditInstanceOnly)
	FText Name;
	//행동
	UPROPERTY(EditInstanceOnly)
	FText Action;
	//수량, Pickup에서만 사용
	UPROPERTY(EditInstanceOnly)
	int32 Quantity;
	//키 누르는 시간, 오래 눌러야되는게 있을 경우
	// (문열기 밸브돌리기 등등.. 근데 필요없을듯)
	UPROPERTY(EditInstanceOnly)
	float InteractionDuration;

};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class IInteractionInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	////액터 입장에서의 상호작용
	//상호작용 전에 테두리이펙트
	virtual void BeginFocus();
	//상호작용 후에 테두리 이펙트 꺼짐?
	virtual void EndFocus();
	//상호작용 시작
	// 상호작용 시간(키 누르고있는 시간)을 0으로 설정해서 상호작용 바로 실행
	virtual void BeginInteract();
	//상호작용 끝
	virtual void EndInteract();
	//상호작용
	virtual void Interact(APolice* _Playercharacter);
	// 상호작용 의사
	virtual void DoctorInteract(ADoctor* _Playercharacter); // 20251022 희빈 의사가 상호작용 하기 위해 추가함

	FInteractableData InteractableData;
};
