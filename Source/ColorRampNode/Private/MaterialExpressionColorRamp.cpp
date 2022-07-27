#include "MaterialExpressionColorRamp.h"

#include "MaterialCompiler.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Curves/CurveLinearColor.h"

#define LOCTEXT_NAMESPACE "MateiralExpressionColorRamp"

// Custom Struct

FGradientColorPos::FGradientColorPos(FLinearColor InColor, float InPosition)
{
	Color = InColor;
	Position = InPosition;
}

// Init black to white default gradient color.
FColorStamp::FColorStamp()
{
	FGradientColorPos NewColor = FGradientColorPos();
	NewColor.Color = FLinearColor(0, 0, 0, 1);
	ColorPosArray.Add(NewColor);

	NewColor = FGradientColorPos();
	NewColor.Position = 1.f;
	ColorPosArray.Add(NewColor);
}

void FColorStamp::SetCurveLinearColor(TObjectPtr<UCurveLinearColor> CurveLinearColor, EColorRampType InterpType)
{
	if (CurveLinearColor.IsNull() || !IsValid(CurveLinearColor))
		return;
	
	FRichCurve NewFloatCurves[4];
	for (FGradientColorPos ColorPos : ColorPosArray)
	{
		FKeyHandle RKey = NewFloatCurves[0].AddKey(ColorPos.Position, ColorPos.Color.R);
		NewFloatCurves[0].SetKeyInterpMode(RKey, ERichCurveInterpMode(InterpType));
		
		FKeyHandle BKey = NewFloatCurves[1].AddKey(ColorPos.Position, ColorPos.Color.G);
		NewFloatCurves[1].SetKeyInterpMode(BKey, ERichCurveInterpMode(InterpType));
		
		FKeyHandle GKey = NewFloatCurves[2].AddKey(ColorPos.Position, ColorPos.Color.B);
		NewFloatCurves[2].SetKeyInterpMode(GKey, ERichCurveInterpMode(InterpType));
		
		FKeyHandle AKey = NewFloatCurves[3].AddKey(ColorPos.Position, ColorPos.Color.A);
		NewFloatCurves[3].SetKeyInterpMode(AKey, ERichCurveInterpMode(InterpType));
	}

	CurveLinearColor->FloatCurves[0] = NewFloatCurves[0];
	CurveLinearColor->FloatCurves[1] = NewFloatCurves[1];
	CurveLinearColor->FloatCurves[2] = NewFloatCurves[2];
	CurveLinearColor->FloatCurves[3] = NewFloatCurves[3];

	CurveOwner = CurveLinearColor;
}

bool FColorStamp::SetFromCurve(TObjectPtr<UCurveLinearColor> CurveLinearColor)
{
	if (CurveLinearColor.IsNull() || !IsValid(CurveLinearColor))
		return false;

	// Make sure Curve's keys num are same.
	if ((CurveLinearColor->FloatCurves[0].GetNumKeys() &
		CurveLinearColor->FloatCurves[1].GetNumKeys() &
		CurveLinearColor->FloatCurves[2].GetNumKeys()) != CurveLinearColor->FloatCurves[0].GetNumKeys())
		return false;

	ColorPosArray.Empty();

	int32 i = 0;
	for (FRichCurveKey Key : CurveLinearColor->FloatCurves[0].Keys)
	{
		ColorPosArray.Add(FGradientColorPos(FLinearColor(
			Key.Value,
			CurveLinearColor->FloatCurves[1].Keys[i].Value,
			CurveLinearColor->FloatCurves[2].Keys[i].Value,
			1.f), Key.Time));
		i++;
	}

	return true;
}

// UMaterialExpressionColorRamp

UMaterialExpressionColorRamp::UMaterialExpressionColorRamp(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MenuCategories.Add(LOCTEXT("MateiralExpressionColorRampCategory", "ColorRamp"));

	RefreshParameters();
}

TObjectPtr<UCurveLinearColor> UMaterialExpressionColorRamp::GetCurve()
{
	return TempCurvePtr;
}

void UMaterialExpressionColorRamp::RefreshTexture()
{
	bValidCurve = ColorStamp.SetFromCurve(TempCurvePtr);
	GenerateRampTex();
}

void UMaterialExpressionColorRamp::GetCaption(TArray<FString>& OutCaptions) const
{
	// Super::GetCaption(OutCaptions);
	OutCaptions.Add(TEXT("ColorRamp"));
}

