#include "FamilyLockActor.h"                 // 자신의 헤더
#include "LockDialComponent.h"               // 각 다이얼(글자 휠)을 담당하는 커스텀 컴포넌트
#include "DrawerActor.h"                     // 연동되는 서랍(자물쇠 해제 시 열림)
#include "Police.h"                          // 플레이어(경찰) 캐릭터 타입
#include "Camera/CameraComponent.h"          // 점검(Inspect)용 카메라
#include "Kismet/GameplayStatics.h"          // 사운드 재생/플레이어 컨트롤러 접근 등 유틸
#include "GameFramework/PlayerController.h"  // 마우스/입력 모드 변경 등
#include "DrawDebugHelpers.h"                // 디버그용 선/구체 그리기
#include "InteractionInterface.h"
//================1105소은 추가======
#include "PoliceMemoWidget.h"               // 메모 위젯(자물쇠 해제 시 갱신)
#include "DialogWidget.h"                   // 대화 위젯
//==================================




AFamilyLockActor::AFamilyLockActor()
{
    // 매 프레임 Tick()을 호출할지 여부. 점검 중 마우스 호버 감지 등에 필요
    PrimaryActorTick.bCanEverTick = true;

    // 잠금(자물쇠) 메인 메시 컴포넌트 생성 → 루트로 설정
    LockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LockMesh"));
    RootComponent = LockMesh;

    // 상호작용 하이라이트용 커스텀 뎁스를 기본 끔 (전체 화면이 노랗게 보이는 문제 방지)
    LockMesh->SetRenderCustomDepth(false);

    // ─────────────────────────────────────────────────────────────────────────
    // 인스펙트(근접 관찰) 모드에서 사용할 전용 카메라 컴포넌트 생성
    // ─────────────────────────────────────────────────────────────────────────
    InspectCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InspectCamera"));
    InspectCamera->SetupAttachment(RootComponent);           // 자물쇠에 붙임
    InspectCamera->SetRelativeLocation(FVector(45.f, 0.f, 8.f)); // 자물쇠 앞/약간 위
    InspectCamera->SetRelativeRotation(FRotator(-5.f, 180.f, 0.f)); // 살짝 아래로, 뒤를 보듯 180도(Yaw) 회전

    // 키피벗(모든 다이얼의 기준점). 자물쇠 루트에 부착
    KeyPivot = CreateDefaultSubobject<USceneComponent>(TEXT("KeyPivot"));
    KeyPivot->SetupAttachment(RootComponent);
    KeyPivot->SetRelativeLocation(FVector(0.f, 7.2f, 0.f));  // 살짝 앞으로(양의 Y)

    // 다이얼(문자 휠) 개수 정의. 여기선 6글자(FAMILY) 고정
    const int32 DialCount = 6;
    DialPivots.Reserve(DialCount); // 피벗 배열 메모리 미리 확보
    Dials.Reserve(DialCount);      // 다이얼 배열 메모리 미리 확보

    // 다이얼 개수만큼 피벗 + 다이얼 컴포넌트 생성/부착
    for (int32 i = 0; i < DialCount; ++i)
    {
        // 각 다이얼의 회전/정렬 기준이 되는 피벗(SceneComponent)
        const FString PivotName = FString::Printf(TEXT("DialPivot_%d"), i);
        USceneComponent* DialPivot = CreateDefaultSubobject<USceneComponent>(*PivotName);
        DialPivot->SetupAttachment(KeyPivot);                 // 모든 피벗은 KeyPivot 밑으로
        DialPivot->SetRelativeLocation(FVector::ZeroVector);  // 상대 위치/회전 초기화
        DialPivot->SetRelativeRotation(FRotator::ZeroRotator);
        DialPivots.Add(DialPivot);

        // 실제 글자 휠을 표현/로직을 처리할 커스텀 컴포넌트
        const FString DialName = FString::Printf(TEXT("Dial_%d"), i);
        ULockDialComponent* Dial = CreateDefaultSubobject<ULockDialComponent>(*DialName);
        Dial->SetupAttachment(DialPivot);                     // 다이얼은 자신의 피벗에 부착
        Dial->SetRelativeLocation(FVector(0, -7.2f, 0));
        Dial->SetRelativeRotation(FRotator::ZeroRotator);
        Dials.Add(Dial);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 인스펙트 전용 조명. 카메라에 붙여 자물쇠를 보기 좋게 비춤
    // ─────────────────────────────────────────────────────────────────────────
    InspectLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("InspectLight"));
    InspectLight->SetupAttachment(InspectCamera);            // 카메라에 부착 → 화면과 함께 이동
    InspectLight->SetRelativeLocation(FVector(15.f, 0.f, 0.f));
    InspectLight->SetIntensity(100.f);                       // 적당한 밝기
    InspectLight->SetLightColor(FLinearColor(1.f, 0.9f, 0.75f)); // 따뜻한 톤
    InspectLight->bUseInverseSquaredFalloff = false;         // Falloff을 선형/가중 방식으로
    InspectLight->SetAttenuationRadius(200.f);               // 빛의 유효 반경
    InspectLight->SetVisibility(false);                      // 기본은 꺼 둠(인스펙트 진입 시 켬)

    UE_LOG(LogTemp, Warning, TEXT("[Lock] Dials created: %d"), Dials.Num());


    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComponent);

    // 크기 조절 (필요에 따라 변경)
    InteractionBox->SetBoxExtent(FVector(50.f, 50.f, 50.f));

    // 라인트레이스만 맞게 설정
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); //라인트레이스는 Visibility 채널 사용
    InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

