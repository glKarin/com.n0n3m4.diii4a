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
//#include "../cgame/cg_local.h"
#endif
#include "bg_public.h"
#include "bg_local.h"


static float CP_TORQUE_SLOPE = (float)(CP_RPM_HP_PEAK * M_PI * CP_TORQUE_PEAK - 16500 * CP_HP_PEAK) / (float)(CP_RPM_HP_PEAK * M_PI * (CP_RPM_HP_PEAK*CP_RPM_HP_PEAK - 2 * CP_RPM_HP_PEAK * CP_RPM_TORQUE_PEAK + CP_RPM_TORQUE_PEAK*CP_RPM_TORQUE_PEAK));
static float CP_GEAR_RATIOS[] = {CP_GEAR1, CP_GEAR2, CP_GEAR3, CP_GEAR4, CP_GEAR5};


#if 0
/*
===================
PM_RPMtoWheelSpeed
===================
*/
static float PM_RPMtoWheelSpeed( car_t *car ){
	float	ratio;

	if (car->gear < 0)
		ratio = CP_GEARR;
	else if (car->gear == 0)
		ratio = CP_GEARN;
	else
		ratio = CP_GEAR_RATIOS[car->gear-1];

//	return (-(car->rpm-CP_RPM_MIN) * M_PI / 30) / (ratio * CP_AXLEGEAR);
	return (-(car->rpm) * M_PI / 30) / (ratio * CP_AXLEGEAR);
}
#endif


/*
===================
PM_WheelSpeedtoRPM
===================
*/
static float PM_WheelSpeedtoRPM( car_t *car, carPoint_t *points ){
	float	ratio, w;
	int		i;

	if (car->gear < 0)
		ratio = CP_GEARR;
	else if (car->gear == 0)
		ratio = CP_GEARN;
	else
		ratio = CP_GEAR_RATIOS[car->gear-1];

	w = 0;
	if (car->gear >= 0){
		for (i = 0; i < FIRST_FRAME_POINT; i++){
			w = min(w, points[i].w);
		}
	}
	else {
		for (i = 0; i < FIRST_FRAME_POINT; i++){
			w = max(w, points[i].w);
		}
	}

//	return (-w / M_PI * 30) * (ratio * CP_AXLEGEAR) + CP_RPM_MIN;
	return (-w / M_PI * 30) * (ratio * CP_AXLEGEAR);
}


/*
================================================================================
PM_UpdateRPM
================================================================================
*/
static void PM_UpdateRPM(car_t *car, carPoint_t *points){
	float	rpmTemp;
	float	shiftDownRPM, shiftUpRPM;

	shiftDownRPM = CP_RPM_MIN + (CP_RPM_MAX - CP_RPM_MIN) * (0.4f + 0.2f * car->throttle);
	shiftUpRPM = CP_RPM_MIN + (CP_RPM_MAX - CP_RPM_MIN) * (0.8f + 0.2f * car->throttle);

//  Com_Printf("shiftDownRPM: %f shiftUpRPM: %f\n", shiftDownRPM, shiftUpRPM);

	if ( shiftUpRPM > CP_RPM_MAX )
		shiftUpRPM = CP_RPM_MAX;

	if (car->gear > 0){
		rpmTemp = PM_WheelSpeedtoRPM(car, points);

//	Com_Printf("1 Gear: %i RPM temp: %f\n", car->gear, rpmTemp);

		while ( rpmTemp < shiftDownRPM ){
			if (car->gear > 1)
				car->gear--;
			else if ( rpmTemp < CP_RPM_MIN ){
				rpmTemp = CP_RPM_MIN;
				break;
			}
			else
				break;

			rpmTemp = PM_WheelSpeedtoRPM(car, points);
			if (rpmTemp > CP_RPM_MAX)
				rpmTemp = CP_RPM_MAX;
		}

//		Com_Printf("2 Gear: %i RPM temp: %f\n", car->gear, rpmTemp);

		while ( rpmTemp > shiftUpRPM ){
			if ( !points[2].onGround || !points[3].onGround || points[2].slipping || points[3].slipping ){
				if ( rpmTemp > CP_RPM_MAX ){
					rpmTemp = CP_RPM_MAX;
					break;
				}
				else if ( rpmTemp > shiftUpRPM )
					break;
			}

			if (car->gear < 5){
				if ( points[2].onGround && points[3].onGround && !points[2].slipping && !points[3].slipping )
					car->gear++;
			}
			else if ( rpmTemp > CP_RPM_MAX ){
				rpmTemp = CP_RPM_MAX;
				break;
			}

			rpmTemp = PM_WheelSpeedtoRPM(car, points);
			if (rpmTemp < CP_RPM_MIN)
				rpmTemp = CP_RPM_MIN;
		}

		car->rpm = rpmTemp;
	}
	else if (car->gear == 0){
		car->rpm = CP_RPM_MIN;
	}
	else {
		rpmTemp = PM_WheelSpeedtoRPM(car, points);
		if (rpmTemp < CP_RPM_MIN)
			rpmTemp = CP_RPM_MIN;
		if (rpmTemp > CP_RPM_MAX)
			rpmTemp = CP_RPM_MAX;

		car->rpm = rpmTemp;
	}
}

