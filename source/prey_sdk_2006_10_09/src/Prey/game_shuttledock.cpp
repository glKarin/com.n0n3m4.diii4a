
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//-----------------------------------------------------------------------
//
// hhShuttleDock
//
//-----------------------------------------------------------------------
const idEventDef EV_TrySpawnConsole("<tryspawnconsole>", NULL);

CLASS_DECLARATION(hhDock, hhShuttleDock)
	EVENT( EV_TrySpawnConsole,			hhShuttleDock::Event_SpawnConsole)
	EVENT( EV_Remove,					hhShuttleDock::Event_Remove)
	EVENT( EV_PostSpawn,				hhShuttleDock::Event_PostSpawn )
END_CLASS

hhShuttleDock::hhShuttleDock() {
	dockingBeam = NULL;
	dockedShuttle = NULL;
	shuttleCount = 0;
	lastConsoleAttempt = 0;
	bLocked = false;
	bPlayingRechargeSound = false;
}

void hhShuttleDock::Spawn() {

	amountHealth = spawnArgs.GetInt("amounthealth");
	amountPower = spawnArgs.GetInt("amountpower");

	bCanExitLocked = spawnArgs.GetBool("canExitLocked");
	bLockOnEntry = spawnArgs.GetBool("lockOnEntry");
	offsetNozzle = spawnArgs.GetVector("offset_nozzle");
	offsetConsole = spawnArgs.GetVector("offset_console");
	offsetShuttlePoint = spawnArgs.GetVector("offset_shuttlepoint");
	maxDistance = spawnArgs.GetFloat("maxdistance");

	dockingForce.SetRestoreFactor(spawnArgs.GetFloat("dockingforce"));
	dockingForce.SetTarget(GetOrigin() + offsetShuttlePoint * GetAxis());

	if (maxDistance) {
		float shuttleMass = dockedShuttle->GetPhysics()->GetMass();
		float weaponKick = dockedShuttle->GetWeaponRecoil();
		float fireRate = dockedShuttle->GetWeaponFireDelay();
		float shuttleThrust = dockedShuttle->GetThrustFactor() + (weaponKick / 128.0f * fireRate);
		float restorationFactor = 128.0f * shuttleMass * shuttleThrust / maxDistance;
		restorationFactor *= 60.0f;	// hack factor to allow for cannon recoil and inaccuracy
		dockingForce.SetRestoreFactor(restorationFactor);
	}

	GetPhysics()->SetContents(CONTENTS_SOLID);

	// Start with dock inactive (faded in)
	SetShaderParm(4, -MS2SEC(gameLocal.time-10000));
	SetShaderParm(5, -1);

	fl.clientEvents = true;

	CancelEvents(&EV_PostSpawn);	// hhDock may have already posted one
	PostEventMS( &EV_PostSpawn, 0 );

	fl.networkSync = true; //rww
}

void hhShuttleDock::Event_PostSpawn() {
	dockingBeam = SpawnDockingBeam(offsetNozzle);
	if (!gameLocal.isClient) {
		hhDock::Event_PostSpawn();
	}
}

void hhShuttleDock::Save(idSaveGame *savefile) const {
	savefile->WriteVec3( offsetNozzle );
	savefile->WriteVec3( offsetConsole );
	savefile->WriteVec3( offsetShuttlePoint );
	dockedShuttle.Save( savefile );
	dockingBeam.Save( savefile );
    savefile->WriteStaticObject( dockingForce );
	savefile->WriteInt( shuttleCount );
	savefile->WriteInt( lastConsoleAttempt );
	savefile->WriteInt( amountHealth );
	savefile->WriteInt( amountPower );
	savefile->WriteFloat( maxDistance );
	savefile->WriteBool( bLocked );
	savefile->WriteBool( bLockOnEntry );
	savefile->WriteBool( bCanExitLocked );
	savefile->WriteBool( bPlayingRechargeSound );
}

void hhShuttleDock::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( offsetNozzle );
	savefile->ReadVec3( offsetConsole );
	savefile->ReadVec3( offsetShuttlePoint );
	dockedShuttle.Restore( savefile );
	dockingBeam.Restore( savefile );
    savefile->ReadStaticObject( dockingForce );
	savefile->ReadInt( shuttleCount );
	savefile->ReadInt( lastConsoleAttempt );
	savefile->ReadInt( amountHealth );
	savefile->ReadInt( amountPower );
	savefile->ReadFloat( maxDistance );
	savefile->ReadBool( bLocked );
	savefile->ReadBool( bLockOnEntry );
	savefile->ReadBool( bCanExitLocked );
	savefile->ReadBool( bPlayingRechargeSound );
}

