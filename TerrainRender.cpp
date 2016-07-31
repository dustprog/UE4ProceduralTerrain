//=============================================================================================================================================
#include "ProceduralTerrainPrivatePCH.h"
#include "TessellationRendering.h"
//=============================================================================================================================================
//=============================================================================================================================================
/** Copy of function from TessellationRendering.cpp */
bool RequiresAdjacencyInformation(UMaterialInterface* Material, const FVertexFactoryType* VertexFactoryType)
{
	EMaterialTessellationMode TessellationMode = MTM_NoTessellation;
	bool bEnableCrackFreeDisplacement = false;
	if( RHISupportsTessellation( GRHIShaderPlatform ) && VertexFactoryType->SupportsTessellationShaders() && Material )
	{
		if( IsInRenderingThread() )
		{
			FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy( false, false );
			check( MaterialRenderProxy );
			const FMaterial* MaterialResource = MaterialRenderProxy->GetMaterial( GRHIFeatureLevel );
			check( MaterialResource );
			TessellationMode = MaterialResource->GetTessellationMode();
			bEnableCrackFreeDisplacement = MaterialResource->IsCrackFreeDisplacementEnabled();
		}
		else if( IsInGameThread() )
		{
			UMaterial* BaseMaterial = Material->GetMaterial();
			check( BaseMaterial );
			TessellationMode = (EMaterialTessellationMode) BaseMaterial->D3D11TessellationMode;
			bEnableCrackFreeDisplacement = BaseMaterial->bEnableCrackFreeDisplacement;
		}
		else
		{
			UMaterialInterface::TMicRecursionGuard RecursionGuard;
			const UMaterial* BaseMaterial = Material->GetMaterial_Concurrent( RecursionGuard );
			check( BaseMaterial );
			TessellationMode = (EMaterialTessellationMode) BaseMaterial->D3D11TessellationMode;
			bEnableCrackFreeDisplacement = BaseMaterial->bEnableCrackFreeDisplacement;
		}
	}

	return TessellationMode == MTM_PNTriangles || ( TessellationMode == MTM_FlatTessellation && bEnableCrackFreeDisplacement );
}
//=============================================================================================================================================
/** Render Data */
//=============================================================================================================================================
FTerrainRenderData::FTerrainRenderData()
{
}
//=============================================================================================================================================
void FTerrainRenderData::InitResources(UQuadTreeMesh* qtreemesh)
{
	HasTessellationData = qtreemesh->BuildTessellationData;
	DynamicTessellation = qtreemesh->DynamicTessellation;
	
	const FColor VertexColor(255, 255, 255);
	
	// For Dynamic Patch based tessellation
	if (DynamicTessellation)
	{
		// Add each quad to the vertex/index buffer
		for (int QuadIdx = 0; QuadIdx < qtreemesh->CustomMeshQuads.Num(); QuadIdx++)
		{
			FMeshQuad& Quad = qtreemesh->CustomMeshQuads[QuadIdx];
			
			FDynamicMeshVertex Vert0;
			Vert0.Position = Quad.Vertex0;
			Vert0.Color = VertexColor;
			int32 VIndex = VertexBuffer.Vertices.Add(Vert0);
			IndexBuffer.Indices.Add(VIndex);
			
			FDynamicMeshVertex Vert1;
			Vert1.Position = Quad.Vertex1;
			Vert1.Color = VertexColor;
			VIndex = VertexBuffer.Vertices.Add(Vert1);
			IndexBuffer.Indices.Add(VIndex);

			FDynamicMeshVertex Vert2;
			Vert2.Position = Quad.Vertex2;
			Vert2.Color = VertexColor;
			VIndex = VertexBuffer.Vertices.Add(Vert2);
			IndexBuffer.Indices.Add(VIndex);
			
			FDynamicMeshVertex Vert3;
			Vert3.Position = Quad.Vertex3;
			Vert3.Color = VertexColor;
			VIndex = VertexBuffer.Vertices.Add(Vert3);
			IndexBuffer.Indices.Add(VIndex);
		}
	}
	else// Triangle base tessellation
	{
		// Add each triangle to the vertex/index buffer
		for (int TriIdx = 0; TriIdx < qtreemesh->CustomMeshTris.Num(); TriIdx++)
		{
			FMeshTriangle& Tri = qtreemesh->CustomMeshTris[TriIdx];

			const FVector Edge01 = (Tri.Vertex1 - Tri.Vertex0);
			const FVector Edge02 = (Tri.Vertex2 - Tri.Vertex0);

			const FVector TangentX = Edge01.SafeNormal();
			const FVector TangentZ = (Edge02 ^ Edge01).SafeNormal();
			const FVector TangentY = (TangentX ^ TangentZ).SafeNormal();

			FDynamicMeshVertex Vert0;
			Vert0.Position = Tri.Vertex0;
			Vert0.Color = VertexColor;
			Vert0.SetTangents(TangentX, TangentY, TangentZ);
			int32 VIndex = VertexBuffer.Vertices.Add(Vert0);
			IndexBuffer.Indices.Add(VIndex);

			FDynamicMeshVertex Vert1;
			Vert1.Position = Tri.Vertex1;
			Vert1.Color = VertexColor;
			Vert1.SetTangents(TangentX, TangentY, TangentZ);
			VIndex = VertexBuffer.Vertices.Add(Vert1);
			IndexBuffer.Indices.Add(VIndex);

			FDynamicMeshVertex Vert2;
			Vert2.Position = Tri.Vertex2;
			Vert2.Color = VertexColor;
			Vert2.SetTangents(TangentX, TangentY, TangentZ);
			VIndex = VertexBuffer.Vertices.Add(Vert2);
			IndexBuffer.Indices.Add(VIndex);
		}
	}

	// Init vertex factory
	VertexFactory.Init(&VertexBuffer);

	// Enqueue initialization of render resource
	BeginInitResource(&VertexBuffer);
	BeginInitResource(&IndexBuffer);
	BeginInitResource(&VertexFactory);
	
	NumPrimitives = IndexBuffer.Indices.Num() / 3;
	MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;
}
//=============================================================================================================================================
void FTerrainRenderData::ReleaseResources()
{
	VertexBuffer.ReleaseResource();
	IndexBuffer.ReleaseResource();
	VertexFactory.ReleaseResource();
}
//=============================================================================================================================================
/** Scene Proxy */
//=============================================================================================================================================
FTerrainSceneProxy::FTerrainSceneProxy(UQuadTreeMesh* qtreemesh)
:  FPrimitiveSceneProxy(qtreemesh),
   MaterialRelevance(qtreemesh->GetMaterialRelevance()),
   Material( NULL ),
   BodySetup( qtreemesh->BodySetup ),
   RenderData( qtreemesh->RenderData ),
   LevelColor( 1,1,1 ),
   PropertyColor( 1,1,1 ),
   WireframeColor( qtreemesh->GetWireframeColor( ) )
{
	check( RenderData );
	
	// Grab material
	Material = qtreemesh->TerrainMaterial;
	if (Material == NULL)
	{
		Material = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	
	FMeshBatchElement& BatchElement = DynamicMesh.Elements[ 0 ];
	FCustomMeshBatchElementParams* BatchElementParams = new FCustomMeshBatchElementParams;
	BatchElementParams->HeightmapTextureResource = RenderData->HeightmapResource;
	BatchElement.UserData = BatchElementParams;
}
//=============================================================================================================================================
FTerrainSceneProxy::~FTerrainSceneProxy()
{
	if (RenderData != NULL) { delete RenderData; }
}
//=============================================================================================================================================
FPrimitiveViewRelevance FTerrainSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	
	Result.bDrawRelevance = IsShown( View );
	Result.bShadowRelevance = IsShadowCast( View );
	Result.bDynamicRelevance = true;
	MaterialRelevance.SetPrimitiveViewRelevance( Result );

	return Result;
}
//=============================================================================================================================================
bool FTerrainSceneProxy::CanBeOccluded() const
{
	return !MaterialRelevance.bDisableDepthTest;
}
//=============================================================================================================================================
uint32 FTerrainSceneProxy::GetMemoryFootprint() const
{
	return ( sizeof( *this ) + GetAllocatedSize( ) );
}
//=============================================================================================================================================
uint32 FTerrainSceneProxy::GetAllocatedSize() const
{
	return ( FPrimitiveSceneProxy::GetAllocatedSize( ) );
}
//=============================================================================================================================================
bool FTerrainSceneProxy::IsCollisionView(const FSceneView* View, bool& bDrawSimpleCollision, bool& bDrawComplexCollision) const
{
	bDrawSimpleCollision = bDrawComplexCollision = false;

	const bool bInCollisionView = View->Family->EngineShowFlags.CollisionVisibility || View->Family->EngineShowFlags.CollisionPawn;
	if( bInCollisionView && IsCollisionEnabled( ) )
	{
		bool bHasResponse = View->Family->EngineShowFlags.CollisionPawn && CollisionResponse.GetResponse( ECC_Pawn ) != ECR_Ignore;
		bHasResponse |= View->Family->EngineShowFlags.CollisionVisibility && CollisionResponse.GetResponse( ECC_Visibility ) != ECR_Ignore;

		if( bHasResponse )
		{
			bDrawComplexCollision = View->Family->EngineShowFlags.CollisionVisibility;
			bDrawSimpleCollision = View->Family->EngineShowFlags.CollisionPawn;
		}
	}

	return bInCollisionView;
}
//=============================================================================================================================================
void FTerrainSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_CustomMeshSceneProxy_DrawDynamicElements);

	const bool bWireframe = AllowDebugViewmodes() && View->Family->EngineShowFlags.Wireframe;
	
	bool bDrawSimpleCollision = false, bDrawComplexCollision = false;
	bool bProxyIsSelected = IsSelected( );
	const bool bInCollisionView = IsCollisionView( View, bDrawSimpleCollision, bDrawComplexCollision );

	const bool bDrawMesh = ( bInCollisionView ) ? ( bDrawComplexCollision ) : true;
	
	// I want dynamic hardware tessellation so the vertices of a quad is the only thing needed
	
	// Should Mesh be drawn
	if( bDrawMesh )
	{
		// Check to see if adjacency data is required
		//const bool bRequiresAdjacencyInformation = RequiresAdjacencyInformation( Material, RenderData->VertexFactory.GetType() );
		
		// Build the render mesh data
		FMeshBatchElement& BatchElement = DynamicMesh.Elements[ 0 ];
		DynamicMesh.bWireframe = bWireframe;
		DynamicMesh.VertexFactory = &RenderData->VertexFactory;
		DynamicMesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
		DynamicMesh.Type = PT_TriangleList;
		DynamicMesh.DepthPriorityGroup = SDPG_World;

		BatchElement.IndexBuffer = &RenderData->IndexBuffer;
		BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate( GetLocalToWorld(), GetBounds(), GetLocalBounds(), true );
		BatchElement.FirstIndex = 0;
		BatchElement.NumPrimitives = RenderData->NumPrimitives;
		BatchElement.MinVertexIndex = 0;
		BatchElement.MaxVertexIndex = RenderData->MaxVertexIndex;
		
		// Use 4 Point Control Patch List for rendering the tessellated mesh
		if( RenderData->HasTessellationData && RenderData->DynamicTessellation )
		{
			// Use adjacency index buffer instead
			DynamicMesh.Type = PT_4_ControlPointPatchList;
			//BatchElement.IndexBuffer = &RenderData->AdjacencyIndexBuffer;
			//BatchElement.FirstIndex *= 4;
		}
		
		// Wireframe view
		if( AllowDebugViewmodes() && bWireframe && !View->Family->EngineShowFlags.Materials )
		{
			FColoredMaterialRenderProxy WireframeMaterialInstance(
				GEngine->WireframeMaterial->GetRenderProxy( false ),
				GetSelectionColor( WireframeColor, !( GIsEditor && View->Family->EngineShowFlags.Selection ) || bProxyIsSelected, IsHovered( ), false )
				);

			DynamicMesh.bWireframe = true;
			DynamicMesh.MaterialRenderProxy = &WireframeMaterialInstance;

			/* Draw wireframe mesh */
			const int32 NumPasses = PDI->DrawMesh( DynamicMesh );

			//INC_DWORD_STAT_BY( STAT_TerrainTriangles, DynamicMesh.GetNumPrimitives() * NumPasses );
		}
		else
		{
			const FLinearColor UtilColor( LevelColor );

			// If in complex collision view
			if( bInCollisionView && bDrawComplexCollision && AllowDebugViewmodes() )
			{
				const FColoredMaterialRenderProxy CollisionMaterialInstance(
					GEngine->ShadedLevelColorationUnlitMaterial->GetRenderProxy( bProxyIsSelected, IsHovered() ),
					WireframeColor
					);

				DynamicMesh.MaterialRenderProxy = &CollisionMaterialInstance;

				// Draw mesh
				const int32 NumPasses = DrawRichMesh(
					PDI,
					DynamicMesh,
					WireframeColor,
					UtilColor,
					PropertyColor,
					this,
					bProxyIsSelected,
					bWireframe
					);

				//INC_DWORD_STAT_BY( STAT_TerrainTriangles, DynamicMesh.GetNumPrimitives() * NumPasses );
			}
			else
			{
				DynamicMesh.MaterialRenderProxy = Material->GetRenderProxy( bProxyIsSelected, IsHovered( ) );

				// Draw mesh as standard
				const int32 NumPasses = DrawRichMesh(
					PDI,
					DynamicMesh,
					WireframeColor,
					UtilColor,
					PropertyColor,
					this,
					bProxyIsSelected,
					bWireframe
					);

				//INC_DWORD_STAT_BY( STAT_TerrainTriangles, DynamicMesh.GetNumPrimitives() * NumPasses );
			}
		}
	}
	
	/* If in simple collision view */
	if( ( bDrawSimpleCollision ) && AllowDebugViewmodes( ) )
	{
		if( BodySetup )
		{
			if( FMath::Abs( GetLocalToWorld( ).Determinant( ) ) < SMALL_NUMBER )
			{
			}
			else
			{
				const FColoredMaterialRenderProxy SolidMaterialInstance(
					GEngine->ShadedLevelColorationUnlitMaterial->GetRenderProxy( IsSelected(), IsHovered() ),
					WireframeColor
					);

				/* Draw physics body as solid */
				FTransform GeomTransform( GetLocalToWorld( ) );
				BodySetup->AggGeom.DrawAggGeom( PDI, GeomTransform, WireframeColor, &SolidMaterialInstance, false, true );
			}
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	/* Render mesh bounds */
	RenderBounds( PDI, View->Family->EngineShowFlags, GetBounds( ), IsSelected( ) );
#endif
	
	/*
	FColoredMaterialRenderProxy WireframeMaterialInstance(
		GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
		FLinearColor(0, 0.5f, 1.f)
		);

	FMaterialRenderProxy* MaterialProxy = NULL;
	if (bWireframe)
	{
		MaterialProxy = &WireframeMaterialInstance;
	}
	else
	{
		MaterialProxy = Material->GetRenderProxy(IsSelected());
	}

	// Draw the mesh.
	FMeshBatch Mesh;
	FMeshBatchElement& BatchElement = Mesh.Elements[0];
	BatchElement.IndexBuffer = &IndexBuffer;
	Mesh.bWireframe = bWireframe;
	Mesh.VertexFactory = &VertexFactory;
	Mesh.MaterialRenderProxy = MaterialProxy;
	BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true);
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;
	Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
	Mesh.Type = PT_TriangleList;
	Mesh.DepthPriorityGroup = SDPG_World;
	PDI->DrawMesh(Mesh);
	*/
}
//=============================================================================================================================================