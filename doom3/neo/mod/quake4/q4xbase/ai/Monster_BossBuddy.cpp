
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

//------------------------------------------------------------
class rvMonsterBossBuddy : public idAI 
//------------------------------------------------------------
{
public:

	CLASS_PROTOTYPE( rvMonsterBossBuddy );

	rvMonsterBossBuddy( void );

	void		Spawn							( void );
	void		InitSpawnArgsVariables			( void );
	void		Save							( idSaveGame *savefile ) const;
	void		Restore							( idRestoreGame *savefile );

	bool		CanTurn							( void ) const;

	void		Think							( void );
	bool		Pain							( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void		Damage							( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	void		OnWakeUp						( void );

	// Add some dynamic externals for debugging
	void		GetDebugInfo					( debugInfoProc_t proc, void* userData );

protected:

	bool		CheckActions					( void );
//	void		PerformAction					( const char* stateName, int blendFrames, bool noPain );
//	bool		PerformAction					( rvAIAction* action, bool (idAI::*condition)(rvAIAction*,int), rvAIActionTimer* timer );

	void		AdjustShieldState				( bool becomeShielded );
	void		ReduceShields					( int amount );

	int			mShots;
	int			mShields;
	int			mMaxShields;
	int			mLastDamageTime;
	int			mShieldsLastFor;		// read from def file, shouldn't need to be saved.

	bool		mIsShielded;
	bool		mRequestedZoneMove;
	bool		mRequestedRecharge;

	//bool		mCanIdle;
	//bool		mChaseMode;

private:

	rvAIAction	mActionRocketAttack;
	rvAIAction	mActionSlashMoveAttack;
	rvAIAction	mActionLightningAttack;
	rvAIAction	mActionDarkMatterAttack;
	rvAIAction	mActionMeleeMoveAttack;
	rvAIAction	mActionMeleeAttack;

	rvScriptFuncUtility	mRequestRecharge;	// script to run when a projectile is launched
	rvScriptFuncUtility	mRequestZoneMove;	// script to run when he should move to the next zone

	stateResult_t		State_Torso_RocketAttack		( const stateParms_t& parms );
	stateResult_t		State_Torso_SlashAttack			( const stateParms_t& parms );
	stateResult_t		State_Torso_TurnRight90			( const stateParms_t& parms );
	stateResult_t		State_Torso_TurnLeft90			( const stateParms_t& parms );

	void				Event_RechargeShields( float amount );

	CLASS_STATES_PROTOTYPE( rvMonsterBossBuddy );
};

#define BOSS_BUDDY_MAX_SHIELDS 8500
//------------------------------------------------------------
// rvMonsterBossBuddy::rvMonsterBossBuddy
//------------------------------------------------------------
rvMonsterBossBuddy::rvMonsterBossBuddy( void ) 
{
	mMaxShields = BOSS_BUDDY_MAX_SHIELDS;
	mShields = mMaxShields;
	mLastDamageTime = 0;
	mIsShielded = false;
	mRequestedZoneMove = false;
	mRequestedRecharge = false;
//	mCanIdle = false;
//	mChaseMode = true;
}

void rvMonsterBossBuddy::InitSpawnArgsVariables ( void )
{
	mShieldsLastFor = (int)(spawnArgs.GetFloat( "mShieldsLastFor", "6" ) * 1000.0f);
	mMaxShields = BOSS_BUDDY_MAX_SHIELDS;
}

//------------------------------------------------------------
// rvMonsterBossBuddy::Spawn
//------------------------------------------------------------
void rvMonsterBossBuddy::Spawn( void ) 
{
	mActionRocketAttack.Init( spawnArgs,	"action_rocketAttack",		NULL,	AIACTIONF_ATTACK );
	mActionLightningAttack.Init( spawnArgs,"action_lightningAttack",	NULL,	AIACTIONF_ATTACK );
	mActionDarkMatterAttack.Init( spawnArgs,"action_dmgAttack",			NULL,	AIACTIONF_ATTACK );
	mActionMeleeMoveAttack.Init( spawnArgs,	"action_meleeMoveAttack",	NULL,	AIACTIONF_ATTACK );
	mActionSlashMoveAttack.Init( spawnArgs,	"action_slashMoveAttack",	NULL,	AIACTIONF_ATTACK );
	mActionMeleeAttack.Init( spawnArgs,		"action_meleeAttack",		NULL,	AIACTIONF_ATTACK );

	InitSpawnArgsVariables();
	mShields = mMaxShields;

	const char  *func;
	if ( spawnArgs.GetString( "requestRecharge", "", &func ) ) 
	{
		mRequestRecharge.Init( func );
	}
	if ( spawnArgs.GetString( "requestZoneMove", "", &func ) ) 
	{
		mRequestZoneMove.Init( func );
	}

	HideSurface( "models/monsters/bossbuddy/forcefield" );
}

//------------------------------------------------------------
// rvMonsterBossBuddy::Save
//------------------------------------------------------------
void rvMonsterBossBuddy::Save( idSaveGame *savefile ) const 
{
	savefile->WriteInt( mShots );
	savefile->WriteInt( mShields );
	savefile->WriteInt( mMaxShields );
	savefile->WriteInt( mLastDamageTime );
	savefile->WriteBool( mIsShielded );
	savefile->WriteBool( mRequestedZoneMove );
	savefile->WriteBool( mRequestedRecharge );

	mActionRocketAttack.Save( savefile );
	mActionLightningAttack.Save( savefile );
	mActionDarkMatterAttack.Save( savefile );
	mActionMeleeMoveAttack.Save( savefile );
	mActionMeleeAttack.Save( savefile );
	mActionSlashMoveAttack.Save( savefile );

	mRequestRecharge.Save( savefile );
	mRequestZoneMove.Save( savefile );
}

//------------------------------------------------------------
// rvMonsterBossBuddy::Restore
//------------------------------------------------------------
void rvMonsterBossBuddy::Restore( idRestoreGame *savefile ) 
{
	savefile->ReadInt( mShots );
	savefile->ReadInt( mShields );
	savefile->ReadInt( mMaxShields );
	savefile->ReadInt( mLastDamageTime );
	savefile->ReadBool( mIsShielded );
	savefile->ReadBool( mRequestedZoneMove );
	savefile->ReadBool( mRequestedRecharge );

	mActionRocketAttack.Restore( savefile );
	mActionLightningAttack.Restore( savefile );
	mActionDarkMatterAttack.Restore( savefile );
	mActionMeleeMoveAttack.Restore( savefile );
	mActionMeleeAttack.Restore( savefile );
	mActionSlashMoveAttack.Restore( savefile );

	mRequestRecharge.Restore( savefile );
	mRequestZoneMove.Restore( savefile );

	InitSpawnArgsVariables();
}

//------------------------------------------------------------
// rvMonsterBerserker::GetDebugInfo
//------------------------------------------------------------
void rvMonsterBossBuddy::GetDebugInfo( debugInfoProc_t proc, void* userData ) 
{
	// Base class first
	idAI::GetDebugInfo( proc, userData );
	
	proc ( "idAI", "action_darkMatterAttack",	aiActionStatusString[mActionDarkMatterAttack.status], userData );
	proc ( "idAI", "action_rocketAttack",		aiActionStatusString[mActionRocketAttack.status], userData );
	proc ( "idAI", "action_meleeMoveAttack",	aiActionStatusString[mActionMeleeMoveAttack.status], userData );
	proc ( "idAI", "action_lightningAttack",	aiActionStatusString[mActionLightningAttack.status], userData );
}

//--------------------------------------------------------------
// Custom Script Events
//--------------------------------------------------------------
const idEventDef EV_RechargeShields( "rechargeShields",	"f", 'f' );

CLASS_DECLARATION( idAI, rvMonsterBossBuddy )
	EVENT( EV_RechargeShields, rvMonsterBossBuddy::Event_RechargeShields )
END_CLASS

//------------------------------------------------------------
// rvMonsterBossBuddy::Event_RechargeShields
//------------------------------------------------------------
void rvMonsterBossBuddy::Event_RechargeShields( float amount ) 
{
	mShields += (int)amount;

	if ( mShields >= mMaxShields ) 
	{
		// charge is done
		mShields = mMaxShields;
		idThread::ReturnInt(0);

		// reset request states
		mRequestedRecharge = false;
		mRequestedZoneMove = false;

		// shield warning no longer neede for now
		gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("hideBossShieldWarn");
	}
	else 
	{
		// still charging
		idThread::ReturnInt(1);
	}
}

//------------------------------------------------------------
// rvMonsterBossBuddy::ReduceShields
//------------------------------------------------------------
void rvMonsterBossBuddy::ReduceShields( int amount )
{
	mShields -= amount;

	// if no mShields left... or the last time we took damage was more than 8 seconds ago
	if ( mShields <= 0 || (mLastDamageTime + 8000) < gameLocal.time )
	{
		//....remove the shielding
		AdjustShieldState( false );
    }

	if (mShields < 1000)
	{
		if  (!mRequestedRecharge ) 
		{
			// entering a dangerous state!  Get to the recharge station, fast!
			gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("showBossShieldWarn");
			ExecScriptFunction( mRequestRecharge );
			mRequestedRecharge = true;
		}
	}
	else if (mShields < 4000)
	{
		if ( !mRequestedZoneMove ) 
		{
			// Getting low, so move him close to the next zone so he can be ready to recharge 
			ExecScriptFunction( mRequestZoneMove );
			mRequestedZoneMove = true;
		}
	}
}

//------------------------------------------------------------
// rvMonsterBossBuddy::AdjustShieldState
//------------------------------------------------------------
void rvMonsterBossBuddy::AdjustShieldState( bool becomeShielded )
{
	// only do the work for adjusting the state when it doesn't match our current state
	if ( !mIsShielded && becomeShielded )
	{
		// Activate Shields!
		ShowSurface( "models/monsters/bossbuddy/forcefield" );
		StartSound( "snd_enable_shields", SND_CHANNEL_ANY, 0, false, NULL );
		gameLocal.GetLocalPlayer()->hud->HandleNamedEvent( "showBossShieldBar" );
	}
	else if ( mIsShielded && !becomeShielded )
	{
		// Deactivate Shields!
		HideSurface( "models/monsters/bossbuddy/forcefield" );
		StartSound ( "snd_disable_shields", SND_CHANNEL_ANY, 0, false, NULL );
//		gameLocal.GetLocalPlayer()->hud->HandleNamedEvent( "hideBossShieldBar" );
	}
	mIsShielded = becomeShielded;
}

//------------------------------------------------------------
// rvMonsterBossBuddy::Damage
//------------------------------------------------------------
void rvMonsterBossBuddy::Think() 
{
	if ( !fl.hidden && !fl.isDormant && (thinkFlags & TH_THINK ) && !aifl.dead ) 
	{
		// run simple shielding logic when we have them active
		if ( mIsShielded )
		{
			ReduceShields( 1 );

			// if they are on but we haven't taken damage in x seconds, turn them off to conserve on shields
			if ( (mLastDamageTime + mShieldsLastFor) < gameLocal.time )
			{
				AdjustShieldState( false );
            }
		}

		// update shield bar
		idUserInterface *hud = gameLocal.GetLocalPlayer()->hud;
		if ( hud ) 
		{
			float percent = ((float)mShields/mMaxShields);

			hud->SetStateFloat( "boss_shield_percent", percent );
			hud->HandleNamedEvent( "updateBossShield" );
		}
	}

	if ( move.obstacle.GetEntity() )
	{
		PerformAction( &mActionSlashMoveAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack, &actionTimerSpecialAttack );
	}

	idAI::Think();
}
/*
//------------------------------------------------------------
// rvMonsterBossBuddy::PerformAction
//------------------------------------------------------------
void rvMonsterBossBuddy::PerformAction( const char* stateName, int blendFrames, bool noPain ) 
{
	// Allow movement in actions
	move.fl.allowAnimMove = true;

	if ( mChaseMode )
	{
		return;
	}

	// Start the action
	SetAnimState( ANIMCHANNEL_TORSO, stateName, blendFrames );

	// Always call finish action when the action is done, it will clear the action flag
	aifl.action = true;
	PostAnimState( ANIMCHANNEL_TORSO, "Torso_FinishAction", 0, 0, SFLAG_ONCLEAR );

	// Go back to idle when done-- sometimes.
	if ( mCanIdle )	
	{	
		PostAnimState( ANIMCHANNEL_TORSO, "Torso_Idle", blendFrames );
	}

	// Main state will wait until action is finished before continuing
	InterruptState( "Wait_ActionNoPain" );
	OnStartAction( );
}

//------------------------------------------------------------
// rvMonsterBossBuddy::PerformAction
//------------------------------------------------------------
bool rvMonsterBossBuddy::PerformAction( rvAIAction* action, bool (idAI::*condition)(rvAIAction*,int), rvAIActionTimer* timer )
{
	if ( mChaseMode )
	{
		return false;
	}

	return idAI::PerformAction( action, condition ,timer );
}
*/
//------------------------------------------------------------
// rvMonsterBossBuddy::Damage
//------------------------------------------------------------
void rvMonsterBossBuddy::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
								const char *damageDefName, const float damageScale, const int location ) 
{
	// get damage amount so we can decay the shields and check for ignoreShields
	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, false );
	if ( !damageDef ) 
	{
		gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
	}

	// NOTE: there is a damage def for the electrocution that is marked 'ignoreShields'.. when present on the damage def,
	//	we don't run shielding logic
	bool directDamage = damageDef->GetBool( "ignoreShields" );

	int loc = location;
	if ( directDamage )
	{
		// Lame, I know, but hack the location
		loc = INVALID_JOINT;
	}
	else if ( attacker == this )
	{
		// can't damage self
		return;
	}

	float scale = 1;

	// Shields will activate for a set amount of time when damage is being taken
	mLastDamageTime = gameLocal.time;

	// if shields are active, we should try to 'eat' them before directing damage to the BB
	if ( mIsShielded && !directDamage ) 
	{
		// BB is resistant to any kind of splash damage when the shields are up
		if ( loc <= INVALID_JOINT )
		{
			// damage must have been done by splash damage
			return;
		}

		int	damage = damageDef->GetInt( "damage" );
		ReduceShields( damage * 8 );

		// Shielding dramatically reduces actual damage done to BB
		scale = 0.1f;
	}
	else if ( mShields > 0 ) // not currently shielded...does he have shields to use?
	{
		// Yep, so turn them on
		AdjustShieldState( true );
	}

	idAI::Damage( inflictor, attacker, dir, damageDefName, damageScale * scale, loc );
}

