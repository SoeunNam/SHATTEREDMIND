#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyFSM.generated.h"

class AEnemy;
class UEnemyAnim;
class AAIController;
class ADoor;
class USoundBase;
class UAudioComponent;

struct FAIRequestID;
struct FPathFollowingResult;

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
    Idle,
    Find,
    Move,
    Attack,
    Damage,
    Die
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SHATTEREDMIND_API UEnemyFSM : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnemyFSM();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // 상태 처리
    void IdleState();
    void FindState();
    void MoveState();
    void AttackState();
    void DamageState();
    void DieState();

    // ------------ HP / 데미지
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Stat")
    int32 MaxHP = 3;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Stat")
    int32 CurrentHP = 3;

    UFUNCTION(BlueprintCallable)
    void ReceiveDamage(int32 Amount = 1);

    UFUNCTION(BlueprintCallable)
    void OnDamageProcess();

    // ------------ 시야/탐지
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Sight")
    float detectRange = 800.f;

    /** 전각(FOV)로 편집, 내부 계산 시 0.5x 사용 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Sight")
    float NormalViewAngle = 90.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Sight")
    FName EyeSocketName = "EyesSocket";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Sight")
    TEnumAsByte<ECollisionChannel> SightTraceChannel = ECC_Visibility;

    float CurrentViewAngle = 90.f;
    bool  bWasInDetect = false;
    bool  bLastIsCrouched = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Target")
    AActor* Target = nullptr;

    static bool HasClearSight(UWorld* World, const AActor* FromActor, const AActor* ToActor,
        ECollisionChannel Channel, const FName& EyeSocket);

    // ------------ 공격
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack")
    float attackRange = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack")
    float MinAttackDist = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack")
    float DesiredAttackDist = 80.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack")
    float attackDelayTime = 0.4f;

    /** 공격 후에도 잠시 어그로 유지(재추격) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack")
    float PostAttackAggroHold = 3.0f;

    /** 공격 진입 거리(표면거리 기준) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack")
    float AttackEnterDist = 90.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack")
    float PostAttackHold = 0.5f;

    bool bAttackCommitted = false;
    bool bDidDamageThisSwing = false;

    void PlayAttackMontageImmediate();

    // ------------ 전투 유지
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
    float AggroHoldDuration = 3.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
    float AggroOnDamageHoldDuration = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
    float HearingHoldDuration = 2.f;

    float AggroHold = 0.f;
    float AggroOnDamageHold = 0.f;
    float HearingHold = 0.f;

    // ====== 발견시 빨강 ‘락’ 유지 옵션 ======
public:
    /** 시야가 끊겨도 이 시간(초) 동안 빨간색 유지 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Sight")
    float KeepAlertSeconds = 1.2f;

    /** 마지막으로 플레이어를 “확실히” 봤던 시간 */
    float LastSeenTime = -1000.f;

    /** 시각 효과(빨강/무색) 중복 적용 방지 캐시 */
    bool bPrevAlertVisual = false;

    // ------------ Find
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Find")
    float findWindupTime = 0.7f;

    float findElapsed = 0.f;
    bool  bFindMontageStarted = false;

    // ------------ 패트롤/네비/문
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Patrol")
    bool bEnablePatrol = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Patrol")
    float idleDelayTime = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Patrol")
    float PatrolRadius = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Patrol")
    float stopDistance = 80.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Patrol")
    float RepathInterval = 1.0f;

    bool   bHasPatrolDest = false;
    FVector PatrolDestination;
    float  LastPathBuildTime = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Door")
    float DoorSearchRadius = 600.f;

    // ------------ 청각
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Hearing")
    bool bEnableHearing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Hearing")
    float HearingRange = 900.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Hearing")
    float HearingMaxRange = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Hearing")
    float LoudnessScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Hearing")
    float OccludedHearingScale = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Hearing")
    float PathPenaltyThreshold = 1.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Hearing")
    float PathPenaltyScale = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Hearing")
    float HearingAcceptanceRadius = 120.f;

    FVector LastHeardLocation = FVector::ZeroVector;

    UFUNCTION(BlueprintCallable)
    void ReportNoise(const FVector& NoiseLocation, float Loudness = 1.f);

    // ------------ 어그로 우선순위
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Chase")
    bool bChaseAttackerOnDamage = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Chase")
    bool bPreferAggressorOverSight = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Chase")
    float DamageChaseAcceptanceRadius = 80.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Chase")
    AActor* LastAggressor = nullptr;

    FVector LastKnownAggressorLoc = FVector::ZeroVector;

    UFUNCTION(BlueprintCallable)
    void ReportPerceivedDamage(AActor* InstigatorActor, const FVector& AtLocation, float DamageValue);

    AActor* ResolveChaseTarget() const;

    // ------------ 스턱/이동 보정
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Move")
    float StuckCheckInterval = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Move")
    float StuckSpeedThreshold = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Move")
    float StuckTimeToRecover = 3.0f;

    FVector LastSamplePos = FVector::ZeroVector;
    float   StuckSampleElapsed = 0.f;
    float   StuckAccum = 0.f;

    bool UpdateStuckAndIsBlocked(float DeltaTime);

    class ADoor* FindBestDoorToward(const FVector& From, const FVector& To) const;
    bool GoToDoorFrontThenThrough(class ADoor* Door, float Ahead);

    // ------------ 라이트 색 적용
