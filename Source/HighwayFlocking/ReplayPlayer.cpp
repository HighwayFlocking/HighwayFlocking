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
#include "ReplayPlayer.h"
#include "Unpack.h"
#include "VehicleCameraComponent.h"
#include "InertiaCamera.h"

#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>

// We write to disk and reset when the buffer is larger than this
#define BUFFER_SIZE MSGPACK_SBUFFER_INIT_SIZE - 1024

#define VEHICLEINFO_DEFAULT 2
#define VEHICLEINFO_SPAWNED 3

#define VEHICLE_DESTROYED 0
#define VEHICLES_CRASHED 1
#define TICK 2

AReplayPlayer::AReplayPlayer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics; // This is the default
	msgpack_zone_init(&mempool, 2048);

	static ConstructorHelpers::FClassFinder<AMockingVehicle> CarFinder(TEXT("/Game/Vehicle/MockingCar"));
	static ConstructorHelpers::FClassFinder<AMockingVehicle> SedanFinder(TEXT("/Game/Vehicle/MockingSedan"));
	static ConstructorHelpers::FClassFinder<AMockingVehicle> EmergencyFinder(TEXT("/Game/Vehicle/MockingEmergencyVehicle"));
	static ConstructorHelpers::FClassFinder<AMockingVehicle> BusFinder(TEXT("/Game/Vehicle/MockingBus"));
	//static ConstructorHelpers::FClassFinder<AMockingVehicle> InertiaCameraFinder(TEXT("/Game/Blueprints/InertiaCameraBP"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Yellow(TEXT("Material'/Game/Sedan/Materials/M_Vehicle_Sedan_yelllo.M_Vehicle_Sedan_yelllo'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Blue(TEXT("Material'/Game/Sedan/Materials/M_Vehicle_Sedan.M_Vehicle_Sedan'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Green(TEXT("Material'/Game/Sedan/Materials/M_Vehicle_Sedan_green.M_Vehicle_Sedan_green'"));


	VehicleTypeClass.Add((uint8)EVehicleType::VT_Car, CarFinder.Class);
	VehicleTypeClass.Add((uint8)EVehicleType::VT_Sedan, SedanFinder.Class);
	VehicleTypeClass.Add((uint8)EVehicleType::VT_Bus, BusFinder.Class);
	VehicleTypeClass.Add((uint8)EVehicleType::VT_Emergency, EmergencyFinder.Class);

	TArray<UMaterial*> Materials;
	Materials.Add(Yellow.Object);
	Materials.Add(Blue.Object);
	Materials.Add(Green.Object);

	VehicleTypeMaterials.Add((uint8)EVehicleType::VT_Car, Materials);
	VehicleTypeMaterials.Add((uint8)EVehicleType::VT_Sedan, Materials);

	static ConstructorHelpers::FClassFinder<AActor> ArrowFinder(TEXT("/Game/Blueprints/Arrow"));

	AArrow = ArrowFinder.Class;

	ShowArrows = true;
	ShowArrowBehaviors.Empty(0);
}

void AReplayPlayer::BeginPlay() {
	for (TActorIterator<AVehiclestats> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		Stats = *ObjIt;
	}
}

void AReplayPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	StopPlayback();
}

void AReplayPlayer::Tick(float Delta) {
	if (!IsPlaybacking) {
		ForceTime = false;
		return;
	}

	if (Paused) {
		ForceTime = false;
		return;
	}

	if (!ForceTime) {
		if (Direction == EPlaybackDirection::PD_Forward) {
			Time += Delta * Speed;
		}
		else {
			Time -= Delta * Speed;
		}
	}
	ForceTime = false;
	msgpack_object* obj = NULL;
	while (true) {
		if (Direction == EPlaybackDirection::PD_Forward) {
			if (Time < NextTime) {
				break;
			}
		}
		else {
			if (Time > NextTime) {
				break;
			}
		}
		if (Direction == EPlaybackDirection::PD_Forward) {
			obj = NextPacket();
		}
		else{
			obj = PrevPacket();
		}

		if (!obj) {
			Paused = true;
			Time = CurrentTime;
			Stats->SetTimePaused(CurrentTime - StartTime);
			return;
		}
		if (!(obj->type == MSGPACK_OBJECT_ARRAY)) {
			continue;
		}
		check(obj->type == MSGPACK_OBJECT_ARRAY);

		ProcessPacket(obj);

		// Alternative use SetTime and add a SetTimeSpeed on VehicleStats. But the difference
		// would be really small
		Stats->SetTimePaused(CurrentTime - StartTime);
	}

	UpdateActors();
}

