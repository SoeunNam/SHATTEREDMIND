// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InteractionInterface.h"
#include "DialogTypes.h"
#include "DialogWidget.h"
#include "Police.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Internationalization/Text.h"
#include "NPC.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "PickUp.generated.h"

UCLASS()
class SHATTEREDMIND_API APickUp : public AActor, public IInteractionInterface
{
    GENERATED_BODY()

private:
    // 헤더 파일(.h)에 타이머 핸들 선언
    FTimerHandle TimerHandle_MyFunction;
    // 상호작용 시 카메라 팔 길이
    float DesiredArmLength = 100.0f;
    // 상호작용 중인 플레이어 저장 (스프링암컴포넌트 복구용)
    APolice* CachedPlayer;
    //원래 팔 길이 저장
    float OriginalArmLength;

public:
    APickUp();
    // NPC 종류 구분용 (블루프린트에서 세팅 가능)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PickUp")
    FString PickUpType;

    UPROPERTY(EditAnywhere, Category = "PickUp")
    UStaticMeshComponent* Mesh;

    UPROPERTY(EditAnywhere, Category = "PickUp")
    UCapsuleComponent* Capsule;

    UPROPERTY(EditInstanceOnly, Category = "PickUp")
    FInteractableData InstanceInteractableData;

    // 대화 데이터
    UPROPERTY(EditAnywhere, Category = "PickUp")
    TArray<FDialogLine> DialogLines;//C++에서 직접 관리

    int32 CurrentDialogIndex = 0;//현재 대화 인덱스

    //블루프린트에서 DialogWidget 클래스 연결
    UPROPERTY(EditAnywhere, Category = "PickUp")
    TSubclassOf<UDialogWidget> DialogWidgetClass; // 여기서 WBP_DialogWidget 연결

    UPROPERTY()
    UDialogWidget* DialogWidget;// 생성된 위젯 인스턴스

    UPROPERTY(EditAnywhere, Category = "PickUp")
    TSubclassOf<UUserWidget> PictureWidgetClass; //여기서 사진 확대 위젯 WBP_Picture연결

    UPROPERTY()
    UUserWidget* PictureWidget;// 생성된 위젯 인스턴스

    bool bIsInteracting;// 상호작용 중인지 여부
    //현재 대사 표시
    void ShowPickupLine();
    void ShowNextPickupLine();
    virtual void BeginPlay() override;
    float LastInteractTime = -1.0f; // 마지막 상호작용 시간


    // 플레이어가 상호작용했을 때 호출
    //UFUNCTION()

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;
    virtual void BeginFocus() override;
    virtual void EndFocus() override;
    virtual void BeginInteract() override;
    virtual void EndInteract() override;
    virtual void Interact(APolice* _Playercharacter) override;
    // NPC 타입에 따라 대사 세팅
    void InitializeDialogLines();
    //두번째 이후 상호작용부터 메모업데이트 대사 없애기
    void InitializeDialogLines2();
    UPROPERTY(EditAnywhere, Category = "Sound")
    USoundBase* DialogEndSound;



    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PickUp")
    class UBoxComponent* InteractionBox;



};