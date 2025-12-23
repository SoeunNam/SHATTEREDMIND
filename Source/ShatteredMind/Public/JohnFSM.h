#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JohnFSM.generated.h"

UENUM(BlueprintType)
enum class EJohnState : uint8
{
	Rest   UMETA(DisplayName = "Rest"),
	Patrol   UMETA(DisplayName = "Patrol")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SHATTEREDMIND_API UJohnFSM : public UActorComponent
{
	GENERATED_BODY()

public:
	UJohnFSM();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// 상태별 함수
	void RestState(float DeltaTime);
	void PatrolState(float DeltaTime);

	// 타깃 설정
	void PickNextTarget();
	bool ProjectToNav(const FVector& In, FVector& Out) const;

public:
	UPROPERTY()
	ACharacter* OwnerChar;

	UPROPERTY(VisibleAnywhere, Category = "JFSM")
	EJohnState mState = EJohnState::Rest;

	// ================= 설정값 =================
	UPROPERTY(EditAnywhere, Category = "JFSM")
	float RestDuration = 5.f; // 쉬는 시간

	UPROPERTY(EditAnywhere, Category = "JFSM")
	int32 MoveCountToReturn = 4; // 몇 번 이동 후 복귀

	UPROPERTY(EditAnywhere, Category = "JFSM")
	float MoveMinDistance = 250.f; // 최소 이동거리
	UPROPERTY(EditAnywhere, Category = "JFSM")
	float MoveMaxDistance = 500.f; // 최대 이동거리

	UPROPERTY(EditAnywhere, Category = "JFSM")
	float MoveMinTime = 0.6f; // 최소 이동 지속시간
	UPROPERTY(EditAnywhere, Category = "JFSM")
	float MoveMaxTime = 1.5f; // 최대 이동 지속시간

	UPROPERTY(EditAnywhere, Category = "JFSM")
	float AcceptanceRadius = 100.f; // 느슨한 도착판정

	// ================= 내부변수 =================
	UPROPERTY(VisibleAnywhere, Category = "JFSM|Debug")
	int32 MoveCount = 0;

	UPROPERTY(VisibleAnywhere, Category = "JFSM|Debug")
	bool bReturning = false;

	UPROPERTY(VisibleAnywhere, Category = "JFSM|Debug")
	float RestTimer = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "JFSM|Debug")
	float MoveTimer = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "JFSM|Debug")
	FVector SpawnLocation;

	UPROPERTY(VisibleAnywhere, Category = "JFSM|Debug")
	FVector TargetLocation;

public:
	UPROPERTY(EditAnywhere, Category = "JFSM|Movement")
	float WalkSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category = "JFSM|Movement")
	float ReturnWalkSpeed = 200.f;

public:
	UFUNCTION(BlueprintCallable, Category = "JFSM")
	void SetPaused(bool bInPaused);

	UPROPERTY(VisibleAnywhere, Category = "JFSM|Debug")
	bool bPaused = false;


};

