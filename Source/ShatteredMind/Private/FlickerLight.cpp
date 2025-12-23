#include "FlickerLight.h"
#include "Components/PointLightComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

AFlickerLight::AFlickerLight()
{
    PrimaryActorTick.bCanEverTick = false;

    PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
    RootComponent = PointLight;

    PointLight->SetIntensity(BaseIntensity);
    PointLight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.8f)); // ¾à°£ ´©·±ºû (³°Àº Çü±¤µî ´À³¦)
}

void AFlickerLight::BeginPlay()
{
    Super::BeginPlay();

    // ½ÃÀÛ ½Ã Ã¹ ±ôºýÀÓ ½ÃÄö½º È£Ãâ
    StartFlickerSequence();
}

void AFlickerLight::StartFlickerSequence()
{
    // ÇÑ ¹øÀÇ ±ôºýÀÓ ½ÃÄö½º¸¦ ½ÃÀÛ
    int32 FlickerCount = FMath::RandRange(2, 5);   // 2~5¹ø ±ôºýÀÓ
    float Interval = 0.08f;                        // ºü¸¥ ±ôºýÀÓ ¼Óµµ
    float TotalDuration = FlickerCount * Interval * 2.0f;

    GetWorldTimerManager().SetTimer(FlickerTimer, this, &AFlickerLight::DoFlicker, Interval, true);

    // ÀÏÁ¤ ½Ã°£ ÈÄ¿¡ ±ôºýÀÓ Á¾·á ÈÄ ´ë±â »óÅÂ·Î ÀüÈ¯
    GetWorldTimerManager().SetTimer(RestTimer, this, &AFlickerLight::StopFlickerSequence, TotalDuration, false);
}

void AFlickerLight::DoFlicker()
{
    static bool bIsOn = true;
    bIsOn = !bIsOn;

    if (bIsOn)
    {
        PointLight->SetIntensity(BaseIntensity * FMath::FRandRange(0.6f, 1.0f));
    }
    else
    {
        PointLight->SetIntensity(BaseIntensity * FMath::FRandRange(0.0f, 0.2f));
    }
}

void AFlickerLight::StopFlickerSequence()
{
    GetWorldTimerManager().ClearTimer(FlickerTimer);

    // ±ôºýÀÓ ¸ØÃã ÈÄ, ·£´ýÇÏ°Ô ºÒÀÌ ¿ÏÀüÈ÷ ²¨Áú ¼öµµ ÀÖÀ½
    if (FMath::RandBool())
    {
        PointLight->SetIntensity(0.0f);
    }
    else
    {
        PointLight->SetIntensity(BaseIntensity * FMath::FRandRange(0.7f, 1.0f));
    }

    // ?? ´ÙÀ½ ±ôºýÀÓ ½ÃÄö½º¸¦ ¸î ÃÊ ÈÄ¿¡ ´Ù½Ã ½ÇÇà
    float NextDelay = FMath::FRandRange(1.5f, 5.0f);
    GetWorldTimerManager().SetTimer(SequenceTimer, this, &AFlickerLight::StartFlickerSequence, NextDelay, false);
}
