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

#include "PIDController.h"

FPIDController::FPIDController(float ProportionalGain, float IntegralGain, float DerivativeGain, float Limit) {
	this->ProportionalGain = ProportionalGain;
	this->IntegralGain = IntegralGain;
	this->DerivativeGain = DerivativeGain;
	this->Limit = Limit;

	this->Integral = 0;
	this->PreviousError = 0;

	for (int i = 0; i < 5; i++) {
		PrevErrors[i] = 0.0f;
	}
}

float FPIDController::NextValue(float Sensed, float Desired, float delta) {
	// http://en.wikipedia.org/wiki/PID_controller#Discrete_implementation
	float Error = Desired - Sensed;

	// Low pass filter

	for (int i = 0; i < 5; i++) {
		PrevErrors[i] = PrevErrors[i + 1];
	}
	PrevErrors[4] = Error;

	Error = Error * 0.4 + PreviousError * 0.6;

	// Calculating the Integral part

	Integral += Error * delta;

	Integral = FMath::Max(-Limit / IntegralGain, Integral);
	Integral = FMath::Min(Limit / IntegralGain, Integral);

	// Calculating the Derivative part

	float Derivative = (Error - PreviousError) / delta;
	float Output =
		ProportionalGain * Error
		+ IntegralGain * Integral
		+ DerivativeGain * Derivative;

	PreviousError = Error;

	Output = FMath::Max(-Limit, Output);
	Output = FMath::Min(Limit, Output);

	return Output;
}
