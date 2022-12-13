#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_KillBeam( "killBeam", NULL, NULL );

CLASS_DECLARATION( hhWeaponFireController, hhSoulStripperAltFireController )
END_CLASS

/***********************************************************************

  hhSoulStripperAltFireController::LaunchProjectiles
	
***********************************************************************/
void hhSoulStripperAltFireController::LaunchProjectiles( const idVec3& launchOrigin, const idMat3& aimAxis, const idVec3& pushVelocity, idEntity* projOwner ) {
	// Spawn minion
	idDict args;

	idVec3 minionOffset = dict->GetVector( "minionOffset" );
	args.SetVector( "origin", launchOrigin + aimAxis * minionOffset );
	args.SetMatrix( "rotation", aimAxis );
	const char *minionDef = dict->GetString( "def_minion" );
	gameLocal.SpawnObject( minionDef, &args );

	// Spawn blast effect
	hhFxInfo fxInfo;
	fxInfo.SetNormal( aimAxis[0] );
	fxInfo.RemoveWhenDone( true );
	self->BroadcastFxInfo( dict->GetString( "fx_blastflash" ), launchOrigin, aimAxis, &fxInfo );
}

CLASS_DECLARATION( hhWeaponFireController, hhBeamBasedFireController )
END_CLASS

/***********************************************************************

  hhBeamBasedFireController::LaunchProjectiles
	
***********************************************************************/
void hhBeamBasedFireController::LaunchProjectiles( const idVec3& launchOrigin, const idMat3& aimAxis, const idVec3& pushVelocity, idEntity* projOwner ) {
	idVec3		start, end;
	trace_t		results;

	float			traceLength;

	idVec3			boneOrigin;
	idMat3			boneAxis;

	if ( projTime < gameLocal.time ) {
		hhFireController::LaunchProjectiles(launchOrigin, aimAxis, pushVelocity, projOwner);
		projTime = gameLocal.time + dict->GetInt("projRate", "100");
	}

	start = launchOrigin;

	traceLength = dict->GetFloat("traceLength","100");

	end = start + traceLength * aimAxis[0];

	// Check to see if the beam should attempt to hit something
	if (owner.IsValid()) {
		//rww - can hit owners due to different angles in mp (although not the local client since the trace is done on the client every frame)
		gameLocal.clip.TracePoint( results, start, end, MASK_SHOT_RENDERMODEL | CONTENTS_GAME_PORTAL, owner.GetEntity() );
	}
	else {
		gameLocal.clip.TracePoint( results, start, end, MASK_SHOT_RENDERMODEL | CONTENTS_GAME_PORTAL, NULL );
	}

	if ( !shotbeam.IsValid() ) {
		shotbeam = hhBeamSystem::SpawnBeam( launchOrigin, dict->GetString( "beam" ), aimAxis, true );
		if( !shotbeam.IsValid() )
			return;

		if (self->GetOwner()) {
			shotbeam->snapshotOwner = self->GetOwner(); //rww - this beam is client-local, but will remove itself when the gun is not actively in the snapshot (meaning, it's not actively updated)
		}
	}

	shotbeam->Activate( true );

	hhFxInfo fxInfo;
	fxInfo.SetNormal( results.c.normal );
	fxInfo.RemoveWhenDone( true );

	if( !impactFx.IsValid() ) {
		const char* str=dict->GetString("fx_impact");
		if ( str && str[0] )
			impactFx=self.GetEntity()->SpawnFxLocal( str, results.endpos, results.c.normal.ToMat3(), &fxInfo, true );
	}

	if( impactFx.IsValid() ) {
		if (self.IsValid() && self->GetOwner()) {
			impactFx->snapshotOwner = self->GetOwner(); //rww
		}
		static_cast<hhEntityFx*>( impactFx.GetEntity() )->SetFxInfo(fxInfo);
		impactFx->Nozzle(true);
		impactFx->SetOrigin(results.endpos-aimAxis[0]*20);
	}

	idVec3 beamOrigin;
	idMat3 beamAxis;
	CalculateMuzzlePosition( beamOrigin, beamAxis ); // Must use this instead of launchOrigin, since launch origin is forced into the player's bbox

	if (gameLocal.isMultiplayer) { //rww - the visual origin of the beam needs to be from the player's world weapon joint to avoid the optic blast
		idVec3 tpOrigin;
		idMat3 tpAxis;
		if (CheckThirdPersonMuzzle(tpOrigin, tpAxis)) {
			beamOrigin = tpOrigin;
			//just keep the existing axis
		}
	}
	shotbeam->SetOrigin( beamOrigin );
	shotbeam->SetAxis( aimAxis );
	shotbeam->SetTargetLocation( results.endpos-aimAxis[0]*40 );
	float length = (results.endpos - start).Length();
	shotbeam->SetBeamOffsetScale( length/150.0f );

	GetSelf()->CancelEvents( &EV_KillBeam );
	GetSelf()->PostEventSec( &EV_KillBeam, dict->GetFloat( "beamTime", "0.1" ) );
}

void hhBeamBasedFireController::KillBeam()
{
	if( impactFx.IsValid() ) {
		impactFx->Nozzle(false);
		if (gameLocal.isClient) {
			impactFx->snapshotOwner = NULL; //rww - prevent it from being unhidden between now and the removal frame on the client
		}
	}

	if( shotbeam.IsValid() )
		shotbeam->Activate( false );
}

void hhBeamBasedFireController::Think()
{
	if( shotbeam.IsValid() ) {
		idVec3 beamOrigin;
		idMat3 beamAxis;
		CalculateMuzzlePosition( beamOrigin, beamAxis ); // Must use this instead of launchOrigin, since launch origin is forced into the player's bbox
		idVec3 adjustedOrigin = AssureInsideCollisionBBox( beamOrigin, GetSelf()->GetAxis(), GetCollisionBBox(), projectileMaxHalfDimension );
		idMat3 aimAxis = DetermineAimAxis( adjustedOrigin, GetSelf()->GetAxis() );

		if (gameLocal.isMultiplayer) { //rww - the visual origin of the beam needs to be from the player's world weapon joint to avoid the optic blast
			idVec3 tpOrigin;
			idMat3 tpAxis;
			if (CheckThirdPersonMuzzle(tpOrigin, tpAxis)) {
				beamOrigin = tpOrigin;
				//just keep the existing axis
			}
		}
		shotbeam->SetOrigin( beamOrigin );
		shotbeam->SetAxis( aimAxis );
	}
}

/***********************************************************************

  hhBeamBasedFireController::Init
	
***********************************************************************/
void hhBeamBasedFireController::Init( const idDict* viewDict, hhWeapon* self, hhPlayer* owner )
{
	hhWeaponFireController::Init( viewDict, self, owner );

	shotbeam = NULL;
	projTime = 0;
}

