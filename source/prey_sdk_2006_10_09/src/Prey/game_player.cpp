
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#define DAMAGE_INDICATOR_TIME		1100		// Update this in hud_damageindicator.guifragment too

const idEventDef EV_PlayWeaponAnim( "playWeaponAnim", "sd" );
const idEventDef EV_RechargeHealth( "<rechargehealth>", NULL );
const idEventDef EV_RechargeRifleAmmo( "<rechargeRifleAmmo>", NULL );
const idEventDef EV_Cinematic( "cinematic", "dd" );
const idEventDef EV_DialogStart( "dialogStart", "ddd" );
const idEventDef EV_DialogStop( "dialogStop", NULL );
const idEventDef EV_LotaTunnelMode( "lotaTunnelMode", "d" );
const idEventDef EV_DrainSpiritPower( "<drainspiritpower>", NULL );
const idEventDef EV_SpawnDeathWraith( "<spawndeathwraith>", NULL );
const idEventDef EV_PrepareToResurrect( "prepareToResurrect" );
const idEventDef EV_ResurrectScreenFade( "<resurrectScreenFade>" );
const idEventDef EV_Resurrect( "resurrect" );
const idEventDef EV_PrepareForDeathWorld( "<prepareForDeathWorld>", NULL );
const idEventDef EV_EnterDeathWorld( "<enterDeathWorld>", NULL );
const idEventDef EV_AdjustSpiritPowerDeathWalk( "<adjustSpiritPowerDeathWalk>" );
const idEventDef EV_SetOverlayMaterial( "setOverlayMaterial", "sd" );
const idEventDef EV_SetOverlayTime( "setOverlayTime", "fd" );
const idEventDef EV_SetOverlayColor( "setOverlayColor", "ffff" );
const idEventDef EV_DDAHeartbeat( "<ddaHeartBeat>", NULL );
const idEventDef EV_ShouldRemainAlignedToAxial( "shouldRemainAlignedToAxial", "d" );
const idEventDef EV_StartHudTranslation( "<starthudtranslation>" );
const idEventDef EV_Unfreeze( "<unfreeze>" );
const idEventDef EV_GetSpiritPower( "getSpiritPower", "", 'f' ); //rww
const idEventDef EV_SetSpiritPower( "setSpiritPower", "f" ); //rww
const idEventDef EV_OnGround( "onGround", NULL, 'f' ); // bg
const idEventDef EV_BindUnfroze( "<bindUnfroze>", "e" ); // mdl
const idEventDef EV_LockWeapon( "lockWeapon", "d" ); // mdl
const idEventDef EV_UnlockWeapon( "unlockWeapon", "d" ); // mdl
const idEventDef EV_SetPrivateCameraView( "setPrivateCameraView", "Ed" ); //rdr
const idEventDef EV_SetCinematicFOV( "setCinematicFOV", "ffff" ); //rdr
const idEventDef EV_StopSpiritWalk( "stopSpiritWalk" ); //rww
const idEventDef EV_DamagePlayer( "damagePlayer", "eevsfd" ); //rww
const idEventDef EV_GetSpiritProxy( "getSpiritProxy", "", 'e' ); //jsh
const idEventDef EV_IsSpiritWalking( "isSpiritWalking", "", 'd' ); // pdm
const idEventDef EV_IsDeathWalking( "isDeathWalking", "", 'd' ); // bjk
const idEventDef EV_GetDDAValue( "getDDAValue", "", 'f' ); // cjr
const idEventDef EV_AllowLighter( "allowLighter", "d" ); // jsh
const idEventDef EV_DisableSpirit( "disableSpirit" ); // mdl
const idEventDef EV_EnableSpirit( "enableSpirit" ); // mdl
const idEventDef EV_UpdateDDA( "<updateDDA>" ); // cjr
const idEventDef EV_AllowDamage( "<allowDamage>" ); // mdl
const idEventDef EV_IgnoreDamage( "<ignoreDamage>" ); // mdl
const idEventDef EV_RespawnCleanup( "<respawncleanup>" ); //rww
const idEventDef EV_ReturnToWeapon( "returnToWeapon", "", 'd' ); //bjk
const idEventDef EV_CanAnimateTorso( "canAnimateTorso", "", 'd' ); //rww

CLASS_DECLARATION( idPlayer, hhPlayer )
	EVENT( EV_PlayWeaponAnim,				hhPlayer::Event_PlayWeaponAnim )
	EVENT( EV_RechargeHealth,				hhPlayer::Event_RechargeHealth )
	EVENT( EV_RechargeRifleAmmo,			hhPlayer::Event_RechargeRifleAmmo )
	EVENT( EV_Cinematic,					hhPlayer::Event_Cinematic )
	EVENT( EV_DialogStart,					hhPlayer::Event_DialogStart )
	EVENT( EV_DialogStop,					hhPlayer::Event_DialogStop )
	EVENT( EV_LotaTunnelMode,				hhPlayer::Event_LotaTunnelMode )
	EVENT( EV_DrainSpiritPower,				hhPlayer::Event_DrainSpiritPower )
	EVENT( EV_SpawnDeathWraith,				hhPlayer::Event_SpawnDeathWraith )
	EVENT( EV_PrepareToResurrect,			hhPlayer::Event_PrepareToResurrect )
	EVENT( EV_ResurrectScreenFade,			hhPlayer::Event_ResurrectScreenFade )
	EVENT( EV_Resurrect,					hhPlayer::Event_Resurrect )
	EVENT( EV_EnterDeathWorld,				hhPlayer::Event_EnterDeathWorld )
	EVENT( EV_PrepareForDeathWorld,			hhPlayer::Event_PrepareForDeathWorld )
	EVENT( EV_AdjustSpiritPowerDeathWalk,	hhPlayer::Event_AdjustSpiritPowerDeathWalk )
	EVENT( EV_ShouldRemainAlignedToAxial,	hhPlayer::Event_ShouldRemainAlignedToAxial )
	EVENT( EV_OrientToGravity,				hhPlayer::Event_OrientToGravity )
	EVENT( EV_ResetGravity,					hhPlayer::Event_ResetGravity )
	EVENT( EV_SetOverlayMaterial,			hhPlayer::Event_SetOverlayMaterial )
	EVENT( EV_SetOverlayTime,				hhPlayer::Event_SetOverlayTime )
	EVENT( EV_SetOverlayColor,				hhPlayer::Event_SetOverlayColor )
	EVENT( EV_DDAHeartbeat,					hhPlayer::Event_DDAHeartBeat )
	EVENT( EV_StartHudTranslation,			hhPlayer::Event_StartHUDTranslation)
	EVENT( EV_Unfreeze,						hhPlayer::Event_Unfreeze)
	EVENT( EV_GetSpiritPower,				hhPlayer::Event_GetSpiritPower) //rww
	EVENT( EV_SetSpiritPower,				hhPlayer::Event_SetSpiritPower) //rww
	EVENT( EV_OnGround,						hhPlayer::Event_OnGround ) // bg
	EVENT( EV_LockWeapon,					hhPlayer::Event_LockWeapon ) // mdl
	EVENT( EV_UnlockWeapon,					hhPlayer::Event_UnlockWeapon ) // mdl
	EVENT( EV_SetPrivateCameraView,			hhPlayer::Event_SetPrivateCameraView ) //rdr
	EVENT( EV_SetCinematicFOV,				hhPlayer::Event_SetCinematicFOV ) //rdr
	EVENT( EV_StopSpiritWalk,				hhPlayer::Event_StopSpiritWalk ) //rww
	EVENT( EV_DamagePlayer,					hhPlayer::Event_DamagePlayer ) //rww
	EVENT( EV_GetSpiritProxy,				hhPlayer::Event_GetSpiritProxy ) //rww
	EVENT( EV_IsSpiritWalking,				hhPlayer::Event_IsSpiritWalking ) //pdm
	EVENT( EV_IsDeathWalking,				hhPlayer::Event_IsDeathWalking ) //bjk
	EVENT( EV_GetDDAValue,					hhPlayer::Event_GetDDAValue ) // cjr
	EVENT( EV_AllowLighter,					hhPlayer::Event_AllowLighter ) // jsh
	EVENT( EV_DisableSpirit,				hhPlayer::Event_DisableSpirit ) // mdl
	EVENT( EV_EnableSpirit,					hhPlayer::Event_EnableSpirit ) // mdl
	EVENT( EV_UpdateDDA,					hhPlayer::Event_UpdateDDA ) // cjr
	EVENT( EV_AllowDamage,					hhPlayer::Event_AllowDamage ) // mdl
	EVENT( EV_IgnoreDamage,					hhPlayer::Event_IgnoreDamage ) // mdl
	EVENT( EV_RespawnCleanup,				hhPlayer::Event_RespawnCleanup ) //rww
	EVENT( EV_ReturnToWeapon,				hhPlayer::Event_ReturnToWeapon ) //bjk
	EVENT( EV_CanAnimateTorso,				hhPlayer::Event_CanAnimateTorso ) //rww
END_CLASS

//rww
CLASS_DECLARATION( hhPlayer, hhArtificialPlayer )
END_CLASS

/*
=================
hhPlayer::hhPlayer
=================
*/
hhPlayer::hhPlayer( void ) :
	thirdPersonCameraClipBounds( idTraceModel(idBounds(idVec3(-4, -8, -8), idVec3(8, 8, 8))) ) {

	hand				= NULL;	// nla
	spiritProxy			= NULL;
	possessedTommy		= NULL;
	talon				= NULL;
	handNext			= NULL;
	deathLookAtEntity	= NULL;
	guiWantsControls	= NULL;
	lighterHandle		= -1;
	nextTalonAttackCommentTime = 0;
	bTalonAttackComment = false;
	bSpiritWalk			= false;
	bDeathWalk			= false;
	bReallyDead			= false;
	bPossessed			= false;
	bInCinematic		= false;
	bFrozen				= false;
	bScopeView			= false; //rww
	bInDeathwalkTransition = false;
	lastAppliedBobCycle = 0;
#if GAMEPAD_SUPPORT	// VENOM BEGIN
	lastAutoLevelTime	= 0;
	lastAccelTime		= 0;
	lastAccelFactor		= 1.0f;
#endif // VENOM END
	cinematicFOV.Init( gameLocal.time, 0.f, 0.f, 0.f, 90.f, 90.f ); //rdr
	mpHitFeedbackTime	= 0; //rww
	bAllowSpirit		= true; //mdl
	bCollidingWithPortal = false;
	bPlayingLowHealthSound = false;
	bLotaTunnelMode = false;

	for (int ix=0; ix<MAX_TRACKED_ATTACKERS; ix++) {
		lastAttackers[ix].attacker = NULL;
		lastAttackers[ix].time = 0;
		lastAttackers[ix].displayed = false;
	}

	forcePredictionButtons = 0; //rww
	//HUMANHEAD PCF mdl 05/02/06 - Initialize weaponFlags to prevent problems during spawn
	weaponFlags = -1;

	//HUMANHEAD PCF rww 05/03/06 - initialize bob to 0
	bob = 0.0f;
}

void hhPlayer::LinkScriptVariables( void ) {
	idPlayer::LinkScriptVariables();

	AI_ASIDE.LinkTo( scriptObject, "AI_ASIDE" );
}

/*
==============
hhPlayer::Spawn

Prepare any resources used by the player.
==============
*/
void hhPlayer::Spawn( void ) {

	SetupWeaponInfo();

	spiritDrainHeartbeatMS = SEC2MS(spawnArgs.GetFloat( "spiritDrainHeartbeat", "0.15" ));

	ddaHeartbeatMS = SEC2MS(spawnArgs.GetFloat( "dda_heartbeat", "1" ));
	ddaNumEnemies = 0;
	ddaProbabilityAccum = 1.0f;

	// Start DDA heartbeat events, only in single player
	if ( !gameLocal.isMultiplayer ) {
		PostEventMS( &EV_DDAHeartbeat, ddaHeartbeatMS );
	}

	//Setup crashland stuff
	float gravityMagnitude = physicsObj.GetGravity().Length();
	crashlandSpeed_fatal = hhUtils::DetermineFinalFallVelocityMagnitude( spawnArgs.GetFloat("fallDist_fatal"), gravityMagnitude );
	crashlandSpeed_soft = hhUtils::DetermineFinalFallVelocityMagnitude( spawnArgs.GetFloat("fallDist_soft"), gravityMagnitude );;
	crashlandSpeed_jump = hhUtils::DetermineFinalFallVelocityMagnitude( spawnArgs.GetFloat("fallDist_jump"), gravityMagnitude );;

	spiritwalkSoundController.SetLoopOnlyOnLocal(entityNumber); //rww
	spiritwalkSoundController.SetOwner( this );
	spiritwalkSoundController.SetLeadIn( spawnArgs.GetString("snd_spiritWalkActivate") );
	spiritwalkSoundController.SetLoop( spawnArgs.GetString("snd_spiritWalkIdle") );
	spiritwalkSoundController.SetLeadOut( spawnArgs.GetString("snd_spiritWalkDeactivate") );
	spiritwalkSoundController.bPlaying = false;

	deathwalkSoundController.SetOwner( this );
	deathwalkSoundController.SetLeadIn( spawnArgs.GetString("snd_deathWalkActivate") );
	deathwalkSoundController.SetLoop( spawnArgs.GetString("snd_deathWalkIdle") );
	deathwalkSoundController.SetLeadOut( spawnArgs.GetString("snd_deathWalkDeactivate") );
	deathwalkSoundController.bPlaying = false;

	wallwalkSoundController.SetOwner( this );
	wallwalkSoundController.SetLeadIn( spawnArgs.GetString("snd_wallwalkActivate") );
	wallwalkSoundController.SetLoop( spawnArgs.GetString("snd_wallwalkIdle") );
	wallwalkSoundController.SetLeadOut( spawnArgs.GetString("snd_wallwalkDeactivate") );
	wallwalkSoundController.bPlaying = false;

	lastResurrectTime	= 0;

	SetVehicleInterface( &vehicleInterfaceLocal );

	kickSpring = spawnArgs.GetFloat( "kickSpring" );	//HUMANHEAD bjk
	kickDamping = spawnArgs.GetFloat( "kickDamping" );	//HUMANHEAD bjk

	// mdl: Make sure the lighter is cleared
	memset( &lighter, 0, sizeof( lighter ) );

	SetupWeaponFlags();
	if ( idealWeapon < 1 || idealWeapon > 8 ) {
		// Go to the highest weapon available if no weapon is current selected (NOTE:  This will be ignored if all weapons are locked)
		NextBestWeapon();	//HUMANHEAD bjk
	} 

	//HUMANHEAD PCF mdl 04/26/06 - Made this a separate if block from above so we can disallow locked weapons
	if ( ! ( weaponFlags & ( 1 << ( idealWeapon - 1 ) ) ) ) {
		// If the current weapon is invalid now, try the next weapon
		NextWeapon();
		if ( ! ( weaponFlags & ( 1 << ( idealWeapon - 1 ) ) ) ) {
			// No weapons available
			if ( weapon.GetEntity() ) {
				weapon.GetEntity()->PutAway();
				weapon.GetEntity()->HideWeapon();
			}
			idealWeapon = 0;
			currentWeapon = -1;
		}
	}

	nextSpiritTime = 0;

	mpHitFeedbackTime = 0; //rww

	airAttackerTime = 0;
	bDeathWalkStage2 = false;

	physicsObj.SetInwardGravity(-1); //rww
}

void hhPlayer::RestorePersistantInfo( void ) {
	int num;

	idPlayer::RestorePersistantInfo();

	// Update persistent amoo maximums
	num = GetWeaponNum("weaponobj_soulstripper");
	assert(num);
	weaponInfo[ num ].ammoMax = spawnArgs.GetInt( "max_ammo_energy" );

	num = GetWeaponNum("weaponobj_bow");
	assert(num);
	weaponInfo[ num ].ammoMax = spawnArgs.GetInt( "max_ammo_spiritpower" );
}

/*
==============
hhPlayer::SpawnTalon()
==============
*/
void hhPlayer::TrySpawnTalon() {
	// TODO:  Deal with co-op issues.  Have multiple Talons in co-op, or just one?
	if (inventory.requirements.bCanSummonTalon && !talon.IsValid()) {
		talon = (hhTalon *)gameLocal.SpawnObject( spawnArgs.GetString( "def_hawkpower" ), NULL );
		if( talon.IsValid() ) {
			talon->SetOwner( this );
			talon->SummonTalon();
			bTalonAttackComment = true;
			nextTalonAttackCommentTime = 0;
		}
	}
}

/*
==============
hhPlayer::TalonAttackComment

Talon is attacking an enemy, so have Tommy make a comment
==============
*/
void hhPlayer::TalonAttackComment() {
	if ( IsDeathWalking() ) {
		return;
	}

	// Play a sound on the player to encourage Talon to attack
	if ( bTalonAttackComment && gameLocal.GetTime() > nextTalonAttackCommentTime ) { // Player can make an attack comment
		int num = spawnArgs.GetInt( "numAttackComments", "6" );

		StartSound( va( "snd_talonattack%d", gameLocal.random.RandomInt( num ) ), SND_CHANNEL_VOICE, 0, false, NULL );
		nextTalonAttackCommentTime = gameLocal.GetTime() + spawnArgs.GetInt( "talonAttackCommentTime", "30000" );
	}
}

/*
==============
hhPlayer::~hhPlayer()

Release any resources used by the player.
==============
*/
hhPlayer::~hhPlayer() {
	//rww - remove me if i'm a pilot
	if (InVehicle() && GetVehicleInterface() && GetVehicleInterface()->GetVehicle()) {
		GetVehicleInterface()->GetVehicle()->EjectPilot();
	}

	StopSpiritWalk();

	RemoveResources();
}


void hhPlayer::RemoveResources() {
	LighterOff();
	SAFE_REMOVE( hand );		//FIXME: Should this be something other than a safe remove?  (ie, remove hand?)
	SAFE_REMOVE( talon );
	SAFE_REMOVE( spiritProxy );
	SAFE_REMOVE( possessedTommy );
}


/*
==============
hhPlayer::Init()

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
Called during SpawnToPoint
==============
*/
void hhPlayer::Init() {
	oldCmdAngles.Zero();

	idPlayer::Init();

	// initialize the script variables
	AI_ASIDE			= false;

	bInDeathwalkTransition = false;
	bShowProgressBar	= false;
	progressBarValue	= 0.0f;
	progressBarState	= 0;
	progressBarGuiValue.Init(gameLocal.time, 0, 0.0f, 0.0f);

	LighterOff(); //rww - make sure lighter is removed first (primarily for mp)

	bobFrac				= 0.0f;
	lighterTemperature	= 0;
	lighterHandle		= -1;
	bPossessed			= false;	// cjr - player is unpossessed at the start
	possessionTimer		= 0;		// cjr - player is unpossessed at the start
	possessionFOV		= 0;		// cjr - no FOV change for possession at start
	nextTalonAttackCommentTime = 0;
	bTalonAttackComment = false;	// cjr - If Tommy can make comments about talon attacking enemies
	bSpiritWalk			= false;	// cjr - initialize spiritwalk variables
	spiritProxy			= NULL;		// cjr - initialize spiritwalk variables
	possessedTommy		= NULL;		// cjr - initialize possessed tommy
	lastWeaponSpirit	= 0;		// cjr - initialize spiritwalk variables
	guiWantsControls	= NULL;		// pdm - gui that controls should be routed to
	bClampYaw			= false;	// pdm - whether do clamp yaw to master's axis (for rail rides)
	maxRelativeYaw		= 180.0f;	// pdm - max deviation from master axis allowed when clamping yaw
	maxRelativePitch	= 0.0f;		// rww - max deviation from master axis allowed when clamping pitch
	bInCinematic		= false;	// pdm - our simple cinematic system (position locking)
	lockView			= false;
	preCinematicWeapon	= 0;
	preCinematicWeaponFlags = 0;
	guiOverlay			= NULL;		// pdm - current overlay gui
	spiritWalkToggleTime = 0;
	bLotaTunnelMode			= false;

	StopSound(SND_CHANNEL_HEART, false); //rww - make sure this is stopped here, could conceivably still be playing on respawn
	bPlayingLowHealthSound = false;

	// Set shuttle view off
	if (entityNumber == gameLocal.localClientNum) { //rww
		renderSystem->SetShuttleView( false );
	}

	if (gameLocal.isMultiplayer) { //rww - reset my really-deadness when i respawn in MP
		bReallyDead = false;
		SetSkinByName(NULL); //make sure we are not using spiritwalk skin.
	}

	SetViewAnglesSensitivity(1.0f);
	cameraInterpolator.SetSelf( this );

	for (int ix=0; ix<MAX_TRACKED_ATTACKERS; ix++) {
		lastAttackers[ix].attacker = NULL;
		lastAttackers[ix].time = 0;
		lastAttackers[ix].displayed = false;
	}

	physicsObj.SetInwardGravity(-1); //rww
}

/*
===========
hhPlayer::SpawnToPoint

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState

HUMANHEAD pdm: Many things are initialized in here as this is called before hhPlayer::Spawn. (from idPlayer::Spawn())
Most of it probably belongs in ::Init() for clarity.
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge (big chunk taken from idPlayer::SpawnToPoint)
============
*/
void hhPlayer::SpawnToPoint( const idVec3 &spawn_origin, const idAngles &spawn_angles ) {	

	RemoveResources();

	// HUMANHEAD aob
	SetAngles( spawn_angles );						// added because we might die while wallwalking
	SetUntransformedViewAngles( spawn_angles );		// Must be initialized before first think below
	SetUntransformedViewAxis( idAngles(0.0f, spawn_angles.yaw, 0.0f).ToMat3() );
	SetViewAnglesSensitivity( 1.0f );				// Must be initialized before first think below
	//HUMANHEAD END

	// idPlayer::SpawnToPoint() code ------------------------------
	idVec3 spec_origin;

	assert( !gameLocal.isClient );

	respawning = true;

	//HUMANHEAD rww - moved this up above call to Init, since things in there can query player's viewpos.
	kickSpring = spawnArgs.GetFloat( "kickSpring" );	//HUMANHEAD bjk
	kickDamping = spawnArgs.GetFloat( "kickDamping" );	//HUMANHEAD bjk

	Init();

	fl.noknockback = false;

	// stop any ragdolls being used
	StopRagdoll();

	// set back the player physics
	SetPhysics( &physicsObj );

	physicsObj.SetClipModelAxis();
	physicsObj.EnableClip();

	if ( !spectating ) {
		SetCombatContents( true );
	}

	physicsObj.SetLinearVelocity( vec3_origin );

	// setup our initial view
	if ( !spectating ) {
		SetOrigin( spawn_origin );
	} else {
		spec_origin = spawn_origin;
		spec_origin[ 2 ] += pm_normalheight.GetFloat();
		spec_origin[ 2 ] += SPECTATE_RAISE;
		SetOrigin( spec_origin );
	}

	// if this is the first spawn of the map, we don't have a usercmd yet,
	// so the delta angles won't be correct.  This will be fixed on the first think.
	viewAngles = ang_zero;
	SetDeltaViewAngles( ang_zero );
	SetViewAngles( spawn_angles );
	spawnAngles = spawn_angles;
	spawnAnglesSet = false;

	legsForward = true;
	legsYaw = 0.0f;
	idealLegsYaw = 0.0f;
	oldViewYaw = viewAngles.yaw;

	if ( spectating ) {
		Hide();
	} else {
		Show();
	}

	if ( gameLocal.isMultiplayer ) {
#ifndef HUMANHEAD //jsh We don't have a teleport effect
		if ( !spectating ) {
			// we may be called twice in a row in some situations. avoid a double fx and 'fly to the roof'
			if ( lastTeleFX < gameLocal.time - 1000 ) {
				idEntityFx::StartFx( spawnArgs.GetString( "fx_spawn" ), &spawn_origin, NULL, this, true );
				lastTeleFX = gameLocal.time;
			}
		}
#endif
		AI_TELEPORT = false;//true; //HUMANHEAD rww - we don't have a teleport anim either, ha ha ha (well we do, but it's not done)
	} else {
		AI_TELEPORT = false;
	}

	// kill anything at the new position
	if ( !spectating ) {
		physicsObj.SetClipMask( MASK_PLAYERSOLID ); // the clip mask is usually maintained in Move(), but KillBox requires it
		gameLocal.KillBox( this );
	}

	// don't allow full run speed for a bit
	physicsObj.SetKnockBack( 100 );

	// set our respawn time and buttons so that if we're killed we don't respawn immediately
	minRespawnTime = gameLocal.time;
	maxRespawnTime = gameLocal.time;
	if ( !spectating ) {
		forceRespawn = false;
	}

	privateCameraView = NULL;

	BecomeActive( TH_THINK );

	// HUMANHEAD pdm: Think moved after our initialization

	respawning			= false;
	lastManOver			= false;
	lastManPlayAgain	= false;
	isTelefragged		= false;
	// idPlayer::SpawnToPoint() code end---------------------------
	// ------------------------------------------------------------

	// aob - added because we might die while wallwalking
	physicsObj.SetGravity( -(spawn_angles.ToMat3()[2]) * gameLocal.GetGravity().Length() );
	physicsObj.SetAxis( idAngles( spawn_angles.pitch, 0.0f, spawn_angles.roll ).ToMat3() );
	physicsObj.CheckWallWalk( true );

	SetEyeHeight( pm_normalviewheight.GetFloat() );

	bob = 0.0f;
	lastAppliedBobCycle = 0.0f;
	cameraInterpolator.SetSelf( this );
	cameraInterpolator.Setup( p_camRotRateScale.GetFloat(), IT_VariableMidPointSinusoidal );
	cameraInterpolator.Reset( spawn_origin, idAngles(spawn_angles.pitch, 0.0f, spawn_angles.roll).ToMat3()[2], EyeHeightIdeal() );

	// Needed after parent call, as these are set in the parent call
	//HUMANHEAD: aob
	prevStepUpOrigin = cameraInterpolator.GetCurrentPosition();
	//HUMANHEAD END

	if ( !gameLocal.isMultiplayer ) { // Only recharge health and rifle ammo in single player
		lastDamagedTime = gameLocal.time; // CJR PCF 04/27/06:  Ensure that lastDamagedTime is set probably at level load
		PostEventSec( &EV_RechargeHealth, spawnArgs.GetFloat( "healthRechargeRate", "0.5" ) );
		PostEventSec( &EV_RechargeRifleAmmo, spawnArgs.GetFloat( "rifleAmmoRechargeRate", "2" ) );

		// Start the DDA update code
		PostEventSec( &EV_UpdateDDA, spawnArgs.GetFloat( "updateDDARate", "0.25" ) );
	}

	//HUMANHEAD pdm: Moved from id code above to be after our initialization
	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	Think();
}

/*
==============
hhPlayer::SetupWeaponEntity
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
==============
*/
void hhPlayer::SetupWeaponEntity( void ) {
	weapon.Clear();
	InvalidateCurrentWeapon();
}

/*
================
hhPlayer::ApplyImpulse
================
*/
void hhPlayer::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) {
	if( ent && (ent->IsType(hhForceField::Type) || ent->IsType(hhDeathWraith::Type)) ) {
		idPlayer::ApplyImpulse( ent, id, point, impulse );
	}
}

/*
================
hhPlayer::CL_UpdateProgress
	Client function to update progress related variables
	called after getting a snapshot, or directly if a local client
================
*/
void hhPlayer::CL_UpdateProgress(bool bBar, float value, int state) {
	idUserInterface *_hud = NULL;

	if (InVehicle() && GetVehicleInterfaceLocal() && GetVehicleInterfaceLocal()->GetHUD()) {
		_hud = GetVehicleInterfaceLocal()->GetHUD();
	}
	else {
		_hud = hud;
	}

	// Update progress bar
	if (bBar && !bShowProgressBar) {
		_hud->HandleNamedEvent("ShowProgressBar");
	}
	else if (!bBar && bShowProgressBar) {
		_hud->HandleNamedEvent("HideProgressBar");
	}

	// See if we just entered a victory state
	if (state != progressBarState) {
		switch (state) {
			case 0:
				_hud->HandleNamedEvent("ProgressNone");
				break;
			case 1:
				_hud->HandleNamedEvent("ProgressVictory");
				break;
			case 2:
				_hud->HandleNamedEvent("ProgressFailure");
				break;
		}
	}

	// Interpolate gui value to target value
	float curValue = progressBarGuiValue.GetCurrentValue(gameLocal.time);
	progressBarGuiValue.Init(gameLocal.time, 500, curValue, value);

	// Update variables
	bShowProgressBar = bBar;
	progressBarValue = value;
	progressBarState = state;
}

// This called once per tick, put things in here that should be updated regardless of frame rate
//	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
void hhPlayer::UpdateHud( idUserInterface *_hud ) {
	idPlayer *aimed;

	if ( !_hud ) {
		return;
	}

	if ( entityNumber != gameLocal.localClientNum ) {
		return;
	}

#define PICKUPTIME_FADEIN_START		0
#define PICKUPTIME_FADEIN_END		500
#define PICKUPTIME_FADEOUT_START	1750
#define PICKUPTIME_FADEOUT_END		2000
#define MULTIPLEITEM_SPEEDUP		7
	int c = inventory.pickupItemNames.Num();
	int time;
	int i;
	int pickupTimeFadeInStart, pickupTimeFadeInStop, pickupTimeFadeOutStart, pickupTimeFadeOutStop;
	float scale = 1.0f / (((c-1.0f)/9.0f) * (MULTIPLEITEM_SPEEDUP-1.0f) + 1.0f);
	pickupTimeFadeInStart = PICKUPTIME_FADEIN_START;
	pickupTimeFadeInStop = PICKUPTIME_FADEIN_END;
	pickupTimeFadeOutStart = PICKUPTIME_FADEOUT_START * scale;
	pickupTimeFadeOutStop = PICKUPTIME_FADEOUT_END * scale;

	// Determine fade in color for icons
	if (c > 0) {
		for ( i = 0; i < c; i++ ) {
			inventory.pickupItemNames[i].time += gameLocal.msec;
			time = inventory.pickupItemNames[i].time;

			if (time < pickupTimeFadeInStart) {
				inventory.pickupItemNames[i].matcolorAlpha = 0.0f;
			}
			else if (time < pickupTimeFadeInStop) {
				inventory.pickupItemNames[i].matcolorAlpha = ((float)(time-pickupTimeFadeInStart))/(pickupTimeFadeInStop-pickupTimeFadeInStart);
			}
			else {
				inventory.pickupItemNames[i].matcolorAlpha = 1.0f;
			}
		}
	
		// Determine fadeout of slot zero icon
		inventory.pickupItemNames[0].slotZeroTime += gameLocal.msec;
		time = inventory.pickupItemNames[0].slotZeroTime;
		if (time < pickupTimeFadeOutStart) {
			inventory.pickupItemNames[0].matcolorAlpha = 1.0f;
		}
		else if (time < pickupTimeFadeOutStop) {
			inventory.pickupItemNames[0].matcolorAlpha = 1.0f - ((float)(time-pickupTimeFadeOutStart))/(pickupTimeFadeOutStop-pickupTimeFadeOutStart);
		}
		else {
			inventory.pickupItemNames[0].matcolorAlpha = 0.0f;
		}

		// Remove any expired icons
		time = inventory.pickupItemNames[0].slotZeroTime;
		if (time > pickupTimeFadeOutStop) {
			// icon has faded out, remove from list and slide them down
			inventory.pickupItemNames.RemoveIndex(0);
		}
	}

	// HUMANHEAD pdm: Changed aim logic to always show color-coded aimee
	if ( MPAim != -1 && gameLocal.entities[ MPAim ]
			&& gameLocal.entities[ MPAim ]->IsType( idPlayer::Type ) ) {

		aimed = static_cast< idPlayer * >( gameLocal.entities[ MPAim ] );
		_hud->SetStateString( "aim_text", gameLocal.userInfo[ MPAim ].GetString( "ui_name" ) );
		idVec4 teamColor = aimed->GetTeamColor();
		_hud->SetStateFloat( "aim_R", teamColor.x );
		_hud->SetStateFloat( "aim_G", teamColor.y );
		_hud->SetStateFloat( "aim_B", teamColor.z );

		//HUMANHEAD rww - health display on hud for gameplay testing
		if (g_showAimHealth.GetBool()) {
			_hud->SetStateInt("aim_health", gameLocal.entities[MPAim]->health);
		}
		//HUMANHEAD END
	}
	else {
		_hud->SetStateString( "aim_text", "" );

		//HUMANHEAD rww
		_hud->SetStateInt("aim_health", 0);
		//HUMANHEAD END
	}

#if !GOLD
	_hud->SetStateInt( "g_showProjectilePct", g_showProjectilePct.GetInteger() );
	if ( numProjectilesFired ) {
		_hud->SetStateString( "projectilepct", va( "Hit %% %.1f", ( (float) numProjectileHits / numProjectilesFired ) * 100 ) );
	} else {
		_hud->SetStateString( "projectilepct", "Hit % 0.0" );
	}
#endif

	if (gameLocal.isNewFrame) { //HUMANHEAD rww - only on newframe
		// Update low health sound
		if ( health > 0 && health < 25 && !IsSpiritOrDeathwalking() ) {
			if (!bPlayingLowHealthSound) {
				StartSound("snd_lowhealth", SND_CHANNEL_HEART, 0, false, NULL);
				bPlayingLowHealthSound = true;
			}
		}
		else {
			if (bPlayingLowHealthSound) {
				StopSound(SND_CHANNEL_HEART, false);
				bPlayingLowHealthSound = false;
			}
		}
	}

	// Handle damage indictors
	idVec3 localDamageVector;
	idVec3 toEnemy;
	for (int ix=0; ix<MAX_TRACKED_ATTACKERS; ix++) {
		if (lastAttackers[ix].attacker.IsValid()) {

			if (!lastAttackers[ix].displayed) {
				_hud->HandleNamedEvent( va("Attacked%i", ix) );//Notify hud we were attacked
				_hud->SetStateBool("displayDamageIndicators", true);
				lastAttackers[ix].displayed = true;
			}

			toEnemy = lastAttackers[ix].attacker->GetOrigin() - GetOrigin();
			toEnemy.z = 0.0f;
			GetAxis().ProjectVector( toEnemy, localDamageVector );
			float yawToEnemy = idMath::AngleNormalize180(localDamageVector.ToYaw());
			_hud->SetStateFloat( va("yawToEnemy%i", ix), yawToEnemy);
			if (gameLocal.time > lastAttackers[ix].time + DAMAGE_INDICATOR_TIME) {
				lastAttackers[ix].attacker = NULL;

				bool allExpired = true;
				for (int jx=0; jx<MAX_TRACKED_ATTACKERS; jx++) {
					if (lastAttackers[jx].attacker.IsValid()) {
						allExpired = false;
						break;
					}
				}
				if (allExpired) {
					_hud->SetStateBool("displayDamageIndicators", false);
				}
			}
		}
	}

	if (gameLocal.isMultiplayer) {
		bool bLagged = isLagged && gameLocal.isMultiplayer && (gameLocal.localClientNum == entityNumber);
		_hud->SetStateBool("hudLag", bLagged);
	}
	// HUMANHEAD PCF pdm 06-26-06: Fix for multiplayer shuttle hud elements appearing in SP after a MP session
	else {
		_hud->SetStateBool( "ismultiplayer", false );
	}
	// HUMANHEAD END
}

//	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
void hhPlayer::UpdateHudWeapon(bool flashWeapon) {
	idUserInterface *_hud = hhPlayer::hud;

	// if updating the hud of a followed client
	if ( gameLocal.localClientNum >= 0 && gameLocal.entities[ gameLocal.localClientNum ] && gameLocal.entities[ gameLocal.localClientNum ]->IsType( idPlayer::Type ) ) {
		idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.localClientNum ] );
		if ( p->spectating && p->spectator == entityNumber ) {
			assert( p->hud );
			_hud = p->hud;
		}
	}

	if ( _hud ) {
		// HUMANHEAD pdm: changed to suit our needs
		_hud->SetStateInt( "currentweapon", GetCurrentWeapon() );
		_hud->SetStateInt( "idealweapon", GetIdealWeapon() );
		//HUMANHEAD PCF mdl 05/05/06 - Added !IsLocked( GetIdealWeapon() )
		if ( flashWeapon && !IsLocked( GetIdealWeapon() ) ) {
			_hud->HandleNamedEvent( "weaponChange" );
		}
	}
}

