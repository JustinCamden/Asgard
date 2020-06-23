// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Asgard.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

DEFINE_LOG_CATEGORY(Asgard);

void FAsgardModule::StartupModule()
{
#if (ENGINE_MINOR_VERSION >= 21)    
    FString ShaderDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
    AddShaderSourceDirectoryMapping("/Project", ShaderDirectory);
#endif
    UE_LOG(Asgard, Warning, TEXT("Asgard: Module started"));
}

void FAsgardModule::ShutdownModule()
{
#if (ENGINE_MINOR_VERSION >= 21)    
    ResetAllShaderSourceDirectoryMappings();
#endif
    UE_LOG(Asgard, Warning, TEXT("Asgard: Module shutdown"));
}

IMPLEMENT_PRIMARY_GAME_MODULE(FAsgardModule, Asgard, "Asgard" );