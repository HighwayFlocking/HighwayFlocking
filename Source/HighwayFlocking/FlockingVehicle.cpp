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

#include "HighwayFlocking.h"
#include "FlockingVehicle.h"
#include "WheelFront.h"
#include "WheelRear.h"
#include "Vehiclestats.h"

using namespace std;

AFlockingVehicle::AFlockingVehicle(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer){
	CamComponent = ObjectInitializer.CreateDefaultSubobject<UCamComponent>(this, "CamComponent");
	CamComponent->AttachParent = RootComponent;

	UWheeledVehicleMovementComponent4W* Vehicle4W = CastChecked<UWheeledVehicleMovementComponent4W>(GetVehicleMovement());

	check(Vehicle4W->WheelSetups.Num() == 4);

	Vehicle4W->WheelSetups[0].BoneName = FName("Wheel_Front_Left");
	Vehicle4W->WheelSetups[0].AdditionalOffset = FVector(0.f, -12.f, 0.f);

	Vehicle4W->WheelSetups[1].BoneName = FName("Wheel_Front_Right");
	Vehicle4W->WheelSetups[1].AdditionalOffset = FVector(0.f, 12.f, 0.f);

	Vehicle4W->WheelSetups[2].BoneName = FName("Wheel_Rear_Left");
	Vehicle4W->WheelSetups[2].AdditionalOffset = FVector(0.f, -12.f, 0.f);

	Vehicle4W->WheelSetups[3].BoneName = FName("Wheel_Rear_Right");
	Vehicle4W->WheelSetups[3].AdditionalOffset = FVector(0.f, 12.f, 0.f);

	StartFlockingState = EFlockingState::FS_Default;
}

void AFlockingVehicle::AutoGearChange(float GoalSpeed) {
	UWheeledVehicleMovementComponent* movementComp = GetVehicleMovementComponent();

	int32 CurrentGear = movementComp->GetCurrentGear();
	int32 NextGear = CurrentGear;

	float TimeSinceSwitch = GetWorld()->GetTimeSeconds() - LastGearSwitchTime;

	if (TimeSinceSwitch < 1) {
		return;
	}

	float Rotations = movementComp->GetEngineRotationSpeed();

	if (Rotations < 5000 && Rotations > 2000) {
		// No gear change, we are in a nice area
		return;
	}

	if (Rotations > 5000) {
		// To high rpm, directly to next gear
		NextGear = CurrentGear + 1;
	}

	if (Rotations < 1000 && CurrentGear != 1) {
		// To low rpm, directly to prev gear
		NextGear = CurrentGear - 1;
	}

	CurrentSpeed = movementComp->GetForwardSpeed();

	float delta = FMath::Abs(CurrentGear - GoalSpeed);

	if (CurrentGear != 1 && Rotations < 1800 && delta > 500) {
		// We need lower gear to accelerate
		NextGear = CurrentGear - 1;
	}

	if (NextGear != CurrentGear) {
		LastGearSwitchTime = GetWorld()->GetTimeSeconds();
		movementComp->SetTargetGear(NextGear, true);
	}
}

void AFlockingVehicle::Steer(FVector DesiredVelocity, float Delta) {
	float MaxSpeedCMs = MaxSpeed / 0.036;
	DesiredVelocity = DesiredVelocity * MaxSpeedCMs;

	UWheeledVehicleMovementComponent* movementComp = GetVehicleMovementComponent();

	//FVector CurrentVelocity = FVector::ForwardVector * speed;

	float CorrectHeading = DesiredVelocity.HeadingAngle();
	float MyHeading = GetActorForwardVector().HeadingAngle();

	if (DesiredVelocity.SizeSquared2D() == 0) {
		CorrectHeading = MyHeading;
	}


	CurrentSpeed = movementComp->GetForwardSpeed();

	float steering = SteeringController->NextValue(MyHeading, CorrectHeading, Delta);

	GoalSpeed = FVector::DotProduct(GetActorForwardVector(), DesiredVelocity);

	// Only 10 km/h when reversing

	GoalSpeed = FMath::Max(GoalSpeed, -10.0f / 0.036f);

	if (GoalSpeed == 0) {
		if (CurrentSpeed == 0) {
			ThrottleOutput = 0.0f;
		}
		else {
			ThrottleOutput = -CurrentSpeed / abs(CurrentSpeed);
		}
	}
	else {
		ThrottleOutput = ThrottleController->NextValue(CurrentSpeed, GoalSpeed, Delta);
		if (!GetMesh()->RigidBodyIsAwake()) {
			GetMesh()->WakeAllRigidBodies();
		}
	}

	movementComp->SetSteeringInput(steering);
	movementComp->SetThrottleInput(ThrottleOutput);

	AutoGearChange(GoalSpeed);
}

