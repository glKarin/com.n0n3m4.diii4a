/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../qcommon/q_shared.h"
#ifdef GAME
#include "g_local.h"
#else
#include "bg_public.h"
//#include "../cgame/cg_local.h"
#endif
#include "bg_local.h"

float CP_CURRENT_GRAVITY;

// not actually used now, use cvars instead
float CP_SPRING_STRENGTH = 110 * (CP_FRAME_MASS / 350.0f) * CP_GRAVITY; // 110
float CP_SHOCK_STRENGTH = 13 * CP_GRAVITY; // 12
float CP_SWAYBAR_STRENGTH = 21 * CP_GRAVITY; // 20 * grav

float CP_M_2_QU = CP_FT_2_QU / CP_FT_2_M; // 35.66
float CP_WR_STRENGTH = 2400.0f * CP_WHEEL_MASS;
float CP_WR_DAMP_STRENGTH = 140.0f * CP_WHEEL_MASS;


static int	numTraces;

/*
================================================================================
PM_DebugDynamics
================================================================================
*/
void PM_DebugDynamics( carBody_t *body, carPoint_t *points ){
	Com_Printf("\n");
	Com_Printf("PM_DebugDynamics: point ORIGIN - %.3f, %.3f, %.3f\n", points[pm->pDebug-1].r[0], points[pm->pDebug-1].r[1], points[pm->pDebug-1].r[2]);
	Com_Printf("PM_DebugDynamics: point VELOCITY - %.3f, %.3f, %.3f\n", points[pm->pDebug-1].v[0], points[pm->pDebug-1].v[1], points[pm->pDebug-1].v[2]);

	Com_Printf("PM_DebugDynamics: point ONGROUND - %i\n", points[pm->pDebug-1].onGround);
	Com_Printf("PM_DebugDynamics: point NORMAL 1- %.3f, %.3f, %.3f\n", points[pm->pDebug-1].normals[0][0], points[pm->pDebug-1].normals[0][1], points[pm->pDebug-1].normals[0][2]);
	Com_Printf("PM_DebugDynamics: point NORMAL 2- %.3f, %.3f, %.3f\n", points[pm->pDebug-1].normals[1][0], points[pm->pDebug-1].normals[1][1], points[pm->pDebug-1].normals[1][2]);
	Com_Printf("PM_DebugDynamics: point NORMAL 3- %.3f, %.3f, %.3f\n", points[pm->pDebug-1].normals[2][0], points[pm->pDebug-1].normals[2][1], points[pm->pDebug-1].normals[2][2]);
//	Com_Printf("PM_DebugDynamics: point MASS - %.3f\n", points[pm->pDebug-1].mass);

	Com_Printf("PM_DebugDynamics: body ORIGIN - %.3f, %.3f, %.3f\n", body->r[0], body->r[1], body->r[2]);
	Com_Printf("PM_DebugDynamics: body VELOCITY - %.3f, %.3f, %.3f\n", body->v[0], body->v[1], body->v[2]);
	Com_Printf("PM_DebugDynamics: body ANG VELOCITY - %.3f, %.3f, %.3f\n", body->w[0], body->w[1], body->w[2]);

//	Com_Printf("PM_DebugDynamics: body COM - %.3f, %.3f, %.3f\n", body->CoM[0], body->CoM[1], body->CoM[2]);
//	Com_Printf("PM_DebugDynamics: body MASS - %.3f\n", body->mass);

	Com_Printf("Direction vectors ----------------------------------------\n");

	Com_Printf("PM_DebugDynamics: body FORWARD - %.3f, %.3f, %.3f\n", body->forward[0], body->forward[1], body->forward[2]);
	Com_Printf("PM_DebugDynamics: body RIGHT - %.3f, %.3f, %.3f\n", body->right[0], body->right[1], body->right[2]);
	Com_Printf("PM_DebugDynamics: body UP - %.3f, %.3f, %.3f\n", body->up[0], body->up[1], body->up[2]);
	Com_Printf("\n");
}


/*
================================================================================
PM_DebugForces
================================================================================
*/
void PM_DebugForces( carBody_t *body, carPoint_t *points ){
	Com_Printf("\n");
	Com_Printf("PM_DebugForces: point GRAVITY - %.3f, %.3f, %.3f\n", points[pm->pDebug-1].forces[GRAVITY][0], points[pm->pDebug-1].forces[GRAVITY][1], points[pm->pDebug-1].forces[GRAVITY][2]);
	Com_Printf("PM_DebugForces: point NORMAL - %.3f, %.3f, %.3f\n", points[pm->pDebug-1].forces[NORMAL][0], points[pm->pDebug-1].forces[NORMAL][1], points[pm->pDebug-1].forces[NORMAL][2]);
	Com_Printf("PM_DebugForces: point SHOCK - %.3f, %.3f, %.3f\n", points[pm->pDebug-1].forces[SHOCK][0], points[pm->pDebug-1].forces[SHOCK][1], points[pm->pDebug-1].forces[SHOCK][2]);
	Com_Printf("PM_DebugForces: point SPRING - %.3f, %.3f, %.3f\n", points[pm->pDebug-1].forces[SPRING][0], points[pm->pDebug-1].forces[SPRING][1], points[pm->pDebug-1].forces[SPRING][2]);
	Com_Printf("PM_DebugForces: point ROAD - %.3f, %.3f, %.3f\n", points[pm->pDebug-1].forces[ROAD][0], points[pm->pDebug-1].forces[ROAD][1], points[pm->pDebug-1].forces[ROAD][2]);
	Com_Printf("PM_DebugForces: point INTERNAL - %.3f, %.3f, %.3f\n", points[pm->pDebug-1].forces[INTERNAL][0], points[pm->pDebug-1].forces[INTERNAL][1], points[pm->pDebug-1].forces[INTERNAL][2]);
	Com_Printf("PM_DebugForces: point AIR_FRICTION - %.3f, %.3f, %.3f\n", points[pm->pDebug-1].forces[AIR_FRICTION][0], points[pm->pDebug-1].forces[AIR_FRICTION][1], points[pm->pDebug-1].forces[AIR_FRICTION][2]);

	Com_Printf("PM_DebugForces: point NETFORCE - %.3f, %.3f, %.3f\n", points[pm->pDebug-1].netForce[0], points[pm->pDebug-1].netForce[1], points[pm->pDebug-1].netForce[2]);

	Com_Printf("PM_DebugForces: body netForce - %.3f, %.3f, %.3f\n", body->netForce[0], body->netForce[1], body->netForce[2]);
	Com_Printf("PM_DebugForces: body netMoment - %.3f, %.3f, %.3f\n", body->netMoment[0], body->netMoment[1], body->netMoment[2]);
	Com_Printf("\n");
}


/*
================================================================================
PM_CopyTargetToSource
================================================================================
*/
static void PM_CopyTargetToSource( carBody_t *tBody, carBody_t *sBody, carPoint_t *tPoints, carPoint_t *sPoints){
	memcpy(sBody, tBody, sizeof(carBody_t));
	memcpy(sPoints, tPoints, NUM_CAR_POINTS * sizeof(carPoint_t));
}


/*
===================
PM_SetCoM
===================
*/
void PM_SetCoM( carBody_t *body, carPoint_t *points ){
	vec3_t	temp, origin;
	//vec3_t delta;
	float	totalMass, length;
	int		i;

	VectorClear(temp);
	totalMass = CP_CAR_MASS;

	for (i = FIRST_FRAME_POINT; i < LAST_FRAME_POINT; i++){
		VectorMA(temp, points[i].mass, points[i].r, temp);
//		totalMass += points[i].mass;
	}

	for (i = 0; i < FIRST_FRAME_POINT; i++){
//		VectorSubtract( points[i+4].r, points[i].r, delta );
//		length = DotProduct( body->up, delta );
		length = body->curSpringLengths[i];

		if ( length < CP_SPRING_MINLEN )
		{
			length = CP_SPRING_MINLEN;
			VectorMA( points[i+4].r, length, body->up, origin );
			VectorMA( temp, points[i].mass, origin, temp );
		}
		else if ( length > CP_SPRING_MAXLEN )
		{
			length = CP_SPRING_MAXLEN;
			VectorMA( points[i+4].r, length, body->up, origin );
			VectorMA( temp, points[i].mass, origin, temp );
		}
		else
			VectorMA( temp, points[i].mass, points[i].r, temp );

//		totalMass += points[i].mass;
	}

	VectorScale( temp, 1.0f / totalMass, body->CoM );
}


/*
===========================================================================================
										POINT FUNCTIONS
===========================================================================================
*/


/*
================================================================================
PM_ClearCarForces
================================================================================
*/
static void PM_ClearCarForces( carBody_t *body, carPoint_t *points){
	int		i, j;

	VectorClear( body->netForce );
	VectorClear( body->netMoment);

	for ( i = 0; i < NUM_CAR_POINTS; i++ ){
		VectorClear( points[i].netForce );
		points[i].netMoment = 0;
		for ( j = 0; j < NUM_CAR_FORCES; j++ ){
			VectorClear( points[i].forces[j] );
		}
	}
}


/*
===================
PM_CalculateNetForce
===================
*/
void PM_CalculateNetForce( carPoint_t *point, int pointIndex ){
	int		i;
	float	dot;
//	vec3_t	vecForce, dV;

	VectorClear( point->netForce );

	for (i = 0; i < NUM_CAR_FORCES; i++){
		if (i == NORMAL) continue;
/*
		if (VectorLength(point->forces[i]) >= 1<<23){
			Com_Printf("PM_CalculateNetForce: Car point force %d is greater than %d\n", i, 1<<23);
			Com_Printf("PM_CalculateNetForce: Car point force is %f\n", VectorLength(point->forces[i]));

			VectorNormalize(point->forces[i]);
			VectorScale(point->forces[i], 1<<23, point->forces[i]);
		}
*/
		if ( VectorNAN( point->forces[i] ) )
		{
			Com_Printf( "Force %i on point %i was NAN\n", i, pointIndex );
			continue;
		}

		VectorAdd( point->netForce, point->forces[i], point->netForce );
	}

	VectorClear( point->forces[NORMAL] );
//	VectorClear( vecForce );

	// calculate normal force
	if ( point->onGround )
	{
		for ( i = 0; i < 3; i++ )
		{
			if ( VectorLengthSquared( point->normals[i] ) < 10e-2 ) continue;

			// calculate new velocity of wheel after collision
			dot = DotProduct( point->normals[i], point->v );
			if ( dot < -10.0f )
			{
//				VectorScale(point->normals[i], -(1 + point->elasticity) * dot, dV);
//				VectorAdd(point->v, dV, point->v);
				VectorMA( point->v, -(1.0f + point->elasticity) * dot, point->normals[i], point->v );
			}
			else if ( dot < 1.0f )
			{
				// going slow enough so just stop it so it doesnt vibrate
//				VectorScale(point->normals[i], -dot, dV);
//				VectorAdd(point->v, dV, point->v);
				VectorMA( point->v, -dot, point->normals[i], point->v );
			}

			// add normal force to balance the other forces
			dot = OVERCLIP * DotProduct( point->normals[i], point->netForce );
			if (dot < 0)
			{
//				VectorScale(point->normals[i], -dot, vecForce);
//				VectorAdd(point->forces[NORMAL], vecForce, point->forces[NORMAL]);
				VectorMA( point->forces[NORMAL], -dot, point->normals[i], point->forces[NORMAL] );
			}
		}
	}

/*
	if (VectorLength(point->forces[NORMAL]) >= 1<<23) {
		Com_Printf("PM_CalculateNetForce: Car point normal force is greater than %d\n", 1<<23);

		VectorNormalize(point->forces[NORMAL]);
		VectorScale(point->forces[NORMAL], 1<<23, point->forces[NORMAL]);
	}
*/
	VectorAdd( point->netForce, point->forces[NORMAL], point->netForce );
}