/***********************************************************************

  hhBeamBasedFireController::~hhBeamBasedFireController
	
***********************************************************************/
hhBeamBasedFireController::~hhBeamBasedFireController()
{
	KillBeam();
	if( impactFx.IsValid() )
		SAFE_REMOVE(impactFx);
	if( shotbeam.IsValid() )
		SAFE_REMOVE(shotbeam);
}

void hhBeamBasedFireController::Save( idSaveGame *savefile ) const {
	shotbeam.Save( savefile );
	savefile->WriteInt( projTime );
	impactFx.Save( savefile );
}

void hhBeamBasedFireController::Restore( idRestoreGame *savefile ) {
	shotbeam.Restore( savefile );
	savefile->ReadInt( projTime );
	impactFx.Restore( savefile );
}

CLASS_DECLARATION( hhBeamBasedFireController, hhSunbeamFireController )
END_CLASS

void hhSunbeamFireController::LaunchProjectiles( const idVec3& launchOrigin, const idMat3& aimAxis, const idVec3& pushVelocity, idEntity* projOwner ) {
	hhBeamBasedFireController::LaunchProjectiles(launchOrigin, aimAxis, pushVelocity, projOwner);
	//HUMANHEAD PCF rww 05/18/06 - since values are assuming 60hz, make them relative
	float backpush = dict->GetFloat("backpush","0.0")*(60.0f/(float)USERCMD_HZ);
	float slowwalk = dict->GetFloat("slowwalk","0.0")*(60.0f/(float)USERCMD_HZ);
	int knockback = dict->GetInt("knockback","0")*(60/USERCMD_HZ);
	//HUMANHEAD END

	hhPhysics_Player* pp = static_cast<hhPhysics_Player*>(owner->GetPlayerPhysics());
	idVec3 v = pp->GetLinearVelocity()+owner->GetAxis()[0]*backpush;
	float f = pp->GetLinearVelocity()*owner->GetAxis()[0];
	v += owner->GetAxis()[0] * ( (f>0) ? slowwalk*f : 0 );
	pp->SetLinearVelocity(v);
	pp->SetKnockBack(knockback);
}

void hhSunbeamFireController::Think()
{
	idVec3 start, end;

	if( shotbeam.IsValid() ) {
		start = shotbeam->GetOrigin();
		end = shotbeam->GetTargetLocation();
		float length = (end - start).Length();

		idVec3 beamOrigin;
		idMat3 beamAxis;
		CalculateMuzzlePosition( beamOrigin, beamAxis ); // Must use this instead of launchOrigin, since launch origin is forced into the player's bbox
		idVec3 adjustedOrigin = AssureInsideCollisionBBox( beamOrigin, GetSelf()->GetAxis(), GetCollisionBBox(), projectileMaxHalfDimension );
		idMat3 aimAxis = DetermineAimAxis( adjustedOrigin, GetSelf()->GetAxis() );
		end = beamOrigin + length * aimAxis[0];

		if (gameLocal.isMultiplayer) { //rww - the visual origin of the beam needs to be from the player's world weapon joint to avoid the optic blast
			idVec3 tpOrigin;
			idMat3 tpAxis;
			if (CheckThirdPersonMuzzle(tpOrigin, tpAxis)) {
				beamOrigin = tpOrigin;
				//just keep the existing axis
			}
		}
		shotbeam->SetOrigin( beamOrigin );
		shotbeam->SetAxis( aimAxis );
		shotbeam->SetTargetLocation( end );

		if( impactFx.IsValid() )
			impactFx->SetOrigin( end );
	}
}

CLASS_DECLARATION( hhWeaponFireController, hhPlasmaFireController )
END_CLASS

//=============================================================================
//
// hhPlasmaFireController::LaunchProjectiles
//
//=============================================================================
bool hhPlasmaFireController::LaunchProjectiles( const idVec3& pushVelocity ) {
	char* projStr = "projectile_plasma";

	fireDelay = dict->GetFloat( "fireRate" );
	spread = DEG2RAD( dict->GetFloat("spread") );
	yawSpread = dict->GetFloat("yawSpread");

	usercmd_t cmd;
	idAngles temp;
	owner->GetPilotInput(cmd,temp);

	idVec3 dir(cmd.forwardmove, -cmd.rightmove, cmd.upmove);

	if( dir.x > 0 ) {
		projStr = "projectile_plasmaf";
		fireDelay = 0.1f;
	}
	
	if( dir.x < 0 ) {
		spread = DEG2RAD(10);
		fireDelay = 0.08f;
		projStr = "projectile_plasmab";
	}

	if( dir.y != 0 ) {
		spread *= 0.4f;
		yawSpread +=8;
	}

	if( dir.z < 0 ) {		//crouch
		spread = DEG2RAD(0);
		fireDelay = 1.0f;
		projStr = "projectile_plasmac";
	}

	if( !owner->AI_ONGROUND ) {		//jump
		fireDelay = 0.06f;
		projStr = "projectile_plasmaj";
	}

	projectile = gameLocal.FindEntityDefDict( projStr, false );

	return hhWeaponFireController::LaunchProjectiles( pushVelocity );
}

/***********************************************************************

  hhWeaponSoulStripper
	
***********************************************************************/

//const idEventDef EV_DamageBeamTarget( "<damageBeamTarget>", NULL );
const idEventDef EV_GetAnimPostFix( "getAnimPostfix", "", 's' );
const idEventDef EV_Leech( "leech", NULL, 'd' );
const idEventDef EV_EndLeech( "endLeech", NULL, 'f' );
const idEventDef EV_LightFadeIn( "<lightFadeIn>", "f" );
const idEventDef EV_LightFadeOut( "<lightFadeOut>", "f" );
const idEventDef EV_LightFade( "lightFade", "fff" );
const idEventDef EV_GetFireFunction( "getFireFunction", NULL, 's' );

CLASS_DECLARATION( hhWeapon, hhWeaponSoulStripper )
	EVENT( EV_PostSpawn,			hhWeaponSoulStripper::Event_PostSpawn )
	EVENT( EV_GetAnimPostFix,		hhWeaponSoulStripper::Event_GetAnimPostFix )
	EVENT( EV_Leech,				hhWeaponSoulStripper::Event_Leech )
	EVENT( EV_EndLeech,				hhWeaponSoulStripper::Event_EndLeech )
	EVENT( AI_PlayAnim,				hhWeaponSoulStripper::Event_PlayAnim )
	EVENT( AI_PlayCycle,			hhWeaponSoulStripper::Event_PlayCycle )
	EVENT( EV_LightFadeIn,			hhWeaponSoulStripper::Event_LightFadeIn )
	EVENT( EV_LightFadeOut,			hhWeaponSoulStripper::Event_LightFadeOut )
	EVENT( EV_LightFade,			hhWeaponSoulStripper::Event_LightFade )
	EVENT( EV_GetFireFunction,		hhWeaponSoulStripper::Event_GetFireFunction )
	EVENT( EV_KillBeam,				hhWeaponSoulStripper::Event_KillBeam )
