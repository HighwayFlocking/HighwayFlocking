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
#include "KeepInsideRoad.h"
#include "../Utils.h"

UKeepInsideRoad::UKeepInsideRoad(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {

}

FVector UKeepInsideRoad::ProtectedGetDesiredVelocity(
		const UWorld* world,
		const FCamPacket& vehicle,
		const TArray<FCamPacket>& Neighbors,
		ARoadmap* Roadmap,
		EFlockingState& NextFlockingState){

	NextFlockingState = EFlockingState::FS_NoChange;

	FVector PointInFront = vehicle.Location + vehicle.ForwardVector * vehicle.speed * nrSecondsLookahead;

	float distanceSq;
	float nearest = Roadmap->SplineInfo.InaccurateFindNearest(PointInFront, distanceSq);

	float RadiusSq = Roadmap->RoadRadius * Roadmap->RoadRadius;

	//DrawDebugCylinder(GetWorld(), MyLocation, PointInFront, 10, 10, FColor::Red);
	//DrawDebugCylinder(GetWorld(), PointInFront, Roadmap->SplineInfo.Eval(nearest, FVector::ZeroVector), 10, 10, FColor::Blue);

	if (distanceSq < RadiusSq) {
		return FVector::UpVector;
	}

	float Distance = FMath::Sqrt(distanceSq);
	float Radius = Roadmap->RoadRadius;

	FVector orthogonalVector = FVector::CrossProduct(FVector::UpVector, vehicle.ForwardVector);

	FVector WorldSteeringVector = Roadmap->SplineInfo.Eval(nearest, FVector::ZeroVector) - vehicle.Location;
	FVector projected = WorldSteeringVector.ProjectOnTo(orthogonalVector);

	projected = projected.GetSafeNormal();

	float extra = Distance - Radius;

	extra = FMath::Min(extra, 200.0f);

	float force = map_value(extra, 0.0f, 200.0f, 0.0f, 1.0f);

	return projected * force;
}
