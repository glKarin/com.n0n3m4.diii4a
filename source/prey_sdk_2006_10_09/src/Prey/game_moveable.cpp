#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#define PLAYER_COLLISION_PRINTF(e) if( (e)->IsType(hhPlayer::Type) ) gameLocal.Printf

const idEventDef EV_HoverTo( "hoverTo", "v" );
const idEventDef EV_HoverMove( "<hovermove>" );
const idEventDef EV_Unhover( "unhover" );
const idEventDef EV_FadeOutDebris( "<fadedebris>", "f" );

CLASS_DECLARATION( idMoveable, hhMoveable )
	EVENT( EV_HoverTo,			hhMoveable::Event_HoverTo )
	EVENT( EV_HoverMove,		hhMoveable::Event_HoverMove )
	EVENT( EV_Unhover,			hhMoveable::Event_Unhover )
	EVENT( EV_Touch,			hhMoveable::Event_Touch )
	EVENT( EV_SpawnFxFlyLocal,	hhMoveable::Event_SpawnFxFlyLocal )
	EVENT( EV_FadeOutDebris,	hhMoveable::Event_StartFadingOut )
END_CLASS

/*
================
hhMoveable::hhMoveable
================
*/
hhMoveable::hhMoveable() {
}

hhMoveable::~hhMoveable() {
	SAFE_REMOVE( fxFly );
}

/*
================
hhMoveable::Spawn
================
*/
void hhMoveable::Spawn() {

	fl.takedamage = health > 0;

	hoverController = NULL;
	nextDamageTime = 0;

	// idMoveable forces friction to (0.6f, 0.6f, friction)
	float linearFriction = spawnArgs.GetFloat( "linearFriction", "0.6" );
	float angularFriction = spawnArgs.GetFloat( "angularFriction", "0.6" );
	float contactFriction = spawnArgs.GetFloat( "friction", "0.6" );
	physicsObj.SetFriction( linearFriction, angularFriction, contactFriction );

	float gravityMagnitude = physicsObj.GetGravity().Length();
	collisionSpeed_min = hhUtils::DetermineFinalFallVelocityMagnitude( spawnArgs.GetFloat("collideDist_min"), gravityMagnitude );

	currentChannel = SCHANNEL_ANY;

	if (spawnArgs.GetBool("walkthrough")) {
		GetPhysics()->SetContents( CONTENTS_TRIGGER | CONTENTS_RENDERMODEL ); // CJR:  Removed CONTENTS_CORPSE because ragdolls were colliding with these moveables 
		GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );
	}

	// Check if the moveable wants to remove itself after a certain amount of time
	float removeTime = spawnArgs.GetFloat("removeTime");
	removeOnCollision = spawnArgs.GetBool("removeOnCollision");

	float duration = spawnArgs.GetFloat("duration"); // Ignore removeTime if duration is set -mdl
	if (duration == 0.0f && !removeOnCollision && removeTime > 0.0f) {
		PostEventSec(&EV_Remove, removeTime);
	}

	if (!gameLocal.isClient) {
		// CJR:  spawn an optional flight fx system
		const char *flyName = spawnArgs.GetString( "fx_fly" );
		if ( flyName && flyName[0] ) {
			BroadcastFx( flyName, EV_SpawnFxFlyLocal );
		}
	}

	fadeAlpha.Init(gameLocal.time, 0, 1.0f, 1.0f);

	float fadeTime = spawnArgs.GetFloat("fadeouttime");
	if (duration > 0.0f) {
		if (fadeTime > duration) {
			fadeTime = duration;
		}
		if (fadeTime > 0.0f) {
			PostEventSec(&EV_FadeOutDebris, duration-fadeTime, fadeTime);
			PostEventSec(&EV_Remove, duration);
		}
	}

	notPushableAI = spawnArgs.GetBool( "notPushableAI", "0" );
}

void hhMoveable::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( collisionSpeed_min );
	savefile->WriteInt( currentChannel );
	savefile->WriteObject( hoverController );
	savefile->WriteVec3( hoverPosition );
	savefile->WriteVec3( hoverAngle );
	savefile->WriteBool( removeOnCollision );
	savefile->WriteBool( notPushableAI );

	savefile->WriteFloat( fadeAlpha.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( fadeAlpha.GetDuration() );
	savefile->WriteFloat( fadeAlpha.GetStartValue() );
	savefile->WriteFloat( fadeAlpha.GetEndValue() );

	fxFly.Save( savefile );
	savefile->WriteInt( nextDamageTime );
}

