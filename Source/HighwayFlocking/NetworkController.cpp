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
#include "NetworkController.h"

#include "Networking.h"
#include "FollowRoadCamera.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "CVehicleSpawner.h"
#include "VehicleStats.h"
#include "MeasurementGate.h"
#include "Pack.h"
#include "Unpack.h"
#include "ReplayPlayer.h"
#include "GitHash.h"
#include "Matinee/MatineeActor.h"

// This is replecated in the simulation.py file, and defines the type of a packet, both
// from the python code to the c++ code, and the other way.

enum EPacketType : uint16 {
	PT_SetPaused = 0,
	PT_GetBehaviors = 1,
	PT_DisableAllSpawners = 2,
	PT_ConfigureSpawner = 3,
	PT_ExecuteCommand = 4,
	PT_GetStats = 5,
	PT_RemoveAllVehicles = 6,
	PT_ResetStats = 7,
	PT_IncidentLog = 8,
	PT_RecreateLogEntry = 9,
	PT_AddArrow = 10,
	PT_StartRecording = 11,
	PT_StopRecording = 12,
	PT_StartPlayback = 13,
	PT_StopPlayback = 14,
	PT_GetGitHash = 15,
	PT_StartMatinee = 16,
	PT_SetArrowsVisible = 17,
	PT_SeekAndPauseReplay = 18,
	PT_SetReplaySpeed = 19,
	PT_SetReplayPaused = 20,
	PT_SelectCamera = 21,
	PT_CameraLookAtVehicle = 22,
	PT_SelectVehicleCamera = 23,
	PT_FollowCam = 24,
};

#define DEFINE_FNAME(str) static const FName str = FName(TEXT(#str))

DEFINE_FNAME(TYPE);
DEFINE_FNAME(DATA);
DEFINE_FNAME(DIRECTION);
DEFINE_FNAME(LANES);
DEFINE_FNAME(MIN_WAIT);
DEFINE_FNAME(MAX_WAIT);
DEFINE_FNAME(VEHICLE_TYPES);
DEFINE_FNAME(CAR);
DEFINE_FNAME(SEDAN);
DEFINE_FNAME(BUS);
DEFINE_FNAME(EMERGENCY);
DEFINE_FNAME(CAMERA_NAME);
DEFINE_FNAME(VEHICLE_ID);
DEFINE_FNAME(OFFSET_Z);
DEFINE_FNAME(OFFSET_Y);
DEFINE_FNAME(LOOK_OFFSET_Z);
DEFINE_FNAME(LOOK_OFFSET_Y);
DEFINE_FNAME(LOOK_DISTANCE);
DEFINE_FNAME(SPEED);
DEFINE_FNAME(REVERSED);
DEFINE_FNAME(FIELD_OF_VIEW);
DEFINE_FNAME(VISIBLE);
DEFINE_FNAME(BEHAVIORS);
DEFINE_FNAME(START_DISTANCE);
DEFINE_FNAME(FOLLOW_SMOOTH);

ANetworkController::ANetworkController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<AMockingVehicle> CarFinder(TEXT("/Game/Vehicle/MockingCar"));
	static ConstructorHelpers::FClassFinder<AMockingVehicle> SedanFinder(TEXT("/Game/Vehicle/MockingSedan"));
	static ConstructorHelpers::FClassFinder<AMockingVehicle> EmergencyFinder(TEXT("/Game/Vehicle/MockingEmergencyVehicle"));
	static ConstructorHelpers::FClassFinder<AMockingVehicle> BusFinder(TEXT("/Game/Vehicle/MockingBus"));
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

	static ConstructorHelpers::FClassFinder<AActor> ArrowFinder(TEXT("/Game/Blueprints/Arrow"));

	AArrow = ArrowFinder.Class;

	msgpack_unpacker_init(&unpacker, MSGPACK_UNPACKER_INIT_BUFFER_SIZE);
}

