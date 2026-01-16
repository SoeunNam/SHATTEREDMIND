// Fill out your copyright notice in the Description page of Project Settings.


#include "Police.h"
#include <GameFramework/SpringArmComponent.h>
#include <Camera/CameraComponent.h>
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "InteractionInterface.h"
#include "PoliceHUD.h"
#include "Engine/Engine.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "PickUp.h"
#include "Internationalization/Text.h"
#include "Kismet/GameplayStatics.h"
#include "PoliceMemoOffWidget.h"
#include "PoliceMonologueWidget.h"
#include "PoliceMemoWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "NPC.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h" // 이동 컴포넌트
#include "Components/CapsuleComponent.h"              // 캡슐 콜리전
#include "FamilyLockActor.h" // 20251101 박희빈 자물쇠 액터 클래스
#include "PickUp.h"


// Sets default values
APolice::APolice()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//1.스켈레탈메시 데이터를 불러오고 싶다.
	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempMesh(TEXT("/Script/Engine.SkeletalMesh'/Game/Survival_Character/Meshes/SK_Survival_Character_Body.SK_Survival_Character_Body'"));
	if (TempMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(TempMesh.Object);
		//2. Mesh컴포넌트이 위치와 회전값을 설정하고 싶다.
		GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -90), FRotator(0, -90, 0));
	}
	//3. Police 카메라 붙이기
	//3-1. SpringArm 컴포넌트 붙이기
	springArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	springArmComp->SetupAttachment(RootComponent);
	springArmComp->SetRelativeLocation(FVector(0, 70, 90));
	springArmComp->TargetArmLength = 200;
	springArmComp->bUsePawnControlRotation = true;
	//3-2. Camera 컴포넌트 붙이기
	policeCamComp = CreateDefaultSubobject<UCameraComponent>(TEXT("PoliceCamComp"));
	policeCamComp->SetupAttachment(springArmComp);
	policeCamComp->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = true;

	InteractionCheckFrequecy = 0.1;
	InteractionCheckDistance = 225.0f;

	BaseEyeHeight = 74.0;

	// Body
	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetMesh());
	BodyMesh->SetLeaderPoseComponent(GetMesh());
	// SkeletalMesh 로드
	static ConstructorHelpers::FObjectFinder<USkeletalMesh>
		BodyAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Survival_Character/Meshes/SK_Survival_Character_Body.SK_Survival_Character_Body'"));
	if (BodyAsset.Succeeded())
	{
		BodyMesh->SetSkeletalMesh(BodyAsset.Object);
	}


	// Jacket
	JacketMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("JacketMesh"));
	JacketMesh->SetupAttachment(GetMesh());
	JacketMesh->SetLeaderPoseComponent(GetMesh());
	// SkeletalMesh 로드
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> JacketAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Survival_Character/Meshes/SK_Survival_Character_Jacket.SK_Survival_Character_Jacket'"));
	if (JacketAsset.Succeeded())
	{
		JacketMesh->SetSkeletalMesh(JacketAsset.Object);
	}

	// Jeans
	JeansMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("JeansMesh"));
	JeansMesh->SetupAttachment(GetMesh());
	JeansMesh->SetLeaderPoseComponent(GetMesh());
	// SkeletalMesh 로드
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> JeansAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Survival_Character/Meshes/SK_Survival_Character_Jeans.SK_Survival_Character_Jeans'"));
	if (JeansAsset.Succeeded())
	{
		JeansMesh->SetSkeletalMesh(JeansAsset.Object);
	}

	// Shoes
	ShoesMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShoesMesh"));
	ShoesMesh->SetupAttachment(GetMesh());
	ShoesMesh->SetLeaderPoseComponent(GetMesh());
	// SkeletalMesh 로드
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ShoesAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Survival_Character/Meshes/SK_Survival_Character_Shoes.SK_Survival_Character_Shoes'"));
	if (ShoesAsset.Succeeded())
	{
		ShoesMesh->SetSkeletalMesh(ShoesAsset.Object);
	}

	// 캐릭터 초기화 시 충돌 설정/캐릭터 튕겨나감 현상 제거 위함
	SetupCollisionSettings();


}


