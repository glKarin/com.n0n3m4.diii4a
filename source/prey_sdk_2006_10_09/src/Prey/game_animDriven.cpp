#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_PlayCycle( "<playCycle>", "ds" );

CLASS_DECLARATION( hhAnimatedEntity, hhAnimDriven )
	EVENT( EV_PlayCycle,		hhAnimDriven::Event_PlayCycle )
END_CLASS


//==============
// hhAnimDriven::Spawn
//==============
void hhAnimDriven::Spawn() {

	passenger = NULL;
	hadPassenger = false;
	spawnTime = gameLocal.time;

	// We are gonna move the guy manually, turn off having the anim move him
	GetAnimator()->RemoveOriginOffset( true );

	// How do we deal with our owner?
	physicsAnim.SetSelf( this );

	if( spawnArgs.GetBool( "solid", "0" ) ) {
		fl.takedamage = true;
		physicsAnim.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
		GetPhysics()->SetContents( CONTENTS_SOLID );
	} else {
		fl.takedamage = false;
		GetPhysics()->SetContents( 0 );
		physicsAnim.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
		GetPhysics()->UnlinkClip();
	}

	
	// move up to make sure the monster is at least an epsilon above the floor
	physicsAnim.SetOrigin( GetOrigin() + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	
	SetPhysics( &physicsAnim );

	//? Have this start at a set time afterwards?
	int animLength = PlayAnim( ANIMCHANNEL_ALL, "initial" );
	PostEventMS( &EV_PlayCycle, animLength, ANIMCHANNEL_ALL, "move" );

	float delta_min = spawnArgs.GetFloat( "delta_scale_min", "1.0" );
	float delta_max = spawnArgs.GetFloat( "delta_scale_max", "1.0" );
	if ( delta_min != 1.0f || delta_max != 1.0f ) {
		deltaScale.x = idMath::ClampFloat( delta_min, delta_max, gameLocal.random.RandomFloat() );
		deltaScale.y = idMath::ClampFloat( delta_min, delta_max, gameLocal.random.RandomFloat() );
		deltaScale.z = idMath::ClampFloat( delta_min, delta_max, gameLocal.random.RandomFloat() );
	} else {
		deltaScale = idVec3( 1.0f, 1.0f, 1.0f );
	}

    BecomeActive( TH_TICKER );
}

void hhAnimDriven::Save(idSaveGame *savefile) const {
	savefile->WriteStaticObject( physicsAnim );
	passenger.Save(savefile);
	savefile->WriteInt( spawnTime );
	savefile->WriteBool( hadPassenger );
}

void hhAnimDriven::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( physicsAnim );
	passenger.Restore(savefile);
	savefile->ReadInt( spawnTime );
	savefile->ReadBool( hadPassenger );
}


//=============
// hhAnimDriven::SetPassenger
//=============
void hhAnimDriven::SetPassenger( idEntity *newPassenger, bool orientFromPassenger ) {
	idVec3 origin;
	idMat3 axis;

	//?! Right now handle only 1 passenger.  If we have one, warn and exit.
	if ( passenger.IsValid() ) {
		const char *oldName = passenger->GetName();
		const char *newName = "NULL";
		
		if ( newPassenger ) {
			newName = newPassenger->GetName();
		}
		gameLocal.Warning( "Error: hhAnimDriven::SetPassenger tried to add passenger %s but already had %s\n",
						   newName, oldName );
		return;
	}
	
	passenger = newPassenger;
	hadPassenger = true;
	fl.takedamage = true;

	if ( orientFromPassenger ) {
		origin = passenger->GetOrigin();
		axis = passenger->GetAxis();
	}
	
	GetPhysics()->SetClipModel( new idClipModel( passenger->GetPhysics()->GetClipModel() ), 1.0f );
	GetPhysics()->SetContents( passenger->GetPhysics()->GetContents() );
	GetPhysics()->SetClipMask( passenger->GetPhysics()->GetClipMask() );

	if ( orientFromPassenger ) {
		SetOrigin( origin );
		SetAxis( axis ); 
	}

	passenger->Bind( this, true );

}


