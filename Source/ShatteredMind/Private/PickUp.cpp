// Fill out your copyright notice in the Description page of Project Settings.


#include "PickUp.h"
#include "Kismet/GameplayStatics.h"
#include "Police.h"
#include "DialogWidget.h"
#include "DialogTypes.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Internationalization/Text.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"


APickUp::APickUp()
{
	PrimaryActorTick.bCanEverTick = false;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	SetRootComponent(Capsule);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Capsule);

	Capsule->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Capsule->InitCapsuleSize(30.f, 60.f);
	Capsule->SetGenerateOverlapEvents(true);

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Mesh->SetRenderCustomDepth(false);
	Mesh->SetCustomDepthStencilValue(1);

	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	CurrentDialogIndex = 0;
	bIsInteracting = false;// 상호작용 중인지



	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);

	// 크기 조절 (필요에 따라 변경)
	InteractionBox->SetBoxExtent(FVector(50.f, 50.f, 50.f));

	// 라인트레이스만 맞게 설정
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); //라인트레이스는 Visibility 채널 사용
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

void APickUp::ShowPickupLine()
{
	UE_LOG(LogTemp, Warning, TEXT("ShowPickupLine called, PickUpType='%s'"), *PickUpType);



	if (!DialogLines.IsValidIndex(CurrentDialogIndex))
	{
		//UE_LOG(LogTemp, Warning, TEXT("ShowDialogLine: Invalid index %d -> calling EndInteract()"), CurrentDialogIndex);
		EndInteract();
		return;
	}
	// 위젯 생성
	if (DialogWidgetClass && !DialogWidget)
	{
		DialogWidget = CreateWidget<UDialogWidget>(GetWorld(), DialogWidgetClass);
		if (DialogWidget)
			DialogWidget->AddToViewport();

	}

	// UI 갱신
	if (DialogWidget)
	{
		// 디버그용 로그
		UE_LOG(LogTemp, Warning, TEXT("ShowDialogLine: Showing index %d"), CurrentDialogIndex);
		DialogWidget->UpdateDialog(DialogLines[CurrentDialogIndex]);
	}
}

void APickUp::ShowNextPickupLine()
{
	//유효한 인덱스가 아니면 종료
	if (!DialogLines.IsValidIndex(CurrentDialogIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowNextLine: index %d invalid -> EndInteract()"), CurrentDialogIndex);
		// 마지막 문장 이후 대화 종료
		EndInteract();
		if (PictureWidget)
		{
			PictureWidget->RemoveFromParent();
			PictureWidget = nullptr; // 안전하게 초기화
		}
		return;
	}

	if (CurrentDialogIndex == DialogLines.Num() - 1 && CurrentDialogIndex != 0)
	{
		if (DialogEndSound)
		{
			UGameplayStatics::PlaySound2D(this, DialogEndSound);
			UE_LOG(LogTemp, Warning, TEXT("Played dialog end sound (index %d)"), CurrentDialogIndex);
		}
	}

	// 위젯 생성 (한 번만 생성)
	if (DialogWidgetClass && !DialogWidget)
	{
		DialogWidget = CreateWidget<UDialogWidget>(GetWorld(), DialogWidgetClass);
		if (DialogWidget) DialogWidget->AddToViewport();
	}


	if (!PictureWidget)
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC && PictureWidgetClass)
		{
			PictureWidget = CreateWidget<UUserWidget>(PC, PictureWidgetClass);
			if (PictureWidget)
			{
				PictureWidget->AddToViewport(10); // Z-Order 지정
				PictureWidget->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create PictureWidget!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("PlayerController or PictureWidgetClass is nullptr!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("not !PictureWidget"));
	}

	// 현재 문장 표시
	if (DialogWidget)
		DialogWidget->UpdateDialog(DialogLines[CurrentDialogIndex]);

	CurrentDialogIndex++;

}

void APickUp::BeginPlay()
{
	Super::BeginPlay();
	//Mesh->SetRenderCustomDepth(false);

	InteractableData = InstanceInteractableData;

	InitializeDialogLines();
}


void APickUp::Tick(float DeltaTime)
{
}



void APickUp::BeginFocus()
{
	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(true);
	}
}

void APickUp::EndFocus()
{
	if (bIsInteracting) return; //대화 중일 땐 포커스 해제 무시

	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(false);
	}
}

void APickUp::BeginInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("Calling BeginInteract override on interface test actor"));

	// CachedPlayer가 nullptr이면 안전하게 리턴
	if (!CachedPlayer || bIsInteracting)
	{
		UE_LOG(LogTemp, Error, TEXT("BeginInteract: CachedPlayer is nullptr!"));
		return;
	}

	bIsInteracting = true;

	//스프링암 길이 저장은 BeginInteract에서 반드시 한 번만
	if (!bIsInteracting && CachedPlayer->springArmComp)
	{
		//저장
		OriginalArmLength = CachedPlayer->springArmComp->TargetArmLength;
		// zoom in
		CachedPlayer->springArmComp->TargetArmLength = 100.f;
		//CachedPlayer->springArmComp->TargetArmLength = FMath::Min(DesiredArmLength, CachedPlayer->springArmComp->TargetArmLength);
	}

	// 플레이어 입력 잠금
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->SetIgnoreLookInput(true);
		PC->SetIgnoreMoveInput(true);

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}

	//CurrentDialogIndex = 0; // 대화 인덱스 초기화


}

