
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterNetworkGuardian : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterNetworkGuardian );

	rvMonsterNetworkGuardian ( void );

	void				Spawn							( void );

	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );

protected:

	virtual bool		CheckActions			( void );

	int					shots;
	int					landTime;

	int					flagFlying;

	float				strafeSpeed;

	//this flag allows the AI to control when it takes off and lands.
	bool				flagAutopilot;

	int					battleStage;
	
	enum	{
		FLY_NONE = 0,
		FLY_TRANSITION,
		FLY_FLYING,
	};

private:

	rvAIAction			actionShotgunRocketAttack;
	rvAIAction			actionFlyingRangedAttack;
	rvAIAction			actionFlyingSweepAttack;
	rvAIAction			actionMeleeAttack;
	rvAIAction			actionBlasterSweepGround;
	rvAIAction			actionBlasterAttack;
	rvAIAction			actionMIRVAttack;



	stateResult_t		State_Wait_Flying		( const stateParms_t& parms );

//	stateResult_t		State_Dead				( const stateParms_t& parms );
	
	// walking melee attacks
	
	// walking ranged attacks
	stateResult_t		State_Torso_ShotgunRocket		( const stateParms_t& parms );
	stateResult_t		State_Torso_BlasterSweepGround	( const stateParms_t& parms );
	stateResult_t		State_Torso_BlasterAttack		( const stateParms_t& parms );
	stateResult_t		State_Torso_MIRVAttack			( const stateParms_t& parms );

	//flying evades
	stateResult_t		State_Torso_EvadeLeft			( const stateParms_t& parms );
	stateResult_t		State_Torso_EvadeRight			( const stateParms_t& parms );
	
	//flying ranged attacks
	stateResult_t		State_Torso_FlyingRanged		( const stateParms_t& parms );
	stateResult_t		State_Torso_FlyingSweep			( const stateParms_t& parms );

	// flying anims
	stateResult_t		State_Torso_LiftOff		( const stateParms_t& parms );
	stateResult_t		State_Torso_Fall		( const stateParms_t& parms );
	// walking turn anims
	stateResult_t		State_Torso_TurnRight90			( const stateParms_t& parms );
	stateResult_t		State_Torso_TurnLeft90			( const stateParms_t& parms );


	//force the NG into walking mode. This does not play the landing anim, and will make him fall from the sky.
	void				Event_ForceWalkMode( void );
	
	//These commands change the NG's state, but do so through states and animations.
	void				Event_ForceLanding( void );
	void				Event_ForceTakeoff( void );

	//toggles the NG between AI controlled flight and script controlled flight
	void				Event_AllowAutopilot( float f);

	//for staged combat, sets the int value of the battle stage
	void				Event_SetBattleStage( float f);

	CLASS_STATES_PROTOTYPE ( rvMonsterNetworkGuardian );
};

const idEventDef EV_ForceWalkMode(	"forceWalkMode" );
const idEventDef EV_ForceLanding(	"forceLanding" );
const idEventDef EV_ForceTakeoff(	"forceTakeoff" );
const idEventDef EV_AllowAutopilot(	"allowAutopilot", "f" );
const idEventDef EV_SetBattleStage( "setBattleStage", "f" );


CLASS_DECLARATION( idAI, rvMonsterNetworkGuardian )
	EVENT( EV_ForceWalkMode,			rvMonsterNetworkGuardian::Event_ForceWalkMode )
	EVENT( EV_ForceLanding,				rvMonsterNetworkGuardian::Event_ForceLanding )
	EVENT( EV_ForceTakeoff,				rvMonsterNetworkGuardian::Event_ForceTakeoff )
	EVENT( EV_AllowAutopilot,			rvMonsterNetworkGuardian::Event_AllowAutopilot )
	EVENT( EV_SetBattleStage,			rvMonsterNetworkGuardian::Event_SetBattleStage )

END_CLASS

/*
================
rvMonsterNetworkGuardian::rvMonsterNetworkGuardian
================
*/
rvMonsterNetworkGuardian::rvMonsterNetworkGuardian ( ) {
	shots = 0;
	landTime = 0;
	flagFlying = FLY_NONE;
	battleStage = 1;
	strafeSpeed = 0;

}

