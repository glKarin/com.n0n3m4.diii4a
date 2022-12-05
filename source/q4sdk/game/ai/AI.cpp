/*
================

AI.cpp

================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

#include "AI.h"
#include "AI_Manager.h"
#include "AI_Util.h"
#include "../Projectile.h"
#include "../spawner.h"
#include "AI_Tactical.h"

const char* aiTalkMessageString [ ] = {
	"None",
	"primary",
	"secondary",
	"loop"
};

static const float AI_SIGHTDELAYSCALE	= 5000.0f;			// Full sight delay at 5 seconds or more of not seeing enemy


/*
===============================================================================

	idAI

===============================================================================
*/

/*
=====================
idAI::idAI
=====================
*/
idAI::idAI ( void ) {
	projectile_height_to_distance_ratio = 1.0f;

	aas						= NULL;
	aasSensor				= NULL;
	aasFind					= NULL;

	lastHitCheckResult		= false;
	lastHitCheckTime		= 0;
	lastAttackTime			= 0;
	projectile				= NULL;
	projectileClipModel		= NULL;
	chatterTime				= 0;
	talkState				= TALK_NEVER;
	talkTarget				= NULL;
	talkMessage				= TALKMSG_NONE;
	talkBusyCount			= 0;

	enemy.ent					= NULL;
	enemy.lastVisibleChangeTime	= 0;
	enemy.lastVisibleTime		= 0;

	fl.neverDormant			= false;		// AI's can go dormant

	allowEyeFocus			= true;
	disablePain				= false;
	allowJointMod			= true;
	focusEntity				= NULL;
	focusTime				= 0;
	alignHeadTime			= 0;
	forceAlignHeadTime		= 0;

	orientationJoint		= INVALID_JOINT;

	eyeVerticalOffset		= 0.0f;
	eyeHorizontalOffset 	= 0.0f;
	headFocusRate			= 0.0f;
	eyeFocusRate			= 0.0f;
	focusAlignTime			= 0;
	focusRange				= 0.0f;
	focusType				= AIFOCUS_NONE;

	memset ( &combat.fl, 0, sizeof(combat.fl) );
	combat.max_chasing_turn			= 0;
	combat.shotAtTime				= 0;
	combat.shotAtAngle				= 0.0f;
	combat.meleeRange				= 0.0f;
 	combat.tacticalPainTaken		= 0;
 	combat.tacticalFlinches			= 0;
 	combat.investigateTime			= 0;
 	combat.aggressiveScale			= 1.0f;

	passive.animFidgetPrefix.Clear ( );
	passive.animIdlePrefix.Clear ( );
	passive.animTalkPrefix.Clear ( );
	passive.idleAnim.Clear();
	passive.prefix.Clear();
	passive.idleAnimChangeTime		= 0;
	passive.fidgetTime				= 0;
	passive.talkTime				= 0;
	memset ( &passive.fl, 0, sizeof(passive.fl) );
			
	pain.lastTakenTime				= 0;
	pain.takenThisFrame				= 0;
	pain.loopEndTime				= 0;
		
	enemy.range = 0;
	enemy.range2d = 0;
	enemy.smoothedLinearVelocity.Zero ( );
	enemy.smoothedPushedVelocity.Zero ( );
	enemy.lastKnownPosition.Zero ( );
	enemy.lastVisibleEyePosition.Zero ( );
	enemy.lastVisibleChestPosition.Zero ( );
	enemy.lastVisibleFromEyePosition.Zero ( );
	enemy.checkTime = 0;
	enemy.changeTime = 0;
	enemy.lastVisibleChangeTime = 0;
	enemy.lastVisibleTime = 0;
	memset ( &enemy.fl, 0, sizeof(enemy.fl) );

	currentFocusPos.Zero();
	eyeAng.Zero();
	lookAng.Zero();
	destLookAng.Zero();
	lookMin.Zero();
	lookMax.Zero();

	eyeMin.Zero();
	eyeMax.Zero();
	
	helperCurrent	= NULL;
	helperIdeal		= NULL;

	speakTime		= 0;

	actionAnimNum	= 0;
	actionSkipTime	= 0;
	actionTime		= 0;
}

/*
=====================
idAI::~idAI
=====================
*/
idAI::~idAI() {
	// Make sure we arent stuck in the simple think list
	simpleThinkNode.Remove ( );

	delete aasFind;
	delete aasSensor;
	delete projectileClipModel;
	DeconstructScriptObject();
	scriptObject.Free();
	aiManager.RemoveTeammate ( this );
	SetPhysics( NULL );
}

/*
=====================
idAI::Save
=====================
*/
void idAI::Save( idSaveGame *savefile ) const {
	int i;

// cnicholson: These 3 vars are intentionally not saved, as noted in the restore
	// NOSAVE: idLinkList<idAI>		simpleThinkNode;
	// NOSAVE: idAAS*				aas;
	// NOSAVE: idAASCallback*		aasFind;
	// NOTE That some AAS stuff is done at end of ::Restore

	// Movement
	move.Save( savefile );
	savedMove.Save( savefile );
	
	savefile->WriteStaticObject( physicsObj );
	savefile->WriteBool( GetPhysics() == static_cast<const idPhysics *>(&physicsObj) );

	savefile->WriteBool( lastHitCheckResult );
	savefile->WriteInt( lastHitCheckTime );
	savefile->WriteInt( lastAttackTime );
	savefile->WriteFloat( projectile_height_to_distance_ratio );

	savefile->WriteInt( attackAnimInfo.Num() );
	for( i = 0; i < attackAnimInfo.Num(); i++ ) {
		savefile->WriteVec3( attackAnimInfo[ i ].attackOffset );
		savefile->WriteVec3( attackAnimInfo[ i ].eyeOffset );
	}
	
	// TOSAVE: mutable idClipModel*	projectileClipModel;
	projectile.Save ( savefile );

	// Talking
	savefile->WriteInt( chatterTime );
//	savefile->WriteInt( chatterRateCombat );	// NOSAVE:
//	savefile->WriteInt( chatterRateIdle );		// NOSAVE:
	savefile->WriteInt( talkState );
	talkTarget.Save( savefile );
	savefile->WriteInt( talkMessage );
	savefile->WriteInt( talkBusyCount );
	savefile->WriteInt ( speakTime );

	// Focus
	lookTarget.Save ( savefile );
	savefile->WriteInt( focusType );
	focusEntity.Save ( savefile );
	savefile->WriteFloat ( focusRange );
	savefile->WriteInt ( focusAlignTime );
	savefile->WriteInt ( focusTime );
	savefile->WriteVec3( currentFocusPos );

	// Looking
	savefile->WriteBool( allowJointMod );
	savefile->WriteInt( alignHeadTime );
	savefile->WriteInt( forceAlignHeadTime );
	savefile->WriteAngles( eyeAng );
	savefile->WriteAngles( lookAng );
	savefile->WriteAngles( destLookAng );
	savefile->WriteAngles( lookMin );
	savefile->WriteAngles( lookMax );

	savefile->WriteInt( lookJoints.Num() );
	for( i = 0; i < lookJoints.Num(); i++ ) {
		savefile->WriteJoint( lookJoints[ i ] );
		savefile->WriteAngles( lookJointAngles[ i ] );
	}

	savefile->WriteFloat( eyeVerticalOffset );
	savefile->WriteFloat( eyeHorizontalOffset );
	savefile->WriteFloat( headFocusRate );
	savefile->WriteFloat( eyeFocusRate );

	// Joint Controllers
	savefile->WriteAngles( eyeMin );
	savefile->WriteAngles( eyeMax );
	savefile->WriteJoint( orientationJoint );

	pusher.Save( savefile );			// cnicholson: Added unsaved var
	scriptedActionEnt.Save( savefile ); // cnicholson: Added unsaved var
    
	savefile->Write( &aifl, sizeof( aifl ) );

	// Misc
	savefile->WriteInt ( actionAnimNum );
	savefile->WriteInt ( actionTime );
	savefile->WriteInt ( actionSkipTime );
	savefile->WriteInt ( flagOverrides );

	// Combat variables
	savefile->Write( &combat.fl, sizeof( combat.fl ) );
	savefile->WriteFloat ( combat.max_chasing_turn );
	savefile->WriteFloat ( combat.shotAtTime );
	savefile->WriteFloat ( combat.shotAtAngle );
	savefile->WriteVec2 ( combat.hideRange );
	savefile->WriteVec2 ( combat.attackRange );
	savefile->WriteInt ( combat.attackSightDelay );
	savefile->WriteFloat ( combat.meleeRange );
	savefile->WriteFloat ( combat.aggressiveRange );
	savefile->WriteFloat ( combat.aggressiveScale );
	savefile->WriteInt ( combat.investigateTime );	
	savefile->WriteFloat ( combat.visStandHeight );
	savefile->WriteFloat ( combat.visCrouchHeight );
	savefile->WriteFloat ( combat.visRange );
	savefile->WriteFloat ( combat.earRange );
	savefile->WriteFloat ( combat.awareRange );
	savefile->WriteInt ( combat.tacticalMaskAvailable );
	savefile->WriteInt ( (int&)combat.tacticalCurrent );	
	savefile->WriteInt ( combat.tacticalUpdateTime );	
	savefile->WriteInt ( combat.tacticalPainTaken );	
	savefile->WriteInt ( combat.tacticalPainThreshold );	
	savefile->WriteInt ( combat.tacticalFlinches );	
	savefile->WriteInt ( combat.maxLostVisTime );	
	savefile->WriteFloat ( combat.threatBase );
	savefile->WriteFloat ( combat.threatCurrent );
	savefile->WriteInt	 ( combat.coverValidTime );
	savefile->WriteInt   ( combat.maxInvalidCoverTime );

	// Passive state variables
	savefile->WriteString ( passive.prefix );
	savefile->WriteString ( passive.animFidgetPrefix );
	savefile->WriteString ( passive.animIdlePrefix);
	savefile->WriteString ( passive.animTalkPrefix );
	savefile->WriteString ( passive.idleAnim );
	savefile->WriteInt	  ( passive.idleAnimChangeTime );
	savefile->WriteInt	  ( passive.fidgetTime );
	savefile->WriteInt	  ( passive.talkTime );
	savefile->Write ( &passive.fl, sizeof(passive.fl) );

	// Enemy 
	enemy.ent.Save ( savefile );
	savefile->Write ( &enemy.fl, sizeof(enemy.fl) );
	savefile->WriteInt ( enemy.lastVisibleChangeTime );
	savefile->WriteVec3 ( enemy.lastKnownPosition );
	savefile->WriteVec3 ( enemy.smoothedLinearVelocity );
	savefile->WriteVec3 ( enemy.smoothedPushedVelocity );
	savefile->WriteVec3 ( enemy.lastVisibleEyePosition );
	savefile->WriteVec3 ( enemy.lastVisibleChestPosition );
	savefile->WriteVec3 ( enemy.lastVisibleFromEyePosition );
	savefile->WriteInt ( enemy.lastVisibleTime );
	savefile->WriteFloat ( enemy.range );
	savefile->WriteFloat ( enemy.range2d );
	savefile->WriteInt ( enemy.changeTime );
	savefile->WriteInt ( enemy.checkTime );

	// Pain variables
	savefile->WriteFloat ( pain.threshold );
	savefile->WriteFloat ( pain.takenThisFrame );
	savefile->WriteInt ( pain.lastTakenTime );
	savefile->WriteInt ( pain.loopEndTime );
	savefile->WriteString ( pain.loopType );

	// Functions
	funcs.first_sight.Save ( savefile );
	funcs.sight.Save ( savefile );
	funcs.pain.Save ( savefile );
	funcs.damage.Save ( savefile );
	funcs.death.Save ( savefile );
	funcs.attack.Save ( savefile );
	funcs.init.Save ( savefile );
	funcs.onclick.Save ( savefile );
	funcs.launch_projectile.Save ( savefile );
	funcs.footstep.Save ( savefile );

	mPlayback.Save( savefile );		// cnicholson: Added save functionality
	mLookPlayback.Save( savefile );	// cnicholson: Added save functionality

	// Tactical Sensor
	aasSensor->Save( savefile );

	// Helpers
	tether.Save ( savefile );
	helperCurrent.Save ( savefile );
	helperIdeal.Save ( savefile );
	leader.Save ( savefile );
	spawner.Save ( savefile );

	// Action timers
	actionTimerRangedAttack.Save ( savefile );
	actionTimerEvade.Save ( savefile );
	actionTimerSpecialAttack.Save ( savefile );
	actionTimerPain.Save ( savefile );
	
	// Actions
	actionEvadeLeft.Save ( savefile );
	actionEvadeRight.Save ( savefile );
	actionRangedAttack.Save ( savefile );
	actionMeleeAttack.Save ( savefile );
	actionLeapAttack.Save ( savefile );
	actionJumpBack.Save ( savefile );
}

/*
=====================
idAI::Restore
=====================
*/
void idAI::Restore( idRestoreGame *savefile ) {
	bool		restorePhysics;
	int			i;
	int			num;
	idBounds	bounds;

	InitNonPersistentSpawnArgs ( );

	// INTENTIONALLY NOT SAVED: idLinkList<idAI>	simpleThinkNode;
	// INTENTIONALLY NOT SAVED: idAAS*				aas;
	// INTENTIONALLY NOT SAVED: idAASCallback*		aasFind;
	// NOTE That some AAS stuff is done at end of ::Restore

	move.Restore( savefile );
	savedMove.Restore( savefile );

	savefile->ReadStaticObject( physicsObj );
	savefile->ReadBool( restorePhysics );
	if ( restorePhysics ) {
		RestorePhysics( &physicsObj );
	}

	savefile->ReadBool( lastHitCheckResult );
	savefile->ReadInt( lastHitCheckTime );
	savefile->ReadInt( lastAttackTime );
	savefile->ReadFloat( projectile_height_to_distance_ratio );

	savefile->ReadInt( num );
	attackAnimInfo.SetGranularity( 1 );
	attackAnimInfo.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadVec3( attackAnimInfo[ i ].attackOffset );
		savefile->ReadVec3( attackAnimInfo[ i ].eyeOffset );
	}

	// TORESTORE: mutable idClipModel*	projectileClipModel;
	projectile.Restore( savefile );

	// Talking
	savefile->ReadInt( chatterTime );
//	savefile->ReadInt( chatterRateCombat );		// Don't save
//	savefile->ReadInt( chatterRateIdle );		// Don't save
	savefile->ReadInt( i );
	talkState = static_cast<talkState_t>( i );
	talkTarget.Restore( savefile );
	savefile->ReadInt( i );
	talkMessage = static_cast<talkMessage_t>( i );
	savefile->ReadInt( talkBusyCount );
	savefile->ReadInt ( speakTime );

	// Focus
	lookTarget.Restore ( savefile );
	savefile->ReadInt( i );
	focusType = static_cast<aiFocus_t>( i );
	focusEntity.Restore ( savefile );
	savefile->ReadFloat ( focusRange );
	savefile->ReadInt ( focusAlignTime );
	savefile->ReadInt ( focusTime );
	savefile->ReadVec3( currentFocusPos );
	
	// Looking
	savefile->ReadBool( allowJointMod );
	savefile->ReadInt( alignHeadTime );
	savefile->ReadInt( forceAlignHeadTime );
	savefile->ReadAngles( eyeAng );
	savefile->ReadAngles( lookAng );
	savefile->ReadAngles( destLookAng );
	savefile->ReadAngles( lookMin );
	savefile->ReadAngles( lookMax );

	savefile->ReadInt( num );
	lookJoints.SetGranularity( 1 );
	lookJoints.SetNum( num );
	lookJointAngles.SetGranularity( 1 );
	lookJointAngles.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadJoint( lookJoints[ i ] );
		savefile->ReadAngles( lookJointAngles[ i ] );
	}

	savefile->ReadFloat( eyeVerticalOffset );
	savefile->ReadFloat( eyeHorizontalOffset );
	savefile->ReadFloat( headFocusRate );
	savefile->ReadFloat( eyeFocusRate );

	// Joint Controllers
	savefile->ReadAngles( eyeMin );
	savefile->ReadAngles( eyeMax );
	savefile->ReadJoint( orientationJoint );

	pusher.Restore( savefile );				// cnicholson: Added unrestored var
	scriptedActionEnt.Restore ( savefile ); // cnicholson: Added unrestored var

	savefile->Read( &aifl, sizeof( aifl ) );

	// Misc
	savefile->ReadInt ( actionAnimNum );
	savefile->ReadInt ( actionTime );
	savefile->ReadInt ( actionSkipTime );
	savefile->ReadInt ( flagOverrides );

	// Combat variables
	savefile->Read ( &combat.fl, sizeof( combat.fl ) );
	savefile->ReadFloat ( combat.max_chasing_turn );
	savefile->ReadFloat ( combat.shotAtTime );
	savefile->ReadFloat ( combat.shotAtAngle );
	savefile->ReadVec2 ( combat.hideRange );
	savefile->ReadVec2 ( combat.attackRange );
	savefile->ReadInt ( combat.attackSightDelay );
	savefile->ReadFloat ( combat.meleeRange );
	savefile->ReadFloat ( combat.aggressiveRange );
	savefile->ReadFloat ( combat.aggressiveScale );
	savefile->ReadInt ( combat.investigateTime );	
	savefile->ReadFloat ( combat.visStandHeight );
	savefile->ReadFloat ( combat.visCrouchHeight );
	savefile->ReadFloat ( combat.visRange );
	savefile->ReadFloat ( combat.earRange );
	savefile->ReadFloat ( combat.awareRange );
	savefile->ReadInt ( combat.tacticalMaskAvailable );	
	savefile->ReadInt ( (int&)combat.tacticalCurrent );	
	savefile->ReadInt ( combat.tacticalUpdateTime );	
	savefile->ReadInt ( combat.tacticalPainTaken );	
	savefile->ReadInt ( combat.tacticalPainThreshold );	
	savefile->ReadInt ( combat.tacticalFlinches );	
	savefile->ReadInt ( combat.maxLostVisTime );	
	savefile->ReadFloat ( combat.threatBase );
	savefile->ReadFloat ( combat.threatCurrent );
	savefile->ReadInt	 ( combat.coverValidTime );
	savefile->ReadInt   ( combat.maxInvalidCoverTime );

	// Passive state variables
	savefile->ReadString ( passive.prefix );
	savefile->ReadString ( passive.animFidgetPrefix );
	savefile->ReadString ( passive.animIdlePrefix);
	savefile->ReadString ( passive.animTalkPrefix );
	savefile->ReadString ( passive.idleAnim );
	savefile->ReadInt	 ( passive.idleAnimChangeTime );
	savefile->ReadInt	 ( passive.fidgetTime );
	savefile->ReadInt	 ( passive.talkTime );
	savefile->Read ( &passive.fl, sizeof(passive.fl) );

	// Enemy 
	enemy.ent.Restore ( savefile );
	savefile->Read ( &enemy.fl, sizeof(enemy.fl) );
	savefile->ReadInt ( enemy.lastVisibleChangeTime );
	savefile->ReadVec3 ( enemy.lastKnownPosition );
	savefile->ReadVec3 ( enemy.smoothedLinearVelocity );
	savefile->ReadVec3 ( enemy.smoothedPushedVelocity );
	savefile->ReadVec3 ( enemy.lastVisibleEyePosition );
	savefile->ReadVec3 ( enemy.lastVisibleChestPosition );
	savefile->ReadVec3 ( enemy.lastVisibleFromEyePosition );
	savefile->ReadInt ( enemy.lastVisibleTime );
	savefile->ReadFloat ( enemy.range );
	savefile->ReadFloat ( enemy.range2d );
	savefile->ReadInt ( enemy.changeTime );
	savefile->ReadInt ( enemy.checkTime );

	// Pain variables
	savefile->ReadFloat ( pain.threshold );
	savefile->ReadFloat ( pain.takenThisFrame );
	savefile->ReadInt ( pain.lastTakenTime );
	savefile->ReadInt ( pain.loopEndTime );
	savefile->ReadString ( pain.loopType );

	// Functions
	funcs.first_sight.Restore ( savefile );
	funcs.sight.Restore ( savefile );
	funcs.pain.Restore ( savefile );
	funcs.damage.Restore ( savefile );
	funcs.death.Restore ( savefile );
	funcs.attack.Restore ( savefile );
	funcs.init.Restore ( savefile );
	funcs.onclick.Restore ( savefile );
	funcs.launch_projectile.Restore ( savefile );
	funcs.footstep.Restore ( savefile );

	mPlayback.Restore( savefile );		// cnicholson: Added restore functionality
	mLookPlayback.Restore( savefile );	// cnicholson: Added restore functionality

	// Tactical Sensor
	aasSensor->Restore( savefile );

	// Helpers
	tether.Restore ( savefile );
	helperCurrent.Restore( savefile );
	helperIdeal.Restore ( savefile );
	leader.Restore ( savefile );
	spawner.Restore ( savefile );

	// Action timers
	actionTimerRangedAttack.Restore ( savefile );
	actionTimerEvade.Restore ( savefile );
	actionTimerSpecialAttack.Restore ( savefile );
	actionTimerPain.Restore ( savefile );
	
	// Actions
	actionEvadeLeft.Restore ( savefile );
	actionEvadeRight.Restore ( savefile );
	actionRangedAttack.Restore ( savefile );
	actionMeleeAttack.Restore ( savefile );
	actionLeapAttack.Restore ( savefile );
	actionJumpBack.Restore ( savefile );

	// Set the AAS if the character has the correct gravity vector
	idVec3 gravity = spawnArgs.GetVector( "gravityDir", "0 0 -1" );
	gravity *= g_gravity.GetFloat();
	if ( gravity == gameLocal.GetGravity() ) {
		SetAAS();
	}
	
	// create combat collision hull for exact collision detection, do initial set un-hidden
	SetCombatModel();
	LinkCombat();
}

