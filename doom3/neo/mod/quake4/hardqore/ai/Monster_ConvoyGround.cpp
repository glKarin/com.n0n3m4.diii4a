
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../vehicle/Vehicle.h"

//
class rvMonsterConvoyGround : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterConvoyGround );

	rvMonsterConvoyGround ( void );

	void					Spawn						( void );
	void					InitSpawnArgsVariables		( void );
	void					Save						( idSaveGame *savefile ) const;
	void					Restore						( idRestoreGame *savefile );
	
	virtual void			Postthink					( void );
	virtual bool			Pain						( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual bool			CheckPainActions			( void );
	
	virtual bool			CanTurn						( void ) const;
	virtual bool			CanMove						( void ) const;
	virtual	void			Damage						( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			AdjustHealthByDamage		( int inDamage );

	virtual void			GetDebugInfo				( debugInfoProc_t proc, void* userData );

protected:

	int						shots;
	int						minShots;
	int						maxShots;
	bool					isOpen;
	bool					vehicleCollision;
	float					moveCurrentAnimRate;
	float					moveAnimRateMin;
	float					moveAnimRateRange;
	float					moveAccelRate;
	bool					onGround;
	idVec3					oldOrigin;
	idVec3					lastPainDir;

	rvAIAction				actionBlasterAttack;
	
	virtual bool			CheckActions				( void );
	virtual int				FilterTactical				( int availableTactical );

	virtual const char*		GetIdleAnimName				( void );

	virtual void			OnDeath						( void );

private:

	// General states
	stateResult_t			State_Fall					( const stateParms_t& parms );

	// Legs States
	stateResult_t			State_Legs_Move				( const stateParms_t& parms );

	// Torso States
	stateResult_t			State_Torso_BlasterAttack	( const stateParms_t& parms );
	stateResult_t			State_Torso_Open			( const stateParms_t& parms );
	stateResult_t			State_Torso_Close			( const stateParms_t& parms );
	stateResult_t			State_Torso_Pain			( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterConvoyGround );
};

CLASS_DECLARATION( idAI, rvMonsterConvoyGround )
END_CLASS

/*
================
rvMonsterConvoyGround::rvMonsterConvoyGround
================
*/
rvMonsterConvoyGround::rvMonsterConvoyGround ( ) {
	shots				= 0;
	isOpen				= false;		
	vehicleCollision	= false;
	moveCurrentAnimRate	= 1.0f;
}

void rvMonsterConvoyGround::InitSpawnArgsVariables ( void )
{
	minShots			= spawnArgs.GetInt ( "minShots" );
	maxShots			= spawnArgs.GetInt ( "maxShots" );
	moveAccelRate		= spawnArgs.GetFloat ( "moveAccelRate", ".1" );
	moveAnimRateMin		= spawnArgs.GetFloat ( "moveMinAnimRate", "1" );
	moveAnimRateRange	= spawnArgs.GetFloat ( "moveMaxAnimRate", "10" ) - moveAnimRateMin;
}

/*
================
rvMonsterConvoyGround::Spawn
================
*/
void rvMonsterConvoyGround::Spawn ( void ) {
	actionBlasterAttack.Init ( spawnArgs, "action_blasterAttack", "Torso_BlasterAttack", AIACTIONF_ATTACK );		

	InitSpawnArgsVariables();
	
	aifl.disableLook = true; 
	
	onGround = true;

}

/*
================
rvMonsterConvoyGround::Prethink
================
*/
void rvMonsterConvoyGround::Postthink ( void ) {
/* FIXME
	if ( onGround && !physicsObj.HasGroundContacts ( ) ) {
		onGround = false;
		InterruptState ( "State_Fall" );
	}
*/
	
	idAI::Postthink ( );
}	

/*
================
rvMonsterConvoyGround::Save
================
*/
bool rvMonsterConvoyGround::Pain ( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	lastPainDir = dir;	
	return idAI::Pain ( inflictor, attacker, damage, dir, location );
}

/*
================
rvMonsterConvoyGround::Save
================
*/
void rvMonsterConvoyGround::Save ( idSaveGame *savefile ) const {
	savefile->WriteInt ( shots );
	//minShots and maxShots are set in the Restore
	savefile->WriteBool ( isOpen );
	savefile->WriteBool ( vehicleCollision );
	savefile->WriteBool ( onGround );

	savefile->WriteFloat ( moveCurrentAnimRate );
	savefile->WriteVec3	( oldOrigin );
	savefile->WriteVec3 ( lastPainDir );

	actionBlasterAttack.Save ( savefile );
}

/*
================
rvMonsterConvoyGround::Restore
================
*/
void rvMonsterConvoyGround::Restore ( idRestoreGame *savefile ) {
	savefile->ReadInt ( shots );
	savefile->ReadBool ( isOpen );
	savefile->ReadBool ( vehicleCollision );
	savefile->ReadBool ( onGround );

	savefile->ReadFloat ( moveCurrentAnimRate );
	savefile->ReadVec3 ( oldOrigin );
	savefile->ReadVec3 ( lastPainDir );

	actionBlasterAttack.Restore ( savefile );

	InitSpawnArgsVariables();
}

/*
================
rvMonsterConvoyGround::CanMove
================
*/
bool rvMonsterConvoyGround::CanMove ( void ) const {
	if ( isOpen ) {
		return false;
	}
	return idAI::CanMove ( );
}

/*
================
rvMonsterConvoyGround::CanTurn
================
*/
bool rvMonsterConvoyGround::CanTurn ( void ) const {
	if ( isOpen ) {
		return false;
	}
	return idAI::CanTurn ( );
}

/*
================
rvMonsterConvoyGround::OnDeath
================
*/
void rvMonsterConvoyGround::OnDeath	( void ) {
	idVec3 fxOrg;
	idVec3 up;
	idMat3 fxAxis;

	//center it
	fxOrg = GetPhysics()->GetCenterMass();

	//point it up
	up.Set( 0, 0, 1 );
	fxAxis = up.ToMat3();

	//if we can play it at the joint, do that
	jointHandle_t axisJoint = animator.GetJointHandle ( "axis" );
	if ( axisJoint != INVALID_JOINT ) {
		idMat3 junk;
		animator.GetJointLocalTransform( axisJoint, gameLocal.GetTime(), fxOrg, junk );
		fxOrg = renderEntity.origin + (fxOrg*renderEntity.axis);
	}

	gameLocal.PlayEffect ( spawnArgs, "fx_death", fxOrg, fxAxis );
	idAI::OnDeath ( );
}

/*
================
rvMonsterConvoyGround::AdjustHealthByDamage
================
*/
void rvMonsterConvoyGround::AdjustHealthByDamage ( int damage ) {
	if ( isOpen || vehicleCollision ) {
		idAI::AdjustHealthByDamage ( damage );
	} else { 
		PlayEffect ( "fx_shieldHit", animator.GetJointHandle ( "axis" ) );
	}
}

void rvMonsterConvoyGround::Damage ( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location )
{
	vehicleCollision = false;
	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, false );
	if ( damageDef && damageDef->GetBool( "vehicle_collision" ) ) {
		vehicleCollision = true;
	}

	idAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}