/*
===================
PM_AccelerateAndMove
===================
*/
static void PM_AccelerateAndMove( car_t *car, carPoint_t *source, carPoint_t *target, float time )
{
	float	alpha;
	vec3_t	vec, avgAccel;
//	vec3_t	avgVel;

	// kill car if forces too high
	if ( VectorNAN( source->netForce ) )
	{
		Com_Printf( "Blowing up car because of car point force\n" );
		pm->damage.damage = 32000;
		pm->damage.dflags = DAMAGE_NO_PROTECTION;
		pm->damage.otherEnt = -1;
		pm->damage.mod = MOD_HIGH_FORCES;
		return;
	}

	// using const acceleration

	// NOTE: this method is really not right but it works so oh well.

	// target has last netForce still in it
	VectorScale( source->netForce, 0.4f / source->mass, avgAccel );
	VectorMA( source->v, time, avgAccel, target->v );

	VectorAdd( source->v, target->v, vec );
	VectorMA( source->r, time / 2, vec, target->r );

	// UPDATE: use a moment of inertia different than .5*mr^2?
	alpha = (source->netMoment + target->netMoment) / (0.75f * source->mass * WHEEL_RADIUS * WHEEL_RADIUS);
	target->w = source->w + (time / 6.0f) * alpha;

	if (source->w < 0 && target->w > 0)
	{
		target->w = 0;
		target->netMoment = 0;
	}
	if (source->w > 0 && target->w < 0)
	{
		target->w = 0;
		target->netMoment = 0;
	}
	
	// wheels are trying to turn in the wrong direction..
	// FIXME: use force in opposite diretion instead of just
	// stopping the wheels from turning.
	// Play transmission grinding sound if the force is too big

	// PHYSICS TEMP
	if ( car->gear > 0 && target->w > -1.5f )
	{
//		Com_Printf("You're screwing up your transmission %f\n", target->w);
		target->w = 0;
	}
	else if ( car->gear < 0 && target->w < 1.5f )
	{
//		Com_Printf("You're screwing up your transmission %f\n", target->w);
		target->w = 0;
	}

	target->slipping = source->slipping;

	// copy netForce and netMoment to target so we can use it during the AccelerateandMove next frame
	VectorCopy( source->netForce, target->netForce );
	target->netMoment = source->netMoment;
}


/*
===========================================================================================
									INITIALIZATION FUNCTIONS
===========================================================================================
*/


/*
===================
PM_UpdateFrameVelocities
===================
*/
static void PM_UpdateFrameVelocities( carBody_t *body, carPoint_t *points )
{
	vec3_t	arm;
	vec3_t	cross;
	int		i;

	for ( i = FIRST_FRAME_POINT; i < NUM_CAR_POINTS; i++ )
	{
		VectorSubtract( points[i].r, body->CoM, arm );
		CrossProduct( body->w, arm, cross );
		VectorAdd( body->v, cross, points[i].v );
	}
}


/*
===================
PM_InitializeFrame
===================
*/
static void PM_InitializeFrame( carBody_t *body, carPoint_t *point, float f, float r, float u )
{
	vec3_t	arm;
	vec3_t	cross;

	VectorScale( body->forward, f * WHEEL_FORWARD, arm );
	VectorMA( arm, r * WHEEL_RIGHT, body->right, arm );
	VectorMA( arm, -u * WHEEL_UP, body->up, arm );
	VectorAdd( body->r, arm, point->r );

	CrossProduct( body->w, arm, cross );
	VectorAdd( body->v, cross, point->v );

	VectorClear( point->normals[0] );
	VectorClear( point->normals[1] );
	VectorClear( point->normals[2] );
	
	point->onGround = qfalse;
}


/*
===================
PM_InitializeWheel
===================
*/
static void PM_InitializeWheel( carBody_t *body, carPoint_t *points, int i, int f, int r )
{
	vec3_t	arm;
	vec3_t	cross;

	VectorScale( body->forward, f * WHEEL_FORWARD, arm );
	VectorMA( arm, r * WHEEL_RIGHT, body->right, arm );
	VectorMA( arm, WHEEL_UP, body->up, arm );
	VectorAdd( body->r, arm, points[i].r );

	CrossProduct( body->w, arm, cross );
	VectorAdd( body->v, cross, points[i].v );

	VectorClear( points[i].normals[0] );
	VectorClear( points[i].normals[1] );
	VectorClear( points[i].normals[2] );

	// UPDATE: added for client prediction, should just leave normals instead?
	points[i].onGround = qfalse;

	// recalculate spring length
	VectorSubtract( points[i+4].r, points[i].r, arm );
	body->curSpringLengths[i] = DotProduct( arm, body->up );
}


/*
===================
PM_InitializeVehicle
===================
*/
void PM_InitializeVehicle( car_t *car, vec3_t origin, vec3_t angles, vec3_t velocity ){
	float	m[3][3];
	int		i;
	float	halfWidth, halfLength, halfHeight;
	float	forwardScale, rightScale, upScale;

	// UPDATE: use memset?

	VectorCopy(origin, car->sBody.r);
	AnglesToOrientation(angles, car->sBody.t);
//	AnglesToQuaternion(angles, car->sBody.q);
	OrientationToVectors(car->sBody.t, car->sBody.forward, car->sBody.right, car->sBody.up);
	VectorCopy(velocity, car->sBody.v);
	VectorClear(car->sBody.w);
	VectorClear(car->sBody.L);

	// set locations and velocities of frame points
	PM_InitializeFrame(&car->sBody, &car->sPoints[FL_FRAME],  1.0f, -1.0f, 0.0f);
	PM_InitializeFrame(&car->sBody, &car->sPoints[FR_FRAME],  1.0f,  1.0f, 0.0f);
	PM_InitializeFrame(&car->sBody, &car->sPoints[RL_FRAME], -1.0f, -1.0f, 0.0f);
	PM_InitializeFrame(&car->sBody, &car->sPoints[RR_FRAME], -1.0f,  1.0f, 0.0f);

	// body collision points
	forwardScale = ((CAR_LENGTH/2.0f) - BODY_RADIUS) / WHEEL_FORWARD;
	rightScale = ((CAR_WIDTH/2.0f) - BODY_RADIUS) / WHEEL_RIGHT;
	upScale = (3.0f) / -WHEEL_UP;
	PM_InitializeFrame(&car->sBody, &car->sPoints[FL_BODY],  forwardScale, -rightScale, upScale);
	PM_InitializeFrame(&car->sBody, &car->sPoints[FR_BODY],  forwardScale,  rightScale, upScale);
	PM_InitializeFrame(&car->sBody, &car->sPoints[ML_BODY],  0.0f, -rightScale, upScale);
	PM_InitializeFrame(&car->sBody, &car->sPoints[MR_BODY],  0.0f,  rightScale, upScale);
	PM_InitializeFrame(&car->sBody, &car->sPoints[RL_BODY], -forwardScale, -rightScale, upScale);
	PM_InitializeFrame(&car->sBody, &car->sPoints[RR_BODY], -forwardScale,  rightScale, upScale);

	PM_InitializeFrame(&car->sBody, &car->sPoints[ML_ROOF],  0.0f, -rightScale, 0.6f);
	PM_InitializeFrame(&car->sBody, &car->sPoints[MR_ROOF],  0.0f,  rightScale, 0.6f);

	// set locations and velocities of wheel points
	PM_InitializeWheel(&car->sBody, car->sPoints, FL_WHEEL,  1.0f, -1.0f);
	PM_InitializeWheel(&car->sBody, car->sPoints, FR_WHEEL,  1.0f,  1.0f);
	PM_InitializeWheel(&car->sBody, car->sPoints, RL_WHEEL, -1.0f, -1.0f);
	PM_InitializeWheel(&car->sBody, car->sPoints, RR_WHEEL, -1.0f,  1.0f);

	car->sBody.mass = 4 * CP_FRAME_MASS;
	car->tBody.mass = 4 * CP_FRAME_MASS;

	for (i = 0; i < FIRST_FRAME_POINT; i++){
		car->sPoints[i].mass = CP_WHEEL_MASS;
		car->sPoints[i].elasticity = CP_WHEEL_ELASTICITY;
		car->sPoints[i].scof = CP_SCOF;
		car->sPoints[i].kcof = CP_KCOF;
		car->sPoints[i].slipping = qfalse;
		car->sPoints[i].onGround = qfalse;
		car->sPoints[i].onGroundTime = 0;
		car->sPoints[i].radius = WHEEL_RADIUS;
		car->tPoints[i].mass = CP_WHEEL_MASS;
		car->tPoints[i].elasticity = CP_WHEEL_ELASTICITY;
		car->tPoints[i].scof = CP_SCOF;
		car->tPoints[i].kcof = CP_KCOF;
		car->tPoints[i].slipping = qfalse;
		car->tPoints[i].onGround = qfalse;
		car->tPoints[i].onGroundTime = 0;
		car->tPoints[i].radius = WHEEL_RADIUS;
	}

	for (; i < NUM_CAR_POINTS; i++){
		if (i < LAST_FRAME_POINT){
			if( i < 6 )
			{
				car->sPoints[i].mass = 0.5f * car->sBody.mass * pm->car_frontweight_dist;
				car->tPoints[i].mass = 0.5f * car->sBody.mass * pm->car_frontweight_dist;
			}
			else
			{
				car->sPoints[i].mass = 0.5f * car->sBody.mass * (1.0f - pm->car_frontweight_dist);
				car->tPoints[i].mass = 0.5f * car->sBody.mass * (1.0f - pm->car_frontweight_dist);
			}
		}
		else {
			car->sPoints[i].mass = 0.0f;
			car->tPoints[i].mass = 0.0f;
		}
		car->sPoints[i].scof = CP_SCOF;
		car->sPoints[i].kcof = CP_KCOF;
		car->sPoints[i].elasticity = pm->car_body_elasticity;
		car->sPoints[i].slipping = qfalse;
		car->sPoints[i].onGround = qfalse;
		car->sPoints[i].onGroundTime = 0;
		car->sPoints[i].radius = BODY_RADIUS;
		car->tPoints[i].scof = CP_SCOF;
		car->tPoints[i].kcof = CP_KCOF;
		car->tPoints[i].elasticity = pm->car_body_elasticity;
		car->tPoints[i].slipping = qfalse;
		car->tPoints[i].onGround = qfalse;
		car->tPoints[i].onGroundTime = 0;
		car->tPoints[i].radius = BODY_RADIUS;
	}

	car->sBody.elasticity = car->sPoints[4].elasticity;
	car->tBody.elasticity = car->sPoints[4].elasticity;

	PM_SetCoM(&car->sBody, car->sPoints);

	halfWidth = CAR_WIDTH / 2.0f;
	halfLength = CAR_LENGTH / 2.0f;
	halfHeight = CAR_HEIGHT / 2.0f;

	car->inverseBodyInertiaTensor[0][0] = pm->car_IT_xScale * 3.0f / (car->sBody.mass * (halfLength * halfLength + halfHeight * halfHeight));
	car->inverseBodyInertiaTensor[1][1] = pm->car_IT_yScale * 3.0f / (car->sBody.mass * (halfWidth * halfWidth + halfHeight * halfHeight));
	car->inverseBodyInertiaTensor[2][2] = pm->car_IT_zScale * 3.0f / (car->sBody.mass * (halfWidth * halfWidth + halfLength * halfLength));

	car->springStrength = CP_SPRING_STRENGTH;
	car->springMaxLength = CP_SPRING_MAXLEN;
	car->springMinLength = CP_SPRING_MINLEN;
//	car->shockStrength = CP_SHOCK_STRENGTH;

	car->gear = 1;
	car->rpm = CP_RPM_MIN;

//	car->aCOF = CP_AIR_COF;
//	car->dfCOF = CP_FRAC_TO_DF;
//	car->ewCOF = CP_ENGINE_TIRE_COF;

	MatrixMultiply(car->sBody.t, car->inverseBodyInertiaTensor, car->sBody.inverseWorldInertiaTensor);
	MatrixTranspose(car->sBody.t, m);
	MatrixMultiply(car->sBody.inverseWorldInertiaTensor, m, car->sBody.inverseWorldInertiaTensor);

	PM_ClearCarForces(&car->sBody, car->sPoints);
	PM_ClearCarForces(&car->tBody, car->tPoints);

	PM_CopyTargetToSource(&car->sBody, &car->tBody, car->sPoints, car->tPoints);
}


