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
#include "Utils.h"

// From http://stackoverflow.com/questions/2824478/shortest-distance-between-two-line-segments

const UWorld* g_world = 0;

inline bool SegmentsIntersect(
	FVector a1,
	FVector a2,
	FVector b1,
	FVector b2
	)
{
	float dx1 = a2.X - a1.X;
	float dy1 = a2.Y - a1.Y;
	float dx2 = b2.X - b1.X;
	float dy2 = b2.Y - b1.Y;
	float delta = dx2 * dy1 - dy2 * dx1;
	if (delta == 0) {
		return false;  // parallel segments
	}
	float s = (dx1 * (b1.Y - a1.Y) + dy1 * (a1.X - b1.X)) / delta;
	float t = (dx2 * (a1.Y - b1.Y) + dy2 * (b1.X - a1.X)) / (-delta);

	return (s >= 0 && s <= 1 && t >= 0 && t <= 1);
}

inline FVector PointSegmentMinVector(FVector P, FVector A, FVector B) {
	FVector D = B - A;
	if (D.X == 0 && D.Y == 0) {
		return A - P;
	}

	// Calculate the t that minimizes the distance.
	float t = ((P.X - A.X) * D.X + (P.Y - A.Y) * D.Y) / (D.X * D.X + D.Y * D.Y);

	// See if this represents one of the segment's
	// end points or a point in the middle.

	if (t < 0) {
		return A - P;
	}
	else if (t > 1) {
		return B - P;
	}
	else {
		FVector Near(
			A.X + t * D.X,
			A.Y + t * D.Y,
			0);
		return Near - P;
	}
}

inline FVector ShortestVector(FVector A, FVector B) {
	if (A.SizeSquared() < B.SizeSquared()) {
		return A;
	}
	else {
		return B;
	}
}

FVector SegmentsShortestVector(
	FVector a1,
	FVector a2,
	FVector b1,
	FVector b2
	)
{
	if (SegmentsIntersect(a1, a2, b1, b2)) {
		return FVector::ZeroVector;
	}

	FVector MinVector =
		PointSegmentMinVector(a1, b1, b2);
	MinVector = ShortestVector(MinVector,
		PointSegmentMinVector(a2, b1, b2));
	MinVector = ShortestVector(MinVector,
		- PointSegmentMinVector(b1, a1, a2));
	MinVector = ShortestVector(MinVector,
		- PointSegmentMinVector(b2, a1, a2));

	return MinVector;
}

typedef struct {
	FVector A;
	FVector B;
	FVector C;
	FVector D;
} FVehiclePoints;

FVehiclePoints GetVehiclePoints(FCamPacket Vehicle, const float Time, const float ExtraFront=0.0f, const float ExtraSide=0.0f) {
	//  A         B
	//   ---------
	//   |       |   ^
	//   |       |   |
	//   |   x   |   -->  (SideVector and ForwardVector)
	//   |       |
	//   |       |
	//   ---------
	//  D         C

	float delta = Time - Vehicle.timestamp;

	FVector SideVector = FVector::CrossProduct(FVector::UpVector, Vehicle.ForwardVector);

	SideVector.Z = 0;
	FVector ForwardVector = Vehicle.ForwardVector;
	ForwardVector.Z = 0;

	FVector Location = Vehicle.Location + ForwardVector * Vehicle.speed * delta;
	Location.Z = 0;

	FVehiclePoints result;

	result.A = Location - SideVector * (Vehicle.Width / 2 + ExtraSide) + ForwardVector * (Vehicle.Length / 2 + ExtraFront);
	result.B = Location + SideVector * (Vehicle.Width / 2 + ExtraSide) + ForwardVector * (Vehicle.Length / 2 + ExtraFront);
	result.C = Location + SideVector * (Vehicle.Width / 2 + ExtraSide) - ForwardVector * (Vehicle.Length / 2);
	result.D = Location - SideVector * (Vehicle.Width / 2 + ExtraSide) - ForwardVector * (Vehicle.Length / 2);

	return result;
}

