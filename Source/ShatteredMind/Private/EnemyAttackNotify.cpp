#include "EnemyAttackNotify.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "PlayerHealthComponent.h"

void UEnemyAttackNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;

    AActor* EnemyActor = MeshComp->GetOwner();
    if (!EnemyActor) return;

    const float AttackRange = 150.f;
    FVector Start = EnemyActor->GetActorLocation() + FVector(0, 0, 50.f); // 가슴 높이
    FVector Forward = EnemyActor->GetActorForwardVector();
    FVector End = Start + Forward * AttackRange;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(EnemyActor);
    Params.bTraceComplex = true;

    FCollisionObjectQueryParams ObjectQuery;
    ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

    bool bHit = EnemyActor->GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectQuery, Params);

#if WITH_EDITOR
    // 라인트레이스 시각화
    DrawDebugLine(EnemyActor->GetWorld(), Start, End, FColor::Red, false, 1.f, 0, 2.f);

    if (bHit)
    {
        DrawDebugSphere(EnemyActor->GetWorld(), Hit.Location, 15.f, 12, FColor::Green, false, 1.f);
        UE_LOG(LogTemp, Warning, TEXT("LINE TRACE HIT: %s at %s"), *Hit.GetActor()->GetName(), *Hit.Location.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("LINE TRACE MISS"));
    }
#endif

    // 플레이어 태그 체크 없이 HealthComponent로만 처리
    if (bHit && Hit.GetActor())
    {
        UPlayerHealthComponent* HealthComp = Hit.GetActor()->FindComponentByClass<UPlayerHealthComponent>();
        if (HealthComp)
        {
            UE_LOG(LogTemp, Warning, TEXT("Applying 10 damage to %s"), *Hit.GetActor()->GetName());
            HealthComp->ApplyDamage(10.f);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("PlayerHealthComponent not found on %s"), *Hit.GetActor()->GetName());
        }
    }
}
