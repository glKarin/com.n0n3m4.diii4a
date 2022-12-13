#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/*
	method:
		walk up to ship console, activate gui
		gui sends command to vehicle to take control
		vehicle animates out of console around you
		control hand is added

	model specs:
		min: -64 -50 -64
		max:  64  50  64

		needs to be centered about the origin (in all axes)
		needs to fit a pilot collision box inside in his "piloting" animation
		needs to be collapsable into the bottom console
*/


//==========================================================================
//
//	hhTractorBeam
//
//==========================================================================

CLASS_DECLARATION(idClass, hhTractorBeam)
END_CLASS

hhTractorBeam::hhTractorBeam() {
	bAllow = true;
	owner = NULL;
	bindController = NULL;
	traceController = NULL;
	traceTarget = NULL;
	beam = NULL;
	targetLocalOffset = vec3_origin;
	targetID = 0;
	bActive = false;
	beamClientPredictTime = 0;
}

void hhTractorBeam::Save(idSaveGame *savefile) const {
	owner.Save(savefile);
	beam.Save(savefile);
	traceTarget.Save(savefile);
	bindController.Save(savefile);
	traceController.Save(savefile);

	savefile->WriteStaticObject(feedbackForce);
	savefile->WriteInt(tractorCost);
	savefile->WriteBool(bAllow);
	savefile->WriteBool(bActive);
	savefile->WriteVec3(offsetTraceStart);
	savefile->WriteVec3(offsetTraceEnd);
	savefile->WriteVec3(offsetEquilibrium);
	savefile->WriteVec3(targetLocalOffset);
	savefile->WriteInt(targetID);
}

void hhTractorBeam::Restore( idRestoreGame *savefile ) {
	owner.Restore(savefile);
	beam.Restore(savefile);
	traceTarget.Restore(savefile);
	bindController.Restore(savefile);
	traceController.Restore(savefile);

	savefile->ReadStaticObject(feedbackForce);
	savefile->ReadInt(tractorCost);
	savefile->ReadBool(bAllow);
	savefile->ReadBool(bActive);
	savefile->ReadVec3(offsetTraceStart);
	savefile->ReadVec3(offsetTraceEnd);
	savefile->ReadVec3(offsetEquilibrium);
	savefile->ReadVec3(targetLocalOffset);
	savefile->ReadInt(targetID);
}

void hhTractorBeam::WriteToSnapshot( idBitMsgDelta &msg ) const {
	//don't need to sync the beam and other components that are purely client-side

	msg.WriteBits(owner.GetSpawnId(), 32);
	msg.WriteBits(traceTarget.GetSpawnId(), 32);
	msg.WriteBits(targetID, 32);

	msg.WriteBits(bActive, 1);
	msg.WriteBits(bAllow, 1);
}

void hhTractorBeam::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	owner.SetSpawnId(msg.ReadBits(32));
	traceTarget.SetSpawnId(msg.ReadBits(32));
	targetID = msg.ReadBits(32);

	bool beamActive = !!msg.ReadBits(1);
	if (beamActive != bActive && beamClientPredictTime <= gameLocal.time) {
		if (beamActive) {
			Activate();
		}
		else {
			Deactivate();
		}
	}
	bAllow = !!msg.ReadBits(1);
}

void hhTractorBeam::SpawnComponents(hhShuttle *shuttle) {
	SetOwner(shuttle);

	idVec3 ownerOrigin = owner->GetOrigin();
	idMat3 ownerAxis = owner->GetAxis();
	idDict *ownerArgs = &owner->spawnArgs;

	tractorCost = ownerArgs->GetInt("tractorcost");
	offsetTraceStart = ownerArgs->GetVector("offset_tractortracestart");
	offsetTraceEnd = ownerArgs->GetVector("offset_tractortraceend");
	offsetEquilibrium = ownerArgs->GetVector("offset_tractorequilibrium");

	float tension = ownerArgs->GetFloat("tension", "1.0");
	float slack;
	if (gameLocal.isMultiplayer) {
		slack = ownerArgs->GetFloat("slack_mp", "1.0");
	}
	else {
		slack = ownerArgs->GetFloat("slack", "1.0");
	}
	float yawLimit = ownerArgs->GetFloat("yawlimit", "180");
	const char *handName = ownerArgs->GetString("def_tractorhand");
	const char *animName = ownerArgs->GetString("boundanim");

	if (gameLocal.isClient) {
		bindController = static_cast<hhBindController *>( gameLocal.SpawnClientObject(ownerArgs->GetString("def_bindController"), NULL) );
	}
	else {
		bindController = static_cast<hhBindController *>( gameLocal.SpawnObject(ownerArgs->GetString("def_bindController")) );
	}
	HH_ASSERT( bindController.IsValid() );
	bindController->fl.networkSync = false;

	bindController->SetTension(tension);
	bindController->SetSlack(slack);
	bindController->SetRiderParameters(animName, handName, yawLimit, 0.0f);
	bindController->SetOrigin(ownerOrigin + offsetEquilibrium * ownerAxis);
	bindController->Bind(owner.GetEntity(), true);
//	bindController->fl.robustDormant = true; // This beam could be going through walls, so set it to a robust dormancy check
	bindController->fl.neverDormant = true;
	bindController->SetShuttle(true);

	if (gameLocal.isClient) {
		traceController = gameLocal.SpawnClientObject( ownerArgs->GetString("def_traceController"), NULL );
	}
	else {
		traceController = gameLocal.SpawnObject( ownerArgs->GetString("def_traceController") );
	}
	HH_ASSERT( traceController.IsValid() );
	traceController->fl.networkSync = false;

	traceController->SetOrigin(ownerOrigin + offsetTraceEnd * ownerAxis);
	traceController->Bind(owner.GetEntity(), true);
//	traceController->fl.robustDormant = true; // This beam could be going through walls, so set it to a robust dormancy check
	traceController->fl.neverDormant = true;

	idVec3 offsetBeamPosition = ownerArgs->GetVector("offset_tractorbeamposition");
	idVec3 pos = ownerOrigin + offsetBeamPosition * ownerAxis;
	beam = hhBeamSystem::SpawnBeam(pos, ownerArgs->GetString("beam_tractor"), mat3_identity, true);
	HH_ASSERT( beam.IsValid() );
	beam->SetOrigin(pos);
	beam->SetAxis(ownerAxis);
	beam->Bind(owner.GetEntity(), false);
	beam->Activate( false );
	beam->fl.robustDormant = true; // This beam could be going through walls, so set it to a robust dormancy check

	float feedbacktension = ownerArgs->GetFloat("feedbacktension");
	feedbackForce.SetRestoreFactor(feedbacktension);
	float feedbackslack = ownerArgs->GetFloat("feedbackslack");
	feedbackForce.SetRestoreSlack(feedbackslack);
}

