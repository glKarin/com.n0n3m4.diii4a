
#pragma hdrstop

#include "Entity.h"
#include "Moveable.h"
#include "Leaf.h"

CLASS_DECLARATION( idMoveable, idEntity_Leaf )

END_CLASS

void idEntity_Leaf::Spawn() {
	liveTime		= spawnArgs.GetFloat( "leaf_liveTime" );	// in seconds. Keep this low to prevent too many leaves in the map at a time. each leaf is an entity! Try to make each leaf live at least until it touches the ground. Some leaves will slide along the ground if they live long enough. default: 8
	moveSpeed		= spawnArgs.GetFloat( "leaf_moveSpeed" );	// from 1-10. How fast the leaves move regardless of wind. Keep this high. default: 10
	spread			= spawnArgs.GetFloat( "leaf_spread" );	// from 1-10. Affects how far apart leafs speread apart when falling. default: 10
	windPower		= spawnArgs.GetFloat( "leaf_windPower" );	// from 1-10. How strong the wind is on the leaves. default: 1
	windDir			= spawnArgs.GetVector("leaf_windDir" );	// Wind direction. default: '0 1 0'
	gravity			= spawnArgs.GetVector("leaf_gravity" );	// Gravity of the leaf. Keep this pretty low, or accept the default. default: '0 0 -20'
	origin			= spawnArgs.GetVector("leaf_origin" );
	ang				= GetAngles();
	dieTime			= gameLocal.time + liveTime * 1000;
	
	if ( moveSpeed == 0 ) {
		moveSpeed = 1;
	}

	if ( origin != idVec3( 0, 0, 0 ) ) {
		SetOrigin( origin );
	}	

	spread = spread / 20; //max spread is 0.5
	
	dir = gameLocal.random.RandomInt( 3 );
	if ( dir < 1 ) { spreadX = spread; }
	else if ( dir < 2 && dir > 1 ) { spreadX = 0; }
	else { spreadX = -spread; }

	dir = gameLocal.random.RandomInt( 3 );
	if ( dir < 1 ) { spreadY = spread; }
	else if ( dir < 2 && dir > 1 ) { spreadY = 0; }
	else { spreadY = -spread; }

	SetAngles( idAngles( gameLocal.random.CRandomFloat()*360, gameLocal.random.CRandomFloat()*360, gameLocal.random.CRandomFloat()*360 ) );
	GetPhysics()->SetLinearVelocity( idVec3( 0, 0, 0 ) );
	GetPhysics()->SetGravity( idVec3( 0, 0, 0 ) ); //gravity - if we set zero gravity, the leaves stop when they hit the ground. dont know why, but it's cool!

	curVel = GetPhysics()->GetLinearVelocity();

	nextAngles = 0;
}

void idEntity_Leaf::Think() {
	idEntity::Think();

	if ( gameLocal.time >= dieTime ) {
		PostEventMS( &EV_Remove, 0.0 );
	}

	if ( gameLocal.random.RandomInt( 100 ) > 33 ) {
		return;
	}

	curDir = GetPhysics()->GetLinearVelocity();
	curDir.Normalize();

	curDir.x = curDir.x + gameLocal.random.RandomFloat() - 0.5 + windDir.x * ( windPower / 14 );
	curDir.y = curDir.y + gameLocal.random.RandomFloat() - 0.5 + windDir.y * ( windPower / 14 );
	curDir.z = curDir.z + gameLocal.random.RandomFloat() - 0.5 + windDir.z * ( windPower / 14 );
	
	// more spread
	curDir.x = curDir.x + spreadX;
	curDir.y = curDir.y + spreadY;

	curVel = curDir * moveSpeed * ( windPower + 1 ) + gravity;
	GetPhysics()->SetLinearVelocity( curVel );

	if ( gameLocal.time > nextAngles ) {
		angles.x = gameLocal.random.CRandomFloat();
		angles.y = gameLocal.random.CRandomFloat();
		angles.z = gameLocal.random.CRandomFloat();
		angles.Normalize();
		angles *= 180 * ( windPower / 10 );
		GetPhysics()->SetAngularVelocity( angles );
		nextAngles = gameLocal.time + 500 + gameLocal.random.RandomFloat() * 3000;
	}
}
