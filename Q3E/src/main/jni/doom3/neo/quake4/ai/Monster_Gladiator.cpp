
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../client/ClientModel.h"

class rvMonsterGladiator : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterGladiator );

	rvMonsterGladiator ( void );

	void					InitSpawnArgsVariables( void );
	void					Spawn				( void );
	void					Save				( idSaveGame *savefile ) const;
	void					Restore				( idRestoreGame *savefile );

	bool					CanTurn				( void ) const;

	virtual void			GetDebugInfo		( debugInfoProc_t proc, void* userData );

	virtual	void			Damage				( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			AddDamageEffect		( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor );

	virtual bool			UpdateRunStatus		( void );

	virtual int				FilterTactical		( int availableTactical );

	virtual int				GetDamageForLocation( int damage, int location );

//	virtual void			SetTether			( rvAITether* newTether );

protected:

	// Actions
	rvAIAction				actionRailgunAttack;

	// Blaster attack
	int						maxShots;	
	int						minShots;
	int						shots;
	int						lastShotTime;

	// Shield
	bool					usingShield;
	idEntityPtr<idEntity>	shield;
	int						shieldStartTime;
	int						shieldWaitTime;
	int						shieldHitDelay;
	//int						shieldInDelay;
	//int						shieldFov;
	int						shieldHealth;
	int						shieldConsecutiveHits;
	int						shieldLastHitTime;

	int						railgunHealth;
	int						railgunDestroyedTime;
	int						nextTurnTime;

	virtual bool			CheckActions				( void );
	void					ShowShield					( void );
	void					HideShield					( int hideTime=0 );
	void					DestroyRailgun				( void );

private:

	// Global States
	stateResult_t			State_Killed				( const stateParms_t& parms );

	// Torso states
	stateResult_t			State_Torso_BlasterAttack	( const stateParms_t& parms );
	stateResult_t			State_Torso_RailgunAttack	( const stateParms_t& parms );
	stateResult_t			State_Torso_ShieldStart		( const stateParms_t& parms );
	stateResult_t			State_Torso_ShieldEnd		( const stateParms_t& parms );
	stateResult_t			State_Torso_TurnRight90		( const stateParms_t& parms );
	stateResult_t			State_Torso_TurnLeft90		( const stateParms_t& parms );
	stateResult_t			State_Torso_ShieldFire		( const stateParms_t& parms );

	rvScriptFuncUtility		mPostWeaponDestroyed;		// script to run after railgun is destroyed

	CLASS_STATES_PROTOTYPE ( rvMonsterGladiator );
};

CLASS_DECLARATION( idAI, rvMonsterGladiator )
END_CLASS

/*
================
rvMonsterGladiator::rvMonsterGladiator
================
*/
rvMonsterGladiator::rvMonsterGladiator ( ) {
	usingShield = false;
}

void rvMonsterGladiator::InitSpawnArgsVariables ( void )
{
	maxShots		= spawnArgs.GetInt ( "maxShots", "1" );
	minShots		= spawnArgs.GetInt ( "minShots", "1" );
	shieldHitDelay  = SEC2MS ( spawnArgs.GetFloat ( "shieldHitDelay", "1" ) );
//	shieldInDelay	= SEC2MS ( spawnArgs.GetFloat ( "shieldInDelay", "3" ) );
//	shieldFov		= spawnArgs.GetInt ( "shieldfov", "90" );
}
/*
================
rvMonsterGladiator::Spawn
================
*/
void rvMonsterGladiator::Spawn ( void ) {
	shieldWaitTime	= 0;
	shieldStartTime = 0;
	shieldHealth	= 250;
	shieldConsecutiveHits	= 0;
	shieldLastHitTime		= 0;

	InitSpawnArgsVariables();

	shots			= 0;	
	lastShotTime	= 0;

	railgunHealth	= spawnArgs.GetInt ( "railgunHealth", "100" );
	railgunDestroyedTime = 0;
	
	actionRailgunAttack.Init	( spawnArgs, "action_railgunAttack",	"Torso_RailgunAttack",	 AIACTIONF_ATTACK );	

	// Disable range attack until using shield	
	//actionRangedAttack.fl.disabled = true;
	const char  *func;
	if ( spawnArgs.GetString( "script_postWeaponDestroyed", "", &func ) ) 
	{
		mPostWeaponDestroyed.Init( func );
	}
}

