#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionInterface.h"           // 상호작용 인터페이스(플레이어가 상호작용할 때 호출)
#include "Components/PointLightComponent.h" // 인스펙트 조명용
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "FamilyLockActor.generated.h"


// ─────────────────────────────────────────────────────────────────────────────
// 전방 선언(헤더 간 의존성/빌드 시간 감소)
// ─────────────────────────────────────────────────────────────────────────────
class UCameraComponent;
class ULockDialComponent;
class ADrawerActor;
class APolice;
class USoundBase;

/**
 * 가족 자물쇠 액터(6자리 다이얼로 "FAMILY" 맞추면 해제)
 * - 인스펙트(근접 관찰) 모드에서 마우스로 다이얼을 가리키고 휠로 회전
 * - 정답이면 잠금 해제 + 물리로 떨어짐 + 서랍 연동 열림
 * - 에디터/런타임 정렬 로직 분리(피벗 정렬은 런타임 기본, 에디터는 수동 배치 허용)
 */
UCLASS(Blueprintable, BlueprintType)
class SHATTEREDMIND_API AFamilyLockActor : public AActor, public IInteractionInterface
{
    GENERATED_BODY()

public:
    // 생성자: 컴포넌트 생성/초기 배치(카메라/라이트/피벗/다이얼 등)
    AFamilyLockActor();

protected:
    // 액터가 월드에 스폰되거나 게임 시작 시 1회 호출
    virtual void BeginPlay() override;

    // 매 프레임 호출(인스펙트 중 마우스 호버 체크 등)
    virtual void Tick(float DeltaSeconds) override;

    // 에디터에서 속성 변경/배치 시 호출(뷰포트 반영용)
    virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
    // 필요 시 에디터 전용 후킹(프로퍼티 변경 감지/뷰포트 강제 새로고침 등)
    // virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    // virtual void PostEditMove(bool bFinished) override;
#endif

public:
    // ─────────────────────────────────────────────────────────────────────────
    // 상호작용(인터페이스) 관련: 플레이어가 액터를 사용했을 때 호출되는 진입점
    // ─────────────────────────────────────────────────────────────────────────

    /** 플레이어가 자물쇠와 상호작용 시 호출(토글 방식: 인스펙트 진입/종료) */
    void Interact(APolice* _PlayerCharacter);

    /** 인스펙트 모드 진입(시점 전환, 마우스/입력 모드 설정, 조명 켜기) */
    void EnterInspect(APolice* Police);

    /** 인스펙트 모드 종료(시점/입력 원복, 조명 끄기, 상태 초기화) */
    void ExitInspect();

    // ─────────────────────────────────────────────────────────────────────────
    // 입력/로직
    // ─────────────────────────────────────────────────────────────────────────

    /** 마우스 휠 회전 입력(호버 중인 다이얼의 인덱스를 +1/-1) */
    void OnWheelAxis(float Value);

    /** 현재 다이얼 조합이 정답인지 검사(맞으면 UnlockAndDrop 호출) */
    void CheckSolved();

    /** 잠금 해제 후 연출(물리로 떨어짐, 사운드, 서랍 열기 등) */
    void UnlockAndDrop();

    /** 다이얼 피벗들을 KeyPivot 기준 등간격으로 재정렬(런타임 기본, 에디터는 수동 허용) */
    void RecenterDialsToKeyPivot();

    /** 화면상의 마우스 포인터 아래에 위치한 다이얼 탐색/하이라이트 갱신 */
    void UpdateHoverUnderMouse();

public:
    // ─────────────────────────────────────────────────────────────────────────
    // 구성 요소(컴포넌트)
    // ─────────────────────────────────────────────────────────────────────────

    /** 자물쇠 본체 메시(루트). 물리 시뮬레이션/외곽선 렌더링의 대상 */
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* LockMesh;

    /** 인스펙트 전용 카메라(시점 전환 시 사용) */
    UPROPERTY(VisibleAnywhere)
    UCameraComponent* InspectCamera;

    /** 모든 다이얼 피벗들의 기준점(오프셋/정렬 기준) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lock|Components", meta = (AllowPrivateAccess = "true"))
    USceneComponent* KeyPivot;

    /** 각 다이얼의 개별 피벗(회전/정렬의 직접 기준) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lock|Components", meta = (AllowPrivateAccess = "true"))
    TArray<USceneComponent*> DialPivots;

    /** 실제 다이얼(글자 인덱스/스크롤/하이라이트 처리) 커스텀 컴포넌트 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lock|Components", meta = (AllowPrivateAccess = "true"))
    TArray<ULockDialComponent*> Dials;

    // ─────────────────────────────────────────────────────────────────────────
    // 데이터/상태
    // ─────────────────────────────────────────────────────────────────────────

    /** 목표 비밀번호(6글자). BeginPlay에서 대문자/길이 보정(FAMILY 기본) */
    UPROPERTY(EditAnywhere)
    FString Target = TEXT("FAMILY");

    /** 현재 마우스 아래(호버 중)인 다이얼(없을 수 있으므로 Weak) */
    UPROPERTY()
    TWeakObjectPtr<ULockDialComponent> HoverDial;

    /** 인스펙트 모드 여부(카메라/입력/조명 상태 전환에 사용) */
    UPROPERTY()
    bool bInspecting = false;

    /** 이미 해제되었는지 여부(중복 상호작용 방지) */
    UPROPERTY()
    bool bUnlocked = false;

    /** 해제 시 자동으로 열릴 서랍 액터 참조(옵션, 없을 수 있음) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Drawer")
    ADrawerActor* LinkedDrawer;

    /** 잠금 해제 시 재생할 사운드(옵션) */
    UPROPERTY(EditAnywhere)
    USoundBase* UnlockSound;

    /** 인스펙트 중인 플레이어 캐릭터 캐시(안전한 생명주기를 위해 Weak 사용) */
    TWeakObjectPtr<APolice> CachedPolice;

    /** 인스펙트 진입 전의 카메라 뷰 타깃(복귀용) */
    TWeakObjectPtr<AActor> PrevViewTarget;

    // ─────────────────────────────────────────────────────────────────────────
    // 쿼리/유틸
    // ─────────────────────────────────────────────────────────────────────────

    /** 현재 인스펙트 모드 여부(블루프린트/외부에서 상태 확인용) */
    UFUNCTION(BlueprintPure, Category = "Lock")
    bool IsInspecting() const;

    /** 인스펙트 시 자물쇠를 보기 좋게 비추는 포인트 라이트(카메라에 부착) */
    UPROPERTY(VisibleAnywhere, Category = "Inspect")
    UPointLightComponent* InspectLight;

    // 자물쇠 눈금 액터
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lock|Components")
    USceneComponent* TickMarkPivot;



    // [🔶 추가] 상호작용 포커스 기능
    virtual void BeginFocus() override;
    virtual void EndFocus() override;

 // ──────────────────────────────────────────────────────────────
// UI 관련 추가
// ──────────────────────────────────────────────────────────────


    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> WheelHintWidgetClass;

    UPROPERTY()
    UUserWidget* WheelHintWidget;


    //======================1105 남소은 메모해금용 =========================
    bool bHasBeenInspected = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogWidgetClass")
    TSubclassOf<class UDialogWidget> DialogWidgetClass;

    UPROPERTY()
    UDialogWidget* DialogWidget;// 생성된 위젯 인스턴스

    // 입력 감시용 변수
    bool bCloseKeyPressed = false;

    // E키로 닫은 직후 재입력 방지용
    bool bRecentlyClosed = false;
    float ClosedTime = 0.f;


    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PickUp")
    class UBoxComponent* InteractionBox;

};
