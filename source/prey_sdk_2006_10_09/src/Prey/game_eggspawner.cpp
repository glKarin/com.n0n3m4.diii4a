#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//==========================================================================
//
//	hhEggSpawner
//
//	When activated, shoots out an egg along it's X axis
//==========================================================================

#define MAX_HATCH_TIME (SEC2MS(10))

CLASS_DECLARATION(hhAnimatedEntity, hhEggSpawner)
	EVENT( EV_PlayIdle,			hhEggSpawner::Event_PlayIdle)
	EVENT( EV_Activate,			hhEggSpawner::Event_Activate)
END_CLASS

void hhEggSpawner::Spawn(void) {
	GetPhysics()->SetContents( CONTENTS_SOLID );
	fl.takedamage = true;

	idleAnim	= GetAnimator()->GetAnim("idle");
	hatchAnim	= GetAnimator()->GetAnim("launch");
	painAnim	= GetAnimator()->GetAnim("pain");

	PostEventMS(&EV_PlayIdle, 0);
}

void hhEggSpawner::Save(idSaveGame *savefile) const {
	savefile->WriteInt( idleAnim );
	savefile->WriteInt( hatchAnim );
	savefile->WriteInt( painAnim );
}

void hhEggSpawner::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( idleAnim );
	savefile->ReadInt( hatchAnim );
	savefile->ReadInt( painAnim );
}

void hhEggSpawner::SpawnEgg(idEntity *activator) {
	idVec3 dir = GetPhysics()->GetAxis()[0];
	idVec3 offset = spawnArgs.GetVector("offset_spawn");
	float power = spawnArgs.GetFloat("power");

	// Play anim
	if (hatchAnim) {
		GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, hatchAnim, gameLocal.time, 0);
		int ms = GetAnimator()->GetAnim( hatchAnim )->Length();
		PostEventMS(&EV_PlayIdle, ms);
	}

	idDict args;
	args.Clear();
	args.SetVector( "origin", GetPhysics()->GetOrigin() + offset * GetPhysics()->GetAxis() );
	hhEgg *egg = static_cast<hhEgg*>(gameLocal.SpawnObject(spawnArgs.GetString("def_egg"), &args));
	if (egg) {
		egg->GetPhysics()->SetLinearVelocity(dir * power);
		egg->SetActivator(activator);

		// Copy our targets
		for ( int i = 0; i < targets.Num(); i++ ) {
			egg->targets.AddUnique(targets[i]);
		}
	}

	StartSound( "snd_spawn", SND_CHANNEL_ANY, 0, true, NULL );
}

void hhEggSpawner::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	// Don't actually take damage, but give feedback
	if (painAnim) {
		GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, painAnim, gameLocal.time, 0);
		int ms = GetAnimator()->GetAnim( painAnim )->Length();
		PostEventMS(&EV_PlayIdle, ms);
	}
}

void hhEggSpawner::Event_PlayIdle() {
	if (idleAnim) {
		GetAnimator()->ClearAllAnims(gameLocal.time, 200);
		GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, idleAnim, gameLocal.time, 200);
	}
}

void hhEggSpawner::Event_Activate(idEntity *activator) {
	SpawnEgg(activator);
}


//==========================================================================
//
//	hhEgg
//
//	Moveable that spawns a creature on impact
//==========================================================================

const idEventDef EV_Hatch("<hatch>");

CLASS_DECLARATION(hhMoveable, hhEgg)
	EVENT( EV_Activate,			hhEgg::Event_Activate)
	EVENT( EV_Hatch,			hhEgg::Event_Hatch)
END_CLASS

void hhEgg::Spawn(void) {
	enemy = NULL;
	bHatched = false;
	bHatching = false;
	const char *tableName = spawnArgs.GetString("table_hatch");
	table = static_cast<const idDeclTable *>(declManager->FindType( DECL_TABLE, tableName, true ));
	hatchTime = -1.0f;

	PostEventSec( &EV_Activate, spawnArgs.GetFloat("secondsBeforeHatch"), this );
}

