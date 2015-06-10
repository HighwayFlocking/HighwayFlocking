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

#include "../HighwayFlocking.h"
#include "OnRampWaiting.h"
#include "../Utils.h"

UOnRampWaiting::UOnRampWaiting(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {

}

FVector UOnRampWaiting::ProtectedGetDesiredVelocity(
		const UWorld* world,
		const FCamPacket& vehicle,
		const TArray<FCamPacket>& Neighbors,
		ARoadmap* Roadmap,
		EFlockingState& NextFlockingState){

	if (world == NULL) {
		return FVector::UpVector;
	}

	if (MeetingPoint == NULL) {
		return FVector::UpVector;
	}

	if (world->GetTimeSeconds() - vehicle.CreationTime < 0.5f) {
		NextFlockingState = EFlockingState::FS_NoChange;
		return FVector::UpVector;
	}

	int32 len = Neighbors.Num();

	float ts = world->GetTimeSeconds();

	float MyTimeAtMeetingPoint = ts + OnRampTimeToMeetingPoint;

	// TODO: Do not count things in that has moved past the point, or that are moving in the opposite direction.

	for (int32 i = 0; i < len; i++) {
		FCamPacket neighbor = Neighbors[i];
		if (neighbor.PriorityLevel <= vehicle.PriorityLevel) {
			continue;
		}

		FVector NToMeetingPoint = (MeetingPoint->GetActorLocation() - neighbor.Location).GetSafeNormal2D();
		FVector LocationAtMeetingTime = neighbor.Location + NToMeetingPoint * (MyTimeAtMeetingPoint - neighbor.timestamp) * neighbor.GoalSpeed;

		if (abs(neighbor.GoalSpeed - neighbor.speed) / neighbor.GoalSpeed > 0.10) {
			// If the bus is running much lower than its goal speed, it means that something has made it to break.
			// This makes it diffucult for us to compute its time at the goal, so we just wait it out
			NextFlockingState = EFlockingState::FS_NoChange;
			return FVector::UpVector;
		}

		//DrawDebugBox(world, LocationAtMeetingTime, FVector(neighbor.Length, neighbor.Width, 300), FColor::Red);

		float DistanceToMeetingPoint = (MeetingPoint->GetActorLocation() - neighbor.Location).Size2D();
		float TimeToMeetingPointStart = (DistanceToMeetingPoint - neighbor.Length/2 - MarginInFront) / neighbor.GoalSpeed;
		float TimeToMeetingPointEnd = (DistanceToMeetingPoint + neighbor.Length + MarginBehind) / neighbor.GoalSpeed;
		float TimeAtMeetingPointStart = neighbor.timestamp + TimeToMeetingPointStart;
		float TimeAtMeetingPointEnd = neighbor.timestamp + TimeToMeetingPointEnd;

		//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::White, FString::Printf(TEXT("MyTime: %.2f, TimeStart: %.2f, TimeEnd: %.2f"), MyTimeAtMeetingPoint, TimeAtMeetingPointStart, TimeAtMeetingPointEnd));

		if (!(MyTimeAtMeetingPoint > TimeAtMeetingPointEnd || MyTimeAtMeetingPoint < TimeAtMeetingPointStart)) {
			NextFlockingState = EFlockingState::FS_NoChange;
			return FVector::UpVector;
		}
	}

	NextFlockingState = EFlockingState::FS_Onramp;
	return FVector::UpVector;
}
