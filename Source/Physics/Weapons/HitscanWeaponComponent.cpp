// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/HitscanWeaponComponent.h"
#include <Kismet/KismetSystemLibrary.h>
#include <Kismet/GameplayStatics.h>
#include "PhysicsCharacter.h"
#include "PhysicsWeaponComponent.h"
#include "Camera/CameraComponent.h" 

void UHitscanWeaponComponent::Fire()
{
	if (!Character || !Character->FirstPersonCameraComponent)
	{
		return;
	}

	Super::Fire();


	FVector vLookingDir = Character->FirstPersonCameraComponent->GetComponentRotation().Vector();
	FVector vFireStartPoint = Character->FirstPersonCameraComponent->GetComponentLocation(); //Character->GetActorLocation();// FireOffset->GetComponentLocation();
	FVector vFireEndPoint = Character->FirstPersonCameraComponent->GetComponentLocation() + vLookingDir * m_fFireDistance;


	FHitResult oHit;

	//DrawDebugLine(GetWorld(), vFireStartPoint, vFireEndPoint, FColor::Blue, false, 1.0f, 0, 2.0f);
	FCollisionQueryParams oParams;
	//oParams.AddIgnoredActor(Character);
	if (GetWorld()->LineTraceSingleByChannel(oHit, vFireStartPoint, vFireEndPoint, ECC_Visibility, oParams))
	{

		if (oHit.GetComponent() && oHit.GetComponent()->IsSimulatingPhysics())
		{
			oHit.GetComponent()->AddImpulse(vLookingDir * 100000);
		
			DrawDebugSphere(GetWorld(), oHit.ImpactPoint, 3, 50, FColor::Green, false, 1.f);
		}
		else
		{
			DrawDebugSphere(GetWorld(), oHit.ImpactPoint, 3, 50, FColor::Red, false, 1.f);
		}
	}

}
