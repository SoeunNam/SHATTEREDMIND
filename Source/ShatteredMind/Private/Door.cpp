// Door.cpp
#include "Door.h"
#include "TimerManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ADoor::ADoor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Mesh
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);           // 회전 문
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));   // 기본은 막음
	Mesh->SetRenderCustomDepth(false);
	Mesh->SetCustomDepthStencilValue(1);
	//Mesh->SetRelativeScale3D(FVector(1.0f, 1.1f, 1.2f));


	// 기본 문 리소스
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TempMesh(
		TEXT("'/Game/Hospital/meshes/SM_Door_1_Door.SM_Door_1_Door'"));
	if (TempMesh.Succeeded())
	{
		Mesh->SetStaticMesh(TempMesh.Object);
	}

	bIsOpen = false;
}

void ADoor::BeginPlay()
{
	Super::BeginPlay();

	InteractableData = InstanceInteractableData;

	// 초기 회전
	ClosedRotation = GetActorRotation();
	TargetRotation = ClosedRotation;

	// 시작 상태 정렬
	if (bIsOpen)
	{
		if (NavMod) NavMod->SetAreaClass(nullptr);
		if (bDisablePawnBlockWhenOpen && Mesh)
			Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
	else
	{
		if (NavMod) NavMod->SetAreaClass(UNavArea_Null::StaticClass());
		if (Mesh) Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
}

void ADoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 부드러운 회전
	const FRotator Current = GetActorRotation();
	SetActorRotation(FMath::RInterpTo(Current, TargetRotation, DeltaTime, OpenSpeed));
}

void ADoor::BeginFocus()
{
	if (Mesh) Mesh->SetRenderCustomDepth(true);
}

void ADoor::EndFocus()
{
	if (Mesh) Mesh->SetRenderCustomDepth(false);
}

void ADoor::BeginInteract() {}
void ADoor::EndInteract() {}

void ADoor::Interact(APolice* /*_Playercharacter*/)
{
	// 플레이어는 토글 유지
	ToggleDoorByAI(this);
}

// ─────────────────────────────────────
// 플레이어용 토글 
// ─────────────────────────────────────
void ADoor::ToggleDoorByAI(AActor* Opener)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (bLocked || (Now - LastChangeTime) < ToggleCooldown) return;

	bIsOpen = !bIsOpen;
	LastChangeTime = Now;

	// 사운드
	if (bIsOpen && DoorOpenSoundCue)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DoorOpenSoundCue, GetActorLocation());
	}
	else if (!bIsOpen && DoorCloseSoundCue)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(
			TimerHandle,
			[this]()
			{
				UGameplayStatics::PlaySoundAtLocation(this, DoorCloseSoundCue, GetActorLocation());
			},
			1.0f, // 1초 뒤 실행
			false // 한 번만
		);
	}

	// 회전 목표
	TargetRotation = bIsOpen
		? (ClosedRotation + FRotator(0.f, OpenAngle, 0.f))
		: ClosedRotation;

	// 충돌 처리(플레이어/몬스터 분기 포함)
	if (bIsOpen)
	{
		ApplyCollisionForOpen(Opener);
	}
	else
	{
		// 닫힘: 확실히 막기
		if (Mesh)
		{
			// 고스트 타이머 클리어
			GetWorldTimerManager().ClearTimer(GhostTimerHandle);

			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
			Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
	}
}



// 문 반대편 좌표 계산
FVector ADoor::GetPassThroughPoint(AActor* Opener) const
{
	const FVector Origin = GetActorLocation();
	FVector Dir = GetActorForwardVector();

	if (Opener)
	{
		const FVector ToOpener = (Opener->GetActorLocation() - Origin).GetSafeNormal();
		if (FVector::DotProduct(ToOpener, Dir) > 0.f)
			Dir *= -1.f;
	}
	return Origin + Dir * PassThroughOffset;
}

void ADoor::DoctorInteract(ADoctor* _Playercharacter)
{
	bIsOpen = !bIsOpen;

	// 토글에 따라 목표 회전
	TargetRotation = bIsOpen ? ClosedRotation + FRotator(0, OpenAngle, 0) : ClosedRotation;

	// 플레이어 상호작용 → Opener를 this(문)로 처리(몬스터 고스트 미적용)
	if (bIsOpen)
	{
		ApplyCollisionForOpen(this);
	}
	else
	{
		// 닫힘: 확실히 막기
		if (Mesh)
		{
			GetWorldTimerManager().ClearTimer(GhostTimerHandle);

			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
			Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
	}

	// 사운드
	if (bIsOpen && DoorOpenSoundCue)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DoorOpenSoundCue, GetActorLocation());
	}
	else if (!bIsOpen && DoorCloseSoundCue)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(
			TimerHandle,
			[this]()
			{
				UGameplayStatics::PlaySoundAtLocation(this, DoorCloseSoundCue, GetActorLocation());
			},
			1.0f,
			false
		);
	}
}

void ADoor::ApplyCollisionForOpen(AActor* Opener)
{
	if (!Mesh) return;

	// 기본: 열림 시 Pawn만 통과(기존 동작 유지)
	if (bDisablePawnBlockWhenOpen)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

}

void ADoor::RestoreOpenCollision()
{
	// 문이 아직 열려있을 때만 복구
	if (!bIsOpen || !Mesh) return;

	// ‘열림 상태’ 기본 충돌(= Pawn만 Ignore)로 되돌림
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}