//	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
void hhPlayer::UpdateHudAmmo(idUserInterface *_hud) {
	assert( _hud );
	float ammoPct, altPct;
	int ammoType, altAmmoType;
	float ammo, altAmmo;
	bool ammoLow, altAmmoLow;

	// HUMANHEAD pdm: Weapon switch overlay, using rover to spread the expense over frames
	static int rover = 0;
	if (++rover > 9) {
		rover = 0;
	}

	if (rover) {
		bool bHeld = false;
		ammoPct = 0.0f;
		altPct = 0.0f;
		ammoType = weaponInfo[rover].ammoType;
		altAmmoType = altWeaponInfo[rover].ammoType;
		ammo = inventory.ammo[ammoType];
		altAmmo = inventory.ammo[altAmmoType];
		ammoLow = false;
		altAmmoLow = false;

		if ( inventory.weapons & ( 1 << rover ) ) {
			// have this weapon
			bHeld = true;
			ammoPct = ammo / weaponInfo[rover].ammoMax;
			altPct = altAmmo / altWeaponInfo[rover].ammoMax;
			ammoLow = ammo > 0 && ammo <= weaponInfo[rover].ammoLow;
			altAmmoLow = altAmmo > 0 && altAmmo <= altWeaponInfo[rover].ammoLow;
		}

		// Spirit ammo doesn't display in a bar
		if ( ammoType == 1 ) {
			ammoType = 0;
			altAmmoType = 0;
		}

		// Zero out alt-ammo bar if appropriate
		if (altAmmoType == 0 || altAmmoType == ammoType) {
			altPct = 0.0f;
		}

		_hud->SetStateBool (va("weapon%d_held", rover), bHeld);
		_hud->SetStateBool (va("weapon%d_ammolow", rover), ammoLow);
		_hud->SetStateBool (va("weapon%d_altammolow", rover), altAmmoLow);
		_hud->SetStateFloat(va("weapon%d_ammo", rover), ammoPct);
		_hud->SetStateFloat(va("weapon%d_altammo", rover), altPct);
		_hud->SetStateBool (va("weapon%d_ammoempty", rover), ammoType != 0 && ammo == 0 && altAmmo == 0);
	}

	// Update the current weapon's ammo status
	bool bDisallowAmmoBars = false;
	ammoPct = 0.0f;
	altPct = 0.0f;
	ammoLow = false;
	altAmmoLow = false;
	ammoType = 0;
	altAmmoType = 0;
	ammo = 0;
	altAmmo = 0;

	if( bLotaTunnelMode || privateCameraView || IsLocked(idealWeapon) || !weapon.IsValid() || !weapon->IsLinked() || currentWeapon == -1) {
		// Don't display ammo bar for invalid weapons, or when weapons are locked
		bDisallowAmmoBars = true;
	}
	else {
		if (currentWeapon >= 0 && currentWeapon < 15) {
			ammoType = weaponInfo[currentWeapon].ammoType;
			altAmmoType = altWeaponInfo[currentWeapon].ammoType;
			ammo = inventory.ammo[ammoType];
			altAmmo = inventory.ammo[altAmmoType];
			ammoPct = ammo / weaponInfo[currentWeapon].ammoMax;
			altPct = altAmmo / altWeaponInfo[currentWeapon].ammoMax;
			ammoLow = ammo > 0 && ammo <= weaponInfo[currentWeapon].ammoLow;
			altAmmoLow = altAmmo > 0 && altAmmo <= altWeaponInfo[currentWeapon].ammoLow;
		}

		if (ammoType == 1) {
			bDisallowAmmoBars = true;
		}
	}

	if ( bDisallowAmmoBars ) {
		_hud->SetStateBool( "player_ammobar", false);
		_hud->SetStateBool( "player_altammobar", false );
	}
	else {
		_hud->SetStateBool( "player_ammobar", ammoType != 0);
		_hud->SetStateBool( "player_altammobar", altAmmoType != 0 && altAmmoType != ammoType );

		_hud->SetStateFloat( "player_ammopercent", ammoPct );
		_hud->SetStateFloat( "player_altammopercent", altPct );
		_hud->SetStateString( "player_ammoamounttext", ammo<0 ? "" : va("%d", ammo) );
		_hud->SetStateString( "player_altammoamounttext", altAmmo<0 ? "" : va("%d", altAmmo) );
		_hud->SetStateBool( "player_ammolow", ammoLow );
		_hud->SetStateBool( "player_altammolow", altAmmoLow );
	}

}

/*
================
hhPlayer::UpdateHudStats
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
================
*/
void hhPlayer::UpdateHudStats( idUserInterface *_hud ) {

	assert( _hud );

	int spiritPower = GetSpiritPower();

	// Disallow health if wrench is locked on a non-lota map --or-- if in a private camera
	bool disallowHealth = bLotaTunnelMode || (IsLocked(1) && !gameLocal.IsLOTA()) || privateCameraView != NULL;
	bool disallowProgress = bLotaTunnelMode || privateCameraView != NULL || IsDeathWalking();

	if (disallowHealth) {
		int blah = 0;
	}
	else {
		int blah2 = 0;
	}

	_hud->SetStateBool("invehicle", InVehicle());
	_hud->SetStateBool("deathwalking", IsDeathWalking());
	_hud->SetStateBool("spiritwalking", IsSpiritWalking());
	_hud->SetStateBool("lighter", IsLighterOn() );
	_hud->SetStateBool("showhealth", !disallowHealth);
	_hud->SetStateBool("allowprogress", !disallowProgress);
	_hud->SetStateBool("showspiritpower", (!disallowHealth && !InVehicle() && (inventory.requirements.bCanSpiritWalk || IsDeathWalking())));
	_hud->SetStateFloat("lightertemp", lighterTemperature);
	_hud->SetStateFloat( "player_spiritpercent", ((float)spiritPower)/inventory.maxSpirit );
	_hud->SetStateFloat( "player_healthpercent", ((float)health)/inventory.maxHealth );
	_hud->SetStateFloat( "player_healthR", inventory.maxHealth > 100 ? 0.75f : 0.6f );
	_hud->SetStateFloat( "player_healthG", inventory.maxHealth > 100 ? 0.85f : 0.0f );
	_hud->SetStateFloat( "player_healthB", inventory.maxHealth > 100 ? 1.0f : 0.0f );
	_hud->SetStateFloat( "player_healthPulseR", inventory.maxHealth > 100 ? 1.0f : 1.0f );
	_hud->SetStateFloat( "player_healthPulseG", inventory.maxHealth > 100 ? 1.0f : 0.0f );
	_hud->SetStateFloat( "player_healthPulseB", inventory.maxHealth > 100 ? 1.0f : 0.0f );
	_hud->SetStateInt( "player_health", health );
	_hud->SetStateInt( "player_maxhealth", inventory.maxHealth ); //rww
	_hud->SetStateString( "player_spirit", spiritPower<0 ? "0" : va("%d", spiritPower) );
	_hud->SetStateFloat( "progress", progressBarGuiValue.GetCurrentValue(gameLocal.time) );

	if ( healthPulse ) {
		_hud->HandleNamedEvent( "healthPulse" );
		StartSound( "snd_healthpulse", SND_CHANNEL_ITEM, 0, false, NULL );
		healthPulse = false;
	}
	if ( spiritPulse ) {
		_hud->HandleNamedEvent( "spiritPulse" );
		StartSound( "snd_spiritpulse", SND_CHANNEL_ITEM, 0, false, NULL );
		spiritPulse = false;
	}

	if ( inventory.ammoPulse ) { 
		_hud->HandleNamedEvent( "ammoPulse" );
		inventory.ammoPulse = false;
	}
	if ( inventory.weaponPulse ) {
		// We need to update the weapon hud manually, but not
		// the armor/ammo/health because they are updated every
		// frame no matter what
		UpdateHudWeapon();
		_hud->HandleNamedEvent( "weaponPulse" );
		inventory.weaponPulse = false;
	}

	UpdateHudAmmo( _hud );

	// Determine slots for our pickup icons
	int c = inventory.pickupItemNames.Num();
	for ( int i=0; i<10; i++ ) {
		if (i<c) {
			_hud->SetStateString( va( "itemicon%i", i ), inventory.pickupItemNames[i].icon );
			_hud->SetStateFloat( va( "itemalpha%i", i ), inventory.pickupItemNames[i].matcolorAlpha );
			_hud->SetStateBool( va( "itemwide%i", i), inventory.pickupItemNames[i].bDoubleWide );
		}
		else {
			_hud->SetStateString( va( "itemicon%i", i ), "" );
			_hud->SetStateFloat( va( "itemalpha%i", i ), 0.0f );
			_hud->SetStateBool( va( "itemwide%i", i), false );
		}
	}
}

/*
===============
hhPlayer::DrawHUD
	HUMANHEAD: This run only for local players
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
===============
*/
void hhPlayer::DrawHUD( idUserInterface *_hud ) {

	if (guiOverlay) {
		guiOverlay->Redraw(gameLocal.realClientTime);
		sysEvent_t ev;
		const char *command;
		ev = sys->GenerateMouseMoveEvent( -2000, -2000 );
		command = guiOverlay->HandleEvent( &ev, gameLocal.time );
// 		HandleGuiCommands( this, command );
		return;
	}

	// HUMANHEAD pdm: removed weapon ptr check here since ours is invalid when in a vehicle
	// Also removed privateCameraView, since we want subtitles when they are on
	if ( gameLocal.GetCamera() || !_hud || !g_showHud.GetBool() || pm_thirdPerson.GetBool() ) {
		return;
	}

	UpdateHudStats( _hud );


	if(weapon.IsValid()) { // HUMANHEAD
		bool allowGuiUpdate = true;
		//rww - update the weapon gui only if the owner is being spectated by this client, or is this client
		if ( gameLocal.localClientNum != entityNumber ) {
			// if updating the hud for a followed client
			if ( gameLocal.localClientNum >= 0 && gameLocal.entities[ gameLocal.localClientNum ] && gameLocal.entities[ gameLocal.localClientNum ]->IsType( idPlayer::Type ) ) {
				idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.localClientNum ] );
				if ( !p->spectating || p->spectator != entityNumber ) {
					allowGuiUpdate = false;
				}
			} else {
				allowGuiUpdate = false;
			}
		}

		if (allowGuiUpdate) {
			weapon->UpdateGUI();
		}
	}

	_hud->SetStateInt( "s_debug", cvarSystem->GetCVarInteger( "s_showLevelMeter" ) );

	//HUMANHEAD aob: vehicle logic
	if ( InVehicle() ) {
		DrawHUDVehicle( _hud );
	}
	else {
		_hud->Redraw( gameLocal.realClientTime );
	}
	//HUMANHEAD END

	// weapon targeting crosshair
	if ( !GuiActive() ) {
		if ( cursor ) {
			UpdateCrosshairs();
			cursor->Redraw( gameLocal.realClientTime );
		}
	}
}

/*
===============
hhPlayer::DrawHUDVehicle
	HUMANHEAD: This run only for local players
===============
*/
void hhPlayer::DrawHUDVehicle( idUserInterface* _hud ) {
	if (GetVehicleInterfaceLocal()) {
		GetVehicleInterfaceLocal()->DrawHUD( _hud );
	}
}

/*
===============
hhPlayer::FireWeapon
===============
*/
void hhPlayer::FireWeapon( void ) {
	idMat3 axis;
	idVec3 muzzle;

	if ( privateCameraView ) {
		return;
	}

	if ( g_editEntityMode.GetInteger() ) {
		GetViewPos( muzzle, axis );
		if ( gameLocal.editEntities->SelectEntity( muzzle, axis[0], this ) ) {
			return;
		}
	}

	//HUMANHEAD: aob - removed ammo check because we allow weapons to change modes
	if( weapon.IsValid() ) {
		AI_ATTACK_HELD = true;
		weapon->BeginAttack();
	}
	//HUMANEHAD END
}

//=========================================================================
//
// hhPlayer::FireWeaponAlt
//
//=========================================================================

void hhPlayer::FireWeaponAlt( void ) {
	idMat3 axis;
	idVec3 muzzle;

	if ( privateCameraView ) {
		return;
	}

	if ( g_editEntityMode.GetBool() ) {
		GetViewPos( muzzle, axis );
		if ( gameLocal.editEntities->SelectEntity( muzzle, axis[0], NULL ) ) {
			return;
		}
	}

	if ( gameLocal.isMultiplayer && spectating ) {
		static int lastSpectatorSwitch = 0;
		if ( gameLocal.time > lastSpectatorSwitch + 500 ) {
			spectator = gameLocal.GetNextClientNum( spectator );
			idPlayer *player = gameLocal.GetClientByNum( spectator );
			while ( player->spectating && player != this ) {
				player = gameLocal.GetClientByNum(spectator);
			}
			lastSpectatorSwitch = gameLocal.time;
		}
		return;
	}

	//HUMANHEAD: aob - removed ammo check because we allow weapons to change modes
	if( weapon.IsValid() ) {
		AI_ATTACK_HELD = true;
		weapon->BeginAltAttack();
	}
	//HUMANEHAD END

	if ( hud ) {
		hud->HandleNamedEvent( "closeObjective" );
	}
}

void hhPlayer::StopFiring( void ) {
	idPlayer::StopFiring();
	if ( weapon.GetEntity() && weapon->IsType( hhWeaponRifle::Type ) ) {
		static_cast<hhWeaponRifle*>( weapon.GetEntity() )->ZoomOut();
	}
}

/*
===============
hhPlayer::HasAmmo
===============
*/
int hhPlayer::HasAmmo( ammo_t type, int amount ) {
	return inventory.HasAmmo( type, amount );
}

/*
===============
hhPlayer::UseAmmo
===============
*/
bool hhPlayer::UseAmmo( ammo_t type, int amount ) {
	return inventory.UseAmmo( type, amount );
}

/*
===============
hhPlayer::NextBestWeapon
===============
*/
void hhPlayer::NextBestWeapon( void ) {
	if ( ActiveGui() || IsSpiritOrDeathwalking() ) {
		return;
	}
	idPlayer::NextBestWeapon();
}

/*
===============
hhPlayer::SelectWeapon
===============
*/
void hhPlayer::SelectWeapon( int num, bool force ) {	
	if ( ! ( weaponFlags & ( 1 << ( num - 1 ) ) ) ) {
		return;
	}
	if ( bFrozen || ActiveGui() || ( SkipWeapon(num) || IsSpiritOrDeathwalking() ) ) {
		return;
	}
	idPlayer::SelectWeapon( num, force );
}

/*
===============
hhPlayer::NextWeapon
===============
*/
void hhPlayer::NextWeapon( void ) {
	if ( ActiveGui() || IsSpiritOrDeathwalking() || bFrozen ) {
		// NOTE: Spirit weapon is disallowed from being cycled to using the weapon*_cycle key
		return;
	}
	//idPlayer::NextWeapon();
	if ( !weaponEnabled || spectating || hiddenWeapon || gameLocal.inCinematic || privateCameraView || gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || health < 0 ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	// check if we have any weapons
	if ( !inventory.weapons ) {
		return;
	}

	const char *weap;
	int w, start;

	w = idealWeapon;
	start = w;
	while( 1 ) {
		w++;
		if ( w >= MAX_WEAPONS ) {
			// No weapon selected and nothing to select
			if ( start == -1 ) {
				idealWeapon = 0;
				currentWeapon = 0;
				return;
			}
			w = 0;
		}
		// Keep us from an infinite loop if no weapons are valid
		if ( w == start ) {
			if ( ! ( weaponFlags & ( 1 << ( w - 1 ) ) ) ) {
				idealWeapon = 0;
				currentWeapon = 0;
			}
			return;
		}
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if ( !spawnArgs.GetBool( va( "weapon%d_cycle", w ) ) ) {
			continue;
		}
		if ( !weap[ 0 ] ) {
			continue;
		}
		if ( ( inventory.weapons & ( 1 << w ) ) == 0 ) {
			continue;
		}
		// Make sure the weapon is valid for this level
		if ( ! ( weaponFlags & ( 1 << ( w - 1 ) ) ) ) {
			continue;
		}
		if ( inventory.HasAmmo( weap ) || inventory.HasAltAmmo( weap ) || spawnArgs.GetBool( va( "weapon%d_allowempty", w ) ) ) {
			break;
		}
	}

	if ( ( w != currentWeapon ) && ( w != idealWeapon ) ) {
		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		UpdateHudWeapon();
	}
}

/*
===============
hhPlayer::PrevWeapon
===============
*/
void hhPlayer::PrevWeapon( void ) {
	if ( ActiveGui() || IsSpiritOrDeathwalking() || bFrozen ) {
		// NOTE: Spirit weapon is disallowed from being cycled to using the weapon*_cycle key
		return;
	}
	//idPlayer::PrevWeapon();
	if ( !weaponEnabled || spectating || hiddenWeapon || gameLocal.inCinematic || privateCameraView || gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || health < 0 ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	// check if we have any weapons
	if ( !inventory.weapons ) {
		return;
	}

	const char *weap;
	int w = idealWeapon, start = w;
	if (w == -1) {
		w = MAX_WEAPONS - 1;
	}
	while( 1 ) {
		w--;
		if ( w < 0 ) {
			// No weapon selected and nothing to select
			if ( start == -1 ) {
				idealWeapon = 0;
				currentWeapon = 0;
				return;
			}

			w = MAX_WEAPONS - 1;
		}
		// Keep us from an infinite loop if no weapons are valid
		if ( w == start ) {
			if ( ! ( weaponFlags & ( 1 << ( w - 1 ) ) ) ) {
				idealWeapon = 0;
				currentWeapon = 0;
			}
			return;
		}
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if ( !spawnArgs.GetBool( va( "weapon%d_cycle", w ) ) ) {
			continue;
		}
		if ( !weap[ 0 ] ) {
			continue;
		}
		if ( ( inventory.weapons & ( 1 << w ) ) == 0 ) {
			continue;
		}
		// Make sure the weapon is valid for this level
		if ( ! ( weaponFlags & ( 1 << ( w - 1 ) ) ) ) {
			continue;
		}
		if ( inventory.HasAmmo( weap ) || inventory.HasAltAmmo( weap ) || spawnArgs.GetBool( va( "weapon%d_allowempty", w ) ) ) {
			break;
		}
	}

	if ( ( w != currentWeapon ) && ( w != idealWeapon ) ) {
		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		UpdateHudWeapon();
	}
}

/*
===============
hhPlayer::SkipWeapon
===============
*/
bool hhPlayer::SkipWeapon( int weaponNum ) const {
	//No bow if not in spirit mode
	return !IsSpiritOrDeathwalking() && weaponNum == spawnArgs.GetInt("spirit_weapon");
}

/*
===============
hhPlayer::SnapDownCurrentWeapon

HUMANHEAD: aob
===============
*/
void hhPlayer::SnapDownCurrentWeapon() {
	if( weapon.IsValid() ) {
		weapon->SnapDown();
	}
}

/*
===============
hhPlayer::SnapUpCurrentWeapon

HUMANHEAD: aob
===============
*/
void hhPlayer::SnapUpCurrentWeapon() {
	if( weapon.IsValid() ) {
		weapon->SnapUp();
	}
}

/*
===============
hhPlayer::SelectEtherealWeapon

HUMANHEAD: aob
===============
*/
void hhPlayer::SelectEtherealWeapon() {
	if ( GetCurrentWeapon() != 0 ) {
		lastWeaponSpirit = GetCurrentWeapon();
	}

	weaponHandState.SetPlayer( this );
	if (!gameLocal.isClient) {
		int spiritWeaponIndex = spawnArgs.GetInt("spirit_weapon");
		const char *spiritWeaponName = NULL;
		if (bDeathWalk || (inventory.weapons & (1 << spiritWeaponIndex)) ) {
			spiritWeaponName = GetWeaponName(spiritWeaponIndex);
		}
		weaponHandState.Archive( spiritWeaponName, 0, (InGUIMode()) ? "guihand_normal" : NULL );
	}

	if ( weapon.IsValid() ) {
		weapon->SetShaderParm( SHADERPARM_MODE, MS2SEC( gameLocal.time ) ); // Glow in the bow
		weapon->SetShaderParm( SHADERPARM_MISC, 1.0f ); // Turn on the arrow
		weapon->SetShaderParm( SHADERPARM_DIVERSITY, MS2SEC( gameLocal.time ) ); // Glow in the arrow
	}
}

/*
===============
hhPlayer::PutawayEtherealWeapon

HUMANHEAD: aob
===============
*/
void hhPlayer::PutawayEtherealWeapon() {
	//If we haven't actually changed weapons yet, put current weapon up
	if( GetCurrentWeapon() != lastWeaponSpirit ) {
		SnapDownCurrentWeapon();
	} else {
		SnapUpCurrentWeapon();
	}

	weaponHandState.RestoreFromArchive();
}

/*
===============
hhPlayer::UpdateWeapon
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
===============
*/
void hhPlayer::UpdateWeapon( void ) {
	
	if ( IsDead() ) { // HUMANHEAD cjr:  Replaced health <= 0 with IsDead() call for deathwalk override
		return;
	}

	assert( !spectating );

	if ( gameLocal.isClient ) {
		// clients need to wait till the weapon and it's world model entity
		// are present and synchronized ( weapon.worldModel idEntityPtr to idAnimatedEntity )
		if ( !weapon.GetEntity()->IsWorldModelReady() ) {
			return;
		}
	}
	else if (gameLocal.isMultiplayer) { //rww - projectile deferring
		if (weapon.IsValid()) {
			weapon->CheckDeferredProjectiles();
		}
	}

	// always make sure the weapon is correctly setup before accessing it
	if ( weapon.GetEntity() && !weapon.GetEntity()->IsLinked() ) {
		if ( idealWeapon != -1 ) {
			animPrefix = spawnArgs.GetString( va( "def_weapon%d", idealWeapon ) );
			weapon.GetEntity()->GetWeaponDef( animPrefix, inventory.clip[ idealWeapon ] );
			animPrefix.Strip( "weaponobj_" ); //HUMANHEAD rww
			assert( weapon.GetEntity()->IsLinked() );
		} else {
			return;
		}
	}

	//HUMANEHAD rww
	if (weapon.IsValid() && weapon->IsType(hhWeaponSoulStripper::Type)) {
		hhWeaponSoulStripper *leechGun = static_cast<hhWeaponSoulStripper *>(weapon.GetEntity());
		leechGun->CheckCans();
	}
	//HUMANHEAD END

	if (!InVehicle()) {
		if ( g_dragEntity.GetBool() ) {
			StopFiring();
			dragEntity.Update( this );
			if ( weapon.IsValid() ) {
				weapon.GetEntity()->FreeModelDef();
			}
			return;
		} else if (ActiveGui()) {
			// gui handling overrides weapon use
			Weapon_GUI();
		} else {
			Weapon_Combat();
		}

		// Determine whether we are in aside state
		if ( weapon.IsValid() && weapon->IsAside() ) {
			if ( !AI_ASIDE ) {
				AI_ASIDE = true;
				SetState( "AsideWeapon" );
				UpdateScript();
			}
		} else {
			AI_ASIDE = false;
		}
	}

	//HUMANHEAD: aob - added weapon validity check
	if( weapon.IsValid() ) {
		// update weapon state, particles, dlights, etc
		weapon->PresentWeapon( showWeaponViewModel );
	}

	// nla
	if ( hand.IsValid() ) {
		hand->Present();
	}
}

/*
===============
hhPlayer::InGUIMode
nla - Used for the GUI hand to check if it should be up.  (Coming back from spiritwalk)
===============
*/
bool hhPlayer::InGUIMode() {

	// Logic adapted from UpdateWeapon
	if ( ActiveGui() && !InVehicle() ) {
		return( true );
	}

	return( false );
}


/*
===============
hhPlayer::Weapon_Combat
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
===============
*/
void hhPlayer::Weapon_Combat( void ) {
	idMat3		axis;
	idVec3		muzzle;
	idDict		args;
	
	if ( !weaponEnabled || gameLocal.inCinematic || privateCameraView ) {
		return;
	}

	// HUMANHEAD nla
	if ( hand.IsValid() && hand->IsType( hhGuiHand::Type ) && hand->IsAttached() && !hand->IsLowering() ) { 
		hand->RemoveHand();
	}	
	// HUMANHEAD END

	if( idealWeapon != 0 && IsLocked(idealWeapon) ) {		//HUMANHEAD bjk PCF (4-30-06) - fix wrench up in roadhouse
		NextWeapon();
	}

	if ( idealWeapon == 0 ) {
		return;
	}

	RaiseWeapon();
	if ( weapon.IsValid() ) {
		weapon.GetEntity()->PutUpright();
	}

	if ( weapon.IsValid() && weapon->IsReloading() ) {
		if ( !AI_RELOAD ) {
			AI_RELOAD = true;
			SetState( "ReloadWeapon" );
			UpdateScript();
		}
	} else {
		AI_RELOAD = false;
	}

	if ( idealWeapon != currentWeapon && (!gameLocal.isMultiplayer || inventory.lastShot[idealWeapon] < gameLocal.time) ) {	//HUMANHEAD bjk PATCH 7-27-06
		if ( weaponCatchup ) {
			assert( gameLocal.isClient );
#ifndef HUMANHEAD //HUMANHEAD rww FIXME!!!! this also produces horrible memory leaks because of the dirty fire controller stuff
			//HUMANHEAD rww - our crazy weapon system does not work ok with this. did we change the weapon dictionary parsing?
			//it seems that it is very much dependant on the base class of weapon at the moment and that of course is going to be
			//that of currentWeapon and not idealWeapon.
			//currentWeapon = idealWeapon;
			//HUMANHEAD END
			weaponGone = false;
			animPrefix = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
			weapon.GetEntity()->GetWeaponDef( animPrefix, inventory.clip[ currentWeapon ] );
			animPrefix.Strip( "weaponobj_" );	//HUMANHEAD pdm: changed from weapon_ to weaponobj_ per our naming convention

			weapon.GetEntity()->NetCatchup();
			const function_t *newstate = GetScriptFunction( "NetCatchup" );
			if ( newstate ) {
				SetState( newstate );
				UpdateScript();
			}
#else
			assert( idealWeapon >= 0 );
			assert( idealWeapon < MAX_WEAPONS );

			animPrefix = spawnArgs.GetString( va( "def_weapon%d", idealWeapon ) );
			const char *currentWeaponName = weapon->spawnArgs.GetString("classname", "");
			//make sure the weapon i have from my snapshot, and the weapon i have selected are the same.
			if (currentWeaponName && !idStr::Cmp(animPrefix, currentWeaponName)) {
				weaponGone = false;
				currentWeapon = idealWeapon;

				weapon->GetWeaponDef( animPrefix, inventory.clip[ currentWeapon ] );
				animPrefix.Strip( "weaponobj_" );
				weapon->Raise();
#endif //HUMANHEAD END
				weaponCatchup = false;			
			}
		} else {
			if ( weapon.IsValid() && (weapon->IsReady() || weapon->IsRising()) ) {
				InvalidateCurrentWeapon();//Needed incase we change weapons quickly, we can go back to old weapon
				weapon->PutAway();
			}

			if ( ( !weapon.IsValid() || weapon->IsHolstered() ) && !bDeathWalk && ! bReallyDead ) {
				assert( idealWeapon >= 0 );
				assert( idealWeapon < MAX_WEAPONS );

				if ( currentWeapon > 0 && !spawnArgs.GetBool( va( "weapon%d_toggle", currentWeapon ) ) ) {	//HUMANHEAD bjk
					previousWeapon = currentWeapon;
				}
				//HUMANHEAD PCF rww 05/03/06 - the local client might get skippiness between switching weapons if we
				//attempt to raise the weapon again before validating that the weapon ent is of the type that we
				//already desire to switch to. this bug is introduced by the concept of switching out entities when
				//changing weapons (not client-friendly).
				if (gameLocal.isClient && entityNumber == gameLocal.localClientNum) {
					if (weapon.IsValid() && weapon->GetDict() && idStr::Icmp(weapon->GetDict()->GetString("classname"), GetWeaponName( idealWeapon )) == 0) {
						currentWeapon = idealWeapon;
						weaponGone = false;
						animPrefix = GetWeaponName( currentWeapon );
						animPrefix.Strip( "weaponobj_" );	//HUMANHEAD pdm: changed from weapon_ to weaponobj_ per our naming convention
						weapon.GetEntity()->Raise();
					}
				}
				//HUMANHEAD END
				else {
					currentWeapon = idealWeapon;
					weaponGone = false;
				
					//HUMANHEAD: aob
					animPrefix = GetWeaponName( currentWeapon );
					if (!gameLocal.isClient) {
						SAFE_REMOVE( weapon );
						weapon = SpawnWeapon( animPrefix.c_str() );
					}
					//HUMANHEAD END
						
					animPrefix.Strip( "weaponobj_" );	//HUMANHEAD pdm: changed from weapon_ to weaponobj_ per our naming convention

					//HUMANHEAD PCF rww 05/03/06 - safety check, make sure weapon is valid particularly for the client
					if (weapon.IsValid()) {
						weapon.GetEntity()->Raise();
					}
				}
			}
		}
	} else if (!bDeathWalk && !bReallyDead) {
		weaponGone = false;	// if you drop and re-get weap, you may miss the = false above 
		if ( weapon.IsValid() && weapon.GetEntity()->IsHolstered() ) {
			if ( !weapon.GetEntity()->AmmoAvailable() ) {
				// weapons can switch automatically if they have no more ammo
				NextBestWeapon();
			} else if( !gameLocal.isMultiplayer || inventory.lastShot[idealWeapon] < gameLocal.time ) {		//HUMANHEAD bjk PATCH 9-11-06
				weapon.GetEntity()->Raise();
				state = GetScriptFunction( "RaiseWeapon" );
				if ( state ) {
					SetState( state );
				}
			}
		}
	}

	if ( weapon.IsValid() ) {
		weapon->PrecomputeTraceInfo();
	}

	// check for attack
	AI_WEAPON_FIRED = false;
	if ( ( usercmd.buttons & BUTTON_ATTACK ) && !weaponGone ) {
		FireWeapon();
	} else if ( oldButtons & BUTTON_ATTACK ) {
		AI_ATTACK_HELD = false;
		if( weapon.IsValid() ) {
			weapon.GetEntity()->EndAttack();
		}
	}

	// HUMANHEAD
	if ( ( usercmd.buttons & BUTTON_ATTACK_ALT ) && !weaponGone ) {
		FireWeaponAlt();
	} else if ( oldButtons & BUTTON_ATTACK_ALT ) {
		AI_ATTACK_HELD = false;
		if( weapon.IsValid() ) {
			weapon.GetEntity()->EndAltAttack();
		}
	}
	// HUMANHEAD END

	// update our ammo clip in our inventory
	if ( weapon.IsValid() && ( currentWeapon >= 0 ) && ( currentWeapon < MAX_WEAPONS ) ) {
		inventory.clip[ currentWeapon ] = weapon->AmmoInClip();
		inventory.altMode[ currentWeapon ] = weapon->GetAltMode();
	}
}

/*
===============
hhPlayer::SpawnWeapon
===============
*/
hhWeapon* hhPlayer::SpawnWeapon( const char* name ) {
	hhWeapon*	weaponPtr = NULL;
	idDict		args;

	args.SetVector( "origin", GetEyePosition() );
	args.SetMatrix( "rotation", viewAngles.ToMat3() );
	weaponPtr = static_cast<hhWeapon*>( gameLocal.SpawnObject(name, &args) );
	weaponPtr->SetOwner( this );
	weaponPtr->Bind( this, true );
							
	//HUMANHEAD: aob - removed last two params
	weaponPtr->GetWeaponDef( name );

	return weaponPtr;
}

/*
=====================
hhPlayer::GetWeaponNum
	HUMANHEAD
=====================
*/
int hhPlayer::GetWeaponNum( const char* weaponName ) const {
	for( int i = 1; i < MAX_WEAPONS; ++i ) {
		if( !idStr::Icmp(GetWeaponName(i), weaponName) ) {
			return i;
		}
	}

	return 0;
}

/*
=====================
hhPlayer::GetWeaponName
	HUMANHEAD
=====================
*/
const char* hhPlayer::GetWeaponName( int num ) const {
	if( num < 1 || num >= MAX_WEAPONS ) {
		return NULL;
	}

	return spawnArgs.GetString( va("def_weapon%d", num) );
}

/*
===============
hhPlayer::Weapon_GUI
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
===============
*/
void hhPlayer::Weapon_GUI( void ) {

	StopFiring();
	weapon.GetEntity()->LowerWeapon();

	// NLANOTE - Same here
	// disable click prediction for the GUIs. handy to check the state sync does the right thing
	if ( gameLocal.isClient && !net_clientPredictGUI.GetBool() ) {
		return;
	}

	if ( health <= 0 || bDeathWalk ) {		//HUMANHEAD bjk PCF (5-5-06) - no hand stuff when dead
		return;
	}

	// HUMANHEAD nla 
	if ( !bDeathWalk ) { // mdl - Don't do hand stuff while deathwalking
		if ( hand.IsValid() && hand->IsType( hhGuiHand::Type ) ) {
			//if ( !hand->IsReady() && !hand->IsRaising() ) {
			if ( !hand->IsReady() && !hand->IsRaising() && !hand->IsLowering() ) {
				hand->Reraise();
			}
		}
		else if ( (!hand.IsValid() && weapon.IsValid() && !weapon->IsAside()  ) || 
				  (hand.IsValid() && !hand->IsType( hhGuiHand::Type ) && !hand->IsLowering() ) ||
				  (!hand.IsValid() && idealWeapon == 0 ) ) {
			if (!gameLocal.isClient) { //rww
				hhHand::AddHand( this, GetGuiHandInfo() );
			}
		}
	}
	// HUMANHEAD END
		
	if ( hand.IsValid() && ( oldButtons ^ usercmd.buttons ) & BUTTON_ATTACK ) {	//HUMANHEAD bjk: no waiting for ready
		sysEvent_t ev;
		const char *command = NULL;
		bool updateVisuals = false;
		
		idUserInterface *ui = ActiveGui();
		if ( ui ) {
			ev = sys->GenerateMouseButtonEvent( 1, ( usercmd.buttons & BUTTON_ATTACK ) != 0 );
			command = ui->HandleEvent( &ev, gameLocal.time, &updateVisuals );
			if ( updateVisuals && focusGUIent && ui == focusUI ) {
				focusGUIent->UpdateVisuals();
			}
		}
		if ( gameLocal.isClient ) {
			// we predict enough, but don't want to execute commands
			if (focusGUIent) {
				if ( hand.IsValid() && hand->IsType( hhGuiHand::Type ) && ev.evValue2) { //rww - still predict hand
					hand->SetAction( focusGUIent->spawnArgs.GetString( "pressAnim", "press" ) );	//HUMANHEAD bjk
					hand->Action();
				}
			}
			return;
		}
		if ( focusGUIent ) {
			HandleGuiCommands( focusGUIent, command );
			// HUMANHEAD nla - Added to handle the hand stuff.  = )
			if ( hand.IsValid() && hand->IsType( hhGuiHand::Type ) && ev.evValue2) {
				hand->SetAction( focusGUIent->spawnArgs.GetString( "pressAnim", "press" ) );	//HUMANHEAD bjk
				hand->Action();
			}
			// HUMANHEAD END
		} else {
			HandleGuiCommands( this, command );
		}
	}
}

/*
================
hhPlayer::UpdateCrosshairs
HUMANHEAD PDM
================
*/
void hhPlayer::UpdateCrosshairs() {
	bool combatCrosshair = false;
	bool targeting = false;
	int crosshair = 0;

	if ( !privateCameraView && !IsLocked(idealWeapon) && (!hand.IsValid() || hand->IsLowered()) && !InCinematic() && weapon.IsValid() && g_crosshair.GetInteger() ) {
		weapon->UpdateCrosshairs(combatCrosshair, targeting);
		crosshair = g_crosshair.GetInteger();
	}

	cursor->SetStateBool( "combatcursor", targeting ? false : combatCrosshair );
	cursor->SetStateBool( "activecombatcursor", targeting );
	cursor->SetStateInt( "crosshair", crosshair );
}


/*
================
hhPlayer::UpdateFocus

Searches nearby entities for interactive guis, possibly making one of them
the focus and sending it a mouse move event
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
================
*/
void hhPlayer::UpdateFocus( void ) {
	idClipModel *clipModelList[ MAX_GENTITIES ];
	idClipModel *clip;
	int			listedClipModels;
	idEntity	*oldFocus;
	idEntity	*ent;
	idUserInterface *oldUI;
	int			i, j;
	idVec3		start, end;
	const char *command;
	trace_t		trace;
	guiPoint_t	pt;
	const idKeyValue *kv;
	sysEvent_t	ev;
	idUserInterface *ui;

	if ( gameLocal.inCinematic ) {
		return;
	}

	oldFocus		= focusGUIent;
	oldUI			= focusUI;

	if ( focusTime <= gameLocal.time ) {
		ClearFocus();
	}

	// don't let spectators interact with GUIs
	if ( spectating ) {
		return;
	}

	start = GetEyePosition();

	//HUMANHEAD rww - the actual viewAngles do not seem to be properly transformed to at this point,
	//so just get a transformed direction now.
	idVec3 viewDir = (untransformedViewAngles.ToMat3()*GetEyeAxis())[0];

	end = start + viewDir * 70.0f;	// was 50, doom=80

	// HUMANHEAD pdm: Changed aim logic a bit
	// player identification -> names to the hud
	if ( gameLocal.isMultiplayer && entityNumber == gameLocal.localClientNum ) {
		idVec3 end = start + viewDir * 768.0f;
		gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
		if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum < MAX_CLIENTS ) ) {
			MPAim = trace.c.entityNum;
		}
		else {
			MPAim = -1;

			//HUMANHEAD rww - if we're hitting a shuttle with a pilot, set the pilot to the MPAim ent
			if (trace.c.entityNum >= MAX_CLIENTS && trace.c.entityNum < MAX_GENTITIES) {
				idEntity *traceEnt = gameLocal.entities[trace.c.entityNum];
				if (traceEnt && traceEnt->IsType(hhVehicle::Type)) { //is it a vehicle?
					hhVehicle *traceVeh = static_cast<hhVehicle *>(traceEnt);
					if (traceVeh->GetPilot() && traceVeh->GetPilot()->IsType(hhPlayer::Type)) { //if it's a vehicle, does it have a player pilot?
						MPAim = traceVeh->GetPilot()->entityNumber;
					}
				}
			}
			//HUMANHEAD END
		}
	}

	idBounds bounds( start );
	bounds.AddPoint( end );

	listedClipModels = gameLocal.clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

	// no pretense at sorting here, just assume that there will only be one active
	// gui within range along the trace
	for ( i = 0; i < listedClipModels; i++ ) {
		clip = clipModelList[ i ];
		ent = clip->GetEntity();

		if ( ent->IsHidden() ) {
			continue;
		}

		// HUMANHEAD pdm: added support here for all guis, not just gui1
		int interactiveMask = 0;
		renderEntity_t *renderEnt = ent->GetRenderEntity();
		if ( renderEnt ) {
			for (int ix=0; ix<MAX_RENDERENTITY_GUI; ix++) {
				if (renderEnt->gui[ix] && renderEnt->gui[ix]->IsInteractive()) {
					interactiveMask |= (1<<ix);
				}
			}
		}
		if (!interactiveMask) {
			continue;
		}

		pt = gameRenderWorld->GuiTrace( ent->GetModelDefHandle(), start, end, interactiveMask );

		if ( ent->fl.accurateGuiTrace ) {
			trace_t tr;
			gameLocal.clip.TracePoint(tr, start, end, CONTENTS_SOLID, this);
			if (tr.fraction < pt.frac) {
				continue;
			}
		}

		if ( pt.x != -1 ) {
			// we have a hit
			renderEntity_t *focusGUIrenderEntity = ent->GetRenderEntity();
			if ( !focusGUIrenderEntity ) {
				continue;
			}

			if ( pt.guiId == 1 ) {
				ui = focusGUIrenderEntity->gui[ 0 ];
			} else if ( pt.guiId == 2 ) {
				ui = focusGUIrenderEntity->gui[ 1 ];
			} else {
				ui = focusGUIrenderEntity->gui[ 2 ];
			}
			
			if ( ui == NULL ) {
				continue;
			}

			ClearFocus();
			focusGUIent = ent;
			focusUI = ui;

			if ( oldFocus != ent ) {
				// new activation
				// going to see if we have anything in inventory a gui might be interested in
				// need to enumerate inventory items
				focusUI->SetStateInt( "inv_count", inventory.items.Num() );
				for ( j = 0; j < inventory.items.Num(); j++ ) {
					idDict *item = inventory.items[ j ];
					const char *iname = item->GetString( "inv_name" );
					const char *iicon = item->GetString( "inv_icon" );
					const char *itext = item->GetString( "inv_text" );

					focusUI->SetStateString( va( "inv_name_%i", j), iname );
					focusUI->SetStateString( va( "inv_icon_%i", j), iicon );
					focusUI->SetStateString( va( "inv_text_%i", j), itext );
					kv = item->MatchPrefix("inv_id", NULL);
					if ( kv ) {
						focusUI->SetStateString( va( "inv_id_%i", j ), kv->GetValue() );
					}
					// HUMANHEAD nla - Changed to pass all "passtogui_" keys to the gui
					kv = item->MatchPrefix("passtogui_", NULL);
					if ( kv ) {
						focusUI->SetStateString( kv->GetKey(), kv->GetValue() );
						kv = item->MatchPrefix( "passtogui_", kv );
					}
					// HUMANHEAD END
					focusUI->SetStateInt( iname, 1 );
				}

				focusUI->SetStateString( "player_health", va("%i", health ) );
				focusUI->SetStateBool( "player_spiritwalking", IsSpiritWalking() );	// for hunterhand gui

				kv = focusGUIent->spawnArgs.MatchPrefix( "gui_parm", NULL );
				while ( kv ) {
					focusUI->SetStateString( kv->GetKey(), kv->GetValue() );
					kv = focusGUIent->spawnArgs.MatchPrefix( "gui_parm", kv );
				}
			}

			// clamp the mouse to the corner
			ev = sys->GenerateMouseMoveEvent( -2000, -2000 );
			command = focusUI->HandleEvent( &ev, gameLocal.time );
 			HandleGuiCommands( focusGUIent, command );

			// move to an absolute position
			ev = sys->GenerateMouseMoveEvent( pt.x * SCREEN_WIDTH, pt.y * SCREEN_HEIGHT );
			command = focusUI->HandleEvent( &ev, gameLocal.time );
			HandleGuiCommands( focusGUIent, command );
			focusTime = gameLocal.time + FOCUS_GUI_TIME;
			break;
		}
	}

	if ( focusGUIent && focusUI ) {
		if ( !oldFocus || oldFocus != focusGUIent ) {
			command = focusUI->Activate( true, gameLocal.time );
			HandleGuiCommands( focusGUIent, command );
			//StartSound( "snd_guienter", SND_CHANNEL_ANY, 0, false, NULL );
		}
	} else if ( oldFocus && oldUI ) {
		command = oldUI->Activate( false, gameLocal.time );
		HandleGuiCommands( oldFocus, command );
		//StartSound( "snd_guiexit", SND_CHANNEL_ANY, 0, false, NULL );
	}
}


