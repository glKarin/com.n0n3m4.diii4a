#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


//==========================================================================
//
//	hhMine
//
//==========================================================================

const idEventDef EV_ExplodeDamage( "explodeDamage", "e" );
const idEventDef EV_MineHover( "<mineHover>" );

CLASS_DECLARATION(hhMoveable, hhMine)
	EVENT( EV_Activate,			hhMine::Event_Trigger)
	EVENT( EV_ExplodeDamage,	hhMine::Event_ExplodeDamage)
	EVENT( EV_Remove,			hhMine::Event_Remove)
	EVENT( EV_ExplodedBy,		hhMine::Event_ExplodedBy )
	EVENT( EV_MineHover,		hhMine::Event_MineHover )
END_CLASS

void hhMine::Spawn(void) {
	spawner = NULL;
	fl.takedamage = spawnArgs.GetBool("takedamage", "1");

	idVec3 worldGravityDir( gameLocal.GetGravity() );
	float gravityMagnitude = spawnArgs.GetFloat( "gravity", va("%.2f", worldGravityDir.Normalize()) );
	GetPhysics()->SetGravity( gravityMagnitude * worldGravityDir );

	bScaleIn = spawnArgs.GetBool("scalein");
	if (bScaleIn) {
		fadeAlpha.Init(gameLocal.time, 2000, 0.01f, 1.0f);
		BecomeActive(TH_MISC1);
	}

	if (spawnArgs.FindKey("snd_spawn")) {
		StartSound( "snd_spawn", SND_CHANNEL_ANY );
	}

	if (spawnArgs.FindKey("snd_idle")) {
		StartSound( "snd_idle", SND_CHANNEL_IDLE );
	}

	bDetonateOnCollision = spawnArgs.GetBool("DetonateOnCollision");
	bDamageOnCollision = spawnArgs.GetBool("DamageOnCollision");
	bExploded = false;

	if (!spawnArgs.GetBool("nodrop") && GetPhysics()->GetGravity() == vec3_origin) {
		gameLocal.Warning("zero grav object without nodrop will not move: %s", name.c_str());
	}

	if (bDetonateOnCollision) {
		SpawnTrigger();
	}
}

void hhMine::Save(idSaveGame *savefile) const {
	savefile->WriteObject( spawner );
	savefile->WriteBool( bDetonateOnCollision );
	savefile->WriteBool( bDamageOnCollision );

	savefile->WriteFloat( fadeAlpha.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( fadeAlpha.GetDuration() );
	savefile->WriteFloat( fadeAlpha.GetStartValue() );
	savefile->WriteFloat( fadeAlpha.GetEndValue() );

	savefile->WriteBool( bScaleIn );
	savefile->WriteBool( bExploded );
}

void hhMine::Restore( idRestoreGame *savefile ) {
	float set;

	savefile->ReadObject( reinterpret_cast<idClass *&>(spawner) );
	savefile->ReadBool( bDetonateOnCollision );
	savefile->ReadBool( bDamageOnCollision );

	savefile->ReadFloat( set );			// idInterpolate<float>
	fadeAlpha.SetStartTime( set );
	savefile->ReadFloat( set );
	fadeAlpha.SetDuration( set );
	savefile->ReadFloat( set );
	fadeAlpha.SetStartValue(set);
	savefile->ReadFloat( set );
	fadeAlpha.SetEndValue( set );

	savefile->ReadBool( bScaleIn );
	savefile->ReadBool( bExploded );
}

void hhMine::SpawnTrigger() {
	idEntity *trigger;
	idDict Args;

	Args.Set( "target", name.c_str() );
	Args.Set( "mins", spawnArgs.GetString("triggerMins") );
	Args.Set( "maxs", spawnArgs.GetString("triggerMaxs") );
	Args.Set( "bind", name.c_str() );
	Args.SetVector( "origin", GetOrigin() );
	Args.SetMatrix( "rotation", GetAxis() );
	trigger = gameLocal.SpawnObject( spawnArgs.GetString("def_trigger"), &Args );
}

void hhMine::Think() {
	if (thinkFlags & TH_MISC1) {
		if (bScaleIn) {
			if (fadeAlpha.IsDone(gameLocal.time)) {
				SetDeformation(DEFORMTYPE_SCALE, 0.0f);	// Turn scaling off
				bScaleIn = false;
				BecomeInactive(TH_MISC1);
			}
			else {
				SetDeformation(DEFORMTYPE_SCALE, fadeAlpha.GetCurrentValue(gameLocal.time));
			}
		}
	}
	//TODO: Could make these come to rest when gravity is zero and not moving
	RunPhysics();
	Present();
}

void hhMine::Launch(idVec3 &velocity, idVec3 &avelocity) {
	GetPhysics()->SetLinearVelocity(velocity);
	GetPhysics()->SetAngularVelocity(avelocity);
	BecomeActive(TH_MISC1);
}

void hhMine::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	// skip idMoveable::Damage which handles damage differently
	idEntity::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);
}

