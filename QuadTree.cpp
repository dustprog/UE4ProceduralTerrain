

#include "Fero.h"
#include "QuadTree.h"

#include <fstream>
#include <random>
#include <cstdlib>
#include <stdlib.h>
using namespace std;


AQuadTree::AQuadTree(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	m_Mesh = PCIP.CreateDefaultSubobject<UGeneratedMeshComponent>(this, TEXT("TerrainMesh"));

	int size = 512;

	m_Heightmap = new Heightmap(size, size);
	//m_Heightmap->LoadValley();
	//m_Heightmap->LoadDoublePass();
	m_Heightmap->LoadLake();
	//m_Heightmap->LoadBasic();
	//m_Heightmap->LoadArchipelago();
	//m_Heightmap->LoadMountains();
	//m_Heightmap->LoadDiamondSquare(size, size, 16, 1.0);

	//Load some default values for the quadtree
	m_UseHeight = true;
	m_QuadTreeSize = size;
	m_LeafWidth = 65;
	m_TerrScale = 32;

	LoadQuadTree();

	//Generate the triangles
	m_Mesh->SetGeneratedMeshTriangles(m_Triangles);

	//m_Mesh->SetStaticMesh();

	RootComponent = m_Mesh;
}

void AQuadTree::SetMyMaterial()
{
	m_Mesh->SetMyMaterial();
}

void AQuadTree::GenerateMesh()
{
	GenerateMesh(m_nodes);
}

void AQuadTree::GenerateMesh(FAQuadNode* node)
{
	if (node == 0) return;

	//Generate the mesh for the node
	LatheMesh(node);

	//Progress through the children
	GenerateMesh(node->m_Children[0]);
	GenerateMesh(node->m_Children[1]);
	GenerateMesh(node->m_Children[2]);
	GenerateMesh(node->m_Children[3]);
}