int32 UMaterialExpressionColorRamp::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	// return Super::Compile(Compiler, OutputIndex);
	
	int32 Result = INDEX_NONE;
	if (Factor.GetTracedInput().Expression)
	{
		if (ColorStamp.ColorPosArray.Num() < 2)
		{
			Result = Compiler->Errorf(TEXT("At least two colors are required."));
			return Result;
		}

		RefreshParameters();
		// if (!TempRampTexPtr || !TempCurvePtr || !OnUpdateCurveHandle.IsValid())
		// {
		// 	RefreshParameters();
		// }
		
		Result = LinearRamp(Factor.Compile(Compiler), Compiler);

		if (!bValidCurve)
		{
			Result = Compiler->Errorf(TEXT("Private Curve was Invalid. Try recreate node."));
		}
	}
	else
	{
		Result = Compiler->Constant(ConstFac.GetLuminance());
	}
	
	return Result;
}

UObject* UMaterialExpressionColorRamp::GetReferencedTexture() const
{
	// return Super::GetReferencedTexture();
	// if (IsValid(TempRampTexPtr))
	return TempRampTexPtr;
}

void UMaterialExpressionColorRamp::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RefreshParameters();

	this->GetAssetOwner()->GetPackage()->MarkPackageDirty();
}

UMaterialExpressionColorRamp::~UMaterialExpressionColorRamp()
{
	if (IsValid(TempRampTexPtr))
	{
		TempRampTexPtr->RemoveFromRoot();
	}

	if (IsValid(TempCurvePtr))
	{
		TempCurvePtr->RemoveFromRoot();
	}
}

void UMaterialExpressionColorRamp::RefreshParameters()
{
	if (IsValid(this->GetAssetOwner()))
	{
		TempTextureName = "ColorRampTempTex_" + this->GetAssetOwner()->GetName() + "_" + this->GetName();
		TempCurveName = "ColorRampTempCurve_" + this->GetAssetOwner()->GetName() + "_" + this->GetName();
	
		ColorStamp.ColorPosArray.Sort();
		RefreshTexture();
		if (!IsValid(TempCurvePtr) || !OnUpdateCurveHandle.IsValid())
			GenerateRampCurve();

		ColorStamp.SetCurveLinearColor(TempCurvePtr, RampType);
	}
}

FColor UMaterialExpressionColorRamp::GetCurrentColor(int32 Pos)
{
	float Time = float(Pos) / float(Resolution);
	
	if (!bUseCustomCurveLinearColor)
	{
		for (int32 i = 0; i < ColorStamp.ColorPosArray.Num()-1; ++i)
		{
			if (Time <= ColorStamp.ColorPosArray[i].Position)
			{
				// if (RampType == CRT_LINEAR)
				return ColorStamp.ColorPosArray[i].Color.ToFColor(!bSRGB);
				// else if (RampType == CRT_CONSTANT)
				// 	return ColorStamp.ColorPosArray[i].Color.ToFColor(true);
			}
			if (ColorStamp.ColorPosArray[i].Position < Time && Time <= ColorStamp.ColorPosArray[i+1].Position)
			{
				if (RampType == CRT_LINEAR)
				{
					float Progress = (Time - ColorStamp.ColorPosArray[i].Position) / (ColorStamp.ColorPosArray[i+1].Position - ColorStamp.ColorPosArray[i].Position);
					FLinearColor Color = ColorStamp.ColorPosArray[i].Color * (1 - Progress) + ColorStamp.ColorPosArray[i+1].Color * Progress;
					return Color.ToFColor(!bSRGB);
				}
				else if (RampType == CRT_CONSTANT)
				{
					return ColorStamp.ColorPosArray[i].Color.ToFColor(true);
				}
			}
			// Last Color
			if (ColorStamp.ColorPosArray[i+1].Position < Time && i+1 == ColorStamp.ColorPosArray.Num()-1)
			{
				// if (RampType == CRT_LINEAR)
				return ColorStamp.ColorPosArray[i+1].Color.ToFColor(!bSRGB);
			}
		}
	}
	else if (IsValid(CustomCurveLinearColor))
	{
		return CustomCurveLinearColor->GetLinearColorValue(Time).ToFColor(true);
	}

	return FColor(0, 0, 0, 255);
}

