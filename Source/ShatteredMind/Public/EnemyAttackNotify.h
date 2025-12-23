#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "EnemyAttackNotify.generated.h"

/**
 * 적 공격 애니메이션 Notify
 */
UCLASS()
class SHATTEREDMIND_API UEnemyAttackNotify : public UAnimNotify
{
    GENERATED_BODY()

public:
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
