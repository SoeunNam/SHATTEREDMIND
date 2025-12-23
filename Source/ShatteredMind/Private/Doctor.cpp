// Doctor.cpp
// ─────────────────────────────────────────────────────────────────────────────
// 플레이어2 ‘의사(Doctor)’ 캐릭터의 C++ 구현 파일.
// 이동/점프/회피/무기 전환/사격/상호작용/소음 브로드캐스트/에임오프셋/망치 충돌 등.
// 초보용으로 섹션별로 설명 주석을 자세히 달아두었음.
// ─────────────────────────────────────────────────────────────────────────────

#include "Doctor.h"
#include "EnemyFSM.h"
#include "DoctorGameModeBase.h"
#include "DamageHelper.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include <GameFramework/SpringArmComponent.h>
#include <Camera/CameraComponent.h>
#include "EnhancedInputSubsystems.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include <GameFramework/CharacterMovementComponent.h>
#include <Kismet/GameplayStatics.h>
#include "Blueprint/UserWidget.h"
#include "DoctorAnim.h"
#include "EvadeComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include <Blueprint/UserWidget.h>
#include <Components/BoxComponent.h>
#include "DrawDebugHelpers.h"
#include "InteractionInterface.h"
#include "Enemy.h"
#include "DoctorHUD.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"

ADoctor::ADoctor()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 Tick() 호출 허용

    // ─────────────────────────────────────────────
    // 1) 메시(캐릭터 외형) 세팅
    // ─────────────────────────────────────────────
    ConstructorHelpers::FObjectFinder<USkeletalMesh> TempMesh(TEXT("/Script/Engine.SkeletalMesh'/Game/Scientists/Scientist_Man/Mesh/SK_sm_01.SK_sm_01'"));
    if (TempMesh.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(TempMesh.Object);
        // 메시가 바닥에 파묻히지 않도록 위치/회전 오프셋
        GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -90), FRotator(0, -90, 0));
    }

    // ─────────────────────────────────────────────
    // 2) 카메라(SpringArm + Camera) 세팅 (3인칭)
    // ─────────────────────────────────────────────
    springArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
    springArmComp->SetupAttachment(RootComponent);
    springArmComp->SetRelativeLocation(FVector(0, 70, 90)); // 캐릭터 기준 오프셋
    springArmComp->TargetArmLength = 200;                   // 카메라 거리
    springArmComp->bUsePawnControlRotation = true;          // 마우스 회전을 암이 따라가도록

    Player2_CamComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Player2_ComComp"));
    Player2_CamComp->SetupAttachment(springArmComp);
    Player2_CamComp->bUsePawnControlRotation = false;       // 카메라는 암 회전을 그대로 사용

    bUseControllerRotationYaw = true; // 좌우(Yaw) 회전을 컨트롤러가 결정
    JumpMaxCount = 2;                 // 2단 점프

    // ─────────────────────────────────────────────
    // 3) 총(권총) 메시 세팅
    // ─────────────────────────────────────────────
    gunMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GunMeshComp"));
    gunMeshComp->SetupAttachment(GetMesh(), TEXT("spine_01Socket")); // 척추 소켓에 부착
    {
        ConstructorHelpers::FObjectFinder<USkeletalMesh> TempGunMesh(TEXT("/Script/Engine.SkeletalMesh'/Game/makarov-pistol_extracted/source/FinalMakarov.FinalMakarov'"));
        if (TempGunMesh.Succeeded())
        {
            gunMeshComp->SetSkeletalMesh(TempGunMesh.Object);
            // 손/허리에 잘 붙도록 위치/회전/스케일 조정(프로젝트에 맞게 튜닝)
            gunMeshComp->SetRelativeLocation(FVector(-0.7f, -15.5f, 16.980762f));
            gunMeshComp->SetRelativeRotation(FRotator(0.000001f, -269.999999f, -180.0f));
            gunMeshComp->SetRelativeScale3D(FVector(1.5f));
        }
    }

    // ─────────────────────────────────────────────
    // 4) 망치(근접 무기) 메시 세팅
    // ─────────────────────────────────────────────
    HammerComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HammerComp"));
    HammerComp->SetupAttachment(GetMesh(), TEXT("spine_03Socket")); // 등쪽 소켓에 부착
    {
        ConstructorHelpers::FObjectFinder<UStaticMesh> TempHammerMesh(TEXT("/Script/Engine.StaticMesh'/Game/Hammer/All/hammer_photoscan/StaticMeshes/Object_4.Object_4'"));
        if (TempHammerMesh.Succeeded())
        {
            HammerComp->SetStaticMesh(TempHammerMesh.Object);
            HammerComp->SetRelativeLocation(FVector(34.0f, -30.0f, -18.0f));
            HammerComp->SetRelativeRotation(FRotator(69.846059f, 358.0f, 69.567958f));
            HammerComp->SetRelativeScale3D(FVector(0.55f));
        }
    }

    // ─────────────────────────────────────────────
    // 5) 망치 충돌 박스 (스윙할 때만 켜서 히트 판정용)
    // ─────────────────────────────────────────────
    HammerCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Hammer_Weapon_Box"));
    HammerCollision->SetupAttachment(HammerComp);          // 망치 메시에 직접 부착
    HammerCollision->SetRelativeLocation(FVector(20.f, 0.f, 0.f)); // 망치 머리 근처
    HammerCollision->SetRelativeRotation(FRotator::ZeroRotator);
    HammerCollision->InitBoxExtent(FVector(8.f, 6.f, 6.f));        // 크기(필요 시 튜닝)

    // 무기 표시 기본값(둘 다 보이게 시작)
    HammerComp->SetVisibility(true);
    gunMeshComp->SetVisibility(true);

    // ─────────────────────────────────────────────
    // 6) 회피 컴포넌트 (구르기/대시 등 Evade 기능 담당)
    // ─────────────────────────────────────────────
    EvadeComponent = CreateDefaultSubobject<UEvadeComponent>(TEXT("EvadeCompont"));

    // ─────────────────────────────────────────────
    // 7) 상호작용(라인 트레이스) 파라미터
    // ─────────────────────────────────────────────
    InteractionCheckFrequecy = 0.1f;  // 0.1초마다 한 번 체크
    InteractionCheckDistance = 225.0f;// 상호작용 감지 거리
    BaseEyeHeight = 74.0f;            // 시선 높이(3인칭용)




    // 체력 컴포넌트 생성
    HealthComp = CreateDefaultSubobject<UPlayerHealthComponent>(TEXT("HealthComp"));
}

