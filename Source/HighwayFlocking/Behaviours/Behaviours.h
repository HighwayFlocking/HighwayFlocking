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
#include "Behaviour.h"
#include "Behaviours.generated.h"

USTRUCT()
struct FPIDGains {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	float ProportionalGain;

	UPROPERTY(EditAnywhere)
	float IntegralGain;

	UPROPERTY(EditAnywhere)
	float DerivativeGain;

	UPROPERTY(EditAnywhere)
	float Limit;
};


/**
 *
 */
UCLASS()
class HIGHWAYFLOCKING_API ABehaviours : public AActor
{
	GENERATED_UCLASS_BODY()

public:

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = Behaviours)
	TArray<UBehaviour*> Behaviours;

	UPROPERTY(EditAnywhere, Category = PIDControllers)
	FPIDGains ThrottleGains;

	UPROPERTY(EditAnywhere, Category = PIDControllers)
	FPIDGains SteeringGains;

	UPROPERTY(EditAnywhere)
	EFlockingState FlockingState;

};
