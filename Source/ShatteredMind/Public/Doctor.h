// Doctor.h
// ─────────────────────────────────────────────────────────────────────────────
// 플레이어2 캐릭터 ‘의사(Doctor)’의 헤더.
// 주요 기능: 이동/점프/달리기/회피, 무기 전환(권총/망치), 근접 히트박스,
// 상호작용(라인트레이스), 조준 UI, 발소리 소음 브로드캐스트 등.
// 초보용으로 각 멤버/함수의 목적과 사용처를 최대한 자세히 달아놓음.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "inputAction.h"
#include "Components/BoxComponent.h"
#include "DoctorHUD.h"
#include "Sound/SoundBase.h"
#include "PlayerHealthComponent.h"
#include "Components/AudioComponent.h"
#include "Doctor.generated.h"

// [[ 상호작용 ]]에서 사용할 타입들 전방 선언
// - 헤더 간 순환 참조를 피하기 위해 여기선 선언만 하고, 실제 정의는 해당 헤더에서 처리
class IInteractionInterface;
struct FDoctorInteractableData;

// ─────────────────────────────────────────────
// [상호작용 실행 상태] : 라인트레이스 체크 주기/현재 대상 저장용
// ─────────────────────────────────────────────
USTRUCT()
struct FDoctorInteractionData
{
    GENERATED_BODY()

    FDoctorInteractionData()
        : CurrentInteractable(nullptr), LastInteractionCheckTime(0.0f)
    {
    }

    // 현재 화면 중앙(라인트레이스)으로 포커스 된 상호작용 가능한 액터(문, 아이템 등)
    UPROPERTY() AActor* CurrentInteractable;

    // 마지막 상호작용 체크 시각(초) — 매 틱마다 쏘지 않고 일정 주기로만 검사하기 위함
    UPROPERTY() float   LastInteractionCheckTime;
};

UCLASS()
class SHATTEREDMIND_API ADoctor : public ACharacter
{
    GENERATED_BODY()

public:
    // 생성자: 메시/카메라/컴포넌트/기본 파라미터 세팅
    ADoctor();

protected:
    // 게임 시작 시(Spawn/BeginPlay) 1회 호출
    virtual void BeginPlay() override;

public:
    // 매 프레임 호출
    virtual void Tick(float DeltaTime) override;

    // 인풋 바인딩(Enhanced Input)
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // =========================================================================
    //  상 호 작 용  (E키 등으로 문/아이템 등 Interaction)
    // =========================================================================
public:


    // 상호작용 입력 액션(E키 등)
    UPROPERTY(EditAnywhere, Category = "Input")
    class UInputAction* ia_Player2_Interaction;

    // 현재 포커스 중인 상호작용 대상(인터페이스로 접근)
    UPROPERTY(VisibleAnywhere, Category = "Interaction")
    TScriptInterface<IInteractionInterface> TargetInteractable;

    // 라인트레이스 체크 주기(초). 너무 자주 쏘면 비용이 커서 주기적으로만 검사
    float        InteractionCheckFrequecy = 0.f;

    // 상호작용 감지 거리(라인트레이스 길이)
    float        InteractionCheckDistance = 0.f;

    // 지속형 상호작용(게이지 등) 타이머 핸들
    FTimerHandle TimerHandle_Interaction;

    // 상호작용 상태 데이터(현재 대상/최근 검사 시각)
    FDoctorInteractionData InteractionData;

    // 현재 상호작용(타이머형)이 진행 중인지 여부
    bool IsInteracting() const
    {
        return GetWorldTimerManager().IsTimerActive(TimerHandle_Interaction);
    }

    // 전방 라인트레이스로 상호작용 가능한 대상 찾기
    void DoctorPerformInteractionCheck();

    // 새 상호작용 대상을 찾았을 때(포커스 진입)
    void DoctorFoundInteractable(AActor* _NewInteractable);

    // 대상을 찾지 못했을 때 또는 포커스 해제 필요할 때
    void DoctorNoInteractableFound();

