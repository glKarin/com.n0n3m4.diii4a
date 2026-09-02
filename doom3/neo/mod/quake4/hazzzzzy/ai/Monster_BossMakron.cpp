#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterBossMakron : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterBossMakron );

	rvMonsterBossMakron ( void );

	void				Spawn							( void );
	void				InitSpawnArgsVariables			( void );

	bool				CanTurn							( void ) const;

	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );

	void				BuildActionArray				( void );

	//void				ScriptedFace					( idEntity* faceEnt, bool endWithIdle );

protected:

	bool				CheckActions					( void );
	bool				CheckTurnActions				( void );

	void				Killed							( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	void				Damage							( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );

	//The Makron behaves differently, and we don't always need him sinking back into an idle after every attack	
	//This version is actually overridden
	void				PerformAction					( const char* stateName, int blendFrames, bool noPain );

	//This just calls the parent version
	bool				PerformAction					( rvAIAction* action, bool (idAI::*condition)(rvAIAction*,int), rvAIActionTimer* timer );

	//This performs a patterned action
	void				PerformPatternedAction			( );

	//For debug, may not need this.
	void				OnStopAction					( void );

	//stops all effects
	void				StopAllEffects					( void );

	//for the cannon fire
	int					shots;

	//determines if the Makron will enter idles between attacks
	bool				noIdle;

	//changes Makron from patterned, script controlled to normal AI
	bool				patternedMode;

	//store the ideal yaw, because somehow between here and the state we set it gets stomped.
	float				facingIdealYaw;
	float				facingTime;
	float				turnRate;

	//flag to indicate whether or not the Makron is in the corner of the map when he takes an action.
	bool				flagCornerState;

	//tallies up the number of consecutive actions the Makron has taken in the corner of the map.
	//If the tally gets too high, teleport him to the map center.
	int					cornerCount;
	
	//in the state of teleporting
	bool				flagTeleporting;

	//for flying mode ----------------------------------------------------------------------------------------
	void				BeginSeparation					( void );
	void				CompleteSeparation				( void );

	bool				flagFlyingMode;
	bool				flagFakeDeath;
	int					flagUndying;
	
	rvClientEffectPtr	effectHover;
	jointHandle_t		jointHoverEffect;

	//for the stomp attack ----------------------------------------------------------------------------------------
	float				stompMaxRadius;
	float				stompWidth;
	float				stompSpeed;
	float				stompRadius;

	//for the lightning bolt sweep attacks -------------------------------------------------------------------
	void				MaintainBoltSweep				( void );
	void				InitBoltSweep					( idVec3 idealTarget );
	void				LightningSweep					( idVec3 attackVector, rvClientEffectPtr& boltEffect, rvClientEffectPtr& impactEffect );
	void				StopAllBoltEffects				( void );

	rvClientEffectPtr		leftBoltEffect;
	rvClientEffectPtr		rightBoltEffect;
	rvClientEffectPtr		leftBoltImpact;
	rvClientEffectPtr		rightBoltImpact;
	rvClientEffectPtr		boltMuzzleFlash;

	idStr				boltEffectName;

	idVec3				leftBoltVector;
	idVec3				rightBoltVector;
	idVec3				boltVectorMin;
	idVec3				boltVectorMax;
	idMat3				boltAimMatrix;

	bool				flagSweepDone;

	//these are grabbed from the def file
	float				boltSweepTime;
	float				boltWaitTime;

	//used internally
	float				boltSweepStartTime;
	float				boltTime;
	float				boltNextStateTime;
	
	int					stateBoltSweep;

	jointHandle_t		jointLightningBolt;

	//end lightning bolt sweep attacks -------------------------------------------------------------------
	//changed by script event, allows for grenades to spawn baddies.
	bool				flagAllowSpawns;
	
	enum {	MAKRON_ACTION_DMG,
			MAKRON_ACTION_MELEE,
			MAKRON_ACTION_CANNON,
			MAKRON_ACTION_CANNON_SWEEP,
			MAKRON_ACTION_GRENADE,
			MAKRON_ACTION_LIGHTNING_1,
			MAKRON_ACTION_LIGHTNING_2,
			MAKRON_ACTION_STOMP,
			MAKRON_ACTION_HEAL,
			MAKRON_ACTION_CHARGE,
			MAKRON_ACTION_KILLPLAYER,
			MAKRON_ACTION_COUNT, };

	rvAIAction			actionDMGAttack;
	rvAIAction			actionMeleeAttack;
	rvAIAction			actionCannonAttack;
	rvAIAction			actionCannonSweepAttack;
	rvAIAction			actionGrenadeAttack;
	rvAIAction			actionLightningPattern1Attack;
	rvAIAction			actionLightningPattern2Attack;
	rvAIAction			actionStompAttack;
	rvAIAction			actionHeal;
	rvAIAction			actionCharge;
	rvAIAction			actionKillPlayer;

	rvAIAction	*		actionArray[ MAKRON_ACTION_COUNT ];

	int					actionPatterned;

	//when the Makron is low on health, he'll use this.
	rvScriptFuncUtility	scriptRecharge;
	rvScriptFuncUtility	scriptTeleport;

	stateResult_t		State_Torso_DMGAttack			( const stateParms_t& parms );
	stateResult_t		State_Torso_GrenadeAttack		( const stateParms_t& parms );
	stateResult_t		State_Torso_MeleeAttack			( const stateParms_t& parms );
	stateResult_t		State_Torso_CannonAttack		( const stateParms_t& parms );
	stateResult_t		State_Torso_CannonSweepAttack	( const stateParms_t& parms );
	stateResult_t		State_Torso_Lightning1Attack	( const stateParms_t& parms );
	stateResult_t		State_Torso_Lightning2Attack	( const stateParms_t& parms );
	stateResult_t		State_Torso_StompAttack			( const stateParms_t& parms );
	stateResult_t		State_Torso_Recharge			( const stateParms_t& parms );
	stateResult_t		State_Torso_Charge				( const stateParms_t& parms );
	stateResult_t		State_Torso_KillPlayer			( const stateParms_t& parms );
	stateResult_t		State_Torso_Teleport			( const stateParms_t& parms );

	//used when killed
	stateResult_t		State_Killed					( const stateParms_t& parms );

	//walking turn anims
	stateResult_t		State_Torso_TurnRight90			( const stateParms_t& parms );
	stateResult_t		State_Torso_TurnLeft90			( const stateParms_t& parms );

	//flying turn anims
	stateResult_t		State_Torso_RotateToAngle		( const stateParms_t& parms );

	stateResult_t		State_ScriptedFace				( const stateParms_t& parms );

	//like jesus, except with robot parts
	stateResult_t		State_Torso_FirstDeath			( const stateParms_t& parms );
	stateResult_t		State_Torso_Resurrection		( const stateParms_t& parms );

	//These two frames activate and deactivate the second lightning sweep
	stateResult_t		Frame_BeginLightningSweep2		( const stateParms_t& parms );
	stateResult_t		Frame_EndLightningSweep2		( const stateParms_t& parms );
	stateResult_t		Frame_StompAttack				( const stateParms_t& parms );
	stateResult_t		Frame_Teleport					( const stateParms_t& parms );


	void				Event_AllowMoreSpawns			( void );
	void				Event_SetNextAction				( const char* actionString );
	void				Event_EnablePatternMode			( void );
	void				Event_DisablePatternMode		( void );
	void				Event_StompAttack				( idVec3 &origin );
	void				Event_Separate					( void );
	void				Event_FlyingRotate				( idVec3 &vecOrg );
	void				Event_ToggleCornerState			( float f );


	CLASS_STATES_PROTOTYPE ( rvMonsterBossMakron );
};

const idEventDef EV_AllowMoreSpawns(	"allowMoreSpawns" );
const idEventDef EV_SetNextAction(	"setNextAction", "s", 'f' );
const idEventDef EV_EnablePatternMode(  "enablePatternMode" );
const idEventDef EV_DisablePatternMode(  "disablePatternMode" );
const idEventDef EV_StompAttack( "stompAttack", "v" );
const idEventDef EV_Separate( "separate" );
const idEventDef EV_FlyingRotate( "flyingRotate", "v");
const idEventDef AI_ScriptedFace					( "scriptedFace", "ed" );
const idEventDef EV_ToggleCornerState( "toggleCornerState", "f" );