EVehicleData* AReplayPlayer::GetOrCreateVehicleData(uint32 UniqueId) {
	if (!Vehicles.Contains(UniqueId)) {
		EVehicleData NewVehicleData;
		NewVehicleData.Vehicle = NULL;
		NewVehicleData.Current.Exists = false;
		NewVehicleData.Next.Exists = false;
		Vehicles.Add(UniqueId, NewVehicleData);
	}
	return &Vehicles[UniqueId];
}

void AReplayPlayer::ProcessPacket(msgpack_object* obj) {
	msgpack_object* p = obj->via.array.ptr;
	const float PacketTime = p[0].via.f64;

	const uint8 Type = p[1].via.u64;

	switch (Type) {
	case VEHICLE_DESTROYED: {
		uint32 UniqueId = p[2].via.u64;
		if (Direction == EPlaybackDirection::PD_Forward) {
			if (!Vehicles.Contains(UniqueId)) {
				return;
			}
			EVehicleDataPoint* VehicleDataPoint = &(Vehicles[UniqueId].Next);
			VehicleDataPoint->Exists = false;
			VehicleDataPoint->New = false;
		}
		else {
			EVehicleDataPoint* VehicleDataPoint = &(GetOrCreateVehicleData(UniqueId)->Next);
			VehicleDataPoint->New = true;
			VehicleDataPoint->Exists = true;
			VehicleDataPoint->Location = Unpack<FVector>(p + 3);;
			VehicleDataPoint->Rotation = Unpack<FRotator>(p + 4);
			VehicleDataPoint->BehaviorVectors = Unpack<TArray<FVector>>(p + 5);
			if (ReplayVersion <= 2) {
				// Between version 2 and 3 two behaviors are removed
				// (We should really save the names somewhere in the replay file, instead of using indexes)
				if (VehicleDataPoint->FlockingState == EFlockingState::FS_Default) {
					VehicleDataPoint->BehaviorVectors.RemoveAt(6);
					VehicleDataPoint->BehaviorVectors.RemoveAt(4);
					VehicleDataPoint->BehaviorVectors.SwapMemory(0, 5);
					VehicleDataPoint->BehaviorVectors.SwapMemory(3, 5);
				}
			}
			VehicleDataPoint->FlockingState = Unpack<EFlockingState>(p + 6);
			VehicleDataPoint->Velocity = Unpack<FVector>(p + 7);
			VehicleDataPoint->ColorMaterialIndex = p[16].via.u64;
			VehicleDataPoint->VehicleType = Unpack<EVehicleType>(p + 14);
			VehicleDataPoint->LastUpdateTime = PacketTime;
		}
		break;
	}
	case VEHICLES_CRASHED: {
		break;
	}
	case TICK: {
		CurrentTime = NextTime;
		NextTime = PacketTime;
		check(p[2].type == MSGPACK_OBJECT_ARRAY);
		msgpack_object* vehicles = p[2].via.array.ptr;
		for (auto Iter = Vehicles.CreateIterator(); Iter; ++Iter) {
			EVehicleData* VehicleData = &(Iter.Value());
			VehicleData->Current = VehicleData->Next;
		}
		for (uint32 i = 0; i < p[2].via.array.size; ++i) {
			check(vehicles[i].type == MSGPACK_OBJECT_ARRAY);
			msgpack_object* vehicle = vehicles[i].via.array.ptr;

			uint8 Type = vehicle[0].via.u64;
			uint32 UniqueId = vehicle[1].via.u64;

			EVehicleData* VehicleData = GetOrCreateVehicleData(UniqueId);
			//delete &(VehicleData->Current.BehaviorVectors);
			EVehicleDataPoint* VehicleDataPoint = &(VehicleData->Next);

			if (Type == VEHICLEINFO_SPAWNED && Direction == EPlaybackDirection::PD_Forward) {
				uint8 SpawnInfoStart = -100;

				// Since we have been needing to add fields in the middle for the default fields
				// to always be the same value, we need to do this.
				switch (ReplayVersion) {
				case 1: SpawnInfoStart = 11; break;
				case 2: SpawnInfoStart = 15; break;
				case 3: SpawnInfoStart = 15; break;
				default: check(false); break;
				}

				VehicleDataPoint->New = true;
				VehicleDataPoint->Exists = true;
				VehicleDataPoint->ColorMaterialIndex = vehicle[SpawnInfoStart + 4].via.u64;
				VehicleDataPoint->VehicleType = Unpack<EVehicleType>(vehicle + SpawnInfoStart + 2);
			}
			else {
				VehicleDataPoint->New = false;
			}
			VehicleDataPoint->Location = Unpack<FVector>(vehicle + 2);
			VehicleDataPoint->Rotation = Unpack<FRotator>(vehicle + 3);
			VehicleDataPoint->FlockingState = Unpack<EFlockingState>(vehicle + 5);
			VehicleDataPoint->BehaviorVectors = Unpack<TArray<FVector>>(vehicle + 4);
			if (ReplayVersion <= 2) {
				// Between version 2 and 3 two behaviors are removed
				// (We should really save the names somewhere in the replay file, instead of using indexes)
				if (VehicleDataPoint->FlockingState == EFlockingState::FS_Default) {
					if (VehicleDataPoint->BehaviorVectors.Num() == 9) {
						VehicleDataPoint->BehaviorVectors.RemoveAt(6);
						VehicleDataPoint->BehaviorVectors.RemoveAt(4);
						VehicleDataPoint->BehaviorVectors.SwapMemory(0, 5);
						VehicleDataPoint->BehaviorVectors.SwapMemory(3, 5);
					}
				}
			}
			VehicleDataPoint->Velocity = Unpack<FVector>(vehicle + 6);
			VehicleDataPoint->LastUpdateTime = PacketTime;

			if (ReplayVersion >= 2) {
				VehicleDataPoint->WheelRotationAngle = Unpack<double>(vehicle + 11);
				VehicleDataPoint->WheelRotationSpeed = Unpack<double>(vehicle + 12);
				VehicleDataPoint->WheelSteerAngle = Unpack<double>(vehicle + 13);
				VehicleDataPoint->SuspensionOffset = Unpack<double>(vehicle + 14);
			}

			if (Type == VEHICLEINFO_SPAWNED && Direction == EPlaybackDirection::PD_Backward) {
				VehicleDataPoint->Exists = false;
			}
		}
		break;
	}
	}
}

