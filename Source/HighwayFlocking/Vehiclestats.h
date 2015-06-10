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

#pragma once

#include "GameFramework/Actor.h"

#include "Utils.h"
#include "Recorder.h"

#include "Vehiclestats.generated.h"

#define NUM_SECONDS_STATS 20

typedef struct {
	uint32 Id;
	float CreationTime;
	FVector Location;
	FVector Velocity;
	FRotator Rotation;
	float GoalSpeed;
	float MaxSpeed;
	EVehicleType VehicleType;
	uint32 PriorityLevel;
	int8 Gear;
	float EngineRotationSpeed;
	uint8 ColorMaterialIndex;
	EFlockingState FlockingState;
	float ThrottleOutput;
	TArray<FVector> BehaviorVectors;
	TArray<uint32> Neighbors;
} FVehicleLogEntry;

typedef struct {
	uint32 Vehicle1Id;
	uint32 Actor2Id;
	float Time;
	TArray<FVehicleLogEntry> Vehicles;
} FIncidentLogEntry;

// TODO: Also log vehicles spawned and finished

UCLASS()
class AVehiclestats : public AActor {
	GENERATED_UCLASS_BODY()

private:

	float ThrougputCache;
	bool ThroughputCacheInvalidated;
	uint64 ThrouhputCacheTime;
	uint64 LastDestroyedTime;
	uint32 NumVehiclesDestroyedSecond[NUM_SECONDS_STATS];
	uint32 NumVehiclesSpawned;
	uint32 NumVehiclesDestroyed;
	uint32 NumVehiclesCrashed;
	uint32 NumIncidents;

	float StartTime;
	float PausedCurrentTime;

	TArray<FVehicleLogEntry> GetAllVehiclesLogEntry();

	ARecorder* Recorder;

public:
	TArray<FIncidentLogEntry> IncidentLog;

	virtual void BeginPlay() override;



	UFUNCTION(BlueprintCallable, Category = Statestics)
		void VehicleSpawned(AFlockingVehicle* Vehicle);

	UFUNCTION(BlueprintCallable, Category = Statestics)
		void VehicleDestroyed(AFlockingVehicle* Vehicle);

	UFUNCTION(BlueprintCallable, Category = Statestics)
		void VehicleCrashed(AFlockingVehicle* MainActor, AActor* OtherActor);

	uint32 GetNumVehiclesSpawned();
	uint32 GetNumVehiclesFinished();
	uint32 GetNumVehiclesOnTheRoad();
	uint32 GetNumVehiclesIncidents();
	uint32 GetNumVehiclesCrashed();
	float GetTime();

	void ResetStats();
	float GetThroughput();

	void SetTime(float NewTime);
	void SetTimePaused(float NewTime);
};
