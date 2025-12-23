// John.cpp
#include "John.h"
#include "JohnFSM.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"

AJohn::AJohn()
{
	PrimaryActorTick.bCanEverTick = true;

	// 메시/애님은 BP에서 세팅 권장
	// 필요 시 C++ 로드 예시:
	// ConstructorHelpers::FObjectFinder<USkeletalMesh> TempMesh(TEXT("/Script/Engine.SkeletalMesh'/Game/John/Mesh_Character_John.Mesh_Character_John'"));
	// if (TempMesh.Succeeded()) {
	// 	GetMesh()->SetSkeletalMesh(TempMesh.Object);
	// 	GetMesh()->SetRelativeLocationAndRotation(FVector(0,0,-88.f), FRotator(0,-90.f,0));
	// }

	// FSM 컴포넌트 추가
	JohnFSM = CreateDefaultSubobject<UJohnFSM>(TEXT("JFSM"));

	//애니메이션 블루프린트 할당하기 
	ConstructorHelpers
		::FClassFinder<UAnimInstance> TempClass(TEXT("/Script/Engine.AnimBlueprint'/Game/Blueprints/ABP_John.ABP_John_C'"));
	if (TempClass.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(TempClass.Class);
	}
}

void AJohn::BeginPlay()
{
	Super::BeginPlay();
}

void AJohn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AJohn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
