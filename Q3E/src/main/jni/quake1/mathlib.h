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
// mathlib.h

#ifndef MATHLIB_H
#define MATHLIB_H

#include "qtypes.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

struct mplane_s;
extern vec3_t vec3_origin;

#define bound(min,num,max) ((num) >= (min) ? ((num) < (max) ? (num) : (max)) : (min))

#ifndef min
#define min(A,B) ((A) < (B) ? (A) : (B))
#define max(A,B) ((A) > (B) ? (A) : (B))
#endif

/// LadyHavoc: this function never returns exactly MIN or exactly MAX, because
/// of a QuakeC bug in id1 where the line
/// self.nextthink = self.nexthink + random() * 0.5;
/// can result in 0 (self.nextthink is 0 at this point in the code to begin
/// with), causing "stone monsters" that never spawned properly, also MAX is
/// avoided because some people use random() as an index into arrays or for
/// loop conditions, where hitting exactly MAX may be a fatal error
#define lhrandom(MIN,MAX) (((double)(rand() + 0.5) / ((double)RAND_MAX + 1)) * ((MAX)-(MIN)) + (MIN))

#define invpow(base,number) (log(number) / log(base))

/// returns log base 2 of "n"
/// \WARNING: "n" MUST be a power of 2!
#define log2i(n) ((((n) & 0xAAAAAAAA) != 0 ? 1 : 0) | (((n) & 0xCCCCCCCC) != 0 ? 2 : 0) | (((n) & 0xF0F0F0F0) != 0 ? 4 : 0) | (((n) & 0xFF00FF00) != 0 ? 8 : 0) | (((n) & 0xFFFF0000) != 0 ? 16 : 0))

/// \TODO: what is this function supposed to do?
#define bit2i(n) log2i((n) << 1)

/// boolean XOR (why doesn't C have the ^^ operator for this purpose?)
#define boolxor(a,b) (!(a) != !(b))

/// returns the smallest integer greater than or equal to "value", or 0 if "value" is too big
unsigned int CeilPowerOf2(unsigned int value);

#define DEG2RAD(a) ((a) * ((float) M_PI / 180.0f))
#define RAD2DEG(a) ((a) * (180.0f / (float) M_PI))
#define ANGLEMOD(a) ((a) - 360.0 * floor((a) / 360.0))

#define Q_rint(x) ((x) > 0 ? (int)((x) + 0.5) : (int)((x) - 0.5)) //johnfitz -- from joequake

#define DotProduct2(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1])
#define Vector2Clear(a) ((a)[0]=(a)[1]=0)
#define Vector2Compare(a,b) (((a)[0]==(b)[0])&&((a)[1]==(b)[1]))
#define Vector2Copy(in,out) ((out)[0]=(in)[0],(out)[1]=(in)[1])
#define Vector2Negate(in,out) ((out)[0]=-((in)[0]),(out)[1]=-((in)[1]))
#define Vector2Set(vec,x,y) ((vec)[0]=(x),(vec)[1]=(y))
#define Vector2Scale(in, scale, out) ((out)[0] = (in)[0] * (scale),(out)[1] = (in)[1] * (scale))
#define Vector2Normalize2(v,dest) {float ilength = (float)DotProduct2((v),(v));if (ilength) ilength = 1.0f / sqrt(ilength);dest[0] = (v)[0] * ilength;dest[1] = (v)[1] * ilength;}
#define Vector2Length(a) (sqrt(DotProduct2(a, a)))

