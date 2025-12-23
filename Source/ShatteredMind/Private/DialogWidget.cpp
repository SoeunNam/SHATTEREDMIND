// Fill out your copyright notice in the Description page of Project Settings.

#include "DialogWidget.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Engine/Font.h"

void UDialogWidget::UpdateDialog(const FDialogLine& DialogLine)
{
    if (SpeakerText)
    {
        SpeakerText->SetText(DialogLine.SpeakerName);


    }

    if (LineText)
    {
        LineText->SetText(DialogLine.LineText);


    }
}
