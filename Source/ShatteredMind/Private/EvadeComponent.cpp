// Fill out your copyright notice in the Description page of Project Settings.


#include "EvadeComponent.h"
#include "Doctor.h"
#include "Kismet/GameplayStatics.h"


// Sets default values for this component's properties
UEvadeComponent::UEvadeComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UEvadeComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

}


// Called every frame
void UEvadeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}



// 회피 애니메이션 종료 시 호출 됨
void UEvadeComponent::OnEvadeMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// 회피 동작을 수행 할 시 무기가 회피하는 동안 보이지 않게 하기 위한 함수

	// 조건 : 플레이어 의사 클래스가 nullptr 이라면 여기서 중단 되어 아래의 코드들을 실행하지 않고 바로 반환된다.
	if (!Doctor_Ref) return;

	// 회피 상태 종료
	bIsEvading = false;
	EvadeDirection = EEvadeDirection::None; // 회피 방향 초기화

	// Show weapon. 
	// player2_Doctor_Ref가 nullptr이 아니라면 무기가 다시 보이게 한다
	if (Doctor_Ref)
	{
		//메시의 3번 슬롯에 WeaponMaterial을 적용하여 무기를 다시 보이게 한다.
		Doctor_Ref->GetMesh()->SetMaterial(3, WeaponMaterial);
	}


}

void UEvadeComponent::OnEvadeMontageBlendedInEnded(UAnimMontage* Montage)
{
	// EvadeSound가 null이 아니라면 등록 된 회피 사운드를 실행한다.
	if (EvadeSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), EvadeSound);
	}
}

void UEvadeComponent::ResetEvadeDirection()
{
	// 회피 방향을 None = 0 으로 초기화 한다.
	EvadeDirection = EEvadeDirection::None;
}

void UEvadeComponent::SendMovementVector(FVector2D MovementVector)
{
	// 인자값으로 들어온 플레이어의 기본 방향(Value) 값을 회피 이동 방향으로 셋팅하여
	// Evade 컴포넌트가 회피 방향을 결정하여 실행 할 수 있도록 한다.
	SetEvadeDirection(MovementVector);

	// 실행 되고 있는 게임 월드 상에서 타이머 매니저를 이용하여 타이머를 잰다. 
	// 일정 시간 후 회피 방향을 초기화 하기 위한 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(ResetEvadeDirectionTimerHandle, this,
		&UEvadeComponent::ResetEvadeDirection, ResetEvadeDirectionTimeRate);

}

void UEvadeComponent::Evade(ADoctor* Doctor)
{
	// 유효한 플레이어 참조가 없다면 (Doctor이 nullptr 이라면)
	// 여기서 함수 동작을 종료하고 반환한다.
	if (!Doctor) return;

	// 회피 방향이 결정 되지 않았다면 여기서 함수 동작을 종료하고 반환한다.
	if (EvadeDirection == EEvadeDirection::None) return;

	// 회피 상태 활성화 
	bIsEvading = true;

	// Doctor_Ref 변수는  Doctor 클래스 형태으로 초기화 된다. ( 가리키는 주소값 )
	Doctor_Ref = Doctor;

	//Hide weapon.
	if (TransparentMaterial)
	{
		// 회피를 진행하는 동안에 3번 무기를 보이지 않게 한다.
		if (Doctor)
		{
			Doctor->GetMesh()->SetMaterial(3, TransparentMaterial);

		}
	}

	// nullptr이 아니라면 플레이어에게서 받은 회피 방향에 해당 되는 몽타주 애니메이션을 실행
	if (Doctor)
	{
		//Fatch related animation. // 플레이어의 애니메이션을 실행하기 위한 애니메이션 타입의 포인터 변수 초기화
		UAnimInstance* AnimInstance = Doctor->GetMesh()->GetAnimInstance();

		// 입력 받은 방향 Enum을 조건문으로 확인하고 int 형식에서 문자타입으로 변환한다. 
		switch (EvadeDirection)
		{
		case EEvadeDirection::None:
			EvadeSectionName = "";
			break;
		case EEvadeDirection::Evade_Forward:
			EvadeSectionName = "Evade_Forward";
			break;
		case EEvadeDirection::Evade_Left:
			EvadeSectionName = "Evade_Left";
			break;
		case EEvadeDirection::Evade_Right:
			EvadeSectionName = "Evade_Right";
			break;
		case EEvadeDirection::Evade_Backward:
			EvadeSectionName = "Evade_Backward";
			break;

		}

		// nullptr 이 아니라면 
		if (AnimInstance)
		{
			// 플레이어 의사의 회피 몽타주 애니메이션을 실행한다.
			AnimInstance->Montage_Play(EvadeMontage);

			// 변환한 문자열 형태의 플레이어의 이동 방향 Derection 이름으로 애니메이션 몽타주 섹션을 결정한다.
			AnimInstance->Montage_JumpToSection(EvadeSectionName); // Evade_Right 섹션까지 나머지 섹션을 건너뛰기 한다는 언리얼엔진의 자체 함수인듯

			// 몽타주 애니메이션 실행 되고 , 끝날 때 동적으로 실행할  델리게이트 함수
			OnMontageBlendingInEnded.BindUObject(this, &UEvadeComponent::OnEvadeMontageBlendedInEnded);
			OnMontageBlendingOutStarted.BindUObject(this, &UEvadeComponent::OnEvadeMontageBlendingOut);


			// Subscribe to the blending in delegate.
			AnimInstance->Montage_SetBlendedInDelegate(OnMontageBlendingInEnded, EvadeMontage);

			// Subscribe to the blending out delegate.
			AnimInstance->Montage_SetBlendingOutDelegate(OnMontageBlendingOutStarted, EvadeMontage);


		}
	}

}



void UEvadeComponent::SetEvadeDirection(FVector2D MovementVector)
{
	// 플레이어 에게서 이동 방향 Delection // Value 값을 인자로 받아서 
	// 이곳에서 회피 방향을 결정 짓도록 셋팅한다 . 
	if (MovementVector.X > 0)
	{
		EvadeDirection = EEvadeDirection::Evade_Forward;
	}
	else if (MovementVector.X < 0)
	{
		EvadeDirection = EEvadeDirection::Evade_Backward;
	}
	else if (MovementVector.Y < 0)
	{
		EvadeDirection = EEvadeDirection::Evade_Left;
	}
	else if (MovementVector.Y > 0)
	{
		EvadeDirection = EEvadeDirection::Evade_Right;
	}
	else
	{
		EvadeDirection = EEvadeDirection::None;
	}
}