void AReplayPlayer::UpdateActors() {

	FActorSpawnParameters spawnParameters;
	spawnParameters.bNoCollisionFail = true;
	spawnParameters.Owner = this;
	spawnParameters.Instigator = NULL;
	spawnParameters.bDeferConstruction = false;

	for (auto Iter = Vehicles.CreateIterator(); Iter; ++Iter) {
		EVehicleData* VehicleData = &(Iter.Value());
		EVehicleDataPoint* Current = &(VehicleData->Current);
		EVehicleDataPoint* Next = &(VehicleData->Next);
		uint32 UniqueId = Iter.Key();

		// Important Note. There might be multiple vehicles with the same unique id, but not
		// at the same time! Therefore, if a new vehicle is spawned with this id, without
		// the old being deleted (could happen if spooling), we need to destroy the old
		// actor.
		if (Current->New && VehicleData->Vehicle) {
			VehicleData->Vehicle->Destroy();
			VehicleData->Vehicle = NULL;
		}
		Current->New = false;

		if (!Current->Exists) {
			AMockingVehicle* Vehicle = VehicleData->Vehicle;
			if (UniqueId == LookAtUniqueId) {
				LookAtActor = NULL;
			}
			if (Vehicle) {
				if (!Vehicle->Destroy()) {
					continue;
				}

			}
			VehicleData->Vehicle = NULL;
			if (!VehicleData->Next.Exists) {
				Iter.RemoveCurrent();
			}

			continue;
		}

		float dt = Time - Current->LastUpdateTime;
		float TillNext;
		if (Current->LastUpdateTime == Next->LastUpdateTime) {
			TillNext = 0;
		}
		else {
			TillNext = (Time - Current->LastUpdateTime) / (Next->LastUpdateTime - Current->LastUpdateTime);
		}
		FVector ChangeLocation = Next->Location - Current->Location;
		FVector TimeCorrectedLocation = Current->Location + ChangeLocation * TillNext;

		FRotator ChangeRotation = (Next->Rotation - Current->Rotation).GetNormalized();
		FRotator TimeCorrectedRotation = (Current->Rotation + ChangeRotation * TillNext).GetNormalized();

		float ChangeSuspensionOffset = Next->SuspensionOffset - Current->SuspensionOffset;
		float TimeCorrectedSuspensionOffset = Current->SuspensionOffset + ChangeSuspensionOffset * TillNext;

		if (VehicleData->Vehicle) {
			VehicleData->Vehicle->SetActorLocation(TimeCorrectedLocation);
			VehicleData->Vehicle->SetActorRotation(TimeCorrectedRotation);
			VehicleData->Vehicle->BehaviorVectors = Current->BehaviorVectors;
			float WheelSpeed = -1.0f * FMath::RadiansToDegrees(Current->WheelRotationSpeed);
			VehicleData->Vehicle->WheelRotationAngle = Current->WheelRotationAngle + WheelSpeed * dt;

			VehicleData->Vehicle->WheelSteerAngle = Current->WheelSteerAngle;
			VehicleData->Vehicle->SuspensionOffset = TimeCorrectedSuspensionOffset;

			if (Current->FlockingState != VehicleData->Vehicle->GetFlockingState()) {
				VehicleData->Vehicle->SetFlockingState(Current->FlockingState, ShowArrows);
			}

			VehicleData->Vehicle->UpdateArrows();


			// We do not want to do this before the vehicle has been created, therefore we do it the first time
			// it arrives here
			if (VehicleData->FirstTick) {
				if (UniqueId == VehicleCameraId) {
					SelectVehicleCamera(VehicleData->Vehicle, VehicleCameraName);
				}

				if (UniqueId == LookAtUniqueId) {
					LookAtActor = VehicleData->Vehicle;
				}
				VehicleData->FirstTick = false;
			}
		} else {
			TSubclassOf<class AMockingVehicle> Type = VehicleTypeClass[(uint8)Current->VehicleType];
			AMockingVehicle* NewVehicle = GetWorld()->SpawnActor<AMockingVehicle>(Type, TimeCorrectedLocation, Current->Rotation, spawnParameters);
			NewVehicle->ReplayUniqueId = UniqueId;
			if (VehicleTypeMaterials.Contains((uint8)Current->VehicleType)) {
				auto Materials = VehicleTypeMaterials[(uint8)Current->VehicleType];
				int32 Index = Current->ColorMaterialIndex;
				UMaterial* Material = Materials[Index];
				NewVehicle->GetMesh()->SetMaterial(2, Material);
			}

			NewVehicle->VehicleType = Current->VehicleType;
			NewVehicle->ShowArrowBehaviors = &ShowArrowBehaviors;
			NewVehicle->SetFlockingState(Current->FlockingState, ShowArrows);
			NewVehicle->BehaviorVectors = Current->BehaviorVectors;
			NewVehicle->UpdateArrows();

			VehicleData->Vehicle = NewVehicle;
			VehicleData->FirstTick = true;
		}

	}
}