void hhShuttleDock::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits(dockingZone.GetSpawnId(), 32);
	msg.WriteBits(dockedShuttle.GetSpawnId(), 32);
	//msg.WriteBits(dockingBeam.GetSpawnId(), 32);
	bool dockingBeamActive = false;
	if (dockingBeam.IsValid()) {
		dockingBeamActive = dockingBeam->IsActivated();
	}
	msg.WriteBits(dockingBeamActive, 1);
}

void hhShuttleDock::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	int spawnId;

	spawnId = msg.ReadBits(32);
	if (!spawnId) {
		dockingZone = NULL;
	}
	else {
		dockingZone.SetSpawnId(spawnId);
	}

	idEntityPtr<hhShuttle> newShuttle;

	spawnId = msg.ReadBits(32);
	if (!spawnId) {
		newShuttle = NULL;
	}
	else {
		newShuttle.SetSpawnId(spawnId);
	}

	if (newShuttle != dockedShuttle) {
		if (dockedShuttle.IsValid()) {
			DetachShuttle(dockedShuttle.GetEntity());
			dockedShuttle = NULL;
		}
		if (newShuttle.IsValid() && !newShuttle->IsHidden()) {
			AttachShuttle(newShuttle.GetEntity());
		}
	}

	bool dockingBeamActive = !!msg.ReadBits(1);
	if (dockingBeam.IsValid()) {
		if (dockingBeamActive != dockingBeam->IsActivated()) {
			dockingBeam->Activate(dockingBeamActive);
		}
	}
}

void hhShuttleDock::ClientPredictionThink( void ) {
	//hhDock::ClientPredictionThink();
	if (dockedShuttle.IsValid() && dockedShuttle.GetEntity()) { //hax
		BecomeActive(TH_THINK);
	}
	else {
		BecomeInactive(TH_THINK);
	}
	Think();
}

void hhShuttleDock::SpawnConsole() {
	idVec3 location = GetOrigin() + offsetConsole * GetAxis();

	StartSound("snd_spawn", SND_CHANNEL_ANY);

	if ( !gameLocal.isClient ) {
		idDict args;
		args.SetVector("origin", location);
		args.SetMatrix("rotation", GetAxis());
		args.Set("startDock", name.c_str());
		if (gameLocal.isMultiplayer) { //rww
			const char *shuttleDef = spawnArgs.GetString("def_shuttle_mp");
			if (shuttleDef && shuttleDef[0]) {
				gameLocal.SpawnObject(shuttleDef, &args);
				return;
			}
		}
		gameLocal.SpawnObject(spawnArgs.GetString("def_shuttle"), &args);
	}
}

hhBeamSystem *hhShuttleDock::SpawnDockingBeam(idVec3 &offset) {
	idVec3 pos = GetPhysics()->GetOrigin() + offset * GetPhysics()->GetAxis();
	hhBeamSystem *beam = hhBeamSystem::SpawnBeam(pos, spawnArgs.GetString("beam_docking"), mat3_identity, true);
	assert(beam);
	beam->fl.networkSync = false;
	beam->SetOrigin(pos);
	beam->SetAxis( GetPhysics()->GetAxis()[2].ToMat3() );
	beam->Bind(this, false);
	beam->Activate( false );
	return beam;
}

bool hhShuttleDock::ValidEntity(idEntity *ent) {
	if (ent->IsType(hhPlayer::Type)) {
		int junk=0;
	}
	if (gameLocal.isMultiplayer) { //rww
		return ( !idStr::Icmp(ent->GetEntityDefName(), spawnArgs.GetString("def_shuttle_mp")) && !ent->IsHidden() );
	}
	return ( !idStr::Icmp(ent->GetEntityDefName(), spawnArgs.GetString("def_shuttle")) && !ent->IsHidden() );
}