void AFlockingVehicle::Tick(float Delta) {
	if (!Roadmap) {
		return;
	}

	if (!BehaviorsInState.Contains((uint8)FlockingState)) {
		return;
	}

	UWorld* world = GetWorld();

	const ABehaviours* Behaviours = BehaviorsInState[(uint8)FlockingState];

	int32 len = Behaviours->Behaviours.Num();

	FVector DesiredVelocity = FVector(0, 0, 0);

	FVector MyLocation = GetActorLocation();
	MyLocation.Z += 200;
	FTransform Transform = ActorToWorld();

	CamComponent->UpdateCamPacket();

	BehaviorVectors.Empty(len);
	int32 CurrentPriorityLevel = 0;
	bool LastPriorityLevel = false;

	EFlockingState NextFlockingState = EFlockingState::FS_NoChange;

	for (int32 i = 0; i < len; i++) {
		UBehaviour* behaviour = Behaviours->Behaviours[i];

		if (LastPriorityLevel && behaviour->Priority > CurrentPriorityLevel) {
			break;
		}
		CurrentPriorityLevel = behaviour->Priority;

		if (behaviour == NULL) {
			BehaviorVectors.Add(FVector::ZeroVector);
			if (ArrowComponentForBehaviour.Contains(behaviour)) {
				UArrowComponent* ArrowComponent = ArrowComponentForBehaviour[behaviour];
				ArrowComponent->SetWorldScale3D(FVector(0.0f, 0.0f, 0.0f));
			}
			continue;
		}
		EFlockingState LocalNextFlockingState = EFlockingState::FS_NoChange;
		FVector BehDesiredVelocity = behaviour->GetDesiredVelocity(
			GetWorld(),
			CamComponent->GetCamPacket(),
			Neighbors,
			Roadmap,
			LocalNextFlockingState);
		if (LocalNextFlockingState != EFlockingState::FS_NoChange && LocalNextFlockingState != FlockingState) {
			NextFlockingState = LocalNextFlockingState;
		}
		if (BehDesiredVelocity == FVector::UpVector) {
			// A vector up means that the behaviour don't care about the velocity
			BehaviorVectors.Add(FVector::ZeroVector);
			if (ArrowComponentForBehaviour.Contains(behaviour)) {
				UArrowComponent* ArrowComponent = ArrowComponentForBehaviour[behaviour];
				ArrowComponent->SetWorldScale3D(FVector(0.0f, 0.0f, 0.0f));
			}
			continue;
		}
		BehDesiredVelocity = BehDesiredVelocity * behaviour->Weight;
		BehaviorVectors.Add(BehDesiredVelocity);
		DesiredVelocity += BehDesiredVelocity;
		if (behaviour->StopLowerPriorities) {
			LastPriorityLevel = true;
		}
		if (BehDesiredVelocity.SizeSquared() > 1.001f) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Too high Desired Velocity on " + behaviour->GetClass()->GetName() + ": " + FString::SanitizeFloat(BehDesiredVelocity.SizeSquared())));
		}

		if (ArrowComponentForBehaviour.Contains(behaviour)) {
			UArrowComponent* ArrowComponent = ArrowComponentForBehaviour[behaviour];
			ArrowComponent->SetWorldRotation(BehDesiredVelocity.Rotation());
			float Size = BehDesiredVelocity.Size();
			ArrowComponent->SetWorldScale3D(FVector(Size, 0.5, 0.5));
		}

		/*FVector VelocityNormal = BehDesiredVelocity.GetSafeNormal2D();
		FVector VelocityRight = FVector::CrossProduct(FVector::UpVector, VelocityNormal).GetSafeNormal2D();
		float VelocityLength = BehDesiredVelocity.Size2D() * 1000;

		FVector End = MyLocation + BehDesiredVelocity * 1000;

		static float ArrayWidth = 10;
		static float ArrayLength = 20;
		static uint32 segments = 10;*/



		//DrawDebugCylinder(GetWorld(), MyLocation, End, 10, segments, behaviour->DebugColor);
		//DrawDebugCylinder(GetWorld(), End, MyLocation + VelocityNormal * (VelocityLength - ArrayLength) + VelocityRight * ArrayWidth, 10, segments, behaviour->DebugColor);
		//DrawDebugCylinder(GetWorld(), End, MyLocation + VelocityNormal * (VelocityLength - ArrayLength) - VelocityRight * ArrayWidth, 10, segments, behaviour->DebugColor);
		//DrawDebugCylinder(GetWorld(), MyLocation + VelocityNormal * (VelocityLength - ArrayLength) + VelocityRight * ArrayWidth, MyLocation + VelocityNormal * (VelocityLength - ArrayLength) - VelocityRight * ArrayWidth, 10, segments, behaviour->DebugColor);
	}

	if (NextFlockingState != EFlockingState::FS_NoChange && NextFlockingState != FlockingState) {
		SetFlockingState(NextFlockingState);
	}

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, KeepOnRoadVector.ToString());

	DesiredVelocity = DesiredVelocity.GetClampedToMaxSize(1.0f);

	Steer(DesiredVelocity, Delta);

	timeSinceBroadcast += Delta;

	if (timeSinceBroadcast > 0.05) {
		timeSinceBroadcast = 0;
		//SendCamPackets();
		float rm_ts = GetWorld()->GetTimeSeconds() - 1.0f;
		Neighbors.RemoveAll([rm_ts](FCamPacket packet) { return packet.timestamp < rm_ts; });
	}

	/*	int32 nlen = Neighbors.Num();
	FVector loc = GetActorLocation() + FVector::UpVector * 500;
	for (int32 i = 0; i < nlen; i++) {
	FCamPacket neighbor = Neighbors[i];

	DrawDebugCylinder(GetWorld(), loc, neighbor.Location + FVector::UpVector * 500, 10, 10, FColor::Yellow);
	}*/

	AfterTick();
}

