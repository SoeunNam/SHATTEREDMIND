// Fill out your copyright notice in the Description page of Project Settings.


#include "NotifyState_HammerWeapon.h"

void UNotifyState_HammerWeapon::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		// 플레이어 의사의 클래스를 사용하기 위해 변수 타입 캐스트
		Player2_Doctor = Cast<ADoctor>(MeshComp->GetOwner());
		// 캐스트가 완료 되었다면
		if (Player2_Doctor)
		{
			// 근접무기 망치의 콜리전 박스를 오버랩 충돌 이벤트가 발생하도록 true으로 옵션 변경
			Player2_Doctor->HammerCollision->SetGenerateOverlapEvents(true);
			Player2_Doctor->ActivateHammerWeapon(); // 콜리전 옵션을 nocolison->only으로 변경

		}
		
		

	
	}
	

}

void UNotifyState_HammerWeapon::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	// 플레이어 의사의 클래스를 사용하기 위해 변수 타입 캐스트
	Player2_Doctor = Cast<ADoctor>(MeshComp->GetOwner());
	// 캐스트가 완료 되었다면
	if (Player2_Doctor)
	{
		// 근접무기 망치의 콜리전 박스의 충돌 이벤트가 발생하지 않도록 오버랩 이벤트 옵션을 꺼줌
		Player2_Doctor->HammerCollision->SetGenerateOverlapEvents(false);
		Player2_Doctor->DeactivateHammerWeapon(); // no Colision으로 변경
	}
	
	
}

