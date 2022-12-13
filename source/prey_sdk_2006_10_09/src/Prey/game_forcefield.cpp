//**************************************************************************
//**
//** GAME_FORCEFIELD.CPP
//**
//** Game code for the forcefield
//**
//**************************************************************************

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( idEntity, hhForceField )
	EVENT( EV_Activate,	   				hhForceField::Event_Activate )
END_CLASS

/*
===========
hhForceField::Spawn
===========
*/
void hhForceField::Spawn(void) {
	fl.takedamage = true;
	SetShaderParm( SHADERPARM_TIMEOFFSET, 1.0f );
	SetShaderParm( SHADERPARM_MODE, 0.0f );

	fade = 0.0f;

	activationRate = spawnArgs.GetFloat( "activationRate" );
	deactivationRate = spawnArgs.GetFloat( "deactivationRate" );
	undamageFadeRate = spawnArgs.GetFloat( "undamageFadeRate" );

//	BecomeActive( TH_THINK|TH_TICKER );

	cachedContents = CONTENTS_FORCEFIELD | CONTENTS_BLOCK_RADIUSDAMAGE | CONTENTS_SHOOTABLE;
	
	physicsObj.SetSelf( this );

	if (spawnArgs.GetBool("isSimpleBox")) {
		// Simple boxes are cheaper and can be bound to other objects
		physicsObj.SetClipModel( new idClipModel(idTraceModel(GetPhysics()->GetBounds())), 1.0f );
		physicsObj.SetContents( cachedContents );
		physicsObj.SetOrigin( GetOrigin() );
		physicsObj.SetAxis( GetAxis() );
		SetPhysics( &physicsObj );
	}
	else {
		// Non-simple has real per-poly collision with it's model, uses default static physics because we don't have a tracemodel.
		// This loses the activation of contacts on the object (so some things may not fall through when disabled), but in my tests,
		// the spirit proxy falls through fine.  NOTE: To fix the movables, etc. not falling when these are turned off, I'm manually
		// Activating appropriate physics that overlap the bounds when Turned off.
//		physicsObj.SetClipModel( new idClipModel(GetPhysics()->GetClipModel()), 1.0f );
//		physicsObj.SetContents( cachedContents );
//		physicsObj.SetOrigin( GetOrigin() );
//		physicsObj.SetAxis( GetAxis() );
//		SetPhysics( &physicsObj );

		// Apparently the flags need to be on the material to make projectiles hit these.
		GetPhysics()->SetContents(cachedContents);
	}

	EnterOnState();
	damagedState = false;

	nextCollideFxTime = gameLocal.time;
	

	fl.networkSync = true; //always want to sync over the net
}

void hhForceField::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( fieldState );
	savefile->WriteBool( damagedState );
	savefile->WriteFloat( activationRate );
	savefile->WriteFloat( deactivationRate );
	savefile->WriteFloat( undamageFadeRate );
	savefile->WriteInt( applyImpulseAttempts );
	savefile->WriteInt( cachedContents );
	savefile->WriteFloat( fade );
	savefile->WriteInt( nextCollideFxTime );
	savefile->WriteStaticObject( physicsObj );
}

void hhForceField::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( reinterpret_cast<int &> ( fieldState ) );
	savefile->ReadBool( damagedState );
	savefile->ReadFloat( activationRate );
	savefile->ReadFloat( deactivationRate );
	savefile->ReadFloat( undamageFadeRate );
	savefile->ReadInt( applyImpulseAttempts );
	savefile->ReadInt( cachedContents );
	savefile->ReadFloat( fade );
	savefile->ReadInt( nextCollideFxTime );
	savefile->ReadStaticObject( physicsObj );

	// Only restore physics if we were using it before
	if (spawnArgs.GetBool("isSimpleBox")) {
		RestorePhysics( &physicsObj );
	}
}

void hhForceField::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot(msg);
	msg.WriteBits(damagedState, 1);
	msg.WriteFloat(activationRate);
	msg.WriteFloat(deactivationRate);
	msg.WriteFloat(undamageFadeRate);
	msg.WriteBits(applyImpulseAttempts, 32);
	msg.WriteBits(cachedContents, 32);
	msg.WriteFloat(fade);
	msg.WriteBits(nextCollideFxTime, 32);
	msg.WriteBits(fieldState, 4);
	msg.WriteBits(IsHidden(), 1);

	msg.WriteFloat(renderEntity.shaderParms[SHADERPARM_TIMEOFFSET]);
	msg.WriteFloat(renderEntity.shaderParms[SHADERPARM_MODE]);
}