void hhMoveable::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( collisionSpeed_min );
	savefile->ReadInt( currentChannel );
	savefile->ReadObject( reinterpret_cast<idClass *&>(hoverController) );
	savefile->ReadVec3( hoverPosition );
	savefile->ReadVec3( hoverAngle );
	savefile->ReadBool( removeOnCollision );
	savefile->ReadBool( notPushableAI );

	float set;

	savefile->ReadFloat( set );			// idInterpolate<float>
	fadeAlpha.SetStartTime( set );
	savefile->ReadFloat( set );
	fadeAlpha.SetDuration( set );
	savefile->ReadFloat( set );
	fadeAlpha.SetStartValue(set);
	savefile->ReadFloat( set );
	fadeAlpha.SetEndValue( set );

	fxFly.Restore( savefile );
	savefile->ReadInt( nextDamageTime );
}

/*
============
hhMoveable::SquishedByDoor
============
*/
void hhMoveable::SquishedByDoor(idEntity *door) {
	// Get rid of any moveables caught in doors
	Killed(door, door, 0, vec3_origin, 0);
}

/*
============
hhMoveable::Killed
============
*/
void hhMoveable::Killed( idEntity *inflictor, idEntity *attacker, int damageAmt, const idVec3 &dir, int location ) {
	fl.takedamage = false;
	GetPhysics()->SetContents(0);	// Turn collision off so other entities can spawn in

	if ( unbindOnDeath ) {
		Unbind();
	}

	if ( renderEntity.gui[ 0 ] ) {
		renderEntity.gui[ 0 ] = NULL;
	}

	ActivateTargets(attacker);

	// nla - Taken from hhAI::Killed
	const char *dropDef = spawnArgs.GetString( "def_drop", NULL);
	if ( dropDef && *dropDef ) {
		idDict args;
		args.Set( "origin", physicsObj.GetOrigin().ToString() );
		gameLocal.SpawnObject( dropDef, &args );
	}

	const char *debrisDef = spawnArgs.GetString("def_debrisspawner", NULL);
	if ( debrisDef && *debrisDef ) {
		hhUtils::SpawnDebrisMass(debrisDef, this);
	}

	PostEventMS( &EV_Remove, 0 );
}

/*
================
hhMoveable::DetermineNextChannel
================
*/
s_channelType hhMoveable::DetermineNextChannel() {
	static int NUM_CHANNELS = 6;
	
	currentChannel = (currentChannel + 1) % NUM_CHANNELS;
	return (s_channelType)((currentChannel == SCHANNEL_ANY) ? ++currentChannel : currentChannel);
}

/*
================
hhMoveable::AttemptToPlayBounceSound
================
*/
void hhMoveable::AttemptToPlayBounceSound( const trace_t &collision, const idVec3 &velocity ) {
	static const float minCollisionVelocity = 20.0f;
	static const float maxCollisionVelocity = 650.0f;
	s_channelType channel;

	float len = velocity * -collision.c.normal;
	if( len > minCollisionVelocity && collision.c.material && !(collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT) ) {
		channel = DetermineNextChannel();
		if( StartSound("snd_bounce", channel) ) {
			// Change volume only after we know the sound played
			float volume = hhUtils::CalculateSoundVolume( len, minCollisionVelocity, maxCollisionVelocity );
			HH_SetSoundVolume( volume, channel );
		}
	}
}

