// Fill out your copyright notice in the Description page of Project Settings.


#include "NPC.h"
#include "Kismet/GameplayStatics.h"
#include "Police.h"
#include "DialogWidget.h"
#include "DialogTypes.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"  
#include "Components/InputComponent.h"
#include "PoliceMemoWidget.h"
#include "JohnFSM.h"
#include "Engine/Font.h"
#include "Internationalization/Text.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h" //플레이어와 충돌시 npc 점프하는 현상 제어위함

// Sets default values
ANPC::ANPC()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetupAttachment(RootComponent);
	GetMesh()->SetRenderCustomDepth(false);
	GetMesh()->SetCustomDepthStencilValue(1);

	GetMesh()->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	CurrentDialogIndex = 0;
	bIsInteracting = false;
	bHasTalkedBefore = false;  // 반드시 초기화


}

// Called when the game starts or when spawned
void ANPC::BeginPlay()
{
	Super::BeginPlay();

	InteractableData = InstanceInteractableData;
	InitializeDialogLines();


	//npc 바로대화시작 약간 미루기 널포인터 터져서
	if (bAutoStart)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PC)
		{
			// 입력 잠금
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->SetIgnoreLookInput(true);
			PC->SetIgnoreMoveInput(true);
		}

		// 스프링암 길이 조절 (자동 상호작용 NPC만)
		APolice* Player = Cast<APolice>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (Player && Player->springArmComp)
		{
			OriginalArmLength = Player->springArmComp->TargetArmLength;
			Player->springArmComp->TargetArmLength = DesiredArmLength;
		}

		// 딜레이 후 첫 문장 표시
		FTimerHandle DelayHandle;
		GetWorldTimerManager().SetTimer(DelayHandle, [this]()
			{
				bIsInteracting = true;
				CurrentDialogIndex = 0;
				ShowDialogLine();
				// 다음 문장 바로 진행 가능하도록 인덱스 증가
				CurrentDialogIndex = 1;
			}, 0.3f, false);


	}

	// 플레이어와 충돌시 npc 날아가는 현상 제어
	if (GetController() && Cast<AAIController>(GetController()))
	{
		SetupCollisionForMovingNPC();
	}
	else
	{
		SetupCollisionForStaticNPC();
	}

	//====================================================================================

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// 물리 상호작용 끄기 (밀림/튐 방지)
		MoveComp->bEnablePhysicsInteraction = false;     // 물리 상호작용 전체 차단
		MoveComp->bPushForceUsingZOffset = false;       // Z축 오프셋으로 튀는거 방지
		MoveComp->PushForceFactor = 0.f;                // 밀림 강도 0으로
		MoveComp->TouchForceFactor = 0.f;               // 접촉으로 주는 힘 제거 (필요하면 조정)
		MoveComp->bTouchForceScaledToMass = false;

		//스코프된 이동 업데이트는 불필요하면 끔
		MoveComp->bEnableScopedMovementUpdates = false;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		// 플레이어와 충돌 시 튀지 않게 겹침(혹은 상황에 따라 Block으로)
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	}
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	GetCharacterMovement()->bUseRVOAvoidance = false;
	GetCharacterMovement()->SetAvoidanceEnabled(false);

}

// Called every frame
void ANPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//NPC한테 말 걸면 플레이어 방향으로 부드럽게 회전 기능
	if (bRotateToPlayer || bReturnToOriginal)  //회전 상태가 켜져 있으면 계속 Tick에서 회전
	{
		FRotator CurrentRotation = GetActorRotation();
		float CurrentYaw = CurrentRotation.Yaw;
		if (bAutoStart)
		{

		}

		// TargetYaw는 이미 Interact 또는 EndInteract에서 설정됨
		float NewYaw = FMath::FInterpTo(CurrentYaw, TargetRotation.Yaw, DeltaTime, OpenSpeed);

		CurrentRotation.Yaw = NewYaw;
		if (bAutoStart)
		{
			NewYaw = 130.f;
		}
		CurrentRotation.Pitch = 0.f;
		CurrentRotation.Roll = 0.f;

		SetActorRotation(CurrentRotation);

		// 목표에 거의 도달하면 회전 상태 종료
		if (FMath::Abs(FMath::FindDeltaAngleDegrees(NewYaw, TargetRotation.Yaw)) <= 0.5f)
		{
			if (bReturnToOriginal)
				bReturnToOriginal = false;  // 원래 방향 복귀 완료
			if (bRotateToPlayer)
				bRotateToPlayer = false;   // 플레이어 바라보기 완료
		}
	}
}