void ANetworkController::BeginPlay() {
	// TODO: Move this to a more fitting place
	FMath::RandInit(FPlatformTime::Cycles());
	FMath::SRandInit(FPlatformTime::Cycles());

	FIPv4Address Address = FIPv4Address::InternalLoopback;
	uint32 Port = 5062;

	TcpServerSocket = FTcpSocketBuilder(TEXT("Controlling TCP Socket"))
		.AsReusable()
		.AsNonBlocking()
		.BoundToAddress(Address)
		.BoundToPort(Port)
		.Listening(8)
		.Build();

	int32 NewSize = 0;
	TcpServerSocket->SetReceiveBufferSize(2 * 1024 * 1024, NewSize);

	if (!TcpServerSocket) {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Listen socket could not be created! ~> %s %d"), *Address.ToText().ToString(), Port));
		return;
	}

	Listener = new FTcpListener(*TcpServerSocket);

	Listener->OnConnectionAccepted().BindUObject(this, &ANetworkController::ConnectionAccepted);

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Listening to %s:%d"), *Address.ToText().ToString(), Port));

	UWorld* World = GetWorld();
	World->GetTimerManager().SetTimer(SendDataTimerHandle, this, &ANetworkController::SendData, 1.0f, true);
}

void ANetworkController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	UWorld* World = GetWorld();
	World->GetTimerManager().ClearTimer(SendDataTimerHandle);
	Listener->Stop();
	delete Listener;
	Listener = NULL;
	TcpServerSocket->Close();

	if (TcpSocket) {
		TcpSocket->Close();
	}
}

bool ANetworkController::ConnectionAccepted(FSocket* Socket, const FIPv4Endpoint& RemoteAddress) {
	if (TcpSocket) {
		TcpSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(TcpSocket);
	}

	TcpSocket = Socket;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Got a connection"));

	return true;
}

void ANetworkController::Tick(float Delta) {
	TryReceiveData();
}

void ANetworkController::TryReceiveData() {
	if (!TcpSocket) {
		return;
	}

	uint32 Size;
	if (TcpSocket->HasPendingData(Size)) {
		uint8 buffer[4096];
		int32 read;

		TcpSocket->Recv(buffer, 4096, read);

		msgpack_unpacker_reserve_buffer(&unpacker, read);
		memcpy(msgpack_unpacker_buffer(&unpacker), buffer, read);
		msgpack_unpacker_buffer_consumed(&unpacker, read);

		msgpack_unpacked result;
		msgpack_unpacked_init(&result);

		while (msgpack_unpacker_next(&unpacker, &result)) {
			msgpack_object* packet = &result.data;
			TMap<FName, msgpack_object*> Received = Unpack<TMap<FName, msgpack_object*>>(packet);

			uint8 PacketTypeId = Received[TYPE]->via.i64;
			EPacketType PacketType = (EPacketType)PacketTypeId;

			msgpack_object* Data = Received[DATA];

			switch (PacketType) {
			case EPacketType::PT_SetPaused: {
				bool Paused = Received[DATA]->via.boolean;
				ReceivedSetPaused(Paused);
				break;
			}
			case EPacketType::PT_GetBehaviors: {
				ReceivedGetBehaviors();
				break;
			}
			case EPacketType::PT_ConfigureSpawner: {
				TMap<FName, msgpack_object*> DataMap = Unpack<TMap<FName, msgpack_object*>>(Data);
				ReceivedConfigureSpawner(DataMap);
				break;
			}
			case EPacketType::PT_DisableAllSpawners: {
				ReceivedDisableAllSpawners();
				break;
			}
			case EPacketType::PT_ExecuteCommand: {
				ReceivedExecuteCommand(Data);
				break;
			}
			case EPacketType::PT_RemoveAllVehicles: {
				ReceivedRemoveAllVehicles();
				break;
			}
			case EPacketType::PT_ResetStats: {
				ReceivedResetStats();
				break;
			}
			case EPacketType::PT_RecreateLogEntry: {
				ReceivedRecreateLogEntry(Data);
				break;
			}
			case EPacketType::PT_AddArrow: {
				ReceivedAddArrow(Data);
				break;
			}
			case EPacketType::PT_StartRecording: {
				ReceivedStartRecording(Data);
				break;
			}
			case EPacketType::PT_StopRecording: {
				ReceivedStopRecording(Data);
				break;
			}
			case EPacketType::PT_StartPlayback: {
				ReceivedStartPlayback(Data);
				break;
			}
			case EPacketType::PT_StopPlayback: {
				ReceivedStopPlayback(Data);
				break;
			}
			case EPacketType::PT_GetGitHash: {
				ReceivedGetGitHash();
				break;
			}
			case EPacketType::PT_StartMatinee: {
				ReceivedStartMatinee(Data);
				break;
			}
			case EPacketType::PT_SetArrowsVisible:
				ReceivedSetArrowsVisible(Data);
				break;
			case EPacketType::PT_SeekAndPauseReplay:
				ReceivedSeekAndPauseReplay(Data);
				break;
			case EPacketType::PT_SetReplaySpeed:
				ReceivedSetReplaySpeed(Data);
				break;
			case EPacketType::PT_SetReplayPaused:
				ReceivedSetReplayPaused(Data);
				break;
			case EPacketType::PT_SelectCamera:
				ReceivedSelectCamera(Data);
				break;
			case EPacketType::PT_CameraLookAtVehicle:
				ReceivedCameraLookAtVehicle(Data);
				break;
			case EPacketType::PT_SelectVehicleCamera:
				ReceivedSelectVehicleCamera(Data);
				break;
			case EPacketType::PT_FollowCam:
				ReceivedFollowCam(Data);
				break;
			}
		}

		msgpack_unpacked_destroy(&result);
	}
}

