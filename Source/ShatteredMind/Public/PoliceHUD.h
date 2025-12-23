// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PoliceHUD.generated.h"

class UMainMenu;
class UInteractionWidget;
struct FInteractableData;
/**
 * 
 */
UCLASS()
class SHATTEREDMIND_API APoliceHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	//
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<UMainMenu> MainMenuClass;
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<UInteractionWidget> InteractionWidgetClass;

	//true면 다른 동작 정지할 수 있게
	bool bIsMenuVisible;

	APoliceHUD();
	void DisplayMenu();
	void OffMenu();

	void ShowInteractionWidget();
	void HideInteractionWidget();
	void UpdateInteractionWidget(const FInteractableData* _InteractableData);


protected:
	UPROPERTY()
	UMainMenu* MainMenuWidget;

	UPROPERTY()
	UInteractionWidget* InteractionWidget;
	
	
	virtual void BeginPlay() override;

};
