// Copyright 2016 massanoori. All Rights Reserved.

#include "AirMeshClothPrivatePCH.h"
#include "AirMeshClothComponent.h"
#include "DynamicMeshBuilder.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "LocalVertexFactory.h"
#include "AirMeshGen.h"
#include "AirMeshClothLog.h"

struct FAirMeshClothDynamicData
{
	TArray<FVector> SimulatedPositions;
};

class FAirMeshClothVertexBuffer : public FVertexBuffer
{
public:
	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(NumVertices * sizeof(FDynamicMeshVertex), BUF_Dynamic, CreateInfo);
	}

	uint32 NumVertices;
};

class FAirMeshClothIndexBuffer : public FIndexBuffer
{
public:
	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint32), NumIndices * sizeof(uint32), BUF_Dynamic, CreateInfo);
	}
	
	uint32 NumIndices;
};

void GenerateIndexBufferContent(uint32 ResolutionX, uint32 ResolutionY, uint32 NumLayers, TArray<int32>& OutIndices)
{
	// build index buffer content
	OutIndices.Empty(ResolutionX * ResolutionY * 2 * 3 * NumLayers);
	uint32 NumVerticesPerLayer = (ResolutionX + 1) * (ResolutionY + 1);
	for (uint32 Layer = 0; Layer < NumLayers; Layer++)
	{
		uint32 BaseIndex = Layer * NumVerticesPerLayer;
		for (uint32 YIndex = 0; YIndex < ResolutionY; YIndex++)
		{
			for (uint32 XIndex = 0; XIndex < ResolutionX; XIndex++)
			{
				int32 I00 = BaseIndex + YIndex * (ResolutionX + 1) + XIndex;
				int32 I01 = BaseIndex + YIndex * (ResolutionX + 1) + XIndex + 1;
				int32 I10 = BaseIndex + (YIndex + 1) * (ResolutionX + 1) + XIndex;
				int32 I11 = BaseIndex + (YIndex + 1) * (ResolutionX + 1) + XIndex + 1;

				OutIndices.Add(I00);
				OutIndices.Add(I01);
				OutIndices.Add(I10);

				OutIndices.Add(I01);
				OutIndices.Add(I11);
				OutIndices.Add(I10);
			}
		}
	}
}

// =================================================================================
// class FAirMeshClothVertexFactory
// =================================================================================

class FAirMeshClothVertexFactory : public FLocalVertexFactory
{
public:
	FAirMeshClothVertexFactory() {}

	void Init(const FAirMeshClothVertexBuffer* VertexBuffer)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			InitAirMeshClothVertexFactory,
			FAirMeshClothVertexFactory*, VertexFactory, this,
			const FAirMeshClothVertexBuffer*, VertexBuffer, VertexBuffer,
		{
			FDataType NewData;
			NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, Position, VET_Float3);
			NewData.TextureCoordinates.Add(
				FVertexStreamComponent(VertexBuffer, STRUCT_OFFSET(FDynamicMeshVertex, TextureCoordinate), sizeof(FDynamicMeshVertex), VET_Float2)
				);
			NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, TangentX, VET_PackedNormal);
			NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, TangentZ, VET_PackedNormal);
			VertexFactory->SetData(NewData);
		});
	}
};

// =================================================================================
// class FAirMeshClothSceneProxy
// =================================================================================