/*
================
rvMonsterConvoyGround::Spawn
================
*/
bool rvMonsterConvoyGround::CheckActions ( void ) {
	if ( isOpen ) {
		if ( move.fl.moving ) {
/*		
			|| !CheckAction_RangedAttack( &actionBlasterAttack, -1 ) 
			|| enemy.range > actionBlasterAttack.maxRange
			|| enemy.range < actionBlasterAttack.minRange 
			|| (!move.fl.moving && (gameLocal.GetTime()-move.startTime) > 3000 ) ) {
*/
			StartSound( "snd_prepare", SND_CHANNEL_ANY, 0, 0, 0  );
			PerformAction ( "Torso_Close", 4, true );
			return true;
		}

		if ( PerformAction ( &actionBlasterAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
			return true;
		}
	} else {
		// Open up if we have stopped and have an enemy
		if ( !move.fl.moving && physicsObj.HasGroundContacts ( ) && enemy.ent && legsAnim.IsIdle ( ) && CheckTactical ( AITACTICAL_RANGED ) ) {
			StartSound( "snd_prepare", SND_CHANNEL_ANY, 0, 0, 0  );
			PerformAction ( "Torso_Open", 4, true );
			return true;
		}
	}

	return idAI::CheckActions ( );
}

/*
================
rvMonsterConvoyGround::CheckPainActions
================
*/
bool rvMonsterConvoyGround::CheckPainActions ( void ) {
	if ( isOpen ) {
		return false;
	}
	
	return idAI::CheckPainActions ( );
}

/*
================
rvMonsterConvoyGround::GetIdleAnimName
================
*/
const char* rvMonsterConvoyGround::GetIdleAnimName ( void ) {
	// Start idle animation
	if ( isOpen ) { 
		return "idle_open";
	}	
	return "idle";
}	

/*
================
rvMonsterConvoyGround::FilterTactical
================
*/
int rvMonsterConvoyGround::FilterTactical ( int availableTactical ) {
	return idAI::FilterTactical ( availableTactical );
}

/*
=====================
rvMonsterConvoyGround::GetDebugInfo
=====================
*/
void rvMonsterConvoyGround::GetDebugInfo ( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );

	proc ( "idAI", "action_blasterAttack",	aiActionStatusString[actionBlasterAttack.status], userData );
	
	proc ( "rvMonsterConvoyGround", "moveAnimRate",	va("%g", moveCurrentAnimRate ), userData );
	proc ( "rvMonsterConvoyGround", "isOpen",		isOpen ? "true" : "false", userData );		
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterConvoyGround )
	STATE ( "State_Fall",			rvMonsterConvoyGround::State_Fall )
	STATE ( "Torso_Open",			rvMonsterConvoyGround::State_Torso_Open )
	STATE ( "Torso_Close",			rvMonsterConvoyGround::State_Torso_Close )
	STATE ( "Torso_BlasterAttack",	rvMonsterConvoyGround::State_Torso_BlasterAttack )
	STATE ( "Torso_Pain",			rvMonsterConvoyGround::State_Torso_Pain )

	STATE ( "Legs_Move",			rvMonsterConvoyGround::State_Legs_Move )