void ANetworkController::SendData() {
	if (!TcpSocket) {
		return;
	}

	// Clear the data if there is anything on the stream
	TryReceiveData();

	AVehiclestats* Stats = NULL;
	for (TActorIterator<AVehiclestats> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		Stats = *ObjIt;
		break;
	}
	if (Stats == NULL) {
		return;
	}

	// Send crashlog

	msgpack_sbuffer sbuf;
	msgpack_sbuffer_init(&sbuf);

	msgpack_packer pk;
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);


	for (auto Iter = Stats->IncidentLog.CreateConstIterator(); Iter; ++Iter) {
		msgpack_pack_array(&pk, 2);
		msgpack_pack_uint16(&pk, EPacketType::PT_IncidentLog);
		Pack(&pk, *Iter);
	}

	Stats->IncidentLog.Empty();


	// Send the statestics

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint16(&pk, EPacketType::PT_GetStats);

	msgpack_pack_map(&pk, 6);

	msgpack_pack_str(&pk, 7);
	msgpack_pack_str_body(&pk, "spawned", 7);
	msgpack_pack_uint32(&pk, Stats->GetNumVehiclesSpawned());

	msgpack_pack_str(&pk, 6);
	msgpack_pack_str_body(&pk, "onroad", 6);
	msgpack_pack_uint32(&pk, Stats->GetNumVehiclesOnTheRoad());

	msgpack_pack_str(&pk, 7);
	msgpack_pack_str_body(&pk, "crashed", 7);
	msgpack_pack_uint32(&pk, Stats->GetNumVehiclesCrashed());

	msgpack_pack_str(&pk, 9);
	msgpack_pack_str_body(&pk, "incidents", 9);
	msgpack_pack_uint32(&pk, Stats->GetNumVehiclesIncidents());

	TArray<float> Throughputs;

	for (TActorIterator<AMeasurementGate> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		AMeasurementGate* Gate = *ObjIt;
		Throughputs.Add(Gate->GetThroughput(0));
		Throughputs.Add(Gate->GetThroughput(1));
	}

	msgpack_pack_str(&pk, 11);
	msgpack_pack_str_body(&pk, "throughputs", 11);
	msgpack_pack_array(&pk, Throughputs.Num());
	for (auto Iter = Throughputs.CreateConstIterator(); Iter; ++Iter) {
		msgpack_pack_float(&pk, *Iter);
	}

	msgpack_pack_str(&pk, 4);
	msgpack_pack_str_body(&pk, "time", 4);
	msgpack_pack_float(&pk, Stats->GetTime());

	int32 BytesLeft = sbuf.size;
	while (BytesLeft > 0) {
		int32 Index = sbuf.size - BytesLeft;
		int32 BytesSent;
		TcpSocket->Send((uint8*)sbuf.data + Index, BytesLeft, BytesSent);
		BytesLeft = BytesLeft - BytesSent;
		if (BytesSent == -1) {
			break;
		}
	}

	msgpack_sbuffer_destroy(&sbuf);
}