class FAirMeshClothSceneProxy : public FPrimitiveSceneProxy
{
public:
	FAirMeshClothSceneProxy(UAirMeshClothComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, SizeX(Component->SizeX)
		, SizeY(Component->SizeY)
		, ResolutionX(Component->ResolutionX)
		, ResolutionY(Component->ResolutionY)
		, NumIterations(Component->NumIterations)
		, NumLayers(Component->NumLayers)
		, LayerInterval(Component->LayerInterval)
		, DynamicData(nullptr)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
	{
		VertexBuffer.NumVertices = GetRequiredVertexCount() * NumLayers;
		IndexBuffer.NumIndices = GetRequiredIndexCount() * NumLayers;

		VertexFactory.Init(&VertexBuffer);

		BeginInitResource(&VertexBuffer);
		BeginInitResource(&IndexBuffer);
		BeginInitResource(&VertexFactory);

		Materials.SetNumUninitialized(NumLayers);
		for (uint32 Layer = 0; Layer < NumLayers; Layer++)
		{
			Materials[Layer] = Component->GetMaterial(Layer);
			if (Materials[Layer] == nullptr)
			{
				Materials[Layer] = UMaterial::GetDefaultMaterial(MD_Surface);
			}
		}
	}

	virtual ~FAirMeshClothSceneProxy()
	{
		VertexFactory.ReleaseResource();
		VertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		delete DynamicData;
	}

	uint32 GetRequiredVertexCount() const
	{
		return (ResolutionX + 1) * (ResolutionY + 1);
	}

	uint32 GetRequiredIndexCount() const
	{
		return ResolutionX * ResolutionY * 2 * 3;
	}

	virtual uint32 GetMemoryFootprint() const override
	{
		return sizeof(*this) + GetAllocatedSize();
	}

	void SetDynamicData_RenderThread(FAirMeshClothDynamicData* InDynamicData)
	{
		delete DynamicData;
		DynamicData = InDynamicData;

		TArray<FDynamicMeshVertex> Vertices;
		TArray<int32> Indices;
		BuildClothMesh(DynamicData->SimulatedPositions, Vertices, Indices);

		check(Vertices.Num() == GetRequiredVertexCount() * NumLayers);
		check(Indices.Num() == GetRequiredIndexCount() * NumLayers);

		void* BufferData = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, Vertices.Num() * sizeof(FDynamicMeshVertex), RLM_WriteOnly);
		FMemory::Memcpy(BufferData, Vertices.GetData(), Vertices.Num() * sizeof(FDynamicMeshVertex));
		RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);

		BufferData = RHILockIndexBuffer(IndexBuffer.IndexBufferRHI, 0, Indices.Num() * sizeof(int32), RLM_WriteOnly);
		FMemory::Memcpy(BufferData, Indices.GetData(), Indices.Num() * sizeof(int32));
		RHIUnlockIndexBuffer(IndexBuffer.IndexBufferRHI);
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = true;//IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		auto WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
			FLinearColor(0, 0.5f, 1.f)
			);

		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

		for (uint32 Layer = 0; Layer < NumLayers; Layer++)
		{
			FMaterialRenderProxy* MaterialProxy = nullptr;
			if (bWireframe)
			{
				MaterialProxy = WireframeMaterialInstance;
			}
			else
			{
				MaterialProxy = Materials[Layer]->GetRenderProxy(IsSelected());
			}

			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];
					// Draw the mesh.
					FMeshBatch& Mesh = Collector.AllocateMesh();
					FMeshBatchElement& BatchElement = Mesh.Elements[0];
					BatchElement.IndexBuffer = &IndexBuffer;
					Mesh.bWireframe = bWireframe;
					Mesh.VertexFactory = &VertexFactory;
					Mesh.MaterialRenderProxy = MaterialProxy;
					BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
					BatchElement.FirstIndex = GetRequiredIndexCount() * Layer;
					BatchElement.NumPrimitives = GetRequiredIndexCount() / 3;
					BatchElement.MinVertexIndex = 0;
					BatchElement.MaxVertexIndex = GetRequiredVertexCount() * NumLayers;
					Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
					Mesh.Type = PT_TriangleList;
					Mesh.DepthPriorityGroup = SDPG_World;
					Mesh.bCanApplyViewModeOverrides = false;
					Collector.AddMesh(ViewIndex, Mesh);
				}
			}
		}
	}

	void BuildClothMesh(const TArray<FVector>& Positions, TArray<FDynamicMeshVertex>& Vertices, TArray<int32>& Indices) const
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_AirMeshClothProxy_BuildClothMesh);

		Vertices.Empty(GetRequiredVertexCount() * NumLayers);

		uint32 NumVerticesPerLayer = (ResolutionX + 1) * (ResolutionY + 1);

		// Copy vertices to content of vertex buffer
		for (int32 VertexIndex = 0; VertexIndex < Positions.Num(); VertexIndex++)
		{
			Vertices.AddUninitialized();
			auto& AddedVertex = Vertices.Last();
			AddedVertex.Position = Positions[VertexIndex];

			int32 XIndex = (VertexIndex % NumVerticesPerLayer) % (ResolutionX + 1);
			int32 YIndex = (VertexIndex % NumVerticesPerLayer) / (ResolutionX + 1);

			float UFrac = XIndex / (float)ResolutionX;
			float VFrac = YIndex / (float)ResolutionY;

			AddedVertex.TextureCoordinate = FVector2D(UFrac, VFrac);
			AddedVertex.Color = FColor(255, 255, 255);

			// Compute tangents

			FVector TangentX(0.0f);
			FVector TangentY(0.0f);

			if (XIndex > 0)
			{
				TangentX += Positions[VertexIndex] - Positions[VertexIndex - 1];
			}
			if (XIndex < (int32)ResolutionX)
			{
				TangentX += Positions[VertexIndex + 1] - Positions[VertexIndex];
			}
			if (YIndex > 0)
			{
				TangentY += Positions[VertexIndex] - Positions[VertexIndex - ResolutionX - 1];
			}
			if (YIndex < (int32)ResolutionY)
			{
				TangentY += Positions[VertexIndex + ResolutionX + 1] - Positions[VertexIndex];
			}

			TangentX = TangentX.GetSafeNormal();
			TangentY = TangentY.GetSafeNormal();

			AddedVertex.SetTangents(
				TangentX,
				TangentY,
				FVector::CrossProduct(TangentX, TangentY).GetSafeNormal());
		}

		// Build index buffer content
		GenerateIndexBufferContent(ResolutionX, ResolutionY, NumLayers, Indices);
	}

