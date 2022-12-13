#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_ModelDefHandleIsValid( "<modelDefHandleIsValid>" );

CLASS_DECLARATION( idEntity, hhRenderEntity )
	EVENT( EV_ModelDefHandleIsValid,	hhRenderEntity::Event_ModelDefHandleIsValid )
END_CLASS

/*
==============
hhRenderEntity::hhRenderEntity
==============
*/
hhRenderEntity::hhRenderEntity() {
	combatModel = NULL;
}

/*
==============
hhRenderEntity::~hhRenderEntity
==============
*/
hhRenderEntity::~hhRenderEntity() {
	SAFE_DELETE_PTR( combatModel );
}

void hhRenderEntity::Save(idSaveGame *savefile) const {
	savefile->WriteClipModel( combatModel );
}

void hhRenderEntity::Restore( idRestoreGame *savefile ) {
	savefile->ReadClipModel( combatModel );

	if( combatModel ) {
		const renderEntity_t *renderEntity = gameRenderWorld->GetRenderEntity( GetModelDefHandle() );
		if( renderEntity ) {
			combatModel->Link( gameLocal.clip, this, 0, renderEntity->origin, renderEntity->axis, GetModelDefHandle() );
		}
	}
}

/*
==============
hhRenderEntity::Think
==============
*/
void hhRenderEntity::Think() {
	idEntity::Think();

	LinkCombatModel( this, GetModelDefHandle() );
}

/*
==============
hhRenderEntity::InitCombatModel
==============
*/
void hhRenderEntity::InitCombatModel( const int renderModelHandle ) {
    if ( combatModel ) {
		combatModel->Unlink();
		combatModel->LoadModel( renderModelHandle );
		combatModel->Link( gameLocal.clip );//Force a reclip because our origin and axis hasn't changed
	} else {
		combatModel = new idClipModel( renderModelHandle );
		HH_ASSERT( combatModel );
	}
}

/*
==============
hhRenderEntity::LinkCombatModel
==============
*/
void hhRenderEntity::LinkCombatModel( idEntity* self, const int renderModelHandle ) {
	if( combatModel && self && renderModelHandle != -1 ) {
		const renderEntity_t *renderEntity = gameRenderWorld->GetRenderEntity( renderModelHandle );
		if( !renderEntity ) {
			return;
		}

		if( combatModel->GetOrigin() != renderEntity->origin || combatModel->GetAxis() != renderEntity->axis || combatModel->GetBounds() != renderEntity->bounds ) {
			combatModel->Link( gameLocal.clip, self, 0, renderEntity->origin, renderEntity->axis, renderModelHandle );
		}
	}
}

/*
==============
hhRenderEntity::Present

AOBMERGE: PLEASE REMOVE THIS WHEN WE GET idRenderEntity
==============
*/
void hhRenderEntity::Present( void ) {
	PROFILE_SCOPE("Present", PROFMASK_NORMAL);

	if ( !gameLocal.isNewFrame ) {
		return;
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	// camera target for remote render views
	if ( cameraTarget && gameLocal.InPlayerPVS( this ) ) {
		renderEntity.remoteRenderView = cameraTarget->GetRenderView();
	}

	// if set to invisible, skip
	if ( !renderEntity.hModel || IsHidden() ) {
		return;
	}

	// add to refresh list
	if ( modelDefHandle == -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
		//HUMANHEAD: aob - needed for combat models
		PostEventMS( &EV_ModelDefHandleIsValid, 0 );
		//HUMANHEAD END
	} else {
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}
}

/*
==============
hhRenderEntity::Event_ModelDefHandleIsValid
==============
*/
void hhRenderEntity::Event_ModelDefHandleIsValid() {
	if( spawnArgs.GetBool("useCombatModel") ) {
		InitCombatModel( GetModelDefHandle() );
		LinkCombatModel( this, GetModelDefHandle() );
	}
}