void hhMine::ApplyImpulse(idEntity * ent, int id, const idVec3 &point, const idVec3 &impulse) {
	if (impulse == vec3_origin && ent->IsType(idActor::Type)) {	// Just pushed
		// Apply player's velocity to the pod so it moves at same rate as player and keeps going
		idVec3 newimpulse = ent->GetPhysics()->GetLinearVelocity() * GetPhysics()->GetMass();
		hhMoveable::ApplyImpulse(ent, id, point, newimpulse);
//		GetPhysics()->SetLinearVelocity(ent->GetPhysics()->GetLinearVelocity()*4);
	}
	if (bDetonateOnCollision) {
		// Need to post an event because already in physics code now, can't nest a projectile spawn from within physics code
		// currently, because rigid body physics ::Evaluate is not reentrant friendly (has a static timer)
		PostEventMS(&EV_ExplodedBy, 0, this);
	}
	hhMoveable::ApplyImpulse(ent, id, point, impulse);
}

bool hhMine::AllowCollision( const trace_t &collision ) {
	idEntity *ent = gameLocal.entities[collision.c.entityNum];
	if ( ent && ent->IsType(hhShuttleForceField::Type) ) {
		return false;	// Allow asteroids to go through shuttle forcefields
	}
	return true;
}

bool hhMine::Collide( const trace_t &collision, const idVec3 &velocity ) {
	const char *decal;
	idEntity *ent = gameLocal.entities[collision.c.entityNum];

	// project decal
	decal = spawnArgs.RandomPrefix( "mtr_decal", gameLocal.random );
	if ( decal && *decal ) {
		gameLocal.ProjectDecal( collision.c.point, -collision.c.normal, spawnArgs.GetFloat( "decal_trace", "128.0" ), true, spawnArgs.GetFloat( "decal_size", "6.0" ), decal );
	}

	if (bDamageOnCollision) {
		const idKeyValue *kv = spawnArgs.FindKey("def_damage");
		if (ent && kv != NULL) {
			ent->Damage(this, gameLocal.world, velocity, kv->GetValue().c_str(), 1.0f, 0);
		}
	}
	if (bDetonateOnCollision) {
		// Need to post an event because already in physics code now, can't nest a projectile spawn from within physics code
		// currently, because rigid body physics ::Evaluate is not reentrant friendly (has a static timer)
		PostEventMS(&EV_ExplodedBy, 0, this);

		return true;
	}
	return false;
}

void hhMine::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	fl.takedamage = false;			// nla - Prevent killed from being called too many times.
	
	// Need to post an event because already in physics code now, can't nest a projectile spawn from within physics code
	// currently, because rigid body physics ::Evaluate is not reentrant friendly (has a static timer)
	PostEventMS(&EV_ExplodedBy, 0, attacker);
}

