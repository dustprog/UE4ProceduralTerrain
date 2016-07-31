

#pragma once

#include "Terrain/Procedural/Heightmap_2D.h"
#include "GameFramework/Actor.h"
#include "Heightmap.generated.h"
using namespace ProceduralTerrain;

/**
 * Heightmap for the QuadTree
 */
UCLASS()
class FERO_API AHeightmap : public AActor
{
	GENERATED_UCLASS_BODY()
public:

};

class Heightmap
{
public:

	Heightmap(int32 width, int32 height);

	/** Loads the heightmap from a image file */
	void Load(FString heightmapName, int32 width, int32 height);

	/** Procedurally Loads a heightmap for a valley */
	void LoadValley();

	/** Procedurally Loads a double pass mountain range */
	void LoadDoublePass();

	/** Procedurally Loads a mountain range */
	void LoadMountains();

	/** Procedurally Loads a hilly map with a radial lake */
	void LoadLake();

	/** Procedurally Load a basic heightmap */
	void LoadBasic();

	/** Procedurally Load an archipelago heightmap */
	void LoadArchipelago();

	/** Sample a value on the heightmap */
	signed short Sample(unsigned int x, unsigned int y);

private:

	/** Width and Height of the heightmap */
	int32 m_Width;
	int32 m_Height;

	/** List of the height values */
	//TArray<signed short> m_Heightmap;
	
	Heightmap_2D* base;

	Amplitudes* m_std_noise;
	Heightmap_1D* m_natural_profile;
};
