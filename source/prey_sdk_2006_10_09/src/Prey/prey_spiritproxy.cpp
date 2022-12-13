//**************************************************************************
//**
//** PREY_SPIRITPROXY.CPP
//**
//** Game code for the player proxy dropped when the player spirit walks
//**
//**************************************************************************

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#define	MIN_ACTIVATION_TIME		SEC2MS( 1.0f )

const idEventDef EV_SpawnEffect( "<spawnEffect>", NULL );
const idEventDef EV_OrientToGravity( "orientToGravity", "d" );

//==========================================================================
// hhSpiritProxy
//==========================================================================

CLASS_DECLARATION( idActor, hhSpiritProxy )
	EVENT( EV_SpawnEffect,					hhSpiritProxy::Event_SpawnEffect )
	EVENT( EV_OrientToGravity,				hhSpiritProxy::Event_OrientToGravity )
	EVENT( EV_ResetGravity,					hhSpiritProxy::Event_ResetGravity )
	EVENT( EV_ShouldRemainAlignedToAxial,	hhSpiritProxy::Event_ShouldRemainAlignedToAxial )
	EVENT( EV_Broadcast_AssignFx,			hhSpiritProxy::Event_AssignSpiritFx )
END_CLASS

hhSpiritProxy::hhSpiritProxy() {
	fl.networkSync = true;
	playerModelNum = 0;
}

void hhSpiritProxy::Spawn(void) {
	fl.takedamage = true;
	spiritFx = NULL;

	clientAnimated = false;
	netAnimType = 0;

	if (gameLocal.isMultiplayer) {
		SetModel("model_multiplayer_tommy");
		SetSkinByName("skins/characters/tommy_mp_spirit");
		playerModelNum = 0;
	}

	// Required so that models move in place.
	GetAnimator()->RemoveOriginOffset( true );

	if (gameLocal.isMultiplayer && !IsType(hhDeathProxy::Type)) { //rww - ambient sound for mp
		StartSound( "snd_spiritSound", SND_CHANNEL_VOICE, 0, false, NULL );
	}
}

//==========================================================================
//
// hhSpiritProxy::SetModel
//
//==========================================================================
void hhSpiritProxy::SetModel( const char *modelname ) {
	spawnArgs.Set("playerModel", modelname);
	idActor::SetModel(modelname);
}

//==========================================================================
//
// hhSpiritProxy::UpdateModelForPlayer
//
//==========================================================================
void hhSpiritProxy::UpdateModelForPlayer(void) {
	int modelNum = 0;
	player->GetUserInfo()->GetInt("ui_modelNum", "0", modelNum);

	if (modelNum != playerModelNum) { //time to change models then.
		idStr customModel;
		if (!IsType(hhMPDeathProxy::Type)) { //don't do this for death prox, since it uses the spirit fadeaway skin
			SetSkin(NULL); //destroy custom skin on the spirit proxy so that the appropriate player skin is used.
		}
		playerModelNum = modelNum;
		if (player->spawnArgs.GetString(va("model_mp%i", modelNum), "", customModel)) {
			SetModel(customModel.c_str());
		}
		else {
			SetModel("model_multiplayer_tommy");
		}
		if (!IsType(hhMPDeathProxy::Type)) {
			//new: check for a custom spirit proxy skin for the given model
			if (!player->spawnArgs.GetString(va("skin_mpspirit%i", modelNum), "", customModel)) {
				customModel = "skins/characters/tommy_mp_spirit";
			}
			SetSkinByName(customModel);

			StartAnimation(); //restart animation on new model (unless you're a corpse)
		}
	}
}

//==========================================================================
//
// hhSpiritProxy::Think
//
//==========================================================================
void hhSpiritProxy::Think() {
	idVec3 oldOrigin = physicsObj.GetOrigin();
	idVec3 oldVelocity = physicsObj.GetLinearVelocity();

	if (gameLocal.isMultiplayer) {
		DrawPlayerIcons();

		if (player.IsValid() && player.GetEntity() && player->IsType(hhPlayer::Type) && !IsType(hhMPDeathProxy::Type)) {
			UpdateModelForPlayer();
		}
	}
	idActor::Think();

	if (player.IsValid() && player->IsType(hhPlayer::Type) && (!IsType(hhMPDeathProxy::Type) || !IsType(hhMPDeathProxy::Type))) {
		CrashLand(oldOrigin, oldVelocity);
	}
}

//==========================================================================
//
// hhSpiritProxy::CreateProxy
//
// Spawns a proxy object and then activates it
//==========================================================================

hhSpiritProxy *hhSpiritProxy::CreateProxy( const char *name, hhPlayer *owner, const idVec3& origin, const idMat3& bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis ) {
	hhSpiritProxy *proxy;

	proxy = (hhSpiritProxy *)gameLocal.SpawnObject( name );
	if( !proxy ) {
		gameLocal.Error("hhSpiritProxy::CreateProxy: Could not spawn the player proxy\n");
	}

	proxy->ActivateProxy( owner, origin, bboxAxis, newViewAxis, newViewAngles, newEyeAxis );

	return proxy;
}


