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

#include "../HighwayFlocking.h"
#include "Cohersion.h"
#include "../Utils.h"

UCohersion::UCohersion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {

}

FVector UCohersion::ProtectedGetDesiredVelocity(
	const UWorld* world,
	const FCamPacket& vehicle,
	const TArray<FCamPacket>& Neighbors,
	ARoadmap* Roadmap,
	EFlockingState& NextFlockingState){

	NextFlockingState = EFlockingState::FS_NoChange;

	if (world == NULL) {
		return FVector::UpVector;
	}

	int32 len = Neighbors.Num();
	if (len == 0) {
		return FVector::UpVector;
	}

	FVector SumVector(0, 0, 0);
	float ts = world->GetTimeSeconds();
	float NeighborsNum = 0;
	float MaxDistanceSq = MaxDistance * MaxDistance;
	FVector sideVector = FVector::CrossProduct(FVector::UpVector, vehicle.ForwardVector);

	for (int32 i = 0; i < len; i++) {
		FCamPacket neighbor = Neighbors[i];

		float delta = ts - neighbor.timestamp;
		FVector NLocation = neighbor.Location + neighbor.ForwardVector * neighbor.speed * delta;

		FVector NeighborVector = NLocation - vehicle.Location;
		NeighborVector.Z = 0;

		if (NeighborVector.SizeSquared() < MaxDistanceSq && FVector::DotProduct(vehicle.ForwardVector, neighbor.ForwardVector) > 0) {

			FVector projected = NeighborVector.ProjectOnTo(sideVector);
			SumVector += projected;

			//SumVector += NeighborVector;
			NeighborsNum += 1;
		}
	}

	if (NeighborsNum == 0) {
		return FVector::UpVector;
	}

	return (SumVector / NeighborsNum).GetSafeNormal2D();
}
