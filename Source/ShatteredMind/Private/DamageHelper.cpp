// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageHelper.h"
#include "EnemyFSM.h"

void UDamageHelper::TryDamageEnemy(const FHitResult& Hit)
{
    // Hit된 액터가 존재하는지 확인
    if (AActor* A = Hit.GetActor())
    {
        // 해당 액터가 EnemyFSM 컴포넌트를 가지고 있는지 검사
        if (UEnemyFSM* FSM = A->FindComponentByClass<UEnemyFSM>())
        {
            // 적 상태머신에 데미지 처리 요청
            FSM->OnDamageProcess();
        }
    }
}
