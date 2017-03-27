#pragma once

#include "Engine.h"
#include "SimsalaBimFunctionLibrary.generated.h"



UCLASS(BlueprintType)
class UIfcProject
	: public UObject
{
GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FString	Name;

	uint64 OID;
};



UCLASS()
class USimsalaBimFunctionLibrary
	: public UBlueprintFunctionLibrary
{
GENERATED_BODY()

public: // cannot set private...
	static uint64 SerializerOID;
	static FString CachedToken;
	static FString ServerName;

public:
	UFUNCTION(BlueprintCallable, Category = "BIM")
	static bool Connect(FString ServerName, FString UserName, FString Password);

	UFUNCTION(BlueprintCallable, Category = "BIM")
	static bool GetProjectList(TArray<UIfcProject*>& Projects);

	UFUNCTION(BlueprintCallable, Category = "BIM")
	static bool LoadProject(AActor* ReferencePoint, UIfcProject* Project);

	//ChangeDefaultMaterial
};