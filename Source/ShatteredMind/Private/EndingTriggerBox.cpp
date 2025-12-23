#include "EndingTriggerBox.h"
#include "PoliceGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"
#include "Components/ShapeComponent.h"     // GetCollisionComponent 관련
#include "Components/BoxComponent.h"       // 디버그 박스용
#include "DrawDebugHelpers.h"              // DrawDebugBox

AEndingTriggerBox::AEndingTriggerBox()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AEndingTriggerBox::BeginPlay()
{
    Super::BeginPlay();

    OnActorBeginOverlap.AddDynamic(this, &AEndingTriggerBox::HandleActorBeginOverlap);

    if (bVerboseLog)
    {
        // GameMode 캐스팅/상태도 미리 출력
        APoliceGameModeBase* GM = Cast<APoliceGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
        UE_LOG(LogTemp, Log,
            TEXT("[EndingTriggerBox::BeginPlay] Ready. Name=%s | EndingLevel=%s | RequireDiary=%s | RequiredTag=%s | GM=%s | bDiaryRead=%s"),
            *GetName(),
            *EndingLevelName.ToString(),
            bRequireDiaryRead ? TEXT("true") : TEXT("false"),
            *RequiredActorTag.ToString(),
            GM ? *GM->GetName() : TEXT("nullptr"),
            (GM && GM->bDiaryRead) ? TEXT("true") : TEXT("false"));
    }

#if WITH_EDITOR
    // 트리거 영역 시각화 ? 에디터에서 위치/크기 확인
    if (bDrawDebugBoxOnBeginPlay)
    {
        if (UBoxComponent* Box = Cast<UBoxComponent>(GetCollisionComponent()))
        {
            const FVector Extent = Box->GetScaledBoxExtent();
            const FTransform& TM = Box->GetComponentTransform();
            DrawDebugBox(GetWorld(), TM.GetLocation(), Extent, TM.GetRotation(), FColor::Green, false, DebugBoxDuration, 0, 2.0f);
            if (bVerboseLog)
            {
                UE_LOG(LogTemp, Log, TEXT("[EndingTriggerBox::BeginPlay] DrawDebugBox at %s Extent=%s"),
                    *TM.GetLocation().ToString(), *Extent.ToString());
            }
        }
        else if (bVerboseLog)
        {
            UE_LOG(LogTemp, Warning, TEXT("[EndingTriggerBox::BeginPlay] CollisionComponent is not UBoxComponent. Debug box skipped."));
        }
    }
#endif
}

void AEndingTriggerBox::HandleActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
    if (bVerboseLog)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EndingTriggerBox::HandleActorBeginOverlap] Overlap! OtherActor=%s | AlreadyTriggered=%s"),
            OtherActor ? *OtherActor->GetName() : TEXT("nullptr"),
            bAlreadyTriggered ? TEXT("true") : TEXT("false"));
    }

    if (!OtherActor || OtherActor == this)
    {
        if (bVerboseLog)
        {
            UE_LOG(LogTemp, Warning, TEXT("[EndingTriggerBox] Early return: OtherActor invalid or self."));
        }
        return;
    }

    if (bAlreadyTriggered)
    {
        if (bVerboseLog)
        {
            UE_LOG(LogTemp, Warning, TEXT("[EndingTriggerBox] Already triggered once. Ignoring further overlaps."));
        }
        return;
    }

    // 조건 검사 상세 로그
    if (!IsEligibleToTrigger(OtherActor))
    {
        if (bVerboseLog)
        {
            UE_LOG(LogTemp, Warning, TEXT("[EndingTriggerBox] Conditions NOT met for %s. No action."), *OtherActor->GetName());
        }
        return;
    }

    bAlreadyTriggered = true;

    // 재발동 방지(충돌 끄기)
    if (bDisableCollisionAfterTrigger)
    {
        if (UPrimitiveComponent* CollisionComp = GetCollisionComponent())
        {
            CollisionComp->SetGenerateOverlapEvents(false);
            CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            if (bVerboseLog)
            {
                UE_LOG(LogTemp, Log, TEXT("[EndingTriggerBox] Collision disabled to prevent re-trigger."));
            }
        }
    }

    // 페이드 시작
    StartFadeAndTransition();
}