void AQuadTree::LatheMesh(FAQuadNode* node)
{
	if (node == 0) return;

	if (node->m_Type != LEAF) return;

	int terrScale = m_TerrScale;

	// Center the grid in model space
	float halfWidth = ((float)m_LeafWidth - 1.0f) / 2.0f;
	float halfLength = ((float)m_LeafWidth - 1.0f) / 2.0f;

	int ymax = node->m_Bounds[2].Z;
	int ymin = node->m_Bounds[0].Z;
	int xmax = node->m_Bounds[1].X;
	int xmin = node->m_Bounds[0].X;

	bool breakY = false;

	FGeneratedMeshTriangle tri1;
	FGeneratedMeshTriangle tri2;

	// Create triangle one
#pragma region "Create triangle one"
	float v1x = xmax;
	float v1y = ymin;
	float v1z = 0;
	if (m_UseHeight) v1z = m_Heightmap->Sample(v1x, v1y);

	float v2x = xmin;
	float v2y = ymin;
	float v2z = 0;
	if (m_UseHeight) v2z = m_Heightmap->Sample(v2x, v2y);

	float v3x = xmin;
	float v3y = ymax;
	float v3z = 0;
	if (m_UseHeight) v3z = m_Heightmap->Sample(v3x, v3y);

	tri1.Vertex0.X = (v1x - halfWidth) * m_TerrScale;
	tri1.Vertex0.Y = (v1y - halfLength) * m_TerrScale;
	tri1.Vertex0.Z = v1z;

	tri1.Vertex1.X = (v2x - halfWidth) * m_TerrScale;
	tri1.Vertex1.Y = (v2y - halfLength) * m_TerrScale;
	tri1.Vertex1.Z = v2z;

	tri1.Vertex2.X = (v3x - halfWidth) * m_TerrScale;
	tri1.Vertex2.Y = (v3y - halfLength) * m_TerrScale;
	tri1.Vertex2.Z = v3z;

	m_Triangles.Add(tri1);
#pragma endregion

	// Create triangle two
#pragma region "Create triangle two"
	v1x = xmax;
	v1y = ymin;
	v1z = 0;
	if (m_UseHeight) v1z = m_Heightmap->Sample(v1x, v1y);

	v2x = xmin;
	v2y = ymax;
	v2z = 0;
	if (m_UseHeight) v2z = m_Heightmap->Sample(v2x, v2y);

	v3x = xmax;
	v3y = ymax;
	v3z = 0;
	if (m_UseHeight) v3z = m_Heightmap->Sample(v3x, v3y);

	tri2.Vertex0.X = (v1x - halfWidth) * m_TerrScale;
	tri2.Vertex0.Y = (v1y - halfLength) * m_TerrScale;
	tri2.Vertex0.Z = v1z;

	tri2.Vertex1.X = (v2x - halfWidth) * m_TerrScale;
	tri2.Vertex1.Y = (v2y - halfLength) * m_TerrScale;
	tri2.Vertex1.Z = v2z;

	tri2.Vertex2.X = (v3x - halfWidth) * m_TerrScale;
	tri2.Vertex2.Y = (v3y - halfLength) * m_TerrScale;
	tri2.Vertex2.Z = v3z;

	m_Triangles.Add(tri2);
#pragma endregion

	/*
	for (int32 y = ymin; breakY == true, y < ymax; y++)
	{
		for (int32 x = xmin + 1; x <= xmax; x++)
		{
			if (y >= ((((m_QuadTreeSize / (m_LeafWidth - 1))) * m_LeafWidth) - 1) * m_TerrScale)
			{
				breakY = true;
				break;
			}
			
			if (y + 1 >= ((((m_QuadTreeSize / (m_LeafWidth - 1))) * m_LeafWidth) - 1) * m_TerrScale)
			{
				breakY = true;
				break;
			}

			//LeafWidth is 5 so that is 5x5 with 8 triangles in a row, 8 * 4 = 32 triangles
			
			FGeneratedMeshTriangle tri1;
			FGeneratedMeshTriangle tri2;

			// Create triangle one
			#pragma region "Create triangle one"
			float v1x = x;
			float v1y = y;
			float v1z = 0;
			if (m_UseHeight) v1z = m_Heightmap->Sample(v1x, v1y);

			float v2x = x - 1;
			float v2y = y;
			float v2z = 0;
			if (m_UseHeight) v2z = m_Heightmap->Sample(v2x, v2y);

			float v3x = x - 1;
			float v3y = y + 1;
			float v3z = 0;
			if (m_UseHeight) v3z = m_Heightmap->Sample(v3x, v3y);

			tri1.Vertex0.X = (v1x - halfWidth) * m_TerrScale;
			tri1.Vertex0.Y = (v1y - halfLength) * m_TerrScale;
			tri1.Vertex0.Z = v1z;

			tri1.Vertex1.X = (v2x - halfWidth) * m_TerrScale;
			tri1.Vertex1.Y = (v2y - halfLength) * m_TerrScale;
			tri1.Vertex1.Z = v2z;

			tri1.Vertex2.X = (v3x - halfWidth) * m_TerrScale;
			tri1.Vertex2.Y = (v3y - halfLength) * m_TerrScale;
			tri1.Vertex2.Z = v3z;

			m_Triangles.Add(tri1);
			#pragma endregion

			// Create triangle two
			#pragma region "Create triangle two"
			v1x = x;
			v1y = y;
			v1z = 0;
			if (m_UseHeight) v1z = m_Heightmap->Sample(v1x, v1y);

			v2x = x - 1;
			v2y = y + 1;
			v2z = 0;
			if (m_UseHeight) v2z = m_Heightmap->Sample(v2x, v2y);

			v3x = x;
			v3y = y + 1;
			v3z = 0;
			if (m_UseHeight) v3z = m_Heightmap->Sample(v3x, v3y);

			tri2.Vertex0.X = (v1x - halfWidth) * m_TerrScale;
			tri2.Vertex0.Y = (v1y - halfLength) * m_TerrScale;
			tri2.Vertex0.Z = v1z;

			tri2.Vertex1.X = (v2x - halfWidth) * m_TerrScale;
			tri2.Vertex1.Y = (v2y - halfLength) * m_TerrScale;
			tri2.Vertex1.Z = v2z;

			tri2.Vertex2.X = (v3x - halfWidth) * m_TerrScale;
			tri2.Vertex2.Y = (v3y - halfLength) * m_TerrScale;
			tri2.Vertex2.Z = v3z;

			m_Triangles.Add(tri2);
			#pragma endregion
		}
	}*/
}

void AQuadTree::LoadQuadTree()
{
	//LoadMap();
	Initialize();
	Create();
	GenerateMesh();
}

void AQuadTree::LoadMap(Heightmap* heightmap)
{

}

void AQuadTree::LoadMap()
{
	for (int z = 0; z < m_QuadTreeSize + 1; z++)
	{
		for (int x = 0; x < m_QuadTreeSize + 1; x++)
		{
			m_Vertices.Add(FVector(x, 0, z));
		}
	}
}

