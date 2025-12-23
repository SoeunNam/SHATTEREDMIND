#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "JohnFSM.h"                     // EJohnState, UJohnFSM
#include "JohnAnim.generated.h"

UCLASS()
class SHATTEREDMIND_API UJohnAnim : public UAnimInstance
{
    GENERATED_BODY()

public:
    UJohnAnim();

    // ABP에서 전이 조건에 바로 쓰는 상태 값
    UPROPERTY(BlueprintReadOnly, Category = "JFSM")
    EJohnState JohnState;

    // 블렌드스페이스가 필요할 때 쓰는 속도
    UPROPERTY(BlueprintReadOnly, Category = "JFSM")
    float Speed = 0.f;

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
    TWeakObjectPtr<class ACharacter> OwnerChar;
    TWeakObjectPtr<class UJohnFSM>   FSM;
};
