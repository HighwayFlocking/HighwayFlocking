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
#include "Utils.h"

template<typename Type>
struct Unpacker {
	static Type Unpack_Impl(const msgpack_object* obj) {
		static_assert(false, "Not defined");
	}
};


template<typename Type>
Type Unpack(msgpack_object* obj) {
	return Unpacker<Type>::Unpack_Impl(obj);
}

template<>
struct Unpacker <msgpack_object*> {
	static msgpack_object* Unpack_Impl(msgpack_object* obj) {
		return obj;
	}
};

template<>
struct Unpacker <bool> {
	static bool Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_BOOLEAN);
		return obj->via.boolean;
	}
};

template<>
struct Unpacker <uint64> {
	static uint64 Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_POSITIVE_INTEGER);
		return obj->via.u64;
	}
};

template<>
struct Unpacker <int64> {
	static int64 Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_POSITIVE_INTEGER || obj->type == MSGPACK_OBJECT_NEGATIVE_INTEGER);
		return obj->via.i64;
	}
};

template<>
struct Unpacker <double> {
	static double Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_FLOAT);
		return obj->via.f64;
	}
};

template<>
struct Unpacker <FString> {
	static FString Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_STR);
		char buffer[255];
		strncpy_s(buffer, obj->via.str.ptr, obj->via.str.size);
		buffer[obj->via.str.size] = '\0';
		return UTF8_TO_TCHAR(buffer);
	}
};

template<>
struct Unpacker <FName> {
	static FName Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_STR);
		char buffer[255];
		strncpy_s(buffer, obj->via.str.ptr, obj->via.str.size);
		buffer[obj->via.str.size] = '\0';
		return UTF8_TO_TCHAR(buffer);
	}
};

template<typename ElementType>
struct Unpacker <TArray<ElementType>> {
	static TArray<ElementType> Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_ARRAY);
		TArray<ElementType> Result;
		msgpack_object* p = obj->via.array.ptr;
		for (uint32 i = 0; i < obj->via.array.size; ++i) {
			Result.Add(Unpack<ElementType>(p + i));
		}
		return Result;
	}
};

template<typename KeyType, typename ValType>
struct Unpacker <TMap<KeyType, ValType>> {
	static TMap<KeyType, ValType> Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_MAP);
		TMap<KeyType, ValType> Result;
		msgpack_object_kv* p = obj->via.map.ptr;
		for (uint32 i = 0; i < obj->via.map.size; ++i) {
			KeyType Key = Unpack<KeyType>(&p[i].key);
			ValType Value = Unpack<ValType>(&p[i].val);
			Result.Add(Key, Value);
		}
		return Result;
	}
};

template<>
struct Unpacker <EFlockingState> {
	static EFlockingState Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_POSITIVE_INTEGER);
		return (EFlockingState)obj->via.u64;
	}
};

template<>
struct Unpacker <EVehicleType> {
	static EVehicleType Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_POSITIVE_INTEGER);
		return (EVehicleType)obj->via.u64;
	}
};

template<>
struct Unpacker <FVector> {
	static FVector Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_ARRAY);
		check(obj->via.array.size == 3);
		msgpack_object* p = obj->via.array.ptr;
		check(p[0].type == MSGPACK_OBJECT_FLOAT);
		check(p[1].type == MSGPACK_OBJECT_FLOAT);
		check(p[2].type == MSGPACK_OBJECT_FLOAT);
		float X = p[0].via.f64;
		float Y = p[1].via.f64;
		float Z = p[2].via.f64;

		return FVector(X, Y, Z);
	}
};

template<>
struct Unpacker <FRotator> {
	static FRotator Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_ARRAY);
		check(obj->via.array.size == 3);
		msgpack_object* p = obj->via.array.ptr;
		check(p[0].type == MSGPACK_OBJECT_FLOAT);
		check(p[1].type == MSGPACK_OBJECT_FLOAT);
		check(p[2].type == MSGPACK_OBJECT_FLOAT);
		float Pitch = p[0].via.f64;
		float Yaw = p[1].via.f64;
		float Roll = p[2].via.f64;

		return FRotator(Pitch, Yaw, Roll);
	}
};

template<>
struct Unpacker <FVehicleLogEntry> {
	static FVehicleLogEntry Unpack_Impl(const msgpack_object* obj) {
		check(obj->type == MSGPACK_OBJECT_ARRAY);
		check(obj->via.array.size == 16);
		msgpack_object* p = obj->via.array.ptr;

		FVehicleLogEntry Result;

		Result.Id = Unpack<uint64>(p + 0);
		Result.CreationTime = Unpack<double>(p + 1);
		Result.Location = Unpack<FVector>(p + 2);
		Result.Velocity = Unpack<FVector>(p + 3);
		Result.Rotation = Unpack<FRotator>(p + 4);
		Result.GoalSpeed = Unpack<double>(p + 5);
		Result.MaxSpeed = Unpack<double>(p + 6);
		Result.VehicleType = Unpack<EVehicleType>(p + 7);
		Result.PriorityLevel = Unpack<uint64>(p + 8);
		Result.Gear = Unpack<int64>(p + 9);
		Result.EngineRotationSpeed = Unpack<double>(p + 10);
		Result.ColorMaterialIndex = Unpack<uint64>(p + 11);
		Result.FlockingState = Unpack<EFlockingState>(p + 12);
		Result.ThrottleOutput = Unpack<double>(p + 13);
		Result.BehaviorVectors = Unpack<TArray<FVector>>(p + 14);
		Result.Neighbors = TArray<uint32>(Unpack<TArray<uint64>>(p + 15));

		return Result;
	}
};