//==========================================================================
//
// hhSpiritProxy::Event_SpawnEffect
//
//==========================================================================

void hhSpiritProxy::Event_SpawnEffect() {
	hhFxInfo	fxInfo;
	idVec3		boneOffset;
	idMat3		boneAxis;

	GetJointWorldTransform( spawnArgs.GetString( "bone_spiritFx" ), boneOffset, boneAxis );	

	fxInfo.RemoveWhenDone( false );
	fxInfo.SetNormal( boneAxis[1] );
	fxInfo.SetEntity( this );
	BroadcastFxInfo( spawnArgs.GetString( "fx_spirit" ), boneOffset, GetAxis(), &fxInfo, &EV_Broadcast_AssignFx );
}

//==========================================================================
//
// hhSpiritProxy::ActivateProxy
//
//==========================================================================

void hhSpiritProxy::ActivateProxy( hhPlayer *owner, const idVec3& origin, const idMat3 &bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis ) {
	assert( owner );

	player = owner;

	viewAxis = newViewAxis;
	viewAngles = newViewAngles;
	eyeAxis = newEyeAxis;
	
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel(owner->GetPhysics()->GetClipModel()), 1.0f );	
	physicsObj.SetOrigin( origin );
	physicsObj.SetAxis( bboxAxis );
	physicsObj.SetClipMask( CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_FORCEFIELD ); // HUMANHEAD mdl:  MASK_PLAYERSOLID - CONTENTS_BODY
	physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP | CONTENTS_RENDERMODEL );
	physicsObj.CheckWallWalk( true );
	if( player->IsCrouching() ) {
		physicsObj.ForceCrouching();
	}
	SetPhysics( &physicsObj );

	if (owner->GetPhysics()->IsType(hhPhysics_Player::Type)) { //rww - don't stay half-oriented if player goes into spirit while gravity-flipping
		hhPhysics_Player *plPhys = static_cast<hhPhysics_Player *>(owner->GetPhysics());
		OrientToGravity(plPhys->OrientToGravity());
		SetGravity(plPhys->GetGravity());
	}

	// Save the time when activated, to keep the player from being bounced back into the proxy too quickly
	activationTime = gameLocal.time;

	StartAnimation();
	
	UpdateVisuals();

	// Set eye height
	eyeOffset = GetPhysics()->GetBounds()[ 1 ].z - 6;

	// TODO: AIMSG_REMOVE: Tell the AI that spirit walk has started
	
	// Spawn spirit effect
	PostEventMS( &EV_SpawnEffect, spawnArgs.GetInt( "spiritBlendTime", "400" ) );

	if (!IsType(hhDeathProxy::Type) && !IsType(hhDeathWalkProxy::Type)) {
		idVec3 vel = player->GetPhysics()->GetLinearVelocity();
		GetPhysics()->SetLinearVelocity( player->GetPhysics()->GetLinearVelocity() );
		GetPhysics()->SetAngularVelocity( player->GetPhysics()->GetAngularVelocity() );
	}

	// Make sure spiritproxy is affected by wallwalkmovers
	BecomeActive( TH_PHYSICS );
}

//==========================================================================
//
// hhSpiritProxy::DeactivateProxy
//
//==========================================================================

void hhSpiritProxy::DeactivateProxy(void) {
	if( !player.IsValid() ) {
		return;
	}

	// TODO: AIMSG_REMOVED Tell the AI that spirit walk has ended
	
	// TODO: AIMSG_REMOVED Tell monsters they 'heard' a sound where the player spirit was removed from
	// This makes it so monsters will 'investigate' where a player just disappeared from. Could be cool game dynamic? Maybe not needed?
	
	if (!IsType(hhDeathProxy::Type)) { //rww - for deathwalk, the player will manage values
		RestorePlayerLocation( GetOrigin(), GetPhysics()->GetAxis(), viewAxis[0], viewAngles );
		idVec3 vel = GetPhysics()->GetLinearVelocity();
		player->GetPhysics()->SetLinearVelocity( GetPhysics()->GetLinearVelocity() );
		player->GetPhysics()->SetAngularVelocity( GetPhysics()->GetAngularVelocity() );
	}

	//HUMANHEAD rww - in multiplayer, telefrag other players who are in my body
	if (gameLocal.isMultiplayer && !gameLocal.isClient) {
		idBounds testBounds = player->GetPhysics()->GetAbsBounds();
		if (testBounds != bounds_zero) {
			idEntity *touch[ MAX_GENTITIES ];

			testBounds.ExpandSelf(-4.0f); //don't do anything if they are right on the edge, to avoid exploiting this by going up to someone and spiriting to kill them
			int num = gameLocal.clip.EntitiesTouchingBounds(testBounds, player->GetPhysics()->GetClipMask(), touch, MAX_GENTITIES);
			for (int i = 0; i < num; i++) {
				if (touch[i] && touch[i]->IsType(hhPlayer::Type) && touch[i] != player.GetEntity() && touch[i]->fl.takedamage) {
					touch[i]->Damage(player.GetEntity(), player.GetEntity(), vec3_origin, "damage_telefrag", 1.0f, INVALID_JOINT);
				}
			}
		}
	}
	//HUMANHEAD END

	Hide(); // JRM: Hide this so it doesn't stick around while waiting to be removed
	GetPhysics()->SetContents( 0 );
	PostEventMS( &EV_Remove, 1000 ); // Keep the proxy around for a few frames to deal with removal issues - JRM: made bigger
	player = NULL; // Completely disconnect the proxy from the owner

	if ( spiritFx.IsValid() ) { // Remove spirit effect
		spiritFx->Nozzle( false );
		SAFE_REMOVE( spiritFx );
	}

	fl.refreshReactions		= FALSE;
	CancelEvents( &EV_SpawnEffect );
}