/*
================================================================================
PM_AirFrictionForces
================================================================================
*/
static void PM_AirFrictionForces( car_t *car, carBody_t *body, carPoint_t *points, float sec ){
	vec3_t		dir, force;
	float		v, friction, area;
	int			i;

	area = fabs((float)(CAR_HEIGHT * CAR_WIDTH) / (float)(CP_M_2_QU*CP_M_2_QU));

	// dont do air friction on tires
	for (i = FIRST_FRAME_POINT; i < NUM_CAR_POINTS; i++){
		v = VectorNormalize2(points[i].v, dir);

		if (fabs(v) < 0.01f) continue;

		v /= CP_M_2_QU; // m / s

		friction = -0.5 * pm->car_air_cof * area * points[i].fluidDensity * v * v / (float)(NUM_CAR_POINTS);

		friction *= CP_M_2_QU; // to qforce
//		friction = 0;

//		Com_Printf("air friction: %0.3f\n", friction);

		// FIXME: fix it so that it doesnt apply too much force and send the car into space
		// try this
		/*
		if (fabs(friction * NUM_CAR_POINTS / body->mass * sec) > VectorLength(points[i].v)){
			Com_Printf("PM_AirFrictionForces: too much force\n");
			friction = -VectorLength(points[i].v) / sec * body->mass / (float)NUM_CAR_POINTS;
		}
		*/

		VectorScale(dir, friction, force);

		// add down force
		VectorMA(force, -fabs(pm->car_air_frac_to_df * DotProduct(force, body->forward)), body->up, force);
//		Com_Printf("down force: %0.3f\n", -fabs(CP_FRAC_TO_DF * DotProduct(force, body->forward)) / 4.0f);

		VectorAdd(points[i].forces[AIR_FRICTION], force, points[i].forces[AIR_FRICTION]);
	}
}