private:
	float SizeX, SizeY;
	uint32 ResolutionX, ResolutionY;
	uint32 NumIterations;
	uint32 NumLayers;
	float LayerInterval;

	FAirMeshClothVertexBuffer VertexBuffer;
	FAirMeshClothIndexBuffer IndexBuffer;

	FAirMeshClothVertexFactory VertexFactory;

	FAirMeshClothDynamicData* DynamicData;

	TArray<UMaterialInterface*> Materials;
	FMaterialRelevance MaterialRelevance;
};

// =================================================================================
// class UAirMeshClothComponent
// =================================================================================

// Sets default values for this component's properties
UAirMeshClothComponent::UAirMeshClothComponent()
	: ResolutionX(16)
	, ResolutionY(16)
	, SizeX(100.0f)
	, SizeY(100.0f)
	, NumLayers(1)
	, NumIterations(4)
	, Damping(0.01f)
	, LayerInterval(5.0f)
	, bUseAirMesh(true)
	, CurrentPositionArrayIndex(0)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UAirMeshClothComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


void UAirMeshClothComponent::OnRegister()
{
	Super::OnRegister();

	GetCurrentPositionArray().Empty((ResolutionX + 1) * (ResolutionY + 1) * NumLayers);
	SimulatedWeights.Empty((ResolutionX + 1) * (ResolutionY + 1) * NumLayers);
	ClothEdges.Empty((
		(ResolutionX + 1) * ResolutionY +
		ResolutionX * (ResolutionY + 1) +
		ResolutionX * ResolutionY * 2) * NumLayers);

	for (uint32 Layer = 0; Layer < NumLayers; Layer++)
	{
		uint32 BaseVertexIndex = Layer * (ResolutionX + 1) * (ResolutionY + 1);

		// Generate positions

		for (uint32 YIndex = 0; YIndex < ResolutionY + 1; YIndex++)
		{
			for (uint32 XIndex = 0; XIndex < ResolutionX + 1; XIndex++)
			{
				GetCurrentPositionArray().AddUninitialized();
				auto& AddedPosition = GetCurrentPositionArray().Last();

				AddedPosition.X = XIndex * SizeX / ResolutionX - SizeX * 0.5f;
				AddedPosition.Y = Layer * LayerInterval;
				AddedPosition.Z = (ResolutionY - YIndex) * SizeY / ResolutionY - SizeY * 0.5f;

				SimulatedWeights.Add(YIndex == 0 ? 0.0f : 1.0f);
			}
		}

		// Generate edge length constraints

		// horizontal
		for (uint32 YIndex = 1 /* 0 */; YIndex < ResolutionY + 1; YIndex++)
		{
			for (uint32 XIndex = 0; XIndex < ResolutionX; XIndex++)
			{
				uint32 Base = YIndex * (ResolutionX + 1) + XIndex + BaseVertexIndex;
				ClothEdges.AddUninitialized();
				auto& Edge = ClothEdges.Last();
				Edge.VertexIndices[0] = Base;
				Edge.VertexIndices[1] = Base + 1;
				Edge.Weights[0] = YIndex == 0 ? 0.0f : 1.0f;
				Edge.Weights[1] = YIndex == 0 ? 0.0f : 1.0f;
			}
		}

		// vertical
		for (uint32 YIndex = 0; YIndex < ResolutionY; YIndex++)
		{
			for (uint32 XIndex = 0; XIndex < ResolutionX + 1; XIndex++)
			{
				uint32 Base = YIndex * (ResolutionX + 1) + XIndex + BaseVertexIndex;
				ClothEdges.AddUninitialized();
				auto& Edge = ClothEdges.Last();
				Edge.VertexIndices[0] = Base;
				Edge.VertexIndices[1] = Base + (ResolutionX + 1);
				Edge.Weights[0] = YIndex == 0 ? 0.0f : 1.0f;
				Edge.Weights[1] = YIndex + 1 == 0 ? 0.0f : 1.0f;
			}
		}

		// cross
		for (uint32 YIndex = 0; YIndex < ResolutionY; YIndex++)
		{
			for (uint32 XIndex = 0; XIndex < ResolutionX; XIndex++)
			{
				uint32 Base = YIndex * (ResolutionX + 1) + XIndex + BaseVertexIndex;

				{
					ClothEdges.AddUninitialized();
					auto& Edge = ClothEdges.Last();
					Edge.VertexIndices[0] = Base;
					Edge.VertexIndices[1] = Base + (ResolutionX + 1) + 1;
					Edge.Weights[0] = YIndex == 0 ? 0.0f : 1.0f;
					Edge.Weights[1] = YIndex + 1 == 0 ? 0.0f : 1.0f;
				}

				{
					ClothEdges.AddUninitialized();
					auto& Edge = ClothEdges.Last();
					Edge.VertexIndices[0] = Base + 1;
					Edge.VertexIndices[1] = Base + (ResolutionX + 1);
					Edge.Weights[0] = YIndex == 0 ? 0.0f : 1.0f;
					Edge.Weights[1] = YIndex + 1 == 0 ? 0.0f : 1.0f;
				}
			}
		}
	}

	// Transform positions

	for (auto& Position : GetCurrentPositionArray())
	{
		Position = ComponentToWorld.TransformPosition(Position);
	}

	PreviousTransform = ComponentToWorld;

	// Compute rest lengths
	for (auto& Edge : ClothEdges)
	{
		auto Diff = GetCurrentPositionArray()[Edge.VertexIndices[0]] - GetCurrentPositionArray()[Edge.VertexIndices[1]];
		Edge.RestLength = Diff.Size();
	}

	// Air mesh is generated only on UE4Editor
	// On UE4Game, tetrahedra are precomputed and deserialized from FArchive
#if WITH_EDITOR
	if (bUseAirMesh)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_AirMeshClothComp_GenerateAirTet);

		// Generate airmesh tetrahedra
		TArray<int32> Indices;
		GenerateIndexBufferContent(ResolutionX, ResolutionY, NumLayers, Indices);
		GenerateAirMeshes(GetCurrentPositionArray(), Indices, AirTetrahedra);
		UE_LOG(LogAirMeshCloth, Log, TEXT("# tetrahedra: %d"), AirTetrahedra.Num());
	}
	else
	{
		AirTetrahedra.Empty();
	}