//==========================================================================
//
// hhSpiritProxy::RestorePlayerLocation
//
//==========================================================================
void hhSpiritProxy::RestorePlayerLocation( const idVec3& origin, const idMat3& bboxAxis, const idVec3& viewDir, const idAngles& angles ) {
	if( IsCrouching() ) {
		player->ForceCrouching();
	}

	player->TeleportNoKillBox( origin, bboxAxis, viewDir, (angles.ToMat3() * eyeAxis.Transpose()).ToAngles() );
}

//==========================================================================
//
// hhSpiritProxy::StartAnimation
//
// Starts the animation on the proxy
//==========================================================================
void hhSpiritProxy::StartAnimation() {
	int spiritBlendTime = spawnArgs.GetInt( "spiritBlendTime", "400" );
	// Copy the current player animation to blend into the spirit anim
	GetAnimator()->CopyAnimations( *( player->GetAnimator() ) );

	// Play the initial animation
	int anim;

	if (gameLocal.isClient) { //rww
		switch (netAnimType) {
			case 1:
				anim = GetAnimator()->GetAnim("spiritleave");
				break;
			case 2:
				anim = GetAnimator()->GetAnim("crouch");
				break;
			default:
				assert(!"hhSpiritProxy has invalid netAnimType");
				anim = GetAnimator()->GetAnim("spiritleave");
				break;
		}
	}
	else {
		if ( IsCrouching() ) {
			// Determine contents of the bounds if the player is crouching
			idBounds bounds;
			idTraceModel trm;

			bounds[0].Set( -pm_bboxwidth.GetFloat() * 0.5f, -pm_bboxwidth.GetFloat() * 0.5f, 0 );
			bounds[1].Set( pm_bboxwidth.GetFloat() * 0.5f, pm_bboxwidth.GetFloat() * 0.5f, pm_normalheight.GetFloat() );

			trm.SetupBox( bounds );
			idClipModel *clipModel = new idClipModel(trm);
			int contents = gameLocal.clip.Contents( player->GetOrigin(), clipModel, player->GetAxis(), CONTENTS_SOLID, player.GetEntity() );
			delete clipModel;

			if ( contents & CONTENTS_SOLID ) { // Standing upright collides with geometry, so play the crouch anim when spiritwalking
				anim = GetAnimator()->GetAnim("crouch");
				netAnimType = 2;
			} else {
				anim = GetAnimator()->GetAnim("spiritleave");
				netAnimType = 1;
			}
		} else {
			anim = GetAnimator()->GetAnim("spiritleave");
			netAnimType = 1;
		}
	}
	GetAnimator()->ClearAllAnims( gameLocal.time,  spiritBlendTime );
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, spiritBlendTime );	
}

//==========================================================================
//
// hhSpiritProxy::Damage
//
// When damaged, the proxy forces the player to automatically return
//==========================================================================

void hhSpiritProxy::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {

	if( !player.IsValid() || !player->IsSpiritOrDeathwalking() ) {
		return;
	}
	if (gameLocal.isClient) { //rww - don't predict damage on the spirit proxy
		return;
	}

	if (!gameLocal.isMultiplayer) { //rww - not in mp
		// Don't allow the player to be bounced back into the proxy immediately if damaged
		if ( gameLocal.time - activationTime < MIN_ACTIVATION_TIME ) {
			return;
		}
	}

	// Save the player, since it is NULLed on this proxy when removed from spiritwalk
	hhPlayer *currentPlayer = player.GetEntity();

	// Disable spiritwalk
	// rww - let's defer this to the next frame so other projectiles and things that would impact this frame still hit
	if (idEvent::NumQueuedEvents(currentPlayer, &EV_StopSpiritWalk) <= 0) {
		currentPlayer->PostEventMS(&EV_StopSpiritWalk, 0);
	}
	//currentPlayer->StopSpiritWalk();

	if ( attacker && attacker == player.GetEntity() ) { // If the attacker is the player itself, then just snap back from spirit mode, without doing any damage
		return;
	}

	// Apply the damage to the player itself
	// rww - post this as an event too so we don't take the damage until after we're out of spirit form
	float localDmgScale = damageScale;
	if (gameLocal.isMultiplayer) { //rww - scale up the damage against bodies in mp, to increase punishment when your body is found
		localDmgScale *= 2.0f;
	}
	currentPlayer->PostEventMS(&EV_DamagePlayer, 1, inflictor, attacker, dir, damageDefName, localDmgScale, location);
	//currentPlayer->Damage( inflictor, attacker, dir, damageDefName, localDmgScale, location );
}

