/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// mathlib.c -- math primitives

#include "quakedef.h"
#include <math.h>

vec3_t vec3_origin = {0,0,0};

/*-----------------------------------------------------------------*/

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F

static void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct( normal, normal );

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabs( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

#ifdef _MSC_VER
#pragma optimize( "", off )
#endif


void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
{
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int	i;
	vec3_t vr, vup, vf;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	zrot[0][0] = cos( DEG2RAD( degrees ) );
	zrot[0][1] = sin( DEG2RAD( degrees ) );
	zrot[1][0] = -sin( DEG2RAD( degrees ) );
	zrot[1][1] = cos( DEG2RAD( degrees ) );

	R_ConcatRotations( m, zrot, tmpmat );
	R_ConcatRotations( tmpmat, im, rot );

	for ( i = 0; i < 3; i++ )
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif

/*-----------------------------------------------------------------*/

float	anglemod(float a)
{
#if 0
	if (a >= 0)
		a -= 360*(int)(a/360);
	else
		a += 360*( 1 + (int)(-a/360) );
#endif
	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int VARGS BoxOnPlaneSide (const vec3_t emins, const vec3_t emaxs, const mplane_t *p)
{
	float	dist1, dist2;
	int		sides;

#if 0	// this is done by the BOX_ON_PLANE_SIDE macro before calling this
		// function
// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}
#endif
	
// general case
	switch (p->signbits)
	{
	default:
	case 0:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	}

#if 0
	int		i;
	vec3_t	corners[2];

	for (i=0 ; i<3 ; i++)
	{
		if (plane->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist = DotProduct (plane->normal, corners[0]) - plane->dist;
	dist2 = DotProduct (plane->normal, corners[1]) - plane->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

#endif

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

#ifdef PARANOID
if (sides == 0)
	Sys_Error ("BoxOnPlaneSide: sides==0");
#endif

	return sides;
}




static void VVPerpendicularVector(vec3_t dst, const vec3_t src)
{
	if (!src[0] && !src[1])
	{
		if (src[2])
			dst[1] = -1;
		else
			dst[1] = 0;
		dst[0] = dst[2] = 0;
	}
	else
	{
		dst[0] = src[1];
		dst[1] = -src[0];
		dst[2] = 0;
		VectorNormalize(dst);
	}
}
void VectorVectors(const vec3_t forward, vec3_t right, vec3_t up)
{
	VVPerpendicularVector(right, forward);
	CrossProduct(right, forward, up);
}

void QDECL VectorAngles(const float *forward, const float *up, float *result, qboolean meshpitch)	//up may be NULL
{
	float	yaw, pitch, roll;	

	if (forward[1] == 0 && forward[0] == 0)
	{
		if (forward[2] > 0)
		{
			pitch = -M_PI * 0.5;
			yaw = up ? atan2(-up[1], -up[0]) : 0;
		}
		else
		{
			pitch = M_PI * 0.5;
			yaw = up ? atan2(up[1], up[0]) : 0;
		}
		roll = 0;
	}
	else
	{
		yaw = atan2(forward[1], forward[0]);
		pitch = -atan2(forward[2], sqrt (forward[0]*forward[0] + forward[1]*forward[1]));

		if (up)
		{
			vec_t cp = cos(pitch), sp = sin(pitch);
			vec_t cy = cos(yaw), sy = sin(yaw);
			vec3_t tleft, tup;
			tleft[0] = -sy;
			tleft[1] = cy;
			tleft[2] = 0;
			tup[0] = sp*cy;
			tup[1] = sp*sy;
			tup[2] = cp;
			roll = -atan2(DotProduct(up, tleft), DotProduct(up, tup));
		}
		else
			roll = 0;
	}

	pitch *= 180 / M_PI;
	yaw *= 180 / M_PI;
	roll *= 180 / M_PI;
	if (meshpitch)
	{
		pitch *= r_meshpitch.value;
		roll *= r_meshroll.value;
	}
	if (pitch < 0)
		pitch += 360;
	if (yaw < 0)
		yaw += 360;
	if (roll < 0)
		roll += 360;

	result[0] = pitch;
	result[1] = yaw;
	result[2] = roll;
}

void QDECL AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	
	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}
void AngleVectorsMesh (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	vec3_t ang;
	ang[0] = angles[0] * r_meshpitch.value;
	ang[1] = angles[1];
	ang[2] = angles[2] * r_meshroll.value;
	AngleVectors (ang, forward, right, up);
}

int VectorCompare (const vec3_t v1, const vec3_t v2)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
		if (v1[i] != v2[i])
			return 0;
			
	return 1;
}
int Vector4Compare (const vec4_t v1, const vec4_t v2)
{
	int		i;
	
	for (i=0 ; i<4 ; i++)
		if (v1[i] != v2[i])
			return 0;
			
	return 1;
}
/*
void _VectorMA (const vec3_t veca, const float scale, const vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct (vec3_t v1, vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}
*/
void CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

vec_t Length(const vec3_t v)
{
	int		i;
	float	length;
	
	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);		// FIXME

	return length;
}

float Q_rsqrt(float number)
{
	int i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * (int *) &y;						// evil floating point bit level hacking
	i  = 0x5f3759df - (i >> 1);               // what the fuck?
	y  = * (float *) &i;
	y  = y * (threehalfs - (x2 * y * y));   // 1st iteration
//	y  = y * (threehalfs - (x2 * y * y));   // 2nd iteration, this can be removed

	return y;
}

float QDECL VectorNormalize (vec3_t v)
{
	float	length;
	float	ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);		// FIXME

	if (length)
	{
		ilength = 1.0/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;
}

void VectorNormalizeFast(vec3_t v)
{
	float ilength;

	ilength = Q_rsqrt(DotProduct(v, v));

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}

void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}


int Q_log2(int val)
{
	int answer=0;
	while ((val>>=1) != 0)
		answer++;
	return answer;
}


/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations (float in1[3][3], float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

/*
================
R_ConcatTransforms
================
*/
void QDECL R_ConcatTransforms (const float in1[3][4], const float in2[3][4], float out[3][4])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
				in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
				in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
				in1[2][2] * in2[2][3] + in1[2][3];
}

//R_ConcatTransforms where there's no offset values, and a transposed axis
void R_ConcatTransformsAxis (const float in1[3][3], const float in2[3][4], float out[3][4])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[1][0] * in2[1][0] +
				in1[2][0] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[1][0] * in2[1][1] +
				in1[2][0] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[2][0] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[1][1] * in2[1][3] +
				in1[2][0] * in2[2][3];
	out[1][0] = in1[0][1] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[2][1] * in2[2][0];
	out[1][1] = in1[0][1] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[2][1] * in2[2][1];
	out[1][2] = in1[0][1] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[2][1] * in2[2][2];
	out[1][3] = in1[0][1] * in2[0][3] + in1[1][1] * in2[1][3] +
				in1[2][1] * in2[2][3];
	out[2][0] = in1[0][2] * in2[0][0] + in1[1][2] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[0][2] * in2[0][1] + in1[1][2] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[0][2] * in2[0][2] + in1[1][2] * in2[1][2] +
				in1[2][2] * in2[2][2];
	out[2][3] = in1[0][2] * in2[0][3] + in1[1][2] * in2[1][3] +
				in1[2][2] * in2[2][3];
}

