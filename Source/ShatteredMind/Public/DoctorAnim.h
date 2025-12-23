// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "DoctorAnim.generated.h"

/**
 * 
 */
UCLASS()
class SHATTEREDMIND_API UDoctorAnim : public UAnimInstance
{
	GENERATED_BODY()




public:

	// ============ 0250926 희빈 머지 ===============================

	// 플레이어 이동 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = DoctorAnim)
	float speed = 0;

	// 플레이어 좌우 이동속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = DoctorAnim)
	float direction = 0;

	// 매 프레임 갱신되는 함수
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 플레이어가 공중에 있는지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = DoctorAnim)
	bool isInAir = false;

	// 재생할 권총 손에 집어들기 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = DoctorAnim)
	class UAnimMontage* holding_a_pistol_AnimMontage;
	// 권총 손에 집어들기 애니메이션 재생 함수
	UFUNCTION(BlueprintCallable)
	UAnimMontage* Play_holding_a_pistol_Anim(FName SectionName = NAME_None);

	



	// 노티 파이를 이용한 총의 위치를 변경하는 함수
	UFUNCTION()
	void AnimNotify_123();

	// 무기를 집어드는  노티파이가 실행중인지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Player2_Aniim)
	bool Weapon_on_off = false;


	// 권총 애니메이션 ( 상체 ) 실행 여부 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Player2_Aniim)
	int Pistol_on_off = 0;


	// 플레이어가 앉아있는 상태인지 확인하는 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = DoctorAnim)
	bool Sit = false;

	// 플레이어가 회피를 한 상태인지 확인하는 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = DoctorAnim)
	bool Roll = false;

	// 플레이어의 카메라 회전 값 ( 상하 )
	// 플레이어 좌우 이동속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = DoctorAnim)
	float Player2_Camera_Pitch;

	
	bool m_bIsDodging = false;

	//애니메이션 몽타주 노티파이 이벤트
	//UFUNCTION()
	//void HandleOnMontageNotifyBegin(FName a_nNotifyName, const FBranchingPointNotifyPayload& a_PBranchingPayload);




	// 에임 오프셋 애니메이션 진행(실행) 방향
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float AO_Yaw;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float AO_Pitch;


	// 플레이어의 권총 발사시 반동으로 실행 될 애니메이션 몽타주 / 마우스 좌클릭
	UPROPERTY(EditDefaultsOnly, Category = Player2_Anim)
	class UAnimMontage* Pistol_hit_AnimMontage;

	// 권총 발사시 반동 애니메이션 재생 함수
	void PlayhitAnim();
	
	// 권총을 든 채로 조준 하고 있는인지 확인할 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Player2_Aniim)
	bool Pistol_Zoom_Anim;

	// 플레이어의 권총 발사시 반동으로 실행 될 애니메이션 몽타주 / 마우스 좌클릭
	UPROPERTY(EditDefaultsOnly, Category = Player2_Anim)
	class UAnimMontage* Hammer_in_hand_AnimMontage;

	// 망치 근접무기 애니메이션을 실행시킬 함수
	UFUNCTION(BlueprintCallable)
	UAnimMontage* Play_Hammer_in_hand_Anim(FName SectionName);

	// 망치를 들고있는지 확인할 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Player2_Aniim)
	bool Hammer_Default_Anim;

	// 무기 애니메이션 권총 / 망치  실행 여부 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Player2_Aniim)
	int Pistol_or_Hammer = 0;

	// 재생할 공격 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = DoctorAnim)
	class UAnimMontage* Hammer_Attack_AnimMontage;
	// 공격 애니메이션 재생 함수
	UFUNCTION(BlueprintCallable)
	UAnimMontage* Play_Hammer_Attack_Anim(FName SectionName);
	

	// 데미지 / 적에게 맞을 때 애니메이션 동기화 
	// 재생할 공격 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = DoctorAnim)
	class UAnimMontage* Player_Damege_AnimMontage;
	// 공격 애니메이션 재생 함수
	void Play_Damege_Anim(FName(SectionName));





};