/*
===============
hhSpiritProxy::OrientToGravity
===============
*/
void hhSpiritProxy::OrientToGravity( bool orient ) {
	physicsObj.OrientToGravity( orient );
}

/*
===============
hhSpiritProxy::Event_OrientToGravity
===============
*/
void hhSpiritProxy::Event_OrientToGravity( bool orient ) {
	OrientToGravity( orient );
}

/*
===============
hhSpiritProxy::Event_AssignSpiritFx
===============
*/
void hhSpiritProxy::Event_AssignSpiritFx( hhEntityFx* fx ) {
	spiritFx = fx;
}

/*
===============
hhSpiritProxy::Event_ResetGravity

HUMANHEAD: pdm: Posted when entity is leaving a gravity zone
===============
*/
void hhSpiritProxy::Event_ResetGravity() {
	if( IsWallWalking() ) {
		return;	// Don't reset if wallwalking
	}

	idActor::Event_ResetGravity();

	OrientToGravity( true );	// let it reset orientation
}

//==========================================================================
//
// hhSpiritProxy::ShouldRemainAlignedToAxial
//
//==========================================================================
void hhSpiritProxy::ShouldRemainAlignedToAxial( bool remainAligned ) {//HUMANHEAD
	physicsObj.ShouldRemainAlignedToAxial( remainAligned );
}

//==========================================================================
//
// hhSpiritProxy::Event_ShouldRemainAlignedToAxial
//
//==========================================================================
void hhSpiritProxy::Event_ShouldRemainAlignedToAxial( bool remainAligned ) {
	ShouldRemainAlignedToAxial( remainAligned );
}

/*
================
hhSpiritProxy::UpdateModelTransform
mdl:  Based on hhPlayer::UpdateModelTransform
================
*/
void hhSpiritProxy::UpdateModelTransform( void ) {
	if( af.IsActive() ) {
		return idActor::UpdateModelTransform();
	}

	idVec3 origin;
	idMat3 axis;

	if( GetPhysicsToVisualTransform(origin, axis) ) {
		GetRenderEntity()->axis = axis;
		GetRenderEntity()->origin = GetPhysics()->GetOrigin() + origin * renderEntity.axis;
	} else {
		GetRenderEntity()->axis = GetAxis();
		GetRenderEntity()->origin = GetOrigin();
	}
}

bool hhSpiritProxy::AllowCollision(const trace_t &collision) {
	if (collision.fraction < 1.0f && collision.c.entityNum < MAX_CLIENTS && collision.c.entityNum >= 0 && gameLocal.entities[collision.c.entityNum]) {
		if (player.GetEntity() == gameLocal.entities[collision.c.entityNum]) {
			return false; //do not collide with the owner of this spirit proxy
		}
	}
	return true;
}

void hhSpiritProxy::Save( idSaveGame *savefile ) const {
	savefile->WriteStaticObject( physicsObj );

	player.Save( savefile );

	savefile->WriteAngles( viewAngles );
	savefile->WriteMat3( eyeAxis );

	spiritFx.Save( savefile );

	savefile->WriteInt( cachedCurrentWeapon );
	savefile->WriteInt( activationTime );
}

void hhSpiritProxy::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	player.Restore( savefile );

	savefile->ReadAngles( viewAngles );
	savefile->ReadMat3( eyeAxis );

	spiritFx.Restore( savefile );

	savefile->ReadInt( cachedCurrentWeapon );
	savefile->ReadInt( activationTime );
}

#define _CHEAP_PROX_SYNC

void hhSpiritProxy::WriteToSnapshot( idBitMsgDelta &msg ) const {
#ifdef _CHEAP_PROX_SYNC
	//we don't want to deal with physics stuff for this proxy on the client i suppose.
	//write the origin/axis directly and don't worry about varying physics types.
	const idVec3 &origin = renderEntity.origin;
	msg.WriteFloat(origin.x);
	msg.WriteFloat(origin.y);
	msg.WriteFloat(origin.z);
#endif

	idQuat q = renderEntity.axis.ToQuat();
	msg.WriteFloat(q.w);
	msg.WriteFloat(q.x);
	msg.WriteFloat(q.y);
	msg.WriteFloat(q.z);

	msg.WriteBits(fl.hidden, 1);

	msg.WriteBits(player.GetSpawnId(), 32);

	msg.WriteBits(netAnimType, 2);

#ifndef _CHEAP_PROX_SYNC
	msg.WriteBits(physicsObj.GetClipMask(), 32);
	msg.WriteBits(physicsObj.GetContents(), 32);
	physicsObj.WriteToSnapshot(msg, false);
#endif
}