/*
================
rvMonsterGladiator::CheckActions

Overriden to handle taking the shield out and putting it away.  Will also ensure the gladiator
stays hidden behind his shield if getting shot at.
================
*/
bool rvMonsterGladiator::CheckActions ( void ) {
	// If not moving, try turning in place
	if ( !move.fl.moving && gameLocal.time > nextTurnTime ) {
		float turnYaw = idMath::AngleNormalize180 ( move.ideal_yaw - move.current_yaw ) ;
		if ( turnYaw > lookMax[YAW] * 0.75f || (turnYaw > 0 && !enemy.fl.inFov) ) {
			PerformAction ( "Torso_TurnRight90", 4, true );
			return true;
		} else if ( turnYaw < -lookMax[YAW] * 0.75f || (turnYaw < 0 && !enemy.fl.inFov) ) {
			PerformAction ( "Torso_TurnLeft90", 4, true );
			return true;
		}
	}

	if ( CheckPainActions ( ) ) {
		return true;
	}

	// Limited actions with shield out
	if ( usingShield ) {
		if ( railgunHealth > 0 && PerformAction ( &actionRailgunAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerSpecialAttack ) ) {
			return true;
		}
		if ( move.moveCommand == MOVE_TO_ENEMY
			&& move.fl.moving )
		{//advancing on enemy with shield up
			if ( gameLocal.GetTime() - lastShotTime > 1500 )
			{//been at least a second since the last time we fired while moving
				if ( !gameLocal.random.RandomInt(2) )
				{//fire!
					PerformAction ( "Torso_ShieldFire", 0, true );
					return true;
				}
			}
		}
		// Only ranged attack and melee attack are available when using shield
		if ( PerformAction ( &actionMeleeAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack )							    ||
			 PerformAction ( &actionRangedAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )   ||
			 ( railgunHealth > 0 
				&& gameLocal.GetTime() - shieldStartTime > 2000
				&& gameLocal.time - pain.lastTakenTime > 500
				&& gameLocal.time - combat.shotAtTime > 300
				&& gameLocal.GetTime() - shieldLastHitTime > 500
				&& PerformAction ( &actionRailgunAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerSpecialAttack ) ) ) {
			shieldWaitTime = 0;
			return true;
		}

		// see if it's safe to lower it?
		if ( gameLocal.GetTime() - shieldStartTime > 2000 )
		{//shield's been up for at least 2 seconds
			if ( !enemy.fl.visible || (gameLocal.time - combat.shotAtTime > 1000 && gameLocal.GetTime() - shieldLastHitTime > 1500) )
			{
				if ( gameLocal.time - pain.lastTakenTime > 1500 )
				{
					PerformAction ( "Torso_ShieldEnd", 4, true );
					return true;
				}
			}
		}
		
		return false;
	}
	else
	{// Bring the shield out?
		if ( combat.tacticalCurrent != AITACTICAL_MELEE || move.fl.done )
		{//not while rushing (NOTE: unless railgun was just destroyed?)
			if ( enemy.fl.visible && enemy.fl.inFov )
			{
				if ( combat.fl.aware && shieldWaitTime < gameLocal.GetTime() ) 
				{
					if ( gameLocal.time - pain.lastTakenTime <= 1500
						|| ( combat.shotAtAngle < 0 && gameLocal.time - combat.shotAtTime < 100 ) 
						|| !gameLocal.random.RandomInt( 20 ) )
					{
						if ( !gameLocal.random.RandomInt( 5 ) )
						{
							PerformAction ( "Torso_ShieldStart", 4, true );
							return true;
						}
					}
				}
			}
		}
		if ( railgunHealth > 0 && PerformAction ( &actionRailgunAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerSpecialAttack ) ) {
			return true;
		}
	}
	
	return idAI::CheckActions ( );
}

