// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InteractionInterface.h"
#include "DialogTypes.h"
#include "DialogWidget.h"
#include "Blueprint/UserWidget.h"
#include "PoliceMemoWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Internationalization/Text.h"
#include "NPC.generated.h"

UCLASS()
class SHATTEREDMIND_API ANPC : public ACharacter, public IInteractionInterface
{
	GENERATED_BODY()

public:
	// 책이나 아이템 설명 (블루프린트에서 개별 지정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText ItemDescription;

	UPROPERTY(EditInstanceOnly, Category = "NPC")
	FInteractableData InstanceInteractableData;

	// NPC 종류 구분용 (블루프린트에서 세팅 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	FString NPCType;

private:
	FRotator OriginalRotation; //원래 방향 저장
	FRotator TargetRotation; // 바라볼 목표 방향
	bool bRotateToPlayer = false; // 플레이어 바라보는 중인지
	bool bReturnToOriginal = false;// 원래 방향으로 복귀 중인지

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float OpenSpeed = 2.0f;  // 회전 속도 (문처럼 조절 가능)


private:
	APolice* CachedPlayer;  // 상호작용 중인 플레이어 저장 (스프링암컴포넌트 복구용)
	float OriginalArmLength;
	float DesiredArmLength = 100.f;// 대화 시 카메라 거리

public:
	// Sets default values for this character's properties
	ANPC();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginFocus() override;
	virtual void EndFocus() override;
	virtual void BeginInteract() override;
	virtual void EndInteract() override;
	virtual void Interact(APolice* _Playercharacter) override;


private:


	//다음 대사로 이동
	void ShowNextLine();
	//현재 대사 표시
	void ShowDialogLine();

public:
	//현재 대화 상태
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	bool bIsInteracting = false;

	// 대화 데이터
	UPROPERTY(EditAnywhere, Category = "Dialog")
	TArray<FDialogLine> DialogLines;//C++에서 직접 관리

	int32 CurrentDialogIndex = 0;//현재 대화 인덱스

	//블루프린트에서 DialogWidget 클래스 연결
	UPROPERTY(EditAnywhere, Category = "Dialog")
	TSubclassOf<UDialogWidget> DialogWidgetClass; // 여기서 WBP_DialogWidget 연결

	UPROPERTY()
	UDialogWidget* DialogWidget;// 생성된 위젯 인스턴스

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialog")
	bool bHasTalkedBefore = false;// 이전에 대화했는지 여부

	//NPC 타입에 따라 대사 세팅
	void InitializeDialogLines();



	float LastInteractTime = -9999.f;
	float InteractDebounce = 0.15f; // 150ms 디바운스 (필요시 조정)

	// ShowNextLine 연속 보호
	UPROPERTY()
	float LastShowLineTime = -9999.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Interaction")
	float ShowLineCooldown = 0.08f; // 80ms (필요시 조정)


protected:
	// 사건 파일 위젯 클래스 (BP에서 연결)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CaseFile")
	TSubclassOf<class UUserWidget> CaseFileWidgetClass;

	// 현재 생성된 사건 파일 위젯
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UUserWidget* CurrentCaseFileWidget;

	// 순경 NPC 여부 (또는 NPCType == "Police"로 판별해도 됨)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CaseFile")
	bool bIsPoliceNPC = false;

	// 대화 일시정지 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CaseFile")
	bool bDialogPaused = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CaseFile")
	FText CaseFileText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CaseFileTextBlock;

	bool bCaseFileShown = false;

private:
	// 사건파일 위젯 표시
	void ShowCaseFileWidget();

	// 사건파일 닫기 (E키)
	void CloseCaseFileWidget();

public:
	//순경 npc 자동시작 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoStart")
	bool bAutoStart = false;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool IsInteracting() const { return bIsInteracting; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoStart")
	bool bUsesFSM = false;

public:

	// 20251029 희빈 플레이어와 충돌시 npc 날라가는 현상 제어위함==

	void SetupCollisionForStaticNPC();   // 가만히 있는 NPC용
	void SetupCollisionForMovingNPC();   // AI로 움직이는 NPC용

	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* DialogEndSound;
};