#define DotProduct4(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2]+(a)[3]*(b)[3])
#define Vector4Clear(a) ((a)[0]=(a)[1]=(a)[2]=(a)[3]=0)
#define Vector4Compare(a,b) (((a)[0]==(b)[0])&&((a)[1]==(b)[1])&&((a)[2]==(b)[2])&&((a)[3]==(b)[3]))
#define Vector4Copy(in,out) ((out)[0]=(in)[0],(out)[1]=(in)[1],(out)[2]=(in)[2],(out)[3]=(in)[3])
#define Vector4Negate(in,out) ((out)[0]=-((in)[0]),(out)[1]=-((in)[1]),(out)[2]=-((in)[2]),(out)[3]=-((in)[3]))
#define Vector4Set(vec,r,g,b,a) ((vec)[0]=(r),(vec)[1]=(g),(vec)[2]=(b),(vec)[3]=(a))
#define Vector4Normalize2(v,dest) {float ilength = (float)DotProduct4((v),(v));if (ilength) ilength = 1.0f / sqrt(ilength);dest[0] = (v)[0] * ilength;dest[1] = (v)[1] * ilength;dest[2] = (v)[2] * ilength;dest[3] = (v)[3] * ilength;}
#define Vector4Subtract(a,b,out) ((out)[0]=(a)[0]-(b)[0],(out)[1]=(a)[1]-(b)[1],(out)[2]=(a)[2]-(b)[2],(out)[3]=(a)[3]-(b)[3])
#define Vector4Add(a,b,out) ((out)[0]=(a)[0]+(b)[0],(out)[1]=(a)[1]+(b)[1],(out)[2]=(a)[2]+(b)[2],(out)[3]=(a)[3]+(b)[3])
#define Vector4Scale(in, scale, out) ((out)[0] = (in)[0] * (scale),(out)[1] = (in)[1] * (scale),(out)[2] = (in)[2] * (scale),(out)[3] = (in)[3] * (scale))
#define Vector4Multiply(a,b,out) ((out)[0]=(a)[0]*(b)[0],(out)[1]=(a)[1]*(b)[1],(out)[2]=(a)[2]*(b)[2],(out)[3]=(a)[3]*(b)[3])
#define Vector4MA(a, scale, b, out) ((out)[0] = (a)[0] + (scale) * (b)[0],(out)[1] = (a)[1] + (scale) * (b)[1],(out)[2] = (a)[2] + (scale) * (b)[2],(out)[3] = (a)[3] + (scale) * (b)[3])
#define Vector4Lerp(v1,lerp,v2,out) ((out)[0] = (v1)[0] + (lerp) * ((v2)[0] - (v1)[0]), (out)[1] = (v1)[1] + (lerp) * ((v2)[1] - (v1)[1]), (out)[2] = (v1)[2] + (lerp) * ((v2)[2] - (v1)[2]), (out)[3] = (v1)[3] + (lerp) * ((v2)[3] - (v1)[3]))