END_CLASS

//=============================================================================
//
// hhWeaponSoulStripper::hhWeaponSoulStripper
//
//=============================================================================

hhWeaponSoulStripper::hhWeaponSoulStripper() {
	// Initialize SFX on the weapon
	beam = NULL; // Beam is properly spawned in PostSpawn
	beamCanA1 = NULL; // Beam is properly spawned in PostSpawn
	beamCanB1 = NULL; // Beam is properly spawned in PostSpawn
	beamCanC1 = NULL; // Beam is properly spawned in PostSpawn
	beamCanA2 = NULL; // Beam is properly spawned in PostSpawn
	beamCanB2 = NULL; // Beam is properly spawned in PostSpawn
	beamCanC2 = NULL; // Beam is properly spawned in PostSpawn
	beamCanA3 = NULL; // Beam is properly spawned in PostSpawn
	beamCanB3 = NULL; // Beam is properly spawned in PostSpawn
	beamCanC3 = NULL; // Beam is properly spawned in PostSpawn
	cansValid = false;

	fxCanA = NULL;
	fxCanB = NULL;
	fxCanC = NULL;

	targetTime = 0;

	fcDeclNum = 0;

	netInitialized = false;
	//HUMANHEAD PCF mdl 05/04/06 - Initialize beamLightHandle here
	beamLightHandle										= -1;
}

//=============================================================================
//
// hhWeaponSoulStripper::Spawn
//
//=============================================================================

void hhWeaponSoulStripper::Spawn() {
	BecomeActive( TH_TICKER );
	fl.clientEvents = true; //rww
}

//=============================================================================
//
// hhWeaponSoulStripper::ParseDef
//
//=============================================================================

void hhWeaponSoulStripper::ParseDef( const char *objectname ) {
	hhWeapon::ParseDef( objectname );

	maxBeamLength = dict->GetFloat( "maxBeamLength", "1024" );

	// Initialize weapon logic
	targetNode = NULL;
	beamLength = 0.0f;

	if (gameLocal.isMultiplayer)
	{ //rww - then do postspawn now.
		Event_PostSpawn();
	}
	else
	{ // Spawn the beams and SFX
		PostEventMS( &EV_PostSpawn, 0 );
	}
}

//=============================================================================
//
// hhWeaponSoulStripper::Event_PostSpawn
//
//=============================================================================

void hhWeaponSoulStripper::Event_PostSpawn() { //rww - note that this function can get called more than once in a leechgun's life time in mp on the client.
	idVec3 boneOrigin;
	idMat3 boneAxis;

	GetJointWorldTransform( dict->GetString("attach_beam"), boneOrigin, boneAxis);

	if (!beam.IsValid()) {
		beam = hhBeamSystem::SpawnBeam( boneOrigin, dict->GetString( "beam_stripper" ), boneAxis, true );
		if ( beam.IsValid() ) {
			beam->Activate( false );
		}
	}
	DestroyCans();
	SpawnCans();

	// Spawn beam light
	memset( &beamLight, 0, sizeof( beamLight ) );
	beamLight.lightId = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;

	beamLight.origin									= GetOrigin();
	beamLight.axis										= GetAxis();
	beamLight.pointLight								= true;
	beamLight.shader									= declManager->FindMaterial( dict->GetString( "mtr_light" ) );
	beamLight.shaderParms[ SHADERPARM_RED ]				= 1.0f;
	beamLight.shaderParms[ SHADERPARM_GREEN ]			= 1.0f;
	beamLight.shaderParms[ SHADERPARM_BLUE ]			= 1.0f;
	beamLight.shaderParms[ SHADERPARM_TIMESCALE ]		= 1.0f;

	beamLight.lightRadius[0]							= 115;
	beamLight.lightRadius[1]							= 115;
	beamLight.lightRadius[2]							= 1;

	//HUMANHEAD PCF mdl 05/04/06 - Moved this to the constructor
	//beamLightHandle										= -1;

	const idDict *energyDef;
	const char* str = owner->inventory.energyType;
	if ( !str || !str[0] )
		return;

	energyDef = gameLocal.FindEntityDefDict( str );
	if ( !energyDef )
		return;

	if (gameLocal.isClient) { //rww - do the minimal fireController update for the client here.
		const idDeclEntityDef *fcDecl = gameLocal.FindEntityDef( energyDef->GetString("def_fireInfo"), false );
		if (fcDecl) {
			const char *fcSpawnClass = fcDecl->dict.GetString("spawnclass", NULL);
			if (!fcSpawnClass || !fcSpawnClass[0]) {
				fcSpawnClass = "hhWeaponFireController"; //default to regular fire controller
			}
			idTypeInfo *cls = idClass::GetClass(fcSpawnClass);
			if (cls) {
				ClientUpdateFC(cls->typeNum, fcDecl->Index());
			}
		}
	}
	else {
		GiveEnergy( energyDef->GetString("def_fireInfo"), false );
	}
}

void hhWeaponSoulStripper::CheckCans(void) {
	bool wantCans = true;
	if (owner.IsValid() && owner->entityNumber != gameLocal.localClientNum) { //rww - we don't need cannister fx unless we are the client that owns this weapon. (or spectating him)
		if ( gameLocal.localClientNum >= 0 && gameLocal.entities[ gameLocal.localClientNum ] && gameLocal.entities[ gameLocal.localClientNum ]->IsType( idPlayer::Type ) ) {
			idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.localClientNum ] );
			if ( !p->spectating || p->spectator != owner->entityNumber ) {
				wantCans = false;
			}
		} else {
			wantCans = false;
		}
	}

	if (wantCans != cansValid) {
		DestroyCans();
		if (wantCans) {
			SpawnCans();
		}
	}
}

void hhWeaponSoulStripper::SpawnCans() {
	if (owner.IsValid() && owner->entityNumber != gameLocal.localClientNum) { //rww - we don't need cannister fx unless we are the client that owns this weapon. (or spectating him)
		if ( gameLocal.localClientNum >= 0 && gameLocal.entities[ gameLocal.localClientNum ] && gameLocal.entities[ gameLocal.localClientNum ]->IsType( idPlayer::Type ) ) {
			idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.localClientNum ] );
			if ( !p->spectating || p->spectator != owner->entityNumber ) {
				return;
			}
		} else {
			return;
		}
	}

	// Spawn individual canister beams (top to center)
	beamCanA1 = SpawnCanisterBeam( "attach_topA", "attach_soulA", beam_canTop );
	beamCanB1 = SpawnCanisterBeam( "attach_topB", "attach_soulB", beam_canTop );
	beamCanC1 = SpawnCanisterBeam( "attach_topC", "attach_soulC", beam_canTop );

	// Spawn individual canister beams (bottom to center)
	beamCanA2 = SpawnCanisterBeam( "attach_bottomA", "attach_soulA", beam_canBot );
	beamCanB2 = SpawnCanisterBeam( "attach_bottomB", "attach_soulB", beam_canBot );
	beamCanC2 = SpawnCanisterBeam( "attach_bottomC", "attach_soulC", beam_canBot );

	// Spawn individual canister beams (faint bottom to top glow)
	beamCanA3 = SpawnCanisterBeam( "attach_bottomA", "attach_topA", beam_canGlow );
	beamCanB3 = SpawnCanisterBeam( "attach_bottomB", "attach_topB", beam_canGlow );
	beamCanC3 = SpawnCanisterBeam( "attach_bottomC", "attach_topC", beam_canGlow );

	fxCanA = SpawnCanisterFx( "attach_bottomA", fx_can );
	fxCanB = SpawnCanisterFx( "attach_bottomB", fx_can );
	fxCanC = SpawnCanisterFx( "attach_bottomC", fx_can );

	cansValid = true;
}

