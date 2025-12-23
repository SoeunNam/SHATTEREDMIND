// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAnim.h"
#include "Enemy.h"
#include "EnemyFSM.h"
#include "GameFramework/Pawn.h"

void UEnemyAnim::OnEndAttackAnimation()
{
    bAttackPlay = false;
}

void UEnemyAnim::PlayAttackMontage()
{
    if (AttackMontage && !Montage_IsPlaying(AttackMontage))
    {
        Montage_Play(AttackMontage);
    }
}

void UEnemyAnim::OnEndFindAnimation()
{
    bAttackPlay = false;
}





// 히트윈도우를 콜리전/트레이스로 관리할 경우 사용(선택)
void UEnemyAnim::AnimNotify_AttackHitOn() { /* 무기 콜리전 Enable */ }
void UEnemyAnim::AnimNotify_AttackHitOff() { /* 무기 콜리전 Disable */ }

