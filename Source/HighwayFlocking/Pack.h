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

#include "msgpack.h"
#include "Vehiclestats.h"

template<typename Type>
struct Packer {
	static void Pack_Impl(msgpack_packer* pk, Type Value) {
		static_assert(false, "No implementation");
	}
};

template<typename Type>
void Pack(msgpack_packer* pk, Type Value) {
	Packer<Type>::Pack_Impl(pk, Value);
}

template<>
struct Packer<uint32> {
	static void Pack_Impl(msgpack_packer* pk, const uint32 &Value) {
		msgpack_pack_uint32(pk, Value);
	}
};

template<>
struct Packer<int32> {
	static void Pack_Impl(msgpack_packer* pk, const int32 &Value) {
		msgpack_pack_int32(pk, Value);
	}
};

template<>
struct Packer<int8> {
	static void Pack_Impl(msgpack_packer* pk, const int8 &Value) {
		msgpack_pack_int8(pk, Value);
	}
};

template<>
struct Packer<uint8> {
	static void Pack_Impl(msgpack_packer* pk, const uint8 &Value) {
		msgpack_pack_uint8(pk, Value);
	}
};

template<>
struct Packer<float> {
	static void Pack_Impl(msgpack_packer* pk, const float &Value) {
		msgpack_pack_float(pk, Value);
	}
};

template<>
struct Packer<const FString*> {
	static void Pack_Impl(msgpack_packer* pk, const FString &Value) {
		const char* data = TCHAR_TO_UTF8(*Value);
		unsigned int len = Value.Len();
		msgpack_pack_str(pk, len);
		msgpack_pack_str_body(pk, data, len);
	}
};


template<>
struct Packer<EFlockingState> {
	static void Pack_Impl(msgpack_packer* pk, const EFlockingState &Value) {
		msgpack_pack_uint8(pk, (uint8)Value);
	}
};

template<>
struct Packer<EVehicleType> {
	static void Pack_Impl(msgpack_packer* pk, const EVehicleType &Value) {
		msgpack_pack_uint8(pk, (uint8)Value);
	}
};

template<>
struct Packer<FVector> {
	static void Pack_Impl(msgpack_packer* pk, const FVector &Value) {
		msgpack_pack_array(pk, 3);
		msgpack_pack_float(pk, Value.X);
		msgpack_pack_float(pk, Value.Y);
		msgpack_pack_float(pk, Value.Z);
	}
};

template<>
struct Packer<FRotator> {
	static void Pack_Impl(msgpack_packer* pk, const FRotator &Value) {
		msgpack_pack_array(pk, 3);
		msgpack_pack_float(pk, Value.Pitch);
		msgpack_pack_float(pk, Value.Yaw);
		msgpack_pack_float(pk, Value.Roll);
	}
};

template<>
struct Packer<FVehicleLogEntry> {
	static void Pack_Impl(msgpack_packer* pk, const FVehicleLogEntry &Value) {
		msgpack_pack_array(pk, 16);
		Pack(pk, Value.Id);
		Pack(pk, Value.CreationTime);
		Pack(pk, Value.Location);
		Pack(pk, Value.Velocity);
		Pack(pk, Value.Rotation);
		Pack(pk, Value.GoalSpeed);
		Pack(pk, Value.MaxSpeed);
		Pack(pk, Value.VehicleType);
		Pack(pk, Value.PriorityLevel);
		Pack(pk, Value.Gear);
		Pack(pk, Value.EngineRotationSpeed);
		Pack(pk, Value.ColorMaterialIndex);
		Pack(pk, Value.FlockingState);
		Pack(pk, Value.ThrottleOutput);
		Pack(pk, Value.BehaviorVectors);
		Pack(pk, Value.Neighbors);
	}
};

template<>
struct Packer<FIncidentLogEntry> {
	static void Pack_Impl(msgpack_packer* pk, const FIncidentLogEntry &Value) {
		msgpack_pack_array(pk, 4);
		Pack(pk, Value.Vehicle1Id);
		Pack(pk, Value.Actor2Id);
		Pack(pk, Value.Time);
		Pack(pk, Value.Vehicles);
	}
};

template<>
struct Packer<FPIDGains> {
	static void Pack_Impl(msgpack_packer* pk, const FPIDGains &PIDGains) {
		msgpack_pack_map(pk, 4);
		msgpack_pack_str(pk, 12);
		msgpack_pack_str_body(pk, "Proportional", 12);
		msgpack_pack_float(pk, PIDGains.ProportionalGain);
		msgpack_pack_str(pk, 8);
		msgpack_pack_str_body(pk, "Integral", 8);
		msgpack_pack_float(pk, PIDGains.IntegralGain);
		msgpack_pack_str(pk, 10);
		msgpack_pack_str_body(pk, "Derivative", 10);
		msgpack_pack_float(pk, PIDGains.DerivativeGain);
		msgpack_pack_str(pk, 5);
		msgpack_pack_str_body(pk, "Limit", 5);
		msgpack_pack_float(pk, PIDGains.Limit);
	}
};

template<typename ElementType>
struct Packer<TArray<ElementType>> {
	static void Pack_Impl(msgpack_packer* pk, const TArray<ElementType> &Array) {
		msgpack_pack_array(pk, Array.Num());
		for (auto Iter = Array.CreateConstIterator(); Iter; ++Iter) {
			Packer<ElementType>::Pack_Impl(pk, *Iter);
		}
	}
};
