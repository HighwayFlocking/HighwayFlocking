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
#include "Vehiclestats.h"

#include "FlockingVehicle.h"

AVehiclestats::AVehiclestats(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

TArray<FVehicleLogEntry> AVehiclestats::GetAllVehiclesLogEntry() {
	TArray<FVehicleLogEntry> Result;

	for (TActorIterator<AFlockingVehicle> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		AFlockingVehicle* Vehicle = *ObjIt;
		FVehicleLogEntry Entry;

		UWheeledVehicleMovementComponent* movementComp = Vehicle->GetVehicleMovementComponent();

		Entry.Id = Vehicle->GetUniqueID();
		Entry.CreationTime = Vehicle->CreationTime;
		Entry.Location = Vehicle->GetActorLocation();
		Entry.Velocity = Vehicle->GetVelocity();
		Entry.Rotation = Vehicle->GetActorRotation();
		Entry.GoalSpeed = Vehicle->GoalSpeed;
		Entry.MaxSpeed = Vehicle->MaxSpeed;
		Entry.VehicleType = Vehicle->VehicleType;
		Entry.PriorityLevel = Vehicle->CamComponent->PriorityLevel;
		Entry.Gear = movementComp->GetCurrentGear();
		Entry.EngineRotationSpeed = movementComp->GetEngineRotationSpeed();
		Entry.ColorMaterialIndex = Vehicle->ColorMaterialIndex;
		Entry.FlockingState = Vehicle->GetFlockingState();
		Entry.ThrottleOutput = Vehicle->ThrottleOutput;
		Entry.BehaviorVectors = TArray<FVector>(Vehicle->BehaviorVectors);
		for (auto Iter = Vehicle->Neighbors.CreateConstIterator(); Iter; ++Iter) {
			Entry.Neighbors.Add(Iter->StationID);
		}

		Result.Add(Entry);
	}
	return Result;
}

void AVehiclestats::BeginPlay() {
	ResetStats();

	for (TActorIterator<ARecorder> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		Recorder = *ObjIt;
		break;
	}
}

void AVehiclestats::VehicleSpawned(AFlockingVehicle* Vehicle) {
	if (Recorder) {
		Recorder->VehicleSpawned(Vehicle);
	}
	NumVehiclesSpawned += 1;
}

void AVehiclestats::VehicleDestroyed(AFlockingVehicle* Vehicle) {
	if (Recorder) {
		Recorder->VehicleDestroyed(Vehicle);
	}
	// We are just interested in the whole seconds
	uint64 ts = (int)GetWorld()->GetTimeSeconds();

	if (ts != LastDestroyedTime) {
		for (int i = LastDestroyedTime + 1; i <= ts; i++) {
			NumVehiclesDestroyedSecond[i % NUM_SECONDS_STATS] = 0;
		}
	}

	NumVehiclesDestroyedSecond[ts % NUM_SECONDS_STATS] += 1;
	LastDestroyedTime = ts;

	NumVehiclesDestroyed += 1;
	ThroughputCacheInvalidated = true;
}

uint32 AVehiclestats::GetNumVehiclesSpawned() {
	return NumVehiclesSpawned;
}

uint32 AVehiclestats::GetNumVehiclesFinished() {
	return NumVehiclesDestroyed - NumVehiclesCrashed;
}

float AVehiclestats::GetThroughput() {
	uint64 ts = (int)GetWorld()->GetTimeSeconds();
	if (!ThroughputCacheInvalidated && ts == ThrouhputCacheTime) {
		return ThrougputCache;
	}

	if (ts != LastDestroyedTime) {
		for (int i = LastDestroyedTime + 1; i <= ts; i++) {
			NumVehiclesDestroyedSecond[i % NUM_SECONDS_STATS] = 0;
		}
	}
	LastDestroyedTime = ts;

	ThrougputCache = 0.0f;

	for (uint32 i = 0; i < NUM_SECONDS_STATS; i++) {
		ThrougputCache += (float)NumVehiclesDestroyedSecond[i];
	}
	ThrougputCache = ThrougputCache * (60.0f / (float)NUM_SECONDS_STATS) * 60.0f; // vehicles per hour
	ThroughputCacheInvalidated = false;
	ThrouhputCacheTime = ts;

	return ThrougputCache;
}

uint32 AVehiclestats::GetNumVehiclesOnTheRoad() {
	return NumVehiclesSpawned - NumVehiclesDestroyed;
}

void AVehiclestats::VehicleCrashed(AFlockingVehicle* MainActor, AActor* OtherActor) {
	if (Recorder) {
		Recorder->VehicleCrashed(MainActor, OtherActor);
	}

	//FIncidentLogEntry LogEntry;

	//LogEntry.Vehicle1Id = MainActor->GetUniqueID();
	//LogEntry.Actor2Id = OtherActor->GetUniqueID();
	//LogEntry.Time = GetWorld()->GetTimeSeconds();
	//LogEntry.Vehicles = GetAllVehiclesLogEntry();
	//IncidentLog.Add(LogEntry);


	AFlockingVehicle* OtherVehicle = Cast<AFlockingVehicle>(OtherActor);

	if (OtherVehicle) {
		NumIncidents += 1;
		NumVehiclesCrashed += 2;
	}
}

uint32 AVehiclestats::GetNumVehiclesIncidents() {
	return NumIncidents;
}

uint32 AVehiclestats::GetNumVehiclesCrashed() {
	return NumVehiclesCrashed;
}

void AVehiclestats::SetTime(float NewTime) {
	PausedCurrentTime = -1;
	StartTime = GetWorld()->GetTimeSeconds() - NewTime;
}

void AVehiclestats::SetTimePaused(float NewTime) {
	PausedCurrentTime = NewTime;
}

float AVehiclestats::GetTime() {
	if (PausedCurrentTime > 0) {
		return PausedCurrentTime;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	return CurrentTime - StartTime;
}

void AVehiclestats::ResetStats() {
	PausedCurrentTime = -1;
	NumIncidents = 0;
	NumVehiclesSpawned = 0;
	NumVehiclesDestroyed = 0;
	StartTime = GetWorld()->GetTimeSeconds();

	ThrougputCache = 0.0f;
	ThroughputCacheInvalidated = true;
	ThrouhputCacheTime = 0;
	LastDestroyedTime = 0;
	for (uint32 i = 0; i < NUM_SECONDS_STATS; i++) {
		NumVehiclesDestroyedSecond[i] = 0;
	}
	NumVehiclesCrashed = 0;
}
