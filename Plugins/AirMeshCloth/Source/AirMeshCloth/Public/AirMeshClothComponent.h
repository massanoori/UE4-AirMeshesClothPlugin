// Copyright 2016 massanoori. All Rights Reserved.

#pragma once

#include "Components/MeshComponent.h"
#include "AirMeshClothComponent.generated.h"


struct FClothEdge
{
	uint32 VertexIndices[2];
	float Weights[2];
	float RestLength;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AIRMESHCLOTH_API UAirMeshClothComponent : public UMeshComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAirMeshClothComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	virtual void SendRenderDynamicData_Concurrent() override;

	virtual void OnRegister() override;

	virtual int32 GetNumMaterials() const override
	{
		return NumLayers;
	}

	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

	UPROPERTY(EditAnywhere, Category = "AirMesh", meta = (ClampMin = 1, UIMin = 1, UIMax = 64))
	uint32 ResolutionX;

	UPROPERTY(EditAnywhere, Category = "AirMesh", meta = (ClampMin = 1, UIMin = 1, UIMax = 64))
	uint32 ResolutionY;

	UPROPERTY(EditAnywhere, Category = "AirMesh", meta = (ClampMin = 0.0, UIMin = 0.0, UIMax = 1000.0))
	float SizeX;

	UPROPERTY(EditAnywhere, Category = "AirMesh", meta = (ClampMin = 0.0, UIMin = 0.0, UIMax = 1000.0))
	float SizeY;

	UPROPERTY(EditAnywhere, Category = "AirMesh", meta = (ClampMin = 1, UIMin = 1, UIMax = 16))
	uint32 NumLayers;

	UPROPERTY(EditAnywhere, Category = "AirMesh", meta = (ClampMin = 1, UIMin = 1, UIMax = 16))
	uint32 NumIterations;

	UPROPERTY(EditAnywhere, Category = "AirMesh", meta = (ClampMin = 0.0, UIMin = 0.0, UIMax = 1.0))
	float Damping;

	UPROPERTY(EditAnywhere, Category = "AirMesh", meta = (ClampMin = 0.0, UIMin = 0.0, UIMax = 100.0))
	float LayerInterval;

	UPROPERTY(EditAnywhere, Category = "AirMesh")
	bool bUseAirMesh;

	virtual void Serialize(FArchive& Ar) override;

private:
	TArray<FVector> SimulatedPositions[2];
	TArray<float> SimulatedWeights;
	TArray<FClothEdge> ClothEdges;
	TArray<TStaticArray<int32, 4u>> AirTetrahedra;
	int32 CurrentPositionArrayIndex;
	FTransform PreviousTransform;

	TArray<FVector>& GetCurrentPositionArray()
	{
		return SimulatedPositions[CurrentPositionArrayIndex];
	}

	const TArray<FVector>& GetCurrentPositionArray() const
	{
		return SimulatedPositions[CurrentPositionArrayIndex];
	}

	TArray<FVector>& GetPreviousPositionArray()
	{
		return SimulatedPositions[CurrentPositionArrayIndex ^ 1];
	}
};
