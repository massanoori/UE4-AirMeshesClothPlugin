// Copyright 2016 massanoori. All Rights Reserved.

#include "AirMeshClothPrivatePCH.h"
#include "AirMeshClothLog.h"


class FAirMeshCloth : public IAirMeshCloth
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FAirMeshCloth, AirMeshCloth )



void FAirMeshCloth::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FAirMeshCloth::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

DEFINE_LOG_CATEGORY(LogAirMeshCloth);