//------------------------------------------------------------
// rvMonsterBossBuddy::Pain
//------------------------------------------------------------
bool rvMonsterBossBuddy::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) 
{	
	// immune to small damage.  Is this safe to do?
	if ( damage > 5 )
	{
		rvClientCrawlEffect* effect;
		effect = new rvClientCrawlEffect( gameLocal.GetEffect( spawnArgs, "fx_shieldcrawl" ), this, SEC2MS(spawnArgs.GetFloat ( "shieldCrawlTime", ".2" )) );
		effect->Play( gameLocal.time, false );

		return idAI::Pain( inflictor, attacker, damage, dir, location );
	}
	return false;
}

//------------------------------------------------------------
// rvMonsterBossBuddy::CheckActions
//------------------------------------------------------------
bool rvMonsterBossBuddy::CheckActions( void ) 
{
	// If not moving, try turning in place
/*	if ( !move.fl.moving && gameLocal.time > combat.investigateTime ) 
	{
		float turnYaw = idMath::AngleNormalize180( move.ideal_yaw - move.current_yaw );
		if ( turnYaw > lookMax[YAW] * 0.75f ) 
		{
			PerformAction( "Torso_TurnRight90", 4, true );
			return true;
		} else if ( turnYaw < -lookMax[YAW] * 0.75f ) 
		{
			PerformAction( "Torso_TurnLeft90", 4, true );
			return true;
		}
	}
*/
	if ( PerformAction( &mActionMeleeMoveAttack,	(checkAction_t)&idAI::CheckAction_MeleeAttack, NULL ) ||
		 PerformAction( &mActionSlashMoveAttack,	(checkAction_t)&idAI::CheckAction_MeleeAttack, &actionTimerSpecialAttack )) 
	{
		return true;
	}

	if ( PerformAction( &mActionDarkMatterAttack,	(checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ||
		 PerformAction( &mActionRocketAttack,		(checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ||
		 PerformAction( &mActionLightningAttack,	(checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ))
	{
		return true;
	}

	return idAI::CheckActions( );
}

//------------------------------------------------------------
// rvMonsterBossBuddy::CanTurn
//------------------------------------------------------------
bool rvMonsterBossBuddy::CanTurn( void ) const 
{
/* 	if ( !idAI::CanTurn ( ) ) {
		return false;
	}
	return move.anim_turn_angles != 0.0f || move.fl.moving; 
*/
	return idAI::CanTurn ( );
}

//------------------------------------------------------------
// rvMonsterBossBuddy::OnWakeUp
//------------------------------------------------------------
void rvMonsterBossBuddy::OnWakeUp( void ) 
{
	mActionDarkMatterAttack.timer.Reset( actionTime, mActionDarkMatterAttack.diversity );
	mActionRocketAttack.timer.Reset( actionTime, mActionDarkMatterAttack.diversity );
	idAI::OnWakeUp( );
}

//------------------------------------------------------------
//	States 
//------------------------------------------------------------

CLASS_STATES_DECLARATION( rvMonsterBossBuddy )
	STATE( "Torso_RocketAttack",	rvMonsterBossBuddy::State_Torso_RocketAttack )
	STATE( "Torso_SlashAttack",		rvMonsterBossBuddy::State_Torso_SlashAttack )
	STATE( "Torso_TurnRight90",		rvMonsterBossBuddy::State_Torso_TurnRight90 )
	STATE( "Torso_TurnLeft90",		rvMonsterBossBuddy::State_Torso_TurnLeft90 )
END_CLASS_STATES

//------------------------------------------------------------
// rvMonsterBossBuddy::State_Torso_RocketAttack
//------------------------------------------------------------
stateResult_t rvMonsterBossBuddy::State_Torso_RocketAttack( const stateParms_t& parms ) 
{
	enum 
	{ 
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch ( parms.stage ) 
	{
		case STAGE_INIT:
			DisableAnimState( ANIMCHANNEL_LEGS );
			PlayAnim( ANIMCHANNEL_TORSO, "attack_rocket2start", parms.blendFrames );
			mShots = (gameLocal.random.RandomInt( 3 ) + 2) * combat.aggressiveScale;
			return SRESULT_STAGE( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone( ANIMCHANNEL_TORSO, 0 )) 
			{
				return SRESULT_STAGE( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_LOOP:
			PlayAnim( ANIMCHANNEL_TORSO, "attack_rocket2loop2", 0 );
			return SRESULT_STAGE( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( AnimDone( ANIMCHANNEL_TORSO, 0 )) 
			{
				if ( --mShots <= 0 ||										// exhausted mShots? .. or
						(!IsEnemyVisible() && rvRandom::irand(0,10)>=8 ) ||	// ... player is no longer visible .. or
						( enemy.ent && DistanceTo(enemy.ent)<256 ) ) 		// ... player is so close, we prolly want to do a melee attack
				{
					PlayAnim( ANIMCHANNEL_TORSO, "attack_rocket2end", 0 );
					return SRESULT_STAGE( STAGE_WAITEND );
				}
				return SRESULT_STAGE( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_WAITEND:
			if ( AnimDone( ANIMCHANNEL_TORSO, 4 )) 
			{
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;				
	}
	return SRESULT_ERROR; 
}

//------------------------------------------------------------
// rvMonsterBossBuddy::State_Torso_SlashAttack
//------------------------------------------------------------
stateResult_t rvMonsterBossBuddy::State_Torso_SlashAttack( const stateParms_t& parms ) 
{
	enum 
	{ 
		STAGE_INIT,
		STAGE_WAIT_FIRST_SWIPE,
		STAGE_WAIT_FINISH
	};
	switch ( parms.stage ) 
	{
		case STAGE_INIT:
			DisableAnimState( ANIMCHANNEL_LEGS );
			PlayAnim( ANIMCHANNEL_TORSO, "melee_move_attack", parms.blendFrames );
			return SRESULT_STAGE( STAGE_WAIT_FIRST_SWIPE );
			
		case STAGE_WAIT_FIRST_SWIPE:
			if ( AnimDone( ANIMCHANNEL_TORSO, 0 )) 
			{
				PlayAnim( ANIMCHANNEL_TORSO, "melee_move_attack", parms.blendFrames );
				return SRESULT_STAGE( STAGE_WAIT_FINISH );
			}
			return SRESULT_WAIT;

		case STAGE_WAIT_FINISH:
			if ( AnimDone( ANIMCHANNEL_TORSO, 0 )) 
			{
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

//------------------------------------------------------------
// rvMonsterBossBuddy::State_Torso_TurnRight90
//------------------------------------------------------------
stateResult_t rvMonsterBossBuddy::State_Torso_TurnRight90( const stateParms_t& parms ) 
{
	enum 
	{ 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) 
	{
		case STAGE_INIT:
			DisableAnimState( ANIMCHANNEL_LEGS );
			PlayAnim( ANIMCHANNEL_TORSO, "turn_right", parms.blendFrames );
			AnimTurn( 90.0f, true );
			return SRESULT_STAGE( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( move.fl.moving || AnimDone( ANIMCHANNEL_TORSO, 0 )) 
			{
				AnimTurn( 0, true );
				combat.investigateTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

//------------------------------------------------------------
// rvMonsterBossBuddy::State_Torso_TurnLeft90
//------------------------------------------------------------
stateResult_t rvMonsterBossBuddy::State_Torso_TurnLeft90( const stateParms_t& parms ) 
{	
	enum 
	{ 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) 
	{
		case STAGE_INIT:
			DisableAnimState( ANIMCHANNEL_LEGS );
			PlayAnim( ANIMCHANNEL_TORSO, "turn_left", parms.blendFrames );
			AnimTurn( 90.0f, true );
			return SRESULT_STAGE( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( move.fl.moving || AnimDone( ANIMCHANNEL_TORSO, 0 )) 
			{
				AnimTurn( 0, true );
				combat.investigateTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}