/*
===============
hhPlayer::ApplyLandDeflect

HUMANHEAD: aob
===============
*/
idVec3 hhPlayer::ApplyLandDeflect( const idVec3& pos, float scale ) {
	idVec3	localPos( pos );
	int		delta = 0;
	float	fraction = 0.0f;

	// add fall height
	delta = gameLocal.time - landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		fraction = (float)delta / LAND_DEFLECT_TIME;
		//HUMANHEAD: aob
		fraction *= scale;
		//HUMANHEAD END
		localPos += cameraInterpolator.GetCurrentUpVector() * landChange * fraction;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		fraction = (float)(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
		//HUMANHEAD: aob
		fraction *= scale;
		//HUMANHEAD END
		localPos += cameraInterpolator.GetCurrentUpVector() * landChange * fraction;
	}

	return localPos;
}

/*
=================
hhPlayer::CrashLand

Check for hard landings that generate sound events
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
=================
*/
void hhPlayer::CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity ) {
	//HUMANHEAD: aob - removed unused vars
	idVec3		origin;
	idVec3		gravityNormal;
	float		delta;
	waterLevel_t waterLevel;
	bool		noDamage;
	//HUMANHEAD END

	//HUMANHEAD: aob
	const trace_t&	trace = physicsObj.GetGroundTrace();
	//HUMANHEAD END

	AI_SOFTLANDING = false;
	AI_HARDLANDING = false;

	//HUMANHEAD: aob - added IsBound and trace check
	// if the player is not on the ground
	if ( (!physicsObj.HasGroundContacts() || trace.fraction == 1.0f) && !IsBound() ) {
		return;
	}

	//HUMANHEAD: aob - only check when we land on the ground
	//If we get here we can assume we currently have ground contacts
	if( physicsObj.HadGroundContacts() ) {
		return;
	}
	//HUMANHEAD END

	gravityNormal = physicsObj.GetGravityNormal();

	// if the player wasn't going down
	if ( ( oldVelocity * -gravityNormal ) >= 0.0f ) {
		return;
	}

	waterLevel = physicsObj.GetWaterLevel();

	// never take falling damage if completely underwater
	if ( waterLevel == WATERLEVEL_HEAD ) {
		return;
	}

	// no falling damage if touching a nodamage surface
	noDamage = false;
	for ( int i = 0; i < physicsObj.GetNumContacts(); i++ ) {
		const contactInfo_t &contact = physicsObj.GetContact( i );
		if ( contact.material->GetSurfaceFlags() & SURF_NODAMAGE ) {
			noDamage = true;
			break;
		}
	}

	//HUMANHEAD: aob - removed velocity calculations

	//HUMANHEAD: aob
	idVec3 deltaVelocity = DetermineDeltaCollisionVelocity( oldVelocity, trace );
	delta = (IsBound()) ? deltaVelocity.Length() : deltaVelocity * gravityNormal;
	//HUMANHEAD END

	// reduce falling damage if there is standing water
	if ( waterLevel == WATERLEVEL_WAIST ) {
		delta *= 0.25f;
	}
	if ( waterLevel == WATERLEVEL_FEET ) {
		delta *= 0.5f;
	}

	if ( delta < crashlandSpeed_jump || IsSpiritOrDeathwalking() ) {
		return;	// Early out
	}

	// Calculate landing sound volume
	float soundScale = hhUtils::CalculateScale( delta, crashlandSpeed_jump, crashlandSpeed_fatal );
	landChange = -32 * soundScale;
	landTime = gameLocal.time;
	float min = hhMath::dB2Scale( spawnArgs.GetInt("minCrashLandVolumedB", "-20") );
	float max = hhMath::dB2Scale( spawnArgs.GetInt("maxCrashLandVolumedB", "-4") );
	soundScale = min + (max - min) * soundScale;

	idVec3		fallDir = oldVelocity;
	idVec3		reverseContactNormal = -physicsObj.GetGroundContactNormal();
	idEntity*	entity = NULL;
	float		damageScale = hhUtils::CalculateScale( delta, crashlandSpeed_soft, crashlandSpeed_fatal );

	fallDir.Normalize();

	if( trace.fraction == 1.0f ) {
		return;
	}
		
	if( !IsBound() ) {
		PlayCrashLandSound( trace, soundScale );
		gameLocal.AlertAI( this, spawnArgs.GetFloat( "land_alert_radius", "400" ) );
	}

	// Determine damage to what you're landing on
	entity = gameLocal.GetTraceEntity( trace );
	if( entity && trace.c.entityNum != ENTITYNUM_WORLD ) {
		entity->ApplyImpulse( this, 0, trace.c.point, (oldVelocity * reverseContactNormal) * reverseContactNormal );//Not sure if this impulse is large enough

		const char* entityDamageName = spawnArgs.GetString( "def_damageFellOnto" );
		if( *entityDamageName && damageScale > 0.0f) {
			entity->Damage( this, this, fallDir, entityDamageName, damageScale, INVALID_JOINT );
		}
	}

	// Calculate damage to self
	const char* selfDamageName = NULL;
	if ( delta < crashlandSpeed_soft ) {			// Soft Fall
		AI_SOFTLANDING = true;
		selfDamageName = spawnArgs.GetString( "def_damageSoftFall" );
	}
	else if ( delta < crashlandSpeed_fatal ) {		// Hard Fall
		AI_HARDLANDING = true;
		selfDamageName = spawnArgs.GetString( "def_damageHardFall" );
	}
	else {											// Fatal Fall
		AI_HARDLANDING = true;
		selfDamageName = spawnArgs.GetString( "def_damageFatalFall" );
	}

	if( *selfDamageName && damageScale > 0.0f && !noDamage ) {
		pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
		Damage( NULL, NULL, fallDir, selfDamageName, damageScale, INVALID_JOINT );
	}
}

/*
===============
hhPlayer::ApplyBobCycle

HUMANHEAD: aob
===============
*/
#define BOB_TO_NOBOB_RETURN_TIME 3000.0f
idVec3 hhPlayer::ApplyBobCycle( const idVec3& pos, const idVec3& velocity ) {
	float			delta = 0.0f;
	idVec3			localViewBob( pos );
	const float		maxBob = 6.0f;

	if( bob > 0.0f && !usercmd.forwardmove && !usercmd.rightmove ) {//smoothly goto bob of zero if not already there
		delta = gameLocal.time - lastAppliedBobCycle;
		bob *= ( 1.0f - (delta / BOB_TO_NOBOB_RETURN_TIME) );
		if( bob < 0.0f ) {
			bob = 0.0f;
		}
	} else {
		// add bob height after any movement smoothing
		lastAppliedBobCycle = gameLocal.time;
		bob = bobfracsin * xyspeed * pm_bobup.GetFloat();
		if( bob > maxBob ) {
			bob = maxBob;
		}
	}
	
	localViewBob -= cameraInterpolator.GetCurrentUpVector() * bob;

	return localViewBob;
}


/*
===============
hhPlayer::BobCycle
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
===============
*/
void hhPlayer::BobCycle( const idVec3 &pushVelocity ) {
	float		bobmove;
	int			old; //, deltaTime;
	idVec3		vel, gravityDir, velocity;
	idMat3		viewaxis;
//	float		bob;
	float		delta;
	float		speed;
//	float		f;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	velocity = physicsObj.GetLinearVelocity() - pushVelocity;

	//HUMANHEAD: aob - changed GetGravityNormal to GetCurrentUpVector to smooth out wallwalk
	gravityDir = -cameraInterpolator.GetCurrentUpVector();
	//HUMANHEAD END
	vel = velocity - ( velocity * gravityDir ) * gravityDir;
	xyspeed = vel.LengthFast();

	// do not evaluate the bob for other clients
	// when doing a spectate follow, don't do any weapon bobbing
	if ( gameLocal.isClient && entityNumber != gameLocal.localClientNum ) {
		//HUMANHEAD rww - allow bob when following players in spectator mode
		bool canBob = false;
		if (gameLocal.localClientNum != -1 && gameLocal.entities[gameLocal.localClientNum] && gameLocal.entities[gameLocal.localClientNum]->IsType(hhPlayer::Type)) {
			hhPlayer *pl = static_cast<hhPlayer *>(gameLocal.entities[gameLocal.localClientNum]);
			if (pl->spectating && pl->spectator == entityNumber) {
				canBob = true;
			}
		}

		if (!canBob) {
		//HUMANHEAD END
			viewBobAngles.Zero();
			viewBob.Zero();
			return;
		}
	}

	if ( !physicsObj.HasGroundContacts() || influenceActive == INFLUENCE_LEVEL2 || ( gameLocal.isMultiplayer && spectating ) ) {
		// airborne
		bobCycle = 0;
		bobFoot = 0;
		bobfracsin = 0;
	} else if ( ( !usercmd.forwardmove && !usercmd.rightmove ) || ( xyspeed <= MIN_BOB_SPEED ) ) {
		// start at beginning of cycle again
		bobCycle = 0;
		bobFoot = 0;
		bobfracsin = 0;
	} else {
		if ( physicsObj.IsCrouching() ) {
			bobmove = pm_crouchbob.GetFloat();
			// ducked characters never play footsteps
		} else {
			// vary the bobbing based on the speed of the player
			bobmove = pm_walkbob.GetFloat() * ( 1.0f - bobFrac ) + pm_runbob.GetFloat() * bobFrac;	
		}

		// check for footstep / splash sounds
		old = bobCycle;
		bobCycle = (int)( old + bobmove * gameLocal.msec ) & 255;
		bobFoot = ( bobCycle & 128 ) >> 7;
		bobfracsin = idMath::Fabs( sin( ( bobCycle & 127 ) / 127.0 * idMath::PI ) );
	}

	// calculate angles for view bobbing
	viewBobAngles.Zero();

	viewaxis = viewAngles.ToMat3();

	// add angles based on velocity
	delta = velocity * viewaxis[0];
	viewBobAngles.pitch += delta * pm_runpitch.GetFloat();
	
	delta = velocity * viewaxis[1];
	viewBobAngles.roll -= delta * pm_runroll.GetFloat();

	// add angles based on bob
	// make sure the bob is visible even at low speeds
	speed = xyspeed > 200 ? xyspeed : 200;

	delta = bobfracsin * pm_bobpitch.GetFloat() * speed;
	if ( physicsObj.IsCrouching() ) {
		delta *= 3;		// crouching
	}
	viewBobAngles.pitch += delta;
	delta = bobfracsin * pm_bobroll.GetFloat() * speed;
	if ( physicsObj.IsCrouching() ) {
		delta *= 3;		// crouching accentuates roll
	}
	if ( bobFoot & 1 ) {
		delta = -delta;
	}
	viewBobAngles.roll += delta;

	//HUMANHEAD: aob
#if HUMANHEAD
	viewBob = ApplyBobCycle( ApplyLandDeflect(vec3_zero, 1.0f), velocity );
#else
	// calculate position for view bobbing
	viewBob.Zero();

	if ( physicsObj.HasSteppedUp() ) {

		// check for stepping up before a previous step is completed
		deltaTime = gameLocal.time - stepUpTime;
		if ( deltaTime < STEPUP_TIME ) {
			stepUpDelta = stepUpDelta * ( STEPUP_TIME - deltaTime ) / STEPUP_TIME + physicsObj.GetStepUp();
		} else {
			stepUpDelta = physicsObj.GetStepUp();
		}
		if ( stepUpDelta > 2.0f * pm_stepsize.GetFloat() ) {
			stepUpDelta = 2.0f * pm_stepsize.GetFloat();
		}
		stepUpTime = gameLocal.time;
	}

	idVec3 gravity = physicsObj.GetGravityNormal();

	// if the player stepped up recently
	deltaTime = gameLocal.time - stepUpTime;
	if ( deltaTime < STEPUP_TIME ) {
		viewBob += gravity * ( stepUpDelta * ( STEPUP_TIME - deltaTime ) / STEPUP_TIME );
	}

	// add bob height after any movement smoothing
	bob = bobfracsin * xyspeed * pm_bobup.GetFloat();
	if ( bob > 6 ) {
		bob = 6;
	}
	viewBob[2] += bob;

	// add fall height
	delta = gameLocal.time - landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		viewBob -= gravity * ( landChange * f );
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		viewBob -= gravity * ( landChange * f );
	}
#endif	//HUMANHEAD END
}

/*
================
hhPlayer::UpdateDeltaViewAngles
================
*/
void hhPlayer::UpdateDeltaViewAngles( const idAngles &angles ) {
	// set the delta angle
	idAngles delta;
	for( int i = 0; i < 3; i++ ) {
#if 1 //rww - revert to id's method
		//delta[i] = ((angles[i] - GetUntransformedViewAngles()[i]) / GetViewAnglesSensitivity()) + GetUntransformedViewAngles()[i] - SHORT2ANGLE(usercmd.angles[i]);
		delta[ i ] = angles[ i ] - SHORT2ANGLE( usercmd.angles[ i ] );
#else // HUMANHEAD mdl:  If you enable this, comment out the SetOrientation() call for vehicles in hhPlayer::Restore()
		delta[i] = (angles[i] - idMath::AngleNormalize180( SHORT2ANGLE(usercmd.angles[ i ]) - oldCmdAngles[i]) * GetViewAnglesSensitivity());
#endif
	}
	SetDeltaViewAngles( delta );
}

/*
================
hhPlayer::DetermineViewAngles
================
*/
idAngles hhPlayer::DetermineViewAngles( const usercmd_t& cmd, idAngles& cmdAngles ) {
	int i;
	idAngles localViewAngles( GetUntransformedViewAngles() );
	idAngles deltaCmdAngles;

	if ( !noclip && ( gameLocal.inCinematic || privateCameraView || gameLocal.GetCamera() || influenceActive == INFLUENCE_LEVEL2 ) ) {
		// no view changes at all, but we still want to update the deltas or else when
		// we get out of this mode, our view will snap to a kind of random angle
		return GetUntransformedViewAngles();
	}

	//HUMANHEAD rww - testing
	/*
	if (gameLoacl.isClient && gameLocal.localClientNum != entityNumber) {
		idQuat a = viewAngles.ToQuat();
		idQuat b = GetUntransformedViewAngles().ToQuat();
		idQuat c;
		c.Slerp(a, b, 0.3f);
		return c.ToAngles();
	}
	*/
	//HUMANHEAD END

	// if dead
	if ( IsDead() ) {
#if 0
		if ( DoThirdPersonDeath() ) {
			localViewAngles.roll = 0.0f;
			localViewAngles.pitch = 30.0f;
		}
		else {
			localViewAngles.roll = 40.0f;
			localViewAngles.pitch = -15.0f;
		}
		return localViewAngles;
#else //HUMANHEAD PCF rww 04/26/06 - look freely while dead in MP
		if ( !DoThirdPersonDeath() ) {
			localViewAngles.roll = 40.0f;
			localViewAngles.pitch = -15.0f;
			return localViewAngles;
		}
#endif //HUMANHEAD END
	}

	//JSHTODO this messes up multiplayer input.  remerge sensitivity code
	// circularly clamp the angles with deltas
	if ( usercmd.buttons & BUTTON_MLOOK ) {
		for ( i = 0; i < 3; i++ ) {
			cmdAngles[i] = SHORT2ANGLE( usercmd.angles[i] );
#if 1 //rww - revert to id's method
			//localViewAngles[i] = idMath::AngleNormalize180( cmdAngles[i] + deltaViewAngles[i] - GetUntransformedViewAngles()[i] ) * GetViewAnglesSensitivity() + GetUntransformedViewAngles()[i];
			localViewAngles[i] = idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[i]) + deltaViewAngles[i] );
#else
			deltaCmdAngles[i] = idMath::AngleNormalize180( cmdAngles[i] - oldCmdAngles[i] ) * GetViewAnglesSensitivity();
			oldCmdAngles[i] = cmdAngles[i];
			localViewAngles[i] = deltaCmdAngles[i] + deltaViewAngles[i];
#endif
		}
	} else {
#if 1 //rww - revert to id's method
		//localViewAngles.yaw = idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[YAW] ) + deltaViewAngles[YAW] - GetUntransformedViewAngles()[YAW] ) * GetViewAnglesSensitivity() + GetUntransformedViewAngles()[YAW];
		localViewAngles.yaw = idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[YAW]) + deltaViewAngles[YAW] );
#if GAMEPAD_SUPPORT	// VENOM BEGIN
		oldCmdAngles[YAW] = SHORT2ANGLE( usercmd.angles[YAW] );
#endif // VENOM END
#else
		deltaCmdAngles[YAW] = idMath::AngleNormalize180( SHORT2ANGLE(usercmd.angles[YAW]) - oldCmdAngles[YAW] ) * GetViewAnglesSensitivity();
		oldCmdAngles[YAW] = SHORT2ANGLE( usercmd.angles[YAW] );
		localViewAngles.yaw = deltaCmdAngles[YAW] + deltaViewAngles[YAW];
#endif

		if (!centerView.IsDone(gameLocal.time)) {
			localViewAngles.pitch = centerView.GetCurrentValue(gameLocal.time);
		}
	}

#if GAMEPAD_SUPPORT	// VENOM BEGIN
	float fpitch = idMath::Fabs(localViewAngles.pitch);
	if( idMath::Abs(usercmd.rightmove) < 10 && 
		idMath::Abs(usercmd.forwardmove) > 80  && 
		fpitch > 1.f  && 
		idMath::Fabs(oldCmdAngles[YAW] - cmdAngles[YAW]) < 1.0f && 
		idMath::Fabs(oldCmdAngles[PITCH] - cmdAngles[PITCH]) < 1.0f
	)
	{
		// dont start the leveling until we have .5 sec of valid movement
		if((gameLocal.time - lastAutoLevelTime) > 500 ) {
			float fadd = 0.6f;
			if(fpitch < 10.f) {
				fadd *= (fpitch/10.f );
				fadd+= 0.05f;
			}
			
			if(localViewAngles.pitch < 0 ) {
				localViewAngles.pitch +=fadd;
			}
			else {
				localViewAngles.pitch -=fadd;
			}
		}
	}
	else {
		lastAutoLevelTime = gameLocal.time;
	}

	oldCmdAngles[YAW]	= cmdAngles[YAW];
	oldCmdAngles[PITCH] = cmdAngles[PITCH];
#endif // VENOM END

	// clamp the pitch
	if ( noclip ) {
		localViewAngles.pitch = hhMath::ClampFloat( -89.0f, 89.0f, localViewAngles.pitch );
	} else {
		//HUMANHEAD PCF rww 04/26/06 - look freely while dead in MP
		if (IsDead() && DoThirdPersonDeath()) {
			localViewAngles.pitch = hhMath::ClampFloat( -45.0f, 45.0f, localViewAngles.pitch );
		}
		else {
		//HUMANHEAD END
			localViewAngles.pitch = hhMath::ClampFloat( pm_minviewpitch.GetFloat(), pm_maxviewpitch.GetFloat(), localViewAngles.pitch );
		}
	}

	// HUMANHEAD pdm: clamp the yaw (only used for bindControllers)
	if ( !noclip && bClampYaw ) {
		float idealYaw=0.0f;
		idVec3 masterOrigin;
		idMat3 masterAxis;

		if (GetMasterPosition(masterOrigin, masterAxis)) {
			idealYaw = masterAxis[0].ToYaw();
			idealYaw = idMath::AngleNormalize180( idealYaw );
		}

		// Transpose the everything to "zero" space before clamping, so idealYaw is at zero in a (-180..180) normalized space
		float zeroSpaceYaw = idMath::AngleNormalize180( localViewAngles.yaw - idealYaw );
		zeroSpaceYaw = idMath::ClampFloat(-maxRelativeYaw, maxRelativeYaw, zeroSpaceYaw);
		localViewAngles.yaw = idMath::AngleNormalize180( zeroSpaceYaw + idealYaw );
	}
	// HUMANHEAD END

	UpdateDeltaViewAngles( localViewAngles );

	// save in the log for analyzing weapon angle offsets
	loggedViewAngles[ gameLocal.framenum & (NUM_LOGGED_VIEW_ANGLES-1) ] = localViewAngles;

	return localViewAngles;
}

/*
================
hhPlayer::SetViewAnglesSensitivity
================
*/
void hhPlayer::SetViewAnglesSensitivity( float factor ) {
	viewAnglesSensitivity = factor;
	if (gameLocal.localClientNum == entityNumber) {
		common->SetGameSensitivityFactor(factor);
	}
}

/*
================
hhPlayer::GetViewAnglesSensitivity
================
*/
float hhPlayer::GetViewAnglesSensitivity() const {
	return viewAnglesSensitivity;
}

/*
================
hhPlayer::UpdateViewAngles
================
*/
void hhPlayer::UpdateViewAngles( void ) {
	if( InVehicle() ) {
		return;
	}

	if (!noclip && (InCinematic() && lockView)) {
		// no view changes at all, but we still want to update the deltas or else when
		// we get out of this mode, our view will snap to a kind of random angle
		UpdateDeltaViewAngles( viewAngles );
		return;
	}

	//HUMANHEAD: aob - moved logic into helper function so we can call this somewhere else while in a vehicle
	viewAngles = DetermineViewAngles( usercmd, cmdAngles );

	//HUMANHEAD rww - moved angle smoothing code here
	// update the smoothed view angles
	if (gameLocal.isClient) {
		if ( gameLocal.framenum >= smoothedFrame && entityNumber != gameLocal.localClientNum ) {
			idAngles anglesDiff = viewAngles - smoothedAngles;
			anglesDiff.Normalize180();
			if ( idMath::Fabs( anglesDiff.yaw ) < 90.0f && idMath::Fabs( anglesDiff.pitch ) < 90.0f ) {
				// smoothen by pushing back to the previous angles
				viewAngles -= gameLocal.clientSmoothing * anglesDiff;
				viewAngles.Normalize180();
			}
			smoothedAngles = viewAngles;
		}
		smoothedOriginUpdated = false;
	}

	// orient the model towards the direction we're looking
	UpdateOrientation( viewAngles );
	//HUMANHEAD END
}

/*
=====================
hhPlayer::UpdateOrientation

HUMANHEAD: aob
=====================
*/
void hhPlayer::UpdateOrientation( const idAngles& newUntransformedViewAngles ) {
	idAngles angles;
	//This is where the camera interpolator transforms the viewAngles to work with wallwalk.
	//We also store the original untransformed view angles for use in some other code.
	//idPlayer called SetAxis, but only used the yaw but because we are transforming our view angles
	//we do something similier by taking the untransformed yaw and rotating by the camera interpolator's axis.
	//This becomes our new axis, the camera interpolator axis plus the untransformed view angles yaw
	SetUntransformedViewAngles( newUntransformedViewAngles );

	angles = GetUntransformedViewAngles();
	if( IsWallWalking() ) {
		angles.pitch = 0.0f;
	}
	SetUntransformedViewAxis( idAngles( 0.0f, angles.yaw, 0.0f ).ToMat3() );

	viewAngles = cameraInterpolator.UpdateViewAngles( angles );
	SetAxis( TransformToPlayerSpace(GetUntransformedViewAxis()) );
}

//=============================================================================
//
// hhPlayer::CheckFOV
//
// Similar to idActor::CheckFOV, but isn't infinite along the vertical plane
//
// CJR
//=============================================================================

bool hhPlayer::CheckFOV( const idVec3 &pos ) {
	if ( fovDot == 1.0f ) {
		return true;
	}

	float	dot;
	idVec3	delta;
	
	delta = pos - GetEyePosition();
	delta.Normalize();

	dot = delta * viewAngles.ToForward();

	return ( dot >= fovDot );
}

//=============================================================================
// hhPlayer::CheckFOV
// jsh - Similar to CheckFOV, but only checks yaw
//=============================================================================
bool hhPlayer::CheckYawFOV( const idVec3 &pos ) {
	if ( fovDot == 1.0f ) {
		return true;
	}

	float	dot;
	idVec3	delta;
	idVec3	viewAng;

	delta = pos - GetEyePosition();
	delta.z = 0;
	delta.Normalize();
	viewAng = viewAngles.ToForward();
	viewAng.z = 0;
	dot = delta * viewAng;

	return ( dot >= fovDot );
}


/*
=====================
hhPlayer::SetOrientation

HUMANHEAD: aob
=====================
*/
void hhPlayer::SetOrientation( const idVec3& origin, const idMat3& bboxAxis, const idVec3& lookDir, const idAngles& newUntransformedViewAngles ) {
	//never let the untransformed view angles have a roll value
	idAngles modifiedUntransViewAngles = newUntransformedViewAngles;
	modifiedUntransViewAngles.roll = 0.0f;

	physicsObj.SetOrigin( GetLocalCoordinates(origin) );
	physicsObj.SetAxis( bboxAxis );
	physicsObj.CheckWallWalk( true );

	SetViewAngles( modifiedUntransViewAngles );
	BufferLoggedViewAngles( modifiedUntransViewAngles );
	SetUntransformedViewAngles( modifiedUntransViewAngles );

	SetAxis( lookDir.ToMat3() );
	SetUntransformedViewAxis( idAngles(0.0f, modifiedUntransViewAngles.yaw, 0.0f).ToMat3() );

	cameraInterpolator.Reset( origin, bboxAxis[2], EyeHeightIdeal() );
	
	UpdateViewAngles();
	UpdateVisuals();
}
/*
=====================
hhPlayer::RestoreOrientation

HUMANHEAD: mdl
Same as SetOrientation, except doesn't check wallwalk, since the materials may not be set before the first frame.
=====================
*/
void hhPlayer::RestoreOrientation( const idVec3& origin, const idMat3& bboxAxis, const idVec3& lookDir, const idAngles& newUntransformedViewAngles ) {
	physicsObj.SetOrigin( GetLocalCoordinates(origin) );
	physicsObj.SetAxis( bboxAxis );

	SetViewAngles( newUntransformedViewAngles );
	BufferLoggedViewAngles( newUntransformedViewAngles );
	SetUntransformedViewAngles( newUntransformedViewAngles );

	SetAxis( lookDir.ToMat3() );
	SetUntransformedViewAxis( idAngles(0.0f, newUntransformedViewAngles.yaw, 0.0f).ToMat3() );
	cameraInterpolator.Reset( origin, bboxAxis[2], EyeHeightIdeal() );
	
	UpdateViewAngles();
	UpdateVisuals();
}

/*
=====================
hhPlayer::BufferLoggedViewAngles

HUMANHEAD: aob - used when teleporting and using portals
=====================
*/
void hhPlayer::BufferLoggedViewAngles( const idAngles& newUntransformedViewAngles ) {
	idAngles deltaAngles = newUntransformedViewAngles - GetUntransformedViewAngles();

	for( int ix = 0; ix < NUM_LOGGED_VIEW_ANGLES; ++ix ) {
		loggedViewAngles[ix] += deltaAngles;
	}
}

/*
================
hhPlayer::UpdateFromPhysics

AOBMERGE - PERSISTANT MERGE
================
*/
void hhPlayer::UpdateFromPhysics( bool moveBack ) {

	// set master delta angles for actors
	if ( GetBindMaster() ) {
		if( !InVehicle() ) {
			idAngles delta = GetDeltaViewAngles();
			if ( moveBack ) {
				delta.yaw -= physicsObj.GetMasterDeltaYaw();
			} else {
				delta.yaw += physicsObj.GetMasterDeltaYaw();
			}
		
			SetDeltaViewAngles( delta );
		} else {
			SetUntransformedViewAxis( mat3_identity );
			SetUntransformedViewAngles( GetUntransformedViewAxis().ToAngles() );
			viewAngles = GetUntransformedViewAngles();
			SetAxis( GetBindMaster()->GetAxis() );

			//AOB - now that we have an updated viewAxis we need to update the 
			//camera.  Feels like a hack!
			cameraInterpolator.SetTargetAxis( GetAxis(), INTERPOLATE_NONE );
		}
	}

	UpdateVisuals();
}

/*
==============
hhPlayer::PerformImpulse
==============
*/