bool hhTractorBeam::Exists() {
	return (owner.IsValid() && bindController.IsValid() && traceController.IsValid() && beam.IsValid());
}

bool hhTractorBeam::IsAllowed() {
	return (Exists() && bAllow && /*!owner->IsDocked() &&*/ owner->HasPower(tractorCost) && owner->noTractorTime <= gameLocal.time);
}

bool hhTractorBeam::IsActive() {
	return (Exists() && bActive);
}

bool hhTractorBeam::IsLocked() {
	return (IsActive() && GetTarget() != NULL);
}

idEntity *hhTractorBeam::GetTarget() {
	if (!bindController.IsValid() || !bindController.GetEntity()) { //rww - in the case of no tractor beam (testing concept for mp), this controller may not exist, so check it.
		return NULL;
	}
	return bindController->GetRider();
}

void hhTractorBeam::RequestState(bool wantsActive) {
	if (wantsActive && !IsActive() && IsAllowed()) {
		Activate();
	}
	else if (!wantsActive && IsActive()) {
		Deactivate();
	}
}

void hhTractorBeam::Activate() {
	bActive = true;
	beamClientPredictTime = gameLocal.time + USERCMD_MSEC*4;
	owner->StartSound("snd_tractor", SND_CHANNEL_TRACTORBEAM);
	owner->StartSound("snd_tractor_start", SND_CHANNEL_ANY);
}

void hhTractorBeam::Deactivate() {
	if ( !gameLocal.isMultiplayer && GetTarget() && GetTarget()->IsType( hhMonsterAI::Type ) ) {
		idVec3 velocity = GetTarget()->GetPhysics()->GetLinearVelocity();
		hhMonsterAI *targetAI = static_cast<hhMonsterAI*>( GetTarget() );
		if ( targetAI && owner.IsValid() ) {
			targetAI->SetEnemy( owner.GetEntity()->GetPilot() );
		}
		if ( velocity.Length() > GetTarget()->spawnArgs.GetFloat( "tractor_kill_speed", "800" ) ) {
			if ( targetAI ) {
				targetAI->Damage( NULL, NULL, idVec3( 0,0,0 ), "damage_monsterfall", 1, INVALID_JOINT ); 
				if ( targetAI->GetAFPhysics() ) {
					targetAI->GetAFPhysics()->SetLinearVelocity( velocity * GetTarget()->spawnArgs.GetFloat( "tractor_speed_scale", "15" ) );
				}
			}
		}
	}
	bActive = false;
	beamClientPredictTime = gameLocal.time + USERCMD_MSEC*4;
	if (beam.IsValid()) {
		beam->Activate( false );
		beam->SetTargetEntity(NULL);
	}
	if (owner.IsValid()) {
		owner->StopSound(SND_CHANNEL_TRACTORBEAM);
		owner->StartSound("snd_tractor_stop", SND_CHANNEL_ANY);
	}
	if (GetTarget()) {
		GetTarget()->fl.isTractored = false; //rww
	}
	if (bindController.IsValid()) {
		bindController->Detach();
	}
	feedbackForce.SetEntity(NULL);
}

bool hhTractorBeam::ValidTarget(idEntity *ent) {
	bool b =	ent &&
				!(ent->IsType(idActor::Type) && static_cast<idActor*>(ent)->InVehicle()) &&
				ent->GetPhysics() && 
				ent->GetPhysics()->GetContents() != 0 &&
				!ent->IsHidden() &&
				ent->fl.canBeTractored;

	if (gameLocal.isMultiplayer && b) { //rww - if mp and a valid target thus far, perform mp-only check(s)
		if (ent->IsType(hhPlayer::Type)) {
			hhPlayer *pl = static_cast<hhPlayer *>(ent);
			if (pl->IsDead()) { //if he died, let go.
				b = false;
			}
			else if (pl->IsWallWalking()) { //don't allow grabbing wallwalking players in mp
				b = false;
			}
		}
		else if (ent->IsType(hhMPDeathProxy::Type)) { //don't grab temporary client-based corpses
			b = false;
		}
	}

	return b;
}

