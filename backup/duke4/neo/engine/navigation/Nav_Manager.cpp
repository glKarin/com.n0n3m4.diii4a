// Nav_Manager.cpp
//


#include "Nav_local.h"

rvmNavigationManagerLocal navigationManagerLocal;
rvmNavigationManager *navigationManager = &navigationManagerLocal;

/*
============================
rvmNavigationManagerLocal::Init
============================
*/
void rvmNavigationManagerLocal::Init(void) {
	
}

/*
============================
rvmNavigationManagerLocal::LoadNavFile
============================
*/
rvmNavFile *rvmNavigationManagerLocal::LoadNavFile(const char *name) {
	for(int i = 0; i < navMeshFiles.Num(); i++)
	{
		if(!strcmp(navMeshFiles[i]->GetName(), name))
		{
			return navMeshFiles[i];
		}
	}

	// Try and load the navigation file.
	rvmNavFileLocal *navFile = new rvmNavFileLocal(name);
	if(!navFile->LoadFromFile())
	{
		common->Warning("LoadNavFile: Failed to load navfile %s\n", name);
		delete navFile;
		return nullptr;
	}

	navMeshFiles.Append(navFile);

	return navFile;
}

/*
============================
rvmNavigationManagerLocal::FreeNavFile
============================
*/
void rvmNavigationManagerLocal::FreeNavFile(rvmNavFile *navFile) {
	// Remove the nav file from the list.
	for(int i = 0; i < navMeshFiles.Num(); i++)
	{
		if(navMeshFiles[i] == navFile)
		{
			navMeshFiles.RemoveIndex(i);
			delete navFile;
			return;
		}
	}

	common->FatalError("FreeNavFile: navfile hasn't been loaded through the navigation manager!");
}