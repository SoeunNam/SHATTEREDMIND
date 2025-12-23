#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TimelineComponent.h"
#include "DrawerActor.generated.h"

UCLASS()
class SHATTEREDMIND_API ADrawerActor : public AActor
{
    GENERATED_BODY()

public:
    ADrawerActor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    /** 서랍 메쉬 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drawer")
    UStaticMeshComponent* DrawerMesh;

    /** 타임라인 컴포넌트 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drawer|Timeline")
    UTimelineComponent* OpenTimeline;

    /** 이동량 곡선 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drawer|Timeline")
    UCurveFloat* OpenCurve;

    /** 이동 거리(cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drawer|Timeline")
    float OpenDistance = 40.f;

    /** 열림 여부 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drawer|State")
    bool bIsOpen = false;

    // 의사의 일기장 클래스를 월드상에서 인스턴스화 될 때만 붙이려고 가져옴 서랍 자체에 붙이는거 X
    UPROPERTY(EditInstanceOnly, Category = "Drawer|Link")
    AActor* DiaryActor;


private:
    FVector StartLocation;

    /** 타임라인 콜백 */
    UFUNCTION()
    void HandleDrawerProgress(float Value);

public:
    /** 외부에서 호출 (예: LockActor에서) */
    UFUNCTION(BlueprintCallable, Category = "Drawer")
    void OpenDrawer();
    UFUNCTION(BlueprintCallable, Category = "Drawer")
    void CloseDrawer(); // ? 추가: 닫을 때 호출되는 함수
};