#define VectorNegate(a,b) ((b)[0]=-((a)[0]),(b)[1]=-((a)[1]),(b)[2]=-((a)[2]))
#define VectorSet(vec,x,y,z) ((vec)[0]=(x),(vec)[1]=(y),(vec)[2]=(z))
#define VectorClear(a) ((a)[0]=(a)[1]=(a)[2]=0)
#define DotProduct(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VectorSubtract(a,b,out) ((out)[0]=(a)[0]-(b)[0],(out)[1]=(a)[1]-(b)[1],(out)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,out) ((out)[0]=(a)[0]+(b)[0],(out)[1]=(a)[1]+(b)[1],(out)[2]=(a)[2]+(b)[2])
#define VectorCopy(in,out) ((out)[0]=(in)[0],(out)[1]=(in)[1],(out)[2]=(in)[2])
#define VectorMultiply(a,b,out) ((out)[0]=(a)[0]*(b)[0],(out)[1]=(a)[1]*(b)[1],(out)[2]=(a)[2]*(b)[2])
#define CrossProduct(a,b,out) ((out)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1],(out)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2],(out)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0])
#define VectorNormalize(v) {float ilength = (float)DotProduct((v),(v));if (ilength) ilength = 1.0f / sqrt(ilength);(v)[0] *= ilength;(v)[1] *= ilength;(v)[2] *= ilength;}
#define VectorNormalize2(v,dest) {float ilength = (float)DotProduct((v),(v));if (ilength) ilength = 1.0f / sqrt(ilength);dest[0] = (v)[0] * ilength;dest[1] = (v)[1] * ilength;dest[2] = (v)[2] * ilength;}
#define VectorNormalizeDouble(v) {double ilength = DotProduct((v),(v));if (ilength) ilength = 1.0 / sqrt(ilength);(v)[0] *= ilength;(v)[1] *= ilength;(v)[2] *= ilength;}
#define VectorDistance2(a, b) (((a)[0] - (b)[0]) * ((a)[0] - (b)[0]) + ((a)[1] - (b)[1]) * ((a)[1] - (b)[1]) + ((a)[2] - (b)[2]) * ((a)[2] - (b)[2]))
#define VectorDistance(a, b) (sqrt(VectorDistance2(a,b)))
#define VectorLength(a) (sqrt((double)DotProduct(a, a)))
#define VectorLength2(a) (DotProduct(a, a))
#define VectorScale(in, scale, out) ((out)[0] = (in)[0] * (scale),(out)[1] = (in)[1] * (scale),(out)[2] = (in)[2] * (scale))
#define VectorScaleCast(in, scale, outtype, out) ((out)[0] = (outtype) ((in)[0] * (scale)),(out)[1] = (outtype) ((in)[1] * (scale)),(out)[2] = (outtype) ((in)[2] * (scale)))
#define VectorCompare(a,b) (((a)[0]==(b)[0])&&((a)[1]==(b)[1])&&((a)[2]==(b)[2]))
#define VectorMA(a, scale, b, out) ((out)[0] = (a)[0] + (scale) * (b)[0],(out)[1] = (a)[1] + (scale) * (b)[1],(out)[2] = (a)[2] + (scale) * (b)[2])
#define VectorM(scale1, b1, out) ((out)[0] = (scale1) * (b1)[0],(out)[1] = (scale1) * (b1)[1],(out)[2] = (scale1) * (b1)[2])
#define VectorMAM(scale1, b1, scale2, b2, out) ((out)[0] = (scale1) * (b1)[0] + (scale2) * (b2)[0],(out)[1] = (scale1) * (b1)[1] + (scale2) * (b2)[1],(out)[2] = (scale1) * (b1)[2] + (scale2) * (b2)[2])
#define VectorMAMAM(scale1, b1, scale2, b2, scale3, b3, out) ((out)[0] = (scale1) * (b1)[0] + (scale2) * (b2)[0] + (scale3) * (b3)[0],(out)[1] = (scale1) * (b1)[1] + (scale2) * (b2)[1] + (scale3) * (b3)[1],(out)[2] = (scale1) * (b1)[2] + (scale2) * (b2)[2] + (scale3) * (b3)[2])
#define VectorMAMAMAM(scale1, b1, scale2, b2, scale3, b3, scale4, b4, out) ((out)[0] = (scale1) * (b1)[0] + (scale2) * (b2)[0] + (scale3) * (b3)[0] + (scale4) * (b4)[0],(out)[1] = (scale1) * (b1)[1] + (scale2) * (b2)[1] + (scale3) * (b3)[1] + (scale4) * (b4)[1],(out)[2] = (scale1) * (b1)[2] + (scale2) * (b2)[2] + (scale3) * (b3)[2] + (scale4) * (b4)[2])
#define VectorRandom(v) do{(v)[0] = lhrandom(-1, 1);(v)[1] = lhrandom(-1, 1);(v)[2] = lhrandom(-1, 1);}while(DotProduct(v, v) > 1)
#define VectorLerp(v1,lerp,v2,out) ((out)[0] = (v1)[0] + (lerp) * ((v2)[0] - (v1)[0]), (out)[1] = (v1)[1] + (lerp) * ((v2)[1] - (v1)[1]), (out)[2] = (v1)[2] + (lerp) * ((v2)[2] - (v1)[2]))
#define VectorReflect(a,r,b,out) do{double d;d = DotProduct((a), (b)) * -(1.0 + (r));VectorMA((a), (d), (b), (out));}while(0)
#define BoxesOverlap(a,b,c,d) ((a)[0] <= (d)[0] && (b)[0] >= (c)[0] && (a)[1] <= (d)[1] && (b)[1] >= (c)[1] && (a)[2] <= (d)[2] && (b)[2] >= (c)[2])
#define BoxInsideBox(a,b,c,d) ((a)[0] >= (c)[0] && (b)[0] <= (d)[0] && (a)[1] >= (c)[1] && (b)[1] <= (d)[1] && (a)[2] >= (c)[2] && (b)[2] <= (d)[2])
#define TriangleBBoxOverlapsBox(a,b,c,d,e) (min((a)[0], min((b)[0], (c)[0])) < (e)[0] && max((a)[0], max((b)[0], (c)[0])) > (d)[0] && min((a)[1], min((b)[1], (c)[1])) < (e)[1] && max((a)[1], max((b)[1], (c)[1])) > (d)[1] && min((a)[2], min((b)[2], (c)[2])) < (e)[2] && max((a)[2], max((b)[2], (c)[2])) > (d)[2])