// Called to bind functionality to input
void ANPC::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ANPC::BeginFocus()
{
	//테두리 표시
	if (GetMesh())
	{
		GetMesh()->SetRenderCustomDepth(true);
	}
}

void ANPC::EndFocus()
{
	if (bIsInteracting) return; //대화 중일 땐 포커스 해제 무시

	//테두리 표시 해제
	if (GetMesh())
	{
		GetMesh()->SetRenderCustomDepth(false);
	}
}

void ANPC::BeginInteract()
{
	//UE_LOG(LogTemp, Warning, TEXT("Calling BeginInteract override on NPC"));

	// 이미 대화 중이라면
	if (bIsInteracting) return; //중복 방지 (대화 중 다시 E 누르면 무시)

	// 플레이어 캐릭터 가져오기
	APolice* Player = Cast<APolice>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player)
	{
		bIsInteracting = false;
		return;
	}

	CachedPlayer = Player;
	// 스프링암 길이 저장
	if (CachedPlayer && CachedPlayer->springArmComp)
	{
		OriginalArmLength = CachedPlayer->springArmComp->TargetArmLength;
		CachedPlayer->springArmComp->TargetArmLength = DesiredArmLength; // 단 한 번 변경
	}

	//이미 대화한 적 있는 NPC 처리
	if (bHasTalkedBefore)
	{
		bIsInteracting = true;

		// 위젯 생성 및 표시
		if (DialogWidgetClass)
		{
			DialogWidget = CreateWidget<UDialogWidget>(GetWorld(), DialogWidgetClass);
			if (DialogWidget)
			{
				DialogWidget->AddToViewport();

				if (NPCType == TEXT("Police"))
				{
					FDialogLine RepeatLine(TEXT(""), TEXT("<Blue>저는 돌아가보겠습니다.</>\n\n"));
					DialogWidget->UpdateDialog(RepeatLine);
				}
				else
				{
					FDialogLine RepeatLine(TEXT(""), TEXT("<Blue>아는 건 다 말씀 드렸어요...</>\n\n"));
					DialogWidget->UpdateDialog(RepeatLine);
				}
			}
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

		// 1초 후 종료 및 위젯 제거, 입력 복원, 스프링암 복원
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ANPC::EndInteract, 1.0f, false);

		return;
	}

	//여기서부터 실제 대화 시작 처리
	bIsInteracting = true; // 대화 상태로 설정
	CurrentDialogIndex = 0; // 대화 인덱스 초기화
	ShowDialogLine(); // 첫 문장 표시
	//현재 npc 방향 저장
	OriginalRotation = GetActorRotation();

	if (Player)
	{
		// 플레이어 위치 가져오기
		FVector PlayerLocation = Player->GetActorLocation();

		// NPC 자신의 위치
		FVector NPCLocation = GetActorLocation();

		FVector Direction = PlayerLocation - NPCLocation;
		Direction.Z = 0.f; // 수평만 회전


		//NPC 회전 계산
		// 플레이어가 거의 정면에 있을 때도 안정적 계산
		if (Direction.SizeSquared() > 0.01f)
		{
			TargetRotation = Direction.Rotation();
			bRotateToPlayer = true;
			bReturnToOriginal = false;
		}

		//플레이어 입력 잠금
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			PC->SetIgnoreLookInput(true);   //마우스 시점 이동 잠금
			PC->SetIgnoreMoveInput(true);   //키보드 이동 입력도 잠금

			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);

		}


	}
}