/*
================================================================================
PM_GroundFrictionForces
================================================================================
*/
static void PM_GroundFrictionForces( car_t *car, carPoint_t *points, int i, vec3_t forward, vec3_t right, vec3_t up, float sec ){
	//float	friction, dot;
	float	normalForce, sideForce;
	vec3_t	accel, targetV, force;

	float	skid, slipangle, forwardForce;
	//float	v, angle, n2, v2;

	normalForce = VectorLength(points[i].forces[NORMAL]);

	if( normalForce < 0.01f )
		return;

	// we want the accel vector parallel to the ground
	VectorScale(forward, points[i].w * WHEEL_RADIUS, targetV);
	VectorAdd(targetV, points[i+4].v, accel);
	VectorInverse(accel);
	VectorMA(accel, -DotProduct(accel, up), up, accel);

//	Com_Printf("Accel: %f\n", DotProduct(accel, forward));

	// FIXME: check if this equation is right
//	noSlipForce = ((2.0 / 3.0) * points[i].mass * DotProduct(accel, forward) / sec) - (points[i].netMoment / (3.0 * WHEEL_RADIUS));
//	noSlipForce = (((1.0 / 3.0) * (points[i].mass) * DotProduct(accel, forward) / sec) - ((2.0 / 3.0) * points[i].netMoment / WHEEL_RADIUS));

	// forces required to keep up with the acceleration
	forwardForce = (1.0f / 10.0f) * points[i].mass * DotProduct(accel, forward) / sec;
	sideForce = (1.0f / 10.0f) * points[i].mass * DotProduct(accel, right) / sec;

/*
	friction = normalForce * (!points[i].slipping ? points[i].scof : points[i].kcof);

	if (pm->ps->powerups[PW_TURBO] > 0)
		friction *= 2.0f;

	sideForce = (points[i].mass + points[i+4].mass) * DotProduct(accel, right) / (sec * 4);

	VectorNormalize2(accel, force);
	VectorScale(force, friction, force);

	dot = DotProduct(force, forward);
	if ((dot < 0 && noSlipForce >= dot) || (dot > 0 && noSlipForce <= dot)){
//		Com_Printf("static friction, noSlipForce %f\n", noSlipForce);
//		Com_Printf("netForce forward %f\n", DotProduct(points[i].netForce, forward));
//		Com_Printf("accel forward %f\n", DotProduct(accel, forward));
		VectorMA(force, noSlipForce - dot, forward, force);
//		Com_Printf("force forward %f\n", DotProduct(force, forward));
		points[i].slipping = qfalse;
	}
	else if ((dot < 0 && noSlipForce < dot * 10.0f) || (dot > 0 && noSlipForce > dot * 10.0f)){
		points[i].slipping = 2;
	}
	else{
		points[i].slipping = qtrue;
	}

	// add extra side friction because the car slips too much
	if (sideForce > normalForce * points[i].scof)
		sideForce = normalForce * points[i].scof; // * 0.9f
	if (sideForce < -normalForce * points[i].scof)
		sideForce = -normalForce * points[i].scof; // * 0.9f
	dot = DotProduct(force, right);
	VectorMA(force, sideForce - dot, right, force);
*/

// test my new physics book
	points[i].slipping = qfalse;
	VectorClear(force);

	// cornering forces
	slipangle = 100 * sideForce / (8.0f * normalForce * points[i].scof); // tire slip at about 20% skid
	if (fabs(slipangle) >= 30.0f)
		points[i].slipping = qtrue;
	else
		sideForce = 1.3f * points[i].scof * 0.97f * normalForce * sin(1.50*atan2(.276208*slipangle-.132*atan2(.244*slipangle, 1), 1));

	// accel forces
	skid = 100 * forwardForce / (5.0f * normalForce * points[i].scof); // tire slip at about 20% skid
	if (fabs(skid) >= 30.0f || points[i].slipping)
		points[i].slipping = qtrue;
	else
		forwardForce = points[i].scof * 1.1f * normalForce * sin(1.55*atan2(.101104*skid+.432*atan2(.178*skid, 1), 1));

	if (points[i].slipping)
	{
		VectorNormalize(accel);
		VectorMA(force, points[i].kcof * normalForce, accel, force);
	}
	else
	{
		// accel force
		VectorMA(force, forwardForce, forward, force);

		// cornering force
//		dot = DotProduct(force, right);
		VectorMA(force, sideForce, right, force);
	}

	points[i].netMoment += (DotProduct(force, forward) * WHEEL_RADIUS);

	VectorAdd(points[i].forces[ROAD], force, points[i].forces[ROAD]);

	// static friction
/*
	if (!points[i].slipping){
		vec3_t	down = {0, 0, -1};
		vec3_t	normal;

		dot = DotProduct(points[i].normals[0], down);
		if (dot != -1){
			Com_Printf("Applying static friction\n");

			VectorMA(points[i].normals[0], -dot, down, normal);
			VectorNormalize(normal); // gives vector in the direction of sliding

			dot = DotProduct(normal, accel);
		}
	}
*/
}


/*
================================================================================
PM_TireFrictionForces
================================================================================
*/
static void PM_TireFrictionForces( car_t *car, carPoint_t *points, int i, vec3_t forward, float sec ){
	float	torque;

	if (fabs(points[i].w) <= 0.001f)
		return;

	torque = -points[i].w * CP_ENGINE_TIRE_COF;
	points[i].netMoment += torque;
/*
	Com_Printf("PM_TireFrictionForces: torque1 %.3f\n", torque);

	torque = 0;
	if (car->throttle < 0.01f)
		torque = (car->rpm - 1000) * 5;
	if (points[i].w > 0.0f)
		torque *= -1.0f;

	Com_Printf("PM_TireFrictionForces: torque2 %.3f\n", torque);

	points[i].netMoment += torque;
*/
}


/*
	FPtoT = 1.355; // Foot-Pounds to Newton*Meter
	// 1 Hp = 0.74667 kW
	// Horsepower = torque * revs/minute * minute/60 s * 2*pi * 1/550
	// Horsepower = torque * revs/minute * 1/5252

	// Tout = Tin * Rout / Rin
	// RPMout = RPMin * Cin / Cout = RPMin * Rin / Rout
	// Rout/Rin = gear ratio

	// MPH = Engine RPM / Ratio * Diameter * pi * 5 / 5280
	// Engine RPM = MPH * Ratio / Diameter / pi / 5 * 5280
	// where Ratio is the gear reduction, Diameter is the tire diameter in inches. 

	// power needed to overcome resistances in (kW)
	// pN = v^3 * Cd * A / 76716 + roll resistance
*/

