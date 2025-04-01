/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "renderer/resources/Model_local.h"
#include "renderer/tr_local.h"	// just for R_FreeWorldInteractions and R_CreateWorldInteractions


class idRenderModelManagerLocal : public idRenderModelManager {
public:
							idRenderModelManagerLocal();
	virtual					~idRenderModelManagerLocal() override {}

	virtual void			Init() override;
	virtual void			Shutdown() override;
	virtual idRenderModel *	AllocModel() override;
	virtual void			FreeModel( idRenderModel *model ) override;
	virtual idRenderModel *	FindModel( const char *modelName ) override;
	virtual idRenderModel *	CheckModel( const char *modelName ) override;
	virtual idRenderModel *	DefaultModel() override;
	virtual void			AddModel( idRenderModel *model ) override;
	virtual void			RemoveModel( idRenderModel *model ) override;
	virtual void			ReloadModels( bool forceAll = false ) override;
	virtual void			FreeModelVertexCaches() override;
	virtual void			WritePrecacheCommands( idFile *file ) override;
	virtual void			BeginLevelLoad() override;
	virtual void			EndLevelLoad() override;

	virtual	void			PrintMemInfo( MemInfo_t *mi ) override;

private:
	idList<idRenderModel*>	models;
	idHashIndex				hash;
	idRenderModel *			defaultModel;
	idRenderModel *			beamModel;
	idRenderModel *			spriteModel;
	idRenderModel *			trailModel;
	bool					insideLevelLoad;		// don't actually load now

	idRenderModel *			GetModel( const char *modelName, bool createIfNotFound );

	static void				PrintModel_f( const idCmdArgs &args );
	static void				ListModels_f( const idCmdArgs &args );
	static void				ReloadModels_f( const idCmdArgs &args );
	static void				TouchModel_f( const idCmdArgs &args );
};


idRenderModelManagerLocal	localModelManager;
idRenderModelManager *		renderModelManager = &localModelManager;

/*
==============
idRenderModelManagerLocal::idRenderModelManagerLocal
==============
*/
idRenderModelManagerLocal::idRenderModelManagerLocal() {
	defaultModel = NULL;
	beamModel = NULL;
	spriteModel = NULL;
	insideLevelLoad = false;
	trailModel = NULL;
}

/*
==============
idRenderModelManagerLocal::PrintModel_f
==============
*/
void idRenderModelManagerLocal::PrintModel_f( const idCmdArgs &args ) {
	idRenderModel	*model;

	if ( args.Argc() != 2 ) {
		common->Printf( "usage: printModel <modelName>\n" );
		return;
	}

	model = renderModelManager->CheckModel( args.Argv( 1 ) );
	if ( !model ) {
		common->Printf( "model \"%s\" not found\n", args.Argv( 1 ) );
		return;
	}

	model->Print();
}

/*
==============
idRenderModelManagerLocal::ListModels_f
==============
*/
void idRenderModelManagerLocal::ListModels_f( const idCmdArgs &args ) {
	int		totalMem = 0;
	int		inUse = 0;

	common->Printf( " mem   srf verts tris\n" );
	common->Printf( " ---   --- ----- ----\n" );

	for ( int i = 0 ; i < localModelManager.models.Num() ; i++ ) {
		idRenderModel	*model = localModelManager.models[i];

		if ( !model->IsLoaded() ) {
			continue;
		}
		model->List();
		totalMem += model->Memory();
		inUse++;
	}

	common->Printf( " ---   --- ----- ----\n" );
	common->Printf( " mem   srf verts tris\n" );

	common->Printf( "%i loaded models\n", inUse );
	common->Printf( "total memory: %4.1fM\n", (float)totalMem / (1024*1024) );
}

/*
==============
idRenderModelManagerLocal::ReloadModels_f
==============
*/
void idRenderModelManagerLocal::ReloadModels_f( const idCmdArgs &args ) {
	if ( idStr::Icmp( args.Argv(1), "all" ) == 0 ) {
		localModelManager.ReloadModels( true );
	} else {
		localModelManager.ReloadModels( false );
	}
}

