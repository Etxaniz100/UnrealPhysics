// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhysicsCharacter.h"
#include "PhysicsProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h" 
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Components/StaticMeshComponent.h>
#include <PhysicsEngine/PhysicsHandleComponent.h>

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

APhysicsCharacter::APhysicsCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	m_PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
}

void APhysicsCharacter::BeginPlay()
{
	Super::BeginPlay();
	m_CurrentStamina = m_MaxStamina;
	m_CurrentHealth = m_MaxHealth;
}

void APhysicsCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);



	if (m_bIsGrabbing)
	{
		FVector vLookingDir = FirstPersonCameraComponent->GetComponentRotation().Vector();
		vLookingDir.Normalize();
		FVector vPos = GetActorLocation() + vLookingDir * m_fGrabDistance;
		m_PhysicsHandle->SetTargetLocation(vPos);
	}
	else
	{
		// Find posible object to outline

		FVector vLookingDir = FirstPersonCameraComponent->GetComponentRotation().Vector();
		FVector vFireStartPoint = FirstPersonCameraComponent->GetComponentLocation();
		FVector vFireEndPoint = FirstPersonCameraComponent->GetComponentLocation() + vLookingDir * m_MaxGrabDistance;

		FHitResult oHit;
		FCollisionQueryParams oParams;

		UPrimitiveComponent* pGrabedComponent = nullptr;

		if (GetWorld()->LineTraceSingleByChannel(oHit, vFireStartPoint, vFireEndPoint, ECC_Visibility, oParams))
		{
			if (oHit.GetActor() != m_pHighlightedObject)
			{
				if(m_pHighlightedObject)
				{
					auto tComps = m_pHighlightedObject->GetComponents();
					for (UActorComponent* pComp : tComps)
					{
						if (UMeshComponent* pMesh = Cast< UMeshComponent>(pComp))
						{
							pMesh->SetOverlayMaterial(nullptr);
						}
					}
					m_pHighlightedObject = nullptr;
				}
			
				if (oHit.GetComponent() && oHit.GetComponent()->IsSimulatingPhysics())
				{
					m_pHighlightedObject = oHit.GetActor();
					pGrabedComponent = oHit.GetComponent();
					auto tComps = m_pHighlightedObject->GetComponents();
					for (UActorComponent* pComp : tComps)
					{
						if (UMeshComponent* pMesh = Cast< UMeshComponent>(pComp))
						{
							pMesh->SetOverlayMaterial(m_HighlightMaterial);
						}
					}
				}
			}
		}
		else
		{
			if (m_pHighlightedObject)
			{
				auto tComps = m_pHighlightedObject->GetComponents();
				for (UActorComponent* pComp : tComps)
				{
					if (UMeshComponent* pMesh = Cast< UMeshComponent>(pComp))
					{
						pMesh->SetOverlayMaterial(nullptr);
					}
				}
				m_pHighlightedObject = nullptr;
			}
		}
	}
}

void APhysicsCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APhysicsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Look);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APhysicsCharacter::Sprint);
		EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::GrabObject);
		EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Completed, this, &APhysicsCharacter::ReleaseObject);
		EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::ZoomIn);
		EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Completed, this, &APhysicsCharacter::ZoomOut);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void APhysicsCharacter::SetIsSprinting(bool NewIsSprinting)
{
}

void APhysicsCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void APhysicsCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X * m_Sensitivity);
		AddControllerPitchInput(LookAxisVector.Y * m_Sensitivity);
	}
}

void APhysicsCharacter::Sprint(const FInputActionValue& Value)
{
	SetIsSprinting(Value.Get<bool>());
}

void APhysicsCharacter::GrabObject(const FInputActionValue& Value)
{
	if (!m_PhysicsHandle || m_bIsGrabbing)
	{
		return;
	}

	FVector vLookingDir = FirstPersonCameraComponent->GetComponentRotation().Vector();
	FVector vFireStartPoint = FirstPersonCameraComponent->GetComponentLocation(); 
	FVector vFireEndPoint = FirstPersonCameraComponent->GetComponentLocation() + vLookingDir * m_MaxGrabDistance;

	FHitResult oHit;
	FCollisionQueryParams oParams;
	
	UPrimitiveComponent* pGrabedComponent = nullptr;

	if (GetWorld()->LineTraceSingleByChannel(oHit, vFireStartPoint, vFireEndPoint, ECC_Visibility, oParams))
	{

		if (oHit.GetComponent() && oHit.GetComponent()->IsSimulatingPhysics())
		{
			pGrabedComponent = oHit.GetComponent();

			DrawDebugSphere(GetWorld(), oHit.ImpactPoint, 3, 50, FColor::Green, false, 1.f);
		}
		else
		{
			DrawDebugSphere(GetWorld(), oHit.ImpactPoint, 3, 50, FColor::Red, false, 1.f);
		}
	}

	if (pGrabedComponent)
	{
		m_bIsGrabbing = true;
		// (class UPrimitiveComponent* Component, FName InBoneName, FVector GrabLocation, FRotator Rotation)
		//m_PhysicsHandle->GrabComponentAtLocationWithRotation(pGrabedComponent, "", oHit.ImpactPoint, oHit.GetActor()->GetActorRotation());
		m_PhysicsHandle->GrabComponent(pGrabedComponent, FName(), oHit.ImpactPoint, false);
		m_fGrabDistance = FVector::Distance(oHit.ImpactPoint, GetActorLocation());
		
		m_PhysicsHandle->SetInterpolationSpeed(m_BaseInterpolationSpeed/(pGrabedComponent->GetMass()==0.f?0.001f:pGrabedComponent->GetMass()));
	}
}

void APhysicsCharacter::ReleaseObject(const FInputActionValue& Value)
{
	if (!m_bIsGrabbing)
	{
		return;
	}
	m_PhysicsHandle->ReleaseComponent();
	m_bIsGrabbing = false;
}

void APhysicsCharacter::ZoomIn()
{
	m_Sensitivity = 0.5f;
	OnZoomIn.Broadcast();
}

void APhysicsCharacter::ZoomOut()
{
	m_Sensitivity = 1.f;
	OnZoomOut.Broadcast();
}
