
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//-----------------------------------------------------------------------
//
// hhShuttleTransport
//
//-----------------------------------------------------------------------

const idEventDef EV_FadeOutTransporter("fadeOutTransporter");

CLASS_DECLARATION(hhDock, hhShuttleTransport)
	EVENT( EV_DockLock,					hhShuttleTransport::Event_Lock)
	EVENT( EV_DockUnlock,				hhShuttleTransport::Event_Unlock)
	EVENT( EV_Activate,					hhShuttleTransport::Event_Activate)
	EVENT( EV_FadeOutTransporter,		hhShuttleTransport::Event_FadeOut)
	EVENT( EV_Remove,					hhShuttleTransport::Event_Remove)
END_CLASS

void hhShuttleTransport::Spawn() {
	dockingBeam = NULL;
	dockedShuttle = NULL;
	shuttleCount = 0;
	bLocked = false;

	amountHealth = spawnArgs.GetInt("amounthealth");
	amountPower = spawnArgs.GetInt("amountpower");

	bCanExitLocked = spawnArgs.GetBool("canExitLocked");
	bLockOnEntry = spawnArgs.GetBool("lockOnEntry");
	bAllowFiring = spawnArgs.GetBool("allowFiring");
	offsetNozzle = spawnArgs.GetVector("offset_nozzle1");
//	offsetNozzle2 = spawnArgs.GetVector("offset_nozzle2");
	offsetShuttlePoint = spawnArgs.GetVector("offset_shuttlepoint");

//	dockingBeam = SpawnDockingBeam(offsetNozzle);

	dockingForce.SetRestoreFactor(spawnArgs.GetFloat("dockingforce"));
	dockingForce.SetTarget(GetOrigin() + offsetShuttlePoint * GetAxis());

	// Fade in
	SetShaderParm(SHADERPARM_TIMEOFFSET, -MS2SEC(gameLocal.time));	// Growth start time
	SetShaderParm(5, 1.0f);											// Growth direction (in)
	SetShaderParm(6, 1.0f);											// Make Beam opaque
	StartSound("snd_fadein", SND_CHANNEL_ANY);
}

void hhShuttleTransport::Save(idSaveGame *savefile) const {
	savefile->WriteVec3( offsetNozzle );
	savefile->WriteVec3( offsetShuttlePoint );
	savefile->WriteObject( dockedShuttle );
	savefile->WriteObject( dockingBeam );
    savefile->WriteStaticObject( dockingForce );
	savefile->WriteInt( shuttleCount );
	savefile->WriteInt( amountHealth );
	savefile->WriteInt( amountPower );
	savefile->WriteBool( bLocked );
	savefile->WriteBool( bLockOnEntry );
	savefile->WriteBool( bCanExitLocked );
}

void hhShuttleTransport::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( offsetNozzle );
	savefile->ReadVec3( offsetShuttlePoint );
	savefile->ReadObject( reinterpret_cast<idClass *&>(dockedShuttle) );
	savefile->ReadObject( reinterpret_cast<idClass *&>(dockingBeam) );
    savefile->ReadStaticObject( dockingForce );
	savefile->ReadInt( shuttleCount );
	savefile->ReadInt( amountHealth );
	savefile->ReadInt( amountPower );
	savefile->ReadBool( bLocked );
	savefile->ReadBool( bLockOnEntry );
	savefile->ReadBool( bCanExitLocked );
}

hhBeamSystem *hhShuttleTransport::SpawnDockingBeam(idVec3 &offset) {
	idVec3 pos = GetPhysics()->GetOrigin() + offset * GetPhysics()->GetAxis();
	hhBeamSystem *beam = hhBeamSystem::SpawnBeam(pos, spawnArgs.GetString("beam_docking"));
	assert(beam);
	beam->SetOrigin(pos);
	beam->SetAxis(GetPhysics()->GetAxis());
	beam->Bind(this, false);
	beam->Hide();
	return beam;
}

bool hhShuttleTransport::ValidEntity(idEntity *ent) {
	return (ent->IsType(hhShuttle::Type) && !ent->IsHidden());
}

void hhShuttleTransport::EntityEntered(idEntity *ent) {
	if (ent->IsType(hhShuttle::Type)) {
		shuttleCount++;
		hhShuttle *shuttle = static_cast<hhShuttle *>(ent);
		if (shuttle->IsVehicle()) {
			AttachShuttle(shuttle);
		}
	}
}

void hhShuttleTransport::EntityEncroaching(idEntity *ent) {

	if (ent->IsType(hhShuttle::Type)) {
		hhShuttle *shuttle = static_cast<hhShuttle *>(ent);

		// Check for any shuttles that just became valid
		if (dockedShuttle == NULL && shuttle->IsVehicle()) {
			AttachShuttle(shuttle);
		}

		// Recharge docked shuttles
		if (shuttle == dockedShuttle) {

			//HUMANHEAD bjk PCF (4-27-06) - shuttle recharge was slow
			if(USERCMD_HZ == 30) {
				shuttle->GiveHealth(2*amountHealth);
				shuttle->GivePower(2*amountPower);
			} else {
				shuttle->GiveHealth(amountHealth);
				shuttle->GivePower(amountPower);
			}
		}
	}
}