// Called when the game starts or when spawned
void APolice::BeginPlay()
{
	Super::BeginPlay();

	auto pc = Cast<APlayerController>(Controller);
	if (pc)
	{
		auto subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer());
		if (subsystem)
		{
			subsystem->AddMappingContext(imc_Police, 0);
		}
	}

	HUD = Cast<APoliceHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());

	FString CurrentLevelName = GetWorld()->GetMapName();
	CurrentLevelName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix); //PIE 접두어 제거

	if (CurrentLevelName == TEXT("Map_Final_Police"))  //레벨 이름 정확히 넣기
	{
		if (PoliceMemoOffWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Map_Final_Police: %s"), PoliceMemoOffWidget ? TEXT("Created") : TEXT("NULL"));

			PoliceMemoOffWidget = CreateWidget<UUserWidget>(GetWorld(), PoliceMemoOffWidgetClass);
			if (PoliceMemoOffWidget)
			{
				PoliceMemoOffWidget->SetVisibility(ESlateVisibility::Visible);  //확실히 Visible
				PoliceMemoOffWidget->AddToViewport(0);  //항상 화면에 있도록 추가
			}
		}
	}


	if (PoliceMemoOnWidgetClass)
	{
		PoliceMemoOnWidget = CreateWidget<UPoliceMemoWidget>(GetWorld(), PoliceMemoOnWidgetClass);
		if (PoliceMemoOnWidget)
		{

			PoliceMemoOnWidget->AddToViewport(5); //항상 화면에 있도록 추가
			PoliceMemoOnWidget->SetVisibility(ESlateVisibility::Hidden); //처음에는 숨김
			if (UPoliceMemoWidget* MemoWidget = Cast<UPoliceMemoWidget>(PoliceMemoOnWidget))
			{
				MemoWidget->MemoText_1->SetVisibility(ESlateVisibility::Hidden);
				MemoWidget->MemoText_2->SetVisibility(ESlateVisibility::Hidden);
				MemoWidget->MemoText_3->SetVisibility(ESlateVisibility::Hidden);
				MemoWidget->MemoText_4->SetVisibility(ESlateVisibility::Hidden);
				MemoWidget->MemoText_5->SetVisibility(ESlateVisibility::Hidden);
				MemoWidget->MemoText_6->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}

	//메모장 잠금 초기화
	bMemoUnlocked.Init(false, 6); //크기 5, 모두 false
	
		//독백 대사 초기화
	MonologueLines = {
	TEXT("자물쇠 암호는 다섯 글자의 단어..."),
	TEXT("암호... 알 것 같아. 어쩌면?"),
	TEXT("복도 끝 방이라고 했었지."),
	TEXT("자물쇠를 맞춰봐야겠어.")
	};
	
}

void APolice::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//StopMonologueRepeat(); //반복 타이머 안전하게 정리
	Super::EndPlay(EndPlayReason); //부모 클래스 기본 EndPlay 호출
}


// Called every frame
void APolice::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Police 이동 처리
	//등속 운동, P(결과 위치) = P0(현재 위치) + v(속도)t(시간)
	direction = FTransform(GetControlRotation()).TransformVector(direction);
	AddMovementInput(direction);
	direction = FVector::ZeroVector;

	//마지막 라인트레이스 생성 이후 생성 주기만큼 시간이 지나면
	if (GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequecy)
	{
		//상호작용가능 액터인지 확인하는 함수
		PerformInteractionCheck();
	}

	//이동처리 
	direction = FVector::ZeroVector;

}

