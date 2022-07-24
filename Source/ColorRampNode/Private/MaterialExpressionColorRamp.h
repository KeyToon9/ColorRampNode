#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpression.h"

#include "MaterialExpressionColorRamp.generated.h"

UENUM()
enum EColorRampType
{
	CRT_LINEAR		UMETA(DisplayName = "Linear"),
	CRT_CONSTANT	UMETA(DisplayName = "Constant")
};

USTRUCT()
struct FGradientColorPos
{
	GENERATED_BODY()

	FGradientColorPos() : Color(FLinearColor(1, 1, 1, 1)), Position(0.f) {}

	FGradientColorPos(FLinearColor InColor, float InPosition);

	UPROPERTY(EditAnywhere)
	FLinearColor Color;

	UPROPERTY(EditAnywhere, meta=(ClampMax=1, ClampMin=0, UIMax=1, UIMin=0))
	float Position;

	FORCEINLINE bool operator<(const FGradientColorPos& OtherPos) const
	{
		return Position < OtherPos.Position;
	}
};

USTRUCT()
struct FColorStamp
{
	GENERATED_BODY()

	FColorStamp();

	UPROPERTY(EditAnywhere)
	TArray<FGradientColorPos> ColorPosArray;

	UPROPERTY()
	TObjectPtr<UCurveLinearColor> CurveOwner;

	// Use FColorStamp Data to set UCurveLinearColor value
	void SetCurveLinearColor(TObjectPtr<UCurveLinearColor> CurveLinearColor, EColorRampType InterpType);

	bool SetFromCurve(TObjectPtr<UCurveLinearColor> CurveLinearColor);
};

UCLASS(DisplayName="ColorRamp")
class COLORRAMPNODE_API UMaterialExpressionColorRamp : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(meta = (RequiredInput = "false"))
	FExpressionInput Factor;

	UPROPERTY(EditAnywhere, Category=MateiralExpressionColorRamp)
	TEnumAsByte<EColorRampType> RampType = CRT_LINEAR;
	
	/** only used if Factor is not hooked up */
	UPROPERTY(EditAnywhere, Category=MateiralExpressionColorRamp, meta=(OverridingInputProperty = "Factor"))
	FLinearColor ConstFac;

	UPROPERTY(EditAnywhere, Category=MateiralExpressionColorRampGradient, DisplayName="sRGB")
	bool bSRGB = false;

	UPROPERTY(EditAnywhere, Category=MateiralExpressionColorRampGradient, meta=(ToolTip = "Only show linear color gradient."))
	FColorStamp ColorStamp;

	UPROPERTY(EditAnywhere, Category=MateiralExpressionColorRampGradient, AdvancedDisplay, meta=(ToolTip = "Color Gradient Texture Width"))
	int32 Resolution = 1024;

	TObjectPtr<UCurveLinearColor> GetCurve();

	void RefreshTexture();

	virtual void GetCaption(TArray<FString>& OutCaptions) const override;

	virtual int32 Compile(FMaterialCompiler* Compiler, int32 OutputIndex) override;

	virtual UObject* GetReferencedTexture() const override;
	virtual bool CanReferenceTexture() const override { return true; }

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual ~UMaterialExpressionColorRamp() override;

private:
	// TODO: set in plugin settings
	FString TempTextureName = TEXT("ColorRampTempTex_");
	FString TempCurveName = TEXT("ColorRampTempCurve_");
	FString PackagePath = TEXT("/Game/");

	// Use UPROPERTY to save TempRampTex reference
	UPROPERTY()
	TObjectPtr<UTexture2D> TempRampTexPtr;

	UPROPERTY()
	TObjectPtr<UCurveLinearColor> TempCurvePtr;

	bool bValidCurve = false;

	FDelegateHandle OnUpdateCurveHandle;
	void RefreshParameters();

	FColor GetCurrentColor(int32 Pos);
	void GenerateRampTex(bool bInit = false);

	void GenerateRampCurve();
	
	int32 Luminance(int32 Input, FMaterialCompiler* Compiler);
	
	int32 LinearRamp(int32 Input, FMaterialCompiler* Compiler);

	void OnUpdateCurve(UCurveBase* , EPropertyChangeType::Type );
};

inline void UMaterialExpressionColorRamp::OnUpdateCurve(UCurveBase* , EPropertyChangeType::Type )
{
	RefreshParameters();
}
