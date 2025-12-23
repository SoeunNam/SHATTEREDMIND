// Fill out your copyright notice in the Description page of Project Settings.


#include "DoctorAnim.h"
#include "Doctor.h"
#include <GameFramework/CharacterMovementComponent.h>
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"




void UDoctorAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	// 부모의 함수 호출 ( 부모의 함수인 NativeUpdateAnimation은
	// 중요한 기능들을 처리하고 있기 때문에 무시하면 안된다. )
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 플레이어의 이동 속도를 가져와 speed에 할당하고 싶다.
	// 1. 소유 폰 얻어 오기
	auto ownerPawn = TryGetPawnOwner();
	// 2. 플레이어로 캐스팅하기
	auto player = Cast<ADoctor>(ownerPawn);
	// 캐스팅이 성공했다면
	if (player)
	{
		// 3. 이동 속도가 필요
		FVector velocity = player->GetVelocity();
		// 4. 플레이어의 전방 벡터가 필요
		FVector forwardVector = player->GetActorForwardVector();
		// 5. Speed에 값(내적) 할당하기
		speed = FVector::DotProduct(forwardVector, velocity);
		// 6. 좌우 속도 할당하기
		FVector rightVector = player->GetActorRightVector();
		direction = FVector::DotProduct(rightVector, velocity);

		// 플레이어가 현재 공중에 있는지 여부를 기억하고 싶다.
		auto movement = player->GetCharacterMovement();
		isInAir = movement->IsFalling();
		// 플레이어가 앉아 있는 상태인지 함수를 통해 변수에게 전달 
		Sit = movement->IsCrouching();
		// 플레이어가 회피를 한 상태인지 함수를 통해 변수에게 전달
		//Roll = movement->IsCrouching();

		// 플레이어의사의 Yaw, Pitch 회전 값을 갱신해줌.
		// 플레이어 의사 클래스의 Tick() 함수가 수행된 이후에, 
		// 애니메이션 클래스의 NativeUpdateAnimation() 함수가 실행되는 순서이다.
		AO_Yaw = player->GetAO_Yaw();
		AO_Pitch = player->GetAO_Pitch();



	}
}
UAnimMontage* UDoctorAnim::Play_holding_a_pistol_Anim(FName SectionName)
{
	if (!holding_a_pistol_AnimMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("[DoctorAnim] ? holding_a_pistol_AnimMontage is NULL!"));
		return nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("[DoctorAnim] ▶ Play_holding_a_pistol_Anim (Section: %s)"), *SectionName.ToString());

	StopAllMontages(0.1f);

	const float PlayResult = Montage_Play(holding_a_pistol_AnimMontage);
	if (PlayResult <= 0.f)
	{
		UE_LOG(LogTemp, Error, TEXT("[DoctorAnim] ? Montage_Play failed"));
		return nullptr;
	}

	// 모든 섹션 이름 출력
	const int32 NumSections = holding_a_pistol_AnimMontage->GetNumSections();
	FString SectionDump;
	for (int32 i = 0; i < NumSections; ++i)
	{
		SectionDump += FString::Printf(TEXT("[%d]%s "), i, *holding_a_pistol_AnimMontage->GetSectionName(i).ToString());
	}
	UE_LOG(LogTemp, Warning, TEXT("[DoctorAnim] § Sections: %s"), *SectionDump);

	// 섹션 점프
	if (!SectionName.IsNone())
	{
		const int32 Index = holding_a_pistol_AnimMontage->GetSectionIndex(SectionName);
		if (Index != INDEX_NONE)
		{
			Montage_JumpToSection(SectionName, holding_a_pistol_AnimMontage);
			UE_LOG(LogTemp, Warning, TEXT("[DoctorAnim] ?? JumpToSection OK: %s (Index=%d)"), *SectionName.ToString(), Index);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[DoctorAnim] ? Section '%s' NOT FOUND!"), *SectionName.ToString());
		}
	}

	// 블렌드아웃/종료 추적
	FOnMontageBlendingOutStarted BlendOut;
	BlendOut.BindLambda([](UAnimMontage* M, bool bInterrupted)
		{
			UE_LOG(LogTemp, Warning, TEXT("[DoctorAnim] ? BlendOut: %s | Interrupted=%d"), *GetNameSafe(M), bInterrupted);
		});
	Montage_SetBlendingOutDelegate(BlendOut, holding_a_pistol_AnimMontage);

	FOnMontageEnded Ended;
	Ended.BindLambda([](UAnimMontage* M, bool bInterrupted)
		{
			UE_LOG(LogTemp, Warning, TEXT("[DoctorAnim] ? Ended: %s | Interrupted=%d"), *GetNameSafe(M), bInterrupted);
		});
	Montage_SetEndDelegate(Ended, holding_a_pistol_AnimMontage);

	return holding_a_pistol_AnimMontage;
}



void UDoctorAnim::AnimNotify_123()
{
	UE_LOG(LogTemp, Warning, TEXT("Notify_123"));

}

void UDoctorAnim::PlayhitAnim()
{
	Montage_Play(Pistol_hit_AnimMontage);

	UE_LOG(LogTemp, Warning, TEXT("Montaju_Play_Hit!!!!!!!!!!!!!!!!!!!"));
}

UAnimMontage* UDoctorAnim::Play_Hammer_in_hand_Anim(FName(SectionName))
{
	//// 망치 들기 , 내리기 애니메이션 몽타주 실행
	//Montage_Play(Hammer_in_hand_AnimMontage);
	//// 정해진 애니메이션 섹션으로 이동
	//Montage_JumpToSection(SectionName, Hammer_in_hand_AnimMontage);

	

	if (Hammer_in_hand_AnimMontage)
	{
		Montage_Play(Hammer_in_hand_AnimMontage);
		if (!SectionName.IsNone())
		{
			Montage_JumpToSection(SectionName, Hammer_in_hand_AnimMontage);
		}
		return Hammer_in_hand_AnimMontage; // ? 반드시 반환!
	}

	return nullptr;

}

UAnimMontage* UDoctorAnim::Play_Hammer_Attack_Anim(FName SectionName)
{
	if (Hammer_Attack_AnimMontage)
	{
		Montage_Play(Hammer_Attack_AnimMontage);

		if (!SectionName.IsNone())
		{
			Montage_JumpToSection(SectionName, Hammer_Attack_AnimMontage);
			UE_LOG(LogTemp, Warning, TEXT("[DoctorAnim] ▶ JumpToSection: %s"), *SectionName.ToString());
		}

		return Hammer_Attack_AnimMontage; // ? 반드시 반환
	}

	UE_LOG(LogTemp, Warning, TEXT("[DoctorAnim] ? Hammer_Attack_AnimMontage is NULL!"));
	return nullptr;
}


void UDoctorAnim::Play_Damege_Anim(FName(SectionName))
{
	// 망치 들기 , 내리기 애니메이션 몽타주 실행
	Montage_Play(Player_Damege_AnimMontage);
	// 정해진 애니메이션 섹션으로 이동
	Montage_JumpToSection(SectionName, Player_Damege_AnimMontage);

	UE_LOG(LogTemp, Warning, TEXT("Montaju_Play_Hammer_attack!!!!!!!!!!!!!!!!!!!"));
}