// Called to bind functionality to input
void APolice::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto PlayerInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (PlayerInput)
	{
		PlayerInput->BindAction(ia_Turn, ETriggerEvent::Triggered, this, &APolice::Turn);
		PlayerInput->BindAction(ia_LookUp, ETriggerEvent::Triggered, this, &APolice::LookUp);
		PlayerInput->BindAction(ia_PoliceMove, ETriggerEvent::Triggered, this, &APolice::Move);
		PlayerInput->BindAction(ia_PoliceJump, ETriggerEvent::Triggered, this, &APolice::InputJump);
		PlayerInput->BindAction(ia_Zoom, ETriggerEvent::Triggered, this, &APolice::Zoom);
		PlayerInput->BindAction(ia_PoliceMemo, ETriggerEvent::Completed, this, &APolice::PoliceMemoOnOff);
		PlayerInput->BindAction(ia_Interaction, ETriggerEvent::Completed, this, &APolice::PoliceBeginInteract);
		PlayerInput->BindAction(ia_LockWheel, ETriggerEvent::Triggered, this, &APolice::OnLockWheelInput); //자물쇠를 마우스 휠로 조작


	}
}
//화면
void APolice::Turn(const struct FInputActionValue& _inputValue)
{
	float value = _inputValue.Get<float>();
	AddControllerYawInput(value);
}
void APolice::LookUp(const struct FInputActionValue& _inputValue)
{
	float value = _inputValue.Get<float>();
	AddControllerPitchInput(value);
}
//마우스 휠 위로 하면 줌 인, 아래로하면 줌 아웃
void APolice::Zoom(const FInputActionValue& _inputValue)
{
	// 1D Axis 값을 float으로 꺼냄
	float value = _inputValue.Get<float>();
	if (value == 0.f)
	{
		return;
	}
	
	springArmComp->TargetArmLength = FMath::Clamp(springArmComp->TargetArmLength - value * 20.f, 100.f, 400.f);
}

//이동함수
void APolice::Move(const struct FInputActionValue& _inputValue)
{
	FVector2D value = _inputValue.Get<FVector2D>();
	//상하 입력 이벤트 처리
	direction.X = value.X;
	//좌우 입력 이벤트 처리
	direction.Y = value.Y;
}
//점프 함수
void APolice::InputJump(const FInputActionValue& _inputValue)
{
	Jump();
}

//매 틱마다 호출되어 라인트레이스를 쏘고 상호작용 가능한 액터인지 확인하는 함수
//라인트레이스 생성(시작지점, 끝지점 지정)
void APolice::PerformInteractionCheck()
{
	//이미 NPC와 대화 중이라면 라인트레이스 안 함
	if (InteractionData.CurrentInteractable)
	{
		APickUp* PickUpActor = Cast<APickUp>(InteractionData.CurrentInteractable);
		ANPC* NPCActor = Cast<ANPC>(InteractionData.CurrentInteractable);
		if ((NPCActor && NPCActor->IsInteracting()) || (PickUpActor && PickUpActor->bIsInteracting))
		{
			return; //대화 중엔 감지 중단
		}
	}

	//라인트레이스 생성 시점
	InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();
	//라인트레이스 시작 지점
	FVector TraceStart = GetPawnViewLocation();
	//라인트레이스 끝나는 지점
	//ViewRotation은 CharactorController에서 나옴, PlayerPawnMesh아니고
	//rotation을 vector로 변환한 값에 라인트레이스 길이를 곱하는 과정
	FVector TraceEnd = TraceStart + (GetViewRotation().Vector() * InteractionCheckDistance);
	//라인트레이스 표시하기, 테스트, 디버깅용
	//DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 1.0f, 0, 2.0f);

	//라인트레이스 인식을 위한 조건
	FCollisionQueryParams QueryParams;
	//나 자신은 맞으면 안되니까
	QueryParams.AddIgnoredActor(this);
	//라인트레이스 맞춘 정보 저장
	FHitResult Hit;
	//라인트레이스가 액터 맞추면
	FHitResult TraceHit;
	if (GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		AActor* HitActor = TraceHit.GetActor();
		if (HitActor && HitActor->GetClass()->ImplementsInterface(UInteractionInterface::StaticClass()))
		{
			// 새로운 액터 발견
			if (HitActor != InteractionData.CurrentInteractable)
			{
				FoundInteractable(HitActor);
			}
		}

		if (TraceHit.GetActor()->GetClass()->ImplementsInterface(UInteractionInterface::StaticClass()))
		{
			const float Distance = (TraceStart - TraceHit.ImpactPoint).Size();

			if (TraceHit.GetActor() != InteractionData.CurrentInteractable && Distance < InteractionCheckDistance)
			{
				FoundInteractable(TraceHit.GetActor());
				return;
			}

			// 이미 보고 있는 액터라면 현상 유지
			if (TraceHit.GetActor() == InteractionData.CurrentInteractable)
			{
				return;
			}
		}
		else
		{
			//상호작용 불가 액터
			NoInteractableFound();
		}
	}
	else
	{
		// 상호작용 불가 액터를 맞았을 때
		if (InteractionData.CurrentInteractable)
		{
			IInteractionInterface* Interactable = Cast<IInteractionInterface>(InteractionData.CurrentInteractable);
			if (Interactable)
			{
				Interactable->EndFocus();
			}

			InteractionData.CurrentInteractable = nullptr;
			TargetInteractable = nullptr;
			HUD->HideInteractionWidget();
		}

		NoInteractableFound();
	}
}

