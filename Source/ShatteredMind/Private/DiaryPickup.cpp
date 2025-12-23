// DiaryPickup.cpp
#include "DiaryPickup.h"
#include "DiaryWidget.h"
#include "Doctor.h"
#include "Police.h"
#include "DoctorHUD.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "PoliceGameModeBase.h"
#include "PoliceMonologueWidget.h" // 20251107 소은 경찰 독백

DEFINE_LOG_CATEGORY_STATIC(LogDiaryPickup, Log, All);

#define DP_LOG(Verbosity, Format, ...) UE_LOG(LogDiaryPickup, Verbosity, TEXT("[%s] " Format), TEXT(__FUNCTION__), ##__VA_ARGS__)
#define DP_SCREEN(Color, Format, ...) if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 2.f, Color, FString::Printf(TEXT(Format), ##__VA_ARGS__));}

// ──────────────────────────────────────────────
//  생성자
// ──────────────────────────────────────────────
ADiaryPickup::ADiaryPickup()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Mesh->SetRenderCustomDepth(false);
    Mesh->SetCustomDepthStencilValue(1);

    bIsInteracting = false;
    CurrentLineIndex = 0;

 
    PrimaryActorTick.bCanEverTick = true; // 반드시 활성화
    PrimaryActorTick.TickInterval = 0.05f; // 20FPS 주기 (0.05초마다)


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

// ──────────────────────────────────────────────
//  BeginPlay
// ──────────────────────────────────────────────
void ADiaryPickup::BeginPlay()
{
    Super::BeginPlay();
    DP_LOG(Log, TEXT("BeginPlay - %s"), *GetName());

    if (GetWorld())
        GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

// ──────────────────────────────────────────────
//  포커스 (노란 테두리)
// ──────────────────────────────────────────────
void ADiaryPickup::BeginFocus()
{
    if (Mesh)
    {
        Mesh->SetRenderCustomDepth(true);
        Mesh->SetCustomDepthStencilValue(1);
        DP_LOG(Log, TEXT("Outline ON (Focus)"));
    }
}

void ADiaryPickup::EndFocus()
{
    if (Mesh && !bIsInteracting)
    {
        Mesh->SetRenderCustomDepth(false);
        DP_LOG(Log, TEXT("Outline OFF (Focus Lost)"));
    }
}

// ──────────────────────────────────────────────
//  상호작용 기본 함수
// ──────────────────────────────────────────────
void ADiaryPickup::BeginInteract()
{
    DP_LOG(Log, TEXT("BeginInteract called (no state change)"));
}

void ADiaryPickup::EndInteract()
{
    DP_LOG(Log, TEXT("EndInteract called (closing UI)"));

    // ──────────────────────────────────────────────
    // 위젯 정리
    // ──────────────────────────────────────────────
    if (IsValid(DiaryWidget))
    {
        if (DiaryWidget->IsInViewport())
        {
            DiaryWidget->RemoveFromParent();
            DP_LOG(Log, TEXT("DiaryWidget removed from viewport"));
        }

        DiaryWidget->SetVisibility(ESlateVisibility::Hidden);
        DiaryWidget->Destruct();
        DiaryWidget = nullptr;
    }

    // 상태 리셋
    bIsInteracting = false;
    CurrentLineIndex = 0;

    // 입력 복구
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->SetIgnoreLookInput(false);
        PC->SetIgnoreMoveInput(false);
        PC->bShowMouseCursor = false;

        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
        FSlateApplication::Get().ResetToDefaultPointerInputSettings();
    }

    DP_SCREEN(FColor::Cyan, "Diary CLOSE (FULL DESTRUCT)");
    DP_LOG(Log, TEXT("Diary fully closed and input restored."));

    // 최근 닫힘 처리
    bRecentlyClosed = true;
    ClosedTime = GetWorld()->GetTimeSeconds();

    // ──────────────────────────────────────────────
    // ✅ 안전한 타이머 실행 (WeakThis 사용)
    // ──────────────────────────────────────────────
    TWeakObjectPtr<ADiaryPickup> WeakThis(this);

    // (1) 1초 후 독백 위젯 생성
    FTimerHandle TimerHandle_OpenMonologue;
    GetWorldTimerManager().SetTimer(
        TimerHandle_OpenMonologue,
        [WeakThis]()
        {
            if (!WeakThis.IsValid()) return;
            WeakThis->ShowPoliceMonologueEnding();
        },
        1.0f,
        false
    );

    // (2) 6초 후 독백 위젯 제거
    FTimerHandle TimerHandle_CloseMonologue;
    GetWorldTimerManager().SetTimer(
        TimerHandle_CloseMonologue,
        [WeakThis]()
        {
            if (!WeakThis.IsValid()) return;

            if (IsValid(WeakThis->PoliceMonologueWidget) && WeakThis->PoliceMonologueWidget->IsInViewport())
            {
                WeakThis->PoliceMonologueWidget->RemoveFromParent();
                WeakThis->PoliceMonologueWidget->SetVisibility(ESlateVisibility::Hidden);
                WeakThis->PoliceMonologueWidget->Destruct();
                WeakThis->PoliceMonologueWidget = nullptr;
                DP_SCREEN(FColor::Cyan, "PoliceMonologueWidget CLOSED");
            }
        },
        6.0f,
        false
    );
}


