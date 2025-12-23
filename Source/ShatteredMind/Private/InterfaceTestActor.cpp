// Fill out your copyright notice in the Description page of Project Settings.


#include "InterfaceTestActor.h"

// Sets default values
AInterfaceTestActor::AInterfaceTestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetRenderCustomDepth(false);
	Mesh->SetCustomDepthStencilValue(1);
}

// Called when the game starts or when spawned
void AInterfaceTestActor::BeginPlay()
{
	Super::BeginPlay();
	//Mesh->SetRenderCustomDepth(false);

	InteractableData = InstanceInteractableData;
}

// Called every frame
void AInterfaceTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	
}

void AInterfaceTestActor::BeginFocus()
{
	//UE_LOG(LogTemp, Warning, TEXT("beginfocus"));

	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(true);
	}
}

void AInterfaceTestActor::EndFocus()
{
	//UE_LOG(LogTemp, Warning, TEXT("endfocus"));

	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(false);
	}
}

void AInterfaceTestActor::BeginInteract()
{
	//UE_LOG(LogTemp, Warning, TEXT("Calling BeginInteract override on interface test actor"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Calling BeginInteract override on interface test actor"));
}

void AInterfaceTestActor::EndInteract()
{
	//UE_LOG(LogTemp, Warning, TEXT("Calling EndInteract override on interface test actor"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Calling EndInteract override on interface test actor"));
}

void AInterfaceTestActor::Interact(APolice* _Playercharacter)
{
	//UE_LOG(LogTemp, Warning, TEXT("Calling Interact override on interface test actor"));
}

void AInterfaceTestActor::DoctorInteract(ADoctor* _Playercharacter)
{
	//UE_LOG(LogTemp, Warning, TEXT("Calling Interact override on interface test actor"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Calling Interact override on interface test actor"));
}