//AFamilyLockActor::AFamilyLockActor()
//{
//    PrimaryActorTick.bCanEverTick = true;
//
//    // 공통 루트 추가
//    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
//    RootComponent = SceneRoot;
//
//    // 자물쇠 외형 메쉬
//    LockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LockMesh"));
//    LockMesh->SetupAttachment(SceneRoot);
//    LockMesh->SetRenderCustomDepth(false);
//    LockMesh->SetRelativeLocation(FVector(0.f, -7.2f, 0.f)); // 🔸 메쉬만 뒤로 보정
//
//    // 인스펙트 카메라
//    InspectCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InspectCamera"));
//    InspectCamera->SetupAttachment(SceneRoot);
//    InspectCamera->SetRelativeLocation(FVector(45.f, 0.f, 8.f));
//    InspectCamera->SetRelativeRotation(FRotator(-5.f, 180.f, 0.f));
//
//    // 다이얼 피벗 기준축 (절대 그대로!)
//    KeyPivot = CreateDefaultSubobject<USceneComponent>(TEXT("KeyPivot"));
//    KeyPivot->SetupAttachment(SceneRoot);
//    KeyPivot->SetRelativeLocation(FVector(0.f, 7.2f, 0.f));  // ✅ 피벗 그대로 유지
//
//    // 다이얼 생성
//    const int32 DialCount = 6;
//    DialPivots.Reserve(DialCount);
//    Dials.Reserve(DialCount);
//
//    for (int32 i = 0; i < DialCount; ++i)
//    {
//        const FString PivotName = FString::Printf(TEXT("DialPivot_%d"), i);
//        USceneComponent* DialPivot = CreateDefaultSubobject<USceneComponent>(*PivotName);
//        DialPivot->SetupAttachment(KeyPivot);
//        DialPivot->SetRelativeLocation(FVector::ZeroVector);
//        DialPivot->SetRelativeRotation(FRotator::ZeroRotator);
//        DialPivots.Add(DialPivot);
//
//        const FString DialName = FString::Printf(TEXT("Dial_%d"), i);
//        ULockDialComponent* Dial = CreateDefaultSubobject<ULockDialComponent>(*DialName);
//        Dial->SetupAttachment(DialPivot);
//        Dials.Add(Dial);
//    }
//
//    InspectLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("InspectLight"));
//    InspectLight->SetupAttachment(InspectCamera);
//    InspectLight->SetRelativeLocation(FVector(15.f, 0.f, 0.f));
//    InspectLight->SetIntensity(100.f);
//    InspectLight->SetLightColor(FLinearColor(1.f, 0.9f, 0.75f));
//    InspectLight->bUseInverseSquaredFalloff = false;
//    InspectLight->SetAttenuationRadius(200.f);
//    InspectLight->SetVisibility(false);
//}