void hhSpiritProxy::ReadFromSnapshot( const idBitMsgDelta &msg ) {
#ifdef _CHEAP_PROX_SYNC
	idVec3 origin;
	origin.x = msg.ReadFloat();
	origin.y = msg.ReadFloat();
	origin.z = msg.ReadFloat();
#endif

	idQuat q;
	q.w = msg.ReadFloat();
	q.x = msg.ReadFloat();
	q.y = msg.ReadFloat();
	q.z = msg.ReadFloat();

#ifdef _CHEAP_PROX_SYNC
	GetPhysics()->SetOrigin(origin);

	idMat3 axis = q.ToMat3();
	GetPhysics()->SetAxis(axis);
	viewAxis = axis;
#else
	viewAxis = q.ToMat3();
#endif

	bool hidden = !!msg.ReadBits(1);
	if (hidden != fl.hidden) {
		if (hidden) {
			Hide();
		}
		else {
			Show();
		}
	}

	player.SetSpawnId(msg.ReadBits(32));

	netAnimType = msg.ReadBits(2);

	//if we haven't started the animation on the client, then start it
	if (!clientAnimated && netAnimType && player.IsValid() && player.GetEntity() && player->IsType(hhPlayer::Type) && player->GetAnimator()) { //verify the owner is still valid
		StartAnimation();
		clientAnimated = true;
	}

#ifndef _CHEAP_PROX_SYNC
	if (player.IsValid() && GetPhysics() != &physicsObj) {
		physicsObj.SetSelf( this );
		physicsObj.SetClipModel( new idClipModel(player->GetPhysics()->GetClipModel()), 1.0f );	
		physicsObj.CheckWallWalk( true );
		if( player->IsCrouching() ) {
			physicsObj.ForceCrouching();
		}
		SetPhysics( &physicsObj );
	}
	physicsObj.SetClipMask(msg.ReadBits(32));
	physicsObj.SetContents(msg.ReadBits(32));
	physicsObj.ReadFromSnapshot(msg, false);
#endif
}

void hhSpiritProxy::ClientPredictionThink( void ) {
	Think();
}

//proxy needs to clear its icons as well when exiting the snapshot
void hhSpiritProxy::NetZombify(void) {
	HidePlayerIcons();
	idActor::NetZombify();
}

// Derived from hhMonsterAI::CrashLand() and hhPlayer::CrashLand() -mdl
void hhSpiritProxy::CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity ) {
	const trace_t& trace = physicsObj.GetGroundTrace();
	if ( af.IsActive() || (!physicsObj.HasGroundContacts() || trace.fraction == 1.0f) && !IsBound() ) {
		return;
	}

	//aob - only check when we land on the ground
	//If we get here we can assume we currently have ground contacts
	if( physicsObj.HadGroundContacts() ) {
		return;
	}

	// if the monster wasn't going down
	if ( ( oldVelocity * -physicsObj.GetGravityNormal() ) >= 0.0f ) {
		return;
	}

	waterLevel_t waterLevel = physicsObj.GetWaterLevel();

	// never take falling damage if completely underwater
	if ( waterLevel == WATERLEVEL_HEAD ) {
		return;
	}

	// no falling damage if touching a nodamage surface
	bool noDamage = false;
	for ( int i = 0; i < physicsObj.GetNumContacts(); i++ ) {
		const contactInfo_t &contact = physicsObj.GetContact( i );
		if ( contact.material->GetSurfaceFlags() & SURF_NODAMAGE ) {
			noDamage = true;
			break;
		}
	}

	idVec3 deltaVelocity = DetermineDeltaCollisionVelocity( oldVelocity, trace );
	float delta = (IsBound()) ? deltaVelocity.Length() : deltaVelocity * physicsObj.GetGravityNormal();

		// reduce falling damage if there is standing water
	if ( waterLevel == WATERLEVEL_WAIST ) {
		delta *= 0.25f;
	}
	if ( waterLevel == WATERLEVEL_FEET ) {
		delta *= 0.5f;
	}

	if ( delta < player->crashlandSpeed_jump ) {
		return;	// Early out
	}

	if( trace.fraction == 1.0f ) {
		return;
	}

	// Determine damage to what you're landing on
	idVec3		fallDir = oldVelocity;
	fallDir.Normalize();
	float		damageScale = hhUtils::CalculateScale( delta, player->crashlandSpeed_soft, player->crashlandSpeed_fatal );
	idVec3		reverseContactNormal = -physicsObj.GetGroundContactNormal();
	idEntity *entity = gameLocal.GetTraceEntity( trace );
	if( entity && trace.c.entityNum != ENTITYNUM_WORLD ) {
		entity->ApplyImpulse( this, 0, trace.c.point, (oldVelocity * reverseContactNormal) * reverseContactNormal );//Not sure if this impulse is large enough

		const char* entityDamageName = spawnArgs.GetString( "def_damageFellOnto" );
		if( *entityDamageName && damageScale > 0.0f) {
			entity->Damage( this, this, fallDir, entityDamageName, damageScale, INVALID_JOINT );
		}
	}

	// Calculate damage to self
	const char* selfDamageName = NULL;
	if ( delta < player->crashlandSpeed_soft ) {			// Soft Fall
		//AI_SOFTLANDING = true;
		selfDamageName = player->spawnArgs.GetString( "def_damageSoftFall" );
	}
	else if ( delta < player->crashlandSpeed_fatal ) {		// Hard Fall
		//AI_HARDLANDING = true;
		selfDamageName = player->spawnArgs.GetString( "def_damageHardFall" );
	}
	else {											// Fatal Fall
		//AI_HARDLANDING = true;
		selfDamageName = player->spawnArgs.GetString( "def_damageFatalFall" );
	}

	if( *selfDamageName && damageScale > 0.0f && !noDamage ) {
		pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
		hhPlayer *tmp = player.GetEntity();
		player->StopSpiritWalk(true); // Invalidates player
		HH_ASSERT(!player.IsValid()); // If it's still valid, we failed to stop spirit walking
		tmp->Damage( NULL, NULL, fallDir, selfDamageName, damageScale, INVALID_JOINT );
	}
}

