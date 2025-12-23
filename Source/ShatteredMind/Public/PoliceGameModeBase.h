// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PoliceGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class SHATTEREDMIND_API APoliceGameModeBase : public AGameModeBase
{
	GENERATED_BODY()


// 플레이어 경찰이 일기장을 열었는지 확인하기 위한 변수
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Story")
	bool bDiaryRead = false;
	
};
