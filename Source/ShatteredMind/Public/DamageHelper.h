// DamageHelper.h
// ───────────────────────────────────────────────
// 적에게 데미지를 주는 공통 함수 모음.
// 블루프린트나 다른 클래스 어디서든 불러올 수 있게
// BlueprintFunctionLibrary 로 구현.
// ───────────────────────────────────────────────

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DamageHelper.generated.h"

/**
 * UDamageHelper
 * - 전역적으로 접근 가능한 static 함수 집합
 * - 현재는 "적 데미지 처리" 기능만 포함
 */
UCLASS()
class SHATTEREDMIND_API UDamageHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * TryDamageEnemy
	 * - 라인트레이스 등의 Hit 결과를 받아서
	 * - Hit된 액터가 EnemyFSM 컴포넌트를 가지고 있으면
	 *   데미지 처리 함수를 호출한다.
	 * - 블루프린트에서도 사용 가능 (BlueprintCallable).
	 *
	 * @param Hit : 충돌 결과 (라인트레이스, 충돌 이벤트 등)
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage")
	static void TryDamageEnemy(const FHitResult& Hit);

};