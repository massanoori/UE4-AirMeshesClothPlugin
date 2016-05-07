// Copyright 2016 massanoori. All Rights Reserved.

#include "AirMeshClothPrivatePCH.h"
#include "AirMeshGen.h"

#if WITH_EDITOR
// Avoid using AGPL software on UE4Game
#include "tetgen_wrapper.h"
#endif

bool GenerateAirMeshes(const TArray<FVector>& Vertices, const TArray<int32>& Indices, TArray<TStaticArray<int32, 4u>>& OutTetrahedra)
{
	check(Indices.Num() % 3 == 0);

#if WITH_EDITOR
	namespace tw = tetgen_wrapper;

	// ================================================================
	// Convert vertex positions to the format readable by tetgen
	// ================================================================

	TArray<double> VerticesForTetgen;

	// 8 points are for bounding box
	VerticesForTetgen.Empty((Vertices.Num() + 8) * 3);

	for (const auto& Vertex : Vertices)
	{
		VerticesForTetgen.Add(Vertex.X);
		VerticesForTetgen.Add(Vertex.Y);
		VerticesForTetgen.Add(Vertex.Z);
	}

	// Generate bounding 8 points

	FBox BoundingBox(Vertices);
	float Extension = BoundingBox.GetExtent().GetAbsMax() * 0.02f;
	BoundingBox.Min -= FVector(Extension);
	BoundingBox.Max += FVector(Extension);

	VerticesForTetgen.Add(BoundingBox.Min.X);
	VerticesForTetgen.Add(BoundingBox.Min.Y);
	VerticesForTetgen.Add(BoundingBox.Min.Z);

	VerticesForTetgen.Add(BoundingBox.Max.X);
	VerticesForTetgen.Add(BoundingBox.Min.Y);
	VerticesForTetgen.Add(BoundingBox.Min.Z);

	VerticesForTetgen.Add(BoundingBox.Min.X);
	VerticesForTetgen.Add(BoundingBox.Max.Y);
	VerticesForTetgen.Add(BoundingBox.Min.Z);

	VerticesForTetgen.Add(BoundingBox.Max.X);
	VerticesForTetgen.Add(BoundingBox.Max.Y);
	VerticesForTetgen.Add(BoundingBox.Min.Z);

	VerticesForTetgen.Add(BoundingBox.Min.X);
	VerticesForTetgen.Add(BoundingBox.Min.Y);
	VerticesForTetgen.Add(BoundingBox.Max.Z);

	VerticesForTetgen.Add(BoundingBox.Max.X);
	VerticesForTetgen.Add(BoundingBox.Min.Y);
	VerticesForTetgen.Add(BoundingBox.Max.Z);

	VerticesForTetgen.Add(BoundingBox.Min.X);
	VerticesForTetgen.Add(BoundingBox.Max.Y);
	VerticesForTetgen.Add(BoundingBox.Max.Z);

	VerticesForTetgen.Add(BoundingBox.Max.X);
	VerticesForTetgen.Add(BoundingBox.Max.Y);
	VerticesForTetgen.Add(BoundingBox.Max.Z);

	// ================================================================
	// Generate polygons for tetgen
	// ================================================================

	int32 NumTriangles = Indices.Num() / 3;

	TArray<tw::polygon> PolygonsForTetgen;
	PolygonsForTetgen.Empty(NumTriangles + 6); // 6 are for bounding box faces
	for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles; TriangleIndex++)
	{
		PolygonsForTetgen.AddUninitialized();
		auto& Added = PolygonsForTetgen.Last();

		Added.num_vertices = 3;
		Added.vertex_list = const_cast<tw::int32*>(&Indices[TriangleIndex * 3]);
	}

	// Indices for bounding box
	int32 BoundingBoxIndices[] =
	{
		0, 1, 3, 2,
		1, 5, 7, 3,
		5, 4, 6, 7,
		4, 0, 2, 6,
		4, 5, 1, 0,
		2, 3, 7, 6,
	};
	check(ARRAYSIZE(BoundingBoxIndices) == 24);

	// add offset
	for (auto& BoundingBoxIndex : BoundingBoxIndices)
	{
		BoundingBoxIndex += Vertices.Num();
	}

	for (int32 FaceIndex = 0; FaceIndex < ARRAYSIZE(BoundingBoxIndices) / 4; FaceIndex++)
	{
		PolygonsForTetgen.AddUninitialized();
		auto& Added = PolygonsForTetgen.Last();

		Added.num_vertices = 4;
		Added.vertex_list = &BoundingBoxIndices[FaceIndex * 4];
	}

	// ================================================================
	// Generate facets for tetgen
	// ================================================================

	TArray<tw::facet> FacetsForTetgen;
	FacetsForTetgen.Empty(PolygonsForTetgen.Num());
	for (auto& Polygon : PolygonsForTetgen)
	{
		FacetsForTetgen.AddUninitialized();
		auto& Added = FacetsForTetgen.Last();

		Added.hole_list = nullptr;
		Added.num_holes = 0;
		Added.polygon_list = &Polygon;
		Added.num_polygons = 1;
	}

	// ================================================================
	// Build tetgen input
	// ================================================================

	tw::input_output InTetgen, OutTetgen;
	InTetgen.automatic_deallocation = false; // pointers are on stack or managed by TArray
	InTetgen.point_list = VerticesForTetgen.GetData();
	InTetgen.num_points = VerticesForTetgen.Num() / 3;
	InTetgen.facet_list = FacetsForTetgen.GetData();
	InTetgen.num_facets = FacetsForTetgen.Num();

	// ================================================================
	// Tetrahedralize
	// ================================================================

	if (0 != tw::tetrahedralize("", InTetgen, OutTetgen))
	{
		return false;
	}

	// ================================================================
	// Gather relevant tetrahedra
	// ================================================================

	int32 NumTetrahedra = 0;

	// Count tetrahedra
	for (tw::int32 TetIndex = 0; TetIndex < OutTetgen.num_tetrahedra; TetIndex++)
	{
		if (OutTetgen.tetrahedron_list[TetIndex * 4 + 0] >= Vertices.Num()) { continue; }
		if (OutTetgen.tetrahedron_list[TetIndex * 4 + 1] >= Vertices.Num()) { continue; }
		if (OutTetgen.tetrahedron_list[TetIndex * 4 + 2] >= Vertices.Num()) { continue; }
		if (OutTetgen.tetrahedron_list[TetIndex * 4 + 3] >= Vertices.Num()) { continue; }

		NumTetrahedra++;
	}

	// Gather tetrahedra
	OutTetrahedra.Empty(NumTetrahedra);
	for (tw::int32 TetIndex = 0; TetIndex < OutTetgen.num_tetrahedra; TetIndex++)
	{
		if (OutTetgen.tetrahedron_list[TetIndex * 4 + 0] >= Vertices.Num()) { continue; }
		if (OutTetgen.tetrahedron_list[TetIndex * 4 + 1] >= Vertices.Num()) { continue; }
		if (OutTetgen.tetrahedron_list[TetIndex * 4 + 2] >= Vertices.Num()) { continue; }
		if (OutTetgen.tetrahedron_list[TetIndex * 4 + 3] >= Vertices.Num()) { continue; }

		OutTetrahedra.AddUninitialized();
		auto& Added = OutTetrahedra.Last();
		FMemory::Memcpy(&Added[0], OutTetgen.tetrahedron_list + TetIndex * 4, sizeof(Added));
	}

	for (auto& Tet : OutTetrahedra)
	{
		const auto& P3 = Vertices[Tet[3]];
		const auto& P23 = Vertices[Tet[2]] - P3;
		const auto& P13 = Vertices[Tet[1]] - P3;
		const auto& P03 = Vertices[Tet[0]] - P3;

		float TetVolume = FVector::DotProduct(P03, FVector::CrossProduct(P13, P23));

		if (TetVolume < 0.0f)
		{
			Swap(Tet[2], Tet[3]);
		}
	}

	return true;
#else
	// On UE4Game, tetrahedra are deserialized
	UE_LOG(LogAirMeshCloth, Warning, TEXT("Tetrahedron generation is disabled on UE4Game."));
	return false;
#endif
}