void AFamilyLockActor::BeginPlay()
{
    Super::BeginPlay();

#if !WITH_EDITOR
    // 🔹 패키징(런타임) 빌드일 때만 다이얼 메시 위치 보정
    for (ULockDialComponent* Dial : Dials)
    {
        if (!Dial) continue;

        FVector Loc = Dial->GetRelativeLocation();
        Loc.Y = -7.2f; // 다이얼 외형만 뒤로 밀기
        Dial->SetRelativeLocation(Loc);

        UE_LOG(LogTemp, Warning, TEXT("[LockRuntimeFix] Dial %s Y=-7.2 applied"), *Dial->GetName());
    }
#endif

    // 🔸 나머지 기존 코드 유지
    Target = Target.ToUpper();
    if (Target.Len() != 6)
        Target = TEXT("FAMILY");

    UE_LOG(LogTemp, Warning, TEXT("[Lock] Target password set to: %s"), *Target);
}

void AFamilyLockActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 인스펙트 중이며 아직 해제되지 않았다면
    if (bInspecting && !bUnlocked)
    {
        UpdateHoverUnderMouse();

        // 🔹 E 키로 인스펙트 종료 가능하게 처리
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            const bool bPressed = PC->IsInputKeyDown(EKeys::E);

            // 새로 눌림 감지 (E키가 처음 눌렸을 때만)
            if (bPressed && !bCloseKeyPressed)
            {
                bCloseKeyPressed = true;
                UE_LOG(LogTemp, Warning, TEXT("[Lock] E pressed during Inspect → ExitInspect()"));
                ExitInspect();
            }
            else if (!bPressed)
            {
                bCloseKeyPressed = false;
            }
        }
    }



}

void AFamilyLockActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // 액터가 에디터에서 배치/값 변경될 때마다 호출.
    // 여기서 재정렬하면 뷰포트에서 위치가 즉시 반영됨.
    RecenterDialsToKeyPivot();
}

void AFamilyLockActor::Interact(APolice* _PlayerCharacter)
{
    //UE_LOG(LogTemp, Warning, TEXT("[Lock] Interact() called"));

     // ✅ 방금 닫은 경우에는 0.4초간 무시
    if (bRecentlyClosed && (GetWorld()->GetTimeSeconds() - ClosedTime) < 0.4f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Lock] Interaction ignored (recently closed)"));
        return;
    }
    else
    {
        bRecentlyClosed = false;
    }

    //=========================================================================소은===============
    if (bHasBeenInspected == false)
    {
        if (DialogWidgetClass)
        {
            DialogWidget = CreateWidget<UDialogWidget>(GetWorld(), DialogWidgetClass);
            if (DialogWidget)
            {

                FTimerHandle WidgetOn;
                GetWorldTimerManager().SetTimer(
                    WidgetOn,
                    [this]()
                    {
                        DialogWidget->AddToViewport();
                        FDialogLine LockLine(TEXT("\n\n\n<Blue>암호는 여섯 글자의 단어인가..?</>"), TEXT(""));
                        DialogWidget->UpdateDialog(LockLine);
                    },
                    4.0f,
                    false
                );

                FTimerHandle WidgetOff;
                GetWorldTimerManager().SetTimer(
                    WidgetOff,
                    [this]()
                    {
                        if (DialogWidget)
                            DialogWidget->SetVisibility(ESlateVisibility::Hidden);
                    },
                    8.0f,
                    false
                );
            }
        }
        bHasBeenInspected = true;
    }//최초1회 상호작용시 dialogwidget 출력

    APolice* PoliceCharacter = Cast<APolice>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (PoliceCharacter)
    {
        PoliceCharacter->UnlockMemoWrap(6); //메모 해금
    }
    //========================================================================================
    // 
    // 


    // 이미 해제된 상태라면 상호작용 무시
    if (bUnlocked)
    {

        return;
    }

    // 인스펙트 중이라면 종료(토글 형태)
    if (bInspecting)
    {

        ExitInspect();
        return;
    }

    // 인스펙트 진입
    EnterInspect(_PlayerCharacter);
}

