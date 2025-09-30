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


// damage stuff
#define DAMAGE_RADIUS				0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR				0x00000002	// armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK			0x00000004	// do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION		0x00000008  // armor, shields, invulnerability, and godmode have no effect
#ifdef MISSIONPACK
#define DAMAGE_NO_TEAM_PROTECTION	0x00000010  // armor, shields, invulnerability, and godmode have no effect
#endif
#define DAMAGE_WEAPON				0x00000020


// SKWID( car movement code )
//=========================================================================================
#define	WHEEL_FORWARD	31.0f
#define WHEEL_RIGHT		18.0f
#define	WHEEL_UP		-10.0f
// END

#define	BODY_RADIUS		13.0f
#define	WHEEL_RADIUS	8.0f
#define	WHEEL_CIRC		2.0f * M_PI * WHEEL_RADIUS

#define CAR_WIDTH		50.0f
#define CAR_HEIGHT		44.0f
#define CAR_LENGTH		100.0f

extern float CP_CURRENT_GRAVITY;
#define CP_GRAVITY_FTPS		32.2f // gravity in feet per second
#define	CP_GRAVITY			350.0f // gravity in qunits per second

#define CP_FT_2_QU			(CP_GRAVITY / CP_GRAVITY_FTPS) // 10.87
#define CP_FT_2_M			0.3048f
extern	float CP_M_2_QU;
//static	float CP_M_2_QU = CP_FT_2_QU / 0.3048f; // 35.66

#define	CP_FRAME_MASS		300.0f // 350
#define	CP_WHEEL_MASS		50.0f // 100
#define	CP_CAR_MASS			( CP_FRAME_MASS * 4 + CP_WHEEL_MASS * 4 )

#define	CP_BODY_ELASTICITY	0.05f
#define	CP_WHEEL_ELASTICITY	0.05f

#define	CP_SPRING_MINLEN	5.0f
// #define	CP_SPRING_MAXLEN	10.0f
#define	CP_SPRING_MAXLEN	15.0f

#define	CP_MAX_SHOCK_FORCE	400000.0f

// Used to fit the spring lengths into the 8 bit numbers that
// are used to transfer them over the net.
// Should be less than 255 / ( CP_SPRING_MAXLEN - CP_SPRING_MINLEN )
#define	CP_SPRING_SCALE		24.0f


// strength of the fake spring that returns the wheel to perpendicular
extern	float CP_WR_STRENGTH;
extern	float CP_SPRING_STRENGTH;
extern	float CP_SHOCK_STRENGTH;
extern	float CP_SWAYBAR_STRENGTH;
/*
extern	float CP_TORQUE_SLOPE;
extern	float CP_GEAR_RATIOS[];
*/

#define	CP_AIR_COF			0.31f
#define	CP_FRAC_TO_DF		0.50f
#define CP_ENGINE_TIRE_COF	1000.0f

#define	CP_SCOF				1.5f		// dry asphalt
#define	CP_KCOF				1.0f
#define	CP_ICE_SCOF			0.3f		// ice
#define	CP_ICE_KCOF			0.2f
#define	CP_DIRT_SCOF		1.2f		// loose dirt
#define	CP_DIRT_KCOF		0.8f
#define	CP_GRASS_SCOF		1.05f		// short grass
#define	CP_GRASS_KCOF		0.7f
#define CP_GRAVEL_SCOF      1.22f       // gravel
#define CP_GRAVEL_KCOF      0.85f
#define	CP_SNOW_SCOF		0.4f		// packed snow
#define	CP_SNOW_KCOF		0.27f
//#define	CP_SNOW_SCOF		0.7f		// packed snow
//#define	CP_SNOW_KCOF		0.5f
#define CP_SAND_SCOF        0.75f
#define CP_SAND_KCOF        0.6f
#define	CP_OIL_SCOF			0.3f		// oil
#define	CP_OIL_KCOF			0.2f

#define	CP_WET_SCALE		0.75f

#define	CP_AIR_DENSITY		1.185f // 1.288075f
#define	CP_WATER_DENSITY	1000.0f
#define	CP_LAVA_DENSITY		60000.0f
#define	CP_SLIME_DENSITY	20000.0f

#define	CP_AXLEGEAR			3.07f
#define	CP_GEARR			-2.80f
#define	CP_GEARN			0.00f
/*
#define	CP_GEAR1			2.82f
#define	CP_GEAR2			1.78f
#define	CP_GEAR3			1.27f
#define	CP_GEAR4			0.92f
#define	CP_GEAR5			0.60f

#define	CP_GEAR1			3.02f
#define	CP_GEAR2			2.01f
#define	CP_GEAR3			1.34f
#define	CP_GEAR4			0.89f
#define	CP_GEAR5			0.60f
*/
#define	CP_GEAR1			2.75f
#define	CP_GEAR2			1.67f
#define	CP_GEAR3			1.19f
#define	CP_GEAR4			0.82f
#define	CP_GEAR5			0.49f


#define	CP_RPM_MAX	      6250
#define	CP_RPM_MIN			  1000