void ADoctor::BeginPlay()
{
    Super::BeginPlay();

    // ===== 진짜 런타임 HealthComp를 찾아서 델리게이트 바인딩 =====
    UPlayerHealthComponent* RealHealth = FindComponentByClass<UPlayerHealthComponent>();
    if (RealHealth)
    {
        UE_LOG(LogTemp, Warning, TEXT("Doctor::BeginPlay() Binding OnPlayerDied to %s"), *RealHealth->GetName());
        RealHealth->OnPlayerDied.AddDynamic(this, &ADoctor::HandlePlayerDeath);

        // 혹시 멤버 HealthComp가 다른 인스턴스를 가리키고 있으면 여기서 동기화
        HealthComp = RealHealth;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Doctor::BeginPlay() NO HealthComp FOUND AT RUNTIME"));
    }

    // 기본 이동 속도 설정(걷기)
    GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

    //// --- FootstepLoopSound 세팅 (의사 전용 발소리 루프) ---
    //if (FootstepLoopSoundAsset.IsValid())
    //{
    //    FootstepLoopSound = FootstepLoopSoundAsset.Get();
    //}
    //else if (FootstepLoopSoundAsset.ToSoftObjectPath().IsValid())
    //{
    //    FootstepLoopSound = FootstepLoopSoundAsset.LoadSynchronous();
    //}

    //if (FootstepLoopSound)
    //{
    //    // AudioComponent 동적 생성
    //    FootstepLoopAC = NewObject<UAudioComponent>(this, TEXT("DoctorFootstepLoopAC"));
    //    if (FootstepLoopAC)
    //    {
    //        FootstepLoopAC->bAutoActivate = false;           // 자동 재생 금지
    //        FootstepLoopAC->bAllowSpatialization = true;     // 공간화(3D 소리)
    //        FootstepLoopAC->bIsUISound = false;              // UI 사운드 아님
    //        FootstepLoopAC->SetSound(FootstepLoopSound);

    //        // 바로 Register / Attach 하지 말고 한 틱 뒤에 처리
    //        GetWorldTimerManager().SetTimer(
    //            FootstepInitTimerHandle,
    //            this,
    //            &ADoctor::InitFootstepAudioComponentDelayed,
    //            0.0f,
    //            false
    //        );
    //    }
    //}


    // ─────────────────────────────────────────────
    // 인핸스드 인풋 매핑 컨텍스트 등록
    // ─────────────────────────────────────────────
    if (auto pc = Cast<APlayerController>(Controller))
    {
        if (auto subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer()))
        {
            subsystem->AddMappingContext(imc_Doctor, 0);
        }
    }

    // ─────────────────────────────────────────────
    // 권총 조준 UI 위젯 생성(추후 조준 시 AddToViewport)
    // ─────────────────────────────────────────────
    _pistolUI = CreateWidget(GetWorld(), pistolUIFactory);

    // ─────────────────────────────────────────────
    // 망치 충돌 설정: 기본은 꺼두고(Off), 공격 타이밍에만 On
    // ─────────────────────────────────────────────
    HammerCollision->SetGenerateOverlapEvents(false);
    HammerCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HammerCollision->SetCollisionObjectType(ECC_WorldDynamic);

    // 기본은 다 무시 → Pawn/동적/정적 오브젝트와만 겹침 허용
    HammerCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    HammerCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    HammerCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    HammerCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);

    HammerCollision->IgnoreActorWhenMoving(this, true); // 자기 자신은 무시
    HammerCollision->OnComponentBeginOverlap.AddDynamic(this, &ADoctor::OnHammerWeaponOverlap);

    // 의사 HUD 변수 초기화 캐스트
    //HUD = Cast<ADoctorHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());



    if (HealthComp)
    {
        // 사망 델리게이트
        HealthComp->OnPlayerDied.AddDynamic(this, &ADoctor::HandlePlayerDeath);

        // 피격 델리게이트
        HealthComp->OnPlayerDamaged.AddDynamic(this, &ADoctor::PlayHitEffect);
    }


    // ===== 피격 오버레이 위젯 깔기 (한 번만) =====
    if (HitOverlayWidgetClass && !HitOverlayInstance)
    {
        HitOverlayInstance = CreateWidget<UUserWidget>(GetWorld(), HitOverlayWidgetClass);
        if (HitOverlayInstance)
        {
            HitOverlayInstance->AddToViewport(40); // 상위 ZOrder, 피격 이펙트 전용
        }
    }

    // 앉기 이동속도 기본값 세팅
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        DefaultWalkSpeed = Move->MaxWalkSpeed;
    }
}

void ADoctor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 매 프레임 처리
    PlayerMove();               // 이동 벡터 적용

    // ── 여기서 발소리 루프 제어 (플레이어 청각용)
    {
        const FVector Vel = GetVelocity();
        const float Speed2D = FVector(Vel.X, Vel.Y, 0.f).Size();

        bool bShouldPlayFootstep = false;

        // 조건:
        // 1) 충분히 움직이고 있다 (속도 기준)
        // 2) 땅에 붙어있다 (낙하 중/점프 중 X)
        // 3) 앉아있지 않다 (Crouch 시엔 안 나게)
        if (Speed2D > 120.f) // 임계속도. 낮추면 살살 움직여도 소리 남
        {
            if (GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround())
            {
                if (!IsCrouched())
                {
                    bShouldPlayFootstep = true;
                }
            }
        }

        if (bShouldPlayFootstep)
        {
            StartFootstepLoop();
        }
        else
        {
            StopFootstepLoop(0.03f);
        }
    }
    // ── 발소리 루프 처리 끝 

    UpdateMovementNoise(DeltaTime); // 발소리 → 적 AI에 소음 브로드캐스트
    AimOffset(DeltaTime);       // 에임 오프셋(상체 트위스트 등) 계산

    // 예전 ‘피격 상태 디버그 메시지’는 요구대로 삭제됨

    if (HammerCollision)
    {
        // 콜리전의 중심 위치, 회전, 크기
        FVector Location = HammerCollision->GetComponentLocation();
        FQuat Rotation = HammerCollision->GetComponentQuat();
        FVector Extent = HammerCollision->GetScaledBoxExtent();

        // 현재 오버랩 중인지 확인
        TArray<AActor*> OverlappingActors;
        HammerCollision->GetOverlappingActors(OverlappingActors);

        bool bIsOverlapping = OverlappingActors.Num() > 0;

        // 초록색: 충돌 중 / 빨강색: 충돌 없음
        FColor DebugColor = bIsOverlapping ? FColor::Green : FColor::Red;

        //// 박스 디버그 그리기
        //DrawDebugBox(GetWorld(), Location, Extent, Rotation, DebugColor, false, -1.f, 0, 1.5f);
    }

    // 상호작용 관련 --
    //마지막 라인트레이스 생성 이후 생성 주기만큼 시간이 지나면
    if (GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequecy)
    {
        //상호작용가능 액터인지 확인하는 함수
        DoctorPerformInteractionCheck();
    }
}

