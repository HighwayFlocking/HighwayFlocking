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

#include "Utils.generated.h"

inline float map_value(float value, float istart, float istop, float ostart, float ostop) {
	return ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
}

UENUM(BlueprintType)
enum class EFlockingState : uint8 {
	FS_Default		UMETA(DisplayName = "Default"),
	FS_Onramp		UMETA(DisplayName = "On Ramp"),
	FS_OnrampWait	UMETA(DisplayName = "On Ramp Waiting"),
	FS_NoChange,
};

typedef struct {
	int32 StationID;
	FVector Location;
	FVector ForwardVector;
	float speed; // cm/s
	// This is the only field not part of the ETSI standard. We only use this in OnRampWaiting.
	float GoalSpeed;
	float timestamp;
	// MaxSpeed is only used when the CamPacket is originating from the current vehile, meaning that even though it is
	// not in the ETSI standard, it is not needed.
	float MaxSpeed; // cm/s
	float Width;
	float Length;
	uint32 PriorityLevel;

	// Not part of the ETSI standard. This is purely an optimization, this is calculated directly from the Location.
	float RoadmapTValue;
	// Not part of the ETSI standard, but only used when originating from the current vehicle. This is only used in the OnRampWaiting
	// behavior to make sure the vehicle does not start before it have lived long enough to have received CAM packages from all neighbors
	float CreationTime;
} FCamPacket;

UENUM(BlueprintType)
enum class EVehicleType : uint8 {
	VT_Car = 0 UMETA(DisplayName = "Car"),
	VT_Sedan = 1 UMETA(DisplayName = "Sedan"),
	VT_Bus = 2 UMETA(DisplayName = "Bus"),
	VT_Emergency = 3 UMETA(DisplayName = "Emergency"),
};

FVector ShortestVectorBetween(FCamPacket Vehicle1, FCamPacket Vehicle2, const float Time, const UWorld* world);
FVector ShortestVectorBetweenLineAndVehicle(FVector LineA, FVector LineB, FCamPacket Vehicle, const float Time, const UWorld* world);
bool VehiclesOverlap(FCamPacket Vehicle1, FCamPacket Vehicle2, const float Time, const UWorld* world, const float ExtraFront, const float ExtraSide);
FVector ShortestVectorFromFront(FCamPacket Vehicle1, FCamPacket Vehicle2, const float Time, const UWorld* world);
