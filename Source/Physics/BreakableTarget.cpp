#include "BreakableTarget.h"
#include <GeometryCollection/GeometryCollectionComponent.h>

FTargetBroken ABreakableTarget::OnTargetBroken;

// Sets default values
ABreakableTarget::ABreakableTarget()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	SetRootComponent(StaticMesh);

	GeometryCollection = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollection"));
	GeometryCollection->SetupAttachment(StaticMesh);
	GeometryCollection->OnChaosBreakEvent.AddDynamic(this, &ABreakableTarget::GeometryCollectionBroken);
	GeometryCollection->SetNotifyBreaks(true);

}

// Called when the game starts or when spawned
void ABreakableTarget::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABreakableTarget::GeometryCollectionBroken(const FChaosBreakEvent& BreakEvent)
{
	if (!m_IsBroken)
	{
		OnTargetBroken.Broadcast(this);
		m_IsBroken = true;
		GeometryCollection->SetNotifyBreaks(false);
	}
}

// Called every frame
void ABreakableTarget::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