/*
================
rvMonsterNetworkGuardian::Spawn
================
*/
void rvMonsterNetworkGuardian::Spawn ( void ) {

	disablePain = true;
	flagAutopilot = false;

	strafeSpeed = spawnArgs.GetFloat( "strafeSpeed", "500" );

	actionShotgunRocketAttack.Init ( spawnArgs,		"action_ShotgunRocket",				"Torso_ShotgunRocket_Attack",			AIACTIONF_ATTACK );
	actionFlyingRangedAttack.Init( spawnArgs,		"action_FlyingRangedAttack",		"Torso_FlyingRanged_Attack",			AIACTIONF_ATTACK );
	actionFlyingSweepAttack.Init( spawnArgs,		"action_blasterSweepAirAttack",		"Torso_FlyingSweep_Attack",				AIACTIONF_ATTACK );
	actionMeleeAttack.Init( spawnArgs,				"action_meleeAttack",				NULL,									AIACTIONF_ATTACK );
	actionBlasterSweepGround.Init( spawnArgs,		"action_blasterSweepGroundAttack",	"Torso_BlasterSweepGround_Attack",		AIACTIONF_ATTACK );
	actionBlasterAttack.Init( spawnArgs,			"action_blasterAttack",				"Torso_Blaster_Attack",					AIACTIONF_ATTACK );
	actionMIRVAttack.Init( spawnArgs,				"action_MIRVAttack",				"Torso_MIRV_Attack",					AIACTIONF_ATTACK );

}


/*
================
rvMonsterNetworkGuardian::Save
================
*/
void rvMonsterNetworkGuardian::Save ( idSaveGame *savefile ) const {
	savefile->WriteInt( shots );
	savefile->WriteInt( landTime );
	savefile->WriteInt( flagFlying );
	savefile->WriteInt( battleStage );
	savefile->WriteBool( flagAutopilot );
	savefile->WriteFloat( strafeSpeed );

	actionShotgunRocketAttack.Save( savefile );
	actionFlyingRangedAttack.Save( savefile );
	actionFlyingSweepAttack.Save( savefile );
	actionMeleeAttack.Save( savefile );
	actionBlasterSweepGround.Save( savefile );
	actionBlasterAttack.Save( savefile );
	actionMIRVAttack.Save( savefile );
}

/*
================
rvMonsterNetworkGuardian::Restore
================
*/
void rvMonsterNetworkGuardian::Restore ( idRestoreGame *savefile ) 
{
	savefile->ReadInt( shots );
	savefile->ReadInt( landTime );
	savefile->ReadInt( flagFlying );
	savefile->ReadInt( battleStage );
	savefile->ReadBool( flagAutopilot );
	savefile->ReadFloat( strafeSpeed );

	actionShotgunRocketAttack.Restore( savefile );
	actionFlyingRangedAttack.Restore( savefile );
	actionFlyingSweepAttack.Restore( savefile );
	actionMeleeAttack.Restore( savefile );
	actionBlasterSweepGround.Restore( savefile );
	actionBlasterAttack.Restore( savefile );
	actionMIRVAttack.Restore( savefile );
}

