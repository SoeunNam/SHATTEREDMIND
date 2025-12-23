#include "LockDialComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

// ──────────────────────────────────────────────
// 생성자
// ──────────────────────────────────────────────
ULockDialComponent::ULockDialComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetCollisionResponseToAllChannels(ECR_Ignore);
    SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    bSelectable = true;
    bRenderCustomDepth = false;
}

// ──────────────────────────────────────────────
// BeginPlay
// ──────────────────────────────────────────────
void ULockDialComponent::BeginPlay()
{
    Super::BeginPlay();

    GenerateTickMarks();
    ApplyRotationFromIndex();
}

// ──────────────────────────────────────────────
// 자동 눈금 생성
// ──────────────────────────────────────────────
void ULockDialComponent::GenerateTickMarks()
{
    TickMarks.Empty();

    USceneComponent* Pivot = GetAttachParent();
    if (!Pivot)
    {
        UE_LOG(LogTemp, Error, TEXT("[Dial] %s has no parent pivot!"), *GetName());
        return;
    }

    const int32 NumTicks = 16;
    const float StepAngle = 360.f / NumTicks;
    const float Radius = 10.f;

    for (int32 i = 0; i < NumTicks; ++i)
    {
        const FString TickName = FString::Printf(TEXT("Tick_%d"), i);
        USceneComponent* Tick = NewObject<USceneComponent>(Pivot, *TickName);
        Tick->RegisterComponent();
        Tick->AttachToComponent(Pivot, FAttachmentTransformRules::KeepRelativeTransform);

        const float Angle = i * StepAngle;
        const float Rad = FMath::DegreesToRadians(Angle);
        FVector LocalPos(FMath::Cos(Rad) * Radius, 0.f, FMath::Sin(Rad) * Radius);

        Tick->SetRelativeLocation(LocalPos);
        Tick->SetRelativeRotation(FRotator(0.f, 0.f, Angle));

        TickMarks.Add(Tick);
    }

    UE_LOG(LogTemp, Warning, TEXT("[Dial] %d TickMarks created for %s"), TickMarks.Num(), *GetName());

//#if WITH_EDITOR
//    if (UWorld* World = GetWorld())
//    {
//        for (auto* TickComp : TickMarks)
//        {
//            if (!TickComp) continue;
//            const FVector Loc = TickComp->GetComponentLocation();
//
//            DrawDebugSphere(World, Loc, 2.5f, 8, FColor::Red, true, -1.f, 0, 0.2f);
//            DrawDebugString(World, Loc + FVector(0, 0, 5.f),
//                FString::Printf(TEXT("%d"), TickMarks.IndexOfByKey(TickComp)),
//                nullptr, FColor::White, 5.f, true);
//        }
//    }
//#endif
}

// ──────────────────────────────────────────────
// 다이얼 인덱스 변경
// ──────────────────────────────────────────────
void ULockDialComponent::SetIndex(int32 NewIndex)
{
    NewIndex = (NewIndex % LetterSequence.Len() + LetterSequence.Len()) % LetterSequence.Len();
    if (CurrentIndex == NewIndex) return;

    CurrentIndex = NewIndex;
    ApplyRotationFromIndex();

    if (ClickSound)
        UGameplayStatics::PlaySoundAtLocation(this, ClickSound, GetComponentLocation());
}

// ──────────────────────────────────────────────
// 스크롤 입력
// ──────────────────────────────────────────────
void ULockDialComponent::StepByScroll(int32 Delta)
{
    if (Delta == 0) return;

    CurrentIndex = (CurrentIndex + Delta + LetterSequence.Len()) % LetterSequence.Len();
    ApplyRotationFromIndex();
    SnapToNearestTick();

    if (ClickSound)
        UGameplayStatics::PlaySoundAtLocation(this, ClickSound, GetComponentLocation());
}

// ──────────────────────────────────────────────
// 회전 적용
// ──────────────────────────────────────────────
void ULockDialComponent::ApplyRotationFromIndex()
{
    // 인덱스 증가 시 실제 메시가 시계 방향으로 회전하도록 부호 반전
    const float Angle = +CurrentIndex * StepAngleDeg;   

    if (USceneComponent* Pivot = GetAttachParent())
    {
        Pivot->SetRelativeRotation(FRotator(0.f, 0.f, Angle));
    }

    UE_LOG(LogTemp, Warning, TEXT("[Dial] ApplyRotationFromIndex: Index=%d, Angle=%.1f°"), CurrentIndex, Angle);
}

