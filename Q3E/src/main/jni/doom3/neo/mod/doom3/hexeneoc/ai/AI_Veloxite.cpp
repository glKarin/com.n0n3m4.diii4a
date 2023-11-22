
#pragma hdrstop

#include "Entity.h"
#include "AI.h"
#include "AI_Veloxite.h"

const float VELOX_MAX_WALL_DIST				= 75.0f; // get on walls this far away
const float VELOX_SURFACECHECK_RATE			= 0.25f; // four times per second should be enough
const float VELOX_WALLCHECK_RATE			= 1.0f;
const float VELOX_LEAP_RANGE_MIN			= 64.0f;
const float VELOX_LEAP_RANGE_MAX			= 275.0f; // this only pertains to AI_LEAPING from walls.
const float VELOX_LEAP_SPEED				= 650.0f;
const float VELOX_LEAP_MAXHEIGHT			= 48.0f;

// for all angles defined below, the angle is between floor and wall/slop angles. in other words, the angle is the opposite of what you'd think (180 - value).
const float VELOX_MAX_WALL_ANGLE			= 100.0f; // more than 100 is probably asking for clipping problems
const float VELOX_MIN_WALL_ANGLE			= 46.0f; // 46 degrees is the very nearly the max slope walkable for aas48 entites
const float VELOX_MIN_SLOPE_ANGLE			= 0.0f; // change gravity only on slopes with this angle or greater
const float VELOX_MIN_TRANS_FRAMES			= 10;
const float VELOX_MAX_TRANS_FRAMES			= 25;
const float VELOX_WALLLEAP_TRANSITIONS		= 4;

// ***********************************************************

// Script Stuff Continued

// ***********************************************************

const idEventDef AI_Veloxite_doSurfaceChecks( "velox_doSurfaceChecks", NULL, 'f' );
const idEventDef AI_Veloxite_getJumpVelocity( "velox_getJumpVelocity", "fffs", 'v' );
const idEventDef AI_Veloxite_startLeaping( "velox_startLeaping" );
const idEventDef AI_Veloxite_doneLeaping( "velox_doneLeaping" );
const idEventDef AI_Veloxite_getOffWall( "velox_getOffWall", "f" );

CLASS_DECLARATION( idAI, idAI_Veloxite )
	EVENT( AI_Veloxite_doSurfaceChecks,	idAI_Veloxite::Event_doSurfaceChecks )
	EVENT( AI_Veloxite_getJumpVelocity,	idAI_Veloxite::Event_getVeloxJumpVelocity )
	EVENT( AI_Veloxite_doneLeaping,		idAI_Veloxite::Event_doneLeaping )
	EVENT( AI_Veloxite_startLeaping,	idAI_Veloxite::Event_startLeaping )
	EVENT( AI_Veloxite_getOffWall,		idAI_Veloxite::Event_getOffWall )
END_CLASS

void idAI_Veloxite::LinkScriptVariables( void ) {
	AI_ALIGNING.LinkTo(				scriptObject, "AI_ALIGNING" );
	AI_WALLING.LinkTo(				scriptObject, "AI_WALLING" );
	AI_LEDGING.LinkTo(				scriptObject, "AI_LEDGING" );
	AI_SLOPING.LinkTo(				scriptObject, "AI_SLOPING" );
	AI_LEAPING.LinkTo(				scriptObject, "AI_LEAPING" );
	AI_FALLING.LinkTo(				scriptObject, "AI_FALLING" );
	AI_DROPATTACK.LinkTo(			scriptObject, "AI_DROPATTACK" );
	AI_ONWALL.LinkTo(				scriptObject, "AI_ONWALL" );
	idAI::LinkScriptVariables();
}