void hhTractorBeam::ReleaseTarget() {
	// Release the target, but stay active
	beam->SetTargetEntity(NULL);
	if (GetTarget()) {
		GetTarget()->fl.isTractored = false; //rww
	}
	bindController->Detach();
	feedbackForce.SetEntity(NULL);
}

void hhTractorBeam::LaunchTarget(float speed) {
	idEntity *target = bindController->GetRider();
	ReleaseTarget();
	if (target) {
		idVec3 vel = owner->GetAxis()[0] * speed;
		target->GetPhysics()->SetLinearVelocity(vel);
	}
}

idEntity *hhTractorBeam::TraceForTarget(trace_t &results) {
	idVec3 start = owner->GetOrigin() + offsetTraceStart * owner->GetAxis();
	idVec3 end;
	if (GetTarget() && results.c.entityNum != ENTITYNUM_NONE) {
		end = GetTarget()->GetOrigin();
	}
	else {
		end = traceController->GetOrigin();
	}
	gameLocal.clip.TracePoint( results, start, end, MASK_TRACTORBEAM, owner.GetEntity() );

	idEntity *validEntity = NULL;
	if (results.fraction < 1.0f) {
		idEntity *entity = gameLocal.GetTraceEntity(results);
		if (ValidTarget(entity)) {
			if (entity && (entity == GetTarget() || !entity->fl.isTractored)) { //rww - if i'm not grabbing it already, don't grab it if it's tractored already.
				validEntity = entity;
			}
		}
	}

	if (beam.IsValid() && !GetTarget()) {
		beam->SetTargetLocation(results.endpos); //rww - in any case, set the beam's target pos to the trace endpos
	}

	return validEntity;
}

void hhTractorBeam::AttachTarget(idEntity *target, trace_t &results) {
	if (target->IsType(idAFEntity_Base::Type) &&			// Active ragdoll
		static_cast<idAFEntity_Base*>(target)->IsActiveAF()) {
		idAFEntity_Base *af = static_cast<idAFEntity_Base*>(target);
		//int joint = CLIPMODEL_ID_TO_JOINT_HANDLE( results.c.id );
		targetID = af->BodyForClipModelId(results.c.id);
		targetLocalOffset = vec3_origin;	// apply to center of mass
	}
	else if (target->IsType(idActor::Type)) {				// Non-ragdolled actor
		targetID = 0;
		targetLocalOffset = target->GetPhysics()->GetBounds().GetCenter();// - target->GetOrigin();

		//rww - for mp kill credits
		if (gameLocal.isMultiplayer) {
			if (target->IsType(hhPlayer::Type) && owner->GetPilot() && owner->GetPilot()->IsType(hhPlayer::Type)) {
				hhPlayer *plTarget = static_cast<hhPlayer *>(target);
				plTarget->airAttacker = owner->GetPilot();
				plTarget->airAttackerTime = gameLocal.time + 300;
			}
			else if (target->IsType(hhSpiritProxy::Type)) { //in mp, snap player back to their body when their prox is tractored
				target->Damage(owner.GetEntity(), owner.GetEntity(), vec3_origin, "damage_generic", 0.0f, -1); //call damage, but don't actually do damage.
			}
		}
	}
	else {													// Everything else
		targetID = results.c.id;
		targetLocalOffset = ( results.c.point - target->GetOrigin(targetID) ) * target->GetAxis(targetID).Transpose();
	}
	bindController->Attach(target, true, targetID, targetLocalOffset);
	beam->SetTargetEntity(target, targetID, targetLocalOffset);
	beam->SetArcVector(owner->GetAxis()[0]);
	feedbackForce.SetEntity(owner.GetEntity());

	target->fl.isTractored = true; //rww
}