/*
===================
PM_SetFluidDensity
===================
*/
static void PM_SetFluidDensity(carPoint_t *points, int i){
	vec3_t	dest;
	int		cont, level, type;

	// check waterlevel
	level = 0;
	type = 0;

	VectorCopy( points[i].r, dest );
	dest[2] = points[i].r[2] - (points[i].radius - 0.5f);
	cont = pm->pointcontents( dest, pm->ps->clientNum );
	if ( cont & MASK_WATER ){ // bottom of point is in the water
		level++;
		type = cont;
	}

	dest[2] = points[i].r[2];
	cont = pm->pointcontents (dest, pm->ps->clientNum );
	if ( cont & MASK_WATER ){ // middle of point is in the water
		level++;
		type = cont;
	}

	dest[2] = points[i].r[2] + (points[i].radius - 0.5f);
	cont = pm->pointcontents (dest, pm->ps->clientNum );
	if ( cont & MASK_WATER ) { // top of point is in the water
		level++;
		type = cont;
	}

	if ( level )
	{
		if (type & CONTENTS_WATER){
			points[i].fluidDensity = (level * CP_WATER_DENSITY + (3.0f - level) * CP_AIR_DENSITY) / 3.0f;
		}
		else if (type & CONTENTS_LAVA){
			points[i].fluidDensity = (level * CP_LAVA_DENSITY + (3.0f - level) * CP_AIR_DENSITY) / 3.0f;
		}
		else if (type & CONTENTS_SLIME){
			points[i].fluidDensity = (level * CP_SLIME_DENSITY + (3.0f - level) * CP_AIR_DENSITY) / 3.0f;
		}
		else{
			points[i].fluidDensity = CP_AIR_DENSITY;
		}

//		Com_Printf("PM_SetFluidDensity: level %i, density %.3f\n", level, points[i].fluidDensity);
	}
	else
	{
		points[i].fluidDensity = CP_AIR_DENSITY;
	}
}


/*
===================
PM_CheckSurfaceFlags
===================
*/
static void PM_CheckSurfaceFlags( trace_t *trace, carPoint_t *point ){

	// TODO: include rolling friction... harder to roll tires in sand or mud

	if( pm->frictionFunc( point, &point->scof, &point->kcof ) )
	{
		point->kcof *= pm->car_friction_scale;
		point->scof *= pm->car_friction_scale;

		return;
	}
	else if (trace->surfaceFlags & SURF_SLICK){
		point->kcof = CP_ICE_KCOF;
		point->scof = CP_ICE_SCOF;
	}
	else if (trace->surfaceFlags & SURF_ICE) {
	    point->kcof = CP_ICE_KCOF;
	    point->scof = CP_ICE_SCOF;
	}
	else if (trace->surfaceFlags & SURF_GRASS){
		point->kcof = CP_GRASS_KCOF;
		point->scof = CP_GRASS_SCOF;
	}
	else if (trace->surfaceFlags & SURF_DUST){
		point->kcof = CP_DIRT_KCOF;
		point->scof = CP_DIRT_SCOF;
	}
    else if (trace->surfaceFlags & SURF_SAND){
        point->kcof = CP_SAND_KCOF;
        point->scof = CP_SAND_SCOF;
    }
	else if (trace->surfaceFlags & SURF_SNOW){
		point->kcof = CP_SNOW_KCOF;
		point->scof = CP_SNOW_SCOF;
	}
	else if (trace->surfaceFlags & SURF_GRAVEL) {
        point->kcof = CP_GRAVEL_KCOF;
        point->scof = CP_GRAVEL_SCOF;
   }
    else if (trace->surfaceFlags & SURF_DIRT) {
        point->kcof = CP_DIRT_KCOF;
        point->scof = CP_DIRT_SCOF;
   }
	else {
		point->kcof = CP_KCOF;
		point->scof = CP_SCOF;
	}

	if (trace->surfaceFlags & SURF_WET){
		point->kcof *= CP_WET_SCALE;
		point->scof *= CP_WET_SCALE;
	}

	    point->kcof *= pm->car_friction_scale;
	    point->scof *= pm->car_friction_scale;
}


/*
================================================================================
PM_ApplyForce

  This function is largely based on the physics articles and 
  source from Chris Hecker: http://www.d6.com/users/checker/dynamics.htm
================================================================================
*/
void PM_ApplyForce( carBody_t *body, vec3_t force, vec3_t at ){
	vec3_t	arm, moment;

	VectorSubtract( at, body->CoM, arm );

	VectorAdd( body->netForce, force, body->netForce );

	CrossProduct( arm, force, moment);
	VectorAdd( body->netMoment, moment, body->netMoment );
}


/*
================================================================================
PM_ApplyCollision

  Rigid body <-> world collision function.

  
  This function is largely based on the physics articles and 
  source from Chris Hecker: http://www.d6.com/users/checker/dynamics.htm

Given: rigid body, impact origin, impact normal, and elasticity
Find:  the new linear and angular velocities as a result of the impact.
================================================================================
*/
static float PM_ApplyCollision( carBody_t *body, carPoint_t *points, vec3_t at, vec3_t normal, float elasticity ){
	vec3_t	arm;
	vec3_t	vP1;
	vec3_t	impulse, impulseMoment;
	vec3_t	delta, cross, cross2;
	float	impulseNum, oppositeImpulseNum, impulseDen, dot;
	int		i;

	VectorSubtract(at, body->CoM, arm);
//	VectorSubtract(at, body->r, arm);

	if (pm->pDebug){
		Com_Printf("PM_ApplyCollision: arm %0.3f, %0.3f, %0.3f\n", arm[0], arm[1], arm[2]);
		Com_Printf("PM_ApplyCollision: normal %0.3f, %0.3f, %0.3f\n", normal[0], normal[1], normal[2]);
	}

	CrossProduct(body->w, arm, cross);
	VectorAdd(body->v, cross, vP1);

	// added from collision
	VectorClear(impulse);
	dot = DotProduct(normal, vP1);
	if ( dot < 0 ){
		impulseNum = -(1.0f + elasticity) * dot;
		oppositeImpulseNum = -(1.0f - elasticity) * dot;

		CrossProduct(arm, normal, cross);
		VectorRotate(cross, body->inverseWorldInertiaTensor, cross2);
		CrossProduct(cross2, arm, cross);
//		Com_Printf( "oneOverMass %f, cross.normal %f\n", 1.0f / body->mass, DotProduct(cross, normal) );
		impulseDen = 1.0f / body->mass + DotProduct(cross, normal);

		VectorScale(normal, impulseNum / impulseDen, impulse);
	}
	else {
		// not hitting surface
//		Com_Printf( "PM_ApplyCollision: not hitting surface, %f\n", dot );
		return 0.0f;
	}

	// apply impulse to primary quantities
	CrossProduct(arm, impulse, impulseMoment);
	VectorScale( impulse, 1.0 / body->mass, impulse );
	VectorAdd(body->v, impulse, body->v);
	VectorAdd(body->L, impulseMoment, body->L);
    
    // compute affected auxiliary quantities
	VectorRotate(body->L, body->inverseWorldInertiaTensor, body->w);

	// temp to help wheel movement when hitting walls
	// FIXME: is this still needed?
	VectorMA(impulse, -DotProduct(impulse, body->up), body->up, delta);
	for (i = 0; i < FIRST_FRAME_POINT; i++){
		VectorAdd(points[i].v, delta, points[i].v);
	}

	PM_UpdateFrameVelocities(body, points);

	if ( fabs(oppositeImpulseNum / impulseDen) > 5000.0f )
		return fabs(oppositeImpulseNum / impulseDen);
	else
		return 0;
}


#if 0
/*
================================================================================
PM_ApplyBodyBodyCollision

  This function is a hacked up version of PM_ApplyCollision to try to simulate
  the collision between two cars.

Given: 2 rigid bodies, impact origin, impact normal, and elasticity
Find:  the new linear and angular velocities of the two objects as a result of the impact.
================================================================================
*/
static float PM_ApplyBodyBodyCollision( carBody_t *body1, carPoint_t *points1, carBody_t *body2, carPoint_t *points2, vec3_t at, vec3_t normal, float elasticity ){
	//vec3_t	arm1, arm2;
	vec3_t	vP1, vP2;
	//vec3_t	impulse, impulseMoment;
	//vec3_t	cross, cross2;
	vec3_t	diff1, diff2, delta;
	//float	impulseNum, oppositeImpulseNum, impulseDen;
	//float	totalMass;
	int		i;

/*
	totalMass = body1->mass + body2->mass;

	VectorSubtract(at, body1->CoM, arm1);
	VectorSubtract(at, body2->CoM, arm2);

	if (pm->pDebug){
		Com_Printf("PM_ApplyBodyBodyCollision: arm1 %0.3f, %0.3f, %0.3f\n", arm1[0], arm1[1], arm1[2]);
		Com_Printf("PM_ApplyBodyBodyCollision: arm2 %0.3f, %0.3f, %0.3f\n", arm2[0], arm2[1], arm2[2]);
		Com_Printf("PM_ApplyBodyBodyCollision: normal %0.3f, %0.3f, %0.3f\n", normal[0], normal[1], normal[2]);
	}

	CrossProduct(body1->w, arm1, cross);
	VectorAdd(body1->v, cross, vP1);

	CrossProduct(body2->w, arm2, cross);
	VectorAdd(body2->v, cross, vP2);
*/

	// hacked up physics
	VectorCopy( body1->v, vP1 );
	VectorCopy( body1->L, vP2 );
	PM_ApplyCollision( body1, points1, at, normal, elasticity / 2.0f );
//	VectorMA( body1->v, body2->mass / totalMass, vP2, body1->v );

//	Com_Printf( "PM_ApplyBodyBodyCollision: v before %0.3f, %0.3f, %0.3f\n", body2->v[0], body2->v[1], body2->v[2] );

	VectorSubtract( vP1, body1->v, diff1 );
	VectorSubtract( vP2, body1->L, diff2 );

	VectorAdd( body2->v, diff1, body2->v );
	VectorAdd( body2->L, diff2, body2->L );

	// compute affected auxiliary quantities
	VectorRotate( body2->L, body2->inverseWorldInertiaTensor, body2->w );

	// temp to help wheel movement when hitting walls
	// FIXME: is this still needed?
	VectorMA( diff1, -DotProduct(diff1, body2->up), body2->up, delta );
	for ( i = 0; i < FIRST_FRAME_POINT; i++ ){
		VectorAdd( points2[i].v, delta, points2[i].v );
	}

	PM_UpdateFrameVelocities( body2, points2 );

//	VectorInverse( normal );
//	PM_ApplyCollision( body2, points2, at, normal, elasticity );
//	VectorMA( body2->v, body1->mass / totalMass, vP1, body2->v );

//	Com_Printf( "PM_ApplyBodyBodyCollision: v after %0.3f, %0.3f, %0.3f\n", body2->v[0], body2->v[1], body2->v[2] );

	return 0;

/*
	VectorSubtract(vP1, vP2, vP1);

	// added from collision
	VectorClear(impulse);
//	if (DotProduct(normal, vP1) < 0){
//		massFraction = (2.0f * body1->mass) / (body1->mass + body2->mass);
//		impulseNum = massFraction * DotProduct(normal, vP1);
//		massFraction = ((1.0f + elasticity) * body1->mass) / (body1->mass + body2->mass);
		impulseNum = -(1 + elasticity) * DotProduct(normal, vP1);
		oppositeImpulseNum = -(1.0f - elasticity) * DotProduct(normal, vP1);

//		impulseDen = 1.0f / body1->mass;
		impulseDen = 1.0f / body1->mass + 1.0f / body2->mass;
		CrossProduct(arm1, normal, cross);
		VectorRotate(cross, body1->inverseWorldInertiaTensor, cross2);
		CrossProduct(cross2, arm1, cross);
		//impulseDen = 1.0f / body1->mass + DotProduct(cross, normal);
		impulseDen += DotProduct(cross, normal);

		CrossProduct(arm2, normal, cross);
		VectorRotate(cross, body2->inverseWorldInertiaTensor, cross2);
		CrossProduct(cross2, arm2, cross);
		impulseDen += DotProduct(cross, normal);

		VectorScale(normal, impulseNum / impulseDen, impulse);

//	}
//	else {
//		// not hitting surface
//		return;
//	}

	// apply impulse to primary quantities
	VectorMA(body1->v, 1.0 / body1->mass, impulse, body1->v);
	CrossProduct(arm1, impulse, impulseMoment);
	VectorAdd(body1->L, impulseMoment, body1->L);
    
    // compute affected auxiliary quantities
	VectorRotate(body1->L, body1->inverseWorldInertiaTensor, body1->w);

	// apply impulse to primary quantities of second object
	VectorInverse(impulse);
	VectorMA(body2->v, 1.0 / body2->mass, impulse, body2->v);
	CrossProduct(arm2, impulse, impulseMoment);
	VectorAdd(body2->L, impulseMoment, body2->L);

	// compute affected auxiliary quantities of second object
	VectorRotate(body2->L, body2->inverseWorldInertiaTensor, body2->w);

	// temp to help wheel movement when hitting walls
//	VectorMA(impulse, -DotProduct(impulse, body->up), body->up, delta);
//	for (i = 0; i < FIRST_FRAME_POINT; i++){
//		VectorMA(points[i].v, 1.0 / body->mass, delta, points[i].v);
//	}

	PM_UpdateFrameVelocities(body1, points1);

	if (fabs(oppositeImpulseNum / impulseDen) > 5000.0f)
		return fabs(oppositeImpulseNum / impulseDen);
	else
		return 0;
*/
}