/*
================
rvMonsterGladiator::ShowShield
================
*/
void rvMonsterGladiator::ShowShield	( void ) {
	// First time?
	if ( !shield ) {
		idEntity* ent;
		idDict   args;
		const idDict *shieldDef = gameLocal.FindEntityDefDict( spawnArgs.GetString ( "def_shield" ), false );
		args.Set ( "classname", spawnArgs.GetString ( "def_shield" ) );
		if ( gameLocal.SpawnEntityDef( args, &ent ) ) {
			shield = ent;
			ent->GetPhysics()->SetClipMask ( 0 );
			ent->GetPhysics()->SetContents ( CONTENTS_RENDERMODEL );
			ent->GetPhysics()->GetClipModel ( )->SetOwner ( this );
			Attach ( ent );
		}
		if ( !shield ) {
			return;
		}
		if ( shieldDef && shield->IsType( idAFAttachment::GetClassType() ) )
		{
			idAFAttachment* afShield = static_cast<idAFAttachment*>(shield.GetEntity());
			if ( afShield )
			{
				jointHandle_t joint = animator.GetJointHandle( shieldDef->GetString( "joint" ) );
				afShield->SetBody ( this, shieldDef->GetString( "model" ), joint );
			}
		}
	} else if ( !shield || !shield->IsHidden() ) {
		return;
	}

	usingShield						= true;		
	shieldWaitTime					= 0;
	animPrefix						= "shield";
	shieldStartTime					= gameLocal.time;
//	actionRangedAttack.fl.disabled	= false;
	shieldHealth					= 250;
	shieldConsecutiveHits			= 0;
	shieldLastHitTime				= 0;
	shield->SetShaderParm( SHADERPARM_MODE, 0 );

	// Looping shield sound
	StartSound ( "snd_shield_loop", SND_CHANNEL_ITEM, 0, false, NULL );

	shield->Show ( );

	SetShaderParm ( 6, gameLocal.time + 2000 );
}

/*
================
rvMonsterGladiator::HideShield
================
*/
void rvMonsterGladiator::HideShield ( int hideTime ) {
	if ( !shield || shield->IsHidden() ) {
		return;
	}

	usingShield						= false;
	animPrefix						= "";
	shieldWaitTime					= gameLocal.GetTime()+hideTime;
//	actionRangedAttack.fl.disabled	= true;
	shieldHealth					= 0;
	shieldConsecutiveHits			= 0;
	shieldLastHitTime				= 0;
	shield->SetShaderParm( SHADERPARM_MODE, 0 );

	// Looping shield sound
	StopSound ( SND_CHANNEL_ITEM, false );

	shield->Hide ( );
}

