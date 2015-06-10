/* Copyright 2015 Sindre Ilebekk Johansen and Andreas Sløgedal Løvland

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "HighwayFlocking.h"
#include "MeasurementGate.h"

static const FColor TriggerBaseColor(100, 255, 100, 255);

AMeasurementGate::AMeasurementGate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBoxComponent>(TEXT("CollisionComp"))) {
	UBoxComponent* BoxCollisionComponent = CastChecked<UBoxComponent>(GetCollisionComponent());

	BoxCollisionComponent->ShapeColor = TriggerBaseColor;
	BoxCollisionComponent->InitBoxExtent(FVector(40.0f, 40.0f, 40.0f));

	static FName CollisionProfileName(TEXT("Trigger"));
	BoxCollisionComponent->SetCollisionProfileName(CollisionProfileName);

	if (GetSpriteComponent())
	{
		GetSpriteComponent()->AttachParent = BoxCollisionComponent;
	}

	OnActorBeginOverlap.AddDynamic(this, &AMeasurementGate::OnBeginOverlap);
}

void AMeasurementGate::OnBeginOverlap(AActor* OtherActor) {
	FName Tag = FName(*(TEXT("Measured ") + GetName()));
	if (OtherActor->Tags.Contains(Tag)) {
		return;
	}
	OtherActor->Tags.Add(Tag);

	uint64 ts = (int)GetWorld()->GetTimeSeconds();

	if (ts != LastMeasurementTime) {
		for (int i = LastMeasurementTime + 1; i <= ts; i++) {
			NumVehiclesDestroyedSecond[i % NUM_SECONDS_STATS][0] = 0;
			NumVehiclesDestroyedSecond[i % NUM_SECONDS_STATS][1] = 0;
		}
	}

	if (FVector::DotProduct(OtherActor->GetActorForwardVector(), GetActorForwardVector()) < 0) {
		NumVehiclesDestroyedSecond[ts % NUM_SECONDS_STATS][0] += 1;
	}
	else {
		NumVehiclesDestroyedSecond[ts % NUM_SECONDS_STATS][1] += 1;
	}
	LastMeasurementTime = ts;
}

float AMeasurementGate::GetThroughput(uint8 Lane) {
	float Result = 0;
	for (uint32 i = 0; i < NUM_SECONDS_STATS; i++) {
		Result += (float)NumVehiclesDestroyedSecond[i][Lane];
	}
	Result = Result * (60.0f / (float)NUM_SECONDS_STATS) * 60.0f; // vehicles per hour
	return Result;
}

#if WITH_EDITOR
void AMeasurementGate::EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	const FVector ModifiedScale = DeltaScale * (AActor::bUsePercentageBasedScaling ? 500.0f : 5.0f);

	UBoxComponent * BoxComponent = Cast<UBoxComponent>(GetRootComponent());
	if (bCtrlDown)
	{
		// CTRL+Scaling modifies trigger collision height.  This is for convenience, so that height
		// can be changed without having to use the non-uniform scaling widget (which is
		// inaccessable with spacebar widget cycling).
		FVector Extent = BoxComponent->GetUnscaledBoxExtent() + FVector(0, 0, ModifiedScale.X);
		Extent.Z = FMath::Max(0.0f, Extent.Z);
		BoxComponent->SetBoxExtent(Extent);
	}
	else
	{
		FVector Extent = BoxComponent->GetUnscaledBoxExtent() + FVector(ModifiedScale.X, ModifiedScale.Y, ModifiedScale.Z);
		Extent.X = FMath::Max(0.0f, Extent.X);
		Extent.Y = FMath::Max(0.0f, Extent.Y);
		Extent.Z = FMath::Max(0.0f, Extent.Z);
		BoxComponent->SetBoxExtent(Extent);
	}
}
#endif	//WITH_EDITOR