/*
=====================
idAI::InitNonPersistentSpawnArgs
=====================
*/
void idAI::InitNonPersistentSpawnArgs ( void ) {
	aasSensor = rvAASTacticalSensor::CREATE_SENSOR(this);
	aasFind   = NULL;

	simpleThinkNode.Remove ( );
	simpleThinkNode.SetOwner ( this );
	
	chatterRateIdle	  = SEC2MS ( spawnArgs.GetFloat ( "chatter_rate_idle", "0" ) );
	chatterRateCombat = SEC2MS ( spawnArgs.GetFloat ( "chatter_rate_combat", "0" ) );
	chatterTime		  = 0;	
	
	combat.tacticalMaskUpdate = 0;
		
	enemy.smoothVelocityRate = spawnArgs.GetFloat ( "smoothVelocityRate", "0.1" );
}

/*
=====================
idAI::Spawn
=====================
*/
void idAI::Spawn( void ) {
	const char*			jointname;
	const idKeyValue*	kv;
	idStr				jointName;
	idAngles			jointScale;
	jointHandle_t		joint;
	idVec3				local_dir;

	// Are all monsters disabled?
	if ( !g_monsters.GetBool() ) {
		PostEventMS( &EV_Remove, 0 );
		return;
	}

	// Initialize the non saved spawn args
	InitNonPersistentSpawnArgs ( );	

	spawnArgs.GetInt(	"team",					"1",		team );
	spawnArgs.GetInt(	"rank",					"0",		rank );

	animPrefix = spawnArgs.GetString ( "animPrefix", "" );

	// Standard flags
	fl.notarget		= spawnArgs.GetBool ( "notarget", "0" );
	fl.quickBurn	= false;

	// AI flags
	flagOverrides = 0;
	memset ( &aifl, 0, sizeof(aifl) );
	aifl.ignoreFlashlight 		= spawnArgs.GetBool ( "ignore_flashlight", "1" );
	aifl.lookAtPlayer	  		= spawnArgs.GetBool ( "lookAtPlayer", "0" );
	aifl.disableLook	  		= spawnArgs.GetBool ( "noLook", "0" );
	aifl.undying		  		= spawnArgs.GetBool ( "undying", "0" );
	aifl.killerGuard			= spawnArgs.GetBool ( "killer_guard", "0" );
	aifl.scriptedEndWithIdle	= true;

	// Setup Move Data
	move.Spawn( spawnArgs );

	pain.threshold		= spawnArgs.GetInt ( "painThreshold", "0" );
	pain.takenThisFrame	= 0;		
	
	// Initialize combat variables
 	combat.fl.aware					= spawnArgs.GetBool ( "ambush", "0" );
 	combat.fl.tetherNoBreak			= spawnArgs.GetBool ( "tetherNoBreak", "0" );
 	combat.fl.noChatter				= spawnArgs.GetBool ( "noCombatChatter" );
	combat.hideRange				= spawnArgs.GetVec2 ( "hideRange", "150 750" );
	combat.attackRange				= spawnArgs.GetVec2 ( "attackRange", "0 1000" );
	combat.attackSightDelay			= SEC2MS ( spawnArgs.GetFloat ( "attackSightDelay", "1" ) );
 	combat.visRange					= spawnArgs.GetFloat( "visRange", "2048" );
 	combat.visStandHeight			= spawnArgs.GetFloat( "visStandHeight", "68" );
 	combat.visCrouchHeight			= spawnArgs.GetFloat( "visCrouchHeight", "48" );
 	combat.earRange					= spawnArgs.GetFloat( "earRange", "2048" );
 	combat.awareRange				= spawnArgs.GetFloat( "awareRange", "150" );
 	combat.aggressiveRange			= spawnArgs.GetFloat( "aggressiveRange", "0" );
 	combat.maxLostVisTime			= SEC2MS ( spawnArgs.GetFloat ( "maxLostVisTime", "10" ) );
 	combat.tacticalPainThreshold    = spawnArgs.GetInt ( "tactical_painThreshold", va("%d", health / 4) );
	combat.coverValidTime			= 0;
	combat.maxInvalidCoverTime		= SEC2MS ( spawnArgs.GetFloat ( "maxInvalidCoverTime", "1" ) );
	combat.threatBase				= spawnArgs.GetFloat ( "threatBase", "1" );
	combat.threatCurrent			= combat.threatBase;

	SetPassivePrefix ( spawnArgs.GetString ( "passivePrefix" ) );

	disablePain = spawnArgs.GetBool ( "nopain", "0" );

	spawnArgs.GetFloat( "melee_range",			"64",		combat.meleeRange );
	spawnArgs.GetFloat( "projectile_height_to_distance_ratio",	"1", projectile_height_to_distance_ratio );
	
	//melee superhero -- take far reduced damage from melee.
	if ( spawnArgs.GetString( "objectivetitle_failed", NULL ) && spawnArgs.GetBool( "meleeSuperhero", "1")   )  {
		aifl.meleeSuperhero = true;
	} else	{
		aifl.meleeSuperhero = false;
	}

	//announce rate
	announceRate = spawnArgs.GetFloat( "announceRate" );
	if( 0.0f == announceRate )	{
		announceRate = AISPEAK_CHANCE;
	}

	fl.takedamage = !spawnArgs.GetBool( "noDamage" );

	animator.RemoveOriginOffset( true );

	// create combat collision hull for exact collision detection
	SetCombatModel();

	lookMin	= spawnArgs.GetAngles( "look_min", "-80 -75 0" );
	lookMax	= spawnArgs.GetAngles( "look_max", "80 75 0" );

	lookJoints.SetGranularity( 1 );
	lookJointAngles.SetGranularity( 1 );
	kv = spawnArgs.MatchPrefix( "look_joint", NULL );
	while( kv ) {
		jointName = kv->GetKey();
		jointName.StripLeadingOnce( "look_joint " );
		joint = animator.GetJointHandle( jointName );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Warning( "Unknown look_joint '%s' on entity %s", jointName.c_str(), name.c_str() );
		} else {
			jointScale = spawnArgs.GetAngles( kv->GetKey(), "0 0 0" );
			jointScale.roll = 0.0f;

			// if no scale on any component, then don't bother adding it.  this may be done to
			// zero out rotation from an inherited entitydef.
			if ( jointScale != ang_zero ) {
				lookJoints.Append( joint );
				lookJointAngles.Append( jointScale );
			}
		}
		kv = spawnArgs.MatchPrefix( "look_joint", kv );
	}

	// calculate joint positions on attack frames so we can do proper "can hit" tests
	CalculateAttackOffsets();

	eyeMin				= spawnArgs.GetAngles( "eye_turn_min", "-10 -30 0" );
	eyeMax				= spawnArgs.GetAngles( "eye_turn_max", "10 30 0" );
	eyeVerticalOffset	= spawnArgs.GetFloat( "eye_verticle_offset", "5" );
	eyeHorizontalOffset = spawnArgs.GetFloat( "eye_horizontal_offset", "-8" );
	headFocusRate		= spawnArgs.GetFloat( "head_focus_rate", "0.06" );
	eyeFocusRate		= spawnArgs.GetFloat( "eye_focus_rate", "0.5" );
	focusRange			= spawnArgs.GetFloat( "focus_range", "0" );
	focusAlignTime		= SEC2MS( spawnArgs.GetFloat( "focus_align_time", "1" ) );

	jointname = spawnArgs.GetString( "joint_orientation" );
	if ( *jointname ) {
		orientationJoint = animator.GetJointHandle( jointname );
		if ( orientationJoint == INVALID_JOINT ) {
			gameLocal.Warning( "Joint '%s' not found on '%s'", jointname, name.c_str() );
		}
	}

	jointname = spawnArgs.GetString( "joint_flytilt" );
	if ( *jointname ) {
		move.flyTiltJoint = animator.GetJointHandle( jointname );
		if ( move.flyTiltJoint == INVALID_JOINT ) {
			gameLocal.Warning( "Joint '%s' not found on '%s'", jointname, name.c_str() );
		}
	}

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetMass( spawnArgs.GetFloat( "mass", "100" ) );

	if ( spawnArgs.GetBool( "big_monster" ) ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetClipMask( MASK_MONSTERSOLID & ~CONTENTS_BODY );
	} else {
		if ( use_combat_bbox ) {
			physicsObj.SetContents( CONTENTS_BODY|CONTENTS_SOLID );
		} else {
			physicsObj.SetContents( CONTENTS_BODY );
		}
		physicsObj.SetClipMask( MASK_MONSTERSOLID );
	}

	// move up to make sure the monster is at least an epsilon above the floor
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	
	idVec3 gravity = spawnArgs.GetVector( "gravityDir", "0 0 -1" );
	gravity *= g_gravity.GetFloat();
	physicsObj.SetGravity( gravity );

	SetPhysics( &physicsObj );

	physicsObj.GetGravityAxis().ProjectVector( viewAxis[ 0 ], local_dir );
	move.current_yaw		= local_dir.ToYaw();
	move.ideal_yaw			= idMath::AngleNormalize180( move.current_yaw );
	
	lookAng.Zero ( );
	lookAng.yaw		= move.current_yaw;


	SetAAS();

	projectile		= NULL;
	projectileClipModel	= NULL;

	if ( spawnArgs.GetBool( "hide" ) || spawnArgs.GetBool( "trigger_anim" ) ) {
		fl.takedamage = false;
		physicsObj.SetContents( 0 );
		physicsObj.GetClipModel()->Unlink();
		Hide();
	} else {
		// play a looping ambient sound if we have one
		StartSound( "snd_ambient", SND_CHANNEL_AMBIENT, 0, false, NULL );
	}

	if ( health <= 0 ) {
		gameLocal.Warning( "entity '%s' doesn't have health set", name.c_str() );
		health = 1;
	}

	BecomeActive( TH_THINK );

	if ( move.fl.allowPushMovables ) {
		af.SetupPose( this, gameLocal.time );
		af.GetPhysics()->EnableClip();
	}

	// init the move variables
	StopMove( MOVE_STATUS_DONE );
	
	// Initialize any scripts
	idStr prefix = "script_";
	for ( kv = spawnArgs.MatchPrefix ( prefix.c_str(), NULL );
		  kv;
		  kv = spawnArgs.MatchPrefix ( prefix.c_str(), kv ) ) {
			  SetScript ( kv->GetKey().c_str() + prefix.Length(), kv->GetValue() );
	}
	
	// Initialize actions	
	actionEvadeLeft.Init		( spawnArgs, "action_evadeLeft",		NULL,					0 );
	actionEvadeRight.Init		( spawnArgs, "action_evadeRight",		NULL, 					0 );
	actionRangedAttack.Init		( spawnArgs, "action_rangedAttack",		NULL, 					AIACTIONF_ATTACK );
	actionMeleeAttack.Init		( spawnArgs, "action_meleeAttack",		NULL, 					AIACTIONF_ATTACK|AIACTIONF_MELEE );
	actionLeapAttack.Init		( spawnArgs, "action_leapAttack",		NULL, 					AIACTIONF_ATTACK );
	actionJumpBack.Init			( spawnArgs, "action_jumpBack",			NULL, 					0 );
			
	// Global action timers
	actionTimerRangedAttack.Init	( spawnArgs, "actionTimer_RangedAttack" );
	actionTimerEvade.Init			( spawnArgs, "actionTimer_Evade" );
	actionTimerSpecialAttack.Init	( spawnArgs, "actionTimer_SpecialAttack" );
	actionTimerPain.Init			( spawnArgs, "actionTimer_pain" );

	// Available tactical options	
	combat.tacticalUpdateTime = 0;
	combat.tacticalCurrent    = AITACTICAL_NONE;
	combat.tacticalMaskUpdate = 0;
	combat.tacticalMaskAvailable  = AITACTICAL_TURRET_BIT|AITACTICAL_MOVE_FOLLOW_BIT|AITACTICAL_MOVE_TETHER_BIT;
	combat.tacticalMaskAvailable |= (spawnArgs.GetBool ( "tactical_cover",   "0" )		? AITACTICAL_COVER_BITS				: 0);
	combat.tacticalMaskAvailable |= (spawnArgs.GetBool ( "tactical_ranged",  "0" ) 		? AITACTICAL_RANGED_BITS			: 0);
	combat.tacticalMaskAvailable |= (spawnArgs.GetBool ( "tactical_hide",    "0" ) 		? AITACTICAL_HIDE_BIT				: 0);	
	combat.tacticalMaskAvailable |= (spawnArgs.GetBool ( "tactical_rush",    "0" ) 		? AITACTICAL_MELEE_BIT				: 0);
	combat.tacticalMaskAvailable |= (spawnArgs.GetBool ( "tactical_passive", "0" ) 		? AITACTICAL_PASSIVE_BIT			: 0);
	combat.tacticalMaskAvailable |= (spawnArgs.GetBool ( "allowPlayerPush",  "0" )		? AITACTICAL_MOVE_PLAYERPUSH_BIT	: 0);
	
	// Talking?
	const char* npc;
	if ( spawnArgs.GetString( "npc_name", NULL, &npc ) != NULL && *npc ) {
		if ( spawnArgs.GetBool ( "follows", "0" ) ) {
			SetTalkState ( TALK_FOLLOW );
		} else {
			switch ( spawnArgs.GetInt( "talks" ) )
			{
			case 2:
				SetTalkState ( TALK_WAIT );
				break;
			case 1:
				SetTalkState ( TALK_OK );
				break;
			case 0:
			default:
				SetTalkState ( TALK_BUSY );
				break;
			}
		}
	} else {
		SetTalkState ( TALK_NEVER );
	}

	// Passive or aggressive ai?
	if ( spawnArgs.GetBool ( "passive" ) ) {
		Event_BecomePassive ( true );
		
		if ( spawnArgs.GetInt ( "passive" ) > 1 ) {
			aifl.disableLook = true;
		}
	}
	
	// Print out a warning about any AI that is spawned unhidden since they will be all thinking
	if( gameLocal.GameState ( ) == GAMESTATE_STARTUP && !spawnArgs.GetInt( "hide" ) && !spawnArgs.GetInt( "trigger_anim" ) && !spawnArgs.GetInt( "trigger_cover" ) && !spawnArgs.GetInt( "trigger_move" ) ){
		gameLocal.Warning( "Unhidden AI placed in map (will be constantly active): %s (%s)", name.c_str(), GetPhysics()->GetOrigin().ToString() );
	}

	Begin ( );

	// RAVEN BEGIN
	// twhitaker: needed this for difficulty settings
	PostEventMS( &EV_PostSpawn, 0 );
	// RAVEN END
}

/*
===================
idAI::Begin
===================
*/
void idAI::Begin ( void ) {
	const char* temp;
	bool		animWalk;
	bool		animRun;

	// Look for the lack of a run or walk animation and force the opposite
	animWalk = HasAnim ( ANIMCHANNEL_LEGS, "walk" );
	animRun  = HasAnim ( ANIMCHANNEL_LEGS, "run" );
	if ( !animWalk && animRun ) {
		move.fl.noWalk = true;
	} else if ( !animRun && animWalk ) {
		move.fl.noRun = true;
	}

	// If a trigger anim or hide is specified then wait until activated before waking up
	if ( spawnArgs.GetString ( "trigger_anim", "", &temp ) || spawnArgs.GetBool ( "hide" ) ) {	
		Hide();
		PostState ( "Wait_Activated" );
		PostState ( "State_WakeUp", SEC2MS(spawnArgs.GetFloat ( "wait" ) ) );
	// Wake up
	} else {
		PostState ( "State_WakeUp", SEC2MS(spawnArgs.GetFloat ( "wait" ) ) );
	}
}

/*
===================
idAI::WakeUp
===================
*/
void idAI::WakeUp ( void ) {
	const char* temp;

	// Already awake?
	if ( aifl.awake ) {
		return;
	}	

	// Find the closest helper
	UpdateHelper ( );

	aifl.awake = true;

	// If the monster is flying then start them with the flying movement type	
	if ( spawnArgs.GetBool ( "static" ) ) {
		SetMoveType ( MOVETYPE_STATIC );
	} else if ( spawnArgs.GetBool ( "flying" ) ) {
		SetMoveType ( MOVETYPE_FLY );
		move.moveDest = physicsObj.GetOrigin();
		move.moveDest.z += move.fly_offset;
	} else {
		SetMoveType ( MOVETYPE_ANIM );
	}	
	
	// Wake up any linked entities
	WakeUpTargets ( );

	// Default enemy?
	if ( !combat.fl.ignoreEnemies ) {
		if ( spawnArgs.GetString ( "enemy", "", &temp ) && *temp ) {	
			SetEnemy ( gameLocal.FindEntity ( temp ) );
		} else if ( spawnArgs.GetBool ( "forceEnemy", "0" ) ) {
			SetEnemy ( FindEnemy ( false, true ) );
		}
	}

	// Default leader?
 	if ( spawnArgs.GetString ( "leader", "", &temp ) && *temp ) {
		SetLeader ( gameLocal.FindEntity ( temp ) );
 	}
 	
 	// If hidden and face enemy is specified we should orient ourselves toward our enemy immediately
 	if ( enemy.ent && IsHidden ( ) && spawnArgs.GetBool ( "faceEnemy" ) ) {
		TurnToward ( enemy.ent->GetPhysics()->GetOrigin() );
		move.current_yaw = move.ideal_yaw;
		viewAxis = idAngles( 0, move.current_yaw, 0 ).ToMat3();
	} 

	Show ( );

	aiManager.AddTeammate ( this );

	// External script functions
	ExecScriptFunction( funcs.init, this );
	
	OnWakeUp ( );
}

/*
===================
idAI::List_f
===================
*/
void idAI::List_f( const idCmdArgs &args ) {
	int			e;
	int			i;
	idEntity*	check;
	int			count;
	idDict		countsFixed;
	idDict		countsSpawned;

	count = 0;

	gameLocal.Printf( "%-4s  %-20s %s\n", " Num", "EntityDef", "Name" );
	gameLocal.Printf( "------------------------------------------------\n" );
	for( e = 0; e < MAX_GENTITIES; e++ ) {		
		check = gameLocal.entities[ e ];
		if ( !check ) {
			continue;
		}
		
		if ( check->IsType ( idAI::Type ) ) {
			idAI* checkAI = static_cast<idAI*>(check);

			// Skip spawned AI
			if ( checkAI->spawner ) {
				continue;
			}
			countsFixed.SetInt ( check->GetEntityDefName(), countsFixed.GetInt ( check->GetEntityDefName(), "0" ) + 1 );

			gameLocal.Printf( "%4i: %-20s %-20s move: %d\n", e, check->GetEntityDefName(), check->name.c_str(), checkAI->move.fl.allowAnimMove );
			count++;
		} else if ( check->IsType ( rvSpawner::GetClassType() ) ) {			
			const idKeyValue*	kv;	
			rvSpawner*			checkSpawner = static_cast<rvSpawner*>(check);
			for ( kv = check->spawnArgs.MatchPrefix( "def_spawn", NULL ); kv; kv = check->spawnArgs.MatchPrefix( "def_spawn", kv ) ) {
				countsSpawned.SetInt ( kv->GetValue(), countsSpawned.GetInt ( kv->GetValue(), "0" ) + 1 );
			}
			for ( i = 0; i < checkSpawner->GetNumSpawnPoints(); i ++ ) {
				check = checkSpawner->GetSpawnPoint ( i );
				if ( !check ) {
					continue;
				}
				for ( kv = check->spawnArgs.MatchPrefix( "def_spawn", NULL ); kv; kv = check->spawnArgs.MatchPrefix( "def_spawn", kv ) ) {
					countsSpawned.SetInt ( kv->GetValue(), countsSpawned.GetInt ( kv->GetValue(), "0" ) + 1 );
				}
			}
		}
	}

	// Combine the two lists
	for ( e = 0; e < countsSpawned.GetNumKeyVals(); e ++ ) {
		const char* keyName = countsSpawned.GetKeyVal ( e )->GetKey();
		countsFixed.Set ( keyName, va("(%s) %3d", 
						  countsSpawned.GetKeyVal ( e )->GetValue().c_str(),
						  countsFixed.GetInt ( keyName, "0" ) + atoi(countsSpawned.GetKeyVal ( e )->GetValue()) ) );
	}	

	// Print out total counts
	if ( countsFixed.GetNumKeyVals() ) {
		gameLocal.Printf( "------------------------------------------------\n" );
		for ( e = 0; e < countsFixed.GetNumKeyVals(); e ++ ) {
			const idKeyValue* kv = countsFixed.GetKeyVal ( e );
			gameLocal.Printf( "%10s: %-20s\n", kv->GetValue().c_str(), kv->GetKey().c_str() );
		}
	}
	
	gameLocal.Printf( "------------------------------------------------\n" );
	gameLocal.Printf( "...%d monsters (%d unique types)\n", count, countsFixed.GetNumKeyVals() );
}

/*
================
idAI::DormantBegin

called when entity becomes dormant
================
*/
void idAI::DormantBegin( void ) {
	// since dormant happens on a timer, we wont get to update particles to
	// hidden through the think loop, but we need to hide them though.
	if ( enemyNode.InList() ) {
		// remove ourselves from the enemy's enemylist
		enemyNode.Remove();
	}
	idActor::DormantBegin();
}

/*
================
idAI::DormantEnd

called when entity wakes from being dormant
================
*/
void idAI::DormantEnd( void ) {
	if ( enemy.ent && !enemyNode.InList() ) {
		// let our enemy know we're back on the trail
		idActor *enemyEnt = dynamic_cast< idActor *>( enemy.ent.GetEntity() );
		if( enemyEnt ){
			enemyNode.AddToEnd( enemyEnt->enemyList );
		}
	}
	idActor::DormantEnd();
}

