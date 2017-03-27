#include "SimsalaBimPCH.h"

#include "SimsalaBimModule.h"

#include "Engine.h"
#include "Http.h"

#include "SimsalaBimFunctionLibrary.h"



void FSimsalaBimModule::StartupModule()
{
	// Auto-login
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