//==========================================================================
// hhDeathWalkProxy
//==========================================================================

CLASS_DECLARATION( hhSpiritProxy, hhDeathWalkProxy )
END_CLASS

hhDeathProxy::~hhDeathProxy(void) {
	HH_ASSERT( gameLocal.isMultiplayer || !player.IsValid() || player->IsDeathWalking() );
}

/*
================
hhDeathWalkProxy::Spawn
================
*/
void hhDeathWalkProxy::Spawn() {
	initialPos = GetOrigin();
	spawnArgs.GetFloat("bodyMoveScale", "512", bodyMoveScale);
	fl.takedamage = false;
	timeSinceStage2Started = 0;
}

void hhDeathWalkProxy::Save(idSaveGame *savefile) const {
	savefile->WriteVec3(initialPos);
	savefile->WriteFloat(bodyMoveScale);
	savefile->WriteInt(timeSinceStage2Started);
}

void hhDeathWalkProxy::Restore(idRestoreGame *savefile) {
	savefile->ReadVec3(initialPos);
	savefile->ReadFloat(bodyMoveScale);
	savefile->ReadInt(timeSinceStage2Started);
}

void hhDeathWalkProxy::Think() {
	float lerpTime = 0.05f;

	if (!player.IsValid() || !player.GetEntity() || !player->IsType(hhPlayer::Type) || !player->IsDeathWalking()) {
		//if player has become invalid or is no longer deathwalking, i wish to be removed.
		PostEventMS(&EV_Remove, 0);
		return;
	}

	hhSpiritProxy::Think();

	float spiritPercent = player->GetDeathWalkPower() / player->spawnArgs.GetFloat( "deathWalkPowerMax", "1000" );

	idVec3 newPos = initialPos;
	if (player->DeathWalkStage2()) {
		timeSinceStage2Started += gameLocal.msec;
		if (timeSinceStage2Started > player->spawnArgs.GetInt("deathwalkBodyDropDelayMS")) {
			newPos.z = initialPos.z - player->spawnArgs.GetFloat("deathwalkOffsetBelowPortal");
			lerpTime = 0.02f;
		}
	}
	else {
		timeSinceStage2Started = 0;
		newPos.z = initialPos.z + bodyMoveScale - spiritPercent*bodyMoveScale;
	}

	//lerp into that position
	idVec3 lerped;
	lerped.SLerp(GetOrigin(), newPos, lerpTime );
	SetOrigin(lerped);
}

void hhDeathWalkProxy::ActivateProxy( hhPlayer *owner, const idVec3& origin, const idMat3 &bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis ) {
	hhSpiritProxy::ActivateProxy(owner, origin, bboxAxis, newViewAxis, newViewAngles, newEyeAxis);
	
	physicsObj.SetContents(0); //not solid
	physicsObj.SetGravity(idVec3(0, 0, 0)); //no gravity

	SetInitialPos(origin); //this is our initial position to move vertically from
}

void hhDeathWalkProxy::SetInitialPos(const idVec3 &pos) {
	initialPos = pos;
}

void hhDeathWalkProxy::StartAnimation() {
	int spiritBlendTime = spawnArgs.GetInt( "spiritBlendTime", "400" );
	// Copy the current player animation to blend into the spirit anim
	GetAnimator()->CopyAnimations( *( player->GetAnimator() ) );

	// Play the initial animation
	int anim = GetAnimator()->GetAnim("deathfloat");
	GetAnimator()->ClearAllAnims( gameLocal.time,  spiritBlendTime );
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, spiritBlendTime );	
	netAnimType = 3; //rww
}