void hhForceField::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	physicsObj.ReadFromSnapshot(msg);
	damagedState = !!msg.ReadBits(1);
	activationRate = msg.ReadFloat();
	deactivationRate = msg.ReadFloat();
	undamageFadeRate = msg.ReadFloat();
	applyImpulseAttempts = msg.ReadBits(32);
	cachedContents = msg.ReadBits(32);
	fade = msg.ReadFloat();
	nextCollideFxTime = msg.ReadBits(32);
	fieldState = (States)msg.ReadBits(4);
	bool hidden = !!msg.ReadBits(1);
	if (IsHidden() != hidden) {
		if (hidden) {
			Hide();
			SetSkinByName( spawnArgs.GetString("skin_off" ) );
			GetPhysics()->SetContents( 0 );
		}
		else {
			Show();
			SetSkinByName( NULL );
			GetPhysics()->SetContents( cachedContents );
		}
	}

	float f;
	bool changed = false;
	
	f = msg.ReadFloat();
	changed = (changed || (renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] != f));
	renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = f;
	
	f = msg.ReadFloat();
	changed = (changed || (renderEntity.shaderParms[SHADERPARM_MODE] != f));
	renderEntity.shaderParms[SHADERPARM_MODE] = f;

	if (changed) {
		UpdateModel();
		UpdateVisuals();
	}
}

void hhForceField::ClientPredictionThink( void ) {
	idEntity::ClientPredictionThink();
}

/*
===========
hhForceField::ApplyImpulse
===========
*/
void hhForceField::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &force ) {

	if (!ent->IsType(idActor::Type)) {
		// Don't play sound for actor footsteps, landings.  They play their own localized sounds for
		// the sake of very large forcefields.
		StartSound( "snd_impulse", SND_CHANNEL_ANY );
	}
	EnterDamagedState();

	// Apply the hit effect
	trace_t trace;
	idStr fxCollide = spawnArgs.GetString( "fx_collide" );

	if ( fxCollide.Length() && gameLocal.time > nextCollideFxTime && point != GetOrigin()) {
		// Trace to find normal
		idVec3 dir = force;
		dir.NormalizeFast();
		dir *= 200;
		idVec3 start = point-dir;
		idVec3 end = point+dir;
		memset(&trace, 0, sizeof(trace));
		gameLocal.clip.TracePoint(trace, start, end, CONTENTS_FORCEFIELD, ent);

		if (trace.fraction < 1.0f) {
			// Spawn fx oriented to normal of collision
			hhFxInfo fxInfo;
			fxInfo.SetNormal( trace.c.normal );
			BroadcastFxInfo( fxCollide, trace.c.point, mat3_identity, &fxInfo, 0, false ); //rww - changed to not broadcast
			nextCollideFxTime = gameLocal.time + 200;
		}
	}
}