CLASS_DECLARATION( idAI, rvMonsterBossMakron )
	EVENT( EV_AllowMoreSpawns,			rvMonsterBossMakron::Event_AllowMoreSpawns )
	EVENT( EV_SetNextAction,			rvMonsterBossMakron::Event_SetNextAction   )
	EVENT( EV_EnablePatternMode,		rvMonsterBossMakron::Event_EnablePatternMode )
	EVENT( EV_DisablePatternMode,		rvMonsterBossMakron::Event_DisablePatternMode )
	EVENT( EV_StompAttack,				rvMonsterBossMakron::Event_StompAttack )
	EVENT( EV_Separate,					rvMonsterBossMakron::Event_Separate )
	EVENT( EV_FlyingRotate,				rvMonsterBossMakron::Event_FlyingRotate )
	EVENT( AI_ScriptedFace,				rvMonsterBossMakron::ScriptedFace )
	EVENT( EV_ToggleCornerState,		rvMonsterBossMakron::Event_ToggleCornerState )
END_CLASS

/*
================
rvMonsterBossMakron::rvMonsterBossMakron
================
*/
rvMonsterBossMakron::rvMonsterBossMakron ( void ) {

	//set up this action array



}

/*
================
rvMonsterBossMakron::BuildActionArray ( void )
================
*/
void rvMonsterBossMakron::BuildActionArray ( void ) {

	actionArray[ MAKRON_ACTION_DMG ] = &actionDMGAttack;
	actionArray[ MAKRON_ACTION_MELEE ] = &actionMeleeAttack;
	actionArray[ MAKRON_ACTION_CANNON ] = &actionCannonAttack;
	actionArray[ MAKRON_ACTION_CANNON_SWEEP ] = &actionCannonSweepAttack;
	actionArray[ MAKRON_ACTION_GRENADE ] = &actionGrenadeAttack;
	actionArray[ MAKRON_ACTION_LIGHTNING_1 ] = &actionLightningPattern1Attack;
	actionArray[ MAKRON_ACTION_LIGHTNING_2 ] = &actionLightningPattern2Attack;
	actionArray[ MAKRON_ACTION_STOMP ] = &actionStompAttack;
	actionArray[ MAKRON_ACTION_HEAL ] = &actionHeal;
	actionArray[ MAKRON_ACTION_CHARGE ] = &actionCharge;
	actionArray[ MAKRON_ACTION_KILLPLAYER ] = &actionKillPlayer;

}
/*
================
rvMonsterBossMakron::Save
================
*/
void rvMonsterBossMakron::Save ( idSaveGame *savefile ) const {

	savefile->WriteInt( shots);

	savefile->WriteFloat( facingIdealYaw);
	savefile->WriteFloat( facingTime);

	savefile->WriteBool( noIdle);

	savefile->WriteBool( patternedMode);

	savefile->WriteBool( flagFlyingMode);
	savefile->WriteBool( flagFakeDeath);
	savefile->WriteInt( flagUndying );

	effectHover.Save( savefile );
	savefile->WriteJoint( jointHoverEffect ); // cnicholson: added unsaved var

	savefile->WriteFloat( stompRadius);

	leftBoltEffect.Save( savefile );
	rightBoltEffect.Save( savefile );
	leftBoltImpact.Save( savefile );
	rightBoltImpact.Save( savefile );
	boltMuzzleFlash.Save( savefile );

	savefile->WriteString( boltEffectName);

	savefile->WriteVec3( leftBoltVector);
	savefile->WriteVec3( rightBoltVector);
	savefile->WriteVec3( boltVectorMin);
	savefile->WriteVec3( boltVectorMax);
	savefile->WriteMat3( boltAimMatrix);	

	savefile->WriteBool( flagSweepDone);

	savefile->WriteFloat( boltSweepTime);

	savefile->WriteFloat( boltSweepStartTime);
	savefile->WriteFloat( boltTime);
	savefile->WriteFloat( boltNextStateTime);

	savefile->WriteInt( stateBoltSweep);

	savefile->WriteBool( flagAllowSpawns);

	savefile->WriteBool( flagCornerState );
	savefile->WriteInt( cornerCount );
	savefile->WriteBool( flagTeleporting );


	actionDMGAttack.Save( savefile );
	actionMeleeAttack.Save( savefile );
	actionCannonAttack.Save( savefile );
	actionCannonSweepAttack.Save( savefile );
	actionGrenadeAttack.Save( savefile );
	actionLightningPattern1Attack.Save( savefile );
	actionLightningPattern2Attack.Save( savefile );
	actionStompAttack.Save( savefile );
	actionHeal.Save( savefile );
	actionCharge.Save( savefile );
	actionKillPlayer.Save( savefile );

	savefile->WriteInt( actionPatterned);

	scriptRecharge.Save( savefile );
	scriptTeleport.Save( savefile );
	// cnicholson: No need to save actionArry, its rebuilt during restore
}

/*
================
rvMonsterBossMakron::Restore
================
*/
void rvMonsterBossMakron::Restore ( idRestoreGame *savefile )  {

	savefile->ReadInt( shots);

	savefile->ReadFloat( facingIdealYaw);
	savefile->ReadFloat( facingTime);

	savefile->ReadBool( noIdle);

	savefile->ReadBool( patternedMode);

	savefile->ReadBool( flagFlyingMode);
	savefile->ReadBool( flagFakeDeath);
	savefile->ReadInt( flagUndying );

	effectHover.Restore( savefile );
	savefile->ReadJoint( jointHoverEffect ); // cnicholson: added unrestoed var

	savefile->ReadFloat( stompRadius);

	leftBoltEffect.Restore( savefile );
	rightBoltEffect.Restore( savefile );
	leftBoltImpact.Restore( savefile );
	rightBoltImpact.Restore( savefile );
	boltMuzzleFlash.Restore( savefile );

	savefile->ReadString( boltEffectName);

	savefile->ReadVec3( leftBoltVector);
	savefile->ReadVec3( rightBoltVector);
	savefile->ReadVec3( boltVectorMin);
	savefile->ReadVec3( boltVectorMax);
	savefile->ReadMat3( boltAimMatrix);

	savefile->ReadBool( flagSweepDone);

	savefile->ReadFloat( boltSweepTime);

	savefile->ReadFloat( boltSweepStartTime);
	savefile->ReadFloat( boltTime);
	savefile->ReadFloat( boltNextStateTime);

	savefile->ReadInt( stateBoltSweep);

	savefile->ReadBool( flagAllowSpawns);

	savefile->ReadBool( flagCornerState );
	savefile->ReadInt( cornerCount );
	savefile->ReadBool( flagTeleporting );


	actionDMGAttack.Restore( savefile );
	actionMeleeAttack.Restore( savefile );
	actionCannonAttack.Restore( savefile );
	actionCannonSweepAttack.Restore( savefile );
	actionGrenadeAttack.Restore( savefile );
	actionLightningPattern1Attack.Restore( savefile );
	actionLightningPattern2Attack.Restore( savefile );
	actionStompAttack.Restore( savefile );
	actionHeal.Restore( savefile );
	actionCharge.Restore( savefile );
	actionKillPlayer.Restore( savefile );


	savefile->ReadInt( actionPatterned);

	scriptRecharge.Restore( savefile );
	scriptTeleport.Restore( savefile );
	//reload the action array, this is done in the init section but if we don't do it here our array is bunk.
	BuildActionArray();
	InitSpawnArgsVariables();

	// pre-cache decls
	gameLocal.FindEntityDefDict ( "monster_makron_legs" );
}


/*
================
rvMonsterBossMakron::StopAllEffects
================
*/
void rvMonsterBossMakron::StopAllEffects ( void ) {

	StopAllBoltEffects();

	//stop the over effect
	if( effectHover )	{
		effectHover.GetEntity()->Stop();
		effectHover = 0;
	}

}

/*
================
rvMonsterBossMakron::StopAllBoltEffects
================
*/
void rvMonsterBossMakron::StopAllBoltEffects ( void ) {
	
	if( leftBoltEffect )	{
		leftBoltEffect.GetEntity()->Stop();
		leftBoltEffect = 0;
	}

	if( rightBoltEffect )	{
		rightBoltEffect.GetEntity()->Stop();
		rightBoltEffect = 0;
	}
	
	if( leftBoltImpact )	{
		leftBoltImpact.GetEntity()->Stop();
		leftBoltImpact = 0;
	}

	if( rightBoltImpact )	{
		rightBoltImpact.GetEntity()->Stop();
		rightBoltImpact = 0;
	}

	if( boltMuzzleFlash )	{
		boltMuzzleFlash .GetEntity()->Stop();
		boltMuzzleFlash  = 0;
	}


}