void ADoctor::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // ─────────────────────────────────────────────
    // 인핸스드 인풋 바인딩
    // ─────────────────────────────────────────────
    auto Player2_Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    if (Player2_Input)
    {
        // 카메라 회전
        Player2_Input->BindAction(ia_Player2_LookUP, ETriggerEvent::Triggered, this, &ADoctor::Player2_LookUp);
        Player2_Input->BindAction(ia_Player2_Turn, ETriggerEvent::Triggered, this, &ADoctor::Player2_Turn);

        // 이동/점프
        Player2_Input->BindAction(ia_Player2_Move, ETriggerEvent::Triggered, this, &ADoctor::Player2_Move);
        Player2_Input->BindAction(ia_Player2_Jump, ETriggerEvent::Started, this, &ADoctor::Player2_Jump);

        // 공격(좌클릭 가정)
        Player2_Input->BindAction(ia_Player2_Fire, ETriggerEvent::Started, this, &ADoctor::Player2_Fire);

        // 무기 전환(권총/망치)
        Player2_Input->BindAction(ia_Pistol_Gun, ETriggerEvent::Started, this, &ADoctor::ChangeToPistolGun);
        Player2_Input->BindAction(ia_Melee_Weapon, ETriggerEvent::Started, this, &ADoctor::ChangeToMeleeWepon);

        // 달리기/짧게 누르면 회피, 길게 누르면 달리기 상태 유지
        Player2_Input->BindAction(ia_Player2_Run, ETriggerEvent::Started, this, &ADoctor::HandleInputStarted);
        Player2_Input->BindAction(ia_Player2_Run, ETriggerEvent::Triggered, this, &ADoctor::Triggerd_Timer_Shift);
        Player2_Input->BindAction(ia_Player2_Run, ETriggerEvent::Completed, this, &ADoctor::HandeInputCompleted);

        // 앉기/일어서기 토글
        Player2_Input->BindAction(ia_Player2_Sit, ETriggerEvent::Started, this, &ADoctor::Player2_Sit);

        // 회피(별도 키)
        Player2_Input->BindAction(ia_Player2_Evade, ETriggerEvent::Started, this, &ADoctor::Player2_Evade);
        Player2_Input->BindAction(ia_Player2_Evade, ETriggerEvent::Completed, this, &ADoctor::Player2_Evade);


        // 권총 조준 (우클릭: 누르고 있는 동안만)
        Player2_Input->BindAction(ia_Camera_zoom_in_out, ETriggerEvent::Started, this, &ADoctor::StartPistolAim);
        Player2_Input->BindAction(ia_Camera_zoom_in_out, ETriggerEvent::Completed, this, &ADoctor::StopPistolAim);




        // 상호작용 E키(예시) — 시작/종료
        Player2_Input->BindAction(ia_Player2_Interaction, ETriggerEvent::Completed, this, &ADoctor::DoctorBeginInteract);
        Player2_Input->BindAction(ia_Player2_Interaction, ETriggerEvent::Completed, this, &ADoctor::DoctorEndInteract);
    }
}

// (TakeDamage 오버라이드/피격 메시지 관련 코드는 요청대로 전부 제거)


// ─────────────────────────────────────────────
// [상호작용] 라인트레이스 + 대상 판별
// ─────────────────────────────────────────────
void ADoctor::DoctorPerformInteractionCheck()
{
    //라인트레이스 생성 시점?
    InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();
    //라인트레이스 시작 지점
    FVector TraceStart = GetPawnViewLocation();
    //라인트레이스 끝나는 지점
    //ViewRotation은 CharactorController에서 나옴, PlayerPawnMesh아니고
    //rotation을 vector로 변환한 값에 라인트레이스 길이를 곱하는 과정
    FVector TraceEnd = TraceStart + (GetViewRotation().Vector() * InteractionCheckDistance);

    //라인트레이스 표시하기, 테스트, 디버깅용
    //DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 1.0f, 0, 2.0f);

    //라인트레이스 인식을 위한 조건
    FCollisionQueryParams QueryParams;
    //나 자신은 맞으면 안되니까
    QueryParams.AddIgnoredActor(this);
    //라인트레이스 맞춘 정보 저장
    FHitResult TraceHit;
    //라인트레이스가 액터 맞추면
    if (GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
    {
        //만약 라인트레이스가 널포인터면?
        if (!TraceHit.GetActor())
        {
            DoctorNoInteractableFound();
            //함수 종료!!
            return;
        }

        //맞은 상호작용 액터가 진짜 상호작용 가능한 액터가 맞으면
        if (TraceHit.GetActor()->GetClass()->ImplementsInterface(UInteractionInterface::StaticClass()))
        {
            //거리
            const float Distance = (TraceStart - TraceHit.ImpactPoint).Size();
            //같은 액터를 보고있는게 아니고 라인 트레이스 범위 안의 액터라면
            if (TraceHit.GetActor() != InteractionData.CurrentInteractable
                && Distance < InteractionCheckDistance)
            {
                //상호작용 할 액터 찾은거고 액터에 넘김(??
                DoctorFoundInteractable(TraceHit.GetActor());
                return;
            }
            //봤던 액터 보고있는거면? 현상유지
            if (TraceHit.GetActor() == InteractionData.CurrentInteractable)
            {
                return;
            }
        }
        else
        {
            DoctorNoInteractableFound();
        }
    }
    else
    {
        DoctorNoInteractableFound();
    }
}

void ADoctor::DoctorFoundInteractable(AActor* _NewInteractable)
{
    //현재 상호작용 중이라면(꾹누르기/지속입력)
    if (IsInteracting())
    {
        //상호작용 중단 먼저/꼬임 방지
        DoctorEndInteract();
    }
    //이전에 보고있는 상호작용 대상이 있다면 그 대상의 포커스 해제
    if (InteractionData.CurrentInteractable)
    {
        TargetInteractable = InteractionData.CurrentInteractable;
        TargetInteractable->EndFocus();
    }
    //새로운 상호작용 대상을 현재 _NewInteractable로 등록
    InteractionData.CurrentInteractable = _NewInteractable;
    TargetInteractable = _NewInteractable;

    //HUD->UpdateInteractionWidget(&TargetInteractable->InteractableData);

    //새로 찾은 대상에 포커스를 시작(테두리표시?)
    TargetInteractable->BeginFocus();
}

void ADoctor::DoctorNoInteractableFound()
{
    if (IsInteracting())
    {
        //진행 중인 상호작용 타이머 초기화??
        GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);
    }
    //이전에 바라보던 상호작용 대상이 남아있다면 그 대상의 포커스 해제,
    //하이라이트나 ui잔상 안남게 처리
    if (InteractionData.CurrentInteractable)
    {
        //유효성 체크(직전에 쳐다본 물건은 하이라이트 해제해주기)
        if (IsValid(TargetInteractable.GetObject()))
        {
            TargetInteractable->EndFocus();
        }

        //HUD->HideInteractionWidget();

        //HUD에서 상호작용 위젯 숨기기
        //이전에 바라보던 오브젝트를 더 이상 현재 상호작용 대상을 취급하지 않기 위해서
        //명시적으로 초기화 해줌
        InteractionData.CurrentInteractable = nullptr;
        TargetInteractable = nullptr;
    }

}