/*
=====================
idAI::ValidateCover

Validate the reserved cover feature with current conditions.  Also quickly tests if
cover does not face enemy at all, which triggers an instant update (ignoring maxCoverInvalidTime)
=====================
*/
bool idAI::ValidateCover( ) {
	if ( !InCoverMode( ) ) {
		return false;
	}
	
	if ( aasSensor->TestValidWithCurrentState() && (!enemy.ent || enemy.range > combat.awareRange ) ) {
		combat.coverValidTime = gameLocal.time;
	} else if ( combat.coverValidTime && !IsCoverValid ( ) ) {
		combat.coverValidTime = 0;
		OnCoverInvalidated();
	}

	return IsCoverValid();
}

/*
=====================
idAI::DoDormantTests
=====================
*/
bool idAI::DoDormantTests ( void ) {
	// If in a scripted move that we should never go dormant in
	if ( aifl.scripted && aifl.scriptedNeverDormant ) {
		return false;
	}
	// If following a player never go dormant
	if ( leader && leader->IsType ( idPlayer::GetClassType ( ) ) ) {
		return false;
	}
	// AI should no longer go dormant when outside of tether
	if ( IsTethered () && !IsWithinTether ( ) ) {
		return false;
	}
	return idActor::DoDormantTests ( );
}

/*
=====================
idAI::Think
=====================
*/
void idAI::Think( void ) {

	// if we are completely closed off from the player, don't do anything at all
	if ( CheckDormant() ) {
		return;
	}

	// Simple think this frame?
	aifl.simpleThink = aiManager.IsSimpleThink ( this );

	aiManager.thinkCount++;

	// AI Speeds
	if ( ai_speeds.GetBool ( ) ) {
		aiManager.timerThink.Start ( );
	}

	if ( thinkFlags & TH_THINK ) {	
		// clear out the enemy when he dies or is hidden
		idEntity* enemyEnt = enemy.ent;
		idActor*  enemyAct = dynamic_cast<idActor*>( enemyEnt );
		
		// Clear our enemy if necessary
		if ( enemyEnt ) {
			if (enemyAct && enemyAct->IsInVehicle()) {
				SetEnemy(enemyAct->GetVehicleController().GetVehicle());	// always get angry at the enemy's vehicle first, not the enemy himself
			} else {
				bool enemyDead = (enemyEnt->fl.takedamage && enemyEnt->health <= 0);
				if ( enemyDead || enemyEnt->fl.notarget || enemyEnt->IsHidden() || (enemyAct && enemyAct->team == team)) {
					ClearEnemy ( enemyDead );
				}
			}
		}

		// Action time is stopped when the torso is not idle
		if ( !aifl.action ) {
			actionTime += gameLocal.msec;
		}

		ValidateCover();

		move.current_yaw += deltaViewAngles.yaw;
		move.ideal_yaw = idMath::AngleNormalize180( move.ideal_yaw + deltaViewAngles.yaw );
		deltaViewAngles.Zero();

		if( move.moveType != MOVETYPE_PLAYBACK ){
			viewAxis = idAngles( 0, move.current_yaw, 0 ).ToMat3();
		}

		if ( !move.fl.allowHiddenMove && IsHidden() ) {
			// hidden monsters
			UpdateStates ();
		} else if( !ai_freeze.GetBool() ) {
			Prethink(); 

			// clear the ik before we do anything else so the skeleton doesn't get updated twice
			walkIK.ClearJointMods();

			// update enemy position if not dead
			if ( !aifl.dead ) {
				UpdateEnemy ( );
			}

			// update state machine
			UpdateStates();

			// run all movement commands
			Move();

			// if not dead, chatter and blink
			if( move.moveType != MOVETYPE_DEAD ){
				UpdateChatter();
				CheckBlink();
			}

			Postthink();
		} else {
			DrawTactical ( );
		}

		// clear pain flag so that we recieve any damage between now and the next time we run the script
		aifl.pain = false;
		aifl.damage = false;
		aifl.pushed = false;
		pusher = NULL;
	} else if ( thinkFlags & TH_PHYSICS ) {
		RunPhysics();
	}

	if ( move.fl.allowPushMovables ) {
		PushWithAF();
	}

	if ( fl.hidden && move.fl.allowHiddenMove ) {
		// UpdateAnimation won't call frame commands when hidden, so call them here when we allow hidden movement
		animator.ServiceAnims( gameLocal.previousTime, gameLocal.time );
	}

	aasSensor->Update();

	UpdateAnimation();
	Present();
	LinkCombat();

	// AI Speeds
	if ( ai_speeds.GetBool ( ) ) {
		aiManager.timerThink.Stop ( );
	}
}

/*
============
idAI::UpdateFocus
============
*/
void idAI::UpdateFocus ( const idMat3& orientationAxis ) {	
	// Alwasy look at enemy	
	if ( !allowJointMod || !allowEyeFocus ) {
		SetFocus ( AIFOCUS_NONE, 0 );
	} else if ( IsBehindCover() && !aifl.action && !IsSpeaking() ) {
		SetFocus ( AIFOCUS_NONE, 0 );
	} else if ( !aifl.scripted && (IsEnemyRecentlyVisible(2.0f) || gameLocal.GetTime()-lastAttackTime<1000) ) {
		//not scripted and: have an enemy OR we shot at an enemy recently (for when we kill an enemy in the middle of a volley)
		SetFocus ( AIFOCUS_ENEMY, 1000 );
	} else if ( move.fl.moving && InCoverMode ( ) && DistanceTo ( aasSensor->ReservedOrigin() ) < move.walkRange * 2.0f ) {
		SetFocus ( AIFOCUS_COVER, 1000 );
	} else if ( InLookAtCoverMode() ) {
		SetFocus ( AIFOCUS_COVERLOOK, 1000 );
	} else if ( talkTarget && gameLocal.time < passive.talkTime ) {
		SetFocus ( AIFOCUS_TALK, 100 );
	} else if ( lookTarget ) {
		SetFocus ( AIFOCUS_TARGET, 100 );
	} else if ( !IsBehindCover() && tether && DistanceTo ( move.moveDest ) < move.walkRange * 2.0f ) {
		SetFocus ( AIFOCUS_TETHER, 100 );
	} else if ( IsTethered() && (move.fl.moving || IsBehindCover()) ) {
		//tethered and at cover or moving - don't look at leader or player
		SetFocus ( AIFOCUS_NONE, 0 );
	} else if ( helperCurrent && helperCurrent->IsCombat ( ) ) {
		SetFocus ( AIFOCUS_HELPER, 100 );		
	} else if ( !aifl.scripted && !move.fl.moving ) {
		idEntity*	lookat   = NULL;
		aiFocus_t	newfocus = AIFOCUS_NONE;
		
		if ( leader ) {
			idVec3 dir2Leader = leader->GetPhysics()->GetOrigin()-GetPhysics()->GetOrigin();
			if ( (viewAxis[0] * dir2Leader) > 0.0f ) {
				//leader is in front of me
				newfocus  = AIFOCUS_LEADER;
				lookat	  = leader;
			} else if ( focusType == AIFOCUS_LEADER ) {
				//stop looking at him!
				SetFocus ( AIFOCUS_NONE, 0 );
			}
		} else if ( aifl.lookAtPlayer && !move.fl.moving ) {
			newfocus  = AIFOCUS_PLAYER;
			lookat	  = gameLocal.GetLocalPlayer();
		}
		
		if ( newfocus != AIFOCUS_NONE && lookat ) {
			if ( DistanceTo ( lookat ) < focusRange * (move.fl.moving?2.0f:1.0f) && CheckFOV ( lookat->GetPhysics()->GetOrigin() ) ) {
				SetFocus ( newfocus, 1000 );
			}		
		}
	}	
	
	// Make sure cover look wasnt invalidated	
	if ( focusType == AIFOCUS_COVERLOOK && !aasSensor->Look() ) {
		focusType = AIFOCUS_NONE;
	} else if ( focusType == AIFOCUS_COVER && !aasSensor->Reserved ( ) ) {
		focusType = AIFOCUS_NONE;
	} else if ( focusType == AIFOCUS_HELPER && !helperCurrent ) {
		focusType = AIFOCUS_NONE;
	} else if ( focusType == AIFOCUS_TETHER && !tether ) {
		focusType = AIFOCUS_NONE;
	}
	
	// Update focus type to none when the focus time runs out
	if ( focusType != AIFOCUS_NONE ) {
		if ( gameLocal.time > focusTime ) {
			focusType = AIFOCUS_NONE;
		}	
	}

	// Calculate the focus position
	if ( focusType == AIFOCUS_NONE ) {
		currentFocusPos = GetEyePosition() + orientationAxis[ 0 ] * 64.0f;
	} else if ( focusType == AIFOCUS_COVER ) {
		currentFocusPos = GetEyePosition() + aasSensor->Reserved()->Normal() * 64.0f;
	} else if ( focusType == AIFOCUS_COVERLOOK ) {
		currentFocusPos = GetEyePosition() + aasSensor->Look()->Normal() * 64.0f;
	} else if ( focusType == AIFOCUS_ENEMY ) {
		currentFocusPos = enemy.lastVisibleEyePosition;
	} else if ( focusType == AIFOCUS_HELPER ) {
		currentFocusPos = helperCurrent->GetPhysics()->GetOrigin() + helperCurrent->GetDirection ( this );
	} else if ( focusType == AIFOCUS_TETHER ) {
		currentFocusPos = GetEyePosition() + tether->GetPhysics()->GetAxis()[0] * 64.0f;
	} else { 
		idEntity* focusEnt = NULL;
		switch ( focusType ) {
			case AIFOCUS_LEADER:		focusEnt = leader;						break;
			case AIFOCUS_PLAYER:		focusEnt = gameLocal.GetLocalPlayer();	break;
			case AIFOCUS_TALK:			focusEnt = talkTarget;					break;
			case AIFOCUS_TARGET:		focusEnt = lookTarget;					break;
		}
		if ( focusEnt ) {
			if ( focusEnt->IsType ( idActor::GetClassType ( ) ) ) {
				currentFocusPos = static_cast<idActor*>(focusEnt)->GetEyePosition() - eyeVerticalOffset * focusEnt->GetPhysics()->GetGravityNormal();
			} else {
				currentFocusPos = focusEnt->GetPhysics()->GetOrigin();
			}
		} else { 
			currentFocusPos = GetEyePosition() + orientationAxis[ 0 ] * 64.0f;
		}
	}

	//draw it so we can see what they think they're looking at!
	if ( DebugFilter(ai_debugMove) ) { // Cyan & Blue = currentFocusPos
		if ( focusType != AIFOCUS_NONE ) {
			gameRenderWorld->DebugArrow( colorCyan, GetEyePosition(), currentFocusPos, 0 );
		} else {
			gameRenderWorld->DebugArrow( colorBlue, GetEyePosition(), currentFocusPos, 0 );
		}
	}
}

/*
=====================
idAI::UpdateStates
=====================
*/
void idAI::UpdateStates ( void ) {
	MEM_SCOPED_TAG(tag,MA_DEFAULT);

	// Continue updating tactical state if for some reason we dont have one 
	if ( !aifl.dead && !aifl.scripted && !aifl.action && stateThread.IsIdle ( ) && aifl.scriptedEndWithIdle ) {
		UpdateTactical ( 0 );
	} else {
		UpdateState();
	}

	// clear the hit enemy flag so we catch the next time we hit someone
	aifl.hitEnemy = false;

	if ( move.fl.allowHiddenMove || !IsHidden() ) {
		// update the animstate if we're not hidden
		UpdateAnimState();
	}
}

/*
=====================
idAI::OnStateChange
=====================
*/
void idAI::OnStateChange ( int channel ) {
}

/*
=====================
idAI::OverrideFlag
=====================
*/
void idAI::OverrideFlag ( aiFlagOverride_t flag, bool value ) {
	bool oldValue;
	
	switch ( flag ) {
		case AIFLAGOVERRIDE_DISABLEPAIN:	oldValue = disablePain;				break;
		case AIFLAGOVERRIDE_DAMAGE:			oldValue = fl.takedamage;			break;
		case AIFLAGOVERRIDE_NOTURN:			oldValue = move.fl.noTurn;		break;
		case AIFLAGOVERRIDE_NOGRAVITY:		oldValue = move.fl.noGravity;	break;
		default:
			return;
	}

	if ( oldValue == value ) {
		return;
	}
	
	int flagOverride		= 1 << (flag * 2);
	int flagOverrideValue	= 1 << ((flag * 2) + 1);
	
	if ( (flagOverrides & flagOverride) && !!(flagOverrides & flagOverrideValue) == value ) {
		flagOverrides &= ~flagOverride;
	} else {
		if ( oldValue ) {
			flagOverrides |= flagOverrideValue;
		} else {
			flagOverrides &= ~flagOverrideValue;
		}
		flagOverrides |= flagOverride;
	}
	
	switch ( flag ) {
		case AIFLAGOVERRIDE_DISABLEPAIN:	disablePain = value;			break;
		case AIFLAGOVERRIDE_DAMAGE:			fl.takedamage = value;			break;
		case AIFLAGOVERRIDE_NOTURN:			move.fl.noTurn = value;		break;
		case AIFLAGOVERRIDE_NOGRAVITY:		move.fl.noGravity = value;	break;
	}
}

/*
=====================
idAI::RestoreFlag
=====================
*/
void idAI::RestoreFlag ( aiFlagOverride_t flag ) {
	int flagOverride		= 1 << (flag * 2);
	int flagOverrideValue	= 1 << ((flag * 2) + 1);
	bool value;
	
	if ( !(flagOverrides&flagOverride) ) {
		return;
	}
	
	value = !!(flagOverrides & flagOverrideValue);

	switch ( flag ) {
		case AIFLAGOVERRIDE_DISABLEPAIN:	disablePain = value;			break;
		case AIFLAGOVERRIDE_DAMAGE:			fl.takedamage = value;			break;
		case AIFLAGOVERRIDE_NOTURN:			move.fl.noTurn = value;		break;
		case AIFLAGOVERRIDE_NOGRAVITY:		move.fl.noGravity = value;	break;
	}
}	

/***********************************************************************

	Damage

***********************************************************************/

/*
=====================
idAI::ReactionTo
=====================
*/
int idAI::ReactionTo( const idEntity *ent ) {

	if ( ent->fl.hidden ) {
		// ignore hidden entities
		return ATTACK_IGNORE;
	}

	if ( !ent->IsType( idActor::GetClassType() ) ) {
		return ATTACK_IGNORE;
	}

	if( combat.fl.ignoreEnemies ){
		return ATTACK_IGNORE;
	}

	const idActor *actor = static_cast<const idActor *>( ent );
	if ( actor->IsType( idPlayer::GetClassType() ) && static_cast<const idPlayer *>(actor)->noclip ) {
		// ignore players in noclip mode
		return ATTACK_IGNORE;
	}

	// actors on different teams will always fight each other
	if ( actor->team != team ) {
		if ( actor->fl.notarget ) {
			// don't attack on sight when attacker is notargeted
			return ATTACK_ON_DAMAGE; /* | ATTACK_ON_ACTIVATE ; */
		}
		return ATTACK_ON_SIGHT | ATTACK_ON_DAMAGE; /* | ATTACK_ON_ACTIVATE; */
	}

	//FIXME: temporarily disabled monsters of same rank fighting because it's happening too much.
	// monsters will fight when attacked by lower or equal ranked monsters.  rank 0 never fights back.
	//if ( rank && ( actor->rank <= rank ) ) {
	if ( rank && ( actor->rank < rank ) ) {
		return ATTACK_ON_DAMAGE;
	}

	// don't fight back
	return ATTACK_IGNORE;
}

/*
=====================
idAI::AdjustHealthByDamage
=====================
*/
void idAI::AdjustHealthByDamage	( int damage ) {
	if ( aifl.undying ) {
		return;
	}	
	idActor::AdjustHealthByDamage ( damage );

	if ( g_perfTest_aiUndying.GetBool() && health <= 0 ) {
		//so we still take pain!
		health = 1;
	}
}

/*
=====================
idAI::Pain
=====================
*/
bool idAI::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	aifl.pain   = idActor::Pain( inflictor, attacker, damage, dir, location );
	aifl.damage = true;

	// force a blink
	blink_time = 0;

	// No special handling if friendly fire
	// jshepard: friendly fire will cause pain. Players will only be able to pain buddy marines
	// via splash damage.

/*
	if ( attacker && attacker->IsType ( idActor::GetClassType ( ) ) ) {
		if ( static_cast<idActor*>( attacker )->team == team ) {
			return aifl.pain;
		}
	}
*/

	// ignore damage from self
	if ( attacker != this ) {
		// React to taking pain
		ReactToPain ( attacker, damage );

		pain.takenThisFrame += damage;
		pain.lastTakenTime = gameLocal.time;
		combat.tacticalPainTaken += damage;

		// If taken too much pain where we then skip the current destination
		if ( combat.tacticalPainThreshold && combat.tacticalPainTaken > combat.tacticalPainThreshold ) {
			if (team==AITEAM_STROGG && 
				(combat.tacticalMaskAvailable&AITACTICAL_COVER_BITS) &&
				IsType(rvAITactical::GetClassType()) && 
				ai_allowTacticalRush.GetBool() && 
				spawnArgs.GetBool("rushOnPain", "1")) {

				// clear any tether
				tether = NULL;

				// change ranged distances
				combat.attackRange[0] = 0.0f;
				combat.attackRange[1] = 100.0f;	// make them get really close

				// remove cover behavior
				combat.tacticalMaskAvailable &= ~AITACTICAL_COVER_BITS;

				// add ranged behavior
				combat.tacticalMaskAvailable |=  AITACTICAL_RANGED_BITS;
			}
			ForceTacticalUpdate ( );
			return true;
		}

		// If looping pain and a new paintype comes in we can stop the loop
		if ( pain.loopEndTime && pain.loopType.Icmp ( painType ) ) {
			pain.loopEndTime = 0;
			pain.loopType = painType;
		}

		ExecScriptFunction( funcs.damage );
	}

	// If we were hit by our enemy and our enemy isnt visible then 
	// force the enemy visiblity to be updated as if we saw him momentarily
	if ( enemy.ent == attacker && !enemy.fl.visible ) {
		UpdateEnemyPosition ( true );
	}	

	return aifl.pain;
}

/*
=====================
idAI::Killed
=====================
*/
void idAI::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	idAngles			ang;
	const char*			modelDeath;
	const idKeyValue*	kv;
	
	if ( g_debugDamage.GetBool() ) {
		gameLocal.Printf( "Damage: joint: '%s', zone '%s'\n", animator.GetJointName( ( jointHandle_t )location ), 
			GetDamageGroup( location ) );
	}

	if ( aifl.dead ) {
		aifl.pain = true;
		aifl.damage = true;
		return;
	}

	aifl.dead = true;

	// turn off my flashlight, if I had one
	ProcessEvent( &AI_Flashlight, false );

	// Detach from any spawners
	if( GetSpawner() ) {
		GetSpawner()->Detach( this );
		SetSpawner( NULL );
	}

	// Hide surfaces on death
	for ( kv = spawnArgs.MatchPrefix ( "deathhidesurface", NULL );
		  kv;
		  kv = spawnArgs.MatchPrefix ( "deathhidesurface", kv ) ) {
		HideSurface ( kv->GetValue() );
	}

	// stop all voice sounds 
	StopSpeaking( true );

	SetMoveType ( MOVETYPE_DEAD );

	move.fl.noGravity = false;
	move.fl.allowPushMovables = false;
	aifl.scripted = false;

	physicsObj.UseFlyMove( false );
	physicsObj.ForceDeltaMove( false );

	// end our looping ambient sound
	StopSound( SND_CHANNEL_AMBIENT, false );

	if ( attacker && attacker->IsType( idActor::GetClassType() ) ) {
		gameLocal.AlertAI( ( idActor * )attacker );

		aiManager.AnnounceKill ( this, attacker, inflictor );
		aiManager.AnnounceDeath ( this, attacker );
   	}

	if ( attacker && attacker->IsType( idActor::GetClassType() ) ) {
		gameLocal.AlertAI( ( idActor * )attacker );
	}

	// activate targets
	ActivateTargets( this );

	RemoveAttachments();
	RemoveProjectile();
	StopMove( MOVE_STATUS_DONE );

	OnDeath();
	CheckDeathObjectives();

	ClearEnemy();

	// make monster nonsolid
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();

	Unbind();
	if ( g_perfTest_aiNoRagdoll.GetBool() ) {
		if ( spawnArgs.MatchPrefix( "lipsync_death" ) ) {
			Speak( "lipsync_death", true );
		} else {
			StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
		}
		physicsObj.SetLinearVelocity( vec3_zero );
		physicsObj.PutToRest();
		physicsObj.DisableImpact();

	} else if( fl.quickBurn ){
		if ( spawnArgs.MatchPrefix( "lipsync_death" ) ) {
			Speak( "lipsync_death", true );
		} else {
			StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
		}

		physicsObj.SetLinearVelocity( vec3_zero );
		physicsObj.PutToRest();
		physicsObj.DisableImpact();

	} else {
		if ( StartRagdoll() ) {
			if ( spawnArgs.MatchPrefix( "lipsync_death" ) ) {
				Speak( "lipsync_death", true );
			} else {
				StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
			}
		}

		if ( spawnArgs.GetString( "model_death", "", &modelDeath ) ) {
			// lost soul is only case that does not use a ragdoll and has a model_death so get the death sound in here
			if ( spawnArgs.MatchPrefix( "lipsync_death" ) ) {
				Speak( "lipsync_death", true );
			} else {
				StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
			}
			renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
			SetModel( modelDeath );
			physicsObj.SetLinearVelocity( vec3_zero );
			physicsObj.PutToRest();
			physicsObj.DisableImpact();
		}
	}

	SetState ( "State_Killed" );

	kv = spawnArgs.MatchPrefix( "def_drops", NULL );
	while( kv ) {
		idDict args;
		idEntity *tEnt;
		if( kv->GetValue() != "" ){
			args.Set( "classname", kv->GetValue() );
			args.Set( "origin", physicsObj.GetAbsBounds().GetCenter().ToString() );
			// Let items know that they are of the dropped variety
			args.Set( "dropped", "1" );
			if (gameLocal.SpawnEntityDef( args, &tEnt )) {
				if ( tEnt && tEnt->GetPhysics()) { //tEnt *should* be valid, but hey...
					// magic/arbitrary number to give it some spin.  Some constants used to ensure guns rarely fall standing up
					tEnt->GetPhysics()->SetAngularVelocity( idVec3( (gameLocal.random.RandomFloat() * 10.0f) + 20.0f,
																	(gameLocal.random.RandomFloat() * 10.0f) + 20.0f,
																	(gameLocal.random.RandomFloat() * 10.0f) + 20.0f));
				}
			}
		}
		kv = spawnArgs.MatchPrefix( "def_drops", kv );
	}
}

