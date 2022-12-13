#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_GiveHealth( "giveHealth" );

const idEventDef EV_IdleMode( "<idlemode>" );
const idEventDef EV_FinishedPuke( "<finishedpuke>" );
const idEventDef EV_Expended( "<expended>" );


CLASS_DECLARATION( hhAnimatedEntity, hhHealthBasin )
	EVENT( EV_Activate,					hhHealthBasin::Event_Activate )
	EVENT( EV_GiveHealth,				hhHealthBasin::Event_GiveHealth )
	EVENT( EV_Broadcast_AppendFxToList,	hhHealthBasin::Event_AppendFxToIdleList )

	EVENT( EV_IdleMode,					hhHealthBasin::Event_IdleMode )
	EVENT( EV_FinishedPuke,				hhHealthBasin::Event_FinishedPuking )
	EVENT( EV_Expended,					hhHealthBasin::Event_Expended )
END_CLASS


void hhHealthBasin::Event_IdleMode() {
	PlayCycle( "idle" );
	StartSound( "snd_idle", SND_CHANNEL_IDLE );
	BasinMode = BASIN_Idle;
}

void hhHealthBasin::Event_FinishedPuking() {
	if( currentHealth <= 0.f ) {
		Event_Expended();
	}
	else {
		PostEventMS( &EV_IdleMode, PlayAnim("transidle", 4) );
	}
}

void hhHealthBasin::Event_Expended() {
	PlayAnim( "transdeath" );
	StartSound( "snd_die", SND_CHANNEL_ANY );
	hhUtils::RemoveContents( idleFxList, true );		//MDC: This doesn't seem to be working, possibly something with the broadcast fx
	BasinMode = BASIN_Expended;
}
/*
================
hhHealthBasin::hhHealthBasin
================
*/
hhHealthBasin::hhHealthBasin() {
}

/*
================
hhHealthBasin::Spawn
================
*/
void hhHealthBasin::Spawn() {
	if ( g_wicked.GetBool() ) { // CJR:  Don't spawn health basins in wicked mode
		PostEventMS( &EV_Remove, 0 );
	}

	currentHealth = spawnArgs.GetFloat( "maxHealth" );
	activator = NULL;
	fl.takedamage = true;

	verificationAbsBounds.Clear();
	GetPhysics()->SetContents( CONTENTS_BODY );

	SpawnTrigger();
	SpawnFx();

	PostEventMS( &EV_IdleMode, 0 );
}

void hhHealthBasin::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( BasinMode );
	activator.Save( savefile );
	savefile->WriteFloat( currentHealth );
	savefile->WriteBounds( verificationAbsBounds );

	int num = idleFxList.Num();
	savefile->WriteInt( num );
	for( int i = 0; i < num; i++ ) {
		idleFxList[i].Save( savefile );
	}
}

void hhHealthBasin::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( reinterpret_cast<int &> ( BasinMode ) );
	activator.Restore( savefile );
	savefile->ReadFloat( currentHealth );
	savefile->ReadBounds( verificationAbsBounds );

	int num;
	savefile->ReadInt( num );
	idleFxList.Clear();
	idleFxList.SetNum( num );
	for( int i = 0; i < num; i++ ) {
		idleFxList[i].Restore( savefile );
	}
}

/*
================
hhHealthBasin::~hhHealthBasin
================
*/
hhHealthBasin::~hhHealthBasin() {
	hhUtils::RemoveContents( idleFxList, true );
}

/*
===============
hhHealthBasin::SpawnTrigger
===============
*/
void hhHealthBasin::SpawnTrigger() {
	idDict args;

	args.Set( "target", name.c_str() );
	args.Set( "mins", spawnArgs.GetString("triggerMins") );
	args.Set( "maxs", spawnArgs.GetString("triggerMaxs") );
	args.SetVector( "origin", GetOrigin() );
	args.SetMatrix( "rotation", GetAxis() );
	gameLocal.SpawnObject( spawnArgs.GetString("def_trigger"), &args );

	verificationAbsBounds.FromTransformedBounds( idBounds(spawnArgs.GetVector("triggerMins"), spawnArgs.GetVector("triggerMaxs") ) + idBounds( vec3_zero, idVec3(75.0f, 0.0f, 0.0f)), GetOrigin(), GetAxis() );
}

/*
===============
hhHealthBasin::SpawnFx
===============
*/
void hhHealthBasin::SpawnFx() {
	BroadcastFxInfoAlongBonePrefixUnique( &spawnArgs, "fx_idle", "joint_idleFx", NULL, &EV_Broadcast_AppendFxToList );
}

/*
===============
hhHealthBasin::PlayAnim
===============
*/
int	hhHealthBasin::PlayAnim( const char* pName, int iBlendTime ) {
	int pAnim = 0;

	ClearAnims( iBlendTime );

	if( !pName && !pName[0] ) {
		return 0;
	}

	pAnim = GetAnimator()->GetAnim( pName );
	GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, pAnim, gameLocal.time, FRAME2MS( iBlendTime ) );

	return (pAnim != NULL) ? GetAnimator()->GetAnim( pAnim )->Length() : 0;
}

/*
===============
hhHealthBasin::PlayCycle
===============
*/
void hhHealthBasin::PlayCycle( const char* pName, int iBlendTime ) {
	ClearAnims( iBlendTime );

	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, GetAnimator()->GetAnim(pName), gameLocal.time, FRAME2MS( iBlendTime ) );
}

/*
===============
hhHealthBasin::ClearAnims
===============
*/
void hhHealthBasin::ClearAnims( int iBlendTime ) {
	GetAnimator()->ClearAllAnims( gameLocal.time, FRAME2MS( iBlendTime ) );
}

/*
===============
hhHealthBasin::ActivatorVerified
===============
*/
bool hhHealthBasin::ActivatorVerified( const idEntityPtr<idActor>& Activator ) {
	if( !Activator.IsValid() || !Activator->GetPhysics()->GetAbsBounds().IntersectsBounds(verificationAbsBounds) ) {
		return false;
	}
	//mdc - also check the health to be a valid activator
	if( Activator->health < Activator->GetMaxHealth() ) {
		return true;
	}
	return false;
}

/*
===============
hhHealthBasin::Event_AppendFxToIdleList
===============
*/
void hhHealthBasin::Event_AppendFxToIdleList( hhEntityFx* fx ) {
	idleFxList.Append( fx );
}

/*
===============
hhHealthBasin::Event_Activate
===============
*/
void hhHealthBasin::Event_Activate( idEntity *activatedby ) {
	if( BasinMode == BASIN_Idle ) {	//only allow activation when idle
		activator = static_cast<idActor*>(activatedby);
		if( ActivatorVerified(activator) ) {	//mdc: only begin puking if valid activator (including less than nominal health)
			BasinMode = BASIN_Puking;
			StopSound( SND_CHANNEL_IDLE );
			StartSound( "snd_puke", SND_CHANNEL_ANY );
			PostEventMS( &EV_FinishedPuke, PlayAnim("puke") );
		}
	}
}

/*
===============
hhHealthBasin::Event_GiveHealth

Called from frame command
===============
*/
void hhHealthBasin::Event_GiveHealth() {
	float		amountApplied = 0.0f;

	assert( currentHealth > 0.0f );

	//Make sure activator is still in the trigger volume, and that they still need health..
	if( !ActivatorVerified(activator) ) {
		return;
	}

	int oldHealth = activator->health;
	if( activator->Give("health", va("%.2f", currentHealth)) ) {
		amountApplied = activator->health - oldHealth;
		currentHealth -= amountApplied;

		ActivateTargets( activator.GetEntity() );
	}
}