// Fill out your copyright notice in the Description page of Project Settings.


#include "PoliceMonologueWidget.h"
#include "Components/TextBlock.h"


void UPoliceMonologueWidget::SetMonologueText(const FString& NewText)
{
    if (MonologueText)
    {
        MonologueText->SetText(FText::FromString(NewText));
    }
}
