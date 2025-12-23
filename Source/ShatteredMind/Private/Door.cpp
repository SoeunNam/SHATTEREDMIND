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

	// NavMesh에 영향 제외
	Mesh->SetCanEverAffectNavigation(false);
#if WITH_EDITORONLY_DATA
	Mesh->bFillCollisionUnderneathForNavmesh = false;
#endif

	// 기본 문 리소스(선택)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TempMesh(
		TEXT("'/Game/Hospital/meshes/SM_Door_1_Door.SM_Door_1_Door'"));
	if (TempMesh.Succeeded())
	{
		Mesh->SetStaticMesh(TempMesh.Object);
	}

	// NavModifier: 닫히면 막힘
	NavMod = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavMod"));
	NavMod->SetAreaClass(UNavArea_Null::StaticClass());
	NavMod->FailsafeExtent = FVector(60.f, 90.f, 120.f);

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
// 공용 토글 (플레이어용; AI는 OpenByAI/CloseByAI 권장)
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

	// NavMesh 영역 전환
	if (NavMod)
	{
		NavMod->SetAreaClass(bIsOpen ? nullptr : UNavArea_Null::StaticClass());
	}

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

// ─────────────────────────────────────
// ★ AI 전용: 한 방향 오픈
// ─────────────────────────────────────
void ADoor::OpenByAI(AActor* Opener)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (bLocked || bIsOpen || (Now - LastChangeTime) < ToggleCooldown) return;

	bIsOpen = true;
	LastChangeTime = Now;

	TargetRotation = ClosedRotation + FRotator(0.f, OpenAngle, 0.f);

	if (NavMod) NavMod->SetAreaClass(nullptr);

	ApplyCollisionForOpen(Opener);

	if (DoorOpenSoundCue)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DoorOpenSoundCue, GetActorLocation());
	}
}

// ─────────────────────────────────────
// ★ AI 전용: 한 방향 클로즈
// ─────────────────────────────────────
void ADoor::CloseByAI()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (bLocked || !bIsOpen || (Now - LastChangeTime) < ToggleCooldown) return;

	bIsOpen = false;
	LastChangeTime = Now;

	TargetRotation = ClosedRotation;

	// 사운드(딜레이)
	if (!bIsOpen && DoorCloseSoundCue)
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

	if (NavMod) NavMod->SetAreaClass(UNavArea_Null::StaticClass());

	// 확실히 막기
	if (Mesh)
	{
		// 고스트 타이머 클리어
		GetWorldTimerManager().ClearTimer(GhostTimerHandle);

		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
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

// ====== 몬스터 전용 보조 로직 ======

bool ADoor::IsMonster(const AActor* Opener) const
{
	// 태그 기반 판정(프로젝트에서 몬스터에 "Enemy" 또는 "Monster" 태그를 붙여 사용)
	if (!Opener) return false;
	if (Opener->ActorHasTag(FName(TEXT("Enemy"))))   return true;
	if (Opener->ActorHasTag(FName(TEXT("Monster")))) return true;

	// 필요 시 향후: IEnemyInterface, 특정 클래스 캐스트 등으로 확장 가능
	return false;
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

	// 몬스터가 여는 경우에는 짧은 시간 완전 NoCollision로 열어 ‘확실 패스’
	if (IsMonster(Opener) && GhostDuration > 0.f)
	{
		// 기존 타이머가 있다면 갱신
		GetWorldTimerManager().ClearTimer(GhostTimerHandle);

		// 완전 고스트
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// GhostDuration 후 “열림 상태 전용 충돌”로 복구
		GetWorldTimerManager().SetTimer(
			GhostTimerHandle,
			this,
			&ADoor::RestoreOpenCollision,
			GhostDuration,
			false
		);
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
