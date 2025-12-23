#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Enemy.generated.h"

class UEnemyFSM;
class USpotLightComponent;

UCLASS()
class SHATTEREDMIND_API AEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemy();

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void PossessedBy(AController* NewController) override;

    /** FSM 컴포넌트 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FSM")
    UEnemyFSM* fsm = nullptr;

    /** 시야 SpotLight */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sight")
    USpotLightComponent* SightLight = nullptr;

    /** 시야 범위(거리) 및 각도(전각/FOV) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight")
    float SightRange = 800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight")
    float SightAngleFull = 60.f;

    /** 색상(Idle/Alert) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight|Colors")
    FLinearColor SightColor_Idle = FLinearColor(0.9f, 0.9f, 0.9f); // 거의 무색(희미한 백색)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight|Colors")
    FLinearColor SightColor_Alert = FLinearColor(1.f, 0.f, 0.f);   // 빨강

    /** 라이트 on/off, 소켓 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight")
    bool bEnableSightLight = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight")
    FName SightSocketName = TEXT("SightSocket");

    /** 밝기(물리 단위: Candelas) */
    // 기본 강도는 사용하지 않고 상태별 강도를 사용(아래 Idle/Alert Intensity)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight", meta = (ClampMin = "0.0"))
    float SightIntensity = 100.f;

    /** 상태별 밝기 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight|Intensity", meta = (ClampMin = "0.0"))
    float IdleIntensity = 15.f;       // 미발견 시 아주 희미

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight|Intensity", meta = (ClampMin = "0.0"))
    float AlertIntensity = 2500.f;    // 발견 시 확 밝게

    /** 적용 함수 */
    UFUNCTION(BlueprintCallable, Category = "Sight")
    void ApplySightLightEnabled();

    UFUNCTION(BlueprintCallable, Category = "Sight")
    void ApplySightToLight(float NewRange, float NewAngleFull);

    UFUNCTION(BlueprintCallable, Category = "Sight")
    void AimSightAt(AActor* Target, float ExtraPitchDeg = -8.f);

    FORCEINLINE USpotLightComponent* GetSightLight() const { return SightLight; }

    /** 경계 상태에 따른 색/밝기 일괄 적용(외부: FSM에서 호출) */
    UFUNCTION(BlueprintCallable, Category = "Sight")
    void SetAlertVisual(bool bAlert);

    // ── 라이트 가시성(시각용) 오클루전 체크 옵션
    UPROPERTY(EditAnywhere, Category = "Sight|Occlusion")
    bool bHideSightLightWhenOccluded = true;

    UPROPERTY(EditAnywhere, Category = "Sight|Occlusion", meta = (ClampMin = "0.02", ClampMax = "0.5"))
    float SightLightOcclusionInterval = 0.10f; // 10Hz

    FTimerHandle SightLightOcclusionTimer;

    UFUNCTION()
    void UpdateSightLightOcclusion();
};
