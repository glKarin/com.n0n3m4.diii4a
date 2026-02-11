
#pragma hdrstop

#include "Entity.h"
#ifdef _HEXENEOC
#include "../Misc.h"
#else
#include "Misc.h"
#endif
#include "Tree.h"

CLASS_DECLARATION( idAnimated, idAnimated_Tree )

END_CLASS

void idAnimated_Tree::Spawn() {
	// randomize tree branch positions
	RandomizeJoints();

	// set up swaying
	idVec3 treeDim = GetModelDims();
	normAngle = idAngles( 0, 0, 0 );
	windSpeed = spawnArgs.GetFloat( "wind_speed" );
	maxAngle = RAD2DEG( idMath::ATan( OBJECT_TREE_MAX_SWAY_DIST / treeDim.z ) );
	float useAngle = maxAngle * ( windSpeed / OBJECT_TREE_MAX_WIND_SPEED );
	swayAngle = spawnArgs.GetAngles( "wind_dir" ); swayAngle *= useAngle;
	randSwayRange = useAngle / 8; // an eighth of swayAngle
	swayTime = 3000 - ( ( windSpeed / OBJECT_TREE_MAX_WIND_SPEED ) * 2000 ) + 1000; // 1 < swatTime < 3
	transitions = ( swayTime / 1000 ) * OBJECT_TREE_JOINT_ANGLE_TRANSITIONS_PER_SEC;
	swayDir = 1;
}

void idAnimated_Tree::Think( void ) {
	idAngles to;
	float doSpeed;
	float rndTime;

	idAnimated::Think();
	
	if ( ( gameLocal.time ) < nextSway ) {
		return;
	}

	if ( swayDir == 1 ) {
		swayDir = -1;
		to = swayAngle;
		to.yaw = to.yaw + gameLocal.random.RandomInt(randSwayRange*2) - randSwayRange;
		to.pitch = to.pitch + gameLocal.random.RandomInt(randSwayRange*2) - randSwayRange;
		
		// only a forward sway should have decreased time per transition (wind gusts)
		rndTime = gameLocal.random.RandomFloat() - 0.5;
	} else {
		swayDir = 1;
		float rnd = ( gameLocal.random.RandomFloat() * windSpeed / 2 ) / OBJECT_TREE_MAX_WIND_SPEED ; // the faster the wind, the less likely to make a full backsway (wind gusts)
		to = swayAngle * rnd;
		
		rndTime = gameLocal.random.RandomFloat() / 32;
	}
	
	doSpeed = swayTime + ( rndTime * 1000 );
	
	for (int i=0; i<OBJECT_TREE_NUM_JOINTS; i++) {
		TransitionJointAngle( (jointHandle_t) i , (jointModTransform_t) 1, to, curAngle, doSpeed, transitions );
	}

	curAngle = to;
	nextSway = gameLocal.time + doSpeed + OBJECT_TREE_PAUSE_TIME;
}

void idAnimated_Tree::RandomizeJoints( void ) {
	idAngles curAng;
	idAngles rndAng;

	rndAng.yaw = gameLocal.random.RandomInt(OBJECT_TREE_MAX_RANDOM_JOINT_ANGLE*2) - OBJECT_TREE_MAX_RANDOM_JOINT_ANGLE;
	rndAng.pitch = gameLocal.random.RandomInt(OBJECT_TREE_MAX_RANDOM_JOINT_ANGLE*2) - OBJECT_TREE_MAX_RANDOM_JOINT_ANGLE;
	rndAng.roll = gameLocal.random.RandomInt(OBJECT_TREE_MAX_RANDOM_JOINT_ANGLE*2) - OBJECT_TREE_MAX_RANDOM_JOINT_ANGLE;

	for (int i=0; i<OBJECT_TREE_NUM_JOINTS; i++) {
		curAng = this->GetJointAngle( (jointHandle_t) i );
		SetJointAngle( (jointHandle_t) i , (jointModTransform_t) 1, rndAng + curAng );
	}
}