//==============
// hhAnimDriven::Think
//==============
void hhAnimDriven::Think() {
	idVec3 delta;

	// Done move unless we have a passenger.  (Give us like 10 secs to get one)
	if ( !passenger.IsValid() ) {
		// If we had a passenger, they removed themselves, and so should we :)
		if ( hadPassenger ) {
			PostEventMS( &EV_Remove, 0 );
		}
		// If never had a passenger for a second, remove ourselves
		else if ( gameLocal.time - spawnTime > 1000 ) {
			PostEventMS( &EV_Remove, 0 );
			gameLocal.Warning( "hhAnimDriven %s existed for a second w/out a passenger.  Removing.", GetName() );
		}
		return;
	}

	// Move them based on their anim
	GetAnimator()->GetDelta( gameLocal.time - gameLocal.msec, gameLocal.time, delta );
	delta *= GetAxis();
	delta.x *= deltaScale.x;
	delta.y *= deltaScale.y;
	delta.z *= deltaScale.z;

	physicsAnim.SetDelta( delta );

	// gameLocal.Printf( "%d Doing %s\n", gameLocal.GetTime(), delta.ToString() );
	
	hhAnimatedEntity::Think();

}


//============
//
//============
int hhAnimDriven::PlayAnim( int channel, const char *animName ) {
	int			animIndex;
	int			length;


	animIndex = GetAnimator()->GetAnim( animName );
	if ( !animIndex ) {
		return( 0 );
	}

	//gameLocal.Printf( "Playing Anim %s\n", animName );

	GetAnimator()->PlayAnim( channel, animIndex, gameLocal.GetTime(), 0 );

	length = GetAnimator()->CurrentAnim( channel )->GetEndTime() - gameLocal.GetTime();

	return( length );

}


//============
// hhAnimDrive::Event_PlayCycle()
//============
void hhAnimDriven::Event_PlayCycle( int channel, const char *animName ) {

	PlayCycle( channel, animName );

}


//============
//
//============
bool hhAnimDriven::PlayCycle( int channel, const char *animName ) {
	int			animIndex;

	animIndex = GetAnimator()->GetAnim( animName );
	if ( !animIndex ) {
		return( false );
	}

	//gameLocal.Printf( "Cycling Anim %s\n", animName );

	GetAnimator()->CycleAnim( channel, animIndex, gameLocal.GetTime(), 0 );

	return( true );
}


/*
================
hhAnimDriven::ClearAllAnims
================
*/
void hhAnimDriven::ClearAllAnims() {

	GetAnimator()->ClearAllAnims( gameLocal.GetTime(), 0 );

}


//===========
//
//============
bool hhAnimDriven::Collide( const trace_t &collision, const idVec3 &velocity ) {
	// If we hit something, and have a passenger, pass along the message
	if( passenger.IsValid() ) {
		if( !passenger->Collide(collision, velocity) ) {
			return( false );
		}

		passenger->Unbind();
	}	//. We have a valid passenger


	// If we made it this far, assume the collision is for real, and
	//   remove ourselves from interacting with the world
	fl.takedamage = false;
	GetPhysics()->SetContents( 0 );
	GetPhysics()->UnlinkClip();
	ClearAllAnims();
	PostEventMS( &EV_Remove, 0 );

	return( true );
}

bool hhAnimDriven::AllowCollision( const trace_t& collision ) {
	if ( !spawnArgs.GetBool( "projectile_collision", "1" ) ) {
		idEntity* ent = gameLocal.entities[ collision.c.entityNum ];
		if ( !ent || ent->IsType( idProjectile::Type ) ) {
			return false;
		}
	}

	return true;
}

void hhAnimDriven::UpdateAnimation( void ) {
	PROFILE_SCOPE("Animation", PROFMASK_NORMAL);	// HUMANHEAD pdm

	// don't do animations if they're not enabled
	if ( !( thinkFlags & TH_ANIMATE ) ) {
		return;
	}

	// is the model an MD5?
	if ( !animator.ModelHandle() ) {
		// no, so nothing to do
		return;
	}

	// call any frame commands that have happened in the past frame
	if ( !fl.hidden ) {
		animator.ServiceAnims( gameLocal.previousTime, gameLocal.time );
	}

	// if the model is animating then we have to update it
	if ( !animator.FrameHasChanged( gameLocal.time ) ) {
		// still fine the way it was
		return;
	}

	// update the renderEntity
	UpdateVisuals();

	// the animation is updated
	animator.ClearForceUpdate();
}