//==========================================================================
// hhDeathProxy
//==========================================================================

CLASS_DECLARATION( hhSpiritProxy, hhDeathProxy )
	EVENT( EV_SpawnEffect,			hhDeathProxy::Event_SpawnEffect )
	EVENT( EV_Activate,				hhDeathProxy::Event_Activate )
END_CLASS


void hhDeathProxy::Spawn() {
	lastPhysicalLocation.Zero();
	lastPhysicalAxis.Identity();
}

void hhDeathProxy::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	//idActor::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}

void hhDeathProxy::Event_SpawnEffect() {
}

void hhDeathProxy::ActivateProxy( hhPlayer *owner, const idVec3& origin, const idMat3 &bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis ) {
	hhSpiritProxy::ActivateProxy( owner, origin, bboxAxis, newViewAxis, newViewAngles, newEyeAxis );

	// Save "return to" location
	lastPhysicalLocation = origin;
	lastPhysicalAxis = bboxAxis;

	// Ragdoll the proxy
	StartRagdoll();
}

void hhDeathProxy::StartAnimation() {
	if (gameLocal.isMultiplayer) { //rww
		UpdateModelForPlayer(); //update proxy model for whatever the player is using.
	}
	// Set the start of the death proxy to the current animation in the player
	GetAnimator()->CopyAnimations( *( player->GetAnimator() ) );
	GetAnimator()->CopyPoses( *( player->GetAnimator() ) );
	netAnimType = 3; //rww
}

// mdl:  Handy for debugging the death proxy
void hhDeathProxy::Event_Activate() {
	hhPlayer *player = reinterpret_cast<hhPlayer *> (gameLocal.GetLocalPlayer());
	ActivateProxy(player, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), viewAxis, viewAngles, player->GetEyeAxis());
}



//==========================================================================
// hhMPDeathProxy
//==========================================================================

idCVar g_mpPlayerRagdollLife(			"g_mpPlayerRagdollLife",				"1500",			CVAR_GAME | CVAR_INTEGER, "player ragdoll life" );
idCVar g_mpSyncPlayerRagdoll(			"g_mpSyncPlayerRagdoll",				"0",			CVAR_GAME | CVAR_BOOL, "whether to sync mp ragdolls" );

const idEventDef EV_CorpseRemove( "<mpCorpseRemove>", NULL );

CLASS_DECLARATION( hhDeathProxy, hhMPDeathProxy )
	EVENT( EV_CorpseRemove,					hhMPDeathProxy::Event_CorpseRemove )
END_CLASS


/*
================
hhMPDeathProxy::Spawn
================
*/
void hhMPDeathProxy::Spawn() {
	hasInitial = false;
	didFling = false;

	int delay = g_mpPlayerRagdollLife.GetInteger();
	fl.clientEvents = true;
	PostEventMS(&EV_CorpseRemove, delay);

	SetShaderParm(SHADERPARM_TIME_OF_DEATH, MS2SEC(gameLocal.time+1000));
	SetSkinByName(spawnArgs.GetString("skin_death"));
	//GetPhysics()->SetGravity(idVec3(0,0,512)); cheesy spirit floating upward effect (doesn't work inside grav zones)
}

/*
================
hhMPDeathProxy::ActivateProxy
================
*/
void hhMPDeathProxy::ActivateProxy( hhPlayer *owner, const idVec3& origin, const idMat3 &bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis ) {
	hhDeathProxy::ActivateProxy(owner, origin, bboxAxis, newViewAxis, newViewAngles, newEyeAxis);

	hasInitial = true;
	initialPos = origin;
	initialRot = bboxAxis.ToCQuat();

	GetPhysics()->SetOrigin(initialPos);
	GetPhysics()->SetAxis(initialRot.ToMat3());
}

/*
================
hhMPDeathProxy::Event_CorpseRemove
================
*/
void hhMPDeathProxy::Event_CorpseRemove(void) {
	Hide();
	GetPhysics()->SetContents(0);
	if (!gameLocal.isClient) {
		PostEventMS(&EV_Remove, 1000);
	}
}

/*
================
hhMPDeathProxy::ActivateProxy
================
*/
void hhMPDeathProxy::SetFling(const idVec3 &point, const idVec3 &force) {
	//initialFlingPoint = point;
	initialFlingForce = force;
}

