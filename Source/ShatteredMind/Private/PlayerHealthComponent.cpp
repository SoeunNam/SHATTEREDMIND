#include "PlayerHealthComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include "Doctor.h"
#include "DoctorAnim.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DoctorGameModeBase.h"

UPlayerHealthComponent::UPlayerHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // 생성 시점 기본 체력 초기화
    CurrentHealth = MaxHealth;
}

void UPlayerHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    // 게임 시작 시점에도 한 번 더 초기화(에디터에서 MaxHealth 바꾼 경우 반영)
    CurrentHealth = MaxHealth;
    bRecentlyHit = false;
}

void UPlayerHealthComponent::ResetHitCooldown()
{
    bRecentlyHit = false;
}

// EnemyFSM 등에서 이걸 호출해준다
void UPlayerHealthComponent::ApplyDamage(float Damage)
{
    if (Damage <= 0.0f || IsDead())
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyDamage blocked. Damage=%f  IsDead=%s"),
            Damage,
            IsDead() ? TEXT("true") : TEXT("false"));
        return;
    }

    if (bRecentlyHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyDamage blocked due to recent hit cooldown"));
        return;
    }

    bRecentlyHit = true;
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            HitCooldownTimerHandle,
            this,
            &UPlayerHealthComponent::ResetHitCooldown,
            HitCooldownTime,
            false
        );
    }

    const float ActualDamage = 1.0f;

    ADoctor* Doctor = Cast<ADoctor>(GetOwner());

    if (Doctor)
    {
        if (UDoctorAnim* Anim = Cast<UDoctorAnim>(Doctor->GetMesh()->GetAnimInstance()))
        {
            const int32 index = FMath::RandRange(0, 1);
            const FString sectionName = FString::Printf(TEXT("Damege%d"), index);
            Anim->Play_Damege_Anim(FName(*sectionName));
        }

        FVector KnockbackDir = -Doctor->GetActorForwardVector();
        KnockbackDir.Z = 0.f;
        KnockbackDir.Normalize();

        const float KnockbackStrength = 400.f;
        Doctor->LaunchCharacter(KnockbackDir * KnockbackStrength, true, false);
    }

    const float OldHP = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

    UE_LOG(LogTemp, Warning, TEXT("HP %.1f -> %.1f"), OldHP, CurrentHealth);

    // HP바 같은 UI용
    OnHealthChanged.Broadcast(CurrentHealth);

    // === 여기 추가!!! ===
    // "플레이어가 피격당했다" 라고 브로드캐스트.
    // Doctor가 이걸 받아서 PlayHitEffect() 호출하게 돼.
    OnPlayerDamaged.Broadcast();

    // 죽었나?
    if (IsDead())
    {
        UE_LOG(LogTemp, Warning, TEXT("Player reached 0 HP, broadcasting OnPlayerDied"));
        OnPlayerDied.Broadcast();
    }
}