void hhTractorBeam::Update() {
	trace_t results;

	if (Exists()) {
		if (IsActive()) {
			if (!IsAllowed() || !owner->ConsumePower(tractorCost)) {
				// Ran out of power or no longer allowed
				Deactivate();
			}

			if (IsLocked()) {
				if (!ValidTarget(GetTarget())) {
					// Locked target is no longer valid
					ReleaseTarget();
				}
				else if (gameLocal.isMultiplayer && GetTarget() && GetTarget()->IsType(hhPlayer::Type)) {
					//rww - just hint the player in the direction of the shuttle that has them picked up for mp (help with disorientation)
					float frametime = ((float)(gameLocal.time - gameLocal.previousTime))/USERCMD_MSEC;
					hhPlayer *targetPl = static_cast<hhPlayer *>(GetTarget());


                    idQuat plOri = targetPl->GetViewAngles().ToQuat();
					idVec3 targetPos = owner->GetOrigin();
					targetPos.z -= 80.0f;

					idVec3 t = (targetPos-targetPl->GetOrigin());
					t.Normalize();
					idQuat idealOri = t.ToMat3().ToQuat();

					idQuat finalOri;
					finalOri.Slerp(plOri, idealOri, 0.02f*frametime);

					targetPl->SetOrientation(targetPl->GetOrigin(), targetPl->GetPhysics()->GetAxis(), finalOri.ToMat3()[0], finalOri.ToAngles());
				}
			}
		}

		// Do the trace as long as we're not already locked, so the GUI can use it to show the cursor
		if (!IsAllowed() || IsLocked()) {
			//traceTarget = NULL;
			//rww - changed to continue tracing even after grabbing something

			results.c.entityNum = 0; //try tracing to the target
			traceTarget = TraceForTarget(results);
			if (traceTarget != GetTarget()) {
				results.c.entityNum = ENTITYNUM_NONE; //try tracing forward along beam
				traceTarget = TraceForTarget(results);
				if (traceTarget != GetTarget()) {
					ReleaseTarget();
					if (beam.IsValid()) {
						beam->SetTargetLocation(results.endpos);
					}
				}
			}
		}
		else {
			traceTarget = TraceForTarget(results);

			if (IsActive()) {
				if (traceTarget != NULL) {
					AttachTarget(traceTarget.GetEntity(), results);
				}
				//rww - instead of doing this and giving a misleading trace result, setting to the actual trace.endpos
				/*
				else {
					// No target found, but leave beam out there
					beam->SetTargetEntity(traceController.GetEntity());
				}
				*/
			}
		}

		if (IsActive()) {
			// Update beam based on shuttle orientation
			if (!beam->IsActivated()) {
				beam->Activate( true );
			}
			beam->SetArcVector( owner->GetAxis()[0] );
		}

		// Update the feedback force
		if (IsLocked()) {
			idVec3 p = GetTarget()->GetOrigin(targetID) + targetLocalOffset * GetTarget()->GetAxis(targetID);
			idVec3 a = owner->GetOrigin() - p;
			a.Normalize();
			a *= offsetEquilibrium.Length();
			idVec3 idealPosition = p + a;
			feedbackForce.SetTarget(idealPosition);
			feedbackForce.Evaluate(gameLocal.time);
		}
	}
}



//==========================================================================
//
//	hhShuttle
//
//==========================================================================
const idEventDef EV_Vehicle_TractorBeam( "tractorBeam" );
const idEventDef EV_Vehicle_TractorBeamDone( "tractorBeamDone" );

CLASS_DECLARATION(hhVehicle, hhShuttle)
	EVENT( EV_PostSpawn,					hhShuttle::Event_PostSpawn )
	EVENT( EV_Vehicle_FireCannon,			hhShuttle::Event_FireCannon )
	EVENT( EV_Vehicle_TractorBeam,			hhShuttle::Event_ActivateTractorBeam )
	EVENT( EV_Vehicle_TractorBeamDone,		hhShuttle::Event_DeactivateTractorBeam )
END_CLASS


hhShuttle::~hhShuttle() {
	if (renderEntity.gui[0]) {
		gameLocal.FocusGUICleanup(renderEntity.gui[0]); //HUMANHEAD rww
		uiManager->DeAlloc(renderEntity.gui[0]);
		renderEntity.gui[0] = NULL;
	}
}

void hhShuttle::Spawn() {
	bSputtering = false;
	malfunctionThrustFactor = spawnArgs.GetFloat( "malfunctionThrustFactor" );

	idealTipState = TIP_STATE_NONE;
	curTipState = TIP_STATE_NONE;
	nextTipChange = gameLocal.time;

	// Fade console in
	SetShaderParm(4, -MS2SEC(gameLocal.time));		// time
	SetShaderParm(5, -1.0f);						// Dir

	teleportDest = GetOrigin() + idVec3(-64, 0, 0) * GetPhysics()->GetAxis();
	teleportAngles = idAngles( 0.0f, GetPhysics()->GetAxis().ToAngles().yaw, 0.0f );

	terminalVelocitySquared = spawnArgs.GetFloat( "terminalvelocity" );
	terminalVelocitySquared *= terminalVelocitySquared;

	noTractorTime = 0; //rww

	fl.clientEvents = true; //rww - allow events to be posted on client for this entity

	PostEventMS( &EV_PostSpawn, 0 );

	fl.networkSync = true;
}

void hhShuttle::Event_PostSpawn() {
	// Spawn all sub-entities after initial population for multiplayer's sake

	// Spawn components for tractor beam
	const idKeyValue *kv1 = spawnArgs.FindKey("attackFunc");
	const idKeyValue *kv2 = spawnArgs.FindKey("altAttackFunc");
	if ((kv1 && !kv1->GetValue().Icmp("tractorBeam")) || (kv2 && !kv2->GetValue().Icmp("tractorBeam")) ) {
		tractor.SpawnComponents( this );
	}

	// Spawn Model based thrusters
	const char *thrusterDef = spawnArgs.GetString("def_thruster");
	if (thrusterDef && *thrusterDef) {
		// Spawn thrusters with appropriate direction vectors
		idVec3 offset;
		char suffix[THRUSTER_DIRECTIONS]	= { 'F',		'B',		'L',		'R',		'U',		'D' };
		int directions[THRUSTER_DIRECTIONS]	= {	MASK_POSX,	MASK_NEGX,	MASK_POSY,	MASK_NEGY,	MASK_POSZ,	MASK_NEGZ };
		for (int i=0; i<THRUSTER_DIRECTIONS; i++) {
			offset = spawnArgs.GetVector(va("offset_thrust%c", suffix[i]));
			thrusters[i] = SpawnThruster(offset, idVec3(directions[i]), thrusterDef, i==THRUSTER_BACK);
		}
	}
}

