// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Doctor.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "NotifyState_HammerWeapon.generated.h"


// 플레이어2 의사 클래스 전방선언
class ADoctor;


/**
 * 
 */
UCLASS()
class SHATTEREDMIND_API UNotifyState_HammerWeapon : public UAnimNotifyState
{
	GENERATED_BODY()


public:

	UPROPERTY()
	ADoctor* Player2_Doctor;

	
	// 노티파이가 실행 될 때 근접 무기 콜리전 활성화
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference);
	// 노티파이가 종료 될 때 근접 무기 콜리전 비활성화
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference);
	
	
};
