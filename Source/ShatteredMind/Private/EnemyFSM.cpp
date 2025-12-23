#include "EnemyFSM.h"
#include "Enemy.h"
#include "EnemyAnim.h"
#include "Door.h"
#include "Doctor.h"
#include "PlayerHealthComponent.h"

#include "AIController.h"
#include "AITypes.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/AudioComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Sound/SoundBase.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

// ========================
// 생성자
// ========================
UEnemyFSM::UEnemyFSM()
{
    PrimaryComponentTick.bCanEverTick = true;
    CurrentHP = MaxHP;
    mState = EEnemyState::Idle;
    PrevState = mState;

    LastSeenTime = -1000.f;
    KeepAlertSeconds = 1.2f;
    bPrevAlertVisual = false;
}

// ========================
// Hit VFX
// ========================
void UEnemyFSM::SpawnHitVFX()
{
    if (!me || !GetWorld() || !*HitVFXActorClass) return;

    USkeletalMeshComponent* Mesh = me->GetMesh();
    if (!Mesh) return;

    const bool bHasSocket = Mesh->DoesSocketExist(HitVFXSocketName);
    const FVector Loc = bHasSocket
        ? Mesh->GetSocketLocation(HitVFXSocketName)
        : (Mesh->GetComponentLocation() + FVector(0, 0, 60));
    const FRotator Rot = bHasSocket
        ? Mesh->GetSocketRotation(HitVFXSocketName)
        : Mesh->GetComponentRotation();

    FActorSpawnParameters P;
    P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AActor* FX = GetWorld()->SpawnActor<AActor>(HitVFXActorClass, Loc, Rot, P))
    {
        if (bAttachHitVFXToMesh && bHasSocket)
        {
            FX->AttachToComponent(Mesh, FAttachmentTransformRules::KeepWorldTransform, HitVFXSocketName);
        }

        if (HitVFXLifeSpan > 0.f)
        {
            FX->SetLifeSpan(HitVFXLifeSpan);
        }
    }
}

void UEnemyFSM::BeginPlay()
{
    Super::BeginPlay();

    me = Cast<AEnemy>(GetOwner());
    if (!me) return;

    anim = Cast<UEnemyAnim>(me->GetMesh() ? me->GetMesh()->GetAnimInstance() : nullptr);
    CurrentHP = FMath::Max(1, MaxHP);

    // 대상: "Playable" 태그 중 가장 가까운 액터
    {
        TArray<AActor*> Found;
        UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Playable"), Found);

        float best = TNumericLimits<float>::Max();
        for (AActor* a : Found)
        {
            if (!IsValid(a)) continue;
            const float d2 = FVector::DistSquared(me->GetActorLocation(), a->GetActorLocation());
            if (d2 < best)
            {
                best = d2;
                Target = a;
            }
        }
    }

    // AIController 캐시 + PathFollowing 델리게이트 바인딩
    ai = Cast<AAIController>(me->GetController());
    if (ai && ai->GetPathFollowingComponent())
    {
        ai->GetPathFollowingComponent()->OnRequestFinished.AddUObject(
            this, &UEnemyFSM::OnMoveCompleted);
    }

    // 초기 시야 각도
    bool bCrouch = false;
    if (ACharacter* P = Cast<ACharacter>(Target))
    {
        bCrouch = P->bIsCrouched;
    }
    CurrentViewAngle = bCrouch ? NormalViewAngle * (2.f / 3.f) : NormalViewAngle;

    // 사운드 로드
    if (FindSoundAsset.IsValid())     FindSound = FindSoundAsset.Get();
    else if (FindSoundAsset.ToSoftObjectPath().IsValid()) FindSound = FindSoundAsset.LoadSynchronous();

    if (HitSoundAsset.IsValid())      HitSound = HitSoundAsset.Get();
    else if (HitSoundAsset.ToSoftObjectPath().IsValid())  HitSound = HitSoundAsset.LoadSynchronous();

    if (MoveLoopSoundAsset.IsValid()) MoveLoopSound = MoveLoopSoundAsset.Get();
    else if (MoveLoopSoundAsset.ToSoftObjectPath().IsValid()) MoveLoopSound = MoveLoopSoundAsset.LoadSynchronous();

    if (me && MoveLoopSound)
    {
        MoveLoopAC = NewObject<UAudioComponent>(me, TEXT("EnemyMoveLoopAC"));
        if (MoveLoopAC)
        {
            MoveLoopAC->bAutoActivate = false;
            MoveLoopAC->bAllowSpatialization = true;
            MoveLoopAC->bIsUISound = false;
            MoveLoopAC->SetSound(MoveLoopSound);

            if (GetWorld())
            {
                GetWorld()->GetTimerManager().SetTimer(
                    MoveLoopInitTimerHandle,
                    this, &UEnemyFSM::DelayedInitMoveLoopAC,
                    0.0f, false
                );
            }
        }
    }

    mState = EEnemyState::Idle;
    PrevState = mState;
    bEnteredDieState = true;
    DeathElapsed = 0.f;

    AggroHold = 0.f;
    AggroOnDamageHold = 0.f;
    HearingHold = 0.f;

    LastSeenTime = -1000.f;
    bPrevAlertVisual = false;

    // 라이트/시야 지연 세팅(메시/소켓 붙은 후)
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            SafeInitTimerHandle,
            this, &UEnemyFSM::DelayedSafeInit,
            0.05f, false
        );
    }
}

void UEnemyFSM::DelayedInitMoveLoopAC()
{
    if (!me || !MoveLoopAC) return;

    if (USceneComponent* RootComp = me->GetRootComponent())
    {
        MoveLoopAC->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
    }

    if (!MoveLoopAC->IsRegistered())
    {
        MoveLoopAC->RegisterComponent();
    }
}