// ──────────────────────────────────────────────────────────────
// [추가] 인스펙트 진입/종료 시 아웃라인 제어
// ──────────────────────────────────────────────────────────────

// 인스펙트 진입 시: 모든 테두리(커스텀 뎁스) OFF
void AFamilyLockActor::EnterInspect(APolice* Police)
{
    // ?? 기존 테두리 OFF 코드 유지
    {
        TArray<UStaticMeshComponent*> MeshComponents;
        GetComponents<UStaticMeshComponent>(MeshComponents);
        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (!MeshComp) continue;
            MeshComp->SetRenderCustomDepth(false);
        }
    }

    // 기존 코드 그대로 ↓
    if (!Police)
    {
        UE_LOG(LogTemp, Error, TEXT("[Lock] EnterInspect() - Police is null!"));
        return;
    }

    if(InteractionBox)
    {
        InteractionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    CachedPolice = Police;
    Police->CurrentLockActor = this;

    if (APlayerController* PC = Cast<APlayerController>(Police->GetController()))
    {
        PrevViewTarget = PC->GetViewTarget();
        PC->SetViewTargetWithBlend(this, 0.3f);
        PC->bShowMouseCursor = true;
        PC->bEnableMouseOverEvents = true;
        PC->bEnableClickEvents = true;

        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Mode);

        // ?? 마우스 휠 힌트 UI 생성
        if (WheelHintWidgetClass)
        {
            WheelHintWidget = CreateWidget<UUserWidget>(PC, WheelHintWidgetClass);
            if (WheelHintWidget)
            {
                WheelHintWidget->AddToViewport(200);
                UE_LOG(LogTemp, Warning, TEXT("[Lock UI] Wheel hint widget shown"));
            }
        }
    }

    bInspecting = true;
    UE_LOG(LogTemp, Warning, TEXT("[Lock] Inspect mode entered."));

    if (InspectLight)
        InspectLight->SetVisibility(true);
}



// 인스펙트 종료 시: 테두리 복원 OFF 상태로 초기화
void AFamilyLockActor::ExitInspect()
{
    UE_LOG(LogTemp, Warning, TEXT("[Lock] ExitInspect() called."));

    // ?? 마우스 휠 힌트 UI 제거
    if (WheelHintWidget)
    {
        WheelHintWidget->RemoveFromParent();
        WheelHintWidget = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("[Lock UI] Wheel hint widget removed"));
    }

    // ?? 기존 하이라이트 OFF 유지
    {
        TArray<UStaticMeshComponent*> MeshComponents;
        GetComponents<UStaticMeshComponent>(MeshComponents);
        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (!MeshComp) continue;
            MeshComp->SetRenderCustomDepth(false);
        }
    }

    // 나머지 기존 코드 그대로 ↓
    if (CachedPolice.IsValid())
    {
        CachedPolice->CurrentLockActor = nullptr;
        CachedPolice = nullptr;
    }

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->bShowMouseCursor = false;
        PC->bEnableMouseOverEvents = false;
        PC->bEnableClickEvents = false;

        FInputModeGameOnly Mode;
        PC->SetInputMode(Mode);

        if (PrevViewTarget.IsValid())
            PC->SetViewTargetWithBlend(PrevViewTarget.Get(), 0.25f);
    }

    HoverDial.Reset();
    bInspecting = false;

    if (InspectLight)
        InspectLight->SetVisibility(false);

    // ✅ 닫은 직후 재입력 방지 타이머 설정
    bRecentlyClosed = true;
    ClosedTime = GetWorld()->GetTimeSeconds();


    if (InteractionBox)
    {
        InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }



}