void idAI_Veloxite::Save( idSaveGame *savefile ) const {
	idAI::Save( savefile );

	savefile->WriteBool( doPostTrans );
	savefile->WriteBool( doTrans );
	savefile->WriteInt( curTrans );
	savefile->WriteInt( numTrans );
	savefile->WriteVec3( transGrav );
	savefile->WriteVec3( destGrav );
	savefile->WriteVec3( upVec );
	savefile->WriteTrace( trace );
	savefile->WriteFloat( nextWallCheck );
	savefile->WriteFloat( maxGraceTime );
	savefile->WriteFloat( debuglevel );
	savefile->WriteVec3( veloxMins );
	savefile->WriteVec3( veloxMaxs );
	savefile->WriteVec3( traceMins );
	savefile->WriteVec3( traceMaxs );
	savefile->WriteBool( AI_ALIGNING ? true : false );
	savefile->WriteBool( AI_WALLING ? true : false );
	savefile->WriteBool( AI_LEDGING ? true : false );
	savefile->WriteBool( AI_SLOPING ? true : false );
	savefile->WriteBool( AI_LEAPING ? true : false );
	savefile->WriteBool( AI_FALLING ? true : false );
	savefile->WriteBool( AI_DROPATTACK ? true : false );
	savefile->WriteBool( AI_ONWALL ? true :  false );
}

void idAI_Veloxite::Restore( idRestoreGame *savefile ) {
	idAI::Restore( savefile );

	savefile->ReadBool(	doPostTrans );
	savefile->ReadBool(	doTrans );
	savefile->ReadInt( curTrans );
	savefile->ReadInt( numTrans );
	savefile->ReadVec3( transGrav );
	savefile->ReadVec3( destGrav );
	savefile->ReadVec3( upVec );
	savefile->ReadTrace( trace );
	savefile->ReadFloat( nextWallCheck );
	savefile->ReadFloat( maxGraceTime );
	savefile->ReadFloat( debuglevel );
	savefile->ReadVec3( veloxMins );
	savefile->ReadVec3( veloxMaxs );
	savefile->ReadVec3( traceMins );
	savefile->ReadVec3( traceMaxs );
	bool b;
	savefile->ReadBool( b ); b ? AI_ALIGNING = true : AI_ALIGNING = false;
	savefile->ReadBool( b ); b ? AI_WALLING = true : AI_WALLING = false;
	savefile->ReadBool( b ); b ? AI_LEDGING = true : AI_LEDGING = false;
	savefile->ReadBool( b ); b ? AI_SLOPING = true : AI_SLOPING = false;
	savefile->ReadBool( b ); b ? AI_LEAPING = true : AI_LEAPING = false;
	savefile->ReadBool( b ); b ? AI_FALLING = true : AI_FALLING = false;
	savefile->ReadBool( b ); b ? AI_DROPATTACK = true : AI_DROPATTACK = false;
	savefile->ReadBool( b ); b ? AI_ONWALL = true : AI_ONWALL = false;

	// Link the script variables back to the scriptobject
	LinkScriptVariables();
}

// ***********************************************************

// Class Method Definitions

// ***********************************************************

void idAI_Veloxite::Spawn( void ) {
	idVec3 veloxSize = spawnArgs.GetVector( "size", "15 15 15" );
	veloxMins	=	-veloxSize / 2;
	veloxMaxs	=	veloxSize / 2;
	traceMins	=	veloxMins;
	traceMaxs	=	traceMaxs;
	transGrav.Zero();
	doTrans		=	false;
	doPostTrans	=	false;
	curTrans	=	0;
	debuglevel	=	1;
	AI_ONWALL	=	onWall();
	LinkScriptVariables();next=0;
	// just to make sure 'trace' is safe
	idVec3 addHeight = physicsObj.GetGravity();
	addHeight.Normalize();
	addHeight *= 24;

	idVec3 A = physicsObj.GetOrigin() + addHeight;
	idVec3 B = A - addHeight * 2;
	gameLocal.clip.TraceBounds( trace, idVec3(), B, idBounds(traceMins, traceMaxs), MASK_MONSTERSOLID, this);
}