// Note that calling any of the next three metohds will destroy all the pointers that
// have been retrieved from one of the metods earlier.
// This is really, really not threadsafe.

msgpack_object* AReplayPlayer::PacketAtOffset(size_t TmpOffset) {
	msgpack_unpack_return ret = msgpack_unpack(data, size, &offset, &mempool, &tmp_object);
	if (ret <= 0) {
		return NULL;
	}

	return &tmp_object;
}

msgpack_object* AReplayPlayer::PeekNextPacket() {
	size_t offset_copy = offset;
	msgpack_zone_clear(&mempool);
	msgpack_unpack_return ret = msgpack_unpack(data, size, &offset_copy, &mempool, &tmp_object);
	if (ret <= 0) {
		return NULL;
	}

	return &tmp_object;
}

msgpack_object* AReplayPlayer::NextPacket() {
	EOffsetHistory* CurrentOffset = (EOffsetHistory*) malloc(sizeof(EOffsetHistory));
	CurrentOffset->Offset = offset;
	CurrentOffset->Previous = OffsetHistory;
	OffsetHistory = CurrentOffset;
	msgpack_zone_clear(&mempool);
	msgpack_unpack_return ret = msgpack_unpack(data, size, &offset, &mempool, &tmp_object);
	if (ret <= 0) {
		return NULL;
	}

	return &tmp_object;
}