void APickUp::EndInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("Calling EndInteract override on interface test actor"));

	if (!bIsInteracting) return;
	bIsInteracting = false;

	if (PickUpType.Equals(TEXT("FireExtinguisher")))
	{
		APolice* PoliceCharacter = Cast<APolice>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (PoliceCharacter)
		{
			PoliceCharacter->UnlockMemoWrap(4);
		}
	}
	if (PickUpType == (TEXT("Stand")))
	{
		APolice* PoliceCharacter = Cast<APolice>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (PoliceCharacter)
		{
			PoliceCharacter->UnlockMemoWrap(5);
		}
	}

	// Widget 제거
	if (DialogWidget)
	{
		DialogWidget->RemoveFromParent();
		DialogWidget = nullptr;
	}

	if (PictureWidget)
	{
		PictureWidget->RemoveFromParent();
		PictureWidget = nullptr;
	}

	// 스프링암 길이 안전 복구
	if (CachedPlayer && CachedPlayer->springArmComp)
	{
		//CachedPlayer->springArmComp->TargetArmLength = OriginalArmLength;
		CachedPlayer->springArmComp->TargetArmLength = 400.f;
		UE_LOG(LogTemp, Warning, TEXT("EndInteract: Restored spring arm length to %f"), OriginalArmLength);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EndInteract: CachedPlayer or springArmComp was nullptr"));
	}

	InitializeDialogLines2();

	CurrentDialogIndex = 0;

	// 입력 복구를 약간 지연
	FTimerHandle InputDelayHandle;
	GetWorld()->GetTimerManager().SetTimer(InputDelayHandle, [this]()
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
			{
				PC->SetIgnoreLookInput(false);
				PC->SetIgnoreMoveInput(false);

				FInputModeGameOnly InputMode;
				InputMode.SetConsumeCaptureMouseDown(false);
				PC->SetInputMode(InputMode);
			}
		}, 0.3f, false); // 0.1~0.3초 정도 지연

}

void APickUp::Interact(APolice* _Playercharacter)
{
	//UE_LOG(LogTemp, Warning, TEXT("Calling Interact override on interface test actor"));

	// UE_LOG(LogTemp, Warning, TEXT("Calling Interact override on NPC"));

	if (!_Playercharacter) return;

	CachedPlayer = _Playercharacter;



	float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastInteractTime < 0.2f) // 0.2초 이하 연속 입력 무시
		return;
	LastInteractTime = Now;

	//이미 상호작용 중이면 리턴
	if (!bIsInteracting)
	{
		BeginInteract();
		ShowNextPickupLine();
	}
	else
	{
		ShowNextPickupLine(); // 이미 상호작용 중이면 다음 문장만
	}
}


void APickUp::InitializeDialogLines()
{
	if (PickUpType.Equals(TEXT("FireExtinguisher"), ESearchCase::IgnoreCase))
	{
		DialogLines.Add(FDialogLine(TEXT("<Blue>의문의 사진</>"), TEXT("<Blue>신문에서 잘라낸 것 같은데...</>\n<Blue>의사 본인도 아니고.</>")));
		DialogLines.Add(FDialogLine(TEXT(""), TEXT("<Green>**수첩에 단서가 기록되었습니다**</>\n<Green>[Q]</>\n")));
	}
	else if (PickUpType.Equals(TEXT("Stand"), ESearchCase::IgnoreCase))
	{
		DialogLines.Add(FDialogLine(TEXT("<Blue>단체 사진</>"), TEXT("<Blue>범인이 여러 환자들과 찍은 사진인가.</>\n<Blue>정말 가족처럼 친밀해 보이는데...</>")));
		DialogLines.Add(FDialogLine(TEXT(""), TEXT("<Green>**수첩에 단서가 기록되었습니다**</>\n<Green>[Q]</>\n")));
	}


}

void APickUp::InitializeDialogLines2()
{
	DialogLines.Empty(); // 초기화

	if (PickUpType.Equals(TEXT("FireExtinguisher"), ESearchCase::IgnoreCase))
	{
		DialogLines.Add(FDialogLine(TEXT("<Blue>의문의 사진</>"), TEXT("<Blue>신문에서 잘라낸 것 같은데...</>\n<Blue>의사 본인도 아니고.</>")));
		//DialogLines.Add(FDialogLine(TEXT("\n\n<Green>**수첩에 단서가 기록되었습니다**</>\n<Green>[Q]</>"), TEXT("")));
	}
	else if (PickUpType.Equals(TEXT("Stand"), ESearchCase::IgnoreCase))
	{
		DialogLines.Add(FDialogLine(TEXT("<Blue>환자들과 의사의 단체 사진</>"), TEXT("<Blue>범인이 여러 환자들과 찍은 사진인가.</>\n<Blue>정말 가족처럼 친밀해 보이는데...</>")));
		//DialogLines.Add(FDialogLine(TEXT("\n\n<Green>**수첩에 단서가 기록되었습니다**</>\n<Green>[Q]</>"), TEXT("")));
	}
}