void hhShuttle::Save(idSaveGame *savefile) const {
	savefile->WriteStaticObject( tractor );
	for (int i=0; i<THRUSTER_DIRECTIONS; i++) {
		thrusters[i].Save(savefile);
	}
	savefile->WriteFloat( terminalVelocitySquared );
	savefile->WriteVec3( teleportDest );
	savefile->WriteAngles( teleportAngles );
	savefile->WriteFloat( malfunctionThrustFactor );
	savefile->WriteBool( bSputtering );
	savefile->WriteInt(noTractorTime);
	savefile->WriteInt( idealTipState );
	savefile->WriteInt( curTipState );
	savefile->WriteInt( nextTipChange );
}

void hhShuttle::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( tractor );
	for (int i=0; i<THRUSTER_DIRECTIONS; i++) {
		thrusters[i].Restore(savefile);
	}
	savefile->ReadFloat( terminalVelocitySquared );
	savefile->ReadVec3( teleportDest );
	savefile->ReadAngles( teleportAngles );
	savefile->ReadFloat( malfunctionThrustFactor );
	savefile->ReadBool( bSputtering );
	savefile->ReadInt(noTractorTime);
	savefile->ReadInt( idealTipState );
	savefile->ReadInt( curTipState );
	savefile->ReadInt( nextTipChange );
}

void hhShuttle::WriteToSnapshot( idBitMsgDelta &msg ) const {
	hhVehicle::WriteToSnapshot(msg);

	/*
	int i;
	for (i = 0; i < THRUSTER_DIRECTIONS; i++) {
		msg.WriteBits(thrusters[i].GetSpawnId(), 32);
	}
	*/

	msg.WriteFloat(renderEntity.shaderParms[4]);
	msg.WriteFloat(renderEntity.shaderParms[5]);

	msg.WriteBits(bSputtering, 1);
	msg.WriteBits(noTractorTime, 32);

	tractor.WriteToSnapshot(msg);
}

void hhShuttle::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hhVehicle::ReadFromSnapshot(msg);

	/*
	int i;
	for (i = 0; i < THRUSTER_DIRECTIONS; i++) {
		int spawnId = msg.ReadBits(32);
		if (!spawnId) {
			thrusters[i] = NULL;
		}
		else {
			thrusters[i].SetSpawnId(spawnId);
		}
	}
	*/

	SetShaderParm(4, msg.ReadFloat());
	SetShaderParm(5, msg.ReadFloat());

	bSputtering = !!msg.ReadBits(1);
	noTractorTime = msg.ReadBits(32);

	tractor.ReadFromSnapshot(msg);
}
 
void hhShuttle::ClientPredictionThink( void ) {
	if (fl.hidden) {
		return;
	}

	Think();

	UpdateVisuals();
	Present();
}


// These for model based thrusters ------------------------------------------------

hhVehicleThruster *hhShuttle::SpawnThruster(idVec3 &offset, idVec3 &dir, const char *thrusterName, bool master) {
	idVec3 pos = GetOrigin() + offset * GetAxis();
	idVec3 direction = dir * GetAxis();
	idMat3 axis = direction.ToMat3();

	idDict args;
	args.SetVector("origin", pos);
	args.SetBool("soundmaster", master);
	hhVehicleThruster *thruster;
	if (gameLocal.isClient) {
		thruster = static_cast<hhVehicleThruster*>(gameLocal.SpawnClientObject(thrusterName, &args));
	}
	else {
		thruster = static_cast<hhVehicleThruster*>(gameLocal.SpawnObject(thrusterName, &args));
	}
	if (thruster) {
		thruster->fl.networkSync = false;

		thruster->SetOrigin(pos);
		thruster->SetAxis(axis);
		thruster->Bind(this, true);
		thruster->SetOwner(this);
		thruster->Hide();
		thruster->SetSmoker(master, offset, dir);
	}
	return thruster;
}

void hhShuttle::FireThrusters( const idVec3& impulse ) {
	idVec3 curVel = physicsObj.GetLinearVelocity();
	idVec3 excess( vec3_zero );
	idVec3 Iu( vec3_zero );
	idVec3 Vu( vec3_zero );
	float impulseLength = 0.0f;
	idVec3 finalImpulse = impulse;

	// Subtract portion of impulse out that is in direction of current terminal velocity
	//FIXME: not sure if this should be in the physics code
	if( !IsNoClipping() && curVel.LengthSqr() > terminalVelocitySquared ) {
		Iu = finalImpulse;
		impulseLength = Iu.Normalize();
		Vu = curVel;
		Vu.Normalize();
		excess = (impulseLength * (Iu * Vu)) * Vu;
		finalImpulse -= excess;	
	}

	// Apply wacky controls when dying
	if (health <= 0) {
		finalImpulse += hhUtils::RandomVector() * 127.0f * malfunctionThrustFactor;
	}

	if (finalImpulse.Length() > VECTOR_EPSILON) { 
		ConsumePower(thrusterCost); 
	}

	// Play sputter sound if trying to move without enough power
	if (!HasPower(thrusterCost)) {
		if (finalImpulse != vec3_origin && !bSputtering) {
			StartSound("snd_sputter", SND_CHANNEL_ANY);
			bSputtering = true;
		}
		else {
			bSputtering = false;
		}

		finalImpulse.Zero();
	}
	
	ApplyImpulse( finalImpulse );
}