/***********************************************************************

	Targeting/Combat

***********************************************************************/

/*
=====================
idAI::Activate

Notifies the script that a monster has been activated by a trigger or flashlight
=====================
*/
void idAI::Activate( idEntity *activator ) {
	idPlayer *player;

	// Set our tether?
	if ( activator && activator->IsType ( rvAITether::GetClassType ( ) ) ) {
		SetTether ( static_cast<rvAITether*>(activator) );
	}

	if ( aifl.dead ) {
		// ignore it when they're dead
		return;
	}

	// make sure he's not dormant
	dormantStart = 0;

	aifl.activated = true;
	if ( !activator || !activator->IsType( idPlayer::GetClassType() ) ) {
		player = gameLocal.GetLocalPlayer();
	} else {
		player = static_cast<idPlayer *>( activator );
	}

	if ( ReactionTo( player ) & ATTACK_ON_ACTIVATE ) {
		SetEnemy( player );
	}
	
	// If being activated by a spawner we need to attach to it
	if ( activator && activator->IsType ( rvSpawner::GetClassType() ) ) {
		rvSpawner* spawn = static_cast<rvSpawner*>( activator );
		spawn->Attach ( this );
		SetSpawner ( spawn );
	}
}

/*
=====================
idAI::TalkTo
=====================
*/
void idAI::TalkTo( idActor *actor ) {

	// jshepard: the dead do not speak.
	if ( aifl.dead )
		return;

	ExecScriptFunction( funcs.onclick );

	// Cant talk when already talking
	if ( aifl.action || IsSpeaking ( ) || !aiManager.CheckTeamTimer ( team, AITEAMTIMER_ACTION_TALK ) ) {
		return;
	}


	switch ( GetTalkState ( ) ) {
		case TALK_OK:
			if ( talkMessage >= TALKMSG_LOOP ) {
				//MCG: requested change: stop responding after one loop
				return;
			}
			// at least one second between talking to people 
			aiManager.SetTeamTimer ( team, AITEAMTIMER_ACTION_TALK, 2000 );
		
			talkTarget = actor;

			// Move to the next talk message
			talkMessage = (talkMessage_t)((int)talkMessage + 1);
			
			// Loop until we find a valid talk message
			while ( 1 ) {
				idStr postfix;
				if ( talkMessage >= TALKMSG_LOOP ) {
					postfix = aiTalkMessageString[TALKMSG_LOOP];
					postfix += va("%d", (int)(talkMessage - TALKMSG_LOOP+1) );
				} else {
					postfix = aiTalkMessageString[talkMessage];
				}
				// Try the lipsync for the specific passive prefix first
				if ( Speak ( va("lipsync_%stalk_%s", passive.prefix.c_str(), postfix.c_str() ) ) ) {
					break;
				}
				// Try the generic lipsync for the current talk message
				if ( Speak ( va("lipsync_talk_%s", postfix.c_str() ) ) ) {
					break;
				}
				// If the first loop failed then there is no other options
				if ( talkMessage == TALKMSG_LOOP ) {
					return;
				} else if ( talkMessage >= TALKMSG_LOOP ) {
					talkMessage = TALKMSG_LOOP;
				} else {
					talkMessage = (talkMessage_t)((int)talkMessage + 1);
				}
			}
			
			// Start talking anim if we have one
			if ( IsSpeaking ( ) ) {
				passive.talkTime = speakTime;
			}
			
			break;
		
		case TALK_FOLLOW:
			if ( actor == leader ) {
				leader = NULL;
				Speak ( "lipsync_stopfollow", true );
			} else {
				leader = actor;
				Speak ( "lipsync_follow", true );
			}
			break;
			
		case TALK_BUSY:
			if ( talkBusyCount > 3 )
			{//only say a max of 4 lines
				return;
			}
			talkBusyCount++;
			//try to say this variant - if we can't, stop forever
			if ( !Speak ( va("lipsync_busy_%d",talkBusyCount) ) )
			{//nevermore
				talkBusyCount = 999;
			}
			break;

		case TALK_WAIT:
			//do nothing
			break;
	}
}

/*
=====================
idAI::GetTalkState
=====================
*/
talkState_t idAI::GetTalkState( void ) const {
	if ( ( talkState != TALK_NEVER ) && aifl.dead ) {
		return TALK_DEAD;
	}
	if ( IsHidden() ) {
		return TALK_NEVER;
	}
	if ( (talkState == TALK_OK || talkState == TALK_FOLLOW) && aifl.scripted ) {
		return TALK_BUSY;
	}
	return talkState;
}

/*
=====================
idAI::TouchedByFlashlight
=====================
*/
void idAI::TouchedByFlashlight( idActor *flashlight_owner ) {
	if ( RespondToFlashlight() ) {
		Activate( flashlight_owner );
	}
}

/*
=====================
idAI::ClearEnemy
=====================
*/
void idAI::ClearEnemy( bool dead ) {
	// Dont bother if we dont have an enemy
	if ( !enemy.ent ) {
		return;
	}

	if ( move.moveCommand == MOVE_TO_ENEMY ) {
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	} else if ( !aifl.scripted
		&& move.moveCommand == MOVE_TO_COVER
		&& !move.fl.done
		&& aasSensor->Reserved()
		&& !IsBehindCover() ) {
		//clear cover and stop move
		aasSensor->Reserve( NULL );
		combat.coverValidTime = 0;
		OnCoverInvalidated();
		ForceTacticalUpdate();
		StopMove( MOVE_STATUS_DONE );
	}

	enemyNode.Remove();
	enemy.ent			= NULL;
	enemy.fl.dead		= dead;	
	combat.fl.seenEnemyDirectly = false;
	
	UpdateHelper ( );
}

/*
=====================
idAI::UpdateEnemyPosition
=====================
*/
void idAI::UpdateEnemyPosition ( bool forceUpdate ) {
	if( !enemy.ent || (!enemy.fl.visible && !forceUpdate) ) {
		return;
	}

	idActor*  enemyActor = dynamic_cast<idActor*>(enemy.ent.GetEntity());
	idEntity* enemyEnt   = static_cast<idEntity*>(enemy.ent.GetEntity());

	enemy.lastVisibleFromEyePosition = GetEyePosition ( );

	// Update the enemy origin if it isnt locked
	if ( !enemy.fl.lockOrigin ) {
		enemy.lastKnownPosition	= enemyEnt->GetPhysics()->GetOrigin();

		if ( enemyActor ) {				
			enemy.lastVisibleEyePosition	= enemyActor->GetEyePosition();
			enemy.lastVisibleChestPosition	= enemyActor->GetChestPosition();
		} else {
			enemy.lastVisibleEyePosition	= enemy.ent->GetPhysics()->GetOrigin ( );
			enemy.lastVisibleChestPosition	= enemy.ent->GetPhysics()->GetOrigin ( );
		}		
	}	
}

/*
=====================
idAI::UpdateEnemy
=====================
*/
void idAI::UpdateEnemy ( void ) {
	predictedPath_t predictedPath;
	
	// If we lost our enemy then clear it out to be sure
	if( !enemy.ent ) {
		return;
	}

	// Rest to not being in fov
	enemy.fl.inFov = false;

	// Bail out if we arent queued to do a complex think
	if ( aifl.simpleThink ) {
		// Keep the fov flag up to date
		if ( IsEnemyVisible ( ) ) { 
			enemy.fl.inFov = CheckFOV( enemy.lastKnownPosition );
		}
	} else { 		
		bool oldVisible;
	
		// Cache current enemy visiblity so we can see if it changes
		oldVisible = IsEnemyVisible ( );
	
		// See if enemy still visible
		UpdateEnemyVisibility  ( );

		// If our enemy isnt visible but is within aware range then we know where he is
		if ( !IsEnemyVisible ( ) && DistanceTo ( enemy.ent ) < combat.awareRange ) {
			UpdateEnemyPosition( true );
		} else {
/*		
			// check if we heard any sounds in the last frame
			if ( enemyEnt == gameLocal.GetAlertActor() ) {
				float dist = ( enemyEnt->GetPhysics()->GetOrigin() - physicsObj.GetOrigin() ).LengthSqr();
				if ( dist < Square( combat.earRange ) ) {
					SetEnemyPosition();
				}
			}
*/
		}

		// Update fov
		enemy.fl.inFov = CheckFOV( enemy.lastKnownPosition );

		// Set enemy.visibleTime to the time when the visibility flag changed
		if ( oldVisible != IsEnemyVisible ( ) ) {
			// Just caught sight of the enemy after loosing him, delay the attack actions a bit
			if ( !oldVisible && combat.attackSightDelay ) {
				float delay;
				if ( enemy.lastVisibleChangeTime ) {
					// The longer the enemy has not been visible, the more we should delay
					delay = idMath::ClampFloat ( 0.0f, 1.0f, (gameLocal.time - enemy.lastVisibleChangeTime) / AI_SIGHTDELAYSCALE );
				} else {
					// The enemy has never been visible so delay the full amount
					delay = 1.0f;
				}
				// Add to the ranged attack timer providing its not already running a timer
				if ( actionTimerRangedAttack.IsDone ( actionTime) ) {
					actionTimerRangedAttack.Clear ( actionTime );
					actionTimerRangedAttack.Add ( combat.attackSightDelay * delay, 0.5f );
				}
				// Add to the special attack timer providing its not already running a timer
				if ( actionTimerSpecialAttack.IsDone ( actionTime ) ) {
					actionTimerSpecialAttack.Clear ( actionTime );		
					actionTimerSpecialAttack.Add ( combat.attackSightDelay * delay, 0.5f );
				}
			}

			enemy.lastVisibleChangeTime = gameLocal.time;
		}
		
		// Handler for visibility change
		if ( oldVisible != IsEnemyVisible ( ) ) {
			OnEnemyVisiblityChange ( oldVisible );
		}
	}

	// Adjust smoothed linear and pushed velocities for enemies
	if ( enemy.fl.visible ) {
		enemy.smoothedLinearVelocity += ((enemy.ent->GetPhysics()->GetLinearVelocity ( ) - enemy.ent->GetPhysics()->GetPushedLinearVelocity ( ) - enemy.smoothedLinearVelocity) * enemy.smoothVelocityRate );
 		enemy.smoothedPushedVelocity += ((enemy.ent->GetPhysics()->GetPushedLinearVelocity ( ) - enemy.smoothedPushedVelocity) * enemy.smoothVelocityRate );
 	} else {
		enemy.smoothedLinearVelocity -= (enemy.smoothedLinearVelocity * enemy.smoothVelocityRate );
 		enemy.smoothedPushedVelocity -= (enemy.smoothedPushedVelocity * enemy.smoothVelocityRate );
	} 	
	
	// Update enemy range
	enemy.range	  = DistanceTo ( enemy.lastKnownPosition );
	enemy.range2d = DistanceTo2d ( enemy.lastKnownPosition );

	// Calulcate the aggression scale
	// Skill level 0: (1.0)
	// Skill level 1: (1.0 - 1.25)
	// Skill level 2: (1.0 - 1.5)
	// Skill level 3: (1.0 - 1.75)
	if ( combat.aggressiveRange > 0.0f ) {
		combat.aggressiveScale = (g_skill.GetFloat() / MAX_SKILL_LEVELS);
		combat.aggressiveScale *= 1.0f - idMath::ClampFloat ( 0.0f, 1.0f, enemy.range / combat.aggressiveRange );
		combat.aggressiveScale += 1.0f;
	}
}

/*
=====================
idAI::LastKnownPosition
=====================
*/
const idVec3& idAI::LastKnownPosition ( const idEntity *ent ) {
	return	(ent==enemy.ent)?(enemy.lastKnownPosition):(ent->GetPhysics()->GetOrigin());
}

/*
=====================
idAI::UpdateEnemyVisibility

Update whether or not the AI can see its enemy or not and determine how.
=====================
*/
void idAI::UpdateEnemyVisibility ( void ) {
	enemy.fl.visible = false;

	// No enemy so nothing to see
	if ( !enemy.ent ) {
		return;
	}

	// Update enemy visibility flag
	enemy.fl.visible = CanSeeFrom ( GetEyePosition ( ), enemy.ent, false );

	// IF the enemy isnt visible and not forcing an update we can just early out
	if ( !enemy.fl.visible ) {
		return;
	}
	
	// If we are seeing our enemy for the first time after changing enemies, call him out
	if ( enemy.lastVisibleTime < enemy.changeTime ) {
		AnnounceNewEnemy( );
	}
	
	combat.fl.seenEnemyDirectly = true;
	enemy.lastVisibleTime		= gameLocal.time;
	
	// Update our known locations of the enemy since we just saw him
	UpdateEnemyPosition ( );
}

/*
=====================
idAI::SetSpawner
=====================
*/
void idAI::SetSpawner ( rvSpawner* _spawner ) {
	spawner = _spawner;
}

/*
=====================
idAI::SetSpawner
=====================
*/
rvSpawner* idAI::GetSpawner ( void ) {
	return spawner;
}

/*
=====================
idAI::SetEnemy
=====================
*/
bool idAI::SetEnemy( idEntity *newEnemy ) {
	idEntity*	oldEnemy;

	// Look for obvious early out
	if ( enemy.ent == newEnemy ) {
		return true; 
	}

	// Cant set enemy if dead
	if ( aifl.dead ) {
		ClearEnemy ( false );
		return false;
	}

	// Cant set enemy if the enemy is dead or has no target set
	if ( newEnemy ) {
		if ( newEnemy->health <= 0 || newEnemy->fl.notarget ) {
			return false;
		}
	}

	oldEnemy  			= enemy.ent;
	enemy.fl.dead		= false;
	enemy.fl.lockOrigin	= false;

	if ( !newEnemy ) {
		ClearEnemy ( false );
	} else {
		// Set our current enemy
		enemy.ent = newEnemy;

		// If the enemy is an actor then add to the actors enemy list
		if( newEnemy->IsType( idActor::GetClassType() ) ){
			idActor* enemyActor = static_cast<idActor*>( newEnemy );
			enemyNode.AddToEnd( enemyActor->enemyList );
		}
	}
	
	// Allow custom handling of enemy change
	if ( enemy.ent != oldEnemy ) {
		OnEnemyChange ( oldEnemy );
	}
	
	return true;
}


/*
===================
idAI::CalculateAttackOffsets

calculate joint positions on attack frames so we can do proper "can hit" tests
===================
*/
void idAI::CalculateAttackOffsets ( void ) {
	const idDeclModelDef*	modelDef;
	int						num;
	int						i;
	int						frame;
	const frameCommand_t*	command;
	idMat3					axis;
	const idAnim*			anim;
	jointHandle_t			joint;

	modelDef = animator.ModelDef();
	if ( !modelDef ) {
		return;
	}
	num = modelDef->NumAnims();
	
	// needs to be off while getting the offsets so that we account for the distance the monster moves in the attack anim
	animator.RemoveOriginOffset( false );

	// anim number 0 is reserved for non-existant anims.  to avoid off by one issues, just allocate an extra spot for
	// launch offsets so that anim number can be used without subtracting 1.
	attackAnimInfo.SetGranularity( 1 );
	attackAnimInfo.SetNum( num + 1 );
	attackAnimInfo[ 0 ].attackOffset.Zero();
	attackAnimInfo[ 0 ].eyeOffset.Zero();

	for( i = 1; i <= num; i++ ) {
		attackAnimInfo[ i ].attackOffset.Zero();
		attackAnimInfo[ i ].eyeOffset.Zero();
		anim = modelDef->GetAnim( i );
		if ( !anim ) {
			continue;
		}
		
		frame = anim->FindFrameForFrameCommand( FC_AI_ATTACK, &command );
		if ( frame >= 0 ) {
			joint = animator.GetJointHandle( command->joint->c_str() );
			if ( joint == INVALID_JOINT ) {
				gameLocal.Error( "Invalid joint '%s' on 'ai_attack' frame command on frame %d of model '%s'", command->joint->c_str(), frame, modelDef->GetName() );
			}
			GetJointTransformForAnim( joint, i, FRAME2MS( frame ), attackAnimInfo[ i ].attackOffset, axis );
			
			if ( chestOffsetJoint!= INVALID_JOINT ) {
				GetJointTransformForAnim( chestOffsetJoint, i, FRAME2MS( frame ), attackAnimInfo[ i ].eyeOffset, axis );
			}
		}
	}

	animator.RemoveOriginOffset( true );
}

/*
=====================
idAI::CreateProjectileClipModel
=====================
*/
void idAI::CreateProjectileClipModel( void ) const {
	if ( projectileClipModel == NULL ) {
		idBounds projectileBounds( vec3_origin );
		projectileBounds.ExpandSelf( 2.0f ); // projectileRadius );
		projectileClipModel	= new idClipModel( idTraceModel( projectileBounds ) );
	}
}

/*
=====================
idAI::GetPredictedAimDirOffset
=====================
*/
void idAI::GetPredictedAimDirOffset ( const idVec3& source, const idVec3& target, float projectileSpeed, const idVec3& targetVelocity, idVec3& offset ) const {
	float  a;
	float  b;
	float  c;
	float  d;
	float  t;
	idVec3 r;
	
	// Make sure there is something to predict
	if ( targetVelocity.LengthSqr ( ) == 0.0f ) {
		offset.Zero ( );
		return;
	}
	
	// Solve for (t):  magnitude(targetVelocity * t + target - source) = projectileSpeed * t	
	
	r = (target-source);
	a = (targetVelocity * targetVelocity) - Square(projectileSpeed);
	b = (targetVelocity * 2.0f) * r;
	c = r*r;
	d = b*b - 4*a*c; 
	t = 0.0f;
	
	if ( d >= 0.0f ) {
		float  t1;
		float  t2;
		float  denom;
		d     = idMath::Sqrt(d);
		denom = 1.0f / (2.0f * a);
		t1    = (-b + d) * denom;
		t2    = (-b - d) * denom;
		if ( t1 < 0.0f && t2 < 0.0f ) {
			t = 0.0f;
		} else if ( t1 < 0.0f ) {
			t = t2;
		} else if ( t2 < 0.0f ) {
			t = t1;
		} else {
			t = Min(t1,t2);
		}
	}

	offset = targetVelocity * t;
}

/*
=====================
idAI::GetAimDir
=====================
*/
bool idAI::GetAimDir( 
	const idVec3&	firePos, 
	const idEntity*	target, 
	const idDict*	projectileDef, 
	idEntity*		ignore, 
	idVec3&			aimDir, 
	float			aimOffset,
	float			predict
	) const 
{
	idVec3		targetPos1;
	idVec3		targetPos2;
	idVec3		targetLinearVel;
	idVec3		targetPushedVel;
	idVec3		delta;
	float		max_height;
	bool		result;
	idEntity*	targetEnt = const_cast<idEntity*>(target);

	// if no aimAtEnt or projectile set
	if ( !targetEnt ) {
		aimDir = viewAxis[ 0 ] * physicsObj.GetGravityAxis();
		return false;
	}

	float	projectileSpeed   = idProjectile::GetVelocity ( projectileDef ).LengthFast ( );
	idVec3	projectileGravity = idProjectile::GetGravity ( projectileDef );

	if ( projectileClipModel == NULL ) {
		CreateProjectileClipModel();
	}

	if ( targetEnt == enemy.ent ) {
		targetPos2 = enemy.lastVisibleEyePosition;
		targetPos1 = enemy.lastVisibleChestPosition;
		targetPushedVel = enemy.smoothedPushedVelocity;
		targetLinearVel = enemy.smoothedLinearVelocity;
	} else if ( targetEnt->IsType( idActor::GetClassType() ) ) {
		targetPos2  = static_cast<idActor*>(targetEnt)->GetEyePosition ( );
		targetPos1  = static_cast<idActor*>(targetEnt)->GetChestPosition ( );
		targetPushedVel = target->GetPhysics()->GetPushedLinearVelocity ( );
		targetLinearVel = (target->GetPhysics()->GetLinearVelocity ( ) - targetPushedVel);
	} else {
		targetPos1 = targetEnt->GetPhysics()->GetAbsBounds().GetCenter();
		targetPos2 = targetPos1;
		targetPushedVel.Zero ( );
		targetLinearVel.Zero ( );
	}
	
	// Target prediction
	if ( projectileSpeed == 0.0f ) {
		// Hitscan prediction actually causes the hitscan to miss unless it is predicted.
		delta = (targetLinearVel * (-1.0f + predict));
	} else {
		// Projectile prediction must figure out how far to lead the shot to hit the target
		GetPredictedAimDirOffset ( firePos, targetPos1, projectileSpeed, (targetLinearVel*predict)+targetPushedVel, delta );
	}
	targetPos1 += delta;
	targetPos2 += delta;

	result = false;
	
	// try aiming for chest
	delta = targetPos1 - firePos;
	max_height = (delta.NormalizeFast ( ) + aimOffset) * projectile_height_to_distance_ratio;	
	if ( max_height > 0.0f ) {
		result = PredictTrajectory( firePos, targetPos1 + delta * aimOffset, projectileSpeed, projectileGravity, projectileClipModel, MASK_SHOT_RENDERMODEL, max_height, ignore, targetEnt, ai_debugTrajectory.GetBool() ? 1000 : 0, aimDir );
		if ( result || !targetEnt->IsType( idActor::GetClassType() ) ) {
			return result;
		}
	}

	// try aiming for head
	delta = targetPos2 - firePos;
	max_height = (delta.NormalizeFast ( ) + aimOffset) * projectile_height_to_distance_ratio;
	if ( max_height > 0.0f ) {
		result = PredictTrajectory( firePos, targetPos2 + delta * aimOffset, projectileSpeed, projectileGravity, projectileClipModel, MASK_SHOT_RENDERMODEL, max_height, ignore, targetEnt, ai_debugTrajectory.GetBool() ? 1000 : 0, aimDir );
	}

	return result;
}

