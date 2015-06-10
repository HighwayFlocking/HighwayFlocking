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

#include "msgpack.h"

#include "FlockingVehicle.h"

#include "Recorder.generated.h"

UCLASS()
class HIGHWAYFLOCKING_API ARecorder : public AActor {
	GENERATED_UCLASS_BODY()

private:
	bool IsRecording;
	msgpack_sbuffer sbuf;
	msgpack_packer pk;
	FILE* file;

	void FlushToDisk();

	UPROPERTY()
	TArray<const AFlockingVehicle*> Vehicles;

	UPROPERTY()
	TArray<const AFlockingVehicle*> NewVehicles;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float Delta) override;
	void StartRecording(FString Filename);
	void StopRecording();

	void VehicleSpawned(AFlockingVehicle* Vehicle);
	void VehicleDestroyed(AFlockingVehicle* Vehicle);
	void VehicleCrashed(AFlockingVehicle* MainActor, AActor* OtherActor);
};