void ANetworkController::ReceivedCameraLookAtVehicle(msgpack_object* Data) {
	for (TActorIterator<AReplayPlayer> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		uint32 VehicleId = Unpack<uint64>(Data);
		(*ObjIt)->SetCameraLookAtVehicle(VehicleId);
		break;
	}
}

void ANetworkController::ReceivedFollowCam(msgpack_object* Data) {
	TMap<FName, msgpack_object*> Received = Unpack<TMap<FName, msgpack_object*>>(Data);
	float OffsetZ = Unpack<double>(Received[OFFSET_Z]);
	float OffsetY = Unpack<double>(Received[OFFSET_Y]);
	float LookDistance = Unpack<double>(Received[LOOK_DISTANCE]);
	float LookOffsetZ = Unpack<double>(Received[LOOK_OFFSET_Z]);
	float LookOffsetY = Unpack<double>(Received[LOOK_OFFSET_Y]);
	float Speed = Unpack<double>(Received[SPEED]);
	bool Rev = Unpack<bool>(Received[REVERSED]);
	float FieldOfView = Unpack<double>(Received[FIELD_OF_VIEW]);
	float StartDistance = Unpack<double>(Received[START_DISTANCE]);
	float FollowSmooth = Unpack<double>(Received[FOLLOW_SMOOTH]);

	APlayerController* PlayerController = NULL;
	for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator) {
		PlayerController = *Iterator;
		break;
	}
	if (!PlayerController) {
		return;
	}

	FActorSpawnParameters spawnParameters;
	spawnParameters.bNoCollisionFail = true;
	spawnParameters.Owner = this;
	spawnParameters.Instigator = NULL;
	spawnParameters.bDeferConstruction = false;

	AFollowRoadCamera* Camera = GetWorld()->SpawnActor<AFollowRoadCamera>(spawnParameters);

	Camera->Offset = FVector(0, OffsetY, OffsetZ);
	Camera->LookAtOffset = FVector(0, LookOffsetY, LookOffsetZ);
	Camera->LookAtDistance = LookDistance;
	Camera->Speed = Speed;
	Camera->FollowSmooth = FollowSmooth;
	Camera->GetCameraComponent()->FieldOfView = FieldOfView;

	PlayerController->SetViewTarget(Camera);

	Camera->Start(Rev, StartDistance);
}

void ANetworkController::ReceivedStartMatinee(msgpack_object* Data) {
	FString name = Unpack<FString>(Data);

	for (TActorIterator<AMatineeActor> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		AMatineeActor* Matinee = *ObjIt;
		if (Matinee->GetName() == name) {
			Matinee->Play();
			break;
		}
	}
}

void ANetworkController::ReceivedSelectCamera(msgpack_object* Data) {
	FString name = Unpack<FString>(Data);
	APlayerController* PlayerController = NULL;
	for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator) {
		PlayerController = *Iterator;
		break;
	}
	if (!PlayerController) {
		return;
	}

	//PlayerController->PlayerCameraManager->PlayCameraAnim()

	for (TActorIterator<ACameraActor> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		ACameraActor* Camera = *ObjIt;
		//GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Yellow, Camera->GetName());
		if (Camera->GetName() == name) {
			PlayerController->SetViewTarget(Camera);

			AFollowRoadCamera* FollowRoadCamera = Cast<AFollowRoadCamera>(Camera);

			if (FollowRoadCamera) {
				FollowRoadCamera->Start(false, 0);
			}

			return;
		}
	}
}

void ANetworkController::ReceivedSelectVehicleCamera(msgpack_object* Data) {
	for (TActorIterator<AReplayPlayer> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		TMap<FName, msgpack_object*> Received = Unpack<TMap<FName, msgpack_object*>>(Data);
		FString CameraName = Unpack<FString>(Received[CAMERA_NAME]);
		uint32 VehicleId = Unpack<uint64>(Received[VEHICLE_ID]);
		(*ObjIt)->SetUseVehicleCamera(VehicleId, CameraName);
		break;
	}
}