/*
=====================
idAI::CreateProjectile
=====================
*/
idProjectile *idAI::CreateProjectile ( const idDict* projectileDict, const idVec3 &pos, const idVec3 &dir ) {
	idEntity*	ent;
	const char*	clsname;

	if ( !projectile.GetEntity() ) {
		gameLocal.SpawnEntityDef( *projectileDict, &ent, false );
		if ( !ent ) {
			clsname = projectileDict->GetString( "classname" );
			gameLocal.Error( "Could not spawn entityDef '%s'", clsname );
		}
		
		if ( !ent->IsType( idProjectile::GetClassType() ) ) {
			clsname = ent->GetClassname();
			gameLocal.Error( "'%s' is not an idProjectile", clsname );
		}
		projectile = (idProjectile*)ent;
	}

	projectile.GetEntity()->Create( this, pos, dir );

	return projectile.GetEntity();
}

/*
=====================
idAI::RemoveProjectile
=====================
*/
void idAI::RemoveProjectile( void ) {
	if ( projectile.GetEntity() ) {
		projectile.GetEntity()->PostEventMS( &EV_Remove, 0 );
		projectile = NULL;
	}
}

/*
=====================
idAI::Attack
=====================
*/
bool idAI::Attack ( const char* attackName, jointHandle_t joint, idEntity* target, const idVec3& pushVelocity ) {
	// Get the attack dictionary
	const idDict* attackDict;
	attackDict = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( va("def_attack_%s", attackName ) ), false );
	if ( !attackDict ) {
		gameLocal.Error ( "could not find attack entityDef 'def_attack_%s (%s)' on AI entity %s", attackName, spawnArgs.GetString ( va("def_attack_%s", attackName ) ), GetName ( ) );
	}

	// Melee Attack?
	if ( spawnArgs.GetBool ( va("attack_%s_melee", attackName ), "0" ) ) {
		return AttackMelee ( attackName, attackDict );
	}

	// Ranged attack (hitscan or projectile)?
	return ( AttackRanged ( attackName, attackDict, joint, target, pushVelocity ) != NULL );
}

/*
=====================
idAI::AttackRanged
=====================
*/
idProjectile* idAI::AttackRanged ( 
	const char*		attackName, 
	const idDict*	attackDict, 
	jointHandle_t	joint, 
	idEntity*		target, 
	const idVec3&	pushVelocity 
	)	 
{
	float				attack_accuracy;
	float				attack_cone;
	float				attack_spread;
	int					attack_count;
	bool				attack_hitscan;
	float				attack_predict;
	float				attack_pullback;
	int					i;
	idVec3				muzzleOrigin;
	idMat3				muzzleAxis;
	idVec3				dir;
	idAngles			ang;
	idMat3				axis;
	idProjectile*		lastProjectile;
	
	lastProjectile		= NULL;
	
	// Generic attack properties
	attack_accuracy		= spawnArgs.GetFloat ( va("attack_%s_accuracy", attackName ), "7" );
	attack_cone			= spawnArgs.GetFloat ( va("attack_%s_cone", attackName ), "75" );
	attack_spread		= DEG2RAD ( spawnArgs.GetFloat ( va("attack_%s_spread", attackName ), "0" ) );
	attack_count		= spawnArgs.GetInt ( va("attack_%s_count", attackName ), "1" );
	attack_hitscan		= spawnArgs.GetBool ( va("attack_%s_hitscan", attackName ), "0" );
	attack_predict		= spawnArgs.GetFloat ( va("attack_%s_predict", attackName ), "0" );
	attack_pullback		= spawnArgs.GetFloat ( va("attack_%s_pullback", attackName ), "0" );

	// Get the muzzle origin and axis from the given launch joint
	GetMuzzle( joint, muzzleOrigin, muzzleAxis );
	if ( attack_pullback )
	{
		muzzleOrigin -= muzzleAxis[0]*attack_pullback;
	}

	// set aiming direction
	bool calcAim = true;
	if ( GetEnemy() && GetEnemy() == target && GetEnemy() == gameLocal.GetLocalPlayer() && spawnArgs.GetBool( va("attack_%s_missFirstShot",attackName) ) ) {
		//purposely miss
		if ( gameLocal.random.RandomFloat() < 0.5f ) {
			//actually, hit anyway...
		} else {
			idVec3 targetPos = enemy.lastVisibleEyePosition;
			idVec3 eFacing;
			idVec3 left, up;
			
			up.Set( 0, 0, 1 );
			left = viewAxis[0].Cross(up);

			if ( GetEnemy()->IsType( idActor::GetClassType() ) )
			{
				eFacing = ((idActor*)GetEnemy())->viewAxis[0];
			}
			else
			{
				eFacing = GetEnemy()->GetPhysics()->GetAxis()[0];
			}
			if ( left*eFacing > 0 )
			{
				targetPos += left * ((gameLocal.random.RandomFloat()*8.0f) + 8.0f);
			}
			else
			{
				targetPos -= left * ((gameLocal.random.RandomFloat()*8.0f) + 8.0f);
			}
			dir = targetPos-muzzleOrigin;
			dir.Normalize();
			attack_accuracy = 0;

			calcAim = false;
			//don't miss next time
			spawnArgs.SetBool( va("attack_%s_missFirstShot",attackName), false );
		}
	}
	if ( calcAim ) {
		if ( target && !spawnArgs.GetBool ( va("attack_%s_lockToJoint",attackName), "0" ) ) {
			GetAimDir( muzzleOrigin, target, attackDict, this, dir, 
					spawnArgs.GetFloat ( va("attack_%s_aimoffset", attackName )),
					attack_predict );
		} else {
			dir = muzzleAxis[0];
			if ( spawnArgs.GetBool ( va("attack_%s_lockToJoint",attackName), "0" ) )
			{
				if ( spawnArgs.GetBool( va("attack_%s_traceToNextJoint", attackName ), "0" ) )
				{
					jointHandle_t endJoint = animator.GetFirstChild( joint );
					if ( endJoint != INVALID_JOINT && endJoint != joint )
					{
						idVec3 endJointPos;
						idMat3 blah;
						GetJointWorldTransform( endJoint, gameLocal.GetTime(), endJointPos, blah );
						dir = endJointPos-muzzleOrigin;
						//ARGHH: need to be able to set range!!!
						//const_cast<idDict*>(attackDict)->SetFloat( "range", dir.Normalize() );//NOTE: yes, I intentionally normalize here as well as use the result for length...
						dir.Normalize();
					}
				}
			}
		}
	}
	
	// random accuracy
	ang = dir.ToAngles();
	ang.pitch += gameLocal.random.CRandomFloat ( ) * attack_accuracy;
	ang.yaw   += gameLocal.random.CRandomFloat ( ) * attack_accuracy;

	// Lock attacks to a cone?
	if ( attack_cone ) {
		float diff;
		// clamp the attack direction to be within monster's attack cone so he doesn't do
		// things like throw the missile backwards if you're behind him
		diff = idMath::AngleDelta( ang.yaw, move.current_yaw );
		if ( diff > attack_cone ) {
			ang.yaw = move.current_yaw + attack_cone;
		} else if ( diff < -attack_cone ) {
			ang.yaw = move.current_yaw - attack_cone;
		}
	}

	ang.Normalize360 ( );
	axis = ang.ToMat3();

	for( i = 0; i < attack_count; i++ ) {
		float angle;
		float spin;
		
		// spread the projectiles out
		angle = idMath::Sin( attack_spread * gameLocal.random.RandomFloat() );
		spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
		dir = axis[ 0 ] + axis[ 2 ] * ( angle * idMath::Sin( spin ) ) - axis[ 1 ] * ( angle * idMath::Cos( spin ) );
		dir.Normalize();

		if ( attack_hitscan ) {
			gameLocal.HitScan( *attackDict, muzzleOrigin, dir, muzzleOrigin, this, false, combat.aggressiveScale );		
		} else {
			// launch the projectile
			if ( !projectile.GetEntity() ) {
				CreateProjectile( attackDict, muzzleOrigin, dir );
			}
			lastProjectile = projectile.GetEntity();
			lastProjectile->Launch( muzzleOrigin, dir, pushVelocity, 0.0f, combat.aggressiveScale );
		
			// Let the script manage projectiles if need be
			ExecScriptFunction ( funcs.launch_projectile, lastProjectile );
		
			projectile = NULL;
		}
	}

	lastAttackTime = gameLocal.time;

	// If shooting at another ai entity then kick off an attack reaction
	if ( enemy.ent && enemy.ent->IsType ( idAI::GetClassType() ) ) {
		static_cast<idAI*>(enemy.ent.GetEntity())->ReactToShotAt ( this, muzzleOrigin, axis[0] );
	}

	return lastProjectile;
}

/*s
================
idAI::DamageFeedback

callback function for when another entity recieved damage from this entity
================
*/
void idAI::DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage ) {
	if ( ( victim == this ) && inflictor->IsType( idProjectile::GetClassType() ) ) {
		// monsters only get half damage from their own projectiles
		damage = ( damage + 1 ) / 2;  // round up so we don't do 0 damage
	} else if ( victim == enemy.ent ) {
		aifl.hitEnemy = true;
	}
}

/*
=====================
idAI::DirectDamage

Causes direct damage to an entity

kickDir is specified in the monster's coordinate system, and gives the direction
that the view kick and knockback should go
=====================
*/
void idAI::DirectDamage( const char *meleeDefName, idEntity *ent ) {
	const idDict *meleeDef;
	const char *p;
	const idSoundShader *shader;

	meleeDef = gameLocal.FindEntityDefDict( meleeDefName, false );
	if ( !meleeDef ) {
		gameLocal.Error( "Unknown damage def '%s' on '%s'", meleeDefName, name.c_str() );
	}

	if ( !ent->fl.takedamage ) {
		const idSoundShader *shader = declManager->FindSound(meleeDef->GetString( "snd_miss" ));
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
		return;
	}

	//
	// do the damage
	//
	p = meleeDef->GetString( "snd_hit" );
	if ( p && *p ) {
		shader = declManager->FindSound( p );
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
	}

	idVec3	kickDir;
	meleeDef->GetVector( "kickDir", "0 0 0", kickDir );

	idVec3	globalKickDir;
	globalKickDir = ( viewAxis * physicsObj.GetGravityAxis() ) * kickDir;

//	ent->Damage( this, this, globalKickDir, meleeDefName, 1.0f, INVALID_JOINT );
	float	damageScale = spawnArgs.GetFloat( "damageScale", "1" );
	ent->Damage( this, this, globalKickDir, meleeDefName, damageScale, NULL );
}