END_CLASS_STATES

/*
================
rvMonsterConvoyGround::State_Fall
================
*/
stateResult_t rvMonsterConvoyGround::State_Fall ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,			// Initialize fall stage
		STAGE_WAITIMPACT,	// Wait for the drop turret to hit the ground
		STAGE_IMPACT,		// Handle drop turret impact, switch to combat state
		STAGE_WAITDONE,
		STAGE_DONE
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			StopMove ( MOVE_STATUS_DONE );
			StopAnimState ( ANIMCHANNEL_LEGS );
			StopAnimState ( ANIMCHANNEL_TORSO );
			StartSound ( "snd_falling", SND_CHANNEL_VOICE, 0, false, NULL );
			PlayEffect ( "fx_droptrail", animator.GetJointHandle ( "origin" ), true );
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayCycle ( ANIMCHANNEL_TORSO, "idle", 0 );
			oldOrigin = physicsObj.GetOrigin ( );
			return SRESULT_STAGE(STAGE_WAITIMPACT);
			
		case STAGE_WAITIMPACT:
			if ( physicsObj.HasGroundContacts ( ) ) {
				return SRESULT_STAGE(STAGE_IMPACT);
			}
			return SRESULT_WAIT;
			
		case STAGE_IMPACT:
			StopSound ( SND_CHANNEL_VOICE, false );
			StopEffect ( "fx_droptrail" );
			PlayEffect ( "fx_landing", GetPhysics()->GetOrigin(), (-GetPhysics()->GetGravityNormal()).ToMat3() );
			
			if ( (physicsObj.GetOrigin ( ) - oldOrigin).LengthSqr() > Square(128.0f) ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "land", 0 );
				return SRESULT_STAGE ( STAGE_WAITDONE );
			}
			return SRESULT_STAGE ( STAGE_DONE );
			
		case STAGE_WAITDONE:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_STAGE ( STAGE_DONE );
			}
			return SRESULT_WAIT;
		
		case STAGE_DONE:
			onGround = true;
			SetAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle" );
			SetAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle" );
			return SRESULT_DONE;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterConvoyGround::State_Torso_BlasterAttack
================
*/
stateResult_t rvMonsterConvoyGround::State_Torso_BlasterAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_ATTACK,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			shots = (gameLocal.random.RandomInt ( maxShots - minShots ) + minShots) * combat.aggressiveScale;
			return SRESULT_STAGE ( STAGE_ATTACK );

		case STAGE_ATTACK:			
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack", parms.blendFrames );
			shots--;
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( --shots <= 0 || (IsEnemyVisible() && !enemy.fl.inFov)  ) {
					return SRESULT_DONE;
				}
				return SRESULT_STAGE ( STAGE_ATTACK );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterConvoyGround::State_Torso_Open
