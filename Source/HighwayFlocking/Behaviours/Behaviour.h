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

#include "../Utils.h"
#include "../Roadmap.h"

#include "Behaviour.generated.h" // Last


/**
 *
 */
UCLASS(abstract, EditInlineNew)
class HIGHWAYFLOCKING_API UBehaviour : public UObject {
	GENERATED_UCLASS_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Variables)
	float Weight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Variables)
	int32 Priority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Variables)
	bool Active;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Variables)
	FColor DebugColor;

	/**
	If true and this behaviour activates (returns anything other than up vector, no behaviours
	in lower priority levels will be queried.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Variables)
	bool StopLowerPriorities;

	FVector GetDesiredVelocity(
			const UWorld* world,
			const FCamPacket& vehicle,
			const TArray<FCamPacket>& Neighbors,
			ARoadmap* Roadmap,
			EFlockingState& NextFlockingState) {
		NextFlockingState = EFlockingState::FS_NoChange;
		if (Active) {
			return ProtectedGetDesiredVelocity(world, vehicle, Neighbors, Roadmap, NextFlockingState);
		}
		else {
			return FVector::UpVector;
		}
	};

protected:

	/**
	* Get the vector relative to world that this Behaviour want for the Desired Velocity.
	* If this behaviour do not care about the Desired Velocity, return FVector::UpVector.
	*/
	virtual FVector ProtectedGetDesiredVelocity(
			const UWorld* world,
			const FCamPacket& vehicle,
			const TArray<FCamPacket>& Neighbors,
			ARoadmap* Roadmap,
			EFlockingState& NextFlockingState) {
		return FVector::UpVector;
	};

};

inline bool operator==(const UBehaviour& lhs, const UBehaviour& rhs){ return lhs.Priority == rhs.Priority; }
inline bool operator!=(const UBehaviour& lhs, const UBehaviour& rhs){ return !operator==(lhs, rhs); }
inline bool operator< (const UBehaviour& lhs, const UBehaviour& rhs){ return lhs.Priority < rhs.Priority; }
inline bool operator> (const UBehaviour& lhs, const UBehaviour& rhs){ return  operator< (rhs, lhs); }
inline bool operator<=(const UBehaviour& lhs, const UBehaviour& rhs){ return !operator> (lhs, rhs); }
inline bool operator>=(const UBehaviour& lhs, const UBehaviour& rhs){ return !operator< (lhs, rhs); }