// ──────────────────────────────────────────────
//  Doctor 상호작용 (HUD 경유)
// ──────────────────────────────────────────────
void ADiaryPickup::DoctorInteract(ADoctor* _Playercharacter)
{
    DP_LOG(Log, TEXT("DoctorInteract called"));

    if (!_Playercharacter)
    {
        DP_LOG(Error, TEXT("DoctorInteract: Player NULL"));
        DP_SCREEN(FColor::Red, "Doctor NULL");
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return;

    if (!bIsInteracting)
    {
        ADoctorHUD* HUD = Cast<ADoctorHUD>(PC->GetHUD());
        if (HUD)
        {
            HUD->ShowDiary(TEXT("Doctor Diary opened via pickup"), PC);
            bIsInteracting = true;
            DP_SCREEN(FColor::Green, "Doctor Diary OPEN");
        }
    }
    else
    {
        EndInteract();
    }
}

// ──────────────────────────────────────────────
//  Police 상호작용 (직접 위젯 생성)
// ──────────────────────────────────────────────
void ADiaryPickup::Interact(APolice* _Playercharacter)
{

    // ✅ 방금 닫힌 경우에는 무시 (0.4초 딜레이)
    if (bRecentlyClosed && (GetWorld()->GetTimeSeconds() - ClosedTime) < 0.4f)
    {
        DP_LOG(Log, TEXT("Interaction ignored (recently closed)"));
        return;
    }
    else
    {
        bRecentlyClosed = false;
    }

    if (!_Playercharacter)
    {
        DP_LOG(Error, TEXT("PoliceInteract: Player is null"));
        DP_SCREEN(FColor::Red, "Police NULL");
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
    {
        DP_LOG(Error, TEXT("PlayerController is null"));
        return;
    }

    // 이미 열려 있으면 닫기
    if (bIsInteracting)
    {
        EndInteract();
        return;
    }

    // 새로 열기
    DP_LOG(Log, TEXT("Opening diary UI"));
    BeginInteract();
   

   
  

    if (DiaryWidgetClass)
    {
        DiaryWidget = CreateWidget<UDiaryWidget>(PC, DiaryWidgetClass);
        if (DiaryWidget)
        {
            DiaryWidget->AddToViewport(100);

            // 일기 내용 표시
            const FString LeftPageText = TEXT(
                "오늘도 환자들과 시간을 보냈다.\n"
                "그들은 내게 의지하고,\n"
                "나도 그들을 가족으로 생각한다.\n"
                "함께 있을 때 마음이 편하고,\n"
                "세상이 조금은 따뜻하게 느껴진다.\n\n"
                "그런데 어제는 문득 떠올랐다.\n"
                "어릴 적 기억이...\n"
                "나를 때리던 아버지의 모습이\n"
                "날카로운 이빨과 발톱을 가진\n"
                "기괴한 괴물로 보였던 순간…\n"
                "그때의 두려움은 아직도 생생하다.\n\n"
                "그 이후 줄곧\n"
                "진정한 가족을 갖는 것이 꿈이었고,\n"
                "최근에는 환자들과 많은 시간을 보내며 \n"
                "내가 생각해왔던\n"
                "진정한 가족처럼 지내고 있다.\n\n"
            );

            const FString RightPageText = TEXT(
                "그러던 중 순간 든 생각이 있었다.\n"
                "진정한 가족이란\n"
                "내가 생각하는 것이 맞을까?\n"
                "어쩌면 괴물인 쪽이\n"
                "<진짜 가족>일 수도 있지 않나?\n\n"
                "혹시 내 환자들도,\n"
                "나를 정말 가족으로 생각해준다면\n"
                "괴물로 변하게 되지않을까?\n"
                "순간 눈앞의 환자가\n"
                "어린 시절 봤던 <그 괴물>로 보였다.\n"
                "곧바로 되돌아왔지만 나에겐 큰 충격이다.\n\n"
                "내가 줄곧 원했던게\n"
                "가족이라고 생각했지만\n"
                "요즘은\n"
                "가족이 애초에 어떤 것인지도 헷갈린다..."
            );

            DiaryWidget->UpdateDiaryPages(LeftPageText, RightPageText);

            // 입력 잠금 & 마우스 커서 표시
            PC->SetIgnoreLookInput(true);
            PC->SetIgnoreMoveInput(true);

            FInputModeGameAndUI InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            InputMode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = true;

            bIsInteracting = true;
            DP_SCREEN(FColor::Green, "Diary OPEN");
            DP_LOG(Log, TEXT("DiaryWidget created successfully"));

            // ✅ 여기에 추가 (중요!)
            bJustOpened = true;
            OpenedTime = GetWorld()->GetTimeSeconds();
        }
        else
        {
            DP_LOG(Error, TEXT("Failed to create DiaryWidget!"));
            DP_SCREEN(FColor::Red, "Widget NULL");
        }
    }
    else
    {
        DP_LOG(Error, TEXT("DiaryWidgetClass not set!"));
        DP_SCREEN(FColor::Red, "WidgetClass NULL");
    }

    DumpState(TEXT("After Interact"));

    // 일기장을 열었으니 엔딩 트리거 박스 실행 허용
    if (APoliceGameModeBase* GM = Cast<APoliceGameModeBase>(UGameplayStatics::GetGameMode(GetWorld())))
    {
        GM->bDiaryRead = true;
    }
}

// ──────────────────────────────────────────────
//  다음 라인 표시 (테스트용)
// ──────────────────────────────────────────────
void ADiaryPickup::ShowNextDiaryLine()
{
    if (!DiaryWidget || !DiaryLines.IsValidIndex(CurrentLineIndex))
    {
        DP_LOG(Log, TEXT("No more lines -> closing"));
        EndInteract();
        return;
    }

    DiaryWidget->UpdateDiaryText(DiaryLines[CurrentLineIndex]);
    DP_LOG(Log, TEXT("Showing line %d: %s"), CurrentLineIndex, *DiaryLines[CurrentLineIndex]);
    CurrentLineIndex++;
}

// ──────────────────────────────────────────────
//  상태 출력 헬퍼 (디버깅용)
// ──────────────────────────────────────────────
void ADiaryPickup::DumpState(const FString& Tag) const
{
    DP_LOG(Log, TEXT("[%s] State | Interacting=%s, Index=%d, Lines=%d, Mesh=%s, WidgetClass=%s, Widget=%s"),
        *Tag,
        bIsInteracting ? TEXT("true") : TEXT("false"),
        CurrentLineIndex,
        DiaryLines.Num(),
        *GetNameSafe(Mesh),
        *GetNameSafe(DiaryWidgetClass),
        *GetNameSafe(DiaryWidget)
    );
}

// ──────────────────────────────────────────────
//  경찰 독백 (엔딩)
// ──────────────────────────────────────────────
void ADiaryPickup::ShowPoliceMonologueEnding()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC && PoliceMonologueWidgetClass)
    {
        PoliceMonologueWidget = CreateWidget<UPoliceMonologueWidget>(PC, PoliceMonologueWidgetClass);
        if (PoliceMonologueWidget)
        {
            PoliceMonologueWidget->SetMonologueText(TEXT("...범행 동기가 나온 것 같군. 조사는 이만 마무리하고 나가야겠어."));
            PoliceMonologueWidget->AddToViewport();
            DP_SCREEN(FColor::Green, "PoliceMonologueWidget OPEN");
        }
    }
}
void ADiaryPickup::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsInteracting)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            // 방금 열렸으면 0.3초 정도는 입력 무시
            if (bJustOpened && (GetWorld()->GetTimeSeconds() - OpenedTime) < 0.3f)
                return;
            else
                bJustOpened = false;

            const bool bPressed = PC->IsInputKeyDown(EKeys::E);

            // E키 눌림 감지 → 닫기
            if (bPressed && !bCloseKeyPressed)
            {
                bCloseKeyPressed = true;
                DP_LOG(Log, TEXT("E pressed inside diary → closing"));
                EndInteract();
            }
            else if (!bPressed)
            {
                bCloseKeyPressed = false;
            }
        }
    }
}

void ADiaryPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (GetWorld())
        GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}