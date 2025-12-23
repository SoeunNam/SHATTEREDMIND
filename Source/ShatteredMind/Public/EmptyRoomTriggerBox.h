// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "EmptyRoomTriggerBox.generated.h"

/**
 *
 */
UCLASS()
class SHATTEREDMIND_API AEmptyRoomTriggerBox : public ATriggerBox
{
	GENERATED_BODY()

public:
	AEmptyRoomTriggerBox();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

	// 위젯 띄우기
	void ShowMonologue();

public:
	UPROPERTY(EditAnywhere, Category = "Monologue")
	TSubclassOf<class UPoliceMonologueWidget> MonologueWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Monologue")
	FText MonologueLine;

private:
	bool bWidgetShown = false; // 한 번만 보여주기 위한 플래그
};
