#include "qtv.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

void AngleVectors (vec3_t angles, float *forward, float *right, float *up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle = angles[1] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[0] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[2] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	forward[0] = cp*cy;
	forward[1] = cp*sy;
	forward[2] = -sp;
	right[0] = (-1*sr*sp*cy+-1*cr*-sy);
	right[1] = (-1*sr*sp*sy+-1*cr*cy);
	right[2] = -1*sr*cp;
	up[0] = (cr*sp*cy+-sr*-sy);
	up[1] = (cr*sp*sy+-sr*cy);
	up[2] = cr*cp;
}




#define DotProduct(a,b) ((a[0]*b[0]) + (a[1]*b[1]) + (a[2]*b[2]))
#define VectorCopy(a,b) do{b[0]=a[0];b[1]=a[1];b[2]=a[2];}while(0)
#define VectorClear(v) do{v[0]=0;v[1]=0;v[2]=0;}while(0)
#define VectorScale(i,s,o) do{o[0]=i[0]*s;o[1]=i[1]*s;o[2]=i[2]*s;}while(0)
#define Length(v) sqrt(DotProduct(v, v))
#define VectorMA(base,s,m,out) do{out[0]=base[0]+s*m[0];out[1]=base[1]+s*m[1];out[2]=base[2]+s*m[2];}while(0)
#define SHORT2ANGLE(s) ((s*360.0f)/65536)

float VectorNormalize(vec3_t v)
{
	float len, ilen;
	len = Length(v);
	if (len)
	{
		ilen = 1/len;
		v[0] *= ilen;
		v[1] *= ilen;
		v[2] *= ilen;
	}
	return len;
}


void PM_SpectatorMove (pmove_t *pmove)
{
	float	speed, drop, friction, control, newspeed;
	float	currentspeed, addspeed, accelspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;

	// friction

	speed = Length (pmove->velocity);
	if (speed < 1)
	{
		VectorClear (pmove->velocity);
	}
	else
	{
		drop = 0;

		friction = pmove->movevars.friction*1.5;	// extra friction
		control = speed < pmove->movevars.stopspeed ? pmove->movevars.stopspeed : speed;
		drop += control*friction*pmove->frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pmove->velocity, newspeed, pmove->velocity);
	}

	// accelerate
	fmove = pmove->cmd.forwardmove;
	smove = pmove->cmd.sidemove;

	VectorNormalize (pmove->forward);
	VectorNormalize (pmove->right);

	for (i=0 ; i<3 ; i++)
		wishvel[i] = pmove->forward[i]*fmove + pmove->right[i]*smove;
	wishvel[2] += pmove->cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if (wishspeed > pmove->movevars.spectatormaxspeed)
	{
		VectorScale (wishvel, pmove->movevars.spectatormaxspeed/wishspeed, wishvel);
		wishspeed = pmove->movevars.spectatormaxspeed;
	}

	currentspeed = DotProduct(pmove->velocity, wishdir);
	addspeed = wishspeed - currentspeed;

	// Buggy QW spectator mode, kept for compatibility
//	if (pmove->pm_type == PM_OLD_SPECTATOR)
	{
		if (addspeed <= 0)
			return;
	}

	if (addspeed > 0)
	{
		accelspeed = pmove->movevars.accelerate*pmove->frametime*wishspeed;
		if (accelspeed > addspeed)
			accelspeed = addspeed;

		for (i=0 ; i<3 ; i++)
			pmove->velocity[i] += accelspeed*wishdir[i];
	}

	// move
	VectorMA (pmove->origin, pmove->frametime, pmove->velocity, pmove->origin);
}

void PM_PlayerMove (pmove_t *pmove)
{
	pmove->frametime = pmove->cmd.msec * 0.001;
/*
	if (pmove.pm_type == PM_NONE || pmove.pm_type == PM_FREEZE)
	{
		PM_CategorizePosition ();
		return;
	}
*/
	// take angles directly from command
	pmove->angles[0] = pmove->cmd.angles[0];
	pmove->angles[1] = pmove->cmd.angles[1];
	pmove->angles[2] = pmove->cmd.angles[2];

	AngleVectors (pmove->angles, pmove->forward, pmove->right, pmove->up);

//	if (pmove->pm_type == PM_SPECTATOR || pmove->pm_type == PM_OLD_SPECTATOR)
	{
		PM_SpectatorMove (pmove);
//		pmove->onground = false;
		return;
	}
}