/*
================================================================================
PM_TireEngineForces
================================================================================
*/
static void PM_TireEngineForces( car_t *car, carPoint_t *points, int i, vec3_t forward ){
	float	torque, ratio, relrpm, friction;
//	float	power;
//	float	normalForce;

	if (car->throttle < 0.00f)
		return;

	if (VectorLength(forward) == 0.0f){
		if (pm->pDebug)
			Com_Printf("PM_TireEngineForces: invalid forward vector\n");
		return;
	}

	if (car->rpm >= CP_RPM_MAX){
		// just add enough torque to stay at constant speed
		return;
//		points[i].w = PM_RPMtoWheelSpeed(car);
//		points[i].netMoment += torque;
	}


	relrpm = (car->rpm - CP_RPM_TORQUE_PEAK);
	torque = car->throttle * ((-1.0f * CP_TORQUE_SLOPE * relrpm * relrpm) + CP_TORQUE_PEAK); // ft.lb
//	power = torque * car->rpm / (30 * 550 / M_PI); // hp

	if (car->gear < 0)
		ratio = CP_GEARR;
	else if (car->gear == 0)
		ratio = CP_GEARN;
	else
		ratio = CP_GEAR_RATIOS[car->gear-1];

	friction = 0;
	if (fabs(car->throttle < 0.01f) && car->gear)
		friction = (CP_M_2_QU * CP_M_2_QU * (car->rpm - CP_RPM_MIN) / 10.0f / ratio);// frictional torque

	ratio *= CP_AXLEGEAR;

	torque *= 1.355818f; // Nm = kg*m^2/s^2
	torque *= -ratio;
	if (i < 2)
		torque *= CP_M_2_QU * CP_M_2_QU / 6.0f; // qu
	else
		torque *= CP_M_2_QU * CP_M_2_QU / 3.0f; // qu

	torque += friction;

	if (pm->ps->powerups[PW_TURBO] > 0){
		torque *= 4.5f;
//		car->sPoints[0].scof *= 2.0f; // need to be able to set this back to normal
//		car->sPoints[0].kcof *= 2.0f;
	}

	points[i].netMoment += torque;
}


/*
================================================================================
PM_TireBrakingForces
================================================================================
*/
static void PM_TireBrakingForces( car_t *car, carPoint_t *points, int i, vec3_t forward, float throttle ){
	float	torque;
	float	normalForce;

	if (throttle >= -0.01f)
		return;

	if (VectorLength(forward) == 0.0f){
		if (pm->pDebug)
			Com_Printf("PM_TireBrakingForces: invalid forward vector\n");
		return;
	}

	normalForce = CP_CURRENT_GRAVITY * (CP_FRAME_MASS + CP_WHEEL_MASS);
	torque = throttle * normalForce * CP_SCOF * 0.6f * WHEEL_RADIUS;

	if (points[i].w < 0.0f)
		torque *= -1;

	if (fabs(points[i].w) < 6.0f)
		torque *= fabs(points[i].w) / 6.0f;

	points[i].netMoment += torque;
}