void ADoctor::DoctorBeginInteract()
{
    //상호작용 시작 이후로 대상에 대한 상호작용 가능 상태에 변화 없음 체크
    DoctorPerformInteractionCheck();
    //이전에 바라보던 상호작용 대상이 남아있다면 그 대상의 포커스 해제,
    //하이라이트나 ui잔상 안남게 처리
    if (InteractionData.CurrentInteractable) // 상호작용 데이터
    {
        //유효성 체크
        if (IsValid(TargetInteractable.GetObject()))
        {
            //상호작용 대상의 BeginInteract 이벤트 실행
            TargetInteractable->BeginInteract();
            //상호작용 시간이 거의 0이면 즉시 상호작용 완료 처리
            if (FMath::IsNearlyZero(TargetInteractable->
                InteractableData.InteractionDuration, 0.1f))//에러 tolerance가 0.1
            {
                //상호작용 완료 동작 수행
                DoctorInteract();
            }
            else
            {
                //상호작용 시간이 존재하면 타이머를 설정하여 일정시간 후 DoctorInteract실행
                GetWorldTimerManager().SetTimer(TimerHandle_Interaction,//타이머 핸들
                    this, &ADoctor::DoctorInteract,//호출할 함수
                    TargetInteractable->InteractableData.InteractionDuration,//대기시간
                    false);//반복실행여부 false
            }
        }
    }
}

void ADoctor::DoctorEndInteract()
{
    // 유지형 상호작용 타이머 취소
    GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

    if (IsValid(TargetInteractable.GetObject()))
    {
        TargetInteractable->EndInteract(); // 대상 쪽 인터랙트 종료 콜백
    }
}

void ADoctor::DoctorInteract()
{
    // 타이머 클리어(안전)
    GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

    if (IsValid(TargetInteractable.GetObject()))
    {
        // 실제 상호작용 수행(예: 문 열림 확정, 아이템 줍기 등)
        TargetInteractable->DoctorInteract(this);
    }


}


// ─────────────────────────────────────────────
// [입력 처리] 카메라 회전/이동/점프 등
// ─────────────────────────────────────────────
void ADoctor::Player2_LookUp(const FInputActionValue& inputValue)
{
    AddControllerPitchInput(inputValue.Get<float>()); // 상하
}

void ADoctor::Player2_Turn(const FInputActionValue& inputValue)
{
    AddControllerYawInput(inputValue.Get<float>());   // 좌우
}

void ADoctor::Player2_Move(const FInputActionValue& inputValue)
{
    // 회피 중이면 이동 입력 무시(애니/루트모션 보호)
    if (EvadeComponent && EvadeComponent->GetIsEvading()) return;

    const FVector2D value = inputValue.Get<FVector2D>();
    // 방향 버퍼에 저장(실제 이동은 PlayerMove()에서 AddMovementInput)
    direction.X = value.X; // 앞/뒤
    direction.Y = value.Y; // 좌/우

    // 회피 컴포넌트에 현재 입력 전달(회피 방향 계산 등)
    if (EvadeComponent) EvadeComponent->SendMovementVector(value);

    // 회피 도중 다시 한 번 가드
    if (EvadeComponent && EvadeComponent->GetIsEvading()) return;

    UE_LOG(LogTemp, Warning, TEXT("Move_input"));
}

void ADoctor::Player2_Jump(const FInputActionValue& /*inputValue*/)
{
    Jump(); // 언리얼 내장 점프
}

void ADoctor::PlayerMove()
{
    // 컨트롤러 회전 기준으로 입력 벡터를 월드 방향으로 변환 후 이동
    direction = FTransform(GetControlRotation()).TransformVector(direction);
    AddMovementInput(direction);
    direction = FVector::ZeroVector; // 한 프레임마다 초기화
}

void ADoctor::Player2_Run()
{
    // 걷기 ↔ 달리기 토글
    auto movement = GetCharacterMovement();
    movement->MaxWalkSpeed = (movement->MaxWalkSpeed > walkSpeed) ? walkSpeed : runSpeed;
}

void ADoctor::Player2_Sit(const FInputActionValue& /*inputValue*/)
{
    auto movement = GetCharacterMovement();
    if (!movement) return;

    if (movement->IsFalling())
    {
        UE_LOG(LogTemp, Warning, TEXT("Jumping - cannot crouch"));
        return;
    }

    movement->GetNavAgentPropertiesRef().bCanCrouch = true;

    if (bIsCrouched)
    {
        UnCrouch();

        // 복귀 시 원래 걷기 속도로 되돌리기
        movement->MaxWalkSpeed = walkSpeed;
        movement->MaxWalkSpeedCrouched = walkSpeed; // 안전하게
        UE_LOG(LogTemp, Warning, TEXT("Crouch OFF - Speed restored: %.1f"), movement->MaxWalkSpeed);
    }
    else
    {
        Crouch();

        // 핵심!! 내부 crouch 전용 속도도 같이 세팅해야 함
        movement->MaxWalkSpeedCrouched = walkSpeed * 0.5f; // 👈 30% 속도
        movement->MaxWalkSpeed = movement->MaxWalkSpeedCrouched;

        UE_LOG(LogTemp, Warning, TEXT("Crouch ON - MaxWalkSpeedCrouched: %.1f"), movement->MaxWalkSpeedCrouched);
    }
}



void ADoctor::Player2_Evade(const FInputActionValue& inputValue)
{
    // 회피 키 입력 시작/종료 콜백(Started/Completed 바인딩됨)
    bEvadeButtonPressed = inputValue.Get<bool>();
    if (!EvadeComponent) return;

    // 현재 회피 중이 아니고, 버튼이 눌렸다면 회피 실행
    if (!EvadeComponent->GetIsEvading() && bEvadeButtonPressed)
    {
        EvadeComponent->Evade(this);
    }
}

// ─────────────────────────────────────────────
// [런/회피 복합 입력] 짧게 누르면 회피, 길게 누르면 달리기 유지
// Started → Triggered(길이 체크) → Completed
// ─────────────────────────────────────────────
void ADoctor::HandleInputStarted(const FInputActionInstance& InputActionInstance)
{
    (void)InputActionInstance; // (미사용 파라미터 경고 제거용)
    HoldStartTime = GetWorld()->GetTimeSeconds(); // 누르기 시작한 시간 기록
    IsShortActionTriggered = false;               // 아직 회피 안썼음
}