void hhPlayer::PerformImpulse( int impulse ) {
	if( InVehicle() && GetVehicleInterface()->InvalidVehicleImpulse(impulse) ) {
		return;
	}

	idPlayer::PerformImpulse( impulse );

	switch( impulse ) {
		case IMPULSE_14: {
			if ( weapon.IsValid() && weapon->IsType( hhWeaponZoomable::Type ) ) {
				hhWeaponZoomable *weap = static_cast<hhWeaponZoomable*>(weapon.GetEntity());
				if ( weap && weap->GetAltMode() ) {
					weap->ZoomInStep();
				} else {
					NextWeapon();
				}
			} else {
				NextWeapon();
			}
			break;
		}
		case IMPULSE_15: {
			if ( weapon.IsValid() && weapon->IsType( hhWeaponZoomable::Type ) ) {
				hhWeaponZoomable *weap = static_cast<hhWeaponZoomable*>(weapon.GetEntity());
				if ( weap && weap->GetAltMode() ) {
					weap->ZoomOutStep();
				} else {
					PrevWeapon();
				}
			} else {
				PrevWeapon();
			}
			break;
		}
		case IMPULSE_16:
			// Toggle Lighter
			if (inventory.requirements.bCanUseLighter && weaponFlags != 0) { // mdl:  Disable lighter if all weapons are disabled
				if (!gameLocal.isClient) {
					ToggleLighter();
				}
			}
			break;
		case IMPULSE_25:
			// Throw grenade
			if ( weaponFlags != 0 && !ActiveGui()) { // mdl:  Disable if all weapons are disabled
				if (!gameLocal.isClient) {
					ThrowGrenade();
				}
			}
			break;
		case IMPULSE_54:
			// Spirit Power key
			if ( IsDeathWalking() ) { // CJR:  Developer-only ability to quickly return from DeathWalk by hitting the spirit key
				if ( developer.GetBool() && ( gameLocal.time - deathWalkTime > spawnArgs.GetInt( "deathWalkMinTime", "4000" ) ) ) { // Force the player to stay in deathwalk for a short period of time
					deathWalkFlash = 0.0f;
					PostEventMS( &EV_PrepareToResurrect, 0 );
				}
			}
			else if (inventory.requirements.bCanSpiritWalk) {
				ToggleSpiritWalk();
			}
			break;

		case IMPULSE_18: {
#if GAMEPAD_SUPPORT	// VENOM BEGIN
			idAngles localViewAngles( GetUntransformedViewAngles() );
			centerView.Init(gameLocal.time, 360, localViewAngles.pitch, 0);
#else
			centerView.Init(gameLocal.time, 200, viewAngles.pitch, 0);
#endif // VENOM END
			break;
		}
	}
}

void hhPlayer::Present() {
	idPlayer::Present();

	if ( lighterHandle != -1 ) {
		// Update oscillation position
		idVec3 oscillation;
		oscillation.x = 0.0f;
		oscillation.y = idMath::Cos( MS2SEC(gameLocal.time) * spawnArgs.GetFloat("lighter_yspeed") ) * spawnArgs.GetFloat("lighter_ysize");
		oscillation.z = idMath::Sin( MS2SEC(gameLocal.time) * spawnArgs.GetFloat("lighter_zspeed") ) * spawnArgs.GetFloat("lighter_zsize");

		idVec3 offset;
		offset = spawnArgs.GetVector("offset_lighter");
		offset.z = EyeHeight();
		lighter.origin = GetOrigin() + GetAxis() * (offset + oscillation);
		lighter.axis = mat3_identity;

		lighter.shaderParms[ SHADERPARM_DIVERSITY ]		= renderEntity.shaderParms[ SHADERPARM_DIVERSITY ];

		gameRenderWorld->UpdateLightDef( lighterHandle, &lighter );
	}
}

void hhPlayer::LighterOn() {
	if (IsSpiritOrDeathwalking() || InVehicle() || spectating || bReallyDead) { // No lighter in spirit mode, deathwalk, or when in vehicles (or when spectating or really dead -rww)
		return;
	}

	if (lighterHandle == -1) {
		// Add the dynamic light
		memset( &lighter, 0, sizeof( lighter ) );
		lighter.lightId = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
		lighter.pointLight								= true; // false; ?
		lighter.shader									= declManager->FindMaterial( spawnArgs.GetString( "mtr_lighter" ) );
		lighter.shaderParms[ SHADERPARM_RED ]			= spawnArgs.GetFloat( "lighterColorR" );
		lighter.shaderParms[ SHADERPARM_GREEN ]			= spawnArgs.GetFloat( "lighterColorG" );
		lighter.shaderParms[ SHADERPARM_BLUE ]			= spawnArgs.GetFloat( "lighterColorB" );
		lighter.shaderParms[ SHADERPARM_ALPHA ]			= 1.0f;
		lighter.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
		lighter.lightRadius								= spawnArgs.GetVector( "lighter_radius" );

		lighter.origin = GetOrigin() + GetAxis() * spawnArgs.GetVector("offset_lighter");
		lighter.axis = mat3_identity;

		lighterHandle = gameRenderWorld->AddLightDef( &lighter );

		StartSound("snd_lighter_on", SND_CHANNEL_ITEM);
	}
}

void hhPlayer::LighterOff() {
	if (lighterHandle != -1) {
		gameRenderWorld->FreeLightDef( lighterHandle );
		lighterHandle = -1;
		StartSound("snd_lighter_off", SND_CHANNEL_ITEM);
	}
}

bool hhPlayer::IsLighterOn() const { 	//HUMANHEAD PCF mdl 05/04/06 - Made const
	return (lighterHandle != -1);
}

void hhPlayer::ToggleLighter() {
	if (IsLighterOn()) {
		LighterOff();
	}
	else {
		LighterOn();
	}
}

void hhPlayer::UpdateLighter() {
	if (IsLighterOn()) {
		// Increase the lighter's temperature until it overheats
		if (!gameLocal.isMultiplayer) { //rww - don't bother overheating the lighter in mp
			lighterTemperature += spawnArgs.GetFloat( "lighterHeatRate", "0.025" ) * MS2SEC( USERCMD_MSEC );
			if (lighterTemperature >= 1.0f) {
				// Too hot, turn the lighter off
				StartSound("snd_lighter_toohot", SND_CHANNEL_ANY);
				lighterTemperature = 1.0f;

				if (!(godmode || noclip)) {
					LighterOff();
				}
			}
		}
	}
	else {
		// Lighter is off, so decrease the lighter's temperature
		if (lighterTemperature > 0) {
			lighterTemperature -= spawnArgs.GetFloat( "lighterCoolRate", "0.05")  * MS2SEC( USERCMD_MSEC );
		}
	}
}

/*
==================
hhPlayer::PlayFootstepSound
==================
*/
void hhPlayer::PlayFootstepSound() {
	if ( IsSpiritOrDeathwalking() ) {
		return; // No footstep sounds in spiritwalk mode
	}

	if ( IsWallWalking() ) {
		// Special case wallwalk since contacts includes some non-wallwalk types
		const char* soundKey = gameLocal.MatterTypeToMatterKey( "snd_footstep", SURFTYPE_WALLWALK );
		StartSound( soundKey, SND_CHANNEL_BODY3, 0, false, NULL );
		return;
	}

	idPlayer::PlayFootstepSound();
}

/*
=====================
hhPlayer::PlayPainSound

HUMANHEAD: aob
=====================
*/
void hhPlayer::PlayPainSound() {
	if( IsSpiritOrDeathwalking() ) {
		return;
	}
	
	//HUMANHEAD PCF rww 09/15/06 - female mp sounds
	if (IsFemale()) {
		StartSound( "snd_pain_small_female", SND_CHANNEL_VOICE );
		return;
	}
	//HUMANHEAD END
	idPlayer::PlayPainSound();
}

/*
===============
hhPlayer::Give
===============
*/
bool hhPlayer::Give( const char *statname, const char *value ) {
	if( IsDead() ) {
		return false;
	}

	return inventory.Give( this, spawnArgs, statname, value, &idealWeapon, true );
}

/*
===============
hhPlayer::ReportAttack
===============
*/
void hhPlayer::ReportAttack(idEntity *attacker) {
	if (gameLocal.isServer && attacker) { //rww - broadcast event for this on the server
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init(msgBuf, sizeof(msgBuf));
		msg.WriteBits(gameLocal.GetSpawnId(attacker), 32);
		//unreliable since it's not super-important, and only send to this player.
		ServerSendEvent(EVENT_REPORTATTACK, &msg, false, -1, entityNumber, true);
	}

	int ix;

	for (ix=0; ix<MAX_TRACKED_ATTACKERS; ix++) {

		// If the attacker already exists, stomp it
		if (lastAttackers[ix].attacker == attacker) {
			lastAttackers[ix].time = gameLocal.time;
			lastAttackers[ix].displayed = false;
			return;
		}

		// If any are expired, free their slot
		if (gameLocal.time > lastAttackers[ix].time + DAMAGE_INDICATOR_TIME) {
			lastAttackers[ix].attacker = NULL;
		}
	}

	// Add this attacker to first free slot, if any
	for (ix=0; ix<MAX_TRACKED_ATTACKERS; ix++) {
		if (!lastAttackers[ix].attacker.IsValid()) {
			lastAttackers[ix].attacker = attacker;
			lastAttackers[ix].time = gameLocal.time;
			lastAttackers[ix].displayed = false;
			return;
		}
	}

	// All slots full, overwrite the oldest slot
	int oldSlot = -1;
	int oldTime = gameLocal.time;
	for (ix=0; ix<MAX_TRACKED_ATTACKERS; ix++) {
		if (lastAttackers[ix].time < oldTime) {
			oldTime = lastAttackers[ix].time;
			oldSlot = ix;
		}
	}
	if (oldSlot > 0) {
		lastAttackers[oldSlot].attacker = attacker;
		lastAttackers[oldSlot].time = gameLocal.time;
		lastAttackers[oldSlot].displayed = false;
	}
}

/*
==================
hhPlayer::Damage
==================
*/
void hhPlayer::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
					   const char *damageDefName, const float damageScale, const int location ) {

	if(	IsSpiritOrDeathwalking() ) { //Player is spirit-walking, so check for special immunities
		const idKeyValue *kv = spawnArgs.MatchPrefix("immunityspirit");
		while( kv && kv->GetValue().Length() ) {
			if ( !kv->GetValue().Icmp(damageDefName) ) {
				return;
			}
			kv = spawnArgs.MatchPrefix("immunityspirit", kv);
		}

		// If the player is spiritwalking, then any damage takes away spirit power instead of health
		ammo_t ammo_spiritpower = idWeapon::GetAmmoNumForName( "ammo_spiritpower" );
		if ( IsSpiritWalking() ) {
			const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
			if ( damageDef ) {
				idPlayer *player = (attacker && attacker->IsType( idPlayer::Type )) ? static_cast<idPlayer*>(attacker) : NULL;
				if ( !(gameLocal.gameType == GAME_TDM
					&& !gameLocal.serverInfo.GetBool( "si_teamDamage" )
					&& !damageDef->GetBool( "noTeam" )
					&& player
					&& player != this		// you get self damage no matter what
					&& player->team == team) ) {
					//rww - don't damage teammates' spirit when ff off
					int damageWhenSpirit = damageDef->GetInt("damageWhenSpirit", "0"); //rww - special damage for knocking players back to body in mp when shot by other things (namely spirit arrows)
					int oldSpirit = inventory.ammo[ ammo_spiritpower ];

					int spiritDamage = (damageDef->GetInt( "damage", "1" ) * spawnArgs.GetFloat( "damageScaleInSpirit" ) * damageScale )+damageWhenSpirit;
					if ( spiritDamage <= 0 && damageScale > 0 ) {
						spiritDamage = 1;
					}

					if ( !UseAmmo( ammo_spiritpower, spiritDamage ) ) {
						inventory.ammo[ ammo_spiritpower ] = 0; // Clear spiritpower amount when returning from excessive damage
					}
				}
			}

			lastDamagedTime = gameLocal.time; // Save the damage time for the health recharge code

			// Track last attacker for use in displaying HUD hit indicator
			if (!gameLocal.isClient) {
				ReportAttack(attacker);
			}

			//HUMANHEAD rww - damage feedback for hitting spirit players in mp
			if (gameLocal.isMultiplayer && attacker && !gameLocal.isClient) {
				hhPlayer *killer = NULL;
				if (attacker->IsType(idPlayer::Type)) {
					killer = static_cast<hhPlayer*>(attacker);
				}
				else if (attacker->IsType(hhVehicle::Type)) {
					hhVehicle *veh = static_cast<hhVehicle*>(attacker);
					if (veh->GetPilot() && veh->GetPilot()->IsType(idPlayer::Type)) {
						killer = static_cast<hhPlayer*>(veh->GetPilot());
					}
				}

				if (killer && killer->entityNumber != entityNumber && killer->mpHitFeedbackTime <= gameLocal.time) {
					if (killer == gameLocal.GetLocalPlayer()) {
						assert(IsSpiritOrDeathwalking());
						if (gameLocal.gameType == GAME_TDM && team == killer->team) {
							killer->StartSound( "snd_hitTeamFeedback", SND_CHANNEL_ITEM, 0, false, NULL );
						}
						else {
							killer->StartSound( "snd_hitSpiritFeedback", SND_CHANNEL_ITEM, 0, false, NULL );

							//hardcoded health ranges for the various flash colors
							float h;
							if (health > 100) {
								h = 0.0f;
							}
							else if (health > 75) {
								h = 0.25f;
							}
							else if (health > 25) {
								h = 0.50f;
							}
							else {
								h = 0.75f;
							}
							SetShaderParm(3, h);
							SetShaderParm(5, -MS2SEC(gameLocal.time)*2);
						}
					}
					else {
						idBitMsg	msg;
						byte		msgBuf[MAX_EVENT_PARAM_SIZE];

						msg.Init(msgBuf, sizeof(msgBuf));
						msg.WriteBits(gameLocal.GetSpawnId(this), 32);
						killer->ServerSendEvent(EVENT_HITNOTIFICATION, &msg, false, -1, killer->entityNumber);
					}
				}
			}
			//HUMANHEAD END

			return;
		}
	}

	lastDamagedTime = gameLocal.time; // Save the damage time for the health recharge code

	// Track last attacker for use in displaying HUD hit indicator
	if (!gameLocal.isClient) {
		ReportAttack(attacker);
	}

	// Check if the damage should "really kill" the player (when in deathwalk mode)
	if ( IsDeathWalking() ) {
		const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
		if ( damageDef ) {
			if ( damageDef->GetBool( "reallyKill", "0" ) ) { // Truly kill the player
				ReallyKilled( inflictor, attacker, damageDef->GetInt( "damage", "9999" ), dir, location );
				return;
			} else if ( damageDef->GetBool( "spiritDamage", "0" ) ) { // Drain spirit power
				ammo_t ammo_spiritpower = idWeapon::GetAmmoNumForName( "ammo_spiritpower" );
				UseAmmo( ammo_spiritpower, damageDef->GetInt( "damage", "1" ) * damageScale );
				return;
			}
		}
	}

	//HUMANHEAD rww - keep track of last person to attack me, debounce time will vary based on ground contact
	if (attacker && attacker->IsType(idPlayer::Type)) {
		airAttacker = attacker;
		if (!AI_ONGROUND) {
			airAttackerTime = gameLocal.time + 300;
		}
		else { //if they are on the ground do a 100ms debounce in case they fall off a ledge or something as a result of leftover attack velocity
			airAttackerTime = gameLocal.time + 100;
		}
	}
	//HUMANHEAD END

	// CJR: DDA:  Only in single player
	if ( !gameLocal.isMultiplayer ) {
		float newDamageScale = gameLocal.GetDDAValue() * 2.0f; // Scale damage from easy to hard
		newDamageScale *= damageScale;

		const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
		if ( damageDef ) {
			bool noGod = damageDef->GetBool("noGod", "0");
			if ( noGod ) {
				// Make sure fatal damage bypasses post-deathwalk invulnerability
				fl.takedamage = true;
			}
		}

		int oldHealth = health;
		idPlayer::Damage( inflictor, attacker, dir, damageDefName, (const float)newDamageScale, location ); // CJR DDA TEST

		if ( attacker && attacker->IsType( hhMonsterAI::Type ) ) { // CJR DDA: Damaged by a monster, add the damage to the monster count
			float delta = oldHealth - health;

			hhMonsterAI *monster = static_cast<hhMonsterAI *>( attacker );
			
			if ( monster ) {
				monster->DamagedPlayer( delta );
			}
		}
	} else {
		idPlayer::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	}	

	//HUMANHEAD rww - hit feedback
	if (gameLocal.isMultiplayer && attacker && !gameLocal.isClient) { //let's broadcast from the server only, so hit feedback is always reliable
		hhPlayer *killer = NULL;
		if (attacker->IsType(idPlayer::Type)) {
			killer = static_cast<hhPlayer*>(attacker);
		}
		else if (attacker->IsType(hhVehicle::Type)) {
			hhVehicle *veh = static_cast<hhVehicle*>(attacker);
			if (veh->GetPilot() && veh->GetPilot()->IsType(idPlayer::Type)) {
				killer = static_cast<hhPlayer*>(veh->GetPilot());
			}
		}

		if (killer && killer->entityNumber != entityNumber && killer->mpHitFeedbackTime <= gameLocal.time) {
			if (killer == gameLocal.GetLocalPlayer()) {
				//don't provide visual indicator when shooting a teammate
				if (gameLocal.gameType == GAME_TDM && team == killer->team) {
					killer->StartSound( "snd_hitTeamFeedback", SND_CHANNEL_ITEM, 0, false, NULL );
				}
				else {
					if (IsSpiritOrDeathwalking()) {
						killer->StartSound( "snd_hitSpiritFeedback", SND_CHANNEL_ITEM, 0, false, NULL );
					}
					else {
						killer->StartSound( "snd_hitFeedback", SND_CHANNEL_ITEM, 0, false, NULL );
					}

					//hardcoded health ranges for the various flash colors
					float h;
					if (health > 100) {
						h = 0.0f;
					}
					else if (health > 75) {
						h = 0.25f;
					}
					else if (health > 25) {
						h = 0.50f;
					}
					else {
						h = 0.75f;
					}
					SetShaderParm(3, h);
					SetShaderParm(5, -MS2SEC(gameLocal.time)*2);
				}
			}
			else {
				idBitMsg	msg;
				byte		msgBuf[MAX_EVENT_PARAM_SIZE];

				msg.Init(msgBuf, sizeof(msgBuf));
				msg.WriteBits(gameLocal.GetSpawnId(this), 32);
				killer->ServerSendEvent(EVENT_HITNOTIFICATION, &msg, false, -1, killer->entityNumber);
			}

			//see how this feels (and more importantly how destructive it is toward bandwidth)
			//killer->mpHitFeedbackTime = gameLocal.time + USERCMD_MSEC;
		}
	}
	//HUMANHEAD END

	if ( bFrozen ) {
		const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
		if ( damageDef && damageDef->GetInt( "free_cocoon", "0" ) ) {
			Event_Unfreeze();
		}
	}
}

void hhPlayer::DoDeathDrop() {
	// General dropping (for monsters, souls, etc.)
	const idKeyValue *kv = NULL;
	kv = spawnArgs.MatchPrefix( "def_drops", NULL );
	while ( kv ) {

		idStr drops = kv->GetValue();
		idDict args;

		idStr last5 = kv->GetKey().Right(5);
		if ( drops.Length() && idStr::Icmp( last5, "Joint" ) != 0) {

			args.Set( "classname", drops );

			// HUMANHEAD pdm: specify monster so souls can call back to remove body when picked up
			args.Set("monsterSpawnedBy", name.c_str());

			idVec3 origin;
			idMat3 axis;
			idStr jointKey = kv->GetKey() + idStr("Joint");
			idStr jointName = spawnArgs.GetString( jointKey );
			idStr joint2JointKey = kv->GetKey() + idStr("Joint2Joint");
			idStr j2jName = spawnArgs.GetString( joint2JointKey );

			idEntity *newEnt = NULL;
			gameLocal.SpawnEntityDef( args, &newEnt );
			HH_ASSERT(newEnt != NULL);

			if(jointName.Length()) {
				jointHandle_t joint = GetAnimator()->GetJointHandle( jointName );
				if (!GetAnimator()->GetJointTransform( joint, gameLocal.time, origin, axis ) ) {
					gameLocal.Printf( "%s refers to invalid joint '%s' on entity '%s'\n", (const char*)jointKey.c_str(), (const char*)jointName, (const char*)name );
					origin = renderEntity.origin;
					axis = renderEntity.axis;
				}
				axis *= renderEntity.axis;
				origin = renderEntity.origin + origin * renderEntity.axis;
				newEnt->SetAxis(axis);
				newEnt->SetOrigin(origin);
			}
			else {
				newEnt->SetAxis(viewAxis);
				newEnt->SetOrigin(GetOrigin());
			}
		}
		
		kv = spawnArgs.MatchPrefix( "def_drops", kv );
	}
}

//HUMANHEAD rww
/*
==================
hhPlayer::Event_RespawnCleanup
performs any necessary operations on death to prepare for a clean spawn.
only cosmetic things should be placed here, this function will not be called
for sure on death, at least for the client.
==================
*/
void hhPlayer::Event_RespawnCleanup(void) {
	//remove any fx entities which are bound to the player
	idEntity *ent;
	idEntity *next;

	for( ent = teamChain; ent != NULL; ent = next ) {
		next = ent->GetTeamChain();
		if ( ent->GetBindMaster() == this && ent->IsType(hhEntityFx::Type) ) {
			ent->Unbind();
			if( !ent->fl.noRemoveWhenUnbound ) {
				ent->PostEventMS( &EV_Remove, 0 );
				if (gameLocal.isClient) {
					ent->Hide();
				}
			}
			next = teamChain;
		}
	}
}
//HUMANHEAD END

/*
==================
hhPlayer::Killed
==================
*/
void hhPlayer::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	//you only die once
	if( AI_DEAD ) {
		return;
	}

	CancelEvents(&EV_DamagePlayer); //rww - don't do any more posted damage events once dead

	if (gameLocal.isMultiplayer) { //rww - this was not handled at all, i guess because there are different circumstances for "death" in sp
		wallwalkSoundController.StopSound( SND_CHANNEL_WALLWALK, SND_CHANNEL_WALLWALK2, false );
		spiritwalkSoundController.StopSound( SND_CHANNEL_BODY, SND_CHANNEL_BODY2 );

		LighterOff();

		if (!gameLocal.isClient && weapon.IsValid() && weapon->IsType(hhWeapon::Type) && weapon->CanDrop()) { //when dying in mp, toss my weapon out.
			idVec3 forward, up;

			viewAngles.ToVectors( &forward, NULL, &up );
			//rww - hackishness to keep the type of ammo in the leechgun that it was dropped with
			idEntity *dropped = weapon->DropItem( 50.0f * forward + 50.0f * up, 0, 60000, true );
			if (dropped && weapon->IsType(hhWeaponSoulStripper::Type)) {
				dropped->spawnArgs.Set("def_droppedEnergyType", inventory.energyType);
			}
		}
	}

	if (!gameLocal.isClient) {
		DoDeathDrop();
	}

	if ( gameLocal.isMultiplayer && !gameLocal.IsCooperative() ) {
		bDeathWalk = false;
		bSpiritWalk = false;
		bReallyDead = true;
	}
	else if ( !inventory.requirements.bCanDeathWalk || !gameLocal.DeathwalkMapLoaded() ) {
		bDeathWalk = false;
		bSpiritWalk = false;
		bReallyDead = true;
	}

	//First thing we do is get out of vehicle
	if (InVehicle()) {
		GetVehicleInterface()->GetVehicle()->EjectPilot();
	}

	// HUMANHEAD cjr:  Update DDA
	if ( !AI_DEAD && !gameLocal.isMultiplayer ) {
		gameLocal.GetDDA()->DDA_AddDeath( this, attacker );
	}
	// HUMANHEAD END

	if ( attacker && attacker->spawnArgs.GetBool("death_look_at", "0"))  {
		attacker->spawnArgs.GetString("death_look_at_bone", "", deathLookAtBone); 
		attacker->spawnArgs.GetString("death_camera_bone", "", deathCameraBone );
		deathLookAtEntity = attacker;
	}

	if ( weapon.IsValid() ) {
		//HUMANHEAD bjk 04/26/06 - no sniper scope when dead
		if ( weapon->IsType( hhWeaponRifle::Type ) ) {
			static_cast<hhWeaponRifle*>( weapon.GetEntity() )->ZoomOut();
		}
		weapon->PutAway();
	}

	// HUMANHEAD nla
	if ( hand.IsValid() ) {
		hand->PutAway();
	}
	// HUMANHEAD END

	if ( !bReallyDead ) { // Only go into deathwalk mode if the player is in the transitional death state
		idVec3 origin;
		idMat3 axis;
		idVec3 viewDir;
		idAngles angles;
		idMat3 eyeAxis;
		GetResurrectionPoint( origin, axis, viewDir, angles, eyeAxis, GetPhysics()->GetAbsBounds(), GetOrigin(), GetPhysics()->GetAxis(), GetAxis()[0], viewAngles );
		DeathWalk( origin, axis, viewDir.ToMat3(), angles, eyeAxis );
	} else {
		if (gameLocal.isMultiplayer) { //HUMANHEAD rww
			PostEventMS(&EV_RespawnCleanup, 32);
			if (!gameLocal.isClient) {
				StartRagdoll(); //start to rag on the player so that the proxy copies off proper af status

				GetPhysics()->SetContents( 0 ); //make non-solid

				hhMPDeathProxy *prox = (hhMPDeathProxy *)hhSpiritProxy::CreateProxy( spawnArgs.GetString("def_deathProxy_mp"), this, GetOrigin(), GetPhysics()->GetAxis(), viewAxis, viewAngles, GetEyeAxis() );
				assert(((hhSpiritProxy *)prox)->IsType(hhMPDeathProxy::Type));
				if (prox) {
					idVec3 flingForce;
					float capDmg = (float)damage;
					if (capDmg > 200.0f) {
						capDmg = 200.0f;
					}
						
					flingForce = dir;
					flingForce.Normalize();
					flingForce *= capDmg;

					prox->GetPhysics()->AddForce(0, prox->GetPhysics()->GetOrigin(0), flingForce*256.0f*256.0f);
					prox->SetFling(prox->GetPhysics()->GetOrigin(0), flingForce);
				}

				StopRagdoll(); //stop again, as we don't need to be ragging on the actual player
			}

			Hide(); //hide player

			minRespawnTime = gameLocal.time + RAGDOLL_DEATH_TIME;
			maxRespawnTime = minRespawnTime + 10000;
		} else {
			GetPhysics()->SetContents(0);
			Hide();

			minRespawnTime = gameLocal.time + 2000;
			maxRespawnTime = minRespawnTime + 5000;
		} //HUMANHEAD END

		physicsObj.SetMovementType( PM_DEAD );
		SAFE_REMOVE( weapon );
	}

	AI_DEAD = true;

	//HUMANHEAD rww
	if (gameLocal.isMultiplayer) {
		SetAnimState( ANIMCHANNEL_LEGS, "Legs_Death", 4 );
		SetAnimState( ANIMCHANNEL_TORSO, "Torso_Death", 4 );
		SetWaitState( "" );
	}
	//HUMANHEAD END

	//HUMANHEAD PCF rww 09/15/06 - female mp sounds
	if (IsFemale()) {
		StartSound( "snd_death_female", SND_CHANNEL_VOICE );
	}
	else {
		//HUMANHEAD END
	StartSound( "snd_death", SND_CHANNEL_VOICE );
	}

	if ( gameLocal.isMultiplayer && !gameLocal.isCoop ) {
		idPlayer *killer = NULL;

		if ( attacker->IsType( idPlayer::Type ) ) {
			killer = static_cast<idPlayer*>(attacker);
		}
		else { //rww - otherwise try to credit airAttacker
			if (airAttacker.IsValid() &&
				airAttacker.GetEntity() &&
				airAttackerTime > gameLocal.time) {

				if (airAttacker->IsType(idPlayer::Type)) {
					killer = static_cast<idPlayer*>(airAttacker.GetEntity());
				}
				else if (airAttacker->IsType(hhVehicle::Type)) {
					hhVehicle *veh = static_cast<hhVehicle*>(airAttacker.GetEntity());
					if (veh->GetPilot() && veh->GetPilot()->IsType(idPlayer::Type)) {
						killer = static_cast<idPlayer*>(veh->GetPilot());
					}
				}
			}
		}
		gameLocal.mpGame.PlayerDeath( this, killer, inflictor, false ); //HUMANHEAD rww - pass inflictor
	}

	UpdateVisuals();

	airAttackerTime = 0; //HUMANHEAD rww - reset air attacker time once dead
}

/*
==============
hhPlayer::Kill
==============
*/
void hhPlayer::Kill( bool delayRespawn, bool nodamage ) {
	if (noclip) {	// HUMANHEAD pdm: Because of deathwalk, this shouldn't be allowed
		return;
	}

	if ( health > 0 ) {
		//HUMANHEAD rww
		if (IsSpiritOrDeathwalking()) {
			if (!IsSpiritWalking()) { //don't "kill" when dead.
				return;
			}
			StopSpiritWalk(true);
		}
		//HUMANHEAD END

		godmode = false;
		health = 0;
		//HUMANHEAD rww - if in a vehicle, eject now
		if (InVehicle()) {
			if (vehicleInterfaceLocal.GetVehicle()) {
				vehicleInterfaceLocal.GetVehicle()->EjectPilot();
			}
		}
		//HUMANHEAD END
		Damage( NULL, NULL, vec3_origin, "damage_suicide", 1.0f, INVALID_JOINT );
	}
}

/*
===============
hhPlayer::DetermineOwnerPosition

HUMANHEAD: aob
===============
*/
void hhPlayer::DetermineOwnerPosition( idVec3 &ownerOrigin, idMat3 &ownerAxis ) {
	ownerAxis	= TransformToPlayerSpace( GetUntransformedViewAxis() );
	ownerOrigin = cameraInterpolator.GetCurrentPosition() + idVec3(g_gun_x.GetFloat(), g_gun_y.GetFloat(), g_gun_z.GetFloat()) * GetAxis();
}

/*
===============
hhPlayer::GetViewPos
===============
*/
//HUMANHEAD bjk
void hhPlayer::GetViewPos( idVec3 &origin, idMat3 &axis ) {
	idAngles angles;

	// if dead, fix the angle and don't add any kick
	// HUMANHEAD cjr:  Replaced health <= 0 with IsDead() call for deathwalk override
	if ( IsDead() && !gameLocal.isMultiplayer ) { //rww - don't want this in mp.
	// HUMANHEAD END
		angles.yaw = viewAngles.yaw;
		angles.roll = 40;
		angles.pitch = -15;
		axis = angles.ToMat3();
		origin = GetEyePosition();
	} else {
		assert(kickSpring < 500.0f && kickSpring > 0.0f);
		assert(kickDamping < 500.0f && kickDamping > 0.0f);
		origin = viewBob + TransformToPlayerSpace( idVec3(g_viewNodalX.GetFloat(), g_viewNodalZ.GetFloat(), g_viewNodalZ.GetFloat() + EyeHeight()) );
		axis = TransformToPlayerSpace( (GetUntransformedViewAngles() + viewBobAngles + playerView.AngleOffset(kickSpring, kickDamping)).ToMat3() );		
	}
}

/*
===============
hhPlayer::CalculateRenderView
===============
*/
void hhPlayer::CalculateRenderView( void ) {
	idPlayer::CalculateRenderView();

	// HUMANHEAD cjr
	if ( IsSpiritOrDeathwalking() ) { // If spiritwalking, then allow the player to see special objects
		renderView->viewSpiritEntities = true;
	}
}


/*
===============
hhPlayer::OffsetThirdPersonView
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
===============
*/
void hhPlayer::OffsetThirdPersonView( float angle, float range, float height, bool clip ) {
	idVec3			view;
	trace_t			trace;
	idMat3			lookAxis;
	idVec3			origin;
	idAngles		angles( GetUntransformedViewAngles() );

	if ( angle ) {
		angles.pitch = 0.0f;
	}

	if ( angles.pitch > 45.0f && !InVehicle() ) {
		angles.pitch = 45.0f;		// don't go too far overhead
	}

	//HUMANHEAD: aob
	lookAxis = TransformToPlayerSpace( angles.ToMat3() * idAngles(0.0f, angle, 0.0f).ToMat3() );
	if( InVehicle() ) {
		origin = GetOrigin() + lookAxis[2] * (height + EyeHeight());
	} else {
		origin = GetEyePosition() + GetEyeAxis()[2] * height;
	}
	view = origin + lookAxis[0] * -range;
	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 16 by 16 block to prevent the view from near clipping anything
	if( !noclip ) {
		int mask = MASK_SHOT_BOUNDINGBOX;

		if (gameLocal.isMultiplayer) { //rww - don't want this hitting corpses. also, shouldn't we pay attention to "clip"?
			mask = MASK_PLAYERSOLID;
		}

		thirdPersonCameraClipBounds.SetOwner( this );
		idEntity* ignore = (InVehicle()) ? GetVehicleInterface()->GetVehicle() : (idEntity*)this;
		gameLocal.clip.Translation( trace, origin, view, &thirdPersonCameraClipBounds, lookAxis, mask, ignore );
		range *= trace.fraction;
	}

	renderView->vieworg = origin + lookAxis[0] * -range;
	renderView->viewaxis = lookAxis;
	//HUMANHEAD END
	renderView->viewID = 0;
}


/*
==============================
hhPlayer::GetGuiHandInfo
  Retrns the proper gui hand info for a given gui
==============================
*/
const char *hhPlayer::GetGuiHandInfo() {
	const char *attrib;
	idDict	*item = NULL;
	const char *handString;

	// Set the gui to:
	//	"required_attribute"	"Hunter Hand"
	// If item exists in player inventory, player uses guihand specified by item
	if (!IsSpiritWalking() && focusGUIent && focusGUIent->spawnArgs.GetString("required_attribute", NULL, &attrib)) {
		item = FindInventoryItem( attrib );
		if ( item ) {
			if (item->GetString("def_guihand", NULL, &handString)) {
				return handString;
			}
		}
	}

	// If no required attribute/player doesn't have the item, check for a def_guihand string
	if ( focusGUIent && focusGUIent->spawnArgs.GetString( "def_guihand", NULL, &handString ) ) {
		return( handString );
	}

	// Otherwise, use the default player hand
	return spawnArgs.GetString("def_guihand");

}


//=============================================================================
//
// Spirit walk functions
//
//=============================================================================

void hhPlayer::StartSpiritWalk( const bool bThrust, bool force ) {
	hhFxInfo	fxInfo;
	idVec3		origin;
	idMat3		axis;

	//rww - don't do anything on the client
	if (gameLocal.isClient) {
		return;
	}

	if ( bReallyDead ) { // Don't allow spiritwalking if truly dead
		if (force) {
			gameLocal.Error("Attempted to force spirit walk when dead.\n");
		}
		return;
	}

	// Make sure spirit walk isn't disabled
	if ( !bAllowSpirit ) {
		StartSound("snd_spiritWalkDenied", SND_CHANNEL_ANY);
		return;
	}

	spiritWalkToggleTime = gameLocal.time;

	if ( gameLocal.isMultiplayer ) { // CJR:  Don't allow spiritwalking in MP unless the player has spirit power
		if ( GetSpiritPower() <= 0 ) {
			StartSound("snd_spiritWalkDenied", SND_CHANNEL_ANY, 0, true);
			return;
		}
	}

	if ( !force && nextSpiritTime > gameLocal.time ) { // mdl: Make sure they didn't just get knocked back into their body by a wraith
		StartSound("snd_spiritWalkDenied", SND_CHANNEL_ANY);
		return;
	}

	if (!IsSpiritOrDeathwalking()) {
		if ( !gameLocal.IsLOTA() ) { // In LOTA, spirit power never drains
			PostEventMS( &EV_DrainSpiritPower, spiritDrainHeartbeatMS );
		}

		if ( !bThrust ) { // Normal spiritwalking
			EnableEthereal( spawnArgs.GetString( "def_spiritProxy" ), GetOrigin(), GetPhysics()->GetAxis(), viewAxis, viewAngles, GetEyeAxis() );
		} else { // Knocked out by a wraith
			EnableEthereal( spawnArgs.GetString( "def_possessedProxy" ), GetOrigin(), GetPhysics()->GetAxis(), viewAxis, viewAngles, GetEyeAxis() );
		}

		SelectEtherealWeapon();

		spiritwalkSoundController.StartSound( SND_CHANNEL_BODY, SND_CHANNEL_BODY2 );

		// Update HUD
		if (hud) {
			hud->HandleNamedEvent("SwitchToEthereal");
		}

		// Set the player's skin to a glowy effect
		SetSkinByName( spawnArgs.GetString("skin_Spiritwalk") );
		SetShaderParm( SHADERPARM_TIMEOFFSET, 1.0f ); // TEMP: cjr - Required by the forcefield material.  Can remove when a proper spiritwalk texture is made

		// Spawn in a flash
		GetViewPos( origin, axis );
		fxInfo.SetEntity( this );
		fxInfo.RemoveWhenDone( true );
		fxInfo.SetBindBone( "origin" );
		BroadcastFxInfoPrefixed( "fx_spiritWalkFlash", origin, axis, &fxInfo );

		// Thrust the player backwards out of the body
		if ( bThrust ) {
			idVec3 vec = GetPhysics()->GetAxis()[0] * -200.0f + GetPhysics()->GetAxis()[2] * 50.0f;

			physicsObj.SetLinearVelocity( physicsObj.GetLinearVelocity() + vec );
			// set the timer so that the player can't cancel out the movement immediately
			physicsObj.SetKnockBack( 100 );
		}

		// bg - trigger map entity, provides a simple hook for map/scripts
		idEntity *swTrig = gameLocal.FindEntity( "sw_spiritWalkEntered" );
		if( swTrig ) {
			swTrig->PostEventMS( &EV_Activate, 0, this );
		}
	}
}

