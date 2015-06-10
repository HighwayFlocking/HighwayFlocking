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
#include "Roadmap.h"

#include "Components/SplineComponent.h"
#include "LandscapeSplinesComponent.h"
#include "LandscapeSplineControlPoint.h"


ARoadmap::ARoadmap(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

void ARoadmap::BeginPlay() {
	CalculateRoadmap();

	for (TActorIterator<AActor> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		AActor* actor = *ObjIt;
		if (!actor->ActorHasTag(FName(TEXT("car_path")))) {
			continue;
		}

		RampSplineComponent = (USplineComponent*)actor->FindComponentByClass(USplineComponent::StaticClass());
		break;
	}
}

void ARoadmap::CalculateRoadmap() {
	ULandscapeSplinesComponent* comp = NULL;
	for (TObjectIterator<ULandscapeSplinesComponent> ObjIt; ObjIt; ++ObjIt) {
		comp = *ObjIt;
		break;
	}

	if (comp == NULL) {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Could not find any landscape splines"));
		return;
	}

	SplineInfo.Reset();

	SplineInfo.Points.Empty(comp->ControlPoints.Num());

	FVector LandscapeLocation = comp->GetOwner()->GetActorLocation();

	// First we need to find one end of the highway, in case the road does not form a circle. If there are no end we can start anywhere
	// This is needed because the ControlPoints is saved in the order they were added, not their individual order
	// TODO: Nothing of this works if the road is dividing

	ULandscapeSplineControlPoint* ControlPoint = NULL;

	for (int32 i = 0; i < comp->ControlPoints.Num(); i++) {
		if (comp->ControlPoints[i]->ConnectedSegments.Num() == 1) {
			ControlPoint = comp->ControlPoints[i];
			break;
		}
	}

	if (ControlPoint == NULL) {
		// The road is a circle, just start at any control point, for example the first one created
		ControlPoint = comp->ControlPoints[0];
	}

	SplineInfo.AddPoint(0, ControlPoint->Location + LandscapeLocation);
	SplineInfo.Points[0].InterpMode = CIM_CurveUser;

	int32 point_i = 1;

	TArray<class ULandscapeSplineSegment*> OrderedSegments;

	ULandscapeSplineControlPoint* StartControlPoint = ControlPoint;

	FTransform transform = comp->GetOwner()->ActorToWorld();

	int round = 0;

	while (ControlPoint) {
		bool found = false;
		for (int32 i = 0; i < ControlPoint->ConnectedSegments.Num(); i++) {
			FLandscapeSplineConnection connection = ControlPoint->ConnectedSegments[i];
			ULandscapeSplineSegment* segment = connection.Segment;
			if (!OrderedSegments.Contains(segment)) {
				OrderedSegments.Add(segment);

				// First we need to find the location and rotation on the start and end of the segment

				ULandscapeSplineControlPoint* NextControlPoint = connection.GetFarConnection().ControlPoint;

				FVector StartLocation; FRotator StartRotation;
				FVector EndLocation; FRotator EndRotation;
				ControlPoint->GetConnectionLocationAndRotation(connection.GetNearConnection().SocketName, StartLocation, StartRotation);
				NextControlPoint->GetConnectionLocationAndRotation(connection.GetFarConnection().SocketName, EndLocation, EndRotation);

				StartLocation = StartLocation + LandscapeLocation;
				EndLocation = EndLocation + LandscapeLocation;

				// The start point already exists (since it was created in the last iteration). We just need to update the leave tangent
				SplineInfo.Points[point_i - 1].LeaveTangent = StartRotation.Vector() * connection.GetNearConnection().TangentLen;


				SplineInfo.AddPoint(point_i, EndLocation);
				SplineInfo.Points[point_i].InterpMode = CIM_CurveUser;
				SplineInfo.Points[point_i].ArriveTangent = EndRotation.Vector() * -connection.GetFarConnection().TangentLen;
				point_i++;

				ControlPoint = connection.GetFarConnection().ControlPoint;
				found = true;
				break;
			}
		}

		if (!found) {
			break;
		}


		// A check to stop if we are in a infinite loop
		round++;
		if (round > 100) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Stopping infinite loop!!!!"));
			break;
		}

		if (ControlPoint == StartControlPoint) {
			break;
		}
	}


}