void hhShuttle::ApplyBoost( float magnitude ) {
	ApplyImpulse( GetAxis()[0] * magnitude );
}

// Update all tick based effects
void hhShuttle::UpdateEffects( const idVec3& localThrust ) {
	// Update thrusters on/off state based on direction of velocity vector
	int directionMask = localThrust.DirectionMask();
	if ( GetPilot() && GetPilot()->IsType( idAI::Type ) ) { 
		idVec3 vel = GetPhysics()->GetLinearVelocity();
		if ( -GetAxis()[2] * vel > 0 ) {
			thrusters[THRUSTER_TOP]->SetThruster( true );
			thrusters[THRUSTER_TOP]->Update( localThrust );
		} else {
			thrusters[THRUSTER_TOP]->SetThruster( false );
			thrusters[THRUSTER_TOP]->Update( localThrust );
		}
		if ( GetAxis()[2] * vel > 0 ) {
			thrusters[THRUSTER_BOTTOM]->SetThruster( true );
			thrusters[THRUSTER_BOTTOM]->Update( localThrust );
		} else {
			thrusters[THRUSTER_BOTTOM]->SetThruster( false );
			thrusters[THRUSTER_BOTTOM]->Update( localThrust );
		}
		if ( GetAxis()[1] * vel > 0 ) {
			thrusters[THRUSTER_RIGHT]->SetThruster( true );
			thrusters[THRUSTER_RIGHT]->Update( localThrust );
		} else {
			thrusters[THRUSTER_RIGHT]->SetThruster( false );
			thrusters[THRUSTER_RIGHT]->Update( localThrust );
		}
		if ( -GetAxis()[1] * vel > 0 ) {
			thrusters[THRUSTER_LEFT]->SetThruster( true );
			thrusters[THRUSTER_LEFT]->Update( localThrust );
		} else {
			thrusters[THRUSTER_LEFT]->SetThruster( false );
			thrusters[THRUSTER_LEFT]->Update( localThrust );
		}
	} else {
		for (int thrusterIndex=0; thrusterIndex<THRUSTER_DIRECTIONS; thrusterIndex++) {
			if (thrusters[thrusterIndex].IsValid()) {
				thrusters[thrusterIndex]->SetThruster( INDEX_IN_MASK(directionMask, thrusterIndex) );
				thrusters[thrusterIndex]->Update( localThrust );
			}
		}
	}
}

// Static function to determine if a pilot is suitable
bool hhShuttle::ValidPilot( idActor *act ) {
	if (act && act->health > 0) {
		if (act->IsType(hhPlayer::Type)) {
			hhPlayer *player = static_cast<hhPlayer*>(act);
			if( !player->IsSpiritOrDeathwalking() && !player->IsPossessed() ) {
				return true;
			}
		}
		else if (act->IsType(idAI::Type)) {
			idAI *ai = static_cast<idAI*>(act);
			if (ai->spawnArgs.GetBool("canPilotShuttle")) {
				return true;
			}
		}
	}
	return false;
}

bool hhShuttle::WillAcceptPilot( idActor *act ) {
	return IsConsole() && hhShuttle::ValidPilot( act ) && CanBecomeVehicle(act);
}

void hhShuttle::AcceptPilot( hhPilotVehicleInterface* pilotInterface ) {
	hhVehicle::AcceptPilot( pilotInterface );

	// Fade the console out
	SetShaderParm(4, -MS2SEC(gameLocal.time));		// time
	SetShaderParm(5, 1.0f);							// dir

	// Alert the hud that control has been taken (currently unused)
	if (renderEntity.gui[0]) {
		renderEntity.gui[0]->Trigger(gameLocal.time);
	}
}

void hhShuttle::EjectPilot() {

	// Fade ship out
	SetShaderParm(4, -MS2SEC(gameLocal.time));		// time
	SetShaderParm(5, -1.0f);						// dir

	tractor.RequestState(false);

	for (int ix=0; ix<THRUSTER_DIRECTIONS; ix++) {
		if (thrusters[ix].IsValid()) {
			thrusters[ix]->SetDying(false);
			thrusters[ix]->SetThruster(false);
			thrusters[ix]->Update(vec3_origin);
		}
	}

	hhVehicle::EjectPilot();
}

void hhShuttle::ProcessPilotInput( const usercmd_t* cmds, const idAngles* viewAngles ) {
	hhVehicle::ProcessPilotInput( cmds, viewAngles );

	if( cmds && IsVehicle() ) {
		UpdateEffects( idVec3(cmds->forwardmove, -cmds->rightmove, cmds->upmove)  );
	}
}

