#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/**********************************************************************

hhGibbable

**********************************************************************/

const idEventDef EV_Respawn("<respawn>");

CLASS_DECLARATION( hhAnimatedEntity, hhGibbable )
	EVENT( EV_Activate,			hhGibbable::Event_Activate)
	EVENT( EV_PlayIdle,			hhGibbable::Event_PlayIdle)
	EVENT( EV_Respawn,			hhGibbable::Event_Respawn)
END_CLASS

// Called during idEntity::Spawn
void hhGibbable::SetModel( const char *modelname ) {
	// NLATODO - Is this called still?
	hhAnimatedEntity::SetModel( modelname );

	bool bAnimates = spawnArgs.FindKey("anim idle") != NULL;
}

void hhGibbable::Spawn(void) {

	bVertexColorFade = spawnArgs.GetBool("materialFade");
	if (bVertexColorFade) {
		SetDeformation(DEFORMTYPE_VERTEXCOLOR, 1.0f);
	}

	//HUMANHEAD: aob - Flynn wanted some gibbables to be triggered only
	fl.takedamage = !spawnArgs.GetBool("noDamage", "0");

	// setup the clipModel
//	GetPhysics()->SetContents( CONTENTS_SOLID );
	GetPhysics()->SetContents( CONTENTS_BODY | CONTENTS_RENDERMODEL );

	idleAnim = GetAnimator()->GetAnim("idle");
	painAnim = GetAnimator()->GetAnim("pain");
	
	idleChannel = GetChannelForAnim( "idle" );
	painChannel = GetChannelForAnim( "pain" );
	
	PostEventMS(&EV_PlayIdle, 1000);
}

void hhGibbable::Save(idSaveGame *savefile) const {
	savefile->WriteInt( idleAnim );
	savefile->WriteInt( painAnim );
	savefile->WriteInt( idleChannel );
	savefile->WriteInt( painChannel );
	savefile->WriteBool( bVertexColorFade );
}

void hhGibbable::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( idleAnim );
	savefile->ReadInt( painAnim );
	savefile->ReadInt( idleChannel );
	savefile->ReadInt( painChannel );
	savefile->ReadBool( bVertexColorFade );
}

bool hhGibbable::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {

	// Adjust vertex color
	if (bVertexColorFade) {
		float fadeAlpha = idMath::ClampFloat(0.0f, 1.0f, ((float)health / spawnArgs.GetFloat("health")));
		SetDeformation(DEFORMTYPE_VERTEXCOLOR, fadeAlpha);
	}

	if (painAnim) {
		GetAnimator()->PlayAnim( painChannel, painAnim, gameLocal.time, 0);
	}
	
	return( hhAnimatedEntity::Pain(inflictor, attacker, damage, dir, location) );
}

void hhGibbable::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	Explode(attacker);
}

int hhGibbable::DetermineThinnestAxis() {
	int best = 0;
	idBounds bounds = GetPhysics()->GetBounds();
	for( int i = 1; i < 3; ++i ) {
		if ( bounds[1][ i ] - bounds[0][ i ] < bounds[1][ best ] - bounds[0][ best ] ) {
			best = i;
		}
	}

	return best;
}

void hhGibbable::Explode(idEntity *activator) {
	hhFxInfo fxInfo;

	Hide();
	fl.takedamage = false;
	GetPhysics()->SetContents( 0 );
	ActivateTargets( activator );
	SetSkinByName(NULL);
	if ( spawnArgs.GetFloat( "respawn", "0" ) ) {
		PostEventSec( &EV_Respawn, spawnArgs.GetFloat( "respawn", "0" ) );
	} else {
		PostEventMS( &EV_Remove, 200 );	// Remove after a small delay to allow sound commands to execute
	}
	StartSound( "snd_gib", SND_CHANNEL_ANY );

	// Find thinnest axis in the bounds and use for fx normal
	idVec3 thinnest = vec3_origin;
	int axisIndex = DetermineThinnestAxis();
	thinnest[axisIndex] = 1.0f;
	thinnest *= GetAxis();

	fxInfo.RemoveWhenDone( true );
	fxInfo.SetNormal(thinnest);
	// Spawn FX system for gib
	BroadcastFxInfo( spawnArgs.GetString("fx_gib"), GetOrigin(), GetAxis(), &fxInfo );

	// Spawn gibs
	if (spawnArgs.FindKey("def_debrisspawner")) {
		hhUtils::SpawnDebrisMass(spawnArgs.GetString("def_debrisspawner"), this );
	}
}

void hhGibbable::Event_Respawn() {
	GetPhysics()->SetContents( CONTENTS_BODY | CONTENTS_RENDERMODEL );
	fl.takedamage = true;
	Show();
}

void hhGibbable::Event_Activate( idEntity *activator ) {
	Explode(activator);
}

void hhGibbable::Event_PlayIdle( void ) {
	if (idleAnim) {
		GetAnimator()->ClearAllAnims(gameLocal.time, 0);
		GetAnimator()->CycleAnim( idleChannel, idleAnim, gameLocal.time, 0);
	}
}