/*
================================================================================
PM_ApplyPointBodyCollision

  This function is a hacked up version of PM_ApplyCollision to try to simulate
  the collision between a tire and the body of a car (like when the suspension bottoms out).

Given: rigid body, impact origin, impact normal, and elasticity
Find:  the new linear and angular velocities as a result of the impact.
================================================================================
*/
static void PM_ApplyPointBodyCollision( carBody_t *body, carPoint_t *points, carPoint_t *pt, vec3_t at, vec3_t normal, float elasticity ){
	vec3_t	arm;
	vec3_t	vP1;
	vec3_t	impulse, impulseMoment;
	vec3_t	cross, cross2;
	float	impulseNum, impulseDen;
	float	massFraction;

	VectorSubtract(at, body->CoM, arm);
//	VectorSubtract(at, body->r, arm);

	if (pm->pDebug){
		Com_Printf("PM_ApplyPointBodyCollision: arm %0.3f, %0.3f, %0.3f\n", arm[0], arm[1], arm[2]);
		Com_Printf("PM_ApplyPointBodyCollision: normal %0.3f, %0.3f, %0.3f\n", normal[0], normal[1], normal[2]);
	}

	CrossProduct(body->w, arm, cross);
	VectorAdd(body->v, cross, vP1);

	// added from collision
	VectorClear(impulse);
//	if (DotProduct(normal, vP1) < 0){
		massFraction = (2.0f * pt->mass) / (pt->mass + body->mass);
		impulseNum = massFraction * (DotProduct(normal, pt->v) - DotProduct(normal, vP1));

		CrossProduct(arm, normal, cross);
		VectorRotate(cross, body->inverseWorldInertiaTensor, cross2);
		CrossProduct(cross2, arm, cross);
		impulseDen = 1.0 / body->mass + DotProduct(cross, normal);

		VectorScale(normal, impulseNum / impulseDen, impulse);
//	}

	// apply impulse to primary quantities of rigid body
	VectorMA(body->v, 1.0 / body->mass, impulse, body->v);
	CrossProduct(arm, impulse, impulseMoment);
	VectorAdd(body->L, impulseMoment, body->L);

    // compute affected auxiliary quantities
	VectorRotate(body->L, body->inverseWorldInertiaTensor, body->w);

	VectorMA(pt->v, -1.0 / pt->mass, impulse, pt->v);
/*
	// temp to help wheel movement when hitting walls
	VectorMA(impulse, -DotProduct(impulse, body->up), body->up, delta);
	for (i = 0; i < FIRST_FRAME_POINT; i++){
		VectorMA(points[i].v, 1.0 / body->mass, delta, points[i].v);
	}
*/
	PM_UpdateFrameVelocities(body, points);
}
#endif

#ifdef CGAME
void CG_Sparks( const vec3_t origin, const vec3_t normal, const vec3_t direction, const float speed );
#endif

/*
================================================================================
PM_CarBodyFrictionForces

  Friction forces between the car body and the world

================================================================================
*/
static void PM_CarBodyFrictionForces( car_t *car, carBody_t *body, carPoint_t *points, int i ){
	vec3_t		vel, force;
	float		normalForce;
	float		speed;

	normalForce = DotProduct( body->netForce, points[i].normals[0] );

	if ( normalForce >= 0.0f ) return;

	VectorMA( body->netForce, -normalForce, points[i].normals[0], force );
	VectorMA( points[i].v, -DotProduct( points[i].v, points[i].normals[0] ), points[i].normals[0], vel );
	speed = VectorNormalize( vel );

	if ( VectorLength(force) > fabs( 0.7f * normalForce ) || speed > 2.0f ){
//		VectorNormalize(vel);
		VectorScale( vel, 0.4f * normalForce, force );
	}
	else {
		VectorInverse( force );
	}

	PM_ApplyForce( body, force, points[i].r );

#ifdef CGAME
	if( speed > 20.0f )
	{
		vec3_t	origin;
		float	r;

		r = random() * 5000 / speed;
		if( r < 80 )
		{
			VectorMA( points[i].r, -BODY_RADIUS / 1.5f, points[i].normals[0], origin );
//			VectorInverse( vel );
			VectorMA( vel, 0.1f, points[i].normals[0], vel );
			VectorNormalize( vel );
			CG_Sparks( origin, points[i].normals[0], vel, speed * 0.5f );
		}
	}
#endif
}


/*
================================================================================
PM_Generate_SwayBar_Forces

  Simulates ARBs in the car

================================================================================
*/
static void PM_Generate_SwayBar_Forces(car_t *car, carBody_t *body, carPoint_t *points, int leftWheel, int rightWheel ){
//	vec3_t		lSpring, rSpring;
//	vec3_t		lengthDiff;
	float		dot, force;
//	carPoint_t	*lWheel;
//	carPoint_t	*rWheel;

//	lWheel = &points[leftWheel];
//	rWheel = &points[rightWheel];

	if ( fabs( DotProduct( body->v, body->forward ) ) < 50.0f)
		return;

//	VectorSubtract(points[leftWheel+4].v, lWheel->v, lSpring);
//	VectorSubtract(points[rightWheel+4].v, rWheel->v, rSpring);

//	VectorSubtract(lSpring, rSpring, lengthDiff);

	dot = body->curSpringLengths[leftWheel] - body->curSpringLengths[rightWheel];

	// if dotproduct(lengthDiff, up) < 0, rSpring > lSpring
	// else rSpring < lSpring
//	dot = DotProduct(body->up, lengthDiff);
	force = dot * pm->car_swaybar * CP_GRAVITY;

	if (force < 0 && !points[leftWheel].onGround)
		return;
	else if (force > 0 && !points[rightWheel].onGround)
		return;

	if (force < 0 && pm->ps->viewangles[ROLL] > 0){
//		Com_Printf("Sway bars shouldnt be forcing\n");
		return;
	}
	else if (force > 0 && pm->ps->viewangles[ROLL] < 0){
//		Com_Printf("Sway bars shouldnt be forcing\n");
		return;
	}

//	Com_Printf("PM_Generate_SwayBar_Forces: force %f, roll %f\n", force, pm->ps->viewangles[ROLL]);

//	VectorScale(body->up, force, lWheel->forces[SWAY_BAR]);
	VectorScale(body->up, -force, points[leftWheel+4].forces[SWAY_BAR]);

//	VectorScale(body->up, -force, rWheel->forces[SWAY_BAR]);
	VectorScale(body->up, force, points[rightWheel+4].forces[SWAY_BAR]);
}


/*
================================================================================
PM_Generate_FrameWheel_Forces

  Handles spring and damper physics for the suspension

================================================================================
*/
static int PM_Generate_FrameWheel_Forces(car_t *car, carPoint_t *points, int frameIndex, int wheelIndex ){
	vec3_t		delta, diff;
	float		compression, springVel, length;
	int			hitType;
	carPoint_t	*frame, *wheel;

	hitType = HTYPE_NO_HIT;

	frame = &points[frameIndex];
	wheel = &points[wheelIndex];

	//
	// Calculate forces on the frame
	//

	// spring force based on length of spring (origin difference of the two points)
	length = car->sBody.curSpringLengths[wheelIndex];

	// shock force based on velocity difference of the two points
	VectorSubtract(frame->v, wheel->v, delta);
	springVel = DotProduct(car->sBody.up, delta);

	if (springVel > 0){
		VectorScale(car->sBody.up, -pm->car_shock_up * CP_GRAVITY * springVel, frame->forces[SHOCK]);
	}
	else {
		VectorScale(car->sBody.up, -pm->car_shock_down * CP_GRAVITY * springVel, frame->forces[SHOCK]);
	}

//	compression = springVel > 0 ? 0 : springVel;
//	VectorScale(car->sBody.up, -car->shockStrength * compression, frame->forces[SHOCK]);

	if (length < car->springMinLength){
		if (springVel < 0){
			VectorScale(car->sBody.up, -springVel, diff);
			VectorSubtract(wheel->v, diff, wheel->v);

			if (wheel->onGround){
				// suspension bottomed out and tires are on the ground so we have
				// to treat this as a collision of the frame with the ground

				hitType = HTYPE_BOTTOMED_OUT;
			}
		}
//		length = car->springMinLength;
	}
	else if (length > car->springMaxLength){
		if (springVel > 0){
//			PM_ApplyPointBodyCollision(&car->sBody, &car->sPoints, car->sBody.up, 1.0);
/*
			VectorScale(car->sBody.up, DotProduct(car->sBody.up, wheelVelocity), diff);
			VectorSubtract(wheelVelocity, diff, delta);

			VectorScale(car->sBody.up, DotProduct(car->sBody.up, frameVelocity), diff);
			VectorAdd(delta, diff, wheel->v);
*/
			// move wheel so its not too far out
			VectorMA( wheel->r, length - car->springMaxLength, car->sBody.up, wheel->r );
			car->sBody.curSpringLengths[wheelIndex] = car->springMaxLength;
			
		}
		hitType = HTYPE_MAXED_OUT;
//		length = car->springMaxLength;
	}

	// linear
//	compression = car->springMaxLength - length;

/*
	if (length > car->springMaxLength)
		compression = car->springMaxLength - length;
	else
		compression = (car->springMaxLength*2.0f - length) / 5.0f;
*/

//	x^2
//	compression = car->springMaxLength - length;
//	compression *= compression;

//	exp(x/max dist)

	compression = car->springMaxLength - length;

	if( compression > 0 )
		compression = (car->springMaxLength - car->springMinLength) * (exp( compression / (car->springMaxLength - car->springMinLength)) - 1 ) / 1.718281828f;

	if (compression < 0)
		VectorScale(car->sBody.up, 3.0f * CP_SPRING_STRENGTH * compression, frame->forces[SPRING]);
	else
		VectorScale(car->sBody.up, car->springStrength * compression, frame->forces[SPRING]);

/*
	{
		float	shock = VectorLength( frame->forces[SHOCK] );
		if( shock > CP_MAX_SHOCK_FORCE )
		{
#ifdef GAME
			Com_Printf( "%i Shock Force: %f\n", (wheel - &car->sPoints[0]), shock );
#endif
			VectorScale( frame->forces[SHOCK], CP_MAX_SHOCK_FORCE / shock, frame->forces[SHOCK] );
		}
	}
*/

	//
	// Calculate forces on the tire
	//
	VectorScale(frame->forces[SPRING], -1, wheel->forces[SPRING]);
	VectorScale(frame->forces[SHOCK], -1, wheel->forces[SHOCK]);

	return hitType;
}