/*
================================================================================
PM_AddRoadForces
================================================================================
*/
void PM_AddRoadForces(car_t *car, carBody_t *body, carPoint_t *points, float sec){
	vec3_t		temp;
	vec3_t		forward, right, up;
	float		v, targetAngle;
	int			i;

	v = DotProduct(body->v, body->forward);

	if (pm->ps->stats[STAT_HEALTH] > 0){
		car->throttle = pm->cmd.forwardmove / 127.0F;

		if (!pm->manualShift){
			if (car->gear < 0)
				car->throttle *= -1.0f;

			if (car->throttle < 0){
				if (car->gear > 0 && v < 40.0f){
					car->gear = -1;
					car->throttle *= -1.0f;
				}
				else if (car->gear < 0 && v > -40.0f){
					car->gear = 1;
					car->throttle *= -1.0f;
				}
			}
		}

		if (pm->controlMode == CT_MOUSE){
			car->wheelAngle = WheelAngle(pm->ps->viewangles[YAW], pm->ps->damageAngles[YAW]);

			if (v < 0.5f && v > -0.5f)
				car->wheelAngle = 0.0;
			else if ( car->gear < 0 )
				car->wheelAngle *= -1.0;
/*
			if (v < 0.5f && v > -0.5f)
				car->wheelAngle = 0.0;
			else if (v < -0.5f)
				car->wheelAngle *= -1.0;
*/
		}
		else {
			targetAngle = pm->cmd.rightmove / 127.0F * 30.0f;

			if( car->wheelAngle > 0 && targetAngle < car->wheelAngle )
			{
				if ( fabs(car->wheelAngle - targetAngle) < fabs(90.0f * sec) )
					car->wheelAngle = targetAngle;
				else
					car->wheelAngle -= 90.0f * sec;
			}
			else if ( car->wheelAngle < 0 && targetAngle > car->wheelAngle )
			{
				if ( fabs(car->wheelAngle - targetAngle) < fabs(90.0f * sec) )
					car->wheelAngle = targetAngle;
				else
					car->wheelAngle += 90.0f * sec;
			}
			else if (car->wheelAngle != targetAngle){
				if (fabs(car->wheelAngle - targetAngle) < fabs(75.0f * sec / (1 + fabs(v) / 800.0f)))
					car->wheelAngle = targetAngle;
				else if (car->wheelAngle > targetAngle)
					car->wheelAngle -= 75.0f * sec / (1 + fabs(v) / 800.0f);
				else if (car->wheelAngle < targetAngle)
					car->wheelAngle += 75.0f * sec / (1 + fabs(v) / 800.0f);
			}

			if (car->wheelAngle > 20.0f)
				car->wheelAngle = 20.0f;
			if (car->wheelAngle < -20.0f)
				car->wheelAngle = -20.0f;

			pm->ps->damageAngles[PITCH] = 0.0f;
			pm->ps->damageAngles[YAW] = car->wheelAngle;

			pm->ps->damagePitch = ANGLE2BYTE(pm->ps->damageAngles[PITCH]);
			pm->ps->damageYaw = ANGLE2BYTE(pm->ps->damageAngles[YAW]);
		}
	}
	else {
		car->throttle = 0.0f;
	}

	// used for drawing car clientside
	if (car->gear < 0)
		pm->ps->extra_eFlags |= CF_REVERSE;
	else
		pm->ps->extra_eFlags &= ~CF_REVERSE;

	if (car->throttle < 0)
		pm->ps->extra_eFlags |= CF_BRAKE;
	else
		pm->ps->extra_eFlags &= ~CF_BRAKE;

	PM_UpdateRPM(car, points);

	PM_AirFrictionForces(car, body, points, sec);

	// front tires
	for (i = FIRST_FW_POINT; i < LAST_FW_POINT; i++)
	{
		// calculate net forces
		PM_CalculateNetForce(&points[i], i);

		if ( points[i].onGround )
		{
			VectorCopy(points[i].normals[0], up);
			// sometimes up is a zero vector when it shouldnt be so just
			// assume its not supposed to be
			if( up[0] == 0.0f && up[1] == 0.0f && up[2] == 0.0f )
				up[2] = 1.0f;
			CrossProduct(body->forward, up, temp);
		}
		else {
			VectorCopy(body->up, up);
			VectorCopy(body->right, temp);
		}

		RotatePointAroundVector(right, up, temp, -car->wheelAngle);
		VectorNormalize(right);
		CrossProduct(up, right, forward);

		PM_TireEngineForces(car, points, i, forward);

		PM_TireBrakingForces(car, points, i, forward, car->throttle);

		PM_TireFrictionForces(car, points, i, forward, sec);

		if (!points[i].onGround) continue;

		PM_GroundFrictionForces(car, points, i, forward, right, up, sec);
	}

	for (i = FIRST_RW_POINT; i < LAST_RW_POINT; i++)
	{
		// calculate net forces
		PM_CalculateNetForce(&points[i], i);

		if (points[i].onGround){
			VectorCopy(points[i].normals[0], up);
			CrossProduct(body->forward, up, right);
			CrossProduct(up, right, forward);
		}
		else {
			VectorCopy(body->forward, forward);
			VectorCopy(body->right, right);
			VectorCopy(body->up, up);
		}

		PM_TireEngineForces(car, points, i, forward);

		PM_TireBrakingForces(car, points, i, forward, car->throttle);

		PM_TireFrictionForces(car, points, i, forward, sec);

		if (!points[i].onGround) continue;

		if (pm->cmd.buttons & BUTTON_HANDBRAKE){
			points[i].w = 0;
			points[i].netMoment = 0;
		}

		PM_GroundFrictionForces(car, points, i, forward, right, up, sec);
	}

	// calculate net forces
	for (i = 0; i < FIRST_FRAME_POINT; i++){
		PM_CalculateNetForce(&points[i], i);
	}
}
