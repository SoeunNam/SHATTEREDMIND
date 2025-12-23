// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyFSM.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnim.generated.h"

// EnemyAnim.h
UCLASS()
class SHATTEREDMIND_API UEnemyAnim : public UAnimInstance
{
    GENERATED_BODY()

public:
    // FSM이 C++에서 바꾸고, AnimBP에서는 읽어서 전이/그래프에 사용
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FSM")
    EEnemyState animState;

    // 공격 몽타주 재생 트리거 (FSM이 true로 셋)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FSM")
    bool bAttackPlay = false;

    // (선택) 공격 몽타주를 에디터에서 꽂아두고 직접 재생하고 싶다면
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim")
    UAnimMontage* AttackMontage = nullptr;

    UFUNCTION(BlueprintCallable, Category = "FSMEvent")
    void OnEndAttackAnimation();

    // (선택) 몽타주 바로 재생하고 싶을 때 호출
    UFUNCTION(BlueprintCallable, Category = "FSMEvent")
    void PlayAttackMontage();


    UPROPERTY(BlueprintReadWrite, Category = "FSM")
    bool bFindDone = false;

    //Find 에니메이션이 끝나는 이벤트 함수 
    UFUNCTION(BlueprintCallable, Category = "FSMEvent")
    void OnEndFindAnimation();

    //피격 애니메이션 재생함수
    UFUNCTION(BlueprintImplementableEvent, Category = "FSMEvent")
    void PlayDamageAnim(FName sectionName);

    //죽음상태 애니메이션 종료여부
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = FSM)
    bool bDieDone = false;

    // 선택: 히트윈도우 On/Off도 원하면
    UFUNCTION() void AnimNotify_AttackHitOn();
    UFUNCTION() void AnimNotify_AttackHitOff();

};