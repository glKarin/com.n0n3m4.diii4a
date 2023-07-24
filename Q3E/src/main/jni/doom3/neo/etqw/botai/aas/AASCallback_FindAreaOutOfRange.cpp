// Copyright (C) 2007 Id Software, Inc.
//

#include "../../precompiled.h"
#pragma hdrstop

#include "../../ContentMask.h"

#include "AAS.h"
#include "AASCallback_FindAreaOutOfRange.h"

/*
============
idAASCallback_FindAreaOutOfRange::idAASCallback_FindAreaOutOfRange
============
*/
idAASCallback_FindAreaOutOfRange::idAASCallback_FindAreaOutOfRange( const idVec3 &targetPos, float maxDist ) {
	this->targetPos		= targetPos;
	this->maxDistSqr	= maxDist * maxDist;
}

/*
============
idAASCallback_FindAreaOutOfRange::AreaIsGoal
============
*/
bool idAASCallback_FindAreaOutOfRange::AreaIsGoal( const idAAS *aas, int areaNum ) {
	const idVec3 &areaCenter = aas->AreaCenter( areaNum );

	if ( maxDistSqr > 0.0f ) {
		float dist = ( targetPos.ToVec2() - areaCenter.ToVec2() ).LengthSqr();
		if ( dist < maxDistSqr ) {
			return false;
		}
	}

	trace_t	trace;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, targetPos, areaCenter + idVec3( 0.0f, 0.0f, 1.0f ), MASK_OPAQUE, NULL );
	if ( trace.fraction < 1.0f ) {
		return false;
	}

	return true;
}
