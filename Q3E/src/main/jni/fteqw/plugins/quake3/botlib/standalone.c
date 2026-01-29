//license GPLv2+
//this file allows botlib to link as a dynamic lib with no external dependancies

#include "q_shared.h"
#include "botlib.h"

vec3_t vec3_origin;
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
void QDECL VectorAngles(float *forward, float *up, float *result, qboolean meshpitch)
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
//	if (meshpitch)
//		pitch *= r_meshpitch.value;
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
float QDECL VectorNormalize (vec3_t v)
{
	return VectorNormalize2(v,v);
}

void	QDECL Com_sprintf (char *dest, int size, const char *fmt, ...)
{
	va_list		argptr;

	va_start (argptr, fmt);
	vsnprintf (dest, size, fmt, argptr);
	va_end (argptr);
}

int Q_strncasecmp (const char *s1, const char *s2, int n)
{
	int		c1, c2;

	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
			{	// strings not equal
				if (c1 > c2)
					return 1;		// strings not equal
				return -1;
			}
		}
		if (!c1)
			return 0;		// strings are equal
//		s1++;
//		s2++;
	}

	return -1;
}
int Q_strcasecmp (const char *s1, const char *s2)
{
	return Q_strncasecmp (s1, s2, 0x7fffffff);
}

int QDECL Q_stricmp (const char *s1, const char *s2)
{
	return Q_strncasecmp (s1, s2, 0x7fffffff);
}

void QDECL Q_strncpyz(char *d, const char *s, int n)
{
	int i;
	n--;
	if (n < 0)
		return;	//this could be an error

	for (i=0; *s; i++)
	{
		if (i == n)
			break;
		*d++ = *s++;
	}
	*d='\0';
}

/*
char	*QDECL va(char *format, ...)
{
#define VA_BUFFER_SIZE 1024
	va_list		argptr;
	static char		string[VA_BUFFER_SIZE];

	va_start (argptr, format);
	vsnprintf (string,sizeof(string)-1, format,argptr);
	va_end (argptr);

	return string;
}
*/