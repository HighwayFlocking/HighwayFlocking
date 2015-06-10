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
#include "VehicleSpectatorPawn.h"

AVehicleSpectatorPawn::AVehicleSpectatorPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAddDefaultMovementBindings = false;
}

void AVehicleSpectatorPawn::SetupPlayerInputComponent(UInputComponent* InputComponent) {
	check(InputComponent);

	InputComponent->BindAxis("VehicleSpectatorPawn_MoveForward", this, &AVehicleSpectatorPawn::MoveForward);
	InputComponent->BindAxis("VehicleSpectatorPawn_MoveRight", this, &AVehicleSpectatorPawn::MoveRight);
	InputComponent->BindAxis("VehicleSpectatorPawn_MoveUp", this, &AVehicleSpectatorPawn::MoveUp_World);
	InputComponent->BindAxis("VehicleSpectatorPawn_Turn", this, &AVehicleSpectatorPawn::AddControllerYawInput);
	InputComponent->BindAxis("VehicleSpectatorPawn_TurnRate", this, &AVehicleSpectatorPawn::TurnAtRate);
	InputComponent->BindAxis("VehicleSpectatorPawn_LookUp", this, &AVehicleSpectatorPawn::AddControllerPitchInput);
	InputComponent->BindAxis("VehicleSpectatorPawn_LookUpRate", this, &AVehicleSpectatorPawn::LookUpAtRate);
}
