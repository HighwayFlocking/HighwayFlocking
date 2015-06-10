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

#include "Widget.h"
#include "TextBlock.h"
#include "PanelWidget.h"
#include "SpinBox.h"
#include "GridSlot.h"
#include "Behaviours/Behaviours.h"

#include "BehavioursWidget.h"

UBehavioursWidget::UBehavioursWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {
	float a = 1 + 1;
}

void UBehavioursWidget::PostLoad() {
	Super::PostLoad();
}

TSharedRef<SWidget> UBehavioursWidget::RebuildWidget() {

	ClearChildren();

	UWorld* World = GetWorld();

	if (World == NULL) {
		return Super::RebuildWidget();
	}

	ColumnFill.Empty();
	ColumnFill.Add(0.0f);
	ColumnFill.Add(1.0f);

	UObject* Outer = World->GetGameInstance() ? (UObject*)World->GetGameInstance() : (UObject*)World;

	ABehaviours* Behaviours = NULL;

	for (TActorIterator<ABehaviours> ObjIt(GetWorld()); ObjIt; ++ObjIt) {
		Behaviours = *ObjIt;
		break;
	}

	if (Behaviours == NULL) {
		return Super::RebuildWidget();
	}

	ValueChangedEvents.Empty();

	for (int i = 0; i < Behaviours->Behaviours.Num(); i++) {
		UBehaviour* Behaviour = Behaviours->Behaviours[i];

		UTextBlock* text = ConstructObject<UTextBlock>(UTextBlock::StaticClass(), Outer);

		UValueChangedEvent* ValueChangedEvent = ConstructObject<UValueChangedEvent>(UValueChangedEvent::StaticClass());

		ValueChangedEvent->BehavioursWidget = this;
		ValueChangedEvent->Behaviour = Behaviour;
		ValueChangedEvents.Add(ValueChangedEvent);

		{
			text->SetText(FText::FromString(Behaviour->GetClass()->GetName() + ": "));
			UGridSlot* slot = AddChildToGrid((UWidget*)text);
			slot->Row = i;
			slot->Column = 0;
		}

		{
			USpinBox* SpinBox = ConstructObject<USpinBox>(USpinBox::StaticClass(), Outer);
			SpinBox->SetMinValue(0.0f);
			SpinBox->Delta = 0.01f;
			SpinBox->Value = Behaviour->Weight;
			UGridSlot* slot = AddChildToGrid((UWidget*)SpinBox);
			slot->Row = i;
			slot->Column = 1;

			SpinBox->OnValueChanged.AddDynamic(ValueChangedEvent, &UValueChangedEvent::OnValueChanged);


		}


	}

	return Super::RebuildWidget();

}

void UBehavioursWidget::ValueChanged(UBehaviour* Behaviour, float Value) {
	Behaviour->Weight = Value;
}
