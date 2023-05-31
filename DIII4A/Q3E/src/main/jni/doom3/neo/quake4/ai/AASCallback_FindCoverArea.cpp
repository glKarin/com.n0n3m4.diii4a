// Copyright (C) 2007 Id Software, Inc.
//



#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "AAS_local.h"

#include "AASCallback_FindCoverArea.h"

/*
============
idAASCallback_FindCoverArea::idAASCallback_FindCoverArea
============
*/
idAASCallback_FindCoverArea::idAASCallback_FindCoverArea( const idVec3& hideFromPos )
{
	int			numPVSAreas;
	idBounds	bounds( hideFromPos - idVec3( 16, 16, 0 ), hideFromPos + idVec3( 16, 16, 64 ) );

	// setup PVS
	numPVSAreas = gameLocal.pvs.GetPVSAreas( bounds, PVSAreas, MAX_PVS_AREAS );
	hidePVS		= gameLocal.pvs.SetupCurrentPVS( PVSAreas, numPVSAreas );
}

/*
============
idAASCallback_FindCoverArea::~idAASCallback_FindCoverArea
============
*/
idAASCallback_FindCoverArea::~idAASCallback_FindCoverArea()
{
	gameLocal.pvs.FreeCurrentPVS( hidePVS );
}

/*
============
idAASCallback_FindCoverArea::AreaIsGoal
============
*/
bool idAASCallback_FindCoverArea::AreaIsGoal( const idAAS* aas, int areaNum )
{
	idVec3	areaCenter;
	int		numPVSAreas;
	int		PVSAreas[ MAX_PVS_AREAS ];

	areaCenter = aas->AreaCenter( areaNum );
	areaCenter[ 2 ] += 1.0f;

	numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( areaCenter ).Expand( 16.0f ), PVSAreas, MAX_PVS_AREAS );
	if( !gameLocal.pvs.InCurrentPVS( hidePVS, PVSAreas, numPVSAreas ) )
	{
		return true;
	}

	return false;
}