/*
================
rvMonsterNetworkGuardian::CheckActions
================
*/
bool rvMonsterNetworkGuardian::CheckActions ( void ) {
	// If not moving, try turning in place
	if ( !move.fl.moving && gameLocal.time > combat.investigateTime ) 
	{
		float turnYaw = idMath::AngleNormalize180 ( move.ideal_yaw - move.current_yaw ) ;
		if ( turnYaw > lookMax[YAW] * 0.75f ) 
		{
			PerformAction ( "Torso_TurnRight90", 4, true );
			return true;
		} 
		else if ( turnYaw < -lookMax[YAW] * 0.75f ) 
		{
			PerformAction ( "Torso_TurnLeft90", 4, true );
			return true;
		}
	}

	//if the flight is transitioning, do nothing
	if( flagFlying == FLY_TRANSITION )	{
		return false;
	}

	//this is the autopilot section.
	if( flagAutopilot )	{
		// If he's been on the ground long enough, fly...
		if ( move.moveType == MOVETYPE_ANIM && move.fl.onGround && !flagFlying && gameLocal.time > landTime ) 
		{
			PostState ( "Wait_Flying" );
			SetAnimState ( ANIMCHANNEL_TORSO, "Torso_LiftOff", 4 );
			disablePain = true;
			actionMeleeAttack.fl.disabled = true;
			flagFlying = FLY_TRANSITION;
			return true;
		} 
		else if ( move.moveType == MOVETYPE_FLY && gameLocal.time > landTime ) 
		{
			SetMoveType ( MOVETYPE_ANIM );
			animPrefix = "";
			move.fl.noGravity = false;
			physicsObj.UseFlyMove ( false );
			actionMeleeAttack.fl.disabled = false;
			flagFlying = FLY_TRANSITION;

			SetAnimState ( ANIMCHANNEL_TORSO, "Torso_Fall", 4 );
			PostAnimState ( ANIMCHANNEL_TORSO, "Torso_FinishAction", 0, SFLAG_ONCLEAR );
			PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );
			
			return true;
		}
	}

	//Normal actions here -----------

	//if he's close enough for melee, use melee
	if( flagFlying == FLY_NONE) {
		if(	PerformAction ( &actionMeleeAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack, NULL ) )	{
			return true;
		}
	}

	//check for ranged attacks on the ground
	if( flagFlying == FLY_NONE )	{ 
		  
		 switch (battleStage)	{
		
			case 1:
				if( PerformAction ( &actionShotgunRocketAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )	||		
					PerformAction ( &actionBlasterAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )		||
					PerformAction ( &actionBlasterSweepGround, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
					return true;
				}
			break;
			case 2:
				if( PerformAction ( &actionMIRVAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ||
					PerformAction ( &actionBlasterAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ||
					PerformAction ( &actionBlasterSweepGround, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ||
					PerformAction ( &actionShotgunRocketAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) )	{
					return true;
				}
			break;
			default:
				gameLocal.Error("Bad battleStage '%d' set for Network Guardian.", battleStage);
			break;
		}
	
	}
	//airborne attack actions
	if( (flagFlying == FLY_FLYING) )	{

		if( PerformAction ( &actionEvadeLeft,   (checkAction_t)&idAI::CheckAction_EvadeLeft, &actionTimerEvade )					||
			PerformAction ( &actionEvadeRight,  (checkAction_t)&idAI::CheckAction_EvadeRight, &actionTimerEvade )					||
			PerformAction ( &actionFlyingRangedAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )	||
			PerformAction ( &actionFlyingSweepAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ){
				return true;
		}
	}
	
	//No.
	//return idAI::CheckActions ( );
	return false;
}

/*
===============================================================================

	Events 

===============================================================================
*/

/*
================
rvMonsterNetworkGuardian::Event_ForceWalkMode
================
*/
// forces NG to obey gravity and immediately switch to walking mode.
void rvMonsterNetworkGuardian::Event_ForceWalkMode( void )	{
	
	SetMoveType ( MOVETYPE_ANIM );
	animPrefix = "";
	move.fl.noGravity = false;
	physicsObj.UseFlyMove ( false );
	actionMeleeAttack.fl.disabled = false;
	move.fl.allowDirectional = false;
	flagFlying = FLY_NONE;

}

/*
================
rvMonsterNetworkGuardian::Event_ForceLanding
================
*/
// forces NG play his landing animation. He will not just fall from the sky.
void rvMonsterNetworkGuardian::Event_ForceLanding( void )	{

	SetMoveType ( MOVETYPE_ANIM );
	animPrefix = "";
	move.fl.noGravity = false;
	physicsObj.UseFlyMove ( false );
	actionMeleeAttack.fl.disabled = false;
	move.fl.allowDirectional = false;
	flagFlying = FLY_TRANSITION;

	SetAnimState ( ANIMCHANNEL_TORSO, "Torso_Fall", 4 );
	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_FinishAction", 0, SFLAG_ONCLEAR );
	//PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );

}

/*
================
rvMonsterNetworkGuardian::Event_ForceTakeoff
================
*/
// forces NG to take off and fly. 
void rvMonsterNetworkGuardian::Event_ForceTakeoff( void )	{

	disablePain = true;
	actionMeleeAttack.fl.disabled = true;
	flagFlying = FLY_TRANSITION;
	SetAnimState ( ANIMCHANNEL_TORSO, "Torso_LiftOff", 4 );
	move.fl.allowDirectional = true;
	SetState ( "Wait_Flying" );

}

/*
================
rvMonsterNetworkGuardian::Event_AllowAutoPilot
================
*/
// toggles the AI autoPilot for deciding when to fly and land. 
void rvMonsterNetworkGuardian::Event_AllowAutopilot( float f )	{
	
	flagAutopilot = f ? true : false;

}

/*
================
rvMonsterNetworkGuardian::Event_SetBattleStage
================
*/
// sets the current battle stage. Each stage has different behaviors.
void rvMonsterNetworkGuardian::Event_SetBattleStage( float f )	{
	
	battleStage = f;

}
/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterNetworkGuardian )
	STATE ( "Wait_Flying",		rvMonsterNetworkGuardian::State_Wait_Flying )
	STATE ( "State_Dead",		rvMonsterNetworkGuardian::State_Dead )

	STATE ( "Torso_ShotgunRocket_Attack",			rvMonsterNetworkGuardian::State_Torso_ShotgunRocket )
	STATE ( "Torso_BlasterSweepGround_Attack",		rvMonsterNetworkGuardian::State_Torso_BlasterSweepGround )
	STATE ( "Torso_Blaster_Attack",					rvMonsterNetworkGuardian::State_Torso_BlasterAttack )
	STATE ( "Torso_MIRV_Attack",					rvMonsterNetworkGuardian::State_Torso_MIRVAttack)

	STATE ( "Torso_EvadeLeft",						rvMonsterNetworkGuardian::State_Torso_EvadeLeft )
	STATE ( "Torso_EvadeRight",						rvMonsterNetworkGuardian::State_Torso_EvadeRight )

	STATE ( "Torso_FlyingRanged_Attack",			rvMonsterNetworkGuardian::State_Torso_FlyingRanged )
	STATE ( "Torso_FlyingSweep_Attack",				rvMonsterNetworkGuardian::State_Torso_FlyingSweep )

	STATE ( "Torso_LiftOff",						rvMonsterNetworkGuardian::State_Torso_LiftOff )
	STATE ( "Torso_Fall",							rvMonsterNetworkGuardian::State_Torso_Fall  )

	STATE ( "Torso_TurnRight90",					rvMonsterNetworkGuardian::State_Torso_TurnRight90 )
	STATE ( "Torso_TurnLeft90",						rvMonsterNetworkGuardian::State_Torso_TurnLeft90 )

END_CLASS_STATES


stateResult_t rvMonsterNetworkGuardian::State_Torso_EvadeLeft ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			{
				idVec3 vel = GetPhysics()->GetLinearVelocity();
				vel += viewAxis[1] * strafeSpeed;
				physicsObj.UseVelocityMove( true );
				GetPhysics()->SetLinearVelocity( vel );
				PlayAnim ( ANIMCHANNEL_TORSO, "evade_left", parms.blendFrames );
			}
			return SRESULT_STAGE ( STAGE_WAIT );

		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

stateResult_t rvMonsterNetworkGuardian::State_Torso_EvadeRight ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			{
				idVec3 vel = GetPhysics()->GetLinearVelocity();
				vel += viewAxis[1] * -strafeSpeed;
				physicsObj.UseVelocityMove( true );
				GetPhysics()->SetLinearVelocity( vel );
				PlayAnim ( ANIMCHANNEL_TORSO, "evade_right", parms.blendFrames );
			}
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
rvMonsterNetworkGuardian::State_Torso_ShotgunRocket
================
*/
stateResult_t rvMonsterNetworkGuardian::State_Torso_ShotgunRocket ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "shotgunRocket_attack", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT);
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;	

	}

	return SRESULT_ERROR;

}

