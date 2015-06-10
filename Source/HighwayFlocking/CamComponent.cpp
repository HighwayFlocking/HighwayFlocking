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

#include "FlockingVehicle.h"

#include "CAMComponent.h"

#define COLLISION_CAM_SENDING ECollisionChannel::ECC_GameTraceChannel1



UCamComponent::UCamComponent(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer) {
	SendInterval = 0.1f;
	bAutoActivate = true;

	BodyInstance.SetCollisionProfileName("Custom");
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	SetCollisionResponseToChannel(COLLISION_CAM_SENDING, ECollisionResponse::ECR_Block);



}

void UCamComponent::OnRegister() {
	Super::OnRegister();


	if (!(GetWorld()->WorldType == EWorldType::Game || GetWorld()->WorldType == EWorldType::PIE)) {
		// We are in a preview world. Do not start anything.
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner) {
		//FVector Origin;
		//FVector Extent;
		//Owner->GetActorBounds(false, Origin, Extent);
		//CamPacket.Width = 218.0f;
		//CamPacket.Length = 452.0f;
	}

	for (TActorIterator<ARoadmap> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		Roadmap = *ObjIt;
		break;
	}

	SetTimer();
}

FCamPacket UCamComponent::GetCamPacket() {
	return CamPacket;
}

void UCamComponent::SetTimer() {
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, this, &UCamComponent::OnTimer, SendInterval, false);
	LastTime = GetWorld()->GetTimeSeconds();
}

void UCamComponent::OnTimer() {
	float delta = GetWorld()->GetTimeSeconds() - LastTime;
	LastTime = GetWorld()->GetTimeSeconds();

	SendCamPackets();

	SetTimer();
}

void UCamComponent::UpdateCamPacket() {
	// This component can only be added to a wheeled vehicle. This is not particularry safe
	// TODO: Fail more gracefully, check earlier that we actually are connected to a wheeled vehicle
	AWheeledVehicle* Owner = (AWheeledVehicle*) GetOwner();
	CamPacket.Location = Owner->GetActorLocation();
	CamPacket.ForwardVector = Owner->GetActorForwardVector();
	CamPacket.ForwardVector.Z = 0;
	CamPacket.ForwardVector = CamPacket.ForwardVector.GetSafeNormal2D();
	CamPacket.speed = Owner->GetVehicleMovement()->GetForwardSpeed();
	CamPacket.timestamp = GetWorld()->GetTimeSeconds();
	CamPacket.StationID = Owner->GetUniqueID();
	CamPacket.Width = VehicleWidth;
	CamPacket.Length = VehicleLength;
	CamPacket.PriorityLevel = PriorityLevel;
	float distance;
	CamPacket.RoadmapTValue = Roadmap->SplineInfo.InaccurateFindNearest(CamPacket.Location, distance);
	AFlockingVehicle* Vehicle = Cast<AFlockingVehicle>(Owner);
	if (Vehicle) {
		CamPacket.MaxSpeed = Vehicle->MaxSpeed / 0.036;
		CamPacket.GoalSpeed = Vehicle->GoalSpeed;
	}
	else {
		CamPacket.MaxSpeed = 2222.2222;
		CamPacket.GoalSpeed = CamPacket.speed;
	}
	CamPacket.CreationTime = Owner->CreationTime;
}

void UCamComponent::SendCamPackets() {
	if (!Roadmap) {
		return;
	}
	AActor* const Owner = GetOwner();
	TArray<FOverlapResult> overlaps;

	UWorld* world = GetWorld();

	world->OverlapMulti(
		overlaps,
		Owner->GetActorLocation(),
		FQuat::Identity,
		COLLISION_CAM_SENDING,
		FCollisionShape::MakeSphere(Roadmap->FlockRadius),
		FCollisionQueryParams(false),
		FCollisionResponseParams::DefaultResponseParam);

	int32 len = overlaps.Num();

	UpdateCamPacket();

	for (int32 i = 0; i < len; i++) {
		FOverlapResult res = overlaps[i];

		if (res.GetActor() == NULL) {
			continue;
		}

		AActor* Actor = res.GetActor();
		UPrimitiveComponent* component = res.GetComponent();

		if (!Actor->IsA(AFlockingVehicle::StaticClass())) {
			continue;
		}

		if (res.GetActor() == Owner) {
			continue;
		}

		AFlockingVehicle* neighbor = (AFlockingVehicle*)res.GetActor();

		neighbor->ReceiveCamPacket(CamPacket);
	}
}