void rvMonsterBossMakron::InitSpawnArgsVariables ( void ) {
	//slick!
	jointLightningBolt = animator.GetJointHandle ( spawnArgs.GetString ( "joint_lightningBolt", "claw_muzzle" ) );

	stompMaxRadius = spawnArgs.GetFloat( "stomp_max_range", "1600");
	stompWidth = spawnArgs.GetFloat( "stomp_width", "32");
	stompSpeed = spawnArgs.GetFloat( "stomp_speed", "32");

	boltWaitTime = spawnArgs.GetFloat( "lightingsweep_wait_time", "1");

	turnRate = spawnArgs.GetFloat( "fly_turnRate", "180");
}

/*
================
rvMonsterBossMakron::Spawn
================
*/
void rvMonsterBossMakron::Spawn ( void ) {

	if( spawnArgs.GetBool("passive"))	{
		flagFakeDeath = true;
	} else {
		flagFakeDeath = false;
	}	

	if( spawnArgs.GetBool("furniture") ) {
		fl.takedamage = false;
	}
	
	if( spawnArgs.GetBool("junior")	)	{
		flagUndying = 1;
	} else {
		flagUndying = 0;
	}

	flagAllowSpawns = false;
	flagFlyingMode = false;
	patternedMode = false;
	noIdle = false;

	flagCornerState = false;
	cornerCount = 0;
	flagTeleporting = false;
	
	//start clean
	actionPatterned = -1;

	boltTime = 0;
	boltSweepTime = spawnArgs.GetFloat( "lightningsweep_sweep_time", "3");

	InitSpawnArgsVariables();

	actionDMGAttack.Init ( spawnArgs,		"action_DMGAttack",				"Torso_DMGAttack",			AIACTIONF_ATTACK );
	actionCannonAttack.Init ( spawnArgs,	"action_cannonAttack",			"Torso_CannonAttack",		AIACTIONF_ATTACK );
	actionCannonSweepAttack.Init ( spawnArgs,	"action_cannonsweepAttack",		"Torso_CannonSweepAttack",	AIACTIONF_ATTACK );
	actionGrenadeAttack.Init ( spawnArgs,		"action_grenadeAttack",			"Torso_GrenadeAttack",		AIACTIONF_ATTACK );
	actionLightningPattern1Attack.Init( spawnArgs,	"action_lightningPattern1Attack",		"Torso_Lightning1Attack",						AIACTIONF_ATTACK );
 	actionLightningPattern2Attack.Init( spawnArgs,	"action_lightningPattern2Attack",		"Torso_Lightning2Attack",						AIACTIONF_ATTACK );
	actionStompAttack.Init ( spawnArgs,		"action_stompAttack",			"Torso_StompAttack",		AIACTIONF_ATTACK );
	actionHeal.Init( spawnArgs,			"action_heal",						"Torso_Recharge",			AIACTIONF_ATTACK );
	actionCharge.Init( spawnArgs,			"action_charge",						"Torso_Charge",			AIACTIONF_ATTACK );
	actionKillPlayer.Init( spawnArgs,		"action_lightningPattern3Attack",				"Torso_KillPlayer", AIACTIONF_ATTACK );

	actionMeleeAttack.Init ( spawnArgs,		"action_meleeAttack",			"Torso_MeleeAttack",						AIACTIONF_ATTACK );

	const char  *func;
	if ( spawnArgs.GetString( "script_recharge", "", &func ) ) 
	{
		scriptRecharge.Init( func );
	}
	if ( spawnArgs.GetString( "script_teleport", "", &func ) ) 
	{
		scriptTeleport.Init( func );
	}

	//build the action array
	BuildActionArray();

	// pre-cache decls
	gameLocal.FindEntityDefDict ( "monster_makron_legs" );
}

