#include "SimsalaBimPCH.h"

#include "SimsalaBimModule.h"

#include "Engine.h"
#include "Http.h"

#include "SimsalaBimFunctionLibrary.h"

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

	[]() {
		if (!GConfig)
		{
			UE_LOG(BIMLOG, Error, TEXT("Config unavailable."));
			return;
		}
		FString ServerName;
		if (!GConfig->GetString(TEXT("BIMServer"), TEXT("URL"), ServerName, GGameIni))
		{
			UE_LOG(BIMLOG, Error, TEXT("Config Server URL not set."));
			return;
		}
		FString Username;
		if (!GConfig->GetString(TEXT("BIMServer"), TEXT("Username"), Username, GGameIni))
		{
			UE_LOG(BIMLOG, Error, TEXT("Config username not set."));
			return;
		}
		FString Password;
		if (!GConfig->GetString(TEXT("BIMServer"), TEXT("Password"), Password, GGameIni))
		{
			UE_LOG(BIMLOG, Error, TEXT("Config password not set."));
			return;
		}
		if (!USimsalaBimFunctionLibrary::Connect(ServerName, Username, Password))
		{
			UE_LOG(BIMLOG, Error, TEXT("Failed to connect to %s as %s."), *ServerName, *Username);
			return;
		}

		UE_LOG(BIMLOG, Log, TEXT("Connection to %s as %s sucessfully established."), *ServerName, *Username);
	}();
	
}

void FSimsalaBimModule::ShutdownModule()
{
}


DEFINE_LOG_CATEGORY(BIMLOG);


IMPLEMENT_MODULE(FSimsalaBimModule, SimsalaBIM)