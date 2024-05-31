/*
this code written by Ashley Rose Hale (LadyHavoc), on 2004-10-17, and placed into public domain
this implements Quadratic BSpline surfaces as seen in Quake3 by id Software

a small rant on misuse of the name 'bezier': many people seem to think that
bezier is a generic term for splines, but it is not, it is a term for a
specific type of bspline (4 control points, cubic bspline), bsplines are the
generalization of the bezier spline to support dimensions other than cubic.

example equations for 1-5 control point bsplines being sampled as t=0...1
1: flat (0th dimension)
o = a
2: linear (1st dimension)
o = a * (1 - t) + b * t
3: quadratic bspline (2nd dimension)
o = a * (1 - t) * (1 - t) + 2 * b * (1 - t) * t + c * t * t
4: cubic (bezier) bspline (3rd dimension)
o = a * (1 - t) * (1 - t) * (1 - t) + 3 * b * (1 - t) * (1 - t) * t + 3 * c * (1 - t) * t * t + d * t * t * t
5: quartic bspline (4th dimension)
o = a * (1 - t) * (1 - t) * (1 - t) * (1 - t) + 4 * b * (1 - t) * (1 - t) * (1 - t) * t + 6 * c * (1 - t) * (1 - t) * t * t + 4 * d * (1 - t) * t * t * t + e * t * t * t * t

arbitrary dimension bspline
double factorial(int n)
{
	int i;
	double f;
	f = 1;
	for (i = 1;i < n;i++)
		f = f * i;
	return f;
}
double bsplinesample(int dimensions, double t, double *param)
{
	double o = 0;
	for (i = 0;i < dimensions + 1;i++)
		o += param[i] * factorial(dimensions)/(factorial(i)*factorial(dimensions-i)) * pow(t, i) * pow(1 - t, dimensions - i);
	return o;
}
*/

#include "quakedef.h"
#include "mathlib.h"

#include <math.h>
#include "curves.h"

// Calculate number of resulting vertex rows/columns by given patch size and tesselation factor
// tess=0 means that we reduce detalization of base 3x3 patches by removing middle row and column of vertices
// "DimForTess" is "DIMension FOR TESSelation factor"
// NB: tess=0 actually means that tess must be 0.5, but obviously it can't because it is of int type. (so "a*tess"-like code is replaced by "a/2" if tess=0)
int Q3PatchDimForTess(int size, int tess)
{
	if (tess > 0)
		return (size - 1) * tess + 1;
	else if (tess == 0)
		return (size - 1) / 2 + 1;
	else
		return 0; // Maybe warn about wrong tess here?
}