/*
==============
idRenderModelManagerLocal::TouchModel_f

Precache a specific model
==============
*/
void idRenderModelManagerLocal::TouchModel_f( const idCmdArgs &args ) {
	const char	*model = args.Argv( 1 );

	if ( !model[0] ) {
		common->Printf( "usage: touchModel <modelName>\n" );
		return;
	}

	common->Printf( "touchModel %s\n", model );
	session->UpdateScreen();
	idRenderModel *m = renderModelManager->CheckModel( model );
	if ( !m ) {
		common->Printf( "...not found\n" );
	}
}

/*
=================
idRenderModelManagerLocal::WritePrecacheCommands
=================
*/
void idRenderModelManagerLocal::WritePrecacheCommands( idFile *f ) {
	for ( int i = 0 ; i < models.Num() ; i++ ) {
		idRenderModel	*model = models[i];

		if ( !model ) {
			continue;
		}
		if ( !model->IsReloadable() ) {
			continue;
		}

		char	str[1024];
		sprintf( str, "touchModel %s\n", model->Name() );
		common->Printf( "%s", str );
		f->Printf( "%s", str );
	}
}

/*
=================
idRenderModelManagerLocal::Init
=================
*/
void idRenderModelManagerLocal::Init() {
	cmdSystem->AddCommand( "listModels", ListModels_f, CMD_FL_RENDERER, "lists all models" );
	cmdSystem->AddCommand( "printModel", PrintModel_f, CMD_FL_RENDERER, "prints model info", idCmdSystem::ArgCompletion_ModelName );
	cmdSystem->AddCommand( "reloadModels", ReloadModels_f, CMD_FL_RENDERER|CMD_FL_CHEAT, "reloads models" );
	cmdSystem->AddCommand( "touchModel", TouchModel_f, CMD_FL_RENDERER, "touches a model", idCmdSystem::ArgCompletion_ModelName );

	insideLevelLoad = false;

	// create a default model
	idRenderModelStatic *model = new idRenderModelStatic;
	model->InitEmpty( "_DEFAULT" );
	model->MakeDefaultModel();
	model->SetLevelLoadReferenced( true );
	defaultModel = model;
	AddModel( model );

	// create the beam model
	idRenderModelStatic *beam = new idRenderModelBeam;
	beam->InitEmpty( "_BEAM" );
	beam->SetLevelLoadReferenced( true );
	beamModel = beam;
	AddModel( beam );

	idRenderModelStatic *sprite = new idRenderModelSprite;
	sprite->InitEmpty( "_SPRITE" );
	sprite->SetLevelLoadReferenced( true );
	spriteModel = sprite;
	AddModel( sprite );
}

/*
=================
idRenderModelManagerLocal::Shutdown
=================
*/
void idRenderModelManagerLocal::Shutdown() {
	models.DeleteContents( true );
	hash.ClearFree();
}

/*
=================
idRenderModelManagerLocal::GetModel
=================
*/
idRenderModel *idRenderModelManagerLocal::GetModel( const char *modelName, bool createIfNotFound ) {
	idStr		canonical;
	idStr		extension;

	if ( !modelName || !modelName[0] ) {
		return NULL;
	}

	canonical = modelName;
	canonical.ToLower();

	// see if it is already present
	int key = hash.GenerateKey( modelName, false );
	for ( int i = hash.First( key ); i != -1; i = hash.Next( i ) ) {
		idRenderModel *model = models[i];

		if ( canonical.Icmp( model->Name() ) == 0 ) {
			if ( !model->IsLoaded() ) {
				// reload it if it was purged
				model->LoadModel();
			} else if ( insideLevelLoad && !model->IsLevelLoadReferenced() ) {
				// we are reusing a model already in memory, but
				// touch all the materials to make sure they stay
				// in memory as well
				model->TouchData();
			}
			model->SetLevelLoadReferenced( true );
			return model;
		}
	}

	// see if we can load it
	canonical.ExtractFileExtension( extension );

	// determine which subclass of idRenderModel to initialize
	idRenderModel *model = nullptr;
	if ( extension.Icmp( "ase" ) == 0 || extension.Icmp( "lwo" ) == 0 || extension.Icmp( "obj" ) == 0 ) {
		model = new idRenderModelStatic;
	} else if ( extension.Icmp( "ma" ) == 0 || extension.Icmp( "flt" ) == 0 ) {
		model = new idRenderModelStatic;
	} else if ( extension.Icmp( "proxy" ) == 0 ) {
		//stgatilov #4970: proxy models substitute rotation hack
		model = new idRenderModelStatic;
	} else if ( extension.Icmp( MD5_MESH_EXT ) == 0 ) {
		model = new idRenderModelMD5;
	} else if ( extension.Icmp( "md3" ) == 0 ) {
		model = new idRenderModelMD3;
	} else if ( extension.Icmp( "prt" ) == 0  ) {
		model = new idRenderModelPrt;
	} else if ( extension.Icmp( "liquid" ) == 0  ) {
		model = new idRenderModelLiquid;
	}

	if (model) {
		//do actual load
		model->InitEmpty(modelName);	//make sure name is set during BeginModelLoad!
		TRACE_CPU_SCOPE_TEXT("Load:Model", model->Name())
		declManager->BeginModelLoad(model);
		model->InitFromFile( modelName );
		declManager->EndModelLoad(model);
	}
	else {
		//can't load: make default
		if ( extension.Length() ) {
			common->Warning( "unknown model type '%s'", canonical.c_str() );
		}

		if ( !createIfNotFound ) {
			return NULL;
		}

		idRenderModelStatic	*smodel = new idRenderModelStatic;
		smodel->InitEmpty( modelName );
		smodel->MakeDefaultModel();

		model = smodel;
	}

	model->SetLevelLoadReferenced( true );

	if ( !createIfNotFound && model->IsDefaultModel() ) {
		delete model;
		model = NULL;

		return NULL;
	}

	AddModel( model );

	return model;
}