/*
bool hhShuttle::UsesCrosshair() const {
	return (!IsDocked() || dock->AllowsFiring()) && hhVehicle::UsesCrosshair();
}
*/

void hhShuttle::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location) {
	for(int ix=0; ix<THRUSTER_DIRECTIONS; ix++) {
		if( thrusters[ix].IsValid() ) {
			thrusters[ix]->SetDying(true);
		}
	}

	hhVehicle::Killed(inflictor, attacker, damage, dir, location);
}

void hhShuttle::DrawHUD( idUserInterface* _hud ) {
	if( !_hud ) {
		return;
	}

	float spawnPower = spawnArgs.GetInt( "maxPower" );
	bool bDocked = dock.IsValid();
	bool bDockedInTransporter = bDocked && dock->IsType(hhShuttleTransport::Type);
	bool bDockLocked = bDocked && dock->IsLocked();
	bool bNeedsRecharge = (health < spawnHealth) || (currentPower < spawnPower);
	_hud->SetStateBool( "docked", bDocked );
	_hud->SetStateBool( "dockedtransporter", bDockedInTransporter );
	_hud->SetStateBool( "docklocked", bDockLocked );
	_hud->SetStateBool( "recharging", bNeedsRecharge && bDocked && dock->Recharges() );
	_hud->SetStateBool( "crosshair", fireController && fireController->UsesCrosshair() );
	_hud->SetStateBool( "lowhealth", health/(float)spawnHealth < 0.20f );
	_hud->SetStateBool( "lowpower", currentPower/spawnPower < 0.20f );

	float velfrac = physicsObj.GetLinearVelocity().LengthSqr() / terminalVelocitySquared;
//	float velfrac = controller.GetThrustBooster();
	velfrac = idMath::ClampFloat( 0.0f, 1.0f, velfrac );
	_hud->SetStateFloat( "velocityfraction", velfrac );

	bool bHasTractor = tractor.Exists();
	bool bTractorActive = tractor.IsActive();
	bool bTractorLocked = tractor.IsLocked();
	bool bTractorSighted = tractor.IsAllowed() && (tractor.GetTraceTarget() != NULL);
	bool bTractorAllowed = tractor.IsAllowed();
	_hud->SetStateBool( "hastractor", bHasTractor );
	_hud->SetStateBool( "tractoractive", bTractorActive );
	_hud->SetStateBool( "tractorsighted", bTractorSighted );
	_hud->SetStateBool( "tractorlocked", bTractorLocked );
	_hud->SetStateBool( "tractorallowed", bTractorAllowed );

	// HUMANHEAD PCF pdm 05-18-06: Removed non-octal mass from shuttle hud
/*	if( bHasTractor && bTractorLocked ) {
		float mass = tractor.GetTarget()->GetPhysics()->GetMass();
		_hud->SetStateInt("tractormass", (int) mass);
		const char *massFormatter = common->GetLanguageDict()->GetString("#str_41161");
		_hud->SetStateString("tractormasstext", va(massFormatter, (int)mass));
	}*/
	// HUMANHEAD END

	if (GetPilot() && GetPilot()->IsType(hhPlayer::Type)) {

		if (!g_tips.GetBool()) {
			idealTipState = TIP_STATE_NONE;
			nextTipChange = gameLocal.time;
		}

		// Transition to desired tip state, always passing through TIP_STATE_NONE
		if ( gameLocal.time >= nextTipChange && idealTipState != curTipState ) {

			if (curTipState != TIP_STATE_NONE) {			// Turn off old tip
				_hud->HandleNamedEvent( "shuttleTipWindowDown" );
				nextTipChange = gameLocal.time + 300;
				curTipState = TIP_STATE_NONE;
			}
			else {											// Turn on new tip
				const char *tip = NULL;
				switch(idealTipState) {
					case TIP_STATE_NONE:
						break;
					case TIP_STATE_DOCKED:
						gameLocal.SetTip(_hud, NULL, "", NULL, NULL, "tip1");
						tip = spawnArgs.GetString("text_exittip");
						gameLocal.SetTip(_hud, "_attackalt", tip, NULL, NULL, "tip2");
						_hud->HandleNamedEvent( "shuttleTipWindowUp" );
						curTipState = idealTipState;
						break;
					case TIP_STATE_UNDOCKED:
						tip = spawnArgs.GetString("text_cannontip");
						gameLocal.SetTip(_hud, "_attack", tip, NULL, NULL, "tip1");
						tip = spawnArgs.GetString("text_tractortip");
						gameLocal.SetTip(_hud, "_attackalt", tip, NULL, NULL, "tip2");
						_hud->HandleNamedEvent( "shuttleTipWindowUp" );
						curTipState = idealTipState;

						// Go away in a little while
						idealTipState = TIP_STATE_NONE;
						nextTipChange = gameLocal.time + 3000;
						break;
				}
			}

		}
	}

	_hud->StateChanged(gameLocal.time);
	hhVehicle::DrawHUD( _hud );
}

