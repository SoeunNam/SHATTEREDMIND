// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionInterface.h"
#include "InterfaceTestActor.generated.h"

UCLASS()
class SHATTEREDMIND_API AInterfaceTestActor : public AActor, public IInteractionInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInterfaceTestActor();

protected:

	UPROPERTY(EditAnywhere, Category = "TestActor")
	UStaticMeshComponent* Mesh;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditInstanceOnly, Category = "TestActor")
	FInteractableData InstanceInteractableData;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void BeginFocus() override;
	virtual void EndFocus() override;
	virtual void BeginInteract() override;
	virtual void EndInteract() override;
	virtual void Interact(APolice* _Playercharacter) override;
	virtual void DoctorInteract(ADoctor* _Playercharacter) override; // 20251022 희빈 의사가 상호작용 하기 위해 추가함
};