void idAI_Veloxite::Think( void ) {
	next=0;
	if ( ( (float) gameLocal.time / 1000 ) > next ) {
		next = ( (float) gameLocal.time / 1000 ) + 1;
	}

	if ( AI_FALLING && !AI_ALIGNING && AI_ONGROUND ) {
		AI_FALLING = false;
	}

	if ( AI_DROPATTACK && !AI_ALIGNING && AI_ONGROUND ) {
		AI_DROPATTACK = false;
	}

	if ( AI_PAIN && onWall() ) {
		getOffSurface( true );
	}

	if ( doTrans ) {
		doSurfaceTransition();
	}

	if ( doPostTrans ) {
		postSurfaceTransition();
	}

	idAI::Think();
}

float idAI_Veloxite::checkSurfaces() {
	if ( checkFallingSideways() ) {
		return VELOX_SURFACECHECK_RATE ; // we're in the air, so we might as well issue a delay
	}

	if (!AI_ONGROUND && disableGravity == true && physicsObj.GetLinearVelocity() == idVec3(0,0,0) ) {
		disableGravity = false;
		getOffSurface(true);
	}

	if (AI_ONGROUND && !AI_FALLING && !AI_ALIGNING && !AI_LEAPING && !AI_DROPATTACK )
	{
		// checkStuckInWall();
		if ( checkSlope() )	{ return VELOX_SURFACECHECK_RATE ; }
		if ( checkLedge() )	{ return VELOX_SURFACECHECK_RATE ; }
		if ( checkWall() )	{ return VELOX_SURFACECHECK_RATE ; }
		if ( AI_ONWALL && checkDropAttack() )	{ return VELOX_SURFACECHECK_RATE ; }
	}

	if ( !AI_ALIGNING ) checkHovering(); // todo: maybe check hovering when all bools are false?

	return VELOX_SURFACECHECK_RATE;
}

bool idAI_Veloxite::checkWall() {
	int asd=0;

	if (( (float) gameLocal.time / 1000 ) < nextWallCheck) {
		return false;
	}

	idVec3 addHeight = physicsObj.GetGravity();
	addHeight.Normalize();
	addHeight = -addHeight*24;

	idVec3	A = physicsObj.GetOrigin() + addHeight;
	idVec3	B = A + ( FacingNormal() * VELOX_MAX_WALL_DIST );
	idVec3	normal = gameLocal.clip.TraceSurfaceNormal(trace, A, B, MASK_MONSTERSOLID, this);

	if ( surfaceType( normal ) == v_wall ) {
		gameLocal.clip.TraceBounds( trace, A, B, idBounds(traceMins, traceMaxs), MASK_MONSTERSOLID, this); // trace point just wouldn't do the trick. dunno why (didn't return an idEntity)
		idEntity *wall = gameLocal.entities[ trace.c.entityNum ];

		if ( wall && wallIsWalkable(wall) ) {
			nextWallCheck = ( (float) gameLocal.time / 1000 ) + VELOX_WALLCHECK_RATE; // to prevent rapid new surface AI_ALIGNING, wait 5 seconds for old surface.
			AI_WALLING=true;
			AI_ALIGNING = true;
			getOnSurface( -normal );
			return true;
		}
	}
	return false;
}


bool idAI_Veloxite::checkDropAttack() {
	idVec3 A = physicsObj.GetOrigin();
	idVec3 B = A + DEFAULT_GRAVITY_NORMAL * 1024;
	gameLocal.clip.TraceBounds( trace, A, B, idBounds(veloxMins, veloxMaxs), MASK_MONSTERSOLID, this);
	idEntity *nmy = gameLocal.entities[ trace.c.entityNum ];

	if ( nmy && nmy->GetName() == enemy.GetEntity()->GetName() ) {
		AI_DROPATTACK = true;
		BeginAttack( "melee_veloxite_fall" );
		SetAnimState( ANIMCHANNEL_TORSO, "Torso_LeapAttack", 4 );
		getOffSurface(true);
	}

	return false;
}