//TraceHit.GetActor() != InteractionData.CurrentInteractable 일때
//새 상호작용 가능 액터를 찾았을 때 호출
void APolice::FoundInteractable(AActor* _NewInteractable)
{
	//현재 상호작용 중이라면(꾹누르기/지속입력)
	if (IsInteracting())
	{
		//상호작용 중단 먼저/꼬임 방지
		PoliceEndInteract();
	}
	//이전에 보고있는 상호작용 대상이 있다면 그 대상의 포커스 해제
	if (InteractionData.CurrentInteractable)
	{
		TargetInteractable = InteractionData.CurrentInteractable;
		TargetInteractable->EndFocus();
	}
	//새로운 상호작용 대상을 현재 _NewInteractable로 등록
	InteractionData.CurrentInteractable = _NewInteractable;
	TargetInteractable = _NewInteractable;

	HUD->UpdateInteractionWidget(&TargetInteractable->InteractableData);

	//새로 찾은 대상에 포커스를 시작(테두리표시?)
	TargetInteractable->BeginFocus();
}

//상호작용 가능한 액터를 찾지 못함
//아무 것도 못찾았을 때(시야 안에 아무 것도 없을 때)
// 상호작용 도중 딴데로 시야 돌렸을 때 이런 상황 포함
void APolice::NoInteractableFound()
{
	//현재 꾹누르기 지속 상호작용 중이면(근데 대상 없으면)
	if (IsInteracting())
	{
		//진행 중인 상호작용 타이머 초기화??
		GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);
	}
	//이전에 바라보던 상호작용 대상이 남아있다면 그 대상의 포커스 해제,
	//하이라이트나 ui잔상 안남게 처리
	if (InteractionData.CurrentInteractable)
	{
		//유효성 체크(직전에 쳐다본 물건은 하이라이트 해제해주기)
		if (IsValid(TargetInteractable.GetObject()))
		{
			TargetInteractable->EndFocus();
		}

		HUD->HideInteractionWidget();

		//HUD에서 상호작용 위젯 숨기기
		//이전에 바라보던 오브젝트를 더 이상 현재 상호작용 대상을 취급하지 않기 위해서
		//명시적으로 초기화 해줌
		InteractionData.CurrentInteractable = nullptr;
		TargetInteractable = nullptr;
	}
}
void APolice::PoliceBeginInteract()
{
	//상호작용 시작 이후로 대상에 대한 상호작용 가능 상태에 변화 없음 체크
	//PerformInteractionCheck();
	//이전에 바라보던 상호작용 대상이 남아있다면 그 대상의 포커스 해제,
	//하이라이트나 ui잔상 안남게 처리
	if (InteractionData.CurrentInteractable)
	{
		//유효성 체크
		if (IsValid(TargetInteractable.GetObject()))
		{
			//상호작용 대상의 BeginInteract 이벤트 실행
			TargetInteractable->BeginInteract();
			//상호작용 시간이 거의 0이면 즉시 상호작용 완료 처리
			if (FMath::IsNearlyZero(TargetInteractable->
				InteractableData.InteractionDuration, 0.1f))//에러 tolerance가 0.1
			{
				//상호작용 완료 동작 수행
				PoliceInteract();
			}
			else
			{
				//상호작용 시간이 존재하면 타이머를 설정하여 일정시간 후 PoliceInteract실행
				GetWorldTimerManager().SetTimer(TimerHandle_Interaction,//타이머 핸들
					this, &APolice::PoliceInteract,//호출할 함수
					TargetInteractable->InteractableData.InteractionDuration,//대기시간
					false);//반복실행여부 false
			}
		}
	}
}
void APolice::PoliceEndInteract()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

	if (IsValid(TargetInteractable.GetObject()))
	{
		TargetInteractable->EndInteract();
	}
}

