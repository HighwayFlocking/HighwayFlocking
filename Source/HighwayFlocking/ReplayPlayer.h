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

//#include "FlockingVehicle.h"
#include "MockingVehicle.h"
#include "Vehiclestats.h"
#include "Behaviours/Behaviour.h"

#include "ReplayPlayer.generated.h"

UENUM(BlueprintType)
enum class EPlaybackDirection : uint8 {
	PD_Forward		UMETA(DisplayName = "Forward"),
	PD_Backward		UMETA(DisplayName = "Backward"),
};

typedef struct {
	bool New;    // Means this is the first packet for a vehicle
	bool Exists; // Means there exists a vehicle with this uid
	FVector Location;
	FRotator Rotation;
	FVector Velocity;
	float LastUpdateTime;
	uint8 ColorMaterialIndex;
	EVehicleType VehicleType;
	EFlockingState FlockingState;
	TArray<FVector> BehaviorVectors;
	float WheelRotationAngle;
	float WheelRotationSpeed;
	float WheelSteerAngle;
	float SuspensionOffset;
} EVehicleDataPoint;

typedef struct {
	AMockingVehicle* Vehicle; // The object representing the last created vehicle with that id
	bool FirstTick;
	EVehicleDataPoint Current;
	EVehicleDataPoint Next;
} EVehicleData;



class EOffsetHistory {
public:
	size_t Offset;
	EOffsetHistory* Previous;
};


UCLASS()
class HIGHWAYFLOCKING_API AReplayPlayer : public AActor {
	GENERATED_UCLASS_BODY()

private:
	bool Paused;
	float Speed;
	bool ShowArrows;

	EPlaybackDirection Direction;
	bool IsPlaybacking;

	TMap<uint32, EVehicleData> Vehicles;

	HANDLE file;
	HANDLE mapping;
	const char* data;
	size_t size;

	float Time;
	float CurrentTime;
	float NextTime;
	float StartTime;
	bool ForceTime;

	TSet<const UBehaviour*> ShowArrowBehaviors;

	msgpack_object* current_object;
	msgpack_zone mempool;
	//The object that current_object can be a pointer to.
	msgpack_object tmp_object;
	size_t offset;

	msgpack_object* NextPacket();
	msgpack_object* PeekNextPacket();
	msgpack_object* PrevPacket();
	msgpack_object* PacketAtOffset(size_t TmpOffset);

	TMap<uint8, TSubclassOf<class AMockingVehicle>> VehicleTypeClass;
	TMap<uint8, TArray<UMaterial*>> VehicleTypeMaterials;

	TSubclassOf<class AActor> AArrow;

	// Linked List to save the offsets of previous packets, to be able to reverse
	EOffsetHistory* OffsetHistory;

	AVehiclestats* Stats;

	void ProcessPacket(msgpack_object* obj);
	void UpdateActors();

	int ReplayVersion;

	EVehicleData* GetOrCreateVehicleData(uint32 UniqueId);

	void SelectVehicleCamera(AMockingVehicle* Vehicle, FString CameraName);

	uint32 LookAtUniqueId;
	uint32 VehicleCameraId;

	FString VehicleCameraName;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float Delta) override;
	void StartPlayback(FString Filename);
	void StopPlayback();

	float GetTime();
	void SetSpeed(float NewSpeed);

	UFUNCTION(BlueprintCallable, Category = ReplayPlayer)
	void ToggleShowArrows();

	UFUNCTION(BlueprintCallable, Category = ReplayPlayer)
	void SetShowArrows(bool AShowArrows);

	void SetShowArrowsBehaviors(TArray<FString> BehaviorNames);

	UFUNCTION(BlueprintCallable, Category = ReplayPlayer)
	void TogglePause();

	void SetPaused(bool NewPaused);

	UFUNCTION(BlueprintCallable, Category = ReplayPlayer)
	void SetDirection(EPlaybackDirection NewDirection);

	UFUNCTION(BlueprintCallable, Category = ReplayPlayer)
	void ResetPlayback();

	UFUNCTION(BlueprintCallable, Category = ReplayPlayer)
	void SeekToIncident(EPlaybackDirection NewDirection);

	UFUNCTION(BlueprintCallable, Category = ReplayPlayer)
	void Step(EPlaybackDirection StepDirection);

	void SetCameraLookAtVehicle(uint32 VehicleId);
	void SetUseVehicleCamera(uint32 VehicleId, FString CameraName);

	void SeekAndPause(float Seconds);

	UPROPERTY(BlueprintReadOnly)
	AActor* LookAtActor;

};