int32 AQuadTree::SampleHeight(int32 index)
{
	return 0;
}

int32 AQuadTree::SampleHeight(int32 x, int32 z)
{
	return 0;
}

void AQuadTree::Initialize()
{
	m_TotalTreeID = 0;

	m_TotalLeaves = (m_QuadTreeSize / (m_LeafWidth - 1)) * (m_QuadTreeSize / (m_LeafWidth - 1));
	m_TotalLeavesInRow = m_TotalLeaves / 2;

	m_NodeCount = NumberOfNodes(m_TotalLeaves, m_LeafWidth - 1);
}

void AQuadTree::Create()
{
	FVector RootBounds[4];

	RootBounds[0].X = 0;
	RootBounds[0].Y = 0;
	RootBounds[0].Z = 0;

	RootBounds[1].X = m_QuadTreeSize;
	RootBounds[1].Y = 0;
	RootBounds[1].Z = 0;

	RootBounds[2].X = 0;
	RootBounds[2].Y = 0;
	RootBounds[2].Z = m_QuadTreeSize;

	RootBounds[3].X = m_QuadTreeSize;
	RootBounds[3].Y = 0;
	RootBounds[3].Z = m_QuadTreeSize;

	CreateNode(m_nodes, RootBounds, 0, 0);
}

int32 AQuadTree::NumberOfNodes(int32 leaves, int32 leafWidth)
{
	int ctr = 0, val = 0;

	while (val == 0)
	{
		ctr += leaves;
		leaves /= leafWidth;

		if (leaves == 0)
			break;

		if (leaves == 1)
			val = 1;
	}

	ctr++;

	return ctr;
}