/*
================================================================================
PM_CalculateForces

  Main force calculating function for the entire car and the wheels

================================================================================
*/
static void PM_CalculateForces( car_t *car, carBody_t *body, carPoint_t *points, float sec ){
	vec3_t	delta, diff;
	//vec3_t	arm;
	vec3_t	force, hitOrigin, normal;
	float	length, k, b, springVel, dot;
	int		count;
	int		i, hitType, n;
	//float	impulseDamage;

	VectorClear(force);

	// add gravity forces
	for (i = 0; i < NUM_CAR_POINTS; i++){
		VectorSet(points[i].forces[GRAVITY], 0, 0, -CP_CURRENT_GRAVITY * points[i].mass);
	}

	VectorClear(hitOrigin);
	count = 0;

	PM_Generate_SwayBar_Forces(car, body, points, 0, 1);
	PM_Generate_SwayBar_Forces(car, body, points, 2, 3);

	// add spring forces
	for (i = 0; i < FIRST_FRAME_POINT; i++)
	{
//		VectorSubtract(points[i+4].r, points[i].r, delta);
		if ( body->curSpringLengths[i] > 20 )
		{
//			Com_Printf("Initialize wheel %i, frame %f,%f,%f, wheel %f,%f,%f\n", i, points[i+4].r[0], points[i+4].r[1], points[i+4].r[2], points[i].r[0], points[i].r[1], points[i].r[2]);
			if (i == 0)
				PM_InitializeWheel(body, points, i, 1.0f, -1.0f);
			else if (i == 1)
				PM_InitializeWheel(body, points, i, 1.0f, 1.0f);
			else if (i == 2)
				PM_InitializeWheel(body, points, i, -1.0f, -1.0f);
			else if (i == 3)
				PM_InitializeWheel(body, points, i, -1.0f, 1.0f);
		}

		hitType = PM_Generate_FrameWheel_Forces(car, points, i+4, i);

		if ( VectorNAN( points[i].netForce ) )
			Com_Printf("Blowing up car because of car point force on wheel %d\n", i );

		if (hitType == HTYPE_BOTTOMED_OUT)
		{
			// add a normal to the frame point
			for ( n = 0; n < 3; n++ )
			{
				if ( VectorLength( points[i+4].normals[n] ) ) continue;
				VectorCopy( body->up, points[i+4].normals[n] );
				break;
			}
			VectorAdd( hitOrigin, points[i+4].r, hitOrigin );
			count++;
		}
		else if (hitType == HTYPE_MAXED_OUT)
		{
			// add a normal to the frame point
			for ( n = 0; n < 3; n++ )
			{
				if ( VectorLength( points[i+4].normals[n] ) ) continue;
				VectorScale( body->up, -1.0f, points[i+4].normals[n] );
				break;
			}

			VectorAdd( points[i].forces[SPRING], points[i].forces[SHOCK], force );

			dot = DotProduct( force, body->up );
			if ( dot < 0.0f )
				VectorMA( points[i].forces[SPRING], -dot, body->up, points[i].forces[SPRING] );
		}
	}

	if ( count != 0 )
	{
		if (pm->pDebug)
			Com_Printf("PM_CalculateForces: Frame-Wheel Collision with %i wheels\n", count);

		VectorScale(hitOrigin, 1.0 / (float)count, hitOrigin);
		/*impulseDamage = */PM_ApplyCollision(body, points, hitOrigin, car->sBody.up, body->elasticity);

		// add normal force
		// FIXME - change this so i can do it only once right before acceleration
		// FIXME: replace with proper normal.netForce
/*
		VectorSet(delta, 0, 0, -CP_CURRENT_GRAVITY * CP_CAR_MASS);
		if (DotProduct(body->up, delta) < 0.0f){
			VectorScale(body->up, -OVERCLIP * DotProduct(body->up, delta), body->normalForce);
			PM_ApplyForce(body, body->normalForce, hitOrigin);
		}
*/

		// damage stuff
// FIXME: taking too much damage during normal driving
/*
		impulseDamage /= (2000.0f / sec) / count;
		if (impulseDamage > 0.00001f){
			VectorCopy(hitOrigin, pm->damage.origin);
			VectorCopy(car->sBody.up, pm->damage.dir);
			pm->damage.damage += impulseDamage;
			pm->damage.mod = MOD_BO_SHOCKS;
		}
*/
		VectorCopy( hitOrigin, pm->damage.origin );
		VectorCopy( car->sBody.up, pm->damage.dir );
		pm->damage.dflags = DAMAGE_NO_KNOCKBACK;
		pm->damage.mod = MOD_BO_SHOCKS;
	}

	// UPDATE - moved lower down
//	PM_Generate_SwayBar_Forces(body, points, 0, 1);
//	PM_Generate_SwayBar_Forces(body, points, 2, 3);
	
	// calculate frame hits
	VectorClear(hitOrigin);
	VectorClear(normal);
	count = 0;

	for (i = FIRST_FRAME_POINT; i < NUM_CAR_POINTS; i++){
		if ( !points[i].onGround ) continue;
		if ( DotProduct(points[i].v, points[i].normals[0] ) > 0.0f) continue;

		VectorAdd( normal, points[i].normals[0], normal );
		VectorAdd( hitOrigin, points[i].r, hitOrigin );
		count++;
	}

	if ( count != 0 )
	{
		VectorScale(hitOrigin, 1.0 / (float)count, hitOrigin);
		VectorNormalize(normal);

		if (pm->pDebug)
			Com_Printf("PM_CalculateForces: Frame hit a surface at %i spots\n", count);

		/*impulseDamage = */PM_ApplyCollision( body, points, hitOrigin, normal, 0.0f );

		// add normal force
		// FIXME - change this so i can do it only once right before acceleration
		// FIXME: replace with proper normal.netForce
/*
		VectorSet(delta, 0, 0, -CP_CURRENT_GRAVITY * CP_CAR_MASS);
		if (DotProduct(normal, delta) < 0.0f){
			VectorScale(body->up, -OVERCLIP * DotProduct(body->up, delta), body->normalForce);
			PM_ApplyForce(body, body->normalForce, hitOrigin);
		}
*/

		// damage stuff - FIXME: dont take damage for sitting up against walls
		// divide by number of spots hit maybe?
/*
		impulseDamage /= (50000.0f / sec) / count;
		if (impulseDamage > 0.00001f){
			VectorCopy(hitOrigin, pm->damage.origin);
			VectorCopy(normal, pm->damage.dir);
			pm->damage.damage += impulseDamage;
			pm->damage.mod = MOD_WORLD_COLLISION;
		}
*/
		VectorCopy(hitOrigin, pm->damage.origin);
		VectorCopy(normal, pm->damage.dir);
		pm->damage.dflags = DAMAGE_NO_KNOCKBACK;
		pm->damage.mod = MOD_WORLD_COLLISION;
	}

	// steering, acceleration, braking, etc
	PM_AddRoadForces(car, body, points, sec);

	for (i = 0; i < FIRST_FRAME_POINT; i++)
	{
		// new
//		VectorMA(car->sPoints[i].r, -WHEEL_RADIUS, car->sPoints[i].normals[0], arm);

		// remove component of force in body up direction
		VectorCopy(points[i].forces[ROAD], force);
		VectorMA(force, -DotProduct(force, car->sBody.up), car->sBody.up, force);
		// apply force to axle, not tire-ground contact
		PM_ApplyForce(body, force, car->sPoints[i].r);
//		PM_ApplyForce(body, force, arm);

		// end new
		VectorScale(points[i].forces[ROAD], points[i].mass / points[i+4].mass, points[i].forces[ROAD]);
//	}

	// internal forces to return the wheel to perpendicular
//	for (i = 0; i < FIRST_FRAME_POINT; i++)
//	{
		VectorSubtract( points[i+4].r, points[i].r, delta );
		VectorMA( delta, -DotProduct(delta, car->sBody.up), car->sBody.up, diff );
		length = VectorNormalize(diff);

		if ( length > 2.0f ){
			// k = 10000; // good at 90fps
			k = pm->car_wheel * CP_WHEEL_MASS;

//			b = 2 * sqrt(k * points[i].mass);
			b = pm->car_wheel_damp * CP_WHEEL_MASS;

			VectorScale(diff, -k * length, force);

			VectorSubtract(points[i+4].v, points[i].v, delta);
			VectorMA(delta, -DotProduct(delta, car->sBody.up), car->sBody.up, diff);
			springVel = VectorNormalize(diff);
			VectorMA(force, -b * springVel, diff, force);

//			VectorCopy(force, points[i+4].forces[INTERNAL]);
			VectorScale(force, -1, points[i].forces[INTERNAL]);
		}
	}

	// calculate net forces
	for (i = 0; i < NUM_CAR_POINTS; i++){
		PM_CalculateNetForce(&points[i], i);
	}
}


