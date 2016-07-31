

#pragma once

#include "GeneratedMeshComponent.h"
#include "Heightmap.h"

#include "GameFramework/Actor.h"
#include "QuadTree.generated.h"

/**
* Type of node in the QuadTree
*/
UENUM(BlueprintType)
enum EEQuadNodeType
{
	LEAF    UMETA(DisplayName = "Leaf"),
	NODE    UMETA(DisplayName = "Node")
};

/**
* A Node in the QuadTree
*/
USTRUCT()
struct FAQuadNode
{
	GENERATED_USTRUCT_BODY()

	/** The Four Children for the QuadTree Node */
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuadNode)
	FAQuadNode* m_Children[4];

	/** ID's of the children */
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuadNode)
	int32 m_Branches[4];

	/** Is this a leaf node or a regular node */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuadNode)
		TEnumAsByte<EEQuadNodeType> m_Type;

	/** ID of the node */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuadNode)
		int32 m_pNodeID;

	/** ID of the parent node */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuadNode)
		int32 m_ParentID;

	/** Bounding Coordinates for the node */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuadNode)
		TArray<FVector> m_Bounds;
};



/**
 * 
 */
UCLASS()
class FERO_API AQuadTree : public AStaticMeshActor
{
	GENERATED_UCLASS_BODY()

public:

	void LoadQuadTree();

	/** Loads the QuadTree heightmap */
	void LoadMap(Heightmap* heightmap);
	void LoadMap();

	int32 SampleHeight(int32 index);
	int32 SampleHeight(int32 x, int32 z);

	UFUNCTION(BlueprintCallable, Category = "Components|CustomMesh")
	void SetMyMaterial();

private:

	void GenerateMesh();
	void GenerateMesh(FAQuadNode* node);
	void LatheMesh(FAQuadNode* node);

	void Initialize();

	void Create();

	int32 NumberOfNodes(int32 leaves, int32 leafWidth);

	void CreateNode(FAQuadNode*& node, FVector bounds[4], int32 parentID, int32 nodeID);

	//
	// Variable declarations
	//

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuadTree)
	TSubobjectPtr<UGeneratedMeshComponent> m_Mesh;

	TArray<FVector> m_Vertices;

	/** The height of the QuadTree */
	//TSubobjectPtr<AHeightmap> m_Heightmap;
	Heightmap* m_Heightmap;

	/** Name of the heightmap */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuadTree)
		FString m_HeightmapName;

	/** The Width and Height of the QuadTree */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuadTree)
		int32 m_QuadTreeSize;

	/** The total amount of leaves in the QuadTree */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = QuadTree)
		int32 m_TotalLeaves;

	/** The total amount of leaves in one row in the QuadTree */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = QuadTree)
		int32 m_TotalLeavesInRow;

	/** Must be one more that it should be */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuadTree)
		int32 m_LeafWidth;

	/** The default size of each node */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuadTree)
		int32 m_NodeSize;

	/** This is incremented as the QuadTree is being built */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuadTree)
		int32 m_TotalTreeID;

	/** The amount of nodes in the QuadTree */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuadTree)
		int32 m_NodeCount;

	/** Scales the x and z positions of each vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuadTree)
		int32 m_TerrScale;

	/** Amount to scale the heightmap after it is loaded */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuadTree)
		float m_HeightScale;

	/** Do we want to use the heightmap or make the QuadTree flat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = QuadTree)
		bool m_UseHeight;

	/** List of nodes in the QuadTree */
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = QuadTree)
	FAQuadNode* m_nodes;

	TArray<FGeneratedMeshTriangle> m_Triangles;
	
};
