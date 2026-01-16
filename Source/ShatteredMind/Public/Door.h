// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Police.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundBase.h"
#include "InteractionInterface.h"
#include "NavModifierComponent.h"
#include "NavAreas/NavArea_Null.h"
#include "Door.generated.h"


UCLASS()
class SHATTEREDMIND_API ADoor : public AActor, public IInteractionInterface
{
	GENERATED_BODY()

public:
	ADoor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// 책이나 아이템 설명 (블루프린트에서 개별 지정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	FText ItemDescription;

	UPROPERTY(EditAnywhere, Category = "Door")
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditInstanceOnly, Category = "Door")
	FInteractableData InstanceInteractableData;

	virtual void BeginFocus() override;
	virtual void EndFocus() override;
	virtual void BeginInteract() override;
	virtual void EndInteract() override;
	virtual void Interact(APolice* _Playercharacter) override;
	virtual void DoctorInteract(ADoctor* _Playercharacter) override; // 플레이어 의사도 상호작용

	UPROPERTY(EditAnywhere, Category = "Door")
	float OpenAngle = 90.f;

	UPROPERTY(EditAnywhere, Category = "Door")
	float OpenSpeed = 2.f; // 회전 속도

	bool bIsOpen = false;
	FRotator ClosedRotation;
	FRotator TargetRotation;

	// ─── 상호작용(플레이어/AI) ───
	UFUNCTION(BlueprintCallable, Category = "Door")
	void ToggleDoorByAI(AActor* Opener); // 플레이어는 토글 사용 OK

	
	// ★ 연타 방지(토글/오픈/클로즈 공통)
	UPROPERTY(EditAnywhere, Category = "Door")
	float ToggleCooldown = 0.35f;

	// 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Sounds")
	USoundCue* DoorOpenSoundCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Sounds")
	USoundCue* DoorCloseSoundCue;

private:
	float LastChangeTime = -1000.f;

	// ===== 몬스터 전용 처리 보조 메서드 =====
	bool IsMonster(const AActor* Opener) const;

	// 열림 시 충돌 적용(플레이어/몬스터 분기)
	void ApplyCollisionForOpen(AActor* Opener);

	// 고스트 기간 종료 후, 열려있는 상태라면 ‘열림용 충돌’로 복구
	void RestoreOpenCollision();

	FTimerHandle GhostTimerHandle;
};