msgpack_object* AReplayPlayer::PrevPacket() {
	if (OffsetHistory == NULL) {
		return NULL;
	}
	EOffsetHistory* LastOffset = OffsetHistory;
	OffsetHistory = LastOffset->Previous;
	size_t last_offset = LastOffset->Offset;
	free(LastOffset);
	LastOffset = NULL;

	// To be able to play forward again. This is now the offset of the one to be read,
	// if we were to read in forward direction.
	// We want the first packet to be read when changing direction to be the same as the
	// last one read before changing direction. This undo the work done if creating
	// or deleting a vehicle.
	offset = last_offset;

	msgpack_unpack_return ret = msgpack_unpack(data, size, &last_offset, &mempool, &tmp_object);

	if (ret <= 0) {
		return NULL;
	}

	return &tmp_object;
}

void AReplayPlayer::StartPlayback(FString Filename) {
	for (TActorIterator<AWheeledVehicle> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		(*ObjIt)->Destroy();
	}

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Playing from '%s'"), *Filename));

	file = CreateFile(*Filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (file == INVALID_HANDLE_VALUE) {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Could not open: %s"), *Filename));
		return;
	};

	size = GetFileSize(file, 0);

	mapping = CreateFileMapping(file, 0, PAGE_READONLY, 0, 0, 0);

	if (mapping == 0) {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Could not map: %s"), *Filename));
		CloseHandle(file);
		return;
	}

	data = (const char*)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);

	if (!data) {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Could not create a view of %s"), *Filename));
		CloseHandle(mapping);
		CloseHandle(file);
		return;
	}

	// If we sometime in the future wants to support Posix (Linux or Mac), something like this might do it
	/*int fd = open(TCHAR_TO_ANSI(*Filename), O_RDONLY);
	fstat(fd, &sb);
	struct stat sb;
	size = sb.st_size;
	file = mmap(NULL, size, PROT_READ, MAP_SHARED, fs, 0);*/

	ResetPlayback();

	IsPlaybacking = true;

	//msgpack_unpacker_init(&unpacker, MSGPACK_UNPACKER_INIT_BUFFER_SIZE);


	// Jump time to the time of the first packet. No reason to look at an empty screen

}

void AReplayPlayer::StopPlayback() {
	if (!IsPlaybacking) {
		return;
	}
	UnmapViewOfFile(data);
	CloseHandle(mapping);
	CloseHandle(file);
	IsPlaybacking = false;
	//POSIX
	//munmap(file, size);
}

void AReplayPlayer::ResetPlayback() {
	Speed = 1.0;
	Direction = EPlaybackDirection::PD_Forward;
	Time = 0.0f;
	offset = 0;
	Paused = false;
	Vehicles.Empty();
	for (TActorIterator<AMockingVehicle> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		(*ObjIt)->Destroy();
	}

	msgpack_object* obj = NextPacket();

	// Version 1 of the replay type started with an array. Version two and further starts
	// with the version number.
	if (obj->type != MSGPACK_OBJECT_ARRAY) {
		check(obj->type == MSGPACK_OBJECT_POSITIVE_INTEGER);
		ReplayVersion = obj->via.u64;
	}
	else {
		offset = 0;
		ReplayVersion = 1;
	}

	// Resetting offset history
	while (OffsetHistory != NULL) {
		EOffsetHistory* LastOffset = OffsetHistory;
		OffsetHistory = LastOffset->Previous;
		free(LastOffset);
	}

	msgpack_object* p = PeekNextPacket()->via.array.ptr;
	float PacketTime = p[0].via.f64;

	StartTime = PacketTime;
	Time = PacketTime;
}