/*
================
rvMonsterGladiator::DestroyRailgun
================
*/
void rvMonsterGladiator::DestroyRailgun ( void ) {
	HideSurface ( "models/monsters/gladiator/glad_railgun" );
	railgunHealth = -1;
	
	idVec3			origin;
	idMat3			axis;
	jointHandle_t	joint;
	
	joint = animator.GetJointHandle ( spawnArgs.GetString ( "joint_railgun_explode", "gun_main_jt" ) );
	GetJointWorldTransform ( joint, gameLocal.time, origin, axis );
	gameLocal.PlayEffect ( spawnArgs, "fx_railgun_explode", origin, axis );	
	PlayEffect ( "fx_railgun_burn", joint, true );
	
	GetAFPhysics()->GetBody ( "b_railgun" )->SetClipMask ( 0 );
	
	pain.takenThisFrame = pain.threshold;
	pain.lastTakenTime = gameLocal.time;

	DisableAnimState( ANIMCHANNEL_LEGS );
	painAnim = "pain_big";
	SetAnimState( ANIMCHANNEL_TORSO, "Torso_Pain" );
	PostAnimState( ANIMCHANNEL_TORSO, "Torso_Idle" );

	railgunDestroyedTime = gameLocal.GetTime();

	// Tweak-out the AI to be more aggressive and more likely to charge?
	actionRailgunAttack.fl.disabled = true;

	combat.attackRange[1] = 200;
	combat.aggressiveRange = 400;
	
	spawnArgs.SetFloat( "action_meleeAttack_rate", 0.3f );
	actionMeleeAttack.Init( spawnArgs, "action_meleeAttack", NULL, AIACTIONF_ATTACK );
	actionMeleeAttack.failRate = 200;
	actionMeleeAttack.chance = 1.0f;

	actionRangedAttack.chance = 0.25f;
	actionRangedAttack.maxRange = 400;
	minShots = 5;
	maxShots = 15;

	combat.tacticalMaskAvailable &= ~AITACTICAL_HIDE_BIT;

	//temporarily disable this so we can charge and get mad
	//FIXME: force MELEE
	actionRangedAttack.timer.Add( 6000 );
	actionTimerRangedAttack.Add( 6000 );
	actionMeleeAttack.timer.Reset( actionTime );

	//drop any tether since we need to advance
	//SetTether(NULL);
	//nevermind: let scripters handle it
	ExecScriptFunction( mPostWeaponDestroyed );
}

/*
================
rvMonsterGladiator::UpdateRunStatus
================
*/
bool rvMonsterGladiator::UpdateRunStatus ( void ) {
	// If rushing and moving forward, run
	if ( combat.tacticalCurrent == AITACTICAL_MELEE && move.currentDirection == MOVEDIR_FORWARD ) {
		move.fl.idealRunning = true;
		return move.fl.running != move.fl.idealRunning;
	}

	// Alwasy walk with shield out
	if ( usingShield ) {
		move.fl.idealRunning = false;
		return move.fl.running != move.fl.idealRunning;
	}
	
	return idAI::UpdateRunStatus ( );
}

/*
============
rvMonsterGladiator::SetTether
============
*/
/*
void rvMonsterGladiator::SetTether ( rvAITether* newTether ) {
	if ( railgunHealth <= 0 ) {
		//don't allow any tethers!
		idAI::SetTether(NULL);
	} else {
		idAI::SetTether(newTether);
	}
}
*/

/*
================
rvMonsterGladiator::FilterTactical
================
*/
int rvMonsterGladiator::FilterTactical ( int availableTactical ) {
	if ( railgunHealth > 0 ) { // Only let the gladiator rush when he is really close to his enemy
		if ( !enemy.range || enemy.range > combat.awareRange ) {
			availableTactical &= ~AITACTICAL_MELEE_BIT;
		} else {
			availableTactical &= ~(AITACTICAL_RANGED_BITS);
		}
	} else if ( gameLocal.GetTime() - railgunDestroyedTime < 6000 )	{
		availableTactical = AITACTICAL_MELEE_BIT;
	}
	
	return idAI::FilterTactical ( availableTactical );
}

/*
================
rvMonsterGladiator::AddDamageEffect
================
*/
void rvMonsterGladiator::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor ) {
	if ( collision.c.material != NULL && (collision.c.material->GetSurfaceFlags() & SURF_NODAMAGE ) ) {
		// Delay putting shield away and shooting until the shield hasnt been hit for a while
		actionRangedAttack.timer.Reset ( actionTime );
		actionRangedAttack.timer.Add ( shieldHitDelay );
		shieldStartTime = gameLocal.time;
		return;
	}
	
	return idAI::AddDamageEffect ( collision, velocity, damageDefName, inflictor );		
}