void ANetworkController::ReceivedGetGitHash() {
	msgpack_sbuffer sbuf;
	msgpack_sbuffer_init(&sbuf);

	msgpack_packer pk;
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint16(&pk, EPacketType::PT_GetGitHash);

	msgpack_pack_str(&pk, GIT_HASH_LEN);
	msgpack_pack_str_body(&pk, GIT_HASH, GIT_HASH_LEN);

	int32 BytesSent;
	TcpSocket->Send((uint8*)sbuf.data, sbuf.size, BytesSent);

	msgpack_sbuffer_destroy(&sbuf);
}

void ANetworkController::ReceivedStartRecording(msgpack_object* Data) {
	for (TActorIterator<ARecorder> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		FString Filename = Unpack<FString>(Data);
		(*ObjIt)->StartRecording(Filename);
		break;
	}
}

void ANetworkController::ReceivedStopRecording(msgpack_object* Data) {
	for (TActorIterator<ARecorder> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		(*ObjIt)->StopRecording();
		break;
	}
}

void ANetworkController::ReceivedSetArrowsVisible(msgpack_object* Data) {
	for (TActorIterator<AReplayPlayer> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		TMap<FName, msgpack_object*> Received = Unpack<TMap<FName, msgpack_object*>>(Data);
		bool Visible = Unpack<bool>(Received[VISIBLE]);
		auto Behaviors = Unpack<TArray<FString>>(Received[BEHAVIORS]);

		(*ObjIt)->SetShowArrows(Visible);
		(*ObjIt)->SetShowArrowsBehaviors(Behaviors);
		break;
	}
}

void ANetworkController::ReceivedSeekAndPauseReplay(msgpack_object* Data) {
	AReplayPlayer* Player = NULL;
	for (TActorIterator<AReplayPlayer> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		Player = *ObjIt;
		break;
	}
	check(Player != NULL);

	float Seconds = Unpack<double>(Data);
	Player->SeekAndPause(Seconds);

	msgpack_sbuffer sbuf;
	msgpack_sbuffer_init(&sbuf);

	msgpack_packer pk;
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint16(&pk, EPacketType::PT_SeekAndPauseReplay);

	msgpack_pack_float(&pk, Player->GetTime());

	int32 BytesSent;
	TcpSocket->Send((uint8*)sbuf.data, sbuf.size, BytesSent);

	msgpack_sbuffer_destroy(&sbuf);
}

void ANetworkController::ReceivedSetReplaySpeed(msgpack_object* Data) {
	for (TActorIterator<AReplayPlayer> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		float Speed = Unpack<double>(Data);
		(*ObjIt)->SetSpeed(Speed);
		break;
	}
}

void ANetworkController::ReceivedSetReplayPaused(msgpack_object* Data) {
	for (TActorIterator<AReplayPlayer> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		bool Paused = Unpack<bool>(Data);
		(*ObjIt)->SetPaused(Paused);
		break;
	}
}

void ANetworkController::ReceivedStartPlayback(msgpack_object* Data) {
	for (TActorIterator<AReplayPlayer> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		FString Filename = Unpack<FString>(Data);
		(*ObjIt)->StartPlayback(Filename);
		break;
	}
}

void ANetworkController::ReceivedStopPlayback(msgpack_object* Data) {
	for (TActorIterator<AReplayPlayer> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		(*ObjIt)->StopPlayback();
		break;
	}
}

void ANetworkController::ReceivedAddArrow(msgpack_object* Data) {
	TArray<msgpack_object*> Parameters = Unpack<TArray<msgpack_object*>>(Data);

	FVector Location = Unpack<FVector>(Parameters[0]);
	FRotator Rotation = Unpack<FRotator>(Parameters[1]);

	FActorSpawnParameters spawnParameters;
	spawnParameters.bNoCollisionFail = true;
	spawnParameters.Owner = this;
	spawnParameters.Instigator = NULL;
	spawnParameters.bDeferConstruction = false;

	AActor* Arrow = GetWorld()->SpawnActor<AActor>(AArrow, Location, Rotation, spawnParameters);
}

