// Fill out your copyright notice in the Description page of Project Settings.

#include "Pistol_Equip.h"
#include "Doctor.h"

void UPistol_Equip::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (AActor* Owner = MeshComp->GetOwner())
    {
        ADoctor* MyChar = Cast<ADoctor>(Owner);
        if (MyChar && MyChar->gunMeshComp && MyChar->HammerComp)
        {
            // ? [1] 권총 꺼내는 중일 때
            if (MyChar->bUsingPistolGun == true)
            {
                // ────────────────────────────────
                // (1) 망치가 손에 있다면 등으로 이동
                // ────────────────────────────────
                MyChar->HammerComp->AttachToComponent(
                    MeshComp,
                    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                    TEXT("spine_03Socket") // 망치 등 소켓
                );
                MyChar->HammerComp->SetRelativeLocation(FVector(34.0f, -30.0f, -18.0f));
                MyChar->HammerComp->SetRelativeRotation(FRotator(69.846059f, 358.0f, 69.567958f));
                MyChar->HammerComp->SetRelativeScale3D(FVector(0.55f));

                // ────────────────────────────────
                // (2) 권총을 손 소켓으로 이동
                // ────────────────────────────────
                MyChar->gunMeshComp->AttachToComponent(
                    MeshComp,
                    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                    SocketName // 손 소켓
                );
                MyChar->gunMeshComp->SetRelativeLocation(FVector(-10.0f, 7.0f, -9.0f));
                MyChar->gunMeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
                MyChar->gunMeshComp->SetRelativeScale3D(FVector(1.5f));

                // ────────────────────────────────
                // (3) 상태 동기화
                // ────────────────────────────────
                MyChar->bUsingHammer = false;
                MyChar->bUsingPistolGun = true;

                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("▶ Pistol_Equip: Hand (True)"));
            }

            // ? [2] 권총 집어넣는 중일 때
            else if (MyChar->bUsingPistolGun == false)
            {
                // ────────────────────────────────
                // (1) 권총을 등으로 이동
                // ────────────────────────────────
                MyChar->gunMeshComp->AttachToComponent(
                    MeshComp,
                    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                    SocketName2 // 권총 등 소켓
                );
                MyChar->gunMeshComp->SetRelativeLocation(FVector(-0.7f, -15.5f, 16.980762f));
                MyChar->gunMeshComp->SetRelativeRotation(FRotator(0.000001f, -269.999999f, -180.0f));
                MyChar->gunMeshComp->SetRelativeScale3D(FVector(1.5f));

                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("▶ Pistol_Equip: Back (False)"));
            }
        }
    }
}