/*
=================
idRenderModelManagerLocal::AllocModel
=================
*/
idRenderModel *idRenderModelManagerLocal::AllocModel() {
	return new idRenderModelStatic();
}

/*
=================
idRenderModelManagerLocal::FreeModel
=================
*/
void idRenderModelManagerLocal::FreeModel( idRenderModel *model ) {
	if ( !model ) {
		return;
	}
	if ( !dynamic_cast<idRenderModelStatic *>( model ) ) {
		common->Error( "idRenderModelManager::FreeModel: model '%s' is not a static model", model->Name() );
		return;
	}
	if ( model == defaultModel ) {
		common->Error( "idRenderModelManager::FreeModel: can't free the default model" );
		return;
	}
	if ( model == beamModel ) {
		common->Error( "idRenderModelManager::FreeModel: can't free the beam model" );
		return;
	}
	if ( model == spriteModel ) { 
		common->Error( "idRenderModelManager::FreeModel: can't free the sprite model" );
		return;
	}

	R_CheckForEntityDefsUsingModel( model );

	delete model;
}

/*
=================
idRenderModelManagerLocal::FindModel
=================
*/
idRenderModel *idRenderModelManagerLocal::FindModel( const char *modelName ) {
	return GetModel( modelName, true );
}

/*
=================
idRenderModelManagerLocal::CheckModel
=================
*/
idRenderModel *idRenderModelManagerLocal::CheckModel( const char *modelName ) {
	return GetModel( modelName, false );
}

/*
=================
idRenderModelManagerLocal::DefaultModel
=================
*/
idRenderModel *idRenderModelManagerLocal::DefaultModel() {
	return defaultModel;
}

/*
=================
idRenderModelManagerLocal::AddModel
=================
*/
void idRenderModelManagerLocal::AddModel( idRenderModel *model ) {
	hash.Add( hash.GenerateKey( model->Name(), false ), models.Append( model ) );
}

/*
=================
idRenderModelManagerLocal::RemoveModel
=================
*/
void idRenderModelManagerLocal::RemoveModel( idRenderModel *model ) {
	int index = models.FindIndex( model );
	hash.RemoveIndex( hash.GenerateKey( model->Name(), false ), index );
	models.RemoveIndex( index );
}

/*
=================
idRenderModelManagerLocal::ReloadModels
=================
*/
void idRenderModelManagerLocal::ReloadModels( bool forceAll ) {
	if ( forceAll ) {
		common->Printf( "Reloading all model files...\n" );
	} else {
		common->Printf( "Checking for changed model files...\n" );
	}

	R_FreeDerivedData();

	// skip the default model at index 0
	for ( int i = 1 ; i < models.Num() ; i++ ) {
		idRenderModel	*model = models[i];

		// we may want to allow world model reloading in the future, but we don't now
		if ( !model->IsReloadable() ) {
			continue;
		}

		if ( !forceAll ) {
			// check timestamp
			ID_TIME_T current;

			fileSystem->ReadFile( model->Name(), NULL, &current );
			if ( current <= model->Timestamp() ) {
				continue;
			}
		}

		common->DPrintf( "reloading %s.\n", model->Name() );

		model->LoadModel();
	}

	// we must force the world to regenerate, because models may
	// have changed size, making their references invalid
	R_ReCreateWorldReferences();
}