#define TriangleNormal(a,b,c,n) ( \
	(n)[0] = ((a)[1] - (b)[1]) * ((c)[2] - (b)[2]) - ((a)[2] - (b)[2]) * ((c)[1] - (b)[1]), \
	(n)[1] = ((a)[2] - (b)[2]) * ((c)[0] - (b)[0]) - ((a)[0] - (b)[0]) * ((c)[2] - (b)[2]), \
	(n)[2] = ((a)[0] - (b)[0]) * ((c)[1] - (b)[1]) - ((a)[1] - (b)[1]) * ((c)[0] - (b)[0]) \
	)

/*! Fast PointInfrontOfTriangle.
 * subtracts v1 from v0 and v2, combined into a crossproduct, combined with a
 * dotproduct of the light location relative to the first point of the
 * triangle (any point works, since any triangle is obviously flat), and
 * finally a comparison to determine if the light is infront of the triangle
 * (the goal of this statement) we do not need to normalize the surface
 * normal because both sides of the comparison use it, therefore they are
 * both multiplied the same amount...  furthermore a subtract can be done on
 * the point to eliminate one dotproduct
 * this is ((p - a) * cross(a-b,c-b))
 */
#define PointInfrontOfTriangle(p,a,b,c) \
( ((p)[0] - (a)[0]) * (((a)[1] - (b)[1]) * ((c)[2] - (b)[2]) - ((a)[2] - (b)[2]) * ((c)[1] - (b)[1])) \
+ ((p)[1] - (a)[1]) * (((a)[2] - (b)[2]) * ((c)[0] - (b)[0]) - ((a)[0] - (b)[0]) * ((c)[2] - (b)[2])) \
+ ((p)[2] - (a)[2]) * (((a)[0] - (b)[0]) * ((c)[1] - (b)[1]) - ((a)[1] - (b)[1]) * ((c)[0] - (b)[0])) > 0)

#if 0
// readable version, kept only for explanatory reasons
int PointInfrontOfTriangle(const float *p, const float *a, const float *b, const float *c)
{
	float dir0[3], dir1[3], normal[3];

	// calculate two mostly perpendicular edge directions
	VectorSubtract(a, b, dir0);
	VectorSubtract(c, b, dir1);

	// we have two edge directions, we can calculate a third vector from
	// them, which is the direction of the surface normal (its magnitude
	// is not 1 however)
	CrossProduct(dir0, dir1, normal);

	// compare distance of light along normal, with distance of any point
	// of the triangle along the same normal (the triangle is planar,
	// I.E. flat, so all points give the same answer)
	return DotProduct(p, normal) > DotProduct(a, normal);
}
#endif

#define lhcheeserand(seed) ((seed) = ((seed) * 987211u) ^ ((seed) >> 13u) ^ 914867)
#define lhcheeserandom(seed,MIN,MAX) ((double)(lhcheeserand(seed) + 0.5) / ((double)4096.0*1024.0*1024.0) * ((MAX)-(MIN)) + (MIN))
#define VectorCheeseRandom(seed,v) do{(v)[0] = lhcheeserandom(seed,-1, 1);(v)[1] = lhcheeserandom(seed,-1, 1);(v)[2] = lhcheeserandom(seed,-1, 1);}while(DotProduct(v, v) > 1)
#define VectorLehmerRandom(seed,v) do{(v)[0] = Math_crandomf(seed);(v)[1] = Math_crandomf(seed);(v)[2] = Math_crandomf(seed);}while(DotProduct(v, v) > 1)

