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
#include "Roadmap.h"
#include "Utils.h"
#include "Behaviours/Behaviours.h"
#include "PIDController.h"
#include "CamComponent.h"
#include "Containers/Map.h"

#include "FlockingVehicle.generated.h"

/**
*
*/
UCLASS()
class HIGHWAYFLOCKING_API AFlockingVehicle : public AWheeledVehicle
{
	GENERATED_UCLASS_BODY()

private:
	ARoadmap* Roadmap;
	TMap<uint8, ABehaviours*> BehaviorsInState;

	float timeSinceBroadcast;

	void Steer(FVector DesiredVelocity, float Delta);

	FPIDController* ThrottleController;
	FPIDController* SteeringController;

	float LastGearSwitchTime;

	void AutoGearChange(float GoalSpeed);

	EFlockingState FlockingState;

public:
	UPROPERTY(BlueprintReadWrite)
		bool DebugEnabled;

	TArray<FCamPacket> Neighbors;

	UPROPERTY()
		TArray<UArrowComponent*> ArrowComponents;

	TMap<UBehaviour*, UArrowComponent*> ArrowComponentForBehaviour;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
		UCamComponent* CamComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float MaxSpeed;

	UPROPERTY(BlueprintReadOnly)
		float GoalSpeed;

	UPROPERTY(BlueprintReadOnly)
		float CurrentSpeed;

	UPROPERTY(EditAnywhere)
	EFlockingState StartFlockingState;

	UPROPERTY(EditAnywhere)
	EVehicleType VehicleType;

	uint8 ColorMaterialIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "FlockingVehicle")
		void SetFlockingState(EFlockingState NFlockingState);

	UFUNCTION(BlueprintCallable, Category = "FlockingVehicle")
	EFlockingState GetFlockingState() const;

	virtual void Tick(float Delta) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void UpdatePhysicsMaterial();
	void ReceiveCamPacket(FCamPacket ECamPacket);

	UFUNCTION(BlueprintImplementableEvent, meta = (FriendlyName = "After Tick ~ After Tick"))
		virtual void AfterTick();

	float ThrottleOutput;

	TArray<FVector> BehaviorVectors;


};