/*
===========
hhForceField::Ticker
===========
*/
void hhForceField::Ticker( void ) {

	if( fieldState == StatePreTurningOn ) {
		idEntity*		entityList[MAX_GENTITIES];
		idEntity*		entity = NULL;
		idPhysics*		physics = NULL;
		int				numEntities = 0;
		int				numEntitiesImpulseAppliedTo = 0;
		idVec3			force = DetermineForce() * GetAxis();

		numEntities = gameLocal.clip.EntitiesTouchingBounds( GetPhysics()->GetAbsBounds(), MASK_SOLID, entityList, MAX_GENTITIES );
		for( int ix = 0; ix < numEntities; ++ix ) {
			entity = entityList[ix];
			
			if( !entity ) {
				continue;
			}

			if( entity == this ) {
				continue;
			}

			//Removing mass from any calculations regarding impulse
			entity->ApplyImpulse( this, 0, entity->GetOrigin(), force * entity->GetPhysics()->GetMass() );
			numEntitiesImpulseAppliedTo++;
		}

		--applyImpulseAttempts;
		if( !numEntitiesImpulseAppliedTo || !applyImpulseAttempts ) {
			fieldState = StateTurningOn;
			GetPhysics()->SetContents( cachedContents );
			SetSkin( NULL );
			StartSound( "snd_start", SND_CHANNEL_ANY );

			Show();

			EnterTurningOnState();
		}
	} else if( fieldState == StateTurningOn ) {
		float deltaTime = MS2SEC( gameLocal.msec );

		renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] += activationRate * deltaTime;
		if( renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] >= 1.0f ) {
			EnterOnState();
		}

		UpdateVisuals();
	} else if( fieldState == StateTurningOff ) {
		float deltaTime = MS2SEC( gameLocal.msec );

		renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] -= deactivationRate * deltaTime;
		if( renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] <= 0.0f ) {
			EnterOffState();
		}

		UpdateVisuals();
	}

	if( damagedState ) {
		float deltaTime = MS2SEC( gameLocal.msec );

		// Fade parm back to normal
		fade -= undamageFadeRate * deltaTime;
		if ( fade <= 0.0f ) { // Finished fading
			fade = 0;
			damagedState = false;
		}

		SetShaderParm( SHADERPARM_MODE, fade );
	}

	if (!damagedState && (fieldState==StateOn || fieldState==StateOff)) {
		BecomeInactive( TH_TICKER );
	}
}

/*
===========
hhForceField::DetermineThinnestAxis
===========
*/
int hhForceField::DetermineThinnestAxis() {
	int best = 0;
	idBounds bounds = GetPhysics()->GetBounds();
	for( int i = 1; i < 3; ++i ) {
		if ( bounds[1][ i ] - bounds[0][ i ] < bounds[1][ best ] - bounds[0][ best ] ) {
			best = i;
		}
	}

	return best;
}

/*
===========
hhForceField::DetermineForce
===========
*/
idVec3 hhForceField::DetermineForce() {
	idVec3	force( vec3_origin );
	float	forceMagnitude = spawnArgs.GetFloat( "forceMagnitude" );
	int		dir = spawnArgs.GetBool( "positiveDir", "1" ) ? 1.0f : -1.0f;

	force[DetermineThinnestAxis()] = forceMagnitude * dir;

	return force;
}

/*
===========
hhForceField::IsAtRest

Used to activate entites at rest
===========
*/
bool hhForceField::IsAtRest( int id ) const {
	return ( fieldState != StateOff );
}

/*
===========
hhForceField::Event_Activate
===========
*/
void hhForceField::Event_Activate( idEntity *activator ) {
	switch( fieldState ) {
		case StatePreTurningOn:
			EnterTurningOffState();
			break;
		case StateTurningOff:
			EnterPreTurningOnState();
			break;
		case StateTurningOn:
			EnterTurningOffState();
			break;
		case StateOn:
			EnterTurningOffState();
			break;
		case StateOff:
			EnterPreTurningOnState();
			break;
	}
}

/*
===========
hhForceField::EnterDamagedState
===========
*/
void hhForceField::EnterDamagedState() {
	damagedState = true;
	fade = 1.0f;
	SetShaderParm( SHADERPARM_MODE, fade ); // Instantly change to the new shader
	BecomeActive( TH_TICKER );
}

/*
===========
hhForceField::EnterPreTurningOnState
===========
*/
void hhForceField::EnterPreTurningOnState() {
	fieldState = StatePreTurningOn;
	applyImpulseAttempts = spawnArgs.GetInt( "applyImpulseAttempts" );
	BecomeActive( TH_TICKER );
}

/*
===========
hhForceField::EnterOnState
===========
*/
void hhForceField::EnterOnState() {
	fieldState = StateOn;
	StartSound( "snd_loop", SND_CHANNEL_IDLE );
	//HUMANHEAD PCF rww 05/15/06 - prevent stuck-in-forcefield by killing everything within
	if (gameLocal.isMultiplayer) {
		gameLocal.KillBoxMasked( this, CONTENTS_BODY );
	}
	//HUMANHEAD END
}

/*
===========
hhForceField::EnterTurningOnState
===========
*/
void hhForceField::EnterTurningOnState() {
	GetPhysics()->SetContents( cachedContents );
	SetSkin( NULL );
	StartSound( "snd_start", SND_CHANNEL_ANY );

	Show();
	BecomeActive( TH_TICKER );
}