/*
// LadyHavoc: quaternion math, untested, don't know if these are correct,
// need to add conversion to/from matrices
// LadyHavoc: later note: the matrix faq is useful: http://skal.planet-d.net/demo/matrixfaq.htm
// LadyHavoc: these are probably very wrong and I'm not sure I care, not used by anything

// returns length of quaternion
#define qlen(a) ((float) sqrt((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2]+(a)[3]*(a)[3]))
// returns squared length of quaternion
#define qlen2(a) ((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2]+(a)[3]*(a)[3])
// makes a quaternion from x, y, z, and a rotation angle (in degrees)
#define QuatMake(x,y,z,r,c)\
{\
if (r == 0)\
{\
(c)[0]=(float) ((x) * (1.0f / 0.0f));\
(c)[1]=(float) ((y) * (1.0f / 0.0f));\
(c)[2]=(float) ((z) * (1.0f / 0.0f));\
(c)[3]=(float) 1.0f;\
}\
else\
{\
float r2 = (r) * 0.5 * (M_PI / 180);\
float r2is = 1.0f / sin(r2);\
(c)[0]=(float) ((x)/r2is);\
(c)[1]=(float) ((y)/r2is);\
(c)[2]=(float) ((z)/r2is);\
(c)[3]=(float) (cos(r2));\
}\
}
// makes a quaternion from a vector and a rotation angle (in degrees)
#define QuatFromVec(a,r,c) QuatMake((a)[0],(a)[1],(a)[2],(r))
// copies a quaternion
#define QuatCopy(a,c) {(c)[0]=(a)[0];(c)[1]=(a)[1];(c)[2]=(a)[2];(c)[3]=(a)[3];}
#define QuatSubtract(a,b,c) {(c)[0]=(a)[0]-(b)[0];(c)[1]=(a)[1]-(b)[1];(c)[2]=(a)[2]-(b)[2];(c)[3]=(a)[3]-(b)[3];}
#define QuatAdd(a,b,c) {(c)[0]=(a)[0]+(b)[0];(c)[1]=(a)[1]+(b)[1];(c)[2]=(a)[2]+(b)[2];(c)[3]=(a)[3]+(b)[3];}
#define QuatScale(a,b,c) {(c)[0]=(a)[0]*b;(c)[1]=(a)[1]*b;(c)[2]=(a)[2]*b;(c)[3]=(a)[3]*b;}
// FIXME: this is wrong, do some more research on quaternions
//#define QuatMultiply(a,b,c) {(c)[0]=(a)[0]*(b)[0];(c)[1]=(a)[1]*(b)[1];(c)[2]=(a)[2]*(b)[2];(c)[3]=(a)[3]*(b)[3];}
// FIXME: this is wrong, do some more research on quaternions
//#define QuatMultiplyAdd(a,b,d,c) {(c)[0]=(a)[0]*(b)[0]+d[0];(c)[1]=(a)[1]*(b)[1]+d[1];(c)[2]=(a)[2]*(b)[2]+d[2];(c)[3]=(a)[3]*(b)[3]+d[3];}
#define qdist(a,b) ((float) sqrt(((b)[0]-(a)[0])*((b)[0]-(a)[0])+((b)[1]-(a)[1])*((b)[1]-(a)[1])+((b)[2]-(a)[2])*((b)[2]-(a)[2])+((b)[3]-(a)[3])*((b)[3]-(a)[3])))
#define qdist2(a,b) (((b)[0]-(a)[0])*((b)[0]-(a)[0])+((b)[1]-(a)[1])*((b)[1]-(a)[1])+((b)[2]-(a)[2])*((b)[2]-(a)[2])+((b)[3]-(a)[3])*((b)[3]-(a)[3]))
*/

#define VectorCopy4(a,b) {(b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2];(b)[3]=(a)[3];}

/// returns vector length
float VectorNormalizeLength (vec3_t v);

#define NUMVERTEXNORMALS	162
extern float m_bytenormals[NUMVERTEXNORMALS][3];