bool idAI_Veloxite::checkLedge() {
	if (AI_FORWARD) {
		idVec3 addHeight = physicsObj.GetGravity();
		addHeight.Normalize();
		addHeight = -addHeight*24; //24 seems to be just enough to penetrate max slopes for ground - at least for velox

		idVec3 myOrigin=physicsObj.GetOrigin();
		idVec3 addForward = FacingNormal() * 24;
		idVec3 A = myOrigin + addHeight;
		idVec3 B = A + addForward;

		idEntity *wall;

		//make sure there isn't a wall right in our way
		idVec3 normal = gameLocal.clip.TraceSurfaceNormal(trace, A, B, MASK_MONSTERSOLID, this);
		if ( normal != idVec3(0,0,0) ) {
			return false;
		}

		// do a trace straight through the floor, a bit of infront of the velox
		A = myOrigin + addHeight + addForward;
		B = myOrigin - addHeight + addForward;
	gameLocal.clip.TraceBounds( trace, A, B, idBounds(traceMins, traceMaxs), MASK_MONSTERSOLID, this);
		wall = gameLocal.entities[ trace.c.entityNum ];

		// if the trace found nothing, then we have a ledge, look for a surface to get on
		if ( !wall ) {
			A = myOrigin - addHeight + addForward;
			B = myOrigin - addHeight - addForward;
			normal = gameLocal.clip.TraceSurfaceNormal(trace, A, B, MASK_MONSTERSOLID, this);
			gameLocal.clip.TraceBounds( trace, A, B, idBounds(traceMins, traceMaxs), MASK_MONSTERSOLID, this);
			wall = gameLocal.entities[ trace.c.entityNum ];
			if ( !wall ) return false;
			if ( wallIsWalkable(wall) ) {
				AI_LEDGING = true;
				AI_ALIGNING = true;
				getOnSurface( -normal );

				return true;

			}
			return false;
		}

	}
	return false;
}

bool idAI_Veloxite::checkFallingSideways( void ) {
	idVec3 vec = physicsObj.GetLinearVelocity();
	float mySpeed = vec.Length();

	if ( !AI_ONGROUND && !AI_ALIGNING && !AI_WALLING && !AI_FALLING && AI_ONWALL && !AI_DROPATTACK && !AI_SLOPING && !AI_LEDGING && !AI_LEAPING && mySpeed > 125.0f) {
		getOffSurface(true);
		return true;
	}
	return false;
}

bool idAI_Veloxite::checkSlope( void ) {
	idVec3 addHeight = physicsObj.GetGravity();
	addHeight.Normalize();
	addHeight = -addHeight* 24; //24 seems to be just enough to penetrate max slopes for ground - at least for velox

	// do a trace straight through the floor
	idVec3 A = physicsObj.GetOrigin() + addHeight;
	idVec3 B = A - addHeight * 2;
	idVec3 normal=gameLocal.clip.TraceSurfaceNormal(trace, A, B, MASK_MONSTERSOLID, this);

	if ( normal != -curNorm() && surfaceType(normal) == v_slope ) { // gravity normal is the opposite of the normal of the plane velox is on
		gameLocal.clip.TraceBounds( trace, A, B, idBounds(traceMins, traceMaxs), MASK_MONSTERSOLID, this); // trace point just wouldn't do the trick. dunno why (didn't return an idEntity)
		idEntity *wall = gameLocal.entities[ trace.c.entityNum ];

		if ( wall && wallIsWalkable( wall ) ) { // make sure they don't climb on stupid things
			AI_SLOPING = true;
			AI_ALIGNING = true;
			getOnSurface( -normal );
			return true;
		}
	}
	return false;
}