inline bool PointInside(FVector Point, FVehiclePoints Rect) {
	// Barycentric Technique from http://www.blackpawn.com/texts/pointinpoly/

	// First using the triangle ABC

	// Compute vectors
	FVector V0 = Rect.C - Rect.A;
	FVector V1 = Rect.B - Rect.A;
	FVector V2 = Point - Rect.A;

	// Compute dot products
	float dot00 = FVector::DotProduct(V0, V0);
	float dot01 = FVector::DotProduct(V0, V1);
	float dot02 = FVector::DotProduct(V0, V2);
	float dot11 = FVector::DotProduct(V1, V1);
	float dot12 = FVector::DotProduct(V1, V2);

	// Compute barycentric coordinates
	float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Check if point is in triangle
	if ((u >= 0) && (v >= 0) && (u + v < 1)) {
		return true;
	}

	// Then the triangle ADC

	// Compute vectors
	V1 = Rect.D - Rect.A;

	// Compute dot products
	dot01 = FVector::DotProduct(V0, V1);
	dot11 = FVector::DotProduct(V1, V1);
	dot12 = FVector::DotProduct(V1, V2);

	// Compute barycentric coordinates
	invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
	u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Check if point is in triangle
	if ((u >= 0) && (v >= 0) && (u + v < 1)) {
		return true;
	}

	return false;
}

bool VehiclesOverlap(FCamPacket Vehicle1, FCamPacket Vehicle2, const float Time, const UWorld* world, const float ExtraFront, const float ExtraSide) {
	g_world = world;

	FVehiclePoints Veh1 = GetVehiclePoints(Vehicle1, Time, ExtraFront, ExtraSide);
	FVehiclePoints Veh2 = GetVehiclePoints(Vehicle2, Time);

	return
		PointInside(Veh1.A, Veh2)
		|| PointInside(Veh1.B, Veh2)
		|| PointInside(Veh1.C, Veh2)
		|| PointInside(Veh1.D, Veh2)

		|| PointInside(Veh2.A, Veh1)
		|| PointInside(Veh2.B, Veh1)
		|| PointInside(Veh2.C, Veh1)
		|| PointInside(Veh2.D, Veh1)

		|| SegmentsIntersect(Veh1.A, Veh1.B, Veh2.A, Veh2.B)

		|| SegmentsIntersect(Veh1.A, Veh1.B, Veh2.B, Veh2.C)

		|| SegmentsIntersect(Veh1.A, Veh1.B, Veh2.C, Veh2.D)

		|| SegmentsIntersect(Veh1.A, Veh1.B, Veh2.D, Veh2.A)


		|| SegmentsIntersect(Veh1.B, Veh1.C, Veh2.A, Veh2.B)

		|| SegmentsIntersect(Veh1.B, Veh1.C, Veh2.B, Veh2.C)

		|| SegmentsIntersect(Veh1.B, Veh1.C, Veh2.C, Veh2.D)

		|| SegmentsIntersect(Veh1.B, Veh1.C, Veh2.D, Veh2.A)


		|| SegmentsIntersect(Veh1.C, Veh1.D, Veh2.A, Veh2.B)

		|| SegmentsIntersect(Veh1.C, Veh1.D, Veh2.B, Veh2.C)

		|| SegmentsIntersect(Veh1.C, Veh1.D, Veh2.C, Veh2.D)

		|| SegmentsIntersect(Veh1.C, Veh1.D, Veh2.D, Veh2.A)


		|| SegmentsIntersect(Veh1.D, Veh1.A, Veh2.A, Veh2.B)

		|| SegmentsIntersect(Veh1.D, Veh1.A, Veh2.B, Veh2.C)

		|| SegmentsIntersect(Veh1.D, Veh1.A, Veh2.C, Veh2.D)

		|| SegmentsIntersect(Veh1.D, Veh1.A, Veh2.D, Veh2.A);

}