public:
    void UpdateSightLightColorByState();
    bool IsAlert() const;

    // ------------ 사운드
public:
    UPROPERTY(EditAnywhere, Category = "Audio|SFX")
    TSoftObjectPtr<USoundBase> FindSoundAsset;
    UPROPERTY() USoundBase* FindSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "Audio|SFX")
    TSoftObjectPtr<USoundBase> HitSoundAsset;
    UPROPERTY() USoundBase* HitSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "Audio|SFX")
    TSoftObjectPtr<USoundBase> MoveLoopSoundAsset;
    UPROPERTY() USoundBase* MoveLoopSound = nullptr;

    UPROPERTY() UAudioComponent* MoveLoopAC = nullptr;

    FTimerHandle MoveLoopInitTimerHandle;
    void DelayedInitMoveLoopAC();

    void StartMoveLoop();
    void StopMoveLoop(float FadeTime = 0.03f);

    float LastFindSfxTime = -1000.f;
    float LastHitSoundTime = -1000.f;

    UPROPERTY(EditAnywhere, Category = "Audio|SFX")
    float HitSoundCooldown = 0.1f;

    // ------------ Hit VFX
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    TSubclassOf<AActor> HitVFXActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    FName HitVFXSocketName = "head";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    bool bAttachHitVFXToMesh = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    float HitVFXLifeSpan = 1.0f;

    float LastHitVFXTime = -1000.f;

    UPROPERTY(EditAnywhere, Category = "VFX")
    float HitVFXCooldown = 0.05f;

    void SpawnHitVFX();

    // ------------ 사망
public:
    bool  bEnteredDieState = true;
    float DeathElapsed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Death")
    float DeathForceDestroyTime = 5.0f;

    // ------------ 내부 타이머
    float currentTime = 0.f;

    // ------------ 내부 포인터
public:
    UPROPERTY() AEnemy* me = nullptr;
    UPROPERTY() UEnemyAnim* anim = nullptr;
    UPROPERTY() AAIController* ai = nullptr;

    EEnemyState mState = EEnemyState::Idle;
    EEnemyState PrevState = EEnemyState::Idle;

    // 지연 초기화
    FTimerHandle SafeInitTimerHandle;
    void DelayedSafeInit();

    // NavMesh 랜덤
    bool GetRandomPostionInNavMesh(FVector centerLocation, float radius, FVector& dest);

    // ===== MoveTo 스팸 방지 캐시 =====
private:
    // 배우 목표
    TWeakObjectPtr<AActor> CachedGoalActor;
    float CachedAccRadius = -1.f;

    // 위치 목표
    FVector CachedGoalLocation = FVector::ZeroVector;
    float   CachedAccRadius_Loc = -1.f;

    bool IssueMoveToActorIfChanged(AActor* Goal, float AcceptanceRadius);
    bool IssueMoveToLocationIfChanged(const FVector& Goal, float AcceptanceRadius);

    // 이동 캐시 무효화(정지 직후/상태 전환 직후 등 사용)
    void InvalidateMoveCache();

public:
    void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);





};
