#include "CurveFilter/CurveEditorSubtractionFilter.h"

UCurveEditorSubtractionFilter::UCurveEditorSubtractionFilter()
{
}

void UCurveEditorSubtractionFilter::ApplyFilter_Impl(
	TSharedRef<FCurveEditor> InCurveEditor,
	const TMap<FCurveModelID, FKeyHandleSet>& InKeysToOperateOn,
	TMap<FCurveModelID, FKeyHandleSet>& OutKeysToSelect)
{
	TArray<FKeyHandle> KeyHandles;
	TArray<FKeyHandle> KeyHandlesToModify;
	TArray<FKeyPosition> SelectedKeyPositions;
	TArray<FKeyPosition> NewKeyPositions;

	for (const TTuple<FCurveModelID, FKeyHandleSet>& Pair : InKeysToOperateOn)
	{
		FCurveModel* Curve = InCurveEditor->FindCurve(Pair.Key);
		if (!Curve)
		{
			continue;
		}

		KeyHandles.Reset(Pair.Value.Num());
		KeyHandles.Append(Pair.Value.AsArray().GetData(), Pair.Value.Num());
		SelectedKeyPositions.SetNum(KeyHandles.Num());
		Curve->GetKeyPositions(KeyHandles, SelectedKeyPositions);

		double MinKey = TNumericLimits<double>::Max();
		double MaxKey = TNumericLimits<double>::Lowest();
		for (const FKeyPosition& Key : SelectedKeyPositions)
		{
			MinKey = FMath::Min(Key.InputValue, MinKey);
			MaxKey = FMath::Max(Key.InputValue, MaxKey);
		}

		KeyHandles.Reset();
		Curve->GetKeys(
			*InCurveEditor,
			MinKey,
			MaxKey,
			TNumericLimits<double>::Lowest(),
			TNumericLimits<double>::Max(),
			KeyHandles);

		if (KeyHandles.IsEmpty())
		{
			continue;
		}

		SelectedKeyPositions.SetNum(KeyHandles.Num());
		Curve->GetKeyPositions(KeyHandles, SelectedKeyPositions);
		NewKeyPositions.Reset(KeyHandles.Num());
		KeyHandlesToModify.Reset(KeyHandles.Num());

		for (int32 KeyIndex = 0; KeyIndex < KeyHandles.Num(); ++KeyIndex)
		{
			FKeyPosition NewPosition = SelectedKeyPositions[KeyIndex];
			switch (Type)
			{
			case EArithmeticType::Addition:
				NewPosition.OutputValue += Value;
				break;
			case EArithmeticType::Subtraction:
				NewPosition.OutputValue -= Value;
				break;
			case EArithmeticType::Division:
				NewPosition.OutputValue /= Value;
				break;
			case EArithmeticType::Multiplation:
				NewPosition.OutputValue *= Value;
				break;
			}

			NewKeyPositions.Add(NewPosition);
			KeyHandlesToModify.Add(KeyHandles[KeyIndex]);
		}

		if (!KeyHandlesToModify.IsEmpty())
		{
			Curve->Modify();
			Curve->SetKeyPositions(KeyHandlesToModify, NewKeyPositions);
		}
	}
}