void hhShuttle::SetDock( const hhDock* dock ) {
	hhVehicle::SetDock(dock);

	bool bDocked = (dock != NULL);
	bool bDockedInTransporter = bDocked && dock->IsType(hhShuttleTransport::Type);

	if (bDocked && !bDockedInTransporter) {
		// Just entered dock, display tip
		idealTipState = TIP_STATE_DOCKED;
		nextTipChange = gameLocal.time;

		// Just entered dock, set this one as my respawn point
		idVec3 location = dock->GetOrigin() + dock->spawnArgs.GetVector("offset_console") * dock->GetAxis();
		teleportDest = location + idVec3(-64, 0, 0) * GetPhysics()->GetAxis();
		teleportAngles = idAngles( 0.0f, dock->GetAxis().ToAngles().yaw, 0.0f );
	}
}

void hhShuttle::Undock() {
	hhVehicle::Undock();

	// Just left dock, display tip
	idealTipState = TIP_STATE_UNDOCKED;
	nextTipChange = gameLocal.time;
}

void hhShuttle::Ticker() {
	// Handle noclipping
	physicsObj.SetContents(IsNoClipping() ? 0 : vehicleContents );
	physicsObj.SetClipMask(IsNoClipping() ? 0 : vehicleClipMask );

	if( IsVehicle() && IsDocked() ) {
		dock->UpdateAxis( GetAxis() );
	}

	tractor.Update();
}

void hhShuttle::RemoveVehicle() {
	if (IsDocked()) {
		dock->ShuttleExit(this);
		dock = NULL;
	}
	GetPhysics()->SetGravity( vec3_origin );	// so shuttle doesn't bounce on ground and make noise
	hhVehicle::RemoveVehicle();
}

void hhShuttle::SetConsoleModel() {
	hhVehicle::SetConsoleModel();
	
	// Fade console in	
	SetShaderParm(4, -MS2SEC(gameLocal.time));		// time
	SetShaderParm(5, -1.0f);						// Dir
	// Need to add gui back(SetModel strips it)
	if (!renderEntity.gui[0]) { //rww - don't double up guis
		renderEntity.gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true);
	}
}

void hhShuttle::SetVehicleModel() {
	hhVehicle::SetVehicleModel();

	// Fade shuttle in
	SetShaderParm(4, -MS2SEC(gameLocal.time));		// time
	SetShaderParm(5, 1.0f);							// Dir
	validThrustTime = gameLocal.time + spawnArgs.GetInt("delay_thrust");
}

void hhShuttle::RecoverFromDying() {
	StopSound(SND_CHANNEL_DYING);
	CancelEvents(&EV_VehicleExplode);
	if (domelight.IsValid()) {
		domelight->SetLightParm(7, 0.0f);
	}
	for(int ix=0; ix<THRUSTER_DIRECTIONS; ix++) {
		if (thrusters[ix].IsValid()) {
			thrusters[ix]->SetDying(false);
		}
	}
}

void hhShuttle::PerformDeathAction(int deathAction, idActor *savedPilot, idEntity *attacker, idVec3 &dir) {
	switch(deathAction) {
		case 2:		// Teleport pilot
			if (!gameLocal.isMultiplayer) { //rww - fall through on purpose
				if (savedPilot->IsType(hhPlayer::Type)) {
					idVec3 origin;
					idMat3 axis;
					idVec3 viewDir;
					idAngles angles;
					idMat3 eyeAxis;
					static_cast<hhPlayer*>(savedPilot)->GetResurrectionPoint( origin, axis, viewDir, angles, eyeAxis, savedPilot->GetPhysics()->GetAbsBounds(), teleportDest, teleportAngles.ToMat3(), teleportAngles.ToForward(), teleportAngles );
					static_cast<hhPlayer*>(savedPilot)->DeathWalk( origin, axis, viewDir.ToMat3(), angles, eyeAxis );
				}
				else {
					savedPilot->Teleport(teleportDest, teleportAngles, NULL);
				}
				break;
			}
		default:
			hhVehicle::PerformDeathAction(deathAction, savedPilot, attacker, dir);
			break;
	}
}

void hhShuttle::InitiateRecharging() {
}

void hhShuttle::FinishRecharging() {
	if (GetPilot() != NULL) {
		fl.takedamage = true;
	}
}

void hhShuttle::Event_FireCannon() {
	if (IsDocked() && GetPilot() && GetPilot()->IsType(idAI::Type) && !dock->AllowsFiring()) {
		// Let monsters boost out of docks a bit
		ApplyBoost( 127.0f * 0.25f );
		return;
	}

	if( IsDocked() && spawnArgs.GetBool("dockBoost") && !dock->AllowsFiring() ) {
		ApplyBoost( 127.0f * dockBoostFactor );
		return;
	}
	if( spawnArgs.GetBool("tractorlaunch") && GetTractorTarget() ) {
		LaunchTractorTarget(spawnArgs.GetFloat("launch_speed"));
		bDisallowAttackUntilRelease = true;
		bDisallowAltAttackUntilRelease = true;
		return;
	}
	if ( !IsDocked() || dock->AllowsFiring()) {
		hhVehicle::Event_FireCannon();
	}
}

void hhShuttle::Event_ActivateTractorBeam() {
	tractor.RequestState( true );
}

void hhShuttle::Event_DeactivateTractorBeam() {
	tractor.RequestState( false );
}
