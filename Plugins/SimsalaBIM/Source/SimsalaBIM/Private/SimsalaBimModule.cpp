#include "SimsalaBimPCH.h"

#include "SimsalaBimModule.h"

#include "Engine.h"
#include "Http.h"

#include "StaticMeshResources.h"

/*
class FCustomMeshVertexBuffer 
	: public FVertexBuffer
{
public:
	TArray<FStaticMeshBuildVertex> Vertices;

	virtual void InitRHI()
	{
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(FStaticMeshBuildVertex), BUF_Static);

		// Copy the vertex data into the vertex buffer.
		void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI, 0, Vertices.Num() * sizeof(FStaticMeshBuildVertex), RLM_WriteOnly);
		FMemory::Memcpy(VertexBufferData, &Vertices[0], Vertices.Num() * sizeof(FStaticMeshBuildVertex));
		RHIUnlockVertexBuffer(VertexBufferRHI);
	}

};

class FCustomMeshIndexBuffer 
	: public FIndexBuffer
{
public:
	TArray<uint32> Indices;

	virtual void InitRHI()
	{
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint32), Indices.Num() * sizeof(uint32), NULL, BUF_Static);

		// Write the indices to the index buffer.
		void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Indices.Num() * sizeof(uint32), RLM_WriteOnly);
		FMemory::Memcpy(Buffer, &Indices[0], Indices.Num() * sizeof(uint32));
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
};
*/

void FSimsalaBimModule::StartupModule()
{
	/*
	const TCHAR* const BimFilename = TEXT("z:/dennio/bimserver.json");
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, BimFilename))
	{
		UE_LOG(BIMLOG, Error, TEXT("Failed to load file: %s"), BimFilename);
		return;
	}
	TSharedRef<TJsonReader<>> Reader = FJsonStringReader::Create(JsonString);
	TSharedPtr<FJsonObject> BimJson;
	if (!FJsonSerializer::Deserialize(Reader, BimJson))
	{
		UE_LOG(BIMLOG, Error, TEXT("Failed to parse file: %s"), BimFilename);
		return;
	}

	const TArray< TSharedPtr<FJsonValue> > *BimContent;
	if (!BimJson->TryGetArrayField(TEXT("objects"), BimContent))
	{
		UE_LOG(BIMLOG, Error, TEXT("Failed to parse file: '%s'. Ill-formatted."), BimFilename);
		return;
	}

	for (auto BimEntryVal : *BimContent)
	{
		const TSharedPtr<FJsonObject> *pBimEntry;
		if (BimEntryVal->TryGetObject(pBimEntry))
		{
			TSharedPtr<FJsonObject> BimEntry = *pBimEntry;
			FString EntryType;
			if (BimEntry->TryGetStringField(TEXT("_t"), EntryType))
			{
				if (EntryType == TEXT("GeometryData"))
				{
					FString VerticesStr, IndicesStr, MaterialsStr, NormalsStr;
					if (!BimEntry->TryGetStringField(TEXT("vertices"), VerticesStr) ||
						!BimEntry->TryGetStringField(TEXT("indices"), IndicesStr) ||
						!BimEntry->TryGetStringField(TEXT("materials"), MaterialsStr) ||
						!BimEntry->TryGetStringField(TEXT("normals"), NormalsStr))
					{
						UE_LOG(BIMLOG, Warning, TEXT("Ill-Formatted GeometryData-entry. Skipping."));
						return;
					}
					else
					{
						TArray<uint8> VerticesBin;
						TArray<FVector> Vertices;
						if (!FBase64::Decode(VerticesStr, VerticesBin))
						{
							UE_LOG(BIMLOG, Error, TEXT("GeometryData (vertices) corrupted."));
							return;
						}
						else
						{
							for (int i = 0;i < VerticesBin.Num() / 3; i += 12)
							{
								float* x = reinterpret_cast<float*>(&VerticesBin[3 * i + 0]);
								float* y = reinterpret_cast<float*>(&VerticesBin[3 * i + 4]);
								float* z = reinterpret_cast<float*>(&VerticesBin[3 * i + 8]);
								Vertices.Add(FVector(*x, *y, *z));
							}
						}

						TArray<uint8> IndicesBin;
						TArray<uint32> Indices;
						if (!FBase64::Decode(IndicesStr, IndicesBin))
						{
							UE_LOG(BIMLOG, Error, TEXT("GeometryData (indices) corrupted."));
							return;
						}
						else
						{
							for (int i = 0;i < IndicesBin.Num(); i += 4)
							{
								uint32* idx = reinterpret_cast<uint32*>(&IndicesBin[i]);
								Indices.Add(*idx);
							}
						}
					}
				}
			}
		}
	}
	*/

	TSharedPtr<FJsonObject> ParametersObject = MakeShareable(new FJsonObject);
	ParametersObject->SetStringField("username", TEXT("admin@local.host"));
	ParametersObject->SetStringField("password", TEXT("1"));

	TSharedPtr<FJsonObject> LoginObject = MakeShareable(new FJsonObject);
	LoginObject->SetObjectField("parameters", ParametersObject);
	LoginObject->SetStringField("interface", "Bimsie1AuthInterface");
	LoginObject->SetStringField("method", "login");

	TSharedPtr<FJsonObject> RequestObject = MakeShareable(new FJsonObject);
	RequestObject->SetObjectField("request", LoginObject);

	UE_LOG(BIMLOG, Log, TEXT("Logging in..."));
	FString Request;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&Request);
	FJsonSerializer::Serialize(RequestObject.ToSharedRef(), Writer);

	TSharedRef< IHttpRequest > HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("POST");
	HttpRequest->SetURL("http://localhost:8082/json");
	HttpRequest->SetHeader("Content-Type", "application/json");
	HttpRequest->SetContentAsString(Request);
	HttpRequest->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded) -> void {
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
			LoginToken = JsonResponse->GetStringField("result");
			UE_LOG(BIMLOG, Log, TEXT("Login successful! Token is: %s"), *LoginToken);
		});
	HttpRequest->ProcessRequest();
}

void FSimsalaBimModule::ShutdownModule()
{
}


DEFINE_LOG_CATEGORY(BIMLOG);


IMPLEMENT_MODULE(FSimsalaBimModule, SimsalaBIM)