// usage:
// to expand a 5x5 patch to 21x21 vertices (4x4 tesselation), one might use this call:
// Q3PatchSubdivideFloat(3, sizeof(float[3]), outvertices, 5, 5, sizeof(float[3]), patchvertices, 4, 4);
void Q3PatchTesselateFloat(int numcomponents, int outputstride, float *outputvertices, int patchwidth, int patchheight, int inputstride, float *patchvertices, int tesselationwidth, int tesselationheight)
{
	int k, l, x, y, component, outputwidth = Q3PatchDimForTess(patchwidth, tesselationwidth);
	float px, py, *v, a, b, c, *cp[3][3], temp[3][64];
	int xmax = max(1, 2*tesselationwidth);
	int ymax = max(1, 2*tesselationheight);
	
	// iterate over the individual 3x3 quadratic spline surfaces one at a time
	// expanding them to fill the output array (with some overlap to ensure
	// the edges are filled)
	for (k = 0;k < patchheight-1;k += 2)
	{
		for (l = 0;l < patchwidth-1;l += 2)
		{
			// set up control point pointers for quicker lookup later
			for (y = 0;y < 3;y++)
				for (x = 0;x < 3;x++)
					cp[y][x] = (float *)((unsigned char *)patchvertices + ((k+y)*patchwidth+(l+x)) * inputstride);
			// for each row...
			for (y = 0;y <= ymax;y++)
			{
				// calculate control points for this row by collapsing the 3
				// rows of control points to one row using py
				py = (float)y / (float)ymax;
				// calculate quadratic spline weights for py
				a = ((1.0f - py) * (1.0f - py));
				b = ((1.0f - py) * (2.0f * py));
				c = ((       py) * (       py));
				for (component = 0;component < numcomponents;component++)
				{
					temp[0][component] = cp[0][0][component] * a + cp[1][0][component] * b + cp[2][0][component] * c;
					temp[1][component] = cp[0][1][component] * a + cp[1][1][component] * b + cp[2][1][component] * c;
					temp[2][component] = cp[0][2][component] * a + cp[1][2][component] * b + cp[2][2][component] * c;
				}
				// fetch a pointer to the beginning of the output vertex row
				v = (float *)((unsigned char *)outputvertices + ((k * ymax / 2 + y) * outputwidth + l * xmax / 2) * outputstride);
				// for each column of the row...
				for (x = 0;x <= xmax;x++)
				{
					// calculate point based on the row control points
					px = (float)x / (float)xmax;
					// calculate quadratic spline weights for px
					// (could be precalculated)
					a = ((1.0f - px) * (1.0f - px));
					b = ((1.0f - px) * (2.0f * px));
					c = ((       px) * (       px));
					for (component = 0;component < numcomponents;component++)
						v[component] = temp[0][component] * a + temp[1][component] * b + temp[2][component] * c;
					// advance to next output vertex using outputstride
					// (the next vertex may not be directly following this
					// one, as this may be part of a larger structure)
					v = (float *)((unsigned char *)v + outputstride);
				}
			}
		}
	}
#if 0
	// enable this if you want results printed out
	printf("vertices[%i][%i] =\n{\n", (patchheight-1)*tesselationheight+1, (patchwidth-1)*tesselationwidth+1);
	for (y = 0;y < (patchheight-1)*tesselationheight+1;y++)
	{
		for (x = 0;x < (patchwidth-1)*tesselationwidth+1;x++)
		{
			printf("(");
			for (component = 0;component < numcomponents;component++)
				printf("%f ", outputvertices[(y*((patchwidth-1)*tesselationwidth+1)+x)*numcomponents+component]);
			printf(") ");
		}
		printf("\n");
	}
	printf("}\n");
#endif
}

static int Q3PatchTesselation(float largestsquared3xcurvearea, float tolerance)
{
	float f;
	// f is actually a squared 2x curve area... so the formula had to be adjusted to give roughly the same subdivisions
	f = pow(largestsquared3xcurvearea / 64.0f, 0.25f) / tolerance;
	//if(f < 0.25) // VERY flat patches
	if(f < 0.0001) // TOTALLY flat patches
		return 0;
	else if(f < 2)
		return 1;
	else
		return (int) floor(log(f) / log(2.0f)) + 1;
		// this is always at least 2
		// maps [0.25..0.5[ to -1 (actually, 1 is returned)
		// maps [0.5..1[ to 0 (actually, 1 is returned)
		// maps [1..2[ to 1
		// maps [2..4[ to 2
		// maps [4..8[ to 4
}

static float Squared3xCurveArea(const float *a, const float *control, const float *b, int components)
{
#if 0
	// mimicing the old behaviour with the new code...

	float deviation;
	float quartercurvearea = 0;
	int c;
	for (c = 0;c < components;c++)
	{
		deviation = control[c] * 0.5f - a[c] * 0.25f - b[c] * 0.25f;
		quartercurvearea += deviation*deviation;
	}

	// But as the new code now works on the squared 2x curve area, let's scale the value
	return quartercurvearea * quartercurvearea * 64.0;

#else
	// ideally, we'd like the area between the spline a->control->b and the line a->b.
	// but as this is hard to calculate, let's calculate an upper bound of it:
	// the area of the triangle a->control->b->a.
	//
	// one can prove that the area of a quadratic spline = 2/3 * the area of
	// the triangle of its control points!
	// to do it, first prove it for the spline through (0,0), (1,1), (2,0)
	// (which is a parabola) and then note that moving the control point
	// left/right is just shearing and keeps the area of both the spline and
	// the triangle invariant.
	//
	// why are we going for the spline area anyway?
	// we know that:
	//
	//   the area between the spline and the line a->b is a measure of the
	//   error of approximation of the spline by the line.
	//
	//   also, on circle-like or parabola-like curves, you easily get that the
	//   double amount of line approximation segments reduces the error to its quarter
	//   (also, easy to prove for splines by doing it for one specific one, and using
	//   affine transforms to get all other splines)
	//
	// so...
	//
	// let's calculate the area! but we have to avoid the cross product, as
	// components is not necessarily 3
	//
	// the area of a triangle spanned by vectors a and b is
	//
	// 0.5 * |a| |b| sin gamma
	//
	// now, cos gamma is
	//
	// a.b / (|a| |b|)
	//
	// so the area is
	// 
	// 0.5 * sqrt(|a|^2 |b|^2 - (a.b)^2)
	int c;
	float aa = 0, bb = 0, ab = 0;
	for (c = 0;c < components;c++)
	{
		float xa = a[c] - control[c];
		float xb = b[c] - control[c];
		aa += xa * xa;
		ab += xa * xb;
		bb += xb * xb;
	}
	// area is 0.5 * sqrt(aa*bb - ab*ab)
	// 2x TRIANGLE area is sqrt(aa*bb - ab*ab)
	// 3x CURVE area is sqrt(aa*bb - ab*ab)
	return aa * bb - ab * ab;
#endif
}