//대화종료처리
void ANPC::EndInteract()
{
	//이미 종료 상태면 아무것도 하지 않음
	if (!bIsInteracting)
	{
		UE_LOG(LogTemp, Warning, TEXT("EndInteract called but bIsInteracting==false. Proceeding with cleanup anyway."));
	}
	bIsInteracting = false;
	bHasTalkedBefore = true;

	//입력 복구를 약간 늦게 (0.3초 후)
	FTimerHandle InputDelayHandle;
	GetWorld()->GetTimerManager().SetTimer(InputDelayHandle, [this]()
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
			{
				PC->SetIgnoreLookInput(false);
				PC->SetIgnoreMoveInput(false);
				FInputModeGameOnly InputMode;
				PC->SetInputMode(InputMode);
			}
			//스프링암 길이 복구
			if (CachedPlayer && CachedPlayer->springArmComp)
			{
				CachedPlayer->springArmComp->TargetArmLength = OriginalArmLength;
			}
		}, 0.3f, false);

	UE_LOG(LogTemp, Warning, TEXT("EndInteract called - cleaning up"));

	//플레이어 입력 다시 활성화
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->SetIgnoreLookInput(false);
		PC->SetIgnoreMoveInput(false);

		//Input Mode를 게임 전용으로 복구
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
	if (CachedPlayer && CachedPlayer->springArmComp)
	{
		CachedPlayer->springArmComp->TargetArmLength = OriginalArmLength;
		UE_LOG(LogTemp, Warning, TEXT("SpringArm length restored"));
	}
	// 원래 보던 방향 복구 (NPC가 idle 상태 유지하게)
	// BeginInteract에서 회전값 저장해두면 여기서 복원 가능
	TargetRotation = OriginalRotation;
	bReturnToOriginal = true;//원래 방향으로 복귀 시작
	bRotateToPlayer = false;

	//대화 종료 시 위젯 제거
	if (DialogWidget)
	{
		DialogWidget->RemoveFromParent();
		DialogWidget = nullptr;
	}
	CurrentDialogIndex = 0;
	bHasTalkedBefore = true; //대화가 끝났으니 true로 설정
	bCaseFileShown = false; //사건파일 다시 열 수 있게 초기화


	APolice* PoliceCharacter = Cast<APolice>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (PoliceCharacter)
	{
		//환자
		if (NPCType.Equals(TEXT("Guard"), ESearchCase::IgnoreCase))
		{
			PoliceCharacter->UnlockMemoWrap(3);
			UE_LOG(LogTemp, Warning, TEXT("Guard met - Memo 3 unlocked"));
		}
		//순경
		else if (NPCType.Equals(TEXT("Police"), ESearchCase::IgnoreCase))
		{
			PoliceCharacter->UnlockMemoWrap(1);
			UE_LOG(LogTemp, Warning, TEXT("Police met - Memo 1 unlocked"));
		}
		//간호사
		else if (NPCType.Equals(TEXT("Nurse"), ESearchCase::IgnoreCase))
		{
			PoliceCharacter->UnlockMemoWrap(2);
			UE_LOG(LogTemp, Warning, TEXT("Nurse met - Memo 2 unlocked"));
		}

	}
}

void ANPC::Interact(APolice* _Playercharacter)
{

	if (!_Playercharacter) return;

	CachedPlayer = _Playercharacter;
	
	// 대화 일시정지 중이면 아무것도 안 함
	if (bDialogPaused)
	{
		CloseCaseFileWidget();
		bDialogPaused = false;// 이제 다음 문장 진행 가능
		ShowNextLine(); // 바로 다음 대사 진행
		return;
	}


	//재시도시 이미 대화한 NPC 처리
	if (bHasTalkedBefore)
	{
		BeginInteract(); //BeginInteract에서 이미 대화한 적 있는지 처리
		return;
	}

	//InteractDebounce 체크
	float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastInteractTime < InteractDebounce)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact ignored due to debounce: delta=%.3f"), Now - LastInteractTime);
		return;
	}
	LastInteractTime = Now;


	//첫 대화 시작은 BeginInteract()로
	if (!bIsInteracting)
	{
		BeginInteract();
	}
	else
	{
		ShowNextLine();
	}
}