void APolice::PoliceInteract()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Interaction);

	if (IsValid(TargetInteractable.GetObject()))
	{
		TargetInteractable->Interact(this);
	}

}

void APolice::PoliceMemoOnOff()
{
	// 위젯이 생성되어 있는지 체크
	if (!PoliceMemoOnWidget && PoliceMemoOnWidgetClass)
	{
		PoliceMemoOnWidget = CreateWidget<UPoliceMemoWidget>(GetWorld(), PoliceMemoOnWidgetClass);
		if (PoliceMemoOnWidget)
		{
			PoliceMemoOnWidget->AddToViewport(5);
			PoliceMemoOnWidget->SetVisibility(ESlateVisibility::Hidden); // 처음에는 숨김
			UE_LOG(LogTemp, Warning, TEXT("PoliceMemoOnWidget created and added to viewport/HIDDEN."));
		}
	}

	if (!PoliceMemoOffWidget && PoliceMemoOffWidgetClass)
	{
		PoliceMemoOffWidget = CreateWidget<UUserWidget>(GetWorld(), PoliceMemoOffWidgetClass);
		if (PoliceMemoOffWidget)
		{
			PoliceMemoOffWidget->AddToViewport(0);
			PoliceMemoOffWidget->SetVisibility(ESlateVisibility::Visible); // 기본 ON 상태
		}
	}

	// 현재 ON 위젯이 화면에 보이는지 체크
	bIsMemoOnVisible = PoliceMemoOnWidget && PoliceMemoOnWidget->GetVisibility() == ESlateVisibility::Visible;

	// 토글 처리
	if (bIsMemoOnVisible)
	{
		PoliceMemoOffWidget->SetVisibility(ESlateVisibility::Visible);
		PoliceMemoOnWidget->SetVisibility(ESlateVisibility::Hidden);

		if (MemoOnOffSound) // 미리 UPROPERTY로 선언해둔 사운드
		{
			UGameplayStatics::PlaySound2D(this, MemoOnOffSound);
		}
	}
	else
	{
		PoliceMemoOffWidget->SetVisibility(ESlateVisibility::Hidden);
		PoliceMemoOnWidget->SetVisibility(ESlateVisibility::Visible);

		if (MemoOnOffSound) // 미리 UPROPERTY로 선언해둔 사운드
		{
			UGameplayStatics::PlaySound2D(this, MemoOnOffSound);
		}

		// ON 위젯 열면 점 끄기
		if (UPoliceMemoOffWidget* OffWidget = Cast<UPoliceMemoOffWidget>(PoliceMemoOffWidget))
		{
			OffWidget->SetUpdateMarkVisible(false);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("PoliceMemo toggled: %s"), bIsMemoOnVisible ? TEXT("OFF") : TEXT("ON"));

}

void APolice::UnlockMemoWrap(int32 MemoIndex)
{
	if (MemoIndex < 1 || MemoIndex > 6) return;

	// 이미 해금됐으면 패스
	if (bMemoUnlocked[MemoIndex - 1]) return;

	bMemoUnlocked[MemoIndex - 1] = true;

	// OFF 위젯에 업데이트 표시
	if (PoliceMemoOffWidget)
	{
		if (UPoliceMemoOffWidget* OffWidget = Cast<UPoliceMemoOffWidget>(PoliceMemoOffWidget))
		{
			OffWidget->SetUpdateMarkVisible(true);   // 초록 점 표시 ON
			//UE_LOG(LogTemp, Warning, TEXT("Update mark ON (Memo %d)"), MemoIndex);
		}
	}

	if (PoliceMemoOnWidget)
	{
		PoliceMemoOnWidget->UnlockMemo(MemoIndex);
	}

}

bool APolice::AllMemosUnlocked() const
{
	for (bool bUnlocked : bMemoUnlocked)
	{
		if (!bUnlocked)
			return false;
	}
	return true;
}

void APolice::ShowMonologueWidget()
{
	if (!MonologueWidgetClass || MonologueLines.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("MonologueWidgetClass가 없거나 MonologueLines가 비어있음!"));
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowMonologueWidget: PlayerController nullptr"));
		return;
	}

	if (!MonologueWidget)
	{
		MonologueWidget = CreateWidget<UPoliceMonologueWidget>(PC, MonologueWidgetClass);
		if (!MonologueWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("MonologueWidget 생성 실패!"));
			return;
		}
	}

	static int32 CurrentIndex = 0;

	//현재 문장 선택
	FString SelectedText = MonologueLines[CurrentIndex];
	MonologueWidget->SetMonologueText(SelectedText);

	// 다음에 보여줄 인덱스 증가
	CurrentIndex++;
	if (CurrentIndex >= MonologueLines.Num())
	{
		CurrentIndex = 0; // 마지막 이후엔 다시 처음으로
	}

	if (!MonologueWidget->IsInViewport())
		MonologueWidget->AddToViewport(4);

	MonologueWidget->SetVisibility(ESlateVisibility::Visible);


	// 3초 후 자동 제거
	FTimerHandle RemoveHandle;
	GetWorldTimerManager().SetTimer(
		RemoveHandle,
		[this]()
		{
			if (MonologueWidget)
				MonologueWidget->SetVisibility(ESlateVisibility::Hidden);
		},
		3.0f,
		false
	);
}

