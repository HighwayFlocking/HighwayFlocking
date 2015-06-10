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
#include "Recorder.h"
#include "Pack.h"
#include "PhysXIncludes.h"

#include <direct.h>

// We write to disk and reset when the buffer is larger than this
#define BUFFER_SIZE MSGPACK_SBUFFER_INIT_SIZE - 1024

#define VEHICLEINFO_DEFAULT 2
#define VEHICLEINFO_SPAWNED 3

#define VEHICLE_DESTROYED 0
#define VEHICLES_CRASHED 1
#define TICK 2

ARecorder::ARecorder(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics; // This is the default
	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
}

void ARecorder::BeginPlay() {
	//StartRecording("D:\\Users\\sindre\\Documents\\Unreal Projects\\master\\controller\\recordings\\AvoidTest.bin");
}

void ARecorder::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	StopRecording();
}

void ARecorder::Tick(float Delta) {
	if (!IsRecording) {
		return;
	}

	msgpack_pack_array(&pk, 3);
	Pack(&pk, GetWorld()->GetTimeSeconds());
	msgpack_pack_uint16(&pk, TICK);

	msgpack_pack_array(&pk, Vehicles.Num() + NewVehicles.Num());
	for (auto Iter = Vehicles.CreateConstIterator(); Iter; ++Iter) {
		const AFlockingVehicle* Vehicle = *Iter;
		if (!Vehicle) {
			continue;
		}
		UWheeledVehicleMovementComponent* movementComp = Vehicle->GetVehicleMovementComponent();
		UVehicleWheel* FrontLeftWheel = movementComp->Wheels[0];

		msgpack_pack_array(&pk, 15);
		msgpack_pack_uint8(&pk, VEHICLEINFO_DEFAULT);  //0  v1
		Pack(&pk, Vehicle->GetUniqueID());  //1  v1
		Pack(&pk, Vehicle->GetActorLocation());  //2  v1
		Pack(&pk, Vehicle->GetActorRotation());  //3  v1
		Pack(&pk, Vehicle->BehaviorVectors);  //4  v1
		Pack(&pk, Vehicle->GetFlockingState());  //5  v1
		Pack(&pk, Vehicle->GetVelocity());  //6  v1
		Pack(&pk, Vehicle->GoalSpeed);  //7  v1
		Pack(&pk, movementComp->GetCurrentGear());  //8  v1
		Pack(&pk, movementComp->GetEngineRotationSpeed());  //9  v1
		Pack(&pk, Vehicle->ThrottleOutput);  //10  v1
		Pack(&pk, FrontLeftWheel->GetRotationAngle());  //11  v2
		Pack(&pk, movementComp->PVehicle->mWheelsDynData.getWheelRotationSpeed(0)); //12  v1
		Pack(&pk, FrontLeftWheel->GetSteerAngle());  //13  v2
		Pack(&pk, FrontLeftWheel->GetSuspensionOffset()); //14 v2
	}
	for (auto Iter = NewVehicles.CreateConstIterator(); Iter; ++Iter) {
		const AFlockingVehicle* Vehicle = *Iter;
		if (!Vehicle) {
			continue;
		}
		UWheeledVehicleMovementComponent* movementComp = Vehicle->GetVehicleMovementComponent();
		UVehicleWheel* FrontLeftWheel = movementComp->Wheels[0];

		msgpack_pack_array(&pk, 20);
		msgpack_pack_uint8(&pk, VEHICLEINFO_SPAWNED);  //0  v1
		Pack(&pk, Vehicle->GetUniqueID());  //1  v1
		Pack(&pk, Vehicle->GetActorLocation());  //2  v1
		Pack(&pk, Vehicle->GetActorRotation());  //3  v1
		Pack(&pk, Vehicle->BehaviorVectors);  //4  v1
		Pack(&pk, Vehicle->GetFlockingState());  //5  v1
		Pack(&pk, Vehicle->GetVelocity());  //6  v1
		Pack(&pk, Vehicle->GoalSpeed);  //7  v1
		Pack(&pk, movementComp->GetCurrentGear());  //8  v1
		Pack(&pk, movementComp->GetEngineRotationSpeed());  //9  v
		Pack(&pk, Vehicle->ThrottleOutput);  //10  v1
		Pack(&pk, FrontLeftWheel->GetRotationAngle());  //11  v2
		Pack(&pk, movementComp->PVehicle->mWheelsDynData.getWheelRotationSpeed(0)); //12  v1
		Pack(&pk, FrontLeftWheel->GetSteerAngle());  //13  v2
		Pack(&pk, FrontLeftWheel->GetSuspensionOffset()); //14 v2

		Pack(&pk, Vehicle->CreationTime);  //11  v1 | 15 v2
		Pack(&pk, Vehicle->MaxSpeed);  //12  v1 | 16  v2
		Pack(&pk, Vehicle->VehicleType);  //13  v1 | 17  v2
		Pack(&pk, Vehicle->CamComponent->PriorityLevel);  //14  v1 | 18 v2
		Pack(&pk, Vehicle->ColorMaterialIndex);  //15  v1 | 19 v2

		Vehicles.Add(Vehicle);
	}
	NewVehicles.Empty();
	if (sbuf.size > BUFFER_SIZE) {
		FlushToDisk();
	}
}