/*
================
rvMonsterBossMakron::CheckTurnActions
================
*/
bool rvMonsterBossMakron::CheckTurnActions ( void ) {

	if ( !flagFakeDeath && !move.fl.moving && !move.anim_turn_angles ) { //&& gameLocal.time > combat.investigateTime ) {
		float turnYaw = idMath::AngleNormalize180 ( move.ideal_yaw - move.current_yaw ) ;
		if ( turnYaw > lookMax[YAW] * 0.75f ) {
			PerformAction ( "Torso_TurnRight90", 4, true );
			return true;
		} else if ( turnYaw < -lookMax[YAW] * 0.75f ) {
			PerformAction ( "Torso_TurnLeft90", 4, true );
			return true;
		}
		////gameLocal.Printf(" turn yaw == %f \n", turnYaw);
	}
	return false;
}
/*
================
rvMonsterBossMakron::CheckActions
================
*/
bool rvMonsterBossMakron::CheckActions ( void ) {

	if( spawnArgs.GetFloat("furniture", "0"))	{
		return true;
	}

	//if in the middle of teleporting, do nothing
	if( flagTeleporting )	{
		return true;
	}

	//gameLocal.Printf("Begin CheckActions \n");		

	if( CheckTurnActions() )	{
		//gameLocal.Printf("---CheckActions: TurnActions was true \n");		
		return true;
	}


	//If we need more baddies, do it right now regardless of range or player FOV
	if ( flagAllowSpawns )	{
		PerformAction ( actionGrenadeAttack.state,actionGrenadeAttack.blendFrames, actionGrenadeAttack.fl.noPain );
		//gameLocal.Printf("---CheckActions: Spawned in more fellas \n");		
		return true;
	}

	//melee attacks
	if ( PerformAction ( &actionMeleeAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack, NULL )  || 
		 PerformAction ( &actionStompAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack, NULL ) 	) {
			 //gameLocal.Printf("---CheckActions: Melee attacks \n");		
		return true;
	}

	//cannon sweep is good to check if we're up close. 
	//if ( PerformAction ( &actionCannonSweepAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) )	{
	//	return true;
	//}

	//If we're in total patterned mode, then don't relay on timers or ranges to perform actions. 
	if ( patternedMode && !flagFakeDeath	)	{
		//gameLocal.Printf("**CheckActions: PerformPatternedAction called \n");		
		PerformPatternedAction ( );
		return true;
	}
	//gameLocal.Printf("---CheckActions: FlagFakeDeath: %d \n", flagFakeDeath );		
	//gameLocal.Printf("---CheckActions: PatternedMode: %d \n", patternedMode );		

	//ranged attacks
	if ( PerformAction ( &actionDMGAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ||
		 PerformAction ( &actionLightningPattern1Attack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ||
		 PerformAction ( &actionGrenadeAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ||
		 PerformAction ( &actionCannonAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
			 //gameLocal.Printf("---CheckActions: Ranged attack called \n");		
		return true;
	}
		 //gameLocal.Printf("---CheckActions: Defaulting to AI CheckActions \n");		
	return idAI::CheckActions ( );
}

/*
================
rvMonsterMakron::PerformPatternedAction
================
*/
void rvMonsterBossMakron::PerformPatternedAction( )	{

		//gameLocal.Printf("PerformPatternedAction::Begin--\n");

	//if we don't have an action to perform, don't.
	if( actionPatterned == -1)	{
		//gameLocal.Printf("actionPatterned = -1\n");
		//gameLocal.Printf("PerformPatternedAction::End--\n");
		return;
	}	

	//if we're in a corner, right now, then add to the corner tally.
	if( flagCornerState )	{
		cornerCount++;
	} else	{
		cornerCount = 0;
	}
	
	//if this is our third consecutive action in the corner, maybe we should teleport?
	if( cornerCount == 3 )	{
		gameLocal.Warning("Makron is stuck in a corner!");
		cornerCount = 0;
		SetState( "Torso_Teleport" );
		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_FinishAction", 0, 0, SFLAG_ONCLEAR );
		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 0, 0 );
		PostAnimState ( ANIMCHANNEL_LEGS, "Torso_Idle", 0, 0 );
		PostState( "State_Combat" );
		return;
	
	}

	rvAIAction* action;
	rvAIActionTimer* timer;

	action = actionArray[ actionPatterned];
	timer  = &actionTimerRangedAttack;

	//if the attack is variable length, then do all this jive...
	float scale;
	scale = 1.0f;

	//gameLocal.Printf("performing the action\n");

	// Perform the raw action
	PerformAction ( action->state, action->blendFrames, action->fl.noPain );

	//Clear this out so the next attack can be queued up.
	actionPatterned = -1;

	// Restart the action timer using the length of the animation being played
	// Even though we don't use the action timer for this instance, we need to keep it updated and fresh.
	action->timer.Reset ( actionTime, action->diversity, scale );

	// Restart the global action timer using the length of the animation being played
	if ( timer ) {
		timer->Reset ( actionTime, action->diversity, scale );
	}

}


/*
================
rvMonsterMakron::CanTurn
================
*/
bool rvMonsterBossMakron::CanTurn ( void ) const {
	if( !flagFlyingMode )	{
		if ( !idAI::CanTurn ( ) ) {
			return false;
		}
		return move.anim_turn_angles != 0.0f || move.fl.moving;
	} else {
		return idAI::CanTurn ( ); 
	}
}


/*
================
rvMonsterMakron::InitBoltSweep
================
*/
void rvMonsterBossMakron::InitBoltSweep ( idVec3 idealTarget )	{

	idVec3		origin;
	idMat3		axis;
	trace_t		tr;
	
	idVec3		fwd;
	idVec3		right;
	idVec3		worldup(0,0,1);

	idVec3		targetPoint;

	//init all the lightning bolt stuff
	boltTime = 0;
	flagSweepDone = false;
	stateBoltSweep = 0;

	//get the vector from makron's gun to the target point.
	GetJointWorldTransform( jointLightningBolt, gameLocal.time, origin, axis);
	targetPoint = idealTarget; //enemy.lastVisibleChestPosition;

	fwd = targetPoint - origin;

	fwd.Normalize();
	right = fwd.Cross( worldup);

	boltAimMatrix[0] = fwd;
	boltAimMatrix[1] = right;
	boltAimMatrix[2] = worldup;

	targetPoint = origin + (fwd * 2400);

	//left
	targetPoint = targetPoint - (right * 1800);
	boltVectorMin = targetPoint - origin;
	boltVectorMin.Normalize();
	leftBoltVector = boltVectorMin;
	
	//right
	targetPoint = targetPoint + (right * 1800 * 2);
	boltVectorMax = targetPoint - origin;
	boltVectorMax.Normalize();
	rightBoltVector = boltVectorMax;			

}

/*
================
rvMonsterMakron::MaintainBoltSweep
================
*/
void rvMonsterBossMakron::MaintainBoltSweep	( void )	{
	enum	{
		STATE_START,
		STATE_WAIT1,
		STATE_SWEEP1,
		STATE_WAIT2,
		STATE_SWEEP2,
		STATE_END,
	};

	//advance the state when time is up.

	switch( stateBoltSweep )	{
	
		//init vars
		case STATE_START:
			flagSweepDone = false;
			boltNextStateTime = gameLocal.time + SEC2MS( boltWaitTime);
			StopAllBoltEffects();
			stateBoltSweep = STATE_WAIT1;
		return;

		//blast at the waiting points, vectors do not move
		case STATE_WAIT1:
			LightningSweep( leftBoltVector, leftBoltEffect, leftBoltImpact);
			LightningSweep( rightBoltVector, rightBoltEffect, rightBoltImpact );

			//check for state advance
			if( gameLocal.time > boltNextStateTime)	{
				stateBoltSweep++;
				boltNextStateTime = gameLocal.time + SEC2MS( boltSweepTime); 
				boltTime = 0;
				boltSweepStartTime = gameLocal.time;
			}
		return;
		
		case STATE_SWEEP1:
			//lerp the lightning bolt vectors
			boltTime = gameLocal.time - boltSweepStartTime;
			leftBoltVector.Lerp( boltVectorMin, boltVectorMax, boltTime / SEC2MS(boltSweepTime));
			rightBoltVector.Lerp( boltVectorMax, boltVectorMin, boltTime / SEC2MS(boltSweepTime));

			//sweep			
			LightningSweep( leftBoltVector, leftBoltEffect, leftBoltImpact);
			LightningSweep( rightBoltVector, rightBoltEffect, rightBoltImpact );

			//check for state advance
			if( gameLocal.time > boltNextStateTime)	{
				stateBoltSweep++;
				boltNextStateTime = gameLocal.time + SEC2MS( boltWaitTime); 
			}

		return;
		case STATE_WAIT2:
			LightningSweep( leftBoltVector, leftBoltEffect, leftBoltImpact);
			LightningSweep( rightBoltVector, rightBoltEffect, rightBoltImpact );

			//check for state advance
			if( gameLocal.time > boltNextStateTime)	{
				stateBoltSweep++;
				boltNextStateTime = gameLocal.time + SEC2MS( boltSweepTime); 
				boltTime = 0;
				boltSweepStartTime = gameLocal.time;
			}
		return;
		case STATE_SWEEP2:
			//lerp the lightning bolt vectors
			boltTime = gameLocal.time - boltSweepStartTime;
			//min and max are reversed here.
			leftBoltVector.Lerp( boltVectorMax, boltVectorMin, boltTime / SEC2MS(boltSweepTime));
			rightBoltVector.Lerp( boltVectorMin, boltVectorMax, boltTime / SEC2MS(boltSweepTime));

			//sweep			
			LightningSweep( leftBoltVector, leftBoltEffect, leftBoltImpact);
			LightningSweep( rightBoltVector, rightBoltEffect, rightBoltImpact );

			//check for state advance
			if( gameLocal.time > boltNextStateTime)	{
				stateBoltSweep++;
				boltNextStateTime = 0;
				StopAllBoltEffects();
			}
		return;
		case STATE_END:
			flagSweepDone = 1;
		return;
	}

}
/*
================
rvMonsterBossMakron::LightningSweep
================
*/
void rvMonsterBossMakron::LightningSweep ( idVec3 attackVector, rvClientEffectPtr& boltEffect, rvClientEffectPtr& impactEffect )	{

	idVec3	origin;
	idMat3	axis;
	trace_t	tr;

	GetJointWorldTransform( jointLightningBolt, gameLocal.time, origin, axis);
	
	//trace out from origin along attackVector
	attackVector.Normalize();
	gameLocal.TracePoint( this, tr, origin, origin + (attackVector * 25600), MASK_SHOT_RENDERMODEL, this);
	//gameRenderWorld->DebugLine( colorRed, origin, tr.c.point, 100, true);
	
	if ( !boltEffect ) {
		boltEffect =  gameLocal.PlayEffect ( gameLocal.GetEffect ( this->spawnArgs, "fx_sweep_fly" ), origin, attackVector.ToMat3(), true, tr.c.point);
	} else {
		boltEffect->SetOrigin ( origin );
		boltEffect->SetAxis ( attackVector.ToMat3() );
		boltEffect->SetEndOrigin ( tr.c.point );
	}	

	if ( !impactEffect ) {
		impactEffect =  gameLocal.PlayEffect ( gameLocal.GetEffect ( this->spawnArgs, "fx_sweep_impact" ), tr.c.point, tr.c.normal.ToMat3(), true, tr.c.point);
	} else {
		impactEffect->SetOrigin ( tr.c.point );
		impactEffect->SetAxis (  tr.c.normal.ToMat3() );
		impactEffect->SetEndOrigin ( tr.c.point );
	}	

	if ( !boltMuzzleFlash )	{
		boltMuzzleFlash =  gameLocal.PlayEffect ( gameLocal.GetEffect ( this->spawnArgs, "fx_sweep_muzzle" ), origin, attackVector.ToMat3(), true, origin);
	} else {
		boltMuzzleFlash->SetOrigin ( origin );
		boltMuzzleFlash->SetAxis ( attackVector.ToMat3() );
		boltMuzzleFlash->SetEndOrigin ( origin );
	}	

	//hurt anything in the way
	idEntity* ent = gameLocal.entities[tr.c.entityNum];

	if( ent)	{
		ent->Damage ( this, this, attackVector, spawnArgs.GetString ( "def_makron_sweep_damage" ), 1.0f, 0 );
	}
	
}

/*
================
rvMonsterBossMakron::PerformAction
================
*/
void rvMonsterBossMakron::PerformAction ( const char* stateName, int blendFrames, bool noPain ) {
	// Allow movement in actions
	move.fl.allowAnimMove = true;

	// Start the action
	SetAnimState ( ANIMCHANNEL_TORSO, stateName, blendFrames );

	// Always call finish action when the action is done, it will clear the action flag
	aifl.action = true;
	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_FinishAction", 0, 0, SFLAG_ONCLEAR );

	// Go back to idle when done-- sometimes.
	if ( !noIdle )	{	
		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", blendFrames );
	}

	// Main state will wait until action is finished before continuing
	InterruptState ( "Wait_ActionNoPain" );

	OnStartAction ( );
}

/*
================
rvMonsterBossMakron::PerformAction
================
*/
bool rvMonsterBossMakron::PerformAction( rvAIAction* action, bool (idAI::*condition)(rvAIAction*,int), rvAIActionTimer* timer ) {
	return idAI::PerformAction( action, condition ,timer );
}

/*
================
rvMonsterBossMakron::PerformAction
================
*/
void rvMonsterBossMakron::OnStopAction( void )	{
	////gameLocal.Printf("\n\nAction stopped ----------- \n");
}

/*
================
rvMonsterBossMakron::Damage
================
*/
void rvMonsterBossMakron::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
								const char *damageDefName, const float damageScale, const int location ) {
	
	//Deal damage here,
	idAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );

	//if the Makron has 0 or less health, make sure he's killed
	///if ( health <= 0 && !flagFlyingMode && !flagFakeDeath )	{
	//	Killed( this, this, 1, dir, location);
	//}
}
/*
================
rvMonsterBossMakron::Killed
================
*/
void rvMonsterBossMakron::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location )	{

	//if this is the undying Makron Jr, don't worry about death. Stop what we're doing,
	//Call the script function and let it ride.
	if( flagUndying	 == 1)	{
		flagUndying = 2;
		ExecScriptFunction( funcs.death );
		StopAnimState ( ANIMCHANNEL_TORSO );
		StopAnimState ( ANIMCHANNEL_LEGS );
		PerformAction ( actionKillPlayer.state, actionKillPlayer.blendFrames, actionKillPlayer.fl.noPain );
		return;
	}

	if( flagUndying == 2)	{
		return;
	}

	//stop all effects.
	StopAllEffects();

	//if the makron isn't in flying mode, it is now.
	if( flagFlyingMode )	{
		//play the falling animation, then die.
		StopAnimState ( ANIMCHANNEL_TORSO );
		StopAnimState ( ANIMCHANNEL_LEGS );
		//SetState( "Torso_Death_Fall" );
		idAI::Killed( inflictor, attacker, damage, dir, location);
		return;
	}
	
	//the Makron is in undying mode until he finishes getting up.
	//gameLocal.Warning("First form defeated!");

	health = 1;
	aifl.undying = true;
	fl.takedamage = false;
	SetMoveType ( MOVETYPE_DEAD );
	StopMove( MOVE_STATUS_DONE );
	//gameLocal.Printf("************\n************\n************\n************\n---flagFakeDeath set to TRUE! ************\n************\n************\n************\n" );		
	flagFakeDeath = true;



	//play the death anim
	StopAnimState ( ANIMCHANNEL_TORSO );
	StopAnimState ( ANIMCHANNEL_LEGS );
	if ( head ) {
		StopAnimState ( ANIMCHANNEL_HEAD );
	}

	//Call this anyway-- hopefully the script is prepared to handle multiple Makron deaths... 
	ExecScriptFunction( funcs.death );

	//make sure he goes through deadness, but we need to post FinishAction afterwards
//	SetState( "Torso_FirstDeath" );
//	aifl.action = true;
	SetState( "Torso_FirstDeath" );
//	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_FinishAction", 0, 0, SFLAG_ONCLEAR );
//	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_FirstDeath", 30, 0, SFLAG_ONCLEAR );

}