void AReplayPlayer::TogglePause() {
	Speed = 1.0;
	Paused = !Paused;
	if (Paused) {
		Stats->SetTimePaused(CurrentTime - StartTime);
	}
}

void AReplayPlayer::SetDirection(EPlaybackDirection NewDirection) {
	if (Paused) {
		Speed = 1.0;
		Paused = false;
		Direction = NewDirection;
	} else if (Direction == NewDirection) {
		Speed *= 1.5;
	}
	else {
		Speed /= 1.5;
		if (Speed <= 0.01) {
			Direction = NewDirection;
			Speed = 1.0;
		}
	}
}

void AReplayPlayer::Step(EPlaybackDirection StepDirection) {
	Direction = StepDirection;
	Paused = true;

	msgpack_object* obj = NULL;
	while (true) {
		if (Direction == EPlaybackDirection::PD_Forward) {
			obj = NextPacket();
		}
		else{
			obj = PrevPacket();
		}

		if (!obj) {
			Paused = true;
			Time = CurrentTime;
			Stats->SetTimePaused(CurrentTime - StartTime);
			return;
		}
		if (!(obj->type == MSGPACK_OBJECT_ARRAY)) {
			continue;
		}
		check(obj->type == MSGPACK_OBJECT_ARRAY);
		ProcessPacket(obj);

		Time = CurrentTime;

		msgpack_object* p = obj->via.array.ptr;
		const uint8 Type = p[1].via.u64;

		if (Type == TICK) {
			break;
		}
	}

	Stats->SetTimePaused(CurrentTime - StartTime);

	UpdateActors();
}

void AReplayPlayer::SeekToIncident(EPlaybackDirection NewDirection) {
	Direction = NewDirection;
	Paused = true;

	msgpack_object* obj = NULL;
	while (true) {
		if (Direction == EPlaybackDirection::PD_Forward) {
			obj = NextPacket();
		}
		else{
			obj = PrevPacket();
		}

		if (!obj) {
			Paused = true;
			Time = CurrentTime;
			Stats->SetTimePaused(CurrentTime - StartTime);
			return;
		}
		if (!(obj->type == MSGPACK_OBJECT_ARRAY)) {
			continue;
		}
		check(obj->type == MSGPACK_OBJECT_ARRAY);

		ProcessPacket(obj);

		msgpack_object* p = obj->via.array.ptr;
		const uint8 Type = p[1].via.u64;
		Time = CurrentTime;

		if (Type == VEHICLES_CRASHED) {
			break;
		}
	}

	Stats->SetTimePaused(CurrentTime - StartTime);

	UpdateActors();

}

void AReplayPlayer::ToggleShowArrows() {
	ShowArrows = !ShowArrows;

	for (TActorIterator<AMockingVehicle> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		AMockingVehicle* Vehicle = *ObjIt;

		Vehicle->SetFlockingState(Vehicle->GetFlockingState(), ShowArrows);
	}
}

void AReplayPlayer::SetShowArrows(bool AShowArrows) {
	ShowArrows = AShowArrows;

	for (TActorIterator<AMockingVehicle> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		AMockingVehicle* Vehicle = *ObjIt;

		Vehicle->SetFlockingState(Vehicle->GetFlockingState(), ShowArrows);
	}
}

void AReplayPlayer::SetShowArrowsBehaviors(TArray<FString> BehaviorNames) {
	ShowArrowBehaviors.Empty(BehaviorNames.Num());

	for (TActorIterator<ABehaviours> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		ABehaviours* Behaviors = *ObjIt;
		for (auto Iter = Behaviors->Behaviours.CreateConstIterator(); Iter; ++Iter) {
			const UBehaviour* Behavior = *Iter;
			FString Name = Behavior->GetClass()->GetName();
			if (BehaviorNames.Contains(Behavior->GetClass()->GetName())) {
				ShowArrowBehaviors.Add(Behavior);
			}
		}
	}

	for (TActorIterator<AMockingVehicle> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		AMockingVehicle* Vehicle = *ObjIt;

		Vehicle->SetFlockingState(Vehicle->GetFlockingState(), ShowArrows);
	}
}

