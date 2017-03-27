#include "SimsalaBimPCH.h"
#include "SimsalaBimFunctionLibrary.h"
#include "SimsalaBimModule.h"

#include "Engine.h"
#include "Base64.h"
#include "RawMesh.h"

#include "StaticMeshResources.h"

//#include "RuntimeMeshCore.h"
#include "RuntimeMeshComponent.h"

#include <limits>
#include <algorithm>

uint64 USimsalaBimFunctionLibrary::SerializerOID;
FString USimsalaBimFunctionLibrary::CachedToken;
FString USimsalaBimFunctionLibrary::ServerName;

bool TryGetUInt64Field(const TSharedPtr<FJsonObject> JsonObject, FString FieldName, uint64& Result)
{
	if (!JsonObject->HasField(FieldName))
	{
		return false;
	}
	const auto JsonVal = *(JsonObject->Values.Find(FieldName));
	int64 Res;
	if (!JsonVal->TryGetNumber(Res))
	{
		return false;
	}
	Result = static_cast<uint64>(Res);
	return true;
}

bool TryGetUInt64ArrayField(const TSharedPtr<FJsonObject> JsonObject, FString FieldName, TArray<uint64>& Result)
{
	if (!JsonObject->HasField(FieldName))
	{
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>> *ArrayField;
	if (!JsonObject->TryGetArrayField(FieldName, ArrayField))
	{
		return false;
	}
	for (const auto& JsonVal : *ArrayField)
	{
		int64 Res;
		if (!JsonVal->TryGetNumber(Res))
		{
			return false;
		}
		Result.Add(static_cast<uint64>(Res));
	}
	return true;
}


enum EIfcElementComposition : uint8
{
	Complex,
	Elemental,
	Partial
};


struct SIfcProject
{
	uint64 ID;
	uint64 S;
	FString GlobalIDStr;
	uint64 OwnerHistoryID;
	FString Name;
	TArray<uint64> IsDecomposedByIDs;
	TArray<uint64> RepresentationContextsIDs;
	uint64 UnitsInContextID;

	static TOptional<SIfcProject> TryParse(const TSharedPtr<FJsonObject> JsonObject);
};

TOptional<SIfcProject> SIfcProject::TryParse(const TSharedPtr<FJsonObject> JsonObject)
{
	SIfcProject Result;
	if (!TryGetUInt64Field(JsonObject, TEXT("_i"), Result.ID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_s"), Result.S))
	{
		return{};
	}
	if (!JsonObject->TryGetStringField(TEXT("GlobalId"), Result.GlobalIDStr))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rOwnerHistory"), Result.OwnerHistoryID))
	{
		return{};
	}
	if (!JsonObject->TryGetStringField(TEXT("Name"), Result.Name))
	{
		return{};
	}
	if (!TryGetUInt64ArrayField(JsonObject, TEXT("_rIsDecomposedBy"), Result.IsDecomposedByIDs))
	{
		return{};
	}
	if (!TryGetUInt64ArrayField(JsonObject, TEXT("_rRepresentationContexts"), Result.RepresentationContextsIDs))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rUnitsInContext"), Result.UnitsInContextID))
	{
		return{};
	}
	return Result;
}


struct SIfcRelAggregates
{
	uint64 ID;
	uint64 S;
	FString GlobalIDStr;
	uint64 OwnerHistoryID;
	uint64 RelatingObjectID;
	TOptional<TArray<uint64>> RelatedObjectsIDs;

	static TOptional<SIfcRelAggregates> TryParse(const TSharedPtr<FJsonObject> JsonObject);
};

TOptional<SIfcRelAggregates> SIfcRelAggregates::TryParse(const TSharedPtr<FJsonObject> JsonObject)
{
	SIfcRelAggregates Result;
	if (!TryGetUInt64Field(JsonObject, TEXT("_i"), Result.ID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_s"), Result.S))
	{
		return{};
	}
	if (!JsonObject->TryGetStringField(TEXT("GlobalId"), Result.GlobalIDStr))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rOwnerHistory"), Result.OwnerHistoryID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rRelatingObject"), Result.RelatingObjectID))
	{
		return{};
	}
	{
		TArray<uint64> Res;
		if (TryGetUInt64ArrayField(JsonObject, TEXT("_rRelatedObjects"), Res))
		{
			Result.RelatedObjectsIDs = Res;
		}
	}
	return Result;
}


struct SIfcLocalPlacement
{
	uint64 ID;
	uint64 S;
	TArray<uint64> PlacesObjectIDs;
	TOptional<TArray<uint64>> ReferencedByPlacementsIDs;
	TOptional<uint64> PlacementRelTo;
	uint64 RelativePlacement;

	static TOptional<SIfcLocalPlacement> TryParse(const TSharedPtr<FJsonObject> JsonObject);
};

TOptional<SIfcLocalPlacement> SIfcLocalPlacement::TryParse(const TSharedPtr<FJsonObject> JsonObject)
{
	SIfcLocalPlacement Result;
	if (!TryGetUInt64Field(JsonObject, TEXT("_i"), Result.ID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_s"), Result.S))
	{
		return{};
	}
	if (!TryGetUInt64ArrayField(JsonObject, TEXT("_rPlacesObject"), Result.PlacesObjectIDs))
	{
		return{};
	}
	{
		TArray<uint64> Res;
		if (TryGetUInt64ArrayField(JsonObject, TEXT("_rReferencedByPlacements"), Res))
		{
			Result.ReferencedByPlacementsIDs = Res;
		}
	}
	{
		uint64 Res;
		if (TryGetUInt64Field(JsonObject, TEXT("_rPlacementRelTo"), Res))
		{
			Result.PlacementRelTo = Res;
		}
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rRelativePlacement"), Result.RelativePlacement))
	{
		return{};
	}
	return Result;
}

struct SIfcAxis2Placement3D
{
	uint64 ID;
	uint64 S;
	uint64 LocationID;
	uint64 AxisID;
	uint64 RefDirectionID;

	static TOptional<SIfcAxis2Placement3D> TryParse(const TSharedPtr<FJsonObject> JsonObject);
};

TOptional<SIfcAxis2Placement3D> SIfcAxis2Placement3D::TryParse(const TSharedPtr<FJsonObject> JsonObject)
{
	SIfcAxis2Placement3D Result;
	if (!TryGetUInt64Field(JsonObject, TEXT("_i"), Result.ID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_s"), Result.S))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rLocation"), Result.LocationID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rAxis"), Result.AxisID))
	{
		return{};
	}	if (!TryGetUInt64Field(JsonObject, TEXT("_rRefDirection"), Result.RefDirectionID))
	{
		return{};
	}
	return Result;
}

struct SIfcDirection
{
	uint64 ID;
	uint64 S;
	FVector Direction;

	static TOptional<SIfcDirection> TryParse(const TSharedPtr<FJsonObject> JsonObject);
};

TOptional<SIfcDirection> SIfcDirection::TryParse(const TSharedPtr<FJsonObject> JsonObject)
{
	SIfcDirection Result;
	if (!TryGetUInt64Field(JsonObject, TEXT("_i"), Result.ID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_s"), Result.S))
	{
		return{};
	}
	const TArray<TSharedPtr<FJsonValue>> *Res;
	if (!JsonObject->TryGetArrayField(TEXT("DirectionRatios"), Res) || Res->Num() != 3)
	{
		return{};
	}
	Result.Direction.X = (*Res)[0]->AsNumber();
	Result.Direction.Y = (*Res)[1]->AsNumber();
	Result.Direction.Z = (*Res)[2]->AsNumber();
	return Result;
}


struct SIfcCartesianPoint
{
	uint64 ID;
	uint64 S;
	FVector Point;

	static TOptional<SIfcCartesianPoint> TryParse(const TSharedPtr<FJsonObject> JsonObject);
};

TOptional<SIfcCartesianPoint> SIfcCartesianPoint::TryParse(const TSharedPtr<FJsonObject> JsonObject)
{
	SIfcCartesianPoint Result;
	if (!TryGetUInt64Field(JsonObject, TEXT("_i"), Result.ID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_s"), Result.S))
	{
		return{};
	}
	const TArray<TSharedPtr<FJsonValue>> *Res;
	if (!JsonObject->TryGetArrayField(TEXT("DirectionRatios"), Res) || Res->Num() != 3)
	{
		return{};
	}
	Result.Point.X = (*Res)[0]->AsNumber();
	Result.Point.Y = (*Res)[1]->AsNumber();
	Result.Point.Z = (*Res)[2]->AsNumber();
	return Result;
}



struct SIfcSite
{
	uint64 ID;
	uint64 S;
	FString GlobalIDStr;
	uint64 OwnerHistoryID;
	TArray<uint64> IsDecomposedByIDs;
	TArray<uint64> DecomposesIDs;
	uint64 ObjectPlacementID;
	uint64 RepresentationID;
	uint64 GeometryID;
	FString CompositionType;

	static TOptional<SIfcSite> TryParse(const TSharedPtr<FJsonObject> JsonObject);
};

TOptional<SIfcSite> SIfcSite::TryParse(const TSharedPtr<FJsonObject> JsonObject)
{
	SIfcSite Result;
	if (!TryGetUInt64Field(JsonObject, TEXT("_i"), Result.ID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_s"), Result.S))
	{
		return{};
	}
	if (!JsonObject->TryGetStringField(TEXT("GlobalId"), Result.GlobalIDStr))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rOwnerHistory"), Result.OwnerHistoryID))
	{
		return{};
	}
	if (!TryGetUInt64ArrayField(JsonObject, TEXT("_rIsDecomposedBy"), Result.IsDecomposedByIDs))
	{
		return{};
	}
	if (!TryGetUInt64ArrayField(JsonObject, TEXT("_rDecomposes"), Result.DecomposesIDs))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rObjectPlacement"), Result.ObjectPlacementID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rRepresentation"), Result.RepresentationID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rgeometry"), Result.GeometryID))
	{
		return{};
	}
	if (!JsonObject->TryGetStringField(TEXT("CompositionType"), Result.CompositionType))
	{
		return{};
	}
	return Result;
}


struct SIfcSpatialStructureElement
{
	uint64 ID;
	uint64 S;
	FString GlobalIDStr;
	uint64 OwnerHistoryID;
	TArray<uint64> IsDecomposedByIDs;
	TArray<uint64> DecomposesIDs;
	uint64 ObjectPlacementID;
	FString CompositionType;

	static TOptional<SIfcSpatialStructureElement> TryParse(const TSharedPtr<FJsonObject> JsonObject);
};



TOptional<SIfcSpatialStructureElement> SIfcSpatialStructureElement::TryParse(const TSharedPtr<FJsonObject> JsonObject)
{
	SIfcSpatialStructureElement Result;
	if (!TryGetUInt64Field(JsonObject, TEXT("_i"), Result.ID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_s"), Result.S))
	{
		return{};
	}
	if (!JsonObject->TryGetStringField(TEXT("GlobalId"), Result.GlobalIDStr))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rOwnerHistory"), Result.OwnerHistoryID))
	{
		return{};
	}
	if (!TryGetUInt64ArrayField(JsonObject, TEXT("_rIsDecomposedBy"), Result.IsDecomposedByIDs))
	{
		return{};
	}
	if (!TryGetUInt64ArrayField(JsonObject, TEXT("_rDecomposes"), Result.DecomposesIDs))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_rObjectPlacement"), Result.ObjectPlacementID))
	{
		return{};
	}
	if (!JsonObject->TryGetStringField(TEXT("CompositionType"), Result.CompositionType))
	{
		return{};
	}
	return Result;
}

struct SIfcGeometryData
{
	uint64 ID;
	uint64 S;
	FString VerticesBase64;
	FString IndicesBase64;
	TOptional<FString> NormalsBase64;
	TOptional<FString> MaterialsBase64;

	static TOptional<SIfcGeometryData> TryParse(const TSharedPtr<FJsonObject> JsonObject);
};

TOptional<SIfcGeometryData> SIfcGeometryData::TryParse(const TSharedPtr<FJsonObject> JsonObject)
{
	SIfcGeometryData Result;
	if (!TryGetUInt64Field(JsonObject, TEXT("_i"), Result.ID))
	{
		return{};
	}
	if (!TryGetUInt64Field(JsonObject, TEXT("_s"), Result.S))
	{
		return{};
	}
	if (!JsonObject->TryGetStringField(TEXT("indices"), Result.VerticesBase64))
	{
		return{};
	}
	if (!JsonObject->TryGetStringField(TEXT("vertices"), Result.IndicesBase64))
	{
		return{};
	}
	FString Res;
	if (JsonObject->TryGetStringField(TEXT("normals"), Res))
	{
		 Result.NormalsBase64 = std::move(Res);
	}
	if (JsonObject->TryGetStringField(TEXT("materials"), Res))
	{
		Result.MaterialsBase64 = std::move(Res);
	}
	return Result;
}


struct SIfcElement
{
	uint64 ID;
	uint64 S;
	uint64 OwnerHistoryID;
	TOptional<FString> Name;
	TOptional<TArray<uint64>> IsDecomposedBy;
	TOptional<uint64> ObjectPlacement;
	TOptional<uint64> Decomposes;
	TOptional<uint64> ContainedInStructure;
};
//////////// -------------------------------------------------------------------- ///////////////



bool USimsalaBimFunctionLibrary::ParseJsonIFC(FString FileName, AActor* TargetActor)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FileName))
	{
		UE_LOG(BIMLOG, Error, TEXT("Failed to load file: %s"), *FileName);
		return false;
	}
	TSharedRef<TJsonReader<>> Reader = FJsonStringReader::Create(JsonString);
	TSharedPtr<FJsonObject> BimJson;
	if (!FJsonSerializer::Deserialize(Reader, BimJson))
	{
		UE_LOG(BIMLOG, Error, TEXT("Failed to parse file: %s"), *FileName);
		return false;
	}

	const TArray< TSharedPtr<FJsonValue> > *BimContent;
	if (!BimJson->TryGetArrayField(TEXT("objects"), BimContent))
	{
		UE_LOG(BIMLOG, Error, TEXT("Failed to parse ill-formatted file: '%s'."), *FileName);
		return false;
	}

	// Preparse a hash map for fast access
	TMap<uint64, TSharedPtr<FJsonObject>> IndexedBimObjects;
	for (int EntryIndex = 0; EntryIndex < BimContent->Num(); EntryIndex++)
	{
		const auto& BimEntryVal = (*BimContent)[EntryIndex];
		const TSharedPtr<FJsonObject> *pBimEntry;
		if (BimEntryVal->TryGetObject(pBimEntry))
		{
			TSharedPtr<FJsonObject> CurrentBimEntry = *pBimEntry;
			uint64 EntryID;
			if (TryGetUInt64Field(CurrentBimEntry, TEXT("_i"), EntryID))
			{
				IndexedBimObjects.Add(EntryID, CurrentBimEntry);
				FString EntryType = TEXT("UNKNOWN");
				CurrentBimEntry->TryGetStringField(TEXT("_t"), EntryType);
				UE_LOG(BIMLOG, Log, TEXT("Found %s with id %llu"), *EntryType, EntryID);
			}
		}
	}
	
	for (int EntryIndex = 0; EntryIndex < BimContent->Num(); EntryIndex++)
	{
		const auto& BimEntryVal = (*BimContent)[EntryIndex];
		const TSharedPtr<FJsonObject> *pBimEntry;
		if (BimEntryVal->TryGetObject(pBimEntry))
		{
			TSharedPtr<FJsonObject> CurrentBimEntry = *pBimEntry;
			FString EntryType;
			if (CurrentBimEntry->TryGetStringField(TEXT("_t"), EntryType))
			{
				if (EntryType == TEXT("GeometryData"))
				{
					FString VerticesStr, IndicesStr;
					if (!CurrentBimEntry->TryGetStringField(TEXT("vertices"), VerticesStr) ||
						!CurrentBimEntry->TryGetStringField(TEXT("indices"), IndicesStr) )
					{
						UE_LOG(BIMLOG, Warning, TEXT("Ill-Formatted GeometryData-entry. Skipping."));
						continue;
					}
					else
					{
						FRawMesh IfcMesh;
						TArray<uint8> VerticesBin;
						if (!FBase64::Decode(VerticesStr, VerticesBin))
						{
							UE_LOG(BIMLOG, Error, TEXT("Corrupt GeometryData (vertices)."));
							continue;
						}
						else
						{
							for (int i = 0;i < VerticesBin.Num() ; i += 3*sizeof(float))
							{
								const float x = *reinterpret_cast<float*>(&VerticesBin[i + 0]);
								const float y = *reinterpret_cast<float*>(&VerticesBin[i + 4]);
								const float z = *reinterpret_cast<float*>(&VerticesBin[i + 8]);
								IfcMesh.VertexPositions.Add(FVector(x, y, z));
							}
						}

						uint32 MaxIdx = 0;
						TArray<uint8> IndicesBin;
						if (!FBase64::Decode(IndicesStr, IndicesBin))
						{
							UE_LOG(BIMLOG, Error, TEXT("Corrupt GeometryData (indices)."));
							continue;
						}
						else
						{
							// For each face ...
							for (int i = 0;i < IndicesBin.Num(); i += 3*sizeof(uint32))
							{
								const uint32 idx1 = *reinterpret_cast<uint32*>(&IndicesBin[i + 0]);
								const uint32 idx2 = *reinterpret_cast<uint32*>(&IndicesBin[i + 4]);
								const uint32 idx3 = *reinterpret_cast<uint32*>(&IndicesBin[i + 8]);

								MaxIdx = FMath::Max(MaxIdx, idx1);
								MaxIdx = FMath::Max(MaxIdx, idx2);
								MaxIdx = FMath::Max(MaxIdx, idx3);
								// Reverse face sides
								IfcMesh.WedgeIndices.Add(idx3);
								IfcMesh.WedgeIndices.Add(idx2);
								IfcMesh.WedgeIndices.Add(idx1);
							}
						}
						///@todo: add material and normals (if available)
						FString MaterialsStr;
						if (CurrentBimEntry->TryGetStringField(TEXT("materials"), MaterialsStr))
						{
							TArray<uint8> MaterialsBin;
							if (FBase64::Decode(IndicesStr, IndicesBin))
							{
								for (int i = 0;i < MaterialsBin.Num(); i += 4 * sizeof(float))
								{
									float r = 255 * *reinterpret_cast<float*>(&MaterialsBin[i + 0 * sizeof(float)]);
									float g = 255 * *reinterpret_cast<float*>(&MaterialsBin[i + 1 * sizeof(float)]);
									float b = 255 * *reinterpret_cast<float*>(&MaterialsBin[i + 2 * sizeof(float)]);
									float a = 255 * *reinterpret_cast<float*>(&MaterialsBin[i + 3 * sizeof(float)]);
									IfcMesh.WedgeColors.Add({ static_cast<uint8>(r), static_cast<uint8>(g), static_cast<uint8>(b), static_cast<uint8>(a) });
								}
							}
							else
							{
								UE_LOG(BIMLOG, Error, TEXT("Corrupt GeometryData (materials)."));
							}
						}

						for (auto idx : IfcMesh.WedgeIndices)
						{
							IfcMesh.WedgeTexCoords[0].Add({ (float)idx / (float)MaxIdx, 1 - (float)idx/ (float)MaxIdx });
						}

						for (int i = 0;i < IfcMesh.WedgeIndices.Num() / 3;i++)
						{
							IfcMesh.FaceSmoothingMasks.Add(1);
							IfcMesh.FaceMaterialIndices.Add(0);
						}						

						if (!IfcMesh.IsValidOrFixable())
						{
							UE_LOG(BIMLOG, Error, TEXT("Corrupt mesh."));
							continue;
						}

						
						auto IfcStaticMesh = NewObject<UStaticMesh>(TargetActor);
						FStaticMeshComponentRecreateRenderStateContext RecreateRenderStateContext(FindObject<UStaticMesh>(TargetActor, *TargetActor->GetFName().ToString()));
						
						FStaticMeshSourceModel* SrcModel = new(IfcStaticMesh->SourceModels) FStaticMeshSourceModel();
						SrcModel->RawMeshBulkData->SaveRawMesh(IfcMesh);
						IfcStaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;
						IfcStaticMesh->Build(); 
						IfcStaticMesh->MarkPackageDirty();

						auto IfcMeshComponent = NewObject<UInstancedStaticMeshComponent>(TargetActor);
						IfcMeshComponent->SetStaticMesh(IfcStaticMesh);
						IfcMeshComponent->RegisterComponentWithWorld(TargetActor->GetWorld());
						//IfcMeshComponent->SetMobility(EComponentMobility::Static);
						TargetActor->AddOwnedComponent(IfcMeshComponent);

						// Look for instantiations.
						for (int InstanceEntryIndex = EntryIndex-1; InstanceEntryIndex > 0; InstanceEntryIndex--)
						{
							FString AdjacentEntryType;
							const auto& InstanceEntryVal = (*BimContent)[InstanceEntryIndex];
							const TSharedPtr<FJsonObject> *pInstanceEntry;
							if (!InstanceEntryVal->TryGetObject(pInstanceEntry)) // Entry is not a json object ...
							{
								break;
							}
							auto &InstanceEntry = *pInstanceEntry;
							if (!InstanceEntry->TryGetStringField(TEXT("_t"), AdjacentEntryType) || AdjacentEntryType != TEXT("GeometryInfo"))
							{
								break;
							}
							// Read trafo matrix
							FString TrafoStr;
							if (!InstanceEntry->TryGetStringField(TEXT("transformation"), TrafoStr))
							{
								UE_LOG(BIMLOG, Warning, TEXT("Ill-Formatted GeometryInfo-entry. Skipping."));
								continue;
							}
							TArray<uint8> TrafoBin;
							if (!FBase64::Decode(TrafoStr, TrafoBin))
							{
								UE_LOG(BIMLOG, Warning, TEXT("Corrupt GeometryInfo (transformation)."));
								continue;
							}
							if (TrafoBin.Num() != sizeof(double) * 16)
							{
								UE_LOG(BIMLOG, Warning, TEXT("Corrupt GeometryInfo (transformation)."));
								continue;
							}

							float RawMatrix[4][4];
							for (int i = 0;i < 4;i++)
							{
								for (int j = 0;j < 4;j++)
								{
									const int Offset = (4 * i + j)*sizeof(double);
									RawMatrix[i][j] = static_cast<float>(*reinterpret_cast<double*>(&TrafoBin[Offset]));
								}
							}

							const auto Trafo = FTransform(
								{ RawMatrix[0][0], RawMatrix[0][1], RawMatrix[0][2] }, 
								{ RawMatrix[1][0], RawMatrix[1][1], RawMatrix[1][2] },
								{ RawMatrix[2][0], RawMatrix[2][1], RawMatrix[2][2] },
								{ RawMatrix[3][0], RawMatrix[3][1], RawMatrix[3][2] }
							);
							IfcMeshComponent->AddInstance(Trafo);
						}
					}
				}
			}
		}
	}
	return true;
}


bool USimsalaBimFunctionLibrary::ParseBinaryIFC(FString FileName, AActor* TargetActor)
{
	if (TargetActor == nullptr || !TargetActor->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("No target actor supplied."));
		return false;
	}

	TArray<uint8> BinaryData;
	FFileHelper::LoadFileToArray(BinaryData, *FileName);
	UE_LOG(BIMLOG, Log, TEXT("Read %i bytes"), BinaryData.Num());
	FMemoryReader Ar = FMemoryReader(BinaryData, true);
	
	TCHAR bgsChr[3] = { (TCHAR)BinaryData[2], (TCHAR)BinaryData[3], (TCHAR)BinaryData[4] };
	FString bgsStr = FString(bgsChr);
	if (bgsStr.Equals("BGS") == false)
	{
		UE_LOG(BIMLOG, Warning, TEXT("NO BGS Identifier found"));
		return false;
	}

	Ar.Seek(5);
	uint8 versInt;
	Ar << versInt;
	UE_LOG(BIMLOG, Log, TEXT("Found Binary Geometry Version \"%i\""), versInt);

	float objMinBX;
	float objMinBY;
	float objMinBZ;
	float objMaxBX;
	float objMaxBY;
	float objMaxBZ;

	Ar << objMinBX;
	Ar << objMinBY;
	Ar << objMinBZ;
	Ar << objMaxBX;
	Ar << objMaxBY;
	Ar << objMaxBZ;
	
	int objCount;
	Ar << objCount;

	TMap<int64, UInstancedStaticMeshComponent*> CreatedMeshes;
	for (int counter = 0; counter < objCount; ++counter)
	{
		FString classname = "";
		uint8 strlen;
		Ar << strlen; // discard first one, because...what the hell?
		//UE_LOG(BIMLOG, Log, TEXT("Strlen: %i"), strlen);
		Ar << strlen; // actual length- 255 chars
		//UE_LOG(BIMLOG, Log, TEXT("Strlen: %i"), strlen);

		if (strlen > 0) 
		{
			TCHAR* chrArr = new TCHAR[strlen + 1];
			for (uint8 i = 0; i < strlen; ++i)
			{
				uint8 chrIn;
				Ar << chrIn;
				chrArr[i] = (TCHAR)chrIn;
			}
			chrArr[strlen] = '\0';
			classname = FString(chrArr);
		}
		
		int64 objOID;
		Ar << objOID;
		UE_LOG(BIMLOG, Log, TEXT("Loading '%s' with OID '%i'"), *classname, objOID);

		uint8 geometryType;
		Ar << geometryType;

		//jump to next alignment of 4 
		int64 pos = Ar.Tell();
		int64 skip = 4 - (pos % 4);
		if (skip > 0 && skip != 4) {
			Ar.Seek(pos + skip);
		}

		float RawMatrix[4][4];
		for (int i = 0;i < 4;i++)
		{
			for (int j = 0;j < 4;j++)
			{
				Ar << RawMatrix[i][j];
			}
		}

		//oid of the geometry data
		int64 geometryDataId;
		Ar << geometryDataId;

		//object bounds
		float objBounds[6];
		for (int i = 0; i < 6; ++i) 
		{
			Ar << objBounds[i];
		}

		FRawMesh IfcMesh;
		//indices
		int indicesLen;
		Ar << indicesLen;
		IfcMesh.WedgeIndices.AddUninitialized(indicesLen);
		for (int i = 0; i < indicesLen; i+=3) 
		{
			int idx1, idx2, idx3;
			Ar << idx1;
			Ar << idx2;
			Ar << idx3;
			IfcMesh.WedgeIndices[i+0] = idx1;
			IfcMesh.WedgeIndices[i+1] = idx3;
			IfcMesh.WedgeIndices[i+2] = idx2;
		}

		//vertices
		int verticesLen;
		Ar << verticesLen;
		IfcMesh.VertexPositions.AddUninitialized(verticesLen);
		float lowestX = std::numeric_limits<float>::infinity();
		float highestX = -std::numeric_limits<float>::infinity();
		float lowestY = std::numeric_limits<float>::infinity();
		float highestY = -std::numeric_limits<float>::infinity();
		for (int i = 0; i < verticesLen/3; ++i) 
		{
			float x, y, z;
			Ar << x;
			Ar << y;
			Ar << z;
			lowestX = FMath::Min(lowestX, x);
			highestX = FMath::Max(highestX, x);
			lowestY = FMath::Min(lowestY, y);
			highestY = FMath::Max(highestY, y);
			IfcMesh.VertexPositions[i] = (FVector(x, y, z));
		}

		//tangents
		int normalsLen;
		Ar << normalsLen;

		//float* normals = new float[normalsLen];
		for (int i = 0; i < normalsLen; ++i) 
		{
			float killData;
			Ar << killData;
		}

		IfcMesh.WedgeTangentZ.AddUninitialized(IfcMesh.WedgeIndices.Num());
		for (int i = 0; i < IfcMesh.WedgeIndices.Num(); i += 3)
		{
			const int idx1 = i;
			const int idx2 = i+1;
			const int idx3 = i+2;
			FVector NormalCurrent = FVector::CrossProduct(IfcMesh.VertexPositions[idx1] - IfcMesh.VertexPositions[idx3], IfcMesh.VertexPositions[idx2] - IfcMesh.VertexPositions[idx3]).GetSafeNormal();
			
			IfcMesh.WedgeTangentZ[idx1] = NormalCurrent;
			IfcMesh.WedgeTangentZ[idx2] = NormalCurrent;
			IfcMesh.WedgeTangentZ[idx3] = NormalCurrent;
		}



		//colors
		int colorsLen;
		Ar << colorsLen;

		//float* colors = new float[colorsLen];
		for (int i = 0; i < colorsLen; ++i) {
			float killData;
			Ar << killData;
		}
		// assign UVs
		float rangeX = (lowestX - highestX) * -1;
		float offsetX = 0 - lowestX;
		float rangeY = (lowestY - highestY) * -1;
		float offsetY = 0 - lowestY;
		IfcMesh.WedgeTexCoords[0].AddUninitialized(IfcMesh.WedgeIndices.Num());
		for (int i = 0;i < IfcMesh.WedgeIndices.Num(); i++)
		{
			auto idx = IfcMesh.WedgeIndices[i];
			auto Pos = IfcMesh.VertexPositions[idx];
			IfcMesh.WedgeTexCoords[0][i] = { (Pos.X + offsetX),  (Pos.Y + offsetY) };
		}

		IfcMesh.FaceSmoothingMasks.AddUninitialized(IfcMesh.WedgeIndices.Num() / 3);
		IfcMesh.FaceMaterialIndices.AddUninitialized(IfcMesh.WedgeIndices.Num() / 3);
		for (int i = 0;i < IfcMesh.WedgeIndices.Num() / 3;i++)
		{
			IfcMesh.FaceSmoothingMasks[i] = 1;
			IfcMesh.FaceMaterialIndices[i] = 0;
		}

		if (!IfcMesh.IsValidOrFixable())
		{
			UE_LOG(BIMLOG, Error, TEXT("Corrupt mesh."));
			continue;
		}

		//UE_LOG(LogTemp, Warning, TEXT("#--- Triangle count %d"), indicesLen / 3);
		//UE_LOG(LogTemp, Warning, TEXT("#--- Normal count %d"), normalsLen);
		//UE_LOG(LogTemp, Warning, TEXT("#--- Color count %d"), colorsLen);

		if (classname.Equals("IfcOpeningElement"))
		{
			continue;
		}

		if (!CreatedMeshes.Contains(geometryDataId))
		{
			auto IfcStaticMesh = NewObject<UStaticMesh>(TargetActor);
			FStaticMeshComponentRecreateRenderStateContext RecreateRenderStateContext(FindObject<UStaticMesh>(TargetActor, *TargetActor->GetFName().ToString()));

			FStaticMeshSourceModel* SrcModel = new(IfcStaticMesh->SourceModels) FStaticMeshSourceModel();
			SrcModel->RawMeshBulkData->SaveRawMesh(IfcMesh);
			IfcStaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;
			IfcStaticMesh->Build();
			IfcStaticMesh->MarkPackageDirty();

			auto IfcMeshComponent = NewObject<UInstancedStaticMeshComponent>(TargetActor);
			IfcMeshComponent->SetStaticMesh(IfcStaticMesh);
			IfcMeshComponent->RegisterComponentWithWorld(TargetActor->GetWorld());
			//IfcMeshComponent->SetMobility(EComponentMobility::Static);
			TargetActor->AddOwnedComponent(IfcMeshComponent);
			CreatedMeshes.Add(geometryDataId, IfcMeshComponent);
		}
		
		const auto Trafo = FTransform(
			{ RawMatrix[0][0], RawMatrix[0][1], RawMatrix[0][2] },
			{ RawMatrix[1][0], RawMatrix[1][1], RawMatrix[1][2] },
			{ RawMatrix[2][0], RawMatrix[2][1], RawMatrix[2][2] },
			{ RawMatrix[3][0], RawMatrix[3][1], RawMatrix[3][2] }
		);
		CreatedMeshes[geometryDataId]->AddInstance(Trafo);
	}
	return true;
}


#include "Http.h"

template<typename FunctorType>
bool MakeRequest( FString Token, FString InterfaceName, FString MethodName, TSharedPtr<FJsonObject> ParametersObject, FunctorType&& Handler)
{
	TSharedPtr<FJsonObject> RequestObject = MakeShareable(new FJsonObject);
	RequestObject->SetObjectField("parameters", ParametersObject);
	RequestObject->SetStringField("interface", InterfaceName);
	RequestObject->SetStringField("method", MethodName);

	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetStringField("token", Token);
	RootObject->SetObjectField("request", RequestObject);

	FString Request;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&Request);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	TSharedRef< IHttpRequest > HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("POST");
	HttpRequest->SetURL(USimsalaBimFunctionLibrary::ServerName + "/json");
	HttpRequest->SetHeader("Content-Type", "application/json");
	HttpRequest->SetContentAsString(Request);
	HttpRequest->OnProcessRequestComplete().BindLambda([&Handler](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded) -> void {
		UE_LOG(BIMLOG, Log, TEXT("Processing response: %s"), *HttpResponse->GetContentAsString());
		if (!bSucceeded)
		{
			UE_LOG(BIMLOG, Warning, TEXT("Request failed!"));
			return;
		}
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
		if (!FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			UE_LOG(BIMLOG, Warning, TEXT("Unable to deserialize request."));
			return;
		}
		TSharedPtr<FJsonObject> JsonResponse = JsonObject->GetObjectField("response");
		if (!Handler(JsonResponse))
		{
			UE_LOG(BIMLOG, Warning, TEXT("Failed to handle response!"));
		}
	});
	HttpRequest->ProcessRequest();

	// Wait blocking
	double LastTime = FPlatformTime::Seconds();
	while (EHttpRequestStatus::Processing == HttpRequest->GetStatus())
	{
		const double AppTime = FPlatformTime::Seconds();
		FHttpModule::Get().GetHttpManager().Tick(AppTime - LastTime);
		LastTime = AppTime;
		FPlatformProcess::Sleep(0.5f);
	}

	return  HttpRequest->GetStatus() == EHttpRequestStatus::Succeeded;
}



bool USimsalaBimFunctionLibrary::Connect(FString ServerName, FString Username, FString Password)
{
	USimsalaBimFunctionLibrary::ServerName = ServerName;
	TSharedPtr<FJsonObject> ParametersObject = MakeShareable(new FJsonObject);
	ParametersObject->SetStringField("username", Username);
	ParametersObject->SetStringField("password", Password);

	TSharedPtr<FJsonObject> LoginObject = MakeShareable(new FJsonObject);
	LoginObject->SetObjectField("parameters", ParametersObject);
	LoginObject->SetStringField("interface", "Bimsie1AuthInterface");
	LoginObject->SetStringField("method", "login");

	TSharedPtr<FJsonObject> RequestObject = MakeShareable(new FJsonObject);
	RequestObject->SetObjectField("request", LoginObject);

	UE_LOG(BIMLOG, Log, TEXT("Logging in as %s"), *Username);
	FString Request;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&Request);
	FJsonSerializer::Serialize(RequestObject.ToSharedRef(), Writer);

	TSharedRef< IHttpRequest > HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("POST");
	HttpRequest->SetURL(ServerName + "/json");
	HttpRequest->SetHeader("Content-Type", "application/json");
	HttpRequest->SetContentAsString(Request);
	HttpRequest->OnProcessRequestComplete().BindLambda(
		[](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded) -> void {
			if (!bSucceeded)
			{
				UE_LOG(BIMLOG, Warning, TEXT("Failed to login!"));
				return;
			}
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				UE_LOG(BIMLOG, Warning, TEXT("Failed to login!"));
				return;
			}
			TSharedPtr<FJsonObject> JsonResponse = JsonObject->GetObjectField("response");
			USimsalaBimFunctionLibrary::CachedToken = JsonResponse->GetStringField("result");
			UE_LOG(BIMLOG, Log, TEXT("Login successful! Token is: %s"), *USimsalaBimFunctionLibrary::CachedToken);
		});
	HttpRequest->ProcessRequest();

	double LastTime = FPlatformTime::Seconds();
	while (EHttpRequestStatus::Processing == HttpRequest->GetStatus())
	{
		const double AppTime = FPlatformTime::Seconds();
		FHttpModule::Get().GetHttpManager().Tick(AppTime - LastTime);
		LastTime = AppTime;
		FPlatformProcess::Sleep(0.5f);
	}

	if (HttpRequest->GetStatus() != EHttpRequestStatus::Succeeded)
	{
		return false;
	}

	TSharedPtr<FJsonObject> ParametersObject2 = MakeShareable(new FJsonObject);
	ParametersObject2->SetStringField("serializerName", "BinaryGeometrySerializer");

	return MakeRequest(CachedToken, "Bimsie1ServiceInterface", "getSerializerByName", ParametersObject2,
		[](TSharedPtr<FJsonObject> JsonResponse) -> bool {
		auto JsonResult = JsonResponse->GetObjectField("result");
		if (!TryGetUInt64Field(JsonResult, TEXT("oid"), USimsalaBimFunctionLibrary::SerializerOID))
		{
			UE_LOG(BIMLOG, Error, TEXT("Ill-formatted result: Unable to acquire oid."));
			return false;
		}
		return true;
	});
}



bool USimsalaBimFunctionLibrary::GetProjectList(TArray<UIfcProject*>& Projects)
{
	TSharedPtr<FJsonObject> ParametersObject = MakeShareable(new FJsonObject);
	ParametersObject->SetBoolField("onlyTopLevel", false);
	ParametersObject->SetBoolField("onlyActive", false);

	return MakeRequest(CachedToken, "Bimsie1ServiceInterface", "getAllProjects", ParametersObject,
		[&Projects](TSharedPtr<FJsonObject> JsonResponse) -> bool {
			auto JsonResult = JsonResponse->GetArrayField("result");
			for (auto JsonProjectVal : JsonResult)
			{
				auto JsonProject = JsonProjectVal->AsObject();
				auto NextProject = NewObject<UIfcProject>();
				if (!TryGetUInt64Field(JsonProject, TEXT("lastRevisionId"), NextProject->OID))
				{
					UE_LOG(BIMLOG, Error, TEXT("Ill-formatted result: Unable to acquire oid."));
					return false;
				}
				JsonProject->TryGetStringField("name", NextProject->Name);
				Projects.Add(NextProject);
			}
			return true;
		});
}


bool USimsalaBimFunctionLibrary::LoadProject(AActor* ReferencePoint, UIfcProject* Project)
{
	if (!ReferencePoint || !ReferencePoint->IsValidLowLevel())
	{
		UE_LOG(BIMLOG, Warning, TEXT("Invalid Reference Point."));
		return false;
	}
	TSharedPtr<FJsonObject> ParametersObject = MakeShareable(new FJsonObject);
	ParametersObject->SetNumberField("roid", Project->OID);
	ParametersObject->SetNumberField("serializerOid", USimsalaBimFunctionLibrary::SerializerOID);
	ParametersObject->SetBoolField("showOwn", false);
	ParametersObject->SetBoolField("sync", true);

	FString DownloadLink;

	bool RequestSucceeded = MakeRequest(CachedToken, "Bimsie1ServiceInterface", "download", ParametersObject,
		[&DownloadLink](TSharedPtr<FJsonObject> JsonResponse) -> bool {
		uint64 JsonResult = JsonResponse->GetNumberField("result");
		DownloadLink = USimsalaBimFunctionLibrary::ServerName + "/download?token=" + USimsalaBimFunctionLibrary::CachedToken +
			"&topicId=" + FString::FromInt(JsonResult) +
			"&longActionId=" + FString::FromInt(JsonResult) +
			"&serializerOid=" + FString::FromInt(USimsalaBimFunctionLibrary::SerializerOID);
		return true;
	});
	// http://localhost:8082/download?token=51510ef03fe178a535fc4114e6b0584c2b9ebd876254e44873962dc0f7b2dc59c96383b6be481a685d9166027a68b9fd&topicId=2&serializerOid=4259878&longActionId=2

	if (!RequestSucceeded)
	{
		return false;
	}

	TArray<uint8> BinaryData;

	TSharedRef< IHttpRequest > HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetURL(DownloadLink);
	HttpRequest->SetHeader("Content-Type", "application/json");
	HttpRequest->OnProcessRequestComplete().BindLambda(
		[&BinaryData](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded) -> void {
		if (!bSucceeded)
		{
			UE_LOG(BIMLOG, Warning, TEXT("Failed to download!"));
			return;
		}
		BinaryData = HttpResponse->GetContent();
	});
	HttpRequest->ProcessRequest();

	double LastTime = FPlatformTime::Seconds();
	while (EHttpRequestStatus::Processing == HttpRequest->GetStatus())
	{
		const double AppTime = FPlatformTime::Seconds();
		FHttpModule::Get().GetHttpManager().Tick(AppTime - LastTime);
		LastTime = AppTime;
		FPlatformProcess::Sleep(0.5f);
	}

	if (HttpRequest->GetStatus() != EHttpRequestStatus::Succeeded)
	{
		return false;
	}

	FMemoryReader Ar = FMemoryReader(BinaryData, true);

	TCHAR bgsChr[3] = { (TCHAR)BinaryData[2], (TCHAR)BinaryData[3], (TCHAR)BinaryData[4] };
	FString bgsStr = FString(bgsChr);
	if (bgsStr.Equals("BGS") == false)
	{
		UE_LOG(BIMLOG, Warning, TEXT("NO BGS Identifier found"));
		return false;
	}

	Ar.Seek(5);
	uint8 versInt;
	Ar << versInt;
	UE_LOG(BIMLOG, Log, TEXT("Found Binary Geometry Version \"%i\""), versInt);

	float objMinBX;
	float objMinBY;
	float objMinBZ;
	float objMaxBX;
	float objMaxBY;
	float objMaxBZ;

	Ar << objMinBX;
	Ar << objMinBY;
	Ar << objMinBZ;
	Ar << objMaxBX;
	Ar << objMaxBY;
	Ar << objMaxBZ;

	int objCount;
	Ar << objCount;

	TMap<int64, URuntimeMeshComponent*> CreatedMeshes;
	for (int counter = 0; counter < objCount; ++counter)
	{
		FString classname = "";
		uint8 strlen;
		Ar << strlen; // discard first one, because...what the hell?
					  //UE_LOG(BIMLOG, Log, TEXT("Strlen: %i"), strlen);
		Ar << strlen; // actual length- 255 chars
					  //UE_LOG(BIMLOG, Log, TEXT("Strlen: %i"), strlen);

		if (strlen > 0)
		{
			TCHAR* chrArr = new TCHAR[strlen + 1];
			for (uint8 i = 0; i < strlen; ++i)
			{
				uint8 chrIn;
				Ar << chrIn;
				chrArr[i] = (TCHAR)chrIn;
			}
			chrArr[strlen] = '\0';
			classname = FString(chrArr);
		}

		int64 objOID;
		Ar << objOID;
		UE_LOG(BIMLOG, Log, TEXT("Loading '%s' with OID '%i'"), *classname, objOID);

		uint8 geometryType;
		Ar << geometryType;

		//jump to next alignment of 4 
		int64 pos = Ar.Tell();
		int64 skip = 4 - (pos % 4);
		if (skip > 0 && skip != 4) {
			Ar.Seek(pos + skip);
		}

		float RawMatrix[4][4];
		for (int i = 0;i < 4;i++)
		{
			for (int j = 0;j < 4;j++)
			{
				Ar << RawMatrix[i][j];
			}
		}

		//oid of the geometry data
		int64 geometryDataId;
		Ar << geometryDataId;

		if (geometryType != 1)
		{
			//object bounds
			float objBounds[6];
			for (int i = 0; i < 6; ++i)
			{
				Ar << objBounds[i];
			}

			//indices
			int indicesLen;
			Ar << indicesLen;
			TArray<int32> Indices;
			Indices.AddUninitialized(indicesLen);
			for (int i = 0; i < indicesLen; i += 3)
			{
				int idx1, idx2, idx3;
				Ar << idx1;
				Ar << idx2;
				Ar << idx3;

				Indices[i + 0] = idx1;
				Indices[i + 1] = idx3;
				Indices[i + 2] = idx2;
			}

			//vertices
			int verticesLen;
			Ar << verticesLen;
			TArray<FVector> Vertices;
			Vertices.AddUninitialized(verticesLen);
			float lowestX = std::numeric_limits<float>::infinity();
			float highestX = -std::numeric_limits<float>::infinity();
			float lowestY = std::numeric_limits<float>::infinity();
			float highestY = -std::numeric_limits<float>::infinity();
			for (int i = 0; i < verticesLen / 3; ++i)
			{
				float x, y, z;
				Ar << x;
				Ar << y;
				Ar << z;
				lowestX = FMath::Min(lowestX, x);
				highestX = FMath::Max(highestX, x);
				lowestY = FMath::Min(lowestY, y);
				highestY = FMath::Max(highestY, y);
				Vertices[i] = FVector(x, y, z);
			}

			//tangents
			int normalsLen;
			Ar << normalsLen;

			TArray<FVector> Normals;
			Normals.AddUninitialized(Indices.Num());
			for (int i = 0; i < normalsLen / 3; ++i)
			{
				float x, y, z;
				Ar << x;
				Ar << y;
				Ar << z;
			}

			TArray<FRuntimeMeshTangent> Tangents;
			Tangents.AddUninitialized(Indices.Num());
			for (int i = 0; i < Indices.Num(); i += 3)
			{
				const int idx1 = i;
				const int idx2 = i + 1;
				const int idx3 = i + 2;
				FVector Tangent = Vertices[idx1] - Vertices[idx3];
				Tangents[idx1] = FRuntimeMeshTangent(Tangent.X, Tangent.Y, Tangent.Z);
				Tangents[idx2] = FRuntimeMeshTangent(Tangent.X, Tangent.Y, Tangent.Z);
				Tangents[idx3] = FRuntimeMeshTangent(Tangent.X, Tangent.Y, Tangent.Z);

				FVector NormalCurrent = FVector::CrossProduct(Tangent, Vertices[idx2] - Vertices[idx3]).GetSafeNormal();
				Normals[idx1] = NormalCurrent;
				Normals[idx2] = NormalCurrent;
				Normals[idx3] = NormalCurrent;
			}

			//colors
			int colorsLen;
			Ar << colorsLen;
			TArray<FColor> VertexColors;
			VertexColors.AddUninitialized(Indices.Num());
			for (int i = 0; i < colorsLen/4; ++i) {
				float r,g,b,a;
				Ar << r;
				Ar << g;
				Ar << b;
				Ar << a;
				VertexColors[i] = FColor(r, g, b, a);
			}

			// assign UVs
			float rangeX = (lowestX - highestX) * -1;
			float offsetX = 0 - lowestX;
			float rangeY = (lowestY - highestY) * -1;
			float offsetY = 0 - lowestY;
			TArray<FVector2D> TextureCoordinates;
			TextureCoordinates.AddUninitialized(Indices.Num());
			for (int i = 0;i < Indices.Num(); i++)
			{
				auto idx = Indices[i];
				auto Pos = Vertices[idx];
				TextureCoordinates[i] = { (Pos.X + offsetX),  (Pos.Y + offsetY) };
			}


			if (classname.Equals("IfcOpeningElement"))
			{
				continue;
			}

			if (CreatedMeshes.Contains(geometryDataId))
			{
				UE_LOG(BIMLOG, Warning, TEXT("Possibly corrupt file: Geometry ID duplication."));
			}

			auto RuntimeMesh = NewObject<URuntimeMeshComponent>(ReferencePoint);
			RuntimeMesh->CreateMeshSection(0, Vertices, Indices, Normals, TextureCoordinates, VertexColors, Tangents, true, EUpdateFrequency::Infrequent);
			RuntimeMesh->RegisterComponentWithWorld(ReferencePoint->GetWorld());
			const auto Trafo = FTransform(
				{ RawMatrix[0][0], RawMatrix[0][1], RawMatrix[0][2] },
				{ RawMatrix[1][0], RawMatrix[1][1], RawMatrix[1][2] },
				{ RawMatrix[2][0], RawMatrix[2][1], RawMatrix[2][2] },
				{ RawMatrix[3][0], RawMatrix[3][1], RawMatrix[3][2] }
			);
			RuntimeMesh->SetRelativeTransform(Trafo);
			ReferencePoint->AddOwnedComponent(RuntimeMesh);
			CreatedMeshes.Add(geometryDataId, RuntimeMesh);
		}
		else // is instance ...
		{
			if (!CreatedMeshes.Contains(geometryDataId))
			{
				UE_LOG(BIMLOG, Warning, TEXT("Possibly corrupt file: Geometry instance without geometry found."));
			}
			else
			{
				const auto Trafo = FTransform(
				{ RawMatrix[0][0], RawMatrix[0][1], RawMatrix[0][2] },
				{ RawMatrix[1][0], RawMatrix[1][1], RawMatrix[1][2] },
				{ RawMatrix[2][0], RawMatrix[2][1], RawMatrix[2][2] },
				{ RawMatrix[3][0], RawMatrix[3][1], RawMatrix[3][2] }
				);
				const IRuntimeMeshVerticesBuilder* VB;
				const FRuntimeMeshIndicesBuilder* IB;
				CreatedMeshes[geometryDataId]->GetSectionMesh(0, VB, IB);

				//TArray<FVector> Vertices;
				//Vertices.AddUninitialized(VB->Length());
				//TArray<FVector> Normals;
				//Normals.AddUninitialized(VB->Length());
				//TArray<FVector2> TextureCoordinates;
				//TextureCoordinates.AddUninitialized(VB->Length());
				//TArray<FColor> VertexColors;
				//VertexColors.AddUninitialized(VB->Length());
				//TArray<FRuntimeMeshTangent> Tangents;
				//Tangents.AddUninitialized(VB->Length());

				//TArray<int32> Indices;
				//Indices.AddUninitialized(IB->Length());

				//for (int i = 0;i < VB->Length(); ++i)
				//{
				//	Vertices[i] = 
				//}

				auto RuntimeMesh = NewObject<URuntimeMeshComponent>(ReferencePoint);
				//RuntimeMesh->CreateMeshSection(0, Vertices, Indices, Normals, TextureCoordinates, VertexColors, Tangents, true, EUpdateFrequency::Infrequent);
				RuntimeMesh->CreateMeshSection(0,*(VB->Clone()), *(IB->Clone()), true, EUpdateFrequency::Infrequent);
				RuntimeMesh->RegisterComponentWithWorld(ReferencePoint->GetWorld());
				ReferencePoint->AddOwnedComponent(RuntimeMesh);
				RuntimeMesh->SetRelativeTransform(Trafo);
			}
		}
	}

	UE_LOG(BIMLOG, Log, TEXT("SHIT IS WORKING!"));

	return true;
}