void hhWeaponSoulStripper::DestroyCans() {
	SAFE_REMOVE( beamCanA1 );
	SAFE_REMOVE( beamCanB1 );
	SAFE_REMOVE( beamCanC1 );

	SAFE_REMOVE( beamCanA2 );
	SAFE_REMOVE( beamCanB2 );
	SAFE_REMOVE( beamCanC2 );

	SAFE_REMOVE( beamCanA3 );
	SAFE_REMOVE( beamCanB3 );
	SAFE_REMOVE( beamCanC3 );

	SAFE_REMOVE( fxCanA );
	SAFE_REMOVE( fxCanB );
	SAFE_REMOVE( fxCanC );

	cansValid = false;
}

//=============================================================================
//
// hhWeaponSoulStripper::SpawnCanisterBeam
//
//=============================================================================

hhBeamSystem *hhWeaponSoulStripper::SpawnCanisterBeam( const char *bottom, const char *top, const idStr &beamName ) {
	hhBeamSystem *system = NULL;
	idVec3 boneOrigin;
	idMat3 boneAxis;

	if( beamName.IsEmpty() )
		return NULL;

	GetJointWorldTransform( dict->GetString(bottom), boneOrigin, boneAxis);

	system = hhBeamSystem::SpawnBeam( boneOrigin, beamName, boneAxis, true );
	if ( !system ) {
		return NULL;
	}

	if (owner.IsValid()) {
		system->snapshotOwner = owner.GetEntity(); //rww
	}

	GetJointWorldTransform( dict->GetString(top), boneOrigin, boneAxis);

	system->SetTargetLocation( boneOrigin );
	system->GetRenderEntity()->weaponDepthHack = true;
	system->GetRenderEntity()->allowSurfaceInViewID = owner->entityNumber+1;
	system->fl.neverDormant = true;

	return system;
}

//=============================================================================
//
// hhWeaponSoulStripper::SpawnCanisterSprite
//
//=============================================================================

idEntity *hhWeaponSoulStripper::SpawnCanisterSprite( const char *attach, const char *spriteName ) {
	idEntity	*ent;
	const char	*jointName;

	assert(gameLocal.isMultiplayer); //rww - i don't see this being used in mp, but if it is, it needs to be reworked.

	jointName = dict->GetString( attach );
	ent = gameLocal.SpawnObject( dict->GetString( spriteName ) );
	if ( ent ) {
		ent->MoveToJoint( this, jointName );
		ent->BindToJoint( this, jointName, false );
		ent->GetRenderEntity()->weaponDepthHack = true;
	}

	return ent;
}

hhEntityFx*	hhWeaponSoulStripper::SpawnCanisterFx( const char *attach, const idStr &name ) {
	const char	*jointName;
	idVec3 boneOrigin;
	idMat3 boneAxis;
	hhEntityFx*	fx;

	if( name.IsEmpty() )
		return NULL;

	jointName = dict->GetString( attach );

	GetJointWorldTransform( dict->GetString(attach), boneOrigin, boneAxis);

	hhFxInfo fxInfo;
	fxInfo.SetEntity( this );
	fxInfo.SetBindBone( jointName );
	fx = SpawnFxLocal( name, boneOrigin, boneAxis, &fxInfo, true );
	fx->GetRenderEntity()->weaponDepthHack = true;
	fx->GetRenderEntity()->allowSurfaceInViewID = owner->entityNumber+1;
	//don't attach canister fx to snapshotOwner, since they are local and temporary
	/*
	if (owner.IsValid()) {
		fx->snapshotOwner = owner.GetEntity(); //rww
	}
	*/

	return fx;
}

//=============================================================================
//
// hhWeaponSoulStripper::CreateAltFireController
//
//=============================================================================
hhWeaponFireController*	hhWeaponSoulStripper::CreateAltFireController() {
	return hhWeapon::CreateAltFireController();
	//return new hhSoulStripperAltFireController;
}

//=============================================================================
//
// hhWeaponSoulStripper::~hhWeaponSoulStripper
//
//=============================================================================

hhWeaponSoulStripper::~hhWeaponSoulStripper() {
	SAFE_REMOVE( beam );

	DestroyCans();

	// Remove the beam light
	if ( beamLightHandle != -1 ) {
		gameRenderWorld->FreeLightDef( beamLightHandle );
		beamLightHandle = -1;
	}

	StopSound( SND_CHANNEL_WEAPON, false );	//rww - don't broadcast this. you're removing the entity on the client too, so not only would it get
											//called there too, but once you receive the event the entity will be gone and it will be tossed.
}

//=============================================================================
//
// hhWeaponSoulStripper::UpdateGUI
//
//=============================================================================
void hhWeaponSoulStripper::UpdateGUI() {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateInt( "souls", (GetAnimPostfix()>0) ? GetAnimPostfix()-'A'+1  : 0 );
		float p = 3.0f * owner->inventory.AmmoPercentage(owner.GetEntity(), GetAmmoType());
		renderEntity.gui[0]->SetStateFloat( "soulpercentageA", idMath::ClampFloat(0.0f, 1.0f, p - 0.0f) );
		renderEntity.gui[0]->SetStateFloat( "soulpercentageB", idMath::ClampFloat(0.0f, 1.0f, p - 1.0f) );
		renderEntity.gui[0]->SetStateFloat( "soulpercentageC", idMath::ClampFloat(0.0f, 1.0f, p - 2.0f) );
		renderEntity.gui[0]->SetStateBool( "leeching", (targetNode) ? true : false);

		const char* str;
		if( targetNode ) {
			const idDict *energyDef;
			str = targetNode->spawnArgs.GetString( "def_energy" );
			if ( str && str[0] ) {
				energyDef = gameLocal.FindEntityDefDict( str );
				if ( energyDef ) {
					renderEntity.gui[0]->SetStateFloat( "meter", (gameLocal.time - targetTime)/energyDef->GetFloat( "leech_time", "500" ) );
					renderEntity.gui[0]->SetStateInt( "leechtype", energyDef->GetInt("beamType")+1 );
				}
			}
		}
		
		if( fireController ) {
			str = fireController->GetString("intgui");
			if( str && str[0] )
				renderEntity.gui[0]->SetStateInt( "type", str[0]-'1'+1 );
		}
		else {
			renderEntity.gui[0]->SetStateInt( "type", 0 );
		}
	}
}

