// DiaryPickup.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionInterface.h"
#include "DiaryWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "DiaryPickup.generated.h"

// 전방 선언
class UDiaryWidget;
class ADoctor;
class APolice;

/**
 * 상호작용 가능한 일기 액터
 * - Doctor: HUD를 통해 위젯 표시
 * - Police: 자체적으로 위젯 생성 및 표시
 */
UCLASS()
class SHATTEREDMIND_API ADiaryPickup : public AActor, public IInteractionInterface
{
    GENERATED_BODY()

public:
    // ──────────────────────────────────────────────
    // 생성자
    // ──────────────────────────────────────────────
    ADiaryPickup();

protected:
    // ──────────────────────────────────────────────
    // Unreal 기본 이벤트
    // ──────────────────────────────────────────────
    virtual void BeginPlay() override;

public:
    // ──────────────────────────────────────────────
    // 컴포넌트
    // ──────────────────────────────────────────────
    /** 일기 메시(루트 컴포넌트) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Diary|Component")
    UStaticMeshComponent* Mesh;

    /** 일기 위젯 클래스 (BP_Diary_Widget 지정 필요) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Diary|UI")
    TSubclassOf<UDiaryWidget> DiaryWidgetClass;

    /** 실제 표시 중인 위젯 인스턴스 */
    UPROPERTY(VisibleAnywhere, Category = "Diary|UI")
    UDiaryWidget* DiaryWidget;

    /** 현재 일기 대사 라인들 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Diary|Content")
    TArray<FString> DiaryLines;

    /** 현재 표시 중인 라인 인덱스 */
    int32 CurrentLineIndex;

    /** 현재 상호작용 중인지 여부 */
    bool bIsInteracting;

    // ──────────────────────────────────────────────
    // 인터페이스 오버라이드 (상호작용)
    // ──────────────────────────────────────────────
    virtual void BeginFocus() override;
    virtual void EndFocus() override;
    virtual void BeginInteract() override;
    virtual void EndInteract() override;
    virtual void Interact(APolice* _Playercharacter) override;
    virtual void DoctorInteract(ADoctor* _Playercharacter) override;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ──────────────────────────────────────────────
    // 보조 함수
    // ──────────────────────────────────────────────
    /** 다음 라인 표시 */
    void ShowNextDiaryLine();

    /** 디버깅용 상태 출력 */
    void DumpState(const FString& Tag) const;

    // ──────────────────────────────────────────────
// 경찰 독백 위젯 호출 관련///////////20251107 남소은
// ──────────────────────────────────────────────
public:
    // Timer 이후 호출될 함수 선언
    UFUNCTION()
    void ShowPoliceMonologueEnding();

    // PoliceMonologueWidget 클래스 참조
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class UPoliceMonologueWidget> PoliceMonologueWidgetClass;

    UPoliceMonologueWidget* PoliceMonologueWidget;


public:
    virtual void Tick(float DeltaTime) override;




protected:
    bool bCloseKeyPressed = false;   // 중복 입력 방지용
    bool bJustOpened = false;       // 새로 추가: 방금 열린 상태
    float OpenedTime = 0.f;         // 시간 저장


    bool bRecentlyClosed = false; // ? 새로 추가
    float ClosedTime = 0.f;       // ? 새로 추가


    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PickUp")
    class UBoxComponent* InteractionBox;
    
};