void hhPlayer::StopSpiritWalk(bool forceAllowance) {
	hhFxInfo	fxInfo;
	idVec3		origin;
	idMat3		axis;

	//rww - don't do anything on the client
	if (gameLocal.isClient) {
		return;
	}

	if ( IsPossessed() ) { // If possessed and the player stops spiritwalking, they die
		PossessKilled();
		return;
	}

	spiritWalkToggleTime = gameLocal.time;

	if ( bReallyDead ) { // Don't allow spiritwalking if truly dead
		return;
	}

	CancelEvents(&EV_SetOverlayMaterial); //HUMANHEAD rww - if being forced back out of spirit mode quickly enough, don't set overlay afterward

	if (IsSpiritOrDeathwalking()) {
		CancelEvents(&EV_DrainSpiritPower);
		if( weapon.IsValid() && weapon->IsType(hhWeaponSpiritBow::Type) ) {
			hhWeaponSpiritBow *bow = static_cast<hhWeaponSpiritBow *>( weapon.GetEntity() );
			if( bow->BowVisionIsEnabled() ) {
				bow->StopBowVision();
			}
		}
		DisableEthereal();
		spiritwalkSoundController.StopSound( SND_CHANNEL_BODY, SND_CHANNEL_BODY2 );
		SetSkin( NULL );

		// bg - trigger map entity, provides a simple hook for map/scripts
		idEntity *swTrig = gameLocal.FindEntity( "sw_spiritWalkExited" );
		if( swTrig ) {
			swTrig->PostEventMS( &EV_Activate, 0, this );
		}

		// Update HUD
		if (hud) {
			hud->HandleNamedEvent("SwitchFromEthereal");
		}
		buttonMask |= BUTTON_ATTACK;	//HUMANHEAD bjk

		// Spawn in a flash
		GetViewPos( origin, axis );
		fxInfo.SetEntity( this );
		fxInfo.RemoveWhenDone( true );
		fxInfo.SetBindBone( "origin" );
		BroadcastFxInfoPrefixed( "fx_spiritWalkFlash", origin, axis, &fxInfo );
	}
}

void hhPlayer::ToggleSpiritWalk( void ) {
	if (spectating) { //rww - do not allow spectators to spirit walk.
		return;
	}

	if ( bPossessed ) { // Cannot toggle away from spiritwalk when possessed
		StartSound("snd_spiritWalkDenied", SND_CHANNEL_ANY);
		return;
	}

	// spiritwalk time check -- force a delay between spiritwalking and not
	if (gameLocal.time < spiritWalkToggleTime + 250) {
		return;
	}

	if( IsSpiritOrDeathwalking() ) {
		StopSpiritWalk();
	} else {
		StartSpiritWalk( false );
	}
}

//HUMANHEAD rww
void hhPlayer::RestorePlayerLocationFromDeathwalk( const idVec3& origin, const idMat3& bboxAxis, const idVec3& viewDir, const idAngles& angles ) {
	idVec3		newOrigin;
	idMat3		newAxis;
	idVec3		newViewDir;
	idAngles	newAngles;

	if( deathwalkLastCrouching ) {
		ForceCrouching();
	}

	SetEyeAxis(deathwalkLastEyeAxis);
	GetResurrectionPoint( newOrigin, newAxis, newViewDir, newAngles, deathwalkLastEyeAxis, GetPhysics()->GetAbsBounds(), deathwalkLastOrigin, deathwalkLastBBoxAxis, viewDir, angles );
	Teleport( newOrigin, newAxis, newViewDir, (newAngles.ToMat3() * deathwalkLastEyeAxis.Transpose()).ToAngles(), NULL );
}
//HUMANHEAD END

//=============================================================================
//
// hhPlayer::EnableEthereal
//
// Sets the player into an ethereal state (used for both spiritwalk and deathwalk)
// This function does the following:
//	- Fade volume down on all class 0 sounds
//	- Sets bSpiritWalk (since this is true for both spirit and death walks)
//	- Spawns a proxy
//  - Sets the new collision
//  - Disables the weapons (the bow is enabled seperately for spiritwalk)
//  - Disables the lighter
//  - Unpossesses the player (if possessed)
//	- Updates bindings
//=============================================================================

void hhPlayer::EnableEthereal( const char *proxyName, const idVec3& origin, const idMat3& bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis ) {
	//rww - don't do anything on the client - DO NOT PUT ANYTHING ABOVE HERE
	if (gameLocal.isClient)	{
		return;
	}

	// Turn on low-pass effect
	if (gameLocal.localClientNum == entityNumber) { //rww - don't do this for anyone but the local client on listen servers
		gameLocal.SpiritWalkSoundMode( true );
	}

	// Spawn proxy Tommy actor
	spiritProxy = hhSpiritProxy::CreateProxy( proxyName, this, origin, bboxAxis, newViewAxis, newViewAngles, newEyeAxis );

	// Change player's collision type (to allow walking through forcefields, etc)
	GetPhysics()->SetClipMask( MASK_SPIRITPLAYER );
	fl.acceptsWounds = false;
	spawnArgs.Set("produces_splats", "0");

	// HUMANHEAD mdl:  Let our enemies know about the spirit proxy
	for ( int i = 0; i < hhMonsterAI::allSimpleMonsters.Num(); i++ ) {
		if ( hhMonsterAI::allSimpleMonsters[i]->GetEnemy() == this ) {
			hhMonsterAI::allSimpleMonsters[i]->ProcessEvent( &MA_EnemyIsSpirit, this, spiritProxy.GetEntity() );
		}
	}

	// If bound to another object (like on a rail ride), swap bindings with the proxy
	if ( GetBindMaster() != NULL ) {
		if (GetBindMaster()->IsType(hhBindController::Type)) {
			hhBindController *binder = static_cast<hhBindController *>(GetBindMaster());
			bool loose = binder->IsLoose();
			binder->Detach();
			binder->Attach( spiritProxy.GetEntity(), loose );
		} else {
			gameLocal.Warning("Handle this: spiritwalking while bound to non-rail");
			SwapBindInfo( spiritProxy.GetEntity() );	// untested
		}
	}

	SnapDownCurrentWeapon();

	LighterOff();

	// Set the player into spiritwalk mode
	bSpiritWalk = true;
}

//=============================================================================
//
// hhPlayer::DisableEthereal
//
// Returns the player from an ethereal state (used for both spiritwalk and deathwalk)
// This function does the following:
//	- Clears bSpiritWalk (since this is true for both spirit and death walks)
//	- Removes the proxy
//  - Sets the new collision
//  - Re-enables the weapons
//	- Updates bindings
//	- Fade volume back up on all class 0 sounds
//=============================================================================

void hhPlayer::DisableEthereal( void ) {
	hhFxInfo	fxInfo;
	idVec3		boneOffset;
	idMat3		boneAxis;

	//rww - don't do anything on the client - DO NOT PUT ANYTHING ABOVE HERE
	if (gameLocal.isClient) {
		return;
	}

	bSpiritWalk = false;

	// Spawn in an effect when the spirit is snapped back
	GetJointWorldTransform( "waist", boneOffset, boneAxis );	
	fxInfo.RemoveWhenDone( true );
	fxInfo.SetNormal( boneAxis[1] );
	BroadcastFxInfo( spawnArgs.GetString( "fx_spiritReturn" ), boneOffset, GetAxis(), &fxInfo );

	if( spiritProxy.IsValid() ) {
		// Handle case of returning to a body on a rail ride
		idEntity *spiritMaster = spiritProxy->GetBindMaster();
		if (spiritMaster != NULL) {
			if (spiritMaster->IsType(hhBindController::Type)) {
				hhBindController *binder = static_cast<hhBindController *>(spiritMaster);
				bool loose = binder->IsLoose();
				binder->Detach();
				spiritProxy->DeactivateProxy();
				binder->Attach(this, loose);
			} else if (spiritMaster->IsType(hhMonsterAI::Type)) {
				if (reinterpret_cast<hhMonsterAI *> (spiritMaster)->GetEnemy() != static_cast<idActor *> (spiritProxy.GetEntity())) {
					gameLocal.Warning("Monster has spirit proxy bound to it but spirit proxy is not it's enemy!\n");
				}
				spiritProxy->DeactivateProxy();
			} else {
				gameLocal.Warning("Handle this: spiritwalking while bound to non-rail");
				idEntity *temp = spiritProxy.GetEntity();	// this removed by DeactivateProxy
				spiritProxy->DeactivateProxy();
				SwapBindInfo(temp);	// untested
			}
		}
		else {	// Normal case
			spiritProxy->DeactivateProxy();
		}
	}

	PutawayEtherealWeapon();

	GetPhysics()->SetClipMask( MASK_PLAYERSOLID );
	fl.acceptsWounds = true;
	spawnArgs.Set("produces_splats", "1");

	// Turn off low-pass effect
	if (gameLocal.localClientNum == entityNumber) { //rww - don't do this for anyone but the local client on listen servers
		gameLocal.SpiritWalkSoundMode( false );
	}

	// HUMANHEAD mdl:  Let our enemies know the spirit proxy is going away
	for ( int i = 0; i < hhMonsterAI::allSimpleMonsters.Num(); i++ ) {
		if ( hhMonsterAI::allSimpleMonsters[i]->GetEnemy() == spiritProxy.GetEntity() ) {
			hhMonsterAI::allSimpleMonsters[i]->ProcessEvent( &MA_EnemyIsPhysical, this, spiritProxy.GetEntity() );
		} else if ( hhMonsterAI::allSimpleMonsters[i]->GetEnemy() == this ) { // Targetting spirit that is going away
			hhMonsterAI::allSimpleMonsters[i]->ProcessEvent( &MA_EnemyIsPhysical, this, NULL );
		}
	}

	//HUMANHEAD rww - clear the focus now that we've changed positions by switching out of spirit mode
	focusTime = 0;
	UpdateFocus();
	//HUMANHEAD END

	spiritProxy = NULL;
}

//=============================================================================
//
// hhPlayer::GetResurrectionPoint
//
// Get the position/orientation for resurrection after deathwalk
//=============================================================================
void hhPlayer::GetResurrectionPoint( idVec3& origin, idMat3& axis, idVec3& viewDir, idAngles& angles, idMat3& eyeAxis, const idBounds& absBounds, const idVec3& defaultOrigin, const idMat3& defaultAxis, const idVec3& defaultViewDir, const idAngles& defaultAngles ) {
	idClipModel *clipModelList[ MAX_GENTITIES ];
	idClipModel* clipModel = NULL;
	idEntity*	entity = NULL;
	hhSafeResurrectionVolume* volume = NULL;

	int num = gameLocal.clip.ClipModelsTouchingBounds( absBounds, CONTENTS_DEATHVOLUME, clipModelList, MAX_GENTITIES );
	for( int ix = 0; ix < num; ++ix ) {
		clipModel = clipModelList[ix];

		if( !clipModel ) {
			continue;
		}

		entity = clipModel->GetEntity();
		if( !entity || !entity->IsType(hhSafeResurrectionVolume::Type) ) {
			continue;
		}

		volume = static_cast<hhSafeResurrectionVolume*>( entity );
		volume->PickRandomPoint( origin, axis );
		viewDir = axis[0];
		angles = axis.ToAngles();
		eyeAxis = mat3_identity;
		return;
	}

	eyeAxis = GetEyeAxis();
	origin = defaultOrigin;
	axis = defaultAxis;
	viewDir = defaultViewDir;
	angles = defaultAngles;

//	gameLocal.Printf("Deathwalk @:\n");
//	gameLocal.Printf("  origin:		%s\n", origin.ToString(0));
//	gameLocal.Printf("  axis:		%s\n", axis.ToString(2));
//	gameLocal.Printf("  viewDir:	%s\n", viewDir.ToString(2));
//	gameLocal.Printf("  angles:		%s\n", angles.ToString(0));
}


//=============================================================================
//
//	hhPlayer::SquishedByDoor
//
//=============================================================================
void hhPlayer::SquishedByDoor(idEntity *door) {
	if ( door == this ) { // Don't allow the squished code to send a player back it the squisher is the player itself
		return;
	}

	// If there is a spirit player in the door, make him go back to physical so he doesn't become stuck
	if (IsSpiritWalking()) {
		StopSpiritWalk();
	}
}

//=============================================================================
//
// hhPlayer::UpdatePossession
//
// Updates the possession effects (if the player is possessed)
//
// HUMANHEAD cjr
//=============================================================================

void hhPlayer::UpdatePossession( void ) {

	if ( !IsPossessed() ) { // Player is not possessed
		return;
	}

/* todo:
	possessionTimer -= MS2SEC( USERCMD_MSEC );
		
	// Update a shaderparm for the view screen to update based upon possessionTimer
	possessionMax = spawnArgs.GetFloat( "possessionTime", "8" );
	possessionScale = possessionTimer / possessionMax;
	playerView.SetViewOverlayColor( idVec4( 1.0f, 1.0f, 1.0f, possessionScale ) );

	// Mess with the player's FOV while they are being possessed
	possessionFOV = g_fov.GetFloat() + (1.0f - possessionScale) * 60 + 1.5f * sin( MS2SEC(gameLocal.time) * 3 );

	if ( possessionTimer <= 0 ) { // The player is now fully possessed, kill them
		// TODO:  Extra code to show the possessed player running around and such?
		Unpossess();
		Damage( NULL, NULL, vec3_origin, "damage_crush", 1.0f, INVALID_JOINT );
	}
*/
}

//=============================================================================
//
// hhPlayer::RechargeHealth
//
// Recharges the player's health to 25%
//
// This is done to avoid gameplay issues where the player has only a few points
// of health.  The game will still be tense if the player is at 25%, but
// will play more fairly.
//
// HUMANHEAD cjr
//=============================================================================

void hhPlayer::Event_RechargeHealth( void ) {

	if ( health > 0 && health < spawnArgs.GetInt( "healthRecharge", "25" ) && !IsSpiritOrDeathwalking() ) { // Only recharge if the player is alive and in the critical zone		
		if ( gameLocal.time - lastDamagedTime >= spawnArgs.GetInt( "healthRechargeDelay", "1500" ) ) { // Delay before recharging
			health++;
		}
	}

	PostEventSec( &EV_RechargeHealth, spawnArgs.GetFloat( "healthRechargeRate", "0.5" ) );
}

//=============================================================================
//
// hhPlayer::RechargeRifleAmmo
//
// Recharges rifle ammo if the player is very low.  This is done to guarantee
// that the player has at least some ammo to solve puzzles that require shooting
// something such as a gravity switch.
//
// HUMANHEAD cjr
//=============================================================================

void hhPlayer::Event_RechargeRifleAmmo( void ) {

	int ammoIndex = inventory.AmmoIndexForAmmoClass( "ammo_rifle" );
	int ammoCount = inventory.ammo[ ammoIndex ];

	int maxAmmo = spawnArgs.GetInt( "rifleAmmoRechargeMax", "20" );
	if ( ammoCount < maxAmmo && !AI_ATTACK_HELD ) { // CJR PCF 04/26/06
		inventory.ammo[ ammoIndex ] += 2;
		if ( inventory.ammo[ ammoIndex ] > maxAmmo ) {
			inventory.ammo[ ammoIndex ] = maxAmmo;
		}
	}

	PostEventSec( &EV_RechargeRifleAmmo, spawnArgs.GetFloat( "rifleAmmoRechargeRate", "2" ) );
}

//=============================================================================
//
// hhPlayer::Event_DrainSpiritPower
//
// HUMANHEAD pdm: 10 Hz timer used to drain spirit power at differing rates
//=============================================================================
void hhPlayer::Event_DrainSpiritPower() {

	// JRM: God mode, don't drain! 
	if(godmode) {
		return;
	}

	ammo_t ammo_spiritpower = idWeapon::GetAmmoNumForName( "ammo_spiritpower" );

	if ( gameLocal.isMultiplayer ) { // CJR:  Drain spirit power in MP
		if ( !UseAmmo( ammo_spiritpower, 1 ) ) {
			StopSpiritWalk();
			return;
		}
	}

	// spirit bow alt mode drain
	if( weapon.IsValid() && weapon->IsType(hhWeaponSpiritBow::Type) ) {
		hhWeaponSpiritBow *bow = static_cast<hhWeaponSpiritBow *>( weapon.GetEntity() );
		if( bow->BowVisionIsEnabled() ) {
			if( !UseAmmo(ammo_spiritpower, 1) ) { // Bow vision drains spirit power
				return;
			}
		}
	}

	PostEventMS( &EV_DrainSpiritPower, spiritDrainHeartbeatMS );
}

//=============================================================================
//
// hhPlayer::DeathWalk
//
//=============================================================================

void hhPlayer::DeathWalk( const idVec3& resurrectOrigin, const idMat3& resurrectBBoxAxis, const idMat3& resurrectViewAxis, const idAngles& resurrectViewAngles, const idMat3& resurrectEyeAxis ) {
	if ( IsSpiritWalking() ) { // Ensure that two proxies cannot be active at the same time
		StopSpiritWalk();
	}

	bDeathWalk = true;
	deathWalkTime = gameLocal.time; // Get the time of death

	health = 0; // Force health to zero

	deathWalkPower = spawnArgs.GetInt( "deathWalkPowerStart" ); // Power in deathwalk.  When full, the player will return
	fl.takedamage = false; // can no longer be damaged in deathwalk mode

	if (hud) {
		hud->HandleNamedEvent("SwitchToEthereal");
	}

	// Disable clip until we get to deathwalk
	bInDeathwalkTransition = true;

	//rww - for deathwalk, let's store these values on the player instead of relying on the death proxy
	deathwalkLastOrigin = resurrectOrigin;
	deathwalkLastBBoxAxis = resurrectBBoxAxis;
	deathwalkLastViewAxis = resurrectViewAxis;
	deathwalkLastViewAngles = resurrectViewAngles;
	deathwalkLastEyeAxis = resurrectEyeAxis;
	deathwalkLastCrouching = IsCrouching();

	EnableEthereal( spawnArgs.GetString("def_deathProxy"), resurrectOrigin, resurrectBBoxAxis, resurrectViewAxis, resurrectViewAngles, resurrectEyeAxis );
	// Set the overlay material for DeathWalk
	playerView.SetViewOverlayColor( colorWhite ); // Guarantee that the color is reset
	PostEventMS( &EV_SetOverlayMaterial, 1, spawnArgs.GetString("mtr_deathWalk"), false );

	//Don't allow weapon use for a bit
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->PutAway();
	}

	// Alert the scripts that death has occured so they can manage music
	idEntity *dwJustDied = gameLocal.FindEntity("dw_justdied");
	if (dwJustDied) {
		dwJustDied->PostEventMS(&EV_Activate, 0, this);
	}

	deathwalkSoundController.StartSound( SND_CHANNEL_BODY, SND_CHANNEL_BODY2 );

	// Flash the screen before entering death world
	playerView.SetViewOverlayColor( idVec4( 1.0f, 1.0f, 1.0f, 0.0f ) );
	deathWalkFlash = 0.0f;
	possessionFOV = 90.0f;

	CancelEvents( &EV_RechargeHealth );
	CancelEvents( &EV_RechargeRifleAmmo );

	// If this is our first time, tell the script
	if (!gameLocal.IsCompetitive() && !inventory.bHasDeathwalked) {
		const function_t *firstFunc = gameLocal.program.FindFunction( "map_deathwalk::FirstDeathwalk" );
		if ( firstFunc ) {
			idThread *thread = new idThread();
			thread->CallFunction( firstFunc, false );
			thread->DelayedStart( 0 );
		}
	}

	//rww - if there is a deathwalk portal, let's create a target for it and activate it
	if (!gameLocal.FindEntity("dw_deathLocation")) { //check to see if we've made ourselves a target yet
		idEntity *dwPortal = gameLocal.FindEntity( "dw_deathPortal" );
		if (dwPortal) {
			trace_t tr;
			idVec3 camPos, camDir;
			idAngles camAngles;
			idDict args;

			//FIXME more complex logic that allows angles to change based on collision circumstances
			//do wider trace that takes the actual view frustum of camera into account
			//check for collisions with the actual view "plane"
			camPos = resurrectOrigin;
			camPos += resurrectBBoxAxis[2] * 64.0f;
			gameLocal.clip.TracePoint(tr, resurrectOrigin, camPos, CONTENTS_SOLID, this);
			camDir = resurrectOrigin-camPos;
			camAngles = camDir.ToAngles();

			args.Clear();
			args.SetVector("origin", tr.endpos);
			args.SetAngles("angles", camAngles);
			args.SetMatrix("rotation", camAngles.ToMat3());
			args.Set("name", "dw_deathLocation");
			gameLocal.SpawnObject("info_null", &args);

			dwPortal->ProcessEvent(&EV_UpdateCameraTarget); //update for the newly placed cameraTarget
			dwPortal->PostEventMS(&EV_Activate, 0, this);
			if (dwPortal->cameraTarget) { //if camera target was legitimate, force an update to the remoteRenderView now
				dwPortal->GetRenderEntity()->remoteRenderView = dwPortal->cameraTarget->GetRenderView();
			}
		}
	}

	Hide(); // Don't show the player when they are in the limbo state between alive and dead
	PostEventSec( &EV_PrepareForDeathWorld, spawnArgs.GetFloat( "prepareForDeathWorldDelay", "1.5" ) );
}

//=============================================================================
//
// hhPlayer::Event_PrepareForDeathWorld
//
// Flashes the screen / fades the FOV
//=============================================================================

void hhPlayer::Event_PrepareForDeathWorld() {	
	deathWalkFlash += spawnArgs.GetFloat( "deathWalkFlashChange", "0.05" );
	if ( deathWalkFlash >= 1.0f ) {
		deathWalkFlash = 1.0f;
		PostEventSec( &EV_EnterDeathWorld, 0.0f ); // Actually send player into the deathworld
	} else {
		PostEventSec( &EV_PrepareForDeathWorld, 0.02f );
	}

	possessionFOV += spawnArgs.GetFloat( "deathWalkFOVChange", "3.0" );
	playerView.SetViewOverlayColor( idVec4( 1.0f, 1.0f, 1.0f, deathWalkFlash ) );
}

//=============================================================================
//
// hhPlayer::Event_EnterDeathWorld
//
// - Teleports the player to the death world
// - Spawns the death wraiths
// - Gives the player a weapon
//=============================================================================

void hhPlayer::Event_EnterDeathWorld() {
	// Reset player view information
	playerView.SetViewOverlayColor( idVec4( 1.0f, 1.0f, 1.0f, 0.0f ) ); // Turn off the flash
	possessionFOV = 0.0f;
	bDeathWalkStage2 = false;

	// Spawn in DeathWraiths after a short period of time
	if ( !inventory.bHasDeathwalked ) { // Delay the wraiths the first time the player deathwalks
		int firstDelay = spawnArgs.GetInt( "deathWalk_firstTimeDelay", "15000" );
		PostEventMS( &EV_SpawnDeathWraith, firstDelay ); // CJR:  delay the deathwraiths for several seconds while the Grandfather DW speech is going on
		PostEventMS( &EV_AdjustSpiritPowerDeathWalk, firstDelay + 2000 );
		inventory.bHasDeathwalked = true;
	} else {
		PostEventMS( &EV_SpawnDeathWraith, 0 ); // CJR:  Non-first time, spawn the wraiths instantly
		PostEventMS( &EV_AdjustSpiritPowerDeathWalk, 2000 );
	}

	// Give the player the spirit bow
	SelectEtherealWeapon();

	// get the entity which tells us where we want to place our dw proxy
	idEntity *dwProxyPosEnt = gameLocal.FindEntity( "dw_floatingBodyMarker" );
	if (dwProxyPosEnt) {
		//if we found that, then this is the new deathwalk, so create the deathwalk proxy.
		hhDeathWalkProxy *dwProxy = (hhDeathWalkProxy *)gameLocal.SpawnObject( spawnArgs.GetString("def_deathWalkProxy"), NULL );
		if (!dwProxy) {
			gameLocal.Error("hhPlayer::Event_EnterDeathWorld: Could not create hhDeathWalkProxy");
			return;
		}
		idAngles dwProxyAngles;
		dwProxyPosEnt->spawnArgs.GetAngles("angles", "0 0 0", dwProxyAngles);
		dwProxy->ActivateProxy(this, dwProxyPosEnt->GetOrigin(), dwProxyAngles.ToMat3(), dwProxyAngles.ToMat3(), dwProxyAngles, GetEyeAxis());
	}

	idEntity* deathWalkPlayerStartManager = gameLocal.FindEntity( "dw_deathWalkPlayerStartManager" );
	if( !deathWalkPlayerStartManager ) {
		return;
	}

	idEntity* deathWalkStart = deathWalkPlayerStartManager->PickRandomTarget();
	if( deathWalkStart ) {
		deathWalkStart->ProcessEvent( &EV_Activate, this );
	}

	// Trigger the 'DeathWalkEntered' entity
	// This allows the scripters to do something upon entering deathwalk
	idEntity *dwe = gameLocal.FindEntity("dw_DeathWalkEntered");
	if (dwe) {
		dwe->PostEventMS(&EV_Activate, 0, this);
	}

	// Allow collisions again now that we're in the death world
	bInDeathwalkTransition = false;
	Show(); // The player is visible in deathmode

	// Reset the player's health and spirit power
	ammo_t ammo_spiritpower = idWeapon::GetAmmoNumForName( "ammo_spiritpower" );
	inventory.ammo[ ammo_spiritpower ] = 0;
	SetHealth( hhMath::hhMax( health, spawnArgs.GetInt("minResurrectHealth", "50") ) );
}

//=============================================================================
//
// hhPlayer::Event_AdjustSpiritPowerDeathWalk
//
//=============================================================================
void hhPlayer::Event_AdjustSpiritPowerDeathWalk() {
	if( !IsDeathWalking() ) {
		return;
	}

	// Increase deathwalk power
	int power = GetDeathWalkPower();
	power += spawnArgs.GetInt( "deathWraithPowerAmount" );
	SetDeathWalkPower( power );

	// Check if deathWalkPower has exceeded the maximum
	if ( deathWalkPower >= spawnArgs.GetInt( "deathWalkPowerMax", "1000") ) {
		DeathWalkSuccess();
		return;
	}

	CancelEvents( &EV_AdjustSpiritPowerDeathWalk );
	PostEventSec( &EV_AdjustSpiritPowerDeathWalk, spawnArgs.GetFloat("deathWalkDeathPowerIncreaseRate") );
}

//=============================================================================
//
// hhPlayer::DeathWalkSuccess
//
// Called when deathwalk timer runs out or we run out of spirit power
//=============================================================================
void hhPlayer::DeathWalkSuccess() {
	CancelEvents( &EV_AdjustSpiritPowerDeathWalk );
	bDeathWalkStage2 = true;

	idEntity *dw_deathWalkResult = gameLocal.FindEntity("dw_deathWalkSuccess");
	if (dw_deathWalkResult) { //if we have a result, activate it.
		dw_deathWalkResult->ProcessEvent(&EV_Activate, this);
	}
}

//=============================================================================
//
// hhPlayer::ReallyKilled
//
// Called when the player should be truly killed
// Funnels through Killed(), but disabled the deathwalk functionality
//=============================================================================

void hhPlayer::ReallyKilled( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( !IsSpiritOrDeathwalking() ) {
		return;
	}

	bDeathWalk = false;
	bSpiritWalk = false;

	bReallyDead = true;

	// Turn off low-pass effect
	if (!gameLocal.isClient) { //rww
		if (gameLocal.localClientNum == entityNumber) { //rww - don't do this for anyone but the local client on listen servers
			gameLocal.SpiritWalkSoundMode( false );
			gameLocal.DialogSoundMode( false );
		}
	}

	if (hud) {
		hud->HandleNamedEvent("SwitchFromEthereal");
	}

	Killed( inflictor, attacker, damage, dir, location );
}

//=============================================================================
//
// hhPlayer::Event_SpawnDeathWraith
//
// Spawns in a special type of wraith that is only visible in deathwalk
//=============================================================================

void hhPlayer::Event_SpawnDeathWraith() {
	idDict		args;
	idEntity	*ent;
	int			maxDeathWalkWraiths = spawnArgs.GetInt( "deathWalkMaxWraiths" );

	if ( !IsDeathWalking() ) { // Don't spawn in a wraith if the player has resurrected
		return;
	}

	float angle = hhMath::PI * gameLocal.random.RandomFloat();
	idVec3 startPoint = idVec3( hhMath::Sin(angle) * 600.0f, hhMath::Cos(angle) * 600.0f, 0.0f ) - GetAxis()[2] * 100.0f;

	args.Clear();
	args.SetVector( "origin", GetEyePosition() + startPoint );
	args.SetMatrix( "rotation", idAngles( 0.0f, (-viewAxis[0]).ToYaw(), 0.0f ).ToMat3() );
	ent = gameLocal.SpawnObject( spawnArgs.GetString("def_deathWraith"), &args );
	if ( ent ) {
		hhDeathWraith *wraith = static_cast< hhDeathWraith * > ( ent );
		wraith->SetEnemy( this );
	}

	// Check the number of deathwraiths in the world
	int count = 0;
	for( ent = gameLocal.activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
		if ( ent && ent->IsType( hhDeathWraith::Type ) ) {
			count++;			
		}
	}

	if ( count < maxDeathWalkWraiths ) {
		PostEventMS( &EV_SpawnDeathWraith, 0 );
	}
}

//=============================================================================
//
// hhPlayer::GetDeathwalkEnergyDestination
//
//=============================================================================
idEntity *hhPlayer::GetDeathwalkEnergyDestination() {
	return gameLocal.FindEntityOfType(hhDeathWalkProxy::Type, NULL);
}

//=============================================================================
//
// hhPlayer::DeathWalkDamagedByWraith
//
//=============================================================================
void hhPlayer::DeathWalkDamagedByWraith(idEntity *attacker, const char *damageType) {
	Damage( attacker, attacker, vec3_origin, damageType, spawnArgs.GetFloat( "deathWalkWraithDamage", "10" ), INVALID_JOINT );

	CancelEvents( &EV_SpawnDeathWraith );
	PostEventMS( &EV_SpawnDeathWraith, 0 );
}

//=============================================================================
//
// hhPlayer::KilledDeathWraith
//
//=============================================================================

void hhPlayer::KilledDeathWraith( void ) {
	CancelEvents( &EV_SpawnDeathWraith );
	PostEventMS( &EV_SpawnDeathWraith, 0 );
}

//=============================================================================
//
// hhPlayer::DeathWraithEnergyArived
//
//=============================================================================

void hhPlayer::DeathWraithEnergyArived(bool energyHealth) {
	if (energyHealth) {
		const char *healthAmount = spawnArgs.GetString("deathWraithHealthAmount");
		Give( "health", healthAmount );
	}
	else {
		Give( "ammo_spiritpower", spawnArgs.GetString( "deathWraithSpiritAmount" ) );
	}

	// Bump the deathwalk power up a bit so the player lowers faster
	int power = GetDeathWalkPower();
	power += spawnArgs.GetInt( "deathWalkPowerEnergyBoost", "100" );
	SetDeathWalkPower( power );
}

//=============================================================================
//
// hhPlayer::Resurrect
//
//=============================================================================

void hhPlayer::Resurrect( void ) {

	bDeathWalk = false;

	possessionFOV = 0.0f;

	PostEventSec( &EV_AllowDamage, 3 ); // Re-enable damage in 3 seconds
	fl.noknockback = false;	// Restore knockback ability

	// Cancel the impending translation to deathworld (only happens if the player resurrect cheats early)
	CancelEvents( &EV_PrepareForDeathWorld );
	CancelEvents( &EV_EnterDeathWorld );
	CancelEvents( &EV_SpawnDeathWraith );

	DisableEthereal();

	// Reset the view overlay
	playerView.SetViewOverlayTime( 0, true );
	playerView.SetViewOverlayColor( idVec4( 1.0f, 1.0f, 1.0f, 0.0f ) );

	//rww - deathwalk position is restored from the player now.
	RestorePlayerLocationFromDeathwalk(deathwalkLastOrigin, deathwalkLastBBoxAxis, deathwalkLastViewAxis[0], deathwalkLastViewAngles);

	deathwalkSoundController.StopSound( SND_CHANNEL_BODY, SND_CHANNEL_BODY2 );

	ProcessEvent( &EV_RechargeHealth );
	ProcessEvent( &EV_RechargeRifleAmmo );

	if (hud) {
		hud->HandleNamedEvent("SwitchFromEthereal");
	}

	// TODO:  Build in an autoimmune time?
	lastResurrectTime = gameLocal.GetTime();

	// Radius blast (doesn't damage enemies / objects, only shoves them back)
	gameLocal.RadiusDamage( GetPhysics()->GetOrigin(), this, this, this, this, spawnArgs.GetString("def_resurrect_damage") );	

	Show(); // Ensure that the player is visible when they resurrect

	// trigger the 'DeathWalkExited' entity
	idEntity *dwe = gameLocal.FindEntity("dw_DeathWalkExited");
	if (dwe) {
		dwe->PostEventMS(&EV_Activate, 0, this);
	}

	// bg: Visual effect for returning from DeathWalk uses landing camera movement.
	landChange = -10;
	landTime = gameLocal.time;

	//rww - deactivate dw portal and remove cam target
	idEntity *dwPortal = gameLocal.FindEntity( "dw_deathPortal" );
	if (dwPortal && dwPortal->IsActive()) {
		dwPortal->cameraTarget = NULL; //reset the camera target
		dwPortal->PostEventMS(&EV_Activate, 0, dwPortal);
	}
	idEntity *dwCamTarget = gameLocal.FindEntity( "dw_deathLocation" );
	if (dwCamTarget) {
		dwCamTarget->PostEventMS(&EV_Remove, 50); //make sure this is removed after the portal has stopped.
	}
}

/*
============
hhPlayer::IsWallWalking
============
*/
bool hhPlayer::IsWallWalking( void ) const {
	return( physicsObj.IsWallWalking() );
}

/*
============
hhPlayer::WasWallWalking
============
*/
bool hhPlayer::WasWallWalking( void ) const {
	return( physicsObj.WasWallWalking() );
}