/*
===========
hhForceField::EnterTurningOffState
===========
*/
void hhForceField::EnterTurningOffState() {
	fieldState = StateTurningOff;
	StopSound( SND_CHANNEL_IDLE );
	StartSound( "snd_stop", SND_CHANNEL_ANY );
	BecomeActive( TH_TICKER );
}

/*
===========
hhForceField::EnterOffState
===========
*/
void hhForceField::EnterOffState() {
	fieldState = StateOff;
	if (spawnArgs.GetBool("isSimpleBox")) {
		GetPhysics()->ActivateContactEntities();
	}
	else {
		// To allow complex shaped forcefields, we need to manually activate the physics of any contacts here
		idClipModel *clipModels[ MAX_GENTITIES ];
		idClipModel *cm;
		idBounds bounds = GetPhysics()->GetAbsBounds();
		bounds.ExpandSelf(bounds.GetRadius()*0.1f);	// Expand the bounds by 10% to catch things on the surface
		int num = gameLocal.clip.ClipModelsTouchingBounds( bounds, MASK_ALL, clipModels, MAX_GENTITIES );
		for ( int i=0;i<num;i++ ) {
			cm = clipModels[ i ];
			idEntity *hit = cm->GetEntity();
			if (hit && hit->GetPhysics()->IsAtRest() &&
				(hit->GetPhysics()->IsType(idPhysics_RigidBody::Type) || hit->GetPhysics()->IsType(idPhysics_AF::Type)) ) {
				if ( !hit->IsType( hhVehicle::Type ) ) {	//HUMANHEAD jsh PCF 5/26/06: fix 30hz vehicle console jittering
					hit->GetPhysics()->Activate();
				}
			}
		}
	}

	// HUMANHEAD PCF pdm 04/27/06: Unbind any bound projectiles
	idEntity *ent;
	idEntity *next;
	for( ent = teamChain; ent != NULL; ent = next ) {
		next = ent->GetTeamChain();
		if ( ent && ent->IsType( hhProjectile::Type ) ) {
			ent->Unbind();			// bjk drops all bound projectiles such as arrows and mines
			next = teamChain;

			//HUMANHEAD bjk PCF (4-28-06) - explode crawlers
			if (ent->IsType(hhProjectileStickyCrawlerGrenade::Type)) {
				static_cast<hhProjectile*>(ent)->PostEventSec( &EV_Explode, 0.2f );
			}
		}
	}

	GetPhysics()->SetContents( 0 );
	Hide();
	SetSkinByName( spawnArgs.GetString("skin_off" ) ); // Set the force field to completely not draw
}

//--------------------------------------------------------------------------
// hhShuttleForcefield
//--------------------------------------------------------------------------
CLASS_DECLARATION( idEntity, hhShuttleForceField )
	EVENT( EV_Activate,	   				hhShuttleForceField::Event_Activate )
END_CLASS

void hhShuttleForceField::Spawn() {
	nextCollideFxTime = gameLocal.time;
	GetPhysics()->SetContents(CONTENTS_SOLID|CONTENTS_PLAYERCLIP);

	// Start in the on state
	fieldState = StateOn;
	fade.Init(gameLocal.time, 0, 1.0f, 1.0f);
	SetShaderParm(SHADERPARM_TIMEOFFSET, fade.GetCurrentValue(gameLocal.time));

	fl.networkSync = true; //rww
}

void hhShuttleForceField::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &force ) {
	trace_t trace;
	idStr fxCollide = spawnArgs.GetString( "fx_collide" );

	if ( fxCollide.Length() && gameLocal.time > nextCollideFxTime && point != GetOrigin()) {

		// Trace to find normal
		idVec3 dir = force;
		dir.NormalizeFast();
		dir *= 200;
		idVec3 start = point-dir;
		idVec3 end = point+dir;
		memset(&trace, 0, sizeof(trace));
		gameLocal.clip.TracePoint(trace, start, end, CONTENTS_SOLID, ent);

		if (trace.fraction < 1.0f) {
			// Spawn fx oriented to normal of collision
			hhFxInfo fxInfo;
			fxInfo.SetNormal( trace.c.normal );
			BroadcastFxInfo( fxCollide, trace.c.point, mat3_identity, &fxInfo );
			nextCollideFxTime = gameLocal.time + 200;

			StartSound( "snd_collide", SND_CHANNEL_ANY, 0, true, NULL );
			ActivatePrefixed( "triggerCollide", this ); // bg: Feedback hook.
		}
	}
}

