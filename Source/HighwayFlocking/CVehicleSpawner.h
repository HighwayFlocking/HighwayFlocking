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
#include "utils.h"
#include "FlockingVehicle.h"

#include "CVehicleSpawner.generated.h"

UENUM(BlueprintType)
enum class EDirection : uint8 {
	D_ToCity=0 UMETA(DisplayName = "To City"),
	D_FromCity=2 UMETA(DisplayName = "From City"),
};

USTRUCT()
struct FVehicleTypeStruct {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	EVehicleType VehicleType;

	UPROPERTY(EditAnywhere)
	int32 Count;

	FVehicleTypeStruct() {
		VehicleType = EVehicleType::VT_Car;
		Count = 0;
	}

	FVehicleTypeStruct(EVehicleType NVehicleType, int32 NCount) {
		VehicleType = NVehicleType;
		Count = NCount;
	}
};

/**
*
*/
UCLASS()
class HIGHWAYFLOCKING_API ACVehicleSpawner : public AActor
{
	GENERATED_UCLASS_BODY()

private:
	TMap<uint8, TSubclassOf<class AFlockingVehicle>> VehicleTypeClass;
	TMap<uint8, TArray<UMaterial*>> VehicleTypeMaterials;

	TArray<EVehicleType> Bucket;

	float GetTimeWait();

public:
	UPROPERTY(EditAnywhere, BluePrintReadWrite)
		bool Active;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
		bool Paused;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
		EFlockingState FlockingState;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
		float StartSpeed;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
		float MinTimeWait;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
		float MaxTimeWait;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
		EDirection Direction;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
		int32 Lane;

	UPROPERTY(EditAnywhere)
		TArray<FVehicleTypeStruct> VehicleTypes;

	virtual void OnTimer();
	virtual void BeginPlay() override;
	void TurnBucket();

};
