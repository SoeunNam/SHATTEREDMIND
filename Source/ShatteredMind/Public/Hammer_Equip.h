// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Hammer_Equip.generated.h"

/**
 * 
 */
UCLASS()
class SHATTEREDMIND_API UHammer_Equip : public UAnimNotify
{
	GENERATED_BODY()
	
public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notify")
    FName SocketName = "Hand_RSocket"; // 기본값: 오른손 소켓

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notify")
    FName SocketName2 = "spine_03Socket"; // 기본값: 오른손 소켓



    // 총을 집는 노티파이 구간에 허리 소켓에 있던 망치(액터)를 손의 소켓으로 이동 시킨다 .
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
