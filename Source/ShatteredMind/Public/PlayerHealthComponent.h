#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerHealthComponent.generated.h"

class ADoctor;

// HP 바 갱신 델리게이트 (파라미터: 새 체력값)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealth);

// 사망 브로드캐스트 (UI / GameMode / 캐릭터 등 바인딩 가능)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);

// === 새로 추가 ===
// 피격 브로드캐스트 (한 대 맞을 때마다 알려주기, 피격 연출용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDamaged);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SHATTEREDMIND_API UPlayerHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPlayerHealthComponent();

protected:
    virtual void BeginPlay() override;

public:
    // 최대 체력
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float MaxHealth = 5.f;

    // 현재 체력
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
    float CurrentHealth = 5.f;

    // 한 번 맞고 난 뒤 잠깐 무적 주는 쿨다운 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Hit")
    float HitCooldownTime = 0.3f;

    // 지금 막 맞아서 쿨다운 중인지
    bool bRecentlyHit = false;

    // 쿨다운 타이머
    FTimerHandle HitCooldownTimerHandle;

    // HP 변경 이벤트 (HUD 등)
    UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnHealthChanged OnHealthChanged;

    // 사망 이벤트 (캐릭터/게임모드가 바인딩해서 처리)
    UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnPlayerDied OnPlayerDied;

    // === 새로 추가 ===
    // "맞았다" 이벤트 (Doctor가 이걸 받아서 화면에 피격 이펙트 뿌림)
    UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnPlayerDamaged OnPlayerDamaged;

public:
    // EnemyFSM 등이 이걸 호출해서 데미지 1 주는 식으로 사용
    UFUNCTION(BlueprintCallable, Category = "Health")
    void ApplyDamage(float Damage);

    UFUNCTION(BlueprintCallable, Category = "Health")
    bool IsDead() const { return CurrentHealth <= 0.f; }

    UFUNCTION(BlueprintCallable, Category = "Health")
    float GetHealthRatio() const
    {
        return (MaxHealth > 0.f) ? (CurrentHealth / MaxHealth) : 0.f;
    }

protected:
    UFUNCTION()
    void ResetHitCooldown();
};