//R_ConcatTransforms where we don't care about the resulting offsets.
void R_ConcatRotationsPad (float in1[3][4], float in2[3][4], float out[3][4])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];

	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];

	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
}

void Matrix3x4_Multiply(const float *a, const float *b, float *out)
{
	out[0]  = a[0] * b[0] + a[4] * b[1] + a[8] * b[2];
	out[1]  = a[1] * b[0] + a[5] * b[1] + a[9] * b[2];
	out[2]  = a[2] * b[0] + a[6] * b[1] + a[10] * b[2];
	out[3]  = a[3] * b[0] + a[7] * b[1] + a[11] * b[2] + b[3];

	out[4]  = a[0] * b[4] + a[4] * b[5] + a[8] * b[6];
	out[5]  = a[1] * b[4] + a[5] * b[5] + a[9] * b[6];
	out[6]  = a[2] * b[4] + a[6] * b[5] + a[10] * b[6];
	out[7]  = a[3] * b[4] + a[7] * b[5] + a[11] * b[6] + b[7];

	out[8]  = a[0] * b[8] + a[4] * b[9] + a[8] * b[10];
	out[9]  = a[1] * b[8] + a[5] * b[9] + a[9] * b[10];
	out[10] = a[2] * b[8] + a[6] * b[9] + a[10] * b[10];
	out[11] = a[3] * b[8] + a[7] * b[9] + a[11] * b[10] + b[11];
}

/*
===================
FloorDivMod

Returns mathematically correct (floor-based) quotient and remainder for
numer and denom, both of which should contain no fractional part. The
quotient must fit in 32 bits.
====================
*/

void FloorDivMod (double numer, double denom, int *quotient,
		int *rem)
{
	int		q, r;
	double	x;

#ifdef PARANOID
	if (denom <= 0.0)
		Sys_Error ("FloorDivMod: bad denominator %f\n", denom);

//	if ((floor(numer) != numer) || (floor(denom) != denom))
//		Sys_Error ("FloorDivMod: non-integer numer or denom %f %f\n",
//				numer, denom);
#endif

	if (numer >= 0.0)
	{

		x = floor(numer / denom);
		q = (int)x;
		r = (int)floor(numer - (x * denom));
	}
	else
	{
	//
	// perform operations with positive values, and fix mod to make floor-based
	//
		x = floor(-numer / denom);
		q = -(int)x;
		r = (int)floor(-numer - (x * denom));
		if (r != 0)
		{
			q--;
			r = (int)denom - r;
		}
	}

	*quotient = q;
	*rem = r;
}


/*
===================
GreatestCommonDivisor
====================
*/
int GreatestCommonDivisor (int i1, int i2)
{
	if (i1 > i2)
	{
		if (i2 == 0)
			return (i1);
		return GreatestCommonDivisor (i2, i1 % i2);
	}
	else
	{
		if (i1 == 0)
			return (i2);
		return GreatestCommonDivisor (i1, i2 % i1);
	}
}


// TODO: move to nonintel.c

/*
===================
Invert24To16

Inverts an 8.24 value to a 16.16 value
====================
*/

fixed16_t Invert24To16(fixed16_t val)
{
	if (val < 256)
		return (0xFFFFFFFF);

	return (fixed16_t)
			(((double)0x10000 * (double)0x1000000 / (double)val) + 0.5);
}









void VectorTransform (const vec3_t in1, const matrix3x4 in2, vec3_t out)
{
	out[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, in2[2]) + in2[2][3];
}

void Bones_To_PosQuat4(int numbones, const float *matrix, short *result)
{	//I ripped this function out of DP. tweaked slightly.
	//http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
	float origininvscale = 64;
	float origin[3];
	float quat[4];
	float quatscale;
	while (numbones --> 0)
	{
		float trace = matrix[0*4+0] + matrix[1*4+1] + matrix[2*4+2];
		origin[0] = matrix[0*4+3];
		origin[1] = matrix[1*4+3];
		origin[2] = matrix[2*4+3];
		if(trace > 0)
		{
			float r = sqrt(1.0f + trace), inv = 0.5f / r;
			quat[0] = (matrix[2*4+1] - matrix[1*4+2]) * inv;
			quat[1] = (matrix[0*4+2] - matrix[2*4+0]) * inv;
			quat[2] = (matrix[1*4+0] - matrix[0*4+1]) * inv;
			quat[3] = 0.5f * r;
		}
		else if(matrix[0*4+0] > matrix[1*4+1] && matrix[0*4+0] > matrix[2*4+2])
		{
			float r = sqrt(1.0f + matrix[0*4+0] - matrix[1*4+1] - matrix[2*4+2]), inv = 0.5f / r;
			quat[0] = 0.5f * r;
			quat[1] = (matrix[1*4+0] + matrix[0*4+1]) * inv;
			quat[2] = (matrix[0*4+2] + matrix[2*4+0]) * inv;
			quat[3] = (matrix[2*4+1] - matrix[1*4+2]) * inv;
		}
		else if(matrix[1*4+1] > matrix[2*4+2])
		{
			float r = sqrt(1.0f + matrix[1*4+1] - matrix[0*4+0] - matrix[2*4+2]), inv = 0.5f / r;
			quat[0] = (matrix[1*4+0] + matrix[0*4+1]) * inv;
			quat[1] = 0.5f * r;
			quat[2] = (matrix[2*4+1] + matrix[1*4+2]) * inv;
			quat[3] = (matrix[0*4+2] - matrix[2*4+0]) * inv;
		}
		else
		{
			float r = sqrt(1.0f + matrix[2*4+2] - matrix[0*4+0] - matrix[1*4+1]), inv = 0.5f / r;
			quat[0] = (matrix[0*4+2] + matrix[2*4+0]) * inv;
			quat[1] = (matrix[2*4+1] + matrix[1*4+2]) * inv;
			quat[2] = 0.5f * r;
			quat[3] = (matrix[1*4+0] - matrix[0*4+1]) * inv;
		}
		// normalize quaternion so that it is unit length
		quatscale = quat[0]*quat[0]+quat[1]*quat[1]+quat[2]*quat[2]+quat[3]*quat[3];
		if (quatscale)
			quatscale = (quat[3] >= 0 ? -32767.0f : 32767.0f) / sqrt(quatscale);
		// use a negative scale on the quat because the above function produces a
		// positive quat[3] and canonical quaternions have negative quat[3]
		result[0] = origin[0] * origininvscale;
		result[1] = origin[1] * origininvscale;
		result[2] = origin[2] * origininvscale;
		result[3] = quat[0] * quatscale;
		result[4] = quat[1] * quatscale;
		result[5] = quat[2] * quatscale;
		result[6] = quat[3] * quatscale;

		matrix += 12;
		result += 7;
	}
}