/*
=====================
rvMonsterGladiator::GetDamageForLocation
=====================
*/
int rvMonsterGladiator::GetDamageForLocation( int damage, int location ) {
	// If the gun was hit only do damage to it
	if ( idStr::Icmp ( GetDamageGroup ( location ), "gun" ) == 0 ) {
//		pain.takenThisFrame = damage;
		if ( railgunHealth > 0 ){
			railgunHealth -= damage;
			if ( railgunHealth <= 0 ) {
				DestroyRailgun ( );
			}
		}
		return 0;
	}
	 
	return idAI::GetDamageForLocation ( damage, location );
}

/*
================
rvMonsterGladiator::CanTurn
================
*/
bool rvMonsterGladiator::CanTurn ( void ) const {
	if ( !idAI::CanTurn ( ) ) {
		return false;
	}
	return move.anim_turn_angles != 0.0f || move.fl.moving;
}

/*
================
rvMonsterGladiator::Damage
================
*/
void rvMonsterGladiator::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
					  const char *damageDefName, const float damageScale, const int location ) 
{	
	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, false );
	if ( damageDef )
	{
		if ( usingShield )
		{//shield up
			if ( !damageDef->GetString( "filter_electricity", NULL ) )
			{//not by electricity
				//If we get hit enough times with shield up, charge forward
				if ( gameLocal.GetTime() - shieldLastHitTime > 1500 )
				{
					shieldConsecutiveHits = 0;
				}
				shieldConsecutiveHits++;
				shieldLastHitTime = gameLocal.GetTime();
				if ( shieldConsecutiveHits > 20 && combat.tacticalCurrent != AITACTICAL_MELEE && move.fl.done )
				{//really laying into us, move up
					combat.tacticalUpdateTime = gameLocal.GetTime();
					MoveToEnemy();
					//reset counter
					shieldConsecutiveHits = 0;
				}
			}

			if ( idStr::Icmp ( GetDamageGroup ( location ), "shield" ) == 0 ) 
			{//Hit in shield
				if ( damageDef->GetString( "filter_electricity", NULL ) )
				{//by electricity
					shieldHealth -= damageDef->GetInt( "damage" ) * damageScale;
					if ( shield )
					{
						shield->SetShaderParm( SHADERPARM_MODE, gameLocal.GetTime() + gameLocal.random.RandomInt(1000) + 1000 );
					}
					StartSound( "snd_shield_flicker", SND_CHANNEL_ANY, 0, false, NULL );
					if ( shieldHealth <= 0 )
					{//drop it
						HideShield( gameLocal.random.RandomInt(3000)+2000 );//FIXME: when it does come back on, flicker back on?
						painAnim = "pain_con";
						AnimTurn( 0, true );
						PerformAction ( "Torso_Pain", 2, true );	
					}
					combat.shotAtTime = gameLocal.GetTime();
				}
				return;
			}
		}
	}
	idAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	if ( aifl.pain )
	{//hurt
		if ( usingShield )
		{//shield up
			//move in!
			combat.tacticalUpdateTime = gameLocal.GetTime();
			MoveToEnemy();
			//reset counter
			shieldConsecutiveHits = 0;
		}
	}
}

/*
================
rvMonsterGladiator::Save
================
*/
void rvMonsterGladiator::Save( idSaveGame *savefile ) const {
	actionRailgunAttack.Save ( savefile ) ;	
	
	savefile->WriteInt ( shots );
	savefile->WriteInt ( lastShotTime );

	savefile->WriteBool ( usingShield );
	shield.Save( savefile );
	savefile->WriteInt ( shieldStartTime );
	savefile->WriteInt ( shieldWaitTime );
	savefile->WriteInt ( shieldHealth );
	savefile->WriteInt ( shieldConsecutiveHits );
	savefile->WriteInt ( shieldLastHitTime );
	
	savefile->WriteInt ( railgunHealth );
	savefile->WriteInt ( railgunDestroyedTime );
	savefile->WriteInt ( nextTurnTime ); // cnicholson: added unsaved var
	mPostWeaponDestroyed.Save( savefile );
}