void hhShuttleTransport::EntityLeaving(idEntity *ent) {
	if (ent->IsType(hhShuttle::Type)) {
		shuttleCount--;
		shuttleCount = idMath::ClampInt(0, shuttleCount, shuttleCount);
		if (ent == dockedShuttle && !IsLocked()) {
			DetachShuttle(dockedShuttle);
		}
	}
}

void hhShuttleTransport::AttachShuttle(hhShuttle *shuttle) {
	if (dockedShuttle == NULL) {
		dockedShuttle = shuttle;
		dockingForce.SetEntity(dockedShuttle);
		if (dockingBeam) {
			dockingBeam->SetTargetEntity(dockedShuttle, 0, dockedShuttle->spawnArgs.GetVector("offset_dockingpoint"));
			dockingBeam->Activate( true );
		}

		dockedShuttle->SetDock(this);
		dockedShuttle->InitiateRecharging();
		StartSound("snd_getin", SND_CHANNEL_ANY);
		StartSound("snd_looper", SND_CHANNEL_DOCKED);

		if (bLockOnEntry) {
			Lock();
		}
		ActivateTargets( shuttle );		// Fire triggers
		BecomeActive(TH_THINK);
		SetShaderParm(6, 0.0f);			// Make beam transparent
	}
}

void hhShuttleTransport::DetachShuttle(hhShuttle *shuttle) {
	assert (shuttle==dockedShuttle);
	if (dockedShuttle != NULL) {
		dockingForce.SetEntity(NULL);
		if (dockingBeam) {
			dockingBeam->SetTargetEntity(NULL);
			dockingBeam->Activate( false );
		}
	
		dockedShuttle->SetDock(NULL);
		dockedShuttle->FinishRecharging();
		dockedShuttle = NULL;
		StartSound("snd_getout", SND_CHANNEL_ANY);
		StopSound(SND_CHANNEL_DOCKED);
		Unlock();					// Shouldn't get in here when locked except when a shuttle dies within
		BecomeInactive(TH_THINK);	// Keep thinking so the restore position is updated
		SetShaderParm(6, 1.0f);		// Make beam opaque
	}
}

void hhShuttleTransport::Think() {
	hhDock::Think();
	if (thinkFlags & TH_THINK) {
		// Apply docking force to shuttle if docked, even if not in zone
		assert(dockingForce.GetEntity() == dockedShuttle);
		dockingForce.SetTarget(GetOrigin() + offsetShuttlePoint * GetAxis());
		dockingForce.Evaluate(gameLocal.time);
	}
}

void hhShuttleTransport::ShuttleExit(hhShuttle *shuttle) {
	DetachShuttle(shuttle);

	// NOTE: Since zones don't get exit messages for entities that are destroyed, this is needed to
	// decrease the shuttle count manually.
	// However, since the shuttle physics shrinks when released, it's possible to get "nearly" outside
	// the zone before releasing, in which case the exit DOES get called, screwing up the count.
	// So... we bound the count to zero.
	shuttleCount--;
	shuttleCount = idMath::ClampInt(0, shuttleCount, shuttleCount);
}

void hhShuttleTransport::UpdateAxis( const idMat3 &newAxis ) {
	// Yaw transport to match yaw of shuttle
	idAngles ang = newAxis.ToAngles();
	ang.pitch = ang.roll = 0.0f;
	SetAxis(ang.ToMat3());
}

void hhShuttleTransport::Lock() {
	bLocked = true;
	dockedShuttle->SetOrigin(GetOrigin() + offsetShuttlePoint * GetAxis());
	dockingForce.SetRestoreFactor(spawnArgs.GetFloat("dockingforce_locked"));
	dockedShuttle->Bind(this, false);
}

void hhShuttleTransport::Unlock() {
	bLocked = false;
	dockingForce.SetRestoreFactor(spawnArgs.GetFloat("dockingforce"));
	if (dockedShuttle) {
		dockedShuttle->Unbind();
	}
}

void hhShuttleTransport::Event_FadeOut() {
	// Fade out
	SetShaderParm(SHADERPARM_TIMEOFFSET, -MS2SEC(gameLocal.time));	// Growth start time
	SetShaderParm(5, -1.0f);										// Growth direction (out)
	SetShaderParm(6, 0.0f);											// Make Beam translucent
	StartSound("snd_fadeout", SND_CHANNEL_ANY);

	PostEventMS(&EV_Remove, 1000);
}

void hhShuttleTransport::Event_Remove() {
	if (IsLocked()) {
		Unlock();
	}
	if (dockedShuttle) {
		DetachShuttle(dockedShuttle);
	}
	hhDock::Event_Remove();
}
