// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics.h"
#include "Clip.h"

ABSTRACT_DECLARATION( idClass, idPhysics )
END_CLASS


/*
================
idPhysics::~idPhysics
================
*/
idPhysics::~idPhysics( void ) {
}


/*
================
idPhysics::SetClipBox
================
*/
void idPhysics::SetClipBox( const idBounds &bounds, float density ) {
	SetClipModel( new idClipModel( idTraceModel( bounds ), false ), density );
}

/*
================
idPhysics::EvaluatePosition
================
*/
idVec3 idPhysics::EvaluatePosition( void ) const {
	return GetBounds().GetCenter() * GetAxis() + GetOrigin();
}

/*
================
idPhysics::SnapTimeToPhysicsFrame
================
*/
int idPhysics::SnapTimeToPhysicsFrame( int t ) {
/*	int s;
	s = t + gameLocal.msec - 1;
	return ( s - s % gameLocal.msec );*/

	return t;
}

/*
================
idPhysics::WakeEntitiesContacting
================
*/
void idPhysics::WakeEntitiesContacting( idEntity* self, const idClipModel* clipModel ) {
	if ( clipModel->GetContents() == 0 ) {
		// do nothing with a disabled clip model
		return;
	}

	const idClipModel* otherModels[ MAX_GENTITIES ];
	int numModels = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS clipModel->GetAbsBounds(), -1, otherModels, MAX_GENTITIES, clipModel->GetEntity() );
	
	const idBounds& bounds = clipModel->GetBounds();
	idMat3 axisT = clipModel->GetAxis().Transpose();
	idVec3 origin = clipModel->GetOrigin();
	for ( int i = 0; i < numModels; i++ ) {
		const idClipModel* cm = otherModels[ i ];
		
		if ( cm->GetEntity() == NULL ) {
			continue;
		}

		// transform bounds into local space and check for intersection
		// not 100% accurate but good enough
		idBounds otherBounds = cm->GetAbsBounds();
		otherBounds.TranslateSelf( -origin );
		otherBounds.RotateSelf( axisT );

		if ( otherBounds.IntersectsBounds( bounds ) ) {
			cm->GetEntity()->ActivatePhysics();
		}
	}
}