void UEnemyFSM::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 델리게이트 해제
    if (ai && ai->GetPathFollowingComponent())
    {
        ai->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(SafeInitTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(MoveLoopInitTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void UEnemyFSM::DelayedSafeInit()
{
    if (!me) return;
    me->ApplySightToLight(detectRange, CurrentViewAngle);
    UpdateSightLightColorByState();
}

// 시야 라인 트레이스
bool UEnemyFSM::HasClearSight(UWorld* World, const AActor* FromActor, const AActor* ToActor,
    ECollisionChannel Channel, const FName& EyeSocket)
{
    if (!World || !FromActor || !ToActor) return false;

    const USkeletalMeshComponent* FromMesh =
        FromActor->FindComponentByClass<USkeletalMeshComponent>();

    const FVector Eye =
        (FromMesh && FromMesh->DoesSocketExist(EyeSocket))
        ? FromMesh->GetSocketLocation(EyeSocket)
        : FromActor->GetActorLocation() + FVector(0, 0, 60);

    const UCapsuleComponent* Cap = ToActor->FindComponentByClass<UCapsuleComponent>();
    const FVector Aim = Cap
        ? (ToActor->GetActorLocation() + FVector(0, 0, Cap->GetScaledCapsuleHalfHeight() * 0.6f))
        : (ToActor->GetActorLocation() + FVector(0, 0, 50));

    FHitResult Hit;
    FCollisionQueryParams P(SCENE_QUERY_STAT(SightLoS), false, FromActor);
    const bool bBlocked = World->LineTraceSingleByChannel(Hit, Eye, Aim, Channel, P);

    return (!bBlocked || Hit.GetActor() == ToActor);
}

bool UEnemyFSM::IsAlert() const
{
    const float Now = GetWorld()->GetTimeSeconds();
    const bool bForceAlert = (Now - LastSeenTime) <= KeepAlertSeconds;

    if (bForceAlert) return true;

    if (mState == EEnemyState::Find ||
        mState == EEnemyState::Attack ||
        mState == EEnemyState::Damage ||
        mState == EEnemyState::Die)
    {
        return true;
    }

    if (AggroHold > 0.f || HearingHold > 0.f || AggroOnDamageHold > 0.f)
    {
        return true;
    }
    return false;
}

void UEnemyFSM::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
    if (!ai && me) ai = Cast<AAIController>(me->GetController());

    // 웅크림 시 시야 약화
    bool bPlayerCrouched = false;
    if (ACharacter* Player = Cast<ACharacter>(Target))
    {
        bPlayerCrouched = Player->bIsCrouched;
    }

    CurrentViewAngle = bPlayerCrouched ? NormalViewAngle * (2.f / 3.f) : NormalViewAngle;
    float EffectiveDetectRange = bPlayerCrouched ? detectRange * 0.8f : detectRange;

    if (AggroHold > 0.f)
    {
        CurrentViewAngle = NormalViewAngle;
        EffectiveDetectRange = detectRange;
    }

    // 시야 라이트 적용 (거리/각도만, 색/밝기 X)
    if (me)
    {
        me->ApplySightToLight(EffectiveDetectRange, CurrentViewAngle);
        bLastIsCrouched = bPlayerCrouched;
    }

    // 시야 포착 판정
    bool bInDetect = false;
    if (me && Target && IsValid(Target))
    {
        const FVector ToTgt = Target->GetActorLocation() - me->GetActorLocation();
        const float Dist = ToTgt.Size();
        const float CosHalf = FMath::Cos(FMath::DegreesToRadians(CurrentViewAngle * 0.5f));
        const float DotVal = FVector::DotProduct(me->GetActorForwardVector(), ToTgt.GetSafeNormal());

        bInDetect = (Dist <= EffectiveDetectRange) && (DotVal >= CosHalf);

        if (bInDetect)
        {
            if (!HasClearSight(GetWorld(), me, Target, SightTraceChannel, EyeSocketName))
            {
                const float NearGraceDist = 160.f;
                bInDetect = (Dist <= NearGraceDist);
            }
        }
    }

    // Idle/Move 상태에서 새롭게 시야 포착 → Find 진입
    if (!bWasInDetect && bInDetect && (mState == EEnemyState::Idle || mState == EEnemyState::Move))
    {
        if (ai)
        {
            ai->StopMovement();
            ai->ClearFocus(EAIFocusPriority::Gameplay);
            InvalidateMoveCache(); // 정지 직후 캐시 무효화
        }
        bHasPatrolDest = false;

        mState = EEnemyState::Find;
        findElapsed = 0.f;
        bFindMontageStarted = false;
        if (anim) anim->animState = mState;

        AggroHold = FMath::Max(AggroHold, AggroHoldDuration);

        // 발견 즉시 빨강 유지 타이머 갱신
        LastSeenTime = GetWorld()->GetTimeSeconds();
        UpdateSightLightColorByState();

        const float Now = GetWorld()->GetTimeSeconds();
        if (FindSound && Now - LastFindSfxTime > 0.2f)
        {
            UGameplayStatics::PlaySoundAtLocation(this, FindSound, me->GetActorLocation());
            LastFindSfxTime = Now;
        }
    }

    // === 타겟 감지 유지 시스템 ===
    const float NowTime = GetWorld()->GetTimeSeconds();
    if (bInDetect)
    {
        LastSeenTime = NowTime; // 계속 갱신 → 끊겨도 KeepAlertSeconds 동안 빨강 유지
    }
    bWasInDetect = bInDetect;

    // 타이머 감소
    if (AggroHold > 0.f)         AggroHold = FMath::Max(0.f, AggroHold - DeltaTime);
    if (HearingHold > 0.f)       HearingHold = FMath::Max(0.f, HearingHold - DeltaTime);
    if (AggroOnDamageHold > 0.f) AggroOnDamageHold = FMath::Max(0.f, AggroOnDamageHold - DeltaTime);

    // 상태 처리
    if (anim) anim->animState = mState;
    switch (mState)
    {
    case EEnemyState::Idle:    IdleState();    break;
    case EEnemyState::Find:    FindState();    break;
    case EEnemyState::Move:    MoveState();    break;
    case EEnemyState::Attack:  AttackState();  break;
    case EEnemyState::Damage:  DamageState();  break;
    case EEnemyState::Die:     DieState();     break;
    }

    // 발소리 루프
    {
        const bool bShouldPlayFootstep =
            (mState == EEnemyState::Move) &&
            me &&
            (me->GetVelocity().Size2D() > 20.f);

        if (bShouldPlayFootstep) StartMoveLoop();
        else                     StopMoveLoop(0.03f);
    }

    // 시각 효과는 "변화가 있을 때만" 적용 (스팸/깜빡임 방지)
    const bool bNowAlert = IsAlert();
    if (bPrevAlertVisual != bNowAlert)
    {
        UpdateSightLightColorByState(); // 내부에서 me->SetAlertVisual(IsAlert())
        bPrevAlertVisual = bNowAlert;
    }

    // 상태 변화 캐시
    if (PrevState != mState)
    {
        // 상태 바뀌는 순간에도 확인(한번 더 보수적으로)
        const bool bNowAlert2 = IsAlert();
        if (bPrevAlertVisual != bNowAlert2)
        {
            UpdateSightLightColorByState();
            bPrevAlertVisual = bNowAlert2;
        }
        PrevState = mState;
    }
}

void UEnemyFSM::FindState()
{
    if (!me) return;

    AActor* ChaseTarget = ResolveChaseTarget();
    if (!IsValid(ChaseTarget))
    {
        mState = EEnemyState::Idle;
        if (anim) anim->animState = mState;
        return;
    }

    if (ai) { ai->StopMovement(); InvalidateMoveCache(); }

    // 타겟 바라보기(짧게)
    const FVector toTgt = ChaseTarget->GetActorLocation() - me->GetActorLocation();
    if (!toTgt.IsNearlyZero())
    {
        const FRotator desired = toTgt.Rotation();
        const FRotator newRot = FMath::RInterpTo(me->GetActorRotation(), desired, GetWorld()->GetDeltaSeconds(), 6.f);
        me->SetActorRotation(newRot);
    }

    findElapsed += GetWorld()->GetDeltaSeconds();
    if (findElapsed >= findWindupTime)
    {
        AggroHold = FMath::Max(AggroHold, AggroHoldDuration);
        mState = EEnemyState::Move;
        if (anim) anim->animState = mState;
    }
}

void UEnemyFSM::IdleState()
{
    currentTime += GetWorld()->DeltaTimeSeconds;

    if (bEnablePatrol && currentTime >= idleDelayTime)
    {
        currentTime = 0.f;
        mState = EEnemyState::Move;
        if (anim) anim->animState = mState;
    }
}

void UEnemyFSM::MoveState()
{
    if (!me) return;
    if (!ai) ai = Cast<AAIController>(me->GetController());
    if (!ai) return;

    const float dt = GetWorld()->GetDeltaSeconds();

    // 애님 정리
    if (anim) anim->StopAllMontages(0.05f);

    // 이동 모드 보정
    if (auto* Move = me->GetCharacterMovement())
    {
        if (Move->MovementMode != MOVE_Walking)
        {
            Move->SetMovementMode(MOVE_Walking);
        }
        Move->MaxWalkSpeed = 300.f;
    }

    // ----- 문 통과 람다
    auto TryOpenDoorAndGoThrough = [&](float Ahead)->bool
        {
            const float LocalDoorUseDistance = 220.f;
            const float LocalDoorTraceRadius = 30.f;
            const float DoorHalfHeight = 60.f;

            const FVector MyLoc = me->GetActorLocation();
            const FVector Fwd = me->GetActorForwardVector();

            float StartOffset = 40.f;
            if (UCapsuleComponent* Cap = me->GetCapsuleComponent())
            {
                StartOffset = FMath::Max(StartOffset, Cap->GetScaledCapsuleRadius() * 0.6f);
            }

            const FVector Start = MyLoc + Fwd * StartOffset;
            const FVector End = MyLoc + Fwd * LocalDoorUseDistance;

            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(DoorUse), false, me);
            FCollisionObjectQueryParams ObjParams;
            ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
            ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);

            if (!GetWorld()->SweepSingleByObjectType(
                Hit, Start, End, FQuat::Identity, ObjParams,
                FCollisionShape::MakeCapsule(LocalDoorTraceRadius, DoorHalfHeight), Params))
            {
                return false;
            }

            ADoor* Door = Cast<ADoor>(Hit.GetActor());
            if (!Door) return false;

            if (!Door->IsOpen())
            {
                Door->OpenByAI(me);
            }

            const FVector DoorFwd = Door->GetActorForwardVector();
            const FVector ToMe = (MyLoc - Door->GetActorLocation()).GetSafeNormal();
            const float DotV = FVector::DotProduct(DoorFwd, ToMe);

            auto MakeThrough = [&](const FVector& Dir)->FVector
                {
                    const FVector ThroughDir = (DotV > 0.f) ? (-Dir) : (Dir);
                    return Door->GetActorLocation() + ThroughDir * Ahead;
                };

            const FVector CandidateA = MakeThrough(DoorFwd);
            const FVector CandidateB = MakeThrough(Door->GetActorRightVector());

            const float dA = FVector::Dist2D(MyLoc, CandidateA);
            const float dB = FVector::Dist2D(MyLoc, CandidateB);
            const FVector ThroughPt = (dA > dB ? CandidateA : CandidateB);

            ai->ClearFocus(EAIFocusPriority::Gameplay);
            IssueMoveToLocationIfChanged(ThroughPt, 60.f);
            return true;
        };

    // ----- 스턱 체크 람다
    auto UpdateStuckAndIsBlocked_Lambda = [&]()->bool
        {
            StuckSampleElapsed += dt;

            if (LastSamplePos.IsZero())
            {
                LastSamplePos = me->GetActorLocation();
            }

            if (StuckSampleElapsed >= StuckCheckInterval)
            {
                const FVector Now = me->GetActorLocation();
                const float Dist2D = FVector::Dist2D(Now, LastSamplePos);
                const float Speed = Dist2D / StuckSampleElapsed;

                if (Speed <= StuckSpeedThreshold) StuckAccum += StuckSampleElapsed;
                else                               StuckAccum = 0.f;

                LastSamplePos = Now;
                StuckSampleElapsed = 0.f;
            }
            return (StuckAccum >= StuckTimeToRecover);
        };

    // ----- 멀리서 문 우회
    auto TryDetourViaBestDoorToward = [&](const FVector& From, const FVector& To, float Ahead)->bool
        {
            TArray<AActor*> Doors;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADoor::StaticClass(), Doors);
            if (Doors.Num() == 0) return false;

            const FVector Dir = (To - From).GetSafeNormal();

            ADoor* Best = nullptr;
            float BestScore = -FLT_MAX;

            for (AActor* A : Doors)
            {
                ADoor* D = Cast<ADoor>(A);
                if (!D) continue;

                const FVector P = D->GetActorLocation();
                const float   Dist = FVector::Dist2D(From, P);
                if (Dist > DoorSearchRadius) continue;

                const FVector ToDoor = (P - From).GetSafeNormal();
                const float   Facing = FVector::DotProduct(Dir, ToDoor);
                const float   PlayerDist = FVector::Dist2D(To, P);

                float Score = Facing * 1000.f - Dist * 0.3f - PlayerDist * 0.1f;
                if (Score > BestScore)
                {
                    BestScore = Score;
                    Best = D;
                }
            }

            if (!Best) return false;
            return GoToDoorFrontThenThrough(Best, Ahead);
        };

    // ========== 1) Aggressor 추격 ==========
    if (bChaseAttackerOnDamage && AggroOnDamageHold > 0.f && IsValid(LastAggressor))
    {
        if (UpdateStuckAndIsBlocked_Lambda())
        {
            if (TryDetourViaBestDoorToward(me->GetActorLocation(), LastAggressor->GetActorLocation(), 180.f))
                return;
        }

        if (TryOpenDoorAndGoThrough(160.f)) return;

        const FVector MyLoc = me->GetActorLocation();
        const FVector TgtLoc = LastAggressor->GetActorLocation();
        const float   Dist3D = FVector::Dist(MyLoc, TgtLoc);

        float SurfDist = Dist3D;
        if (UCapsuleComponent* MyCap = me->GetCapsuleComponent())
        {
            if (UCapsuleComponent* TgtCap = Cast<UCapsuleComponent>(LastAggressor->FindComponentByClass<UCapsuleComponent>()))
            {
                const float SumRadius = MyCap->GetScaledCapsuleRadius() + TgtCap->GetScaledCapsuleRadius();
                SurfDist = FMath::Max(0.f, Dist3D - SumRadius);
            }
        }

        const float EnterRange = FMath::Max(10.f, AttackEnterDist);
        if (SurfDist <= EnterRange)
        {
            if (ai) { ai->StopMovement(); InvalidateMoveCache(); }
            mState = EEnemyState::Attack;
            if (anim) anim->animState = mState;
            PlayAttackMontageImmediate();
            StuckAccum = 0.f;
            return;
        }

        ai->SetFocus(LastAggressor);
        const float Accept = FMath::Clamp(DamageChaseAcceptanceRadius, 80.f, AttackEnterDist - 10.f);
        IssueMoveToActorIfChanged(LastAggressor, Accept);
        return;
    }

    // ========== 2) 시야 추격 ==========
    AActor* ChaseTarget = ResolveChaseTarget();
    if ((bWasInDetect || AggroHold > 0.f) && IsValid(ChaseTarget))
    {
        if (UpdateStuckAndIsBlocked_Lambda())
        {
            if (TryDetourViaBestDoorToward(me->GetActorLocation(), ChaseTarget->GetActorLocation(), 180.f))
                return;
        }

        if (TryOpenDoorAndGoThrough(160.f)) return;

        const FVector MyLoc = me->GetActorLocation();
        const FVector TgtLoc = ChaseTarget->GetActorLocation();
        const float   Dist3D = FVector::Dist(MyLoc, TgtLoc);

        float SurfDist = Dist3D;
        if (UCapsuleComponent* MyCap = me->GetCapsuleComponent())
        {
            if (UCapsuleComponent* TgtCap = Cast<UCapsuleComponent>(ChaseTarget->FindComponentByClass<UCapsuleComponent>()))
            {
                const float SumRadius = MyCap->GetScaledCapsuleRadius() + TgtCap->GetScaledCapsuleRadius();
                SurfDist = FMath::Max(0.f, Dist3D - SumRadius);
            }
        }

        const float EnterRange = FMath::Max(10.f, AttackEnterDist);
        if (SurfDist <= EnterRange)
        {
            if (ai) { ai->StopMovement(); InvalidateMoveCache(); }
            mState = EEnemyState::Attack;
            if (anim) anim->animState = mState;
            PlayAttackMontageImmediate();
            StuckAccum = 0.f;
            return;
        }

        float DesiredAcc = FMath::Clamp(AttackEnterDist * 0.9f, 60.f, AttackEnterDist - 10.f);
        ai->SetFocus(ChaseTarget);
        IssueMoveToActorIfChanged(ChaseTarget, DesiredAcc);
        return;
    }

    // ========== 3) 청각 수색 ==========
    if (bEnableHearing && HearingHold > 0.f)
    {
        if (UpdateStuckAndIsBlocked_Lambda())
        {
            if (TryDetourViaBestDoorToward(me->GetActorLocation(), LastHeardLocation, 180.f))
                return;
        }

        if (TryOpenDoorAndGoThrough(140.f)) return;

        const float Dist2D = FVector::Dist2D(me->GetActorLocation(), LastHeardLocation);
        if (Dist2D <= HearingAcceptanceRadius)
        {
            HearingHold = 0.f;
        }
        else
        {
            ai->ClearFocus(EAIFocusPriority::Gameplay);
            IssueMoveToLocationIfChanged(LastHeardLocation, HearingAcceptanceRadius);
            return;
        }
    }

    // ========== 4) 패트롤 ==========
    if (!bEnablePatrol)
    {
        if (ai)
        {
            ai->StopMovement();
            ai->ClearFocus(EAIFocusPriority::Gameplay);
            InvalidateMoveCache();
        }
        bHasPatrolDest = false;
        return;
    }

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSys) return;

    if (!bHasPatrolDest)
    {
        if (TryOpenDoorAndGoThrough(140.f)) return;

        FVector NewDest;
        if (GetRandomPostionInNavMesh(me->GetActorLocation(), PatrolRadius, NewDest))
        {
            PatrolDestination = NewDest;
            bHasPatrolDest = true;
            LastPathBuildTime = GetWorld()->GetTimeSeconds();

            ai->ClearFocus(EAIFocusPriority::Gameplay);
            IssueMoveToLocationIfChanged(PatrolDestination, stopDistance);
        }
        return;
    }

    // 목적지가 있는 상태 → 여기서 스턱 체크!!!
    if (UpdateStuckAndIsBlocked(GetWorld()->GetDeltaSeconds()))
    {
        bHasPatrolDest = false;
        ai->StopMovement();
        InvalidateMoveCache();
        return;
    }

    const float NowTime2 = GetWorld()->GetTimeSeconds();
    if (NowTime2 - LastPathBuildTime > RepathInterval)
    {
        const float DistToDest = FVector::Dist(me->GetActorLocation(), PatrolDestination);
        if (DistToDest < 120.f)
        {
            if (TryOpenDoorAndGoThrough(140.f)) return;

            FVector NewDest;
            if (GetRandomPostionInNavMesh(me->GetActorLocation(), PatrolRadius, NewDest))
            {
                PatrolDestination = NewDest;
                ai->ClearFocus(EAIFocusPriority::Gameplay);
                IssueMoveToLocationIfChanged(PatrolDestination, stopDistance);
            }
        }
        LastPathBuildTime = NowTime2;
    }
}