void hhShuttleDock::EntityEntered(idEntity *ent) {
	if (ent->IsType(hhShuttle::Type) && !dockedShuttle.IsValid()) {
		shuttleCount++;
		hhShuttle *shuttle = static_cast<hhShuttle *>(ent);
		if (shuttle->IsVehicle()) {
			AttachShuttle(shuttle);
		}
	}
}

void hhShuttleDock::EntityEncroaching(idEntity *ent) {
	// If we contain a pilot, but no shuttle: spawn one
	if (ent->IsType(idActor::Type) && !static_cast<idActor*>(ent)->InVehicle() && shuttleCount <= 0) {
		if (gameLocal.time > lastConsoleAttempt) {
			PostEventMS(&EV_TrySpawnConsole, 0);
			lastConsoleAttempt = gameLocal.time + 100;
		}
	}

	else if (ent->IsType(hhShuttle::Type)) {
		hhShuttle *shuttle = static_cast<hhShuttle *>(ent);

		// Check for any shuttles that just became valid
		if (!dockedShuttle.IsValid() && shuttle->IsVehicle()) {
			AttachShuttle(shuttle);
		}

		// Recharge docked shuttles
		if (dockedShuttle == shuttle) {
			if( shuttle->IsConsole() ) {
				DetachShuttle( shuttle );
			} else {
				shuttle->RecoverFromDying();	// Recover every time, just to be sure
				if (shuttle->IsDamaged() || shuttle->NeedsPower()) {

					//HUMANHEAD bjk PCF (4-27-06) - shuttle recharge was slow
					if(USERCMD_HZ == 30) {
						shuttle->GiveHealth(2*amountHealth);
						shuttle->GivePower(2*amountPower);
					} else {
						shuttle->GiveHealth(amountHealth);
						shuttle->GivePower(amountPower);
					}

					if (!bPlayingRechargeSound) {
						StartSound("snd_recharge", SND_CHANNEL_RECHARGE);
						bPlayingRechargeSound = true;
					}
				}
				else {
					if (bPlayingRechargeSound) {
						StopSound(SND_CHANNEL_RECHARGE);
						bPlayingRechargeSound = false;
					}
				}
			}
		}
	}
}

void hhShuttleDock::EntityLeaving(idEntity *ent) {
	if (ent->IsType(hhShuttle::Type)) {
		shuttleCount--;
		shuttleCount = idMath::ClampInt(0, shuttleCount, shuttleCount);
		if (dockedShuttle == ent && !IsLocked()) {
			DetachShuttle(dockedShuttle.GetEntity());
		}
	}
}

void hhShuttleDock::AttachShuttle(hhShuttle *shuttle) {
	if (!dockedShuttle.IsValid()) {
		assert(!shuttle->IsHidden());

		dockedShuttle = shuttle;
		dockingForce.SetEntity(dockedShuttle.GetEntity());
		dockingBeam->SetTargetEntity(dockedShuttle.GetEntity(), 0, dockedShuttle->spawnArgs.GetVector("offset_dockingpoint"));
		dockingBeam->Activate( true );
		dockedShuttle->SetDock( this );

		if (amountHealth || amountPower) {
			if ( shuttle->GetPilot() && !shuttle->GetPilot()->IsType( idAI::Type ) ) {
				dockedShuttle->InitiateRecharging();
			}
		}

		StartSound("snd_looper", SND_CHANNEL_DOCKED);

		if (bLockOnEntry) {
			Lock();
		}

		if (spawnArgs.GetBool("nonsolidwhenactive")) {
			GetPhysics()->SetContents(0);
		}

		ActivateTargets( shuttle );		// Fire triggers
		BecomeActive(TH_THINK);

		// Dock is active, make dock fade out
		SetShaderParm(4, -MS2SEC(gameLocal.time));
		SetShaderParm(5, 1);
	}
}

void hhShuttleDock::DetachShuttle(hhShuttle *shuttle) {
	assert(dockingBeam.IsValid());
	assert(shuttle);

	dockingForce.SetEntity(NULL);
	dockingBeam->SetTargetEntity(NULL);
	dockingBeam->Activate( false );
	shuttle->Undock();
	
	shuttle->FinishRecharging();
	dockedShuttle = NULL;
	StopSound(SND_CHANNEL_DOCKED);
	StopSound(SND_CHANNEL_RECHARGE);
	bPlayingRechargeSound = false;
	Unlock();					// Shouldn't get in here when locked except when a shuttle dies within
	BecomeInactive(TH_THINK);	// Keep thinking so the restore position is updated

	if (spawnArgs.GetBool("nonsolidwhenactive")) {
		GetPhysics()->SetContents(CONTENTS_SOLID);
	}

	// Dock is inactive, make dock fade in
	SetShaderParm(4, -MS2SEC(gameLocal.time));
	SetShaderParm(5, -1);
}