/*
================================================================================
PM_AccelerateAndMoveBody

  This function is largely based on the physics articles and 
  source from Chris Hecker: http://www.d6.com/users/checker/dynamics.htm

================================================================================
*/
static void PM_AccelerateAndMoveBody( car_t *car, carBody_t *sBody, carBody_t *tBody, float time ){
	float		m[3][3];
	float		m2[3][3];
	vec3_t		vec;

	vec3_t		force;
	int			i, n;
	float		dot, impulseDamage;
	float		time_2 = time / 2.0f;

	// FIXME: handle collisions here as well?
	// FIXME: just have one normal force for the entire body instead of for each point?
	for (i = FIRST_FRAME_POINT; i < NUM_CAR_POINTS; i++){
		if (!car->sPoints[i].onGround) continue;
		for (n = 0; n < 3; n++){
			if (!VectorLength(car->sPoints[i].normals[n])) continue;
			dot = DotProduct(car->sPoints[i].normals[n], sBody->netForce);
			if (dot >= 0.0f) continue;

			VectorScale(car->sPoints[i].normals[n], -OVERCLIP * dot, force);
			PM_ApplyForce(sBody, force, car->sPoints[i].r);
		}
	}

	// damage stuff
	VectorSubtract(tBody->v, sBody->v, vec);
	impulseDamage = VectorLength(vec);
	if (impulseDamage > 400.0f){
//		Com_Printf("impulseDamage %f\n", impulseDamage);
		pm->damage.damage += impulseDamage / 25.0f;
	}


	// linear
	// NOTE: this method is really not right but it works so oh well.
	// tBody contains netForce from last frame
	VectorAdd(sBody->netForce, tBody->netForce, vec);
	VectorMA(sBody->v, time_2 / sBody->mass, vec, tBody->v);

	VectorAdd(sBody->v, tBody->v, vec);
	VectorMA(sBody->r, time_2, vec, tBody->r);

	// angular
	VectorAdd(sBody->netMoment, tBody->netMoment, vec);

	VectorMA(sBody->L, time_2, vec, tBody->L);
	VectorRotate(tBody->L, sBody->inverseWorldInertiaTensor, tBody->w);

	VectorAdd(sBody->w, tBody->w, vec);

	m[0][0] = 0;				m[0][1] = time_2 * -vec[2];	m[0][2] = time_2 * vec[1];
	m[1][0] = time_2 * vec[2];	m[1][1] = 0;				m[1][2] = time_2 * -vec[0];
	m[2][0] = time_2 * -vec[1];	m[2][1] = time_2 * vec[0];	m[2][2] = 0;

	MatrixMultiply(m, sBody->t, m2);
	MatrixAdd(sBody->t, m2, tBody->t);
	OrthonormalizeOrientation(tBody->t);

	// Optimization note: check if inverseWorldInertiaTensor is only really
	//    needed for collisions.  If it is then might be best just to calculate
	//    it there since collisions dont usually happen every frame.
	// Note: only used in collisions and converting L to w.  If I can get w
	//    some other way then I can move it.

//	MatrixMultiply(tBody->t, car->inverseBodyInertiaTensor, m);
	// This should be the same as MatrixMultiply(tBody->t, car->inverseBodyInertiaTensor, m);
	// without so many multiplies and adds.
	m[0][0] = car->inverseBodyInertiaTensor[0][0] * tBody->t[0][0];
	m[1][0] = car->inverseBodyInertiaTensor[0][0] * tBody->t[1][0];
	m[2][0] = car->inverseBodyInertiaTensor[0][0] * tBody->t[2][0];

	m[0][1] = car->inverseBodyInertiaTensor[1][1] * tBody->t[0][1];
	m[1][1] = car->inverseBodyInertiaTensor[1][1] * tBody->t[1][1];
	m[2][1] = car->inverseBodyInertiaTensor[1][1] * tBody->t[2][1];

	m[0][2] = car->inverseBodyInertiaTensor[2][2] * tBody->t[0][2];
	m[1][2] = car->inverseBodyInertiaTensor[2][2] * tBody->t[1][2];
	m[2][2] = car->inverseBodyInertiaTensor[2][2] * tBody->t[2][2];

	MatrixTranspose(tBody->t, m2);
	MatrixMultiply(m, m2, tBody->inverseWorldInertiaTensor);

	// generate direction vectors
	OrientationToVectors(tBody->t, tBody->forward, tBody->right, tBody->up);

	// copy netForce and netMoment to target so we can use it next frame
	VectorCopy(sBody->netForce, tBody->netForce);
	VectorCopy(sBody->netMoment, tBody->netMoment);
}


/*
================================================================================
PM_CalculateSecondaryQuantities

  Used in cgame to calculate w and forward, right, up from the new known
  primary values from the server

================================================================================
*/
void PM_CalculateSecondaryQuantities( car_t *car, carBody_t *body, carPoint_t *points ){
	float		m[3][3];
	float		m2[3][3];
	float		forwardScale, rightScale, upScale;
	vec3_t		diff;
	int			i;

//	MatrixMultiply(body->t, car->inverseBodyInertiaTensor, m);
	m[0][0] = car->inverseBodyInertiaTensor[0][0] * body->t[0][0];
	m[1][0] = car->inverseBodyInertiaTensor[0][0] * body->t[1][0];
	m[2][0] = car->inverseBodyInertiaTensor[0][0] * body->t[2][0];

	m[0][1] = car->inverseBodyInertiaTensor[1][1] * body->t[0][1];
	m[1][1] = car->inverseBodyInertiaTensor[1][1] * body->t[1][1];
	m[2][1] = car->inverseBodyInertiaTensor[1][1] * body->t[2][1];

	m[0][2] = car->inverseBodyInertiaTensor[2][2] * body->t[0][2];
	m[1][2] = car->inverseBodyInertiaTensor[2][2] * body->t[1][2];
	m[2][2] = car->inverseBodyInertiaTensor[2][2] * body->t[2][2];

	MatrixTranspose(body->t, m2);
	MatrixMultiply(m, m2, body->inverseWorldInertiaTensor);

	VectorRotate(body->L, body->inverseWorldInertiaTensor, body->w);

	OrientationToVectors(body->t, body->forward, body->right, body->up);

	// set locations and velocities of frame points
	PM_InitializeFrame(body, &points[FL_FRAME],  1.0f, -1.0f, 0.0f);
	PM_InitializeFrame(body, &points[FR_FRAME],  1.0f,  1.0f, 0.0f);
	PM_InitializeFrame(body, &points[RL_FRAME], -1.0f, -1.0f, 0.0f);
	PM_InitializeFrame(body, &points[RR_FRAME], -1.0f,  1.0f, 0.0f);

	// body collision points
	forwardScale = ((CAR_LENGTH/2.0f) - BODY_RADIUS) / WHEEL_FORWARD;
	rightScale = ((CAR_WIDTH/2.0f) - BODY_RADIUS) / WHEEL_RIGHT;
	upScale = (0) / -WHEEL_UP;
	PM_InitializeFrame(body, &points[FL_BODY],  forwardScale, -rightScale, upScale);
	PM_InitializeFrame(body, &points[FR_BODY],  forwardScale,  rightScale, upScale);
	PM_InitializeFrame(body, &points[ML_BODY],  0.0f, -rightScale, upScale);
	PM_InitializeFrame(body, &points[MR_BODY],  0.0f,  rightScale, upScale);
	PM_InitializeFrame(body, &points[RL_BODY], -forwardScale, -rightScale, upScale);
	PM_InitializeFrame(body, &points[RR_BODY], -forwardScale,  rightScale, upScale);

	PM_InitializeFrame(body, &points[ML_ROOF],  0.0f, -rightScale, 0.6f);
	PM_InitializeFrame(body, &points[MR_ROOF],  0.0f,  rightScale, 0.6f);

	// calculate spring lengths
	for( i = 0; i < FIRST_FRAME_POINT; i++ )
	{
		VectorSubtract( points[i+4].r, points[i].r, diff );
		body->curSpringLengths[i] = DotProduct( diff, body->up );
	}

	PM_SetCoM(body, points);

	PM_ClearCarForces(body, points);
	PM_CopyTargetToSource(&car->sBody, &car->tBody, car->sPoints, car->tPoints);
/*
	VectorCopy( car->sBody.v, car->oldBodies[2].v );
	VectorCopy( car->sBody.w, car->oldBodies[2].w );
	VectorCopy( car->sBody.netForce, car->oldBodies[2].netForce );
	VectorCopy( car->sBody.netMoment, car->oldBodies[2].netMoment );

	VectorCopy( car->sBody.v, car->oldBodies[1].v );
	VectorCopy( car->sBody.w, car->oldBodies[1].w );
	VectorCopy( car->sBody.netForce, car->oldBodies[1].netForce );
	VectorCopy( car->sBody.netMoment, car->oldBodies[1].netMoment );

	VectorCopy( car->sBody.v, car->oldBodies[0].v );
	VectorCopy( car->sBody.w, car->oldBodies[0].w );
	VectorCopy( car->sBody.netForce, car->oldBodies[0].netForce );
	VectorCopy( car->sBody.netMoment, car->oldBodies[0].netMoment );

	for( i = 0; i < LAST_RW_POINT; i++ )
	{
		VectorCopy( car->sPoints[i].netForce, car->oldPoints[2][i].netForce );
		VectorCopy( car->sPoints[i].v, car->oldPoints[2][i].v );
		car->oldPoints[2][i].netMoment = car->sPoints[i].netMoment;

		VectorCopy( car->sPoints[i].netForce, car->oldPoints[1][i].netForce );
		VectorCopy( car->sPoints[i].v, car->oldPoints[1][i].v );
		car->oldPoints[1][i].netMoment = car->sPoints[i].netMoment;

		VectorCopy( car->sPoints[i].netForce, car->oldPoints[0][i].netForce );
		VectorCopy( car->sPoints[i].v, car->oldPoints[0][i].v );
		car->oldPoints[0][i].netMoment = car->sPoints[i].netMoment;
	}
*/
}


/*
================================================================================
PM_CalculateTargetBody

  Integrates the current state to find the next state.

================================================================================
*/
static void PM_CalculateTargetBody(car_t *car, carBody_t *sBody, carBody_t *tBody, carPoint_t *sPoints, carPoint_t *tPoints, float time){
	int		i;
	float	forwardScale, rightScale, upScale;
	vec3_t	diff;

	// move body
	PM_AccelerateAndMoveBody( car, sBody, tBody, time );

	// move wheels
	for (i = 0; i < FIRST_FRAME_POINT; i++){
		PM_AccelerateAndMove( car, &sPoints[i], &tPoints[i], time );
	}

	// set locations and velocities of frame points
	PM_InitializeFrame(tBody, &tPoints[FL_FRAME],  1.0f, -1.0f, 0.0f);
	PM_InitializeFrame(tBody, &tPoints[FR_FRAME],  1.0f,  1.0f, 0.0f);
	PM_InitializeFrame(tBody, &tPoints[RL_FRAME], -1.0f, -1.0f, 0.0f);
	PM_InitializeFrame(tBody, &tPoints[RR_FRAME], -1.0f,  1.0f, 0.0f);

	// body collision points
	forwardScale = ((CAR_LENGTH/2.0f) - BODY_RADIUS) / WHEEL_FORWARD;
	rightScale = ((CAR_WIDTH/2.0f) - BODY_RADIUS) / WHEEL_RIGHT;
	upScale = (0) / -WHEEL_UP;
	PM_InitializeFrame(tBody, &tPoints[FL_BODY],  forwardScale, -rightScale, upScale);
	PM_InitializeFrame(tBody, &tPoints[FR_BODY],  forwardScale,  rightScale, upScale);
	PM_InitializeFrame(tBody, &tPoints[ML_BODY],  0.0f, -rightScale, upScale);
	PM_InitializeFrame(tBody, &tPoints[MR_BODY],  0.0f,  rightScale, upScale);
	PM_InitializeFrame(tBody, &tPoints[RL_BODY], -forwardScale, -rightScale, upScale);
	PM_InitializeFrame(tBody, &tPoints[RR_BODY], -forwardScale,  rightScale, upScale);

	PM_InitializeFrame(tBody, &tPoints[ML_ROOF],  0.0f, -rightScale, 0.6f);
	PM_InitializeFrame(tBody, &tPoints[MR_ROOF],  0.0f,  rightScale, 0.6f);

	// calculate spring lengths
	for( i = 0; i < FIRST_FRAME_POINT; i++ )
	{
		VectorSubtract( tPoints[i+4].r, tPoints[i].r, diff );
		tBody->curSpringLengths[i] = DotProduct( diff, tBody->up );
	}

	// set the center of mass
	PM_SetCoM(tBody, tPoints);
}


