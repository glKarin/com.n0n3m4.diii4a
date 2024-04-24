
/*
Polygon clipping routines written by Ashley Rose Hale (LadyHavoc) and placed into public domain.
*/

#include <math.h>
#include "polygon.h"

void PolygonF_QuadForPlane(float *outpoints, float planenormalx, float planenormaly, float planenormalz, float planedist, float quadsize)
{
	float d, quadright[3], quadup[3];
	if (fabs(planenormalz) > fabs(planenormalx) && fabs(planenormalz) > fabs(planenormaly))
	{
		quadup[0] = 1;
		quadup[1] = 0;
		quadup[2] = 0;
	}
	else
	{
		quadup[0] = 0;
		quadup[1] = 0;
		quadup[2] = 1;
	}
	// d = -DotProduct(quadup, planenormal);
	d = -(quadup[0] * planenormalx + quadup[1] * planenormaly + quadup[2] * planenormalz);
	// VectorMA(quadup, d, planenormal, quadup);
	quadup[0] += d * planenormalx;
	quadup[1] += d * planenormaly;
	quadup[2] += d * planenormalz;
	// VectorNormalize(quadup);
	d = (float)(1.0 / sqrt(quadup[0] * quadup[0] + quadup[1] * quadup[1] + quadup[2] * quadup[2]));
	quadup[0] *= d;
	quadup[1] *= d;
	quadup[2] *= d;
	// CrossProduct(quadup,planenormal,quadright);
	quadright[0] = quadup[1] * planenormalz - quadup[2] * planenormaly;
	quadright[1] = quadup[2] * planenormalx - quadup[0] * planenormalz;
	quadright[2] = quadup[0] * planenormaly - quadup[1] * planenormalx;
	// make the points
	outpoints[0] = planedist * planenormalx - quadsize * quadright[0] + quadsize * quadup[0];
	outpoints[1] = planedist * planenormaly - quadsize * quadright[1] + quadsize * quadup[1];
	outpoints[2] = planedist * planenormalz - quadsize * quadright[2] + quadsize * quadup[2];
	outpoints[3] = planedist * planenormalx + quadsize * quadright[0] + quadsize * quadup[0];
	outpoints[4] = planedist * planenormaly + quadsize * quadright[1] + quadsize * quadup[1];
	outpoints[5] = planedist * planenormalz + quadsize * quadright[2] + quadsize * quadup[2];
	outpoints[6] = planedist * planenormalx + quadsize * quadright[0] - quadsize * quadup[0];
	outpoints[7] = planedist * planenormaly + quadsize * quadright[1] - quadsize * quadup[1];
	outpoints[8] = planedist * planenormalz + quadsize * quadright[2] - quadsize * quadup[2];
	outpoints[9] = planedist * planenormalx - quadsize * quadright[0] - quadsize * quadup[0];
	outpoints[10] = planedist * planenormaly - quadsize * quadright[1] - quadsize * quadup[1];
	outpoints[11] = planedist * planenormalz - quadsize * quadright[2] - quadsize * quadup[2];
}

void PolygonD_QuadForPlane(double *outpoints, double planenormalx, double planenormaly, double planenormalz, double planedist, double quadsize)
{
	double d, quadright[3], quadup[3];
	if (fabs(planenormalz) > fabs(planenormalx) && fabs(planenormalz) > fabs(planenormaly))
	{
		quadup[0] = 1;
		quadup[1] = 0;
		quadup[2] = 0;
	}
	else
	{
		quadup[0] = 0;
		quadup[1] = 0;
		quadup[2] = 1;
	}
	// d = -DotProduct(quadup, planenormal);
	d = -(quadup[0] * planenormalx + quadup[1] * planenormaly + quadup[2] * planenormalz);
	// VectorMA(quadup, d, planenormal, quadup);
	quadup[0] += d * planenormalx;
	quadup[1] += d * planenormaly;
	quadup[2] += d * planenormalz;
	// VectorNormalize(quadup);
	d = 1.0 / sqrt(quadup[0] * quadup[0] + quadup[1] * quadup[1] + quadup[2] * quadup[2]);
	quadup[0] *= d;
	quadup[1] *= d;
	quadup[2] *= d;
	// CrossProduct(quadup,planenormal,quadright);
	quadright[0] = quadup[1] * planenormalz - quadup[2] * planenormaly;
	quadright[1] = quadup[2] * planenormalx - quadup[0] * planenormalz;
	quadright[2] = quadup[0] * planenormaly - quadup[1] * planenormalx;
	// make the points
	outpoints[0] = planedist * planenormalx - quadsize * quadright[0] + quadsize * quadup[0];
	outpoints[1] = planedist * planenormaly - quadsize * quadright[1] + quadsize * quadup[1];
	outpoints[2] = planedist * planenormalz - quadsize * quadright[2] + quadsize * quadup[2];
	outpoints[3] = planedist * planenormalx + quadsize * quadright[0] + quadsize * quadup[0];
	outpoints[4] = planedist * planenormaly + quadsize * quadright[1] + quadsize * quadup[1];
	outpoints[5] = planedist * planenormalz + quadsize * quadright[2] + quadsize * quadup[2];
	outpoints[6] = planedist * planenormalx + quadsize * quadright[0] - quadsize * quadup[0];
	outpoints[7] = planedist * planenormaly + quadsize * quadright[1] - quadsize * quadup[1];
	outpoints[8] = planedist * planenormalz + quadsize * quadright[2] - quadsize * quadup[2];
	outpoints[9] = planedist * planenormalx - quadsize * quadright[0] - quadsize * quadup[0];
	outpoints[10] = planedist * planenormaly - quadsize * quadright[1] - quadsize * quadup[1];
	outpoints[11] = planedist * planenormalz - quadsize * quadright[2] - quadsize * quadup[2];
}