/*
================
rvMonsterBossMakron::BeginSeparation( void )
================
*/
void rvMonsterBossMakron::BeginSeparation( void )	{

	//spawn in a new Makron leg,			
	idDict	  args;
	idEntity*  newLegs;

	//We may need to do this at a later point
	//SetSkin ( declManager->FindSkin	 ( spawnArgs.GetString ( "skin_legs" ) ) );

	args.Copy ( *gameLocal.FindEntityDefDict ( "monster_makron_legs" ) );
	args.SetVector ( "origin", GetPhysics()->GetOrigin() );
	args.SetInt ( "angle", move.current_yaw );
	gameLocal.SpawnEntityDef ( args, &newLegs );

	//store the name of the entity in the Makron's keys so we can burn it out as well.
	spawnArgs.Set( "legs_name", newLegs->GetName() );


}

/*
================
rvMonsterBossMakron::CompleteSeparation( void )
================
*/
void rvMonsterBossMakron::CompleteSeparation( void )	{

		//flying mode now
		flagFlyingMode = true;
		SetMoveType ( MOVETYPE_FLY );
		move.fl.noGravity = true;
		animPrefix = "fly";

		//is there a cooler way to do this? Heal over time?
		//health = spawnArgs.GetFloat("health_flying", "5000" );
		
		ExecScriptFunction( funcs.init);

		//makron is once again pwnable.		
		aifl.undying = false;
		fl.takedamage = true;
		flagFakeDeath = false;
		patternedMode = true;

		//make sure he cleans up.
//		PostAnimState ( ANIMCHANNEL_ALL, "Torso_FinishAction", 0, 0, SFLAG_ONCLEAR );
//		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_FinishAction", 0, 0, SFLAG_ONCLEAR );
//		PostAnimState ( ANIMCHANNEL_LEGS, "Torso_FinishAction", 0, 0, SFLAG_ONCLEAR );
		//gameLocal.Printf("*****\n*****\n*****\nPast 'PostAnimState' flagFakeDeath is: %d \n*****\n*****\n*****\n", flagFakeDeath);
		

//		These four lines work!
		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_FinishAction", 0, 0, SFLAG_ONCLEAR );
		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 0, 0 );
		PostAnimState ( ANIMCHANNEL_LEGS, "Torso_Idle", 0, 0 );
		PostState( "State_Combat" );


}

/*
================
rvMonsterBossMakron::ScriptedFace( idEntity* faceEnt, bool endWithIdle )
================
*/
/*
void rvMonsterBossMakron::ScriptedFace ( idEntity* faceEnt, bool endWithIdle ) {
	
	//set the ideal yaw, the change to the facing state.
	FaceEntity( faceEnt );

	//store the ideal yaw, because somehow between here and the state we set it gets stomped.
	facingIdealYaw = move.ideal_yaw;

	//become scripted
	aifl.scripted = true;

	//This will get us close to facing the entity correctly.
	SetState( "State_ScriptedFace", SFLAG_ONCLEAR);

}
*/

/*
===============================================================================

	Events 

===============================================================================
*/

/*
================
rvMonsterBossMakron::Event_AllowMoreSpawns
================
*/
// this will allow Makron to spawn more baddies.
void rvMonsterBossMakron::Event_AllowMoreSpawns( void )	{
	flagAllowSpawns = true;
}

/*
================
rvMonsterBossMakron::Event_EnablePatternMode
================
*/
// When set, the Makron will now only fight via scripted patterns
void rvMonsterBossMakron::Event_EnablePatternMode( void )	{
	patternedMode = true;
	noIdle = true;
	flagTeleporting = false;
}

/*
================
rvMonsterBossMakron::Event_DisablePatternMode
================
*/
void rvMonsterBossMakron::Event_DisablePatternMode( void )	{
	patternedMode = false;
	noIdle = false;
}

/*
================
rvMonsterBossMakron::Event_Separate
================
*/
void rvMonsterBossMakron::Event_Separate( void )	{

	//all we need to do here is post the separation state, right?
	BeginSeparation();
	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Resurrection", 0, 5000, SFLAG_ONCLEAR );

}

/*
================
rvMonsterBossMakron::Event_Separate
================
*/
void rvMonsterBossMakron::Event_FlyingRotate( idVec3& vecOrg )	{

	//set move.ideal_yaw
	TurnToward( vecOrg );
	
	//copy it over
	facingIdealYaw = move.ideal_yaw;
	aifl.scripted = true;

	//set the state
	SetState( "Torso_RotateToAngle");

}

