#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "LockDialComponent.generated.h"

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SHATTEREDMIND_API ULockDialComponent : public UStaticMeshComponent
{
    GENERATED_BODY()

public:
    ULockDialComponent();

protected:
    virtual void BeginPlay() override;

public:
    // ──────────────────────────────
    // 회전/스냅 관련 설정값
    // ──────────────────────────────
    UPROPERTY(EditAnywhere, Category = "Lock|Rotation")
    float StepAngleDeg = 360.f / 26.f;

    UPROPERTY(VisibleAnywhere, Category = "Lock|Rotation")
    int32 CurrentIndex = 0;

    UPROPERTY(EditAnywhere, Category = "Lock|Snap")
    int32 TickCount = 16;

    UPROPERTY(EditAnywhere, Category = "Lock|Snap")
    float SnapTolerance = 2.0f;

    UPROPERTY(VisibleAnywhere, Category = "Lock|Snap")
    TArray<USceneComponent*> TickMarks;

    UPROPERTY(EditAnywhere, Category = "Lock|Audio")
    USoundBase* ClickSound;

    UPROPERTY(EditAnywhere, Category = "Lock|Letters")
    FString LetterSequence = TEXT("GRWUZBLYMKPEIAQF");

    // 외부에서 Tick 기준 피벗을 참조할 수 있도록 추가
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Snap")
    USceneComponent* TickMarkPivot;

    // ──────────────────────────────
    // 핵심 동작 함수
    // ──────────────────────────────
    void SetIndex(int32 NewIndex);
    void StepByScroll(int32 Delta);
    void ApplyRotationFromIndex();
    TCHAR GetCurrentLetter() const;
    void GenerateTickMarks();
    void SnapToNearestTick();

    /** 다이얼을 한 칸 회전시키되 눈금 기준으로 스냅 (진짜 자물쇠 느낌) */
    void RotateDialByStep(int32 Direction);

    /** 하이라이트 효과 켜기/끄기 */
    UFUNCTION()
    void SetHighlight(bool bOn);
};
