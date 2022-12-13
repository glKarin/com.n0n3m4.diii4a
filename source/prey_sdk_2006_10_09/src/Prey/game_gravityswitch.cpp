#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_CheckAgain( "<checkagain>", NULL );

CLASS_DECLARATION( hhDamageTrigger, hhGravitySwitch )
	EVENT( EV_PostSpawn,			hhGravitySwitch::Event_PostSpawn )
	EVENT( EV_CheckAgain,			hhGravitySwitch::Event_CheckAgain )
END_CLASS


void hhGravitySwitch::Spawn( void ) {
	fl.networkSync = true;

	fl.clientEvents = true;

	CancelEvents(&EV_PostSpawn);	// Parent actually already posted one
	PostEventMS(&EV_PostSpawn, 0);
}

void hhGravitySwitch::Event_PostSpawn(void) {
	idVec3 origin = GetOrigin() - GetAxis()[0]*10.0f;

	// Determine axis of emitter
	// NOTE: axis of gravityswitch is reversed, axis of emitters is: identity==up
	idVec3 up(0.0f, 0.0f, 1.0f);
	idVec3 back(-1.0f, 0.0f, 0.0f);
	idMat3 axis = up.ToMat3().Inverse()*back.ToMat3().Inverse()*GetAxis();		//a mess to * axis to get the emitter to point the same way as the model

	idDict args;
	args.Clear();
	args.SetVector("origin", origin);
	args.SetMatrix("rotation", axis);

	const char *effectName = spawnArgs.GetString("def_effect", NULL);
	if (effectName && *effectName) {
		effect = static_cast<hhFuncEmitter*>(gameLocal.SpawnClientObject(effectName, &args) );
		if (effect.IsValid()) {
			effect->Hide();
		}
	}
}

hhGravitySwitch::~hhGravitySwitch() {
	SAFE_REMOVE( effect );
}

void hhGravitySwitch::Save(idSaveGame *savefile) const {
	effect.Save(savefile);
}

void hhGravitySwitch::Restore( idRestoreGame *savefile ) {
	effect.Restore(savefile);
}

void hhGravitySwitch::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location) {
	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( damageDef && damageDef->GetBool("radius")) {
		return;	// Gravity switches are immune to all splash damage
	}
	hhDamageTrigger::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);
}

idVec3 hhGravitySwitch::GetGravityVector() {
	idVec3 vector;

	if (!spawnArgs.GetVector("vector", NULL, vector)) {
		float strength = spawnArgs.GetFloat("strength", "1");
		idVec3 direction = GetPhysics()->GetAxis()[0];
		vector = direction * strength * DEFAULT_GRAVITY;
	}

	return vector;
}

void hhGravitySwitch::SetGravityVector(idEntity *activator) {
	idVec3 newGravity = GetGravityVector();
	bool bSwitchedGravity = false;

	int numTargets = targets.Num();
	for ( int ix = 0; ix < numTargets; ix++) {
		idEntity *ent = targets[ix].GetEntity();
		if (ent && ent->IsType(hhGravityZone::Type)) {
			hhGravityZone *zone = static_cast<hhGravityZone*>(ent);

			idVec3 zoneGravity = zone->GetDestinationGravity();
			if (!zoneGravity.Compare(newGravity, VECTOR_EPSILON)) {
				zone->SetGravityOnZone( newGravity );
				bSwitchedGravity = true;
			}
		}
	}

	if (bSwitchedGravity) {
		if (gameLocal.isMultiplayer && !gameLocal.isClient) { //rww - play sound when shot in MP
			StartSound("snd_gravity_mpshot", SND_CHANNEL_ANY, 0, true);
		}
		ActivateTargets(activator);
		BecomeActive(TH_TICKER);
	}
}

void hhGravitySwitch::Ticker() {
	// See if our gravity zone(s) are following our gravity
	int numTargets = 1;	// only need to check one, they are all assumed to be the same.  If not, we have problems anyway.
	for ( int ix = 0; ix < numTargets; ix++) {
		idEntity *ent = targets[ix].GetEntity();
		if (ent && ent->IsType(hhGravityZone::Type)) {
			hhGravityZone *zone = static_cast<hhGravityZone*>(ent);
			if (zone->GetDestinationGravity().Compare( GetGravityVector(), VECTOR_EPSILON )) {
				// Zone is still following my gravity, emit smoke
				if (effect.IsValid() && effect->IsHidden()) {
					effect->Show();
				}
				return;
			}
		}
	}

	// If not, stop thinking about it.
	BecomeInactive(TH_TICKER);
	if (effect.IsValid() && !effect->IsHidden()) {
		effect->Hide();
	}

	// Check again in a while in case another switch turns the zone onto my gravity
	CancelEvents(&EV_CheckAgain);
	PostEventMS(&EV_CheckAgain, 1000);
}

void hhGravitySwitch::Event_CheckAgain() {
	BecomeActive(TH_TICKER);	// Check again
}

void hhGravitySwitch::TriggerAction(idEntity *activator) {

	SetGravityVector(activator);

	// Handle default trigger behavior
	// Hack, make targetlist appear to be empty so ActivateTargets() does nothing, we handle this ourself
	int num = targets.Num();
	targets.SetNum(0, false);
	hhDamageTrigger::TriggerAction(activator);
	targets.SetNum(num, false);
}

void hhGravitySwitch::Event_Enable() {
	if (!bEnabled) {
		StartSound("snd_gravity_enable", SND_CHANNEL_ANY, 0, true);
	}
	SetShaderParm(SHADERPARM_TIMEOFFSET, -MS2SEC(gameLocal.time));
	SetShaderParm(SHADERPARM_MISC, 1);
	bEnabled = true;
	fl.takedamage = true;
	GetPhysics()->SetContents( CONTENTS_SHOOTABLE|CONTENTS_IKCLIP|CONTENTS_SHOOTABLEBYARROW );
	BecomeActive(TH_TICKER);
}

void hhGravitySwitch::Event_Disable() {
	if (bEnabled) {
		StartSound("snd_gravity_disable", SND_CHANNEL_ANY, 0, true);
	}
	SetShaderParm(SHADERPARM_MISC, 0);
	GetPhysics()->SetContents( CONTENTS_IKCLIP );
	fl.takedamage = false;
	bEnabled = false;
	BecomeInactive(TH_TICKER);
	if (effect.IsValid()) {
		effect->Hide();
	}
}

void hhGravitySwitch::WriteToSnapshot( idBitMsgDelta &msg ) const {
	GetPhysics()->WriteToSnapshot(msg);
	msg.WriteBits(bEnabled, 1);
	msg.WriteFloat(renderEntity.shaderParms[SHADERPARM_TIMEOFFSET]);
	msg.WriteFloat(renderEntity.shaderParms[SHADERPARM_MISC]);
	
	if (effect.IsValid()) {
		msg.WriteBits(effect->IsHidden(), 1);
	}
	else {
		msg.WriteBits(0, 1);
	}
}

void hhGravitySwitch::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	GetPhysics()->ReadFromSnapshot(msg);
	bEnabled = !!msg.ReadBits(1);
	renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = msg.ReadFloat();
	renderEntity.shaderParms[SHADERPARM_MISC] = msg.ReadFloat();
	
	bool fxHidden = !!msg.ReadBits(1);
	if (effect.IsValid() && fxHidden != effect->IsHidden()) {
		if (fxHidden) {
			effect->Hide();
		}
		else {
			effect->Show();
		}
	}
}