/*
================
rvMonsterBossMakron::Event_SetNextAction
================
*/
void rvMonsterBossMakron::Event_SetNextAction( const char * actionString)	{
	
	//if the next action is occupied, return false
	if( actionPatterned != -1)	{
		idThread::ReturnFloat(0);
		return;
	}

	//otherwise, select the action from a list
	if( !idStr::Cmp( actionString, "actionCannon"))	{
		actionPatterned = MAKRON_ACTION_CANNON;
	}
	else if( !idStr::Cmp( actionString, "actionCannonSweep"))	{
		actionPatterned = MAKRON_ACTION_CANNON_SWEEP;
	}
	else if( !idStr::Cmp( actionString, "actionDMG"))	{
		actionPatterned = MAKRON_ACTION_DMG;
	}
	else if( !idStr::Cmp( actionString, "actionDMGrenades"))	{
		actionPatterned = MAKRON_ACTION_GRENADE;
	}
	else if( !idStr::Cmp( actionString, "actionLightningSweep1"))	{
		actionPatterned = MAKRON_ACTION_LIGHTNING_1;
	}
	else if( !idStr::Cmp( actionString, "actionLightningSweep2"))	{
		actionPatterned = MAKRON_ACTION_LIGHTNING_2;
	}
	else if( !idStr::Cmp( actionString, "actionStomp"))	{
		actionPatterned = MAKRON_ACTION_STOMP;
	}
	else if( !idStr::Cmp( actionString, "actionHeal"))	{
		actionPatterned = MAKRON_ACTION_HEAL;
	}
	else if( !idStr::Cmp( actionString, "actionCharge"))	{
		actionPatterned = MAKRON_ACTION_CHARGE;
	}
	else if( !idStr::Cmp( actionString, "actionKillPlayer"))	{
		actionPatterned = MAKRON_ACTION_KILLPLAYER;
	}
	else if( !idStr::Cmp( actionString, "actionEndPattern"))	{
		actionPatterned = -1;
	}
	else	{
		gameLocal.Error(" Bad action %s passed into MonsterMakron::SetNextAction", actionString);
		idThread::ReturnFloat(0);
		return;
	}

	idThread::ReturnFloat(1);
	return;



}


/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterBossMakron )
	STATE ( "Torso_DMGAttack",		rvMonsterBossMakron::State_Torso_DMGAttack )
	STATE ( "Torso_MeleeAttack",		rvMonsterBossMakron::State_Torso_MeleeAttack )
	STATE ( "Torso_CannonAttack",		rvMonsterBossMakron::State_Torso_CannonAttack )
	STATE ( "Torso_GrenadeAttack",		rvMonsterBossMakron::State_Torso_GrenadeAttack )
	STATE ( "Torso_CannonSweepAttack",	rvMonsterBossMakron::State_Torso_CannonSweepAttack )
	STATE ( "Torso_Lightning1Attack",	rvMonsterBossMakron::State_Torso_Lightning1Attack )
	STATE ( "Torso_Lightning2Attack",	rvMonsterBossMakron::State_Torso_Lightning2Attack )
	STATE ( "Torso_StompAttack",		rvMonsterBossMakron::State_Torso_StompAttack )
	STATE ( "Torso_Recharge",			rvMonsterBossMakron::State_Torso_Recharge )
	STATE ( "Torso_Charge",				rvMonsterBossMakron::State_Torso_Charge )
	STATE ( "Torso_KillPlayer",			rvMonsterBossMakron::State_Torso_KillPlayer )
	STATE ( "Torso_FirstDeath",			rvMonsterBossMakron::State_Torso_FirstDeath )
	STATE ( "Torso_Resurrection",		rvMonsterBossMakron::State_Torso_Resurrection )
	STATE ( "State_Killed",				rvMonsterBossMakron::State_Killed )
	STATE ( "Torso_Teleport",			rvMonsterBossMakron::State_Torso_Teleport )

	STATE ( "Torso_TurnRight90",		rvMonsterBossMakron::State_Torso_TurnRight90 )
	STATE ( "Torso_TurnLeft90",			rvMonsterBossMakron::State_Torso_TurnLeft90 )

	STATE ( "Torso_RotateToAngle",		rvMonsterBossMakron::State_Torso_RotateToAngle )

	STATE ( "Frame_BeginLightningSweep2",	rvMonsterBossMakron::Frame_BeginLightningSweep2 )
	STATE ( "Frame_EndLightningSweep2",	rvMonsterBossMakron::Frame_EndLightningSweep2 )
	STATE ( "Frame_StompAttack",		rvMonsterBossMakron::Frame_StompAttack )
	STATE ( "Frame_Teleport",			rvMonsterBossMakron::Frame_Teleport )
	
	STATE ( "State_ScriptedFace",		rvMonsterBossMakron::State_ScriptedFace )


END_CLASS_STATES


/*
================
rvBossMakron::State_Torso_DMGAttack
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_DMGAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			//gameLocal.Warning("Makron DMG Go!");
			//fire the DMG
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_dmg", parms.blendFrames );
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
rvBossMakron::State_Torso_Charge
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_Charge ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "run", parms.blendFrames );
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
rvBossMakron::State_ScriptedFace
================
*/

stateResult_t rvMonsterBossMakron::State_ScriptedFace ( const stateParms_t& parms ) {

	//note this uses the Makron's version of FacingIdeal, 
	if( !flagFlyingMode)	{
		if ( !aifl.scripted || (!CheckTurnActions( ) && (!move.anim_turn_angles))) {
			return SRESULT_DONE;
		}
		
		return SRESULT_WAIT;

	} else {

		if ( !aifl.scripted || FacingIdeal() ) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;

	}
}

/*
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	idStr turnAnim;
	float turnYaw;

	switch( parms.stage )	{
		case STAGE_INIT:

			DisableAnimState ( ANIMCHANNEL_LEGS );
			//which way do we need to face?
			turnYaw = idMath::AngleNormalize180 ( facingIdealYaw - move.current_yaw ) ;
			if ( turnYaw > lookMax[YAW] * 0.75f ) {
				turnAnim = "turn_right_90";
			} else if ( turnYaw < -lookMax[YAW] * 0.75f ) {
				turnAnim = "turn_left_90";
			} else	{
				//guess we don't need to turn? We're done.
				aifl.scripted = false;
				return SRESULT_DONE;	
			}
			PlayAnim ( ANIMCHANNEL_TORSO, turnAnim, 4 );
			AnimTurn ( 90.0f, true );
			return SRESULT_WAIT;
	
		case STAGE_WAIT:
			if ( move.fl.moving || AnimDone ( ANIMCHANNEL_TORSO, 0 )) {
				AnimTurn ( 0, true );
				combat.investigateTime = gameLocal.time + 250;
				//back to the start to make sure we're facing the right way.
				return  SRESULT_STAGE ( STAGE_INIT );
			}
			return SRESULT_WAIT;
	}
	
	return SRESULT_ERROR; 

}
*/


/*
================
rvBossMakron::State_Torso_FirstDeath
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_FirstDeath ( const stateParms_t& parms ) {
	

	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			//force a long blend on this anim since it will be sudden
			PlayAnim ( ANIMCHANNEL_TORSO, "separation_start", 30 );
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
rvBossMakron::State_Torso_Resurrection
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_Resurrection ( const stateParms_t& parms ) {
/*	enum {
		STAGE_INIT,
		STAGE_WAIT_FIRST,
		STAGE_RISE,
		STAGE_WAIT_SECOND
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			//force a long blend on this anim since it will be sudden
			PlayAnim ( ANIMCHANNEL_TORSO, "separation_start", 30 );
			return SRESULT_STAGE ( STAGE_WAIT_FIRST);
		
		case STAGE_WAIT_FIRST:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_STAGE ( STAGE_RISE);
			}
			return SRESULT_WAIT;

		case STAGE_RISE:
			//hide the leg surface
			const idKeyValue* kv;
			kv = spawnArgs.MatchPrefix ( "surface_legs" );
			HideSurface ( kv->GetValue() );
			
			//start the effect
			jointHoverEffect = animator.GetJointHandle ( spawnArgs.GetString("joint_hover","thruster") );
			effectHover = PlayEffect ( "fx_hover", jointHoverEffect, true );
	
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "separation_rise", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT_SECOND);

		case STAGE_WAIT_SECOND:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				CompleteSeparation();
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;

	}
*/
	enum {
		STAGE_RISE,
		STAGE_WAIT,
	};

	switch( parms.stage )	{
		case STAGE_RISE:
			//hide the leg surface
			const idKeyValue* kv;
			kv = spawnArgs.MatchPrefix ( "surface_legs" );
			HideSurface ( kv->GetValue() );
			
			//start the effect
			jointHoverEffect = animator.GetJointHandle ( spawnArgs.GetString("joint_hover","thruster") );
			effectHover = PlayEffect ( "fx_hover", jointHoverEffect, true );
	
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "separation_rise", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT);

		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				CompleteSeparation();
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;

	}

	return SRESULT_ERROR;

}