// ──────────────────────────────────────────────
// 가까운 눈금으로 자동 스냅
// ──────────────────────────────────────────────
void ULockDialComponent::SnapToNearestTick()
{
    if (!GetAttachParent() || TickMarks.Num() == 0) return;

    const float CurrentRoll = GetAttachParent()->GetRelativeRotation().Roll;
    float ClosestAngle = 0.f;
    float MinDist = 9999.f;

    for (USceneComponent* Tick : TickMarks)
    {
        if (!Tick) continue;

        const float TickRoll = Tick->GetRelativeRotation().Roll;
        float Diff = FMath::FindDeltaAngleDegrees(CurrentRoll, TickRoll);

        if (FMath::Abs(Diff) < MinDist)
        {
            MinDist = FMath::Abs(Diff);
            ClosestAngle = TickRoll;
        }
    }

    if (MinDist <= SnapTolerance)
    {
        GetAttachParent()->SetRelativeRotation(FRotator(0.f, 0.f, ClosestAngle));
        UE_LOG(LogTemp, Log, TEXT("[Dial] Snapped to tick (angle=%.2f)"), ClosestAngle);
    }
}

// ──────────────────────────────────────────────
// ✅ 새 추가: 한 칸씩 눈금 기준 회전 (정확히 스냅되는 다이얼)
// ──────────────────────────────────────────────
//void ULockDialComponent::RotateDialByStep(int32 Direction)
//{
//    if (TickMarks.Num() == 0) return;
//
//    const int32 OldIndex = CurrentIndex;
//    const int32 Step = FMath::Clamp(Direction, -1, 1);
//    CurrentIndex = (CurrentIndex + Step + TickMarks.Num()) % TickMarks.Num();
//
//    float TargetAngle = TickMarks[CurrentIndex]->GetRelativeRotation().Roll;
//
//    if (USceneComponent* Pivot = GetAttachParent())
//    {
//        Pivot->SetRelativeRotation(FRotator(0.f, 0.f, TargetAngle));
//    }
//
//    UE_LOG(LogTemp, Warning, TEXT("[Dial] RotateDialByStep: %d -> %d (Angle=%.1f)"),
//        OldIndex, CurrentIndex, TargetAngle);
//
//    if (ClickSound)
//        UGameplayStatics::PlaySoundAtLocation(this, ClickSound, GetComponentLocation());
//
//   
//
//}

void ULockDialComponent::RotateDialByStep(int32 Direction)
{
    if (TickMarks.Num() == 0) return;

    const int32 OldIndex = CurrentIndex;
    const int32 Step = FMath::Clamp(Direction, -1, 1);
    CurrentIndex = (CurrentIndex + Step + TickMarks.Num()) % TickMarks.Num();

    float TargetAngle = -TickMarks[CurrentIndex]->GetRelativeRotation().Roll;

    if (USceneComponent* Pivot = GetAttachParent())
    {
        Pivot->SetRelativeRotation(FRotator(0.f, 0.f, TargetAngle));

        // ✅ 여기서 Pivot이 유효하니까 이 안에서 로그 출력해야 함
        UE_LOG(LogTemp, Warning, TEXT("[DialTest] Step=%d | Index=%d | Angle=%.1f | TickAngle=%.1f"),
            Direction, CurrentIndex, Pivot->GetRelativeRotation().Roll,
            TickMarks[CurrentIndex]->GetRelativeRotation().Roll);
    }

    UE_LOG(LogTemp, Warning, TEXT("[Dial] RotateDialByStep: %d -> %d (Angle=%.1f)"),
        OldIndex, CurrentIndex, TargetAngle);

    // 다이얼이 멀어서 잘 안들리니까 다이얼위치가 아닌 카메라 위치에서 다이얼 돌리는 사운드 재생
    if (ClickSound)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC && PC->PlayerCameraManager)
        {
            FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), ClickSound, CamLoc);
        }
    }


}


// ──────────────────────────────────────────────
// 현재 글자 반환
// ──────────────────────────────────────────────
TCHAR ULockDialComponent::GetCurrentLetter() const
{
    const int32 Len = LetterSequence.Len();
    if (Len == 0) return TEXT('?');

    const int32 SafeIndex = (CurrentIndex % Len + Len) % Len;
    return LetterSequence[SafeIndex];
}

// ──────────────────────────────────────────────
// 하이라이트 표시
// ──────────────────────────────────────────────
void ULockDialComponent::SetHighlight(bool bOn)
{
    SetRenderCustomDepth(bOn);
    //SetCustomDepthStencilValue(1);
    UE_LOG(LogTemp, Log, TEXT("[Dial] Highlight %s"), bOn ? TEXT("ON") : TEXT("OFF"));
}