void hhShuttleForceField::Ticker() {
	SetShaderParm(SHADERPARM_TIMEOFFSET, fade.GetCurrentValue(gameLocal.time));
	if (fade.IsDone(gameLocal.time)) {
		if (fieldState == StateTurningOn) {
			// Entering On state
			fieldState = StateOn;
			GetPhysics()->SetContents(CONTENTS_SOLID|CONTENTS_PLAYERCLIP);
		}
		else if (fieldState == StateTurningOff) {
			// Entering Off state
			fieldState = StateOff;
			GetPhysics()->SetContents(0);
		}

		BecomeInactive(TH_TICKER);
	}
}

void hhShuttleForceField::Event_Activate(idEntity *activator) {
	int duration = SEC2MS(spawnArgs.GetFloat("fade_duration"));
	float currentFade = fade.GetCurrentValue(gameLocal.time);
	if (fieldState == StateOn || fieldState == StateTurningOn) {
		// Entering TurningOff state
		fade.Init(gameLocal.time, duration, currentFade, 0.0f);
		fieldState = StateTurningOff;
	}
	else if (fieldState == StateOff || fieldState == StateTurningOff) {
		// Entering TurningOn state
		fade.Init(gameLocal.time, duration, currentFade, 1.0f);
		fieldState = StateTurningOn;
		//HUMANHEAD PCF rww 05/15/06 - prevent stuck-in-forcefield by killing everything within
		if (gameLocal.isMultiplayer) {
			gameLocal.KillBoxMasked( this, CONTENTS_BODY );
		}
		//HUMANHEAD END
	}
	BecomeActive( TH_TICKER );
}

void hhShuttleForceField::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( nextCollideFxTime );
	savefile->WriteInt( fieldState );
	savefile->WriteFloat( fade.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( fade.GetDuration() );
	savefile->WriteFloat( fade.GetStartValue() );
	savefile->WriteFloat( fade.GetEndValue() );
}

void hhShuttleForceField::Restore( idRestoreGame *savefile ) {
	float set;

	savefile->ReadInt( nextCollideFxTime );
	savefile->ReadInt( reinterpret_cast<int &> ( fieldState ) );

	savefile->ReadFloat( set );			// idInterpolate<float>
	fade.SetStartTime( set );
	savefile->ReadFloat( set );
	fade.SetDuration( set );
	savefile->ReadFloat( set );
	fade.SetStartValue(set);
	savefile->ReadFloat( set );
	fade.SetEndValue( set );
}

void hhShuttleForceField::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits(GetPhysics()->GetContents(), 32);

	msg.WriteBits(nextCollideFxTime, 32);
	msg.WriteBits(fieldState, 8);

	msg.WriteFloat(fade.GetStartTime());
	msg.WriteFloat(fade.GetDuration());
	msg.WriteFloat(fade.GetStartValue());
	msg.WriteFloat(fade.GetEndValue());

	msg.WriteFloat(renderEntity.shaderParms[SHADERPARM_TIMEOFFSET]);
	msg.WriteFloat(renderEntity.shaderParms[SHADERPARM_MODE]);
}

void hhShuttleForceField::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	int contents = msg.ReadBits(32);
	if (contents != GetPhysics()->GetContents()) {
		GetPhysics()->SetContents(contents);
	}

	nextCollideFxTime = msg.ReadBits(32);
	fieldState = (States)msg.ReadBits(8);

	fade.SetStartTime(msg.ReadFloat());
	fade.SetDuration(msg.ReadFloat());
	fade.SetStartValue(msg.ReadFloat());
	fade.SetEndValue(msg.ReadFloat());

	float f;
	bool changed = false;
	
	f = msg.ReadFloat();
	changed = (changed || (renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] != f));
	renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = f;
	
	f = msg.ReadFloat();
	changed = (changed || (renderEntity.shaderParms[SHADERPARM_MODE] != f));
	renderEntity.shaderParms[SHADERPARM_MODE] = f;

	if (changed) {
		UpdateVisuals();
	}
}

void hhShuttleForceField::ClientPredictionThink( void ) {
	idEntity::ClientPredictionThink();
}
