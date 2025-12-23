#include "Hammer_Equip.h"
#include "Doctor.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"

void UHammer_Equip::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Hammer_Equip] ? MeshComp is null"));
        return;
    }

    if (AActor* Owner = MeshComp->GetOwner())
    {
        ADoctor* MyChar = Cast<ADoctor>(Owner);
        if (!MyChar)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Hammer_Equip] ? Owner is not ADoctor (Class: %s)"), *Owner->GetClass()->GetName());
            return;
        }

        if (!MyChar->HammerComp)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Hammer_Equip] ? HammerComp is NULL"));
            return;
        }

        // ───────────────────────────────────────────────
        // ① 현재 무기 상태 출력
        // ───────────────────────────────────────────────
        FString StateText = MyChar->bUsingHammer ? TEXT("TRUE") : TEXT("FALSE");
        UE_LOG(LogTemp, Warning, TEXT("[Hammer_Equip] Triggered | bUsingHammer=%s | Anim=%s"),
            *StateText, *Animation->GetName());

        // ───────────────────────────────────────────────
        // ② 부착 로직
        // ───────────────────────────────────────────────
        if (MyChar->bUsingHammer)
        {
            // ? 권총이 손에 들려 있다면 먼저 등으로 옮김
            if (MyChar->gunMeshComp)
            {
                MyChar->gunMeshComp->AttachToComponent(
                    MeshComp,
                    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                    TEXT("spine_01Socket") // 권총 등 소켓 (필요 시 spine_03Socket으로 교체)
                );
                MyChar->gunMeshComp->SetRelativeLocation(FVector(-0.7f, -15.5f, 16.980762f));
                MyChar->gunMeshComp->SetRelativeRotation(FRotator(0.000001f, -269.999999f, -180.0f));
                MyChar->gunMeshComp->SetRelativeScale3D(FVector(1.5f));
            }

            // ? 망치를 손으로 부착
            UE_LOG(LogTemp, Warning, TEXT("[Hammer_Equip] ▶ Attaching to HAND socket: %s"), *SocketName.ToString());
            MyChar->HammerComp->AttachToComponent(
                MeshComp,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                SocketName
            );
            MyChar->HammerComp->SetRelativeLocation(FVector(-40.0f, -6.22055f, -3.591436f));
            MyChar->HammerComp->SetRelativeRotation(FRotator(-17.495241f, -9.846552f, 61.518761f));

            MyChar->bUsingHammer = true;
            MyChar->bUsingPistolGun = false; // ? 상태 동기화
        }
        else
        {
            // ? 망치를 등으로 부착
            UE_LOG(LogTemp, Warning, TEXT("[Hammer_Equip] ▶ Attaching to BACK socket: %s"), *SocketName2.ToString());
            MyChar->HammerComp->AttachToComponent(
                MeshComp,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                SocketName2
            );
            MyChar->HammerComp->SetRelativeLocation(FVector(34.0f, -30.0f, -18.0f));
            MyChar->HammerComp->SetRelativeRotation(FRotator(69.846059f, 358.0f, 69.567958f));
            MyChar->HammerComp->SetRelativeScale3D(FVector(0.55f));
        }

        // ───────────────────────────────────────────────
        // ③ 결과 로그
        // ───────────────────────────────────────────────
        FVector Loc = MyChar->HammerComp->GetComponentLocation();
        UE_LOG(LogTemp, Warning, TEXT("[Hammer_Equip] ? Attached Loc=%s | Rot=%s"),
            *Loc.ToString(), *MyChar->HammerComp->GetComponentRotation().ToString());
    }
}