void hhMine::Explode( idEntity *attacker ) {
	hhFxInfo fxInfo;
	int splash_damage_delay = SEC2MS( spawnArgs.GetFloat( "splash_damage_delay", "0.5" ) );

	if (bExploded || IsHidden() ) {
		return;
	}

	bExploded = true;

	// Activate targets
	ActivateTargets( attacker );

	// Set for removal
	if ( spawnArgs.GetBool( "respawn" ) ) {
		fl.takedamage = true;
		PostEventSec( &EV_Show, spawnArgs.GetFloat( "respawn_delay", "2" ) );
		bExploded = false;
		Hide();
	} else {
		PostEventMS( &EV_Remove, 1500 + splash_damage_delay );
		GetPhysics()->SetContents( 0 );
		fl.takedamage = false;
		Hide();
	}

	RemoveBinds();

	// Spawn explosion
	StopSound( SND_CHANNEL_IDLE );
	StartSound( "snd_explode", SND_CHANNEL_ANY );

	//fixme: if we stay with moveables, this can be replaced by key "gib"
	if ( spawnArgs.GetString("def_debrisspawner")[0] ) {
		hhUtils::SpawnDebrisMass(spawnArgs.GetString("def_debrisspawner"), GetOrigin());
	}

	//fixme: fx are spawned by debris system?
	fxInfo.SetNormal( GetAxis()[2] );
	fxInfo.RemoveWhenDone( true );
	BroadcastFxInfoPrefixed( "fx_detonate", GetOrigin(), GetAxis(), &fxInfo );

	if (spawnArgs.FindKey("def_splash_damage") != NULL) {
		PostEventMS(&EV_ExplodeDamage, splash_damage_delay, attacker);
	}
}

bool hhMine::WasSpawnedBy(idEntity *theSpawner) {
	return (spawner == theSpawner);
}

void hhMine::Event_Trigger( idEntity *activator ) {
	Explode(activator);
}

void hhMine::Event_ExplodedBy( idEntity *activator) {
	Explode(activator);
}

void hhMine::Event_MineHover() {
	GetPhysics()->SetLinearVelocity( vec3_zero );
}

void hhMine::Event_Remove() {
	if (spawner) {	// notify spawner
		spawner->MineRemoved(this);
		spawner = NULL;
	}
	hhMoveable::Event_Remove();
}

void hhMine::Event_ExplodeDamage( idEntity *attacker ) {
	gameLocal.RadiusDamage( GetPhysics()->GetOrigin(), this, attacker, this, this, spawnArgs.GetString("def_splash_damage") );
}


//==========================================================================
//
//	hhMineSpawner
//
//==========================================================================
const idEventDef EV_SpawnMine("SpawnMine", NULL);

CLASS_DECLARATION(hhAnimatedEntity, hhMineSpawner)
	EVENT( EV_Activate,		hhMineSpawner::Event_Activate)
	EVENT( EV_SpawnMine,	hhMineSpawner::Event_SpawnMine)
	EVENT( EV_Remove,		hhMineSpawner::Event_Remove )
END_CLASS

void hhMineSpawner::Spawn(void) {
	population = 0;
	targetPopulation = spawnArgs.GetInt("population");
	mineVelocity = spawnArgs.GetVector("velocity");
	mineAVelocity = spawnArgs.GetVector("avelocity");
	bRandomDirection = spawnArgs.GetBool("randdir");
	bRandomRotation = spawnArgs.GetBool("randrot");
	spawnDelay = SEC2MS(spawnArgs.GetFloat("spawndelay"));
	speed = mineVelocity.Length();
	aspeed = mineAVelocity.Length();
	active = spawnArgs.GetBool("start_on");

	GetPhysics()->SetContents(0);
	if (targetPopulation > 0 && active) {
		PostEventMS(&EV_SpawnMine, 2000);
	}
	BecomeActive(TH_THINK);		// Need to be active in order to get dormant messages
}

void hhMineSpawner::Save(idSaveGame *savefile) const {
	savefile->WriteInt( population );
	savefile->WriteInt( targetPopulation );
	savefile->WriteVec3( mineVelocity );
	savefile->WriteVec3( mineAVelocity );
	savefile->WriteBool( bRandomDirection );
	savefile->WriteBool( bRandomRotation );
	savefile->WriteFloat( speed );
	savefile->WriteFloat( aspeed );
	savefile->WriteInt( spawnDelay );
	savefile->WriteBool( active );
}

