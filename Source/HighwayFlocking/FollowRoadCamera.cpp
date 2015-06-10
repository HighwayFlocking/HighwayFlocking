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

#include "FollowRoadCamera.h"

AFollowRoadCamera::AFollowRoadCamera(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	SplineComponent = ObjectInitializer.CreateDefaultSubobject<USplineComponent>(this, "SplineComponent");
	SplineComponent->AttachParent = RootComponent;

	PrimaryActorTick.bCanEverTick = true;
	Speed = 80.0f;

	LookAtOffset = FVector::ZeroVector;
	Rev = false;
	MaxDist = 0.0f;
	Offset = FVector::ZeroVector;
	LookAtDistance = 0.0f;
	DegreesAboveFollow = 0.0f;
	FollowSmooth = 0.95;
}

void AFollowRoadCamera::BeginPlay() {
	for (TActorIterator<AReplayPlayer> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		ReplayPlayer = *ObjIt;
		break;
	}

	for (TActorIterator<ARoadmap> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		Roadmap = *ObjIt;
		break;
	}

	if (!Roadmap) {
		return;
	}

	/*float Min = 9999.0f;
	float Max = 0.0f;

	for (auto Iter = SplineInfo->Points.CreateConstIterator(); Iter; ++Iter) {
	auto Point = &*Iter;
	float Val = Point->InVal;
	Min = FMath::Min(Min, Val);
	Max = FMath::Max(Max, Val);
	}

	float TVal = 0.0f;
	float TotalDist = 0.0f;
	FVector PrevPoint = SplineInfo->Eval(0.0f, FVector(0, 0, 0));
	while (TVal < Max) {
	FVector Point = SplineInfo->Eval(TVal, PrevPoint);
	float Dist = (Point - PrevPoint).Size2D();
	PrevPoint = Point;
	TotalDist += Dist;
	SplineInfoDistance.AddPoint(TotalDist, TVal);

	TVal += 0.5;
	}


	for (auto Iter = SplineInfoDistance.Points.CreateIterator(); Iter; ++Iter) {
		auto Point = &*Iter;
		Point->InterpMode = CIM_CurveAuto;
	}

	SplineInfoDistance.AutoSetTangents();
	*/
}

void AFollowRoadCamera::Start(bool NReversed, float StartDistance)
{
	// This needs to be called after ARodmap::BeginPlay!
	Running = true;
	Rev = NReversed;

	SplineComponent->SplineInfo = Roadmap->SplineInfo;
	SplineComponent->UpdateSpline();
	SplineInfoDistance = SplineComponent->SplineReparamTable;

	MaxDist = SplineInfoDistance.Points.Last().InVal;
	FirstTick = true;

	float SpeedCMs = Speed / 0.036;
	float TimePassed = StartDistance / SpeedCMs;
	StartTime = GetWorld()->GetTimeSeconds() - TimePassed;

}

/************************************************************************/
/* Really bad name for a function which either clamps or normalizes the */
/* Axis, depending on what will get the value the longest away from     */
/* the point which two values that are close are really far from each   */
/* i.e. 360 or 0 for clamped, or -180 or 180 for normalized             */
/************************************************************************/
void GetNiceAngleValue(float& Angle1, float& Angle2) {
	// First we clamp the value to [0, 360)
	Angle1 = FRotator::ClampAxis(Angle1);
	Angle2 = FRotator::ClampAxis(Angle2);
	if (Angle1 > 150 && Angle1 < 210 && Angle2 > 150 && Angle2 < 210) {
		// We are in a good area, just return the new angle
		return;
	}
	else {
		// if not, we are normalizing

		// Yes, this runs ClampAxis one more time, but it is
		// more clear what this does.
		Angle1 = FRotator::NormalizeAxis(Angle1);
		Angle2 = FRotator::NormalizeAxis(Angle2);
	}
}

void GetNiceRotatorValue(FRotator& Rotator1, FRotator& Rotator2) {
	Rotator1.Normalize();
	Rotator2.Normalize();
	GetNiceAngleValue(Rotator1.Pitch, Rotator2.Pitch);
	GetNiceAngleValue(Rotator1.Yaw, Rotator2.Yaw);
	GetNiceAngleValue(Rotator1.Roll, Rotator2.Roll);
}

void AFollowRoadCamera::Tick(float Delta) {
	if (!Running) {
		return;
	}

	if (!Roadmap) {
		return;
	}

	float SpeedCMs = Speed / 0.036;

	float CurrentTime = GetWorld()->GetTimeSeconds();

	float TimePassed = CurrentTime - StartTime;

	float DistancePassed = TimePassed * SpeedCMs;

	if (Rev) {
		DistancePassed = MaxDist - DistancePassed;
	}

	float TValue = SplineInfoDistance.Eval(DistancePassed, 0);

	FVector Location = Roadmap->SplineInfo.Eval(TValue, FVector(0, 0, 0));
	FVector Tangent = Roadmap->SplineInfo.EvalDerivative(TValue, FVector(0, 0, 0)).GetSafeNormal2D();
	FVector Side = FVector::CrossProduct(FVector::UpVector, Tangent);

	Location += Tangent * Offset.X;
	Location += Side * Offset.Y;
	Location += FVector::UpVector * Offset.Z;

	FVector LookAtLocation;

	if (ReplayPlayer && ReplayPlayer->LookAtActor) {
		LookAtLocation = ReplayPlayer->LookAtActor->GetActorLocation();
	}
	else {

		float DistanceLookAt = 0;

		if (Rev) {
			DistanceLookAt = DistancePassed - LookAtDistance;
		}
		else {
			DistanceLookAt = DistancePassed + LookAtDistance;
		}

		float TValueLookAt = SplineInfoDistance.Eval(DistanceLookAt, 0);
		LookAtLocation = Roadmap->SplineInfo.Eval(TValueLookAt, FVector(0, 0, 0));
		FVector LookAtTangent = Roadmap->SplineInfo.EvalDerivative(TValueLookAt, FVector(0, 0, 0)).GetSafeNormal2D();
		FVector LookAtSide = FVector::CrossProduct(FVector::UpVector, LookAtTangent);

		LookAtLocation += LookAtTangent * LookAtOffset.X;
		LookAtLocation += LookAtSide * LookAtOffset.Y;
		LookAtLocation += FVector::UpVector * LookAtOffset.Z;
	}

	FRotator NewRotation = (LookAtLocation - Location).Rotation();
	NewRotation.Pitch += DegreesAboveFollow;

	if (FirstTick) {
		SetActorLocation(Location);
		SetActorRotation(NewRotation);
		FirstTick = false;
		return;
	}

	FVector LastLocation = GetActorLocation();
	FRotator LastRotation = GetActorRotation();
	// Note: This will not work if LastRotation and NewRotation are too far from
	// each other (more than 20 degrees), however, in that case, nothing will
	// work...
	GetNiceRotatorValue(NewRotation, LastRotation);
	NewRotation = NewRotation * (1-FollowSmooth) + LastRotation * FollowSmooth;
	NewRotation.Normalize();
	SetActorLocation(Location * 0.1 + LastLocation * 0.9);
	SetActorRotation(NewRotation);
}