int PolygonF_Clip(int innumpoints, const float *inpoints, float planenormalx, float planenormaly, float planenormalz, float planedist, float epsilon, int outfrontmaxpoints, float *outfrontpoints)
{
	int i, frontcount = 0;
	const float *n, *p;
	float frac, pdist, ndist;
	if (innumpoints < 1)
		return 0;
	n = inpoints;
	ndist = n[0] * planenormalx + n[1] * planenormaly + n[2] * planenormalz - planedist;
	for(i = 0;i < innumpoints;i++)
	{
		p = n;
		pdist = ndist;
		n = inpoints + ((i + 1) < innumpoints ? (i + 1) : 0) * 3;
		ndist = n[0] * planenormalx + n[1] * planenormaly + n[2] * planenormalz - planedist;
		if (pdist >= -epsilon)
		{
			if (frontcount < outfrontmaxpoints)
			{
				*outfrontpoints++ = p[0];
				*outfrontpoints++ = p[1];
				*outfrontpoints++ = p[2];
			}
			frontcount++;
		}
		if ((pdist > epsilon && ndist < -epsilon) || (pdist < -epsilon && ndist > epsilon))
		{
			frac = pdist / (pdist - ndist);
			if (frontcount < outfrontmaxpoints)
			{
				*outfrontpoints++ = p[0] + frac * (n[0] - p[0]);
				*outfrontpoints++ = p[1] + frac * (n[1] - p[1]);
				*outfrontpoints++ = p[2] + frac * (n[2] - p[2]);
			}
			frontcount++;
		}
	}
	return frontcount;
}

int PolygonD_Clip(int innumpoints, const double *inpoints, double planenormalx, double planenormaly, double planenormalz, double planedist, double epsilon, int outfrontmaxpoints, double *outfrontpoints)
{
	int i, frontcount = 0;
	const double *n, *p;
	double frac, pdist, ndist;
	if (innumpoints < 1)
		return 0;
	n = inpoints;
	ndist = n[0] * planenormalx + n[1] * planenormaly + n[2] * planenormalz - planedist;
	for(i = 0;i < innumpoints;i++)
	{
		p = n;
		pdist = ndist;
		n = inpoints + ((i + 1) < innumpoints ? (i + 1) : 0) * 3;
		ndist = n[0] * planenormalx + n[1] * planenormaly + n[2] * planenormalz - planedist;
		if (pdist >= -epsilon)
		{
			if (frontcount < outfrontmaxpoints)
			{
				*outfrontpoints++ = p[0];
				*outfrontpoints++ = p[1];
				*outfrontpoints++ = p[2];
			}
			frontcount++;
		}
		if ((pdist > epsilon && ndist < -epsilon) || (pdist < -epsilon && ndist > epsilon))
		{
			frac = pdist / (pdist - ndist);
			if (frontcount < outfrontmaxpoints)
			{
				*outfrontpoints++ = p[0] + frac * (n[0] - p[0]);
				*outfrontpoints++ = p[1] + frac * (n[1] - p[1]);
				*outfrontpoints++ = p[2] + frac * (n[2] - p[2]);
			}
			frontcount++;
		}
	}
	return frontcount;
}