bool AEndingTriggerBox::IsEligibleToTrigger(AActor* OtherActor) const
{
    // 1) 태그 검사
    const bool bNeedTag = !RequiredActorTag.IsNone();
    const bool bHasTag = (bNeedTag ? OtherActor->ActorHasTag(RequiredActorTag) : true);

    if (bVerboseLog)
    {
        UE_LOG(LogTemp, Log, TEXT("[EndingTriggerBox::IsEligibleToTrigger] NeedTag=%s Tag=%s HasTag=%s"),
            bNeedTag ? TEXT("true") : TEXT("false"),
            *RequiredActorTag.ToString(),
            bHasTag ? TEXT("true") : TEXT("false"));
    }

    if (!bHasTag)
    {
        if (bVerboseLog)
        {
            UE_LOG(LogTemp, Warning, TEXT("[EndingTriggerBox] FAIL: OtherActor(%s) does not have RequiredTag(%s)"),
                *OtherActor->GetName(), *RequiredActorTag.ToString());
        }
        return false;
    }

    // 2) 일기장 열람 여부(옵션)
    if (bRequireDiaryRead)
    {
        const APoliceGameModeBase* GM = Cast<APoliceGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
        if (!GM)
        {
            if (bVerboseLog)
            {
                UE_LOG(LogTemp, Error, TEXT("[EndingTriggerBox] FAIL: GameMode cast to APoliceGameModeBase FAILED."));
            }
            return false;
        }

        if (bVerboseLog)
        {
            UE_LOG(LogTemp, Log, TEXT("[EndingTriggerBox] RequireDiaryRead=true | GM=%s | bDiaryRead=%s"),
                *GM->GetName(), GM->bDiaryRead ? TEXT("true") : TEXT("false"));
        }

        if (!GM->bDiaryRead)
        {
            if (bVerboseLog)
            {
                UE_LOG(LogTemp, Warning, TEXT("[EndingTriggerBox] FAIL: bDiaryRead=false (Diary not read yet)"));
            }
            return false;
        }
    }

    if (bVerboseLog)
    {
        UE_LOG(LogTemp, Log, TEXT("[EndingTriggerBox] ? Eligible to trigger."));
    }
    return true;
}

void AEndingTriggerBox::StartFadeAndTransition()
{
    if (bVerboseLog)
    {
        UE_LOG(LogTemp, Log, TEXT("[EndingTriggerBox::StartFadeAndTransition] FadeDuration=%.2f DelayBeforeLevelLoad=%.2f"),
            FadeDuration, DelayBeforeLevelLoad);
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC || !PC->PlayerCameraManager)
    {
        if (bVerboseLog)
        {
            UE_LOG(LogTemp, Warning, TEXT("[EndingTriggerBox] No valid PlayerController/CameraManager. Skipping fade, executing ending now."));
        }
        ExecuteEnding();
        return;
    }

    // 페이드 아웃 시작
    PC->PlayerCameraManager->StartCameraFade(
        0.0f, 1.0f,
        FadeDuration,
        FLinearColor::Black,
        false,
        true
    );

    // ?? 로컬 타이머 핸들 생성
    FTimerHandle TimerHandle;
    FTimerDelegate Del;
    Del.BindUFunction(this, FName("ExecuteEnding"));

    GetWorldTimerManager().SetTimer(TimerHandle, Del, DelayBeforeLevelLoad, false);

    if (bVerboseLog)
    {
        UE_LOG(LogTemp, Log, TEXT("[EndingTriggerBox] Fade started. Will load level after %.2fs"), DelayBeforeLevelLoad);
    }
}


void AEndingTriggerBox::ExecuteEnding()
{
    if (EndingLevelName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("[EndingTriggerBox::ExecuteEnding] FAIL: EndingLevelName is None"));
        return;
    }

    if (bVerboseLog)
    {
        UE_LOG(LogTemp, Log, TEXT("[EndingTriggerBox::ExecuteEnding] Loading Ending Level: %s"),
            *EndingLevelName.ToString());
    }

    UGameplayStatics::OpenLevel(GetWorld(), EndingLevelName);
}