/*
=================
idRenderModelManagerLocal::FreeModelVertexCaches
=================
*/
void idRenderModelManagerLocal::FreeModelVertexCaches() {
	for ( int i = 0 ; i < models.Num() ; i++ ) {
		idRenderModel *model = models[i];
		model->FreeVertexCache();
	}
}

/*
=================
idRenderModelManagerLocal::BeginLevelLoad
=================
*/
void idRenderModelManagerLocal::BeginLevelLoad() {
	insideLevelLoad = true;

	for ( int i = 0 ; i < models.Num() ; i++ ) {
		idRenderModel *model = models[i];

		if ( com_purgeAll.GetBool() && model->IsReloadable() ) {
			R_CheckForEntityDefsUsingModel( model );
			model->PurgeModel();
		}

		model->SetLevelLoadReferenced( false );
	}

	// purge unused triangle surface memory
	R_PurgeTriSurfData( frameData );
}

static idCVar r_vertexCacheStatic( "r_vertexCacheStatic", "1", CVAR_BOOL | CVAR_RENDERER, "Use static buffers in VertexCache?" );
static idCVarInt r_capModelSize( "r_capModelSize", "0", CVAR_TOOL, "" );
static idCVarBool r_modelSizeStats( "r_modelSizeStats", "0", CVAR_TOOL, "" );

/*
=================
idRenderModelManagerLocal::EndLevelLoad
=================
*/
void idRenderModelManagerLocal::EndLevelLoad() {
	common->Printf( "----- idRenderModelManagerLocal::EndLevelLoad -----\n" );

	int start = Sys_Milliseconds();

	insideLevelLoad = false;
	int	purgeCount = 0;
	int	keepCount = 0;
	int	loadCount = 0;

	// purge any models not touched
	for ( int i = 0 ; i < models.Num() ; i++ ) {
		idRenderModel *model = models[i];

		if ( !model->IsLevelLoadReferenced() && model->IsLoaded() && model->IsReloadable() ) {

//			common->Printf( "purging %s\n", model->Name() );

			purgeCount++;

			R_CheckForEntityDefsUsingModel( model );

			model->PurgeModel();

		} else {

//			common->Printf( "keeping %s\n", model->Name() );

			keepCount++;
		}
	}

	// purge unused triangle surface memory
	R_PurgeTriSurfData( frameData );

	// load any new ones
	for ( int i = 0 ; i < models.Num() ; i++ ) {
		idRenderModel *model = models[i];

		if ( model->IsLevelLoadReferenced() && !model->IsLoaded() && model->IsReloadable() ) {

			loadCount++;
			model->LoadModel();

			/* grayman #3763 - obsolete
			if ( ( loadCount & 15 ) == 0 ) {
				session->PacifierUpdate();
			}
			*/
		}
	}

	if (r_vertexCacheStatic.GetBool()) {
		std::map<int, int> modelStats;
		std::vector<int> modelSizes;
		std::vector<int> modelOffsets;

		// create static vertex/index buffers for all models
		for( int i = 0; i < models.Num(); i++ ) {
			idRenderModel *model = models[i];
			if( model->IsLoaded() ) {
				int totalSize = 0;
				for( int j = 0; j < model->NumSurfaces(); j++ ) {
					auto& tri = *model->Surface( j )->geometry;
					R_CreateStaticBuffersForTri( tri );
					totalSize += tri.ambientCache.size;
				}
				if ( r_capModelSize > 0 ) {
					if ( totalSize > 1024 * r_capModelSize )
						common->Warning( "Model capped: %dKB - %s\n", totalSize / 1024, model->Name() );
				}
				if ( r_modelSizeStats ) {
					modelSizes.push_back( totalSize );
					extern uint32_t staticVertexSize;
					modelOffsets.push_back( staticVertexSize );
					int index = 0;
					while ( totalSize ) {
						totalSize /= 10;
						modelStats[index]++;
						index++;
					}
				}
			}
		}
		if ( r_modelSizeStats ) {
			common->Printf( "Total models: %d, ", models.Num() );
			int size = 1;
			for ( auto it = modelStats.begin(); it != modelStats.end(); ++it )
			{
				int value = it->second;
				common->Printf( ">%d bytes: %d, ", size, value );
				size *= 10;
			}
			common->Printf( "\n" );
			int rollGrp = 1, rollSum = 0;
			int modelNum = int(modelSizes.size());
			common->Printf( "Size distribution grouped by 10%%, from low to high:\n" );
			for ( int index = 0; index < modelNum; index++ ) {
				rollSum += modelSizes[index];
				if ( index == modelNum * rollGrp / 10 - 1 ) {
					common->Printf( "  %d%%: %d KB\n", rollGrp * 10, rollSum / 1024 );
					rollGrp++;
				}
			}
			common->Printf( "Offset distribution grouped by 10%%, from low to high:\n" );
			rollGrp = 1;
			for ( int index = 0; index < modelNum; index++ ) {
				if ( index == modelNum * rollGrp / 10 - 1 ) {
					common->Printf( "  %d%%: %d KB\n", rollGrp * 10, modelOffsets[index] / 1024 );
					rollGrp++;
				}
			}
			std::sort( modelSizes.begin(), modelSizes.end() );
			rollGrp = 1, rollSum = 0;
			common->Printf( "Size distribution grouped by 10%%, from smallest to biggest:\n" );
			for ( int index = 0; index < modelNum; index++ ) {
				rollSum += modelSizes[index];
				if ( index == modelNum * rollGrp / 10 - 1 ) {
					common->Printf( "  %d%%: %d KB\n", rollGrp * 10, rollSum / 1024 );
					rollGrp++;
				}
			}
			rollSum = 0, rollGrp = 0;
			common->Printf( "Size distribution - top N:\n" );
			for ( int index = modelNum - 1; index >= 0; index-- ) {
				rollSum += modelSizes[index];
				if( modelNum - index == (int64)1 << (rollGrp * 2)) {
					common->Printf( "  top %d: %d KB\n", 1 << ( rollGrp * 2 ), rollSum / 1024 );
					rollGrp++;
				}
			}
		}
	}

	vertexCache.PrepareStaticCacheForUpload();
	// previous frame contents are now invalid, purge them
	R_ToggleSmpFrame();
	R_ToggleSmpFrame();

	// _D3XP added this
	int	end = Sys_Milliseconds();
	common->Printf( "%5i models purged from previous level, ", purgeCount );
	common->Printf( "%5i models kept.\n", keepCount );
	if ( loadCount ) {
		common->Printf( "%5i new models loaded in %5.1f seconds\n", loadCount, (end-start) * 0.001 );
	}
	common->Printf( "---------------------------------------------------\n" );
}

