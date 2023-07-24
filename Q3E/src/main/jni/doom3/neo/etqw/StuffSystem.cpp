// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "StuffSystem.h"
#include "Player.h"
#include "Misc.h"
#include "../bse/BSEInterface.h"
#include "../bse/BSE_Envelope.h"
#include "../bse/BSE_SpawnDomains.h"
#include "../bse/BSE_Particle.h"
#include "../bse/BSE.h"

CLASS_DECLARATION( idEntity, sdStuffSystem )
END_CLASS

/*
================
sdStuffSystem::Spawn
================
*/
void sdStuffSystem::Spawn() {
	if ( !gameLocal.DoClientSideStuff() ) {
		return;
	}
	idStr mapdir = gameLocal.GetMapName();
	mapdir.StripPath();
	mapdir.StripFileExtension();

	modelDefHandle = -1;
	memset( &renderEntity, 0, sizeof( renderEntity ) );

	renderEntity.spawnID = gameLocal.GetSpawnId( this );//entityNumber;
	renderEntity.axis.Identity();

	renderEntity.flags.pushIntoOutsideAreas = spawnArgs.GetBool( "pushIntoOutsideAreas" );
	renderEntity.hModel = renderModelManager->FindModel( va( "stuff/%s/stuff.clust", mapdir.c_str() ) );
	renderEntity.bounds = renderEntity.hModel->Bounds();
	renderEntity.suppressSurfaceInViewID = MIRROR_VIEW_ID;

	BecomeActive( TH_UPDATEVISUALS );
	Present();
}
