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
#include "KeepRight.h"
#include "../Utils.h"

UKeepRight::UKeepRight(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {

}

FVector UKeepRight::ProtectedGetDesiredVelocity(
	const UWorld* world,
	const FCamPacket& vehicle,
	const TArray<FCamPacket>& Neighbors,
	ARoadmap* Roadmap,
	EFlockingState& NextFlockingState){

	NextFlockingState = EFlockingState::FS_NoChange;

	if (vehicle.PriorityLevel != 1) {
		return FVector::UpVector;
	}

	FVector MyLocation;
	float MyDistance;
	FVector MyToMidpoint;
	FVector sideVector;
	{
		MyLocation = vehicle.Location;
		MyLocation.Z = 0;
		float MyDistanceSq;
		const float nearest = Roadmap->SplineInfo.InaccurateFindNearest(MyLocation, MyDistanceSq);	// returns a "t"-value
		FVector MyRoadPoint = Roadmap->SplineInfo.Eval(nearest, FVector::ZeroVector);
		MyRoadPoint.Z = 0;
		MyToMidpoint = MyRoadPoint - MyLocation;
		MyToMidpoint.Z = 0;
		float roadRadius = Roadmap->RoadRadius;
		MyDistance = MyToMidpoint.Size2D();

		sideVector = FVector::CrossProduct(FVector::UpVector, vehicle.ForwardVector);

		if (FVector::DotProduct(sideVector, MyToMidpoint) > 0) {
			MyDistance = abs(roadRadius - MyDistance);
		}
		else {
			MyDistance += roadRadius;
		}
	}

	FVector result = sideVector / pow((MyDistance / Scaling), Exponent);
	if (result.Size2D() > 1)
		return result.GetSafeNormal2D();
	return result;
}
