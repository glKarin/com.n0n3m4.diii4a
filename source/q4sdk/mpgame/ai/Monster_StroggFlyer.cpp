
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

extern const char* aiActionStatusString [ rvAIAction::STATUS_MAX ];

class rvMonsterStroggFlyer : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterStroggFlyer );

	rvMonsterStroggFlyer ( void );

	void				InitSpawnArgsVariables( void );
	void				Spawn				( void );
	void				Save				( idSaveGame *savefile ) const;
	void				Restore				( idRestoreGame *savefile );

	virtual void		GetDebugInfo		( debugInfoProc_t proc, void* userData );

protected:

	virtual bool		CheckActions		( void );

	virtual void		OnUpdatePlayback	( const rvDeclPlaybackData& pbd );
	virtual void		OnWakeUp			( void );

	rvAIAction			actionBombAttack;
	rvAIAction			actionBlasterAttack;
	
	idVec3				velocity;

	int					shotCount;
	jointHandle_t		jointGunRight;
	jointHandle_t		jointGunLeft;

	int					lastAttackTime;
	int					attackStartTime;

	int					blasterAttackDuration;
	int					blasterAttackRate;
	int					bombAttackDuration;
	int					bombAttackRate;
	
private:

	stateResult_t		State_ScriptedPlaybackMove	( const stateParms_t& parms );
	stateResult_t		State_Killed				( const stateParms_t& parms );

	stateResult_t		State_Torso_BlasterAttack	( const stateParms_t& parms );
	stateResult_t		State_Torso_BombAttack		( const stateParms_t& parms );

	void				AttackBlaster				( void );
	void				AttackBomb					( void );

	CLASS_STATES_PROTOTYPE ( rvMonsterStroggFlyer );
};

CLASS_DECLARATION( idAI, rvMonsterStroggFlyer )
END_CLASS

/*
================
rvMonsterStroggFlyer::rvMonsterStroggFlyer
================
*/
rvMonsterStroggFlyer::rvMonsterStroggFlyer ( ) {
	shotCount = 0;
	lastAttackTime = 0;
	attackStartTime = 0;
}

void rvMonsterStroggFlyer::InitSpawnArgsVariables( void )
{
	jointGunRight	= animator.GetJointHandle ( spawnArgs.GetString ( "joint_gun_right" ) );
	jointGunLeft	= animator.GetJointHandle ( spawnArgs.GetString ( "joint_gun_left" ) );
	
	blasterAttackDuration	= SEC2MS ( spawnArgs.GetFloat ( "blasterAttackDuration", "1" ) );
	blasterAttackRate		= SEC2MS ( spawnArgs.GetFloat ( "blasterAttackRate", ".25" ) );
	bombAttackDuration		= SEC2MS ( spawnArgs.GetFloat ( "bombAttackDuration", "1" ) );
	bombAttackRate			= SEC2MS ( spawnArgs.GetFloat ( "bombAttackRate", ".25" ) );
}

/*
================
rvMonsterStroggFlyer::Spawn
================
*/
void rvMonsterStroggFlyer::Spawn ( void ) {
	actionBombAttack.Init	 ( spawnArgs, "action_bombAttack",		"Torso_BombAttack",		AIACTIONF_ATTACK );
	actionBlasterAttack.Init ( spawnArgs, "action_blasterAttack",	"Torso_BlasterAttack",	AIACTIONF_ATTACK );
	
	InitSpawnArgsVariables();
}

/*
================
rvMonsterStroggFlyer::OnUpdatePlayback
================
*/
void rvMonsterStroggFlyer::OnUpdatePlayback ( const rvDeclPlaybackData& pbd ) {
	byte	buttons;
	byte	changed;
	byte	impulse;

	velocity = pbd.GetVelocity ( );

	buttons = pbd.GetButtons();
	changed = pbd.GetChanged();
	impulse = pbd.GetImpulse();

	// Shoot the blaster if the attack button was pressed
	if ( (changed & buttons) & BUTTON_ATTACK ){
		AttackBlaster ( );
	}

	if ( (changed & buttons) & BUTTON_ZOOM ){
		AttackBomb ( );
	}

	switch ( impulse ) {
		case 40:
			aifl.disableAttacks = true;
			break;
		
		case 41:
			aifl.disableAttacks = false;
			break;

		case 42:
			StartSound ( "snd_bombrun", SND_CHANNEL_ANY, 0, false, NULL );
			break;
	}
}

/*
================
rvMonsterStroggFlyer::OnWakeUp
================
*/
void rvMonsterStroggFlyer::OnWakeUp ( void ) {
	jointHandle_t joint;
	joint = GetAnimator()->GetJointHandle( spawnArgs.GetString ( "joint_thruster", "tail_thrusters" ) );
	if ( joint != INVALID_JOINT ) {
		PlayEffect ( "fx_exhaust", joint, true );
	}
	StartSound ( "snd_flyloop", SND_CHANNEL_ANY, 0, false, NULL );
	
	return idAI::OnWakeUp ( );
}


/*
================
rvMonsterStroggFlyer::AttackBlaster
================
*/
void rvMonsterStroggFlyer::AttackBlaster ( void ) {
	jointHandle_t joint;	
	joint = ((shotCount++)%2) ? jointGunRight : jointGunLeft;
	
	if ( joint != INVALID_JOINT ) {
		PlayEffect ( "fx_muzzleflash", joint );
		Attack ( "blaster", joint, enemy.ent );
	}
}

