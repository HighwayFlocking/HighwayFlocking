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
#include "AvoidOncoming.h"
#include "../Utils.h"

UAvoidOncoming::UAvoidOncoming(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {

}

FVector UAvoidOncoming::ProtectedGetDesiredVelocity(
		const UWorld* world,
		const FCamPacket& vehicle,
		const TArray<FCamPacket>& Neighbors,
		ARoadmap* Roadmap,
		EFlockingState& NextFlockingState){

	NextFlockingState = EFlockingState::FS_NoChange;


	/*
	In this we define a length from the midpoint. This length is set such that the middle of the road is 0,
	and right of the middle is positive. Left of the middle is negative. Right and left is defined by the
	current vehicles direction.
	*/

	FVector MyLocation;
	float MyDistance;	// the distance to the road midpoint
	FVector RightSideCrossProduct;
	FVector MyToMidpoint;
	FVector SideVector;

	{
		MyLocation = vehicle.Location + vehicle.ForwardVector * 500;
		MyLocation.Z = 0;

		float MyDistanceSq;
		const float nearest = Roadmap->SplineInfo.InaccurateFindNearest(MyLocation, MyDistanceSq);
		FVector MyRoadPoint = Roadmap->SplineInfo.Eval(nearest, FVector::ZeroVector);
		MyRoadPoint.Z = 0;
		MyToMidpoint = MyRoadPoint - MyLocation;
		MyToMidpoint.Z = 0;
		MyDistance = MyToMidpoint.Size2D();
		FVector RoadTangent = Roadmap->SplineInfo.EvalDerivative(nearest, FVector::ZeroVector);
		RoadTangent.Z = 0;

		// MyCrossProduct should be either (0,0,1) or (0,0,-1), depending on which side of the
		// road we are.
		const FVector MyCrossProduct = FVector::CrossProduct(MyToMidpoint, RoadTangent).GetSafeNormal();

		// Check to see if we are on the left side of the road
		// SideVector is pointing to the right of us.

		SideVector = FVector::CrossProduct(FVector::UpVector, vehicle.ForwardVector);
		if (FVector::DotProduct(SideVector, MyToMidpoint) > 0) {
			MyDistance = -MyDistance;
			RightSideCrossProduct = -MyCrossProduct;
		}
		else {
			RightSideCrossProduct = MyCrossProduct;
		}

		MyDistance = MyDistance - vehicle.Width / 2 - Margin;
	}

	if (MyDistance > Roadmap->RoadRadius) {
		return FVector::UpVector;	// we are outside of the road
	}

	float Closest = -9e10;

	int32 len = Neighbors.Num();
	float ts = world->GetTimeSeconds();

	for (int32 i = 0; i < len; i++) {
		FCamPacket Neighbor = Neighbors[i];
		float delta = ts - Neighbor.timestamp;
		FVector NLocation = Neighbor.Location + Neighbor.ForwardVector * Neighbor.speed * delta;

		if (FVector::DotProduct(vehicle.ForwardVector, Neighbor.ForwardVector) > 0) {
			// We do not care about vehicles running the same way as we do.
			continue;
		}

		FVector NeighborVector = NLocation - MyLocation;

		if (FVector::DotProduct(NeighborVector, vehicle.ForwardVector) < 0) {
			// We do not care about vehicles behind us
			continue;
		}

		float NeighborNearest = Neighbor.RoadmapTValue;

		FVector NeighborRoadPoint = Roadmap->SplineInfo.Eval(NeighborNearest, FVector::ZeroVector);
		NeighborRoadPoint.Z = 0;
		FVector NeighborToMidpoint = NeighborRoadPoint - NLocation;
		NeighborToMidpoint.Z = 0;
		float NeighborDistance = NeighborToMidpoint.Size2D();
		FVector NeighborRoadTangent = Roadmap->SplineInfo.EvalDerivative(NeighborNearest, FVector::ZeroVector);
		NeighborRoadTangent.Z = 0;

		FVector NeighborCrossProduct = FVector::CrossProduct(NeighborToMidpoint, NeighborRoadTangent).GetSafeNormal();
		float NeighborCrossProductSign = NeighborCrossProduct.Z / abs(NeighborCrossProduct.Z);
		float RightSideCrossProductSign = RightSideCrossProduct.Z / abs(RightSideCrossProduct.Z);
		if (NeighborCrossProductSign != RightSideCrossProductSign) {
			NeighborDistance = -NeighborDistance;
		}

		if (NeighborDistance > Roadmap->RoadRadius) {
			// The vehicle is outside the road on our right side, we do not want to drive around it, since that means that we would
			// be driving of the road. This will not hit if it is on the left side, sinde then NeighborDistance would be negative.
			continue;
		}

		NeighborDistance += Neighbor.Width;

		if (NeighborDistance > Closest) {
			Closest = NeighborDistance;
		}
	}

	if (Closest < MyDistance) {
		// We are safe
		return FVector::UpVector;
	}

	float Difference = Closest - MyDistance;

	FVector MoveDirection = MyToMidpoint;

	// Check if we are on the right side
	if (FVector::DotProduct(SideVector, MyToMidpoint) < 0) {
		MoveDirection = -MoveDirection;
	}

	MoveDirection = MoveDirection.GetSafeNormal2D();

	float Force = FMath::Min(1.0f, map_value(Difference, 0, 300, 0, 1));

	float ForceMultiplier = FMath::Clamp(map_value(MyDistance, Roadmap->RoadRadius - StartDecayDistanceFromEdge, Roadmap->RoadRadius - NoEffectDistanceFromEdge, 1.0f, 0.0f), 0.0f, 1.0f);


	return MoveDirection * Force * ForceMultiplier;
}