void ARecorder::VehicleSpawned(AFlockingVehicle* Vehicle) {
	if (!IsRecording) {
		return;
	}
	// We wait for the Vehicle to be fully initiated, so wait till next tick to tell get its data
	NewVehicles.Add(Vehicle);

}

void ARecorder::VehicleDestroyed(AFlockingVehicle* Vehicle) {
	if (!IsRecording) {
		return;
	}
	if (!Vehicle) {
		return;
	}
	UWheeledVehicleMovementComponent* movementComp = Vehicle->GetVehicleMovementComponent();

	Vehicles.Remove(Vehicle);
	msgpack_pack_array(&pk, 17);
	Pack(&pk, GetWorld()->GetTimeSeconds());  //0  v1
	msgpack_pack_uint16(&pk, VEHICLE_DESTROYED);  //1  v1
	// Since it should be possible to play  backwards, VEHICLE_DESTROYED should contain the same information
	// as VEHICLE_SPAWNED
	Pack(&pk, Vehicle->GetUniqueID());  //2  v1
	Pack(&pk, Vehicle->GetActorLocation());  //3  v1
	Pack(&pk, Vehicle->GetActorRotation());  //4  v1
	Pack(&pk, Vehicle->BehaviorVectors);  //5  v1
	Pack(&pk, Vehicle->GetFlockingState());  //6  v1
	Pack(&pk, Vehicle->GetVelocity());  //7  v1
	Pack(&pk, Vehicle->GoalSpeed);  //8  v1
	Pack(&pk, movementComp->GetCurrentGear());  //9  v1
	Pack(&pk, movementComp->GetEngineRotationSpeed());  //10  v1
	Pack(&pk, Vehicle->ThrottleOutput);  //11  v1

	Pack(&pk, Vehicle->CreationTime);  //12  v1
	Pack(&pk, Vehicle->MaxSpeed);  //13  v1
	Pack(&pk, Vehicle->VehicleType);  //14  v1
	Pack(&pk, Vehicle->CamComponent->PriorityLevel);  //15  v1
	Pack(&pk, Vehicle->ColorMaterialIndex);  //16  v1
}

void ARecorder::VehicleCrashed(AFlockingVehicle* MainActor, AActor* OtherActor) {
	if (!IsRecording) {
		return;
	}
	msgpack_pack_array(&pk, 4);
	Pack(&pk, GetWorld()->GetTimeSeconds());
	msgpack_pack_uint16(&pk, VEHICLES_CRASHED);
	Pack(&pk, MainActor->GetUniqueID());
	Pack(&pk, OtherActor->GetUniqueID());
}

void ARecorder::FlushToDisk() {
	fwrite(sbuf.data, sizeof(char), sbuf.size, file);
	msgpack_sbuffer_clear(&sbuf);
}

void ARecorder::StartRecording(FString Filename) {
	Vehicles.Empty();
	for (TActorIterator<AFlockingVehicle> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		NewVehicles.Add(*ObjIt);
	}
	msgpack_sbuffer_clear(&sbuf);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Recording to '%s'"), *Filename));
	fopen_s(&file, TCHAR_TO_ANSI(*Filename), "wb");

	// First we add the version
	msgpack_pack_uint8(&pk, 3);

	if (file) {
		IsRecording = true;
	}
}

void ARecorder::StopRecording() {
	if (!IsRecording) {
		return;
	}
	FlushToDisk();
	IsRecording = false;
	if (file) {
		fclose(file);
	}
}
