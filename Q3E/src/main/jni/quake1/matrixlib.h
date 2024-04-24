
#ifndef MATRIXLIB_H
#define MATRIXLIB_H

#include "qtypes.h"

//#define MATRIX4x4_OPENGLORIENTATION

typedef struct matrix4x4_s
{
	vec_t m[4][4];
}
matrix4x4_t;

extern const matrix4x4_t identitymatrix;

// functions for manipulating 4x4 matrices

// copy a matrix4x4
void Matrix4x4_Copy (matrix4x4_t *out, const matrix4x4_t *in);
// copy only the rotation portion of a matrix4x4
void Matrix4x4_CopyRotateOnly (matrix4x4_t *out, const matrix4x4_t *in);
// copy only the translate portion of a matrix4x4
void Matrix4x4_CopyTranslateOnly (matrix4x4_t *out, const matrix4x4_t *in);
// multiply two matrix4x4 together, combining their transformations
// (warning: order matters - Concat(a, b, c) != Concat(a, c, b))
void Matrix4x4_Concat (matrix4x4_t *out, const matrix4x4_t *in1, const matrix4x4_t *in2);
// swaps the rows and columns of the matrix
// (is this useful for anything?)
void Matrix4x4_Transpose (matrix4x4_t *out, const matrix4x4_t *in1);
// creates a matrix that does the opposite of the matrix provided
// this is a full matrix inverter, it should be able to invert any matrix that
// is possible to invert
// (non-uniform scaling, rotation, shearing, and translation, possibly others)
// warning: this function is SLOW
int Matrix4x4_Invert_Full (matrix4x4_t *out, const matrix4x4_t *in1);
// creates a matrix that does the opposite of the matrix provided
// only supports translate, rotate, scale (not scale3) matrices
void Matrix4x4_Invert_Simple (matrix4x4_t *out, const matrix4x4_t *in1);
// blends between two matrices, used primarily for animation interpolation
// (note: it is recommended to follow this with Matrix4x4_Normalize, a method
//  known as nlerp rotation, often better for animation purposes than slerp)
void Matrix4x4_Interpolate (matrix4x4_t *out, matrix4x4_t *in1, matrix4x4_t *in2, double frac);
// zeros all matrix components, used with Matrix4x4_Accumulate
void Matrix4x4_Clear (matrix4x4_t *out);
// adds a weighted contribution from the supplied matrix, used to blend 3 or
// more matrices with weighting, it is recommended that Matrix4x4_Normalize be
// called afterward (a method known as nlerp rotation, often better for
// animation purposes than slerp)
void Matrix4x4_Accumulate (matrix4x4_t *out, matrix4x4_t *in, double weight);
// creates a matrix that does the same rotation and translation as the matrix
// provided, but no uniform scaling, does not support scale3 matrices
void Matrix4x4_Normalize (matrix4x4_t *out, matrix4x4_t *in1);
// creates a matrix with vectors normalized individually (use after
// Matrix4x4_Accumulate)
void Matrix4x4_Normalize3 (matrix4x4_t *out, matrix4x4_t *in1);
// modifies a matrix to have all vectors and origin reflected across the plane
// to the opposite side (at least if axisscale is -2)
void Matrix4x4_Reflect (matrix4x4_t *out, double normalx, double normaly, double normalz, double dist, double axisscale);

// creates an identity matrix
// (a matrix which does nothing)
void Matrix4x4_CreateIdentity (matrix4x4_t *out);
// creates a translate matrix
// (moves vectors)
void Matrix4x4_CreateTranslate (matrix4x4_t *out, double x, double y, double z);
// creates a rotate matrix
// (rotates vectors)
void Matrix4x4_CreateRotate (matrix4x4_t *out, double angle, double x, double y, double z);
// creates a scaling matrix
// (expands or contracts vectors)
// (warning: do not apply this kind of matrix to direction vectors)
void Matrix4x4_CreateScale (matrix4x4_t *out, double x);
// creates a squishing matrix
// (expands or contracts vectors differently in different axis)
// (warning: this is not reversed by Invert_Simple)
// (warning: do not apply this kind of matrix to direction vectors)
void Matrix4x4_CreateScale3 (matrix4x4_t *out, double x, double y, double z);
// creates a matrix for a quake entity
void Matrix4x4_CreateFromQuakeEntity(matrix4x4_t *out, double x, double y, double z, double pitch, double yaw, double roll, double scale);
// creates a duke3d view matrix for a quake view matrix ;)
void Matrix4x4_QuakeToDuke3D(const matrix4x4_t *in, matrix4x4_t *out, double maxShearAngle);

// converts a matrix4x4 to a set of 3D vectors for the 3 axial directions, and the translate
void Matrix4x4_ToVectors(const matrix4x4_t *in, vec_t vx[3], vec_t vy[3], vec_t vz[3], vec_t t[3]);
// creates a matrix4x4 from a set of 3D vectors for axial directions, and translate
void Matrix4x4_FromVectors(matrix4x4_t *out, const vec_t vx[3], const vec_t vy[3], const vec_t vz[3], const vec_t t[3]);

// converts a matrix4x4 to a double[16] array in the OpenGL orientation
void Matrix4x4_ToArrayDoubleGL(const matrix4x4_t *in, double out[16]);
// creates a matrix4x4 from a double[16] array in the OpenGL orientation
void Matrix4x4_FromArrayDoubleGL(matrix4x4_t *out, const double in[16]);
// converts a matrix4x4 to a double[16] array in the Direct3D orientation
void Matrix4x4_ToArrayDoubleD3D(const matrix4x4_t *in, double out[16]);
// creates a matrix4x4 from a double[16] array in the Direct3D orientation
void Matrix4x4_FromArrayDoubleD3D(matrix4x4_t *out, const double in[4][4]);