float AReplayPlayer::GetTime() {
	return Time;
}

void AReplayPlayer::SeekAndPause(float Seconds) {
	if (Seconds > Time) {
		Direction = EPlaybackDirection::PD_Forward;
	}
	else {
		Direction = EPlaybackDirection::PD_Backward;
	}
	Paused = true;

	Time = Seconds;

	msgpack_object* obj = NULL;
	while (true) {
		if (Direction == EPlaybackDirection::PD_Forward) {
			if (Time < NextTime) {
				break;
			}
		}
		else {
			if (Time > NextTime) {
				break;
			}
		}
		if (Direction == EPlaybackDirection::PD_Forward) {
			obj = NextPacket();
		}
		else{
			obj = PrevPacket();
		}

		if (!obj) {
			Paused = true;
			Time = CurrentTime;
			Stats->SetTimePaused(CurrentTime - StartTime);
			return;
		}
		if (!(obj->type == MSGPACK_OBJECT_ARRAY)) {
			continue;
		}
		check(obj->type == MSGPACK_OBJECT_ARRAY);
		ProcessPacket(obj);
	}

	Direction = EPlaybackDirection::PD_Forward;

	Stats->SetTimePaused(CurrentTime - StartTime);

	UpdateActors();
}

void AReplayPlayer::SetSpeed(float NewSpeed) {
	Speed = NewSpeed;
}

void AReplayPlayer::SetPaused(bool NewPaused) {
	Paused = NewPaused;
}

void AReplayPlayer::SetUseVehicleCamera(uint32 VehicleId, FString CameraName) {
	VehicleCameraId = VehicleId;
	VehicleCameraName = CameraName;

	if (Vehicles.Contains(VehicleId)) {
		const EVehicleData* VehicleData = &(Vehicles[VehicleId]);
		if (VehicleData->FirstTick) {
			return;
		}
		AMockingVehicle* Vehicle = VehicleData->Vehicle;
		if (Vehicle) {
			SelectVehicleCamera(Vehicle, VehicleCameraName);
		}
	}
}

void AReplayPlayer::SetCameraLookAtVehicle(uint32 VehicleId) {
	LookAtUniqueId = VehicleId;
	if (Vehicles.Contains(VehicleId)) {
		AMockingVehicle* Vehicle = Vehicles[VehicleId].Vehicle;
		if (Vehicle) {
			LookAtActor = Vehicle;
		}
	}
}

void AReplayPlayer::SelectVehicleCamera(AMockingVehicle* Vehicle, FString CameraName) {
	APlayerController* PlayerController = NULL;
	for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator) {
		PlayerController = *Iterator;
		break;
	}

	if (!PlayerController) {
		return;
	}

	PlayerController->Possess(Vehicle);

	for (auto Iter = Vehicle->GetComponents().CreateConstIterator(); Iter; Iter++) {
		UActorComponent* Component = *Iter;
		if (!Component) {
			continue;
		}
		UVehicleCameraComponent* CameraComponent = Cast<UVehicleCameraComponent>(Component);
		if (!CameraComponent) {
			continue;
		}

		/*FActorSpawnParameters spawnParameters;
		spawnParameters.bNoCollisionFail = true;
		spawnParameters.Owner = this;
		spawnParameters.Instigator = NULL;
		spawnParameters.bDeferConstruction = false;*/

		if (CameraComponent->Name == CameraName) {
			/*AInertiaCamera* Camera = GetWorld()->SpawnActor<AInertiaCamera>(
				Vehicle->GetActorLocation(),
				CameraComponent->GetComponentRotation(),
				spawnParameters);
			Camera->FollowComponent = CameraComponent;
			Camera->FollowActor = Vehicle;
			PlayerController->SetViewTarget(Camera);*/
			CameraComponent->Activate();
		}
		else {
			CameraComponent->bAutoActivate = false;
			CameraComponent->Deactivate();
		}
	}
}