void AFamilyLockActor::UpdateHoverUnderMouse()
{
    // 현재 로컬 플레이어 컨트롤러 취득
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    // 현재 마우스 위치 가져오기(스크린 좌표)
    float X, Y;
    PC->GetMousePosition(X, Y);

    // 마우스 아래로 가시성 채널 트레이스하여 어떤 컴포넌트를 가리키는지 확인
    FHitResult Hit;
    if (PC->GetHitResultAtScreenPosition(FVector2D(X, Y), ECC_Visibility, true, Hit))
    {
        // 직접 맞은 컴포넌트가 다이얼인지 캐스팅
        ULockDialComponent* NewHover = Cast<ULockDialComponent>(Hit.GetComponent());

        // 다이얼이 아닐 경우, 부모가 다이얼인지(다이얼 메시 등)도 함께 체크
        if (!NewHover && Hit.GetComponent())
        {
            ULockDialComponent* ParentDial = Hit.GetComponent()->GetAttachParent()
                ? Cast<ULockDialComponent>(Hit.GetComponent()->GetAttachParent())
                : nullptr;

            if (ParentDial)
                NewHover = ParentDial;
        }

        // 호버 대상이 바뀌었으면 이전 하이라이트 해제 → 새 하이라이트 적용
        if (HoverDial.Get() != NewHover)
        {
            if (HoverDial.IsValid()) HoverDial->SetHighlight(false);
            HoverDial = NewHover;
            if (HoverDial.IsValid())
            {
                HoverDial->SetHighlight(true);
                UE_LOG(LogTemp, Warning, TEXT("[Lock] Hovering dial: %s"), *HoverDial->GetName());
            }
        }
    }
    else if (HoverDial.IsValid())
    {
        // 더 이상 호버 대상이 없으면 하이라이트 해제
        HoverDial->SetHighlight(false);
        HoverDial.Reset();
    }
}

void AFamilyLockActor::OnWheelAxis(float Value)
{
    // 🔸 이미 해제된 상태면 조작 불가
    if (bUnlocked)
        return;

    // 인스펙트 중이 아닐 때는 무시
    if (!bInspecting || FMath::IsNearlyZero(Value))
        return;

    if (!HoverDial.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Lock] OnWheelAxis() - No dial under mouse."));
        return;
    }

    // 휠 방향 교정 (위로 굴릴 때 값이 반대로 나오면 - 부호 조정)
    const int32 Step = (Value > 0.f) ? -1 : +1;

    HoverDial->RotateDialByStep(Step);

    UE_LOG(LogTemp, Warning, TEXT("[Lock] Wheel %.2f → Step %d | Dial=%s"),
        Value, Step, *HoverDial->GetName());

    CheckSolved();
}

//void AFamilyLockActor::OnWheelAxis(float Value)
//{
//    // 인스펙트 중이 아니면 무시
//    if (!bInspecting || FMath::IsNearlyZero(Value))
//        return;
//
//    if (!HoverDial.IsValid())
//    {
//        UE_LOG(LogTemp, Warning, TEXT("[Lock] OnWheelAxis() -> No dial under mouse."));
//        return;
//    }
//
//    // ?? Unreal의 마우스 휠은 OS/엔진 설정에 따라 반전될 수 있음
//    // 일반적으로 위로 스크롤 → Value > 0
//    //             아래로 스크롤 → Value < 0
//    // 지금 네 경우처럼 반대로 나오면 아래 라인을 바꿔가며 테스트하면 됨.
//
//    int32 Step = 0;
//    if (Value > 0.f)
//    {
//        Step = +1; // 휠 위로 → 다음 글자 (예: G → R)
//        UE_LOG(LogTemp, Warning, TEXT("?? Wheel UP (+1)"));
//    }
//    else if (Value < 0.f)
//    {
//        Step = -1; // 휠 아래로 → 이전 글자 (예: G → F)
//        UE_LOG(LogTemp, Warning, TEXT("?? Wheel DOWN (-1)"));
//    }
//
//    if (HoverDial.IsValid())
//    {
//        HoverDial->StepByScroll(Step);
//
//        UE_LOG(LogTemp, Warning, TEXT("[Lock] Wheel input: %.2f (Step=%d)"), Value, Step);
//    }
//}


