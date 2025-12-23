#include "DoctorGameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy.h"
#include "Doctor.h"
#include "Engine/World.h"

ADoctorGameModeBase::ADoctorGameModeBase()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ADoctorGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), Enemies);

    AliveEnemies = 0;
    for (AActor* EnemyActor : Enemies)
    {
        if (!IsValid(EnemyActor)) continue;
        ++AliveEnemies;
        EnemyActor->OnDestroyed.AddDynamic(this, &ADoctorGameModeBase::HandleEnemyDestroyed);
    }
    InitialEnemyCount = AliveEnemies; // ★ 추가
}
void ADoctorGameModeBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bGameEnded) return;

    ElapsedTime += DeltaSeconds;
    CheckTimeOver();
}

void ADoctorGameModeBase::CheckTimeOver()
{
    if (ElapsedTime >= TimeLimit && !bGameEnded)
    {
        // 시간 초과 엔딩
        ShowEndWidget(TimeOverWidgetClass);
        bGameEnded = true;
    }
}

void ADoctorGameModeBase::HandleEnemyDestroyed(AActor* /*DestroyedActor*/)
{
    if (bGameEnded) return;

    AliveEnemies = FMath::Max(0, AliveEnemies - 1);

#if !UE_BUILD_SHIPPING
    GEngine->AddOnScreenDebugMessage(
        12346, 2.f, FColor::Green,
        FString::Printf(TEXT("Enemy down. Alive: %d"), AliveEnemies)
    );
#endif

    CheckAllEnemiesDead();
}

void ADoctorGameModeBase::CheckAllEnemiesDead()
{
    if (AliveEnemies == 0 && !bGameEnded)
    {
        // 전멸 클리어 엔딩
        ShowEndWidget(ClearWidgetClass);
        bGameEnded = true;
    }
}

void ADoctorGameModeBase::OnPlayerDied()
{
    if (bGameEnded) return;
    bGameEnded = true;

    // 1) 우선 Pawn에서 킬 읽기 (기존 방식)
    int32 KillsFromPawn = 0;
    if (ACharacter* PCPawn = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
    {
        if (ADoctor* Doc = Cast<ADoctor>(PCPawn))
        {
            KillsFromPawn = Doc->GetKillCount();
        }
    }

    // 2) 폴백: 적 감소량으로 최소 1킬 여부 추정
    const int32 Decreased = FMath::Max(0, InitialEnemyCount - AliveEnemies);
    const bool bAtLeastOneKill = (KillsFromPawn > 0) || (Decreased >= 1);

    if (!bAtLeastOneKill)
    {
        ShowEndWidget(Dead_NoKill_WidgetClass);
    }
    else
    {
        ShowEndWidget(Dead_WithKills_WidgetClass);
    }
}

// 참고: 필요하면 외부에서 부를 수 있는 훅
void ADoctorGameModeBase::OnEnemyKilled()
{
    // 지금은 HandleEnemyDestroyed()에서 죽은 적을 세니까
    // 여긴 안 써도 됨. 나중에 너희가 직접 호출형으로 바꾸고 싶으면 여기서 로직 적어.
}

void ADoctorGameModeBase::ShowEndWidget(TSubclassOf<UUserWidget> WidgetToShow)
{
    if (!WidgetToShow) return;

    UUserWidget* W = CreateWidget<UUserWidget>(GetWorld(), WidgetToShow);
    if (!W) return;

    W->SetIsFocusable(true);
    W->AddToViewport(10);
    UGameplayStatics::SetGamePaused(GetWorld(), true);

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->bShowMouseCursor = true;
        FInputModeUIOnly InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetWidgetToFocus(W->TakeWidget());
        PC->SetInputMode(InputMode);
        W->SetKeyboardFocus();
    }
}