bool idAI_Veloxite::checkHovering( void ) {
	if ( AI_ONGROUND ) {
		idVec3 addHeight = physicsObj.GetGravity();
		addHeight.Normalize();
		addHeight *= 24; //24 seems to be just enough to penetrate max slopes for ground - at least for velox

		// do a trace straight through the floor
		idVec3 A = physicsObj.GetOrigin() + addHeight;
		idVec3 B = A - addHeight * 2;
	gameLocal.clip.TraceBounds( trace, A, B, idBounds(traceMins, traceMaxs), MASK_MONSTERSOLID, this); // trace point just wouldn't do the trick. dunno why (didn't return an idEntity)
		idEntity *wall = gameLocal.entities[ trace.c.entityNum ];
	
		if ( wall && !wallIsWalkable( wall ) ) {
			idVec3 vecUp = physicsObj.GetGravity(); vecUp.Normalize();
			idVec3 dir = FacingNormal() + vecUp * 0.5; dir.Normalize();

			physicsObj.SetLinearVelocity( dir * 150);
			return true;
		}
	}
	return false;
}

void idAI_Veloxite::getOffSurface( bool fall ) {
	if ( AI_ONWALL || AI_LEAPING ) {
		// sometimes we might want to keep pre-existing velocity, such as for leapattacks. in such cases, fall is false.
		if (fall) {
			AI_FALLING = true; // prevent checkSurfaces while AI_FALLING. clipping issues otherwise.
			idVec3 thisNorm = physicsObj.GetGravity();
			thisNorm.Normalize();
			thisNorm = -thisNorm;

			// set origin a bit away from the wall to prevent velox from getting on the other side during a drop attack, or getting stuck when falling off a wall.
			physicsObj.SetOrigin( physicsObj.GetOrigin() + thisNorm * 10 );

			physicsObj.SetGravity(thisNorm*100);  // reduce gravity to reduse "wall friction"
			physicsObj.SetLinearVelocity(DEFAULT_GRAVITY * DEFAULT_GRAVITY_NORMAL); // pull to floor
		}

		getOnSurface(DEFAULT_GRAVITY_NORMAL);
	}
}

void idAI_Veloxite::getOnSurface( const idVec3 &newNorm, int numt ) {
	AI_ALIGNING = true;
	float	upMax;  // max height we will lift the veloxite to in the transition (avoid clipping probs)
	destGrav = newNorm * DEFAULT_GRAVITY;

	// PREPARING TO TRANSITION
	// we need to move the velox up, so it doesn't get stuck in a wall when the gravity changes.
	if (!AI_LEAPING) { // if we're AI_LEAPING, don't screw with the origin

		// if AI_LEDGING, we need to go away from the new surface instead
		// if not AI_LEDGING, upVec will point mostly straight up (relative to the velox)
		if (AI_LEDGING) {
			upVec = destGrav; upVec.Normalize(); upVec = -upVec;
        } else {
			upVec = -curNorm();
		}

		// the more perpendicular the wall is to the ground, the more height. from 0 to 90 degrees, increase height. after 90 degrees, decrease height from max. this works for AI_LEDGING, AI_WALLING, and AI_SLOPING
#ifdef _HEXENEOC
		float ang = hxVec3::toAngle(curNorm(), newNorm);
#else
		float ang = curNorm().toAngle(newNorm);
#endif
		if (ang > 90) {
			ang = ang - 90;
			ang = 90 - ang;
		}

		float proportion;
		if (AI_SLOPING) {
			proportion = ang / 180;
		}else{
			proportion = ang / 90;
		}

		// now define the maximum height we will lift the velox
		if (AI_LEDGING) { upMax = 2.0f; }
		else if (AI_SLOPING) { upMax = 0.66f; }
		else { upMax = 1.66f; } // AI_FALLING, AI_WALLING
		
		upMax = upMax;

		// now proportion is a decimal value from 0 to 1. do some magic to get the right height:
		upVec = upVec * (((upMax-1) * proportion) + 1); // multiplying by less than 1 causes upVec to get shorter (bad)

		if ( numt < 1 ) {
			// we transition the gravity change because when you change gravity, the veolx's model aligns instantly. if done gradually, it looks like a natural rotation.
			// the more perpendicular the wall is to the ground, the longer the transition. from 0 to 90 degrees, increase ti after 90 degrees, decrease time from max.
			numTrans = ( ( VELOX_MAX_TRANS_FRAMES - VELOX_MIN_TRANS_FRAMES ) * proportion ) + VELOX_MIN_TRANS_FRAMES;
		} else {
			numTrans = numt;
		}
	}

	if (AI_LEDGING || AI_WALLING) { disableGravity = true; }
	doTrans = true;
	curTrans = 0;
	transGrav = physicsObj.GetGravity();
	doSurfaceTransition();
}