/*
================
rvMonsterGladiator::Restore
================
*/
void rvMonsterGladiator::Restore( idRestoreGame *savefile ) {
	actionRailgunAttack.Restore ( savefile ) ;	

	savefile->ReadInt ( shots );
	savefile->ReadInt ( lastShotTime );
	savefile->ReadBool ( usingShield );
	shield.Restore( savefile );
	savefile->ReadInt ( shieldStartTime );
	savefile->ReadInt ( shieldWaitTime );
	savefile->ReadInt ( shieldHealth );
	savefile->ReadInt ( shieldConsecutiveHits );
	savefile->ReadInt ( shieldLastHitTime );

	savefile->ReadInt ( railgunHealth );
	savefile->ReadInt ( railgunDestroyedTime );
	savefile->ReadInt ( nextTurnTime ); // cnicholson: added unsaved var
	mPostWeaponDestroyed.Restore( savefile );

	InitSpawnArgsVariables();
}

/*
================
rvMonsterGladiator::GetDebugInfo
================
*/
void rvMonsterGladiator::GetDebugInfo( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );
	
	proc ( "idAI", "action_RailgunAttack",	aiActionStatusString[actionRailgunAttack.status], userData );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterGladiator )
	STATE ( "State_Killed",					rvMonsterGladiator::State_Killed )

	STATE ( "Torso_BlasterAttack",			rvMonsterGladiator::State_Torso_BlasterAttack )
	STATE ( "Torso_RailgunAttack",			rvMonsterGladiator::State_Torso_RailgunAttack )
	STATE ( "Torso_ShieldStart",			rvMonsterGladiator::State_Torso_ShieldStart )
	STATE ( "Torso_ShieldEnd",				rvMonsterGladiator::State_Torso_ShieldEnd )

	STATE ( "Torso_TurnRight90",			rvMonsterGladiator::State_Torso_TurnRight90 )
	STATE ( "Torso_TurnLeft90",				rvMonsterGladiator::State_Torso_TurnLeft90 )
	STATE ( "Torso_ShieldFire",				rvMonsterGladiator::State_Torso_ShieldFire )
	
END_CLASS_STATES

/*
================
rvMonsterGladiator::State_Killed
================
*/
stateResult_t rvMonsterGladiator::State_Killed ( const stateParms_t& parms ) {
	HideShield ( );
	return idAI::State_Killed ( parms );
}

/*
================
rvMonsterGladiator::State_Torso_ShieldStart
================
*/
stateResult_t rvMonsterGladiator::State_Torso_ShieldStart ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			ShowShield ( );
			PlayAnim ( ANIMCHANNEL_TORSO, "start", parms.blendFrames );		
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 )
				|| animator.CurrentAnim( ANIMCHANNEL_TORSO )->GetEndTime() < 0//anim somehow cycled?!!
				|| idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "shield_end" ) ) {//anim changed
				SetShaderParm ( 6, 0 );
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 0 );
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterGladiator::State_Torso_ShieldEnd
================
*/
stateResult_t rvMonsterGladiator::State_Torso_ShieldEnd ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "end", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 2 ) //anim done
				|| animator.CurrentAnim( ANIMCHANNEL_TORSO )->GetEndTime() < 0//anim somehow cycled?!!!
				|| idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "shield_end" ) ) {//anim changed
				HideShield ( 2000 );
				actionRailgunAttack.timer.Reset( actionTime );
				actionMeleeAttack.timer.Reset( actionTime );
				actionRangedAttack.timer.Reset( actionTime );
				actionTimerRangedAttack.Reset( actionTime );
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 2 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterGladiator::State_Torso_BlasterAttack
================
*/
stateResult_t rvMonsterGladiator::State_Torso_BlasterAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "blaster_start", parms.blendFrames );
			//shots = 4;
			shots = (minShots + gameLocal.random.RandomInt(maxShots-minShots+1)) * combat.aggressiveScale;
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 )
				|| animator.CurrentAnim( ANIMCHANNEL_TORSO )->GetEndTime() < 0//anim somehow cycled?!!!
				|| idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "blaster_start" ) ) {//anim changed
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_LOOP:
			PlayAnim ( ANIMCHANNEL_TORSO, "blaster_loop", 0 );
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 )
				|| animator.CurrentAnim( ANIMCHANNEL_TORSO )->GetEndTime() < 0//anim somehow cycled?!!!
				|| idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "blaster_loop" ) ) {//anim changed
				if ( --shots <= 0 || !enemy.fl.inFov || aifl.damage ) {
					PlayAnim ( ANIMCHANNEL_TORSO, "blaster_end", 0 );
					return SRESULT_STAGE ( STAGE_WAITEND );
				}
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_WAITEND:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 )
				|| animator.CurrentAnim( ANIMCHANNEL_TORSO )->GetEndTime() < 0//anim somehow cycled?!!!
				|| idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "blaster_loop" ) ) {//anim changed
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;				
	}
	return SRESULT_ERROR; 
}