void AQuadTree::CreateNode(FAQuadNode*& node, FVector bounds[4], int32 parentID, int32 nodeID)
{
	node = new FAQuadNode();

	node->m_Children[0] = 0;
	node->m_Children[1] = 0;
	node->m_Children[2] = 0;
	node->m_Children[3] = 0;

	float xDiff = bounds[0].X - bounds[1].X;
	float zDiff = bounds[0].Z - bounds[1].Z;

	//Find the width and height of the node
	int NodeWidth = (int)abs(xDiff);
	int NodeHeight = (int)abs(zDiff);

	EEQuadNodeType type;

	if (NodeWidth == m_LeafWidth - 1)
		type = LEAF;
	else
		type = NODE;

	node->m_Type = type;
	node->m_pNodeID = nodeID;
	node->m_ParentID = parentID;

	int bounds0X = (int)bounds[0].X;
	int bounds0Z = (int)bounds[0].Z;

	int bounds1X = (int)bounds[1].X;
	int bounds1Z = (int)bounds[1].Z;

	int bounds2X = (int)bounds[2].X;
	int bounds2Z = (int)bounds[2].Z;

	int bounds3X = (int)bounds[3].X;
	int bounds3Z = (int)bounds[3].Z;

	float height0 = 0;
	float height1 = 0;
	float height2 = 0;
	float height3 = 0;

	if (m_UseHeight)
	{
		if (bounds0X < m_QuadTreeSize && bounds0Z < m_QuadTreeSize) { height0 = m_Heightmap->Sample(bounds0Z, bounds0X); }
		if (bounds1X < m_QuadTreeSize && bounds1Z < m_QuadTreeSize) { height1 = m_Heightmap->Sample(bounds1Z, bounds1X); }
		if (bounds2X < m_QuadTreeSize && bounds2Z < m_QuadTreeSize) { height2 = m_Heightmap->Sample(bounds2Z, bounds2X); }
		if (bounds3X < m_QuadTreeSize && bounds3Z < m_QuadTreeSize) { height3 = m_Heightmap->Sample(bounds3Z, bounds3X); }
	}

	//Need to get the bounding coord for this node
	node->m_Bounds.Add(FVector(bounds0X, height0, bounds0Z));
	node->m_Bounds.Add(FVector(bounds1X, height1, bounds1Z));
	node->m_Bounds.Add(FVector(bounds2X, height2, bounds2Z));
	node->m_Bounds.Add(FVector(bounds3X, height3, bounds3Z));

	if (type == LEAF)
	{
		return;
	}
	else//Create a node
	{
#pragma region "Child Node 1"
		//======================================================================================================================
		//Child Node 1
		m_TotalTreeID++;
		node->m_Branches[0] = m_TotalTreeID;
		FVector ChildBounds1[4];

		//Top-Left
		ChildBounds1[0].X = bounds[0].X;
		ChildBounds1[0].Y = 0;
		ChildBounds1[0].Z = bounds[0].Z;

		//Top-Right
		ChildBounds1[1].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds1[1].Y = 0;
		ChildBounds1[1].Z = bounds[1].Z;

		//Bottom-Left
		ChildBounds1[2].X = bounds[2].X;
		ChildBounds1[2].Y = 0;
		ChildBounds1[2].Z = bounds[0].Z + abs((bounds[0].Z - bounds[2].Z) / 2);

		//Bottom-Right
		ChildBounds1[3].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds1[3].Y = 0;
		ChildBounds1[3].Z = bounds[0].Z + abs((bounds[0].Z - bounds[2].Z) / 2);

		CreateNode(node->m_Children[0], ChildBounds1, nodeID, m_TotalTreeID);
		//======================================================================================================================
#pragma endregion

#pragma region "Child Node 2"
		//======================================================================================================================
		//Child Node 2
		m_TotalTreeID++;
		node->m_Branches[1] = m_TotalTreeID;
		FVector ChildBounds2[4];

		//Top-Left
		ChildBounds2[0].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds2[0].Y = 0;
		ChildBounds2[0].Z = bounds[1].Z;

		//Top-Right
		ChildBounds2[1].X = bounds[1].X;
		ChildBounds2[1].Y = 0;
		ChildBounds2[1].Z = bounds[1].Z;

		//Bottom-Left
		ChildBounds2[2].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds2[2].Y = 0;
		ChildBounds2[2].Z = bounds[1].Z + abs((bounds[1].Z - bounds[3].Z) / 2);

		//Bottom-Right
		ChildBounds2[3].X = bounds[1].X;
		ChildBounds2[3].Y = 0;
		ChildBounds2[3].Z = bounds[1].Z + abs((bounds[1].Z - bounds[3].Z) / 2);

		CreateNode(node->m_Children[1], ChildBounds2, nodeID, m_TotalTreeID);
		//======================================================================================================================
#pragma endregion

#pragma region "Child Node 3"
		//======================================================================================================================
		//Child Node 3
		m_TotalTreeID++;
		node->m_Branches[2] = m_TotalTreeID;
		FVector ChildBounds3[4];

		//Top-Left
		ChildBounds3[0].X = bounds[0].X;
		ChildBounds3[0].Y = 0;
		ChildBounds3[0].Z = bounds[0].Z + abs((bounds[0].Z - bounds[2].Z) / 2);

		//Top-Right
		ChildBounds3[1].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds3[1].Y = 0;
		ChildBounds3[1].Z = bounds[0].Z + abs((bounds[0].Z - bounds[2].Z) / 2);

		//Bottom-Left
		ChildBounds3[2].X = bounds[2].X;
		ChildBounds3[2].Y = 0;
		ChildBounds3[2].Z = bounds[2].Z;

		//Bottom-Right
		ChildBounds3[3].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds3[3].Y = 0;
		ChildBounds3[3].Z = bounds[3].Z;

		CreateNode(node->m_Children[2], ChildBounds3, nodeID, m_TotalTreeID);
		//======================================================================================================================
#pragma endregion

#pragma region "Child Node 4"
		//======================================================================================================================
		//Child Node 4
		m_TotalTreeID++;
		node->m_Branches[3] = m_TotalTreeID;
		FVector ChildBounds4[4];

		//Top-Left
		ChildBounds4[0].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds4[0].Y = 0;
		ChildBounds4[0].Z = bounds[1].Z + abs((bounds[1].Z - bounds[3].Z) / 2);

		//Top-Right
		ChildBounds4[1].X = bounds[3].X;
		ChildBounds4[1].Y = 0;
		ChildBounds4[1].Z = bounds[1].Z + abs((bounds[1].Z - bounds[3].Z) / 2);//-1

		//Bottom-Left
		ChildBounds4[2].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds4[2].Y = 0;
		ChildBounds4[2].Z = bounds[3].Z;

		//Bottom-Right
		ChildBounds4[3].X = bounds[3].X;
		ChildBounds4[3].Y = 0;
		ChildBounds4[3].Z = bounds[3].Z;

		CreateNode(node->m_Children[3], ChildBounds4, nodeID, m_TotalTreeID);
		//======================================================================================================================
#pragma endregion
	}
}