void idAI_Veloxite::doSurfaceTransition( void ) {
	idVec3 midGrav;

	if ( curTrans >= numTrans || AI_LEAPING ) { // no transition for AI_LEAPING, causes too many problems.
		doTrans = false;
		curTrans = 0;
		physicsObj.SetGravity(destGrav); // set the final gravity just incase we missed some in the rounding
		disableGravity = false;
		RunPhysics(); //physicsObj.Evaluate();  // particularly for velocity
		maxGraceTime=( (float) gameLocal.time / 1000 )+0.5;
		doPostTrans = true;
		return;
	}

	// TRANSITION	
	curTrans++;
	// move the Veloxite up:
	physicsObj.SetOrigin(physicsObj.GetOrigin() + upVec); // Z.TODO: clip test with velox model

	// change velox gravity to the new normal:
	midGrav =	transGrav	* ( 1.0f / curTrans) + 
				destGrav	* ( 1.0f / (numTrans+1-curTrans));
	physicsObj.SetGravity(midGrav);

	return;
}

void idAI_Veloxite::postSurfaceTransition( void ) {
	if (!physicsObj.OnGround()
		&& !AI_PAIN
		&& !AI_LEAPING
		&& !(( (float) gameLocal.time / 1000 ) > maxGraceTime)
		&& physicsObj.GetLinearVelocity() != idVec3(0,0,0) ) {
		return;
	}

	doPostTrans = false;
	AI_ALIGNING = false;
	AI_WALLING = false;
	AI_LEDGING = false;
	AI_SLOPING = false;
	AI_DROPATTACK = false;

	idVec3 worldNorm = gameLocal.GetGravity();
	worldNorm.Normalize();

	AI_ONWALL = onWall();

}

/******************************************************************

Script Events

******************************************************************/

void idAI_Veloxite::Event_getOffWall( float fall ) {
	getOffSurface( !(!fall) );
}

void idAI_Veloxite::Event_doSurfaceChecks( void ) {
	idThread::ReturnFloat( checkSurfaces() );
}

void idAI_Veloxite::Event_doneLeaping( void ) {
	AI_LEAPING = false;
}

void idAI_Veloxite::Event_startLeaping( void ) {
	// leaping from wall handled here
//	if ( AI_ONWALL ) {
//		StopMove( MOVE_STATUS_DONE );
//		getOffSurface(false);

//		//get jumpVelocity
//		idVec3 jumpTo = lastVisibleEnemyPos + lastVisibleEnemyEyeOffset;
//		idVec3 jumpVel = jumpTo - physicsObj.GetOrigin();
//		jumpVel *= ;

//		TurnToward( jumpTo );
//		SetAnimState( ANIMCHANNEL_TORSO, "Torso_LeapAttackFromWall", 4 );
//	}

	// leaping from ground handled in script

	AI_LEAPING = true;
}