void PolygonF_Divide(int innumpoints, const float *inpoints, float planenormalx, float planenormaly, float planenormalz, float planedist, float epsilon, int outfrontmaxpoints, float *outfrontpoints, int *neededfrontpoints, int outbackmaxpoints, float *outbackpoints, int *neededbackpoints, int *oncountpointer)
{
	int i, frontcount = 0, backcount = 0, oncount = 0;
	const float *n, *p;
	float frac, pdist, ndist;
	if (innumpoints)
	{
		n = inpoints;
		ndist = n[0] * planenormalx + n[1] * planenormaly + n[2] * planenormalz - planedist;
		for(i = 0;i < innumpoints;i++)
		{
			p = n;
			pdist = ndist;
			n = inpoints + ((i + 1) < innumpoints ? (i + 1) : 0) * 3;
			ndist = n[0] * planenormalx + n[1] * planenormaly + n[2] * planenormalz - planedist;
			if (pdist >= -epsilon)
			{
				if (pdist <= epsilon)
					oncount++;
				if (frontcount < outfrontmaxpoints)
				{
					*outfrontpoints++ = p[0];
					*outfrontpoints++ = p[1];
					*outfrontpoints++ = p[2];
				}
				frontcount++;
			}
			if (pdist <= epsilon)
			{
				if (backcount < outbackmaxpoints)
				{
					*outbackpoints++ = p[0];
					*outbackpoints++ = p[1];
					*outbackpoints++ = p[2];
				}
				backcount++;
			}
			if ((pdist > epsilon && ndist < -epsilon) || (pdist < -epsilon && ndist > epsilon))
			{
				oncount++;
				frac = pdist / (pdist - ndist);
				if (frontcount < outfrontmaxpoints)
				{
					*outfrontpoints++ = p[0] + frac * (n[0] - p[0]);
					*outfrontpoints++ = p[1] + frac * (n[1] - p[1]);
					*outfrontpoints++ = p[2] + frac * (n[2] - p[2]);
				}
				frontcount++;
				if (backcount < outbackmaxpoints)
				{
					*outbackpoints++ = p[0] + frac * (n[0] - p[0]);
					*outbackpoints++ = p[1] + frac * (n[1] - p[1]);
					*outbackpoints++ = p[2] + frac * (n[2] - p[2]);
				}
				backcount++;
			}
		}
	}
	if (neededfrontpoints)
		*neededfrontpoints = frontcount;
	if (neededbackpoints)
		*neededbackpoints = backcount;
	if (oncountpointer)
		*oncountpointer = oncount;
}

void PolygonD_Divide(int innumpoints, const double *inpoints, double planenormalx, double planenormaly, double planenormalz, double planedist, double epsilon, int outfrontmaxpoints, double *outfrontpoints, int *neededfrontpoints, int outbackmaxpoints, double *outbackpoints, int *neededbackpoints, int *oncountpointer)
{
	int i = 0, frontcount = 0, backcount = 0, oncount = 0;
	const double *n, *p;
	double frac, pdist, ndist;
	if (innumpoints)
	{
		n = inpoints;
		ndist = n[0] * planenormalx + n[1] * planenormaly + n[2] * planenormalz - planedist;
		for(i = 0;i < innumpoints;i++)
		{
			p = n;
			pdist = ndist;
			n = inpoints + ((i + 1) < innumpoints ? (i + 1) : 0) * 3;
			ndist = n[0] * planenormalx + n[1] * planenormaly + n[2] * planenormalz - planedist;
			if (pdist >= -epsilon)
			{
				if (pdist <= epsilon)
					oncount++;
				if (frontcount < outfrontmaxpoints)
				{
					*outfrontpoints++ = p[0];
					*outfrontpoints++ = p[1];
					*outfrontpoints++ = p[2];
				}
				frontcount++;
			}
			if (pdist <= epsilon)
			{
				if (backcount < outbackmaxpoints)
				{
					*outbackpoints++ = p[0];
					*outbackpoints++ = p[1];
					*outbackpoints++ = p[2];
				}
				backcount++;
			}
			if ((pdist > epsilon && ndist < -epsilon) || (pdist < -epsilon && ndist > epsilon))
			{
				oncount++;
				frac = pdist / (pdist - ndist);
				if (frontcount < outfrontmaxpoints)
				{
					*outfrontpoints++ = p[0] + frac * (n[0] - p[0]);
					*outfrontpoints++ = p[1] + frac * (n[1] - p[1]);
					*outfrontpoints++ = p[2] + frac * (n[2] - p[2]);
				}
				frontcount++;
				if (backcount < outbackmaxpoints)
				{
					*outbackpoints++ = p[0] + frac * (n[0] - p[0]);
					*outbackpoints++ = p[1] + frac * (n[1] - p[1]);
					*outbackpoints++ = p[2] + frac * (n[2] - p[2]);
				}
				backcount++;
			}
		}
	}
	if (neededfrontpoints)
		*neededfrontpoints = frontcount;
	if (neededbackpoints)
		*neededbackpoints = backcount;
	if (oncountpointer)
		*oncountpointer = oncount;
}

