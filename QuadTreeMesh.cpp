//=============================================================================================================================================
#include "ProceduralTerrainPrivatePCH.h"
//=============================================================================================================================================
//=============================================================================================================================================
const double MyU2Rad = ( double ) 0.000095875262;
//=============================================================================================================================================
inline bool UsingLowDetail( )
{
	// Get detail mode
	static const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable( TEXT( "r.DetailMode" ) );

	// clamp range
	int32 Ret = FMath::Clamp( CVar->GetInt(), 0, 2 );

	return ( Ret == 0 );
}
//=============================================================================================================================================
UQuadTreeMesh::UQuadTreeMesh(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = false;
	
	bTickInEditor = true;
	bAutoActivate = true;
	
	SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	bAutoActivate = true;
	
	MeshGridType = EGridType::FGT_Square;
	GridSpacing = 24.f;
	MeshXSize = 64;
	MeshYSize = 64;
	HeightScale = 1.f;
	
	DynamicTessellation = true;
	BuildTessellationData = false;
	TessellationRatio = 0.125f;
	UpdateComponent = true;
	
	// Create dynamic delegate for overlapped event
	OnComponentBeginOverlap.AddDynamic( this, &UFluidSurfaceComponent::ComponentTouched );
}
//=============================================================================================================================================
void UQuadTreeMesh::Init()
{
	// Ensure sizes are within limits (In case changed via code, bypassing UI limits)
	if( MeshXSize < 0 )
		MeshXSize = 0;
	else if( MeshXSize > 4096 )
		MeshXSize = 4096;
	else if( ( MeshXSize % 32 ) != 0 )
		MeshXSize -= ( MeshXSize % 32 );

	if( MeshYSize < 0 )
		MeshYSize = 0;
	else if( MeshYSize > 4096 )
		MeshYSize = 4096;
	else if( ( MeshYSize % 32 ) != 0 )
		MeshYSize -= ( MeshYSize % 32 );
	
	// Calculate 'origin' aka. bottom left (min) corner
	float RadX;
	float RadY;
	
	if( GridType == EGridType::FGT_Hexagonal )
	{
		RadX = ( 0.5f * ( MeshXSize - 1 ) * GridSpacing );
		RadY = ROOT3OVER2 * ( 0.5f * ( MeshYSize - 1 ) * GridSpacing );
	}
	else
	{
		RadX = ( 0.5f * ( MeshXSize - 1 ) * GridSpacing );
		RadY = ( 0.5f * ( MeshYSize - 1 ) * GridSpacing );
	}
	
	Origin.X = -RadX;
	Origin.Y = -RadY;
	Origin.Z = 0.f;
	
	// Calculate bounding box
	FVector p;
	
	BoundingBox.Init();
	
	p = Origin;
	p.Z = -MESHBOXHEIGHT;
	FluidBoundingBox += p;
	
	p.X = RadX;
	p.Y = RadY;
	p.Z = MESHBOXHEIGHT;
	FluidBoundingBox += p;
	
	if( RenderData )
	{
		RenderData->ReleaseResources();
		RenderData = NULL;
	}
	
	// Create render data
	RenderData = new FTerrainRenderData();
	RenderData->InitResources( this );
}
//=============================================================================================================================================
void UQuadTreeMesh::GetNearestIndex( const FVector& Pos, int& xIndex, int& yIndex )
{
	FVector LocalPos = GetWorldToComponent().TransformPosition( Pos );
	
	xIndex = FMath::RoundToInt( ( LocalPos.X - Origin.X ) / GridSpacing );
	xIndex = FMath::Clamp( xIndex, 0, MeshXSize - 1 );
	
	if( GridType == EGridType::FGT_Hexagonal )
		yIndex = FMath::RoundToInt( ( LocalPos.Y - Origin.Y ) / ( ROOT3OVER2 * GridSpacing ) );
	else
		yIndex = FMath::RoundToInt( ( LocalPos.Y - Origin.Y ) / GridSpacing );
	
	yIndex = FMath::Clamp( yIndex, 0, MeshYSize - 1 );
}
//=============================================================================================================================================
bool UQuadTreeMesh::SetGeneratedMeshTriangles(const TArray<FGeneratedMeshTriangle>& Triangles)
{
	CustomMeshTris = Triangles;
	
	// Need to recreate scene proxy to send it over
	MarkRenderStateDirty();

	return true;
}
//=============================================================================================================================================
bool UQuadTreeMesh::SetGeneratedMeshQuads(const TArray<FMeshQuad>& Quads)
{
	CustomMeshQuads = Quads;
	
	// Need to recreate scene proxy to send it over
	MarkRenderStateDirty();

	return true;
}
//=============================================================================================================================================
FPrimitiveSceneProxy* UQuadTreeMesh::CreateSceneProxy()
{
	if (CustomMeshTris.Num() == 0) return NULL;
	
	return ( RenderData ) ? new FCustomMeshSceneProxy( this ) : NULL;
}
//=============================================================================================================================================
void UQuadTreeMesh::ComponentTouched( AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult )
{
	if( !Other )
		return;
	
	FVector ActorLocation = Other->GetActorLocation();
	
	//if( TouchEffect )
	//{
		// Spawn touch effect emitter
	//	FRotator Rotation = FRotator( 0, 0, 0 );
	//	AEmitter* Emitter = GetWorld( )->SpawnActor<AEmitter>( ActorLocation, Rotation );
	//	Emitter->ParticleSystemComponent->SetTemplate( TouchEffect );
	//}
}
//=============================================================================================================================================
void UQuadTreeMesh::UpdateBody()
{
	if( BodySetup == NULL )
	{
		/* Create physics body */
		BodySetup = ConstructObject<UBodySetup>( UBodySetup::StaticClass( ), this );
	}

	BodySetup->AggGeom.EmptyElements( );
	
	FVector BoxExtents = FluidBoundingBox.GetExtent( );
	FKBoxElem& BoxElem = *new ( BodySetup->AggGeom.BoxElems ) FKBoxElem( BoxExtents.X * 2, BoxExtents.Y * 2, BoxExtents.Z * 2 );
	BoxElem.Center = FVector::ZeroVector;
	BoxElem.Orientation = GetComponentQuat( );
	
	BodySetup->ClearPhysicsMeshes();
	BodySetup->CreatePhysicsMeshes();

	/* Setup collision defaults */
	BodyInstance.SetResponseToAllChannels( ECR_Overlap );
	BodyInstance.SetResponseToChannel( ECC_Camera, ECR_Ignore );
	BodyInstance.SetResponseToChannel( ECC_Visibility, ECR_Block );
	BodyInstance.SetInstanceNotifyRBCollision( true );
}
//=============================================================================================================================================
FColor UQuadTreeMesh::GetWireframeColor() const
{
	return FColor( 0, 255, 255, 255 );
}
//=============================================================================================================================================
int32 UQuadTreeMesh::GetNumMaterials() const
{
	return 1;
}
//=============================================================================================================================================
void UQuadTreeMesh::BeginDestroy()
{
	Super::BeginDestroy();
	if( RenderData )
		RenderData->ReleaseResources();
}
//=============================================================================================================================================
void UQuadTreeMesh::CreatePhysicsState()
{
#if WITH_EDITOR
	if( bPhysicsStateCreated )
		DestroyPhysicsState();

	UpdateBody();
#endif

	return Super::CreatePhysicsState();
}
//=============================================================================================================================================
UMaterialInterface* UQuadTreeMesh::GetMaterial( int32 MaterialIndex ) const
{
	return TerrainMaterial;
}
//=============================================================================================================================================
FBoxSphereBounds UQuadTreeMesh::CalcBounds(const FTransform & LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	//NewBounds.Origin = FVector::ZeroVector;
	//NewBounds.BoxExtent = FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX);
	//NewBounds.SphereRadius = FMath::Sqrt(3.0f * FMath::Square(HALF_WORLD_MAX));
	FBox NewBox = FBox( FVector( -( ( MeshXSize * GridSpacing ) / 2.0f ), -( ( MeshYSize * GridSpacing ) / 2.0f ), -10.0f ),
						FVector( ( MeshYSize * GridSpacing ) / 2.0f, ( MeshYSize * GridSpacing ) / 2.0f, 10.0f ) );
	NewBounds = FBoxSphereBounds( NewBox );
	NewBounds.Origin = GetComponentLocation( );
	return NewBounds;
}
//=============================================================================================================================================