void idAI_Veloxite::Event_getVeloxJumpVelocity( float speed, float max_height, float channel, const char *animname ) {
	float range;
	idVec3 jumpVelocity;

	if ( AI_ENEMY_IN_FOV ) { //todo: no?
		if ( AI_LEAPING ) {
			goto returnZero;
		}
				
		if ( AI_ONWALL ) {
			if ( !enemy.GetEntity() ) {
				goto returnZero;
			}

			// don't leap if player is much higher than veloxc because he cant be reached
			idVec3 veloxO = physicsObj.GetOrigin();
			idVec3 enemyO = enemy.GetEntity()->GetPhysics()->GetOrigin();
			float zDist = enemyO.z - veloxO.z;
			if ( zDist > VELOX_LEAP_MAXHEIGHT / 2 ) {
				goto returnZero;
			}

			// now check horizontal range (no height considered)
			range = ( idVec3(veloxO.x,veloxO.y,0) - idVec3(enemyO.x,enemyO.y,0) ).Length();
			if ( VELOX_LEAP_RANGE_MIN > range && range > VELOX_LEAP_RANGE_MAX) {
				goto returnZero;
			}

			// trace to the player
			gameLocal.clip.TraceBounds( trace, veloxO, enemyO, idBounds(veloxMins, veloxMaxs), MASK_MONSTERSOLID, this);
			idEntity *traceEnt = gameLocal.entities[ trace.c.entityNum ];
			if ( !traceEnt) {
				goto returnZero;
			}
			if ( traceEnt->spawnArgs.GetString("name") != enemy.GetEntity()->spawnArgs.GetString("name") ) {
				goto returnZero;
			}
			jumpVelocity = enemyO - veloxO;
			// if player is higher, add height to make sure velox will reach him
			if ( zDist > 0 ) { jumpVelocity.z += zDist * 2; }
			jumpVelocity.Normalize();
			jumpVelocity *= VELOX_LEAP_SPEED;
			idThread::ReturnVector( jumpVelocity ); return;
		} else {
			if ( !idAI::CanHitEnemy() ) {
				goto returnZero;
			}
			range = ( enemy.GetEntity()->GetPhysics()->GetOrigin() - physicsObj.GetOrigin() ).Length();
			if ( range < VELOX_LEAP_RANGE_MIN ) {
				goto returnZero;
			}
			// dont check range max, not necessary
			
			idThread::ReturnVector( GetJumpVelocity( PredictEnemyPos( GetAnimLength( (int)channel, animname ) ), VELOX_LEAP_SPEED, VELOX_LEAP_MAXHEIGHT ) ); return;
		}
	}

returnZero:

	idThread::ReturnVector( idVec3(0,0,0) ); return;
}


/******************************************************************

Helper Functions

******************************************************************/

ID_INLINE bool idAI_Veloxite::wallIsWalkable( idEntity *wall ) {
	if ( !wall ) { return false; }
	if (wall->name == "world" || wall->name.Mid(0, 11) == "func_static" || wall->spawnArgs.GetInt("veloxite_walkable") == 1) {
		if (wall->name.Right(4) != "_sky") { // not a skybox
			return true;
		}
	}
	return false;
}

ID_INLINE bool idAI_Veloxite::onWall() {
#ifdef _HEXENEOC
	float ang = hxVec3::toAngle(curNorm(), DEFAULT_GRAVITY_NORMAL);
#else
	float ang = curNorm().toAngle(DEFAULT_GRAVITY_NORMAL);
#endif

	if ( ang > VELOX_MIN_WALL_ANGLE ) {
		return true;
	} else {
		return false;
	}
}

ID_INLINE v_stype idAI_Veloxite::surfaceType( idVec3 normal ) {
	v_stype t = v_none;

	if ( normal == idVec3(0,0,0) ) {
		return t;
	}

#ifdef _HEXENEOC
float ang = hxVec3::toAngle(normal, -curNorm());
#else
float ang = normal.toAngle(-curNorm());
#endif

	if ( VELOX_MIN_SLOPE_ANGLE <= ang && ang < VELOX_MIN_WALL_ANGLE ) {
		t = v_slope;
	}

	if ( VELOX_MIN_WALL_ANGLE <= ang && ang < VELOX_MAX_WALL_ANGLE  ) {
		t = v_wall;
	}

	return t;
}
