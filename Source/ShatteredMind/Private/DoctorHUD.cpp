#include "DoctorHUD.h"
#include "DiaryWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

void ADoctorHUD::ShowDiary(const FString& Text, APlayerController* PC)
{
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[HUD] PlayerController is null"));
        return;
    }

    // 이미 열려 있으면 닫기
    if (DiaryWidget && DiaryWidget->IsInViewport())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUD] Closing Diary UI"));
        DiaryWidget->RemoveFromParent();
        DiaryWidget = nullptr;

        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);

        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;
        return;
    }

    // Widget Class 확인
    if (!DiaryWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[HUD] DiaryWidgetClass is NULL! (Check Blueprint setting)"));
        return;
    }

    // Widget 생성
    DiaryWidget = CreateWidget<UDiaryWidget>(PC, DiaryWidgetClass);
    if (!DiaryWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("[HUD] DiaryWidget creation FAILED"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUD] DiaryWidget created successfully"));

    DiaryWidget->AddToViewport(100);
    DiaryWidget->SetVisibility(ESlateVisibility::Visible);
    DiaryWidget->UpdateDiaryText(Text);

    // 이동/카메라 입력 잠금
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);

    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(InputMode);
    PC->bShowMouseCursor = true;

    UE_LOG(LogTemp, Warning, TEXT("[HUD] Diary opened successfully"));
}