void AFamilyLockActor::CheckSolved()
{
    FString Now;
    FString Indexes;

    // ?? 현재 다이얼 상태를 모두 문자열로 합치기
    for (int32 i = 0; i < Dials.Num(); ++i)
    {
        if (!Dials[i]) continue;

        // 다이얼의 현재 문자 얻기 (눈금 기준 정확한 알파벳)
        TCHAR CurrentChar = Dials[i]->GetCurrentLetter();
        Now.AppendChar(CurrentChar);

        Indexes += FString::Printf(TEXT("[%d:%d] "), i, Dials[i]->CurrentIndex);
    }

    UE_LOG(LogTemp, Warning, TEXT("[Lock Debug] Current Input: %s | Indexes: %s"), *Now, *Indexes);

    // ?? 비밀번호 일치 검사
    if (Now.Equals(Target, ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Lock Debug] ? Correct Password! Unlocking..."));
        UnlockAndDrop();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[Lock Debug] ? Not matched yet."));
    }

    // ?? 에디터 디버그용 시각 확인
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 2.f, FColor::Cyan,
            FString::Printf(TEXT("Lock Input: %s / Target: %s"), *Now, *Target)
        );
    }
}

void AFamilyLockActor::UnlockAndDrop()
{
    // ✅ 정답 상태로 설정
    bUnlocked = true;

    // 🔹 다이얼 하이라이트 해제
    for (auto* Dial : Dials)
        if (Dial)
            Dial->SetHighlight(false);

    // 🔹 정답 단어 표시
    FString SolvedWord;
    for (ULockDialComponent* Dial : Dials)
        if (Dial)
            SolvedWord.AppendChar(Dial->GetCurrentLetter());

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 3.f, FColor::Green,
            FString::Printf(TEXT("✅ Correct! The word is \"%s\""), *SolvedWord)
        );
    }

    // 🔹 위젯 제거 (ExitInspect 생략으로 직접 처리)
    if (WheelHintWidget)
    {
        WheelHintWidget->RemoveFromParent();
        WheelHintWidget = nullptr;
    }
    if (DialogWidget)
    {
        DialogWidget->RemoveFromParent();
        DialogWidget = nullptr;
    }

    // 🔹 플레이어 컨트롤러 입력 잠금
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);

        // 커서 유지 (지금은 인스펙트 상태 유지)
        PC->bShowMouseCursor = true;

        // 인풋모드 GameAndUI → UI포커스 고정 (움직이지 않음)
        FInputModeUIOnly Mode;
        PC->SetInputMode(Mode);
    }

    // 🔹 “찰칵” 사운드 재생
    if (UnlockSound)
        UGameplayStatics::PlaySoundAtLocation(this, UnlockSound, GetActorLocation());

    // 🔹 일정 시간 대기 후 페이드 시작
    FTimerHandle FadeTimer;
    GetWorldTimerManager().SetTimer(
        FadeTimer,
        [this]()
        {
            if (!LockMesh) return;

            UMaterialInstanceDynamic* DynMat = LockMesh->CreateAndSetMaterialInstanceDynamic(0);
            if (!DynMat) return;

            DynMat->SetScalarParameterValue(FName("Opacity"), 1.0f);

            const float FadeDuration = 1.5f;
            const int32 Steps = 25;
            const float StepTime = FadeDuration / Steps;

            for (int32 i = 0; i <= Steps; ++i)
            {
                const float Alpha = 1.0f - (float(i) / Steps);
                FTimerHandle OpacityHandle;
                GetWorldTimerManager().SetTimer(
                    OpacityHandle,
                    [DynMat, Alpha]()
                    {
                        if (DynMat)
                            DynMat->SetScalarParameterValue(FName("Opacity"), Alpha);
                    },
                    StepTime * i,
                    false
                );
            }

            // 🔹 페이드 종료 후 카메라 복귀 + 입력 복원 + 서랍 열기
            FTimerHandle EndHandle;
            GetWorldTimerManager().SetTimer(
                EndHandle,
                [this]()
                {
                    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
                    if (PC)
                    {
                        // 커서 끄기 + 입력 모드 복귀
                        PC->bShowMouseCursor = false;
                        FInputModeGameOnly GameMode;
                        PC->SetInputMode(GameMode);

                        // 입력 다시 허용
                        PC->SetIgnoreMoveInput(false);
                        PC->SetIgnoreLookInput(false);

                        // 카메라 부드럽게 복귀
                        if (PrevViewTarget.IsValid())
                        {
                            PC->SetViewTargetWithBlend(
                                PrevViewTarget.Get(),
                                1.2f,
                                EViewTargetBlendFunction::VTBlend_Cubic
                            );
                        }
                    }

                    // 서랍 열기
                    if (LinkedDrawer)
                    {
                        LinkedDrawer->OpenDrawer();
                        UE_LOG(LogTemp, Warning, TEXT("[Lock] Drawer opened after fade."));
                    }

                    // 자물쇠 제거
                    Destroy();
                },
                FadeDuration + 0.2f,
                false
            );
        },
        1.2f, // 찰칵 후 잠시 멈춤
        false
    );
}