/*
================
rvMonsterGladiator::State_Torso_RailgunAttack
================
*/
stateResult_t rvMonsterGladiator::State_Torso_RailgunAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( usingShield ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_ShieldEnd", parms.blendFrames );
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_RailgunAttack", parms.blendFrames );
				return SRESULT_DONE;
			}
			
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "railgun_attack", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames )
				|| animator.CurrentAnim( ANIMCHANNEL_TORSO )->GetEndTime() < 0//anim somehow cycled?!!!
				|| idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "railgun_attack" ) ) {//anim changed
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;		
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterGladiator::State_Torso_TurnRight90
================
*/
stateResult_t rvMonsterGladiator::State_Torso_TurnRight90 ( const stateParms_t& parms ) {	
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
			if ( move.fl.moving 
				|| AnimDone ( ANIMCHANNEL_TORSO, 0 ) 
				|| animator.CurrentAnim( ANIMCHANNEL_TORSO )->GetEndTime() < 0//anim somehow cycled?!!!
				|| idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "turn_right" ) ) {//anim changed
				AnimTurn ( 0, true );
				nextTurnTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterGladiator::State_Torso_TurnLeft90
================
*/
stateResult_t rvMonsterGladiator::State_Torso_TurnLeft90 ( const stateParms_t& parms ) {	
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
			if ( move.fl.moving 
				|| AnimDone ( ANIMCHANNEL_TORSO, 0 ) 
				|| animator.CurrentAnim( ANIMCHANNEL_TORSO )->GetEndTime() < 0//anim somehow cycled?!!!
				|| idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "turn_left" ) ) {//anim changed
				AnimTurn ( 0, true );
				nextTurnTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterGladiator::State_Torso_ShieldFire
================
*/
stateResult_t rvMonsterGladiator::State_Torso_ShieldFire ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_ATTACK,
		STAGE_ATTACK_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT: 
			if ( !enemy.ent ) {
				return SRESULT_DONE;	
			}
			shots = (minShots + gameLocal.random.RandomInt(maxShots-minShots+1)) * combat.aggressiveScale;
			PlayCycle( ANIMCHANNEL_TORSO, "walk_aim", 1 );
			return SRESULT_STAGE ( STAGE_ATTACK );
		
		case STAGE_ATTACK: 
			Attack( "blaster", animator.GetJointHandle( "lft_wrist_jt"), GetEnemy() );
			PlayEffect( "fx_blaster_flash", animator.GetJointHandle("lft_wrist_jt") );
			lastShotTime = gameLocal.GetTime();
			return SRESULT_STAGE ( STAGE_ATTACK_WAIT );
			
		case STAGE_ATTACK_WAIT:
			if ( move.fl.done )
			{
				return SRESULT_DONE;
			}
			if ( (gameLocal.GetTime()-lastShotTime) >= 250 ) {
				shots--;
				if ( GetEnemy() && shots > 0 )
				{
					return SRESULT_STAGE ( STAGE_ATTACK );
				}
				PlayCycle( ANIMCHANNEL_TORSO, "walk", 1 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