/*
=====================
idAI::TestMelee
=====================
*/
bool idAI::TestMelee( void ) const {
	trace_t		trace;
	idEntity*	enemyEnt = enemy.ent;

	if ( !enemyEnt || !combat.meleeRange ) {
		return false;
	}

	//FIXME: make work with gravity vector
	idVec3 org = physicsObj.GetOrigin();
	const idBounds &myBounds = physicsObj.GetBounds();
	idBounds bounds;

	// expand the bounds out by our melee range
	bounds[0][0] = -combat.meleeRange;
	bounds[0][1] = -combat.meleeRange;
	bounds[0][2] = myBounds[0][2] - 4.0f;
	bounds[1][0] = combat.meleeRange;
	bounds[1][1] = combat.meleeRange;
	bounds[1][2] = myBounds[1][2] + 4.0f;
	bounds.TranslateSelf( org );

	idVec3 enemyOrg = enemyEnt->GetPhysics()->GetOrigin();
	idBounds enemyBounds = enemyEnt->GetPhysics()->GetBounds();
	enemyBounds.TranslateSelf( enemyOrg );

	if ( DebugFilter(ai_debugMove) ) {	//YELLOW = Test Melee Bounds
		gameRenderWorld->DebugBounds( colorYellow, bounds, vec3_zero, gameLocal.msec );
	}

	if ( !bounds.IntersectsBounds( enemyBounds ) ) {
		return false;
	}

	idVec3 start = GetEyePosition();
	idVec3 end = enemyEnt->GetEyePosition();

	gameLocal.TracePoint( this, trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
	if ( ( trace.fraction == 1.0f ) || ( gameLocal.GetTraceEntity( trace ) == enemyEnt ) ) {
		return true;
	}

	return false;
}

/*
=====================
idAI::AttackMelee

jointname allows the endpoint to be exactly specified in the model,
as for the commando tentacle.  If not specified, it will be set to
the facing direction + combat.meleeRange.

kickDir is specified in the monster's coordinate system, and gives the direction
that the view kick and knockback should go
=====================
*/
bool idAI::AttackMelee ( const char *attackName, const idDict* meleeDict ) {
	idEntity*				enemyEnt = enemy.ent;
	const char*				p;
	const idSoundShader*	shader;

	if ( !enemyEnt ) {
		p = meleeDict->GetString( "snd_miss" );
		if ( p && *p ) {
			shader = declManager->FindSound( p );
			StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
		}
		return false;
	}

	// check for the "saving throw" automatic melee miss on lethal blow
	// stupid place for this.
	bool forceMiss = false;
	if ( enemyEnt->IsType( idPlayer::GetClassType() ) && g_skill.GetInteger() < 2 ) {
		int	damage, armor;
		idPlayer *player = static_cast<idPlayer*>( enemyEnt );
		player->CalcDamagePoints( this, this, meleeDict, 1.0f, INVALID_JOINT, &damage, &armor );

		if ( enemyEnt->health <= damage ) {
			int	t = gameLocal.time - player->lastSavingThrowTime;
			if ( t > SAVING_THROW_TIME ) {
				player->lastSavingThrowTime = gameLocal.time;
				t = 0;
			}
			if ( t < 1000 ) {
				forceMiss = true;
			}
		}
	}

	// make sure the trace can actually hit the enemy
	if ( forceMiss || !TestMelee( ) ) {
		// missed
		p = meleeDict->GetString( "snd_miss" );
		if ( p && *p ) {
			shader = declManager->FindSound( p );
			StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
		}
		return false;
	}

	//
	// do the damage
	//
	p = meleeDict->GetString( "snd_hit" );
	if ( p && *p ) {
		shader = declManager->FindSound( p );
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
	}

	idVec3	kickDir;
	meleeDict->GetVector( "kickDir", "0 0 0", kickDir );

	idVec3	globalKickDir;
	globalKickDir = ( viewAxis * physicsObj.GetGravityAxis() ) * kickDir;

	// This allows some AI to be soft against melee-- most marines are selfMeleeDamageScale of 3, which means they take 3X from melee attacks!
	float damageScale = spawnArgs.GetFloat( "damageScale", "1" ) * enemyEnt->spawnArgs.GetFloat ( "selfMeleeDamageScale", "1" );

	//if attacker is a melee superhero, damageScale is way increased
	idAI* enemyAI = static_cast<idAI*>(enemyEnt);
	if( aifl.meleeSuperhero)	{
		if( damageScale >=1 )	{
			damageScale *= 6;
		} else	{
			damageScale = 6;
		}
	}
	
	//if the defender is a melee superhero, damageScale is way decreased
	if( enemyAI->aifl.meleeSuperhero)	{
		damageScale = 0.5f;
	}

	int   location    = INVALID_JOINT;
	if ( enemyEnt->IsType ( idAI::Type ) ) {
		location = static_cast<idAI*>(enemyEnt)->chestOffsetJoint;
	}
	enemyEnt->Damage( this, this, globalKickDir, meleeDict->GetString ( "classname" ), damageScale, location );

	if ( meleeDict->GetString( "fx_impact", NULL ) ) {
		if ( enemyEnt == gameLocal.GetLocalPlayer() ) {
			idPlayer *ePlayer = static_cast<idPlayer*>(enemyEnt);
			if ( ePlayer ) {
				idVec3 dir = ePlayer->firstPersonViewOrigin-GetEyePosition();
				dir.Normalize();
				idVec3 org = ePlayer->firstPersonViewOrigin + (dir * -((gameLocal.random.RandomFloat()*8.0f)+8.0f) );
				idAngles ang = ePlayer->firstPersonViewAxis.ToAngles() * -1;
				idMat3 axis = ang.ToMat3();
				gameLocal.PlayEffect( *meleeDict, "fx_impact", org, viewAxis );
			}
		}
	}

	lastAttackTime = gameLocal.time;

	return true;
}

/*
================
idAI::PushWithAF
================
*/
void idAI::PushWithAF( void ) {
	int i, j;
	afTouch_t touchList[ MAX_GENTITIES ];
	idEntity *pushed_ents[ MAX_GENTITIES ];
	idEntity *ent;
	idVec3 vel;
	int num_pushed;

	num_pushed = 0;
	af.ChangePose( this, gameLocal.time );
	int num = af.EntitiesTouchingAF( touchList );
	for( i = 0; i < num; i++ ) {
		if ( touchList[ i ].touchedEnt->IsType( idProjectile::GetClassType() ) ) {
			// skip projectiles
			continue;
		}

		// make sure we havent pushed this entity already.  this avoids causing double damage
		for( j = 0; j < num_pushed; j++ ) {
			if ( pushed_ents[ j ] == touchList[ i ].touchedEnt ) {
				break;
			}
		}
		if ( j >= num_pushed ) {
			ent = touchList[ i ].touchedEnt;
			pushed_ents[num_pushed++] = ent;
			vel = ent->GetPhysics()->GetAbsBounds().GetCenter() - touchList[ i ].touchedByBody->GetWorldOrigin();
			vel.Normalize();
			ent->GetPhysics()->SetLinearVelocity( 100.0f * vel, touchList[ i ].touchedClipModel->GetId() );
		}
	}
}

/***********************************************************************

	Misc

***********************************************************************/

/*
================
idAI::GetMuzzle
================
*/
void idAI::GetMuzzle( jointHandle_t joint, idVec3 &muzzle, idMat3 &axis ) {
	if ( joint == INVALID_JOINT ) {
		muzzle = physicsObj.GetOrigin() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 14;
		muzzle -= physicsObj.GetGravityNormal() * physicsObj.GetBounds()[ 1 ].z * 0.5f;
	} else {
		GetJointWorldTransform( joint, gameLocal.time, muzzle, axis );
		//MCG
		//pull the muzzle back inside the bounds if possible
		//FIXME: this is nasty, we should just be able to check for starting in solid and register that as a hit!  But...
		/*
		float scale = 0.0f;
		if ( physicsObj.GetBounds().RayIntersection( muzzle, -axis[0], scale ) )
		{
			if ( scale != 0.0f )
			{//not already inside
				muzzle += scale * -axis[0];
			}
		}
		else
		{//just pull it back a little anyway?
			idVec3 xyOfs = muzzle-physicsObj.GetOrigin();
			xyOfs.z = 0;
			muzzle += xyOfs.Length() * -axis[0];
		}
		*/
	}
}

/*
================
idAI::Hide
================
*/
void idAI::Hide( void ) {
	idActor::Hide();
	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();
	StopSound( SND_CHANNEL_AMBIENT, false );

	enemy.fl.inFov		= false;
	enemy.fl.visible	= false;

	StopMove( MOVE_STATUS_DONE );
}

/*
================
idAI::Show
================
*/
void idAI::Show( void ) {
	idActor::Show();
	if ( spawnArgs.GetBool( "big_monster" ) ) {
		physicsObj.SetContents( 0 );
	} else if ( use_combat_bbox ) {
		physicsObj.SetContents( CONTENTS_BODY|CONTENTS_SOLID );
	} else {
		physicsObj.SetContents( CONTENTS_BODY );
	}

	physicsObj.GetClipModel()->Link();

	fl.takedamage = !spawnArgs.GetBool( "noDamage" );
	StartSound( "snd_ambient", SND_CHANNEL_AMBIENT, 0, false, NULL );
}

/*
================
idAI::CanPlayChatterSounds

Used for playing chatter sounds on monsters.
================
*/
bool idAI::CanPlayChatterSounds( void ) const {
	if ( aifl.dead ) {
		return false;
	}

	if ( IsHidden() ) {
		return false;
	}

	if ( enemy.ent ) {
		return true;
	}

	if ( spawnArgs.GetBool( "no_idle_chatter" ) ) {
		return false;
	}

	return true;
}

/*
=====================
idAI::UpdateChatter
=====================
*/
void idAI::UpdateChatter ( void ) {	
	int			chatterRate;
	const char* chatter;

	// No chatter?
	if ( !chatterRateIdle && !chatterRateCombat ) {
		return;
	}

	// check if it's time to play a chat sound
	if ( IsHidden() || !aifl.awake || aifl.dead || ( chatterTime > gameLocal.time ) ) {
		return;
	}
	
	// Skip first chatter
	if ( enemy.ent ) {
		chatter		= "lipsync_chatter_combat";
		chatterRate = chatterRateCombat;
	} else {
		chatter		= "lipsync_chatter_idle";
		chatterRate = chatterRateIdle;
	}

	// Can chatter ?
	if ( !chatterRate ) {
		return;
	}

	// Start chattering, but not if he's already speaking.  And he might already be speaking because he was scripted to do so.
	if ( chatterTime > 0 && !IsSpeaking() ) {
		Speak ( chatter, true );
	}

	// set the next chat time
	chatterTime = gameLocal.time + chatterRate + (gameLocal.random.RandomFloat() * chatterRate * 0.5f) - (chatterRate * 0.25f);
}

/*
=====================
idAI::HeardSound
=====================
*/
idEntity *idAI::HeardSound( int ignore_team ){
	// check if we heard any sounds in the last frame
	idActor	*actor = gameLocal.GetAlertActor();
	if ( actor && ( !ignore_team || ( ReactionTo( actor ) & ATTACK_ON_SIGHT ) ) && gameLocal.InPlayerPVS( this ) ) 	{
		idVec3 pos = actor->GetPhysics()->GetOrigin();
		idVec3 org = physicsObj.GetOrigin();
		float dist = ( pos - org ).LengthSqr();
		
		if ( dist < Square( combat.earRange ) ) {
			//really close?
			if ( dist < Square( combat.earRange/4.0f ) ) {		
				return actor;
			//possible LOS
			} else if ( dist < Square( combat.visRange * 2.0f ) && CanSee( actor, false ) ) {
				return actor;
			} else if ( combat.fl.aware ) {
				//FIXME: or, maybe find cover/hide/ambush spot and wait to ambush them?
			//don't have an enemy and not tethered to a position
			} else if ( !GetEnemy() && !tether ) {
				//go into search mode
				WanderAround();
				move.fl.noRun = false;
				move.fl.idealRunning = true;
				//undid this: was causing them to not be able to do fine nav.
				//move.fl.noWalk = true;
			}
		}
	}

	return NULL;
}

/*
============
idAI::SetLeader
============
*/
void idAI::SetLeader ( idEntity *newLeader ) {
	idEntity* oldLeader = leader;
	
	if( !newLeader ){
		leader = NULL;
	} else if ( !newLeader->IsType( idActor::GetClassType() ) ) {
		gameLocal.Error( "'%s' is not an idActor (player or ai controlled character)", newLeader->name.c_str() );
	} else {
		leader = static_cast<idActor *>( newLeader );	
	}
	
	if ( oldLeader != leader ) {
		OnLeaderChange ( oldLeader );
	}
}



/***********************************************************************

	Head & torso aiming

***********************************************************************/

/*
================
idAI::UpdateAnimationControllers
================
*/
bool idAI::UpdateAnimationControllers( void ) {
	idVec3		left;
	idVec3 		dir;
	idVec3 		orientationJointPos;
	idVec3 		localDir;
	idAngles 	newLookAng;
	idMat3		mat;
	idMat3		axis;
	idMat3		orientationJointAxis;
	idVec3		pos;
	int			i;
	idAngles	jointAng;
	float		orientationJointYaw;
	float		currentHeadFocusRate = ( combat.fl.aware ? headFocusRate : headFocusRate * 0.5f );

	MEM_SCOPED_TAG(tag,MA_ANIM);

	if ( aifl.dead ) {
		return idActor::UpdateAnimationControllers();
	}

	if ( orientationJoint == INVALID_JOINT ) {
		orientationJointAxis = viewAxis;
		orientationJointPos = physicsObj.GetOrigin();
		orientationJointYaw = move.current_yaw;
	} else {
		GetJointWorldTransform( orientationJoint, gameLocal.time, orientationJointPos, orientationJointAxis );
		orientationJointYaw = orientationJointAxis[ 2 ].ToYaw();
		orientationJointAxis = idAngles( 0.0f, orientationJointYaw, 0.0f ).ToMat3();
	}
	
	// Update the IK after we've gotten all the joint positions we need, but before we set any joint positions.
	// Getting the joint positions causes the joints to be updated.  The IK gets joint positions itself (which
	// are already up to date because of getting the joints in this function) and then sets their positions, which
	// forces the heirarchy to be updated again next time we get a joint or present the model.  If IK is enabled,
	// or if we have a seperate head, we end up transforming the joints twice per frame.  Characters with no
	// head entity and no ik will only transform their joints once.  Set g_debuganim to the current entity number
	// in order to see how many times an entity transforms the joints per frame.
	idActor::UpdateAnimationControllers();

	// Update the focus position
	UpdateFocus( orientationJointAxis );

	//MCG NOTE: don't know why Dube added this extra check for the torsoAnim (5/09/05), but it was causing popping, so I took it out... :/
	//bool canLook = ( !torsoAnim.AnimDone(0) || !torsoAnim.GetAnimator()->CurrentAnim(ANIMCHANNEL_TORSO)->IsDone(gameLocal.GetTime()) || torsoAnim.Disabled() ) && !torsoAnim.GetAnimFlags().ai_no_look && !aifl.disableLook;
	//bool canLook = (!torsoAnim.GetAnimFlags().ai_no_look && !aifl.disableLook);
	
	bool canLook = (!animator.GetAnimFlags(animator.CurrentAnim(ANIMCHANNEL_TORSO)->AnimNum()).ai_no_look && !aifl.disableLook);
	if ( !canLook ) {
		//actually, do the looking, but bring it back forward...
		currentFocusPos = GetEyePosition() + orientationJointAxis[ 0 ] * 64.0f;
	}

	// Determine the new look yaw
	dir = currentFocusPos - orientationJointPos;
	newLookAng.yaw = dir.ToYaw( );

	// Determine the new look pitch
	dir = currentFocusPos - GetEyePosition();
	dir.NormalizeFast();
	orientationJointAxis.ProjectVector( dir, localDir );
	newLookAng.pitch = -idMath::AngleNormalize180( localDir.ToPitch() );
	newLookAng.roll	= 0.0f;
	
	if ( !canLook ) {
		//actually, do the looking, but bring it back forward...
		newLookAng.yaw = orientationJointAxis[0].ToYaw();
		newLookAng.pitch = 0;
	}
	// Add the angle change using the head focus rate and convert the look angles to
	// local angles so they can be properly clamped.
	float f = lookAng.yaw;
	if ( lookMin.yaw <= -360.0f && lookMax.yaw >= 360.0f )
	{
		lookAng.yaw    = f - orientationJointYaw;
		float diff;
		diff = ( idMath::AngleNormalize360(newLookAng.yaw) - idMath::AngleNormalize360(f) );
		diff = idMath::AngleNormalize180( diff );
		lookAng.yaw    += diff * currentHeadFocusRate;
		//lookAng.yaw   += ( newLookAng.yaw - f ) * currentHeadFocusRate;
	}
	else
	{
		lookAng.yaw    = idMath::AngleNormalize180 ( f - orientationJointYaw );
		lookAng.yaw   += (idMath::AngleNormalize180 ( newLookAng.yaw - f ) * currentHeadFocusRate);
	}
	lookAng.pitch += ( idMath::AngleNormalize180( newLookAng.pitch - lookAng.pitch ) * currentHeadFocusRate );

	// Clamp the look angles
	lookAng.Clamp( lookMin, lookMax );

	// Calcuate the eye angles
	f = eyeAng.yaw;
	eyeAng.yaw    = idMath::AngleNormalize180( f - orientationJointYaw );
	eyeAng.yaw   += (idMath::AngleNormalize180( newLookAng.yaw - f ) * eyeFocusRate);	
	eyeAng.pitch += (idMath::AngleNormalize180( newLookAng.pitch - eyeAng.pitch ) * eyeFocusRate);

	// Clamp eye angles relative to the look angles
	jointAng = eyeAng - lookAng;
	jointAng.Normalize180( );	
	jointAng.Clamp( eyeMin, eyeMax );
	eyeAng = lookAng + jointAng;
	eyeAng.Normalize180( );	

	if ( canLook ) {
		// Apply the look angles to the look joints
		if ( animator.GetAnimFlags( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimNum()).ai_look_head_only ) {
			if ( neckJoint != INVALID_JOINT || headJoint != INVALID_JOINT ) {
				float jScale = 1.0f;
				if ( neckJoint != INVALID_JOINT && headJoint != INVALID_JOINT ) {
					jScale = 0.5f;
				}
				jointAng.pitch	= lookAng.pitch * jScale;
				jointAng.yaw	= lookAng.yaw * jScale;
				if ( neckJoint != INVALID_JOINT ) {
					animator.SetJointAxis( neckJoint, JOINTMOD_WORLD, jointAng.ToMat3() );
				}
				if ( headJoint != INVALID_JOINT ) {
					animator.SetJointAxis( headJoint, JOINTMOD_WORLD, jointAng.ToMat3() );
				}
			}
			//what if we have a previous joint mod on the rest of the lookJoints 
			//from an anim that *wasn't* ai_look_head_only...?
			//just clear them?  Or move them towards 0?
			//FIXME: move them towards zero...
			if ( newLookAng.Compare( lookAng ) ) {
				//snap back now!
				for( i = 0; i < lookJoints.Num(); i++ ) {
					if ( lookJoints[i] != neckJoint
						&& lookJoints[i] != headJoint ) {
						//snap back now!
						animator.ClearJoint ( lookJoints[i] );
					}
				}
			} else {
				//blend back
				//yes, this is framerate dependant and inefficient and wrong, but... eliminates pops
				jointAng.roll = 0.0f;
				jointMod_t *curJointMod;
				idAngles curJointAngleMod;
				for( i = 0; i < lookJoints.Num(); i++ ) {
					if ( lookJoints[i] != neckJoint
						&& lookJoints[i] != headJoint ) {
						//blend back
						curJointMod = animator.FindExistingJointMod( lookJoints[ i ], NULL );
						if ( curJointMod )
						{
							curJointAngleMod = curJointMod->mat.ToAngles();
							curJointAngleMod *= 0.75f;
							if ( fabs(curJointAngleMod.pitch) < 1.0f 
								&& fabs(curJointAngleMod.yaw) < 1.0f
								&& fabs(curJointAngleMod.roll) < 1.0f )
							{
								//snap back now!
								animator.ClearJoint ( lookJoints[i] );
							}
							else
							{
								animator.SetJointAxis( lookJoints[ i ], JOINTMOD_WORLD, curJointAngleMod.ToMat3() );
							}
						}
					}
				}
			}
		} else {
			jointAng.roll = 0.0f;
			for( i = 0; i < lookJoints.Num(); i++ ) {
				jointAng.pitch	= lookAng.pitch * lookJointAngles[ i ].pitch;
				jointAng.yaw	= lookAng.yaw * lookJointAngles[ i ].yaw;
				animator.SetJointAxis( lookJoints[ i ], JOINTMOD_WORLD, jointAng.ToMat3() );
			}
		}
	} else {
		//if ( animator.GetAnimFlags(animator.CurrentAnim(ANIMCHANNEL_TORSO)->AnimNum()).ai_no_look || aifl.disableLook ) {
		if ( newLookAng.Compare( lookAng ) ) {
			//snap back now!
			for( i = 0; i < lookJoints.Num(); i++ ) {
				animator.ClearJoint ( lookJoints[i] );
			}
		} else {
			//PCJ back to neutral
			jointAng.roll = 0.0f;
			for( i = 0; i < lookJoints.Num(); i++ ) {
				jointAng.pitch	= lookAng.pitch * lookJointAngles[ i ].pitch;
				jointAng.yaw	= lookAng.yaw * lookJointAngles[ i ].yaw;
				animator.SetJointAxis( lookJoints[ i ], JOINTMOD_WORLD, jointAng.ToMat3() );
			}
		}
	}	

	// Convert the look angles back to world angles.  This is done to prevent dramatic orietation changes
	// from changing the look direction abruptly (unless of course the minimums are not met)
	lookAng.yaw = idMath::AngleNormalize180( lookAng.yaw + orientationJointYaw );
	eyeAng.yaw = idMath::AngleNormalize180( eyeAng.yaw + orientationJointYaw );
	
	if ( move.moveType == MOVETYPE_FLY || move.fl.flyTurning ) {
		// lean into turns
		AdjustFlyingAngles();
	}

	// Orient the eyes towards their target
	if ( leftEyeJoint != INVALID_JOINT && rightEyeJoint != INVALID_JOINT ) {
		if ( head ) {
			idAnimator *headAnimator = head->GetAnimator();

			if ( focusType != AIFOCUS_NONE && allowEyeFocus && canLook ) {
				//tweak these since it's looking at the wrong spot and not calculating from each eye and I don't have time to bother rewriting all of this properly
				eyeAng.yaw -= 0.5f;
				eyeAng.pitch += 3.25f;
				idMat3 eyeAxis = ( eyeAng ).ToMat3() * head->GetPhysics()->GetAxis().Transpose ( );  

				headAnimator->SetJointAxis ( leftEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyeAxis );	

				eyeAng.yaw += 3.0f;
				eyeAxis = ( eyeAng ).ToMat3() * head->GetPhysics()->GetAxis().Transpose ( );  

				headAnimator->SetJointAxis ( rightEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyeAxis );

				if ( ai_debugEyeFocus.GetBool() ) {
					idVec3 eyeOrigin;
					idMat3 axis;
					
					head->GetJointWorldTransform ( rightEyeJoint, gameLocal.time, eyeOrigin, axis );
					gameRenderWorld->DebugArrow ( colorGreen, eyeOrigin, ( eyeOrigin + (32.0f*axis[0])), 1, 0 );

					head->GetJointWorldTransform ( leftEyeJoint,  gameLocal.time, eyeOrigin,  axis );
					gameRenderWorld->DebugArrow ( colorGreen, eyeOrigin, ( eyeOrigin + (32.0f*axis[0])), 1, 0 );
				}
			} else {
				headAnimator->ClearJoint( leftEyeJoint );
				headAnimator->ClearJoint( rightEyeJoint );
			}
		} else {
			if ( allowEyeFocus && focusType != AIFOCUS_NONE && !torsoAnim.GetAnimFlags().ai_no_look && !aifl.disableLook ) {
				idMat3 eyeAxis = ( eyeAng ).ToMat3() * GetPhysics()->GetAxis().Transpose ( );  
				animator.SetJointAxis ( rightEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyeAxis );
				animator.SetJointAxis ( leftEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyeAxis );			
			} else {
				animator.ClearJoint( leftEyeJoint );
				animator.ClearJoint( rightEyeJoint );
			}
		}
	}
	
	return true;
}

void idAI::OnTouch( idEntity *other, trace_t *trace ) {
	// if we dont have an enemy or had one for at least a second that is a potential enemy the set the enemy	
	if ( other->IsType( idActor::GetClassType() )
		&& !other->fl.notarget 
		&& ( ReactionTo( other )&ATTACK_ON_SIGHT)
		&& (!enemy.ent || gameLocal.time - enemy.changeTime > 1000 ) ) {
		SetEnemy( other );
	}

	if ( !enemy.ent && !other->fl.notarget && ( ReactionTo( other ) & ATTACK_ON_ACTIVATE ) ) {
		Activate( other );
	}
	pusher = other;
	aifl.pushed = true;
	
	// If pushed by the player update tactical
	if ( pusher && pusher->IsType ( idPlayer::GetClassType() ) && (combat.tacticalMaskAvailable & AITACTICAL_MOVE_PLAYERPUSH_BIT) ) {
		ForceTacticalUpdate ( );
	}		
}

idProjectile* idAI::AttackProjectile ( const idDict* projectileDict, const idVec3 &org, const idAngles &ang ) {
	idVec3				start;
	trace_t				tr;
	idBounds			projBounds;
	const idClipModel*	projClip;
	idMat3				axis;
	float				distance;
	idProjectile*		result;

	if ( !projectileDict ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		return NULL;
	}

	axis = ang.ToMat3();
	if ( !projectile.GetEntity() ) {
		CreateProjectile( projectileDict, org, axis[ 0 ] );
	}

	// make sure the projectile starts inside the monster bounding box
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	projClip = projectile.GetEntity()->GetPhysics()->GetClipModel();
	projBounds = projClip->GetBounds().Rotate( projClip->GetAxis() );

	// check if the owner bounds is bigger than the projectile bounds
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( org, viewAxis[ 0 ], distance ) ) {
			start = org + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

// RAVEN BEGIN
// ddynerman: multiple clip worlds
	gameLocal.Translation( this, tr, start, org, projClip, projClip->GetAxis(), MASK_SHOT_RENDERMODEL, this );
// RAVEN END

	// launch the projectile
 	projectile.GetEntity()->Launch( tr.endpos, axis[ 0 ], vec3_origin );
 	result = projectile;
	projectile = NULL;

	lastAttackTime = gameLocal.time;
	
	return result;
}

void idAI::RadiusDamageFromJoint( const char *jointname, const char *damageDefName ) {
	jointHandle_t joint;
	idVec3 org;
	idMat3 axis;

	if ( !jointname || !jointname[ 0 ] ) {
		org = physicsObj.GetOrigin();
	} else {
		joint = animator.GetJointHandle( jointname );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Unknown joint '%s' on %s", jointname, GetEntityDefName() );
		}
		GetJointWorldTransform( joint, gameLocal.time, org, axis );
	}

	gameLocal.RadiusDamage( org, this, this, this, this, damageDefName );
}

/*
=====================
idAI::CanBecomeSolid

returns true if the AI entity could become solid at its current position
=====================
*/
bool idAI::CanBecomeSolid ( void ) {
	int				i;
	int				num;
	idEntity *		hit;
	idClipModel*	cm;
	idClipModel*	clipModels[ MAX_GENTITIES ];

	// Determine what we are currently touching
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	num = gameLocal.ClipModelsTouchingBounds( this, physicsObj.GetAbsBounds(), MASK_MONSTERSOLID, clipModels, MAX_GENTITIES );
// RAVEN END
	for ( i = 0; i < num; i++ ) {
		cm = clipModels[ i ];

		// don't check render entities
		if ( cm->IsRenderModel() ) {
			continue;
		}

		hit = cm->GetEntity();
		if ( ( hit == this ) || !hit->fl.takedamage ) {
			continue;
		}

		if ( physicsObj.ClipContents( cm ) ) {
			return false;
		}
	}

	return true;
}

/*
=====================
idAI::BecomeSolid
=====================
*/
void idAI::BecomeSolid( void ) {
	physicsObj.EnableClip();
	if ( spawnArgs.GetBool( "big_monster" ) ) {
		physicsObj.SetContents( 0 );
	} else if ( use_combat_bbox ) {
		physicsObj.SetContents( CONTENTS_BODY|CONTENTS_SOLID );
	} else {
		physicsObj.SetContents( CONTENTS_BODY );
	}
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	physicsObj.GetClipModel()->Link();
// RAVEN END
	fl.takedamage = !spawnArgs.GetBool( "noDamage" );
}

/*
=====================
idAI::BecomeNonSolid
=====================
*/
void idAI::BecomeNonSolid( void ) {
	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();
}

const char *idAI::ChooseAnim( int channel, const char *animname ) {
	int anim;

	anim = GetAnim( channel, animname );
	if ( anim ) {
		if ( channel == ANIMCHANNEL_HEAD ) {
			if ( head.GetEntity() ) {
				return head.GetEntity()->GetAnimator()->AnimFullName( anim );
			}
		} else {
			return animator.AnimFullName( anim );
		}
	}

	return "";
}

/*
============
idAI::ExecScriptFunction
============
*/
void idAI::ExecScriptFunction ( rvScriptFuncUtility& func, idEntity* parm ) {
	if( parm ) {
		func.InsertEntity( parm, 0 );
	} else {
		func.InsertEntity( this, 0 );
	}

	func.CallFunc( &spawnArgs );

	if( parm ) {
		func.RemoveIndex( 0 );
	}
}

/*
============
idAI::Prethink
============
*/
void idAI::Prethink ( void ) {
	// Update our helper if we are moving
	if ( move.fl.moving ) {
		UpdateHelper ( );
	}
	
	if ( leader ) {
		idEntity* groundEnt	= leader->GetGroundEntity ( );
		if ( !(tether && !aifl.tetherMover) && groundEnt ) {
			if ( groundEnt->IsType ( idMover::GetClassType ( ) ) ) {			
				idEntity* ent;
				idEntity* next;
				for( ent = groundEnt->GetNextTeamEntity(); ent != NULL; ent = next ) {
					next = ent->GetNextTeamEntity();
					if ( ent->GetBindMaster() == groundEnt && ent->IsType ( rvAITether::GetClassType ( ) ) ) {
						SetTether ( static_cast<rvAITether*>(ent) );
						aifl.tetherMover = true;
						break;
					}
				}
			} else {
				SetTether ( NULL );
			}
		}					
	} else if ( tether && aifl.tetherMover ) {
		SetTether ( NULL );
	}
}

/*
============
idAI::Postthink
============
*/
void idAI::Postthink( void ){
	if ( !aifl.simpleThink ) {
		pain.takenThisFrame = 0;
	}

	// Draw debug tactical information 
	DrawTactical ( );
				
	if( vehicleController.IsDriving() ){	// Generate some sort of command?
		usercmd_t				usercmd;

		// Note!  usercmd angles stuff is in deltas, not in absolute values.

		memset( &usercmd, 0, sizeof( usercmd ) );

		idVec3 toEnemy;

		if( enemy.ent ){
			toEnemy = enemy.ent->GetPhysics()->GetOrigin();
			toEnemy -= GetPhysics()->GetOrigin();
			toEnemy.Normalize();
			
			idAngles enemyAng;

			enemyAng = toEnemy.ToAngles();

			usercmd.angles[PITCH] = ANGLE2SHORT( enemyAng.pitch );
			usercmd.angles[YAW] = ANGLE2SHORT( enemyAng.yaw );

			usercmd.buttons = BUTTON_ATTACK;

			vehicleController.SetInput ( usercmd, enemyAng );
		}
	}
	
	// Keep our threat value up to date
	UpdateThreat ( );
}