/*
============
hhPlayer::MangleControls
============
*/
void hhPlayer::MangleControls( usercmd_t *cmd ) {
	// When frozen in a cinematic, we want to restrict any movement and disallow many features
	if (guiWantsControls.IsValid()) {
		if (cmd->buttons & BUTTON_ATTACK_ALT) {
			guiWantsControls->LockedGuiReleased(this);
			guiWantsControls = NULL;	// release
		}
		else {
			guiWantsControls->PlayerControls(cmd);
		}
		cmd->forwardmove = 0;
		cmd->rightmove = 0;
		cmd->upmove = 0;
		cmd->buttons &= ~(BUTTON_ATTACK|BUTTON_ZOOM|BUTTON_ATTACK_ALT);
		cmd->impulse = 0;
	}
	else if (InCinematic()) {
		cmd->forwardmove = 0;
		cmd->rightmove = 0;
		cmd->upmove = 0;
		cmd->buttons &= ~(BUTTON_ATTACK|BUTTON_ZOOM|BUTTON_ATTACK_ALT);
		cmd->impulse = 0;
	}
}

/*
============
hhPlayer::GetPilotInput

Called from PilotVehicleInterface
============
*/
void hhPlayer::GetPilotInput( usercmd_t& pilotCmds, idAngles& pilotViewAngles ) {
	pilotCmds = gameLocal.usercmds[ entityNumber ];
	pilotViewAngles = DetermineViewAngles( pilotCmds, cmdAngles );
}

/*
==============
hhPlayer::Think
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
==============
*/
void hhPlayer::Think( void ) {
	renderEntity_t *headRenderEnt;

	UpdatePossession();

	UpdatePlayerIcons();

	// latch button actions
	oldButtons = usercmd.buttons;

	// grab out usercmd
	usercmd_t oldCmd = usercmd;
	usercmd = gameLocal.usercmds[ entityNumber ];
	buttonMask &= usercmd.buttons;
	usercmd.buttons &= ~buttonMask;

	if ( gameLocal.inCinematic && gameLocal.skipCinematic ) {
		return;
	}

	//HUMANHEAD rww - keep air attacker timer refreshed
	if (!AI_ONGROUND && airAttackerTime > gameLocal.time) {
		airAttackerTime = gameLocal.time + 300;
	}

	// HUMANHEAD pdm: Depending on our mode, might want to modify the input
	MangleControls( &usercmd );
	// HUMANHEAD END

	// clear the ik before we do anything else so the skeleton doesn't get updated twice
	walkIK.ClearJointMods();

	// if this is the very first frame of the map, set the delta view angles
	// based on the usercmd angles
	if ( !spawnAnglesSet && ( gameLocal.GameState() != GAMESTATE_STARTUP ) ) {
		spawnAnglesSet = true;
		SetViewAngles( spawnAngles );
		oldFlags = usercmd.flags;
	}

	if ( gameLocal.inCinematic || influenceActive ) {
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
	}

	// log movement changes for weapon bobbing effects
	if ( usercmd.forwardmove != oldCmd.forwardmove ) {
		loggedAccel_t	*acc = &loggedAccel[currentLoggedAccel&(NUM_LOGGED_ACCELS-1)];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[0] = usercmd.forwardmove - oldCmd.forwardmove;
		acc->dir[1] = acc->dir[2] = 0;
	}

	if ( usercmd.rightmove != oldCmd.rightmove ) {
		loggedAccel_t	*acc = &loggedAccel[currentLoggedAccel&(NUM_LOGGED_ACCELS-1)];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[1] = usercmd.rightmove - oldCmd.rightmove;
		acc->dir[0] = acc->dir[2] = 0;
	}

	// freelook centering
	if ( ( usercmd.buttons ^ oldCmd.buttons ) & BUTTON_MLOOK ) {
		centerView.Init( gameLocal.time, 200, viewAngles.pitch, 0 );
	}

	// zooming
	if ( ( usercmd.buttons ^ oldCmd.buttons ) & BUTTON_ZOOM ) {
		if ( ( usercmd.buttons & BUTTON_ZOOM ) && weapon.GetEntity() ) {
			zoomFov.Init( gameLocal.time, 200.0f, CalcFov( false ), weapon.GetEntity()->GetZoomFov() );
		} else {
			zoomFov.Init( gameLocal.time, 200.0f, zoomFov.GetCurrentValue( gameLocal.time ), DefaultFov() );
		}
	} 

	if ( g_fov.IsModified() ) {
		idEntity *weaponEnt = weapon.GetEntity();
		if ( ! ( weaponEnt &&
				 weaponEnt->IsType( hhWeaponZoomable::Type ) &&
				 reinterpret_cast<hhWeaponZoomable *> (weaponEnt)->IsZoomed() ) )
		{
			GetZoomFov().Init( gameLocal.GetTime(), 0.0f, CalcFov(true), g_fov.GetInteger() );
			g_fov.ClearModified();
		}
	}

	// if we have an active gui, we will unrotate the view angles as
	// we turn the mouse movements into gui events
	idUserInterface *gui = ActiveGui();
	if ( gui && gui != focusUI ) {
		RouteGuiMouse( gui );
	}

	// set the push velocity on the weapon before running the physics
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->SetPushVelocity( physicsObj.GetPushedLinearVelocity() );
	}

	EvaluateControls();

	if ( !af.IsActive() ) {
		AdjustBodyAngles();
		CopyJointsFromBodyToHead();
	}

	//HUMANHEAD: aob - added vehicle check.  Vehicle moves us
	if( !InVehicle() ) {
		Move();
	}
	//HUMANHEAD END

	if ( !g_stopTime.GetBool() ) {
		//HUMANHEAD: aob - changed heath check to IsDead check.  Player needs to touch triggers when deathwalking.
		if ( !noclip && !spectating && !IsDead() && !IsHidden() ) {
			TouchTriggers();
		}

		UpdateLighter();

		// update GUIs, Items, and character interactions
		UpdateFocus();
	
		UpdateLocation();

		// update player script
		UpdateScript();

		// service animations
		if ( !spectating && !af.IsActive() && !gameLocal.inCinematic ) {
    		UpdateConditions();
			UpdateAnimState();
			CheckBlink();
		}
	
		// clear out our pain flag so we can tell if we recieve any damage between now and the next time we think
		AI_PAIN = false;
	}

	// calculate the exact bobbed view position, which is used to
	// position the view weapon, among other things
	CalculateFirstPersonView();

	// this may use firstPersonView, or a thirdPeroson / camera view
	CalculateRenderView();

	if ( spectating ) {
		if (!gameLocal.isClient) {
			UpdateSpectating();
		}
	//HUMANHEAD: aob - changed heath check to IsDead check.  Player needs to update weapon when deathwalking
	} else if ( !IsDead() && !bFrozen ) {	//HUMANHEAD bjk PCF (4-27-06) - no setting unnecessary weapon state
		if ( !gameLocal.isClient || weapon.GetEntity()) {
			UpdateWeapon();
		}
	}

	if (InVehicle()) {
		UpdateHud( GetVehicleInterfaceLocal()->GetHUD() );
	}
	else {
		UpdateHud( hud );
	}

	UpdateDeathSkin( false );

	if ( gameLocal.isMultiplayer ) {
		DrawPlayerIcons();
	}

	if ( head.GetEntity() ) {
		headRenderEnt = head.GetEntity()->GetRenderEntity();
	} else {
		headRenderEnt = NULL;
	}

	if ( gameLocal.isMultiplayer || g_showPlayerShadow.GetBool() ) {
		renderEntity.suppressShadowInViewID	= 0;
		if ( headRenderEnt ) {
			headRenderEnt->suppressShadowInViewID = 0;
		}
	} else {
		renderEntity.suppressShadowInViewID	= entityNumber+1;
		if ( headRenderEnt ) {
			headRenderEnt->suppressShadowInViewID = entityNumber+1;
		}
	}
	// never cast shadows from our first-person muzzle flashes
	renderEntity.suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	if ( headRenderEnt ) {
		headRenderEnt->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	}

	if ( !g_stopTime.GetBool() ) {
		UpdateAnimation();
		Present();
		UpdateDamageEffects();
		if (gameLocal.isMultiplayer) { //rww
			UpdateWounds();
		}
		LinkCombat();
		playerView.CalculateShake();
	}

	if ( g_showEnemies.GetBool() ) {
		idActor *ent;
		int num = 0;
		for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
			gameLocal.Printf( "enemy (%d)'%s'\n", ent->entityNumber, ent->name.c_str() );
			gameRenderWorld->DebugBounds( colorRed, ent->GetPhysics()->GetBounds().Expand( 2 ), ent->GetPhysics()->GetOrigin() );
			num++;
		}
		gameLocal.Printf( "%d: enemies\n", num );
	}
}

/*
==============
hhPlayer::AdjustBodyAngles
==============
*/
void hhPlayer::AdjustBodyAngles( void ) {
	if (InVehicle()) {
		return;
	}

	if (bClampYaw) { //rww - for slabs
		//first, clamp the viewangles while bound to a slab
		if (maxRelativePitch >= 0.0f) {
			if (untransformedViewAngles.pitch > maxRelativePitch) {
				untransformedViewAngles.pitch = maxRelativePitch;
				SetViewAngles(untransformedViewAngles);
			}
			else if (untransformedViewAngles.pitch < -maxRelativePitch) {
				untransformedViewAngles.pitch = -maxRelativePitch;
				SetViewAngles(untransformedViewAngles);
			}
		}

		animator.SetJointAxis( hipJoint, JOINTMOD_WORLD, mat3_identity ); //no leg offset while slabbed

#if 0 //old headlook code
		if (head.IsValid() && head.GetEntity()) { //has a head
			idVec3 origin;
			idMat3 axis;

			head->GetPhysics()->Evaluate(gameLocal.time-gameLocal.previousTime, gameLocal.time); //make sure it is bolted up to date

			if (GetMasterPosition(origin, axis)) {
				idAngles masterAngles = axis.ToAngles(); //bound angles
				idAngles ang;
				idMat3 rot;

				//determine our "look" angles
				ang.yaw = viewAngles.yaw-masterAngles.yaw;
				ang.pitch = viewAngles.pitch;
				ang.roll = 0.0f;

				ang = ang.Normalize180();
				ang *= 0.5f; //scale the angles down so the head doesn't go completely sideways or anything

				//set the yaw only
				animator.SetJointAxis(headJoint, JOINTMOD_WORLD, idAngles(0.0f, ang.yaw, 0.0f).ToMat3());
				ang.yaw = 0.0f;

				hhMath::BuildRotationMatrix(DEG2RAD(-ang.pitch), 0, rot); //create a rotation matrix for the pitch

				//get the local axis to multiply it by the pitch rotation matrix
				animator.GetJointLocalTransform(headJoint, gameLocal.time, origin, axis);

				//use the multiplied axis
				animator.SetJointAxis(headJoint, JOINTMOD_LOCAL, rot*axis); //point head in proper direction
			}
		}
#endif
	}
	else {
#if 0 //old headlook code
		//rww - reset head joint override when not on a slab
		animator.SetJointAxis( headJoint, JOINTMOD_LOCAL, idAngles( 0.0f, 0.0f, 0.0f ).ToMat3() );
#endif
		idPlayer::AdjustBodyAngles();
	}
}

/*
==============
hhPlayer::Move
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
==============
*/
void hhPlayer::Move( void ) {
	float newEyeOffset;
	idVec3 oldOrigin;
	idVec3 oldVelocity;
	idVec3 pushVelocity;

	// save old origin and velocity for crashlanding
	oldOrigin = physicsObj.GetOrigin();
	oldVelocity = physicsObj.GetLinearVelocity();
	pushVelocity = physicsObj.GetPushedLinearVelocity();

	// set physics variables
	physicsObj.SetMaxStepHeight( GetStepHeight() );//HUMANHEAD: aob
	physicsObj.SetMaxJumpHeight( pm_jumpheight.GetFloat() );

	if ( noclip ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_NOCLIP );
	} else if ( spectating ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_SPECTATOR );
	} else if ( bInDeathwalkTransition ) {		// In between life and death, don't allow ragdoll to collide with player
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_NORMAL );
	} else if ( IsDead() ) { 					// HUMANHEAD cjr:  Replaced health <= 0 with IsDead() call for deathwalk override
		if (gameLocal.isMultiplayer) {			// HUMANHEAD rww - contents 0 because we use a dead body proxy in mp
			physicsObj.SetContents(0);
		}
		else {
			physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP );
		} //HUMANHEAD END
		physicsObj.SetMovementType( PM_DEAD );	
	} else if ( gameLocal.inCinematic || gameLocal.GetCamera() || privateCameraView ) {
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_FREEZE );
	} else if ( bFrozen && !IsSpiritOrDeathwalking() ) {		// HUMANHEAD: freeze support
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_FREEZE );
	} else {
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_NORMAL );
	}

	if ( spectating ) {
		physicsObj.SetClipMask( MASK_SPECTATOR );
	} else if ( IsDead() ) {	// HUMANHEAD cjr:  Replaced health <= 0 with IsDead() call for deathwalk override
		physicsObj.SetClipMask( MASK_DEADSOLID );
	// HUMANHEAD cjr: allow spirit walk
	} else if ( IsSpiritOrDeathwalking() ) {
		physicsObj.SetClipMask( MASK_SPIRITPLAYER );
	// HUMANHEAD END
	} else {
		physicsObj.SetClipMask( MASK_PLAYERSOLID );
	}

	physicsObj.SetDebugLevel( g_debugMove.GetBool() );
	physicsObj.SetPlayerInput( usercmd, viewAngles );

	//HUMANHEAD: aob - moved down a few lines
	// FIXME: physics gets disabled somehow
	//BecomeActive( TH_PHYSICS );
	//RunPhysics();
	//HUMANHEAD END

	// update our last valid AAS location for the AI
	SetAASLocation();

	if ( spectating ) {
		newEyeOffset = 0.0f;
	// HUMANHEAD cjr:  Replaced health <= 0 with IsDead() call for deathwalk override
	} else if ( IsDead() ) {
		newEyeOffset = pm_deadviewheight.GetFloat();
	} else if ( physicsObj.IsCrouching() ) {
		newEyeOffset = pm_crouchviewheight.GetFloat();
	} else if ( GetBindMaster() && GetBindMaster()->IsType( idAFEntity_Vehicle::Type ) ) {
		newEyeOffset = 0.0f;
	} else {
		newEyeOffset = pm_normalviewheight.GetFloat();
	}

	if ( EyeHeight() != newEyeOffset ) {
		//AOB: camera does smoothing
		SetEyeHeight( newEyeOffset );
	}

	// HUMANHEAD aob 
	float camRotScale = p_camRotRateScale.GetFloat();
	//rww - if on a wallwalk mover, increase the scale
	if (GetPhysics() == &physicsObj && physicsObj.GetGroundTrace().fraction < 1.0f) {
		const trace_t &grTr = physicsObj.GetGroundTrace();
		if (grTr.c.entityNum >= 0 && grTr.c.entityNum < MAX_GENTITIES && gameLocal.entities[grTr.c.entityNum] && gameLocal.entities[grTr.c.entityNum]->IsType(hhMover::Type) && grTr.c.material && grTr.c.material->GetSurfaceType() == SURFTYPE_WALLWALK) {
			camRotScale *= 5.0f;
		}
	}
	cameraInterpolator.Setup( camRotScale, IT_VariableMidPointSinusoidal );//Also set in constructor

	// moved here to allow weapons physics get correct eyeOffset
	BecomeActive( TH_PHYSICS ); //rww - prevent the bug that id so lovingly left for us
	RunPhysics();
	// HUMANHEAD END

	// HUMANHEAD pdm: Experimental: allow player to jump up onto wallwalk
#define WALLWALK_EXPERIMENT 0
#if WALLWALK_EXPERIMENT
	if (!physicsObj.IsWallWalking() && physicsObj.HasJumped()) {
		trace_t trace;
		memset(&trace, 0, sizeof(trace));
		idVec3 start, end;
		start = physicsObj.GetOrigin();
		end = start + idVec3(0,0,100);	//fixme: jumpheight
		if (gameLocal.clip.TracePoint( trace, start, end, MASK_SOLID, this )) {
			if (gameLocal.GetMatterType(trace, NULL) == SURFTYPE_WALLWALK) {
				gameLocal.Printf("Trying to flip over for wallwalk\n");

				// Clear velocity, so won't miss wallwalk above
				physicsObj.SetLinearVelocity(vec3_origin);

				SetGravity(-trace.c.normal * DEFAULT_GRAVITY);
				ProcessEvent( &EV_ShouldRemainAlignedToAxial, (int)false );
				ProcessEvent( &EV_OrientToGravity, (int)true );

				physicsObj.IsWallWalking( true );

/*				idVec3 flippedOrigin = trace.endpos;
				idMat3 flippedAxis;
				flippedAxis[0] = GetAxis()[0];
				flippedAxis[2] = trace.c.normal;
				flippedAxis[1] = flippedAxis[0].Cross(flippedAxis[2]);

//				cameraInterpolator.SetInterpolationType(flags);
//				cameraInterpolator.SetTargetEyeOffset(idealEyeOffset, flags);
//				cameraInterpolator.SetTargetPosition(idealPosition, flags);

				// Flip the physics object
				physicsObj.SetOrigin(flippedOrigin);
				physicsObj.SetAxis(flippedAxis);
				physicsObj.CheckWallWalk(true);*/
			}
		}
	}
#endif
	// HUMANHEAD END

	if ( noclip || gameLocal.inCinematic ) {
		AI_CROUCH	= false;
		AI_ONGROUND	= false;
		AI_ONLADDER	= false;
		AI_JUMP		= false;
		AI_REALLYFALL = true; //HUMANHEAD rww - well, it fits with ONGROUND false
	} else {
		AI_CROUCH	= physicsObj.IsCrouching();
		AI_ONGROUND	= physicsObj.HasGroundContacts();
		AI_ONLADDER	= physicsObj.OnLadder();
		AI_JUMP		= physicsObj.HasJumped();
		//HUMANHEAD rww
		if (!physicsObj.IsInwardGravity()) {
			AI_REALLYFALL = !AI_ONGROUND;
		}
		else if (!AI_ONGROUND) { //for inward gravity zones, do an extra ground check when in-air
			AI_REALLYFALL = !physicsObj.ExtraGroundCheck();
		}
		//HUMANHEAD END

		// check if we're standing on top of a monster and give a push if we are
		idEntity *groundEnt = physicsObj.GetGroundEntity();
		if ( groundEnt && groundEnt->IsType( idAI::Type ) ) {
			idVec3 vel = physicsObj.GetLinearVelocity();
			if ( vel.ToVec2().LengthSqr() < 0.1f ) {
				vel.ToVec2() = physicsObj.GetOrigin().ToVec2() - groundEnt->GetPhysics()->GetAbsBounds().GetCenter().ToVec2();
				vel.ToVec2().NormalizeFast();
				vel.ToVec2() *= pm_walkspeed.GetFloat();
			} else {
				// give em a push in the direction they're going
				vel *= 1.1f;
			}
			physicsObj.SetLinearVelocity( vel );
		}
	}

	if ( AI_JUMP ) {
		// bounce the view weapon
		loggedAccel_t	*acc = &loggedAccel[currentLoggedAccel&(NUM_LOGGED_ACCELS-1)];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[2] = 200;
		acc->dir[0] = acc->dir[1] = 0;
	}

	BobCycle( pushVelocity );
	if ( !noclip ) { // HUMANHEAD: Only crashland if not spiritwalking or not deathwalking
		CrashLand( oldOrigin, oldVelocity );
	}

	//HUMANHEAD: aob - put this into helper func
	if (!gameLocal.isClient) {
		if( IsWallWalking() && !WasWallWalking() ) {
			wallwalkSoundController.StartSound( SND_CHANNEL_WALLWALK, SND_CHANNEL_WALLWALK2, SSF_LOOPING, false );
		} else if( !IsWallWalking() && WasWallWalking() ) {
			wallwalkSoundController.StopSound( SND_CHANNEL_WALLWALK, SND_CHANNEL_WALLWALK2, false );
		}
	}
	physicsObj.WasWallWalking( IsWallWalking() ); //rww - moved here
	//HUMANHEAD END
}

/*
===============
hhPlayer::ShouldTouchTrigger
===============
*/
bool hhPlayer::ShouldTouchTrigger( idEntity* entity ) const {
	if( !entity ) {
		return false;
	}

	if ( entity->fl.onlySpiritWalkTouch ) { // cjr - Trigger can only be touched by spirit
		return IsSpiritWalking();
	}

	if( IsSpiritOrDeathwalking() && !entity->fl.allowSpiritWalkTouch ) { // Trigger can be touched by either physical or spirit
		return false; // jrm - reversed logic
	}

	return true;
}

/*
===============
hhPlayer::HandleSingleGuiCommand
===============
*/
bool hhPlayer::HandleSingleGuiCommand(idEntity *entityGui, idLexer *src) {

	idToken token;

	if (!src->ReadToken(&token)) {
		return false;
	}

	if (token == ";") {
		return false;
	}

	if (token.Icmp("guilockplayer") == 0) {
		//Now locked in place:
		// manglecontrols() will route controls to current gui focus
		if (entityGui->IsType(hhConsole::Type)) {
			guiWantsControls = static_cast<hhConsole*>(entityGui);
		}
		return true;
	}

	src->UnreadToken(&token);
	return idPlayer::HandleSingleGuiCommand(entityGui, src);
}

void hhPlayer::SetOverlayGui(const char *guiName) {
	if (guiName && *guiName && guiOverlay == NULL) {
		guiOverlay = uiManager->FindGui(guiName, true);
		if (guiOverlay) {
			guiOverlay->Activate(true, gameLocal.time);
		}
	}
	else {
		guiOverlay = NULL;
	}
}

/*
============
hhPlayer::ForceWeapon
nla: used to instantly force the spirit weapon, without lowering and raising
============
*/
void hhPlayer::ForceWeapon( int weaponNum ) {
	const char *		weaponDef;
	hhWeapon *			newWeapon;

	if (spectating) { //rww - if spectating this is bad.
		gameLocal.Error("hhPlayer::ForceWeapon called on spectator. (client %i)", entityNumber);
	}

	// HUMANHEAD mdl:  Special case - coming out of spirit mode when no physical weapon is available
	if ( weaponNum == -1 ) {
		idealWeapon = 0;
		currentWeapon = -1;
		return;
	}

	if ( weaponNum < 1 ) {
		gameLocal.Warning( "Error: Illegal weapon num passed: %d\n", weaponNum );
	}

	weaponDef = GetWeaponName( weaponNum );

	newWeapon = SpawnWeapon( weaponDef );

	weapon = newWeapon;

	idealWeapon = currentWeapon = weaponNum;

	animPrefix = weaponDef;
	animPrefix.Strip( "weaponobj_" );
}


/*
============
hhPlayer::ForceWeapon
============
*/
void hhPlayer::ForceWeapon( hhWeapon *newWeapon ) {
	int weaponNum;


	weapon = newWeapon;

	weaponNum = GetWeaponNum( weapon->GetDict()->GetString( "classname" ) );

	idealWeapon = currentWeapon = weaponNum;

}

/*
===========
hhPlayer::Teleport

HUMANHEAD cjr:  Now calls TeleportNoKillBox, then applies a kill box
============
*/
void hhPlayer::Teleport( const idVec3& origin, const idAngles& angles, idEntity *destination ) {
	//pdm - converted this to call our other version so it doesn't unalign the clip model
	TeleportNoKillBox( origin, mat3_identity, angles.ToForward(), angles );

	// mdl:  Moved here from TeleportNoBox, so player can't avoid falling damage by toggling spirit mode quickly
	physicsObj.SetLinearVelocity( vec3_origin );
	physicsObj.SetKnockBack( 250 ); // Slow the player down after a teleport for a moment

	// kill anything at the new position
	gameLocal.KillBox( this, destination != NULL );

	teleportEntity = destination;
}

/*
===========
hhPlayer::Teleport

HUMANHEAD aob:  Needed for coming back from deathwalk
============
*/
void hhPlayer::Teleport( const idVec3& origin, const idMat3& bboxAxis, const idVec3& viewDir, const idAngles& newUntransformedViewAngles, idEntity *destination ) {
	TeleportNoKillBox( origin, bboxAxis, viewDir, newUntransformedViewAngles );

	// kill anything at the new position
	gameLocal.KillBox( this, destination != NULL );

	teleportEntity = destination;
}

/*
===========
hhPlayer::TeleportNoKillBox

HUMANHEAD cjr:  Called from SpiritProxy
============
*/
void hhPlayer::TeleportNoKillBox( const idVec3& origin, const idMat3& bboxAxis, const idVec3& viewDir, const idAngles& newUntransformedViewAngles ) {
	DisableIK();

	SetOrientation( origin, bboxAxis, viewDir, newUntransformedViewAngles );

	CancelEvents( &EV_ResetGravity );
	ProcessEvent( &EV_ResetGravity ); // Guarantee that gravity is instantly reset after this teleport

	EnableIK();
}

/*
===========
hhPlayer::TeleportNoKillBox

HUMANHEAD cjr: Teleport, but don't telefrag anything
============
*/

void hhPlayer::TeleportNoKillBox( const idVec3& origin, const idMat3& bboxAxis ) {
	TeleportNoKillBox( origin, bboxAxis, GetAxis()[0], GetUntransformedViewAngles() );
}

/*
=====================
hhPlayer::Possess

//HUMANHEAD: cjr - this is the entry point for possession.
=====================
*/
void hhPlayer::Possess( idEntity* possessor ) {
	bPossessed = true;
	
	/* TODO:
		- needs a new spirit proxy (a little hidden object just as storage that is linked to the possessed human)
		- disallow the player from returning until the possessed tommy has been "killed"
			

		possessed tommy:
		- attacks whatever
		- health ticking down -- when it reaches zero, ragdoll and instantly kill the spirit player
		- if killed by the spirit bow, then the wraith is removed (seperate health?)
	*/

	// If we're spirit walking when we're possessed, snap back to our body for a moment, then get thrown back out.
	//TODO this isn't ideal
	if ( IsSpiritOrDeathwalking() ) {
		DisableEthereal();
	}

	// Must allow spirit walking at this point
	HH_ASSERT( bAllowSpirit );

	// Force the player into spirit and thrust them backwards
	StartSpiritWalk( true, true );

	// test:  spawn in a possessed version
	// need: effects here...flash or whatever
	possessedTommy = (hhPossessedTommy *)gameLocal.SpawnObject( "monster_possessed_tommy", NULL );

	if ( possessedTommy.IsValid() ) { // Copy player stats to the possessed Tommy
		/*if ( IsSpiritOrDeathwalking() ) { //TODO mdl:  Remove this if we go with wraiths knocking players back to their body
			// This should means we were called by 
			possessedTommy->SetOrigin( spiritProxy->GetOrigin() );
			possessedTommy->SetAxis( spiritProxy->GetAxis() );
		} else*/ {
			possessedTommy->SetOrigin( GetOrigin() );
			possessedTommy->SetAxis( GetAxis() );
		}
		possessedTommy->SetPossessedProxy( spiritProxy.GetEntity() );
	}
}

/*
=====================
hhPlayer::Unpossess

//HUMANHEAD: cjr
=====================*/

void hhPlayer::Unpossess() {
	bPossessed = false;

	possessedTommy = NULL;
/*
	idDict		args;
	idEntity	*ent;

	playerView.SetViewOverlayMaterial( NULL );
	possessionFOV = 0; // Restore the original FOV

	// Spawn a wraith to fly out of the player's origin
	args.Clear();
	args.SetVector( "origin", GetOrigin() + GetEyePosition() + GetAxis()[0] * 50 );
	args.SetMatrix( "rotation", GetAxis() );
//TODO: Externalize this monster_wraith
	args.Set( "classname", "monster_wraith" );

	gameLocal.SpawnEntityDef( args, &ent );
	((hhWraith *)ent)->SetEnemy( this );

	// TODO: Unpossessed by portalling, so kill the wraith
*/
}

//=============================================================================
//
// hhPlayer::CanBePossessed
//
// Players can only be possessed if they are are not currently possessed
//=============================================================================

bool hhPlayer::CanBePossessed( void ) {
	if ( godmode || noclip || !bAllowSpirit ) { // Don't possess if in god mode or noclipping
		return false;
	}

	return !(IsPossessed() || InVehicle() || IsSpiritOrDeathwalking() );
}

//=============================================================================
//
// hhPlayer::PossessKilled
//
// Killed when possessed (spirit power ran out)
// 
// - Ragdoll the possessed body
// - Return to the location of the possessed tommy and die
//=============================================================================

void hhPlayer::PossessKilled( void ) {
	if ( possessedTommy.IsValid() ) {
		possessedTommy->PostEventMS( &EV_Remove, 0 );
	}

	Unpossess();
	StopSpiritWalk();
	Kill( 0, 0 ); // FIXME:  Some other way to kill self (instead of suicide?)
}

//=============================================================================
//
// hhPlayer::Portalled
//
// Player just portalled
//=============================================================================

void hhPlayer::Portalled( idEntity *portal ) {
	if ( talon.IsValid() && talon->GetBindMaster() != this ) { // If Talon isn't bound to Tommy, portal him with the player
		talon->Portalled( portal );
	}

	if (gameLocal.isClient) { //HUMANHEAD rww - compensate for angle jump
		bBufferNextSnapAngles = true;
		smoothedAngles = viewAngles;
	}
	smoothedFrame = 0; //HUMANHEAD rww - skip smoothing on the portalled frame
	walkIK.InvalidateHeights(); //HUMANHEAD rww - don't try to interpolate ik positions between two equal planes on either side of a portal
	if (gameLocal.isServer) { //HUMANHEAD rww - send an unreliable event in the coming snapshot
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init(msgBuf, sizeof(msgBuf));
		msg.WriteBits(gameLocal.GetSpawnId(portal), 32);
		ServerSendEvent(EVENT_PORTALLED, &msg, false, -1, -1, true); //only send it to the client who portalled
	}
}

/*
================
hhPlayer::UpdateModelTransform
================
*/
void hhPlayer::UpdateModelTransform( void ) {
	idVec3 origin;
	idMat3 axis;

	if( GetPhysicsToVisualTransform(origin, axis) ) {
		//HUMANHEAD: aob
		GetRenderEntity()->axis = axis;
		idVec3 absOrigin = TransformToPlayerSpaceNotInterpolated( origin ); //rww - don't use interpolated origin
		GetRenderEntity()->origin = absOrigin;
		//HUMANHEAD END
	} else {
		//HUMANHEAD: aob
		GetRenderEntity()->axis = GetAxis();
		GetRenderEntity()->origin = GetOrigin();
		//HUMANHEAD END
	}
}

/*
====================
hhPlayer::CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
Takes possession FOV into account

HUMANHEAD cjr
====================
*/
float hhPlayer::CalcFov( bool honorZoom ) {
	float	fov;

	// HUMANHEAD mdl:  Refactored this to work properly with possessionFOV.  Being zoomed in will ignore possessionFOV, however.
	idEntity *weaponEnt = weapon.GetEntity();
	if ( possessionFOV > 0.0f && 
		 ! ( weaponEnt &&
			 weaponEnt->IsType( hhWeaponZoomable::Type ) &&
			 reinterpret_cast<hhWeaponZoomable *> (weaponEnt)->IsZoomed() ) )
	{
		fov = possessionFOV;
	} else if ( IsDeathWalking() ) {
		fov = spawnArgs.GetFloat("deathwalkFOV", "90");
	} else if ( pm_thirdPerson.GetBool() ) {
		fov = g_fov.GetFloat();
	} else if ( InCinematic() ) {
		fov = cinematicFOV.GetCurrentValue(gameLocal.time);
	} else {
		fov = zoomFov.GetCurrentValue(gameLocal.time);
	}

	//HUMANHEAD: aob
	fov = hhMath::ClampFloat( 1.0f, 179.0f, fov );
	//HUMANHEAD END

	return fov;
}

/*
===============
hhPlayer::EnterVehicle
===============
*/
void hhPlayer::EnterVehicle( hhVehicle* vehicle ) {
	if (!gameLocal.isClient) { //HUMANHEAD PCF rww 05/04/06 - do not do the check on the client, just listen to what the snapshot says.
		if (!vehicle->WillAcceptPilot(this)) {
			return;
		}
	}

	// Cancel any pending damage allowing events, since we'll be turning damage off
	CancelEvents(&EV_AllowDamage);

	// Set shuttle view
	if (entityNumber == gameLocal.localClientNum) { //rww
		renderSystem->SetShuttleView( true );
	}

	// Move eye to proper height
	SetEyeHeight( vehicle->spawnArgs.GetFloat("pilot_eyeHeight") );

	// Turn off any illegal behaviors
	if( IsSpiritWalking() ) {
		StopSpiritWalk();
	}

	LighterOff();

	// nla - Remove any offset cause by jumping.  (Fixes the jumping and getting in the shuttle bug)
	SetViewBob( vec3_origin );

	// CJR: Inform Talon that the player has entered a vehicle
	if( talon.IsValid() ) {
		talon->OwnerEnteredVehicle();
	}

	ShouldRemainAlignedToAxial( false );

	cameraInterpolator.SetInterpolationType( IT_None );
	SetOrientation( GetOrigin(), mat3_identity, vehicle->GetAxis()[0], vehicle->GetAxis()[0].ToAngles() );
	//Need to clear untransformedViewAxis so cameraInterpolator doesn't do any unnessacary transforms
	SetUntransformedViewAxis( mat3_identity );

	//Hack
	cameraInterpolator.Reset( GetOrigin(), mat3_identity[2], EyeHeightIdeal() );

	idPlayer::EnterVehicle( vehicle );
}

/*
===============
hhPlayer::ExitVehicle

This should only be called from vehicle
===============
*/
void hhPlayer::ExitVehicle( hhVehicle* vehicle ) {

	// Allow model in player's view again
	if (vehicle) {
		vehicle->GetRenderEntity()->suppressSurfaceInViewID = 0;
	}

	// Set shuttle view off
	if (entityNumber == gameLocal.localClientNum) { //rww
		renderSystem->SetShuttleView( false );
	}

	ShouldRemainAlignedToAxial( true );

	// CJR: Inform Talon that the player has left a vehicle
	if( talon.IsValid() ) {
		talon->OwnerExitedVehicle();
	}

	idPlayer::ExitVehicle( vehicle );

	buttonMask |= BUTTON_ATTACK_ALT;	//HUMANHEAD bjk

	// Reset the animPrefix
	animPrefix = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
	animPrefix.Strip( "weaponobj_" );

	if (vehicle) {
		SetOrientation( GetOrigin(), mat3_identity, vehicle->GetAxis()[0], vehicle->GetAxis()[0].ToAngles().Normalize180() );
	}
	cameraInterpolator.SetInterpolationType( IT_VariableMidPointSinusoidal );
}

//
// ResetClipModel()
//
// HUMANHEAD: aob
//
void hhPlayer::ResetClipModel() {
	//Needed for touching triggers.  Our bbox is its original size.
	SetClipModel();
}

/*
===============
hhPlayer::BecameBound
===============
*/
void hhPlayer::BecameBound(hhBindController *b) {
	if( !gameLocal.isMultiplayer && weapon.IsValid() ) { //rww - can use weapon while bound in mp
		weapon->Hide();
	}
}

/*
===============
hhPlayer::BecameUnbound
===============
*/
void hhPlayer::BecameUnbound(hhBindController *b) {
	if( !gameLocal.isMultiplayer && weapon.IsValid() ) { //rww - can use weapon while bound in mp
		weapon->Show();
	}
}

void hhPlayer::GetLocationText( idStr &locationString ) {
	idLocationEntity* locationEntity = gameLocal.LocationForPoint( GetEyePosition() );
	if( locationEntity ) {
		locationString = locationEntity->GetLocation();
	}
	else {
		locationString = common->GetLanguageDict()->GetString( "#str_02911" );
	}
}

void hhPlayer::UpdateLocation( void ) {
	if( hud ) {
		hud->SetStateBool("showlocations", developer.GetBool());
		if (developer.GetBool()) {
			idStr locationString;
			GetLocationText(locationString);
			hud->SetStateString( "location", locationString.c_str() );
			hud->SetStateInt( "areanum", gameRenderWorld->PointInArea( GetEyePosition() ));
		}
	}
}

