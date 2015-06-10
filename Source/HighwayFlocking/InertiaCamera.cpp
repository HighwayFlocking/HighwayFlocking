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

#include "Kismet/KismetMathLibrary.h"

#include "InertiaCamera.h"

AInertiaCamera::AInertiaCamera(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	PrimaryActorTick.bCanEverTick = true;
}

void AInertiaCamera::Tick(float Delta) {
	if (FollowComponent == NULL) {
		return;
	}

	FVector Location = GetActorLocation();
	FRotator Rotation = GetActorRotation();

	FVector CompLocation = FollowComponent->GetComponentLocation();
	FRotator CompRotation = FollowComponent->GetComponentRotation();

	Location = Location * 0.7 + CompLocation * 0.3;
	Rotation = Rotation * 0.7 + CompRotation * 0.3;

	LocationError = (CompLocation - Location) * 0.05 + LocationError * 0.95;
	RotationError = (CompRotation - Rotation) * 0.05 + RotationError * 0.95;

	//Location = FMath::VInterpTo(Location, CompLocation, Delta, 10);
	//Rotation = FMath::RInterpTo(Rotation, CompRotation, Delta, 10);

	SetActorRotation(Rotation + RotationError);
	SetActorLocation(Location + LocationError);

/*
	CurrentLocation = 0.5 * CurrentLocation + 0.5 * FollowComponent->GetComponentLocation();

	FTransform Transform = FollowComponent->ComponentToWorld;
	FTransform CurrentTransform = GetTransform();
	Transform.Rotat

	FTransform ThisTransform = UKismetMathLibrary::TInterpTo(GetTransform(), Transform, Delta, 10);

	SetActorTransform(ThisTransform);*/

	//SetActorLocation(CurrentLocation);
	//FVector MyLocation = GetActorLocation();
	//FVector CurrentLocation = FollowComponent->GetComponentLocation();
	//SetActorLocation(CurrentLocation, false);
	//SetActorRotation(FollowComponent->GetComponentRotation());

	//SetActorLocation(FollowActor->GetActorLocation() + FollowComponent->RelativeLocation);
	//SetActorTransform(FollowComponent->GetRelativeTransform() * FollowActor->GetTransform());
	//SetActorLocation(FollowActor->GetActorLocation() + FVector(0, 0, 1000));

	//FTransform Trans = FollowComponent->ComponentToWorld;

	//FVector LocationComp = CurrentLocation;
	//FVector LocationAct = FollowActor->GetActorLocation();
	//FVector MyLoc = GetActorLocation();
	//SetActorLocation(LocationComp);
	//SetActorRotation(FollowComponent->GetComponentRotation());
}