void hhWeaponSoulStripper::Event_Leech() {
	idVec3		start, end;
	trace_t		results;
	int captured = 0;

	if ( !beam.IsValid() ) {
		idThread::ReturnInt( 0 );
		return;
	}

	start = GetOrigin();

	// Currently latched onto a target, check to see if the target is within the valid cone and there is still a LOS to the target
	if ( targetNode ) {
		end = beam->GetTargetLocation();

		// Verify that the target is within a cone in front of the weapon
		idVec3 vecToTarget = end - start;
		vecToTarget.Normalize();
		float dpX = GetAxis()[0] * vecToTarget;
		float dpY = GetAxis()[1] * vecToTarget;

		// Target isn't in the cone, so reset the trace
		if ( idMath::Fabs( dpY ) >= spawnArgs.GetFloat( "beamTargetAngle", "0.4" ) 	|| !targetNode->CanLeech() ) {
			//ReleaseEntity();
			targetNode->LeechTrigger(GetOwner(),"leech_end");
			targetNode = NULL;
			end = start + maxBeamLength * GetAxis()[0];
		}
	}
	else
		end = start + maxBeamLength * GetAxis()[0];

	// Check to see if the beam should attempt to hit something
	if (owner.IsValid()) {
		//rww - can hit owners due to different angles in mp (although not the local client since the trace is done on the client every frame)
		gameLocal.clip.TracePoint( results, start, end, MASK_SHOT_RENDERMODEL | CONTENTS_GAME_PORTAL, owner.GetEntity() );
	}
	else {
		gameLocal.clip.TracePoint( results, start, end, MASK_SHOT_RENDERMODEL | CONTENTS_GAME_PORTAL, this );
	}

	beamLength = (results.endpos - start).Length();

	// Hit something
	if ( results.fraction < 1.0f ) {
		captured = CaptureEnergy( results );

		// Update the main beam
		UpdateBeam( start, true ); // struck an entity
	}

	//if( owner.IsValid() ) {
	//	owner->WeaponFireFeedback( dict );
	//}

	// Return true if the beam has struck an entity
	idThread::ReturnFloat( (float)captured );
}

//=============================================================================
//
// hhWeaponSoulStripper::Event_RecoilBeam
//
//=============================================================================

void hhWeaponSoulStripper::Event_EndLeech() {
	idVec3				start;
	idMat3				axis;

	if ( !beam.IsValid() ) {
		return;
	}

	if ( targetNode )
		targetNode->LeechTrigger(GetOwner(),"leech_end");
	targetNode = NULL;

	// Update the main beam
	UpdateBeam( start, false );

	if( owner.IsValid() ) {
		owner->WeaponFireFeedback( dict );
	}

	idThread::ReturnFloat( 0 );
}

//=============================================================================
//
// hhWeaponSoulStripper::Ticker
//
// Controls the length of the beam and firing projectiles
//=============================================================================

void hhWeaponSoulStripper::Ticker() {
	// Update canister beams
	UpdateCanisterBeam( beamCanA1.GetEntity(), "attach_topA", "attach_soulA" );
	UpdateCanisterBeam( beamCanB1.GetEntity(), "attach_topB", "attach_soulB" );
	UpdateCanisterBeam( beamCanC1.GetEntity(), "attach_topC", "attach_soulC" );
	UpdateCanisterBeam( beamCanA2.GetEntity(), "attach_bottomA", "attach_soulA" );
	UpdateCanisterBeam( beamCanB2.GetEntity(), "attach_bottomB", "attach_soulB" );
	UpdateCanisterBeam( beamCanC2.GetEntity(), "attach_bottomC", "attach_soulC" );
	UpdateCanisterBeam( beamCanA3.GetEntity(), "attach_bottomA", "attach_topA" );
	UpdateCanisterBeam( beamCanB3.GetEntity(), "attach_bottomB", "attach_topB" );
	UpdateCanisterBeam( beamCanC3.GetEntity(), "attach_bottomC", "attach_topC" );

	hhBeamBasedFireController *beamController;
	hhSunbeamFireController *sun;
	if ( fireController && fireController->IsType(hhBeamBasedFireController::Type) ) {
		if ( fireController->IsType(hhSunbeamFireController::Type) ) {
			sun = static_cast<hhSunbeamFireController *>(fireController);
			sun->Think();
		}
		else {
			beamController = static_cast<hhBeamBasedFireController *>(fireController);
			beamController->Think();
		}
	}

	// Update our current animation if cans changed
	if ( GetAnimPostfix() != lastCanState ) {
		if ( GetAnimator()->IsAnimating( gameLocal.time ) && lastAnim.Length() != 0 ) {
			idStr animname = lastAnim;
			animname.Append( GetAnimPostfix() );
			int anim = GetAnimator()->GetAnim( animname );
			if( anim )
				GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, FRAME2MS(4) );
		}

		lastCanState = GetAnimPostfix();
	}
}

//=============================================================================
//
// hhWeaponSoulStripper::UpdateBeam
//
// Updates the beam system
//		- beam length
//		- scale issues (don't let the beam wiggle as wildly when it is pulled close)
//		- beam light
//=============================================================================

void hhWeaponSoulStripper::UpdateBeam( idVec3 start, bool struckEntity ) {
	float			scale;
	idVec3			boneOrigin;
	idMat3			boneAxis;

	if ( !beam.IsValid() ) {
		return;
	}

	if ( targetNode ) {
		beam->Activate( true );

		GetJointWorldTransform( dict->GetString("attach_beam"), boneOrigin, boneAxis);

		beam->SetOrigin( boneOrigin );

		beam->SetArcVector( GetAxis()[0] );
		scale = beamLength / 150.0f;
		if( scale > 1 ) {
			scale = 1;
		}

		if ( targetNode->entityNumber < ENTITYNUM_MAX_NORMAL ) {
			beam->SetTargetLocation( targetNode->leechPoint );
		}

		int beamType=0;
		const idDict *energyDef;
		const char* str = targetNode->spawnArgs.GetString( "def_energy" );
		if ( str && str[0] ) {
			energyDef = gameLocal.FindEntityDefDict( str );
			if ( energyDef )
				beamType=energyDef->GetInt("beamType");
		}

		beam->SetShaderParm( SHADERPARM_MODE, beamType );
		beamLight.shaderParms[ SHADERPARM_MODE ] = beamType;

		beam->SetBeamOffsetScale( scale );

		// Update the beam light 
		beamLight.axis[0] = GetAxis()[2];
		beamLight.axis[1] = GetAxis()[1];
		beamLight.axis[2] = GetAxis()[0]; // Re-align the light to point along the beam
		beamLight.origin = (GetOrigin() + GetAxis()[0] * beamLength * 0.25f); // Set beam origin to the midpoint of the weapon origin and the target
		beamLight.lightRadius[2] = 128.0f + beamLength * 0.5f; // Scale the light size based upon the beam (larger than half, so the beam extends beyond the extents slightly)

		if ( beamLightHandle != -1 ) {
			gameRenderWorld->UpdateLightDef( beamLightHandle, &beamLight );
		} else {
			beamLightHandle = gameRenderWorld->AddLightDef( &beamLight );
		}
	} else {
		beam->Activate( false );

		// Hide the beam light
		if ( beamLightHandle != -1 ) {
			gameRenderWorld->FreeLightDef( beamLightHandle );
			beamLightHandle = -1;
		}
	}
}

