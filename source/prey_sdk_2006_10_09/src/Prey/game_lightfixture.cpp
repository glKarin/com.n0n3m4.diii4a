#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( hhAFEntity, hhLightFixture )
	EVENT( EV_PostSpawn,		hhLightFixture::Event_PostSpawn )
	EVENT( EV_Hide,				hhLightFixture::Event_Hide )
	EVENT( EV_Show,				hhLightFixture::Event_Show )
END_CLASS

/*
===============
hhLightFixture::Spawn
===============
*/
void hhLightFixture::Spawn() {
	collisionBone = INVALID_JOINT;
	boundLight = NULL;

	PostEventMS( &EV_PostSpawn, 10 );
}

void hhLightFixture::Save(idSaveGame *savefile) const {
	savefile->WriteInt( collisionBone );
	boundLight.Save(savefile);
}

void hhLightFixture::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( (int &)collisionBone );
	boundLight.Restore(savefile);
}

/*
===============
hhLightFixture::~hhLightFixture
===============
*/
hhLightFixture::~hhLightFixture() {
	RemoveLight();
}

/*
===============
hhLightFixture::Damage
===============
*/
void hhLightFixture::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if( collisionBone != INVALID_JOINT && (location == INVALID_JOINT || collisionBone == location) ) {
		hhAFEntity::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	}
}

/*
===============
hhLightFixture::Killed
===============
*/
void hhLightFixture::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	hhAFEntity::Killed( inflictor, attacker, damage, dir, location );

	if( StillBound(boundLight.GetEntity()) ) {
		//boundLight->SetShader( spawnArgs.GetString("mtr_lightDestroyed") );
		boundLight->BecomeBroken( attacker );
	}

	const char* skinName = spawnArgs.GetString( "skin_destroyed" );
	if( skinName && skinName[0] ) {
		SetSkinByName( skinName );
	}

	// offset the start time of the shader to sync it to the game time
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	// set the state parm
	renderEntity.shaderParms[ SHADERPARM_MODE ] = 1;

	UpdateVisuals();
}

/*
===============
hhLightFixture::GetBoundLight
===============
*/
void hhLightFixture::GetBoundLight() {
	idVec3		color;

	boundLight = SearchForBoundLight();
	if( boundLight.IsValid() ) {
		boundLight->fl.takedamage = false;

		collisionBone = GetAnimator()->GetJointHandle( boundLight->spawnArgs.GetString("bindToJoint") );

		SetColor( boundLight->spawnArgs.GetVector("_color", "1 1 1") );
	}
}


/*
===============
hhLightFixture::SearchForBoundLight
===============
*/
idLight* hhLightFixture::SearchForBoundLight() {
	for( idEntity* entity = GetTeamChain(); entity; entity = entity->GetTeamChain() ) {
		if( entity && entity->IsType(idLight::Type) ) {
			return static_cast<idLight*>( entity );
		}
	}

	return NULL;
}

/*
===============
hhLightFixture::StillBound
===============
*/
bool hhLightFixture::StillBound( const idLight* light ) {
	return (light) ? light->IsBoundTo(this) : false;
}

/*
===============
hhLightFixture::RemoveLight
===============
*/
void hhLightFixture::RemoveLight() {
	SAFE_REMOVE( boundLight );
}

/*
================
hhLightFixture::Present
================
*/
void hhLightFixture::Present( void ) {
	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}

	// add the model
	hhAFEntity::Present();

	// reference the sound for shader synced effects
	if ( boundLight.IsValid() && StillBound(boundLight.GetEntity()) ) {
		renderEntity.referenceSound = boundLight->GetSoundEmitter();
	}
	else {
		renderEntity.referenceSound = refSound.referenceSound;
	}

	PresentModelDefChange();
}

/*
================
hhLightFixture::PresentModelDefChange
================
*/
void hhLightFixture::PresentModelDefChange( void ) {

	if ( !renderEntity.hModel || IsHidden() ) {
		return;
	}

	// add to refresh list
	if ( modelDefHandle == -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}
}

/*
===============
hhLightFixture::Event_PostSpawn
===============
*/
void hhLightFixture::Event_PostSpawn() {
	GetBoundLight();
}

/*
===============
hhLightFixture::Event_Hide
===============
*/
void hhLightFixture::Event_Hide() {
	idEntity::Event_Hide();

	if( boundLight.IsValid() ) {
		boundLight->Off();
	}
}

/*
===============
hhLightFixture::Event_Show
===============
*/
void hhLightFixture::Event_Show() {
	idEntity::Event_Show();

	if( boundLight.IsValid() ) {
		boundLight->On();
	}
}