// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#include "OptimizeTextureSize.h"
#include "ProductivityToolsSettings.h"
#include "Tools.h"

#include "ScopedTransaction.h"

DEFINE_LOG_CATEGORY(LogProductivityToolsOptimizeTextureSize);

#define LOCTEXT_NAMESPACE "FProductivityToolsModule"

TMap<UTexture*, int32> FOptimizeTextureSize::LODBiasesMap = TMap<UTexture*, int32>();

void FOptimizeTextureSize::OptimizeTextureSize(TArray<FAssetData> SelectedAssets, TArray<FString> SelectedPaths)
{
	SelectedAssets.Append(FTools::GetAssetsFromPaths(SelectedPaths, UTexture::StaticClass()));
	OptimizeTextureSize(SelectedAssets);
}

void FOptimizeTextureSize::OptimizeTextureSize(TArray<FString> SelectedPaths)
{
	OptimizeTextureSize(FTools::GetAssetsFromPaths(SelectedPaths, UTexture::StaticClass()));
}

void FOptimizeTextureSize::OptimizeTextureSize(TArray<FAssetData> SelectedAssets)
{
	const FScopedTransaction Transaction(LOCTEXT("OptimizeTextureSizeAction", "Optimize Texture Size"));

	LODBiasesMap = TMap<UTexture*, int32>();

	TArray<UTexture*> SelectedTextures = TArray<UTexture*>();
	for (FAssetData SelectedAsset : SelectedAssets)
	{
		SelectedTextures.Add((UTexture*)SelectedAsset.GetAsset());
	}

	// Retrieve all static meshes in Content folder
	TArray<FAssetData> StaticMeshesData = FTools::GetAssetsFromPath("/Game", UStaticMesh::StaticClass());

	for (FAssetData StaticMeshData : StaticMeshesData)
	{
		UStaticMesh* StaticMesh = (UStaticMesh*)(StaticMeshData.GetAsset());

		// Retrieve an approximative size of the mesh
		FVector MeshSize = StaticMesh->GetBounds().BoxExtent * 2.0f;

		// Retrieve all textures used by materials in this mesh
		TArray<UTexture*> Textures = GetUsedTextures(StaticMesh);

		// Check if the static mesh is not small enough to reduce its texture size
		if (SETTINGS->bReduceSizeForSmallObjectsOnly
			&& MeshSize.X > SETTINGS->MaximumSizeForSmallObjects.X  && MeshSize.Y > SETTINGS->MaximumSizeForSmallObjects.Y && MeshSize.Z > SETTINGS->MaximumSizeForSmallObjects.Z)
		{
			// If the mesh is too large, keep the textures as they are
			for (UTexture* Texture : Textures)
			{
				if (SelectedTextures.Contains(Texture))
				{
					SaveLODBias(Texture, Texture->LODBias);
				}
			}
					
			continue;
		}

		for (UTexture* Texture : Textures) 
		{
			if (SelectedTextures.Contains(Texture))
			{
				FIntPoint TextureResolution = FIntPoint((int32)Texture->GetSurfaceWidth(), (int32)Texture->GetSurfaceHeight());

				// Calculate the new LOD bias for each texture
				int32 NewLODBias = CalculateNewLODBias(MeshSize, TextureResolution);

				// Save the LOD Bias for this texture to compare with other LOD biases found (if the texture is used by several meshes)
				SaveLODBias(Texture, NewLODBias);
			}	
		}
	}

	// Finally update texture assets with their corresponding LOD bias
	UpdateLODBiases();
}

TArray<UTexture*> FOptimizeTextureSize::GetUsedTextures(UStaticMesh* StaticMesh)
{
	TArray<UTexture*> UsedTextures = TArray<UTexture*>();

	// For all materials used by this static mesh
	for (FStaticMaterial StaticMaterial : StaticMesh->StaticMaterials)
	{
		TArray<UTexture*> MaterialTextures;

		if (StaticMaterial.MaterialInterface->IsValidLowLevel())
		{
			// Retrieve all textures used by each material
			StaticMaterial.MaterialInterface->GetUsedTextures(MaterialTextures, EMaterialQualityLevel::Num, true, ERHIFeatureLevel::Num, true);
		}
		UsedTextures.Append(MaterialTextures);
	}

	return UsedTextures;
}

int32 FOptimizeTextureSize::CalculateNewLODBias(FVector MeshSize, FIntPoint TextureResolution)
{
	float MeshMeanSize = (MeshSize.X + MeshSize.Y + MeshSize.Z) / 3;

	float ResolutionMean = (TextureResolution.X + TextureResolution.Y) / 2;

	// Calculate a relation bewteen the mesh size and the texture resolution
	float Ratio = ResolutionMean / MeshMeanSize;

	// Retrieve the power of two just above this ratio
	int32 PowerOfTwoRatio = 0;
	while (FGenericPlatformMath::Pow(2, PowerOfTwoRatio) <= Ratio)
	{
		PowerOfTwoRatio++;
	}

	// Offset this power of two using the setting's value
	int32 NewLODBias = FGenericPlatformMath::Max(PowerOfTwoRatio - SETTINGS->LODBiasOffset, 0);

	// Check if the newly found LOD bias won't make the texture resolution lower than the one in the settings
	while ((TextureResolution.X >> NewLODBias) < SETTINGS->MinimumInGameResolution.X && (TextureResolution.Y >> NewLODBias) < SETTINGS->MinimumInGameResolution.Y)
	{
		// Decrease LOD bias until the texture's resolution is larger than the minimum resolution in the settings
		NewLODBias--;

		if (NewLODBias == 0) {
			break;
		}
	}

	return NewLODBias;
}

void FOptimizeTextureSize::SaveLODBias(UTexture* Texture, int32 NewLODBias)
{
	// Check if this texture was already used in another static mesh
	if (LODBiasesMap.Contains(Texture))
	{
		// If we found a different LOD bias for this texture, keep the smallest one
		if (LODBiasesMap[Texture] > NewLODBias)
		{
			LODBiasesMap.Add(Texture, NewLODBias);
		}
	}
	else
	{
		LODBiasesMap.Add(Texture, NewLODBias);
	}
}

void FOptimizeTextureSize::UpdateLODBiases()
{
	for (TPair<UTexture*, int32>& Pair : LODBiasesMap)
	{
		UTexture* Texture = Pair.Key;
		int32 NewLODBias = Pair.Value;

		// Verify if the new LOD bias is different than the one already specified for the textures
		if (SETTINGS->bAllowDecreaseLODBias && Texture->LODBias != NewLODBias
			|| !SETTINGS->bAllowDecreaseLODBias && Texture->LODBias < NewLODBias)
		{
			UE_LOG(LogProductivityToolsOptimizeTextureSize, Log, TEXT("Updating %s with LOD Bias of %d"), *Texture->GetName(), NewLODBias);
			Texture->LODBias = NewLODBias;

			// Set the Texture Asset as modified (mark asset changes as unsaved)
			Texture->Modify();
			Texture->UpdateCachedLODBias();
		}
	}
}

#undef LOCTEXT_NAMESPACE