/*
================
rvMonsterStroggFlyer::AttackBomb
================
*/
void rvMonsterStroggFlyer::AttackBomb ( void ) {
	jointHandle_t joint;	
	joint = ((shotCount++)%2) ? jointGunRight : jointGunLeft;
	
	if ( joint != INVALID_JOINT ) {
		StartSound ( "snd_bombrun", SND_CHANNEL_ANY, 0, false, NULL );
		PlayEffect ( "fx_bombflash", joint );
		Attack ( "bomb", joint, enemy.ent );
	}
}

/*
================
rvMonsterStroggFlyer::CheckActions
================
*/
bool rvMonsterStroggFlyer::CheckActions ( void ) {
	if ( PerformAction ( &actionBombAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )    ||
	     PerformAction ( &actionBlasterAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )    ) {
		return true;
	}
	return idAI::CheckActions ( );
}

/*
================
rvMonsterStroggFlyer::Save
================
*/
void rvMonsterStroggFlyer::Save( idSaveGame *savefile ) const {
	actionBombAttack.Save ( savefile ) ;	
	actionBlasterAttack.Save ( savefile );
	
	savefile->WriteVec3 ( velocity );

	savefile->WriteInt ( shotCount );

	savefile->WriteInt ( lastAttackTime );
	savefile->WriteInt ( attackStartTime );
}

/*
================
rvMonsterStroggFlyer::Restore
================
*/
void rvMonsterStroggFlyer::Restore( idRestoreGame *savefile ) {
	actionBombAttack.Restore ( savefile ) ;	
	actionBlasterAttack.Restore ( savefile );
	
	savefile->ReadVec3 ( velocity );

	savefile->ReadInt ( shotCount );

	savefile->ReadInt ( lastAttackTime );
	savefile->ReadInt ( attackStartTime );

	InitSpawnArgsVariables();
}

/*
================
rvMonsterStroggFlyer::GetDebugInfo
================
*/
void rvMonsterStroggFlyer::GetDebugInfo	( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );
	
	proc ( "idAI", "action_blasterAttack",	aiActionStatusString[actionBlasterAttack.status], userData );
	proc ( "idAI", "action_bombAttack",		aiActionStatusString[actionBombAttack.status], userData );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterStroggFlyer )
	STATE ( "State_WakeUp",					rvMonsterStroggFlyer::State_WakeUp )
	STATE ( "State_ScriptedPlaybackMove",	rvMonsterStroggFlyer::State_ScriptedPlaybackMove )
	STATE ( "State_Killed",					rvMonsterStroggFlyer::State_Killed )

	STATE ( "Torso_BlasterAttack",			rvMonsterStroggFlyer::State_Torso_BlasterAttack )
	STATE ( "Torso_BombAttack",				rvMonsterStroggFlyer::State_Torso_BombAttack )
END_CLASS_STATES

/*
================
rvMonsterStroggFlyer::State_ScriptedPlaybackMove
================
*/
stateResult_t rvMonsterStroggFlyer::State_ScriptedPlaybackMove ( const stateParms_t& parms ) {
	// When the playback finishes cancel any running states
	if ( !mPlayback.IsActive() ) {
 		StopAnimState ( ANIMCHANNEL_TORSO );
 		StopAnimState ( ANIMCHANNEL_LEGS );
		return SRESULT_DONE;
	}
	
	// Keep the enemy status up to date
	if ( !enemy.ent ) {
		CheckForEnemy ( true );
	}
	
	// Perform actions
	UpdateAction ( );
	
	return SRESULT_WAIT;
}

/*
================
rvMonsterStroggFlyer::State_Killed
================
*/
stateResult_t rvMonsterStroggFlyer::State_Killed ( const stateParms_t& parms ) {
	PlayEffect ( "fx_death", GetPhysics()->GetOrigin(), (-GetPhysics()->GetGravityNormal()).ToMat3() );
	return idAI::State_Killed ( parms );
}

/*
================
rvMonsterStroggFlyer::State_Torso_BlasterAttack
================
*/
stateResult_t rvMonsterStroggFlyer::State_Torso_BlasterAttack ( const stateParms_t& parms ) {	
	enum {
		STAGE_INIT,
		STAGE_BLASTER,
		STAGE_BLASTERWAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			attackStartTime = gameLocal.time;
			return SRESULT_STAGE ( STAGE_BLASTER );
		
		case STAGE_BLASTER:
			lastAttackTime = gameLocal.time;
			AttackBlaster ( );
			return SRESULT_STAGE ( STAGE_BLASTERWAIT );
		
		case STAGE_BLASTERWAIT:
			if ( !enemy.fl.inFov || gameLocal.time - attackStartTime > blasterAttackDuration ) {
				return SRESULT_DONE;
			}
			if ( gameLocal.time - lastAttackTime > blasterAttackRate ) {
				return SRESULT_STAGE ( STAGE_BLASTER );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterStroggFlyer::State_Torso_BombAttack
================
*/
stateResult_t rvMonsterStroggFlyer::State_Torso_BombAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_BOMB,
		STAGE_BOMBWAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			attackStartTime = gameLocal.time;
			return SRESULT_STAGE ( STAGE_BOMB );
		
		case STAGE_BOMB:
			lastAttackTime = gameLocal.time;
			AttackBomb ( );
			return SRESULT_STAGE ( STAGE_BOMBWAIT );
		
		case STAGE_BOMBWAIT:
			if ( !enemy.fl.inFov || gameLocal.time - attackStartTime > bombAttackDuration ) {
				return SRESULT_DONE;
			}
			if ( gameLocal.time - lastAttackTime > bombAttackRate ) {
				return SRESULT_STAGE ( STAGE_BOMB );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
