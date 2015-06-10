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
#include "CVehicleSpawner.h"

ACVehicleSpawner::ACVehicleSpawner(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer){
	USkeletalMeshComponent* Mesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("Mesh"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PlaceHolderMesh(TEXT("SkeletalMesh'/Game/Spawner/placeholder_vehicle.placeholder_vehicle'"));
	Mesh->SetSkeletalMesh(PlaceHolderMesh.Object);
	RootComponent = Mesh;

	SetActorHiddenInGame(true);

	static ConstructorHelpers::FClassFinder<AFlockingVehicle> CarFinder(TEXT("/Game/Vehicle/FlockingCar"));
	static ConstructorHelpers::FClassFinder<AFlockingVehicle> SedanFinder(TEXT("/Game/Vehicle/FlockingSedan"));
	static ConstructorHelpers::FClassFinder<AFlockingVehicle> EmergencyFinder(TEXT("/Game/Vehicle/EmergencyVehicle"));
	static ConstructorHelpers::FClassFinder<AFlockingVehicle> BusFinder(TEXT("/Game/Vehicle/FlockingBus"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Yellow(TEXT("Material'/Game/Sedan/Materials/M_Vehicle_Sedan_yelllo.M_Vehicle_Sedan_yelllo'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Blue(TEXT("Material'/Game/Sedan/Materials/M_Vehicle_Sedan.M_Vehicle_Sedan'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Green(TEXT("Material'/Game/Sedan/Materials/M_Vehicle_Sedan_green.M_Vehicle_Sedan_green'"));


	VehicleTypeClass.Add((uint8)EVehicleType::VT_Car, CarFinder.Class);
	VehicleTypeClass.Add((uint8)EVehicleType::VT_Sedan, SedanFinder.Class);
	VehicleTypeClass.Add((uint8)EVehicleType::VT_Bus, BusFinder.Class);
	VehicleTypeClass.Add((uint8)EVehicleType::VT_Emergency, EmergencyFinder.Class);

	TArray<UMaterial*> Materials;
	Materials.Add(Yellow.Object);
	Materials.Add(Blue.Object);
	Materials.Add(Green.Object);

	VehicleTypeMaterials.Add((uint8)EVehicleType::VT_Car, Materials);
	VehicleTypeMaterials.Add((uint8)EVehicleType::VT_Sedan, Materials);
}

float ACVehicleSpawner::GetTimeWait() {
	return FMath::FRandRange(MinTimeWait, MaxTimeWait);
}

void ACVehicleSpawner::BeginPlay() {
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, this, &ACVehicleSpawner::OnTimer, GetTimeWait(), false);

	TurnBucket();
}

void ACVehicleSpawner::TurnBucket() {
	Bucket.Empty();

	for (int i = 0; i < VehicleTypes.Num(); i++) {
		FVehicleTypeStruct VehicleType = VehicleTypes[i];
		for (int j = 0; j < VehicleType.Count; j++) {
			Bucket.Add(VehicleType.VehicleType);
		}
	}
}

void ACVehicleSpawner::OnTimer() {
	FTimerHandle Handle;
	if (!Active || Paused) {
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &ACVehicleSpawner::OnTimer, GetTimeWait(), false);
		return;
	}

	if (Bucket.Num() == 0) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("You need to add Vehicle Types to the Vehicle Spawner"));
	}

	FVector StartTrace = GetActorLocation() - GetActorForwardVector() * 400 + FVector(0, 0, 100);
	FVector EndTrace = GetActorLocation() + GetActorForwardVector() * 400 + FVector(0, 0, 100);
	if (GetWorld()->LineTraceTest(
			StartTrace,
			EndTrace,
			ECollisionChannel::ECC_Vehicle,
			FCollisionQueryParams(),
			FCollisionResponseParams()
			)) {
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &ACVehicleSpawner::OnTimer, 0.1f, false);
		return;
	}

	FActorSpawnParameters spawnParameters;
	spawnParameters.bNoCollisionFail = true;
	spawnParameters.Owner = this;
	spawnParameters.Instigator = NULL;
	spawnParameters.bDeferConstruction = false;

	int32 BucketIndex = FMath::RandRange(0, Bucket.Num() - 1);

	EVehicleType VehicleType = Bucket[BucketIndex];
	Bucket.RemoveAtSwap(BucketIndex);

	if (Bucket.Num() == 0) {
		TurnBucket();
	}

	TSubclassOf<class AFlockingVehicle> Type = VehicleTypeClass[(uint8)VehicleType];
	AFlockingVehicle* NewVehicle = GetWorld()->SpawnActor<AFlockingVehicle>(Type, GetActorLocation(), GetActorRotation(), spawnParameters);

	NewVehicle->SetFlockingState(FlockingState);
	NewVehicle->SpawnDefaultController();
	NewVehicle->GetMesh()->SetAllPhysicsLinearVelocity(GetActorForwardVector() * StartSpeed / 0.036);

	if (NewVehicle->VehicleType != VehicleType) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("Vehicle Type is not correct."));
	}

	GetWorld()->GetTimerManager().SetTimer(Handle, this, &ACVehicleSpawner::OnTimer, GetTimeWait(), false);

	if (VehicleTypeMaterials.Contains((uint8)VehicleType)) {
		auto Materials = VehicleTypeMaterials[(uint8)VehicleType];
		int32 Index = FMath::RandRange(0, Materials.Num() - 1);
		UMaterial* Material = Materials[Index];
		NewVehicle->GetMesh()->SetMaterial(2, Material);
		NewVehicle->ColorMaterialIndex = Index;
	}
}