/*
================
rvMonsterNetworkGuardian::State_Torso_BlasterSweepGround
================
*/
stateResult_t rvMonsterNetworkGuardian::State_Torso_BlasterSweepGround ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "attack_spray_grd", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT);
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;	

	}

	return SRESULT_ERROR;

}
/*
================
rvMonsterNetworkGuardian::State_Torso_FlyingSweep
================
*/
stateResult_t rvMonsterNetworkGuardian::State_Torso_FlyingSweep ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "attack_spray_air", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT);
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;	

	}

	return SRESULT_ERROR;

}

/*
================
rvMonsterNetworkGuardian::State_Torso_FlyingRanged
================
*/
stateResult_t rvMonsterNetworkGuardian::State_Torso_FlyingRanged ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "flyingRanged_attack", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT);
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;	

	}

	return SRESULT_ERROR;

}

/*
================
rvMonsterNetworkGuardian::State_Torso_MIRVAttack
================
*/
stateResult_t rvMonsterNetworkGuardian::State_Torso_MIRVAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "attack_vert", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT);
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;	

	}

	return SRESULT_ERROR;

}


/*
================
rvMonsterNetworkGuardian::State_Wait_Flying
================
*/
stateResult_t rvMonsterNetworkGuardian::State_Wait_Flying ( const stateParms_t& parms ) {
	if ( move.moveType == MOVETYPE_ANIM ) {
		return SRESULT_WAIT;
	}
	return SRESULT_DONE; 
}

