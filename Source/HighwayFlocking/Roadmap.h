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
#include "Math/InterpCurve.h"
#include "Components/SplineComponent.h"
#include "Utils.h"

#include "Roadmap.generated.h"

/**
 *
 */
UCLASS()
class HIGHWAYFLOCKING_API ARoadmap : public AActor
{
	GENERATED_BODY()

private:
	void CalculateRoadmap();

public:
	ARoadmap(const FObjectInitializer& ObjectInitializer);
	//Spline data for roadmap. t-value to point
	FInterpCurveVector SplineInfo;

	USplineComponent* RampSplineComponent;

	UPROPERTY(EditAnywhere)
	float RoadRadius;

	UPROPERTY(EditAnywhere)
	float FlockRadius;

	UPROPERTY(EditAnywhere)
	float MinDistanceSide;

	UPROPERTY(EditAnywhere)
	float MinDistanceFront;

	UPROPERTY(EditAnywhere)
	float AvoidDistanceSide;

	UPROPERTY(EditAnywhere)
	float AvoidDistanceFront;

	UPROPERTY(EditAnywhere, Category=BehaviourWeights)
	float KeepRoadTangentWeight;

	UPROPERTY(EditAnywhere, Category = BehaviourWeights)
	float KeepSpeedWeight;

	UPROPERTY(EditAnywhere, Category = BehaviourWeights)
	float AlignSpeedWeight;

	UPROPERTY(EditAnywhere, Category = BehaviourWeights)
	float KeepOnRoadWeight;

	UPROPERTY(EditAnywhere, Category = BehaviourWeights)
	float AvoidWeight;

	UPROPERTY(EditAnywhere, Category = BehaviourWeights)
	float CohesionWeight;

	void BeginPlay() override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category=Roadmap)
	FVector GetRoadTangentNearest(FVector Location);

	bool Vehicle2IsOnRight(FCamPacket Vehicle1, FCamPacket Vehicle2);

};