FVector ShortestVectorFromFront(FCamPacket Vehicle1, FCamPacket Vehicle2, const float Time, const UWorld* world) {
	g_world = world;
	FVehiclePoints Veh1 = GetVehiclePoints(Vehicle1, Time);
	FVehiclePoints Veh2 = GetVehiclePoints(Vehicle2, Time);

	FVector ZVector = Vehicle1.Location;
	ZVector.X = 0;
	ZVector.Y = 0;
	ZVector.Z += 500;

	/*DrawDebugPoint(world, Veh1.A + ZVector, 10, FColor::White, false, -1, 100);
	DrawDebugPoint(world, Veh1.B + ZVector, 10, FColor::White);

	DrawDebugCylinder(world, Veh1.A + ZVector, Veh2.A + ZVector, 10, 10, FColor::Red);

	DrawDebugPoint(world, Veh2.A + ZVector, 10, FColor::Red, false, -1, 100);
	DrawDebugPoint(world, Veh2.B + ZVector, 10, FColor::Red);
	DrawDebugPoint(world, Veh2.C + ZVector, 10, FColor::Red);
	DrawDebugPoint(world, Veh2.D + ZVector, 10, FColor::Red);*/

	FVector MinVector =
		SegmentsShortestVector(Veh1.A, Veh1.B, Veh2.A, Veh2.B);

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.A, Veh1.B, Veh2.B, Veh2.C));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.A, Veh1.B, Veh2.C, Veh2.D));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.A, Veh1.B, Veh2.D, Veh2.A));


	return MinVector;
}

FVector ShortestVectorBetweenLineAndVehicle(FVector LineA, FVector LineB, FCamPacket Vehicle, const float Time, const UWorld* world) {
	g_world = world;
	FVehiclePoints Veh = GetVehiclePoints(Vehicle, Time);

	FVector MinVector =
		SegmentsShortestVector(LineA, LineB, Veh.A, Veh.B);

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(LineA, LineB, Veh.B, Veh.C));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(LineA, LineB, Veh.C, Veh.D));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(LineA, LineB, Veh.D, Veh.A));

	return MinVector;
}

FVector ShortestVectorBetween(FCamPacket Vehicle1, FCamPacket Vehicle2, const float Time, const UWorld* world) {
	// We are not checking if they totally intersect. This is because that would mean that the vehicles already
	// have crashed. We check the intersection of each of the edges.

	g_world = world;

	FVehiclePoints Veh1 = GetVehiclePoints(Vehicle1, Time);
	FVehiclePoints Veh2 = GetVehiclePoints(Vehicle2, Time);

	/*DrawDebugSolidBox(world, Veh1.A, FVector(30, 30, 200), FColor::Black);
	DrawDebugSolidBox(world, Veh1.B, FVector(30, 30, 200), FColor::Red);
	DrawDebugSolidBox(world, Veh1.C, FVector(30, 30, 200), FColor::Red);
	DrawDebugSolidBox(world, Veh1.D, FVector(30, 30, 200), FColor::Red);

	DrawDebugSolidBox(world, Veh2.A, FVector(30, 30, 200), FColor::Green);
	DrawDebugSolidBox(world, Veh2.B, FVector(30, 30, 200), FColor::Green);
	DrawDebugSolidBox(world, Veh2.C, FVector(30, 30, 200), FColor::Green);
	DrawDebugSolidBox(world, Veh2.D, FVector(30, 30, 200), FColor::Green);*/


	FVector MinVector =
		SegmentsShortestVector(Veh1.A, Veh1.B, Veh2.A, Veh2.B);

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.A, Veh1.B, Veh2.B, Veh2.C));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.A, Veh1.B, Veh2.C, Veh2.D));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.A, Veh1.B, Veh2.D, Veh2.A));


	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.B, Veh1.C, Veh2.A, Veh2.B));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.B, Veh1.C, Veh2.B, Veh2.C));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.B, Veh1.C, Veh2.C, Veh2.D));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.B, Veh1.C, Veh2.D, Veh2.A));


	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.C, Veh1.D, Veh2.A, Veh2.B));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.C, Veh1.D, Veh2.B, Veh2.C));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.C, Veh1.D, Veh2.C, Veh2.D));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.C, Veh1.D, Veh2.D, Veh2.A));


	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.D, Veh1.A, Veh2.A, Veh2.B));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.D, Veh1.A, Veh2.B, Veh2.C));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.D, Veh1.A, Veh2.C, Veh2.D));

	MinVector = ShortestVector(MinVector,
		SegmentsShortestVector(Veh1.D, Veh1.A, Veh2.D, Veh2.A));


	return MinVector;


}