FVector ARoadmap::GetRoadTangentNearest(FVector Location) {
	float distanceSq;
	float nearest = SplineInfo.InaccurateFindNearest(Location, distanceSq);

	FVector RoadTangent = SplineInfo.EvalDerivative(nearest, FVector::ZeroVector).GetSafeNormal2D();

	return RoadTangent;
}

/**
* Returns true if Vehicle2 is on the right side of Vehicle1, where right and left is defined
* by the direction Vehicle1 is traveling on the road.
*/
bool ARoadmap::Vehicle2IsOnRight(FCamPacket Vehicle1, FCamPacket Vehicle2) {
	FVector Location1 = Vehicle1.Location;
	FVector Location2 = Vehicle2.Location;
	float DistanceSq;
	float Nearest1 = SplineInfo.InaccurateFindNearest(Location1, DistanceSq);
	float Nearest2 = SplineInfo.InaccurateFindNearest(Location2, DistanceSq);
	FVector RoadPoint1 = SplineInfo.Eval(Nearest1, FVector::ZeroVector);
	FVector RoadPoint2 = SplineInfo.Eval(Nearest2, FVector::ZeroVector);

	FVector ToRoadPoint1 = RoadPoint1 - Location1;
	ToRoadPoint1.Z = 0;
	FVector ToRoadPoint2 = RoadPoint2 - Location2;
	ToRoadPoint2.Z = 0;

	FVector RoadTangent1 = SplineInfo.EvalDerivative(Nearest1, FVector::ZeroVector);
	FVector RoadTangent2 = SplineInfo.EvalDerivative(Nearest2, FVector::ZeroVector);
	RoadTangent1.Z = 0;
	RoadTangent2.Z = 0;

	// This should be either (0,0,1) or (0,0,-1), depending on which side of the road the vehicles are
	// (Defined by the direction the road is going)

	FVector CrossProduct1 = FVector::CrossProduct(ToRoadPoint1, RoadTangent1);
	FVector CrossProduct2 = FVector::CrossProduct(ToRoadPoint2, RoadTangent2);
	float CrossProductSign1 = CrossProduct1.Z / abs(CrossProduct1.Z);
	float CrossProductSign2 = CrossProduct2.Z / abs(CrossProduct2.Z);

	// SideVector is pointing to the right of the vehicle
	FVector SideVector1 = FVector::CrossProduct(FVector::UpVector, Vehicle1.ForwardVector);

	// This Vector should be either (0,0,1) or (0,0,-1), depending of which of these defines the right side
	// of the road, where right and left is defined by Vehicle1
	float RightSideCrossProductSign;
	if (FVector::DotProduct(SideVector1, ToRoadPoint1) > 0) {
		RightSideCrossProductSign = -CrossProductSign1;
	}
	else {
		RightSideCrossProductSign = CrossProductSign1;
	}

	// If CrossProduct1 and CrossProduct2 is different, Vehicle2 is on the right if CrossProduct2 is RightSideCrossProduct

	if (CrossProductSign1 != CrossProductSign2) {
		return CrossProductSign2 == RightSideCrossProductSign;
	}

	float Distance1 = ToRoadPoint1.SizeSquared2D();
	float Distance2 = ToRoadPoint2.SizeSquared2D();

	// We know that they are on the same side, and CrossProduct1 == CrossProduct2. So we just look at the distances
	// from the road center for each of them, and combine that with tho knowledge of which side they are on

	if (Distance2 > Distance1) {
		return CrossProductSign1 == RightSideCrossProductSign;
	}
	else {
		return CrossProductSign1 != RightSideCrossProductSign;
	}

}