/*
============
idAI::
============
*/

void idAI::OnDeath( void ){
	if( vehicleController.IsDriving() ){
		usercmd_t				usercmd;

		memset( &usercmd, 0, sizeof( usercmd ) );
		usercmd.buttons = BUTTON_ATTACK;
		usercmd.upmove = 300.0f; // This will cause the character to eject.

		vehicleController.SetInput( usercmd, idAngles( 0, 0, 0 ) );

		// Fixme!  Is this safe to do immediately?
		vehicleController.Eject();
	}

	aiManager.RemoveTeammate ( this );

	ExecScriptFunction( funcs.death );

/* DONT DROP ANYTHING FOR NOW
	float rVal = gameLocal.random.RandomInt( 100 );

	if( spawnArgs.GetFloat( "no_drops" ) >= 1.0 ){
		spawnArgs.Set( "def_dropsItem1", "" );
	}else{
		// Fixme!  Better guys should drop better stuffs!  Make drops related to guy type?  Do something cooler here?
		if( rVal < 25 ){	// Half of guys drop nothing?
			spawnArgs.Set( "def_dropsItem1", "" );
		}else if( rVal < 50 ){
			spawnArgs.Set( "def_dropsItem1", "item_health_small" );
		}
	}
*/
}

/*
============
idAI::OnWakeUp
============
*/
void idAI::OnWakeUp ( void ) {
}

/*
============
idAI::OnUpdatePlayback
============
*/
void idAI::OnUpdatePlayback ( const rvDeclPlaybackData& pbd ) {
	return;
}

/*
============
idAI::OnLeaderChange
============
*/
void idAI::OnLeaderChange ( idEntity* oldLeader ) {
	ForceTacticalUpdate ( );
}

/*
============
idAI::OnEnemyChange
============
*/
void idAI::OnEnemyChange ( idEntity* oldEnemy ) {
	// Make sure we update our tactical state immediately
	ForceTacticalUpdate ( );

	// see if we should announce the enemy
	if ( enemy.ent ) {
		combat.fl.aware = true;
		combat.investigateTime = 0;

		enemy.changeTime = gameLocal.time;

		enemy.lastKnownPosition = enemy.ent->GetPhysics()->GetOrigin ( );
		
		UpdateEnemyVisibility ( );
		UpdateEnemyPosition ( true );
		UpdateEnemy ( );		
	} else {
		enemy.range		 = 0;
		enemy.range2d 	 = 0;
		enemy.changeTime = 0;
		enemy.smoothedLinearVelocity.Zero ( );
		enemy.smoothedPushedVelocity.Zero ( );
	}	
	
	enemy.fl.visible = false;
}

/*
============
idAI::OnTacticalChange
============
*/
void idAI::OnTacticalChange ( aiTactical_t oldTactical ) {
	// if acutally moving to a new tactial location announce it
	if ( move.fl.moving ) {
		AnnounceTactical( combat.tacticalCurrent );
	}
}

/*
============
idAI::OnFriendlyFire
============
*/
void idAI::OnFriendlyFire ( idActor* attacker ) {
	AnnounceFriendlyFire( static_cast<idActor*>(attacker) );
}	

/*
============
idAI::OnStartMoving
============
*/
void idAI::OnStartMoving ( void ) {
	aifl.simpleThink = false;
	combat.fl.crouchViewClear = false;
}

/*
============
idAI::OnStopMoving
============
*/
void idAI::OnStopMoving ( aiMoveCommand_t oldMoveCommand ) {
}

/*
============
idAI::OnStartAction
============
*/
void idAI::OnStartAction ( void ) {
}

/*
============
idAI::OnStopAction
============
*/
void idAI::OnStopAction	( void ) {
}

/*
============
idAI::OnEnemyVisiblityChange
============
*/
void idAI::OnEnemyVisiblityChange ( bool oldVisible ) {
}

/*
============
idAI::OnSetKey
============
*/
void idAI::OnSetKey	( const char* key, const char* value ) {
	if ( !idStr::Icmp ( key, "noCombatChatter" ) ) {
		combat.fl.noChatter = spawnArgs.GetBool ( key );
	} else if ( !idStr::Icmp ( key, "allowPlayerPush" ) ) {
		combat.tacticalMaskAvailable &= ~(AITACTICAL_MOVE_PLAYERPUSH_BIT);		
		if ( spawnArgs.GetBool ( key ) ) {
			combat.tacticalMaskAvailable |= AITACTICAL_MOVE_PLAYERPUSH_BIT;
		}
	} else if ( !idStr::Icmp ( key, "noLook" ) ) {
		aifl.disableLook = spawnArgs.GetBool ( key );
	} else if ( !idStr::Icmp ( key, "killer_guard" ) ) {
		aifl.killerGuard = spawnArgs.GetBool ( key );
	}
}

/*
============
idAI::OnCoverInvalidated
============
*/
void idAI::OnCoverInvalidated ( void ) {
	// Force a tactical update now
	ForceTacticalUpdate ( );
}

/*
============
idAI::OnCoverNotFacingEnemy
============
*/
void idAI::OnCoverNotFacingEnemy ( void ) {
	// Clear attack timers so we can shoot right now
	actionTimerRangedAttack.Clear ( actionTime );
	actionRangedAttack.timer.Clear( actionTime );
}

/*
============
idAI::SkipCurrentDestination

Is the AI's current destination ok enough to stay at?
============
*/
bool idAI::SkipCurrentDestination ( void ) const {
	// can only skip current destination when we are stopped
	if ( move.fl.moving ) {
		return false;
	}
/*
	// If we are currently behind cover and that cover is no longer valid we should skip it
	if ( IsBehindCover ( ) && !IsCoverValid ( ) ) {
		return true;
	}
*/
	return false;
}

/*
============
idAI::SkipImpulse
============
*/
bool idAI::SkipImpulse( idEntity *ent, int id ){
	bool skip = idActor::SkipImpulse( ent, id );

	if( af.IsActive ( ) ) {
		return false;
	}
	if( !fl.takedamage ){
		return true;
	}
	if( move.moveCommand == MOVE_RV_PLAYBACK ){
		return true;
	}	
	
	return skip;
}

/*
============
idAI::CanHitEnemy
============
*/
bool idAI::CanHitEnemy ( void ) {
	trace_t	tr;
	idEntity *hit;

	idEntity *enemyEnt = enemy.ent;
	if ( !IsEnemyVisible ( ) || !enemyEnt ) {
		return false;
	}

	// don't check twice per frame
	if ( gameLocal.time == lastHitCheckTime ) {
		return lastHitCheckResult;
	}

	lastHitCheckTime = gameLocal.time;

	idVec3 toPos = enemyEnt->GetEyePosition();
	idVec3 eye = GetEyePosition();
	idVec3 dir;

	// expand the ray out as far as possible so we can detect anything behind the enemy
	dir = toPos - eye;
	dir.Normalize();
	toPos = eye + dir * MAX_WORLD_SIZE;
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	if ( g_perfTest_aiNoVisTrace.GetBool() ) {
		lastHitCheckResult = true;
		return lastHitCheckResult;
	}

	gameLocal.TracePoint( this, tr, eye, toPos, MASK_SHOT_BOUNDINGBOX, this );
	hit = gameLocal.GetTraceEntity( tr );
// RAVEN END
	if ( tr.fraction >= 1.0f || ( hit == enemyEnt ) ) {
		lastHitCheckResult = true;
	} else if ( ( tr.fraction < 1.0f ) && ( hit->IsType( idAI::Type ) ) && 
		( static_cast<idAI *>( hit )->team != team ) ) {
		lastHitCheckResult = true;
	} else {
		lastHitCheckResult = false;
	}

	return lastHitCheckResult;
}

/*
============
idAI::CanHitEnemyFromJoint
============
*/

bool idAI::CanHitEnemyFromJoint( const char *jointname ){
	trace_t	tr;
	idVec3	muzzle;
	idMat3	axis;

	idEntity *enemyEnt = enemy.ent;
	if ( !IsEnemyVisible ( ) || !enemyEnt ) {
		return false;
	}

	// don't check twice per frame
	if ( gameLocal.time == lastHitCheckTime ) {
		return lastHitCheckResult;
	}

	if ( g_perfTest_aiNoVisTrace.GetBool() ) {
		lastHitCheckResult = true;
		return true;
	}

	lastHitCheckTime = gameLocal.time;

	idVec3 toPos = enemyEnt->GetEyePosition();
	jointHandle_t joint = animator.GetJointHandle( jointname );
	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "Unknown joint '%s' on %s", jointname, GetEntityDefName() );
	}
	animator.GetJointTransform( joint, gameLocal.time, muzzle, axis );
	muzzle = physicsObj.GetOrigin() + ( muzzle + modelOffset ) * viewAxis * physicsObj.GetGravityAxis();
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	gameLocal.TracePoint( this, tr, muzzle, toPos, MASK_SHOT_BOUNDINGBOX, this );
// RAVEN END
	if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == enemyEnt ) ) {
		lastHitCheckResult = true;
	} else {
		lastHitCheckResult = false;
	}

	return lastHitCheckResult;
}

/*
============
idAI::
============

float HeightForTrajectory( const idVec3 &start, float zVel, float gravity );

int idAI::TestTrajectory( const idVec3 &firePos, const idVec3 &target, const char *projectileName ){
	idVec3	projVelocity;
	idVec3	projGravity;
	float	testTime;
	float	zVel, height, pitch, s, c;
	idVec3	dir;
	float	newVel;
	float	delta_x;
	float	delta_z;

	projectileDef	= gameLocal.FindEntityDefDict ( projectileName );	
	projVelocity	= idProjectile::GetVelocity( projectileDef );
	projGravity		= idProjectile::GetGravity( projectileDef ).z * GetPhysics()->GetGravity();
	pitch			= DEG2RAD( gameLocal.random.RandomFloat() * 50 + 20 );	// Random pitch range between 20 and 70.  Should this be customizeable?

	idMath::SinCos( pitch, s, c );

	delta_x			= idMath::Sqrt( ( target.x - firePos.x ) * ( target.x - firePos.x ) + ( target.y - firePos.y ) * ( target.y - firePos.y ) );
	delta_z			= target.z - firePos.z;
	newVel			= ( delta_x / idMath::Cos( pitch ) ) * idMath::Sqrt( projGravity.z / ( 2.0f * ( delta_x * idMath::Tan( pitch ) - delta_z ) ) );
	testTime		= delta_x / ( newVel * c );
	zVel			= newVel * s;

	float a = idMath::ASin ( delta_x * GetPhysics()->GetGravity().Length() / (projVelocity.x * projVelocity.x) );
	a = a / 2;

	float r = (projVelocity.x * projVelocity.x) * idMath::Sin ( 2 * a ) / GetPhysics()->GetGravity().Length();
	if ( r < delta_x - (delta_x * 0.1) ) {
		mVar.valid_lobbed_shot = 0;
		return 0;
	} else {
		mVar.lobDir = target - firePos;
		mVar.lobDir.z = 0;
		mVar.lobDir.z = idMath::Tan ( a ) * mVar.lobDir.LengthFast();
		mVar.lobDir.Normalize ( );
		mVar.valid_lobbed_shot = gameLocal.time;
		mVar.lob_vel_scale = 1.0f; // newVel / projVelocity.Length();
		return 1;
	}

	projGravity[2] *= -1.0f;

	dir				= target - firePos;
	dir.z			= 0;
	dir.Normalize();
	delta_x			= idMath::Sqrt( 1 - ( c * c ) );
	dir.x			*= delta_x;
	dir.y			*= delta_x;
	dir.z			= c;

	height = HeightForTrajectory( firePos, zVel, projGravity[2] ) - firePos.z;
	if ( height > MAX_WORLD_SIZE ) {
		// goes higher than we want to allow
		mVar.valid_lobbed_shot = 0;
		return 0;
	}

	if ( idAI::TestTrajectory ( firePos, target, zVel, projGravity[2], testTime, height * 2, NULL, 
								MASK_SHOT_RENDERMODEL, this, enemy.GetEntity(), ai_debugTrajectory.GetBool() ? 4000 : 0 ) ) {

		if ( ai_debugTrajectory.GetBool() ) {
			float t = testTime / 100.0f;
			idVec3 velocity = dir * newVel;
			idVec3 lastPos, pos;
			lastPos = firePos;
			pos = firePos;
			for ( int j = 1; j < 100; j++ ) {
				pos += velocity * t;
				velocity += projGravity * t;
				gameRenderWorld->DebugLine( colorCyan, lastPos, pos, ai_debugTrajectory.GetBool() ? 4000 : 0 );
				lastPos = pos;
			}
		}

		mVar.lobDir = dir;
		mVar.valid_lobbed_shot = gameLocal.time;
		mVar.lob_vel_scale = newVel / projVelocity.Length();
	}else{
		mVar.valid_lobbed_shot = 0;
	}

	return ( (int)mVar.valid_lobbed_shot != 0 );

}
*/

/*
============
idAI::
============
*/

float idAI::GetTurnDelta( void ){
	float amount;

	if ( move.turnRate ) {
		amount = idMath::AngleNormalize180( move.ideal_yaw - move.current_yaw );
		return amount;
	} else {
		return 0.0f;
	}
}

/*
============
idAI::GetIdleAnimName
============
*/
const char* idAI::GetIdleAnimName ( void ) {
	const char* animName = NULL;
	
	// Start idle animation
	if ( enemy.ent ) {
		animName = "idle_alert";
	}
	
	if ( animName && HasAnim ( ANIMCHANNEL_ALL, animName ) ) {
		return animName;
	}
	
	return "idle";
}

/*
===============================================================================

	idAI - Enemy Finding

===============================================================================
*/

/*
============
idAI::FindEnemy
============
*/
idEntity *idAI::FindEnemy ( bool inFov, bool forceNearest, float maxDistSqr ){
	idActor*	actor;
	idActor*	bestEnemy;
	idActor*	bestEnemyBackup;
	float		bestThreat;
	float		bestThreatBackup;
	float		distSqr;
	float		enemyRangeSqr;
	float		awareRangeSqr;
	idVec3		origin;
	idVec3		delta;
	pvsHandle_t pvs;
	
	// Setup our local variables used in the search
	pvs				 = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );
	bestThreat		 = 0.0f;
	bestThreatBackup = 0.0f;
	bestEnemy		 = NULL;
	bestEnemyBackup	 = NULL;
	awareRangeSqr	 = Square ( combat.awareRange );
	enemyRangeSqr	 = enemy.ent ? Square ( enemy.range ) : Square ( combat.attackRange[1] );
	origin			 = GetEyePosition ( );

	// Iterate through the enemy team
	for( actor = aiManager.GetEnemyTeam ( (aiTeam_t)team ); actor; actor = actor->teamNode.Next() ) {
		// Skip hidden enemies and enemies that cant be targeted
		if( actor->fl.notarget || actor->fl.isDormant || ( actor->IsHidden ( ) && !actor->IsInVehicle() ) ) {
			continue;
		}

		// Calculate the distance between ourselves and our potential enemy
		delta   = physicsObj.GetOrigin() - actor->GetPhysics()->GetOrigin();
		distSqr = delta.LengthSqr();

		// Calculate the adjusted threat for this actor		
		float threat = CalculateEnemyThreat ( actor );

		// Save the highest threat enemy as a backup in case we cannot find one we can see
		if ( threat > bestThreatBackup ) {
			bestThreatBackup = threat;
			bestEnemyBackup  = actor;
		}

		// If we have already found a more threatening enemy then attack that
		if ( threat < bestThreat ) {
			continue;
		}
		
		// If this enemy isnt in the same pvps then use them as a backup
		if ( !gameLocal.pvs.InCurrentPVS( pvs, actor->GetPVSAreas(), actor->GetNumPVSAreas() ) ) {
			continue;
		}

		// this is pretty specific to Quake 4, but we don't want the player to be around an enemy too long who can't see him. We need to randomly spike the
		// awareRange on creatures to simulate them looking behind them, or noticing someone standing around for too long.
		// Modders take note, this will prevent most "sneaking up on bad guys" action because they will likely spike their aware ranges out
		// during the sneaking.
		if( gameLocal.random.RandomFloat() < 0.005f )	{
			awareRangeSqr *= 15;
		}

		// fov doesn't matter if they're within awareRange, we "sense" them if we're alert... (or should LOS not even matter at this point?)
		if ( distSqr < awareRangeSqr || CanSeeFrom ( origin, actor, (inFov && !(combat.fl.aware&&distSqr<awareRangeSqr)) ) ) {
			bestThreat = threat;
			bestEnemy  = actor;
		}
	}

	// If force nearest is set we will give them an enemy reguardless of distance or sight
	if( forceNearest ){
		if( bestEnemy == NULL ){
			bestEnemy = bestEnemyBackup;
		}
	}

	gameLocal.pvs.FreeCurrentPVS( pvs );
		
	return bestEnemy;
}

/*
============
idAI::CheckForEnemy

Look for a suitable enemy
============
*/
bool idAI::CheckForEnemy ( bool useFov, bool force ) {
	idEntity *newEnemy;

	if ( combat.fl.ignoreEnemies ) {
		return false;
	}

	// If we already have an enemy and arent being forced to find a new on then just return now
	if ( enemy.ent && !force ) {
		return true;
	}

	// Save last time we checked for a new enemy
	enemy.checkTime = gameLocal.time;

	// Dont use fov check when behind cover because you are up against a wall
	newEnemy = FindEnemy ( !IsBehindCover ( ), 0, 0.0f );
 	
 	// Havent found an enemy yet, see if we heard something that can be our enemy
	if ( !newEnemy ) {
		newEnemy = HeardSound( true );
	}

	// If we still havent found an enemy, see if a teammate can give us one	
	if ( !newEnemy ) {
		return CheckForTeammateEnemy ( );
	}

	SetEnemy( newEnemy );
	return true;
}

/*
============
idAI::CheckForTeammateEnemy
============
*/
bool idAI::CheckForTeammateEnemy( void ) {
	idActor*	teammate;
	idEntity*	teammateEnemy;

	// Not looking for a new enemy.
	if ( combat.fl.ignoreEnemies ) {
		return false;
	}
	
	// Find an enemy from a nearby ally
	teammateEnemy = aiManager.NearestTeammateEnemy ( this, 1000.0f, false, false, &teammate );
	if ( !teammateEnemy || teammateEnemy == enemy.ent ) {
		return false;
	}
	
	assert ( teammate );
	
	// Attempt to set the new enemy
	if ( !SetEnemy ( teammateEnemy ) ) {
		return false;
	}
	
	// If the ally is another AI entity we can use their enemy visibility information
	if ( teammate->IsType ( idAI::Type ) ) {
		idAI* teammateAI = static_cast<idAI*>(teammate);
		enemy.smoothedLinearVelocity		= teammateAI->enemy.smoothedLinearVelocity;
		enemy.smoothedPushedVelocity		= teammateAI->enemy.smoothedPushedVelocity;
		enemy.lastKnownPosition				= teammateAI->enemy.lastKnownPosition;
		enemy.lastVisibleEyePosition		= teammateAI->enemy.lastVisibleEyePosition;
		enemy.lastVisibleFromEyePosition	= teammateAI->enemy.lastVisibleEyePosition;
		enemy.lastVisibleChestPosition		= teammateAI->enemy.lastVisibleChestPosition;
		enemy.lastVisibleTime				= 0;
	}
	
	return true;
}

/*
============
idAI::CheckForCloserEnemy
============
*/
bool idAI::CheckForCloserEnemy ( void ) {
	idEntity*	newEnemy = NULL;
	float		maxDistSqr;

	// Not looking for a new enemy.
	if ( combat.fl.ignoreEnemies ) {
		return false;
	}

	// See if we happen to have heard someone this frame that we can use 
	newEnemy = HeardSound( true );
	if ( newEnemy && newEnemy != enemy.ent && newEnemy->IsType( idActor::GetClassType() ) ) {
		//heard someone else!
		float newDist = DistanceTo ( newEnemy->GetPhysics()->GetOrigin() );
		
		// Are they closer than the enemy we are fighting?
		if ( newDist < enemy.range ) {
			//new enemy is closer than current one, take them!
			SetEnemy( newEnemy );
			return true;
		}
	}

	if ( GetEnemy() && enemy.range ) {
		maxDistSqr = Min( Square ( enemy.range ), Square ( combat.awareRange ) );
	} else {
		maxDistSqr = Square ( combat.awareRange );
	}

	newEnemy = FindEnemy( false, 0, maxDistSqr );

	if ( !newEnemy ) {
		return false;
	}

	SetEnemy( newEnemy );
	return true;
}

