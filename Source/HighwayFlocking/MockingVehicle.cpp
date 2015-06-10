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

#include "MockingVehicle.h"

#include "Behaviours/Behaviours.h"

AMockingVehicle::AMockingVehicle(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer){
	Mesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("MockingVehicleMesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	Mesh->BodyInstance.bSimulatePhysics = true;
	Mesh->BodyInstance.bNotifyRigidBodyCollision = true;
	Mesh->BodyInstance.bUseCCD = true;
	Mesh->bBlendPhysics = true;
	Mesh->bGenerateOverlapEvents = true;
	Mesh->bCanEverAffectNavigation = false;
	RootComponent = Mesh;

	Mesh->SetSimulatePhysics(false);
	bFindCameraComponentWhenViewTarget = true;


}

void AMockingVehicle::BeginPlay() {
	for (TActorIterator<ABehaviours> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		ABehaviours* Behaviors = *ObjIt;
		BehaviorsInState.Add((uint8)Behaviors->FlockingState, Behaviors);
	}
}

class USkeletalMeshComponent* AMockingVehicle::GetMesh() const {
	return Mesh;
}

EFlockingState AMockingVehicle::GetFlockingState() {
	return FlockingState;
}

void AMockingVehicle::SetFlockingState(EFlockingState NewFlockingState, bool ShowArrows) {
	for (int32 i = 0; i < ArrowComponents.Num(); i++) {
		UArrowComponent* ArrowComponent = ArrowComponents[i];
		ArrowComponent->DestroyComponent();
	}

	ArrowComponents.Empty(0);
	ArrowComponentForBehaviour.Empty(0);

	FlockingState = NewFlockingState;

	if (!BehaviorsInState.Contains((uint8)FlockingState)) {
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("Could not find a behaviours object!"));
		return;
	}

	ABehaviours* Behaviors = BehaviorsInState[(uint8)FlockingState];

	if (ShowArrows) {
		if (GetWorld()) {
			ArrowComponents.Empty(Behaviors->Behaviours.Num());
			ArrowComponentForBehaviour.Empty(Behaviors->Behaviours.Num());
			for (int32 i = 0; i < Behaviors->Behaviours.Num(); i++) {
				UBehaviour* behaviour = Behaviors->Behaviours[i];
				if (ShowArrowBehaviors->Num() == 0 || ShowArrowBehaviors->Contains(behaviour)) {
					UArrowComponent* ArrowComponent = ConstructObject<UArrowComponent>(UArrowComponent::StaticClass(), this);
					ArrowComponent->RegisterComponent();
					ArrowComponent->AttachTo(GetRootComponent(), NAME_None, EAttachLocation::SnapToTarget);
					ArrowComponent->ArrowColor = behaviour->DebugColor;
					if (VehicleType == EVehicleType::VT_Bus)
					{
						ArrowComponent->RelativeLocation = FVector(0, 0, 400);
					}
					else
					{
						ArrowComponent->RelativeLocation = FVector(0, 0, 200);
					}
					ArrowComponent->bHiddenInGame = false;
					ArrowComponent->ArrowSize = 8.0f;
					ArrowComponent->SetWorldScale3D(FVector(0.0, 0.5, 0.5));
					ArrowComponent->SetWorldRotation(FVector(0, 0.0, 1.0).Rotation());

					ArrowComponents.Add(ArrowComponent);
					ArrowComponentForBehaviour.Add(behaviour, ArrowComponent);
				}
			}
		}
	}
}

void AMockingVehicle::UpdateArrows() {
	if (!BehaviorsInState.Contains((uint8)FlockingState)) {
		return;
	}

	const ABehaviours* Behaviours = BehaviorsInState[(uint8)FlockingState];
	int32 len = Behaviours->Behaviours.Num();

	for (int32 i = 0; i < len; i++) {
		UBehaviour* behaviour = Behaviours->Behaviours[i];

		if (ArrowComponentForBehaviour.Contains(behaviour)) {
			UArrowComponent* ArrowComponent = ArrowComponentForBehaviour[behaviour];
			if (!ArrowComponent) {
				continue;
			}
			if (i >= BehaviorVectors.Num()) {
				continue;
			}
			ArrowComponent->SetWorldRotation(BehaviorVectors[i].Rotation());
			float Size = BehaviorVectors[i].Size();
			ArrowComponent->SetWorldScale3D(FVector(Size, 0.5, 0.5));
		}
	}
}