void QDECL GenMatrixPosQuat4Scale(const vec3_t pos, const vec4_t quat, const vec3_t scale, float result[12])
{
	float xx, xy, xz, xw, yy, yz, yw, zz, zw;
	float x2, y2, z2;
	float s;
	x2 = quat[0] + quat[0];
	y2 = quat[1] + quat[1];
	z2 = quat[2] + quat[2];

	xx = quat[0] * x2;   xy = quat[0] * y2;   xz = quat[0] * z2;
	yy = quat[1] * y2;   yz = quat[1] * z2;   zz = quat[2] * z2;
	xw = quat[3] * x2;   yw = quat[3] * y2;   zw = quat[3] * z2;

	s = scale[0];
	result[0*4+0] = s*(1.0f - (yy + zz));
	result[1*4+0] = s*(xy + zw);
	result[2*4+0] = s*(xz - yw);

	s = scale[1];
	result[0*4+1] = s*(xy - zw);
	result[1*4+1] = s*(1.0f - (xx + zz));
	result[2*4+1] = s*(yz + xw);

	s = scale[2];
	result[0*4+2] = s*(xz + yw);
	result[1*4+2] = s*(yz - xw);
	result[2*4+2] = s*(1.0f - (xx + yy));

	result[0*4+3]  =     pos[0];
	result[1*4+3]  =     pos[1];
	result[2*4+3]  =     pos[2];
}
#if 0//def HALFLIFEMODELS

static void AngleQuaternion( const vec3_t angles, vec4_t quaternion )
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	// FIXME: rescale the inputs to 1/2 angle
	angle = angles[2] * 0.5;
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[1] * 0.5;
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[0] * 0.5;
	sr = sin(angle);
	cr = cos(angle);

	quaternion[0] = sr*cp*cy-cr*sp*sy; // X
	quaternion[1] = cr*sp*cy+sr*cp*sy; // Y
	quaternion[2] = cr*cp*sy-sr*sp*cy; // Z
	quaternion[3] = cr*cp*cy+sr*sp*sy; // W
}

