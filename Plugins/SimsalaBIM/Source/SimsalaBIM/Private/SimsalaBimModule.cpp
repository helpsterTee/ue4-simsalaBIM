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
*/
}

void FSimsalaBimModule::ShutdownModule()
{
}


DEFINE_LOG_CATEGORY(BIMLOG);


IMPLEMENT_MODULE(FSimsalaBimModule, SimsalaBIM)