/*
================
hhMoveable::Collide
================
*/
bool hhMoveable::Collide( const trace_t &collision, const idVec3 &velocity ) {
	idVec3 dir = velocity;
	dir.Normalize();

	if (removeOnCollision) {
		StartSound( "snd_splat", SND_CHANNEL_ANY );
		PostEventMS(&EV_Remove, 0);
		return true;
	}

	AttemptToPlayBounceSound( collision, velocity );

	idEntity*	entity = ValidateEntity( collision.c.entityNum );
	if ( gameLocal.time > nextDamageTime && damage.Length() ) {
		if ( entity ) {
			if ( DetermineCollisionSpeed(entity, collision.c.point, entity->GetPhysics()->GetLinearVelocity(), GetOrigin(), GetPhysics()->GetLinearVelocity()) > collisionSpeed_min ) {
				nextDamageTime = gameLocal.time + SEC2MS(0.5);	//prevent multi-collision damage
				entity->Damage( this, GetPhysics()->GetClipModel()->GetOwner(), dir, damage, 1.0f, INVALID_JOINT );
			}
		}
	}


	if ( fxCollide.Length() && gameLocal.time > nextCollideFxTime ) {
		float len = velocity * -collision.c.normal;
		if (len > 50) {
			hhFxInfo fxInfo;
			fxInfo.RemoveWhenDone( true );
			BroadcastFxInfo( fxCollide, collision.c.point, mat3_identity, &fxInfo );
			nextCollideFxTime = gameLocal.time + 1000;
		}
	}

	// CJR:  Disable the flight fx effect the first time the moveable hits something
	if ( fxFly.IsValid() ) {
		fxFly->Nozzle( false );
		SAFE_REMOVE( fxFly );
	}

	return false;
}

/*
================
hhMoveable::ValidateEntity
================
*/
idEntity* hhMoveable::ValidateEntity( const int collisionEntityNum ) {
	if( collisionEntityNum >= ENTITYNUM_WORLD ) {
		return NULL;
	}

	return gameLocal.entities[collisionEntityNum];
}

/*
================
hhMoveable::DetermineCollisionSpeed

Used so we only use linear velocity in our equations
================
*/
float hhMoveable::DetermineCollisionSpeed( const idEntity* entity, const idVec3& point1, const idVec3& velocity1, const idVec3& point2, const idVec3& velocity2 ) {
	idVec3 originVector = point1 - point2;
	originVector.Normalize();
	idVec3 reversedOriginVector = -originVector;
	float vel1Dot = (velocity1 * reversedOriginVector);
	float vel2Dot = (velocity2 * originVector);

	if( vel2Dot < 0.0f ) {
		return 0.0f;
	}

	idVec3 adjustedVel1 = vel1Dot * reversedOriginVector;
	idVec3 adjustedVel2 = vel2Dot * originVector;

	//PLAYER_COLLISION_PRINTF(entity)( "Vel1: %s, Vel2: %s\n", adjustedVel1.ToString(), adjustedVel2.ToString() );
		
 	return (adjustedVel2 - adjustedVel1).Length();
}

/*
================
hhMoveable::ApplyImpulse
================
*/
void hhMoveable::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) {
 	if ( notPushableAI && ent && ent->IsType( idAI::Type ) ) {
		return;
	}
	GetPhysics()->ApplyImpulse( id, point, impulse );
}

void hhMoveable::AllowImpact( bool allow ) {
	if ( allow ) {
		physicsObj.EnableImpact();
	} else {
		physicsObj.DisableImpact();
	}
}

/*
========================
hhMoveable::HoverTo
========================
*/
void hhMoveable::Event_HoverTo( const idVec3 &position ) {

	if ( !hoverController ) {
		const char *controllerDef = spawnArgs.GetString( "def_hoverController", NULL );
 	
 		if ( controllerDef ) {
			hoverController = static_cast<hhBindController *>( gameLocal.SpawnObject( controllerDef ) );
		}
		else {
			hoverController = NULL;
		}
	}

	//gameLocal.Printf( "Hover to %s %.2f\n", position.ToString(), (float) GetPhysics()->GetMass() );
	if ( !hoverController ) {
		gameLocal.Warning( "Event_HoverTo: Tried to hover with an invalid def_hoverController" );
		return;
	}

	float tension;	
	tension = GetPhysics()->GetMass() / 64.0 * spawnArgs.GetFloat( "hover_tension", ".01" );

	hoverPosition = position;
	if ( ! spawnArgs.GetBool( "hover_gravity", "0" ) || gameLocal.GetGravityNormal().Compare(vec3_origin, VECTOR_EPSILON ) ) {
		hoverAngle = GetPhysics()->GetOrigin() - position;
	}
	else {
		hoverAngle = gameLocal.GetGravityNormal();
	}
	hoverAngle.Normalize();

	hoverController->SetTension( tension );
	hoverController->SetOrigin( hoverPosition );
	hoverController->Attach( this, true );

	float rotation = spawnArgs.GetFloat( "hover_rotation", "0" );
	if ( rotation ) {
		GetPhysics()->SetAngularVelocity( idVec3( rotation, rotation, rotation ));
	}
	
	PostEventMS( &EV_HoverMove, 100 );
}