// converts a matrix4x4 to a float[16] array in the OpenGL orientation
void Matrix4x4_ToArrayFloatGL(const matrix4x4_t *in, float out[16]);
// creates a matrix4x4 from a float[16] array in the OpenGL orientation
void Matrix4x4_FromArrayFloatGL(matrix4x4_t *out, const float in[16]);
// converts a matrix4x4 to a float[16] array in the Direct3D orientation
void Matrix4x4_ToArrayFloatD3D(const matrix4x4_t *in, float out[16]);
// creates a matrix4x4 from a float[16] array in the Direct3D orientation
void Matrix4x4_FromArrayFloatD3D(matrix4x4_t *out, const float in[16]);

// converts a matrix4x4 to a float[12] array in the OpenGL orientation
void Matrix4x4_ToArray12FloatGL(const matrix4x4_t *in, float out[4][3]);
// creates a matrix4x4 from a float[12] array in the OpenGL orientation
void Matrix4x4_FromArray12FloatGL(matrix4x4_t *out, const float in[12]);
// converts a matrix4x4 to a float[12] array in the Direct3D orientation
void Matrix4x4_ToArray12FloatD3D(const matrix4x4_t *in, float out[12]);
// creates a matrix4x4 from a float[12] array in the Direct3D orientation
void Matrix4x4_FromArray12FloatD3D(matrix4x4_t *out, const float in[12]);

// creates a matrix4x4 from an origin and quaternion (used mostly with skeletal model formats such as PSK)
void Matrix4x4_FromOriginQuat(matrix4x4_t *m, double ox, double oy, double oz, double x, double y, double z, double w);
// creates an origin and quaternion from a matrix4x4_t, quat[3] is always positive
void Matrix4x4_ToOrigin3Quat4Float(const matrix4x4_t *m, float *origin, float *quat);
// creates a matrix4x4 from an origin and canonical unit-length quaternion (used mostly with skeletal model formats such as MD5)
void Matrix4x4_FromDoom3Joint(matrix4x4_t *m, double ox, double oy, double oz, double x, double y, double z);

// creates a matrix4x4_t from an origin and canonical unit-length quaternion in short[7] normalized format
void Matrix4x4_FromBonePose7s(matrix4x4_t *m, float originscale, const short *pose7s);
// creates a short[7] representation from normalized matrix4x4_t
void Matrix4x4_ToBonePose7s(const matrix4x4_t *m, float origininvscale, short *pose7s);

// blends two matrices together, at a given percentage (blend controls percentage of in2)
void Matrix4x4_Blend (matrix4x4_t *out, const matrix4x4_t *in1, const matrix4x4_t *in2, double blend);

// transforms a 3D vector through a matrix4x4
void Matrix4x4_Transform (const matrix4x4_t *in, const vec_t v[3], vec_t out[3]);
// transforms a 4D vector through a matrix4x4
// (warning: if you don't know why you would need this, you don't need it)
// (warning: the 4th component of the vector should be 1.0)
void Matrix4x4_Transform4 (const matrix4x4_t *in, const vec_t v[4], vec_t out[4]);
// reverse transforms a 3D vector through a matrix4x4, at least for *simple*
// cases (rotation and translation *ONLY*), this attempts to undo the results
// of Transform
//void Matrix4x4_SimpleUntransform (const matrix4x4_t *in, const vec_t v[3], vec_t out[3]);
// transforms a direction vector through the rotation part of a matrix
void Matrix4x4_Transform3x3 (const matrix4x4_t *in, const vec_t v[3], vec_t out[3]);
// transforms a positive distance plane (A*x+B*y+C*z-D=0) through a rotation or translation matrix
void Matrix4x4_TransformPositivePlane (const matrix4x4_t *in, vec_t x, vec_t y, vec_t z, vec_t d, vec_t *o);
// transforms a standard plane (A*x+B*y+C*z+D=0) through a rotation or translation matrix
void Matrix4x4_TransformStandardPlane (const matrix4x4_t *in, vec_t x, vec_t y, vec_t z, vec_t d, vec_t *o);

// ease of use functions
// immediately applies a Translate to the matrix
void Matrix4x4_ConcatTranslate (matrix4x4_t *out, double x, double y, double z);
// immediately applies a Rotate to the matrix
void Matrix4x4_ConcatRotate (matrix4x4_t *out, double angle, double x, double y, double z);
// immediately applies a Scale to the matrix
void Matrix4x4_ConcatScale (matrix4x4_t *out, double x);
// immediately applies a Scale3 to the matrix
void Matrix4x4_ConcatScale3 (matrix4x4_t *out, double x, double y, double z);

// extracts origin vector (translate) from matrix
void Matrix4x4_OriginFromMatrix (const matrix4x4_t *in, vec_t *out);
// extracts scaling factor from matrix (only works for uniform scaling)
double Matrix4x4_ScaleFromMatrix (const matrix4x4_t *in);

// replaces origin vector (translate) in matrix
void Matrix4x4_SetOrigin (matrix4x4_t *out, double x, double y, double z);
// moves origin vector (translate) in matrix by a simple translate
void Matrix4x4_AdjustOrigin (matrix4x4_t *out, double x, double y, double z);
// scales vectors of a matrix in place and allows you to scale origin as well
void Matrix4x4_Scale (matrix4x4_t *out, double rotatescale, double originscale);
// ensures each element of the 3x3 rotation matrix is facing in the + direction
void Matrix4x4_Abs (matrix4x4_t *out);

#endif