void UEnemyFSM::AttackState()
{
    if (!me || !anim) return;

    // 타깃 거리 보고 바로 Move 복귀 유도
    {
        AActor* ChaseTarget = ResolveChaseTarget();
        if (IsValid(ChaseTarget))
        {
            const float Dist3D = FVector::Dist(me->GetActorLocation(), ChaseTarget->GetActorLocation());
            float SurfDist = Dist3D;

            if (UCapsuleComponent* MyCap = me->GetCapsuleComponent())
            {
                if (UCapsuleComponent* TgtCap = Cast<UCapsuleComponent>(ChaseTarget->FindComponentByClass<UCapsuleComponent>()))
                {
                    const float SumRadius = MyCap->GetScaledCapsuleRadius() + TgtCap->GetScaledCapsuleRadius();
                    SurfDist = FMath::Max(0.f, Dist3D - SumRadius);
                }
            }

            if (SurfDist > AttackEnterDist + 5.f)
            {
                mState = EEnemyState::Move;
                if (anim) anim->animState = mState;
                InvalidateMoveCache();
                return;
            }
        }
    }

    if (ai) { ai->StopMovement(); InvalidateMoveCache(); }
    if (auto* Move = me->GetCharacterMovement())
    {
        Move->StopMovementImmediately();
    }

    // 타깃 바라보기
    {
        AActor* ChaseTarget2 = ResolveChaseTarget();
        if (IsValid(ChaseTarget2))
        {
            const FVector toTgt = ChaseTarget2->GetActorLocation() - me->GetActorLocation();
            if (!toTgt.IsNearlyZero())
            {
                const FRotator desired = toTgt.Rotation();
                const FRotator newRot = FMath::RInterpTo(me->GetActorRotation(), desired, GetWorld()->GetDeltaSeconds(), 10.f);
                me->SetActorRotation(newRot);
            }
        }
    }

    // 몽타주 공격
    if (anim->AttackMontage)
    {
        if (!anim->Montage_IsPlaying(anim->AttackMontage) && !bAttackCommitted)
        {
            anim->Montage_Play(anim->AttackMontage, 1.2f);

            bAttackCommitted = true;
            bDidDamageThisSwing = false;
            currentTime = 0.f;

            AggroHold = FMath::Max(AggroHold, PostAttackAggroHold);
            // 공격 시작도 전투 중 → 빨강 유지 타임 갱신
            LastSeenTime = GetWorld()->GetTimeSeconds();
            return;
        }

        if (anim->Montage_IsPlaying(anim->AttackMontage))
        {
            AggroHold = FMath::Max(AggroHold, 0.3f);

            if (!bDidDamageThisSwing)
            {
                AActor* Victim = ResolveChaseTarget();
                if (IsValid(Victim))
                {
                    const float Dist = FVector::Dist(me->GetActorLocation(), Victim->GetActorLocation());
                    if (Dist <= attackRange)
                    {
                        if (ADoctor* Doc = Cast<ADoctor>(Victim))
                        {
                            if (!Doc->bIsDead)
                            {
                                if (UPlayerHealthComponent* Health = Doc->FindComponentByClass<UPlayerHealthComponent>())
                                {
                                    Health->ApplyDamage(1.0f);
                                }
                                bDidDamageThisSwing = true;
                                AggroHold = FMath::Max(AggroHold, PostAttackAggroHold);
                                LastSeenTime = GetWorld()->GetTimeSeconds();
                            }
                        }
                    }
                }
            }
            return;
        }

        // 쿨타임
        currentTime += GetWorld()->GetDeltaSeconds();
        if (currentTime >= attackDelayTime)
        {
            bAttackCommitted = false;
            bDidDamageThisSwing = false;

            mState = EEnemyState::Move;
            if (anim) anim->animState = mState;
            AggroHold = FMath::Max(AggroHold, PostAttackAggroHold);
            InvalidateMoveCache();
        }
        return;
    }

    // (Fallback) 몽타주 없는 경우
    if (!bAttackCommitted)
    {
        anim->bAttackPlay = true;
        bAttackCommitted = true;
        bDidDamageThisSwing = false;
        currentTime = 0.f;
        AggroHold = FMath::Max(AggroHold, PostAttackAggroHold);
        LastSeenTime = GetWorld()->GetTimeSeconds();
        return;
    }

    currentTime += GetWorld()->GetDeltaSeconds();
    if (currentTime >= attackDelayTime)
    {
        anim->bAttackPlay = false;

        if (!bDidDamageThisSwing)
        {
            AActor* Victim = ResolveChaseTarget();
            if (IsValid(Victim))
            {
                const float Dist = FVector::Dist(me->GetActorLocation(), Victim->GetActorLocation());
                if (Dist <= attackRange)
                {
                    if (ADoctor* Doc = Cast<ADoctor>(Victim))
                    {
                        if (!Doc->bIsDead)
                        {
                            if (UPlayerHealthComponent* Health = Doc->FindComponentByClass<UPlayerHealthComponent>())
                            {
                                Health->ApplyDamage(1.0f);
                            }
                            AggroHold = FMath::Max(AggroHold, PostAttackAggroHold);
                            LastSeenTime = GetWorld()->GetTimeSeconds();
                        }
                    }
                }
            }
        }

        bDidDamageThisSwing = false;
        bAttackCommitted = false;

        mState = EEnemyState::Move;
        if (anim) anim->animState = mState;
        AggroHold = FMath::Max(AggroHold, PostAttackAggroHold);
        InvalidateMoveCache();
    }
}

