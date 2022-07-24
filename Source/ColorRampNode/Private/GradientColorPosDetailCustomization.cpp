#include "GradientColorPosDetailCustomization.h"
#include "DetailWidgetRow.h"
#include "MaterialExpressionColorRamp.h"
#include "Curves/CurveLinearColor.h"

#include "SCustomColorGradientEditor.h"

#define LOCTEXT_NAMESPACE "MateiralExpressionColorRamp"

void FGradientColorPosDetailCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	SAssignNew(GradientEditor, SCustomColorGradientEditor)
	.ViewMaxInput(1.f);

	TArray<UObject*> OutterObjects;
	PropertyHandle->GetOuterObjects(OutterObjects);

	UMaterialExpressionColorRamp* MaterialExpressionColorRamp = nullptr;
	if (OutterObjects.Num())
	{
		MaterialExpressionColorRamp = Cast<UMaterialExpressionColorRamp>(OutterObjects[0]);
	}

	if (IsValid(MaterialExpressionColorRamp))
	{
		UCurveLinearColor* Curve = MaterialExpressionColorRamp->GetCurve();
		if (Curve)
		{
			GradientEditor->SetCurveOwner(Curve);
			GradientEditor->SetUseSRGB(MaterialExpressionColorRamp->bSRGB);
			MaterialExpressionColorRamp->RefreshTexture();
		}
	}
	
	HeaderRow
	.NameContent()
	[
		SNew(STextBlock)
		.Text(FText(LOCTEXT("GradientColorEditorHeaderName", "Color Stamp")))
	]
	.ValueContent()
	[
		GradientEditor.ToSharedRef()
	];
}

void FGradientColorPosDetailCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

#undef LOCTEXT_NAMESPACE