void UMaterialExpressionColorRamp::GenerateRampTex(bool bInit)
{
	// todo: move to constructor
	UPackage* Package;
	FString PackageName = PackagePath + TempTextureName;
	Package = LoadPackage(nullptr, *PackageName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	if (!Package)
	{
		Package = CreatePackage(*PackageName);
	}
	// check(Package)
	Package->FullyLoad();

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *TempTextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewTexture->AddToRoot();
	
	FTexturePlatformData* Data = new FTexturePlatformData();
	Data->SizeX = Resolution;
	Data->SizeY = 1;
	Data->SetNumSlices(1);
	Data->PixelFormat = EPixelFormat::PF_B8G8R8A8;
	
	NewTexture->SetPlatformData(Data);

	uint8* Pixels = new uint8[Resolution * 1 * 4];
	if (!bInit)
	{
		for (int32 x = 0; x < Resolution; x++)
		{
			FColor Col = GetCurrentColor(x);
			Pixels[4 * x] = Col.B;
			Pixels[4 * x + 1] = Col.G;
			Pixels[4 * x + 2] = Col.R;
			
			Pixels[4 * x + 3] = 255;
		}
	}
	else
	{
		for (int32 x = 0; x < Resolution; x++)
		{
			Pixels[4 * x] = 0;
			Pixels[4 * x + 1] = 0;
			Pixels[4 * x + 2] = 0;
			Pixels[4 * x + 3] = 255;
		}
	}

	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->GetPlatformData()->Mips.Add(Mip);
	Mip->SizeX = Resolution;
	Mip->SizeY = 1;

	Mip->BulkData.Lock(LOCK_READ_WRITE);
	uint8* TextureData = (uint8*)Mip->BulkData.Realloc(Resolution * 4);
	FMemory::Memcpy(TextureData, Pixels, sizeof(uint8) * Resolution * 4);
	Mip->BulkData.Unlock();

	NewTexture->Source.Init(Resolution, 1, 1, 1, ETextureSourceFormat::TSF_BGRA8, Pixels);
	NewTexture->SRGB = 0;
	NewTexture->UpdateResource();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);
	// FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	// bool bSaved = UPackage::SavePackage(Package, NewTexture, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);

	TempRampTexPtr = NewTexture;

	delete[] Pixels;
}

void UMaterialExpressionColorRamp::GenerateRampCurve()
{
	if (IsValid(TempCurvePtr))
	{
		if (OnUpdateCurveHandle.IsValid())
			TempCurvePtr->OnUpdateCurve.Remove(OnUpdateCurveHandle);
	}
	
	UPackage* Package;
	FString PackageName = PackagePath + TempCurveName;
	Package = LoadPackage(nullptr, *PackageName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	if (!Package)
	{
		Package = CreatePackage(*PackageName);
	}
	// check(Package)
	Package->FullyLoad();

	UCurveLinearColor* NewCurve = NewObject<UCurveLinearColor>(Package, *TempCurveName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewCurve->AddToRoot();

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewCurve);

	TempCurvePtr = NewCurve;
	bValidCurve = true;

	OnUpdateCurveHandle = TempCurvePtr->OnUpdateCurve.AddUObject(this, &UMaterialExpressionColorRamp::OnUpdateCurve);
}


int32 UMaterialExpressionColorRamp::Luminance(int32 Input, FMaterialCompiler* Compiler)
{
	int32 RW = Compiler->Constant(0.299f);
	int32 GW = Compiler->Constant(0.578f);
	int32 BW = Compiler->Constant(0.114f);

	int32 R = Compiler->ComponentMask(Input, true, false, false, false);
	int32 G = Compiler->ComponentMask(Input, false, true, false, false);
	int32 B = Compiler->ComponentMask(Input, false, false, true, false);

	return Compiler->Add(
			Compiler->Add(
				Compiler->Mul(R, RW),
				Compiler->Mul(G, GW)),
				Compiler->Mul(B, BW));
}

int32 UMaterialExpressionColorRamp::LinearRamp(int32 Input, FMaterialCompiler* Compiler)
{
	if (!TempRampTexPtr)
	{
		return INDEX_NONE;
	}
	
	int32 Value = Luminance(Input, Compiler);
	int32 Coord = Compiler->AppendVector(Value, Compiler->Constant(0));
	int32 Tex = Compiler->Texture(TempRampTexPtr, EMaterialSamplerType::SAMPLERTYPE_Color);

	return Compiler->TextureSample(Tex, Coord, EMaterialSamplerType::SAMPLERTYPE_LinearColor);
}

#undef LOCTEXT_NAMESPACE