/*
================
rvMonsterNetworkGuardian::State_Dead
================
*/
/*
stateResult_t rvMonsterNetworkGuardian::State_Dead ( const stateParms_t& parms ) {
	return SRESULT_DONE;
}*/

/*
================
rvMonsterNetworkGuardian::State_Torso_LiftOff
================
*/
stateResult_t rvMonsterNetworkGuardian::State_Torso_LiftOff ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			move.fl.noGravity = true;
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "liftoff", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				SetMoveType ( MOVETYPE_FLY );
				landTime = gameLocal.time + DelayTime ( 5000, 10000 );
				animPrefix = "fly";
				SetAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );
				SetAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 4 );
				flagFlying = FLY_FLYING;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;	
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterNetworkGuardian::State_Torso_Fall
================
*/
stateResult_t rvMonsterNetworkGuardian::State_Torso_Fall ( const stateParms_t& parms ) {
	enum {
		STAGE_FALLSTART,
		STAGE_FALLSTARTWAIT,
		STAGE_FALLLOOPWAIT,
		STAGE_FALLENDWAIT
	};
	switch ( parms.stage ) {
		case STAGE_FALLSTART:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "fly_descend_start", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_FALLSTARTWAIT );
			
		case STAGE_FALLSTARTWAIT:
			if ( move.fl.onGround ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "fly_descend_end", 4 );
				return SRESULT_STAGE ( STAGE_FALLENDWAIT );
			}
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "fly_descend_loop", 0 );
				return SRESULT_STAGE ( STAGE_FALLLOOPWAIT );
			}
			return SRESULT_WAIT;
			
		case STAGE_FALLLOOPWAIT:
			if ( move.fl.onGround ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "fly_descend_end", 0 );
				return SRESULT_STAGE ( STAGE_FALLENDWAIT );
			}
			return SRESULT_WAIT;			
					
		case STAGE_FALLENDWAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				// we've landed! determine the next fly time
				landTime = gameLocal.time + DelayTime ( 10000, 15000 );
				flagFlying = FLY_NONE;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}	
	return SRESULT_ERROR;
}

//================
//rvMonsterNetworkGuardian::State_Torso_TurnRight90
//================
stateResult_t rvMonsterNetworkGuardian::State_Torso_TurnRight90 ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "turn_left", parms.blendFrames );
			AnimTurn ( 90.0f, true );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( move.fl.moving || AnimDone ( ANIMCHANNEL_TORSO, 0 )) {
				AnimTurn ( 0, true );
				combat.investigateTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

//================
//rvMonsterNetworkGuardian::State_Torso_TurnLeft90
//================
stateResult_t rvMonsterNetworkGuardian::State_Torso_TurnLeft90 ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "turn_right", parms.blendFrames );
			AnimTurn ( 90.0f, true );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( move.fl.moving || AnimDone ( ANIMCHANNEL_TORSO, 0 )) {
				AnimTurn ( 0, true );
				combat.investigateTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

//================
//rvMonsterNetworkGuardian::State_Torso_BlasterAttack
//================
stateResult_t rvMonsterNetworkGuardian::State_Torso_BlasterAttack ( const stateParms_t& parms ) {	

	enum { 
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			//if flying, do not override legs
			if( (flagFlying != FLY_NONE ) &&  move.fl.moving )	{
				DisableAnimState ( ANIMCHANNEL_LEGS );
			}
			PlayAnim ( ANIMCHANNEL_TORSO, "attack_blaster_start", parms.blendFrames );
			shots = (gameLocal.random.RandomInt ( 12 ) + 8) * combat.aggressiveScale;
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_LOOP:
			PlayAnim ( ANIMCHANNEL_TORSO, "attack_blaster_loop", 0 );
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( --shots <= 0 || (IsEnemyVisible() && !enemy.fl.inFov)  ) {
					PlayAnim ( ANIMCHANNEL_TORSO, "attack_blaster_end", 0 );
					return SRESULT_STAGE ( STAGE_WAITEND );
				}
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_WAITEND:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;				
	}
	return SRESULT_ERROR; 

}