    // 상호작용 시작(버튼 누름) — 즉시형/지속형 분기
    void DoctorBeginInteract();

    // 상호작용 종료(버튼 뗌 또는 취소)
    void DoctorEndInteract();

    // 상호작용 실제 수행(타이머 완료 시 호출)
    void DoctorInteract();

    // =========================================================================
    //  카 메 라 / 입 력  (3인칭 카메라 & 마우스 회전/이동)
    // =========================================================================
public:
    // 3인칭 카메라 거리/충돌 보정용 스프링암
    UPROPERTY(VisibleAnywhere, Category = Camera)
    class USpringArmComponent* springArmComp;

    // 실제 화면에 보이는 카메라
    UPROPERTY(VisibleAnywhere, Category = Camera)
    class UCameraComponent* Player2_CamComp;

    // 이 캐릭터에 적용할 인풋 매핑 컨텍스트(Enhanced Input)
    UPROPERTY(EditDefaultsOnly, Category = "input")
    class UInputMappingContext* imc_Doctor;

    // 마우스 상하 회전 입력(피치)
    UPROPERTY(EditDefaultsOnly, Category = "input")
    class UInputAction* ia_Player2_LookUP;

    // 마우스 좌우 회전 입력(요)
    UPROPERTY(EditDefaultsOnly, Category = "input")
    class UInputAction* ia_Player2_Turn;

    // 상하 회전 처리 함수
    void Player2_LookUp(const struct FInputActionValue& inputValue);

    // 좌우 회전 처리 함수
    void Player2_Turn(const struct FInputActionValue& inputValue);

    // 이동(WSAD/스틱 등 2D 벡터)
    UPROPERTY(EditDefaultsOnly, Category = "input")
    class UInputAction* ia_Player2_Move;

    // 걷기 속도(기본 이동 속도)
    UPROPERTY(EditDefaultsOnly, Category = PlayerSettiog)
    float walkSpeed = 200;

    // 달리기 속도(Shift 길게 누름 등으로 전환)
    UPROPERTY(EditDefaultsOnly, Category = PlayerSettiog)
    float runSpeed = 600;

    // 입력으로 들어온 한 프레임의 이동 방향(컨트롤러 회전 기반으로 월드 벡터로 변환 예정)
    FVector direction;

    // 이동 입력 처리(버퍼에 저장)
    void    Player2_Move(const struct FInputActionValue& inputValue);

    // 점프 입력 액션
    UPROPERTY(EditDefaultsOnly, Category = "input")
    class UInputAction* ia_Player2_Jump;

    // 점프 처리
    void Player2_Jump(const struct FInputActionValue& inputValue);

    // 실제 이동 반영(AddMovementInput) — Tick에서 호출
    void PlayerMove();

    // =========================================================================
    //  무 기 / 발 사  (권총 트레이스, 망치 근접)
    // =========================================================================
public:
    // 권총 메시(스켈레탈) — 소켓에서 총구 위치/애님 사용
    UPROPERTY(VisibleAnywhere, Category = WeaponMesh)
    class USkeletalMeshComponent* gunMeshComp;

    // 총알 히트 이펙트 파티클(탄흔/스파크 등)
    UPROPERTY(EditAnywhere, Category = BulletEffect)
    class UParticleSystem* bulletEffectFactory;

    // 공격 입력(좌클릭/RT 등)
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    class UInputAction* ia_Player2_Fire;

    // 공격 처리(망치: 충돌 On/Off, 권총: 라인트레이스)
    void Player2_Fire(const struct FInputActionValue& inputValue);

    // 망치 메시(스태틱) — 기본은 등쪽 소켓에 장착
    UPROPERTY(VisibleAnywhere, Category = WeaponMesh)
    class UStaticMeshComponent* HammerComp;

    // 권총 전환 키(토글)
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    class UInputAction* ia_Pistol_Gun;

    // 망치 전환 키(토글)
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    class UInputAction* ia_Melee_Weapon;

