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
#include "Avoid.h"
#include "../Utils.h"

UAvoid::UAvoid(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {

}

FVector UAvoid::ProtectedGetDesiredVelocity(
		const UWorld* world,
		const FCamPacket& vehicle,
		const TArray<FCamPacket>& Neighbors,
		ARoadmap* Roadmap,
		EFlockingState& NextFlockingState){

	NextFlockingState = EFlockingState::FS_NoChange;

	if (world == NULL) {
		return FVector::UpVector;
	}

	// The general idea is that we go through each neighbor. For each one we calculate a avoid
	// vector and a power. We then take the average.

	int32 len = Neighbors.Num();

	if (len == 0) {
		// If we have no neighbors, we just return that we do not care. That is defined to be
		// the up vector.
		return FVector::UpVector;
	}

	FVector Location = vehicle.Location;

	FVector AvoidVector(0, 0, 0);

	// We use some math to find a vector that is orthogonal to the tangent on the road.
	FVector ForwardVector = Roadmap->SplineInfo.EvalDerivative(vehicle.RoadmapTValue, vehicle.ForwardVector).GetSafeNormal2D();
	FVector SideVector = FVector::CrossProduct(FVector::UpVector, ForwardVector);

	float ts = world->GetTimeSeconds();

	float AvoidNum = 0;

	float VehicleFrontStartDistance = FrontStartDistance;

	if (vehicle.PriorityLevel == 1) {
		VehicleFrontStartDistance = FrontStartDistancePri1;
	}
	else if (vehicle.PriorityLevel == 2) {
		VehicleFrontStartDistance = FrontStartDistancePri2;
	}


	for (int32 i = 0; i < len; i++) {
		FCamPacket neighbor = Neighbors[i];

		float ThisFrontStartDistance = VehicleFrontStartDistance;

		if (neighbor.PriorityLevel == 1) {
			ThisFrontStartDistance = FrontStartDistancePri1;
		}
		else if (neighbor.PriorityLevel == 2) {
			ThisFrontStartDistance = FrontStartDistancePri2;
		}

		float MinDistanceSq = FVector(ThisFrontStartDistance + vehicle.Length + neighbor.Length, SideStartDistance + vehicle.Width + neighbor.Width, 0).SizeSquared2D();

		// We will try to calculate the position the neighbor is in right now. We assume that the
		// vehicle have driven following its last forward vector using its last speed. This is a
		// relative safe assumtion, since we get a update package 20 times a second.

		float delta = ts - neighbor.timestamp;
		FVector NLocation = neighbor.Location + neighbor.ForwardVector * neighbor.speed * delta;

		// ShortestVectorBetween is a relatively expensive operation, therefore we first check the distance between
		// just the midpoints.

		if ((Location - NLocation).SizeSquared2D() > MinDistanceSq) {
			continue;
		}

		FVector LocalAvoidVector = ShortestVectorBetween(neighbor, vehicle, ts, world);//Location - neighbor.Location;
		LocalAvoidVector.Z = 0;

		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Hello!"));

		//DrawDebugCylinder(world, Location + FVector::UpVector * 100, Location + LocalAvoidVector + FVector::UpVector * 100, 10, 10, FColor::Green);

		float FrontDistance = abs(FVector::DotProduct(LocalAvoidVector, ForwardVector));
		float SideDistance = abs(FVector::DotProduct(LocalAvoidVector, SideVector));

		bool collide = FrontDistance < ThisFrontStartDistance && SideDistance < SideStartDistance;

		// For distances lower than MinDistance, we return a vector with length 0.
		// For distances between MinDistance and StartDistance we return a vector with
		// length between 1 and 0;

		if (collide) {
			float power;
			if (FrontDistance < FrontMinDistance && SideDistance < SideMinDistance) {
				power = 1;
			}
			else {
				float PowerFront = map_value(FrontDistance, ThisFrontStartDistance, FrontMinDistance, 0, 1);
				float PowerSide = map_value(SideDistance, SideStartDistance, SideMinDistance, 0, 1);
				power = FMath::Min(PowerFront, PowerSide);
			}

			// For the direction, we use the vector between the midpoints.

			LocalAvoidVector = Location - NLocation;

			LocalAvoidVector = LocalAvoidVector.GetSafeNormal();

			AvoidVector += LocalAvoidVector * power;
			AvoidNum += 1;
		}
	}

	if (AvoidNum == 0) {
		return FVector::UpVector;
	}

	AvoidVector = AvoidVector / AvoidNum;

	//FVector RoadTangent = Roadmap->GetRoadTangentNearest(Location).SafeNormal();

	// Move in Max Speed in a direction that is like

	return AvoidVector;

}
