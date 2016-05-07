// Copyright 2016 massanoori. All Rights Reserved.

#pragma once

bool GenerateAirMeshes(const TArray<FVector>& Vertices, const TArray<int32>& Indices, TArray<TStaticArray<int32, 4u>>& OutTetrahedra);
