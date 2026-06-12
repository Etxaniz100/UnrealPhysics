// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/ProjectileWeaponComponent.h"
#include "PhysicsCharacter.h"
#include "PhysicsProjectile.h"
#include "Camera/CameraComponent.h" 

void UProjectileWeaponComponent::Fire()
{
	Super::Fire();


	FVector vLookingDir = Character->FirstPersonCameraComponent->GetComponentRotation().Vector();
	FVector vFireStartPoint = Character->FirstPersonCameraComponent->GetComponentLocation(); //Character->GetActorLocation();// FireOffset->GetComponentLocation();
	FVector vFireEndPoint = Character->FirstPersonCameraComponent->GetComponentLocation() + vLookingDir * m_fFireDistance;

	FHitResult oHit;

	//DrawDebugLine(GetWorld(), vFireStartPoint, vFireEndPoint, FColor::Blue, false, 1.0f, 0, 2.0f);
	FCollisionQueryParams oParams;
	//oParams.AddIgnoredActor(Character);

	FVector vImpactPoint = vFireEndPoint;

	if (GetWorld()->LineTraceSingleByChannel(oHit, vFireStartPoint, vFireEndPoint, ECC_Visibility, oParams))
	{
		vImpactPoint = oHit.ImpactPoint;
	}

	DrawDebugSphere(GetWorld(), vImpactPoint, 3, 50, FColor::Green, false, 1.f);


	FVector vSpawnLocation = GetOwner()->GetActorLocation() + GetComponentRotation().RotateVector(MuzzleOffset);
	FVector vFireDirection = vImpactPoint - vSpawnLocation;
	//FRotator oSpawnRotation = GetComponentRotation();

	FActorSpawnParameters oSpawnParams;
	oSpawnParams.Owner = GetOwner();
	//oSpawnParams.Instigator = GetInstigator();
	oSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APhysicsProjectile* pProjectile = GetWorld()->SpawnActor<APhysicsProjectile>(m_ProjectileClass, vSpawnLocation, vFireDirection.Rotation(), oSpawnParams);
	if (pProjectile)
	{
		pProjectile->m_OwnerWeapon = this;
	}
}