/*
=================
idRenderModelManagerLocal::PrintMemInfo
=================
*/
void idRenderModelManagerLocal::PrintMemInfo( MemInfo_t *mi ) {
	int i, j, totalMem = 0;
	int *sortIndex;
	idFile *f;

	f = fileSystem->OpenFileWrite( mi->filebase + "_models.txt" );
	if ( !f ) {
		return;
	}

	// sort first
	sortIndex = new int[ localModelManager.models.Num()];

	for ( i = 0; i <  localModelManager.models.Num(); i++ ) {
		sortIndex[i] = i;
	}

	for ( i = 0; i <  localModelManager.models.Num() - 1; i++ ) {
		for ( j = i + 1; j <  localModelManager.models.Num(); j++ ) {
			if (  localModelManager.models[sortIndex[i]]->Memory() <  localModelManager.models[sortIndex[j]]->Memory() ) {
				int temp = sortIndex[i];
				sortIndex[i] = sortIndex[j];
				sortIndex[j] = temp;
			}
		}
	}

	// print next
	for ( int i = 0 ; i < localModelManager.models.Num() ; i++ ) {
		idRenderModel	*model = localModelManager.models[sortIndex[i]];
		int mem;

		if ( !model->IsLoaded() ) {
			continue;
		}

		mem = model->Memory();
		totalMem += mem;
		f->Printf( "%s %s\n", idStr::FormatNumber( mem ).c_str(), model->Name() );
	}

	delete sortIndex;
	mi->modelAssetsTotal = totalMem;

	f->Printf( "\nTotal model bytes allocated: %s\n", idStr::FormatNumber( totalMem ).c_str() );
	fileSystem->CloseFile( f );
}