// 반복 시작 (모든 메모 해금 후 호출)
void APolice::StartMonologueRepeat()
{
	if (!MonologueWidgetClass || MonologueLines.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("MonologueWidgetClass가 없거나 MonologueLines가 비어있음!"));
		return;
	}

	// 이미 타이머가 돌아가고 있으면 중복 방지
	if (GetWorldTimerManager().IsTimerActive(MonologueRepeatTimer))
		return;

	GetWorldTimerManager().SetTimer(
		MonologueRepeatTimer,
		this,
		&APolice::ShowMonologueWidget,
		8.0f,
		true
	);

	UE_LOG(LogTemp, Warning, TEXT("MonologueRepeatTimer"));

}

void APolice::StopMonologueRepeat()
{
	GetWorldTimerManager().ClearTimer(MonologueRepeatTimer);
	if (MonologueWidget)
	{
		MonologueWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void APolice::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("Overlap Begin Triggered!"));
}

void APolice::SetupCollisionSettings()
{
	//캡슐 컴포넌트: Pawn과 충돌 시 Overlap 처리
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	//Physics Interaction 완전히 끄기
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bEnablePhysicsInteraction = false;
		GetCharacterMovement()->bPushForceUsingZOffset = false;
		GetCharacterMovement()->PushForceFactor = 0.f;

		//필요 시 충돌로 인한 점프 입력 방지
		GetCharacterMovement()->bEnableScopedMovementUpdates = false;
	}
}

void APolice::OnLockWheelInput(const FInputActionValue& Value)
{
	float AxisValue = Value.Get<float>();
	UE_LOG(LogTemp, Warning, TEXT("[Police] Wheel input: %.2f"), AxisValue);


	//자물쇠 인터랙트 중일 때만 작동하도록 조건 체크
	if (CurrentLockActor && CurrentLockActor->IsInspecting())
	{
		CurrentLockActor->OnWheelAxis(AxisValue);
	}
	else
	{
		//평소엔 기존 줌 기능 사용
		Zoom(Value);
	}
}
