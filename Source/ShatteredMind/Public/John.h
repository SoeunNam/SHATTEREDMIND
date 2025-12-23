// John.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "John.generated.h"

UCLASS()
class SHATTEREDMIND_API AJohn : public ACharacter
{
	GENERATED_BODY()

public:
	AJohn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	// Á¸ FSM ÄÄÆ÷³ÍÆ®
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "John")
	class UJohnFSM* JohnFSM;

	UFUNCTION(BlueprintPure, Category = "John")
	class UJohnFSM* GetJohnFSM() const { return JohnFSM; }

};
