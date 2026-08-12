#pragma once

#include "CoreMinimal.h"
#include "Filters/CurveEditorFilterBase.h"
#include "CurveEditorTailToZeroFilter.generated.h"

UCLASS()
class UE5PROJECTEDITOR_API UCurveEditorTailToZeroFilter : public UCurveEditorFilterBase
{
	GENERATED_BODY()

public:
	UCurveEditorTailToZeroFilter();
};
