#include "NoiseTrigger.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Internationalization/Text.h"
#include "PoliceMonologueWidget.h"
#include "TimerManager.h"

ANoiseTrigger::ANoiseTrigger()
{
    OnActorBeginOverlap.AddDynamic(this, &ANoiseTrigger::OnOverlapBegin);
}

void ANoiseTrigger::ShowPoliceMonologue()
{
    if (PoliceMonologueWidgetClass)
    {
        UPoliceMonologueWidget* Monologue = CreateWidget<UPoliceMonologueWidget>(GetWorld(), PoliceMonologueWidgetClass);
        if (Monologue)
        {
            Monologue->AddToViewport();
            Monologue->SetMonologueText(TEXT("왠지 어릴 때 봤던 현장이 생각나네..."));

            // 5초 후 위젯 제거
            FTimerHandle RemoveTimerHandle;
            GetWorld()->GetTimerManager().SetTimer(
                RemoveTimerHandle,
                [Monologue]()
                {
                    if (Monologue)
                    {
                        Monologue->RemoveFromParent();
                    }
                },
                3.0f,   // 유지 시간 (원하는 대로 조정)
                false
            );
        }
    }
}

void ANoiseTrigger::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("NoiseWidgetClass=%s"), *GetNameSafe(NoiseWidgetClass));
}

void ANoiseTrigger::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
    UE_LOG(LogTemp, Warning, TEXT(">>> Overlap Begin Triggered!"));

    if (bHasTriggered) return; // 이미 한 번 실행했으면 패스

    if (OtherActor && OtherActor->ActorHasTag("Player"))
    {
        bHasTriggered = true; // 이제 실행 기록

        if (!NoiseWidget)
        {
            // 경로에서 위젯 클래스 로드
            TSubclassOf<UUserWidget> LoadedWidgetClass = LoadClass<UUserWidget>(
                nullptr,
                TEXT("/Game/PoliceInterface/WBP_ScreenNoise.WBP_ScreenNoise_C")
            );

            if (LoadedWidgetClass)
            {
                NoiseWidget = CreateWidget<UUserWidget>(GetWorld(), LoadedWidgetClass);
                if (NoiseWidget)
                {
                    NoiseWidget->AddToViewport();

                    // 1초 후 제거
                    FTimerHandle TimerHandle;
                    GetWorld()->GetTimerManager().SetTimer(
                        TimerHandle,
                        this,
                        &ANoiseTrigger::HideNoiseWidget,
                        1.0f,
                        false
                    );
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("XXX NoiseWidgetClass is null!"));
        }
    }
}

void ANoiseTrigger::HideNoiseWidget()
{
    if (NoiseWidget)
    {
        NoiseWidget->RemoveFromParent();
        NoiseWidget = nullptr;
    }

    // 2초 뒤 경찰 독백 위젯 표시
    FTimerHandle MonologueTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        MonologueTimerHandle,
        this,
        &ANoiseTrigger::ShowPoliceMonologue,
        1.0f,
        false
    );
}