// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EvadeComponent.generated.h"

// 회피 방향을 정의하는 열거형(숫자 값으로 방향을 구분하며, 해당 방향에 맞는 애니메이션 섹션 실행에 사용됨)
UENUM(BlueprintType)
enum class EEvadeDirection : uint8
{
	None    UMETA(DisplayName = "None"),
	Evade_Forward     UMETA(DisplayName = "Evade_Forward"),
	Evade_Left       UMETA(DisplayName = "Evade_Left"),
	Evade_Right      UMETA(DisplayName = "Evade_Right"),
	Evade_Backward    UMETA(DisplayName = "Evade_Backward")

};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SHATTEREDMIND_API UEvadeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UEvadeComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


private:

	// 플레이어 캐릭터 참조
	class ADoctor* Doctor_Ref;

	// 현재 회피 중인지 여부를 나타내는 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evade/Dodge",
		meta = (AllowPrivateAccess = "true"))
	bool bIsEvading;

	// 현재 회피 방향
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evade/Dodge",
		meta = (AllowPrivateAccess = "true"))
	EEvadeDirection EvadeDirection;

	// 재생할 회피 애니메이션 몽타주 변수 (플레이어 블루프린트에서 지정 가능)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evade/Dodge",
		meta = (AllowPrivateAccess = "true"))
	UAnimMontage* EvadeMontage;

	// 실행할 애니메이션 섹션 이름 (예 : Evsade_Right )
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evade/Dodge",
		meta = (AllowPrivateAccess = "true"))
	FName EvadeSectionName;

	// 무기를 숨길 때 사용할 머터리얼 ( 투명 처리용 )
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evade/Dodge",
		meta = (AllowPrivateAccess = "true"))
	UMaterialInterface* TransparentMaterial;

	// 회피 중 무기를 일시적으로 숨기기 위한 머터리얼
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evade/Dodge",
		meta = (AllowPrivateAccess = "true"))
	UMaterialInterface* WeaponMaterial;

	// 회피 시 실행 할 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evade/Dodge",
		meta = (AllowPrivateAccess = "true"))
	USoundBase* EvadeSound;

	// 애니메이션 몽타주가 블렌딩 아웃될 때 호출되는 델리게이트
	FOnMontageBlendingOutStarted OnMontageBlendingOutStarted;
	// 애니메이션 몽타주가 블렌딩 인이 끝났을 때 호출되는 델리게이트
	FOnMontageBlendedInEnded OnMontageBlendingInEnded;

	// 회피 애니메이션 종료 시 처리 함수
	void OnEvadeMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void OnEvadeMontageBlendedInEnded(UAnimMontage* Montage);

	// 회피 방향 초기화를 위한 타이머 핸들
	FTimerHandle ResetEvadeDirectionTimerHandle;

	// 회피 방향 초기화까지의 시간 간격
	float ResetEvadeDirectionTimeRate;

	// 회피 방향을 리셋하는 함수
	void ResetEvadeDirection();

public:

	// 플레이어의 이동 입력 벡터를 받아 회피 방향 설정에 사용
	void SendMovementVector(FVector2D MovementVector);

	// 회피 동작 실행 : 플레이어 캐릭터를 인자로 받아 회피 처리 수행
	void Evade(ADoctor* Doctor);

	// 현재 회피 중인지 반환하는 함수
	FORCEINLINE bool GetIsEvading() const {
		return bIsEvading;
	}

	// 회피 할 방향을 셋팅하는 함수 Doctor 클래스에서 플레이어의 기본 이동 방향인(Derection)  Value 값을 인자값으로 받아온다.
	// 입력 벡터를 기반으로 회피 방향 설정
	void SetEvadeDirection(FVector2D MovementVector);


};
