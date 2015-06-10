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
#include "Engine/TriggerBase.h"

#include "MeasurementGate.generated.h"

#define NUM_SECONDS_STATS 20

/**
*
*/
UCLASS()
class HIGHWAYFLOCKING_API AMeasurementGate : public ATriggerBase {
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// Begin AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	// End AActor interface.
#endif

private:
	uint64 LastMeasurementTime;
	uint32 NumVehiclesDestroyedSecond[NUM_SECONDS_STATS][2];


public:
	UFUNCTION()
	void OnBeginOverlap(AActor* OtherActor);

	float GetThroughput(uint8 Lane);

};