void AFlockingVehicle::ReceiveCamPacket(FCamPacket CamPacket) {
	int32 len = Neighbors.Num();

	for (int i = 0; i < len; i++) {
		if (Neighbors[i].StationID == CamPacket.StationID) {
			Neighbors.RemoveAtSwap(i, 1, false);
			break;
		}
	}

	Neighbors.Add(CamPacket);
}

void AFlockingVehicle::BeginPlay() {
	for (TActorIterator<AVehiclestats> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		AVehiclestats* Stats = *ObjIt;
		Stats->VehicleSpawned(this);
	}

	Neighbors.Empty(3);

	float MaxSpeedCMs = MaxSpeed / 0.036;
	GoalSpeed = MaxSpeedCMs;


	GetVehicleMovementComponent()->SetHandbrakeInput(false);

	GetVehicleMovementComponent()->SetUseAutoGears(false);

	for (TActorIterator<ARoadmap> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		Roadmap = *ObjIt;
		break;
	}

	if (Roadmap == NULL) {
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("Could not find a roadmap!"));
	}

	for (TActorIterator<ABehaviours> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		ABehaviours* Behaviors = *ObjIt;
		BehaviorsInState.Add((uint8)Behaviors->FlockingState, Behaviors);
	}

	SetFlockingState(StartFlockingState);



	//SendCamPackets();
	timeSinceBroadcast = 0;




	UWheeledVehicleMovementComponent* movementComp = GetVehicleMovementComponent();

	movementComp->SetTargetGear(3, true);
	LastGearSwitchTime = GetWorld()->GetTimeSeconds();

}

EFlockingState AFlockingVehicle::GetFlockingState() const {
	return FlockingState;
}

void AFlockingVehicle::SetFlockingState(EFlockingState NFlockingState) {
	for (int32 i = 0; i < ArrowComponents.Num(); i++) {
		UArrowComponent* ArrowComponent = ArrowComponents[i];
		ArrowComponent->DestroyComponent();
	}

	ArrowComponents.Empty(0);
	ArrowComponentForBehaviour.Empty(0);

	FlockingState = NFlockingState;

	if (!BehaviorsInState.Contains((uint8)FlockingState)) {
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("Could not find a behaviours object!"));
		return;
	}

	ABehaviours* Behaviors = BehaviorsInState[(uint8)FlockingState];

	bool ShowArrows = false;

	if (ShowArrows) {
		if (GetWorld()) {
			ArrowComponents.Empty(Behaviors->Behaviours.Num());
			ArrowComponentForBehaviour.Empty(Behaviors->Behaviours.Num());
			for (int32 i = 0; i < Behaviors->Behaviours.Num(); i++) {
				UBehaviour* behaviour = Behaviors->Behaviours[i];
				UArrowComponent* ArrowComponent = ConstructObject<UArrowComponent>(UArrowComponent::StaticClass(), this);
				ArrowComponent->RegisterComponent();
				ArrowComponent->AttachTo(GetRootComponent(), NAME_None, EAttachLocation::SnapToTarget);
				ArrowComponent->ArrowColor = behaviour->DebugColor;
				if (VehicleType == EVehicleType::VT_Bus)
				{
					ArrowComponent->RelativeLocation = FVector(0, 0, 400);
				}
				else
				{
					ArrowComponent->RelativeLocation = FVector(0, 0, 200);
				}
				ArrowComponent->bHiddenInGame = false;
				ArrowComponent->ArrowSize = 8.0f;
				ArrowComponent->SetWorldScale3D(FVector(0.0, 0.5, 0.5));
				ArrowComponent->SetWorldRotation(FVector(0, 0.0, 1.0).Rotation());

				ArrowComponents.Add(ArrowComponent);
				ArrowComponentForBehaviour.Add(behaviour, ArrowComponent);
			}
		}
	}

	ThrottleController = new FPIDController(
		Behaviors->ThrottleGains.ProportionalGain,
		Behaviors->ThrottleGains.IntegralGain,
		Behaviors->ThrottleGains.DerivativeGain,
		Behaviors->ThrottleGains.Limit);

	SteeringController = new FPIDController(
		Behaviors->SteeringGains.ProportionalGain,
		Behaviors->SteeringGains.IntegralGain,
		Behaviors->SteeringGains.DerivativeGain,
		Behaviors->SteeringGains.Limit);
}

void AFlockingVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	for (TActorIterator<AVehiclestats> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		AVehiclestats* Stats = *ObjIt;
		Stats->VehicleDestroyed(this);
	}
}