void hhShuttleDock::Think() {
	hhDock::Think();
	if (thinkFlags & TH_THINK) {
		// Apply docking force to shuttle if docked, even if not in zone
		assert(dockedShuttle == dockingForce.GetEntity());
		idVec3 target = GetOrigin() + offsetShuttlePoint * GetAxis();
		dockingForce.SetTarget(target);
		dockingForce.Evaluate(gameLocal.time);

		if (maxDistance) {
			// Drop the player if forced outside of our threshold
			float threshold = maxDistance * 1.5f;
			if ( (dockingForce.GetEntity()->GetOrigin() - target).LengthSqr() >= threshold*threshold) {
				// exit
				dockedShuttle->EjectPilot();
			}
		}
	}
}

void hhShuttleDock::ShuttleExit(hhShuttle *shuttle) {
	DetachShuttle(shuttle);

	// NOTE: Since zones don't get exit messages for entities that are destroyed, this is needed to
	// decrease the shuttle count manually.
	// However, since the shuttle physics shrinks when released, it's possible to get "nearly" outside
	// the zone before releasing, in which case the exit DOES get called, screwing up the count.
	// So... we bound the count to zero.
	shuttleCount--;
	shuttleCount = idMath::ClampInt(0, shuttleCount, shuttleCount);
}

bool hhShuttleDock::AllowsExit() {
	return !bLocked || CanExitLocked();
}

void hhShuttleDock::Lock() {
	bLocked = true;
	dockingForce.SetRestoreFactor(spawnArgs.GetFloat("dockingforce_locked"));
}

void hhShuttleDock::Unlock() {
	bLocked = false;
	dockingForce.SetRestoreFactor(spawnArgs.GetFloat("dockingforce"));
}

void hhShuttleDock::Event_SpawnConsole() {
	if ( !spawnArgs.GetBool( "consolespawn" ) ) {
		return;
	}

	// Check to make sure there's room
	idEntity *touch[ MAX_GENTITIES ];
	idBounds bounds, localBounds;

	idVec3 location = GetOrigin() + offsetConsole * GetAxis();
	localBounds[0] = spawnArgs.GetVector("consoleMins");
	localBounds[1] = spawnArgs.GetVector("consoleMaxs");

	if ( GetAxis().IsRotated() ) {
		bounds.FromTransformedBounds( localBounds, location, GetAxis() );
	}
	else {
		bounds[0] = localBounds[0] + location;
		bounds[1] = localBounds[1] + location;
	}

	int num = gameLocal.clip.EntitiesTouchingBounds( bounds, CLIPMASK_VEHICLE, touch, MAX_GENTITIES );
	bool blocked = false;
	for (int i=0; i<num; i++) {
		if (touch[i] && touch[i] != this && dockingZone != touch[i] && (touch[i]->GetPhysics()->GetContents() & CLIPMASK_VEHICLE)) {
			blocked = true;
			break;
		}
	}

	if (blocked) {
		// Now do more expensive check to see if there's really something blocking us.
		// All the EntitiesTouchingBounds() check does is check the extents.
		// Make bounds into clip model, and use to test contents at our spawn location
		idTraceModel trm;
		trm.SetupBox( localBounds );
		idClipModel *clipModel = new idClipModel( trm );
		int contents = gameLocal.clip.Contents(location, clipModel, GetAxis(), CLIPMASK_VEHICLE, this);
		delete clipModel;
		blocked = contents != 0;
	}

	if (!blocked) {
		SpawnConsole();
	}
}

void hhShuttleDock::Event_Remove() {
	if (IsLocked()) {
		Unlock();
	}
	if (dockedShuttle.IsValid()) {
		DetachShuttle(dockedShuttle.GetEntity());
	}
	hhDock::Event_Remove();
}

hhShuttle *hhShuttleDock::GetDockedShuttle(void) {
	if (!dockedShuttle.IsValid()) {
		return NULL;
	}
	return dockedShuttle.GetEntity();
}