void AFamilyLockActor::RecenterDialsToKeyPivot()
{
    // 다이얼 간격/기준 Y
    const float StepX = 0.9f;
    const float PivotY = 7.2f;
    const int32 DialCount = Dials.Num();

    // 피벗/다이얼 개수가 안 맞으면 안전 종료
    if (DialPivots.Num() != DialCount) return;

    // 중앙 정렬: 왼쪽 끝 시작점 계산(등간격 배치)
    const float StartX = -StepX * (DialCount - 1) * 0.5f;

    for (int32 i = 0; i < DialCount; ++i)
    {
        if (!DialPivots[i]) continue;

#if WITH_EDITOR
        // 에디터에서는 미리보기를 위해 즉시 새로고침
        DialPivots[i]->Modify();
        DialPivots[i]->MarkRenderStateDirty();
        DialPivots[i]->ReregisterComponent();
#else
        // 🔸 기존 코드를 완전히 없애진 않고, “BP에서 따로 세팅하지 않은 경우만” 정렬 적용
        FVector CurrentLoc = DialPivots[i]->GetRelativeLocation();
        FRotator CurrentRot = DialPivots[i]->GetRelativeRotation();

        const bool bHasCustomLocation = !CurrentLoc.IsNearlyZero(0.01f);
        const bool bHasCustomRotation = !CurrentRot.IsNearlyZero(0.01f);

        if (!bHasCustomLocation && !bHasCustomRotation)
        {
            // ➕ 블루프린트에서 조정하지 않은 경우에만 C++ 기본 위치로 정렬
            FVector LocalPos(StartX + StepX * i, PivotY, 0.f);
            DialPivots[i]->SetRelativeLocation(LocalPos);
            DialPivots[i]->SetRelativeRotation(FRotator(0.0f, 0.0f, 60.0f));
        }
#endif
    }
}

bool AFamilyLockActor::IsInspecting() const
{
    // 외부(예: UI/플레이어 로직)에서 현재 인스펙트 상태 질의할 때 사용
    return bInspecting;
}


// ─────────────────────────────────────────────────────────────────────────────
// [?? 추가] 상호작용 인터페이스용 포커스 기능
// ─────────────────────────────────────────────────────────────────────────────

void AFamilyLockActor::BeginFocus()
{
    UE_LOG(LogTemp, Warning, TEXT("[Lock Focus] BeginFocus() called - Outline ON"));

    // 액터의 모든 StaticMeshComponent를 찾아서 하이라이트 ON
    TArray<UStaticMeshComponent*> MeshComponents;
    GetComponents<UStaticMeshComponent>(MeshComponents);

    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (!MeshComp) continue;
        MeshComp->SetRenderCustomDepth(true);
        MeshComp->SetCustomDepthStencilValue(1);
    }
}

void AFamilyLockActor::EndFocus()
{
    UE_LOG(LogTemp, Warning, TEXT("[Lock Focus] EndFocus() called - Outline OFF"));

    // 액터의 모든 StaticMeshComponent 하이라이트 OFF
    TArray<UStaticMeshComponent*> MeshComponents;
    GetComponents<UStaticMeshComponent>(MeshComponents);

    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (!MeshComp) continue;
        MeshComp->SetRenderCustomDepth(false);
    }
}