//=============================================================================
//
// hhWeaponSoulStripper::UpdateCanisterBeam
//
//=============================================================================

void hhWeaponSoulStripper::UpdateCanisterBeam( hhBeamSystem *system, const char *top, const char *bottom ) {
	idVec3 boneOrigin;
	idMat3 boneAxis;

	if ( !system ) {
		return;
	}

	GetJointWorldTransform( dict->GetString( top ), boneOrigin, boneAxis);
	system->SetTargetLocation( boneOrigin );
	GetJointWorldTransform( dict->GetString( bottom ), boneOrigin, boneAxis);
	system->SetOrigin( boneOrigin );
}

//=============================================================================
//
// hhWeaponSoulStripper::CaptureEnergy
//
//=============================================================================

int hhWeaponSoulStripper::CaptureEnergy( trace_t &results ) {
	idEntity	*hitEntity;
	hhEnergyNode*	hitNode;

	hitEntity = gameLocal.GetTraceEntity(results);

	if ( hitEntity && hitEntity->IsType( hhEnergyNode::Type ) ) {
		hitNode = static_cast<hhEnergyNode*>(hitEntity);
		if ( !hitNode->CanLeech() )
			return 0;
	}
	else {
		if ( targetNode )
			targetNode->LeechTrigger(GetOwner(),"leech_end");
		targetNode = NULL;
		return 0;
	}

	if ( targetNode != hitNode ) {
		if ( targetNode )
			targetNode->LeechTrigger(GetOwner(),"leech_end");
		targetTime = gameLocal.time;
		targetNode = hitNode;
		targetNode->LeechTrigger(GetOwner(),"leech_start");
	}

	const idDict *energyDef;
	const char* str = targetNode->spawnArgs.GetString( "def_energy" );
	if ( !str || !str[0] )
		return 0;	// blank node

	energyDef = gameLocal.FindEntityDefDict( str );
	if ( !energyDef )
		return 0;

	if ( gameLocal.time - targetTime < energyDef->GetInt( "leech_time", "500" ) )
		return 1;	//not enough time

	targetNode->LeechTrigger(GetOwner(),"leech_success");
	targetNode->Finish();
	targetTime = gameLocal.time;

	//create new firecontroller for new energy type
	if( !GiveEnergy( energyDef->GetString("def_fireInfo"), true ) )
		return 0;

	StartSound( "snd_intake", SND_CHANNEL_ANY );
	SpawnCanisterFx( "attach_sidevent", spawnArgs.GetString("fx_sidevent") );
	SpawnCanisterFx( "attach_steam", spawnArgs.GetString("fx_steam") );

	owner->inventory.energyType = str;

	return 2;
}

bool hhWeaponSoulStripper::GiveEnergy( const char *energyType, bool fill ) {
	if ( !energyType || !energyType[0] )
		return false;

	//create new firecontroller for new energy type
	if (gameLocal.isClient)	//rww - handle this properly through snapshots
		return true;

	SAFE_DELETE_PTR(fireController);
	const idDeclEntityDef *fcDecl = gameLocal.FindEntityDef( energyType, false );
	const idDict* infoDict = fcDecl ? &fcDecl->dict : NULL;
	fcDeclNum = 0;
	if( infoDict ) {
		const char	*spawn;
		idTypeInfo	*cls;

		if( fill ) {
			int num = owner->GetWeaponNum("weaponobj_soulstripper");
			assert(num);
			owner->weaponInfo[ num ].ammoMax = infoDict->GetInt("ammoAmount");
			owner->spawnArgs.SetInt( "max_ammo_energy", infoDict->GetInt("ammoAmount") );
			owner->inventory.ammo[owner->inventory.AmmoIndexForAmmoClass("ammo_energy")]=0;
			owner->Give( "ammo_energy", infoDict->GetString("ammoAmount") );
		}
		beam_canTop = infoDict->GetString( "beam_canTop" );
		beam_canBot = infoDict->GetString( "beam_canBot" );
		beam_canGlow = infoDict->GetString( "beam_canGlow" );
		fx_can = infoDict->GetString( "fx_can" );

		DestroyCans();
		SpawnCans();

		infoDict->GetString( "spawnclass", NULL, &spawn );
		if ( !spawn || !spawn[0] ) { //rww - changed to be in line with client behavior
			spawn = "hhWeaponFireController"; //default to regular fire controller
		}
		cls = idClass::GetClass( spawn );
		if ( !cls || !cls->IsType( hhWeaponFireController::Type ) ) {
			common->Warning( "Could not spawn.  Class '%s' not found.", spawn );
			return false;
		}
		fireController = static_cast<hhWeaponFireController *>( cls->CreateInstance() );
		if ( !fireController ) {
			common->Warning( "Could not spawn '%s'. Instance could not be created.", spawn );
			return false;
		}
		fcDeclNum = fcDecl->Index();
		/*
		else {
			fireController = CreateFireController();
		}
		*/

		fireController->Init( infoDict, this, owner.GetEntity() );
	}
	
	//if it failed, use generic empty controller
	if ( !fireController ) {
		const idDict* infoDict = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_fireInfo"), false );
		if( infoDict ) {
			fireController = CreateFireController();
			fireController->Init( infoDict, this, owner.GetEntity() );
		}			
	}
	return true;
}

//=============================================================================
//
// hhWeaponSoulStripper::GetAnimPostfix
//
//  Appends to the passed in postfix.
//=============================================================================

char hhWeaponSoulStripper::GetAnimPostfix() {
	// Choose the anim postfix based upon ammo
	float p=owner->inventory.AmmoPercentage(owner.GetEntity(), GetAmmoType());
	if(p > 0)
		return (int)(2.9f*p) + 'A';
	else
		return 0;
}

//=============================================================================
//
// hhWeaponSoulStripper::Event_GetAnimPostFix
//
//=============================================================================

