// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlickerLight.generated.h"

UCLASS()
class SHATTEREDMIND_API AFlickerLight : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFlickerLight();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	//// Called every frame
	//virtual void Tick(float DeltaTime) override;


private:
    UPROPERTY(VisibleAnywhere)
    class UPointLightComponent* PointLight;

    UPROPERTY(EditAnywhere, Category = "Flicker")
    float BaseIntensity = 3500.0f;

    FTimerHandle FlickerTimer;
    FTimerHandle RestTimer;
    FTimerHandle SequenceTimer;

    void StartFlickerSequence(); // ±Ù∫˝¿” Ω√¿€
    void DoFlicker();            // Ω«¡¶ ±Ù∫˝¿”
    void StopFlickerSequence();  // «— Ω√ƒˆΩ∫ ¡æ∑·
};