/*
=====================
hhMoveable::HoverMove
=====================
*/
void hhMoveable::Event_HoverMove( ) {
	float dist;
	float freq;  // (in Hz)
	float period;
	float offset;
	float normalizedT;


	if ( !hoverController ) {
		gameLocal.Warning( "Event_HoverMove: Tried to hover move with an invalid def_hoverController" );
		return;
	}

	// Get the total distance to travel
	//! Cache these
	float height = GetPhysics()->GetBounds()[ 1 ][ 2 ] - GetPhysics()->GetBounds()[ 0 ][ 2 ];
	dist = spawnArgs.GetFloat( "hover_height_frac", ".1" ) * height;
	freq = spawnArgs.GetFloat( "hover_freq", ".5" );
	
	if ( !freq || !dist ) {
		return;
	}

	period = 1 / freq;
	
	// Find out what offset we should be at this point in time.
	normalizedT = ( gameLocal.time % (int) ( period * 1000 ) ) / (float) ( period * 1000 );
	offset = idMath::Sin( normalizedT * idMath::TWO_PI ) * dist;

	// Set our new position	
	hoverController->SetOrigin( hoverPosition + hoverAngle * offset );
	
	// Lets do it again in 1/10th of a second
	PostEventMS( &EV_HoverMove, 100 );
}


/*
=====================
hhMoveable::Unhover
=====================
*/
void hhMoveable::Event_Unhover( ) {

	if ( !hoverController ) {
		gameLocal.Warning( "Event_Unhover: Tried to unhover with an invalid def_hoverController" );
		return;
	}
	//gameLocal.Printf( "Unhover man" );

	hoverController->Detach( );
	
	SAFE_REMOVE( hoverController );
	CancelEvents( &EV_HoverMove );
}

/*
================
hhMoveable::Event_Touch
================
*/
void hhMoveable::Event_Touch( idEntity *other, trace_t *trace ) {
	if (spawnArgs.GetBool("walkthrough")) {
		idVec3 otherVel = other->GetPhysics()->GetLinearVelocity();
		float otherSpeed = otherVel.NormalizeFast();
		if (otherSpeed > 50.0f && GetPhysics()->IsAtRest()) {
			idVec3 toSide = hhUtils::RandomSign() * other->GetAxis()[1];
			idVec3 toMoveable = GetOrigin() - other->GetOrigin();
			toMoveable.NormalizeFast();
			idVec3 newVel = ( 3*otherVel + 6*toSide + hhUtils::RandomVector() ) * (1.0f/10.0f);
			newVel.z = 0.1f;
			newVel.NormalizeFast();
			newVel *= otherSpeed*1.5f;
			GetPhysics()->SetLinearVelocity(newVel);
		}
	}
}

/*
================
hhMoveable::Event_SpawnFxFlyLocal

CJR:  Based upon the code in projectiles
================
*/
void hhMoveable::Event_SpawnFxFlyLocal( const char* defName ) {
	if( !defName || !defName[0] ) {
		return;
	}
		
	hhFxInfo fxInfo;

	fxInfo.SetNormal( -GetAxis()[0] );
	fxInfo.SetEntity( this );
	fxInfo.RemoveWhenDone( false );
	fxFly = SpawnFxLocal( defName, GetOrigin(), GetAxis(), &fxInfo );
}

/*
============
hhMoveable::Event_StartFadingOut()
============
*/
void hhMoveable::Event_StartFadingOut(float fadetime) {
	float scale = renderEntity.shaderParms[SHADERPARM_ANY_DEFORM_PARM1];
	fadeAlpha.Init( gameLocal.time, SEC2MS(fadetime), scale > 0.0f ? scale : 1.0f, 0.01f );
	BecomeActive( TH_TICKER );
}

/*
============
hhMoveable::Ticker()
============
*/
void hhMoveable::Ticker( void ) {
	SetDeformation( DEFORMTYPE_SCALE, fadeAlpha.GetCurrentValue(gameLocal.time) );
}