void ANPC::ShowDialogLine()
{
	UE_LOG(LogTemp, Warning, TEXT("ShowDialogLine called: CurrentDialogIndex=%d, DialogLines.Num=%d"), CurrentDialogIndex, DialogLines.Num());

	if (!DialogLines.IsValidIndex(CurrentDialogIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowDialogLine: Invalid index %d -> calling EndInteract()"), CurrentDialogIndex);
		EndInteract();
		return;
	}

	//위젯 생성
	if (DialogWidgetClass && !DialogWidget)
	{
		DialogWidget = CreateWidget<UDialogWidget>(GetWorld(), DialogWidgetClass);
		if (DialogWidget)
			DialogWidget->AddToViewport();
	}

	//UI 갱신
	if (DialogWidget)
	{
		//디버그용 로그
		UE_LOG(LogTemp, Warning, TEXT("ShowDialogLine: Showing index %d"), CurrentDialogIndex);
		DialogWidget->UpdateDialog(DialogLines[CurrentDialogIndex]);
	}
}

void ANPC::ShowNextLine()
{
	if (bDialogPaused) return; //사건파일 열려있으면 진행 금지

	if (!DialogLines.IsValidIndex(CurrentDialogIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowNextLine: index %d invalid -> EndInteract()"), CurrentDialogIndex);
		//마지막 문장 이후 대화 종료
		EndInteract();
		return;
	}
	//대화 종료 직전 사운드 재생
	if (CurrentDialogIndex == DialogLines.Num() - 1)
	{
		if (DialogEndSound) //미리 UPROPERTY로 선언해둔 사운드
		{
			UGameplayStatics::PlaySound2D(this, DialogEndSound);
		}
	}

	//먼저 문장 표시
	if (DialogWidget)
	{
		DialogWidget->UpdateDialog(DialogLines[CurrentDialogIndex]);
	}

	//특정 문장에서 사건파일 열기 조건
	if (bIsPoliceNPC && !bCaseFileShown && DialogLines[CurrentDialogIndex].LineText.ToString().Contains(TEXT("Report the case")))
	{
		ShowCaseFileWidget();
		bDialogPaused = true; // 대화 일시정지
		bCaseFileShown = true; //한번만 열기위해 체크하는 bool
		return; // 사건파일 닫힐 때까지 대화 멈춤
	}

	CurrentDialogIndex++;
}


void ANPC::InitializeDialogLines()
{
	//블루프린트 이름으로 NPC 구분해서 대사 세팅
	DialogLines.Empty(); //초기화

	UE_LOG(LogTemp, Warning, TEXT("InitializeDialogLines: NPCType='%s'"), *NPCType);

	APolice* PoliceCharacter = Cast<APolice>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (PoliceCharacter)
	{
		//순경
		if (NPCType.Equals(TEXT("Police"), ESearchCase::IgnoreCase))
		{
			DialogLines.Add(FDialogLine(TEXT("<Blue>순경</>"), TEXT("<Blue>형사님!</>")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>순경</>"), TEXT("<Blue>현재 용의자가 의사소통이 안되는 상황이라</>\n<Blue>범행동기 파악이 어려운 상황입니다.</>")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>순경</>"), TEXT("<Blue>피해자는 대부분 이 병원 간호사와 환자들입니다.</>\n\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>순경</>"), TEXT("<Blue>용의자는 계속 괴물을 잡았다는 얘기만 하고 있고요...</>\n\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>순경</>"), TEXT("<Blue>그 의사 개인 연구실 문은 열려있는데</>\n<Blue>책상 서랍에 자물쇠가 채워져있더라구요.</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>순경</>"), TEXT("<Blue>연구실은 이 복도 끝 왼쪽 방입니다.</>\n\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>순경</>"), TEXT("<Blue>먼저 한 번 확인해보셔야 될 것 같아요.</>\n\n")));
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>그래. 이만 복귀해.</>\n\n")));
			DialogLines.Add(FDialogLine(TEXT(""), TEXT("<Green>**수첩에 단서가 기록되었습니다**</>\n<Green>[Q]</>\n")));
		}
		//간호사
		else if (NPCType.Equals(TEXT("Nurse"), ESearchCase::IgnoreCase))
		{
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>안녕하세요. 신고해주신 분 되시죠?</>\n<Black>괜찮으신가요?</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>간호사</>"), TEXT("<Blue>아, 네...</>\n<Blue>저는 숨어있었어서 괜찮지만...</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>덕분에 범인은 빠르게 체포되어서 이송 중 입니다...</>\n\n")));
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>하지만 정신이 나갔다고 해야할 지...</>\n<Black>의사소통이 되지 않는 상태더라고요...</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>그래서 혹시 지금 질문 몇 가지 드려도 괜찮을까요?</>\n\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>간호사</>"), TEXT("<Blue>네...</>\n\n")));
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>범인과는 평소 어떻게 아는 사이죠?</>\n\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>간호사</>"), TEXT("<Blue>저희 병원 외과 전문의세요.</>\n<Blue>저는 간호사구요...</>\n<Blue>제 담당 환자들 담당의세요.</>")));
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>네. 그럼 평소 나름 가까운 사이시겠네요.</>\n\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>간호사</>"), TEXT("<Blue>그렇죠.</>\n<Blue>둘 다 이 병원에서 근무한지 오래 됐으니까요...</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>간호사</>"), TEXT("<Blue>그런데 이런 악행을 저지를 사람이라고는</>\n<Blue>상상도 못했어요.</>\n<Blue>환자들은 물론 모든 사람들에게 살가운 분이셨는데...</>")));
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>그 의사에게서 특이하다고 생각되는 점은 없으셨나요?</>\n<Black>어떤 거라도 좋으니까요.</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>간호사</>"), TEXT("<Blue>글쎄요...다른 사람들에게 다정하시고</>\n<Blue>아 때론 엄하시기도 하지만요...</>\n<Blue>책에나 나오는 이상적인 아버지 같은 분이셨어요.</>")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>간호사</>"), TEXT("<Blue>...너무 이상적이라는게</>\n<Blue>특이한 점이라면 특이한 점이겠네요.</>\n<Blue>아무튼 전혀...이런 일을 저지르실거라고는...</>")));
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>협조 감사드립니다.</>\n<Black>혹시 나중에 경찰서에서 다시 연락드릴 수 있습니다.</>\n")));
			DialogLines.Add(FDialogLine(TEXT(""), TEXT("<Green>**수첩에 단서가 기록되었습니다**</>\n<Green>[Q]</>\n")));
		}
		//상호작용 환자인데 메모위젯 바인딩 새로하기 귀찮아서 그냥 npctype를 guard로 함
		else if (NPCType.Equals(TEXT("Guard"), ESearchCase::IgnoreCase))
		{
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>안녕하세요. 괜찮으신가요?</>\n<Black>잠깐 조사에 응해주실 수 있으신가요?</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>입원 환자</>"), TEXT("<Blue>으으...</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>...차트를 보니까 담당의가 그 의사..이신데</>\n<Black>평소 어떤 사람인지 여쭤봐도 되겠습니까?</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>입원 환자</>"), TEXT("<Blue>의사선생님... 나한테 잘해주셨는데...</>\n\n")));
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>다른 의사들과 비교해서</>\n<Black>특이하다고 느끼신 부분은 없었나요?</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>입원 환자</>"), TEXT("<Blue>진료 내용과 관계없는 질문을 종종...</>\n<Blue>하시긴했는데 나쁜 건 아니고요...</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>입원 환자</>"), TEXT("<Blue>가족들이랑은 가까운 지, 어떤 대화를 하는지...</>\n<Blue>저 뿐만 아니라 다른 환자들에게도요...</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Blue>입원 환자</>"), TEXT("<Blue>보기 드물게 다정한 의사라고 생각했습니다...</>\n<Blue>그런데 어떻게 이렇게 잔혹한...</>\n")));
			DialogLines.Add(FDialogLine(TEXT("<Black>형사(나)</>"), TEXT("<Black>답변 감사합니다.</>\n<Black>이후 추가 조사 요청 시 응해주시면 감사드리겠습니다.</>\n")));
			DialogLines.Add(FDialogLine(TEXT(""), TEXT("<Green>**수첩에 단서가 기록되었습니다**</>\n<Green>[Q]</>\n")));
		}
		//디버그용
		else
		{
			DialogLines.Add(FDialogLine(TEXT("Debug"), TEXT("Debug first line")));
			DialogLines.Add(FDialogLine(TEXT("Debug"), TEXT("Debug second line")));
		}

		UE_LOG(LogTemp, Warning, TEXT("InitializeDialogLines: DialogLines.Num=%d"), DialogLines.Num());
	}
}

