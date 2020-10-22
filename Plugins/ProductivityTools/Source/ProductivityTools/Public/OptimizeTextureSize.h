// Copyright (c) 2019 Isara Technologies. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProductivityToolsOptimizeTextureSize, Log, All);

class FOptimizeTextureSize
{
private:
	static TMap<UTexture*, int32> LODBiasesMap;

public:

	/**
	* Function called by the Optimize Texture Size button
	* Reduce LOD bias for the needed textures used by static meshes in the content browser
	* Modify only selected textures in the content browser or in selected paths
	*
	* @param	SelectedAssets		The Selected Texture Assets in the content browser
	* @param	SelectedPaths		The Selected Paths in the content browser
	*/
	static void OptimizeTextureSize(TArray<FAssetData> SelectedAssets, TArray<FString> SelectedPaths);
	
	/**
	* Reduce LOD bias for the needed textures used by static meshes in the content browser
	*
	* @param	SelectedPaths		The Selected Paths in the content browser
	*/
	static void OptimizeTextureSize(TArray<FString> SelectedPaths);

	/**
	* Reduce LOD bias for the needed textures used by static meshes in the content browser
	*
	* @param	SelectedAssets		The Selected Texture Assets in the content browser
	*/
	static void OptimizeTextureSize(TArray<FAssetData> SelectedAssets);

	

	/**
	* Retrieve the texture assets used by the specified static mesh
	*
	* @param	StaticMesh		The static mesh to retrieve the textures from
	*
	* @return	An array of textures used by materials in the static mesh
	*/
	static TArray<UTexture*> GetUsedTextures(UStaticMesh* StaticMesh);

	/**
	* Find a corresponding LOD bias using the mesh size and the texture resolution
	* The return value can be offset in the settings
	*
	* @param	MeshSize			The approximative size of the mesh
	* @param	TextureResolution	The resolution of the texture
	*
	* @return	The LOD bias to use for the specified mesh size and texture resolution
	*/
	static int32 CalculateNewLODBias(FVector MeshSize, FIntPoint TextureResolution);

	/**
	* Save the specified LOD bias with the corresponding texture in order to update it later
	* If multiple LOD bias are saved with the same texture, the lowest one is kept
	*
	* @param	Texture		The texture to change its LOD bias
	* @param	NewLODBias	The new value for the LOD bias
	*/
	static void SaveLODBias(UTexture* Texture, int32 NewLODBias);

	/**
	* Update and mark as modified the previously saved textures with their new LOD bias
	*/
	static void UpdateLODBiases();
};