/*
============
idAI::CheckForReplaceEnemy

TODO: Call CalculateThreat ( ent ) and compare to current entity
============
*/
bool idAI::CheckForReplaceEnemy ( idEntity* replacement ) {
	bool replace;

	// If our replacement is a driver a vehicle and they are hidden we will 
	// want to shoot back at their vehicle not them.
	idActor* actor;
	actor = dynamic_cast<idActor*>(replacement);
	if ( actor && actor->IsInVehicle ( ) && actor->IsHidden ( ) ) {
		replacement = actor->GetVehicleController ( ).GetVehicle ( );
	}

	// Invalid replacement?
	if ( !replacement) {
		return false;
	}

	// Not looking for a new enemy.
	if ( combat.fl.ignoreEnemies ) {
		return false;
	}

	if ( replacement == enemy.ent ) {
		return false;
	}
	 
	// Dont want to set our enemy to a friendly target
	if ( replacement->IsType( idActor::GetClassType() ) && (static_cast<idActor*>(replacement))->team == team ) {
		return false;
	}
	
	// Not having an enemy will set it immediately
	if ( !enemy.ent ) {
		SetEnemy ( replacement );
		return true;
	}

	// Dont change enemies too often when being hit
	if ( gameLocal.time - enemy.changeTime < 1000 ) {
		return false;
	}

	replace = false;

	// If new enemy is more threatening then replace it reguardless if we can see it
	if ( CalculateEnemyThreat ( replacement ) > CalculateEnemyThreat ( enemy.ent ) ) {
		replace = true;
	// Replace our enemy if we havent seen ours in a bit
	} else if ( !IsEnemyRecentlyVisible ( 0.25f ) ) {
		replace = true;
	}
	
	// Replace enemy?
	if ( replace ) {
		SetEnemy ( replacement );
	}
	
	return replace;
}

/*
============
idAI::UpdateThreat
============
*/
void idAI::UpdateThreat ( void ) {
	// Start threat at base threat level
	combat.threatCurrent = combat.threatBase;
	
	// Adjust threat using current tactical state
	switch ( combat.tacticalCurrent ) {
		case AITACTICAL_HIDE:	combat.threatCurrent *= 0.5f;	break;
		case AITACTICAL_MELEE:	combat.threatCurrent *= 2.0f;	break;
	}		
	
	// Signifigantly reduced threat when in undying mode
	if ( aifl.undying ) {
		combat.threatCurrent *= 0.25f;
	}
}

/*
============
idAI::CalculateEnemyThreat
============
*/
float idAI::CalculateEnemyThreat ( idEntity* enemyEnt ) {
	// Calculate the adjusted threat for this actor		
	float threat = 1.0f;
	if ( enemyEnt->IsType ( idAI::GetClassType ( ) ) ) {
		idAI* enemyAI = static_cast<idAI*>(enemyEnt);
		threat = enemyAI->combat.threatCurrent;
		
		// Increase threat for enemies that are targetting us
		if ( enemyAI->enemy.ent == this && enemyAI->combat.tacticalCurrent == AITACTICAL_MELEE ) {
			threat *= 2.0f;
		}
	} else if ( enemyEnt->IsType ( idPlayer::GetClassType ( ) ) ) {
		threat = 2.0f; 
	} else {
		threat = 1.0f;
	}				

	float enemyRangeSqr;
	float distSqr;	
	
	enemyRangeSqr = (enemy.ent) ? Square ( enemy.range ) : Square ( combat.attackRange[1] );
	distSqr		  = (physicsObj.GetOrigin ( ) - enemyEnt->GetPhysics()->GetOrigin ( )).LengthSqr ( );
	
	if ( distSqr > 0 ) {
		return threat * (enemyRangeSqr / distSqr);
	}
	
	return threat;
}

/*
============
idAI::CheckBlink
============
*/
void idAI::CheckBlink ( void ) {
//	if ( IsSpeaking ( ) ) {
//		return;
//	}
	idActor::CheckBlink ( );
}

/*
============
idAI::Speak
============
*/
bool idAI::Speak( const char *lipsync, bool random ){
	assert( idStr::Icmpn( lipsync, "lipsync_", 7 ) == 0 );
	
	if ( random ) {
		// If there is no lipsync then skip it
		if ( spawnArgs.MatchPrefix ( lipsync ) ) {
			lipsync = spawnArgs.RandomPrefix ( lipsync, gameLocal.random );
		} else { 
			lipsync = NULL;
		}
	} else {
		lipsync = spawnArgs.GetString ( lipsync );
	}
	
	if ( !lipsync || !*lipsync ) {
		return false;
	}
	
	if ( head ) {
		speakTime = head->StartLipSyncing( lipsync );
	} else {
		speakTime = 0;		
		StartSoundShader (declManager->FindSound ( lipsync ), SND_CHANNEL_VOICE, SSF_IS_VO, false, &speakTime );
	}

	speakTime += gameLocal.time;
	return true;	
}

/*
============
idAI::StopSpeaking
============
*/
void idAI::StopSpeaking( bool stopAnims ){
	speakTime = 0;
	StopSound( SND_CHANNEL_VOICE, false );
	if ( head.GetEntity() ) {
		head.GetEntity()->StopSound( SND_CHANNEL_VOICE, false );
		if ( stopAnims ) {
			head.GetEntity()->GetAnimator()->ClearAllAnims( gameLocal.time, 100 );
		}
	}
}

/*
============
idAI::CanHitEnemyFromAnim
============
*/
bool idAI::CanHitEnemyFromAnim( int animNum, idVec3 offset ) {
	idVec3		dir;
	idVec3		local_dir;
	idVec3		fromPos;
	idMat3		axis;
	idVec3		start;
	trace_t		tr;
	idEntity*	enemyEnt;

	// Need an enemy.
	if ( !enemy.ent ) {
		return false;
	}

	// Enemy actor pointer
	enemyEnt = static_cast<idEntity*>(enemy.ent.GetEntity());

	// just do a ray test if close enough
	if ( enemyEnt->GetPhysics()->GetAbsBounds().IntersectsBounds( physicsObj.GetAbsBounds().Expand( 16.0f ) ) ) {
		return CanHitEnemy();
	}

	// calculate the world transform of the launch position
  	idVec3 org = physicsObj.GetOrigin()+offset;
	idVec3 from;
  	dir = enemy.lastVisibleChestPosition - org;
  	physicsObj.GetGravityAxis().ProjectVector( dir, local_dir );
  	local_dir.z = 0.0f;
  	local_dir.ToVec2().Normalize();
  	axis = local_dir.ToMat3();
  	from = org + attackAnimInfo[ animNum ].attackOffset * axis;

/*
	if( DebugFilter(ai_debugTactical) ) {
		gameRenderWorld->DebugLine ( colorYellow, org + attackAnimInfo[ animNum ].eyeOffset * viewAxis, from, 5000 );
		gameRenderWorld->DebugLine ( colorOrange, from, enemy.lastVisibleEyePosition, 5000 );
	}
*/

	// If the point we are shooting from is within our bounds then we are good to go, otherwise make sure its not in a wall
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	if ( !ownerBounds.ContainsPoint ( from ) ) {
		trace_t tr;
		if ( !g_perfTest_aiNoVisTrace.GetBool() ) {
			gameLocal.TracePoint( this, tr, org + attackAnimInfo[ animNum ].eyeOffset * axis, from, MASK_SHOT_BOUNDINGBOX, this );
			if ( tr.fraction < 1.0f ) {
				return false;
			}
		}
	}		

	return CanSeeFrom ( from, enemy.lastVisibleEyePosition, true );
}

/*
================
idAI::ScriptedBegin
================
*/
bool idAI::ScriptedBegin ( bool endWithIdle, bool allowDormant ) {
	if ( aifl.dead ) {
		return false;
	}
	
	// Wakeup if not awake already
	WakeUp ( );
		
	aifl.scriptedEndWithIdle = endWithIdle;
	aifl.scripted			 = true;
//	combat.fl.aware			 = false;
	combat.tacticalCurrent	 = AITACTICAL_NONE;

	// Make sure the entity never goes dormant during a scripted event or
	// the event may never end.
	aifl.scriptedNeverDormant = !allowDormant;
	dormantStart = 0;

/*		
	// actors will ignore enemies during scripted events
	ClearEnemy ( );
*/
	
	// Cancel any current movement
	StopMove ( MOVE_STATUS_DONE );
	
	move.fl.allowAnimMove = true;
	
	return true;
}

/*
================
idAI::ScriptedEnd
================
*/
void idAI::ScriptedEnd ( void ) {
	dormantStart = 0;
	aifl.scripted = false;
}

/*
================
idAI::ScriptedStop
================
*/
void idAI::ScriptedStop ( void ) {
	if ( !aifl.scripted ) {
		return;
	}
	aifl.scriptedEndWithIdle = true;
	aifl.scripted			 = false;
	StopMove( MOVE_STATUS_DONE );
}

/*
================
idAI::ScriptedMove
================
*/
void idAI::ScriptedMove ( idEntity* destEnt, float minDist, bool endWithIdle ) {
	if ( !ScriptedBegin ( endWithIdle ) ) {
		return;
	}

	//disable all temporary blocked reachabilities due to teammate obstacle avoidance
	aiManager.UnMarkAllReachBlocked();
	//attempt the move - NOTE: this *can* fail if there's no route or AAS obstacles are in the way!
	MoveToEntity ( destEnt, minDist );
	//re-enable all temporary blocked reachabilities due to teammate obstacle avoidance
	aiManager.ReMarkAllReachBlocked();

	// Move the torso to the idle state if its not already there
	SetAnimState( ANIMCHANNEL_TORSO, "Torso_Idle", 4 ); 

	// Move the legs into the idle state so he will start moving
	SetAnimState( ANIMCHANNEL_LEGS, "Legs_Idle", 4 ); 

	// Set up state loop for moving
	SetState ( "State_ScriptedMove" );
	PostState ( "State_ScriptedStop" );
}

/*
================
idAI::ScriptedFace
================
*/
void idAI::ScriptedFace ( idEntity* faceEnt, bool endWithIdle ) {
	if ( !ScriptedBegin ( endWithIdle ) ) {
		return;
	}

	// Force idle while facing
	SetAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 4 ); 
	SetAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 4 ); 

	// Start facing the entity
	FaceEntity ( faceEnt );

	SetState ( "State_ScriptedFace" );
	PostState ( "State_ScriptedStop" );
}

/*
================
idAI::ScriptedAnim

Plays an the given animation in an un-interruptable state.  If looping will continue indefinately until
another operation which will stop a scripted sequence is called. When done can optionally return the character
back to their normal processing if endWithIdle is set to true.
================
*/
void idAI::ScriptedAnim ( const char* animname, int blendFrames, bool loop, bool endWithIdle ) {
	// Start the scripted sequence
	if ( !ScriptedBegin ( endWithIdle, true ) ) {
		return;
	}
	
	TurnToward ( move.current_yaw );

	if ( loop ) {
		// Loop the given animation
		PlayCycle ( ANIMCHANNEL_TORSO, animname, blendFrames );		
	} else {
		// Play the given animation
		PlayAnim ( ANIMCHANNEL_TORSO, animname, blendFrames );
	}
	
	SetAnimState ( ANIMCHANNEL_LEGS, "Wait_Frame" );
	SetAnimState ( ANIMCHANNEL_TORSO, "Torso_ScriptedAnim" );

	DisableAnimState ( ANIMCHANNEL_LEGS );

	SetState ( "Wait_ScriptedDone" );
	PostState ( "State_ScriptedStop" );
}


/*
============
idAI::ScriptedPlaybackAim
============
*/
void idAI::ScriptedPlaybackAim ( const char* playback, int flags, int numFrames ) {
	// Start the scripted sequence
	if ( !ScriptedBegin ( false ) ) {
		return;
	}

	mLookPlayback.Start( spawnArgs.GetString ( playback ), this, flags, numFrames );

	// Wait till its done and mark it finished
	SetState ( "State_ScriptedPlaybackAim" );
	PostState ( "State_ScriptedStop" );
}		

/*
============
idAI::ScriptedAction
============
*/
void idAI::ScriptedAction ( idEntity* actionEnt, bool endWithIdle ) {
	const char* actionName;
	
	if ( !actionEnt ) {
		return;
	}
	
	// Get the action name
	actionName = actionEnt->spawnArgs.GetString ( "action" );
	if ( !*actionName ) {
		gameLocal.Error ( "missing action keyword on scripted action entity '%s' for ai '%s'",
						  actionEnt->GetName(),
						  GetName() );
		return;
	}

	// Start the scripted sequence
	if ( !ScriptedBegin ( endWithIdle ) ) {
		return;
	}

	scriptedActionEnt = actionEnt;
	
	SetState ( "State_ScriptedStop" );
	PerformAction ( va("TorsoAction_%s", actionName ), 4, true );
}

/*
============
idAI::FootStep
============
*/
void idAI::FootStep ( void ) {
	idActor::FootStep ( );
	
	ExecScriptFunction( funcs.footstep );	
}

/*
============
idAI::SetScript
============
*/
void idAI::SetScript( const char* scriptName, const char* funcName ) {
	if ( !funcName || !funcName[0] ) {
		return;
	} 

	// Set the associated script
	if ( !idStr::Icmp ( scriptName, "first_sight" ) ) {
		funcs.first_sight.Init( funcName );
	} else if ( !idStr::Icmp ( scriptName, "sight" ) ) {
		funcs.sight.Init( funcName );
	} else if ( !idStr::Icmp ( scriptName, "pain" ) ) {
		funcs.pain.Init( funcName );
	} else if ( !idStr::Icmp ( scriptName, "damage" ) ) {
		funcs.damage.Init( funcName );
	} else if ( !idStr::Icmp ( scriptName, "death" ) ) {
		funcs.death.Init( funcName );
	} else if ( !idStr::Icmp ( scriptName, "attack" ) ) {
		funcs.attack.Init( funcName );
	} else if ( !idStr::Icmp ( scriptName, "init" ) ) {
		funcs.init.Init( funcName );
	} else if ( !idStr::Icmp ( scriptName, "onclick" ) ) {
		funcs.onclick.Init( funcName );
	} else if ( !idStr::Icmp ( scriptName, "launch_projectile" ) ) {
		funcs.launch_projectile.Init( funcName );
	} else if ( !idStr::Icmp ( scriptName, "footstep" ) ) {
		funcs.footstep.Init( funcName );
	} else if ( !idStr::Icmp ( scriptName, "postHeal" ) ) {
		// hax: this is a medic only script and I don't want to store it on every AI type..
		//	I also don't want it generating the warning below in this case.
	} else if ( !idStr::Icmp ( scriptName, "postWeaponDestroyed" ) ) {
		// hax: this is a Gladiator/Light Tank only script and I don't want to store it on every AI type..
		//	I also don't want it generating the warning below in this case.
	} else {
		gameLocal.Warning ( "unknown script '%s' specified on entity '%s'", scriptName, name.c_str() );
	}
}

/*
===============================================================================

	idAI - Reactions

===============================================================================
*/

/*
============
idAI::ReactToShotAt
============
*/
void idAI::ReactToShotAt ( idEntity* attacker, const idVec3 &origOrigin, const idVec3 &origDir ) {
	if ( g_perfTest_aiNoDodge.GetBool() ) {
		return;
	}

	idVec3 foo;
	idVec3 diff;
	diff = GetPhysics()->GetOrigin() - origOrigin;
	diff = origOrigin + diff.ProjectOntoVector ( origDir ) * origDir;
	diff = diff - GetPhysics()->GetOrigin();
	diff.NormalizeFast ( );
	diff.z = 0;
			
	idAngles angles = diff.ToAngles ( );
	float angleDelta = idMath::AngleDelta ( angles[YAW], move.current_yaw );

	combat.shotAtTime	= gameLocal.time;
	combat.shotAtAngle  = angleDelta;
			
	// Someone is attacking us so give them a chance to be our new enemy
	CheckForReplaceEnemy ( attacker );
}

/*
============
idAI::ReactToPain
============
*/
void idAI::ReactToPain ( idEntity* attacker, int damage ) {
	CheckForReplaceEnemy ( attacker );
}
	
/*
===============================================================================

	idAI - Helpers

===============================================================================
*/

/*
============
idAI::UpdateHelper
============
*/
void idAI::UpdateHelper ( void ) {
	rvAIHelper* oldhelper;
			
	// Link ourselves to the closest helper
	oldhelper     = helperCurrent;
	helperCurrent = aiManager.FindClosestHelper ( physicsObj.GetOrigin() );
	
	// Ideal stays the same as current as long as it was the same when we started
	if ( oldhelper == helperIdeal ) {
		helperIdeal = helperCurrent;
	}
}

/*
============
idAI::GetActiveHelper

When we have an enemy our current helper becomes the active helper, when we dont have an 
enemy we instead use our ideal.
============
*/
rvAIHelper* idAI::GetActiveHelper ( void ) {
	return GetEnemy ( ) ? helperCurrent : helperIdeal;
}

/*
===============================================================================

	idAI - Tethers

===============================================================================
*/

/*
============
idAI::SetTether
============
*/
void idAI::SetTether ( rvAITether* newTether ) {
	aifl.tetherMover = false;
	
	// Clear our current tether?
	if ( !newTether ) {
		if ( tether ) {
			tether = NULL;
			ForceTacticalUpdate ( );
		}
	} else if ( newTether->IsType ( rvAITetherClear::GetClassType ( ) ) ) {
		SetTether ( NULL );
	} else {
		if ( newTether && !newTether->ValidateAAS ( this ) ) {
			// If you have aas error out to make them fix it
			if ( aas ) {
				gameLocal.Error ( "tether entity '%s' does no link into the aas for ai '%s'. (try moving it closer to the floor where the aas is)",
								   newTether->GetName(), GetName ()  );
			// If we dont have aas, just warn								   
			} else {
				gameLocal.Warning ( "tether entity '%s' does no link into the aas for ai '%s'. (there is no aas available)",
								   newTether->GetName(), GetName ()  );
			}				
			SetTether ( NULL );
		} else if ( newTether != tether ) {
			tether = newTether;
			ForceTacticalUpdate ( );
		}
	}
}

/*
============
idAI::GetTether
============
*/
rvAITether* idAI::GetTether ( void ) {
	return tether;
}

/*
============
idAI::IsTethered
============
*/
bool idAI::IsTethered ( void ) const {
	// Need a tether entity to be tethered
	if ( !tether ) {
		return false;
	}
	// If we have an enemy and that enemy is within our tether then break it if we can
	if ( enemy.ent && enemy.ent->IsType ( idAI::GetClassType() ) && tether->CanBreak ( ) ) {
		if ( tether->ValidateDestination ( static_cast<idAI*>(enemy.ent.GetEntity()), enemy.lastKnownPosition ) ) {
			return false;
		}
	}
	return true;
}

/*
============
idAI::IsWithinTether
============
*/
bool idAI::IsWithinTether ( void ) const {
	if ( !IsTethered ( ) ) {
		return false;
	}
	if ( !tether->ValidateDestination ( (idAI*)this, physicsObj.GetOrigin ( ) ) ) {
		return false;
	}
	return true;
}

/*
===============================================================================

	idAI - NonCombat

===============================================================================
*/

/*
=====================
idAI::SetTalkState
=====================
*/
void idAI::SetTalkState ( talkState_t state ) {
	// Make sure state is valid
	if ( ( state < 0 ) || ( state >= NUM_TALK_STATES ) ) {
		gameLocal.Error( "Invalid talk state (%d)", (int)state ); 
	}
	
	// Same state we are already in?
	if ( talkState == state ) {
		return;
	}
	
	// Set new talk state
	talkState = state;

}

/*
============
idAI::SetPassivePrefix
============
*/
void idAI::SetPassivePrefix ( const char* prefix ) {
	passive.prefix = prefix;
	if ( passive.prefix.Length() ) {
		passive.prefix += "_";
	}

	// Force an idle change
	passive.idleAnimChangeTime = 0;	
	passive.fidgetTime		   = 0;
	passive.talkTime		   = 0;
	
	// Get animation prefixs
	passive.fl.multipleIdles = GetPassiveAnimPrefix ( "idle",		passive.animIdlePrefix );	
							   GetPassiveAnimPrefix ( "fidget",		passive.animFidgetPrefix );
							   GetPassiveAnimPrefix ( "talk",		passive.animTalkPrefix );
}

/*
============
idAI::GetPassiveAnimPrefix
============
*/
bool idAI::GetPassiveAnimPrefix ( const char* animName, idStr& animPrefix ) {
	const idKeyValue* key;
	
	// First see if we have custom idle animations for the passive prefix
	key = NULL;
	if ( passive.prefix.Length ( ) ) {
		animPrefix = va("anim_%s%s", passive.prefix.c_str(), animName );
		key		   = spawnArgs.MatchPrefix ( animPrefix );
	}
	
	// If there are no custom idle animations for the prefix then see if there are any custom anims at all
	if ( !key ) {
		animPrefix = va("anim_%s", animName );
		key		   = spawnArgs.MatchPrefix ( animPrefix );
	}
	
	if ( !key ) {
		animPrefix = "";
		return false;
	}
	
	return spawnArgs.MatchPrefix ( animPrefix, key ) ? true : false;
}

/*
===================
idAI::IsMeleeNeeded
===================
*/
bool idAI::IsMeleeNeeded( void )	{

	if( enemy.ent && enemy.ent->IsType ( idAI::Type ))	{
		
		idAI* enemyAI = static_cast<idAI*>(enemy.ent.GetEntity());

		//if our enemy is closing in on us and demands melee, we'll meet him.
		if ( enemyAI->combat.tacticalCurrent == AITACTICAL_MELEE && enemy.range < combat.meleeRange ) {
			return true;
		}
	
		//other checks...
	}
	
	return false;
}

/*
===================
idAI::IsCrouching
===================
*/
bool idAI::IsCrouching( void ) const {
	return move.fl.crouching;
}

bool idAI::CheckDeathCausesMissionFailure( void )
{
	if ( spawnArgs.GetString( "objectivetitle_failed", NULL ) )
	{
		return true;
	}
	if ( targets.Num() )
	{
		//go through my targets and see if any are of class rvObjectiveFailed
		idEntity* targEnt;
		for( int i = 0; i < targets.Num(); i++ ) {
			targEnt = targets[ i ].GetEntity();
			if ( !targEnt )
			{
				continue;
			}
			if ( !targEnt->IsType( rvObjectiveFailed::GetClassType() ) ) {
				continue;
			}
			if ( !spawnArgs.GetString( "inv_objective", NULL ) ) {
				continue;
			}
			//yep!
			return true;
		}
	}
	return false;
}
