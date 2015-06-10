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
#include "RenderUtils.h"
#include "MockingVehicle.h"

#include "CHud.h"

ACHud::ACHud(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("Material'/Game/Materials/M_Crosshair.M_Crosshair'"));
	CrosshairMaterial = Material.Object;

	static ConstructorHelpers::FObjectFinder<UFont> FontFinder(TEXT("Font'/Engine/EngineFonts/RobotoDistanceField.RobotoDistanceField'"));
	Font = FontFinder.Object;

	HideHud = false;
	DrawVehicleIds = false;
}

void ACHud::DrawHUD() {
	Super::DrawHUD();

	if (HideHud) {
		return;
	}

	int32 SizeX = Canvas->SizeX;
	int32 SizeY = Canvas->SizeY;

	float RatioX = ((float)SizeX) / 1280;
	float RatioY = ((float)SizeY) / 720;

	DrawMaterial(CrosshairMaterial, SizeX / 2 - 20, SizeY / 2 - 20, 40, 40, 0, 0, 1, 1);

	if (!Stats) {
		for (TActorIterator<AVehiclestats> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
			Stats = *ObjIt;
			break;
		}
		if (!Stats) {
			DrawText(
				FString::Printf(TEXT("Did not find any stats objects!")),
				FLinearColor::Red, RatioX * 700, RatioY * 90, Font, RatioY * 1.4);
			return;
		}
	}

	float time = Stats->GetTime();
	int minutes = (int) time / 60;
	int seconds = (int)time % 60;

	FString TimeString = FString::Printf(TEXT("%02d:%02d"), minutes, seconds);
	DrawText(TimeString, FLinearColor::White, RatioX * 1100, RatioY * 45, Font, RatioY * 1.4);

	uint32 y = 80;

	DrawText(
		FString::Printf(TEXT("Vehicles Spawned: %d"), Stats->GetNumVehiclesSpawned()),
		FLinearColor::White, RatioX * 1000, RatioY * y, Font, RatioY * 0.9);

	y += 25;

	DrawText(
		FString::Printf(TEXT("Vehicles On Road: %d"), Stats->GetNumVehiclesOnTheRoad()),
		FLinearColor::White, RatioX * 1000, RatioY * y, Font, RatioY * 0.9);

	y += 25;

	DrawText(
		FString::Printf(TEXT("Vehicles Incidents: %d"), Stats->GetNumVehiclesIncidents()),
		FLinearColor::White, RatioX * 1000, RatioY * y, Font, RatioY * 0.9);

	y += 25;

	DrawText(
		FString::Printf(TEXT("Throughput: %.2f veh/h"), Stats->GetThroughput()),
		FLinearColor::White, RatioX * 1000, RatioY * y, Font, RatioY * 0.9);

	if (DrawVehicleIds) {
		for (TActorIterator<AMockingVehicle> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
			AMockingVehicle* Vehicle = *ObjIt;

			FVector ScreenLocation = Project(Vehicle->GetActorLocation() + Vehicle->GetActorForwardVector() * 200);

			DrawText(FString::FromInt(Vehicle->ReplayUniqueId), FColor::White, ScreenLocation.X, ScreenLocation.Y, Font, RatioY * 0.5);
		}
	}

	AWheeledVehicle* Vehicle = Cast<AWheeledVehicle>(GetOwningPawn());

	if (Vehicle == NULL) {
		return;
	}

	UWheeledVehicleMovementComponent* movement = Vehicle->GetVehicleMovement();

	FString SpeedText = FString::FromInt(FMath::RoundToInt(movement->GetForwardSpeed() * 0.036)) + TEXT(" km/h");
	DrawText(SpeedText, FLinearColor::White, RatioX * 805, RatioY * 455, Font, RatioY * 1.4);

	FString GearDisplayString = TEXT("Gear: ") + FString::FromInt(movement->GetCurrentGear());
	DrawText(GearDisplayString, FLinearColor::White, RatioX * 805, RatioY * 500, Font, RatioY * 1.4);

	FString RPMDisplayText = FString::SanitizeFloat(movement->GetEngineRotationSpeed()) + TEXT(" RPM");
	DrawText(RPMDisplayText, FLinearColor::White, RatioX * 805, RatioY * 410, Font, RatioY * 1.4);

	int32 X = RatioX * 805;
	int32 Y = RatioY * 200;

	FCanvasTileItem TileItem(FVector2D(X - 15, Y - 25), FVector2D(30, 50), FColor::White);
	TileItem.BlendMode = SE_BLEND_Translucent;

	float Angle = FMath::RadiansToDegrees(Vehicle->GetActorForwardVector().HeadingAngle());

	TileItem.Rotation = FRotator(0, Angle + 90, 0);
	TileItem.PivotPoint = FVector2D(0.5f, 0.5f);
	Canvas->DrawItem(TileItem);

	DrawMaterial(CrosshairMaterial, X - 10, Y - 10, 20, 20, 0, 0, 1, 1);

	AFlockingVehicle* FlockingVehicle = Cast<AFlockingVehicle>(Vehicle);

	if (FlockingVehicle == NULL) {
		return;
	}

	FString ThrottleDisplayText = TEXT("Throttle: ") + FString::SanitizeFloat(FlockingVehicle->ThrottleOutput);
	DrawText(ThrottleDisplayText, FLinearColor::White, RatioX * 805, RatioY * 365, Font, RatioY * 1.4);

	TArray<FCamPacket>* Neighbors = &FlockingVehicle->Neighbors;

	int32 NeighborsLen = Neighbors->Num();

	FString NeighborsNumText = TEXT("Neighbors: ") + FString::FromInt(Neighbors->Num());
	DrawText(NeighborsNumText, FLinearColor::White, RatioX * 805, RatioY * 320, Font, RatioY * 1.4);

	FString GoalSpeedText = TEXT("Goal Speed: ") + FString::FromInt(FlockingVehicle->GoalSpeed* 0.036) + TEXT(" km/h");
	DrawText(GoalSpeedText, FLinearColor::White, RatioX * 805, RatioY * 275, Font, RatioY * 1.4);

	float ts = GetWorld()->GetTimeSeconds();

	for (int i = 0; i < NeighborsLen; i++) {
		FCamPacket Neighbor = (*Neighbors)[i];
		float delta = ts - Neighbor.timestamp;
		FVector NLocation = Neighbor.Location + Neighbor.ForwardVector * Neighbor.speed * delta;
		FVector ScreenLocation = Project(NLocation);

		if (ScreenLocation.Z == 0.0f) {
			continue;
		}

		DrawRect(FColor::White, ScreenLocation.X, ScreenLocation.Y, 10, 10);
		FVector NeighborVector = Neighbor.ForwardVector * Neighbor.speed / Neighbor.MaxSpeed;
		float MyDir = FVector::DotProduct(NeighborVector, FlockingVehicle->GetActorForwardVector().GetSafeNormal2D());
		DrawText(FString::SanitizeFloat(MyDir), FColor::White, ScreenLocation.X, ScreenLocation.Y + 10, Font, RatioY * 0.5);
		FVector InFront = Project(NLocation + NeighborVector * 500);
		if (InFront.Z > 0.0f) {
			Draw2DLine(ScreenLocation.X, ScreenLocation.Y, InFront.X, InFront.Y, FColor::White);
		}
	}

	ABehaviours* Behaviours = NULL;

	UWorld* World = GetWorld();

	if (World != NULL) {
		for (TActorIterator<ABehaviours> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
			Behaviours = *ObjIt;
			break;
		}
	}

	if (Behaviours == NULL) {
		return;
	}

	for (int i = 0; i < FlockingVehicle->BehaviorVectors.Num(); i++) {
		FVector Vector = FlockingVehicle->BehaviorVectors[i] * 100;
		UBehaviour* Behavior = Behaviours->Behaviours[i];
		FColor Color = Behavior->DebugColor;

		Draw2DLine(X, Y, X + Vector.X, Y + Vector.Y, Color);
	}
}