void hhWeaponSoulStripper::Event_GetAnimPostFix() {
	idStr postfix( GetAnimPostfix() );
	idThread::ReturnString( postfix.c_str() );
}

//=============================================================================
//
// hhWeaponSoulStripper::Event_PlayAnim
//
//=============================================================================

void hhWeaponSoulStripper::Event_PlayAnim( int channel, const char *animname ) {
	lastAnim = "";

	idStr anim = animname;
	anim.Append( GetAnimPostfix() );
	hhWeapon::Event_PlayAnim( channel, anim.c_str() );
}

//=============================================================================
//
// hhWeaponSoulStripper::Event_PlayCycle
//
//=============================================================================

void hhWeaponSoulStripper::Event_PlayCycle( int channel, const char *animname ) {
	lastAnim = animname;

	idStr anim = animname;
	anim.Append( GetAnimPostfix() );
	hhWeapon::Event_PlayCycle( channel, anim.c_str() );
}

//=============================================================================
//
// hhWeaponSoulStripper::PresentWeapon
//
//=============================================================================

void hhWeaponSoulStripper::PresentWeapon( bool showViewModel ) {
	if ( IsHidden() || !owner->CanShowWeaponViewmodel() || pm_thirdPerson.GetBool() ) {
		if ( beamCanA1.IsValid() ) beamCanA1->Activate( false );
		if ( beamCanB1.IsValid() ) beamCanB1->Activate( false );
		if ( beamCanC1.IsValid() ) beamCanC1->Activate( false );
		if ( beamCanA2.IsValid() ) beamCanA2->Activate( false );
		if ( beamCanB2.IsValid() ) beamCanB2->Activate( false );
		if ( beamCanC2.IsValid() ) beamCanC2->Activate( false );
		if ( beamCanA3.IsValid() ) beamCanA3->Activate( false );
		if ( beamCanB3.IsValid() ) beamCanB3->Activate( false );
		if ( beamCanC3.IsValid() ) beamCanC3->Activate( false );
		if ( fxCanA.IsValid() ) fxCanA->Hide();
		if ( fxCanB.IsValid() ) fxCanB->Hide();
		if ( fxCanC.IsValid() ) fxCanC->Hide();
	} else {
		if ( beamCanA1.IsValid() ) beamCanA1->Activate( true );
		if ( beamCanB1.IsValid() ) beamCanB1->Activate( true );
		if ( beamCanC1.IsValid() ) beamCanC1->Activate( true );
		if ( beamCanA2.IsValid() ) beamCanA2->Activate( true );
		if ( beamCanB2.IsValid() ) beamCanB2->Activate( true );
		if ( beamCanC2.IsValid() ) beamCanC2->Activate( true );
		if ( beamCanA3.IsValid() ) beamCanA3->Activate( true );
		if ( beamCanB3.IsValid() ) beamCanB3->Activate( true );
		if ( beamCanC3.IsValid() ) beamCanC3->Activate( true );
		if ( fxCanA.IsValid() ) fxCanA->Show();
		if ( fxCanB.IsValid() ) fxCanB->Show();
		if ( fxCanC.IsValid() ) fxCanC->Show();
	}

	hhWeapon::PresentWeapon( showViewModel );
}

//=============================================================================
//
// hhWeaponSoulStripper::Show
//
//=============================================================================

void hhWeaponSoulStripper::Show() {
	if ( beamCanA1.IsValid() ) beamCanA1->Show();
	if ( beamCanB1.IsValid() ) beamCanB1->Show();
	if ( beamCanC1.IsValid() ) beamCanC1->Show();
	if ( beamCanA2.IsValid() ) beamCanA2->Show();
	if ( beamCanB2.IsValid() ) beamCanB2->Show();
	if ( beamCanC2.IsValid() ) beamCanC2->Show();
	if ( beamCanA3.IsValid() ) beamCanA3->Show();
	if ( beamCanB3.IsValid() ) beamCanB3->Show();
	if ( beamCanC3.IsValid() ) beamCanC3->Show();
	if ( fxCanA.IsValid() ) fxCanA->Show();
	if ( fxCanB.IsValid() ) fxCanB->Show();
	if ( fxCanC.IsValid() ) fxCanC->Show();

	hhWeapon::Show();
}

//=============================================================================
//
// hhWeaponSoulStripper::Hide
//
//=============================================================================

void hhWeaponSoulStripper::Hide() {
	if ( beamCanA1.IsValid() ) beamCanA1->Hide();
	if ( beamCanB1.IsValid() ) beamCanB1->Hide();
	if ( beamCanC1.IsValid() ) beamCanC1->Hide();
	if ( beamCanA2.IsValid() ) beamCanA2->Hide();
	if ( beamCanB2.IsValid() ) beamCanB2->Hide();
	if ( beamCanC2.IsValid() ) beamCanC2->Hide();
	if ( beamCanA3.IsValid() ) beamCanA3->Hide();
	if ( beamCanB3.IsValid() ) beamCanB3->Hide();
	if ( beamCanC3.IsValid() ) beamCanC3->Hide();
	if ( fxCanA.IsValid() ) fxCanA->Hide();
	if ( fxCanB.IsValid() ) fxCanB->Hide();
	if ( fxCanC.IsValid() ) fxCanC->Hide();

	hhWeapon::Hide();
}


/*
================
hhWeaponSoulStripper::Save
================
*/
void hhWeaponSoulStripper::Save( idSaveGame *savefile ) const {
	savefile->WriteRenderLight( beamLight );
	//HUMANHEAD PCF mdl 05/04/06 - Don't save light handles
	//savefile->WriteInt( beamLightHandle );

	beam.Save( savefile );
	beamCanA1.Save( savefile );
	beamCanB1.Save( savefile );
	beamCanC1.Save( savefile );
	beamCanA2.Save( savefile );
	beamCanB2.Save( savefile );
	beamCanC2.Save( savefile );
	beamCanA3.Save( savefile );
	beamCanB3.Save( savefile );
	beamCanC3.Save( savefile );
	fxCanA.Save( savefile );
	fxCanB.Save( savefile );
	fxCanC.Save( savefile );
	savefile->WriteBool(cansValid);

	savefile->WriteFloat( beamLength );
	savefile->WriteFloat( maxBeamLength );
	savefile->WriteObject( targetNode );
	savefile->WriteVec3( targetOffset );

	savefile->WriteString( lastAnim );
//	savefile->WriteInt( lastAltAmmo );
}

