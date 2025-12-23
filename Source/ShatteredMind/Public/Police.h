// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CollisionQueryParams.h"
#include "PoliceHUD.h"
#include "PoliceMonologueWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Internationalization/Text.h"
#include "Blueprint/UserWidget.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h" 
#include "Police.generated.h"

class IInteractionInterface;
class AFamilyLockActor; // 20251101 박희빈 자물쇠 액터 클래스
struct FInteractableData;

USTRUCT()
struct FInteractionData
{
	GENERATED_BODY()

	FInteractionData() : CurrentInteractable(nullptr), LastInteractionCheckTime(0.0f)
	{

	}

	//현재 상호작용 가능한 액터인지 확인
	UPROPERTY()
	AActor* CurrentInteractable;

	//라인트레이스 쏠 주기? 매 프레임마다 쏠 필요는 없기 때문
	UPROPERTY()
	float LastInteractionCheckTime;
};

UCLASS()
class APolice : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APolice();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Skeletal Mesh Components (필요한 파츠만)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Parts")
	USkeletalMeshComponent* BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Parts")
	USkeletalMeshComponent* JacketMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Parts")
	USkeletalMeshComponent* JeansMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Parts")
	USkeletalMeshComponent* ShoesMesh;

public:
	//스프링 암 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* springArmComp;
	//카메라
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* policeCamComp;
	//경찰 IMC
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputMappingContext* imc_Police;

	//카메라 y축 회전
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* ia_LookUp;
	//카메라 x축 회전
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* ia_Turn;
	//카메라 zoom in
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* ia_Zoom;

	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* ia_PoliceMemo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	//카메라 좌우 회전 입력 처리
	void Turn(const struct FInputActionValue& _inputValue);
	//카메라 상하 회전 입력 처리
	void LookUp(const struct FInputActionValue& _inputValue);
	//카메라 zoom입력 처리
	void Zoom(const FInputActionValue& _inputValue);


	//경찰 이동
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_PoliceMove;
	//이동 속도
	UPROPERTY(EditAnywhere, Category = "Input")
	float walkSpeed = 400;
	//이동 방향
	FVector direction;
	void Move(const struct FInputActionValue& _inputValue);
	//점프
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_PoliceJump;
	//점프 입력 이벤트 처리 함수
	void InputJump(const struct FInputActionValue& _inputValue);


	//상호작용관련
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* ia_Interaction;
	//protected:
		//CurrentInteractable이랑 비교
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TScriptInterface<IInteractionInterface> TargetInteractable;

	//라인트레이스 얼마만에 한 번 씩 쏠 지
	float InteractionCheckFrequecy;

	//라인트레이스 얼마나 멀리 쏠 지
	float InteractionCheckDistance;
	//타이머 관리
	FTimerHandle TimerHandle_Interaction;
	//상호작용데이터
	FInteractionData InteractionData;

	//지금 상호작용하고 있는지 아닌지를 return
	//타이머가 0으로 설정되어있으면 (=그냥 누르면 끝인거는) 상관없음
	////////나도 딱히 지속입력 안해서 필요없을듯 나중에 없앨 수 있으면 없애기?
	bool IsInteracting() const
	{
		return GetWorldTimerManager().IsTimerActive(TimerHandle_Interaction);
	}


	//매 틱마다 호출되어 라인트레이스를 쏘고 상호작용 가능한 액터인지 확인하는 함수
	//라인트레이스 생성(시작지점, 끝지점 지정)
	void PerformInteractionCheck();
	//상호작용 가능한 액터를 찾음
	void FoundInteractable(AActor* _NewInteractable);
	//상호작용 가능한 액터를 찾지 못함
	//아무 것도 못찾았을 때(시야 안에 아무 것도 없을 때)
	// 상호작용 도중 딴데로 시야 돌렸을 때 이런 상황 포함
	void NoInteractableFound();
	//플레이어 입장에서의 상호작용
	void PoliceBeginInteract();
	void PoliceEndInteract();
	void PoliceInteract();

	//메모장 ui 온오프
	void PoliceMemoOnOff();

protected:
	UPROPERTY()
	APoliceHUD* HUD;
public:

	//경찰 메모 위젯 바인딩 위해서
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> PoliceMemoOffWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UPoliceMemoWidget> PoliceMemoOnWidgetClass;

	class UUserWidget* PoliceMemoOffWidget;

	class UPoliceMemoWidget* PoliceMemoOnWidget;

	bool bIsMemoOnVisible = false;

	void UnlockMemoWrap(int32 MemoIndex);
	UPROPERTY()
	TArray<bool> bMemoUnlocked;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> MonologueWidgetClass;

	UPROPERTY()
	UPoliceMonologueWidget* MonologueWidget;

	// 마지막 모놀로그 타이머
	FTimerHandle MonologueRepeatTimer;

	UPROPERTY()
	TArray<FString> MonologueLines;

	bool bHasInteracted;
	bool AllMemosUnlocked() const;

	UFUNCTION()
	void ShowMonologueWidget();

	UFUNCTION()
	void StartMonologueRepeat();

	UFUNCTION()
	void StopMonologueRepeat();

	// Overlap 이벤트
	UFUNCTION()
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor,
		class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
	UPROPERTY()
	AActor* LastSeenInteractable = nullptr; // 마지막 대화 NPC 저장용



	// 캐릭터 충돌과 물리 반응 관련 설정 
	//  20251029 박희빈 캐릭터 튕겨나감 현상 제거 위함
	void SetupCollisionSettings();


	// ================= Footstep Audio =================
public:

	// ==20251101 박희빈 플레이어 경찰이 자물쇠를 열기 위해서는 마우스 휠로 다이얼 입력을 받아야함.===========

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	class UInputAction* ia_LockWheel;


	void OnLockWheelInput(const FInputActionValue& Value);

	UPROPERTY()
	class AFamilyLockActor* CurrentLockActor;

	// =========================================================================================================

	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* MemoOnOffSound;
};
