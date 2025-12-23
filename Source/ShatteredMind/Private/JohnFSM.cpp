#include "JohnFSM.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"

UJohnFSM::UJohnFSM()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UJohnFSM::BeginPlay()
{
    Super::BeginPlay();

    OwnerChar = Cast<ACharacter>(GetOwner());
    if (OwnerChar)
    {
        SpawnLocation = OwnerChar->GetActorLocation();

        // 자연스러운 걷기 설정
        auto Move = OwnerChar->GetCharacterMovement();
        Move->MaxWalkSpeed = WalkSpeed;
        Move->bOrientRotationToMovement = true;
        OwnerChar->bUseControllerRotationYaw = false;
    }

    mState = EJohnState::Rest;
    RestTimer = 0.f;
    MoveTimer = 0.f;
    MoveCount = 0;
    bReturning = false;
}



void UJohnFSM::RestState(float DeltaTime)
{
    RestTimer += DeltaTime;
    if (RestTimer >= RestDuration)
    {
        mState = EJohnState::Patrol;
        RestTimer = 0.f;
        MoveCount = 0;
        bReturning = false;
        PickNextTarget();
    }
}

void UJohnFSM::PatrolState(float DeltaTime)
{
    if (!OwnerChar) return;

    const FVector MyLoc = OwnerChar->GetActorLocation();
    const float Dist2D = FVector::Dist2D(MyLoc, TargetLocation);

    // 이동 타이머 감소
    if (MoveTimer > 0.f)
        MoveTimer -= DeltaTime;

    // 느슨한 도착 판정: 가까워졌거나 이동시간 초과 시 다음 목적지
    if (Dist2D <= AcceptanceRadius || MoveTimer <= 0.f)
    {
        if (!bReturning)
        {
            MoveCount++;
            if (MoveCount >= MoveCountToReturn)
            {
                bReturning = true;
                TargetLocation = SpawnLocation;
                ProjectToNav(TargetLocation, TargetLocation);
            }
            else
            {
                PickNextTarget();
            }
        }
        else
        {
            // 복귀 완료
            const float ReturnDist = FVector::Dist2D(MyLoc, SpawnLocation);
            if (ReturnDist <= AcceptanceRadius)
            {
                mState = EJohnState::Rest;
                RestTimer = 0.f;
                return;
            }
            else
            {
                TargetLocation = SpawnLocation;
                ProjectToNav(TargetLocation, TargetLocation);
            }
        }
    }

    // 자연스럽게 이동
    FVector Dir = (TargetLocation - MyLoc);
    Dir.Z = 0.f;
    if (Dir.Normalize())
    {
        OwnerChar->AddMovementInput(Dir, 1.f);
    }
}

void UJohnFSM::PickNextTarget()
{
    if (!OwnerChar) return;

    if (bReturning)
    {
        TargetLocation = SpawnLocation;
        ProjectToNav(TargetLocation, TargetLocation);
        return;
    }

    const FVector MyLoc = OwnerChar->GetActorLocation();

    // 랜덤 방향 + 랜덤 거리
    const float Distance = FMath::FRandRange(MoveMinDistance, MoveMaxDistance);
    const float Angle = FMath::FRandRange(0.f, 360.f);
    const FVector Dir = FVector(FMath::Cos(FMath::DegreesToRadians(Angle)), FMath::Sin(FMath::DegreesToRadians(Angle)), 0.f);
    FVector Dest = MyLoc + Dir * Distance;

    ProjectToNav(Dest, Dest);
    TargetLocation = Dest;

    // 이동 시간 랜덤 설정
    MoveTimer = FMath::FRandRange(MoveMinTime, MoveMaxTime);
}

bool UJohnFSM::ProjectToNav(const FVector& In, FVector& Out) const
{
    if (const UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation Hit;
        if (Nav->ProjectPointToNavigation(In, Hit, FVector(200.f, 200.f, 300.f)))
        {
            Out = Hit.Location;
            return true;
        }
    }
    Out = In;
    return false;
}

void UJohnFSM::SetPaused(bool bInPaused)
{
    bPaused = bInPaused;

    if (!OwnerChar) return;

    if (auto Move = OwnerChar->GetCharacterMovement())
    {
        if (bPaused)
        {
            Move->StopMovementImmediately();
            Move->DisableMovement();              // 이동 완전 비활성
            // SetComponentTickEnabled(false);    // (선택) 틱 자체를 끌 거면 주석 해제
        }
        else
        {
            // 이동 재개
            Move->SetMovementMode(MOVE_Walking);
            Move->MaxWalkSpeed = bReturning ? ReturnWalkSpeed : WalkSpeed;
            // SetComponentTickEnabled(true);     // (선택) 위에서 끄면 여기서 다시 켜기
        }
    }

    if (auto AIC = Cast<AAIController>(OwnerChar->GetController()))
    {
        if (bPaused)
        {
            AIC->StopMovement();
        }
    }
}

void UJohnFSM::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bPaused) return; // 상호작용 중엔 완전 스킵

    switch (mState)
    {
    case EJohnState::Rest:   RestState(DeltaTime);   break;
    case EJohnState::Patrol: PatrolState(DeltaTime); break;
    }
}
