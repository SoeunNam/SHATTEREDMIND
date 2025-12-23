#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DoctorGameModeBase.generated.h"

UCLASS()
class SHATTEREDMIND_API ADoctorGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADoctorGameModeBase();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

public:
    // 플레이어 사망 시 Doctor가 호출해줄 함수
    UFUNCTION(BlueprintCallable)
    void OnPlayerDied();

    // 외부(Enemy 쪽)에서 적 사망 체크할 필요 있으면 여기 써도 됨
    UFUNCTION(BlueprintCallable)
    void OnEnemyKilled(); // KillCount 따로 안 써도 되면 생략 가능

private:
    // ===== 타이머로 게임 끝나는 조건 =====
    UPROPERTY(EditAnywhere, Category = "Game End")
    float TimeLimit = 300.f; // 5분 제한
    float ElapsedTime = 0.f;

    // ===== 적 관리 =====
    int32 AliveEnemies = 0;
    UFUNCTION()
    void HandleEnemyDestroyed(AActor* DestroyedActor);

    // ===== 게임 종료 중복 방지 =====
    bool bGameEnded = false;

    // ===== UI 위젯들 =====
public:
    // 클리어 위젯 (모든 적 처치 등)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> ClearWidgetClass;

    // 시간초과 위젯
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> TimeOverWidgetClass;

    // 즉사(0킬 사망) 위젯
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> Dead_NoKill_WidgetClass;

    // 분투(1킬 이상 하고 사망) 위젯
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> Dead_WithKills_WidgetClass;

private:
    void ShowEndWidget(TSubclassOf<class UUserWidget> WidgetToShow);
    void CheckTimeOver();
    void CheckAllEnemiesDead();

private:
    int32 InitialEnemyCount = 0;  // ★ 추가
};