    // 현재 권총 사용 중인지(장착 상태)
    bool bUsingPistolGun = false;

    // 현재 망치 사용 중인지(장착 상태)
    bool bUsingHammer = false;

    // 권총으로 전환/해제
    void ChangeToPistolGun(const struct FInputActionValue& inputValue);

    // 망치로 전환/해제
    void ChangeToMeleeWepon(const struct FInputActionValue& inputValue);

    // 총 메시 가시성 토글(프로젝트 정책에 따라 외부에서 호출 가능)
    void change_pistol();

    // =========================================================================
    //  이 동 / 회 피  (짧게: 회피, 길게: 달리기 유지)
    // =========================================================================
public:
    // 달리기/회피 복합 입력(Started/Triggered/Completed로 길이 판정)
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    class UInputAction* ia_Player2_Run;

    // (구형) 걷기/달리기 토글 — 현재는 길게 누르기 로직으로 대체되어 사용 적음
    void Player2_Run();

    // 앉기(토글) 입력 — Crouch/UnCrouch
    UPROPERTY(EditDefaultsOnly, Category = "input")
    class UInputAction* ia_Player2_Sit;

    // 앉기 처리
    void Player2_Sit(const struct FInputActionValue& inputValue);

    // 회피(별도 키, V 등)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ia_Player2_Evade;

    // 회피 버튼이 눌렸는지(Started/Completed에서 bool로 전달받음)
    bool bEvadeButtonPressed = false;

    // 회피 기능 담당 컴포넌트(루트모션/무적시간/방향 등 구현)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evade/Dodge",
        meta = (AllowPrivateAccess = "true"))
    class UEvadeComponent* EvadeComponent;

    // 회피 입력 처리(Started/Completed 공용)
    void  Player2_Evade(const struct FInputActionValue& inputValue);

    // ‘달리기/회피’ 복합 입력에서 길이 비교용 시작 시각
    float HoldStartTime = 0.f;

    // 짧은 입력(회피) 1회만 트리거 되도록 막는 플래그
    bool  IsShortActionTriggered = false;

    // 달리기/회피 복합 입력: 키 누름 시작(Started)
    void  HandleInputStarted(const FInputActionInstance& InputActionInstance);

    // 달리기/회피 복합 입력: 키 뗌(Completed) — 0.3초 미만이면 회피
    void  HandeInputCompleted();

    // 달리기/회피 복합 입력: 누르고 있는 동안(Triggered) — 0.3초 이상이면 달리기 유지
    void  Triggerd_Timer_Shift();

    // =========================================================================
    //  에 임 오 프 셋 / 조 준  U I
    // =========================================================================
public:
    // 애님BP에서 사용할 에임 오프셋 값 (Yaw/Pitch)
    float    AO_Yaw = 0.f;
    float    AO_Pitch = 0.f;

    // 기준 회전(정지 상태에서 AO_Yaw 계산용 기준)
    FRotator StartingAimRotation = FRotator::ZeroRotator;

    FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
    FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }

    // 매 틱 호출: 이동/공중 여부에 따라 AO 기준 갱신/계산
    void AimOffset(float DeletaTime);

    // 권총 조준 UI(크로스헤어 등) 위젯 클래스
    UPROPERTY(EditDefaultsOnly, Category = PistolUI)
    TSubclassOf<class UUserWidget> pistolUIFactory;

    // 생성된 조준 UI 인스턴스(조준 시 AddToViewport / 해제 시 RemoveFromParent)
    UPROPERTY()
    class UUserWidget* _pistolUI = nullptr;

    // 마우스 우클릭 등: 조준 토글 입력
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    class UInputAction* ia_Camera_zoom_in_out;

    // 조준 토글 처리(카메라 FOV/애님 플래그/UI)
    void Pistol_Aim(const struct FInputActionValue& inputValue);

    // 현재 조준 중인지
    bool bPistolAim = false;

    // =========================================================================
    //  망 치  충 돌 (근접 무기 히트박스)
    // =========================================================================
