// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Pistol_Equip.generated.h"

/**
 * 
 */
UCLASS()
class SHATTEREDMIND_API UPistol_Equip : public UAnimNotify
{
	GENERATED_BODY()


public:

    //====20250926 희빈 머지 ==========

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notify")
    FName SocketName = "Hand_RSocket"; // 기본값: 오른손 소켓

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notify")
    FName SocketName2 = "spine_01Socket"; // 기본값: 오른손 소켓



    // 총을 집는 노티파이 구간에 허리 소켓에 있던 권총(액터)를 손의 소켓으로 이동 시킨다 .
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	
};