void UEnemyFSM::DamageState()
{
    const float dt = GetWorld()->GetDeltaSeconds();
    currentTime += dt;

    if (currentTime > 0.4f)
    {
        currentTime = 0.f;

        const bool bStayCombat =
            (AggroHold > 0.f) ||
            (AggroOnDamageHold > 0.f) ||
            bWasInDetect;

        if (bStayCombat)
        {
            AActor* T = ResolveChaseTarget();
            if (IsValid(T))
            {
                const float Dist3D = FVector::Dist(me->GetActorLocation(), T->GetActorLocation());
                float SurfDist = Dist3D;
                if (UCapsuleComponent* MyCap = me->GetCapsuleComponent())
                {
                    if (UCapsuleComponent* TCap = T->FindComponentByClass<UCapsuleComponent>())
                    {
                        const float SumR = MyCap->GetScaledCapsuleRadius() + TCap->GetScaledCapsuleRadius();
                        SurfDist = FMath::Max(0.f, Dist3D - SumR);
                    }
                }

                if (SurfDist <= AttackEnterDist)
                {
                    if (ai) { ai->StopMovement(); InvalidateMoveCache(); }
                    if (anim) anim->StopAllMontages(0.05f);

                    bAttackCommitted = false;
                    mState = EEnemyState::Attack;
                    if (anim) anim->animState = mState;

                    PlayAttackMontageImmediate();
                    LastSeenTime = GetWorld()->GetTimeSeconds();
                    return;
                }

                if (ai)
                {
                    ai->SetFocus(T);
                    const float Acc = FMath::Clamp(AttackEnterDist * 0.9f, 60.f, AttackEnterDist - 10.f);
                    IssueMoveToActorIfChanged(T, Acc);
                }

                mState = EEnemyState::Move;
                if (anim) anim->animState = mState;
                return;
            }

            mState = EEnemyState::Move;
            if (anim) anim->animState = mState;
        }
        else
        {
            mState = EEnemyState::Idle;
            if (anim) anim->animState = mState;
        }
    }
}