void hhEgg::Save(idSaveGame *savefile) const {

	savefile->WriteBool( bHatching );
	savefile->WriteBool( bHatched );

	savefile->WriteFloat( deformAlpha.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( deformAlpha.GetDuration() );
	savefile->WriteFloat( deformAlpha.GetStartValue() );
	savefile->WriteFloat( deformAlpha.GetEndValue() );
	savefile->WriteFloat( hatchTime );

	enemy.Save(savefile);
}

void hhEgg::Restore( idRestoreGame *savefile ) {
	float set;

	savefile->ReadBool( bHatching );
	savefile->ReadBool( bHatched );

	savefile->ReadFloat( set );						// idInterpolate<float>
	deformAlpha.SetStartTime( set );
	savefile->ReadFloat( set );
	deformAlpha.SetDuration( set );
	savefile->ReadFloat( set );
	deformAlpha.SetStartValue(set);
	savefile->ReadFloat( set );
	deformAlpha.SetEndValue( set );

	savefile->ReadFloat( hatchTime );

	const char *tableName = spawnArgs.GetString("table_hatch");
	table = static_cast<const idDeclTable *>(declManager->FindType( DECL_TABLE, tableName, true ));

	enemy.Restore(savefile);
}


void hhEgg::SetActivator(idEntity *activator) {
	if (activator && activator->IsType(idActor::Type)) {
		enemy = static_cast<idActor*>(activator);
	}
}

void hhEgg::Ticker() {
	if (bHatching) {
		// over time, send different parms into deformation
		if (deformAlpha.IsDone(gameLocal.time)) {
			Hatch();
		}
		else {
			float alpha = deformAlpha.GetCurrentValue(gameLocal.time);
			float value = table->TableLookup(alpha);
			SetShaderParm(SHADERPARM_ANY_DEFORM, DEFORMTYPE_VIBRATE);
			SetShaderParm(SHADERPARM_ANY_DEFORM_PARM1, value);
		}
	}
}

bool hhEgg::Collide( const trace_t &collision, const idVec3 &velocity ) {
	AttemptToPlayBounceSound( collision, velocity );

	if (spawnArgs.GetBool("spawnOnImpact")) {	//obs
		Hatch();
		return true;
	}
	return false;
}

void hhEgg::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location) {
	Hatch();
}

bool hhEgg::SpawnHatchling(const char *monsterName, const idVec3 &spawnLocation) {
	idDict args;
	args.Clear();
	args.SetVector( "origin", spawnLocation );
	args.SetFloat( "angle", GetAxis().ToAngles().yaw );
	args.Set( "trigger_anim", "birth" );
	hhMonsterAI *monster = static_cast<hhMonsterAI*>(gameLocal.SpawnObject(monsterName, &args));

	if (monster) {
		// Set enemy
		if (enemy.IsValid()) {
			monster->SetEnemy(enemy.GetEntity());
		}

		// Copy our targets
		for ( int i = 0; i < targets.Num(); i++ ) {
			monster->targets.AddUnique(targets[i]);
		}

		monster->PostEventSec( &EV_Activate, 0, this );
	}

	return monster != NULL;
}

void hhEgg::Hatch() {
	if (!bHatched) {
		if (hatchTime == -1.0f) {
			hatchTime = gameLocal.time;
		}
		// Spawn hatchling
		const char *monsterName = spawnArgs.GetString("def_hatchling", NULL);
		if (monsterName) {
			idVec3 spawnLocation = GetPhysics()->GetOrigin();

			const float hatchOffsetDistance = 50.0f;
			bool spawned = false;
			idVec3 offset;
			idVec3 offsetLocation;
			trace_t trace;
			memset(&trace, 0, sizeof(trace_t));

			if (hhUtils::EntityDefWillFit(monsterName, spawnLocation, mat3_identity, MASK_MONSTERSOLID, this)) {
				//gameRenderWorld->DebugArrow(colorGreen, spawnLocation, offsetLocation, 10, 10000);
				spawned = SpawnHatchling(monsterName, spawnLocation);
			}

			if (!spawned) {	// Creature wouldn't fit, try again later
				CancelEvents(&EV_Hatch);
				if (gameLocal.time - hatchTime > MAX_HATCH_TIME) {
					// Give up after 15 seconds of struggling
					ActivateTargets(this);
					// Spawn gibs from monster
					const idDict *dict = gameLocal.FindEntityDefDict(monsterName, false);
					if (dict && dict->FindKey("def_gibdebrisspawner")) {
						GetPhysics()->SetLinearVelocity( vec3_zero );	//HUMANHEAD jsh PCF zero out velocity to prevent debris huge translation 
						hhUtils::SpawnDebrisMass(dict->GetString("def_gibdebrisspawner"), this);
					}
				} else {
					//idVec3 dir = hhUtils::RandomVector();
					//gameRenderWorld->DebugArrow(colorGreen, spawnLocation, spawnLocation + dir * 50.0f, 10, 250);
					GetPhysics()->AddForce(0, GetPhysics()->GetOrigin(), hhUtils::RandomVector() * idMath::ClampFloat(0x1000000, 0x100000000, (0x1000000 * ((gameLocal.time - hatchTime) / 25.0f))));
					PostEventMS(&EV_Hatch, 250);
					return;
				}
			}
		}

		hatchTime = -1.0f;
		bHatched = true;
		GetPhysics()->SetContents(0);
		PostEventMS(&EV_Remove, 0);

		StartSound( "snd_hatch", SND_CHANNEL_ANY, 0, true, NULL );
		GetPhysics()->SetLinearVelocity( vec3_zero );	//HUMANHEAD jsh PCF zero out velocity to prevent debris huge translation 
		hhUtils::SpawnDebrisMass(spawnArgs.GetString("def_afterbirth"), this);
	}
}

void hhEgg::Event_Activate(idEntity *activator) {
	if (!bHatching) {
		bHatching = true;
		int msForTable = SEC2MS(spawnArgs.GetFloat("secondsToHatch"));
		deformAlpha.Init(gameLocal.time, msForTable, 0.0f, 1.0f);
		BecomeActive(TH_TICKER);
	}
}

void hhEgg::Event_Hatch(void) {
	Hatch();
}