/*
================
rvBossMakron::State_Lightning2Attack
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_Lightning2Attack ( const stateParms_t& parms ) {
	
	idVec3	boltVector;

	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};
	
	switch( parms.stage )	{
		case STAGE_INIT:

			//prep up for the bolt
			//gameLocal.Warning("Prepping sweep 2");
			//init these values
			StopAllBoltEffects();
			stateBoltSweep = 0;
			flagSweepDone = false;
			//set up bolt targeting at a little above the players chest-- make him duck this one.
			boltVector = enemy.lastVisibleEyePosition;
			boltVector.z += 8;
			InitBoltSweep( boltVector );

			//play the anim
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "claw_sweep", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT);
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_DONE;
			}
			if( stateBoltSweep == 1)	{
				//fire the bolt out from the claw based on where the claw is pointing, sort of.
				boltTime = gameLocal.time - boltSweepStartTime;
				leftBoltVector.Lerp( boltVectorMax, boltVectorMin, boltTime / SEC2MS(boltSweepTime));

				LightningSweep( leftBoltVector, leftBoltEffect, leftBoltImpact );	
			}
			else if( stateBoltSweep == 2)	{
				//gameLocal.Warning("Sweep 2 done.");
				StopAllBoltEffects();
				stateBoltSweep = 0;
			}
			return SRESULT_WAIT;	

	}

	return SRESULT_ERROR;

}


/*
================
rvMonsterBossMakron::State_Torso_StompAttack
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_StompAttack( const stateParms_t& parms )	{

	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			//can't do the stomp attack while flying-- ain't got no legs!!
			if ( flagFlyingMode )	{
				return SRESULT_DONE;
			}
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "shockwave_stomp", parms.blendFrames );
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
rvMonsterBossMakron::State_Torso_Teleport
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_Teleport( const stateParms_t& parms )	{

	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			//No teleporting in flying mode.
			if ( flagFlyingMode )	{
				return SRESULT_DONE;
			}
			DisableAnimState ( ANIMCHANNEL_LEGS );
			//can't take damage in teleport anim, bad things happen!
			aifl.undying = true;
			PlayAnim ( ANIMCHANNEL_TORSO, "teleport_stomp", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT);

		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {

				//restore former state, which is killable unless some other effect renders Makron unkillable.
				if( !flagUndying )	{
					aifl.undying = false;
				}
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;	

	}

	return SRESULT_ERROR;



}

/*
================
rvBossMakron::State_Torso_GrenadeAttack
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_GrenadeAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			//gameLocal.Warning("Grenades!");
			//fire the DMG
			DisableAnimState ( ANIMCHANNEL_LEGS );
			//only allow spawns if the script tells us we can
			if( !flagAllowSpawns)	{
				PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_grenade_dm", parms.blendFrames );
			}
			else	{
				flagAllowSpawns = false;
				PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_grenade_spawn", parms.blendFrames );
			}
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
rvBossMakron::State_Torso_Recharge
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_Recharge ( const stateParms_t& parms ) {
	
	ExecScriptFunction( scriptRecharge );
	return SRESULT_DONE;
}

/*
================
rvBossMakron::State_Torso_MeleeAttack
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_MeleeAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			//swing!
			//gameLocal.Warning("Makron Melee Attack");
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "melee_attack", parms.blendFrames );
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
rvMonsterBossMakron::State_Torso_RotateToAngle
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_RotateToAngle ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_WAIT_LOOP,
	};
	
	float facingTimeDelta;
	float turnYaw;

	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			turnYaw = idMath::AngleNormalize180 ( facingIdealYaw - move.current_yaw ) ;
			if( turnYaw > 1.0f || turnYaw < -1.0f)	{
				facingTime = MS2SEC( gameLocal.time);
				aifl.scripted = true;
				return SRESULT_STAGE ( STAGE_WAIT_LOOP );
			}
			aifl.scripted = false;
			return SRESULT_DONE;
		case STAGE_WAIT_LOOP:
			turnYaw = idMath::AngleNormalize180 ( facingIdealYaw - move.current_yaw ) ;
			if( turnYaw > 1.0f || turnYaw < -1.0f)	{
				facingTimeDelta = MS2SEC( gameLocal.time) - facingTime;
				idAngles ang = GetPhysics()->GetAxis().ToAngles();
				ang.yaw += ( turnRate * facingTimeDelta );
				SetAngles( ang);
				move.current_yaw = ang.yaw;
				return SRESULT_STAGE( STAGE_WAIT_LOOP);	
			}
			aifl.scripted = false;
			return SRESULT_DONE;
	}

	return SRESULT_ERROR;
}
/*
================
rvMonsterBossMakron::State_Torso_TurnRight90
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_TurnRight90 ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "turn_right_90", parms.blendFrames );
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

/*
================
rvMonsterBossMakron::State_Torso_TurnLeft90
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_TurnLeft90 ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "turn_left_90", parms.blendFrames );
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

/*
================
rvMonsterBossMakron::State_Torso_CannonAttack
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_CannonAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	//once an anim starts with the legs disabled, the rest of the anims should match that.
	static bool	noLegs;
	switch ( parms.stage ) {
		case STAGE_INIT:
			//if flying, do not override legs
			if( !flagFlyingMode || ( flagFlyingMode && !move.fl.moving ))	{
				noLegs = true;
				DisableAnimState ( ANIMCHANNEL_LEGS );
				PlayAnim ( ANIMCHANNEL_TORSO, "range_cannon_start", parms.blendFrames );
			} else {
				noLegs = false;
				PlayAnim ( ANIMCHANNEL_TORSO, "range_cannon_start", parms.blendFrames );
				PlayAnim ( ANIMCHANNEL_LEGS, "range_cannon_start", parms.blendFrames );
	
			}
			shots = (gameLocal.random.RandomInt ( 8 ) + 4) * combat.aggressiveScale;
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_LOOP:
			//if we're flying, and moving, fire fast!
			//if( flagFlyingMode &&  move.fl.moving )	{
			if( !noLegs )	{
				PlayAnim ( ANIMCHANNEL_TORSO, "range_cannon_fire_fast", 0 );
				PlayAnim ( ANIMCHANNEL_LEGS, "range_cannon_fire_fast", 0 );
			} else {
				DisableAnimState ( ANIMCHANNEL_LEGS );
				PlayAnim ( ANIMCHANNEL_TORSO, "range_cannon_fire", 0 );
			}
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( --shots <= 0 || (IsEnemyVisible() && !enemy.fl.inFov)  ) {
					//if( flagFlyingMode &&  move.fl.moving )	{
					if( !noLegs )	{
						PlayAnim ( ANIMCHANNEL_TORSO, "range_cannon_end", 0 );
						PlayAnim ( ANIMCHANNEL_LEGS, "range_cannon_end", 0 );
					} else {
						DisableAnimState ( ANIMCHANNEL_LEGS );
						PlayAnim ( ANIMCHANNEL_TORSO, "range_cannon_end", 0 );
					}
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


/*
================
rvMonsterBossMakron::State_Torso_Lightning1Attack
================
*/

