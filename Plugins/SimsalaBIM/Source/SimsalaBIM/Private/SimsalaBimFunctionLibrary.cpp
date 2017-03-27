#include "SimsalaBimPCH.h"
#include "SimsalaBimFunctionLibrary.h"
#include "SimsalaBimModule.h"

#include "Engine.h"
#include "Base64.h"
#include "Http.h"

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
				UE_LOG(BIMLOG, Error, TEXT("Failed to login!"));
				return;
			}
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				UE_LOG(BIMLOG, Error, TEXT("Failed to deserialize login response: %s"), *HttpResponse->GetContentAsString());
				return;
			}
			TSharedPtr<FJsonObject> JsonResponse = JsonObject->GetObjectField("response");
			USimsalaBimFunctionLibrary::CachedToken = JsonResponse->GetStringField("result");
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

	TCHAR bgsChr[4] = { (TCHAR)BinaryData[2], (TCHAR)BinaryData[3], (TCHAR)BinaryData[4], (TCHAR)0 };
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