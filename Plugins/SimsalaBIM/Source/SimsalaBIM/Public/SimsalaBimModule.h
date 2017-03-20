#pragma once 

#include "ModuleManager.h"


class FSimsalaBimModule
	: public IModuleInterface
{
private:
	FString LoginToken;

public:
	/** IModuleInterface implementation */
	void StartupModule();
	void ShutdownModule();
};


DECLARE_LOG_CATEGORY_EXTERN(BIMLOG, Log, All);