void ADoctor::HandeInputCompleted()
{
    // 키를 뗄 때, 누른 시간 길이 측정
    const float HoldDuration = GetWorld()->GetTimeSeconds() - HoldStartTime;

    // 0.3초 미만 = 짧게 누름 → 회피 1회
    if (HoldDuration < 0.3f && !IsShortActionTriggered)
    {
        if (EvadeComponent && !EvadeComponent->GetIsEvading())
        {
            EvadeComponent->Evade(this);
        }
        IsShortActionTriggered = true;
    }

    // 키 뗄 때 달리기였으면 걷기로 복귀
    auto movement = GetCharacterMovement();
    if (movement->MaxWalkSpeed > walkSpeed)
    {
        movement->MaxWalkSpeed = walkSpeed;
    }

    IsShortActionTriggered = false; // 다음 입력을 위해 리셋
}

void ADoctor::Triggerd_Timer_Shift()
{
    // 키를 누르고 있는 동안 주기적으로 호출됨 → 길게 누름 판정
    const float HoldDuration = GetWorld()->GetTimeSeconds() - HoldStartTime;

    // 0.3초 이상 = 길게 누름 → 달리기 상태 진입(유지)
    if (HoldDuration >= 0.3f && !IsShortActionTriggered)
    {
        auto movement = GetCharacterMovement();
        movement->MaxWalkSpeed = runSpeed;
        IsShortActionTriggered = true; // 중복 방지
    }
}


// ─────────────────────────────────────────────
// [에임 오프셋] 서있는 상태에서 상체만 좌우로 비틀어보는 값 계산
// 애님 블루프린트에서 AO_Yaw/AO_Pitch 사용
// ─────────────────────────────────────────────
void ADoctor::AimOffset(float DeletaTime)
{
    FVector Velocity = GetVelocity(); Velocity.Z = 0.f; // 수평 속도만
    const float Speed = Velocity.Size();
    const bool  bIsInAir = GetCharacterMovement()->IsFalling();

    // 정지 + 지상일 때 → 시작 시점 대비 회전 차이만큼 AO_Yaw 발생
    if (Speed == 0.f && !bIsInAir)
    {
        const FRotator CurrentAimRotation(0.f, GetBaseAimRotation().Yaw, 0.f);
        const FRotator DeltaAimRotation =
            UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
        AO_Yaw = DeltaAimRotation.Yaw;
    }
    // 이동 중이거나 공중이면 기준 회전 갱신 및 AO_Yaw 리셋
    if (Speed > 0 || bIsInAir)
    {
        StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
        AO_Yaw = 0.f;
    }

    // 상하 각도는 그냥 Pitch 사용
    AO_Pitch = GetBaseAimRotation().Pitch;
}


// ─────────────────────────────────────────────
// [권총 조준 토글] 우클릭 가정: UI + FOV + 애님 플래그
// ─────────────────────────────────────────────
void ADoctor::Pistol_Aim(const FInputActionValue& /*inputValue*/)
{


    if (!bUsingPistolGun || bIsSwitchingWeapon || bIsDead) return;

    // ✅ 연타 방지 락 (토글 후 일정 시간 동안 입력 무시)
    if (bIsAimingSwitchLocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Doctor] 🚫 Aim ignored: locked"));
        return;
    }

    bIsAimingSwitchLocked = true;
    GetWorldTimerManager().SetTimer(
        AimLockHandle,
        [this]() { bIsAimingSwitchLocked = false; },
        0.25f, // 0.25초 이내 연타 방지
        false
    );

    UDoctorAnim* anim = Cast<UDoctorAnim>(GetMesh()->GetAnimInstance());
    if (!anim) return;

    // ✅ 상태 토글
    bPistolAim = !bPistolAim;

    if (bPistolAim)
    {
        // 조준 시작
        if (_pistolUI && !_pistolUI->IsInViewport())
        {
            _pistolUI->AddToViewport();
        }

        Player2_CamComp->SetFieldOfView(45.0f);
        anim->Pistol_Zoom_Anim = true;

        UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ Aim ON"));
    }
    else
    {
        // 조준 해제
        if (_pistolUI && _pistolUI->IsInViewport())
        {
            _pistolUI->RemoveFromParent();
        }

        Player2_CamComp->SetFieldOfView(90.0f);
        anim->Pistol_Zoom_Anim = false;

        UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ Aim OFF"));
    }

    // ✅ 위젯 상태 보정: Add/Remove 타이밍 꼬임 방지용 (0.05초 후 재확인)
    GetWorldTimerManager().SetTimer(
        AimUICheckHandle,
        [this]()
        {
            if (bPistolAim)
            {
                if (_pistolUI && !_pistolUI->IsInViewport())
                {
                    _pistolUI->AddToViewport();
                    UE_LOG(LogTemp, Warning, TEXT("[Doctor] 🔁 UI restored (Aim ON)"));
                }
            }
            else
            {
                if (_pistolUI && _pistolUI->IsInViewport())
                {
                    _pistolUI->RemoveFromParent();
                    UE_LOG(LogTemp, Warning, TEXT("[Doctor] 🔁 UI cleared (Aim OFF)"));
                }
            }
        },
        0.05f,
        false
    );
}

// ─────────────────────────────────────────────
// [망치 충돌] BeginOverlap 콜백
// 스윙 타이밍에 켜진 박스가 다른 액터와 겹치면 호출됨
// ─────────────────────────────────────────────
void ADoctor::OnHammerWeaponOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 /*OtherBodyIndex*/, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this) return; // 자기 자신 제외
    if (bHasDealtDamage) return;                   // 스윙당 1회만 데미지

    bHasDealtDamage = true;

    // 스윕이 아닌 경우(또는 HitResult 비정상) 수동으로 보정
    FHitResult HR = SweepResult;
    if (!bFromSweep || !HR.bBlockingHit)
    {
        HR = FHitResult();
        HR.HitObjectHandle = FActorInstanceHandle(OtherActor);
        HR.Component = OtherComp;
        HR.ImpactPoint = HammerCollision->GetComponentLocation();
        HR.Location = HR.ImpactPoint;
    }

    // 적에게 데미지 시도(EnemyFSM 쪽으로 전달)
    UDamageHelper::TryDamageEnemy(HR);
}


// ─────────────────────────────────────────────
// [망치 활성/비활성] 공격 창에만 충돌 켜기
// ─────────────────────────────────────────────
void ADoctor::ActivateHammerWeapon()
{
    bHasDealtDamage = false; // 새 스윙 시작 → 다시 1회 데미지 허용

    // 직전 프레임의 오버랩 상태 초기화 후 On으로 전환(노티파이 없이도 즉시 판정)
    HammerCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HammerCollision->UpdateOverlaps();
    HammerCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HammerCollision->SetGenerateOverlapEvents(true);
}