stateResult_t rvMonsterBossMakron::State_Torso_Lightning1Attack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_INIT_BOLT,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};

	idVec3		origin;
	idVec3		targetPoint;
	idMat3		axis;
	trace_t		tr;

	switch ( parms.stage ) {
		case STAGE_INIT:
			//gameLocal.Warning( "Lightningbolt!");
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "range_blast_start", parms.blendFrames );
			shots = (gameLocal.random.RandomInt ( 3 ) + 2) * combat.aggressiveScale;
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_INIT_BOLT );
			}
			return SRESULT_WAIT;

		case STAGE_INIT_BOLT:
			//aim a little below the player's chest
			targetPoint = enemy.lastVisibleChestPosition;
			targetPoint.z -= 24;
			InitBoltSweep( targetPoint );
			return SRESULT_STAGE ( STAGE_LOOP );

		case STAGE_LOOP:
			PlayAnim ( ANIMCHANNEL_TORSO, "range_blast_loop", 0 );
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			//sweep the blasts back and forth
			MaintainBoltSweep();

			//keep playing the anim until the sweeping is done.
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) || flagFakeDeath ) {
				if ( flagSweepDone ) {
					PlayAnim ( ANIMCHANNEL_TORSO, "range_blast_end", 0 );
					return SRESULT_STAGE ( STAGE_WAITEND );
				}
				else	{
					PlayAnim ( ANIMCHANNEL_TORSO, "range_blast_loop", 0 );
				}	
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


/*
================
rvMonsterBossMakron::State_Torso_KillPlayer
================
*/

stateResult_t rvMonsterBossMakron::State_Torso_KillPlayer ( const stateParms_t& parms ) {
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
			PlayAnim ( ANIMCHANNEL_TORSO, "range_blast_start", parms.blendFrames );
			shots = 8;
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;

		case STAGE_LOOP:
			PlayAnim ( ANIMCHANNEL_TORSO, "range_blast_loop_killplayer", 0 );
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			//keep playing the anim until the sweeping is done.
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) || flagFakeDeath ) {
				shots--;
				if ( shots < 1) {
					PlayAnim ( ANIMCHANNEL_TORSO, "range_blast_end", 0 );
					return SRESULT_STAGE ( STAGE_WAITEND );
				}
				else	{
					PlayAnim ( ANIMCHANNEL_TORSO, "range_blast_loop_killplayer", 0 );
				}	
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



/*
================
rvBossMakron::State_Torso_CannonSweepAttack
================
*/
stateResult_t rvMonsterBossMakron::State_Torso_CannonSweepAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};

	switch( parms.stage )	{
		case STAGE_INIT:
			//gameLocal.Warning("Makron CannonSweep Go!");
			//sweep across with the cannon
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_cannonsweep", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT);
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_DONE;
			}
			//if the flag is up, fire.
			
			return SRESULT_WAIT;	

	}

	return SRESULT_ERROR;

}

/*
================
rvMonsterBossMakron::State_Killed
================
*/
stateResult_t rvMonsterBossMakron::State_Killed ( const stateParms_t& parms ) {
	enum {
		STAGE_FALLSTART,
		STAGE_FALLSTARTWAIT,
		STAGE_FALLLOOPWAIT,
		STAGE_FALLENDWAIT
	};
	switch ( parms.stage ) {
		case STAGE_FALLSTART:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "death_start", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_FALLSTARTWAIT );
			
		case STAGE_FALLSTARTWAIT:
			if ( move.fl.onGround ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "death_end", 4 );
				return SRESULT_STAGE ( STAGE_FALLENDWAIT );
			}
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "death_loop", 0 );
				return SRESULT_STAGE ( STAGE_FALLLOOPWAIT );
			}
			return SRESULT_WAIT;
			
		case STAGE_FALLLOOPWAIT:
			if ( move.fl.onGround ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "death_end", 0 );
				return SRESULT_STAGE ( STAGE_FALLENDWAIT );
			}
			return SRESULT_WAIT;			
					
		case STAGE_FALLENDWAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				disablePain = true;
				// At this point the Makron is killed!
				// Make sure all animation stops
				//StopAnimState ( ANIMCHANNEL_TORSO );
				//StopAnimState ( ANIMCHANNEL_LEGS );
				//if ( head ) {
				//	StopAnimState ( ANIMCHANNEL_HEAD );
				//}

				// Make sure all animations stop
				//animator.ClearAllAnims ( gameLocal.time, 0 );

				if( spawnArgs.GetBool ( "remove_on_death" )  ){
					PostState ( "State_Remove" );
				} else { 
					PostState ( "State_Dead" );
				}

				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}	
	return SRESULT_ERROR;
}

/*
================
rvMonsterBossMakron::Frame_BeginLightningSweep2
================
*/

stateResult_t rvMonsterBossMakron::Frame_BeginLightningSweep2 ( const stateParms_t& parms )	{

	//begin the sweep with this flag
	stateBoltSweep = 1;
	boltSweepStartTime = gameLocal.time;
	boltSweepTime = 1;
	return SRESULT_OK;
}

/*
================
rvMonsterBossMakron::Frame_BeginLightningSweep2
================
*/

stateResult_t rvMonsterBossMakron::Frame_EndLightningSweep2 ( const stateParms_t& parms )	{
	
	//end the sweep with this flag
	stateBoltSweep = 2;
	return SRESULT_OK;
	
}

/*
================
rvMonsterBossMakron::Frame_Teleport
================
*/

stateResult_t rvMonsterBossMakron::Frame_Teleport ( const stateParms_t& parms )	{

	//hide
	Hide();

	//turn on the do-nothing teleport flag
	flagTeleporting = true;

	//call some script.
	ExecScriptFunction( scriptTeleport );
	return SRESULT_DONE;

}


/*
================
rvMonsterBossMakron::Frame_StompAttack
================
*/

stateResult_t rvMonsterBossMakron::Frame_StompAttack ( const stateParms_t& parms )	{
	
	idVec3		origin;
	idVec3		worldUp(0, 0, 1);

	// Eminate from Makron origin
	origin = this->GetPhysics()->GetOrigin();
	
	//start radius at 256;
	stompRadius = spawnArgs.GetFloat("stomp_start_size", "64");

	//stomp
	gameLocal.PlayEffect ( gameLocal.GetEffect ( this->spawnArgs, "fx_stomp_wave" ), origin, worldUp.ToMat3(), false, origin);

	//bamf!
	PostEventMS( &EV_StompAttack, 0, origin);

	return SRESULT_OK;

}
/*
================
rvMonsterBossMakron::Event_ToggleCornerState
================
*/
void rvMonsterBossMakron::Event_ToggleCornerState ( float f )	{
	if( f ==  1.0f)	{
		flagCornerState = true;
	} else {
		flagCornerState = false;
	}
}

/*
================
rvMonsterBossMakron::Event_StompAttack
================
*/

void rvMonsterBossMakron::Event_StompAttack (idVec3& origin) 	{
	
	idVec3		targetOrigin;
	idVec3		worldUp;
	idEntity*	entities[ 1024 ];
	int			count;
	int			i;
	float		stompZ;
	idVec3		dir;
	modelTrace_t	result;


	stompZ = origin.z;

	worldUp.x = 0;
	worldUp.y = 0;
	worldUp.z = 1;
	
	//if the radius is too big, stop.
	if ( stompRadius > stompMaxRadius )	{
		return;
	}

	//get all enemies within radius. If they are:
	//	 within radius, 
	//	 more than (radius - stompWidth) units away,
	//   Z valued within 16 of the the stomp Z
	//they take stomp damage.
	count = gameLocal.EntitiesWithinRadius ( origin, stompRadius, entities, 1024 );

	//gameRenderWorld->DebugCircle( colorRed,origin,worldUp,stompRadius,24,20,false);
	//gameRenderWorld->DebugCircle( colorBlue,origin,worldUp,stompRadius - stompWidth,24,20,false);

	for ( i = 0; i < count; i ++ ) {
		idEntity* ent = entities[i];	

		//don't stomp ourself, derp...	
		if ( !ent || ent == this ) {
			continue;
		}

		// Must be an actor that takes damage to be affected		
		if ( !ent->fl.takedamage || !ent->IsType ( idActor::GetClassType() ) ) {
			continue;
		}
				
		// Are they Z equal (about?)
		targetOrigin =  ent->GetPhysics()->GetOrigin();	
		if( idMath::Abs( targetOrigin.z - origin.z) > 16)	{
			continue;
		}

		// are they within the stomp width?
		if( targetOrigin.Dist( origin) < ( stompRadius - stompWidth)  || 
			targetOrigin.Dist( origin) > stompRadius )	{
			continue;
		}
		
		if( gameRenderWorld->FastWorldTrace(result, origin, ent->GetPhysics()->GetCenterMass()) ) {
			continue;
		}

		//ok, damage them
		dir = targetOrigin - origin;
		dir.NormalizeFast ( );
		ent->Damage ( this, this, dir, spawnArgs.GetString ( "def_makron_stomp_damage" ), 1.0f, 0 );
		//gameRenderWorld->DebugArrow( colorYellow, origin, targetOrigin, 5, 1000);
	
	}
		
	//move the radius along
	stompRadius += stompSpeed;

	//run it back
	PostEventSec( &EV_StompAttack, stompSpeed / stompMaxRadius , origin );

}

	
