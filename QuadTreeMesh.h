//=============================================================================================================================================
#pragma once
//=============================================================================================================================================
#include "ProceduralTerrainPrivatePCH.h"
#include "Components/MeshComponent.h"
#include "QuadTreeMesh.generated.h"
//=============================================================================================================================================
//=============================================================================================================================================
#define ROOT3OVER2			(0.866025f)
#define MESHBOXHEIGHT		(5)
//=============================================================================================================================================
UENUM()
namespace EGridType
{
	enum Type
	{
		GT_Square		= 0,
		GT_Hexagonal	= 1,
		GT_MAX			= 2,
	};
}
//=============================================================================================================================================
// Stores a range of values
//=============================================================================================================================================
USTRUCT( BlueprintType )
struct FRangedValues
{
	GENERATED_USTRUCT_BODY()

	/** Minimum value of range */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Range" )
	float MinValue;

	/** Maximum value of range */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Range" )
	float MaxValue;

	/** Get a random number from within the range */
	float GetRand()
	{
		return FMath::FRandRange( MinValue, MaxValue );
	}
};
//=============================================================================================================================================
USTRUCT(BlueprintType)
struct FMeshQuad
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = Quad)
	FVector Vertex0;

	UPROPERTY(EditAnywhere, Category = Quad)
	FVector Vertex1;

	UPROPERTY(EditAnywhere, Category = Quad)
	FVector Vertex2;
	
	UPROPERTY(EditAnywhere, Category = Quad)
	FVector Vertex3;
};
//=============================================================================================================================================
USTRUCT(BlueprintType)
struct FMeshTriangle
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = Triangle)
	FVector Vertex0;

	UPROPERTY(EditAnywhere, Category = Triangle)
	FVector Vertex1;

	UPROPERTY(EditAnywhere, Category = Triangle)
	FVector Vertex2;
};
//=============================================================================================================================================
//
// Component that allows you to specify custom triangle mesh geometry for the Terrain
//
UCLASS(ClassGroup = ( Rendering, Common ), hidecategories = (Object, LOD, Physics, Collision, Activation, "Components|Activation"), editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class UQuadTreeMesh : public UMeshComponent
{
	GENERATED_UCLASS_BODY()
	
	/** Set the geometry to use on this triangle mesh */
	UFUNCTION(BlueprintCallable, Category = "QuadTreeMesh")
	bool SetGeneratedMeshTriangles(const TArray<FMeshTriangle>& Triangles);
	
	/** Set the geometry to use on this quad mesh */
	UFUNCTION(BlueprintCallable, Category = "QuadTreeMesh")
	bool SetGeneratedMeshQuads(const TArray<FMeshQuad>& Quads);
	
	/** Color to use when displayed in wireframe */
	FColor GetWireframeColor() const;
	
	/* Begin UPrimitiveComponent interface */
	virtual FPrimitiveSceneProxy* CreateSceneProxy( ) override;
	virtual class UBodySetup* GetBodySetup( ) override;
	// DONT KNOW IF NEED ReceiveComponentDamage
	//virtual void ReceiveComponentDamage( float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser ) override;
	virtual void CreatePhysicsState() override;
	/* End UPrimitiveComponent interface*/
	
	// Begin UMeshComponent interface.
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial( int32 MaterialIndex ) const override;
	// End UMeshComponent interface.
	
	// Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	// Begin USceneComponent interface.
	
	/* Perform initialization */
	void Init();

	/* Get nearest index */
	void GetNearestIndex( const FVector& Pos, int& xIndex, int& yIndex );

	/* Update the physics body */
	void UpdateBody();

	/* Get World to Component transform */
	FORCEINLINE FTransform GetWorldToComponent( ) const { return FTransform( ComponentToWorld.ToMatrixWithScale( ).Inverse( ) ); }
	
	/* Delegates */
	UFUNCTION( )
	void ComponentTouched( AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult );
	
	/* Begin UObject interace */
	virtual void BeginDestroy() override;
	
	/** Triangles for the mesh */
	TArray<FMeshTriangle> CustomMeshTris;
	
	/** Quads for Dynamic mesh tessellation */
	TArray<FMeshQuad> CustomMeshQuads;
	
	friend class FCustomMeshSceneProxy;
	
public:
	
	/** Render data */
	TScopedPointer<class FTerrainRenderData> RenderData;
	
	/** Bounding box of the terrain mesh */
	FBox BoundingBox;
	
	/** Origin of mesh surface */
	FVector Origin;
	
	/** Grid type */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "QuadTreeMesh" )
	TEnumAsByte<EGridType::Type> MeshGridType;
	
	/** Distance between grid points */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "QuadTreeMesh" )
	float GridSpacing;
	
	/** Vertical scale factor */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "QuadTreeMesh" )
	float HeightScale;
	
	/** Num vertices in the X direction */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "QuadTreeMesh", Meta = ( ClampMin = "32", UIMin = "32", UIMax = "4096" ) )
	int32 MeshXSize;
	
	/** Num vertices in the Y direction */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "QuadTreeMesh", Meta = ( ClampMin = "32", UIMin = "32", UIMax = "4096" ) )
	int32 MeshYSize;
	
	/** Collision */
	UPROPERTY( Transient, DuplicateTransient )
	class UBodySetup* BodySetup;

	UPROPERTY( EditAnywhere, Category = "QuadTreeMesh" )
	class UMaterialInterface* TerrainMaterial;
	
	/** Controls wether or not to tick this component (Editor only) */
	UPROPERTY( EditAnywhere, Category = "QuadTreeMesh" )
	bool UpdateComponent;

	/** Weather or not to build tessellation data */
	UPROPERTY( EditAnywhere, Category = "QuadTreeMesh" )
	bool BuildTessellationData;
	
	/** Are we using dynamic hardware tessellation */
	UPROPERTY( EditAnywhere, Category = "QuadTreeMesh" )
	bool DynamicTessellation;
	
	/** The ratio of vertices to use during tessellation */
	UPROPERTY( EditAnywhere, Category = "QuadTreeMesh", Meta = ( EditCondition = "BuildTessellationData", ClampMin = "0.0", UIMin  = "0.0", UIMax = "2.0" ) )
	float TessellationRatio;
};