unsigned char NormalToByte(const vec3_t n);
void ByteToNormal(unsigned char num, vec3_t n);

void R_ConcatRotations (const float in1[3*3], const float in2[3*3], float out[3*3]);
void R_ConcatTransforms (const float in1[3*4], const float in2[3*4], float out[3*4]);

void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
/// LadyHavoc: proper matrix version of AngleVectors
void AngleVectorsFLU (const vec3_t angles, vec3_t forward, vec3_t left, vec3_t up);
/// divVerent: improper matrix version of AngleVectors
void AngleVectorsDuke3DFLU (const vec3_t angles, vec3_t forward, vec3_t left, vec3_t up, double maxShearAngle);
/// LadyHavoc: builds a [3][4] matrix
void AngleMatrix (const vec3_t angles, const vec3_t translate, vec_t matrix[][4]);
/// LadyHavoc: calculates pitch/yaw/roll angles from forward and up vectors
void AnglesFromVectors (vec3_t angles, const vec3_t forward, const vec3_t up, qbool flippitch);

/// LadyHavoc: like AngleVectors, but taking a forward vector instead of angles, useful!
void VectorVectors(const vec3_t forward, vec3_t right, vec3_t up);
void VectorVectorsDouble(const double *forward, double *right, double *up);

void PlaneClassify(struct mplane_s *p);
int BoxOnPlaneSide(const vec3_t emins, const vec3_t emaxs, const struct mplane_s *p);
int BoxOnPlaneSide_Separate(const vec3_t emins, const vec3_t emaxs, const vec3_t normal, const vec_t dist);
void BoxPlaneCorners(const vec3_t emins, const vec3_t emaxs, const struct mplane_s *p, vec3_t outnear, vec3_t outfar);
void BoxPlaneCorners_Separate(const vec3_t emins, const vec3_t emaxs, const vec3_t normal, vec3_t outnear, vec3_t outfar);
void BoxPlaneCornerDistances(const vec3_t emins, const vec3_t emaxs, const struct mplane_s *p, vec_t *outnear, vec_t *outfar);
void BoxPlaneCornerDistances_Separate(const vec3_t emins, const vec3_t emaxs, const vec3_t normal, vec_t *outnear, vec_t *outfar);

#define PlaneDist(point,plane)  ((plane)->type < 3 ? (point)[(plane)->type] : DotProduct((point), (plane)->normal))
#define PlaneDiff(point,plane) (((plane)->type < 3 ? (point)[(plane)->type] : DotProduct((point), (plane)->normal)) - (plane)->dist)

/// LadyHavoc: minimal plane structure
typedef struct tinyplane_s
{
	float normal[3], dist;
}
tinyplane_t;

typedef struct tinydoubleplane_s
{
	double normal[3], dist;
}
tinydoubleplane_t;

void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);

float RadiusFromBounds (const vec3_t mins, const vec3_t maxs);
float RadiusFromBoundsAndOrigin (const vec3_t mins, const vec3_t maxs, const vec3_t origin);

struct matrix4x4_s;
/// print a matrix to the console
void Matrix4x4_Print(const struct matrix4x4_s *in);
int Math_atov(const char *s, prvm_vec3_t out);

void BoxFromPoints(vec3_t mins, vec3_t maxs, int numpoints, vec_t *point3f);

int LoopingFrameNumberFromDouble(double t, int loopframes);

// implementation of 128bit Lehmer Random Number Generator with 2^126 period
// https://en.wikipedia.org/Lehmer_random_number_generator
typedef struct randomseed_s
{
	unsigned int s[4];
}
randomseed_t;

void Math_RandomSeed_Reset(randomseed_t *r);
void Math_RandomSeed_FromInts(randomseed_t *r, unsigned int s0, unsigned int s1, unsigned int s2, unsigned int s3);
unsigned long long Math_rand64(randomseed_t *r);
float Math_randomf(randomseed_t *r);
float Math_crandomf(randomseed_t *r);
float Math_randomrangef(randomseed_t *r, float minf, float maxf);
int Math_randomrangei(randomseed_t *r, int mini, int maxi);

void Mathlib_Init(void);

#endif

