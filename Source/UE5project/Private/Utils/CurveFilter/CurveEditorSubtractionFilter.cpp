// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/CurveFilter/CurveEditorSubtractionFilter.h"

UCurveEditorSubtractionFilter::UCurveEditorSubtractionFilter()
{

}

void UCurveEditorSubtractionFilter::ApplyFilter_Impl(TSharedRef<FCurveEditor> InCurveEditor, const TMap<FCurveModelID, FKeyHandleSet>& InKeysToOperateOn, TMap<FCurveModelID, FKeyHandleSet>& OutKeysToSelect)
{
	TArray<FKeyHandle> KeyHandles;
	TArray<FKeyHandle> KeyHandlesToModify;
	TArray<FKeyPosition> SelectedKeyPositions;

	TArray<FKeyPosition> NewKeyPositions;

	for (const TTuple<FCurveModelID, FKeyHandleSet>& Pair : InKeysToOperateOn)
	{
		// first check if curve exists
		FCurveModel* Curve = InCurveEditor->FindCurve(Pair.Key);
		if (!Curve)
		{
			continue;
		}

		KeyHandles.Reset(Pair.Value.Num());
		KeyHandles.Append(Pair.Value.AsArray().GetData(), Pair.Value.Num());

		// Get all the selected keys
		SelectedKeyPositions.SetNum(KeyHandles.Num());
		Curve->GetKeyPositions(KeyHandles, SelectedKeyPositions);

		// Find the hull of the range of the selected keys
		double MinKey = TNumericLimits<double>::Max(), MaxKey = TNumericLimits<double>::Lowest();
		for (FKeyPosition Key : SelectedKeyPositions)
		{
			MinKey = FMath::Min(Key.InputValue, MinKey);
			MaxKey = FMath::Max(Key.InputValue, MaxKey);
		}

		// Get all keys that exist between the time range
		KeyHandles.Reset();
		Curve->GetKeys(*InCurveEditor, MinKey, MaxKey, TNumericLimits<double>::Lowest(), TNumericLimits<double>::Max(), KeyHandles);

		if (KeyHandles.Num() >= 1)
		{
			// get the selected key positions
			SelectedKeyPositions.SetNum(KeyHandles.Num());
			Curve->GetKeyPositions(KeyHandles, SelectedKeyPositions);

			// reset the old calculated key positions
			NewKeyPositions.Reset();
			KeyHandlesToModify.Reset();

			for (int32 KeyIndex = 0; KeyIndex < KeyHandles.Num(); ++KeyIndex)
			{
				// calculate the subtracted key
				float NextKeyVal;

				switch(Type)
				{
				case EArithmeticType::Addition:
					NextKeyVal = SelectedKeyPositions[KeyIndex].OutputValue + Value;
					break;
				case EArithmeticType::Subtraction:
					NextKeyVal = SelectedKeyPositions[KeyIndex].OutputValue - Value;
					break;
				case EArithmeticType::Division:
					NextKeyVal = SelectedKeyPositions[KeyIndex].OutputValue / Value;
					break;
				case EArithmeticType::Multiplation:
					NextKeyVal = SelectedKeyPositions[KeyIndex].OutputValue * Value;
					break;
				default:
					NextKeyVal = 0.0f;
					break;
				}

				SelectedKeyPositions[KeyIndex].OutputValue = NextKeyVal;

				NewKeyPositions.Add(SelectedKeyPositions[KeyIndex]);
				KeyHandlesToModify.Add(KeyHandles[KeyIndex]);
			}

			Curve->Modify();
			Curve->SetKeyPositions(KeyHandlesToModify, NewKeyPositions);
		}
	}
}