/*
===============
hhPlayer::FillDebugVars
===============
*/
void hhPlayer::FillDebugVars(idDict *args, int page) {
	idStr text;

	switch(page) {
	case 1:
		args->SetInt("deathpower", GetDeathWalkPower());
		args->SetInt("spiritpower", GetSpiritPower());
		args->SetBool("AI_CROUCH", AI_CROUCH != 0);
		args->SetBool("AI_ONGROUND", AI_ONGROUND != 0);
		args->SetBool("AI_JUMP", AI_JUMP != 0);
		args->SetBool("AI_SOFTLANDING", AI_SOFTLANDING != 0);
		args->SetBool("AI_HARDLANDING", AI_HARDLANDING != 0);
		args->SetBool("AI_FORWARD", AI_FORWARD != 0);
		args->SetBool("AI_BACKWARD", AI_BACKWARD != 0);
		args->SetBool("AI_STRAFE_LEFT", AI_STRAFE_LEFT != 0);
		args->SetBool("AI_STRAFE_RIGHT", AI_STRAFE_RIGHT != 0);
		args->SetBool("AI_ATTACK_HELD", AI_ATTACK_HELD != 0);
		args->SetBool("AI_WEAPON_FIRED", AI_WEAPON_FIRED != 0);
		args->SetBool("AI_JUMP", AI_JUMP != 0);
		args->SetBool("AI_DEAD", AI_DEAD != 0);
		args->SetBool("AI_PAIN", AI_PAIN != 0);
		args->SetBool("AI_RELOAD", AI_RELOAD != 0);
		args->SetBool("AI_TELEPORT", AI_TELEPORT != 0);
		args->SetBool("AI_TURN_LEFT", AI_TURN_LEFT != 0);
		args->SetBool("AI_TURN_RIGHT", AI_TURN_RIGHT != 0);
		args->SetBool("AI_ASIDE", AI_ASIDE != 0);
		args->SetBool("AI_REALLYFALL", AI_REALLYFALL != 0); //HUMANHEAD rww
		args->SetBool("HasGroundContacts", physicsObj.HasGroundContacts());
		args->Set("animPrefix", animPrefix.c_str());
		break;
	case 2:
		args->Set("Physics Axis", GetPhysics()->GetAxis().ToAngles().ToString());
		args->Set("viewAxis", viewAxis.ToAngles().ToString());
		args->Set("cmdAngles", cmdAngles.ToString());
		args->Set("deltaViewAngles", GetDeltaViewAngles().ToString() );
		args->Set("viewAngles", viewAngles.ToString());
		args->Set("untransAngles", GetUntransformedViewAngles().ToString());
		args->Set("xPhysics Axis", GetPhysics()->GetAxis().ToString());
		args->Set("xviewAxis", viewAxis.ToString());
		args->Set("idealLegsYaw", va("%.1f", idealLegsYaw));
		args->SetInt("usercmd.forwardmove", usercmd.forwardmove);
		args->SetInt("usercmd.rightmove", usercmd.rightmove);
		args->SetInt("legsForward", legsForward);

		switch(physicsObj.GetWaterLevel()) {
			case WATERLEVEL_NONE:	text = "none"; break;
			case WATERLEVEL_FEET:	text = "feet"; break;
			case WATERLEVEL_WAIST:	text = "waist"; break;
			case WATERLEVEL_HEAD:	text = "head"; break;
		}
		args->Set("waterlevel", text);
		text = collisionModelManager->ContentsName(physicsObj.GetWaterType());
		args->Set("watertype", text);
		break;
	case 3:
		break;
	}
	idPlayer::FillDebugVars(args, page);
}

//
// GetAimPosition()
//
idVec3 hhPlayer::GetAimPosition() const {
	return GetEyePosition();
}

/*
===============
hhPlayer::WriteToSnapshot
===============
*/
void hhPlayer::WriteToSnapshot( idBitMsgDelta &msg ) const {
	bool vehControlling = vehicleInterfaceLocal.ControllingVehicle();

	msg.WriteBits(vehControlling, 1);

	//rww - our stuff is now intermingled with id's for more ideal send ordering
	physicsObj.WriteToSnapshot( msg, vehControlling );

	if (!vehControlling) {
		//not syncing these causes denormalization/nan issues, FIXME
		msg.WriteFloat(untransformedViewAngles[0]);
		msg.WriteFloat(untransformedViewAngles[1]);
		msg.WriteFloat(untransformedViewAngles[2]);
		idCQuat q = untransformedViewAxis.ToCQuat();
		msg.WriteFloat(q.x);
		msg.WriteFloat(q.y);
		msg.WriteFloat(q.z);

		//rww - write cameraInterpolator
		cameraInterpolator.WriteToSnapshot(msg, this);

		//still need delta angles
		//msg.WriteDeltaFloat( 0.0f, deltaViewAngles[0] );
		//msg.WriteDeltaFloat( 0.0f, deltaViewAngles[1] );
		//msg.WriteDeltaFloat( 0.0f, deltaViewAngles[2] );
	}
#if 0 //for debugging differences in snapshot
	else {
		msg.WriteFloat(0.0f);
		msg.WriteFloat(0.0f);
		msg.WriteFloat(0.0f);
		msg.WriteFloat(0.0f);
		msg.WriteFloat(0.0f);
		msg.WriteFloat(0.0f);

		msg.WriteFloat(0.0f);
		msg.WriteFloat(0.0f);
		msg.WriteFloat(0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteFloat(0.0f, 4, 4);

		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteFloat(0.0f, 4, 4);

		msg.WriteFloat(0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteFloat(0.0f, 4, 4);

		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
		msg.WriteDeltaFloat(0.0f, 0.0f);
	}
#endif

	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[0] );
	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[1] );
	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[2] );

	msg.WriteShort( health );
	msg.WriteBits( gameLocal.ServerRemapDecl( -1, DECL_ENTITYDEF, lastDamageDef ), gameLocal.entityDefBits );
	msg.WriteDir( lastDamageDir, 9 );
	msg.WriteShort( lastDamageLocation );
	msg.WriteBits( idealWeapon, idMath::BitsForInteger( MAX_WEAPONS ) );
	msg.WriteBits( inventory.weapons, MAX_WEAPONS );
	msg.WriteBits( weapon.GetSpawnId(), 32 );
	msg.WriteBits( spectator, idMath::BitsForInteger( MAX_CLIENTS ) );
	msg.WriteBits( lastHitToggle, 1 );
	msg.WriteBits( weaponGone, 1 );
	WriteBindToSnapshot( msg );
//===================END OF ID DATA
	msg.WriteShort(inventory.maxHealth); //rww - since maxhealth can go above 100 in mp now

	msg.WriteBits( spiritProxy.GetSpawnId(), 32 );

	//rww - more spiritwalk stuff
	msg.WriteBits(lastWeaponSpirit, 32);
	msg.WriteBits(bSpiritWalk, 1);

	//not needed anymore
	/*
	msg.WriteBits( bShowProgressBar, 1 );
	msg.WriteFloat( progressBarValue );
	msg.WriteBits( progressBarState, 2 );	// possible values [0..2]
	*/

	//rww - send ammo for current weapon - this is because our weapon routines rely on if we have ammo
	//in scripts and so on, and will be predicted wrong for other players (we get our own ammo in the playerstate)
	//rwwFIXME: if we have proper weapon switching when you run out of ammo will this actually be necessary?
	//i don't think it's all that costly, but still.
	if (weapon.IsValid()) {
		ammo_t ammoType = weapon->GetAmmoType();
		if (ammoType > 0) {
			msg.WriteBits(inventory.ammo[ammoType], ASYNC_PLAYER_INV_AMMO_BITS);
		}
		else {
			msg.WriteBits(0, ASYNC_PLAYER_INV_AMMO_BITS);
		}
		ammoType = weapon->GetAltAmmoType();
		if (ammoType > 0) {
			msg.WriteBits(inventory.ammo[ammoType], ASYNC_PLAYER_INV_AMMO_BITS);
		}
		else {
			msg.WriteBits(0, ASYNC_PLAYER_INV_AMMO_BITS);
		}
	}
	else {
		msg.WriteBits(0, ASYNC_PLAYER_INV_AMMO_BITS);
		msg.WriteBits(0, ASYNC_PLAYER_INV_AMMO_BITS);
	}

	//need to sync buttonMask since we're using it for some state-based things
	msg.WriteBits(buttonMask, 8);

	msg.WriteFloat(EyeHeight());

	//HUMANHEAD PCF rww 05/04/06 - do not sync AI_VEHICLE, it is now based purely on the clientside
	//enter/exit of vehicles, with the vehControlling stack bool determining if we should be in a
	//vehicle on the client or not.
	//msg.WriteBits(AI_VEHICLE, 1);

	//rww - hand stuff
	//weaponHandState.WriteToSnapshot(msg);
	msg.WriteBits(hand.GetSpawnId(), 32);
	msg.WriteBits(handNext.GetSpawnId(), 32);

	//jsh - vehicle stuff
	msg.WriteBits( vehicleInterfaceLocal.GetVehicleSpawnId(), 32 );
	msg.WriteBits( vehicleInterfaceLocal.GetHandSpawnId(), 32 );
	//vehicleInterfaceLocal.GetWeaponHandState()->WriteToSnapshot(msg);

	//rww - lighter sync validation
	msg.WriteBits((lighterHandle != -1), 1);

	//rww - tractor
	msg.WriteBits(fl.isTractored, 1);

	spiritwalkSoundController.WriteToSnapshot(msg);
	//deathwalkSoundController.WriteToSnapshot(msg);
	wallwalkSoundController.WriteToSnapshot(msg);

	//HUMANHEAD rww - leechgun energy ammo
	//note - current weapon's ammo always sent now
	//ammo_t energyammo = hhWeaponFireController::GetAmmoType("ammo_energy");
	//msg.WriteBits(inventory.ammo[energyammo], ASYNC_PLAYER_INV_AMMO_BITS);
}

/*
===============
hhPlayer::ReadFromSnapshot
===============
*/
void hhPlayer::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	int		i, oldHealth, newIdealWeapon, weaponSpawnId;
	bool	newHitToggle, stateHitch;

	bool vehControlling = !!msg.ReadBits(1);

	if ( snapshotSequence - lastSnapshotSequence > 1 ) {
		stateHitch = true;
	} else {
		stateHitch = false;
	}
	lastSnapshotSequence = snapshotSequence;

	oldHealth = health;

	physicsObj.ReadFromSnapshot( msg, vehControlling );

	if (!vehControlling) {
		//not syncing these causes denormalization/nan issues, FIXME
		if (bBufferNextSnapAngles) {
			idAngles n;
			n[0] = msg.ReadFloat();
			n[1] = msg.ReadFloat();
			n[2] = msg.ReadFloat();
			BufferLoggedViewAngles(n);
			untransformedViewAngles = n;
			bBufferNextSnapAngles = false;
		}
		else {
			untransformedViewAngles[0] = msg.ReadFloat();
			untransformedViewAngles[1] = msg.ReadFloat();
			untransformedViewAngles[2] = msg.ReadFloat();
		}
		idCQuat q;
		q.x = msg.ReadFloat();
		q.y = msg.ReadFloat();
		q.z = msg.ReadFloat();
		untransformedViewAxis = q.ToMat3();

		//rww - read cameraInterpolator
		cameraInterpolator.ReadFromSnapshot(msg, this);

		//deltaViewAngles[0] = msg.ReadDeltaFloat( 0.0f );
		//deltaViewAngles[1] = msg.ReadDeltaFloat( 0.0f );
		//deltaViewAngles[2] = msg.ReadDeltaFloat( 0.0f );
	}
#if 0 //for debugging differences in snapshot
	else {
		msg.ReadFloat();
		msg.ReadFloat();
		msg.ReadFloat();
		msg.ReadFloat();
		msg.ReadFloat();
		msg.ReadFloat();

		msg.ReadFloat();
		msg.ReadFloat();
		msg.ReadFloat();
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadFloat(4, 4);

		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadFloat(4, 4);

		msg.ReadFloat();
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadFloat(4, 4);

		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
		msg.ReadDeltaFloat(0.0f);
	}
#endif

	deltaViewAngles[0] = msg.ReadDeltaFloat( 0.0f );
	deltaViewAngles[1] = msg.ReadDeltaFloat( 0.0f );
	deltaViewAngles[2] = msg.ReadDeltaFloat( 0.0f );

	health = msg.ReadShort();
	lastDamageDef = gameLocal.ClientRemapDecl( DECL_ENTITYDEF, msg.ReadBits( gameLocal.entityDefBits ) );
	lastDamageDir = msg.ReadDir( 9 );
	lastDamageLocation = msg.ReadShort();
	newIdealWeapon = msg.ReadBits( idMath::BitsForInteger( MAX_WEAPONS ) );
	inventory.weapons = msg.ReadBits( MAX_WEAPONS );
	weaponSpawnId = msg.ReadBits( 32 );
	spectator = msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) );
	newHitToggle = msg.ReadBits( 1 ) != 0;
	weaponGone = msg.ReadBits( 1 ) != 0;
	ReadBindFromSnapshot( msg );

//===================END OF ID DATA

	inventory.maxHealth = msg.ReadShort(); //rww - since maxhealth can go above 100 in mp now

	idQuat quat;

	/*
	if( spiritProxy.SetSpawnId( msg.ReadBits( 32 ) ) ) {
		if (spiritProxy.GetEntity() && spiritProxy.GetEntity()->IsType(hhSpiritProxy::Type))
		{
			spiritProxy.GetEntity()->ActivateProxy( this, GetOrigin(), GetPhysics()->GetAxis(), viewAxis, viewAngles );
		}
	}*/
	spiritProxy.SetSpawnId( msg.ReadBits( 32 ) );

	//rww - more spiritwalk stuff
	lastWeaponSpirit = msg.ReadBits(32);
	
	bool spiritWalking = !!msg.ReadBits(1);
	if (spiritWalking != bSpiritWalk && (weapon.IsValid() || !spiritWalking)) {
		bSpiritWalk = spiritWalking;

		LighterOff(); //make sure lighter is off when switching spiritwalk

		if (bSpiritWalk) {
			if (gameLocal.localClientNum == entityNumber) {
				if (hud) {
					hud->HandleNamedEvent("SwitchToEthereal");
				}
				gameLocal.SpiritWalkSoundMode( true );
			}

			// Set the player's skin to a glowy effect
			SetSkinByName( spawnArgs.GetString("skin_Spiritwalk") );
			SetShaderParm( SHADERPARM_TIMEOFFSET, 1.0f ); // TEMP: cjr - Required by the forcefield material.  Can remove when a proper spiritwalk texture is made

			//put bow in proper state
			SelectEtherealWeapon();
		}
		else {
			if (gameLocal.localClientNum == entityNumber) {
				if (hud) {
					hud->HandleNamedEvent("SwitchFromEthereal");
				}
				gameLocal.SpiritWalkSoundMode( false );
			}

			SetSkinByName( NULL );
		}
	}

	//not needed anymore
	/*
	// Handle progress bar
	bool bBar = msg.ReadBits(1) != 0;
	float value = msg.ReadFloat();
	int state = msg.ReadBits(2);
	CL_UpdateProgress(bBar, value, state);
	*/

	// if not a local client assume the client has all ammo types
	if ( entityNumber != gameLocal.localClientNum ) {
		for( i = 0; i < AMMO_NUMTYPES; i++ ) {
			inventory.ammo[ i ] = 999;
		}
	}

	//rww - send ammo for current weapon - this is because our weapon routines rely on if we have ammo
	//in scripts and so on, and will be predicted wrong for other players (we get our own ammo in the playerstate)
	//rwwFIXME: if we have proper weapon switching when you run out of ammo will this actually be necessary?
	//i don't think it's all that costly, but still.
	int primAmmo = msg.ReadBits(ASYNC_PLAYER_INV_AMMO_BITS);
	int altAmmo = msg.ReadBits(ASYNC_PLAYER_INV_AMMO_BITS);
	if (entityNumber != gameLocal.localClientNum) {
		//since we have our own ammo in the playerstate, don't want to stomp it
		if (weapon.IsValid()) {
			ammo_t ammoType = weapon->GetAmmoType();
			if (ammoType > 0) {
				inventory.ammo[ammoType] = primAmmo;
			}
			ammoType = weapon->GetAltAmmoType();
			if (ammoType > 0) {
				inventory.ammo[ammoType] = altAmmo;
			}
		}
	}

	//need to sync buttonMask since we're using it for some state-based things
	buttonMask = msg.ReadBits(8);

	SetEyeHeight(msg.ReadFloat());

	//HUMANHEAD PCF rww 05/04/06 - do not sync AI_VEHICLE, it is now based purely on the clientside
	//enter/exit of vehicles, with the vehControlling stack bool determining if we should be in a
	//vehicle on the client or not.
	//AI_VEHICLE = !!msg.ReadBits(1);

	//rww - hand stuff
	//weaponHandState.ReadFromSnapshot(msg);
	hand.SetSpawnId(msg.ReadBits(32));
	handNext.SetSpawnId(msg.ReadBits(32));

	//HUMANHEAD PCF rww 05/04/06 - base this check on AI_VEHICLE and make sure the player is exited,
	//even if the vehicle is not in the snapshot at this point.
	if (!vehControlling && AI_VEHICLE) { //then exit on the client
		if (vehicleInterfaceLocal.ControllingVehicle()) {
			hhVehicle *veh = GetVehicleInterface()->GetVehicle();
			if (veh && veh->GetPilot() == this) {
				veh->EjectPilot();
			}
		}
		ExitVehicle(NULL); //be extra safe in case vehicle is no longer around
	}

	//jsh - vehicle stuff
	int vehSpawnId = msg.ReadBits( 32 );
	//HUMANHEAD PCF rww 05/04/06 - base this check ON AI_VEHICLE, in case snapshot where controlling and
	//where the vehicle is set do not exactly coincide.
	vehicleInterfaceLocal.SetVehicleSpawnId( vehSpawnId );
	if( !AI_VEHICLE ) {
		if (vehControlling) {
			hhVehicle *veh = vehicleInterfaceLocal.GetVehicle();
			if (veh && veh->IsType(hhVehicle::Type)) {
				//GetVehicleInterface()->TakeControl( vehicleInterfaceLocal.GetVehicle(), this );
				EnterVehicle( vehicleInterfaceLocal.GetVehicle() );
			}
		}
	}

	/*
	if( vehicleInterfaceLocal.SetHandSpawnId( msg.ReadBits( 32 ) ) ) {
		vehicleInterfaceLocal.GetHandEntity()->AttachHand( this, true );
	}
	*/
	vehicleInterfaceLocal.SetHandSpawnId( msg.ReadBits( 32 ) );
	//vehicleInterfaceLocal.GetWeaponHandState()->ReadFromSnapshot(msg);

	//rww - lighter sync validation
	bool newLighterOn = !!msg.ReadBits(1);
	if (newLighterOn != IsLighterOn()) {
		ToggleLighter();
	}

	//rww - tractor
	fl.isTractored = !!msg.ReadBits(1);

	spiritwalkSoundController.ReadFromSnapshot(msg);
	//deathwalkSoundController.ReadFromSnapshot(msg);
	wallwalkSoundController.ReadFromSnapshot(msg);

	//=========================================== start id code
	if ( weapon.SetSpawnId( weaponSpawnId ) ) {
		//HUMANHEAD rww - i am getting crashes here and the stack is all messed up,
		//claiming that SetOwner is off into null, when supposedly the entity is not.
		//rearranging this code so it's easier to see what's going on in a crash.
		hhWeapon *weapEnt = weapon.GetEntity();
		if ( weapEnt ) {
			// maintain ownership locally
			weapEnt->SetOwner( this );
		}
		currentWeapon = -1;
	}

	//HUMANHEAD rww - but we do want ammo count for the leechgun on other players for prediction
	//note - current weapon's ammo always sent now
	//ammo_t energyammo = hhWeaponFireController::GetAmmoType("ammo_energy");
	//inventory.ammo[energyammo] = msg.ReadBits(ASYNC_PLAYER_INV_AMMO_BITS);

	if ( oldHealth > 0 && health <= 0 ) {
		if ( stateHitch ) {
			// so we just hide and don't show a death skin
			UpdateDeathSkin( true );
		}
		// die
		AI_DEAD = true;
		SetAnimState( ANIMCHANNEL_LEGS, "Legs_Death", 4 );
		SetAnimState( ANIMCHANNEL_TORSO, "Torso_Death", 4 );
		SetWaitState( "" );
		animator.ClearAllJoints();

		//HUMANHEAD rww - don't want this
		/*
		if ( entityNumber == gameLocal.localClientNum ) {
			playerView.Fade( colorBlack, 12000 );
		}
		*/
		//HUMANHEAD END

		fl.clientEvents = true; //client event hackery for non-critical cosmetic cleanups
		PostEventMS(&EV_RespawnCleanup, 32);
		fl.clientEvents = false;
		StartRagdoll();
		physicsObj.SetMovementType( PM_DEAD );
		if ( !stateHitch ) {
			//HUMANHEAD PCF rww 09/15/06 - female mp sounds
			if (IsFemale()) {
				StartSound( "snd_death_female", SND_CHANNEL_VOICE, 0, false, NULL );
			}
			else {
				//HUMANHEAD END
			StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
		}
		}
		if ( weapon.GetEntity() ) {
			weapon.GetEntity()->OwnerDied();
		}

		//HUMANHEAD rww
		GetPhysics()->SetContents(0);
		Hide();

		//so camera can operate on client.
		minRespawnTime = gameLocal.time + RAGDOLL_DEATH_TIME;
		maxRespawnTime = minRespawnTime + 10000;
		//HUMANHEAD END
	} else if ( oldHealth <= 0 && health > 0 ) {
		// respawn
		Init();
		StopRagdoll();
		SetPhysics( &physicsObj );
		physicsObj.EnableClip();
		SetCombatContents( true );
		//HUMANHEAD rww
		if (!spectating) {
			Show();
		}
		//HUMANHEAD END
	} else if ( health < oldHealth && health > 0 ) {
		if ( stateHitch ) {
			lastDmgTime = gameLocal.time;
		} else {
			// damage feedback
			const idDeclEntityDef *def = static_cast<const idDeclEntityDef *>( declManager->DeclByIndex( DECL_ENTITYDEF, lastDamageDef, false ) );
			if ( def ) {
				playerView.DamageImpulse( lastDamageDir * viewAxis.Transpose(), &def->dict );
				AI_PAIN = Pain( NULL, NULL, oldHealth - health, lastDamageDir, lastDamageLocation );
				lastDmgTime = gameLocal.time;
			} else {
				common->Warning( "NET: no damage def for damage feedback '%s'\n", lastDamageDef );
			}
		}
	}

	// If the player is alive, restore proper physics object
	if ( health > 0 && IsActiveAF() ) {
		StopRagdoll();
		SetPhysics( &physicsObj );
		physicsObj.EnableClip();
		SetCombatContents( true );
	}

	if ( idealWeapon != newIdealWeapon ) {
		if ( stateHitch ) {
			weaponCatchup = true;
		}
		idealWeapon = newIdealWeapon;
		UpdateHudWeapon();
	}

	if ( lastHitToggle != newHitToggle ) {
		SetLastHitTime( gameLocal.realClientTime );
	}
	//=========================================== end id code

	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}

/*
================
hhPlayer::ServerReceiveEvent
================
*/
bool hhPlayer::ServerReceiveEvent( int event, int time, const idBitMsg &msg ) {
	if ( idEntity::ServerReceiveEvent( event, time, msg ) ) {
		return true;
	}

	// client->server events
	switch( event ) {
		case EVENT_IMPULSE: {
			int impulse = msg.ReadBits( 6 );
			PerformImpulse(impulse);
			if (vehicleInterfaceLocal.ControllingVehicle()) {
				hhVehicle *veh = vehicleInterfaceLocal.GetVehicle();
				if (veh) {
					veh->DoPlayerImpulse(impulse);
				}
			}
			return true;
		}
		default: {
			return false;
		}
	}
}

/*
================
hhPlayer::WritePlayerStateToSnapshot
================
*/
void hhPlayer::WritePlayerStateToSnapshot( idBitMsgDelta &msg ) const {
	idPlayer::WritePlayerStateToSnapshot(msg);

	//rww - extra playerView stuff
	playerView.WriteToSnapshot(msg);

	//rww - scope view
	msg.WriteBits(bScopeView, 1);

	//rww - zoom fov
	msg.WriteFloat(zoomFov.GetDuration());
	msg.WriteFloat(zoomFov.GetEndValue());
	msg.WriteFloat(zoomFov.GetStartTime());
	msg.WriteFloat(zoomFov.GetStartValue());

	//rww - view angle sensitivity
	msg.WriteFloat(GetViewAnglesSensitivity());
}

/*
================
hhPlayer::ReadPlayerStateFromSnapshot
================
*/
void hhPlayer::ReadPlayerStateFromSnapshot( const idBitMsgDelta &msg ) {
	idPlayer::ReadPlayerStateFromSnapshot(msg);

	//rww - extra playerView stuff
	playerView.ReadFromSnapshot(msg);

	//rww - scope and fov handling
	bool canOverrideView = true;
	if (weapon.IsValid() && weapon->IsType(hhWeaponZoomable::Type)) { //if we have a zoomed weapon, don't override with snapshot while the prediction fudge timer is on
		hhWeaponZoomable *weap = static_cast<hhWeaponZoomable *>(weapon.GetEntity());
		if (weap->clientZoomTime >= gameLocal.time) {
			canOverrideView = false;
		}
	}

	bool scopeView = !!msg.ReadBits(1);
	float zfDur = msg.ReadFloat();
	float zfEnd = msg.ReadFloat();
	float zfStT = msg.ReadFloat();
	float zfStV = msg.ReadFloat();

	renderSystem->SetShuttleView( InVehicle() );

	if (canOverrideView) { //if we are currently allowed to override with snapshot values, do so.
		bScopeView = scopeView;
		if (bScopeView != renderSystem->IsScopeView()) {
			renderSystem->SetScopeView(bScopeView);
		}
		zoomFov.SetDuration(zfDur);
		zoomFov.SetEndValue(zfEnd);
		zoomFov.SetStartTime(zfStT);
		zoomFov.SetStartValue(zfStV);
	}

	//rww - view angle sensitivity
	SetViewAnglesSensitivity(msg.ReadFloat());
}

/*
================
hhPlayer::ClientPredictionThink
================
*/
void hhPlayer::ClientPredictionThink( void ) {
	if (gameLocal.localClientNum != entityNumber && forcePredictionButtons) {
		//used by some weapons which rely on prediction-based projectiles, to ensure we don't miss a launch
		gameLocal.usercmds[ entityNumber ].buttons |= forcePredictionButtons;
		if (gameLocal.isNewFrame) {
			forcePredictionButtons = 0;
		}
	}

	idPlayer::ClientPredictionThink();

	if (InVehicle()) {
		UpdateHud( GetVehicleInterfaceLocal()->GetHUD() );
	}
	else {
		UpdateHud( hud );
	}
}

/*
================
hhPlayer::GetPhysicsToVisualTransform
================
*/
bool hhPlayer::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	if ( af.IsActive() ) {
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}

	//rww - adopted id's smoothing code
	if ( gameLocal.isClient && !bindMaster && gameLocal.framenum >= smoothedFrame && ( entityNumber != gameLocal.localClientNum || selfSmooth ) ) {
		if (!smoothedOriginUpdated) {
			idVec3 renderOrigin = TransformToPlayerSpace(modelOffset);
			idVec3 originalOrigin = renderOrigin;

			//id's cheesy smooth code (changed to vec3 because there is no "down" in prey)
			idVec3 originDiff = renderOrigin - smoothedOrigin;
			if (smoothedFrame == 0) { //for teleporting
				originDiff = vec3_origin;
			}
			smoothedOrigin = renderOrigin;
			if ( originDiff.LengthSqr() < Square( 100.0f ) ) {
				// smoothen by pushing back to the previous position
				if ( selfSmooth ) {
					assert( entityNumber == gameLocal.localClientNum );
					renderOrigin -= net_clientSelfSmoothing.GetFloat() * originDiff;
				} else {
					renderOrigin -= gameLocal.clientSmoothing * originDiff;
				}

				//get rid of the smoothing on the "vertical" axis
				idVec3 a(renderOrigin.x*viewAxis[2].x, renderOrigin.y*viewAxis[2].y, renderOrigin.z*viewAxis[2].z);
				idVec3 b(originalOrigin.x*viewAxis[2].x, originalOrigin.y*viewAxis[2].y, originalOrigin.z*viewAxis[2].z);
				renderOrigin -= (a-b);
			}

			smoothedFrame = gameLocal.framenum;
			smoothedOriginUpdated = true;
		}

		axis = viewAxis;
		origin = ( smoothedOrigin - GetPhysics()->GetOrigin() ) * GetEyeAxis().Transpose();
		return true;
	}

	if (bClampYaw) { //rww - lock model angles
		idVec3 masterOrigin;
		idMat3 masterAxis;

		if (GetMasterPosition(masterOrigin, masterAxis)) {
			axis = masterAxis;
			origin = modelOffset;

			return true;
		}
	}

	axis = viewAxis;
	origin = modelOffset;

	return true;
}

bool hhPlayer::UpdateAnimationControllers( void ) {
	bool retValue = idPlayer::UpdateAnimationControllers();

	hhAnimator *theAnimator;
	if (head.IsValid()) {
		theAnimator = head->GetAnimator();
	}
	else {
		theAnimator = GetAnimator();
	}
	JawFlap(theAnimator);

	return retValue;
}

/*
===============
hhPlayer::Event_PlayWeaponAnim
===============
*/
void hhPlayer::Event_PlayWeaponAnim( const char* animName, int numTries ) {
	//AOB: I would like a better solution then constantly banging until weapon is valid.
	if( (!weapon.IsValid() || GetCurrentWeapon() != idealWeapon) && numTries > 0 ) {
		CancelEvents( &EV_PlayWeaponAnim );
		PostEventMS( &EV_PlayWeaponAnim, 50, animName, numTries - 1 ); 
		return;
	}

	CancelEvents( &EV_PlayWeaponAnim );
	if( weapon.IsValid() ) {
		weapon->ProcessEvent( &EV_PlayAnimWhenReady, animName );
	}
}

void hhPlayer::DialogStart(bool bDisallowPlayerDeath, bool bVoiceDucking, bool bLowerWeapon) {
	bDialogDamageMode = bDisallowPlayerDeath;
	bDialogWeaponMode = bLowerWeapon;
	gameLocal.DialogSoundMode(bVoiceDucking);
	if (bDialogWeaponMode) {	// Lock weapon
		preCinematicWeaponFlags = weaponFlags;
		preCinematicWeapon = GetIdealWeapon();
		LockWeapon(-1);
	}
}

void hhPlayer::DialogStop() {
	bDialogDamageMode = false;
	gameLocal.DialogSoundMode(false);
	if (bDialogWeaponMode) {
		weaponFlags = preCinematicWeaponFlags;
		SelectWeapon(preCinematicWeapon, true);
		bDialogWeaponMode = false;
	}
}

/*
===============
hhPlayer::Event_DialogStart
	HUMANHEAD pdm
===============
*/
void hhPlayer::Event_DialogStart( int bDisallowPlayerDeath, int bVoiceDucking, int bLowerWeapon ) {
	DialogStart(bDisallowPlayerDeath != 0, bVoiceDucking != 0, bLowerWeapon != 0);
}

/*
===============
hhPlayer::Event_DialogStop
	HUMANHEAD pdm
===============
*/
void hhPlayer::Event_DialogStop() {
	DialogStop();
}

/*
===============
hhPlayer::Event_LotaTunnelMode
	HUMANHEAD pdm
===============
*/
void hhPlayer::Event_LotaTunnelMode(bool on) {
	bLotaTunnelMode = on;
}

/*
===============
hhPlayer::Event_Cinematic
	HUMANHEAD pdm
===============
*/
void hhPlayer::Event_Cinematic( int on, int lockView ) {
	bool cinematic = (on != 0);
	InCinematic( cinematic );
	playerView.SetLetterBox(cinematic);
	if (cinematic) {
		if ( IsSpiritOrDeathwalking() ) {
			StopSpiritWalk();
		}
		if ( IsPossessed() ) {
			Unpossess();
		}

		// Lock weapon
		preCinematicWeaponFlags = weaponFlags;
		preCinematicWeapon = GetCurrentWeapon();
		LockWeapon(-1);

		// disable damage
		fl.takedamage = false;
		this->lockView = (lockView != 0);
	}
	else {
		fl.takedamage = true;
		this->lockView = false;

		//UnlockWeapon(preCinematicWeapon);
		weaponFlags = preCinematicWeaponFlags;
		//SetCurrentWeapon(preCinematicWeapon);
		SelectWeapon(preCinematicWeapon, true);
	}
}

//=============================================================================
//
// hhPlayer::Event_PrepareToResurrect
//
// Flashes the screen / fades the FOV
//=============================================================================

void hhPlayer::Event_PrepareToResurrect() {	
	deathWalkFlash = 0;
	possessionFOV = 90.0f;
	PostEventSec( &EV_ResurrectScreenFade, 0 );
}

//=============================================================================
//
// hhPlayer::Event_ResurrectScreenFade
//
// The actual code to incrementally flash the screen
//=============================================================================

void hhPlayer::Event_ResurrectScreenFade() {
	deathWalkFlash += spawnArgs.GetFloat( "resurrectFlashChange", "0.04" );
	if ( deathWalkFlash >= 1.0f ) {
		deathWalkFlash = 1.0f;
		PostEventSec( &EV_Resurrect, 0.0f ); // Actually send player back to the physical realm
	} else {
		PostEventSec( &EV_ResurrectScreenFade, 0.02f );
	}

	possessionFOV += spawnArgs.GetFloat( "resurrectFOVChange", "1.0" );
	playerView.SetViewOverlayColor( idVec4( 1.0f, 1.0f, 1.0f, deathWalkFlash ) );
}

//=============================================================================
//
// hhPlayer::Event_Resurrect
//
//=============================================================================
void hhPlayer::Event_Resurrect() {
	Resurrect();
}

//=============================================================================
//
// hhPlayer::Event_Event_ShouldRemainAlignedToAxial
//
//=============================================================================
void hhPlayer::Event_ShouldRemainAlignedToAxial( bool remainAligned ) {
	ShouldRemainAlignedToAxial( remainAligned );
}

//=============================================================================
//
// hhPlayer::Event_OrientToGravity
//
//=============================================================================
void hhPlayer::Event_OrientToGravity( bool orient ) {
	OrientToGravity( orient );
}

//=============================================================================
//
// hhPlayer::Event_ResetGravity
//	HUMANHEAD: pdm: Posted when entity is leaving a gravity zone
//=============================================================================
void hhPlayer::Event_ResetGravity() {
	if( IsWallWalking() ) {
		return;	// Don't reset if wallwalking
	}

	if (spectating) { //HUMANHEAD rww - don't reset if spectating
		return;
	}

	idPlayer::Event_ResetGravity();

	OrientToGravity( true );	// let it reset orientation
}

//=============================================================================
//
// hhPlayer::Event_SetOverlayMaterial
//
//=============================================================================
void hhPlayer::Event_SetOverlayMaterial( const char *mtrName, const int requiresScratch ) {
	if ( mtrName && mtrName[0] ) {
		playerView.SetViewOverlayMaterial( declManager->FindMaterial(mtrName), requiresScratch );
	} else {
		playerView.SetViewOverlayMaterial( NULL );
	}
}

//=============================================================================
//
// hhPlayer::Event_SetOverlayTime
//
//=============================================================================
void hhPlayer::Event_SetOverlayTime( const float newTime, const int requiresScratch ) {
	playerView.SetViewOverlayTime( newTime, requiresScratch );
}

//=============================================================================
//
// hhPlayer::Event_SetOverlayColor
//
//=============================================================================
void hhPlayer::Event_SetOverlayColor( const float r, const float g, const float b, const float a ) {
	idVec4 color;

	color.x = r;
	color.y = g;
	color.z = b;
	color.w = a;

	playerView.SetViewOverlayColor( color );
}

//=============================================================================
//
// hhPlayer::Event_DDAHeartBeat
//
//=============================================================================