void ANetworkController::ReceivedRecreateLogEntry(msgpack_object* data) {
	TArray<FVehicleLogEntry> Vehicles = Unpack<TArray<FVehicleLogEntry>>(data);

	FActorSpawnParameters spawnParameters;
	spawnParameters.bNoCollisionFail = true;
	spawnParameters.Owner = this;
	spawnParameters.Instigator = NULL;
	spawnParameters.bDeferConstruction = false;

	for (auto Iter = Vehicles.CreateConstIterator(); Iter; ++Iter) {
		const FVehicleLogEntry* Entry = &*Iter;
		TSubclassOf<class AMockingVehicle> Type = VehicleTypeClass[(uint8)Entry->VehicleType];
		AMockingVehicle* NewVehicle = GetWorld()->SpawnActor<AMockingVehicle>(Type, Entry->Location, Entry->Rotation, spawnParameters);

		if (VehicleTypeMaterials.Contains((uint8)Entry->VehicleType)) {
			auto Materials = VehicleTypeMaterials[(uint8)Entry->VehicleType];
			int32 Index = Entry->ColorMaterialIndex;
			UMaterial* Material = Materials[Index];
			NewVehicle->GetMesh()->SetMaterial(2, Material);
		}

		NewVehicle->SetFlockingState(Entry->FlockingState, true);
		NewVehicle->BehaviorVectors = Entry->BehaviorVectors;
		NewVehicle->UpdateArrows();
	}
}

void ANetworkController::ReceivedRemoveAllVehicles() {
	for (TActorIterator<AWheeledVehicle> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		AWheeledVehicle* Vehicle = *ObjIt;
		Vehicle->Destroy();
	}

	for (TActorIterator<AMockingVehicle> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		AMockingVehicle* Vehicle = *ObjIt;
		Vehicle->Destroy();
	}

	for (TActorIterator<AActor> ObjIt(GetWorld(), AArrow); ObjIt; ++ObjIt) {
		(*ObjIt)->Destroy();
	}
}

void ANetworkController::ReceivedResetStats() {
	for (TActorIterator<AVehiclestats> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		auto Stats = *ObjIt;
		Stats->ResetStats();
	}
}

void ANetworkController::ReceivedConfigureSpawner(TMap<FName, msgpack_object*> data) {

	const EDirection Direction = (EDirection)data[DIRECTION]->via.i64;
	const TArray<int64> Lanes = Unpack<TArray<int64>>(data[LANES]);
	const float MinWait = data[MIN_WAIT]->via.f64;
	const float MaxWait = data[MAX_WAIT]->via.f64;
	const TMap<FName, int64> VehicleTypes = Unpack<TMap<FName, int64>>(data[VEHICLE_TYPES]);

	for (TActorIterator<ACVehicleSpawner> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		ACVehicleSpawner* Spawner = *ObjIt;
		if (!Lanes.Contains(Spawner->Lane)) {
			continue;
		}
		if (!(Spawner->Direction == Direction)) {
			continue;
		}

		Spawner->Active = true;
		Spawner->MaxTimeWait = MaxWait;
		Spawner->MinTimeWait = MinWait;
		Spawner->VehicleTypes.Empty();
		for (auto Iter = VehicleTypes.CreateConstIterator(); Iter; ++Iter) {
			EVehicleType VehicleType = EVehicleType::VT_Car;
			if (Iter.Key() == CAR) {
				VehicleType = EVehicleType::VT_Car;
			}
			else if (Iter.Key() == SEDAN) {
				VehicleType = EVehicleType::VT_Sedan;
			}
			else if (Iter.Key() == BUS) {
				VehicleType = EVehicleType::VT_Bus;
			}
			else if (Iter.Key() == EMERGENCY) {
				VehicleType = EVehicleType::VT_Emergency;
			}
			else {
				check(false);
			}

			Spawner->VehicleTypes.Add(FVehicleTypeStruct(VehicleType, Iter.Value()));
		}

		Spawner->TurnBucket();
	}
}

void ANetworkController::ReceivedDisableAllSpawners() {
	for (TActorIterator<ACVehicleSpawner> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		ACVehicleSpawner* Spawner = *ObjIt;
		Spawner->Active = false;
	}
}