/*
================
hhMPDeathProxy::WriteToSnapshot
================
*/
void hhMPDeathProxy::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits(player.GetSpawnId(), 32);

	msg.WriteBits(IsActiveAF(), 1);

	bool physSync = g_mpSyncPlayerRagdoll.GetBool(); //the server can decide to do this.
	msg.WriteBits(physSync, 1);
	if (physSync) {
		GetPhysics()->WriteToSnapshot(msg);
	}
	else { //if we are not properly sync'ing then use just an initial orientation
		msg.WriteFloat(initialPos.x);
		msg.WriteFloat(initialPos.y);
		msg.WriteFloat(initialPos.z);

		msg.WriteFloat(initialRot.x);
		msg.WriteFloat(initialRot.y);
		msg.WriteFloat(initialRot.z);

		//FIXME get some of this from the server? may be necessary if a more complex method of determining the fling direction is devised.
		/*
		msg.WriteFloat(initialFlingPoint.x);
		msg.WriteFloat(initialFlingPoint.y);
		msg.WriteFloat(initialFlingPoint.z);
		*/

		msg.WriteFloat(initialFlingForce.x);
		msg.WriteFloat(initialFlingForce.y);
		msg.WriteFloat(initialFlingForce.z);
	}
}

/*
================
hhMPDeathProxy::ReadFromSnapshot
================
*/
void hhMPDeathProxy::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	player.SetSpawnId(msg.ReadBits(32));

	bool isRagging = !!msg.ReadBits(1);

	bool physSync = !!msg.ReadBits(1);

	if (physSync) {
		if (isRagging && !IsActiveAF()) { //then start ragging on the client
			if (player.IsValid()) { //update model for player before initiating ragdoll
				UpdateModelForPlayer();
			}
			StartRagdoll();
		}
		GetPhysics()->ReadFromSnapshot(msg);
	}
	else { //if we are not properly sync'ing then use just an initial orientation
		initialPos.x = msg.ReadFloat();
		initialPos.y = msg.ReadFloat();
		initialPos.z = msg.ReadFloat();

		initialRot.x = msg.ReadFloat();
		initialRot.y = msg.ReadFloat();
		initialRot.z = msg.ReadFloat();

		initialFlingForce.x = msg.ReadFloat();
		initialFlingForce.y = msg.ReadFloat();
		initialFlingForce.z = msg.ReadFloat();

		if (!hasInitial) { //if not set at all yet, put it here.
			GetPhysics()->SetOrigin(initialPos); //set before we start to ragdoll, so that the af starts out at a valid orientation
			GetPhysics()->SetAxis(initialRot.ToMat3());

			hasInitial = true;
		}

		if (isRagging && !IsActiveAF()) { //then start ragging on the client
			if (player.IsValid()) { //update model for player before initiating ragdoll
				UpdateModelForPlayer();
			}
			StartRagdoll();

			//set the initial orientation again after beginning ragdoll
			GetPhysics()->SetOrigin(initialPos);
			GetPhysics()->SetAxis(initialRot.ToMat3());
		}
	}
}

/*
================
hhMPDeathProxy::ClientPredictionThink
================
*/
void hhMPDeathProxy::ClientPredictionThink( void ) {
	if (!gameLocal.isNewFrame) {
		return;
	}

	Think();
	if (hasInitial && !didFling) {
		GetPhysics()->AddForce(0, GetPhysics()->GetOrigin(0), initialFlingForce*256.0f*256.0f);
		didFling = true;
	}
}


//==========================================================================
//
// hhPossessedProxy
//
// A version of the spirit proxy spawned when the player is possessed
//==========================================================================

CLASS_DECLARATION( hhSpiritProxy, hhPossessedProxy )
END_CLASS

void hhPossessedProxy::ActivateProxy( hhPlayer *owner, const idVec3& origin, const idMat3 &bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis ) {
	hhSpiritProxy::ActivateProxy(owner, origin, bboxAxis, newViewAxis, newViewAngles, newEyeAxis);

	Hide();
	physicsObj.SetContents( 0 );
}

/*
===============
hhSpiritProxy::DrawPlayerIcons
===============
*/
void hhSpiritProxy::DrawPlayerIcons( void ) {
	if ( !NeedsIcon() ) {
		//playerIcon.FreeIcon();
		playerTeamIcon.FreeIcon();
		return;
	}
	//player->UpdatePlayerIcons(); //update the owner's icon status
	//playerIcon.Draw( this, INVALID_JOINT );
	playerTeamIcon.Draw( this, INVALID_JOINT );
}

/*
===============
hhSpiritProxy::HidePlayerIcons
===============
*/
void hhSpiritProxy::HidePlayerIcons( void ) {
	//playerIcon.FreeIcon();
	playerTeamIcon.FreeIcon();
}

/*
===============
hhSpiritProxy::NeedsIcon
==============
*/
bool hhSpiritProxy::NeedsIcon( void ) {
	if (IsType(hhMPDeathProxy::Type)) {
		return false;
	}
	if (!player.IsValid()) {
		return false;
	}
	if (IsHidden()) {
		return false;
	}
	return player->entityNumber != gameLocal.localClientNum && ( /*player->isLagged || player->isChatting ||*/ gameLocal.gameType == GAME_TDM ) && gameLocal.EntInClientSnapshot(entityNumber);
}
