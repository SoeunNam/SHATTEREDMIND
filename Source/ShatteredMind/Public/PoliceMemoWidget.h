// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "PoliceMemoWidget.generated.h"

/**
 *
 */
UCLASS()
class SHATTEREDMIND_API UPoliceMemoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	//수첩 해금
	void UnlockMemo(int32 MemoIndex);
	void InitializeMemoTexts();
	// 위젯이 Construct 되었을 때 호출되는 오버라이드
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MemoText_1;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MemoText_2;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MemoText_3;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MemoText_4;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MemoText_5;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MemoText_6;
};