/*
qboolean PM_Hit_Car( trace_t trace ){
	// hit another car
	vec3_t	f, r, u, delta;
	float	dotF, dotR, dotU;
	qboolean lengthHitOK, widthHitOK, heightHitOK;

	lengthHitOK = qfalse;
	widthHitOK = qfalse;
	heightHitOK = qfalse;

	VectorSubtract(trace.endpos, pm->cars[trace.entityNum]->sBody.r, delta);
	OrientationToVectors(pm->cars[trace.entityNum]->sBody.t, f, r, u);

	// need to add radius of trace box to this check?
	dotF = DotProduct(delta, f);
	dotR = DotProduct(delta, r);
	dotU = DotProduct(delta, u);

	if (dotU < CAR_HEIGHT/2 + WHEEL_RADIUS && dotU > -CAR_HEIGHT/2 - WHEEL_RADIUS){
		heightHitOK = qtrue;
	}
	if (dotF < CAR_LENGTH/2 + WHEEL_RADIUS && dotF > -CAR_LENGTH/2 - WHEEL_RADIUS){
		lengthHitOK = qtrue;
	}
	if (dotR < CAR_WIDTH/2 + WHEEL_RADIUS && dotR > -CAR_WIDTH/2 - WHEEL_RADIUS){
		widthHitOK = qtrue;
	}

//	if (heightHitOK){
#ifdef GAME
//		G_LogPrintf( "%d hit %d\n", pm->ps->clientNum, trace.entityNum);
//		G_LogPrintf( "trace.endpos : %f, %f, %f\n", trace.endpos[0], trace.endpos[1], trace.endpos[2]);
//		G_LogPrintf( "pm->cars[trace.entityNum]->sBody.r : %f, %f, %f\n", pm->cars[trace.entityNum]->sBody.r[0], pm->cars[trace.entityNum]->sBody.r[1], pm->cars[trace.entityNum]->sBody.r[2]);
//		G_LogPrintf( "delta : %f, %f, %f\n", delta[0], delta[1], delta[2]);

//		G_LogPrintf( "height dist: %f, max %f\n", dotU, CAR_HEIGHT/2 + WHEEL_RADIUS);
//		G_LogPrintf( "length dist: %f, max %f\n", dotF, CAR_LENGTH/2 + WHEEL_RADIUS);
//		G_LogPrintf( "width dist: %f, max %f\n", dotR, CAR_WIDTH/2 + WHEEL_RADIUS);
//		G_LogPrintf( "f     : %f, %f, %f\n", f[0], f[1], f[2]);
//		G_LogPrintf( "r     : %f, %f, %f\n", r[0], r[1], r[2]);
//		G_LogPrintf( "u     : %f, %f, %f\n", u[0], u[1], u[2]);
#endif
//	}

	if (widthHitOK){
#ifdef GAME
		G_LogPrintf("width is ok\n");
#endif
		return qtrue;
	}

	if (lengthHitOK && heightHitOK && widthHitOK){
#ifdef GAME
		G_LogPrintf("all dimensions are ok\n");
#endif
		return qtrue;
	}

	return qfalse;
}
*/


/*
===================
PM_Trace_Points

  Traces out the points from current state to next state to check for collisions.
  Partially based on the PM_SlideMove function.

===================
*/
#define	MAX_CLIP_PLANES	3
static void PM_Trace_Points( car_t *car, carPoint_t *sPoints, carPoint_t *tPoints, float time ){
	vec3_t	maxs, mins;
	vec3_t	start, dest, dir;
	//vec3_t	normal;
	trace_t	trace;
	int		i, j;
	//float	minTrace = 1.0f;
	//int		hitFirst, hitEnt;
	carPoint_t	*sPoint, *tPoint;

	//int		count = 0;
	vec3_t	hitOrigin;

// new stuff
	int			bumpcount, numbumps;
	float		d;
	int			numplanes;
	vec3_t		vel;
	vec3_t		clipVelocity;
	int			k;
	//int			l;
	float		time_left;
	float		into;
	
	numbumps = 4;
// end

	VectorClear(hitOrigin);

	// should actually do all points
	for( i = 0 ; i < NUM_CAR_POINTS ; i++ ) {
		// skip collision detection of suspension points
		if( i >= FIRST_FRAME_POINT && i < LAST_FRAME_POINT ) continue;

		sPoint = &sPoints[i];
		tPoint = &tPoints[i];

		VectorSet( mins, -sPoint->radius, -sPoint->radius, -sPoint->radius );
		VectorSet( maxs,  sPoint->radius,  sPoint->radius,  sPoint->radius );

		if( i >= LAST_FRAME_POINT ) {
			mins[2] /= 1.5f;
			maxs[2] /= 1.5f;
		}

/*
		VectorSubtract( car->sBody.r, sPoints[i].r, dir);
		if (VectorLength(dir))
			VectorScale(dir, 20.0f / VectorLength(dir), dir);

		VectorCopy( sPoints[i].r, start );
		VectorAdd( start, dir, start );

		VectorCopy( tPoints[i].r, dest );

		// trace the frame to target position
		pm->trace( &trace, start, mins, maxs, dest, pm->ps->clientNum, pm->tracemask );

#ifdef GAME
		if (trace.contents & CONTENTS_BODY){
//			G_LogPrintf("car was hit\n");
			if (trace.startsolid)
				Com_Printf("start solid and hit CONTENTS_BODY\n");
			if (trace.allsolid)
				Com_Printf("all solid and hit CONTENTS_BODY\n");

			// need to make it at least a tiny ways
			if (trace.fraction != 0.0f){
				if (trace.fraction < minTrace){
					minTrace = trace.fraction;
					VectorClear(hitOrigin);
					count = 0;
				}
				if (trace.fraction == minTrace){
					VectorAdd(hitOrigin, trace.endpos, hitOrigin);
					count++;
					if (g_entities[trace.entityNum].flags & FL_EXTRA_BBOX)
						hitEnt = g_entities[trace.entityNum].r.ownerNum;
					else
						hitEnt = trace.entityNum;

//					PM_CheckSurfaceFlags( &trace, &tPoints[i] );
				}

//				PM_SetFluidDensity(tPoints, i);
//				continue;
			}

			if (trace.fraction == 0)
				Com_Printf("fraction == 0 and hit CONTENTS_BODY\n");

			// trace the frame to target position but skip other cars
			pm->trace( &trace, start, mins, maxs, dest, pm->ps->clientNum, pm->tracemask & ~CONTENTS_BODY );
		}

		VectorClear(tPoints[i].normals[1]);
#endif

		if (trace.contents & CONTENTS_BODY){
			Com_Printf("hit body again?\n");
		}

		// if the frame is flush on the ground
		if( trace.fraction == 0.0F ) {
			tPoints[i].onGround = qtrue;
			if( VectorLength(trace.plane.normal) > 0.0F ) {
				VectorCopy( trace.plane.normal, tPoints[i].normals[0] );
			}

			PM_CheckSurfaceFlags( &trace, &tPoints[i] );
		} else if (trace.fraction < 1.0F){
			tPoints[i].onGround = qtrue;
			VectorCopy( trace.plane.normal, tPoints[i].normals[0] );

			PM_CheckSurfaceFlags( &trace, &tPoints[i] );
		} else {
			tPoints[i].onGround = qfalse;
			continue;
		}
*/

		// move the point in towards the center of the car
		// this is to try to stop the wheel bboxes from sitting directly on the surface
		// and then not getting a valid trace because it is startsoild

		VectorSubtract( car->sBody.r, sPoint->r, dir);
		if ( VectorLengthSquared(dir) )
			VectorScale( dir, 20.0f / VectorLength(dir), dir );

		VectorAdd( sPoint->r, dir, start );
		VectorCopy( tPoint->r, dest );

/*
		numTraces++;
		pm->trace( &trace, sPoints[i].r, mins, maxs, sPoints[i].r, pm->ps->clientNum, pm->tracemask );

		if( trace.startsolid )
		{
			// if we are starting solid then  offset the starting point otherwise
			// we dont need to.

			Com_Printf( "the start is solid!\n" );

			// try adding on a non zero velocity and the previous normal vector
//			if( VectorLengthSquared( sPoints[i].v ) == 0.0f )
//				VectorMA( sPoints[i].lastNonZeroVelocity, -1.0f, sPoints[i].normals[0], dir );
//			else
				VectorMA( sPoints[i].v, -20.0f, sPoints[i].normals[0], dir );
			VectorNormalize( dir );
			VectorSubtract( sPoints[i].r, dir, start );
		}
		else
			VectorCopy( sPoints[i].r, start );
		VectorCopy( tPoints[i].r, dest );
*/

		// OPTIMIZE: just use sPoints[i].v if not startsolid?
//		VectorSubtract( dest, start, vel );
		VectorSubtract( tPoint->r, start, vel );
		VectorScale( vel, 1 / time, vel );

		numplanes = 0;
		time_left = time;

		for ( bumpcount=0 ; bumpcount < numbumps ; bumpcount++ ) {
			VectorMA( start, time_left, vel, dest );

			// see if we can make it there
			numTraces++;
			pm->trace ( &trace, start, mins, maxs, dest, pm->ps->clientNum, pm->tracemask);

			if ( trace.startsolid && !bumpcount ) {
				VectorCopy( sPoint->r, start );
				VectorSubtract( dest, start, vel );
				VectorScale( vel, 1 / time, vel );

//				Com_Printf("Start point %d is in a solid\n", i);
				continue;
			}

			if (trace.allsolid) {
				// entity is completely trapped in another solid
//				Com_Printf("trace %d is all solid\n", i);
				break;
			}

			if (trace.fraction > 0) {
				// actually covered some distance
//				VectorCopy ( trace.endpos, start );
				VectorScale ( trace.endpos, 0.999f, start );
			}

			if ( trace.fraction == 1 ) {
				if ( !bumpcount ){
					tPoint->onGround = qfalse;
					VectorClear(tPoint->normals[0]);
					VectorClear(tPoint->normals[1]);
					VectorClear(tPoint->normals[2]);
				}

				break;		// moved the entire distance
			}

			time_left -= time_left * trace.fraction;

			if ( numplanes >= MAX_CLIP_PLANES ) {
				// this shouldn't really happen
//				Com_Printf("numplanes >= MAX_CLIP_PLANES\n");
				break;
			}

			//
			// if this is the same plane we hit before, nudge velocity
			// out along it, which fixes some epsilon issues with
			// non-axial planes
			//

			for ( j = 0 ; j < numplanes; j++ ) {
				if ( DotProduct( trace.plane.normal, tPoint->normals[j] ) > 0.99 ) {
					VectorAdd( trace.plane.normal, vel, vel );
					break;
				}
			}

			if ( j < numplanes ) {
				continue;
			}

#ifdef GAME
			if (trace.contents & CONTENTS_BODY){
				if (g_entities[trace.entityNum].flags & FL_EXTRA_BBOX){
					hitFirst = g_entities[trace.entityNum].r.ownerNum;
//					Com_Printf("Hit extra bbox\n");
				}
				else
					hitFirst = trace.entityNum;

				if (g_entities[hitFirst].client){

//					G_LogPrintf("car was hit\n");
//					if (trace.startsolid)
//						Com_Printf("start solid and hit CONTENTS_BODY\n");
//					if (trace.allsolid)
//						Com_Printf("all solid and hit CONTENTS_BODY\n");

					// need to make it at least a tiny ways
					if (trace.fraction != 0.0f)
					{
						if (trace.fraction < minTrace)
						{
							minTrace = trace.fraction;
							VectorClear(hitOrigin);
							count = 0;
						}

						if (trace.fraction == minTrace)
						{
							VectorAdd(hitOrigin, trace.endpos, hitOrigin);
							count++;
							hitEnt = hitFirst;

							tPoints[i].onGround = qtrue;

//							PM_CheckSurfaceFlags( &trace, &tPoints[i] );
						}

//						PM_SetFluidDensity(tPoints, i);
//						continue;
					}
					else
					{
						// inside another car
//						Com_Printf("fraction == 0 and hit CONTENTS_BODY\n");
					}

					// trace the frame to target position but skip other cars
//					pm->trace( &trace, start, mins, maxs, dest, pm->ps->clientNum, pm->tracemask & ~CONTENTS_BODY );
					pm->tracemask &= ~CONTENTS_BODY;
					continue;
				}
				else {
//					Com_Printf( "hit non player CONTENTS_BODY\n" );
					PM_AddTouchEnt( trace.entityNum, trace.endpos );

					pm->tracemask &= ~CONTENTS_BODY;
					continue;
				}
			}
#endif

			VectorCopy ( trace.plane.normal, tPoint->normals[numplanes] );
			tPoint->onGround = qtrue;
			PM_CheckSurfaceFlags( &trace, tPoint );
			numplanes++;

			//
			// modify velocity so it parallels all of the clip planes
			//

			// find a plane that it enters
			for ( j = 0 ; j < numplanes ; j++ ) {
				into = DotProduct( vel, tPoint->normals[j] );
				if ( into > 0.01f ) {
					continue;		// move doesn't interact with the plane
				}

				// see how hard we are hitting things
//				if ( -into > pml.impactSpeed ) {
//					pml.impactSpeed = -into;
//				}

				// slide along the plane
				VectorMA( vel, -into * OVERCLIP, tPoint->normals[j], clipVelocity );
//				PM_ClipVelocity ( vel, tPoints[i].normals[j], clipVelocity, OVERCLIP );

				// see if there is a second plane that the new move enters
				for ( k = 0 ; k < numplanes ; k++ ) {
					if ( k == j ) {
						continue;
					}

					into = DotProduct( clipVelocity, tPoint->normals[k] );
					if ( into > 0.01f ) {
						continue;		// move doesn't interact with the plane
					}

					// try clipping the move to the plane
					VectorMA( clipVelocity, -into * OVERCLIP, tPoint->normals[j], clipVelocity );
//					PM_ClipVelocity( clipVelocity, tPoints[i].normals[k], clipVelocity, OVERCLIP );

					// see if it goes back into the first clip plane
					if ( DotProduct( clipVelocity, tPoint->normals[j] ) >= 0 ) {
						continue;
					}

					// slide the original velocity along the crease
					CrossProduct ( tPoint->normals[j], tPoint->normals[k], dir );
					VectorNormalize( dir );
					d = DotProduct( dir, vel );
					VectorScale( dir, d, clipVelocity );

					// dont do this because i never see that triple plane interaction message
					// so it probably only rarely happens, if ever
/*
					// see if there is a third plane the the new move enters
					for ( l = 0 ; l < numplanes ; l++ ) {
						if ( l == j || l == k ) {
							continue;
						}
						if ( DotProduct( clipVelocity, tPoints[i].normals[l] ) >= 0.1 ) {
							continue;		// move doesn't interact with the plane
						}

						// stop dead at a tripple plane interaction
						Com_Printf( "triple plane interaction\n" );
						return;
					}
*/
				}

				// if we have fixed all interactions, try another move
				VectorCopy( clipVelocity, vel );
//				VectorCopy( endClipVelocity, endVelocity );
				break;
			}
		}

		// NEW- copy final position to the point
//		VectorCopy( start, tPoints[i].r );

		PM_SetFluidDensity( tPoints, i );
	}


#ifdef GAME
	if ( count && pm->cars[hitEnt] )
	{
		float	impulseDamage;

//		G_LogPrintf( "minTrace %f\n", minTrace );
//		G_LogPrintf("count = %d\n", count);
//		G_LogPrintf("pml.physicsSplit = %d\n", pml.physicsSplit);

		// calculate car position at collision
//		G_LogPrintf( "car was hit with %f traced\n", minTrace );
		VectorScale(hitOrigin, 1.0f / count, hitOrigin);

//		G_LogPrintf("hitOrigin = %f, %f, %f\n", hitOrigin[0], hitOrigin[1], hitOrigin[2]);

		PM_CalculateTargetBody(car, &car->sBody, &car->tBody, car->sPoints, car->tPoints, time * minTrace);

//		VectorSubtract(car->tBody.r, pm->cars[hitEnt]->sBody.r, normal);
		VectorSubtract(hitOrigin, pm->cars[hitEnt]->sBody.r, normal);
		VectorNormalize(normal);

		impulseDamage = PM_ApplyBodyBodyCollision(&car->tBody, car->tPoints, &pm->cars[hitEnt]->sBody, pm->cars[hitEnt]->sPoints, hitOrigin, normal, 0.25f);

		// set normal on this car
		for (i = 0; i < 3; i++){
			if (!VectorLength(tPoints[hitEnt].normals[i])){
				VectorScale(normal, -1.0f, tPoints[hitEnt].normals[i]);
				break;
			}
		}

		// damage stuff

		VectorCopy(hitOrigin, pm->damage.origin);
		VectorCopy(normal, pm->damage.dir);
		pm->damage.dflags = DAMAGE_NO_KNOCKBACK;
		pm->damage.mod = MOD_CAR_COLLISION;
		pm->damage.otherEnt = trace.entityNum;

		PM_CopyTargetToSource(&car->tBody, &car->sBody, tPoints, sPoints);

		// run physics again but remove CONTENTS_BODY first so it cant collide with cars
		pml.physicsSplit++;
		pm->tracemask &= ~CONTENTS_BODY;
		PM_DriveMove(car, time * (1.0f - minTrace), qfalse);
	}
#endif

	// break the frame up into parts
	// fix this so it doesnt cause infinite loops
/*
	if (minTrace < 1.0f){
		// Check for touching multiple surfaces with different normals
		// (wheel on ground and against wall)

		VectorCopy( sPoints[hitFirst].r, start );
		VectorCopy( tPoints[hitFirst].r, dest );
		VectorSubtract(dest, start, dir);
		length = VectorNormalize(dir);
		VectorMA(start, (length * minTrace) - 0.05f, dir, start);

		VectorClear(normal);

		for (i = 0; i < 3; i++){
			VectorCopy(start, dest);
			dest[i] += dir[i] > 0.0f ? 0.10f : -0.10f;

			pm->trace( &trace, start, mins, maxs, dest, pm->ps->clientNum, pm->tracemask );
			if (trace.fraction < 1.0F){
				uniqueNormal = qtrue;

				for(j = 0; j < 3; j++){
					if (i == j) continue;
					if(VectorCompare(trace.plane.normal, tPoints[hitFirst].normals[j])){
						uniqueNormal = qfalse;
						break;
					}
				}
				if (uniqueNormal && VectorLength(trace.plane.normal)){
					VectorCopy(trace.plane.normal, tPoints[hitFirst].normals[i]);
				}
			}
		}

		if (pml.physicsSplit > 5){
//			Com_Printf("Breaking out of physics loop\n");
		}
		else {
			pml.physicsSplit++;

			PM_CalculateTargetBody(car, &car->sBody, &car->tBody, car->sPoints, car->tPoints, time * minTrace);
			PM_CopyTargetToSource(&car->tBody, &car->sBody, car->tPoints, car->sPoints);
			PM_DriveMove(car, time * (1.0f - minTrace));
		}
//		}
	}
*/
}