void UEnemyFSM::DieState()
{
    if (!me) return;

    if (!anim)
    {
        anim = Cast<UEnemyAnim>(me->GetMesh() ? me->GetMesh()->GetAnimInstance() : nullptr);
        if (!anim) return;
    }

    if (bEnteredDieState)
    {
        bEnteredDieState = false;

        if (me->GetSightLight())
        {
            me->GetSightLight()->SetVisibility(false, true);
        }

        if (auto* Move = me->GetCharacterMovement())
        {
            Move->StopMovementImmediately();
        }

        if (UCapsuleComponent* Cap = me->GetCapsuleComponent())
        {
            Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
        if (USkeletalMeshComponent* Mesh = me->GetMesh())
        {
            Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

    const float dt = GetWorld()->GetDeltaSeconds();
    DeathElapsed += dt;

    const bool bDeathReady = (anim->bDieDone) || (DeathElapsed >= DeathForceDestroyTime);
    if (bDeathReady)
    {
        me->SetActorHiddenInGame(true);
        me->SetLifeSpan(0.1f);
    }
}

void UEnemyFSM::ReceiveDamage(int32 Amount /*=1*/)
{
    if (!me) return;

    const float Now = GetWorld()->GetTimeSeconds();

    if (HitSound && (Now - LastHitSoundTime) >= HitSoundCooldown)
    {
        UGameplayStatics::PlaySoundAtLocation(this, HitSound, me->GetActorLocation());
        LastHitSoundTime = Now;
    }

    if ((Now - LastHitVFXTime) >= HitVFXCooldown)
    {
        SpawnHitVFX();
        LastHitVFXTime = Now;
    }

    const int32 Clamped = FMath::Max(1, Amount);
    CurrentHP = FMath::Max(0, CurrentHP - Clamped);

    AggroHold = FMath::Max(AggroHold, AggroHoldDuration);
    AggroOnDamageHold = FMath::Max(AggroOnDamageHold, AggroOnDamageHoldDuration);

    // 타격을 받았다는 건 전투 상황 → 빨강 유지 타이머도 갱신
    LastSeenTime = Now;
    UpdateSightLightColorByState();

    if (CurrentHP > 0)
    {
        const bool bInCombat =
            (mState == EEnemyState::Find ||
                mState == EEnemyState::Move ||
                mState == EEnemyState::Attack);

        int32 index = FMath::RandRange(0, 1);
        FString section = FString::Printf(TEXT("Damage%d"), index);

        if (bInCombat)
        {
            if (mState == EEnemyState::Attack)
            {
                if (ai) { ai->StopMovement(); InvalidateMoveCache(); }
                bAttackCommitted = false;
                PlayAttackMontageImmediate();
                return;
            }
            else
            {
                if (anim) anim->StopAllMontages(0.0f);
                if (anim) anim->PlayDamageAnim(FName(*section));
                if (ai) { ai->StopMovement(); InvalidateMoveCache(); }
                return;
            }
        }

        mState = EEnemyState::Damage;
        if (anim) anim->animState = mState;
        if (anim)
        {
            anim->StopAllMontages(0.0f);
            anim->PlayDamageAnim(FName(*section));
        }
        if (ai) { ai->StopMovement(); InvalidateMoveCache(); }
        return;
    }

    mState = EEnemyState::Die;
    if (anim) anim->animState = mState;

    if (auto* Move = me->GetCharacterMovement())
    {
        Move->StopMovementImmediately();
    }
    if (ai)
    {
        ai->StopMovement();
        ai->UnPossess();
    }

    StopMoveLoop(0.0f);

    if (UCapsuleComponent* Cap = me->GetCapsuleComponent())
    {
        Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (USkeletalMeshComponent* Mesh = me->GetMesh())
    {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (anim)
    {
        anim->StopAllMontages(0.0f);
        anim->PlayDamageAnim(TEXT("Die"));
    }

    me->SetLifeSpan(1.0f);
}

void UEnemyFSM::OnDamageProcess()
{
    ReceiveDamage(1);
}

void UEnemyFSM::PlayAttackMontageImmediate()
{
    if (!anim) return;

    if (anim->AttackMontage)
    {
        if (!anim->Montage_IsPlaying(anim->AttackMontage))
        {
            anim->Montage_Play(anim->AttackMontage, 1.0f);

            bAttackCommitted = true;
            bDidDamageThisSwing = false;
            currentTime = 0.f;
            AggroHold = FMath::Max(AggroHold, PostAttackAggroHold);
            LastSeenTime = GetWorld()->GetTimeSeconds();
        }
        return;
    }

    // Fallback
    anim->bAttackPlay = true;
    bAttackCommitted = true;
    currentTime = 0.f;
    AggroHold = FMath::Max(AggroHold, PostAttackAggroHold);
    LastSeenTime = GetWorld()->GetTimeSeconds();
}

void UEnemyFSM::UpdateSightLightColorByState()
{
    if (!me) return;
    const bool bAlert = IsAlert();
    me->SetAlertVisual(bAlert);
}

bool UEnemyFSM::GetRandomPostionInNavMesh(FVector centerLocation, float radius, FVector& dest)
{
    if (UNavigationSystemV1* ns = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation loc;
        bool result = ns->GetRandomReachablePointInRadius(centerLocation, radius, loc);
        dest = loc.Location;
        return result;
    }
    return false;
}

void UEnemyFSM::ReportNoise(const FVector& NoiseLocation, float Loudness /*=1.f*/)
{
    if (!bEnableHearing || !me) return;

    float EffRange = FMath::Min(
        HearingRange * FMath::Max(0.1f, Loudness) * FMath::Max(0.1f, LoudnessScale),
        HearingMaxRange > 0.f ? HearingMaxRange : TNumericLimits<float>::Max());

    bool bHasLoS = true;
    {
        FHitResult Hit;
        FCollisionQueryParams P(SCENE_QUERY_STAT(HearingLoS), false, me);
        bHasLoS = !GetWorld()->LineTraceSingleByChannel(Hit, NoiseLocation, me->GetActorLocation(), ECC_Visibility, P)
            || (Hit.GetActor() == me);
    }
    if (!bHasLoS)
    {
        EffRange *= OccludedHearingScale;
    }

    if (UNavigationSystemV1* NS = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        UNavigationPath* Path = NS->FindPathToLocationSynchronously(GetWorld(), me->GetActorLocation(), NoiseLocation);
        if (Path && Path->IsValid() && Path->PathPoints.Num() >= 2)
        {
            float PathLen = 0.f;
            for (int32 i = 1; i < Path->PathPoints.Num(); ++i)
            {
                PathLen += FVector::Dist(Path->PathPoints[i - 1], Path->PathPoints[i]);
            }

            const float Direct = FVector::Dist(me->GetActorLocation(), NoiseLocation);
            if (Direct > KINDA_SMALL_NUMBER && (PathLen / Direct) >= PathPenaltyThreshold)
            {
                EffRange *= PathPenaltyScale;
            }
        }
    }

    const float Dist2 = FVector::DistSquared(me->GetActorLocation(), NoiseLocation);
    if (Dist2 > FMath::Square(EffRange))
    {
        return;
    }

    const float BaseHold = HearingHoldDuration + FMath::Clamp(Loudness - 1.f, 0.f, 2.f) * 0.5f;
    float Hold = bHasLoS ? BaseHold : BaseHold * OccludedHearingScale;
    HearingHold = FMath::Max(HearingHold, Hold);

    LastHeardLocation = NoiseLocation;
    if (!bHasLoS)
    {
        const float Jitter = 80.f;
        LastHeardLocation += FVector(
            FMath::FRandRange(-Jitter, Jitter),
            FMath::FRandRange(-Jitter, Jitter),
            0.f
        );
    }

    if (mState == EEnemyState::Idle || mState == EEnemyState::Find)
    {
        mState = EEnemyState::Move;
        if (anim) anim->animState = mState;
    }

    if (!ai && me) ai = Cast<AAIController>(me->GetController());
    if (ai)
    {
        ai->ClearFocus(EAIFocusPriority::Gameplay);
        IssueMoveToLocationIfChanged(LastHeardLocation, HearingAcceptanceRadius);
    }

    // 소리로 전투 의심 → 빨강 유지 타임 약간만 보정 (선택)
    LastSeenTime = GetWorld()->GetTimeSeconds();
    UpdateSightLightColorByState();
}

void UEnemyFSM::ReportPerceivedDamage(AActor* InstigatorActor, const FVector& AtLocation, float /*DamageValue*/)
{
    if (!bChaseAttackerOnDamage) return;

    LastAggressor = InstigatorActor;
    LastKnownAggressorLoc = IsValid(InstigatorActor) ? InstigatorActor->GetActorLocation() : AtLocation;

    AggroOnDamageHold = FMath::Max(AggroOnDamageHold, AggroOnDamageHoldDuration);
    AggroHold = FMath::Max(AggroHold, 0.6f);

    // 피격 인지 → 즉시 빨강 유지 갱신
    LastSeenTime = GetWorld()->GetTimeSeconds();
    UpdateSightLightColorByState();

    if (mState == EEnemyState::Idle || mState == EEnemyState::Find)
    {
        mState = EEnemyState::Move;
        if (anim) anim->animState = mState;
    }
}

AActor* UEnemyFSM::ResolveChaseTarget() const
{
    if (bChaseAttackerOnDamage &&
        bPreferAggressorOverSight &&
        AggroOnDamageHold > 0.f &&
        IsValid(LastAggressor))
    {
        return LastAggressor;
    }
    return Target;
}

bool UEnemyFSM::UpdateStuckAndIsBlocked(float DeltaTime)
{
    if (!me) return false;

    StuckSampleElapsed += DeltaTime;
    if (LastSamplePos.IsZero())
    {
        LastSamplePos = me->GetActorLocation();
    }

    if (StuckSampleElapsed >= StuckCheckInterval)
    {
        const FVector Now = me->GetActorLocation();
        const float Dist = FVector::Dist2D(Now, LastSamplePos);
        const float Speed = Dist / StuckSampleElapsed;

        if (Speed <= StuckSpeedThreshold) StuckAccum += StuckSampleElapsed;
        else                               StuckAccum = 0.f;

        LastSamplePos = Now;
        StuckSampleElapsed = 0.f;
    }
    return (StuckAccum >= StuckTimeToRecover);
}

ADoor* UEnemyFSM::FindBestDoorToward(const FVector& From, const FVector& To) const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    TArray<AActor*> Doors;
    UGameplayStatics::GetAllActorsOfClass(World, ADoor::StaticClass(), Doors);
    if (Doors.Num() == 0) return nullptr;

    const FVector Dir = (To - From).GetSafeNormal();
    ADoor* Best = nullptr;
    float BestScore = -FLT_MAX;

    for (AActor* A : Doors)
    {
        ADoor* D = Cast<ADoor>(A);
        if (!D) continue;

        const FVector P = D->GetActorLocation();
        const float   Dist = FVector::Dist2D(From, P);
        if (Dist > DoorSearchRadius) continue;

        const FVector ToDoor = (P - From).GetSafeNormal();
        const float   Facing = FVector::DotProduct(Dir, ToDoor);
        const float   PlayerDist = FVector::Dist2D(To, P);

        float Score = Facing * 1000.f - Dist * 0.3f - PlayerDist * 0.1f;
        if (Score > BestScore)
        {
            BestScore = Score;
            Best = D;
        }
    }
    return Best;
}

bool UEnemyFSM::GoToDoorFrontThenThrough(ADoor* Door, float Ahead)
{
    if (!me || !Door || !ai) return false;

    const FVector MyLoc = me->GetActorLocation();
    const FVector DoorLoc = Door->GetActorLocation();
    const FVector DoorFwd = Door->GetActorForwardVector();

    const float Dot = FVector::DotProduct(DoorFwd, (MyLoc - DoorLoc).GetSafeNormal());
    const bool bFront = (Dot > 0.f);

    FVector Entry = DoorLoc + (bFront ? DoorFwd : -DoorFwd) * 90.f;

    if (UNavigationSystemV1* NS = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation Proj;
        if (NS->ProjectPointToNavigation(Entry, Proj, FVector(200, 200, 400)))
        {
            Entry = Proj.Location;
        }
    }

    const float EntryAccept = 70.f;
    if (FVector::Dist2D(MyLoc, Entry) > EntryAccept)
    {
        ai->ClearFocus(EAIFocusPriority::Gameplay);
        IssueMoveToLocationIfChanged(Entry, EntryAccept);
        return true;
    }

    if (!Door->IsOpen())
    {
        Door->OpenByAI(me);
    }

    FVector ThroughPt = DoorLoc + (bFront ? -DoorFwd : DoorFwd) * Ahead;
    if (UNavigationSystemV1* NS = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation Proj;
        if (NS->ProjectPointToNavigation(ThroughPt, Proj, FVector(300, 300, 400)))
        {
            ThroughPt = Proj.Location;
        }
    }

    ai->ClearFocus(EAIFocusPriority::Gameplay);
    IssueMoveToLocationIfChanged(ThroughPt, 60.f);
    return true;
}

void UEnemyFSM::StartMoveLoop()
{
    if (!MoveLoopAC || !MoveLoopSound) return;
    if (MoveLoopAC->IsPlaying()) return;

    MoveLoopAC->FadeIn(0.02f);
}

void UEnemyFSM::StopMoveLoop(float FadeTime)
{
    if (!MoveLoopAC) return;
    if (!MoveLoopAC->IsPlaying()) return;

    MoveLoopAC->FadeOut(FadeTime, 0.f);
}

// ========================
// MoveTo 스팸 방지
// ========================
bool UEnemyFSM::IssueMoveToActorIfChanged(AActor* Goal, float AcceptanceRadius)
{
    if (!ai || !IsValid(Goal)) return false;

    if (CachedGoalActor.Get() == Goal && FMath::IsNearlyEqual(CachedAccRadius, AcceptanceRadius, 0.1f))
    {
        return false;
    }

    CachedGoalActor = Goal;
    CachedAccRadius = AcceptanceRadius;
    CachedGoalLocation = FVector::ZeroVector;
    CachedAccRadius_Loc = -1.f;

    FAIMoveRequest Req;
    Req.SetGoalActor(Goal);
    Req.SetReachTestIncludesAgentRadius(false);
    Req.SetReachTestIncludesGoalRadius(false);
    Req.SetAcceptanceRadius(AcceptanceRadius);

    ai->MoveTo(Req);
    return true;
}

bool UEnemyFSM::IssueMoveToLocationIfChanged(const FVector& Goal, float AcceptanceRadius)
{
    if (!ai) return false;

    if (!CachedGoalLocation.IsNearlyZero() &&
        FVector::DistSquared2D(CachedGoalLocation, Goal) < FMath::Square(5.f) &&
        FMath::IsNearlyEqual(CachedAccRadius_Loc, AcceptanceRadius, 0.1f))
    {
        return false;
    }

    CachedGoalLocation = Goal;
    CachedAccRadius_Loc = AcceptanceRadius;
    CachedGoalActor = nullptr;
    CachedAccRadius = -1.f;

    ai->MoveToLocation(Goal, AcceptanceRadius, false);
    return true;
}

void UEnemyFSM::InvalidateMoveCache()
{
    CachedGoalActor = nullptr;
    CachedGoalLocation = FVector::ZeroVector;
    CachedAccRadius = -1.f;
    CachedAccRadius_Loc = -1.f;
}

// ========================
// PathFollowing 결과 콜백
// ========================
void UEnemyFSM::OnMoveCompleted(FAIRequestID /*RequestID*/, const FPathFollowingResult& /*Result*/)
{
    if (!me) me = Cast<AEnemy>(GetOwner());
    if (!me) return;

    // 전투/청각 추격 중이면 패스 (패트롤만 갱신)
    if (IsAlert() || HearingHold > 0.f) return;

    if (bEnablePatrol && (mState == EEnemyState::Move || mState == EEnemyState::Idle || mState == EEnemyState::Find))
    {
        // 다음 랜덤 포인트 발급 트리거
        bHasPatrolDest = false;
        currentTime = idleDelayTime; // 곧 MoveState -> 새로운 목적지
        if (ai) ai->StopMovement();
    }
}