// #define	CP_HP_PEAK			191.0f
// #define	CP_RPM_HP_PEAK		4600.0f
#define	CP_HP_PEAK			320.0f
#define	CP_RPM_HP_PEAK		5000.0f
// #define	CP_TORQUE_PEAK		230.0f
// #define	CP_RPM_TORQUE_PEAK	3800.0f
#define	CP_TORQUE_PEAK		400.0f
#define	CP_RPM_TORQUE_PEAK	2800.0f

#define HTYPE_NO_HIT		0
#define HTYPE_BOTTOMED_OUT	1
#define HTYPE_MAXED_OUT		2

#define CF_REVERSE			1
#define CF_BRAKE			2

// assign sign bit of b to a, floats only
// #define SetSign(a,b)		(*(DWORD *)&(a)) = ((*(DWORD *)&(b)) & 0x80000000) | ((*(DWORD *)&(a)) & 0x7FFFFFFF)

typedef enum {
	GRAVITY,
	NORMAL,
	SHOCK,
	SPRING,
	SWAY_BAR,
	ROAD,
	INTERNAL,
	AIR_FRICTION,
	NUM_CAR_FORCES
} carForces_t;


#define FIRST_FW_POINT	0
#define LAST_FW_POINT	2
#define FIRST_RW_POINT	2
#define LAST_RW_POINT	4

#define FIRST_FRAME_POINT	4
#define LAST_FRAME_POINT	8
typedef enum {
	// these are the points that store wheel info
	FL_WHEEL,
	FR_WHEEL,
	RL_WHEEL,
	RR_WHEEL,

	// these points hold position info of the suspension used in suspension force calculations
	FL_FRAME,
	FR_FRAME,
	RL_FRAME,
	RR_FRAME,

	// these are used for collision detection of the body of the car
	FL_BODY,
	FR_BODY,
	ML_BODY,
	MR_BODY,
	RL_BODY,
	RR_BODY,
	
	ML_ROOF,
	MR_ROOF,
	
	NUM_CAR_POINTS
} carPointNumbers_t;

typedef struct {
	vec3_t	r;
	vec3_t	v;
//	vec3_t	lastNoZeroVelocity;

	float	w;

	vec3_t	forces[NUM_CAR_FORCES];
	vec3_t	netForce;
	float	netMoment;

	vec3_t	normals[3];
						 
	float	mass;
	float	elasticity;
	float	radius;

	float	kcof;
	float	scof;

	float	fluidDensity;

	int		onGroundTime;
	int		offGroundTime;

	qboolean	onGround;
	qboolean	slipping;
} carPoint_t;


typedef struct {
	float	damage;
	int		mod;
	int		dflags;
	int		otherEnt;
	vec3_t	origin;
	vec3_t	dir;
} collisionDamage_t;


typedef struct {
	float	inverseWorldInertiaTensor[3][3];

	vec3_t	r;
	vec3_t	v;

//	vec4_t	q;
	float	t[3][3];

	vec3_t	w;
	vec3_t	L;

	vec3_t	netForce;
	vec3_t	netMoment;
//	vec3_t	normalForce;

	vec3_t	CoM;

	// FIXME: use the t components instead of these
	vec3_t	forward;
	vec3_t	right;
	vec3_t	up;

	float	curSpringLengths[FIRST_FRAME_POINT];

	float	mass;
	float	elasticity;
} carBody_t;


typedef struct {
	vec3_t	v;
	vec3_t	netForce;
	float	netMoment;
} pointHistory_t;


typedef struct {
	vec3_t	v;
	vec3_t	w;
	vec3_t	netForce;
	vec3_t	netMoment;
} bodyHistory_t;


typedef struct {
	float		inverseBodyInertiaTensor[3][3];

	carPoint_t	sPoints[NUM_CAR_POINTS];
	carBody_t	sBody;

	carPoint_t	tPoints[NUM_CAR_POINTS];
	carBody_t	tBody;

//	pointHistory_t	oldPoints[3][LAST_RW_POINT];
//	bodyHistory_t	oldBodies[3];

	// FIXME: remove these to save memory if i can

	float	springStrength;
	float	springMaxLength;
	float	springMinLength;
//	float	shockStrength;
//	float	swayBarStrength;

	float	wheelAngle;
	float	throttle;

	int		wheelOnGroundTime;
	int		onGroundTime;

	int		wheelsOffGroundTime;
	int		offGroundTime;

	int		gear; // -1 reverse, 0 neutral, 1+ forward gears
	float	rpm;

	qboolean	initializeOnNextMove;

//	float	aCOF;
//	float	dfCOF;
//	float	ewCOF;
} car_t;

// internal functions that are in different files
void PM_AddRoadForces(car_t *car, carBody_t *body, carPoint_t *points, float sec);
void PM_CalculateNetForce( carPoint_t *point, int pointIndex );
void PM_DriveMove( car_t *car, float time, qboolean includeBodies );

// used externally
void PM_ApplyForce( carBody_t *body, vec3_t force, vec3_t at );
void PM_InitializeVehicle( car_t *car, vec3_t origin, vec3_t angles, vec3_t velocity );
void PM_CalculateSecondaryQuantities( car_t *car, carBody_t *body, carPoint_t *points );
void PM_SetCoM( carBody_t *body, carPoint_t *points );