static void QuaternionMatrix( const vec4_t quaternion, float (*matrix)[4] )
{

	matrix[0][0] = 1.0 - 2.0 * quaternion[1] * quaternion[1] - 2.0 * quaternion[2] * quaternion[2];
	matrix[1][0] = 2.0 * quaternion[0] * quaternion[1] + 2.0 * quaternion[3] * quaternion[2];
	matrix[2][0] = 2.0 * quaternion[0] * quaternion[2] - 2.0 * quaternion[3] * quaternion[1];

	matrix[0][1] = 2.0 * quaternion[0] * quaternion[1] - 2.0 * quaternion[3] * quaternion[2];
	matrix[1][1] = 1.0 - 2.0 * quaternion[0] * quaternion[0] - 2.0 * quaternion[2] * quaternion[2];
	matrix[2][1] = 2.0 * quaternion[1] * quaternion[2] + 2.0 * quaternion[3] * quaternion[0];

	matrix[0][2] = 2.0 * quaternion[0] * quaternion[2] + 2.0 * quaternion[3] * quaternion[1];
	matrix[1][2] = 2.0 * quaternion[1] * quaternion[2] - 2.0 * quaternion[3] * quaternion[0];
	matrix[2][2] = 1.0 - 2.0 * quaternion[0] * quaternion[0] - 2.0 * quaternion[1] * quaternion[1];
}
#endif
void QuaternionSlerp( const vec4_t p, vec4_t q, float t, vec4_t qt )
{
	int i;
	float omega, cosom, sinom, sclp, sclq;

	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;
	for (i = 0; i < 4; i++) {
		a += (p[i]-q[i])*(p[i]-q[i]);
		b += (p[i]+q[i])*(p[i]+q[i]);
	}
	if (a > b) {
		for (i = 0; i < 4; i++) {
			q[i] = -q[i];
		}
	}

	cosom = p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];

	if ((1.0 + cosom) > 0.00000001) {
		if ((1.0 - cosom) > 0.00000001) {
			omega = acos( cosom );
			sinom = sin( omega );
			sclp = sin( (1.0 - t)*omega) / sinom;
			sclq = sin( t*omega ) / sinom;
		}
		else {
			sclp = 1.0 - t;
			sclq = t;
		}
		for (i = 0; i < 4; i++) {
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else {
		qt[0] = -p[1];
		qt[1] = p[0];
		qt[2] = -p[3];
		qt[3] = p[2];
		sclp = sin( (1.0 - t) * 0.5 * M_PI);
		sclq = sin( t * 0.5 * M_PI);
		for (i = 0; i < 4; i++) {
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
}

//This function is GL stylie (use as 2nd arg to ML_MultMatrix4).
float *Matrix4x4_CM_NewRotation(float a, float x, float y, float z)
{
	static float ret[16];
	float c = cos(a* M_PI / 180.0);
	float s = sin(a* M_PI / 180.0);

	ret[0] = x*x*(1-c)+c;
	ret[4] = x*y*(1-c)-z*s;
	ret[8] = x*z*(1-c)+y*s;
	ret[12] = 0;

	ret[1] = y*x*(1-c)+z*s;
    ret[5] = y*y*(1-c)+c;
	ret[9] = y*z*(1-c)-x*s;
	ret[13] = 0;

	ret[2] = x*z*(1-c)-y*s;
	ret[6] = y*z*(1-c)+x*s;
	ret[10] = z*z*(1-c)+c;
	ret[14] = 0;

	ret[3] = 0;
	ret[7] = 0;
	ret[11] = 0;
	ret[15] = 1;
	return ret;
}

//This function is GL stylie (use as 2nd arg to ML_MultMatrix4).
float *Matrix4x4_CM_NewTranslation(float x, float y, float z)
{
	static float ret[16];
	ret[0] = 1;
	ret[4] = 0;
	ret[8] = 0;
	ret[12] = x;

	ret[1] = 0;
    ret[5] = 1;
	ret[9] = 0;
	ret[13] = y;

	ret[2] = 0;
	ret[6] = 0;
	ret[10] = 1;
	ret[14] = z;

	ret[3] = 0;
	ret[7] = 0;
	ret[11] = 0;
	ret[15] = 1;
	return ret;
}

//be aware that this generates two sorts of matricies depending on order of a+b
void Matrix4_Multiply(const float *a, const float *b, float *out)
{
	out[0]  = a[0] * b[0] + a[4] * b[1] + a[8] * b[2] + a[12] * b[3];
	out[1]  = a[1] * b[0] + a[5] * b[1] + a[9] * b[2] + a[13] * b[3];
	out[2]  = a[2] * b[0] + a[6] * b[1] + a[10] * b[2] + a[14] * b[3];
	out[3]  = a[3] * b[0] + a[7] * b[1] + a[11] * b[2] + a[15] * b[3];

	out[4]  = a[0] * b[4] + a[4] * b[5] + a[8] * b[6] + a[12] * b[7];
	out[5]  = a[1] * b[4] + a[5] * b[5] + a[9] * b[6] + a[13] * b[7];
	out[6]  = a[2] * b[4] + a[6] * b[5] + a[10] * b[6] + a[14] * b[7];
	out[7]  = a[3] * b[4] + a[7] * b[5] + a[11] * b[6] + a[15] * b[7];

	out[8]  = a[0] * b[8] + a[4] * b[9] + a[8] * b[10] + a[12] * b[11];
	out[9]  = a[1] * b[8] + a[5] * b[9] + a[9] * b[10] + a[13] * b[11];
	out[10] = a[2] * b[8] + a[6] * b[9] + a[10] * b[10] + a[14] * b[11];
	out[11] = a[3] * b[8] + a[7] * b[9] + a[11] * b[10] + a[15] * b[11];

	out[12] = a[0] * b[12] + a[4] * b[13] + a[8] * b[14] + a[12] * b[15];
	out[13] = a[1] * b[12] + a[5] * b[13] + a[9] * b[14] + a[13] * b[15];
	out[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15];
	out[15] = a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15];
}

void Matrix3x4_RM_Transform3(const float *matrix, const float *vector, float *product)
{
	product[0] = matrix[0]*vector[0] + matrix[1]*vector[1] + matrix[2]*vector[2] + matrix[3];
	product[1] = matrix[4]*vector[0] + matrix[5]*vector[1] + matrix[6]*vector[2] + matrix[7];
	product[2] = matrix[8]*vector[0] + matrix[9]*vector[1] + matrix[10]*vector[2] + matrix[11];
}
void Matrix3x4_RM_Transform3x3(const float *matrix, const float *vector, float *product)
{
	product[0] = matrix[0]*vector[0] + matrix[1]*vector[1] + matrix[2]*vector[2];
	product[1] = matrix[4]*vector[0] + matrix[5]*vector[1] + matrix[6]*vector[2];
	product[2] = matrix[8]*vector[0] + matrix[9]*vector[1] + matrix[10]*vector[2];
}

//transform 4d vector by a 4d matrix.
void Matrix4x4_CM_Transform4(const float *matrix, const float *vector, float *product)
{
	product[0] = matrix[0]*vector[0] + matrix[4]*vector[1] + matrix[8]*vector[2] + matrix[12]*vector[3];
	product[1] = matrix[1]*vector[0] + matrix[5]*vector[1] + matrix[9]*vector[2] + matrix[13]*vector[3];
	product[2] = matrix[2]*vector[0] + matrix[6]*vector[1] + matrix[10]*vector[2] + matrix[14]*vector[3];
	product[3] = matrix[3]*vector[0] + matrix[7]*vector[1] + matrix[11]*vector[2] + matrix[15]*vector[3];
}

//ignore the entire right+bottom row/column of the 4*4 matrix
void Matrix4x4_CM_Transform3x3(const float *matrix, const float *vector, float *product)
{
	product[0] = matrix[0]*vector[0] + matrix[4]*vector[1] + matrix[8]*vector[2];
	product[1] = matrix[1]*vector[0] + matrix[5]*vector[1] + matrix[9]*vector[2];
	product[2] = matrix[2]*vector[0] + matrix[6]*vector[1] + matrix[10]*vector[2];
}

//disregard the extra bit of the matrix
void Matrix4x4_CM_Transform3(const float *matrix, const float *vector, float *product)
{
	product[0] = matrix[0]*vector[0] + matrix[4]*vector[1] + matrix[8]*vector[2] + matrix[12];
	product[1] = matrix[1]*vector[0] + matrix[5]*vector[1] + matrix[9]*vector[2] + matrix[13];
	product[2] = matrix[2]*vector[0] + matrix[6]*vector[1] + matrix[10]*vector[2] + matrix[14];
}
void Matrix4x4_CM_Transform34(const float *matrix, const vec3_t vector, vec4_t product)
{
	//transform as though vector[3] == 1
	product[0] = matrix[0]*vector[0] + matrix[4]*vector[1] + matrix[8]*vector[2] + matrix[12];
	product[1] = matrix[1]*vector[0] + matrix[5]*vector[1] + matrix[9]*vector[2] + matrix[13];
	product[2] = matrix[2]*vector[0] + matrix[6]*vector[1] + matrix[10]*vector[2] + matrix[14];
	product[3] = matrix[3]*vector[0] + matrix[7]*vector[1] + matrix[11]*vector[2] + matrix[15];
}

void Matrix4x4_CM_ModelViewMatrix(float *modelview, const vec3_t viewangles, const vec3_t vieworg)
{
#if 1
	float *out = modelview;
	float cp = cos(-viewangles[0] * M_PI / 180.0);
	float sp = sin(-viewangles[0] * M_PI / 180.0);
	float cy = cos(-viewangles[1] * M_PI / 180.0);
	float sy = sin(-viewangles[1] * M_PI / 180.0);
	float cr = cos(-viewangles[2] * M_PI / 180.0);
	float sr = sin(-viewangles[2] * M_PI / 180.0);

	out[0]  = -sr*sp*cy - cr*sy;
	out[1]  = -cr*sp*cy + sr*sy;
	out[2]  = -cp*cy;
	out[3]  = 0;
	out[4]  = sr*sp*sy - cr*cy;
	out[5]  = cr*sp*sy + sr*cy;
	out[6]  = cp*sy;
	out[7]  = 0;
	out[8]  = sr*cp;
	out[9]  = cr*cp;
	out[10] = -sp;
	out[11] = 0;
	out[12] =   - out[0]*vieworg[0] - out[4]*vieworg[1] - out[ 8]*vieworg[2];
	out[13] =   - out[1]*vieworg[0] - out[5]*vieworg[1] - out[ 9]*vieworg[2];
	out[14] =   - out[2]*vieworg[0] - out[6]*vieworg[1] - out[10]*vieworg[2];
	out[15] = 1 - out[3]*vieworg[0] - out[7]*vieworg[1] - out[11]*vieworg[2];
#else
	float tempmat[16];
	//load identity.
	memset(modelview, 0, sizeof(*modelview)*16);
#if FULLYGL
	modelview[0] = 1;
	modelview[5] = 1;
	modelview[10] = 1;
	modelview[15] = 1;

	Matrix4_Multiply(modelview, Matrix4_CM_NewRotation(-90,  1, 0, 0), tempmat);	    // put Z going up
	Matrix4_Multiply(tempmat, Matrix4_CM_NewRotation(90,  0, 0, 1), modelview);	    // put Z going up
#else
	//use this lame wierd and crazy identity matrix..
	modelview[2] = -1;
	modelview[4] = -1;
	modelview[9] = 1;
	modelview[15] = 1;
#endif
	//figure out the current modelview matrix

	//I would if some of these, but then I'd still need a couple of copys
	Matrix4_Multiply(modelview, Matrix4x4_CM_NewRotation(-viewangles[2],  1, 0, 0), tempmat);
	Matrix4_Multiply(tempmat, Matrix4x4_CM_NewRotation(-viewangles[0],  0, 1, 0), modelview);
	Matrix4_Multiply(modelview, Matrix4x4_CM_NewRotation(-viewangles[1],  0, 0, 1), tempmat);

	Matrix4_Multiply(tempmat, Matrix4x4_CM_NewTranslation(-vieworg[0],  -vieworg[1],  -vieworg[2]), modelview);	    // put Z going up
#endif
}

void Matrix4x4_CM_CreateTranslate (float *out, float x, float y, float z)
{
	out[0] = 1;
	out[1] = 0;
	out[2] = 0;
	out[3] = 0;

	out[4] = 0;
    out[5] = 1;
	out[6] = 0;
	out[7] = 0;

	out[8] = 0;
	out[9] = 0;
	out[10] = 1;
	out[11] = 0;

	out[12] = x;
	out[13] = y;
	out[14] = z;
	out[15] = 1;
}

void Matrix4x4_RM_CreateTranslate (float *out, float x, float y, float z)
{
	out[0] = 1;
	out[4] = 0;
	out[8] = 0;
	out[12] = 0;

	out[1] = 0;
    out[5] = 1;
	out[9] = 0;
	out[13] = 0;

	out[2] = 0;
	out[6] = 0;
	out[10] = 1;
	out[14] = 0;

	out[3] = x;
	out[7] = y;
	out[11] = z;
	out[15] = 1;
}

void Matrix4x4_CM_LightMatrixFromAxis(float *modelview, const vec3_t px, const vec3_t py, const vec3_t pz, const vec3_t org)
{
	modelview[ 0] = px[0];
	modelview[ 1] = py[0];
	modelview[ 2] = pz[0];
	modelview[ 3] = 0;
	modelview[ 4] = px[1];
	modelview[ 5] = py[1];
	modelview[ 6] = pz[1];
	modelview[ 7] = 0;
	modelview[ 8] = px[2];
	modelview[ 9] = py[2];
	modelview[10] = pz[2];
	modelview[11] = 0;
	modelview[12] = -(px[0]*org[0] + px[1]*org[1] + px[2]*org[2]);
	modelview[13] = -(py[0]*org[0] + py[1]*org[1] + py[2]*org[2]);
	modelview[14] = -(pz[0]*org[0] + pz[1]*org[1] + pz[2]*org[2]);
	modelview[15] = 1;
}

void Matrix4x4_CM_ModelViewMatrixFromAxis(float *modelview, const vec3_t pn, const vec3_t right, const vec3_t up, const vec3_t vieworg)
{
	float tempmat[16];

	tempmat[ 0] = right[0];
	tempmat[ 1] = up[0];
	tempmat[ 2] = -pn[0];
	tempmat[ 3] = 0;
	tempmat[ 4] = right[1];
	tempmat[ 5] = up[1];
	tempmat[ 6] = -pn[1];
	tempmat[ 7] = 0;
	tempmat[ 8] = right[2];
	tempmat[ 9] = up[2];
	tempmat[10] = -pn[2];
	tempmat[11] = 0;
	tempmat[12] = 0;
	tempmat[13] = 0;
	tempmat[14] = 0;
	tempmat[15] = 1;

	Matrix4_Multiply(tempmat, Matrix4x4_CM_NewTranslation(-vieworg[0],  -vieworg[1],  -vieworg[2]), modelview);	    // put Z going up
}


void Matrix3x4_RM_FromAngles(const vec3_t angles, const vec3_t origin, float *out)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	out[0] = cp*cy;
	out[1] = (sr*sp*cy+cr*-sy);
	out[2] = (cr*sp*cy+-sr*-sy);
	out[3] = origin[0];

	out[4] = cp*sy;
	out[5] = (sr*sp*sy+cr*cy);
	out[6] = (cr*sp*sy+-sr*cy);
	out[7] = origin[1];

	out[8] = -sp;
	out[9] = sr*cp;
	out[10] = cr*cp;
	out[11] = origin[2];
}
void Matrix3x4_RM_ToVectors(const float *in, float vx[3], float vy[3], float vz[3], float t[3])
{
	vx[0] = in[0];
	vx[1] = in[4];
	vx[2] = in[8];

	vy[0] = in[1];
	vy[1] = in[5];
	vy[2] = in[9];

	vz[0] = in[2];
	vz[1] = in[6];
	vz[2] = in[10];

	t [0] = in[3];
	t [1] = in[7];
	t [2] = in[11];
}

void Matrix4x4_RM_FromVectors(float *out, const float vx[3], const float vy[3], const float vz[3], const float t[3])
{
	out[0] = vx[0];
	out[1] = vy[0];
	out[2] = vz[0];
	out[3] = t[0];
	out[4] = vx[1];
	out[5] = vy[1];
	out[6] = vz[1];
	out[7] = t[1];
	out[8] = vx[2];
	out[9] = vy[2];
	out[10] = vz[2];
	out[11] = t[2];
	out[12] = 0.0f;
	out[13] = 0.0f;
	out[14] = 0.0f;
	out[15] = 1.0f;
}

void Matrix3x4_RM_FromVectors(float *out, const float vx[3], const float vy[3], const float vz[3], const float t[3])
{
	out[0] = vx[0];
	out[1] = vy[0];
	out[2] = vz[0];
	out[3] = t[0];
	out[4] = vx[1];
	out[5] = vy[1];
	out[6] = vz[1];
	out[7] = t[1];
	out[8] = vx[2];
	out[9] = vy[2];
	out[10] = vz[2];
	out[11] = t[2];
}

void Matrix4x4_CM_ModelMatrixFromAxis(float *modelview, const vec3_t pn, const vec3_t right, const vec3_t up, const vec3_t vieworg)
{
	float tempmat[16];

	tempmat[ 0] = pn[0];
	tempmat[ 1] = pn[1];
	tempmat[ 2] = pn[2];
	tempmat[ 3] = 0;
	tempmat[ 4] = right[0];
	tempmat[ 5] = right[1];
	tempmat[ 6] = right[2];
	tempmat[ 7] = 0;
	tempmat[ 8] = up[0];
	tempmat[ 9] = up[1];
	tempmat[10] = up[2];
	tempmat[11] = 0;
	tempmat[12] = 0;
	tempmat[13] = 0;
	tempmat[14] = 0;
	tempmat[15] = 1;

	Matrix4_Multiply(Matrix4x4_CM_NewTranslation(vieworg[0],  vieworg[1],  vieworg[2]), tempmat, modelview);	    // put Z going up
}

void Matrix4x4_CM_ModelMatrix(float *modelview, vec_t x, vec_t y, vec_t z, vec_t pitch, vec_t yaw, vec_t roll, vec_t scale)
{
	float tempmat[16];
	//load identity.
	memset(modelview, 0, sizeof(*modelview)*16);
#if FULLYGL
	modelview[0] = 1;
	modelview[5] = 1;
	modelview[10] = 1;
	modelview[15] = 1;

	Matrix4_Multiply(modelview, Matrix4x4_CM_NewRotation(-90,  1, 0, 0), tempmat);	    // put Z going up
	Matrix4_Multiply(tempmat, Matrix4x4_CM_NewRotation(90,  0, 0, 1), modelview);	    // put Z going up
#else
	//use this lame wierd and crazy identity matrix..
	modelview[2] = -1;
	modelview[4] = -1;
	modelview[9] = 1;
	modelview[15] = 1;
#endif
	//figure out the current modelview matrix

	//I would if some of these, but then I'd still need a couple of copys
	Matrix4_Multiply(modelview, Matrix4x4_CM_NewRotation(-roll,  1, 0, 0), tempmat);
	Matrix4_Multiply(tempmat, Matrix4x4_CM_NewRotation(-pitch,  0, 1, 0), modelview);
	Matrix4_Multiply(modelview, Matrix4x4_CM_NewRotation(-yaw,  0, 0, 1), tempmat);

	Matrix4_Multiply(tempmat, Matrix4x4_CM_NewTranslation(x,  y,  z), modelview);
}

void Matrix4x4_Identity(float *outm)
{
	outm[ 0] = 1;
	outm[ 1] = 0;
	outm[ 2] = 0;
	outm[ 3] = 0;
	outm[ 4] = 0;
	outm[ 5] = 1;
	outm[ 6] = 0;
	outm[ 7] = 0;
	outm[ 8] = 0;
	outm[ 9] = 0;
	outm[10] = 1;
	outm[11] = 0;
	outm[12] = 0;
	outm[13] = 0;
	outm[14] = 0;
	outm[15] = 1;
}

void Matrix4x4_CM_Projection_Offset(float *proj, float fovl, float fovr, float fovd, float fovu, float neard, float fard, qboolean d3d)
{
	double dn = (d3d?0:-1), df = 1;	//d3d outputs near as 0, opengl has near as -1. that's the only difference.
	double ymax = tan( fovu * M_PI / 180.0 );
	double ymin = tan( fovd * M_PI / 180.0 );
	double xmin = tan( fovl * M_PI / 180.0 );
	double xmax = tan( fovr * M_PI / 180.0 );

	if (fard <= neard)
	{	//switch to an infinite projection
		const double epsilon = 1.0/(1<<22);

		proj[0] = (2) / (xmax - xmin);
		proj[4] = 0;
		proj[8] = (xmax + xmin) / (xmax - xmin);
		proj[12] = 0;

		proj[1] = 0;
		proj[5] = (2) / (ymax - ymin);
		proj[9] = (ymax + ymin) / (ymax - ymin);
		proj[13] = 0;

		proj[2] = 0;
		proj[6] = 0;
		proj[10] = epsilon-1;
		proj[14] = (epsilon-(df-dn))*neard;

		proj[3] = 0;
		proj[7] = 0;
		proj[11] = -1;
		proj[15] = 0;
	}
	else
	{
		proj[0] = (2) / (xmax - xmin);
		proj[4] = 0;
		proj[8] = (xmax + xmin) / (xmax - xmin);
		proj[12] = 0;

		proj[1] = 0;
		proj[5] = (2) / (ymax - ymin);
		proj[9] = (ymax + ymin) / (ymax - ymin);
		proj[13] = 0;

		proj[2] = 0;
		proj[6] = 0;
		proj[10] = (fard*df-neard*dn)/(neard-fard);
		proj[14] = ((df-dn)*fard*neard)/(neard-fard);

		proj[3] = 0;
		proj[7] = 0;
		proj[11] = -1;
		proj[15] = 0;
	}
}

void Matrix4x4_CM_Projection_Far(float *proj, float fovx, float fovy, float neard, float fard, qboolean d3d)
{
	double xmin, xmax, ymin, ymax;
	double dn = (d3d?0:-1), df = 1;

	//proj
	ymax = neard * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	if (fovx == fovy)
	{
		xmax = ymax;
		xmin = ymin;
	}
	else
	{
		xmax = neard * tan( fovx * M_PI / 360.0 );
		xmin = -xmax;
	}

	proj[0] = (2*neard) / (xmax - xmin);
	proj[4] = 0;
	proj[8] = (xmax + xmin) / (xmax - xmin);
	proj[12] = 0;

	proj[1] = 0;
	proj[5] = (2*neard) / (ymax - ymin);
	proj[9] = (ymax + ymin) / (ymax - ymin);
	proj[13] = 0;

	proj[2] = 0;
	proj[6] = 0;
	proj[10] = (fard*df-neard*dn)/(neard-fard);
	proj[14] = ((df-dn)*fard*neard)/(neard-fard);
	
	proj[3] = 0;
	proj[7] = 0;
	proj[11] = -1;
	proj[15] = 0;
}

void Matrix4x4_CM_Projection_Inf(float *proj, float fovx, float fovy, float neard, qboolean d3d)
{
//FIXME: glDepthRange(1,0) for reverse-z (with cull flipped). combine with arb_clip_control for 0-1. this should give much better depth precision with floating point depth buffers.
	float xmin, xmax, ymin, ymax;
	double dn = (d3d?0:-1), df = 1;

	//proj
	ymax = neard * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	if (fovx == fovy)
	{
		xmax = ymax;
		xmin = ymin;
	}
	else
	{
		xmax = neard * tan( fovx * M_PI / 360.0 );
		xmin = -xmax;
	}

	proj[0] = (2*neard) / (xmax - xmin);
	proj[4] = 0;
	proj[8] = (xmax + xmin) / (xmax - xmin);
	proj[12] = 0;

	proj[1] = 0;
	proj[5] = (2*neard) / (ymax - ymin);
	proj[9] = (ymax + ymin) / (ymax - ymin);
	proj[13] = 0;

#if 1
	{
		const double epsilon = 1.0/(1<<22);
		proj[2] = 0;
		proj[6] = 0;
		proj[10] = epsilon-1;
		proj[14] = (epsilon-(df-dn))*neard;
	}
#elif 1
	{	//mathematical target
		const float fard = (1<<22);
		proj[2] = 0;
		proj[6] = 0;
		proj[10] = (fard*df-neard*dn)/(neard-fard);
		proj[14] = ((df-dn)*fard*neard)/(neard-fard);
	}
#else
	//old logic
	proj[2] = 0;
	proj[6] = 0;
	proj[10] = -1  * ((float)(1<<21)/(1<<22));
	proj[14] = -2*neard;
#endif
	
	proj[3] = 0;
	proj[7] = 0;
	proj[11] = -1;
	proj[15] = 0;
}
void Matrix4x4_CM_Projection2(float *proj, float fovx, float fovy, float neard)
{
	float xmin, xmax, ymin, ymax;
	float nudge = 1;

	//proj
	ymax = neard * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	xmax = neard * tan( fovx * M_PI / 360.0 );
	xmin = -xmax;

	proj[0] = (2*neard) / (xmax - xmin);
	proj[4] = 0;
	proj[8] = (xmax + xmin) / (xmax - xmin);
	proj[12] = 0;

	proj[1] = 0;
	proj[5] = (2*neard) / (ymax - ymin);
	proj[9] = (ymax + ymin) / (ymax - ymin);
	proj[13] = 0;

	proj[2] = 0;
	proj[6] = 0;
	proj[10] = -1  * nudge;
	proj[14] = -2*neard * nudge;
	
	proj[3] = 0;
	proj[7] = 0;
	proj[11] = -1;
	proj[15] = 0;
}

void Matrix4x4_CM_Orthographic(float *proj, float xmin, float xmax, float ymin, float ymax,
		     float znear, float zfar)
{
	proj[0] = 2/(xmax-xmin);
	proj[4] = 0;
	proj[8] = 0;
	proj[12] = -(xmax+xmin)/(xmax-xmin);

	proj[1] = 0;
	proj[5] = 2/(ymax-ymin);
	proj[9] = 0;
	proj[13] = -(ymax+ymin)/(ymax-ymin);

	proj[2] = 0;
	proj[6] = 0;
	proj[10] = -2/(zfar-znear);
	proj[14] = -(zfar+znear)/(zfar-znear);
	
	proj[3] = 0;
	proj[7] = 0;
	proj[11] = 0;
	proj[15] = 1;
}
void Matrix4x4_CM_OrthographicD3D(float *proj, float xmin, float xmax, float ymax, float ymin,
		     float znear, float zfar)
{
	proj[0] = 2/(xmax-xmin);
	proj[4] = 0;
	proj[8] = 0;
	proj[12] = (xmax+xmin)/(xmin-xmax);

	proj[1] = 0;
	proj[5] = 2/(ymax-ymin);
	proj[9] = 0;
	proj[13] = (ymax+ymin)/(ymin-ymax);

	proj[2] = 0;
	proj[6] = 0;
	proj[10] = 1/(znear-zfar);
	proj[14] = znear/(znear-zfar);
	
	proj[3] = 0;
	proj[7] = 0;
	proj[11] = 0;
	proj[15] = 1;
}
/*
 * Compute inverse of 4x4 transformation matrix.
 * Code contributed by Jacques Leroy jle@star.be
 * Return true for success, false for failure (singular matrix)
 * This came to FTE via mesa's GLU.
 */
qboolean Matrix4_Invert(const float *m, float *out)
{
/* NB. OpenGL Matrices are COLUMN major. */
#define SWAP_ROWS(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]

   float wtmp[4][8];
   float m0, m1, m2, m3, s;
   float *r0, *r1, *r2, *r3;

   r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

   r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
      r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
      r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
      r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
      r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
      r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
      r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
      r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
      r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
      r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
      r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
      r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

   /* choose pivot - or die */
   if (fabs(r3[0]) > fabs(r2[0]))
      SWAP_ROWS(r3, r2);
   if (fabs(r2[0]) > fabs(r1[0]))
      SWAP_ROWS(r2, r1);
   if (fabs(r1[0]) > fabs(r0[0]))
      SWAP_ROWS(r1, r0);
   if (0.0 == r0[0])
      return false;

   /* eliminate first variable     */
   m1 = r1[0] / r0[0];
   m2 = r2[0] / r0[0];
   m3 = r3[0] / r0[0];
   s = r0[1];
   r1[1] -= m1 * s;
   r2[1] -= m2 * s;
   r3[1] -= m3 * s;
   s = r0[2];
   r1[2] -= m1 * s;
   r2[2] -= m2 * s;
   r3[2] -= m3 * s;
   s = r0[3];
   r1[3] -= m1 * s;
   r2[3] -= m2 * s;
   r3[3] -= m3 * s;
   s = r0[4];
   if (s != 0.0) {
      r1[4] -= m1 * s;
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r0[5];
   if (s != 0.0) {
      r1[5] -= m1 * s;
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r0[6];
   if (s != 0.0) {
      r1[6] -= m1 * s;
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r0[7];
   if (s != 0.0) {
      r1[7] -= m1 * s;
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }

   /* choose pivot - or die */
   if (fabs(r3[1]) > fabs(r2[1]))
      SWAP_ROWS(r3, r2);
   if (fabs(r2[1]) > fabs(r1[1]))
      SWAP_ROWS(r2, r1);
   if (0.0 == r1[1])
      return false;

   /* eliminate second variable */
   m2 = r2[1] / r1[1];
   m3 = r3[1] / r1[1];
   r2[2] -= m2 * r1[2];
   r3[2] -= m3 * r1[2];
   r2[3] -= m2 * r1[3];
   r3[3] -= m3 * r1[3];
   s = r1[4];
   if (0.0 != s) {
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r1[5];
   if (0.0 != s) {
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r1[6];
   if (0.0 != s) {
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r1[7];
   if (0.0 != s) {
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }

   /* choose pivot - or die */
   if (fabs(r3[2]) > fabs(r2[2]))
      SWAP_ROWS(r3, r2);
   if (0.0 == r2[2])
      return false;

   /* eliminate third variable */
   m3 = r3[2] / r2[2];
   r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
      r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];

   /* last check */
   if (0.0 == r3[3])
      return false;

   s = 1.0 / r3[3];             /* now back substitute row 3 */
   r3[4] *= s;
   r3[5] *= s;
   r3[6] *= s;
   r3[7] *= s;

   m2 = r2[3];                  /* now back substitute row 2 */
   s = 1.0 / r2[2];
   r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
      r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
   m1 = r1[3];
   r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
      r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
   m0 = r0[3];
   r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
      r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

   m1 = r1[2];                  /* now back substitute row 1 */
   s = 1.0 / r1[1];
   r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
      r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
   m0 = r0[2];
   r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
      r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

   m0 = r0[1];                  /* now back substitute row 0 */
   s = 1.0 / r0[0];
   r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
      r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

   MAT(out, 0, 0) = r0[4];
   MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
   MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
   MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
   MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
   MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
   MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
   MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
   MAT(out, 3, 3) = r3[7];

   return true;

#undef MAT
#undef SWAP_ROWS
}

void Matrix3x3_RM_Invert_Simple (const vec3_t in1[3], vec3_t out[3])
{
	// we only support uniform scaling, so assume the first row is enough
	// (note the lack of sqrt here, because we're trying to undo the scaling,
	// this means multiplying by the inverse scale twice - squaring it, which
	// makes the sqrt a waste of time)
#if 1
	double scale = 1.0 / (in1[0][0] * in1[0][0] + in1[0][1] * in1[0][1] + in1[0][2] * in1[0][2]);
#else
	double scale = 3.0 / sqrt
		 (in1->m[0][0] * in1->m[0][0] + in1->m[0][1] * in1->m[0][1] + in1->m[0][2] * in1->m[0][2]
		+ in1->m[1][0] * in1->m[1][0] + in1->m[1][1] * in1->m[1][1] + in1->m[1][2] * in1->m[1][2]
		+ in1->m[2][0] * in1->m[2][0] + in1->m[2][1] * in1->m[2][1] + in1->m[2][2] * in1->m[2][2]);
	scale *= scale;
#endif

	// invert the rotation by transposing and multiplying by the squared
	// recipricol of the input matrix scale as described above
	out[0][0] = in1[0][0] * scale;
	out[0][1] = in1[1][0] * scale;
	out[0][2] = in1[2][0] * scale;

	out[1][0] = in1[0][1] * scale;
	out[1][1] = in1[1][1] * scale;
	out[1][2] = in1[2][1] * scale;

	out[2][0] = in1[0][2] * scale;
	out[2][1] = in1[1][2] * scale;
	out[2][2] = in1[2][2] * scale;
}

void Matrix3x4_Invert (const float *in1, float *out)
{
	vec3_t a, b, c, trans;

	VectorSet (a, in1[0], in1[4], in1[8]);
	VectorSet (b, in1[1], in1[5], in1[9]);
	VectorSet (c, in1[2], in1[6], in1[10]);

	VectorScale (a, 1 / DotProduct (a, a), a);
	VectorScale (b, 1 / DotProduct (b, b), b);
	VectorScale (c, 1 / DotProduct (c, c), c);

	VectorSet (trans, in1[3], in1[7], in1[11]);

	Vector4Set (out+0, a[0], a[1], a[2], -DotProduct (a, trans));
	Vector4Set (out+4, b[0], b[1], b[2], -DotProduct (b, trans));
	Vector4Set (out+8, c[0], c[1], c[2], -DotProduct (c, trans));
}

void QDECL Matrix3x4_Invert_Simple (const float *in1, float *out)
{
	// we only support uniform scaling, so assume the first row is enough
	// (note the lack of sqrt here, because we're trying to undo the scaling,
	// this means multiplying by the inverse scale twice - squaring it, which
	// makes the sqrt a waste of time)
#if 1
	double scale = 1.0 / (in1[0] * in1[0] + in1[1] * in1[1] + in1[2] * in1[2]);
#else
	double scale = 3.0 / sqrt
		 (in1->m[0][0] * in1->m[0][0] + in1->m[0][1] * in1->m[0][1] + in1->m[0][2] * in1->m[0][2]
		+ in1->m[1][0] * in1->m[1][0] + in1->m[1][1] * in1->m[1][1] + in1->m[1][2] * in1->m[1][2]
		+ in1->m[2][0] * in1->m[2][0] + in1->m[2][1] * in1->m[2][1] + in1->m[2][2] * in1->m[2][2]);
	scale *= scale;
#endif

	// invert the rotation by transposing and multiplying by the squared
	// recipricol of the input matrix scale as described above
	out[0] = in1[0] * scale;
	out[1] = in1[4] * scale;
	out[2] = in1[8] * scale;
	out[4] = in1[1] * scale;
	out[5] = in1[5] * scale;
	out[6] = in1[9] * scale;
	out[8] = in1[2] * scale;
	out[9] = in1[6] * scale;
	out[10] = in1[10] * scale;

	// invert the translate
	out[3] = -(in1[3] * out[0] + in1[7] * out[1] + in1[11] * out[2]);
	out[7] = -(in1[3] * out[4] + in1[7] * out[5] + in1[11] * out[6]);
	out[11] = -(in1[3] * out[8] + in1[7] * out[9] + in1[11] * out[10]);
}

void Matrix3x4_InvertTo4x4_Simple (const float *in1, float *out)
{
	Matrix3x4_Invert_Simple(in1, out);
	out[12] = 0;
	out[13] = 0;
	out[14] = 0;
	out[15] = 1;
}

void Matrix3x4_InvertTo3x3(const float *in, float *result)
{
	float t1[16], tr[16];
	memcpy(t1, in, sizeof(float)*12);
	t1[12] = 0;
	t1[13] = 0;
	t1[14] = 0;
	t1[15] = 1;
	Matrix4_Invert(t1, tr);
	VectorCopy(tr+0, result+0);
	VectorCopy(tr+4, result+3);
	VectorCopy(tr+8, result+6);
	return;
/*
#define A(x,y) in[x+y*4]
#define result(x,y) result[x+y*3]
	double determinant =    +A(0,0)*(A(1,1)*A(2,2)-A(2,1)*A(1,2))
							-A(0,1)*(A(1,0)*A(2,2)-A(1,2)*A(2,0))
							+A(0,2)*(A(1,0)*A(2,1)-A(1,1)*A(2,0));
	double invdet = 1/determinant;
	result(0,0) =  (A(1,1)*A(2,2)-A(2,1)*A(1,2))*invdet;
	result(1,0) = -(A(0,1)*A(2,2)-A(0,2)*A(2,1))*invdet;
	result(2,0) =  (A(0,1)*A(1,2)-A(0,2)*A(1,1))*invdet;
	result(0,1) = -(A(1,0)*A(2,2)-A(1,2)*A(2,0))*invdet;
	result(1,1) =  (A(0,0)*A(2,2)-A(0,2)*A(2,0))*invdet;
	result(2,1) = -(A(0,0)*A(1,2)-A(1,0)*A(0,2))*invdet;
	result(0,2) =  (A(1,0)*A(2,1)-A(2,0)*A(1,1))*invdet;
	result(1,2) = -(A(0,0)*A(2,1)-A(2,0)*A(0,1))*invdet;
	result(2,2) =  (A(0,0)*A(1,1)-A(1,0)*A(0,1))*invdet;
	*/
}

//screen->3d

void Matrix4x4_CM_UnProject(const vec3_t in, vec3_t out, const vec3_t viewangles, const vec3_t vieworg, float fovx, float fovy)
{
	float modelview[16];
	float proj[16];
	float tempm[16];

	Matrix4x4_CM_ModelViewMatrix(modelview, viewangles, vieworg);
	Matrix4x4_CM_Projection_Inf(proj, fovx, fovy, 4, true);
	Matrix4_Multiply(proj, modelview, tempm);

	Matrix4_Invert(tempm, proj);

	{
		float v[4], tempv[4];
		v[0] = in[0]*2-1;
		v[1] = in[1]*2-1;
		v[2] = in[2];
		v[3] = 1;

		//don't use 1, because the far clip plane really is an infinite distance away
		if (v[2] >= 1)
			v[2] = 0.999999;

		Matrix4x4_CM_Transform4(proj, v, tempv); 

		out[0] = tempv[0]/tempv[3];
		out[1] = tempv[1]/tempv[3];
		out[2] = tempv[2]/tempv[3];
	}
}

//returns fractions of screen.
//uses GL style rotations and translations and stuff.
//3d -> screen (fixme: offscreen return values needed)
//returns false if the 2d point is offscreen.
qboolean Matrix4x4_CM_Project (const vec3_t in, vec3_t out, const vec3_t viewangles, const vec3_t vieworg, float fovx, float fovy)
{
	qboolean result = true;
	float modelview[16];
	float proj[16];

	Matrix4x4_CM_ModelViewMatrix(modelview, viewangles, vieworg);
	Matrix4x4_CM_Projection_Inf(proj, fovx, fovy, 4, true);

	{
		float v[4], tempv[4];
		v[0] = in[0];
		v[1] = in[1];
		v[2] = in[2];
		v[3] = 1;

		Matrix4x4_CM_Transform4(modelview, v, tempv); 
		Matrix4x4_CM_Transform4(proj, tempv, v);

		v[0] /= v[3];
		v[1] /= v[3];
		if (v[2] < 0)
			result = false;	//too close to the view
		v[2] /= v[3];

		out[0] = (1+v[0])/2;
		out[1] = (1+v[1])/2;
		out[2] = (1+v[2])/2;
		if (out[2] > 1)
			result = false;	//beyond far clip plane
	}
	return result;
}


//I much prefer it to take float*...
void Matrix3_Multiply (vec3_t *in1, vec3_t *in2, vec3_t *out)
{
	out[0][0] = in1[0][0]*in2[0][0] + in1[0][1]*in2[1][0] + in1[0][2]*in2[2][0];
	out[0][1] = in1[0][0]*in2[0][1] + in1[0][1]*in2[1][1] + in1[0][2]*in2[2][1];
	out[0][2] = in1[0][0]*in2[0][2] + in1[0][1]*in2[1][2] + in1[0][2]*in2[2][2];
	out[1][0] = in1[1][0]*in2[0][0] + in1[1][1]*in2[1][0] +	in1[1][2]*in2[2][0];
	out[1][1] = in1[1][0]*in2[0][1] + in1[1][1]*in2[1][1] + in1[1][2]*in2[2][1];
	out[1][2] = in1[1][0]*in2[0][2] + in1[1][1]*in2[1][2] +	in1[1][2]*in2[2][2];
	out[2][0] = in1[2][0]*in2[0][0] + in1[2][1]*in2[1][0] +	in1[2][2]*in2[2][0];
	out[2][1] = in1[2][0]*in2[0][1] + in1[2][1]*in2[1][1] +	in1[2][2]*in2[2][1];
	out[2][2] = in1[2][0]*in2[0][2] + in1[2][1]*in2[1][2] +	in1[2][2]*in2[2][2];
}

vec_t QDECL VectorNormalize2 (const vec3_t v, vec3_t out)
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);
	if (length)
	{
		ilength = 1/length;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	}
	else
	{
		VectorClear (out);
	}
		
	return length;
}
float ColorNormalize (const vec3_t in, vec3_t out)
{
	float f = max (max (in[0], in[1]), in[2]);

	if ( f > 1.0 ) {
		f = 1.0 / f;
		out[0] = in[0] * f;
		out[1] = in[1] * f;
		out[2] = in[2] * f;
	} else {
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
	}

	return f;
}

void MakeNormalVectors (const vec3_t forward, vec3_t right, vec3_t up)
{
	float		d;

	// this rotate and negat guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}