void hhMineSpawner::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( population );
	savefile->ReadInt( targetPopulation );
	savefile->ReadVec3( mineVelocity );
	savefile->ReadVec3( mineAVelocity );
	savefile->ReadBool( bRandomDirection );
	savefile->ReadBool( bRandomRotation );
	savefile->ReadFloat( speed );
	savefile->ReadFloat( aspeed );
	savefile->ReadInt( spawnDelay );
	savefile->ReadBool( active );
}

void hhMineSpawner::DormantBegin() {
	// Remove all pending spawn events
	CancelEvents(&EV_SpawnMine);
}

void hhMineSpawner::DormantEnd() {
	// restart if active
	CheckPopulation();
}

void hhMineSpawner::CheckPopulation() {
	if (active && population < targetPopulation) {
		CancelEvents(&EV_SpawnMine);
		PostEventMS(&EV_SpawnMine, spawnDelay);
	}
}

void hhMineSpawner::MineRemoved(hhMine *mine) {
	--population;
	CheckPopulation();
}

void hhMineSpawner::SpawnMine() {
	if (fl.isDormant) {
		return;
	}

	const char *mineDefName = spawnArgs.GetString("def_mine");
	idBounds bounds = GetPhysics()->GetAbsBounds();
	idVec3 location = hhUtils::RandomPointInBounds(bounds.Expand(-33));

	// If won't fit, wait and spawn later
	if ( spawnArgs.GetBool( "force_spawn", "0" ) || hhUtils::EntityDefWillFit(mineDefName, location, mat3_identity, CONTENTS_SOLID, NULL)) {
		if (bRandomDirection) {
			mineVelocity = hhUtils::RandomVector() * speed;
		}
		if (bRandomRotation) {
			mineAVelocity = hhUtils::RandomVector() * aspeed;
		}
		idDict args;
		args.SetVector("origin", location);

		hhMine *mine = static_cast<hhMine*>(gameLocal.SpawnObject(mineDefName, &args));
		if (mine) {
			ActivateTargets( this );
			mine->SetSpawner(this);
			mine->Launch(mineVelocity, mineAVelocity);
			float stopDelay = spawnArgs.GetFloat( "stop_delay", "0" );
			if ( stopDelay > 0.0f ) {
				mine->PostEventSec( &EV_MineHover, stopDelay );
			}
		}
		++population;
		CheckPopulation();
	}
	else {
		// Check population with low delay, since it's already time to spawn
		if (active && population < targetPopulation) {
			CancelEvents(&EV_SpawnMine);
			PostEventMS(&EV_SpawnMine, 500);
		}
	}
}

void hhMineSpawner::Event_Activate(idEntity *activator) {
	if ( spawnArgs.GetBool( "limit_triggers", "0" ) ) {
		active = false;
		CancelEvents(&EV_SpawnMine);	// Turn off
		if ( population < targetPopulation ) {
			SpawnMine();					// This also will continue to check population
		}
		return;
	}

	// Use population=0 to spawn purely based on triggers
	if (active && targetPopulation != 0) {
		active = false;
		CancelEvents(&EV_SpawnMine);	// Turn off
	}
	else {
		active = true;
		SpawnMine();					// This also will continue to check population
	}
}

void hhMineSpawner::Event_SpawnMine() {
	SpawnMine();
}

void hhMineSpawner::Event_Remove() {
	// Handle removal gracefully
	idEntity *ent = NULL;
	hhMine *mine = NULL;
	for (int ix=0; ix<gameLocal.num_entities; ix++) {
		ent = gameLocal.entities[ix];
		if (ent && ent->IsType(hhMine::Type)) {
			mine = static_cast<hhMine*>(ent);
			if (mine->WasSpawnedBy(this)) {
				mine->SetSpawner(NULL);
				if ( spawnArgs.GetBool( "explode_on_remove" ) ) {
					mine->Explode( NULL );
				}
			}
		}
	}
	idEntity::Event_Remove();
}