/*
================
hhWeaponSoulStripper::Restore
================
*/
void hhWeaponSoulStripper::Restore( idRestoreGame *savefile ) {
	savefile->ReadRenderLight( beamLight );
	//HUMANHEAD PCF mdl 05/04/06 - Don't save light handles
	//savefile->ReadInt( beamLightHandle );

	beam.Restore( savefile );
	beamCanA1.Restore( savefile );
	beamCanB1.Restore( savefile );
	beamCanC1.Restore( savefile );
	beamCanA2.Restore( savefile );
	beamCanB2.Restore( savefile );
	beamCanC2.Restore( savefile );
	beamCanA3.Restore( savefile );
	beamCanB3.Restore( savefile );
	beamCanC3.Restore( savefile );
	fxCanA.Restore( savefile );
	fxCanB.Restore( savefile );
	fxCanC.Restore( savefile );
	savefile->ReadBool(cansValid);

	savefile->ReadFloat( beamLength );
	savefile->ReadFloat( maxBeamLength );
	savefile->ReadObject( reinterpret_cast<idClass *&> ( targetNode ) );
	savefile->ReadVec3( targetOffset );

	savefile->ReadString( lastAnim );;
}

/*
=================
hhWeaponSoulStripper::ReadFromSnapshot
rww - update the fire controller on the client
=================
*/
void hhWeaponSoulStripper::ClientUpdateFC(int fcType, int fcDefNumber) {
	SAFE_DELETE_PTR(fireController);

    idTypeInfo *typeInfo = idClass::GetType(fcType);
	const idDeclEntityDef *fcDecl = static_cast<const idDeclEntityDef *>(declManager->DeclByIndex(DECL_ENTITYDEF, fcDefNumber, false));
	if (typeInfo && fcDecl) {
		int num = owner->GetWeaponNum("weaponobj_soulstripper");
		assert(num);
		owner->weaponInfo[ num ].ammoMax = fcDecl->dict.GetInt("ammoAmount");
		owner->spawnArgs.SetInt( "max_ammo_energy", fcDecl->dict.GetInt("ammoAmount") );
		beam_canTop = fcDecl->dict.GetString( "beam_canTop" );
		beam_canBot = fcDecl->dict.GetString( "beam_canBot" );
		beam_canGlow = fcDecl->dict.GetString( "beam_canGlow" );
		fx_can = fcDecl->dict.GetString( "fx_can" );
		DestroyCans();
		SpawnCans();

		fireController = static_cast<hhWeaponFireController *>(typeInfo->CreateInstance());
		if (fireController) {
			fireController->Init(&fcDecl->dict, this, owner.GetEntity());
		}
	}
}

/*
=================
hhWeaponSoulStripper::WriteToSnapshot
rww - write applicable weapon values to snapshot
=================
*/
void hhWeaponSoulStripper::WriteToSnapshot( idBitMsgDelta &msg ) const {
	//target entity
	if (targetNode) {
		msg.WriteBits(1, 1);
		msg.WriteBits(targetNode->entityNumber, GENTITYNUM_BITS);
	}
	else {
		msg.WriteBits(0, 1);
	}

	//write the fire controller type
	if (fireController) {
		msg.WriteBits(fireController->GetType()->typeNum, idClass::GetTypeNumBits());
		msg.WriteBits(gameLocal.ServerRemapDecl(-1, DECL_ENTITYDEF, fcDeclNum ), gameLocal.entityDefBits);
	}
	else {
		msg.WriteBits(0, idClass::GetTypeNumBits());
		msg.WriteBits(0, gameLocal.entityDefBits);
	}

	hhWeapon::WriteToSnapshot(msg);
}

/*
=================
hhWeaponSoulStripper::ReadFromSnapshot
rww - read applicable weapon values from snapshot
=================
*/
void hhWeaponSoulStripper::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	//target entity
	bool hasEnt = !!msg.ReadBits(1);
	if (hasEnt)	{
		int entNum = msg.ReadBits(GENTITYNUM_BITS);
		targetNode = static_cast<hhEnergyNode*>(gameLocal.entities[entNum]);
	}
	else {
		targetNode = NULL;
	}

	int fcType = msg.ReadBits(idClass::GetTypeNumBits());
	int fcDefNumber = gameLocal.ClientRemapDecl(DECL_ENTITYDEF, msg.ReadBits(gameLocal.entityDefBits));

	//standard weapon stuff (primarily done before switch logic to get owner)
	hhWeapon::ReadFromSnapshot(msg);

	if (owner.IsValid() && fcType && (!fireController || fcType != fireController->GetType()->typeNum || !netInitialized)) {
		//if there is no fire controller on the client or the fire controller does not much the type given by the server, switch
		ClientUpdateFC(fcType, fcDefNumber);
		netInitialized = true;
	}
}

/*
================
hhWeaponSoulStripper::GetClipBits
================
*/
int hhWeaponSoulStripper::GetClipBits(void) const {
	return 12; //0-4096
}

void hhWeaponSoulStripper::Event_LightFadeIn( float fadetime ) {
	if (gameLocal.isMultiplayer) { //rww - none of this in mp
		return;
	}
	idEntity *ent;
	idEntity *next;
	idLight *light;
	idVec3 color;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = next ) {
		next = ent->spawnNode.Next();
		if( !gameLocal.InPlayerPVS(ent) )
			continue;
		if ( !ent->IsType( idLight::Type ) )
			continue;
		light = static_cast<idLight*>( ent );
		if ( light->GetMaterial()->IsFogLight() )
			continue;
		light->GetColor( color );
		if ( color == idVec3(0.001f,0.f,0.f) )
			light->FadeIn( fadetime );
	}
}

void hhWeaponSoulStripper::Event_LightFadeOut( float fadetime ) {
	if (gameLocal.isMultiplayer) { //rww - none of this in mp
		return;
	}
	idEntity *ent;
	idEntity *next;
	idLight *light;
	idVec3 color;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = next ) {
		next = ent->spawnNode.Next();
		if( !gameLocal.InPlayerPVS(ent) )
			continue;
		if ( !ent->IsType( idLight::Type ) )
			continue;
		light = static_cast<idLight*>( ent );
		if ( light->GetMaterial()->IsFogLight() )
			continue;
		light->GetColor( color );
		if( light->spawnArgs.GetVector("_color", "1 1 1") == color && light->GetCurrentLevel()==1 )
			light->Fade(idVec4(0.001f,0.f,0.f,0.f), fadetime );
	}
}

void hhWeaponSoulStripper::Event_LightFade( float fadeOut, float pause, float fadeIn ) {
	PostEventSec( &EV_LightFadeOut, 0, fadeOut );
	PostEventSec( &EV_LightFadeIn, pause, fadeIn );
}

void hhWeaponSoulStripper::Event_GetFireFunction() {
	if ( fireController ) {
		idThread::ReturnString( fireController->GetScriptFunction() );
	}
}

void hhWeaponSoulStripper::Event_KillBeam() {
	if ( fireController && fireController->IsType(hhBeamBasedFireController::Type) ) {
		hhBeamBasedFireController *beamController = static_cast<hhBeamBasedFireController *>(fireController);
		beamController->KillBeam();
	}
}

/*void hhWeaponSoulStripper::Event_GetFireSound() {
	if ( fireController ) {
		idThread::ReturnString( fireController->GetScriptFunction() );
	}
}*/