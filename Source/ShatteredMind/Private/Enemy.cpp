#include "Enemy.h"
#include "EnemyFSM.h"
#include "Components/SpotLightComponent.h"
#include "Engine/EngineTypes.h" // ELightUnits
#include "AIController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemy::AEnemy()
{
    PrimaryActorTick.bCanEverTick = true;

    // FSM
    fsm = CreateDefaultSubobject<UEnemyFSM>(TEXT("FSM"));

    // SpotLight
    SightLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SightLight"));

    // 소켓 기본값 및 부착
    if (SightSocketName.IsNone())
    {
        SightSocketName = TEXT("SightSocket");
    }
    SightLight->SetupAttachment(GetMesh(), SightSocketName);

    // 라이트 컴포넌트 기본 세팅
    SightLight->SetMobility(EComponentMobility::Movable);
    SightLight->SetVisibility(true, true);
    SightLight->SetHiddenInGame(false);
    SightLight->bAffectsWorld = true;

    // 위치/기본 방향(소켓이 정확하면 지워도 됨)
    SightLight->SetRelativeLocation(FVector(30.f, 0.f, 120.f));
    SightLight->SetRelativeRotation(FRotator(-12.f, 0.f, 0.f));

    // 캐릭터 회전 정책
    bUseControllerRotationYaw = true;
    if (auto* Move = GetCharacterMovement())
    {
        Move->bOrientRotationToMovement = false;
        Move->bUseControllerDesiredRotation = true;
        Move->RotationRate = FRotator(0.f, 540.f, 0.f);
        Move->BrakingDecelerationWalking = 2048.f;
        Move->GroundFriction = 8.f;
        Move->MaxAcceleration = 2048.f;
        Move->MaxWalkSpeed = 500.f;
    }
}

void AEnemy::BeginPlay()
{
    Super::BeginPlay();

    // 테스트용(원하면 끄기)
    UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.LightMaxDrawDistanceScale 10"));

    // 거리/각도 반영
    ApplySightToLight(SightRange, SightAngleFull);
    ApplySightLightEnabled();

    // 시작은 미발견(Idle)로 가정하여 희미한/무색 상태
    SetAlertVisual(false);

    // 라이트가 문/벽 뒤에 있으면 시각적으로 숨기기(타이머로 저렴하게)
    if (bEnableSightLight && bHideSightLightWhenOccluded)
    {
        const float Jitter = FMath::FRandRange(0.f, SightLightOcclusionInterval); // 동기화 깜빡임 방지
        GetWorldTimerManager().SetTimer(
            SightLightOcclusionTimer, this, &AEnemy::UpdateSightLightOcclusion,
            SightLightOcclusionInterval, true, Jitter);
    }
}

void AEnemy::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // 에디터/BP 값 변경 반영
    ApplySightToLight(SightRange, SightAngleFull);
    ApplySightLightEnabled();

    // 에디터에서 미리 보기 목적: FSM이 없으면 Idle로
    const bool bAlertNow = (fsm && fsm->IsAlert());
    SetAlertVisual(bAlertNow);
}

void AEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemy::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (UEnemyFSM* FSM = FindComponentByClass<UEnemyFSM>())
    {
        FSM->ai = Cast<AAIController>(NewController);
    }
}

void AEnemy::ApplySightLightEnabled()
{
    if (!SightLight) return;

    const bool b = bEnableSightLight;

    SightLight->SetVisibility(b, true);
    SightLight->SetHiddenInGame(!b);
    SightLight->SetActive(b);
    SightLight->bAffectsWorld = b;
}

void AEnemy::ApplySightToLight(float NewRange, float NewAngleFull)
{
    if (!SightLight || !bEnableSightLight) return;

    // 내부 저장
    SightRange = NewRange;
    SightAngleFull = NewAngleFull;

    // 물리 감쇠 + 물리 단위(칸델라)
    SightLight->bUseInverseSquaredFalloff = true;
    SightLight->SetIntensityUnits(ELightUnits::Candelas);
    SightLight->SetCastShadows(true);
    SightLight->bCastVolumetricShadow = true;
    SightLight->VolumetricScatteringIntensity = 0.5f;
    SightLight->SetAffectTranslucentLighting(false);
    SightLight->IndirectLightingIntensity = 0.0f;

    // 거리/각도
    SightLight->SetAttenuationRadius(SightRange * 1.2f);
    const float HalfAngle = FMath::Clamp(SightAngleFull * 0.5f, 1.f, 89.f);
    SightLight->SetOuterConeAngle(HalfAngle);
    SightLight->SetInnerConeAngle(FMath::Clamp(HalfAngle - 5.f, 1.f, HalfAngle));


}

void AEnemy::AimSightAt(AActor* Target, float ExtraPitchDeg)
{
    if (!SightLight || !Target) return;

    SightLight->SetUsingAbsoluteRotation(true);

    const FVector From = SightLight->GetComponentLocation();
    const FVector To = Target->GetActorLocation();

    FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(From, To);
    LookAt.Pitch += ExtraPitchDeg;

    SightLight->SetWorldRotation(LookAt);
}

void AEnemy::SetAlertVisual(bool bAlert)
{
    if (!SightLight) return;

    // 상태별 밝기/색 적용 (sRGB 변환 켜서 보기 좋은 색 유지)
    if (bAlert)
    {
        SightLight->SetLightColor(SightColor_Alert, /*bSRGB=*/true);
        SightLight->SetIntensity(AlertIntensity);
        SightLight->SetHiddenInGame(false);
        SightLight->SetVisibility(true, true);
        SightLight->SetActive(true);
    }
    else
    {
        SightLight->SetLightColor(SightColor_Idle, /*bSRGB=*/true);
        SightLight->SetIntensity(IdleIntensity); // 거의 안 보이게
        SightLight->SetHiddenInGame(false);      // 완전 숨김 X (희미하게 보이게)
        SightLight->SetVisibility(true, true);
        SightLight->SetActive(true);
    }
}

void AEnemy::UpdateSightLightOcclusion()
{
    if (!SightLight || !bEnableSightLight || !bHideSightLightWhenOccluded) return;

    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PC) return;

    // 플레이어 시점(카메라) 위치
    FVector ViewLoc; FRotator ViewRot;
    PC->GetPlayerViewPoint(ViewLoc, ViewRot);

    // 라이트 → 플레이어 시점 라인트레이스
    const FVector From = SightLight->GetComponentLocation();

    FHitResult Hit;
    FCollisionQueryParams P(SCENE_QUERY_STAT(SightOcclusion), false, this);
    P.AddIgnoredActor(this);

    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
        Hit, From, ViewLoc, ECC_Visibility, P);

    // 문/벽 등으로 가려졌다면 시각적으로 숨김, 아니면 표시
    const bool bOccluded = bBlocked && Hit.GetActor() && (Hit.GetActor() != PC->GetPawn());

    // 완전 꺼버리면 “발견 시 빨간 전환”을 체크하기 어려우니
    // 숨길 땐 Hidden/Visibility만 조정하고, Intensity는 유지
    SightLight->SetHiddenInGame(bOccluded);
    SightLight->SetVisibility(!bOccluded, true);
    SightLight->SetActive(!bOccluded ? true : SightLight->IsActive());
}

