//----------------------------------------------------------------
// ClientModel.cpp
//
// A non-interactive client-side model
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "ClientModel.h"

/*
===============================================================================

rvClientModel

===============================================================================
*/
CLASS_DECLARATION( rvClientEntity, rvClientModel )
END_CLASS


/*
================
rvClientModel::rvClientModel
================
*/
rvClientModel::rvClientModel ( void ) {
	memset ( &renderEntity, 0, sizeof(renderEntity) );
	worldAxis = mat3_identity;
	entityDefHandle = -1;
}

/*
================
rvClientModel::~rvClientModel
================
*/
rvClientModel::~rvClientModel ( void ) {
	FreeEntityDef ( );
}

/*
================
rvClientModel::FreeEntityDef
================
*/
void rvClientModel::FreeEntityDef ( void ) {
	if ( entityDefHandle >= 0 ) {
		gameRenderWorld->FreeEntityDef ( entityDefHandle );
		entityDefHandle = -1;
	}	
}

/*
================
rvClientModel::Spawn
================
*/
void rvClientModel::Spawn ( void ) {
	const char* spawnarg;

	spawnArgs.GetString ( "classname", "", classname );

	// parse static models the same way the editor display does
	gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &renderEntity );

	renderEntity.entityNum = entityNumber;

	spawnarg = spawnArgs.GetString( "model" );
	if ( spawnarg && *spawnarg ) {
		SetModel( spawnarg );
	}
}

/*
================
rvClientModel::Think
================
*/
void rvClientModel::Think ( void ) {
	if( bindMaster && (bindMaster->GetRenderEntity()->hModel && bindMaster->GetModelDefHandle() == -1) ) {
		return;
	}
	UpdateBind();
	Present();
}

/*
================
rvClientModel::Present
================
*/
void rvClientModel::Present(void) {
	// Hide client entities bound to a hidden entity
	if ( bindMaster && (bindMaster->IsHidden ( ) || (bindMaster->GetRenderEntity()->hModel && bindMaster->GetModelDefHandle() == -1) ) ) {
		return;
	}

	renderEntity.origin = worldOrigin;
	renderEntity.axis = worldAxis;

	// add to refresh list
	if ( entityDefHandle == -1 ) {
		entityDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( entityDefHandle, &renderEntity );
	}		
}

/*
================
rvClientModel::SetCustomShader
================
*/
bool rvClientModel::SetCustomShader ( const char* shaderName ) {
	if ( shaderName == NULL ) {
		return false;
	}
	
	const idMaterial* material = declManager->FindMaterial( shaderName );

	if ( material == NULL ) {
		return false;
	}
	
	renderEntity.customShader = material;

	return true;
}

/*
================
rvClientModel::Save
================
*/
void rvClientModel::Save( idSaveGame *savefile ) const {
	savefile->WriteRenderEntity( renderEntity );
	savefile->WriteInt( entityDefHandle );

	savefile->WriteString ( classname );	// cnicholson: Added unsaved var

}

/*
================
rvClientModel::Restore
================
*/
void rvClientModel::Restore( idRestoreGame *savefile ) {
	savefile->ReadRenderEntity( renderEntity, NULL );
	savefile->ReadInt( entityDefHandle );

	savefile->ReadString ( classname );		// cnicholson: Added unrestored var

	// restore must retrieve entityDefHandle from the renderer
 	if ( entityDefHandle != -1 ) {
 		entityDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
 	}
}

/*
================
rvClientModel::SetModel
================
*/
void rvClientModel::SetModel( const char* modelname ) {
	FreeEntityDef();

	renderEntity.hModel = renderModelManager->FindModel( modelname );

	if ( renderEntity.hModel ) {
		renderEntity.hModel->Reset();
	}

	renderEntity.callback = NULL;
	renderEntity.numJoints = 0;
	renderEntity.joints = NULL;
	if ( renderEntity.hModel ) {
		renderEntity.bounds = renderEntity.hModel->Bounds( &renderEntity );
	} else {
		renderEntity.bounds.Zero();
	}
}

/*
==============
rvClientModel::ProjectOverlay
==============
*/
void rvClientModel::ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material ) {
	float s, c;
	idMat3 axis, axistemp;
	idVec3 localOrigin, localAxis[2];
	idPlane localPlane[2];

	// make sure the entity has a valid model handle
	if ( entityDefHandle < 0 ) {
		return;
	}

	// only do this on dynamic md5 models
	if ( renderEntity.hModel->IsDynamicModel() != DM_CACHED ) {
		return;
	}

	idMath::SinCos16( gameLocal.random.RandomFloat() * idMath::TWO_PI, s, c );

	axis[2] = -dir;
	axis[2].NormalVectors( axistemp[0], axistemp[1] );
	axis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * -s;
	axis[1] = axistemp[ 0 ] * -s + axistemp[ 1 ] * -c;

	renderEntity.axis.ProjectVector( origin - renderEntity.origin, localOrigin );
	renderEntity.axis.ProjectVector( axis[0], localAxis[0] );
	renderEntity.axis.ProjectVector( axis[1], localAxis[1] );

	size = 1.0f / size;
	localAxis[0] *= size;
	localAxis[1] *= size;

	localPlane[0] = localAxis[0];
	localPlane[0][3] = -( localOrigin * localAxis[0] ) + 0.5f;

	localPlane[1] = localAxis[1];
	localPlane[1][3] = -( localOrigin * localAxis[1] ) + 0.5f;

	const idMaterial *mtr = declManager->FindMaterial( material );

	// project an overlay onto the model
	gameRenderWorld->ProjectOverlay( entityDefHandle, localPlane, mtr );

	// make sure non-animating models update their overlay
	UpdateVisuals();
}

