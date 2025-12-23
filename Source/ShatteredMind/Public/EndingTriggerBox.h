// EndingTriggerBox.h
// ─────────────────────────────────────────────────────────────
// 목적:
//   플레이어가 트리거 박스에 들어왔을 때 조건(태그/일기장열람)을 검사하고,
//   페이드아웃 → 엔딩 레벨로 전환.
//
// 디버그 기능:
//   - BeginPlay/Overlap 등 주요 지점에 상세 로그 출력
//   - 에디터에서 트리거 영역을 DrawDebugBox로 시각화
//   - 조건별 실패 이유를 로그로 명확히 출력
// ─────────────────────────────────────────────────────────────

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "EndingTriggerBox.generated.h"

class APoliceGameModeBase;

UCLASS()
class SHATTEREDMIND_API AEndingTriggerBox : public ATriggerBox
{
    GENERATED_BODY()

public:
    AEndingTriggerBox();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

    bool IsEligibleToTrigger(AActor* OtherActor) const;

    // 페이드아웃 + 딜레이 후 ExecuteEnding 호출
    void StartFadeAndTransition();

    // 실제 맵 전환
    UFUNCTION() // 타이머 바인딩을 위해 UFUNCTION으로 둔다
        void ExecuteEnding();

private:
    bool bAlreadyTriggered = false;

public:
    // ── 설정 값들 ───────────────────────────────────────────

    // 전환할 엔딩 레벨 이름(정확한 맵 이름)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending")
    FName EndingLevelName = TEXT("EndingLevel");

    // 반응할 액터 태그(기본 Player) ? 플레이어에 동일 태그 필요
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending")
    FName RequiredActorTag = TEXT("Player");

    // 일기장 열람 여부 확인(PoliceGameModeBase::bDiaryRead) 강제 여부
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending|Conditions")
    bool bRequireDiaryRead = true;

    // 발동 후 콜리전 비활성화(재발동 방지)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending|Safety")
    bool bDisableCollisionAfterTrigger = true;

    // 로그 자세히 출력
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending|Debug")
    bool bVerboseLog = true;

    // 페이드아웃 시간(초)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending|Fade")
    float FadeDuration = 1.5f;

    // 페이드 완료 후 맵 전환까지 지연
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending|Fade")
    float DelayBeforeLevelLoad = 1.6f;

    // 에디터에서 트리거 박스를 시각화
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending|Debug")
    bool bDrawDebugBoxOnBeginPlay = true;

    // 디버그 박스 표시 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending|Debug")
    float DebugBoxDuration = 4.0f;
};
