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
#include "AvoidPrioritized.h"
#include "../Utils.h"

UAvoidPrioritized::UAvoidPrioritized(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {

}

FVector UAvoidPrioritized::ProtectedGetDesiredVelocity(
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
		// If we have no neighbors, we just return that we do not care. That is defined to be
		// the up vector.
		return FVector::UpVector;
	}

	FVector Location = vehicle.Location;

	FVector AvoidVector(0, 0, 0);

	float ts = world->GetTimeSeconds();

	float AvoidNum = 0;

	FVector MyLocation = vehicle.Location;

	FVector RoadTangent = Roadmap->GetRoadTangentNearest(MyLocation);

	if (FVector::DotProduct(RoadTangent, vehicle.ForwardVector) < 0) {
		RoadTangent = -RoadTangent;
	}

	FVector RoadTangentRightVector = FVector::CrossProduct(FVector::UpVector, RoadTangent).GetSafeNormal2D();

	for (int32 i = 0; i < len; i++) {
		FCamPacket neighbor = Neighbors[i];
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("O, Hai there %d, %d!"), neighbor.PriorityLevel, vehicle.PriorityLevel));
		if (neighbor.PriorityLevel <= vehicle.PriorityLevel) {
			continue;
		}
		float MinDistanceSq = FVector(FrontDistance + vehicle.Length + neighbor.Length, AvoidStartDistance + neighbor.Width, 0).SizeSquared2D();

		float delta = ts - neighbor.timestamp;
		FVector NLocation = neighbor.Location + neighbor.ForwardVector * neighbor.speed * delta;

		if ((Location - NLocation).SizeSquared2D() > MinDistanceSq) {
			continue;
		}

		FVector SideVector = FVector::CrossProduct(FVector::UpVector, neighbor.ForwardVector);
		FVector ForwardVector = neighbor.ForwardVector;

		FVector LineStart = NLocation;
		FVector LineEnd = NLocation + ForwardVector * (neighbor.Length + FrontDistance);

		FVector LocalAvoidVector = ShortestVectorBetweenLineAndVehicle(LineStart, LineEnd, vehicle, ts, world);
		LocalAvoidVector.Z = 0;

		float Distance = LocalAvoidVector.Size();

		if (Distance < AvoidStartDistance) {
			float power;
			if (Distance < AvoidMinDistance) {
				power = 1.0f;
			}
			else {
				power = map_value(Distance, AvoidStartDistance, AvoidMinDistance, 0, 1);
			}

			if (neighbor.PriorityLevel == 1) {
				AvoidVector += -RoadTangentRightVector * power;
			}
			else {
				int32 sign;
				if (Roadmap->Vehicle2IsOnRight(vehicle, neighbor)) {
					sign = -1;
				}
				else {
					sign = 1;
				}
				AvoidVector += sign * RoadTangentRightVector * power;
			}
			AvoidNum += 1;
		}

	}

	if (AvoidNum == 0) {
		return FVector::UpVector;
	}

	AvoidVector = AvoidVector / AvoidNum;

	return AvoidVector;

}