#endif

	// Initialize previous positions with current positions
	GetPreviousPositionArray() = GetCurrentPositionArray();
}

FBoxSphereBounds UAirMeshClothComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FBox Box(GetCurrentPositionArray());
	return FBoxSphereBounds(Box);
}

void UAirMeshClothComponent::Serialize(FArchive & Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsFilterEditorOnly())
	{
		// Save tetrahedra for UE4Game and deserialize tetrahedra on UE4Game
		Ar << AirTetrahedra;
	}
}


// Called every frame
void UAirMeshClothComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	QUICK_SCOPE_CYCLE_COUNTER(STAT_AirMeshClothComp_TickSimulation);

	float ClampedDeltaTime = FMath::Clamp(DeltaTime, 0.0f, 1.0f / 30.0f);

	// Time integration

	for (int32 VertexIndex = 0; VertexIndex < SimulatedPositions[0].Num(); VertexIndex++)
	{
		auto CurrentPosition = GetCurrentPositionArray()[VertexIndex];
		auto& PreviousPosition = GetPreviousPositionArray()[VertexIndex];

		if (SimulatedWeights[VertexIndex] == 0.0f)
		{
			PreviousPosition = PreviousTransform.InverseTransformPosition(CurrentPosition);
			PreviousPosition = ComponentToWorld.TransformPosition(PreviousPosition);
		}
		else
		{
			auto Step = CurrentPosition - PreviousPosition;
			PreviousPosition = CurrentPosition + Step * (1.0f - Damping) +
				FVector(0.0f, 0.0f, 1.0f) * ClampedDeltaTime * ClampedDeltaTime * GetWorld()->GetGravityZ();
		}
	}

	// Swap array
	CurrentPositionArrayIndex ^= 1;

	PreviousTransform = ComponentToWorld;

	// Enforce constraints

	for (uint32 Iteration = 0; Iteration < NumIterations; Iteration++)
	{
		// Length constraints
		for (const auto& Edge : ClothEdges)
		{
			float WeightSum = Edge.Weights[0] + Edge.Weights[1];
			
			auto& V0 = GetCurrentPositionArray()[Edge.VertexIndices[0]];
			auto& V1 = GetCurrentPositionArray()[Edge.VertexIndices[1]];

			auto Diff = V1 - V0;
			float CurrentLength = (V1 - V0).Size();
			float Scale = (CurrentLength - Edge.RestLength) / (CurrentLength * WeightSum);
			V0 += Diff * Scale * Edge.Weights[0];
			V1 -= Diff * Scale * Edge.Weights[1];
		}

		// Airmesh constraints
		if (bUseAirMesh)
		{
			for (const auto& AirTet : AirTetrahedra)
			{
				auto& Positions = GetCurrentPositionArray();

				auto& P0 = Positions[AirTet[0]];
				auto& P1 = Positions[AirTet[1]];
				auto& P2 = Positions[AirTet[2]];
				auto& P3 = Positions[AirTet[3]];

				// Check negative volume
				auto Grad0 = FVector::CrossProduct(P1 - P3, P2 - P3);
				float Volume = FVector::DotProduct(P0 - P3, Grad0);

				if (Volume >= 0.0f)
				{
					continue;
				}

				// Negating 2nd and 4th gradients
				// based on an extraction formula of determinant

				auto Grad1 = -FVector::CrossProduct(P2 - P0, P3 - P0);
				auto Grad2 = FVector::CrossProduct(P3 - P1, P0 - P1);
				auto Grad3 = -FVector::CrossProduct(P0 - P2, P1 - P2);

				float Grad0SizeSq = Grad0.SizeSquared();

				float Denominator =
					SimulatedWeights[AirTet[0]] * Grad0SizeSq +
					SimulatedWeights[AirTet[1]] * Grad1.SizeSquared() +
					SimulatedWeights[AirTet[2]] * Grad2.SizeSquared() +
					SimulatedWeights[AirTet[3]] * Grad3.SizeSquared();

				if (FMath::IsNearlyZero(Denominator, Grad0SizeSq * 1e-7f))
				{
					continue;
				}

				float ScalingFactor = Volume / Denominator;

				P0 -= SimulatedWeights[AirTet[0]] * ScalingFactor * Grad0;
				P1 -= SimulatedWeights[AirTet[1]] * ScalingFactor * Grad1;
				P2 -= SimulatedWeights[AirTet[2]] * ScalingFactor * Grad2;
				P3 -= SimulatedWeights[AirTet[3]] * ScalingFactor * Grad3;
			}
		}
	}

	// Need to send new data to render thread
	MarkRenderDynamicDataDirty();

	// Call this because bounds have changed
	UpdateComponentToWorld();
}

FPrimitiveSceneProxy * UAirMeshClothComponent::CreateSceneProxy()
{
	return new FAirMeshClothSceneProxy(this);
}

void UAirMeshClothComponent::SendRenderDynamicData_Concurrent()
{
	if (SceneProxy)
	{
		auto DynamicData = new FAirMeshClothDynamicData();
		DynamicData->SimulatedPositions = GetCurrentPositionArray();
		for (auto& Position : DynamicData->SimulatedPositions)
		{
			Position = ComponentToWorld.InverseTransformPosition(Position);
		}

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FSendAirMeshClothDynamicData,
			FAirMeshClothSceneProxy*, AirMeshClothSceneProxy, (FAirMeshClothSceneProxy*)SceneProxy,
			FAirMeshClothDynamicData*, DynamicData, DynamicData,
		{
			AirMeshClothSceneProxy->SetDynamicData_RenderThread(DynamicData);
		});
	}
}