void ANPC::ShowCaseFileWidget()
{
	if (!bIsPoliceNPC || !CaseFileWidgetClass) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	if (!CurrentCaseFileWidget)
	{
		CurrentCaseFileWidget = CreateWidget<UUserWidget>(PC, CaseFileWidgetClass);
		//방어	
		if (!CurrentCaseFileWidget)
		{
			return;
		}
	}

	CurrentCaseFileWidget->AddToViewport(100);
	CurrentCaseFileWidget->SetVisibility(ESlateVisibility::Visible);
	CurrentCaseFileWidget->InvalidateLayoutAndVolatility(); //강제 갱신

	// TextBlock에 글 세팅
	if (UTextBlock* TextBlock = Cast<UTextBlock>(CurrentCaseFileWidget->WidgetTree->FindWidget(TEXT("CaseFileText"))))
	{
		if (CaseFileText.IsEmpty())
			CaseFileText = FText::FromString(TEXT("사건 발생 시각(신고 접수 시각)\n:1994년 4월 20일 오전 3시(오전 6시)\n\n사건 발생 위치\n: Ravenwood Psychiatric Hospital\n1247 County Road 16\nAshford, West Virginia 25114, USA\n\n사건 현장\n:병원 내 5번 병실\n\n범행 도구: 소화기, 수액 스탠드\n\n사망자 총 4명\n\n환자A\n:수액 스탠드에 경동맥 자상, 과다출혈\n\n환자B\n:수액 스탠드에 우측 상복부 간 위치 자상, 과다출혈\n\n환자C\n:허리 뒤 쪽 신장 부위에 타박상, 내부 과다 출혈\n\n환자D\n:소화기로 인한 후두부 뇌간 부위 함몰, 뇌자상 및 뇌출혈\n\n현재 용의자 없음"));

		TextBlock->SetText(CaseFileText);
		TextBlock->SetVisibility(ESlateVisibility::Visible);

		// 부모 패널도 Visible
		if (UWidget* Parent = TextBlock->GetParent())
		{
			Parent->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TextBlock 'CaseFileText' not found in CaseFileWidget!"));
	}


	//입력 모드 UI + 게임 입력 모두 허용
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(CurrentCaseFileWidget->GetCachedWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);

	//대화 일시정지
	bDialogPaused = true;

}

void ANPC::CloseCaseFileWidget()
{
	if (!CurrentCaseFileWidget) return;

	CurrentCaseFileWidget->RemoveFromParent();
	CurrentCaseFileWidget = nullptr;

	//대화 재개
	bDialogPaused = false;

	//사건파일 끝나고 다음 문장 진행
	ShowDialogLine();
	CurrentDialogIndex++;
}

void ANPC::SetupCollisionForStaticNPC()
{
	UE_LOG(LogTemp, Log, TEXT("Static NPC collision setup"));

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetEnableGravity(false);
}

void ANPC::SetupCollisionForMovingNPC()
{
	UE_LOG(LogTemp, Log, TEXT("Moving NPC collision setup"));

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Capsule->SetCollisionResponseToAllChannels(ECR_Block);
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	MoveComp->bUseRVOAvoidance = false;  // AI 끼리 충돌 회피 기능 끄기
	MoveComp->bEnablePhysicsInteraction = false;
}