void hhPlayer::Event_DDAHeartBeat() {
	gameLocal.GetDDA()->DDA_Heartbeat( this );
	PostEventMS( &EV_DDAHeartbeat, ddaHeartbeatMS );
}

//================
//hhPlayer::Save
//================
void hhPlayer::Save( idSaveGame *savefile ) const {
	// Public vars
	savefile->Write(weaponInfo, sizeof(weaponInfo_t)*15);
	savefile->Write(altWeaponInfo, sizeof(weaponInfo_t)*15);
	savefile->WriteFloat( lighterTemperature );
	savefile->WriteRenderLight( lighter );
	//HUMANHEAD PCF mdl 05/04/06 - Don't save light handles
    //savefile->WriteInt( lighterHandle );
	spiritProxy.Save( savefile );
	savefile->WriteInt( lastWeaponSpirit );
	talon.Save( savefile );
	savefile->WriteInt( nextTalonAttackCommentTime );
	savefile->WriteBool( bTalonAttackComment );
	savefile->WriteBool( bSpiritWalk );
	savefile->WriteBool( bDeathWalk );
	savefile->WriteBool( bReallyDead );
	guiWantsControls.Save( savefile );
	savefile->WriteFloat( deathWalkFlash );
	savefile->WriteInt( deathWalkTime );
	savefile->WriteInt( deathWalkPower );
	savefile->WriteBool( bInDeathwalkTransition );
	savefile->WriteBool( bInCinematic );
	savefile->WriteBool( bPlayingLowHealthSound );
	savefile->WriteBool( bPossessed );
	savefile->WriteFloat( possessionTimer );
	savefile->WriteFloat( possessionFOV );
	savefile->WriteInt( preCinematicWeapon );
	savefile->WriteInt( preCinematicWeaponFlags );
	savefile->WriteInt( lastDamagedTime );
	savefile->WriteStaticObject( weaponHandState );
	hand.Save( savefile );
	handNext.Save( savefile );
	possessedTommy.Save( savefile );

	savefile->WriteStaticObject( vehicleInterfaceLocal );
	savefile->WriteObject( vehicleInterfaceLocal.GetVehicle() );

	deathLookAtEntity.Save( savefile );
	savefile->WriteString( deathLookAtBone );
	savefile->WriteString( deathCameraBone );
	savefile->WriteStaticObject( spiritwalkSoundController );
	savefile->WriteStaticObject( deathwalkSoundController );
	savefile->WriteStaticObject( wallwalkSoundController );
	savefile->WriteBool( bShowProgressBar );
	savefile->WriteFloat( progressBarValue );

	savefile->WriteFloat( progressBarGuiValue.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( progressBarGuiValue.GetDuration() );
	savefile->WriteFloat( progressBarGuiValue.GetStartValue() );
	savefile->WriteFloat( progressBarGuiValue.GetEndValue() );

	savefile->WriteInt( progressBarState );
	savefile->WriteBool( bClampYaw );
	savefile->WriteFloat( maxRelativeYaw );
	savefile->WriteFloat( maxRelativePitch );
	savefile->WriteFloat( bob );
	savefile->WriteInt( lastAppliedBobCycle );
	savefile->WriteInt( prevStepUpTime );
	savefile->WriteVec3( prevStepUpOrigin );
	savefile->WriteFloat( crashlandSpeed_fatal );
	savefile->WriteFloat( crashlandSpeed_soft );
	savefile->WriteFloat( crashlandSpeed_jump );
	// Protected vars
	savefile->WriteUserInterface( guiOverlay, false );
	thirdPersonCameraClipBounds.Save( savefile );
	savefile->WriteFloat( viewAnglesSensitivity );
	savefile->WriteInt( lastResurrectTime );
	savefile->WriteInt( spiritDrainHeartbeatMS );
	savefile->WriteInt( ddaHeartbeatMS );

	savefile->WriteInt( spiritWalkToggleTime );
	savefile->WriteBool( bDeathWalkStage2 );
	savefile->WriteBool( bFrozen );
	savefile->WriteAngles( untransformedViewAngles );
	savefile->WriteMat3( untransformedViewAxis );
	savefile->WriteInt( nextSpiritTime );

	savefile->WriteFloat( cinematicFOV.GetStartTime() );
	savefile->WriteFloat( cinematicFOV.GetAcceleration() );
	savefile->WriteFloat( cinematicFOV.GetDeceleration() );
	savefile->WriteFloat( cinematicFOV.GetDuration() );
	savefile->WriteFloat( cinematicFOV.GetStartValue() );
	savefile->WriteFloat( cinematicFOV.GetEndValue() );
	savefile->WriteBool( bAllowSpirit );
	savefile->WriteInt( airAttackerTime );
	savefile->WriteBool( bScopeView );
	savefile->WriteInt( ddaNumEnemies );
	savefile->WriteFloat( ddaProbabilityAccum );
	savefile->WriteInt( weaponFlags );
	savefile->WriteBool( lockView );

	savefile->WriteBool( bCollidingWithPortal );
	savefile->WriteBool( bLotaTunnelMode );
	savefile->WriteInt( forcePredictionButtons );

	for (int ix=0; ix<MAX_TRACKED_ATTACKERS; ix++) {
		lastAttackers[ix].attacker.Save( savefile );
		savefile->WriteInt( lastAttackers[ix].time );
		savefile->WriteBool( lastAttackers[ix].displayed );
	}

	if ( InVehicle() ) {
		savefile->WriteMat3( vehicleInterfaceLocal.GetVehicle()->GetPhysics()->GetAxis() );
		savefile->WriteVec3( vehicleInterfaceLocal.GetVehicle()->GetAxis()[0] );
	}

	//HUMANHEAD PCF mdl 04/28/06 - Moved camera interpolater down here to fix jump off wallwalk view angle problem
	cameraInterpolator.Save( savefile );
	//HUMANHEAD PCF mdl 05/04/06 - Save whether the light handle is active
	savefile->WriteBool( IsLighterOn() );
}

//================
//hhPlayer::Restore
//================
void hhPlayer::Restore( idRestoreGame *savefile ) {
	// Public vars
	savefile->Read(weaponInfo, sizeof(weaponInfo_t)*15);
	savefile->Read(altWeaponInfo, sizeof(weaponInfo_t)*15);
	savefile->ReadFloat( lighterTemperature );
	savefile->ReadRenderLight( lighter );
	//HUMANHEAD PCF mdl 05/04/06 - Don't save light handles
	//savefile->ReadInt( lighterHandle );
	spiritProxy.Restore( savefile );
	savefile->ReadInt( lastWeaponSpirit );
	talon.Restore( savefile );
	savefile->ReadInt( nextTalonAttackCommentTime );
	savefile->ReadBool( bTalonAttackComment );
	savefile->ReadBool( bSpiritWalk );
	savefile->ReadBool( bDeathWalk );
	savefile->ReadBool( bReallyDead );
	guiWantsControls.Restore( savefile );
	savefile->ReadFloat( deathWalkFlash );
	savefile->ReadInt( deathWalkTime );
	savefile->ReadInt( deathWalkPower );
	savefile->ReadBool( bInDeathwalkTransition );
	savefile->ReadBool( bInCinematic );
	savefile->ReadBool( bPlayingLowHealthSound );
	savefile->ReadBool( bPossessed );
	savefile->ReadFloat( possessionTimer );
	savefile->ReadFloat( possessionFOV );
	savefile->ReadInt( preCinematicWeapon );
	savefile->ReadInt( preCinematicWeaponFlags );
	savefile->ReadInt( lastDamagedTime );
	savefile->ReadStaticObject( weaponHandState );
	hand.Restore( savefile );
	handNext.Restore( savefile );
	possessedTommy.Restore( savefile );

	savefile->ReadStaticObject( vehicleInterfaceLocal );
	SetVehicleInterface( &vehicleInterfaceLocal );

	hhVehicle *vehicle;
	savefile->ReadObject( reinterpret_cast<idClass *&> ( vehicle ) );
	if( vehicle ) {
		vehicle->RestorePilot( &vehicleInterfaceLocal );
	}

	deathLookAtEntity.Restore( savefile );
	savefile->ReadString( deathLookAtBone );
	savefile->ReadString( deathCameraBone );
	savefile->ReadStaticObject( spiritwalkSoundController );
	savefile->ReadStaticObject( deathwalkSoundController );
	savefile->ReadStaticObject( wallwalkSoundController );
	savefile->ReadBool( bShowProgressBar );
	savefile->ReadFloat( progressBarValue );

	float set;
	savefile->ReadFloat( set );			// idInterpolate<float>
	progressBarGuiValue.SetStartTime( set );
	savefile->ReadFloat( set );
	progressBarGuiValue.SetDuration( set );
	savefile->ReadFloat( set );
	progressBarGuiValue.SetStartValue(set);
	savefile->ReadFloat( set );
	progressBarGuiValue.SetEndValue( set );

	savefile->ReadInt( progressBarState );
	savefile->ReadBool( bClampYaw );
	savefile->ReadFloat( maxRelativeYaw );
	savefile->ReadFloat( maxRelativePitch );
	savefile->ReadFloat( bob );
	savefile->ReadInt( lastAppliedBobCycle );
	savefile->ReadInt( prevStepUpTime );
	savefile->ReadVec3( prevStepUpOrigin );
	savefile->ReadFloat( crashlandSpeed_fatal );
	savefile->ReadFloat( crashlandSpeed_soft );
	savefile->ReadFloat( crashlandSpeed_jump );
	// Protected vars
	savefile->ReadUserInterface( guiOverlay );

	thirdPersonCameraClipBounds.Restore( savefile );
	savefile->ReadFloat( viewAnglesSensitivity );
	savefile->ReadInt( lastResurrectTime );
	savefile->ReadInt( spiritDrainHeartbeatMS );
	savefile->ReadInt( ddaHeartbeatMS );

	savefile->ReadInt( spiritWalkToggleTime );
	savefile->ReadBool( bDeathWalkStage2 );
	savefile->ReadBool( bFrozen );
	savefile->ReadAngles( untransformedViewAngles );
	savefile->ReadMat3( untransformedViewAxis );
	savefile->ReadInt( nextSpiritTime );

	float startTime, accelTime, decelTime, duration, startPos, endPos;

	savefile->ReadFloat( startTime );
	savefile->ReadFloat( accelTime );
	savefile->ReadFloat( decelTime );
	savefile->ReadFloat( duration );
	savefile->ReadFloat( startPos );
	savefile->ReadFloat( endPos );
	cinematicFOV.Init( startTime, accelTime, decelTime, duration, startPos, endPos );
	savefile->ReadBool( bAllowSpirit );
	savefile->ReadInt( airAttackerTime );
	savefile->ReadBool( bScopeView );
	savefile->ReadInt( ddaNumEnemies );
	savefile->ReadFloat( ddaProbabilityAccum );
	savefile->ReadInt( weaponFlags );
	savefile->ReadBool( lockView );

	savefile->ReadBool( bCollidingWithPortal );
	savefile->ReadBool( bLotaTunnelMode );
	savefile->ReadInt( forcePredictionButtons );

	for (int ix=0; ix<MAX_TRACKED_ATTACKERS; ix++) {
		lastAttackers[ix].attacker.Restore( savefile );
		savefile->ReadInt( lastAttackers[ix].time );
		savefile->ReadBool( lastAttackers[ix].displayed );
	}

	// We don't want to preserve these
	memset( &oldCmdAngles, 0, sizeof( oldCmdAngles ) );

	kickSpring = spawnArgs.GetFloat( "kickSpring" );	//HUMANHEAD bjk
	kickDamping = spawnArgs.GetFloat( "kickDamping" );	//HUMANHEAD bjk

	if( InVehicle() ) {
		hhVehicle *vehicle = vehicleInterfaceLocal.GetVehicle();
		HH_ASSERT( vehicle );

		idMat3 axis;
		idVec3 axis0;

		// This prevents a crashbug if the vehicle's physics doesn't exist yet.
		savefile->ReadMat3( axis ); // vehicle->GetPhysics()->GetAxis()
		savefile->ReadVec3( axis0 ); // vehicle->GetAxis()[0]

		// Keep orientation correct in vehicles
		RestoreOrientation( GetOrigin(), axis, axis0, axis0.ToAngles() );

		//HUMANHEAD PCF mdl 05/02/06 - Added this to re-enable shuttle view
		if (renderSystem) {
			// Set shuttle view
			renderSystem->SetShuttleView( true );
		}
	} else {
		// Keep orientation correct for walkwalk and gravity rooms
		RestoreOrientation( GetOrigin(), physicsObj.GetAxis(), viewAngles.ToMat3()[0], untransformedViewAngles );
	}

	//HUMANHEAD PCF mdl 04/28/06 - Moved camera interpolater down here to fix jump off wallwalk view angle problem
	cameraInterpolator.Restore( savefile );

	//HUMANHEAD PCF mdl 05/04/06 - Restore lighter
	bool bLighter;
	savefile->ReadBool( bLighter );
	if ( bLighter ) {
		lighterHandle = gameRenderWorld->AddLightDef( &lighter );
	}
}

int hhPlayer::GetSpiritPower() {
	ammo_t ammo_spiritpower = idWeapon::GetAmmoNumForName("ammo_spiritpower");
	return inventory.ammo[ammo_spiritpower];
}

void hhPlayer::SetSpiritPower(int amount) {
	ammo_t ammo_spiritpower = idWeapon::GetAmmoNumForName("ammo_spiritpower");
	inventory.ammo[ammo_spiritpower] = amount;
	if (inventory.ammo[ammo_spiritpower] > inventory.maxSpirit) {
		inventory.ammo[ammo_spiritpower] = inventory.maxSpirit;
	}
}

void hhPlayer::Event_StartHUDTranslation() {
	vehicleInterfaceLocal.StartHUDTranslation();
}

void hhPlayer::Freeze(float unfreezeDelay) {
	bFrozen = true;
	StopFiring();
	if ( weapon.IsValid() ) {
		weapon->PutAway();
	}
	if ( unfreezeDelay > 0.0f ) {
		PostEventSec( &EV_Unfreeze, unfreezeDelay );
	}
}

void hhPlayer::Unfreeze(void) {
	bFrozen = false;
	if ( weapon.IsValid() ) {
		weapon->PostEventMS( &EV_Show, 1000 );
		weapon->PostEventMS( &EV_Weapon_WeaponRising, 1000 );
	}
}

void hhPlayer::Event_Unfreeze() {
	Unfreeze();
	if ( bindMaster && bindMaster->RespondsTo( EV_BindUnfroze ) ) {
		GetBindMaster()->PostEventMS( &EV_BindUnfroze, 0, this );
	}
}

void hhPlayer::Event_GetSpiritPower() { //rww
	idThread::ReturnFloat((float)GetSpiritPower());
}

void hhPlayer::Event_SetSpiritPower(const float s) { //rww
	SetSpiritPower((int)s);
}

void hhPlayer::Event_OnGround() { // bg
	idThread::ReturnFloat( physicsObj.HasGroundContacts() );
}

void hhPlayer::SetupWeaponFlags( void ) {
	const char *mapName = gameLocal.serverInfo.GetString( "si_map" );
	const idDecl *mapDecl = declManager->FindType(DECL_MAPDEF, mapName, false );
	if ( !mapDecl ) {
		weaponFlags = -1; // No map decls?  Allow all weapons
		return;
	}
	const idDeclEntityDef *mapInfo = static_cast<const idDeclEntityDef *>(mapDecl);

	const char *weaponList = mapInfo->dict.GetString( "disableWeapons" );
	weaponFlags = 0;
	if ( weaponList && weaponList[0] ) {
		if ( !idStr::Icmp( weaponList, "all" ) ) {
			// No weapons are enabled, so just return
			return;
		}

		if ( idStr::FindText( weaponList, "wrench", false ) == -1 ) {
			weaponFlags |= HH_WEAPON_WRENCH;
		}
		if ( idStr::FindText( weaponList, "rifle", false ) == -1 ) {
			weaponFlags |= HH_WEAPON_RIFLE;
		}
		if ( idStr::FindText( weaponList, "crawler", false ) == -1 ) {
			weaponFlags |= HH_WEAPON_CRAWLER;
		}
		if ( idStr::FindText( weaponList, "autocannon", false ) == -1 ) {
			weaponFlags |= HH_WEAPON_AUTOCANNON;
		}
		if ( idStr::FindText( weaponList, "hiderweapon", false ) == -1 ) {
			weaponFlags |= HH_WEAPON_HIDERWEAPON;
		}
		if ( idStr::FindText( weaponList, "rocketlauncher", false ) == -1 ) {
			weaponFlags |= HH_WEAPON_ROCKETLAUNCHER;
		}
		if ( idStr::FindText( weaponList, "soulstripper", false ) == -1 ){ 
			weaponFlags |= HH_WEAPON_SOULSTRIPPER;
		}
	}

	if ( weaponFlags == 0 ) {
		// Allow all weapons by default
		weaponFlags = -1;
	}
}

void hhPlayer::LockWeapon( int weaponNum ) {
	// Special case, -1 locks all weapons AND the lighter
	if ( weaponNum == -1 ) {
		if (IsLighterOn()) {
			LighterOff();
		}
		if (weapon.IsValid()) {
			weapon->PutAway();
		}
		weaponFlags = 0;
		idealWeapon = 0;
		currentWeapon = 0;
		return;
	}

	if ( weaponNum <= 0 || weaponNum >= MAX_WEAPONS ) {
		gameLocal.Error( "Attempted to unlock unknown weapon '%d'", weaponNum );
	}

	int flag = ( 1 << ( weaponNum - 1 ) ); 
	if ( weaponFlags & flag ) { // Only lock if not already locked
		weaponFlags &= ~flag;
		if ( idealWeapon == weaponNum ) {
			if (weapon.IsValid()) {
				weapon->PutAway();
			}
			NextWeapon();
		}
	}
}

void hhPlayer::UnlockWeapon( int weaponNum ) {
	// Special case, -1 unlocks all weapons
	if ( weaponNum == -1 ) {
		weaponFlags = -1;
		if ( idealWeapon == 0 )  {
			idealWeapon = 1; // Default to the wrench
		}
		return;
	}

	if ( weaponNum <= 0 || weaponNum >= MAX_WEAPONS ) {
		gameLocal.Error( "Attempted to unlock unknown weapon '%d'", weaponNum );
	}

	int flag = ( 1 << ( weaponNum - 1 ) ); 
	if ( weaponFlags & flag ) {
		// Already unlocked
		return;
	}

	weaponFlags |= flag;
	idealWeapon = weaponNum;
}

bool hhPlayer::IsLocked(int weaponNum) {
	//HUMANHEAD PCF mdl 05/05/06 - Changed to < 1 to catch weaponNum = -1
	if (weaponNum < 1) {
		return true; // CJR: if the player has no weapons, then consider all weapons locked
	}
	return !(weaponFlags & (1 << (weaponNum-1)));
}

void hhPlayer::Event_LockWeapon( int weaponNum ) {
	LockWeapon( weaponNum );
}

void hhPlayer::Event_UnlockWeapon( int weaponNum ) {
	UnlockWeapon( weaponNum );
}

//HUMANHEAD rdr
void hhPlayer::Event_SetPrivateCameraView( idEntity *camView, int noHide ) {
	if ( camView && camView->IsType( idCamera::Type ) ) {
		idPlayer::SetPrivateCameraView( static_cast<idCamera*>( camView ), noHide != 0 );
	}
	else {
		idPlayer::SetPrivateCameraView( NULL );
	}
}

//HUMANHEAD rdr
void hhPlayer::Event_SetCinematicFOV( float fieldOfView, float accelTime, float decelTime, float duration ) {
	if ( duration > 0.f ) {
		cinematicFOV.Init( gameLocal.time, SEC2MS( accelTime ), SEC2MS( decelTime ), SEC2MS( duration ), cinematicFOV.GetCurrentValue( gameLocal.GetTime() ), fieldOfView );
	} else {
		cinematicFOV.Init( gameLocal.time, 0.f, 0.f, 0.f, cinematicFOV.GetEndValue(), fieldOfView );
	}
}

//HUMANHEAD rww
void hhPlayer::Event_StopSpiritWalk() {
	StopSpiritWalk();
}

void hhPlayer::Event_DamagePlayer(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, char *damageDefName, float damageScale, int location) {
	Damage(inflictor, attacker, dir, damageDefName, damageScale, location);
}
//HUMANHEAD END

void hhPlayer::Event_GetSpiritProxy() {
	if ( spiritProxy.IsValid() ) {
		idThread::ReturnEntity( spiritProxy.GetEntity() );
	} else {
		idThread::ReturnEntity( NULL );
	}
}

void hhPlayer::Event_IsSpiritWalking() {
	idThread::ReturnInt( IsSpiritWalking() ? 1 : 0 );
}

void hhPlayer::Event_IsDeathWalking() {
	idThread::ReturnInt( IsDeathWalking() ? 1 : 0 );
}

void hhPlayer::Event_AllowLighter( bool allow ) {
	inventory.requirements.bCanUseLighter = allow;
	if ( !allow && IsLighterOn() ) {
		LighterOff();
	}
}

//=============================================================================
//
// hhPlayer::Event_GetDDAValue
//
// Returns the current DDA value from zero to one
//=============================================================================

void hhPlayer::Event_GetDDAValue() {
	idThread::ReturnFloat( gameLocal.GetDDAValue() );
}

//HUMANHEAD rww
hhArtificialPlayer::hhArtificialPlayer(void) {
	memset(&lastAICmd, 0, sizeof(lastAICmd));
	testCrouchActive = false;
	testCrouchTime = 0;
}

void hhArtificialPlayer::Spawn( void ) {
	idDict apUI;
	gameLocal.GetAPUserInfo(apUI, entityNumber);
	gameLocal.SetUserInfo(entityNumber, apUI, gameLocal.isClient, false);

	SetPlayerModel(false); //force an update on the player model
}

void hhArtificialPlayer::Think( void ) {
	PROFILE_START("hhArtificialPlayer::Think", PROFMASK_NORMAL);
	bool iFeelHoppy = false;
	usercmd_t *nextCmd = &gameLocal.usercmds[ entityNumber ];
	idAngles idealAngles = GetViewAngles();

	nextCmd->gameFrame = gameLocal.framenum;
	nextCmd->gameTime = gameLocal.time;

	nextCmd->forwardmove = idMath::ClampChar(127);

	//this is not meant to resemble proper ai logic, it's just for testing.
	idEntity *closestEnt = NULL;
	float closestDist = idMath::INFINITY;	

	int numSourceAreas, sourceAreas[ idEntity::MAX_PVS_AREAS ];
	numSourceAreas = gameRenderWorld->BoundsInAreas( GetPlayerPhysics()->GetAbsBounds(), sourceAreas, idEntity::MAX_PVS_AREAS );
	pvsHandle_t pvsHandle = gameLocal.pvs.SetupCurrentPVS( sourceAreas, numSourceAreas, PVS_NORMAL );

	for(int i=0;i<gameLocal.numClients;i++) {
		idEntity *ent = gameLocal.entities[i];		
		if(ent) {
			HH_ASSERT(ent->IsType(hhPlayer::Type));
			hhPlayer *plEnt = static_cast<hhPlayer *>(ent);

			float l = (ent->GetOrigin()-GetOrigin()).Length();
			if(l < 4096.0f && (l < closestDist || !closestEnt) && ent->PhysicsTeamInPVS(pvsHandle)) {
				trace_t tr;

				gameLocal.clip.TracePoint(tr, GetOrigin(), ent->GetOrigin(), GetPhysics()->GetClipMask(), this);
				if (tr.c.entityNum == ent->entityNumber) { //if we hit the thing then it's visible.
					if (ent->health > 0) {
						if (gameLocal.gameType != GAME_TDM || plEnt->team != team) {
							closestDist = l;
							closestEnt  = ent;
						}
					}
				}
			}
		}
	}

	gameLocal.pvs.FreeCurrentPVS( pvsHandle );

	if (closestEnt) {
		idealAngles = (closestEnt->GetOrigin()-GetOrigin()).ToAngles();
	}
	else { //if no one to run mindlessly at, do some stuff.
		const float testDist = 64.0f;
		idVec3 fwd = GetOrigin()+(idealAngles.ToForward()*testDist);

		trace_t tr;
		gameLocal.clip.TraceBounds(tr, GetOrigin(), fwd, GetPhysics()->GetBounds(), GetPhysics()->GetClipMask(), this);
		if (tr.fraction != 1.0f) { //if we hit something set the yaw to a new random angle.
			iFeelHoppy = true;

			idealAngles.yaw = rand()%360;
		}
	}

	if ((closestEnt || health <= 0) && rand()%2 == 1) {
		nextCmd->buttons |= BUTTON_ATTACK;
	}
	else {
		nextCmd->buttons &= ~BUTTON_ATTACK;
	}

	if (testCrouchTime < gameLocal.time) {
		testCrouchTime = gameLocal.time + rand()%4000;
		testCrouchActive = !testCrouchActive;
	}

	if (iFeelHoppy) {
		nextCmd->upmove = idMath::ClampChar(127);
	}
	else {
		if (testCrouchActive) {
			nextCmd->upmove = idMath::ClampChar(-127);
		}
		else {
			nextCmd->upmove = 0;
		}
	}

	idealAngles.pitch = idMath::AngleNormalize180(idealAngles.pitch);
	idealAngles.yaw = idMath::AngleNormalize180(idealAngles.yaw);
	idealAngles.roll = idMath::AngleNormalize180(idealAngles.roll);
	SetUntransformedViewAngles(idAngles(0.0f, 0.0f, 0.0f));
	UpdateDeltaViewAngles(idealAngles);
	UpdateOrientation(idealAngles);
	PROFILE_STOP("hhArtificialPlayer::Think", PROFMASK_NORMAL);
	hhPlayer::Think();
}

void hhArtificialPlayer::ClientPredictionThink( void ) {
	gameLocal.usercmds[entityNumber] = lastAICmd; //copy over the last manually snapshotted usercmd first
	hhPlayer::ClientPredictionThink();
}

void hhArtificialPlayer::WriteToSnapshot( idBitMsgDelta &msg ) const {
	//instead of sync'ing, just mirror what the server has in ::Spawn
	/*
	msg.WriteDict(*gameLocal.GetUserInfo(entityNumber));	//ap's don't broadcast their info to the server, so the server otherwise
															//will never broadcast it out to other clients
	*/

	hhPlayer::WriteToSnapshot(msg);

	usercmd_t &cmd = gameLocal.usercmds[entityNumber];
	msg.WriteLong( cmd.gameTime );
	msg.WriteByte( cmd.buttons );
    msg.WriteShort( cmd.mx );
	msg.WriteShort( cmd.my );
	msg.WriteChar( cmd.forwardmove );
	msg.WriteChar( cmd.rightmove );
	msg.WriteChar( cmd.upmove );
	msg.WriteShort( cmd.angles[0] );
	msg.WriteShort( cmd.angles[1] );
	msg.WriteShort( cmd.angles[2] );

	msg.WriteBits(spectating, 1);
}

void hhArtificialPlayer::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	/*
	((idBitMsgDelta)msg).ReadDict(*((idDict *)gameLocal.GetUserInfo(entityNumber)));
	if (!clientReceivedUI) {
		UserInfoChanged( false );
		clientReceivedUI = true;
	}
	*/

	hhPlayer::ReadFromSnapshot(msg);

	usercmd_t &cmd = lastAICmd;
	cmd.gameTime = msg.ReadLong();
    cmd.buttons = msg.ReadByte();
    cmd.mx = msg.ReadShort();
	cmd.my = msg.ReadShort();
	cmd.forwardmove = msg.ReadChar();
	cmd.rightmove = msg.ReadChar();
	cmd.upmove = msg.ReadChar();
	cmd.angles[0] = msg.ReadShort();
	cmd.angles[1] = msg.ReadShort();
	cmd.angles[2] = msg.ReadShort();

	bool spec = !!msg.ReadBits(1);	//doesn't go through right for ap's or something based on events sometimes, hack fix
	if (spec != spectating) {
		Spectate(spec);
	}
}
//HUMANHEAD END

void hhPlayer::DisableSpiritWalk(int timeout) {
	nextSpiritTime = gameLocal.time + SEC2MS(timeout);
	StopSpiritWalk();
}

//=============================================================================
//
// hhPlayer::Event_UpdateDDA
//
// CJR:  Update the probability the player will die this tick, and 
// adjust the difficulty accordingly
//=============================================================================

void hhPlayer::Event_UpdateDDA() {
	idEntity	*ent;
	int			updateFlags;

	// Find the number of alive, non-dormant creatures attacking the player
	ddaNumEnemies = 0;
	ddaProbabilityAccum = 1.0f;
	updateFlags = 0;

	for( ent = gameLocal.activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
		if ( ent->fl.isDormant || !ent->IsType( hhMonsterAI::Type ) || ent->fl.hidden || ent->health <= 0 ) {
			continue;
		}

		hhMonsterAI *monster = static_cast<hhMonsterAI *>(ent);
		if ( monster->GetEnemy() != this ) {
			continue;
		}

		// PVS Check:  only include creatures that the player can attack or be attacked by
		if ( !gameLocal.InPlayerPVS( ent ) ) {
			continue;
		}

		// Accumulate the survival rate against each enemy
		int index = monster->spawnArgs.GetInt( "ddaIndex", "0" );

		if ( index == -1 ) { // Skip certain monsters, such as the vacuum or the crawlers
			continue;
		}
			
		float prob = gameLocal.GetDDA()->DDA_GetProbability( index, GetHealth() );

		ddaProbabilityAccum *= prob;

		// Recompute the actual difficulty based upon the creatures in view
		updateFlags |= ( 1 << index );

		ddaNumEnemies++;
	}

	gameLocal.GetDDA()->RecalculateDifficulty( updateFlags );

	PostEventSec( &EV_UpdateDDA, spawnArgs.GetFloat( "updateDDARate", "0.25" ) );

	if ( g_printDDA.GetBool() ) {
		if ( ddaNumEnemies > 0 ){
			common->Printf("Probability [%.2f]\n", ddaProbabilityAccum );
		}
	}
}

void hhPlayer::Event_DisableSpirit(void) {
	// HUMANHEAD PCF pdm 05-17-06: Removed assert, as scripters were potentially calling during deathwalk
	// HUMANHEAD PCF pdm 05-17-06: Only exit spirit realm if spiritwalking, not deathwalking

	if ( IsSpiritWalking() ) {
		StopSpiritWalk();
	}

	bAllowSpirit = false;
}

void hhPlayer::Event_EnableSpirit(void) {
	// HUMANHEAD PCF pdm 05-17-06: Removed assert

	bAllowSpirit = true;
}

//HUMANHEAD bjk
void hhPlayer::ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material ) {
	// no need for these in sp
	if( gameLocal.isMultiplayer ) {
		idActor::ProjectOverlay( origin, dir, size, material );
	}
}

// HUMANHEAD mdl:  Ripped from idAI for short invulnerability after returning from deathwalk
void hhPlayer::Event_AllowDamage( void ) {
	fl.takedamage = true;
}

void hhPlayer::Event_IgnoreDamage( void ) {
	fl.takedamage = false;
}

// Precompute all the weapons' info for faster HUD updates
void hhPlayer::SetupWeaponInfo() {
	const char *fireInfoName = NULL;
	const idDeclEntityDef *decl = NULL;
	const char *ammoName = NULL;
	float maxAmmo;

	memset(weaponInfo, 0, sizeof(weaponInfo_t)*15);
	memset(altWeaponInfo, 0, sizeof(weaponInfo_t)*15);

	for (int ix=0; ix<15; ix++) {
		const char *weaponClassName = spawnArgs.GetString( va( "def_weapon%d", ix ), NULL );
		if ( weaponClassName && *weaponClassName ) {

			const idDeclEntityDef *weaponObjDecl = gameLocal.FindEntityDef( weaponClassName, false );
			if ( weaponObjDecl ) {
				fireInfoName = weaponObjDecl->dict.GetString("def_fireInfo");
				decl = gameLocal.FindEntityDef( fireInfoName, false );
				if ( decl ) {
					weaponInfo[ix].ammoType = inventory.AmmoIndexForAmmoClass( decl->dict.GetString( "ammoType" ) );
					weaponInfo[ix].ammoRequired = decl->dict.GetInt("ammoRequired");
					weaponInfo[ix].ammoLow = decl->dict.GetInt("lowAmmo");

					ammoName = idWeapon::GetAmmoNameForNum( weaponInfo[ix].ammoType );
					maxAmmo = inventory.MaxAmmoForAmmoClass( this, ammoName );
					weaponInfo[ix].ammoMax = max(1, maxAmmo);
				}

				fireInfoName = weaponObjDecl->dict.GetString("def_altFireInfo");
				decl = gameLocal.FindEntityDef( fireInfoName, false );
				if ( decl ) {
					altWeaponInfo[ix].ammoType = inventory.AmmoIndexForAmmoClass( decl->dict.GetString( "ammoType" ) );
					altWeaponInfo[ix].ammoRequired = decl->dict.GetInt("ammoRequired");
					altWeaponInfo[ix].ammoLow = decl->dict.GetInt("lowAmmo");

					ammoName = idWeapon::GetAmmoNameForNum( altWeaponInfo[ix].ammoType );
					maxAmmo = inventory.MaxAmmoForAmmoClass( this, ammoName );
					altWeaponInfo[ix].ammoMax = max(1, maxAmmo);
				}
			}
		}
	}
}

/*
===============
hhPlayer::ThrowGrenade
HUMANHEAD bjk
===============
*/
void hhPlayer::ThrowGrenade( void ) {
	idMat3 axis;
	idVec3 muzzle;

	if( inventory.HasAmmo(inventory.AmmoIndexForAmmoClass( "ammo_crawler" ), 1) == false )
		return;

	if (IsSpiritOrDeathwalking() || InVehicle() || bReallyDead) {
		return;
	}

	if ( privateCameraView || !weaponEnabled || spectating || gameLocal.inCinematic || health < 0 || gameLocal.isClient )
		return;

	if( weapon.IsValid() ) {
		if ( inventory.weapons & ( 1 << 12 ) && currentWeapon != 12 && currentWeapon != 3 && idealWeapon == currentWeapon ) {
			idealWeapon = 12;
			previousWeapon = currentWeapon;
		}
		else if( currentWeapon == 3 ) {
			FireWeapon();
			usercmd.buttons = usercmd.buttons | BUTTON_ATTACK;
		}
	}
}

//HUMANHEAD bjk
void hhPlayer::Event_ReturnToWeapon() {
	const char *weap;

	if ( !weaponEnabled || spectating || gameLocal.inCinematic || health < 0 ) {
		idThread::ReturnInt(0);
		return;
	}

	if ( ( previousWeapon < 0 ) || ( previousWeapon >= MAX_WEAPONS ) ) {
		idThread::ReturnInt(0);
		return;
	}

	if ( gameLocal.isClient ) {
		idThread::ReturnInt(0);
		return;
	}

	weap = spawnArgs.GetString( va( "def_weapon%d", previousWeapon ) );
	if ( !weap[ 0 ] ) {
		gameLocal.Printf( "Invalid weapon\n" );
		idThread::ReturnInt(0);
		return;
	}

	if ( inventory.weapons & ( 1 << previousWeapon ) ) {
		idealWeapon = previousWeapon;
	}

	idThread::ReturnInt(1);
}

//HUMANHEAD rww
void hhPlayer::Event_CanAnimateTorso(void) {
	if (!gameLocal.isMultiplayer) { //don't worry about it
		idThread::ReturnInt(1);
		return;
	}

	if (!weapon.IsValid() || !weapon->IsType(hhWeapon::Type)) { //bad
		idThread::ReturnInt(0);
		return;
	}

	idThread::ReturnInt(1);
}

void hhPlayer::Show(void) {
	idActor::Show();

	hhWeapon *weap;
	weap = weapon.GetEntity();
	if ( weap ) {
		if (!IsLocked(currentWeapon)) {
			weap->ShowWorldModel();
		} else {
			weap->HideWorldModel();
		}
	}
}