void ADoctor::DeactivateHammerWeapon()
{
    HammerCollision->SetGenerateOverlapEvents(false);
    HammerCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


// ─────────────────────────────────────────────
// [이동 소음] 일정 속도 이상에서 주기적으로 소음 발생 → 적에게 알림
// ─────────────────────────────────────────────
void ADoctor::UpdateMovementNoise(float DeltaTime)
{
    if (IsCrouched()) // 웅크리면 발소리 없음
    {
        FootstepAccum = 0.f;
        return;
    }

    const FVector V = GetVelocity();
    const float Speed2D = FVector(V.X, V.Y, 0.f).Size();

    if (Speed2D < FootstepMinSpeed) // 너무 느리면 소음 X
    {
        FootstepAccum = 0.f;
        return;
    }

    const UCharacterMovementComponent* Move = GetCharacterMovement();
    const bool bRunning = (Move && Move->MaxWalkSpeed > walkSpeed + 1.f);

    const float Interval = bRunning ? FootstepInterval_Run : FootstepInterval_Walk; // 주기
    const float Loudness = bRunning ? Loudness_Run : Loudness_Walk;                 // 크기

    FootstepAccum += DeltaTime;
    if (FootstepAccum >= Interval)
    {
        FootstepAccum = 0.f;
        BroadcastNoiseToEnemies(Loudness); // 주변 Enemy FSM에 소음 전달
    }
}

// ─────────────────────────────────────────────
// [소음 브로드캐스트] 모든 Enemy 액터를 찾아서 FSM.ReportNoise 호출
// ─────────────────────────────────────────────
void ADoctor::BroadcastNoiseToEnemies(float Loudness)
{
    const FVector NoiseLoc = GetActorLocation();

    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), Enemies);

    for (AActor* A : Enemies)
    {
        if (AEnemy* Enemy = Cast<AEnemy>(A))
        {
            if (UEnemyFSM* FSM = Enemy->FindComponentByClass<UEnemyFSM>())
            {
                FSM->ReportNoise(NoiseLoc, Loudness);
            }
        }
    }
}


// ─────────────────────────────────────────────
// [공격 입력] 좌클릭: 망치면 근접공격, 권총이면 트레이스 사격
// ─────────────────────────────────────────────
void ADoctor::Player2_Fire(const FInputActionValue& /*inputValue*/)
{
    auto anim = Cast<UDoctorAnim>(GetMesh()->GetAnimInstance());






    // 1) 근접 무기(망치)
    if (bUsingHammer)
    {
        // ✅ 이미 공격 중이면 입력 무시
        if (bIsHammerAttacking)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Doctor] ⚠ Hammer attack ignored - still attacking"));
            return;
        }

        // ✅ 망치 애니메이션 Equip 중일 때도 공격 금지
        if (anim && anim->IsAnyMontagePlaying())
        {
            if (anim->Montage_IsPlaying(anim->Play_Hammer_in_hand_Anim(TEXT("hand_in"))) ||
                anim->Montage_IsPlaying(anim->Play_Hammer_in_hand_Anim(TEXT("hand_out"))))
            {
                UE_LOG(LogTemp, Warning, TEXT("[Doctor] 🚫 Fire ignored: Hammer equip/unequip playing"));
                return;
            }
        }

        // ✅ 망치 장착/해제 애니메이션이 진행 중이라면 공격 금지
        if (anim && anim->IsAnyMontagePlaying())
        {
            UAnimMontage* CurrentMontage = anim->GetCurrentActiveMontage();
            if (CurrentMontage)
            {
                const FName MontageName = CurrentMontage->GetFName();

                if (MontageName == FName(TEXT("hand_in")) ||
                    MontageName == FName(TEXT("hand_out")))
                {
                    UE_LOG(LogTemp, Warning, TEXT("[Doctor] 🚫 Fire ignored: Hammer equip/unequip montage"));
                    return;
                }
            }
        }

        bIsHammerAttacking = true; // ✅ 공격 중 상태 ON
        ActivateHammerWeapon();

        int32 index = FMath::RandRange(0, 1);
        FString sectionName = FString::Printf(TEXT("Hammer_attack%d"), index);

        if (anim)
        {
            UAnimMontage* Montage = anim->Play_Hammer_Attack_Anim(FName(*sectionName));

            // ✅ 애니메이션 끝났을 때 공격 상태 해제
            if (Montage)
            {
                FOnMontageEnded EndDelegate;
                EndDelegate.BindUObject(this, &ADoctor::OnHammerAttackEnded);
                anim->Montage_SetEndDelegate(EndDelegate, Montage);
            }
        }

        // 망치 충돌 비활성화 타이머
        FTimerHandle HammerOffTimer;
        GetWorldTimerManager().SetTimer(HammerOffTimer, this, &ADoctor::DeactivateHammerWeapon, 0.35f, false);
        return;
    }

    // 이하 권총 코드 그대로 유지
    if (!bUsingPistolGun)
        return;
    if (bUsingPistolGun && bPistolAim)
    {
        if (anim) anim->PlayhitAnim();
        BroadcastNoiseToEnemies(1.6f);
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), GunFireSound, gunMeshComp->GetSocketLocation(TEXT("FirePosition")));

        const FVector CamLoc = Player2_CamComp->GetComponentLocation();
        const FVector CamDir = Player2_CamComp->GetForwardVector();
        const FVector TraceEnd = CamLoc + CamDir * 5000.f;
        const FVector Muzzle = gunMeshComp->GetSocketLocation(TEXT("FirePosition"));

        FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponTrace), true);
        Params.AddIgnoredActor(this);

        FHitResult WorldHit;
        const bool bHitWorld = GetWorld()->LineTraceSingleByChannel(WorldHit, CamLoc, TraceEnd, ECC_Visibility, Params);

        float WorldHitDist = TNumericLimits<float>::Max();
        if (bHitWorld)
            WorldHitDist = (WorldHit.ImpactPoint - CamLoc).Size();

        FHitResult PawnHit;
        FCollisionObjectQueryParams ObjParams;
        ObjParams.AddObjectTypesToQuery(ECC_Pawn);

        const bool bHitPawn = GetWorld()->LineTraceSingleByObjectType(PawnHit, CamLoc, TraceEnd, ObjParams, Params);

        FHitResult FinalHit;
        bool bUsePawn = false;

        if (bHitPawn)
        {
            const float PawnDist = (PawnHit.ImpactPoint - CamLoc).Size();
            if (PawnDist <= WorldHitDist)
            {
                FinalHit = PawnHit;
                bUsePawn = true;
            }
        }

        if (!bUsePawn && bHitWorld)
            FinalHit = WorldHit;

        if (FinalHit.bBlockingHit)
        {
            DrawDebugLine(GetWorld(), Muzzle, FinalHit.ImpactPoint, FColor::Red, false, 1.f, 0, 1.f);

            if (bulletEffectFactory)
            {
                FTransform FX;
                FX.SetLocation(FinalHit.ImpactPoint);
                UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), bulletEffectFactory, FX);
            }

            if (bUsePawn && FinalHit.GetActor())
                UDamageHelper::TryDamageEnemy(FinalHit);
        }
        else
        {
            DrawDebugLine(GetWorld(), Muzzle, TraceEnd, FColor::Red, false, 1.f, 0, 1.f);
        }
    }
}