public:
    // 망치 히트박스(스윙 타이밍에만 활성화)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowpriavateAccess = "true"))
    class UBoxComponent* HammerCollision = nullptr;

    // 망치 충돌 콜백: BeginOverlap에서 들어오는 히트 처리
    UFUNCTION()
    void OnHammerWeaponOverlap(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    // 공격 창 시작(충돌 ON, 스윙당 1회 데미지 허용)
    virtual void ActivateHammerWeapon();

    // 공격 창 종료(충돌 OFF)
    virtual void DeactivateHammerWeapon();

    // 망치 데미지 수치(사용처: 애님/플레이어 이펙트 등 — 실제 적용은 DamageHelper/EnemyFSM)
    UPROPERTY(EditAnywhere, Category = "Weapon|Effect")
    float HammerDamage = 30.f;

    // 히트 파티클/사운드(원하면 사용) — 현재 샘플 코드에선 미사용 가능
    UPROPERTY(EditAnywhere, Category = "Weapon|Effect")
    class UParticleSystem* HitParticle = nullptr;

    UPROPERTY(EditAnywhere, Category = "Weapon|Effect")
    class USoundBase* HitSound = nullptr;

    // 스윙당 1회만 데미지 적용하기 위한 가드 플래그
    bool bHasDealtDamage = false;

    // =========================================================================
    //  발 소 리 / A I  소 음  브 로 드 캐 스 트
    // =========================================================================
public:
    // 소음 발생 최소 속도(cm/s). 이보다 느리면 발소리 없음
    UPROPERTY(EditAnywhere, Category = "AI|Noise")
    float FootstepMinSpeed = 80.f;

    // 걷기 상태에서 발소리 간격(초)
    UPROPERTY(EditAnywhere, Category = "AI|Noise")
    float FootstepInterval_Walk = 0.45f;

    // 달리기 상태에서 발소리 간격(초)
    UPROPERTY(EditAnywhere, Category = "AI|Noise")
    float FootstepInterval_Run = 0.25f;

    // 걷기 소음 크기(EnemyFSM에서 임의의 단위로 해석)
    UPROPERTY(EditAnywhere, Category = "AI|Noise")
    float Loudness_Walk = 1.0f;

    // 달리기 소음 크기(더 멀리 들리도록)
    UPROPERTY(EditAnywhere, Category = "AI|Noise")
    float Loudness_Run = 1.7f;

    // 내부 누적 타이머(발소리 주기 계산)
    float FootstepAccum = 0.f;

    // 매 틱 호출: 현재 속도/상태에 따라 발소리 발생
    void UpdateMovementNoise(float DeltaTime);

    // 주변 Enemy에게 ‘소음 발생’ 이벤트 전달(FSM.ReportNoise 호출)
    void BroadcastNoiseToEnemies(float Loudness);

    // =========================================================================
    //  체 력  (UI/디버그 등에서 사용 — 실제 피격 로직은 현재 제거됨)
    // =========================================================================
public:



    // =========================================================================
    //  플레이어 의사 UI - 상호작용 일기장 , 무기획득 , 체력 바 등등..
    // =========================================================================
public:
    //// 의사 UI
    //UPROPERTY()
    //ADoctorHUD* HUD;


public:

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* GunFireSound;


public:
    // 체력 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
    UPlayerHealthComponent* HealthComp;

    // 맞을 때마다 발생할 피 튀김 효과
    UPROPERTY(EditAnywhere, Category = "VFX")
    TArray<UParticleSystem*> BloodEffects;

    // 맞을 때 발생할 사운드
    UPROPERTY(EditAnywhere, Category = "Sound")
    USoundBase* DoctorHitSound;



    // =========================================================================
//  발 소 리  루 프  (실제 플레이어 귀에 들리는 걷는 소리)
//  - 이동 중에만 재생, 멈추면 바로 꺼짐
//  - 애니 노티파이 없이 Tick에서 속도 보고 관리
// =========================================================================
public:
    // 에디터에서 넣어줄 루프 사운드 (예: 발소리/옷 마찰/헐떡임 등)
    UPROPERTY(EditAnywhere, Category = "Audio|Footstep")
    TSoftObjectPtr<USoundBase> FootstepLoopSoundAsset;

    // 로드된 진짜 사운드
    UPROPERTY()
    USoundBase* FootstepLoopSound = nullptr;

    // 실제 재생 담당하는 오디오 컴포넌트
    UPROPERTY()
    UAudioComponent* FootstepLoopAC = nullptr;

    // AudioComponent를 안전하게 부착/등록하는 지연 초기화
    void InitFootstepAudioComponentDelayed();

    // 걷는 중일 때 페이드인
    void StartFootstepLoop();

    // 멈출 때 페이드아웃
    void StopFootstepLoop(float FadeTime = 0.05f);

    // 위 함수 호출 시점 제어용 타이머
    FTimerHandle FootstepInitTimerHandle;


public:
    // 사망 여부(중복 처리 방지)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    bool bIsDead = false;

    // 내가 죽기 전에 몇 킬 했는지 기록할 변수
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    int32 KillCount = 0;

    // GameMode가 쓸 수 있게 Getter
    UFUNCTION(BlueprintCallable, Category = "State")
    int32 GetKillCount() const { return KillCount; }

    // 외부(Enemy 쪽)에서 적 처치할 때 불러줄 함수
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void AddKill();

    // 체력 0 됐을 때 최종 사망 처리 (움직임 끊고 GameMode에 알리기)
    UFUNCTION(BlueprintCallable, Category = "State")
    void HandlePlayerDeath();

    // 데미지 처리 오버라이드 (언리얼 표준 데미지 파이프라인)
    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator,
        AActor* DamageCauser
    ) override;

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> HitEffectWidgetClass;

    UFUNCTION()
    void PlayHitEffect();




    // 전체 덮는 캔버스 (WBP_HitOverlay)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitUI")
    TSubclassOf<UUserWidget> HitOverlayWidgetClass;

    // 실제로 화면에 깔려서 유지 중인 오버레이 인스턴스
    UPROPERTY()
    UUserWidget* HitOverlayInstance;


    // Doctor.h
    UPROPERTY(EditDefaultsOnly, Category = "CameraShake")
    TSubclassOf<UCameraShakeBase> HitShakeClass;
    
    bool bWantToEquipHammerAfterPistol = false; // 권총 집어넣은 후 망치 장착 예약
    bool bWantToEquipPistolAfterHammer = false; // 망치 집어넣은 후 권총 장착 예약



 
    /** 기본 이동속도 저장용 (앉기 전 속도) */
    float DefaultWalkSpeed = 0.f;



    /** 무기 전환 중인지 여부 (전환 중에는 입력 차단용) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    bool bIsSwitchingWeapon = false;

    // 멤버 변수 추가 (Doctor.h)
    bool bWeaponMontageLock = false; // 전환 중 다른 무기 몽타주 금지


    void OnHammerPutAwayEnded(UAnimMontage* Montage, bool bInterrupted);
    void OnPistolPutAwayEnded(UAnimMontage* Montage, bool bInterrupted);


   

    // 공격 중 상태 플래그 (망치 공격 중 중복 입력 방지)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bIsHammerAttacking = false;

    // 공격 애니메이션 종료 시 호출되는 콜백
    UFUNCTION()
    void OnHammerAttackEnded(UAnimMontage* Montage, bool bInterrupted);




    // 연타 방지용 플래그
    bool bIsAimingSwitchLocked = false;

    // 연타 방지 타이머 핸들
    FTimerHandle AimLockHandle;

    // 위젯 보정용 타이머
    FTimerHandle AimUICheckHandle;



    UFUNCTION()
    void StartPistolAim(const FInputActionValue& inputValue);

    UFUNCTION()
    void StopPistolAim(const FInputActionValue& inputValue);

    // 무기 장착/해제 중 상태 표시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    bool bIsWeaponEquipping = false;
};
