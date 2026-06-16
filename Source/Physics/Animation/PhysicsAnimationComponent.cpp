// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/PhysicsAnimationComponent.h"
#include "PhysicsAnimationComponent.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include <GameFramework/CharacterMovementComponent.h>
#include <GameFramework/Character.h>
#include "AIController.h"

// Sets default values for this component's properties
UPhysicsAnimationComponent::UPhysicsAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UPhysicsAnimationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* pOwnerActor = GetOwner())
	{
		m_Mesh = pOwnerActor->GetComponentByClass<USkeletalMeshComponent>();
		pOwnerActor->OnTakePointDamage.AddDynamic(this, &UPhysicsAnimationComponent::OkTakePointDamage);
	}

	m_fBlendTimer = 0.f;
}

void UPhysicsAnimationComponent::OkTakePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	if (!m_Mesh || !m_bEnabled) return;


	bool bBreakable = false;
	
	if (m_tBreakableBones.Contains(BoneName))
	{
		bBreakable = true;

		// FVector Impulse, FVector HitLocation, FName InBoneName
		m_Mesh->BreakConstraint(FVector::ZeroVector, HitLocation, BoneName);
		m_Mesh->SetBodySimulatePhysics(m_sAnimatedBoneName, true);

		// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConstraintBrokenSignature, int32, ConstraintIndex);
		m_Mesh->OnConstraintBroken.Broadcast(m_Mesh->FindConstraintIndex(BoneName));

		m_tBrokenBones.AddUnique(BoneName);
	}
	else
	{
		m_sAnimatedBoneName = m_sOverrideAnimatedBoneName.IsNone() ? BoneName : m_sOverrideAnimatedBoneName;
		
		m_Mesh->SetAllBodiesBelowSimulatePhysics(m_sAnimatedBoneName, true, true);

		m_fBlendTimer = m_fBlendDuration;
	}
	
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, bBreakable?FColor::Green:FColor::Red, BoneName.ToString());

}

void UPhysicsAnimationComponent::EndComponent()
{
	m_fBlendTimer = 0.f;
	m_bEnabled = false;
	m_Mesh->SetAllBodiesBelowPhysicsBlendWeight(m_sAnimatedBoneName, 1);
	m_Mesh->SetAllBodiesBelowSimulatePhysics(m_sAnimatedBoneName, false, true);
	m_tBrokenBones.Empty();
}

void UPhysicsAnimationComponent::CustomSetAllBodiesBelowPhysicsBlendWeight(const FName& InBoneName, float PhysicsBlendWeight, bool bSkipCustomPhysicsType, bool bIncludeSelf)
{
	m_Mesh->SetAllBodiesBelowPhysicsBlendWeight(InBoneName, PhysicsBlendWeight, bSkipCustomPhysicsType, bIncludeSelf);

	int32 NumBodiesFound = m_Mesh->ForEachBodyBelow(InBoneName, bIncludeSelf, bSkipCustomPhysicsType, [PhysicsBlendWeight, this, InBoneName](FBodyInstance* BI)
		{
				BI->PhysicsBlendWeight = (m_tBrokenBones.Find(InBoneName)) ? 1 : PhysicsBlendWeight;
		});
}

// Called every frame
void UPhysicsAnimationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (m_fBlendTimer > 0.f)
	{
		m_fBlendTimer = FMath::Max(m_fBlendTimer, 0.f);
		m_fBlendTimer -= DeltaTime;

		if (m_pCurve && m_Mesh)
		{
			//CustomSetAllBodiesBelowPhysicsBlendWeight(m_sAnimatedBoneName, m_pCurve->GetFloatValue(1.f - m_fBlendTimer/m_fBlendDuration));
			m_Mesh->SetAllBodiesBelowPhysicsBlendWeight(m_sAnimatedBoneName, m_pCurve->GetFloatValue(1.f - m_fBlendTimer / m_fBlendDuration));
		}

		if (m_fBlendTimer <= 0.f)
		{
			bool bNewSimulate = false;
			m_Mesh->SetAllBodiesBelowSimulatePhysics(m_sAnimatedBoneName, bNewSimulate, true);
		}
	}
}