// returns how much tesselation of each segment is needed to remain under tolerance
int Q3PatchTesselationOnX(int patchwidth, int patchheight, int components, const float *in, float tolerance)
{
	int x, y;
	const float *patch;
	float squared3xcurvearea, largestsquared3xcurvearea;
	largestsquared3xcurvearea = 0;
	for (y = 0;y < patchheight;y++)
	{
		for (x = 0;x < patchwidth-1;x += 2)
		{
			patch = in + ((y * patchwidth) + x) * components;
			squared3xcurvearea = Squared3xCurveArea(&patch[0], &patch[components], &patch[2*components], components);
			if (largestsquared3xcurvearea < squared3xcurvearea)
				largestsquared3xcurvearea = squared3xcurvearea;
		}
	}
	return Q3PatchTesselation(largestsquared3xcurvearea, tolerance);
}

// returns how much tesselation of each segment is needed to remain under tolerance
int Q3PatchTesselationOnY(int patchwidth, int patchheight, int components, const float *in, float tolerance)
{
	int x, y;
	const float *patch;
	float squared3xcurvearea, largestsquared3xcurvearea;
	largestsquared3xcurvearea = 0;
	for (y = 0;y < patchheight-1;y += 2)
	{
		for (x = 0;x < patchwidth;x++)
		{
			patch = in + ((y * patchwidth) + x) * components;
			squared3xcurvearea = Squared3xCurveArea(&patch[0], &patch[patchwidth*components], &patch[2*patchwidth*components], components);
			if (largestsquared3xcurvearea < squared3xcurvearea)
				largestsquared3xcurvearea = squared3xcurvearea;
		}
	}
	return Q3PatchTesselation(largestsquared3xcurvearea, tolerance);
}

// Find an equal vertex in array. Check only vertices with odd X and Y
static int FindEqualOddVertexInArray(int numcomponents, float *vertex, float *vertices, int width, int height)
{
	int x, y, j;
	for (y=0; y<height; y+=2)
	{
		for (x=0; x<width; x+=2)
		{
			qbool found = true;
			for (j=0; j<numcomponents; j++)
				if (fabs(*(vertex+j) - *(vertices+j)) > 0.05)
				// div0: this is notably smaller than the smallest radiant grid
				// but large enough so we don't need to get scared of roundoff
				// errors
				{
					found = false;
					break;
				}
			if(found)
				return y*width+x;
			vertices += numcomponents*2;
		}
		vertices += numcomponents*(width-1);
	}
	return -1;
}

#define SIDE_INVALID -1
#define SIDE_X 0
#define SIDE_Y 1

static int GetSide(int p1, int p2, int width, int height, int *pointdist)
{
	int x1 = p1 % width, y1 = p1 / width;
	int x2 = p2 % width, y2 = p2 / width;
	if (p1 < 0 || p2 < 0)
		return SIDE_INVALID;
	if (x1 == x2)
	{
		if (y1 != y2)
		{
			*pointdist = abs(y2 - y1);
			return SIDE_Y;
		}
		else
			return SIDE_INVALID;
	}
	else if (y1 == y2)
	{
		*pointdist = abs(x2 - x1);
		return SIDE_X;
	}
	else
		return SIDE_INVALID;
}

