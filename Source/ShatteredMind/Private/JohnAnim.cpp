#include "JohnAnim.h"
#include "GameFramework/Character.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/ActorComponent.h"

UJohnAnim::UJohnAnim()
{
    JohnState = EJohnState::Rest;
}

void UJohnAnim::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    APawn* PawnOwner = TryGetPawnOwner();
    if (ACharacter* C = Cast<ACharacter>(PawnOwner))
    {
        OwnerChar = C;

        if (UActorComponent* Comp = C->GetComponentByClass(UJohnFSM::StaticClass()))
        {
            FSM = Cast<UJohnFSM>(Comp);
        }
    }
}

void UJohnAnim::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!OwnerChar.IsValid())
    {
        // 잃어버렸다면 다시 획득 시도
        APawn* PawnOwner = TryGetPawnOwner();
        OwnerChar = Cast<ACharacter>(PawnOwner);
    }

    if (!FSM.IsValid() && OwnerChar.IsValid())
    {
        if (UActorComponent* Comp = OwnerChar->GetComponentByClass(UJohnFSM::StaticClass()))
        {
            FSM = Cast<UJohnFSM>(Comp);
        }
    }

    // 상태 동기화
    if (FSM.IsValid())
    {
        JohnState = FSM->mState;
    }
    else
    {
        JohnState = EJohnState::Rest; // 안전 기본값
    }

    // 속도 계산(블렌드스페이스용)
    if (OwnerChar.IsValid())
    {
        const FVector V = OwnerChar->GetVelocity();
        Speed = FVector(V.X, V.Y, 0.f).Size();
    }
    else
    {
        Speed = 0.f;
    }
}
