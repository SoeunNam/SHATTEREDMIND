// Fill out your copyright notice in the Description page of Project Settings.


#include "PoliceMemoOffWidget.h"
#include "Components/TextBlock.h"

void UPoliceMemoOffWidget::SetUpdateMarkVisible(bool bVisible)
{
	if (TextBlock_UpdateMark)
	{
		TextBlock_UpdateMark->SetVisibility(
			bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}
