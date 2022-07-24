// Copyright Epic Games, Inc. All Rights Reserved.

#include "ColorRampNode.h"
#include "GradientColorPosDetailCustomization.h"

#define LOCTEXT_NAMESPACE "FColorRampNodeModule"

void FColorRampNodeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.RegisterCustomPropertyTypeLayout("ColorStamp", FOnGetPropertyTypeCustomizationInstance::CreateLambda(
		[](){ return MakeShareable(new FGradientColorPosDetailCustomization); }));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FColorRampNodeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FColorRampNodeModule, ColorRampNode)