/*
================
rvClientModel::UpdateRenderEntity
================
*/
bool rvClientModel::UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) {
	if ( gameLocal.inCinematic && gameLocal.skipCinematic ) {
		return false;
	}

	idAnimator *animator = GetAnimator();
	if ( animator ) {
		return animator->CreateFrame( gameLocal.time, false );
	}

	return false;
}

/*
================
rvClientModel::ModelCallback

NOTE: may not change the game state whatsoever!
================
*/
bool rvClientModel::ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView ) {
	rvClientEntity *cent;

	cent = gameLocal.clientEntities[ renderEntity->entityNum ];
	if ( !cent ) {
		gameLocal.Error( "rvClientModel::ModelCallback: callback with NULL client entity '%d'", renderEntity->entityNum );
		return false;
	}

	if( !cent->IsType( rvClientModel::GetClassType() ) ) {
		gameLocal.Error( "rvClientModel::ModelCallback: callback with non-client model on client entity '%d'", renderEntity->entityNum );
		return false;
	}

	return ((rvClientModel*)cent)->UpdateRenderEntity( renderEntity, renderView );
}

/*
================
rvClientModel::GetPhysicsToVisualTransform
================
*/
bool rvClientModel::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	return false;
}

/*
================
rvClientModel::UpdateModelTransform
================
*/
void rvClientModel::UpdateModelTransform( void ) {
	idVec3 origin;
	idMat3 axis;

	if ( GetPhysicsToVisualTransform( origin, axis ) ) {
		renderEntity.axis = axis * worldAxis;
		renderEntity.origin = worldOrigin + origin * renderEntity.axis;
	} else {
		renderEntity.axis = worldAxis;
		renderEntity.origin = worldOrigin;
	}
}

/*
================
rvClientModel::UpdateModel
================
*/
void rvClientModel::UpdateModel( void ) {
	UpdateModelTransform();

	idAnimator *animator = GetAnimator();
	if ( animator && animator->ModelHandle() ) {
		// set the callback to update the joints
		renderEntity.callback = rvClientModel::ModelCallback;
	}
}

/*
================
rvClientModel::UpdateVisuals
================
*/
void rvClientModel::UpdateVisuals( void ) {
	UpdateModel();
	UpdateSound();
}

/*
================
rvClientModel::SetSkin
================
*/
void rvClientModel::SetSkin( const idDeclSkin *skin ) {
	renderEntity.customSkin = skin;
	UpdateVisuals();
}

/*
===============================================================================

rvAnimatedClientEntity

===============================================================================
*/

CLASS_DECLARATION( rvClientModel, rvAnimatedClientEntity )
END_CLASS

/*
================
rvAnimatedClientEntity::rvAnimatedClientEntity
================
*/
rvAnimatedClientEntity::rvAnimatedClientEntity ( void ) {
}

/*
================
rvAnimatedClientEntity::~rvAnimatedClientEntity
================
*/
rvAnimatedClientEntity::~rvAnimatedClientEntity ( void ) {
}

/*
================
rvAnimatedClientEntity::Spawn
================
*/
void rvAnimatedClientEntity::Spawn( void ) {
	SetModel( spawnArgs.GetString( "model" ) );
}
/*
================
rvAnimatedClientEntity::Think
================
*/
void rvAnimatedClientEntity::Think ( void ) {
	UpdateAnimation();

	rvClientEntity::Think();
}

/*
================
rvAnimatedClientEntity::UpdateAnimation
================
*/
void rvAnimatedClientEntity::UpdateAnimation( void ) {
	// is the model an MD5?
	if ( !animator.ModelHandle() ) {
		// no, so nothing to do
		return;
	}

	// call any frame commands that have happened in the past frame
	animator.ServiceAnims( gameLocal.previousTime, gameLocal.time );

	// if the model is animating then we have to update it
	if ( !animator.FrameHasChanged( gameLocal.time ) ) {
		// still fine the way it was
		return;
	}

	// get the latest frame bounds
	animator.GetBounds( gameLocal.time, renderEntity.bounds );
	if ( renderEntity.bounds.IsCleared() ) {
		gameLocal.DPrintf( "rvAnimatedClientEntity %s %d: inside out bounds - %d\n", GetClassname(), entityNumber, gameLocal.time );
	}

	// update the renderEntity
	UpdateVisuals();
	Present();

	// the animation is updated
	animator.ClearForceUpdate();
}

/*
================
rvAnimatedClientEntity::SetModel
================
*/
void rvAnimatedClientEntity::SetModel( const char *modelname ) {
	FreeEntityDef();

	renderEntity.hModel = animator.SetModel( modelname );
	if ( !renderEntity.hModel ) {
		rvClientModel::SetModel( modelname );
		return;
	}

	if ( !renderEntity.customSkin ) {
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
	}

	// set the callback to update the joints
	renderEntity.callback = rvClientModel::ModelCallback;
	animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
	animator.GetBounds( gameLocal.time, renderEntity.bounds );

	//UpdateVisuals();
	Present();
}