// Increase tesselation of one of two touching patches to make a seamless connection between them
// Returns 0 in case if patches were not modified, otherwise 1
int Q3PatchAdjustTesselation(int numcomponents, patchinfo_t *patch1, float *patchvertices1, patchinfo_t *patch2, float *patchvertices2)
{
	// what we are doing here is:
	//   we take for each corner of one patch
	//   and check if the other patch contains that corner
	//   once we have a pair of such matches

	struct {int id1,id2;} commonverts[8];
	int i, j, k, side1, side2, *tess1, *tess2;
	int dist1 = 0, dist2 = 0;
	qbool modified = false;

	// Potential paired vertices (corners of the first patch)
	commonverts[0].id1 = 0;
	commonverts[1].id1 = patch1->xsize-1;
	commonverts[2].id1 = patch1->xsize*(patch1->ysize-1);
	commonverts[3].id1 = patch1->xsize*patch1->ysize-1;
	for (i=0;i<4;++i)
		commonverts[i].id2 = FindEqualOddVertexInArray(numcomponents, patchvertices1+numcomponents*commonverts[i].id1, patchvertices2, patch2->xsize, patch2->ysize);

	// Corners of the second patch
	commonverts[4].id2 = 0;
	commonverts[5].id2 = patch2->xsize-1;
	commonverts[6].id2 = patch2->xsize*(patch2->ysize-1);
	commonverts[7].id2 = patch2->xsize*patch2->ysize-1;
	for (i=4;i<8;++i)
		commonverts[i].id1 = FindEqualOddVertexInArray(numcomponents, patchvertices2+numcomponents*commonverts[i].id2, patchvertices1, patch1->xsize, patch1->ysize);

	for (i=0;i<8;++i)
		for (j=i+1;j<8;++j)
		{
			side1 = GetSide(commonverts[i].id1,commonverts[j].id1,patch1->xsize,patch1->ysize,&dist1);
			side2 = GetSide(commonverts[i].id2,commonverts[j].id2,patch2->xsize,patch2->ysize,&dist2);

			if (side1 == SIDE_INVALID || side2 == SIDE_INVALID)
				continue;

			if(dist1 != dist2)
			{
				// no patch welding if the resolutions mismatch
				continue;
			}

			// Update every lod level
			for (k=0;k<PATCH_LODS_NUM;++k)
			{
				tess1 = side1 == SIDE_X ? &patch1->lods[k].xtess : &patch1->lods[k].ytess;
				tess2 = side2 == SIDE_X ? &patch2->lods[k].xtess : &patch2->lods[k].ytess;
				if (*tess1 != *tess2)
				{
					if (*tess1 < *tess2)
						*tess1 = *tess2;
					else
						*tess2 = *tess1;
					modified = true;
				}
			}
		}

	return modified;
}

#undef SIDE_INVALID 
#undef SIDE_X
#undef SIDE_Y

// calculates elements for a grid of vertices
// (such as those produced by Q3PatchTesselate)
// (note: width and height are the actual vertex size, this produces
// (width-1)*(height-1)*2 triangles, 3 elements each)
void Q3PatchTriangleElements(int *elements, int width, int height, int firstvertex)
{
	int x, y, row0, row1;
	for (y = 0;y < height - 1;y++)
	{
		if(y % 2)
		{
			// swap the triangle order in odd rows as optimization for collision stride
			row0 = firstvertex + (y + 0) * width + width - 2;
			row1 = firstvertex + (y + 1) * width + width - 2;
			for (x = 0;x < width - 1;x++)
			{
				*elements++ = row1;
				*elements++ = row1 + 1;
				*elements++ = row0 + 1;
				*elements++ = row0;
				*elements++ = row1;
				*elements++ = row0 + 1;
				row0--;
				row1--;
			}
		}
		else
		{
			row0 = firstvertex + (y + 0) * width;
			row1 = firstvertex + (y + 1) * width;
			for (x = 0;x < width - 1;x++)
			{
				*elements++ = row0;
				*elements++ = row1;
				*elements++ = row0 + 1;
				*elements++ = row1;
				*elements++ = row1 + 1;
				*elements++ = row0 + 1;
				row0++;
				row1++;
			}
		}
	}
}

