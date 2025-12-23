#include "DrawerActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"

// ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
// ?? ÀüÁ¦
// - Çì´õ(.h)¿¡ ÀÌ¹Ì ¾Æ·¡ ÇÁ·ÎÆÛÆ¼/ÇÔ¼öµéÀÌ ÀÖ´Ù°í °¡Á¤ÇÕ´Ï´Ù
//   UPROPERTY(VisibleAnywhere) UStaticMeshComponent* DrawerMesh;
//   UPROPERTY(VisibleAnywhere) UTimelineComponent* OpenTimeline;
//   UPROPERTY(EditAnywhere)    UCurveFloat* OpenCurve;
//   UPROPERTY(EditAnywhere)    float OpenDistance;
//   UPROPERTY(EditAnywhere)    AActor* DiaryActor;        // ¼­¶ø ¾È ÀÏ±âÀå ¾×ÅÍ(Å¸ÀÔ Á¦ÇÑ X)
//   bool bIsOpen; FVector StartLocation;
//   void HandleDrawerProgress(float Value);
//   void OpenDrawer();
//   // CloseDrawer() ¾øÀ¸¸é Ãß°¡ ¾È ÇØµµ µ¿ÀÛÇÔ (¿­¸²¸¸ Ã³¸®).
// ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡

ADrawerActor::ADrawerActor()
{
    PrimaryActorTick.bCanEverTick = true;

    DrawerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DrawerMesh"));
    RootComponent = DrawerMesh;

    OpenTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("OpenTimeline"));

    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // ? ±âº» Ãæµ¹ ¼³Á¤
    // 1) ¸ðµç Ã¤³Î BlockÀ¸·Î ÃÊ±âÈ­
    // 2) »óÈ£ÀÛ¿ë ¶óÀÎÆ®·¹ÀÌ½º(ECC_Visibility)´Â "´ÝÈû »óÅÂ"¿¡¼± Block À¯Áö
    //    (¿­¸± ¶§¸¸ Ignore·Î ÀüÈ¯)
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    DrawerMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    DrawerMesh->SetCollisionResponseToAllChannels(ECR_Block);                 // [Áß¿ä] ¸ÕÀú ÀüÃ¼ Block
    DrawerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);     // »óÈ£ÀÛ¿ë Æ®·¹ÀÌ½º ¸·À½(´ÝÈû »óÅÂ)

    // ¹°¸®/¿À¹ö·¦ ±âº» °ª À¯Áö (ÇÊ¿ä ½Ã ÇÁ·ÎÁ§Æ® ·ê¿¡ ¸Â°Ô Á¶Á¤)
    DrawerMesh->SetGenerateOverlapEvents(false);
}

void ADrawerActor::BeginPlay()
{
    Super::BeginPlay();
    StartLocation = GetActorLocation();

    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // ? Å¸ÀÓ¶óÀÎ/Ä¿ºê ÁØºñ
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    if (!OpenCurve)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Drawer Debug] OpenCurve is missing, creating default curve."));

        OpenCurve = NewObject<UCurveFloat>(this, TEXT("AutoCurve"));
        OpenCurve->FloatCurve.AddKey(0.0f, 0.0f);
        OpenCurve->FloatCurve.AddKey(1.0f, 1.0f);
    }

    if (OpenCurve)
    {
        FOnTimelineFloat ProgressFunction;
        ProgressFunction.BindUFunction(this, FName("HandleDrawerProgress"));
        OpenTimeline->AddInterpFloat(OpenCurve, ProgressFunction);
        UE_LOG(LogTemp, Warning, TEXT("[Drawer Debug] Timeline bound successfully for %s"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Drawer Debug] ? OpenCurve creation failed for %s"), *GetName());
    }

    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // ? ÀÏ±âÀå ¾×ÅÍ°¡ ÁöÁ¤µÇ¾î ÀÖÀ¸¸é ºÎÂø ¹× ÄÝ¸®Àü º¸Á¤
    //  - ¿©±â¼­´Â DiaryPickup Å¸ÀÔÀ» Á÷Á¢ include/ÀÇÁ¸ÇÏÁö ¾Ê½À´Ï´Ù.
    //  - ³»ºÎÀÇ UStaticMeshComponent¸¦ Ã£¾Æ Visibility=Block º¸Àå
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    if (DiaryActor)
    {
        // ºÎÂø (¿ø·¡ ÀÖ´ø ³× ·ÎÁ÷ À¯Áö)
        DiaryActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        UE_LOG(LogTemp, Warning, TEXT("[Drawer] Attached DiaryActor: %s"), *DiaryActor->GetName());

        // ÀÏ±âÀåÀÇ UStaticMeshComponent Ã£¾Æ¼­ Visibility=Block º¸Àå
        if (UStaticMeshComponent* DiaryMesh = DiaryActor->FindComponentByClass<UStaticMeshComponent>())
        {
            DiaryMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);                // Æ®·¹ÀÌ½º Àü¿ëÀÌ¸é ±ò²û
            DiaryMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
            DiaryMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);         // »óÈ£ÀÛ¿ë Æ®·¹ÀÌ½º¿¡¸¸ °É¸®°Ô

            UE_LOG(LogTemp, Warning, TEXT("[Drawer] Diary mesh collision set (QueryOnly, Visibility=Block)"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[Drawer] ? DiaryActor has no UStaticMeshComponent"));
        }
    }

    // »óÅÂ ´ýÇÁ
    UE_LOG(LogTemp, Warning, TEXT("[Drawer Debug] BeginPlay: DrawerMesh Vis=%d, IsOpen=%d"),
        (int32)DrawerMesh->GetCollisionResponseToChannel(ECC_Visibility), (int32)bIsOpen);
}

void ADrawerActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Å¸ÀÓ¶óÀÎ ¼öµ¿ °»½Å(ÄÄÆ÷³ÍÆ® ÀÚµ¿ TickÀÌ ¾Æ´Ñ °æ¿ì ´ëºñ)
    if (OpenTimeline)
        OpenTimeline->TickComponent(DeltaTime, ELevelTick::LEVELTICK_TimeOnly, nullptr);

    // ³× ±âÁ¸ ¡°ºÎµå·´°Ô ¿­±â¡± º¸°£ ÄÚµå À¯Áö
    if (bIsOpen)
    {
        static float Alpha = 0.f;
        Alpha = FMath::Clamp(Alpha + DeltaTime * 0.8f, 0.f, 1.f);

        float SmoothAlpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.5f);
        FVector TargetLocation = StartLocation + GetActorRightVector() * (OpenDistance * SmoothAlpha);
        SetActorLocation(TargetLocation);

        if (Alpha >= 1.f)
            bIsOpen = false;
    }
}

void ADrawerActor::HandleDrawerProgress(float Value)
{
    FVector NewLocation = StartLocation + GetActorRightVector() * (OpenDistance * Value);
    SetActorLocation(NewLocation);

    UE_LOG(LogTemp, Warning, TEXT("[Drawer] HandleDrawerProgress Value=%.2f | Location=%s"),
        Value, *NewLocation.ToString());
}

// ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
// ? ¿­±â: ¶óÀÎÆ®·¹ÀÌ½º¸¦ ÀÏ±âÀåÀ¸·Î ¡°Åë°ú¡±½ÃÅ°´Â ÇÙ½É
//  - ¿­¸®´Â ¼ø°£¿¡¸¸ DrawerMeshÀÇ Visibility¸¦ Ignore·Î ¹Ù²Þ
//  - ¹°¸®/´Ù¸¥ Ã¤³Î Block »óÅÂ´Â ±×´ë·Î À¯Áö ¡æ ¼­¶øÀº ¿©ÀüÈ÷ ¼¼°è¿Í Ãæµ¹ÇÔ
// ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
void ADrawerActor::OpenDrawer()
{
    UE_LOG(LogTemp, Warning, TEXT("[Drawer Debug] OpenDrawer() called. bIsOpen=%d, OpenCurve=%s"),
        bIsOpen, *GetNameSafe(OpenCurve));

    if (bIsOpen || !OpenCurve)
    {
        UE_LOG(LogTemp, Error, TEXT("[Drawer Debug] ? Drawer cannot open. (Already open or Curve missing)"));
        return;
    }

    bIsOpen = true;

    // ¡Ø ÇÙ½É: ¡°¿­¸®´Â µ¿¾È¡±¿£ »óÈ£ÀÛ¿ë Æ®·¹ÀÌ½º°¡ ¼­¶øÀ» ¹«½ÃÇÏµµ·Ï
    //   (ÀÏ±âÀå(mesh)Àº Visibility=Block »óÅÂÀÌ¹Ç·Î, °ð¹Ù·Î ÀÏ±âÀå¿¡ Æ®·¹ÀÌ½º°¡ ´êÀ½)
    DrawerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    UE_LOG(LogTemp, Warning, TEXT("[Drawer Debug] DrawerMesh Visibility=Ignore (trace pass-through enabled)"));

    // ÇÊ¿ä¿¡ µû¶ó ¿­¸®´Â µ¿¾È ¿À¹ö·¦ ÀÌº¥Æ®¸¦ ²ô°í ½ÍÀ¸¸é ¾Æ·¡ ÁÖ¼® ÇØÁ¦
    // DrawerMesh->SetGenerateOverlapEvents(false);

    if (OpenTimeline)
    {
        OpenTimeline->PlayFromStart();
        UE_LOG(LogTemp, Warning, TEXT("[Drawer Debug] ¢º Drawer Timeline started for %s"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Drawer Debug] ? Timeline is null!"));
    }
}

// ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
// (¼±ÅÃ) ´Ý±â ÇÔ¼ö°¡ ÀÖ´Ù¸é, ´ÝÈú ¶§ ´Ù½Ã Visibility BlockÀ¸·Î º¹¿ø
// Çì´õ¿¡ CloseDrawer ¼±¾ðÀÌ ¾ø´Ù¸é ÀÌ ÇÔ¼ö´Â »ý·«ÇØµµ ¹«¹æÇÕ´Ï´Ù.
// ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
void ADrawerActor::CloseDrawer()
{
    // ´ÝÈú ¶© ´Ù½Ã Æ®·¹ÀÌ½º¸¦ ¸·¾Æ, ¼­¶øÀÌ ÀÏ±âÀåÀ» °¡¸®µµ·Ï
    DrawerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    // DrawerMesh->SetGenerateOverlapEvents(true); // ÇÊ¿ä½Ã

    if (OpenTimeline)
    {
        OpenTimeline->Reverse();
        UE_LOG(LogTemp, Warning, TEXT("[Drawer Debug] ¢¸ Drawer Timeline reversed (closing) for %s"), *GetName());
    }
}
