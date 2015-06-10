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
#include "Networking.h"

#include "msgpack.h"

#include "MockingVehicle.h"

#include "NetworkController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDelegateStartMatinee, int32, MatineeIndex);

UCLASS()
class HIGHWAYFLOCKING_API ANetworkController : public AActor {
	GENERATED_UCLASS_BODY()

private:
	FSocket* TcpServerSocket;
	FSocket* TcpSocket;
	FTcpListener* Listener;
	FTimerHandle SendDataTimerHandle;

	TMap<uint8, TSubclassOf<class AMockingVehicle>> VehicleTypeClass;
	TMap<uint8, TArray<UMaterial*>> VehicleTypeMaterials;

	TSubclassOf<class AActor> AArrow;

	msgpack_unpacker unpacker;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float Delta) override;

	bool ConnectionAccepted(FSocket* Socket, const FIPv4Endpoint& RemoteAddress);

	void ReceivedSetPaused(bool Paused);
	void ReceivedGetBehaviors();
	void ReceivedConfigureSpawner(TMap<FName, msgpack_object*> data);
	void ReceivedDisableAllSpawners();
	void ReceivedExecuteCommand(msgpack_object* data);
	void ReceivedRemoveAllVehicles();
	void ReceivedResetStats();
	void ReceivedRecreateLogEntry(msgpack_object* data);
	void ReceivedAddArrow(msgpack_object* Data);
	void ReceivedStartRecording(msgpack_object* Data);
	void ReceivedStopRecording(msgpack_object* Data);
	void ReceivedStartPlayback(msgpack_object* Data);
	void ReceivedStopPlayback(msgpack_object* Data);
	void ReceivedGetGitHash();
	void ReceivedStartMatinee(msgpack_object* Data);
	void ReceivedSetArrowsVisible(msgpack_object* Data);
	void ReceivedSeekAndPauseReplay(msgpack_object* Data);
	void ReceivedSetReplaySpeed(msgpack_object* Data);
	void ReceivedSetReplayPaused(msgpack_object* Data);
	void ReceivedSelectCamera(msgpack_object* Data);
	void ReceivedSelectVehicleCamera(msgpack_object* Data);
	void ReceivedCameraLookAtVehicle(msgpack_object* Data);
	void ReceivedFollowCam(msgpack_object* Data);

	void SendData();
	void TryReceiveData();

	UPROPERTY(BlueprintAssignable, Category = "Network Events")
	FDelegateStartMatinee OnStartMatinee;
};