// ─────────────────────────────────────────────
// [무기 전환] 권총 토글
// ─────────────────────────────────────────────
void ADoctor::ChangeToPistolGun(const FInputActionValue& /*inputValue*/)
{
    UDoctorAnim* anim = Cast<UDoctorAnim>(GetMesh()->GetAnimInstance());
    if (!anim || bIsSwitchingWeapon) return;

    UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ ChangeToPistolGun | Hammer=%d Pistol=%d"), bUsingHammer, bUsingPistolGun);

    // ✅ 전환 시작: 공격/조준 입력 차단
    bIsSwitchingWeapon = true;

    // ─────────────────────────────
    // 망치 들고 있다면 → 먼저 집어넣기
    // ─────────────────────────────
    if (bUsingHammer)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ Putting away hammer first"));
        bUsingHammer = false;

        if (UAnimMontage* Montage = anim->Play_Hammer_in_hand_Anim(TEXT("hand_out")))
        {
            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &ADoctor::OnHammerPutAwayEnded);
            anim->Montage_SetEndDelegate(EndDelegate, Montage);
            return; // ⚠ 여기서 끝. OnHammerPutAwayEnded에서 이어서 권총 꺼냄
        }
    }

    // ─────────────────────────────
    // 망치가 없으면 바로 권총 토글
    // ─────────────────────────────
    if (bUsingPistolGun)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ Putting pistol away"));
        if (UAnimMontage* Montage = anim->Play_holding_a_pistol_Anim(TEXT("Pistol_off")))
        {
            anim->Pistol_or_Hammer = 0;
            bUsingPistolGun = false;
            anim->Weapon_on_off = false;
            anim->Pistol_on_off = 0;
        }
    }
    else if (bUsingPistolGun == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ Drawing pistol"));
        if (UAnimMontage* Montage = anim->Play_holding_a_pistol_Anim(TEXT("Pistol_on")))
        {
            anim->Pistol_or_Hammer = 0;
            bUsingPistolGun = true;
            anim->Weapon_on_off = true;
            anim->Pistol_on_off = 1;
        }
    }

    // ✅ 애니메이션 없이 바로 토글된 경우 → 즉시 복귀
    bIsSwitchingWeapon = false;
}


// ─────────────────────────────────────────────
// [무기 전환] 망치 토글
// ─────────────────────────────────────────────
void ADoctor::ChangeToMeleeWepon(const FInputActionValue& /*inputValue*/)
{
    if (bIsSwitchingWeapon || bWeaponMontageLock) return;


    UDoctorAnim* anim = Cast<UDoctorAnim>(GetMesh()->GetAnimInstance());
    if (!anim) return;

    UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ ChangeToMeleeWepon | Hammer=%d Pistol=%d"), bUsingHammer, bUsingPistolGun);

    // ✅ 전환 시작
    bIsSwitchingWeapon = true;

    // ─────────────────────────────
    // 권총 들고 있다면 → 먼저 집어넣기
    // ─────────────────────────────
    if (bUsingPistolGun)
    {
        if (UAnimMontage* Montage = anim->Play_holding_a_pistol_Anim(TEXT("Pistol_off")))
        {
            UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ Putting pistol away (Pistol_off)"));
            bWeaponMontageLock = true;
            bUsingPistolGun = false;

            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &ADoctor::OnPistolPutAwayEnded);
            anim->Montage_SetEndDelegate(EndDelegate, Montage);
            return; // ⚠ 콜백에서 이어짐
        }
    }

    // ─────────────────────────────
    // 망치 직접 토글
    // ─────────────────────────────
    if (bUsingHammer)
    {
        if (UAnimMontage* Montage = anim->Play_Hammer_in_hand_Anim(TEXT("hand_out")))
        {
            UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ Hammer put away (hand_out)"));
            bUsingHammer = false;
            anim->Hammer_Default_Anim = false;
        }
    }
    else
    {
        if (UAnimMontage* Montage = anim->Play_Hammer_in_hand_Anim(TEXT("hand_in")))
        {
            UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ Hammer equip (hand_in)"));
            bUsingHammer = true;
            bUsingPistolGun = false;
            anim->Hammer_Default_Anim = true;
            anim->Pistol_or_Hammer = 1;
        }
    }

    // ✅ 애니 없이 직접 토글된 경우 즉시 복귀
    bIsSwitchingWeapon = false;
    bWeaponMontageLock = false;
}


// ─────────────────────────────────────────────
// [콜백] 망치 집어넣기 종료 후 → 권총 꺼내기
// ─────────────────────────────────────────────
void ADoctor::OnHammerPutAwayEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UDoctorAnim* anim = Cast<UDoctorAnim>(GetMesh()->GetAnimInstance());
    if (!anim)
    {
        bIsSwitchingWeapon = false;
        return;
    }

    if (bInterrupted)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Doctor] ❌ HammerPutAway interrupted"));
        bIsSwitchingWeapon = false;
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ HammerPutAwayEnded → Drawing pistol"));

    if (UAnimMontage* PistolMontage = anim->Play_holding_a_pistol_Anim(TEXT("Pistol_on")))
    {
        anim->Pistol_or_Hammer = 0;
        anim->Pistol_on_off = 1;
        anim->Weapon_on_off = true;
        bUsingPistolGun = true;
    }

    // ✅ 전환 완전히 끝난 시점
    bIsSwitchingWeapon = false;
}


// ─────────────────────────────────────────────
// [콜백] 권총 집어넣기 종료 후 → 망치 꺼내기
// ─────────────────────────────────────────────
void ADoctor::OnPistolPutAwayEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UDoctorAnim* anim = Cast<UDoctorAnim>(GetMesh()->GetAnimInstance());
    if (!anim)
    {
        bIsSwitchingWeapon = false;
        bWeaponMontageLock = false;
        return;
    }

    bWeaponMontageLock = false;

    if (bInterrupted)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Doctor] ❌ Pistol_off interrupted"));
        bIsSwitchingWeapon = false;
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ PistolPutAwayEnded → Drawing hammer"));

    if (UAnimMontage* HammerMontage = anim->Play_Hammer_in_hand_Anim(TEXT("hand_in")))
    {
        anim->Pistol_or_Hammer = 1;
        anim->Hammer_Default_Anim = true;
        bUsingHammer = true;
        bUsingPistolGun = false;
    }

    // ✅ 전환 완전히 끝난 시점
    bIsSwitchingWeapon = false;
}

// ─────────────────────────────────────────────
// [총 메시 가시성] 필요 시 외부에서 호출
// ─────────────────────────────────────────────
void ADoctor::change_pistol()
{
    // 권총 사용 중이면 숨김, 아니면 보이기
    gunMeshComp->SetVisibility(!bUsingPistolGun);
}