//int trap_Milliseconds( void );

/*
================================================================================
PM_DriveMove

  The mother functioner of Q3Rally car movement.  Calculates the forces and torques
  on the car, integrates to get the next state and traces to handle collisions with
  the world and other objects.

================================================================================
*/
void PM_DriveMove( car_t *car, float time, qboolean includeBodies )
{
	int		i;
	//int		t, t1, t2, t3, t4, t5;

	if( car->initializeOnNextMove )
	{
		PM_InitializeVehicle( car, pm->ps->origin, pm->ps->viewangles, pm->ps->velocity );
		car->initializeOnNextMove = qfalse;
	}

	//t = trap_Milliseconds();

	numTraces = 0;

	if (pml.physicsSplit > 2){
		Com_Printf("forcing end of frame\n");
		return;
	}

	if (time == 0)
		return;

//	if ( VectorNAN(car->sBody.r) || VectorNAN(car->sBody.v) || VectorNAN(car->sBody.L) )
	if ( VectorNAN(car->sBody.r) || VectorNAN(car->sBody.v) ||
		car->sBody.L[0] > 1<<25 ||
		car->sBody.L[1] > 1<<25 ||
		car->sBody.L[2] > 1<<25 )
	{
//		Com_Printf( "Blowing up car because of car body\n" );
		pm->damage.damage = 32000;
		pm->damage.dflags = DAMAGE_NO_PROTECTION;
		pm->damage.otherEnt = -1;
		pm->damage.mod = MOD_HIGH_FORCES;
		return;
	}

	CP_CURRENT_GRAVITY = (pm->ps->gravity / 800.0f) * CP_GRAVITY;

	// set spring strengths for "jump" and "crouch"
// FIXME: change this so it works in cgame and game
#if GAME
	car->springStrength = pm->car_spring * (CP_FRAME_MASS / 350.0f) * CP_GRAVITY;
#else
	car->springStrength = CP_SPRING_STRENGTH;
#endif
//	car->shockStrength = CP_SHOCK_STRENGTH;

	if (pm->cmd.upmove > 0){
		car->springStrength *= 5.0f;
	}
	else if (pm->cmd.upmove < 0){
		car->springStrength /= 5.0f;
	}

	pm->damage.damage = 0;
	pm->damage.otherEnt = -1;

	//t1 = trap_Milliseconds();

	// calculate target positions etc
	// -------------------------------------------------------------------------

	// calculate forces
	PM_CalculateForces(car, &car->sBody, car->sPoints, time);

	//t2 = trap_Milliseconds();

	// apply frame forces to the body
//	for (i = FIRST_FRAME_POINT; i < LAST_FRAME_POINT; i++){
	for (i = FIRST_FRAME_POINT; i < NUM_CAR_POINTS; i++){
		PM_ApplyForce(&car->sBody, car->sPoints[i].netForce, car->sPoints[i].r);
//	}

//	for (i = FIRST_FRAME_POINT; i < NUM_CAR_POINTS; i++){
		if (!car->sPoints[i].onGround) continue;

		PM_CarBodyFrictionForces( car, &car->sBody, car->sPoints, i );

		if (VectorNAN(car->sPoints[i].netForce))
			Com_Printf("Blowing up car because of car point force on frame\n");
	}

	//t3 = trap_Milliseconds();

	// print out the forces
	/*
	if (pm->pDebug > 0 && pm->pDebug <= 8){
		if (pm->client)
			Com_Printf("client sBody -----------------------------------------\n");
		else 
			Com_Printf("server sBody -----------------------------------------\n");
		PM_DebugForces(&car->sBody, car->sPoints);
		PM_DebugDynamics(&car->sBody, car->sPoints);
	}
	*/

	PM_CalculateTargetBody(car, &car->sBody, &car->tBody, car->sPoints, car->tPoints, time);

	//t4 = trap_Milliseconds();

	PM_Trace_Points(car, car->sPoints, car->tPoints, time);

	//t5 = trap_Milliseconds();

	// print out the forces
/*
	if (pm->pDebug > 0 && pm->pDebug <= 8){
		if (pm->client)
			Com_Printf("client tBody -----------------------------------------\n");
		else 
			Com_Printf("server tBody -----------------------------------------\n");
//		PM_DebugForces(&car->tBody, car->tPoints);
		PM_DebugDynamics(&car->tBody, car->tPoints);
	}
*/

	PM_CopyTargetToSource(&car->tBody, &car->sBody, car->tPoints, car->sPoints);

	// clear forces at the end of the frame instead of at the beginning so we
	// can apply forces from nearby explosions between frames
 	PM_ClearCarForces(&car->sBody, car->sPoints);

// #ifdef CGAME
// 	Com_Printf( "numTraces %i\n", numTraces );
// #endif
//	Com_Printf( "t1 %d, t2 %d, t3 %d, t4 %d, t5 %d\n", t1 - t, t2 - t, t3 - t, t4 - t, t5 - t );
}

