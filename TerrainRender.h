//=============================================================================================================================================
#pragma once
//=============================================================================================================================================
#include "../Public/ProceduralTerrainEngine.h"
#include "QuadTreeMesh.h"
#include "Engine.h"
#include "Core.h"
//=============================================================================================================================================
//=============================================================================================================================================
class FTerrainRenderData
{
public:
	
	/** Default Constructor */
	FTerrainRenderData();
	
	/** Initialize resources */
	void InitResources(UQuadTreeMesh* qtreemesh);
	
	/** Release any resources */
	void ReleaseResources();
	
	/** Main Vertex Buffer */
	FCustomMeshVertexBuffer VertexBuffer;

	/** Main Index Buffer */
	FCustomMeshIndexBuffer IndexBuffer;
	
	/** Main Vertex Factory*/
	FCustomMeshVertexFactory VertexFactory;
	
	/** Main height map texture */
	FTexture2DRHIRef HeightmapResource;
	
	/** Has adjanceny buffer created */
	bool HasTessellationData;
	
	/** Are we using dynamic tessellation */
	bool DynamicTessellation;
	
	uint32 NumPrimitives;
	uint32 MaxVertexIndex;
};
//=============================================================================================================================================
//=============================================================================================================================================
/** Scene proxy */
class FTerrainSceneProxy : public FPrimitiveSceneProxy
{
	friend class FTerrainRenderData;
	
public:
	
	/** Initialization constructor */
	FTerrainSceneProxy(UQuadTreeMesh* qtreemesh);

	/** Virtual destructor */
	virtual ~FTerrainSceneProxy();

	/* Begin UPrimitiveSceneProxy interface */
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View);
	virtual bool CanBeOccluded() const override;
	virtual uint32 GetMemoryFootprint() const;
	uint32 GetAllocatedSize() const;
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View);
	/* End UPrimitiveSceneProxy interface */

	/* Set data sent from the game thread */
	//void SetDynamicData_RenderThread(FFluidSurfaceDynamicData* NewDynamicData);
	
	/** Render data */
	FTerrainRenderData* RenderData;
	
protected:

	bool IsCollisionView(const FSceneView* View, bool& bDrawSimpleCollision, bool& bDrawComplexCollision) const;
	
private:

	/** Current material */
	UMaterialInterface* Material;

	/** Material relevance */
	FMaterialRelevance MaterialRelevance;
	
	/** Physics body */
	UBodySetup* BodySetup;

	/** Mesh data */
	FMeshBatch DynamicMesh;
	
	/** Colors */
	FLinearColor LevelColor;
	FLinearColor PropertyColor;
	const FLinearColor WireframeColor;

	/** Collision response of this component */
	FCollisionResponseContainer CollisionResponse;
};
//=============================================================================================================================================