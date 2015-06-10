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

#include "GameFramework/Pawn.h"
#include "Behaviours/Behaviours.h"

#include "MockingVehicle.generated.h"

UCLASS()
class HIGHWAYFLOCKING_API AMockingVehicle : public APawn {
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY(Category = MockingVehicle, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Mesh;

	TMap<uint8, ABehaviours*> BehaviorsInState;

	UPROPERTY()
		TArray<UArrowComponent*> ArrowComponents;
	TMap<UBehaviour*, UArrowComponent*> ArrowComponentForBehaviour;
	EFlockingState FlockingState;

public:
	class USkeletalMeshComponent* GetMesh() const;

	void BeginPlay();

	TArray<FVector> BehaviorVectors;
	void SetFlockingState(EFlockingState NewFlockingState, bool ShowArrows);

	EFlockingState GetFlockingState();

	void UpdateArrows();

	UPROPERTY(BlueprintReadOnly)
	float WheelRotationAngle;

	UPROPERTY(BlueprintReadOnly)
	float WheelSteerAngle;

	UPROPERTY(BlueprintReadOnly)
	float SuspensionOffset;

	UPROPERTY(BlueprintReadOnly)
	int32 ReplayUniqueId;

	TSet<const UBehaviour*>* ShowArrowBehaviors;

	EVehicleType VehicleType;
};