================
*/
stateResult_t rvMonsterConvoyGround::State_Torso_Open ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAIT,
	};

	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "extend_legs", parms.blendFrames );
			isOpen = true;
			aifl.disableLook = false;
			return SRESULT_STAGE ( STAGE_WAIT );

		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE; 
			}
			return SRESULT_WAIT;
	}			
	return SRESULT_ERROR; 
}

/*
================
rvMonsterConvoyGround::State_Torso_Close
================
*/
stateResult_t rvMonsterConvoyGround::State_Torso_Close ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAIT,
	};

	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "retract_legs", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );

		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				isOpen = false;
				aifl.disableLook = true;
				ForceTacticalUpdate();
				return SRESULT_DONE; 
			}
			return SRESULT_WAIT;
			
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterConvoyGround::State_Torso_Pain
================
*/
stateResult_t rvMonsterConvoyGround::State_Torso_Pain ( const stateParms_t& parms ) {
	enum {
		STAGE_START,
		STAGE_END
	};
	
	switch ( parms.stage ) {
		case STAGE_START:
			// Force the orientation to the direction we got hit from so the animation looks correct		
			OverrideFlag ( AIFLAGOVERRIDE_NOTURN, true );
			TurnToward ( physicsObj.GetOrigin() - lastPainDir * 128.0f );
			move.current_yaw = move.ideal_yaw;
			
			// Just in case the pain anim wasnt set before we got here.
			if ( !painAnim.Length ( ) ) {
				painAnim = "pain";
			}
			
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, painAnim, parms.blendFrames );
			return SRESULT_STAGE ( STAGE_END );
	
		case STAGE_END:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterConvoyGround::State_Legs_Move
================
*/
stateResult_t rvMonsterConvoyGround::State_Legs_Move ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_MOVE
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			move.fl.allowAnimMove		= true;
			move.fl.allowPrevAnimMove	= false;
			move.fl.running				= true;
			move.currentDirection		= MOVEDIR_FORWARD;
			// TODO: Looks like current anim rate never gets reset, so they do not correctly accelerate from a stop
			//	unfortunately, adding this change (with a decent acceleration factor) caused them to do lots of
			//	not-so-good looking short moves.
//			moveCurrentAnimRate = 0;

			oldOrigin = physicsObj.GetOrigin ( );
			PlayCycle ( ANIMCHANNEL_LEGS, "run", 0 );
			StartSound( "snd_move", SND_CHANNEL_BODY3, 0, false, NULL );

			return SRESULT_STAGE ( STAGE_MOVE );
			
		case STAGE_MOVE:

			// If not moving forward just go back to idle
			if ( !move.fl.moving || !CanMove() ) {
				StopSound( SND_CHANNEL_BODY3, 0 );
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 0 );
				return SRESULT_DONE;
			}

			// If on the ground update the animation rate based on the normal of the ground plane
			if ( !ai_debugHelpers.GetBool ( ) && physicsObj.HasGroundContacts ( ) ) {
				float	rate;
				idVec3	dir;
								
				dir = (physicsObj.GetOrigin ( ) - oldOrigin);
				
				if ( DistanceTo ( move.moveDest ) < move.walkRange ) {
					rate = moveAnimRateMin;
				} else if ( dir.Normalize ( ) > 0.0f ) {
					rate = idMath::ClampFloat ( -0.7f, 0.7f, physicsObj.GetGravityNormal ( ) * dir ) / 0.7f;
					rate = moveAnimRateMin + moveAnimRateRange * (1.0f + rate) / 2.0f;
				} else {
					rate = moveAnimRateMin + moveAnimRateRange * 0.5f;
				}
				moveCurrentAnimRate += ((rate - moveCurrentAnimRate) * moveAccelRate);
				
				animator.CurrentAnim ( ANIMCHANNEL_LEGS )->SetPlaybackRate ( gameLocal.time, moveCurrentAnimRate );
			}

			oldOrigin = physicsObj.GetOrigin ( );
			
			return SRESULT_WAIT;
	}
		
	return SRESULT_ERROR;
}