void ANetworkController::ReceivedGetBehaviors() {
	msgpack_sbuffer sbuf;
	msgpack_sbuffer_init(&sbuf);

	/* serialize values into the buffer using msgpack_sbuffer_write callback function. */
	msgpack_packer pk;
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint16(&pk, EPacketType::PT_GetBehaviors);

	TArray<ABehaviours*> BehaviorsList;
	for (TActorIterator<ABehaviours> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		ABehaviours* Behaviors = *ObjIt;
		BehaviorsList.Add(Behaviors);
	}

	int32 BehaviorsListNum = BehaviorsList.Num();
	msgpack_pack_array(&pk, BehaviorsListNum);
	for (int i = 0; i < BehaviorsListNum; i++) {
		ABehaviours* Behaviors = BehaviorsList[i];
		msgpack_pack_map(&pk, 4);
		msgpack_pack_str(&pk, 13);
		msgpack_pack_str_body(&pk, "FlockingState", 13);
		msgpack_pack_uint8(&pk, (uint8)Behaviors->FlockingState);
		msgpack_pack_str(&pk, 13);
		msgpack_pack_str_body(&pk, "ThrottleGains", 13);
		Pack(&pk, Behaviors->ThrottleGains);
		msgpack_pack_str(&pk, 13);
		msgpack_pack_str_body(&pk, "SteeringGains", 13);
		Pack(&pk, Behaviors->SteeringGains);

		int32 BehaviorsNum = Behaviors->Behaviours.Num();
		msgpack_pack_str(&pk, 9);
		msgpack_pack_str_body(&pk, "Behaviors", 9);
		msgpack_pack_array(&pk, BehaviorsNum);
		for (int j = 0; j < BehaviorsNum; j++) {
			UBehaviour* Behavior = Behaviors->Behaviours[j];

			TArray<UProperty*> Properties;
			for (TFieldIterator<UProperty> PropIt(Behavior->GetClass()); PropIt; ++PropIt) {
				UProperty* Property = *PropIt;
				if (Cast<UNumericProperty>(Property) || Cast<UBoolProperty>(Property)) {
					Properties.Add(Property);
				}
			}

			msgpack_pack_map(&pk, Properties.Num() + 1);
			msgpack_pack_str(&pk, 4);
			msgpack_pack_str_body(&pk, "Name", 4);
			FString Name = Behavior->GetClass()->GetName();
			msgpack_pack_str(&pk, Name.Len());
			msgpack_pack_str_body(&pk, TCHAR_TO_UTF8(*Name), Name.Len());
			for (auto PropIt(Properties.CreateIterator()); PropIt; ++PropIt) {
				UProperty* Property = *PropIt;
				const void* Value = Property->ContainerPtrToValuePtr<uint8>(Behavior);
				FString Name = Property->GetName();
				msgpack_pack_str(&pk, Name.Len());
				msgpack_pack_str_body(&pk, TCHAR_TO_UTF8(*Name), Name.Len());
				if (UNumericProperty *NumericProperty = Cast<UNumericProperty>(Property)) {
					if (NumericProperty->IsFloatingPoint()) {
						msgpack_pack_double(&pk, NumericProperty->GetFloatingPointPropertyValue(Value));
					}
					else if (NumericProperty->IsInteger()) {
						msgpack_pack_int(&pk, NumericProperty->GetSignedIntPropertyValue(Value));
					}
				}
				else if (UBoolProperty *BoolProperty = Cast<UBoolProperty>(Property)) {
					if (BoolProperty->GetPropertyValue(Value)) {
						msgpack_pack_true(&pk);
					}
					else {
						msgpack_pack_false(&pk);
					}
				}
			}
		}
	}

	int32 BytesSent;
	TcpSocket->Send((uint8*)sbuf.data, sbuf.size, BytesSent);

	msgpack_sbuffer_destroy(&sbuf);
}

void ANetworkController::ReceivedSetPaused(bool Paused) {
	for (TActorIterator<ACVehicleSpawner> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		ACVehicleSpawner* Spawner = *ObjIt;
		Spawner->Paused = Paused;
	}
}

void ANetworkController::ReceivedExecuteCommand(msgpack_object* data) {
	FString Command = Unpack<FString>(data);
	//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::White, "'" + Command + "'");
	UWorld* World = GetWorld();
	GEngine->Exec(World, *Command);
}