void ADoctor::InitFootstepAudioComponentDelayed()
{
    if (!FootstepLoopAC) return;

    // 이 캐릭터의 루트(캡슐)에 붙여서 3D 위치 추적되게 함
    if (USceneComponent* RootComp = GetRootComponent())
    {
        FootstepLoopAC->AttachToComponent(
            RootComp,
            FAttachmentTransformRules::KeepRelativeTransform
        );
    }

    // 혹시 아직 등록 안 됐으면 RegisterComponent 호출
    if (!FootstepLoopAC->IsRegistered())
    {
        FootstepLoopAC->RegisterComponent();
    }
}


void ADoctor::StartFootstepLoop()
{
    if (!FootstepLoopAC || !FootstepLoopSound) return;
    if (FootstepLoopAC->IsPlaying()) return; // 이미 재생 중이면 또 켜지지 않게

    // 짧게 페이드 인으로 자연스럽게 시작
    FootstepLoopAC->FadeIn(0.02f);
}

void ADoctor::StopFootstepLoop(float FadeTime /*=0.05f*/)
{
    if (!FootstepLoopAC) return;
    if (!FootstepLoopAC->IsPlaying()) return; // 이미 꺼져있으면 무시

    FootstepLoopAC->FadeOut(FadeTime, 0.f);
}


// 킬카운트 증가 (적이 죽을 때 EnemyFSM 쪽에서 Doctor->AddKill() 호출하게 하면 됨)
void ADoctor::AddKill()
{
    KillCount++;
    UE_LOG(LogTemp, Warning, TEXT("KillCount now: %d"), KillCount);
}

// 언리얼 공식 데미지 루트 (총알이나 근접이 ApplyDamage() 호출하면 여기로 옴)
// BUT: 너는 지금 HealthComp에서 체력 깎는 걸 하고 있으니까
//      여기서도 HealthComp로 포워딩만 해주면 돼.
float ADoctor::TakeDamage(
    float DamageAmount,
    FDamageEvent const& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser
)
{
    const float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // 이미 죽은 상태면 무시
    if (bIsDead) return Actual;

    // 진짜 체력 처리는 HealthComp가 담당
    if (HealthComp)
    {
        HealthComp->ApplyDamage(Actual);
    }

    return Actual;
}

// 체력 0 되었을 때 최종 사망 처리
void ADoctor::HandlePlayerDeath()
{
    if (bIsDead) return;
    if (HealthComp && !HealthComp->IsDead()) return;

    bIsDead = true;

    // 1. 이동만 막기 (입력 완전 차단은 X)
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
    }

    // 2. 카메라/조작은 막되, UI 입력은 남기기
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);
        // PC->DisableInput(PC);  ← ⚠️ 이건 주석처리!!
    }

    // 3. GameMode에게 사망 보고
    if (UWorld* World = GetWorld())
    {
        if (ADoctorGameModeBase* GM = Cast<ADoctorGameModeBase>(UGameplayStatics::GetGameMode(World)))
        {
            GM->OnPlayerDied();
        }
    }
}


void ADoctor::PlayHitEffect()
{
    if (!HitEffectWidgetClass) return;
    if (!HitOverlayInstance) return; // BeginPlay에서 만들어 둔 전체 캔버스 위젯

    // 1) 새로 뜰 "상처/피격" 위젯 생성
    UUserWidget* NewHit = CreateWidget<UUserWidget>(GetWorld(), HitEffectWidgetClass);
    if (!NewHit) return;

    // 2) 오버레이(전체화면 캔버스)의 루트가 CanvasPanel이어야 한다
    UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(HitOverlayInstance->GetRootWidget());
    if (!RootCanvas) return;

    // 3) 캔버스에 자식으로 추가하고 슬롯 얻기
    UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(NewHit);
    if (!CanvasSlot) return;

    // 화면 중앙 계산
    FVector2D ViewportSize(1920.f, 1080.f);
    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->GetViewportSize(ViewportSize);
    }

    const float CenterX = ViewportSize.X * 0.5f;
    const float CenterY = ViewportSize.Y * 0.5f;

    CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
    CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // 위젯 중앙이 좌표에 오도록
    CanvasSlot->SetAutoSize(true);
    CanvasSlot->SetPosition(FVector2D(0.f, 0.f));

    // 8) 블프 이벤트 "PlayHitEffect" 실행해서
    //    - Visible
    //    - Opacity 1 -> Delay -> FadeOut -> Hidden
    static const FName FuncName_PlayHitEffect(TEXT("PlayHitEffect"));
    if (UFunction* Func = NewHit->FindFunction(FuncName_PlayHitEffect))
    {
        NewHit->ProcessEvent(Func, nullptr);
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->PlayerCameraManager->StartCameraShake(HitShakeClass, 0.3f);
    }

    // 9) 안전하게 삭제 타이머 예약
    //    GC로 사라졌을 수도 있으니까 WeakPtr로 잡는다
    TWeakObjectPtr<UUserWidget> WeakHitWidget = NewHit;

    FTimerHandle TempHandle;
    GetWorldTimerManager().SetTimer(
        TempHandle,
        [WeakHitWidget]()
        {
            if (WeakHitWidget.IsValid())
            {
                WeakHitWidget->RemoveFromParent();
            }
        },
        2.3f,   // BP 애니 끝나는 타이밍에 맞춰서 조정 가능
        false
    );
}


void ADoctor::OnHammerAttackEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (bInterrupted)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Doctor] ❌ Hammer attack montage interrupted"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Doctor] ✅ Hammer attack finished"));
    }

    bIsHammerAttacking = false; // ✅ 다시 공격 가능
}


void ADoctor::StartPistolAim(const FInputActionValue& /*inputValue*/)
{
    if (!bUsingPistolGun || bIsSwitchingWeapon || bIsDead) return;
    if (bIsHammerAttacking) return;

    UDoctorAnim* anim = Cast<UDoctorAnim>(GetMesh()->GetAnimInstance());
    if (!anim) return;

    // 이미 조준 중이면 무시
    if (bPistolAim) return;

    bPistolAim = true;

    if (_pistolUI && !_pistolUI->IsInViewport())
        _pistolUI->AddToViewport();

    Player2_CamComp->SetFieldOfView(45.0f);
    anim->Pistol_Zoom_Anim = true;

    UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ Aim ON (Pressed)"));
}


void ADoctor::StopPistolAim(const FInputActionValue& /*inputValue*/)
{
    if (!bUsingPistolGun || bIsDead) return;

    UDoctorAnim* anim = Cast<UDoctorAnim>(GetMesh()->GetAnimInstance());
    if (!anim) return;

    // 이미 비조준 상태면 무시
    if (!bPistolAim) return;

    bPistolAim = false;

    if (_pistolUI && _pistolUI->IsInViewport())
        _pistolUI->RemoveFromParent();

    Player2_CamComp->SetFieldOfView(90.0f);
    anim->Pistol_Zoom_Anim = false;

    UE_LOG(LogTemp, Warning, TEXT("[Doctor] ▶ Aim OFF (Released)"));
}
