#include "darkplaces.h"
#include <math.h>

#ifdef _MSC_VER
#pragma warning(disable : 4244)     // LadyHavoc: MSVC++ 4 x86, double/float
#pragma warning(disable : 4305)         // LadyHavoc: MSVC++ 6 x86, double/float
#endif

const matrix4x4_t identitymatrix =
{
	{
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	}
};

void Matrix4x4_Copy (matrix4x4_t *out, const matrix4x4_t *in)
{
	*out = *in;
}

void Matrix4x4_CopyRotateOnly (matrix4x4_t *out, const matrix4x4_t *in)
{
	out->m[0][0] = in->m[0][0];
	out->m[0][1] = in->m[0][1];
	out->m[0][2] = in->m[0][2];
	out->m[0][3] = 0.0f;
	out->m[1][0] = in->m[1][0];
	out->m[1][1] = in->m[1][1];
	out->m[1][2] = in->m[1][2];
	out->m[1][3] = 0.0f;
	out->m[2][0] = in->m[2][0];
	out->m[2][1] = in->m[2][1];
	out->m[2][2] = in->m[2][2];
	out->m[2][3] = 0.0f;
	out->m[3][0] = 0.0f;
	out->m[3][1] = 0.0f;
	out->m[3][2] = 0.0f;
	out->m[3][3] = 1.0f;
}

void Matrix4x4_CopyTranslateOnly (matrix4x4_t *out, const matrix4x4_t *in)
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0] = 1.0f;
	out->m[1][0] = 0.0f;
	out->m[2][0] = 0.0f;
	out->m[3][0] = in->m[0][3];
	out->m[0][1] = 0.0f;
	out->m[1][1] = 1.0f;
	out->m[2][1] = 0.0f;
	out->m[3][1] = in->m[1][3];
	out->m[0][2] = 0.0f;
	out->m[1][2] = 0.0f;
	out->m[2][2] = 1.0f;
	out->m[3][2] = in->m[2][3];
	out->m[0][3] = 0.0f;
	out->m[1][3] = 0.0f;
	out->m[2][3] = 0.0f;
	out->m[3][3] = 1.0f;
#else
	out->m[0][0] = 1.0f;
	out->m[0][1] = 0.0f;
	out->m[0][2] = 0.0f;
	out->m[0][3] = in->m[0][3];
	out->m[1][0] = 0.0f;
	out->m[1][1] = 1.0f;
	out->m[1][2] = 0.0f;
	out->m[1][3] = in->m[1][3];
	out->m[2][0] = 0.0f;
	out->m[2][1] = 0.0f;
	out->m[2][2] = 1.0f;
	out->m[2][3] = in->m[2][3];
	out->m[3][0] = 0.0f;
	out->m[3][1] = 0.0f;
	out->m[3][2] = 0.0f;
	out->m[3][3] = 1.0f;
#endif
}

void Matrix4x4_Concat (matrix4x4_t *out, const matrix4x4_t *in1, const matrix4x4_t *in2)
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0] = in1->m[0][0] * in2->m[0][0] + in1->m[1][0] * in2->m[0][1] + in1->m[2][0] * in2->m[0][2] + in1->m[3][0] * in2->m[0][3];
	out->m[1][0] = in1->m[0][0] * in2->m[1][0] + in1->m[1][0] * in2->m[1][1] + in1->m[2][0] * in2->m[1][2] + in1->m[3][0] * in2->m[1][3];
	out->m[2][0] = in1->m[0][0] * in2->m[2][0] + in1->m[1][0] * in2->m[2][1] + in1->m[2][0] * in2->m[2][2] + in1->m[3][0] * in2->m[2][3];
	out->m[3][0] = in1->m[0][0] * in2->m[3][0] + in1->m[1][0] * in2->m[3][1] + in1->m[2][0] * in2->m[3][2] + in1->m[3][0] * in2->m[3][3];
	out->m[0][1] = in1->m[0][1] * in2->m[0][0] + in1->m[1][1] * in2->m[0][1] + in1->m[2][1] * in2->m[0][2] + in1->m[3][1] * in2->m[0][3];
	out->m[1][1] = in1->m[0][1] * in2->m[1][0] + in1->m[1][1] * in2->m[1][1] + in1->m[2][1] * in2->m[1][2] + in1->m[3][1] * in2->m[1][3];
	out->m[2][1] = in1->m[0][1] * in2->m[2][0] + in1->m[1][1] * in2->m[2][1] + in1->m[2][1] * in2->m[2][2] + in1->m[3][1] * in2->m[2][3];
	out->m[3][1] = in1->m[0][1] * in2->m[3][0] + in1->m[1][1] * in2->m[3][1] + in1->m[2][1] * in2->m[3][2] + in1->m[3][1] * in2->m[3][3];
	out->m[0][2] = in1->m[0][2] * in2->m[0][0] + in1->m[1][2] * in2->m[0][1] + in1->m[2][2] * in2->m[0][2] + in1->m[3][2] * in2->m[0][3];
	out->m[1][2] = in1->m[0][2] * in2->m[1][0] + in1->m[1][2] * in2->m[1][1] + in1->m[2][2] * in2->m[1][2] + in1->m[3][2] * in2->m[1][3];
	out->m[2][2] = in1->m[0][2] * in2->m[2][0] + in1->m[1][2] * in2->m[2][1] + in1->m[2][2] * in2->m[2][2] + in1->m[3][2] * in2->m[2][3];
	out->m[3][2] = in1->m[0][2] * in2->m[3][0] + in1->m[1][2] * in2->m[3][1] + in1->m[2][2] * in2->m[3][2] + in1->m[3][2] * in2->m[3][3];
	out->m[0][3] = in1->m[0][3] * in2->m[0][0] + in1->m[1][3] * in2->m[0][1] + in1->m[2][3] * in2->m[0][2] + in1->m[3][3] * in2->m[0][3];
	out->m[1][3] = in1->m[0][3] * in2->m[1][0] + in1->m[1][3] * in2->m[1][1] + in1->m[2][3] * in2->m[1][2] + in1->m[3][3] * in2->m[1][3];
	out->m[2][3] = in1->m[0][3] * in2->m[2][0] + in1->m[1][3] * in2->m[2][1] + in1->m[2][3] * in2->m[2][2] + in1->m[3][3] * in2->m[2][3];
	out->m[3][3] = in1->m[0][3] * in2->m[3][0] + in1->m[1][3] * in2->m[3][1] + in1->m[2][3] * in2->m[3][2] + in1->m[3][3] * in2->m[3][3];
#else
	out->m[0][0] = in1->m[0][0] * in2->m[0][0] + in1->m[0][1] * in2->m[1][0] + in1->m[0][2] * in2->m[2][0] + in1->m[0][3] * in2->m[3][0];
	out->m[0][1] = in1->m[0][0] * in2->m[0][1] + in1->m[0][1] * in2->m[1][1] + in1->m[0][2] * in2->m[2][1] + in1->m[0][3] * in2->m[3][1];
	out->m[0][2] = in1->m[0][0] * in2->m[0][2] + in1->m[0][1] * in2->m[1][2] + in1->m[0][2] * in2->m[2][2] + in1->m[0][3] * in2->m[3][2];
	out->m[0][3] = in1->m[0][0] * in2->m[0][3] + in1->m[0][1] * in2->m[1][3] + in1->m[0][2] * in2->m[2][3] + in1->m[0][3] * in2->m[3][3];
	out->m[1][0] = in1->m[1][0] * in2->m[0][0] + in1->m[1][1] * in2->m[1][0] + in1->m[1][2] * in2->m[2][0] + in1->m[1][3] * in2->m[3][0];
	out->m[1][1] = in1->m[1][0] * in2->m[0][1] + in1->m[1][1] * in2->m[1][1] + in1->m[1][2] * in2->m[2][1] + in1->m[1][3] * in2->m[3][1];
	out->m[1][2] = in1->m[1][0] * in2->m[0][2] + in1->m[1][1] * in2->m[1][2] + in1->m[1][2] * in2->m[2][2] + in1->m[1][3] * in2->m[3][2];
	out->m[1][3] = in1->m[1][0] * in2->m[0][3] + in1->m[1][1] * in2->m[1][3] + in1->m[1][2] * in2->m[2][3] + in1->m[1][3] * in2->m[3][3];
	out->m[2][0] = in1->m[2][0] * in2->m[0][0] + in1->m[2][1] * in2->m[1][0] + in1->m[2][2] * in2->m[2][0] + in1->m[2][3] * in2->m[3][0];
	out->m[2][1] = in1->m[2][0] * in2->m[0][1] + in1->m[2][1] * in2->m[1][1] + in1->m[2][2] * in2->m[2][1] + in1->m[2][3] * in2->m[3][1];
	out->m[2][2] = in1->m[2][0] * in2->m[0][2] + in1->m[2][1] * in2->m[1][2] + in1->m[2][2] * in2->m[2][2] + in1->m[2][3] * in2->m[3][2];
	out->m[2][3] = in1->m[2][0] * in2->m[0][3] + in1->m[2][1] * in2->m[1][3] + in1->m[2][2] * in2->m[2][3] + in1->m[2][3] * in2->m[3][3];
	out->m[3][0] = in1->m[3][0] * in2->m[0][0] + in1->m[3][1] * in2->m[1][0] + in1->m[3][2] * in2->m[2][0] + in1->m[3][3] * in2->m[3][0];
	out->m[3][1] = in1->m[3][0] * in2->m[0][1] + in1->m[3][1] * in2->m[1][1] + in1->m[3][2] * in2->m[2][1] + in1->m[3][3] * in2->m[3][1];
	out->m[3][2] = in1->m[3][0] * in2->m[0][2] + in1->m[3][1] * in2->m[1][2] + in1->m[3][2] * in2->m[2][2] + in1->m[3][3] * in2->m[3][2];
	out->m[3][3] = in1->m[3][0] * in2->m[0][3] + in1->m[3][1] * in2->m[1][3] + in1->m[3][2] * in2->m[2][3] + in1->m[3][3] * in2->m[3][3];
#endif
}

void Matrix4x4_Transpose (matrix4x4_t *out, const matrix4x4_t *in1)
{
	out->m[0][0] = in1->m[0][0];
	out->m[0][1] = in1->m[1][0];
	out->m[0][2] = in1->m[2][0];
	out->m[0][3] = in1->m[3][0];
	out->m[1][0] = in1->m[0][1];
	out->m[1][1] = in1->m[1][1];
	out->m[1][2] = in1->m[2][1];
	out->m[1][3] = in1->m[3][1];
	out->m[2][0] = in1->m[0][2];
	out->m[2][1] = in1->m[1][2];
	out->m[2][2] = in1->m[2][2];
	out->m[2][3] = in1->m[3][2];
	out->m[3][0] = in1->m[0][3];
	out->m[3][1] = in1->m[1][3];
	out->m[3][2] = in1->m[2][3];
	out->m[3][3] = in1->m[3][3];
}

#if 1
// Adapted from code contributed to Mesa by David Moore (Mesa 7.6 under SGI Free License B - which is MIT/X11-type)
// added helper for common subexpression elimination by eihrul, and other optimizations by div0
int Matrix4x4_Invert_Full (matrix4x4_t *out, const matrix4x4_t *in1)
{
		float det;

		// note: orientation does not matter, as transpose(invert(transpose(m))) == invert(m), proof:
		//   transpose(invert(transpose(m))) * m
		// = transpose(invert(transpose(m))) * transpose(transpose(m))
		// = transpose(transpose(m) * invert(transpose(m)))
		// = transpose(identity)
		// = identity

		// this seems to help gcc's common subexpression elimination, and also makes the code look nicer
		float   m00 = in1->m[0][0], m01 = in1->m[0][1], m02 = in1->m[0][2], m03 = in1->m[0][3],
				m10 = in1->m[1][0], m11 = in1->m[1][1], m12 = in1->m[1][2], m13 = in1->m[1][3],
				m20 = in1->m[2][0], m21 = in1->m[2][1], m22 = in1->m[2][2], m23 = in1->m[2][3],
				m30 = in1->m[3][0], m31 = in1->m[3][1], m32 = in1->m[3][2], m33 = in1->m[3][3];

		// calculate the adjoint
		out->m[0][0] =  (m11*(m22*m33 - m23*m32) - m21*(m12*m33 - m13*m32) + m31*(m12*m23 - m13*m22));
		out->m[0][1] = -(m01*(m22*m33 - m23*m32) - m21*(m02*m33 - m03*m32) + m31*(m02*m23 - m03*m22));
		out->m[0][2] =  (m01*(m12*m33 - m13*m32) - m11*(m02*m33 - m03*m32) + m31*(m02*m13 - m03*m12));
		out->m[0][3] = -(m01*(m12*m23 - m13*m22) - m11*(m02*m23 - m03*m22) + m21*(m02*m13 - m03*m12));
		out->m[1][0] = -(m10*(m22*m33 - m23*m32) - m20*(m12*m33 - m13*m32) + m30*(m12*m23 - m13*m22));
		out->m[1][1] =  (m00*(m22*m33 - m23*m32) - m20*(m02*m33 - m03*m32) + m30*(m02*m23 - m03*m22));
		out->m[1][2] = -(m00*(m12*m33 - m13*m32) - m10*(m02*m33 - m03*m32) + m30*(m02*m13 - m03*m12));
		out->m[1][3] =  (m00*(m12*m23 - m13*m22) - m10*(m02*m23 - m03*m22) + m20*(m02*m13 - m03*m12));
		out->m[2][0] =  (m10*(m21*m33 - m23*m31) - m20*(m11*m33 - m13*m31) + m30*(m11*m23 - m13*m21));
		out->m[2][1] = -(m00*(m21*m33 - m23*m31) - m20*(m01*m33 - m03*m31) + m30*(m01*m23 - m03*m21));
		out->m[2][2] =  (m00*(m11*m33 - m13*m31) - m10*(m01*m33 - m03*m31) + m30*(m01*m13 - m03*m11));
		out->m[2][3] = -(m00*(m11*m23 - m13*m21) - m10*(m01*m23 - m03*m21) + m20*(m01*m13 - m03*m11));
		out->m[3][0] = -(m10*(m21*m32 - m22*m31) - m20*(m11*m32 - m12*m31) + m30*(m11*m22 - m12*m21));
		out->m[3][1] =  (m00*(m21*m32 - m22*m31) - m20*(m01*m32 - m02*m31) + m30*(m01*m22 - m02*m21));
		out->m[3][2] = -(m00*(m11*m32 - m12*m31) - m10*(m01*m32 - m02*m31) + m30*(m01*m12 - m02*m11));
		out->m[3][3] =  (m00*(m11*m22 - m12*m21) - m10*(m01*m22 - m02*m21) + m20*(m01*m12 - m02*m11));

		// calculate the determinant (as inverse == 1/det * adjoint, adjoint * m == identity * det, so this calculates the det)
		det = m00*out->m[0][0] + m10*out->m[0][1] + m20*out->m[0][2] + m30*out->m[0][3];
		if (det == 0.0f)
				return 0;

		// multiplications are faster than divisions, usually
		det = 1.0f / det;

		// manually unrolled loop to multiply all matrix elements by 1/det
		out->m[0][0] *= det; out->m[0][1] *= det; out->m[0][2] *= det; out->m[0][3] *= det;
		out->m[1][0] *= det; out->m[1][1] *= det; out->m[1][2] *= det; out->m[1][3] *= det;
		out->m[2][0] *= det; out->m[2][1] *= det; out->m[2][2] *= det; out->m[2][3] *= det;
		out->m[3][0] *= det; out->m[3][1] *= det; out->m[3][2] *= det; out->m[3][3] *= det;

		return 1;
}
#elif 1
// Adapted from code contributed to Mesa by David Moore (Mesa 7.6 under SGI Free License B - which is MIT/X11-type)
int Matrix4x4_Invert_Full (matrix4x4_t *out, const matrix4x4_t *in1)
{
	matrix4x4_t temp;
	double det;
	int i, j;

#ifdef MATRIX4x4_OPENGLORIENTATION
	temp.m[0][0] =  in1->m[1][1]*in1->m[2][2]*in1->m[3][3] - in1->m[1][1]*in1->m[2][3]*in1->m[3][2] - in1->m[2][1]*in1->m[1][2]*in1->m[3][3] + in1->m[2][1]*in1->m[1][3]*in1->m[3][2] + in1->m[3][1]*in1->m[1][2]*in1->m[2][3] - in1->m[3][1]*in1->m[1][3]*in1->m[2][2];
	temp.m[1][0] = -in1->m[1][0]*in1->m[2][2]*in1->m[3][3] + in1->m[1][0]*in1->m[2][3]*in1->m[3][2] + in1->m[2][0]*in1->m[1][2]*in1->m[3][3] - in1->m[2][0]*in1->m[1][3]*in1->m[3][2] - in1->m[3][0]*in1->m[1][2]*in1->m[2][3] + in1->m[3][0]*in1->m[1][3]*in1->m[2][2];
	temp.m[2][0] =  in1->m[1][0]*in1->m[2][1]*in1->m[3][3] - in1->m[1][0]*in1->m[2][3]*in1->m[3][1] - in1->m[2][0]*in1->m[1][1]*in1->m[3][3] + in1->m[2][0]*in1->m[1][3]*in1->m[3][1] + in1->m[3][0]*in1->m[1][1]*in1->m[2][3] - in1->m[3][0]*in1->m[1][3]*in1->m[2][1];
	temp.m[3][0] = -in1->m[1][0]*in1->m[2][1]*in1->m[3][2] + in1->m[1][0]*in1->m[2][2]*in1->m[3][1] + in1->m[2][0]*in1->m[1][1]*in1->m[3][2] - in1->m[2][0]*in1->m[1][2]*in1->m[3][1] - in1->m[3][0]*in1->m[1][1]*in1->m[2][2] + in1->m[3][0]*in1->m[1][2]*in1->m[2][1];
	temp.m[0][1] = -in1->m[0][1]*in1->m[2][2]*in1->m[3][3] + in1->m[0][1]*in1->m[2][3]*in1->m[3][2] + in1->m[2][1]*in1->m[0][2]*in1->m[3][3] - in1->m[2][1]*in1->m[0][3]*in1->m[3][2] - in1->m[3][1]*in1->m[0][2]*in1->m[2][3] + in1->m[3][1]*in1->m[0][3]*in1->m[2][2];
	temp.m[1][1] =  in1->m[0][0]*in1->m[2][2]*in1->m[3][3] - in1->m[0][0]*in1->m[2][3]*in1->m[3][2] - in1->m[2][0]*in1->m[0][2]*in1->m[3][3] + in1->m[2][0]*in1->m[0][3]*in1->m[3][2] + in1->m[3][0]*in1->m[0][2]*in1->m[2][3] - in1->m[3][0]*in1->m[0][3]*in1->m[2][2];
	temp.m[2][1] = -in1->m[0][0]*in1->m[2][1]*in1->m[3][3] + in1->m[0][0]*in1->m[2][3]*in1->m[3][1] + in1->m[2][0]*in1->m[0][1]*in1->m[3][3] - in1->m[2][0]*in1->m[0][3]*in1->m[3][1] - in1->m[3][0]*in1->m[0][1]*in1->m[2][3] + in1->m[3][0]*in1->m[0][3]*in1->m[2][1];
	temp.m[3][1] =  in1->m[0][0]*in1->m[2][1]*in1->m[3][2] - in1->m[0][0]*in1->m[2][2]*in1->m[3][1] - in1->m[2][0]*in1->m[0][1]*in1->m[3][2] + in1->m[2][0]*in1->m[0][2]*in1->m[3][1] + in1->m[3][0]*in1->m[0][1]*in1->m[2][2] - in1->m[3][0]*in1->m[0][2]*in1->m[2][1];
	temp.m[0][2] =  in1->m[0][1]*in1->m[1][2]*in1->m[3][3] - in1->m[0][1]*in1->m[1][3]*in1->m[3][2] - in1->m[1][1]*in1->m[0][2]*in1->m[3][3] + in1->m[1][1]*in1->m[0][3]*in1->m[3][2] + in1->m[3][1]*in1->m[0][2]*in1->m[1][3] - in1->m[3][1]*in1->m[0][3]*in1->m[1][2];
	temp.m[1][2] = -in1->m[0][0]*in1->m[1][2]*in1->m[3][3] + in1->m[0][0]*in1->m[1][3]*in1->m[3][2] + in1->m[1][0]*in1->m[0][2]*in1->m[3][3] - in1->m[1][0]*in1->m[0][3]*in1->m[3][2] - in1->m[3][0]*in1->m[0][2]*in1->m[1][3] + in1->m[3][0]*in1->m[0][3]*in1->m[1][2];
	temp.m[2][2] =  in1->m[0][0]*in1->m[1][1]*in1->m[3][3] - in1->m[0][0]*in1->m[1][3]*in1->m[3][1] - in1->m[1][0]*in1->m[0][1]*in1->m[3][3] + in1->m[1][0]*in1->m[0][3]*in1->m[3][1] + in1->m[3][0]*in1->m[0][1]*in1->m[1][3] - in1->m[3][0]*in1->m[0][3]*in1->m[1][1];
	temp.m[3][2] = -in1->m[0][0]*in1->m[1][1]*in1->m[3][2] + in1->m[0][0]*in1->m[1][2]*in1->m[3][1] + in1->m[1][0]*in1->m[0][1]*in1->m[3][2] - in1->m[1][0]*in1->m[0][2]*in1->m[3][1] - in1->m[3][0]*in1->m[0][1]*in1->m[1][2] + in1->m[3][0]*in1->m[0][2]*in1->m[1][1];
	temp.m[0][3] = -in1->m[0][1]*in1->m[1][2]*in1->m[2][3] + in1->m[0][1]*in1->m[1][3]*in1->m[2][2] + in1->m[1][1]*in1->m[0][2]*in1->m[2][3] - in1->m[1][1]*in1->m[0][3]*in1->m[2][2] - in1->m[2][1]*in1->m[0][2]*in1->m[1][3] + in1->m[2][1]*in1->m[0][3]*in1->m[1][2];
	temp.m[1][3] =  in1->m[0][0]*in1->m[1][2]*in1->m[2][3] - in1->m[0][0]*in1->m[1][3]*in1->m[2][2] - in1->m[1][0]*in1->m[0][2]*in1->m[2][3] + in1->m[1][0]*in1->m[0][3]*in1->m[2][2] + in1->m[2][0]*in1->m[0][2]*in1->m[1][3] - in1->m[2][0]*in1->m[0][3]*in1->m[1][2];
	temp.m[2][3] = -in1->m[0][0]*in1->m[1][1]*in1->m[2][3] + in1->m[0][0]*in1->m[1][3]*in1->m[2][1] + in1->m[1][0]*in1->m[0][1]*in1->m[2][3] - in1->m[1][0]*in1->m[0][3]*in1->m[2][1] - in1->m[2][0]*in1->m[0][1]*in1->m[1][3] + in1->m[2][0]*in1->m[0][3]*in1->m[1][1];
	temp.m[3][3] =  in1->m[0][0]*in1->m[1][1]*in1->m[2][2] - in1->m[0][0]*in1->m[1][2]*in1->m[2][1] - in1->m[1][0]*in1->m[0][1]*in1->m[2][2] + in1->m[1][0]*in1->m[0][2]*in1->m[2][1] + in1->m[2][0]*in1->m[0][1]*in1->m[1][2] - in1->m[2][0]*in1->m[0][2]*in1->m[1][1];
#else
	temp.m[0][0] =  in1->m[1][1]*in1->m[2][2]*in1->m[3][3] - in1->m[1][1]*in1->m[3][2]*in1->m[2][3] - in1->m[1][2]*in1->m[2][1]*in1->m[3][3] + in1->m[1][2]*in1->m[3][1]*in1->m[2][3] + in1->m[1][3]*in1->m[2][1]*in1->m[3][2] - in1->m[1][3]*in1->m[3][1]*in1->m[2][2];
	temp.m[0][1] = -in1->m[0][1]*in1->m[2][2]*in1->m[3][3] + in1->m[0][1]*in1->m[3][2]*in1->m[2][3] + in1->m[0][2]*in1->m[2][1]*in1->m[3][3] - in1->m[0][2]*in1->m[3][1]*in1->m[2][3] - in1->m[0][3]*in1->m[2][1]*in1->m[3][2] + in1->m[0][3]*in1->m[3][1]*in1->m[2][2];
	temp.m[0][2] =  in1->m[0][1]*in1->m[1][2]*in1->m[3][3] - in1->m[0][1]*in1->m[3][2]*in1->m[1][3] - in1->m[0][2]*in1->m[1][1]*in1->m[3][3] + in1->m[0][2]*in1->m[3][1]*in1->m[1][3] + in1->m[0][3]*in1->m[1][1]*in1->m[3][2] - in1->m[0][3]*in1->m[3][1]*in1->m[1][2];
	temp.m[0][3] = -in1->m[0][1]*in1->m[1][2]*in1->m[2][3] + in1->m[0][1]*in1->m[2][2]*in1->m[1][3] + in1->m[0][2]*in1->m[1][1]*in1->m[2][3] - in1->m[0][2]*in1->m[2][1]*in1->m[1][3] - in1->m[0][3]*in1->m[1][1]*in1->m[2][2] + in1->m[0][3]*in1->m[2][1]*in1->m[1][2];
	temp.m[1][0] = -in1->m[1][0]*in1->m[2][2]*in1->m[3][3] + in1->m[1][0]*in1->m[3][2]*in1->m[2][3] + in1->m[1][2]*in1->m[2][0]*in1->m[3][3] - in1->m[1][2]*in1->m[3][0]*in1->m[2][3] - in1->m[1][3]*in1->m[2][0]*in1->m[3][2] + in1->m[1][3]*in1->m[3][0]*in1->m[2][2];
	temp.m[1][1] =  in1->m[0][0]*in1->m[2][2]*in1->m[3][3] - in1->m[0][0]*in1->m[3][2]*in1->m[2][3] - in1->m[0][2]*in1->m[2][0]*in1->m[3][3] + in1->m[0][2]*in1->m[3][0]*in1->m[2][3] + in1->m[0][3]*in1->m[2][0]*in1->m[3][2] - in1->m[0][3]*in1->m[3][0]*in1->m[2][2];
	temp.m[1][2] = -in1->m[0][0]*in1->m[1][2]*in1->m[3][3] + in1->m[0][0]*in1->m[3][2]*in1->m[1][3] + in1->m[0][2]*in1->m[1][0]*in1->m[3][3] - in1->m[0][2]*in1->m[3][0]*in1->m[1][3] - in1->m[0][3]*in1->m[1][0]*in1->m[3][2] + in1->m[0][3]*in1->m[3][0]*in1->m[1][2];
	temp.m[1][3] =  in1->m[0][0]*in1->m[1][2]*in1->m[2][3] - in1->m[0][0]*in1->m[2][2]*in1->m[1][3] - in1->m[0][2]*in1->m[1][0]*in1->m[2][3] + in1->m[0][2]*in1->m[2][0]*in1->m[1][3] + in1->m[0][3]*in1->m[1][0]*in1->m[2][2] - in1->m[0][3]*in1->m[2][0]*in1->m[1][2];
	temp.m[2][0] =  in1->m[1][0]*in1->m[2][1]*in1->m[3][3] - in1->m[1][0]*in1->m[3][1]*in1->m[2][3] - in1->m[1][1]*in1->m[2][0]*in1->m[3][3] + in1->m[1][1]*in1->m[3][0]*in1->m[2][3] + in1->m[1][3]*in1->m[2][0]*in1->m[3][1] - in1->m[1][3]*in1->m[3][0]*in1->m[2][1];
	temp.m[2][1] = -in1->m[0][0]*in1->m[2][1]*in1->m[3][3] + in1->m[0][0]*in1->m[3][1]*in1->m[2][3] + in1->m[0][1]*in1->m[2][0]*in1->m[3][3] - in1->m[0][1]*in1->m[3][0]*in1->m[2][3] - in1->m[0][3]*in1->m[2][0]*in1->m[3][1] + in1->m[0][3]*in1->m[3][0]*in1->m[2][1];
	temp.m[2][2] =  in1->m[0][0]*in1->m[1][1]*in1->m[3][3] - in1->m[0][0]*in1->m[3][1]*in1->m[1][3] - in1->m[0][1]*in1->m[1][0]*in1->m[3][3] + in1->m[0][1]*in1->m[3][0]*in1->m[1][3] + in1->m[0][3]*in1->m[1][0]*in1->m[3][1] - in1->m[0][3]*in1->m[3][0]*in1->m[1][1];
	temp.m[2][3] = -in1->m[0][0]*in1->m[1][1]*in1->m[2][3] + in1->m[0][0]*in1->m[2][1]*in1->m[1][3] + in1->m[0][1]*in1->m[1][0]*in1->m[2][3] - in1->m[0][1]*in1->m[2][0]*in1->m[1][3] - in1->m[0][3]*in1->m[1][0]*in1->m[2][1] + in1->m[0][3]*in1->m[2][0]*in1->m[1][1];
	temp.m[3][0] = -in1->m[1][0]*in1->m[2][1]*in1->m[3][2] + in1->m[1][0]*in1->m[3][1]*in1->m[2][2] + in1->m[1][1]*in1->m[2][0]*in1->m[3][2] - in1->m[1][1]*in1->m[3][0]*in1->m[2][2] - in1->m[1][2]*in1->m[2][0]*in1->m[3][1] + in1->m[1][2]*in1->m[3][0]*in1->m[2][1];
	temp.m[3][1] =  in1->m[0][0]*in1->m[2][1]*in1->m[3][2] - in1->m[0][0]*in1->m[3][1]*in1->m[2][2] - in1->m[0][1]*in1->m[2][0]*in1->m[3][2] + in1->m[0][1]*in1->m[3][0]*in1->m[2][2] + in1->m[0][2]*in1->m[2][0]*in1->m[3][1] - in1->m[0][2]*in1->m[3][0]*in1->m[2][1];
	temp.m[3][2] = -in1->m[0][0]*in1->m[1][1]*in1->m[3][2] + in1->m[0][0]*in1->m[3][1]*in1->m[1][2] + in1->m[0][1]*in1->m[1][0]*in1->m[3][2] - in1->m[0][1]*in1->m[3][0]*in1->m[1][2] - in1->m[0][2]*in1->m[1][0]*in1->m[3][1] + in1->m[0][2]*in1->m[3][0]*in1->m[1][1];
	temp.m[3][3] =  in1->m[0][0]*in1->m[1][1]*in1->m[2][2] - in1->m[0][0]*in1->m[2][1]*in1->m[1][2] - in1->m[0][1]*in1->m[1][0]*in1->m[2][2] + in1->m[0][1]*in1->m[2][0]*in1->m[1][2] + in1->m[0][2]*in1->m[1][0]*in1->m[2][1] - in1->m[0][2]*in1->m[2][0]*in1->m[1][1];
#endif

	det = in1->m[0][0]*temp.m[0][0] + in1->m[1][0]*temp.m[0][1] + in1->m[2][0]*temp.m[0][2] + in1->m[3][0]*temp.m[0][3];
	if (det == 0.0f)
		return 0;

	det = 1.0f / det;

	for (i = 0;i < 4;i++)
		for (j = 0;j < 4;j++)
			out->m[i][j] = temp.m[i][j] * det;

	return 1;
}
#else
int Matrix4x4_Invert_Full (matrix4x4_t *out, const matrix4x4_t *in1)
{
	double	*temp;
	double	*r[4];
	double	rtemp[4][8];
	double	m[4];
	double	s;

	r[0]	= rtemp[0];
	r[1]	= rtemp[1];
	r[2]	= rtemp[2];
	r[3]	= rtemp[3];

#ifdef MATRIX4x4_OPENGLORIENTATION
	r[0][0]	= in1->m[0][0];	r[0][1]	= in1->m[1][0];	r[0][2]	= in1->m[2][0];	r[0][3]	= in1->m[3][0];
	r[0][4]	= 1.0;			r[0][5]	=				r[0][6]	=				r[0][7]	= 0.0;

	r[1][0]	= in1->m[0][1];	r[1][1]	= in1->m[1][1];	r[1][2]	= in1->m[2][1];	r[1][3]	= in1->m[3][1];
	r[1][5]	= 1.0;			r[1][4]	=				r[1][6]	=				r[1][7]	= 0.0;

	r[2][0]	= in1->m[0][2];	r[2][1]	= in1->m[1][2];	r[2][2]	= in1->m[2][2];	r[2][3]	= in1->m[3][2];
	r[2][6]	= 1.0;			r[2][4]	=				r[2][5]	=				r[2][7]	= 0.0;

	r[3][0]	= in1->m[0][3];	r[3][1]	= in1->m[1][3];	r[3][2]	= in1->m[2][3];	r[3][3]	= in1->m[3][3];
	r[3][7]	= 1.0;			r[3][4]	=				r[3][5]	=				r[3][6]	= 0.0;
#else
	r[0][0]	= in1->m[0][0];	r[0][1]	= in1->m[0][1];	r[0][2]	= in1->m[0][2];	r[0][3]	= in1->m[0][3];
	r[0][4]	= 1.0;			r[0][5]	=				r[0][6]	=				r[0][7]	= 0.0;

	r[1][0]	= in1->m[1][0];	r[1][1]	= in1->m[1][1];	r[1][2]	= in1->m[1][2];	r[1][3]	= in1->m[1][3];
	r[1][5]	= 1.0;			r[1][4]	=				r[1][6]	=				r[1][7]	= 0.0;

	r[2][0]	= in1->m[2][0];	r[2][1]	= in1->m[2][1];	r[2][2]	= in1->m[2][2];	r[2][3]	= in1->m[2][3];
	r[2][6]	= 1.0;			r[2][4]	=				r[2][5]	=				r[2][7]	= 0.0;

	r[3][0]	= in1->m[3][0];	r[3][1]	= in1->m[3][1];	r[3][2]	= in1->m[3][2];	r[3][3]	= in1->m[3][3];
	r[3][7]	= 1.0;			r[3][4]	=				r[3][5]	=				r[3][6]	= 0.0;
#endif

	if (fabs (r[3][0]) > fabs (r[2][0])) { temp = r[3]; r[3] = r[2]; r[2] = temp; }
	if (fabs (r[2][0]) > fabs (r[1][0])) { temp = r[2]; r[2] = r[1]; r[1] = temp; }
	if (fabs (r[1][0]) > fabs (r[0][0])) { temp = r[1]; r[1] = r[0]; r[0] = temp; }

	if (r[0][0])
	{
		m[1]	= r[1][0] / r[0][0];
		m[2]	= r[2][0] / r[0][0];
		m[3]	= r[3][0] / r[0][0];

		s	= r[0][1]; r[1][1] -= m[1] * s; r[2][1] -= m[2] * s; r[3][1] -= m[3] * s;
		s	= r[0][2]; r[1][2] -= m[1] * s; r[2][2] -= m[2] * s; r[3][2] -= m[3] * s;
		s	= r[0][3]; r[1][3] -= m[1] * s; r[2][3] -= m[2] * s; r[3][3] -= m[3] * s;

		s	= r[0][4]; if (s) { r[1][4] -= m[1] * s; r[2][4] -= m[2] * s; r[3][4] -= m[3] * s; }
		s	= r[0][5]; if (s) { r[1][5] -= m[1] * s; r[2][5] -= m[2] * s; r[3][5] -= m[3] * s; }
		s	= r[0][6]; if (s) { r[1][6] -= m[1] * s; r[2][6] -= m[2] * s; r[3][6] -= m[3] * s; }
		s	= r[0][7]; if (s) { r[1][7] -= m[1] * s; r[2][7] -= m[2] * s; r[3][7] -= m[3] * s; }

		if (fabs (r[3][1]) > fabs (r[2][1])) { temp = r[3]; r[3] = r[2]; r[2] = temp; }
		if (fabs (r[2][1]) > fabs (r[1][1])) { temp = r[2]; r[2] = r[1]; r[1] = temp; }

		if (r[1][1])
		{
			m[2]		= r[2][1] / r[1][1];
			m[3]		= r[3][1] / r[1][1];
			r[2][2]	-= m[2] * r[1][2];
			r[3][2]	-= m[3] * r[1][2];
			r[2][3]	-= m[2] * r[1][3];
			r[3][3]	-= m[3] * r[1][3];

			s	= r[1][4]; if (s) { r[2][4] -= m[2] * s; r[3][4] -= m[3] * s; }
			s	= r[1][5]; if (s) { r[2][5] -= m[2] * s; r[3][5] -= m[3] * s; }
			s	= r[1][6]; if (s) { r[2][6] -= m[2] * s; r[3][6] -= m[3] * s; }
			s	= r[1][7]; if (s) { r[2][7] -= m[2] * s; r[3][7] -= m[3] * s; }

			if (fabs (r[3][2]) > fabs (r[2][2])) { temp = r[3]; r[3] = r[2]; r[2] = temp; }

			if (r[2][2])
			{
				m[3]		= r[3][2] / r[2][2];
				r[3][3]	-= m[3] * r[2][3];
				r[3][4]	-= m[3] * r[2][4];
				r[3][5]	-= m[3] * r[2][5];
				r[3][6]	-= m[3] * r[2][6];
				r[3][7]	-= m[3] * r[2][7];

				if (r[3][3])
				{
					s			= 1.0 / r[3][3];
					r[3][4]	*= s;
					r[3][5]	*= s;
					r[3][6]	*= s;
					r[3][7]	*= s;

					m[2]		= r[2][3];
					s			= 1.0 / r[2][2];
					r[2][4]	= s * (r[2][4] - r[3][4] * m[2]);
					r[2][5]	= s * (r[2][5] - r[3][5] * m[2]);
					r[2][6]	= s * (r[2][6] - r[3][6] * m[2]);
					r[2][7]	= s * (r[2][7] - r[3][7] * m[2]);

					m[1]		= r[1][3];
					r[1][4]	-= r[3][4] * m[1], r[1][5] -= r[3][5] * m[1];
					r[1][6]	-= r[3][6] * m[1], r[1][7] -= r[3][7] * m[1];

					m[0]		= r[0][3];
					r[0][4]	-= r[3][4] * m[0], r[0][5] -= r[3][5] * m[0];
					r[0][6]	-= r[3][6] * m[0], r[0][7] -= r[3][7] * m[0];

					m[1]		= r[1][2];
					s			= 1.0 / r[1][1];
					r[1][4]	= s * (r[1][4] - r[2][4] * m[1]), r[1][5] = s * (r[1][5] - r[2][5] * m[1]);
					r[1][6]	= s * (r[1][6] - r[2][6] * m[1]), r[1][7] = s * (r[1][7] - r[2][7] * m[1]);

					m[0]		= r[0][2];
					r[0][4]	-= r[2][4] * m[0], r[0][5] -= r[2][5] * m[0];
					r[0][6]	-= r[2][6] * m[0], r[0][7] -= r[2][7] * m[0];

					m[0]		= r[0][1];
					s			= 1.0 / r[0][0];
					r[0][4]	= s * (r[0][4] - r[1][4] * m[0]), r[0][5] = s * (r[0][5] - r[1][5] * m[0]);
					r[0][6]	= s * (r[0][6] - r[1][6] * m[0]), r[0][7] = s * (r[0][7] - r[1][7] * m[0]);

#ifdef MATRIX4x4_OPENGLORIENTATION
					out->m[0][0]	= r[0][4];
					out->m[0][1]	= r[1][4];
					out->m[0][2]	= r[2][4];
					out->m[0][3]	= r[3][4];
					out->m[1][0]	= r[0][5];
					out->m[1][1]	= r[1][5];
					out->m[1][2]	= r[2][5];
					out->m[1][3]	= r[3][5];
					out->m[2][0]	= r[0][6];
					out->m[2][1]	= r[1][6];
					out->m[2][2]	= r[2][6];
					out->m[2][3]	= r[3][6];
					out->m[3][0]	= r[0][7];
					out->m[3][1]	= r[1][7];
					out->m[3][2]	= r[2][7];
					out->m[3][3]	= r[3][7];
#else
					out->m[0][0]	= r[0][4];
					out->m[0][1]	= r[0][5];
					out->m[0][2]	= r[0][6];
					out->m[0][3]	= r[0][7];
					out->m[1][0]	= r[1][4];
					out->m[1][1]	= r[1][5];
					out->m[1][2]	= r[1][6];
					out->m[1][3]	= r[1][7];
					out->m[2][0]	= r[2][4];
					out->m[2][1]	= r[2][5];
					out->m[2][2]	= r[2][6];
					out->m[2][3]	= r[2][7];
					out->m[3][0]	= r[3][4];
					out->m[3][1]	= r[3][5];
					out->m[3][2]	= r[3][6];
					out->m[3][3]	= r[3][7];
#endif

					return 1;
				}
			}
		}
	}

	return 0;
}
#endif

void Matrix4x4_Invert_Simple (matrix4x4_t *out, const matrix4x4_t *in1)
{
	// we only support uniform scaling, so assume the first row is enough
	// (note the lack of sqrt here, because we're trying to undo the scaling,
	// this means multiplying by the inverse scale twice - squaring it, which
	// makes the sqrt a waste of time)
#if 1
	double scale = 1.0 / (in1->m[0][0] * in1->m[0][0] + in1->m[0][1] * in1->m[0][1] + in1->m[0][2] * in1->m[0][2]);
#else
	double scale = 3.0 / sqrt
		 (in1->m[0][0] * in1->m[0][0] + in1->m[0][1] * in1->m[0][1] + in1->m[0][2] * in1->m[0][2]
		+ in1->m[1][0] * in1->m[1][0] + in1->m[1][1] * in1->m[1][1] + in1->m[1][2] * in1->m[1][2]
		+ in1->m[2][0] * in1->m[2][0] + in1->m[2][1] * in1->m[2][1] + in1->m[2][2] * in1->m[2][2]);
	scale *= scale;
#endif

	// invert the rotation by transposing and multiplying by the squared
	// recipricol of the input matrix scale as described above
	out->m[0][0] = in1->m[0][0] * scale;
	out->m[0][1] = in1->m[1][0] * scale;
	out->m[0][2] = in1->m[2][0] * scale;
	out->m[1][0] = in1->m[0][1] * scale;
	out->m[1][1] = in1->m[1][1] * scale;
	out->m[1][2] = in1->m[2][1] * scale;
	out->m[2][0] = in1->m[0][2] * scale;
	out->m[2][1] = in1->m[1][2] * scale;
	out->m[2][2] = in1->m[2][2] * scale;

#ifdef MATRIX4x4_OPENGLORIENTATION
	// invert the translate
	out->m[3][0] = -(in1->m[3][0] * out->m[0][0] + in1->m[3][1] * out->m[1][0] + in1->m[3][2] * out->m[2][0]);
	out->m[3][1] = -(in1->m[3][0] * out->m[0][1] + in1->m[3][1] * out->m[1][1] + in1->m[3][2] * out->m[2][1]);
	out->m[3][2] = -(in1->m[3][0] * out->m[0][2] + in1->m[3][1] * out->m[1][2] + in1->m[3][2] * out->m[2][2]);

	// don't know if there's anything worth doing here
	out->m[0][3] = 0;
	out->m[1][3] = 0;
	out->m[2][3] = 0;
	out->m[3][3] = 1;
#else
	// invert the translate
	out->m[0][3] = -(in1->m[0][3] * out->m[0][0] + in1->m[1][3] * out->m[0][1] + in1->m[2][3] * out->m[0][2]);
	out->m[1][3] = -(in1->m[0][3] * out->m[1][0] + in1->m[1][3] * out->m[1][1] + in1->m[2][3] * out->m[1][2]);
	out->m[2][3] = -(in1->m[0][3] * out->m[2][0] + in1->m[1][3] * out->m[2][1] + in1->m[2][3] * out->m[2][2]);

	// don't know if there's anything worth doing here
	out->m[3][0] = 0;
	out->m[3][1] = 0;
	out->m[3][2] = 0;
	out->m[3][3] = 1;
#endif
}

void Matrix4x4_Interpolate (matrix4x4_t *out, matrix4x4_t *in1, matrix4x4_t *in2, double frac)
{
	int i, j;
	for (i = 0;i < 4;i++)
		for (j = 0;j < 4;j++)
			out->m[i][j] = in1->m[i][j] + frac * (in2->m[i][j] - in1->m[i][j]);
}

void Matrix4x4_Clear (matrix4x4_t *out)
{
	int i, j;
	for (i = 0;i < 4;i++)
		for (j = 0;j < 4;j++)
			out->m[i][j] = 0;
}

void Matrix4x4_Accumulate (matrix4x4_t *out, matrix4x4_t *in, double weight)
{
	int i, j;
	for (i = 0;i < 4;i++)
		for (j = 0;j < 4;j++)
			out->m[i][j] += in->m[i][j] * weight;
}

void Matrix4x4_Normalize (matrix4x4_t *out, matrix4x4_t *in1)
{
	// scale rotation matrix vectors to a length of 1
	// note: this is only designed to undo uniform scaling
	double scale = 1.0 / sqrt(in1->m[0][0] * in1->m[0][0] + in1->m[0][1] * in1->m[0][1] + in1->m[0][2] * in1->m[0][2]);
	*out = *in1;
	Matrix4x4_Scale(out, scale, 1);
}

void Matrix4x4_Normalize3 (matrix4x4_t *out, matrix4x4_t *in1)
{
	int i;
	double scale;
	// scale each rotation matrix vector to a length of 1
	// intended for use after Matrix4x4_Interpolate or Matrix4x4_Accumulate
	*out = *in1;
	for (i = 0;i < 3;i++)
	{
#ifdef MATRIX4x4_OPENGLORIENTATION
		scale = sqrt(in1->m[i][0] * in1->m[i][0] + in1->m[i][1] * in1->m[i][1] + in1->m[i][2] * in1->m[i][2]);
		if (scale)
			scale = 1.0 / scale;
		out->m[i][0] *= scale;
		out->m[i][1] *= scale;
		out->m[i][2] *= scale;
#else
		scale = sqrt(in1->m[0][i] * in1->m[0][i] + in1->m[1][i] * in1->m[1][i] + in1->m[2][i] * in1->m[2][i]);
		if (scale)
			scale = 1.0 / scale;
		out->m[0][i] *= scale;
		out->m[1][i] *= scale;
		out->m[2][i] *= scale;
#endif
	}
}

void Matrix4x4_Reflect (matrix4x4_t *out, double normalx, double normaly, double normalz, double dist, double axisscale)
{
	int i;
	double d;
	double p[4], p2[4];
	p[0] = normalx;
	p[1] = normaly;
	p[2] = normalz;
	p[3] = -dist;
	p2[0] = p[0] * axisscale;
	p2[1] = p[1] * axisscale;
	p2[2] = p[2] * axisscale;
	p2[3] = 0;
	for (i = 0;i < 4;i++)
	{
#ifdef MATRIX4x4_OPENGLORIENTATION
		d = out->m[i][0] * p[0] + out->m[i][1] * p[1] + out->m[i][2] * p[2] + out->m[i][3] * p[3];
		out->m[i][0] += p2[0] * d;
		out->m[i][1] += p2[1] * d;
		out->m[i][2] += p2[2] * d;
#else
		d = out->m[0][i] * p[0] + out->m[1][i] * p[1] + out->m[2][i] * p[2] + out->m[3][i] * p[3];
		out->m[0][i] += p2[0] * d;
		out->m[1][i] += p2[1] * d;
		out->m[2][i] += p2[2] * d;
#endif
	}
}

void Matrix4x4_CreateIdentity (matrix4x4_t *out)
{
	out->m[0][0]=1.0f;
	out->m[0][1]=0.0f;
	out->m[0][2]=0.0f;
	out->m[0][3]=0.0f;
	out->m[1][0]=0.0f;
	out->m[1][1]=1.0f;
	out->m[1][2]=0.0f;
	out->m[1][3]=0.0f;
	out->m[2][0]=0.0f;
	out->m[2][1]=0.0f;
	out->m[2][2]=1.0f;
	out->m[2][3]=0.0f;
	out->m[3][0]=0.0f;
	out->m[3][1]=0.0f;
	out->m[3][2]=0.0f;
	out->m[3][3]=1.0f;
}

void Matrix4x4_CreateTranslate (matrix4x4_t *out, double x, double y, double z)
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0]=1.0f;
	out->m[1][0]=0.0f;
	out->m[2][0]=0.0f;
	out->m[3][0]=x;
	out->m[0][1]=0.0f;
	out->m[1][1]=1.0f;
	out->m[2][1]=0.0f;
	out->m[3][1]=y;
	out->m[0][2]=0.0f;
	out->m[1][2]=0.0f;
	out->m[2][2]=1.0f;
	out->m[3][2]=z;
	out->m[0][3]=0.0f;
	out->m[1][3]=0.0f;
	out->m[2][3]=0.0f;
	out->m[3][3]=1.0f;
#else
	out->m[0][0]=1.0f;
	out->m[0][1]=0.0f;
	out->m[0][2]=0.0f;
	out->m[0][3]=x;
	out->m[1][0]=0.0f;
	out->m[1][1]=1.0f;
	out->m[1][2]=0.0f;
	out->m[1][3]=y;
	out->m[2][0]=0.0f;
	out->m[2][1]=0.0f;
	out->m[2][2]=1.0f;
	out->m[2][3]=z;
	out->m[3][0]=0.0f;
	out->m[3][1]=0.0f;
	out->m[3][2]=0.0f;
	out->m[3][3]=1.0f;
#endif
}

void Matrix4x4_CreateRotate (matrix4x4_t *out, double angle, double x, double y, double z)
{
	double len, c, s;

	len = x*x+y*y+z*z;
	if (len != 0.0f)
		len = 1.0f / sqrt(len);
	x *= len;
	y *= len;
	z *= len;

	angle *= (-M_PI / 180.0);
	c = cos(angle);
	s = sin(angle);

#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0]=x * x + c * (1 - x * x);
	out->m[1][0]=x * y * (1 - c) + z * s;
	out->m[2][0]=z * x * (1 - c) - y * s;
	out->m[3][0]=0.0f;
	out->m[0][1]=x * y * (1 - c) - z * s;
	out->m[1][1]=y * y + c * (1 - y * y);
	out->m[2][1]=y * z * (1 - c) + x * s;
	out->m[3][1]=0.0f;
	out->m[0][2]=z * x * (1 - c) + y * s;
	out->m[1][2]=y * z * (1 - c) - x * s;
	out->m[2][2]=z * z + c * (1 - z * z);
	out->m[3][2]=0.0f;
	out->m[0][3]=0.0f;
	out->m[1][3]=0.0f;
	out->m[2][3]=0.0f;
	out->m[3][3]=1.0f;
#else
	out->m[0][0]=x * x + c * (1 - x * x);
	out->m[0][1]=x * y * (1 - c) + z * s;
	out->m[0][2]=z * x * (1 - c) - y * s;
	out->m[0][3]=0.0f;
	out->m[1][0]=x * y * (1 - c) - z * s;
	out->m[1][1]=y * y + c * (1 - y * y);
	out->m[1][2]=y * z * (1 - c) + x * s;
	out->m[1][3]=0.0f;
	out->m[2][0]=z * x * (1 - c) + y * s;
	out->m[2][1]=y * z * (1 - c) - x * s;
	out->m[2][2]=z * z + c * (1 - z * z);
	out->m[2][3]=0.0f;
	out->m[3][0]=0.0f;
	out->m[3][1]=0.0f;
	out->m[3][2]=0.0f;
	out->m[3][3]=1.0f;
#endif
}

void Matrix4x4_CreateScale (matrix4x4_t *out, double x)
{
	out->m[0][0]=x;
	out->m[0][1]=0.0f;
	out->m[0][2]=0.0f;
	out->m[0][3]=0.0f;
	out->m[1][0]=0.0f;
	out->m[1][1]=x;
	out->m[1][2]=0.0f;
	out->m[1][3]=0.0f;
	out->m[2][0]=0.0f;
	out->m[2][1]=0.0f;
	out->m[2][2]=x;
	out->m[2][3]=0.0f;
	out->m[3][0]=0.0f;
	out->m[3][1]=0.0f;
	out->m[3][2]=0.0f;
	out->m[3][3]=1.0f;
}

void Matrix4x4_CreateScale3 (matrix4x4_t *out, double x, double y, double z)
{
	out->m[0][0]=x;
	out->m[0][1]=0.0f;
	out->m[0][2]=0.0f;
	out->m[0][3]=0.0f;
	out->m[1][0]=0.0f;
	out->m[1][1]=y;
	out->m[1][2]=0.0f;
	out->m[1][3]=0.0f;
	out->m[2][0]=0.0f;
	out->m[2][1]=0.0f;
	out->m[2][2]=z;
	out->m[2][3]=0.0f;
	out->m[3][0]=0.0f;
	out->m[3][1]=0.0f;
	out->m[3][2]=0.0f;
	out->m[3][3]=1.0f;
}

void Matrix4x4_CreateFromQuakeEntity(matrix4x4_t *out, double x, double y, double z, double pitch, double yaw, double roll, double scale)
{
	double angle, sr, sp, sy, cr, cp, cy;

	if (roll)
	{
		angle = yaw * (M_PI*2 / 360);
		sy = sin(angle);
		cy = cos(angle);
		angle = pitch * (M_PI*2 / 360);
		sp = sin(angle);
		cp = cos(angle);
		angle = roll * (M_PI*2 / 360);
		sr = sin(angle);
		cr = cos(angle);
#ifdef MATRIX4x4_OPENGLORIENTATION
		out->m[0][0] = (cp*cy) * scale;
		out->m[1][0] = (sr*sp*cy+cr*-sy) * scale;
		out->m[2][0] = (cr*sp*cy+-sr*-sy) * scale;
		out->m[3][0] = x;
		out->m[0][1] = (cp*sy) * scale;
		out->m[1][1] = (sr*sp*sy+cr*cy) * scale;
		out->m[2][1] = (cr*sp*sy+-sr*cy) * scale;
		out->m[3][1] = y;
		out->m[0][2] = (-sp) * scale;
		out->m[1][2] = (sr*cp) * scale;
		out->m[2][2] = (cr*cp) * scale;
		out->m[3][2] = z;
		out->m[0][3] = 0;
		out->m[1][3] = 0;
		out->m[2][3] = 0;
		out->m[3][3] = 1;
#else
		out->m[0][0] = (cp*cy) * scale;
		out->m[0][1] = (sr*sp*cy+cr*-sy) * scale;
		out->m[0][2] = (cr*sp*cy+-sr*-sy) * scale;
		out->m[0][3] = x;
		out->m[1][0] = (cp*sy) * scale;
		out->m[1][1] = (sr*sp*sy+cr*cy) * scale;
		out->m[1][2] = (cr*sp*sy+-sr*cy) * scale;
		out->m[1][3] = y;
		out->m[2][0] = (-sp) * scale;
		out->m[2][1] = (sr*cp) * scale;
		out->m[2][2] = (cr*cp) * scale;
		out->m[2][3] = z;
		out->m[3][0] = 0;
		out->m[3][1] = 0;
		out->m[3][2] = 0;
		out->m[3][3] = 1;
#endif
	}
	else if (pitch)
	{
		angle = yaw * (M_PI*2 / 360);
		sy = sin(angle);
		cy = cos(angle);
		angle = pitch * (M_PI*2 / 360);
		sp = sin(angle);
		cp = cos(angle);
#ifdef MATRIX4x4_OPENGLORIENTATION
		out->m[0][0] = (cp*cy) * scale;
		out->m[1][0] = (-sy) * scale;
		out->m[2][0] = (sp*cy) * scale;
		out->m[3][0] = x;
		out->m[0][1] = (cp*sy) * scale;
		out->m[1][1] = (cy) * scale;
		out->m[2][1] = (sp*sy) * scale;
		out->m[3][1] = y;
		out->m[0][2] = (-sp) * scale;
		out->m[1][2] = 0;
		out->m[2][2] = (cp) * scale;
		out->m[3][2] = z;
		out->m[0][3] = 0;
		out->m[1][3] = 0;
		out->m[2][3] = 0;
		out->m[3][3] = 1;
#else
		out->m[0][0] = (cp*cy) * scale;
		out->m[0][1] = (-sy) * scale;
		out->m[0][2] = (sp*cy) * scale;
		out->m[0][3] = x;
		out->m[1][0] = (cp*sy) * scale;
		out->m[1][1] = (cy) * scale;
		out->m[1][2] = (sp*sy) * scale;
		out->m[1][3] = y;
		out->m[2][0] = (-sp) * scale;
		out->m[2][1] = 0;
		out->m[2][2] = (cp) * scale;
		out->m[2][3] = z;
		out->m[3][0] = 0;
		out->m[3][1] = 0;
		out->m[3][2] = 0;
		out->m[3][3] = 1;
#endif
	}
	else if (yaw)
	{
		angle = yaw * (M_PI*2 / 360);
		sy = sin(angle);
		cy = cos(angle);
#ifdef MATRIX4x4_OPENGLORIENTATION
		out->m[0][0] = (cy) * scale;
		out->m[1][0] = (-sy) * scale;
		out->m[2][0] = 0;
		out->m[3][0] = x;
		out->m[0][1] = (sy) * scale;
		out->m[1][1] = (cy) * scale;
		out->m[2][1] = 0;
		out->m[3][1] = y;
		out->m[0][2] = 0;
		out->m[1][2] = 0;
		out->m[2][2] = scale;
		out->m[3][2] = z;
		out->m[0][3] = 0;
		out->m[1][3] = 0;
		out->m[2][3] = 0;
		out->m[3][3] = 1;
#else
		out->m[0][0] = (cy) * scale;
		out->m[0][1] = (-sy) * scale;
		out->m[0][2] = 0;
		out->m[0][3] = x;
		out->m[1][0] = (sy) * scale;
		out->m[1][1] = (cy) * scale;
		out->m[1][2] = 0;
		out->m[1][3] = y;
		out->m[2][0] = 0;
		out->m[2][1] = 0;
		out->m[2][2] = scale;
		out->m[2][3] = z;
		out->m[3][0] = 0;
		out->m[3][1] = 0;
		out->m[3][2] = 0;
		out->m[3][3] = 1;
#endif
	}
	else
	{
#ifdef MATRIX4x4_OPENGLORIENTATION
		out->m[0][0] = scale;
		out->m[1][0] = 0;
		out->m[2][0] = 0;
		out->m[3][0] = x;
		out->m[0][1] = 0;
		out->m[1][1] = scale;
		out->m[2][1] = 0;
		out->m[3][1] = y;
		out->m[0][2] = 0;
		out->m[1][2] = 0;
		out->m[2][2] = scale;
		out->m[3][2] = z;
		out->m[0][3] = 0;
		out->m[1][3] = 0;
		out->m[2][3] = 0;
		out->m[3][3] = 1;
#else
		out->m[0][0] = scale;
		out->m[0][1] = 0;
		out->m[0][2] = 0;
		out->m[0][3] = x;
		out->m[1][0] = 0;
		out->m[1][1] = scale;
		out->m[1][2] = 0;
		out->m[1][3] = y;
		out->m[2][0] = 0;
		out->m[2][1] = 0;
		out->m[2][2] = scale;
		out->m[2][3] = z;
		out->m[3][0] = 0;
		out->m[3][1] = 0;
		out->m[3][2] = 0;
		out->m[3][3] = 1;
#endif
	}
}

void Matrix4x4_QuakeToDuke3D(const matrix4x4_t *in, matrix4x4_t *out, double maxShearAngle)
{
	// Sorry - this isn't direct at all. We can't just use an alternative to
	// Matrix4x4_CreateFromQuakeEntity as in some cases the input for
	// generating the view matrix is generated externally.
	vec3_t forward, left, up, angles;
	double scaleforward, scaleleft, scaleup;
#ifdef MATRIX4x4_OPENGLORIENTATION
	VectorSet(forward, in->m[0][0], in->m[0][1], in->m[0][2]);
	VectorSet(left, in->m[1][0], in->m[1][1], in->m[1][2]);
	VectorSet(up, in->m[2][0], in->m[2][1], in->m[2][2]);
#else
	VectorSet(forward, in->m[0][0], in->m[1][0], in->m[2][0]);
	VectorSet(left, in->m[0][1], in->m[1][1], in->m[2][1]);
	VectorSet(up, in->m[0][2], in->m[1][2], in->m[2][2]);
#endif
	scaleforward = VectorNormalizeLength(forward);
	scaleleft = VectorNormalizeLength(left);
	scaleup = VectorNormalizeLength(up);
	AnglesFromVectors(angles, forward, up, false);
	AngleVectorsDuke3DFLU(angles, forward, left, up, maxShearAngle);
	VectorScale(forward, scaleforward, forward);
	VectorScale(left, scaleleft, left);
	VectorScale(up, scaleup, up);
	*out = *in;
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0] = forward[0];
	out->m[1][0] = left[0];
	out->m[2][0] = up[0];
	out->m[0][1] = forward[1];
	out->m[1][1] = left[1];
	out->m[2][1] = up[1];
	out->m[0][2] = forward[2];
	out->m[1][2] = left[2];
	out->m[2][2] = up[2];
#else
	out->m[0][0] = forward[0];
	out->m[0][1] = left[0];
	out->m[0][2] = up[0];
	out->m[1][0] = forward[1];
	out->m[1][1] = left[1];
	out->m[1][2] = up[1];
	out->m[2][0] = forward[2];
	out->m[2][1] = left[2];
	out->m[2][2] = up[2];
#endif
}

void Matrix4x4_ToVectors(const matrix4x4_t *in, float vx[3], float vy[3], float vz[3], float t[3])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	vx[0] = in->m[0][0];
	vx[1] = in->m[0][1];
	vx[2] = in->m[0][2];
	vy[0] = in->m[1][0];
	vy[1] = in->m[1][1];
	vy[2] = in->m[1][2];
	vz[0] = in->m[2][0];
	vz[1] = in->m[2][1];
	vz[2] = in->m[2][2];
	t [0] = in->m[3][0];
	t [1] = in->m[3][1];
	t [2] = in->m[3][2];
#else
	vx[0] = in->m[0][0];
	vx[1] = in->m[1][0];
	vx[2] = in->m[2][0];
	vy[0] = in->m[0][1];
	vy[1] = in->m[1][1];
	vy[2] = in->m[2][1];
	vz[0] = in->m[0][2];
	vz[1] = in->m[1][2];
	vz[2] = in->m[2][2];
	t [0] = in->m[0][3];
	t [1] = in->m[1][3];
	t [2] = in->m[2][3];
#endif
}

void Matrix4x4_FromVectors(matrix4x4_t *out, const float vx[3], const float vy[3], const float vz[3], const float t[3])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0] = vx[0];
	out->m[1][0] = vy[0];
	out->m[2][0] = vz[0];
	out->m[3][0] = t[0];
	out->m[0][1] = vx[1];
	out->m[1][1] = vy[1];
	out->m[2][1] = vz[1];
	out->m[3][1] = t[1];
	out->m[0][2] = vx[2];
	out->m[1][2] = vy[2];
	out->m[2][2] = vz[2];
	out->m[3][2] = t[2];
	out->m[0][3] = 0.0f;
	out->m[1][3] = 0.0f;
	out->m[2][3] = 0.0f;
	out->m[3][3] = 1.0f;
#else
	out->m[0][0] = vx[0];
	out->m[0][1] = vy[0];
	out->m[0][2] = vz[0];
	out->m[0][3] = t[0];
	out->m[1][0] = vx[1];
	out->m[1][1] = vy[1];
	out->m[1][2] = vz[1];
	out->m[1][3] = t[1];
	out->m[2][0] = vx[2];
	out->m[2][1] = vy[2];
	out->m[2][2] = vz[2];
	out->m[2][3] = t[2];
	out->m[3][0] = 0.0f;
	out->m[3][1] = 0.0f;
	out->m[3][2] = 0.0f;
	out->m[3][3] = 1.0f;
#endif
}

void Matrix4x4_ToArrayDoubleGL(const matrix4x4_t *in, double out[16])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out[ 0] = in->m[0][0];
	out[ 1] = in->m[0][1];
	out[ 2] = in->m[0][2];
	out[ 3] = in->m[0][3];
	out[ 4] = in->m[1][0];
	out[ 5] = in->m[1][1];
	out[ 6] = in->m[1][2];
	out[ 7] = in->m[1][3];
	out[ 8] = in->m[2][0];
	out[ 9] = in->m[2][1];
	out[10] = in->m[2][2];
	out[11] = in->m[2][3];
	out[12] = in->m[3][0];
	out[13] = in->m[3][1];
	out[14] = in->m[3][2];
	out[15] = in->m[3][3];
#else
	out[ 0] = in->m[0][0];
	out[ 1] = in->m[1][0];
	out[ 2] = in->m[2][0];
	out[ 3] = in->m[3][0];
	out[ 4] = in->m[0][1];
	out[ 5] = in->m[1][1];
	out[ 6] = in->m[2][1];
	out[ 7] = in->m[3][1];
	out[ 8] = in->m[0][2];
	out[ 9] = in->m[1][2];
	out[10] = in->m[2][2];
	out[11] = in->m[3][2];
	out[12] = in->m[0][3];
	out[13] = in->m[1][3];
	out[14] = in->m[2][3];
	out[15] = in->m[3][3];
#endif
}

void Matrix4x4_FromArrayDoubleGL (matrix4x4_t *out, const double in[16])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0] = in[0];
	out->m[0][1] = in[1];
	out->m[0][2] = in[2];
	out->m[0][3] = in[3];
	out->m[1][0] = in[4];
	out->m[1][1] = in[5];
	out->m[1][2] = in[6];
	out->m[1][3] = in[7];
	out->m[2][0] = in[8];
	out->m[2][1] = in[9];
	out->m[2][2] = in[10];
	out->m[2][3] = in[11];
	out->m[3][0] = in[12];
	out->m[3][1] = in[13];
	out->m[3][2] = in[14];
	out->m[3][3] = in[15];
#else
	out->m[0][0] = in[0];
	out->m[1][0] = in[1];
	out->m[2][0] = in[2];
	out->m[3][0] = in[3];
	out->m[0][1] = in[4];
	out->m[1][1] = in[5];
	out->m[2][1] = in[6];
	out->m[3][1] = in[7];
	out->m[0][2] = in[8];
	out->m[1][2] = in[9];
	out->m[2][2] = in[10];
	out->m[3][2] = in[11];
	out->m[0][3] = in[12];
	out->m[1][3] = in[13];
	out->m[2][3] = in[14];
	out->m[3][3] = in[15];
#endif
}

void Matrix4x4_ToArrayDoubleD3D(const matrix4x4_t *in, double out[16])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out[ 0] = in->m[0][0];
	out[ 1] = in->m[1][0];
	out[ 2] = in->m[2][0];
	out[ 3] = in->m[3][0];
	out[ 4] = in->m[0][1];
	out[ 5] = in->m[1][1];
	out[ 6] = in->m[2][1];
	out[ 7] = in->m[3][1];
	out[ 8] = in->m[0][2];
	out[ 9] = in->m[1][2];
	out[10] = in->m[2][2];
	out[11] = in->m[3][2];
	out[12] = in->m[0][3];
	out[13] = in->m[1][3];
	out[14] = in->m[2][3];
	out[15] = in->m[3][3];
#else
	out[ 0] = in->m[0][0];
	out[ 1] = in->m[0][1];
	out[ 2] = in->m[0][2];
	out[ 3] = in->m[0][3];
	out[ 4] = in->m[1][0];
	out[ 5] = in->m[1][1];
	out[ 6] = in->m[1][2];
	out[ 7] = in->m[1][3];
	out[ 8] = in->m[2][0];
	out[ 9] = in->m[2][1];
	out[10] = in->m[2][2];
	out[11] = in->m[2][3];
	out[12] = in->m[3][0];
	out[13] = in->m[3][1];
	out[14] = in->m[3][2];
	out[15] = in->m[3][3];
#endif
}

void Matrix4x4_FromArrayDoubleD3D (matrix4x4_t *out, const double in[4][4])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0] = in[0][0];
	out->m[1][0] = in[0][1];
	out->m[2][0] = in[0][2];
	out->m[3][0] = in[0][3];
	out->m[0][1] = in[1][0];
	out->m[1][1] = in[1][1];
	out->m[2][1] = in[1][2];
	out->m[3][1] = in[1][3];
	out->m[0][2] = in[2][0];
	out->m[1][2] = in[2][1];
	out->m[2][2] = in[2][2];
	out->m[3][2] = in[2][3];
	out->m[0][3] = in[3][0];
	out->m[1][3] = in[3][1];
	out->m[2][3] = in[3][2];
	out->m[3][3] = in[3][3];
#else
	out->m[0][0] = in[0][0];
	out->m[0][1] = in[0][1];
	out->m[0][2] = in[0][2];
	out->m[0][3] = in[0][3];
	out->m[1][0] = in[1][0];
	out->m[1][1] = in[1][1];
	out->m[1][2] = in[1][2];
	out->m[1][3] = in[1][3];
	out->m[2][0] = in[2][0];
	out->m[2][1] = in[2][1];
	out->m[2][2] = in[2][2];
	out->m[2][3] = in[2][3];
	out->m[3][0] = in[3][0];
	out->m[3][1] = in[3][1];
	out->m[3][2] = in[3][2];
	out->m[3][3] = in[3][3];
#endif
}

void Matrix4x4_ToArrayFloatGL(const matrix4x4_t *in, float out[16])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out[ 0] = in->m[0][0];
	out[ 1] = in->m[0][1];
	out[ 2] = in->m[0][2];
	out[ 3] = in->m[0][3];
	out[ 4] = in->m[1][0];
	out[ 5] = in->m[1][1];
	out[ 6] = in->m[1][2];
	out[ 7] = in->m[1][3];
	out[ 8] = in->m[2][0];
	out[ 9] = in->m[2][1];
	out[10] = in->m[2][2];
	out[11] = in->m[2][3];
	out[12] = in->m[3][0];
	out[13] = in->m[3][1];
	out[14] = in->m[3][2];
	out[15] = in->m[3][3];
#else
	out[ 0] = in->m[0][0];
	out[ 1] = in->m[1][0];
	out[ 2] = in->m[2][0];
	out[ 3] = in->m[3][0];
	out[ 4] = in->m[0][1];
	out[ 5] = in->m[1][1];
	out[ 6] = in->m[2][1];
	out[ 7] = in->m[3][1];
	out[ 8] = in->m[0][2];
	out[ 9] = in->m[1][2];
	out[10] = in->m[2][2];
	out[11] = in->m[3][2];
	out[12] = in->m[0][3];
	out[13] = in->m[1][3];
	out[14] = in->m[2][3];
	out[15] = in->m[3][3];
#endif
}

void Matrix4x4_FromArrayFloatGL (matrix4x4_t *out, const float in[16])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0] = in[0];
	out->m[0][1] = in[1];
	out->m[0][2] = in[2];
	out->m[0][3] = in[3];
	out->m[1][0] = in[4];
	out->m[1][1] = in[5];
	out->m[1][2] = in[6];
	out->m[1][3] = in[7];
	out->m[2][0] = in[8];
	out->m[2][1] = in[9];
	out->m[2][2] = in[10];
	out->m[2][3] = in[11];
	out->m[3][0] = in[12];
	out->m[3][1] = in[13];
	out->m[3][2] = in[14];
	out->m[3][3] = in[15];
#else
	out->m[0][0] = in[0];
	out->m[1][0] = in[1];
	out->m[2][0] = in[2];
	out->m[3][0] = in[3];
	out->m[0][1] = in[4];
	out->m[1][1] = in[5];
	out->m[2][1] = in[6];
	out->m[3][1] = in[7];
	out->m[0][2] = in[8];
	out->m[1][2] = in[9];
	out->m[2][2] = in[10];
	out->m[3][2] = in[11];
	out->m[0][3] = in[12];
	out->m[1][3] = in[13];
	out->m[2][3] = in[14];
	out->m[3][3] = in[15];
#endif
}

void Matrix4x4_ToArrayFloatD3D(const matrix4x4_t *in, float out[16])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out[ 0] = in->m[0][0];
	out[ 1] = in->m[1][0];
	out[ 2] = in->m[2][0];
	out[ 3] = in->m[3][0];
	out[ 4] = in->m[0][1];
	out[ 5] = in->m[1][1];
	out[ 6] = in->m[2][1];
	out[ 7] = in->m[3][1];
	out[ 8] = in->m[0][2];
	out[ 9] = in->m[1][2];
	out[10] = in->m[2][2];
	out[11] = in->m[3][2];
	out[12] = in->m[0][3];
	out[13] = in->m[1][3];
	out[14] = in->m[2][3];
	out[15] = in->m[3][3];
#else
	out[ 0] = in->m[0][0];
	out[ 1] = in->m[0][1];
	out[ 2] = in->m[0][2];
	out[ 3] = in->m[0][3];
	out[ 4] = in->m[1][0];
	out[ 5] = in->m[1][1];
	out[ 6] = in->m[1][2];
	out[ 7] = in->m[1][3];
	out[ 8] = in->m[2][0];
	out[ 9] = in->m[2][1];
	out[10] = in->m[2][2];
	out[11] = in->m[2][3];
	out[12] = in->m[3][0];
	out[13] = in->m[3][1];
	out[14] = in->m[3][2];
	out[15] = in->m[3][3];
#endif
}

void Matrix4x4_FromArrayFloatD3D (matrix4x4_t *out, const float in[16])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0] = in[0];
	out->m[1][0] = in[1];
	out->m[2][0] = in[2];
	out->m[3][0] = in[3];
	out->m[0][1] = in[4];
	out->m[1][1] = in[5];
	out->m[2][1] = in[6];
	out->m[3][1] = in[7];
	out->m[0][2] = in[8];
	out->m[1][2] = in[9];
	out->m[2][2] = in[10];
	out->m[3][2] = in[11];
	out->m[0][3] = in[12];
	out->m[1][3] = in[13];
	out->m[2][3] = in[14];
	out->m[3][3] = in[15];
#else
	out->m[0][0] = in[0];
	out->m[0][1] = in[1];
	out->m[0][2] = in[2];
	out->m[0][3] = in[3];
	out->m[1][0] = in[4];
	out->m[1][1] = in[5];
	out->m[1][2] = in[6];
	out->m[1][3] = in[7];
	out->m[2][0] = in[8];
	out->m[2][1] = in[9];
	out->m[2][2] = in[10];
	out->m[2][3] = in[11];
	out->m[3][0] = in[12];
	out->m[3][1] = in[13];
	out->m[3][2] = in[14];
	out->m[3][3] = in[15];
#endif
}

void Matrix4x4_ToArray12FloatGL(const matrix4x4_t *in, float out[4][3])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out[0][0] = in->m[0][0];
	out[0][1] = in->m[0][1];
	out[0][2] = in->m[0][2];
	out[1][0] = in->m[1][0];
	out[1][1] = in->m[1][1];
	out[1][2] = in->m[1][2];
	out[2][0] = in->m[2][0];
	out[2][1] = in->m[2][1];
	out[2][2] = in->m[2][2];
	out[3][0] = in->m[3][0];
	out[3][1] = in->m[3][1];
	out[3][2] = in->m[3][2];
#else
	out[0][0] = in->m[0][0];
	out[0][1] = in->m[1][0];
	out[0][2] = in->m[2][0];
	out[1][0] = in->m[0][1];
	out[1][1] = in->m[1][1];
	out[1][2] = in->m[2][1];
	out[2][0] = in->m[0][2];
	out[2][1] = in->m[1][2];
	out[2][2] = in->m[2][2];
	out[3][0] = in->m[0][3];
	out[3][1] = in->m[1][3];
	out[3][2] = in->m[2][3];
#endif
}

void Matrix4x4_FromArray12FloatGL(matrix4x4_t *out, const float in[12])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0] = in[0];
	out->m[0][1] = in[1];
	out->m[0][2] = in[2];
	out->m[0][3] = 0;
	out->m[1][0] = in[3];
	out->m[1][1] = in[4];
	out->m[1][2] = in[5];
	out->m[1][3] = 0;
	out->m[2][0] = in[6];
	out->m[2][1] = in[7];
	out->m[2][2] = in[8];
	out->m[2][3] = 0;
	out->m[3][0] = in[9];
	out->m[3][1] = in[10];
	out->m[3][2] = in[11];
	out->m[3][3] = 1;
#else
	out->m[0][0] = in[0];
	out->m[1][0] = in[1];
	out->m[2][0] = in[2];
	out->m[3][0] = 0;
	out->m[0][1] = in[3];
	out->m[1][1] = in[4];
	out->m[2][1] = in[5];
	out->m[3][1] = 0;
	out->m[0][2] = in[6];
	out->m[1][2] = in[7];
	out->m[2][2] = in[8];
	out->m[3][2] = 0;
	out->m[0][3] = in[9];
	out->m[1][3] = in[10];
	out->m[2][3] = in[11];
	out->m[3][3] = 1;
#endif
}

void Matrix4x4_ToArray12FloatD3D(const matrix4x4_t *in, float out[12])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out[ 0] = in->m[0][0];
	out[ 1] = in->m[1][0];
	out[ 2] = in->m[2][0];
	out[ 3] = in->m[3][0];
	out[ 4] = in->m[0][1];
	out[ 5] = in->m[1][1];
	out[ 6] = in->m[2][1];
	out[ 7] = in->m[3][1];
	out[ 8] = in->m[0][2];
	out[ 9] = in->m[1][2];
	out[10] = in->m[2][2];
	out[11] = in->m[3][2];
#else
	out[ 0] = in->m[0][0];
	out[ 1] = in->m[0][1];
	out[ 2] = in->m[0][2];
	out[ 3] = in->m[0][3];
	out[ 4] = in->m[1][0];
	out[ 5] = in->m[1][1];
	out[ 6] = in->m[1][2];
	out[ 7] = in->m[1][3];
	out[ 8] = in->m[2][0];
	out[ 9] = in->m[2][1];
	out[10] = in->m[2][2];
	out[11] = in->m[2][3];
#endif
}

void Matrix4x4_FromArray12FloatD3D(matrix4x4_t *out, const float in[12])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[0][0] = in[0];
	out->m[1][0] = in[1];
	out->m[2][0] = in[2];
	out->m[3][0] = in[3];
	out->m[0][1] = in[4];
	out->m[1][1] = in[5];
	out->m[2][1] = in[6];
	out->m[3][1] = in[7];
	out->m[0][2] = in[8];
	out->m[1][2] = in[9];
	out->m[2][2] = in[10];
	out->m[3][2] = in[11];
	out->m[0][3] = 0;
	out->m[1][3] = 0;
	out->m[2][3] = 0;
	out->m[3][3] = 1;
#else
	out->m[0][0] = in[0];
	out->m[0][1] = in[1];
	out->m[0][2] = in[2];
	out->m[0][3] = in[3];
	out->m[1][0] = in[4];
	out->m[1][1] = in[5];
	out->m[1][2] = in[6];
	out->m[1][3] = in[7];
	out->m[2][0] = in[8];
	out->m[2][1] = in[9];
	out->m[2][2] = in[10];
	out->m[2][3] = in[11];
	out->m[3][0] = 0;
	out->m[3][1] = 0;
	out->m[3][2] = 0;
	out->m[3][3] = 1;
#endif
}

void Matrix4x4_FromOriginQuat(matrix4x4_t *m, double ox, double oy, double oz, double x, double y, double z, double w)
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	m->m[0][0]=1-2*(y*y+z*z);m->m[1][0]=  2*(x*y-z*w);m->m[2][0]=  2*(x*z+y*w);m->m[3][0]=ox;
	m->m[0][1]=  2*(x*y+z*w);m->m[1][1]=1-2*(x*x+z*z);m->m[2][1]=  2*(y*z-x*w);m->m[3][1]=oy;
	m->m[0][2]=  2*(x*z-y*w);m->m[1][2]=  2*(y*z+x*w);m->m[2][2]=1-2*(x*x+y*y);m->m[3][2]=oz;
	m->m[0][3]=  0          ;m->m[1][3]=  0          ;m->m[2][3]=  0          ;m->m[3][3]=1;
#else
	m->m[0][0]=1-2*(y*y+z*z);m->m[0][1]=  2*(x*y-z*w);m->m[0][2]=  2*(x*z+y*w);m->m[0][3]=ox;
	m->m[1][0]=  2*(x*y+z*w);m->m[1][1]=1-2*(x*x+z*z);m->m[1][2]=  2*(y*z-x*w);m->m[1][3]=oy;
	m->m[2][0]=  2*(x*z-y*w);m->m[2][1]=  2*(y*z+x*w);m->m[2][2]=1-2*(x*x+y*y);m->m[2][3]=oz;
	m->m[3][0]=  0          ;m->m[3][1]=  0          ;m->m[3][2]=  0          ;m->m[3][3]=1;
#endif
}

// see http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
void Matrix4x4_ToOrigin3Quat4Float(const matrix4x4_t *m, float *origin, float *quat)
{
#if 0
	float s;
	quat[3] = sqrt(1.0f + m->m[0][0] + m->m[1][1] + m->m[2][2]) * 0.5f;
	s = 0.25f / quat[3];
#ifdef MATRIX4x4_OPENGLORIENTATION
	origin[0] = m->m[3][0];
	origin[1] = m->m[3][1];
	origin[2] = m->m[3][2];
	quat[0] = (m->m[1][2] - m->m[2][1]) * s;
	quat[1] = (m->m[2][0] - m->m[0][2]) * s;
	quat[2] = (m->m[0][1] - m->m[1][0]) * s;
#else
	origin[0] = m->m[0][3];
	origin[1] = m->m[1][3];
	origin[2] = m->m[2][3];
	quat[0] = (m->m[2][1] - m->m[1][2]) * s;
	quat[1] = (m->m[0][2] - m->m[2][0]) * s;
	quat[2] = (m->m[1][0] - m->m[0][1]) * s;
#endif

#else

#ifdef MATRIX4x4_OPENGLORIENTATION
	float trace = m->m[0][0] + m->m[1][1] + m->m[2][2];
	origin[0] = m->m[3][0];
	origin[1] = m->m[3][1];
	origin[2] = m->m[3][2];
	if(trace > 0)
	{
		float r = sqrt(1.0f + trace), inv = 0.5f / r;
		quat[0] = (m->m[1][2] - m->m[2][1]) * inv;
		quat[1] = (m->m[2][0] - m->m[0][2]) * inv;
		quat[2] = (m->m[0][1] - m->m[1][0]) * inv;
		quat[3] = 0.5f * r;
	}
	else if(m->m[0][0] > m->m[1][1] && m->m[0][0] > m->m[2][2])
	{
		float r = sqrt(1.0f + m->m[0][0] - m->m[1][1] - m->m[2][2]), inv = 0.5f / r;
		quat[0] = 0.5f * r;
		quat[1] = (m->m[0][1] + m->m[1][0]) * inv;
		quat[2] = (m->m[2][0] + m->m[0][2]) * inv;
		quat[3] = (m->m[1][2] - m->m[2][1]) * inv;
	}
	else if(m->m[1][1] > m->m[2][2])
	{
		float r = sqrt(1.0f + m->m[1][1] - m->m[0][0] - m->m[2][2]), inv = 0.5f / r;
		quat[0] = (m->m[0][1] + m->m[1][0]) * inv;
		quat[1] = 0.5f * r;
		quat[2] = (m->m[1][2] + m->m[2][1]) * inv;
		quat[3] = (m->m[2][0] - m->m[0][2]) * inv;
	}
	else
	{
		float r = sqrt(1.0f + m->m[2][2] - m->m[0][0] - m->m[1][1]), inv = 0.5f / r;
		quat[0] = (m->m[2][0] + m->m[0][2]) * inv;
		quat[1] = (m->m[1][2] + m->m[2][1]) * inv;
		quat[2] = 0.5f * r;
		quat[3] = (m->m[0][1] - m->m[1][0]) * inv;
	}
#else
	float trace = m->m[0][0] + m->m[1][1] + m->m[2][2];
	origin[0] = m->m[0][3];
	origin[1] = m->m[1][3];
	origin[2] = m->m[2][3];
	if(trace > 0)
	{
		float r = sqrt(1.0f + trace), inv = 0.5f / r;
		quat[0] = (m->m[2][1] - m->m[1][2]) * inv;
		quat[1] = (m->m[0][2] - m->m[2][0]) * inv;
		quat[2] = (m->m[1][0] - m->m[0][1]) * inv;
		quat[3] = 0.5f * r;
	}
	else if(m->m[0][0] > m->m[1][1] && m->m[0][0] > m->m[2][2])
	{
		float r = sqrt(1.0f + m->m[0][0] - m->m[1][1] - m->m[2][2]), inv = 0.5f / r;
		quat[0] = 0.5f * r;
		quat[1] = (m->m[1][0] + m->m[0][1]) * inv;
		quat[2] = (m->m[0][2] + m->m[2][0]) * inv;
		quat[3] = (m->m[2][1] - m->m[1][2]) * inv;
	}
	else if(m->m[1][1] > m->m[2][2])
	{
		float r = sqrt(1.0f + m->m[1][1] - m->m[0][0] - m->m[2][2]), inv = 0.5f / r;
		quat[0] = (m->m[1][0] + m->m[0][1]) * inv;
		quat[1] = 0.5f * r;
		quat[2] = (m->m[2][1] + m->m[1][2]) * inv;
		quat[3] = (m->m[0][2] - m->m[2][0]) * inv;
	}
	else
	{
		float r = sqrt(1.0f + m->m[2][2] - m->m[0][0] - m->m[1][1]), inv = 0.5f / r;
		quat[0] = (m->m[0][2] + m->m[2][0]) * inv;
		quat[1] = (m->m[2][1] + m->m[1][2]) * inv;
		quat[2] = 0.5f * r;
		quat[3] = (m->m[1][0] - m->m[0][1]) * inv;
	}
#endif

#endif
}

// LadyHavoc: I got this code from:
//http://www.doom3world.org/phpbb2/viewtopic.php?t=2884
void Matrix4x4_FromDoom3Joint(matrix4x4_t *m, double ox, double oy, double oz, double x, double y, double z)
{
	double w = 1.0f - (x*x+y*y+z*z);
	w = w > 0.0f ? -sqrt(w) : 0.0f;
#ifdef MATRIX4x4_OPENGLORIENTATION
	m->m[0][0]=1-2*(y*y+z*z);m->m[1][0]=  2*(x*y-z*w);m->m[2][0]=  2*(x*z+y*w);m->m[3][0]=ox;
	m->m[0][1]=  2*(x*y+z*w);m->m[1][1]=1-2*(x*x+z*z);m->m[2][1]=  2*(y*z-x*w);m->m[3][1]=oy;
	m->m[0][2]=  2*(x*z-y*w);m->m[1][2]=  2*(y*z+x*w);m->m[2][2]=1-2*(x*x+y*y);m->m[3][2]=oz;
	m->m[0][3]=  0          ;m->m[1][3]=  0          ;m->m[2][3]=  0          ;m->m[3][3]=1;
#else
	m->m[0][0]=1-2*(y*y+z*z);m->m[0][1]=  2*(x*y-z*w);m->m[0][2]=  2*(x*z+y*w);m->m[0][3]=ox;
	m->m[1][0]=  2*(x*y+z*w);m->m[1][1]=1-2*(x*x+z*z);m->m[1][2]=  2*(y*z-x*w);m->m[1][3]=oy;
	m->m[2][0]=  2*(x*z-y*w);m->m[2][1]=  2*(y*z+x*w);m->m[2][2]=1-2*(x*x+y*y);m->m[2][3]=oz;
	m->m[3][0]=  0          ;m->m[3][1]=  0          ;m->m[3][2]=  0          ;m->m[3][3]=1;
#endif
}

void Matrix4x4_FromBonePose7s(matrix4x4_t *m, float originscale, const short *pose7s)
{
	float origin[3];
	float quat[4];
	float quatscale = pose7s[6] > 0 ? -1.0f / 32767.0f : 1.0f / 32767.0f;
	origin[0] = pose7s[0] * originscale;
	origin[1] = pose7s[1] * originscale;
	origin[2] = pose7s[2] * originscale;
	quat[0] = pose7s[3] * quatscale;
	quat[1] = pose7s[4] * quatscale;
	quat[2] = pose7s[5] * quatscale;
	quat[3] = pose7s[6] * quatscale;
	Matrix4x4_FromOriginQuat(m, origin[0], origin[1], origin[2], quat[0], quat[1], quat[2], quat[3]);
}

void Matrix4x4_ToBonePose7s(const matrix4x4_t *m, float origininvscale, short *pose7s)
{
	float origin[3];
	float quat[4];
	float quatscale;
	Matrix4x4_ToOrigin3Quat4Float(m, origin, quat);
	// normalize quaternion so that it is unit length
	quatscale = quat[0]*quat[0]+quat[1]*quat[1]+quat[2]*quat[2]+quat[3]*quat[3];
	if (quatscale)
		quatscale = (quat[3] >= 0 ? -32767.0f : 32767.0f) / sqrt(quatscale);
	// use a negative scale on the quat because the above function produces a
	// positive quat[3] and canonical quaternions have negative quat[3]
	pose7s[0] = origin[0] * origininvscale;
	pose7s[1] = origin[1] * origininvscale;
	pose7s[2] = origin[2] * origininvscale;
	pose7s[3] = quat[0] * quatscale;
	pose7s[4] = quat[1] * quatscale;
	pose7s[5] = quat[2] * quatscale;
	pose7s[6] = quat[3] * quatscale;
}

void Matrix4x4_Blend (matrix4x4_t *out, const matrix4x4_t *in1, const matrix4x4_t *in2, double blend)
{
	double iblend = 1 - blend;
	out->m[0][0] = in1->m[0][0] * iblend + in2->m[0][0] * blend;
	out->m[0][1] = in1->m[0][1] * iblend + in2->m[0][1] * blend;
	out->m[0][2] = in1->m[0][2] * iblend + in2->m[0][2] * blend;
	out->m[0][3] = in1->m[0][3] * iblend + in2->m[0][3] * blend;
	out->m[1][0] = in1->m[1][0] * iblend + in2->m[1][0] * blend;
	out->m[1][1] = in1->m[1][1] * iblend + in2->m[1][1] * blend;
	out->m[1][2] = in1->m[1][2] * iblend + in2->m[1][2] * blend;
	out->m[1][3] = in1->m[1][3] * iblend + in2->m[1][3] * blend;
	out->m[2][0] = in1->m[2][0] * iblend + in2->m[2][0] * blend;
	out->m[2][1] = in1->m[2][1] * iblend + in2->m[2][1] * blend;
	out->m[2][2] = in1->m[2][2] * iblend + in2->m[2][2] * blend;
	out->m[2][3] = in1->m[2][3] * iblend + in2->m[2][3] * blend;
	out->m[3][0] = in1->m[3][0] * iblend + in2->m[3][0] * blend;
	out->m[3][1] = in1->m[3][1] * iblend + in2->m[3][1] * blend;
	out->m[3][2] = in1->m[3][2] * iblend + in2->m[3][2] * blend;
	out->m[3][3] = in1->m[3][3] * iblend + in2->m[3][3] * blend;
}


void Matrix4x4_Transform (const matrix4x4_t *in, const float v[3], float out[3])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out[0] = v[0] * in->m[0][0] + v[1] * in->m[1][0] + v[2] * in->m[2][0] + in->m[3][0];
	out[1] = v[0] * in->m[0][1] + v[1] * in->m[1][1] + v[2] * in->m[2][1] + in->m[3][1];
	out[2] = v[0] * in->m[0][2] + v[1] * in->m[1][2] + v[2] * in->m[2][2] + in->m[3][2];
#else
	out[0] = v[0] * in->m[0][0] + v[1] * in->m[0][1] + v[2] * in->m[0][2] + in->m[0][3];
	out[1] = v[0] * in->m[1][0] + v[1] * in->m[1][1] + v[2] * in->m[1][2] + in->m[1][3];
	out[2] = v[0] * in->m[2][0] + v[1] * in->m[2][1] + v[2] * in->m[2][2] + in->m[2][3];
#endif
}

void Matrix4x4_Transform4 (const matrix4x4_t *in, const float v[4], float out[4])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out[0] = v[0] * in->m[0][0] + v[1] * in->m[1][0] + v[2] * in->m[2][0] + v[3] * in->m[3][0];
	out[1] = v[0] * in->m[0][1] + v[1] * in->m[1][1] + v[2] * in->m[2][1] + v[3] * in->m[3][1];
	out[2] = v[0] * in->m[0][2] + v[1] * in->m[1][2] + v[2] * in->m[2][2] + v[3] * in->m[3][2];
	out[3] = v[0] * in->m[0][3] + v[1] * in->m[1][3] + v[2] * in->m[2][3] + v[3] * in->m[3][3];
#else
	out[0] = v[0] * in->m[0][0] + v[1] * in->m[0][1] + v[2] * in->m[0][2] + v[3] * in->m[0][3];
	out[1] = v[0] * in->m[1][0] + v[1] * in->m[1][1] + v[2] * in->m[1][2] + v[3] * in->m[1][3];
	out[2] = v[0] * in->m[2][0] + v[1] * in->m[2][1] + v[2] * in->m[2][2] + v[3] * in->m[2][3];
	out[3] = v[0] * in->m[3][0] + v[1] * in->m[3][1] + v[2] * in->m[3][2] + v[3] * in->m[3][3];
#endif
}

void Matrix4x4_Transform3x3 (const matrix4x4_t *in, const float v[3], float out[3])
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out[0] = v[0] * in->m[0][0] + v[1] * in->m[1][0] + v[2] * in->m[2][0];
	out[1] = v[0] * in->m[0][1] + v[1] * in->m[1][1] + v[2] * in->m[2][1];
	out[2] = v[0] * in->m[0][2] + v[1] * in->m[1][2] + v[2] * in->m[2][2];
#else
	out[0] = v[0] * in->m[0][0] + v[1] * in->m[0][1] + v[2] * in->m[0][2];
	out[1] = v[0] * in->m[1][0] + v[1] * in->m[1][1] + v[2] * in->m[1][2];
	out[2] = v[0] * in->m[2][0] + v[1] * in->m[2][1] + v[2] * in->m[2][2];
#endif
}

// transforms a positive distance plane (A*x+B*y+C*z-D=0) through a rotation or translation matrix
void Matrix4x4_TransformPositivePlane(const matrix4x4_t *in, float x, float y, float z, float d, float *o)
{
	float scale = sqrt(in->m[0][0] * in->m[0][0] + in->m[0][1] * in->m[0][1] + in->m[0][2] * in->m[0][2]);
	float iscale = 1.0f / scale;
#ifdef MATRIX4x4_OPENGLORIENTATION
	o[0] = (x * in->m[0][0] + y * in->m[1][0] + z * in->m[2][0]) * iscale;
	o[1] = (x * in->m[0][1] + y * in->m[1][1] + z * in->m[2][1]) * iscale;
	o[2] = (x * in->m[0][2] + y * in->m[1][2] + z * in->m[2][2]) * iscale;
	o[3] = d * scale + (o[0] * in->m[3][0] + o[1] * in->m[3][1] + o[2] * in->m[3][2]);
#else
	o[0] = (x * in->m[0][0] + y * in->m[0][1] + z * in->m[0][2]) * iscale;
	o[1] = (x * in->m[1][0] + y * in->m[1][1] + z * in->m[1][2]) * iscale;
	o[2] = (x * in->m[2][0] + y * in->m[2][1] + z * in->m[2][2]) * iscale;
	o[3] = d * scale + (o[0] * in->m[0][3] + o[1] * in->m[1][3] + o[2] * in->m[2][3]);
#endif
}

// transforms a standard plane (A*x+B*y+C*z+D=0) through a rotation or translation matrix
void Matrix4x4_TransformStandardPlane(const matrix4x4_t *in, float x, float y, float z, float d, float *o)
{
	float scale = sqrt(in->m[0][0] * in->m[0][0] + in->m[0][1] * in->m[0][1] + in->m[0][2] * in->m[0][2]);
	float iscale = 1.0f / scale;
#ifdef MATRIX4x4_OPENGLORIENTATION
	o[0] = (x * in->m[0][0] + y * in->m[1][0] + z * in->m[2][0]) * iscale;
	o[1] = (x * in->m[0][1] + y * in->m[1][1] + z * in->m[2][1]) * iscale;
	o[2] = (x * in->m[0][2] + y * in->m[1][2] + z * in->m[2][2]) * iscale;
	o[3] = d * scale - (o[0] * in->m[3][0] + o[1] * in->m[3][1] + o[2] * in->m[3][2]);
#else
	o[0] = (x * in->m[0][0] + y * in->m[0][1] + z * in->m[0][2]) * iscale;
	o[1] = (x * in->m[1][0] + y * in->m[1][1] + z * in->m[1][2]) * iscale;
	o[2] = (x * in->m[2][0] + y * in->m[2][1] + z * in->m[2][2]) * iscale;
	o[3] = d * scale - (o[0] * in->m[0][3] + o[1] * in->m[1][3] + o[2] * in->m[2][3]);
#endif
}

/*
void Matrix4x4_SimpleUntransform (const matrix4x4_t *in, const float v[3], float out[3])
{
	double t[3];
#ifdef MATRIX4x4_OPENGLORIENTATION
	t[0] = v[0] - in->m[3][0];
	t[1] = v[1] - in->m[3][1];
	t[2] = v[2] - in->m[3][2];
	out[0] = t[0] * in->m[0][0] + t[1] * in->m[0][1] + t[2] * in->m[0][2];
	out[1] = t[0] * in->m[1][0] + t[1] * in->m[1][1] + t[2] * in->m[1][2];
	out[2] = t[0] * in->m[2][0] + t[1] * in->m[2][1] + t[2] * in->m[2][2];
#else
	t[0] = v[0] - in->m[0][3];
	t[1] = v[1] - in->m[1][3];
	t[2] = v[2] - in->m[2][3];
	out[0] = t[0] * in->m[0][0] + t[1] * in->m[1][0] + t[2] * in->m[2][0];
	out[1] = t[0] * in->m[0][1] + t[1] * in->m[1][1] + t[2] * in->m[2][1];
	out[2] = t[0] * in->m[0][2] + t[1] * in->m[1][2] + t[2] * in->m[2][2];
#endif
}
*/

// FIXME: optimize
void Matrix4x4_ConcatTranslate (matrix4x4_t *out, double x, double y, double z)
{
	matrix4x4_t base, temp;
	base = *out;
	Matrix4x4_CreateTranslate(&temp, x, y, z);
	Matrix4x4_Concat(out, &base, &temp);
}

// FIXME: optimize
void Matrix4x4_ConcatRotate (matrix4x4_t *out, double angle, double x, double y, double z)
{
	matrix4x4_t base, temp;
	base = *out;
	Matrix4x4_CreateRotate(&temp, angle, x, y, z);
	Matrix4x4_Concat(out, &base, &temp);
}

// FIXME: optimize
void Matrix4x4_ConcatScale (matrix4x4_t *out, double x)
{
	matrix4x4_t base, temp;
	base = *out;
	Matrix4x4_CreateScale(&temp, x);
	Matrix4x4_Concat(out, &base, &temp);
}

// FIXME: optimize
void Matrix4x4_ConcatScale3 (matrix4x4_t *out, double x, double y, double z)
{
	matrix4x4_t base, temp;
	base = *out;
	Matrix4x4_CreateScale3(&temp, x, y, z);
	Matrix4x4_Concat(out, &base, &temp);
}

void Matrix4x4_OriginFromMatrix (const matrix4x4_t *in, float *out)
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out[0] = in->m[3][0];
	out[1] = in->m[3][1];
	out[2] = in->m[3][2];
#else
	out[0] = in->m[0][3];
	out[1] = in->m[1][3];
	out[2] = in->m[2][3];
#endif
}

double Matrix4x4_ScaleFromMatrix (const matrix4x4_t *in)
{
	// we only support uniform scaling, so assume the first row is enough
	return sqrt(in->m[0][0] * in->m[0][0] + in->m[0][1] * in->m[0][1] + in->m[0][2] * in->m[0][2]);
}

void Matrix4x4_SetOrigin (matrix4x4_t *out, double x, double y, double z)
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[3][0] = x;
	out->m[3][1] = y;
	out->m[3][2] = z;
#else
	out->m[0][3] = x;
	out->m[1][3] = y;
	out->m[2][3] = z;
#endif
}

void Matrix4x4_AdjustOrigin (matrix4x4_t *out, double x, double y, double z)
{
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[3][0] += x;
	out->m[3][1] += y;
	out->m[3][2] += z;
#else
	out->m[0][3] += x;
	out->m[1][3] += y;
	out->m[2][3] += z;
#endif
}

void Matrix4x4_Scale (matrix4x4_t *out, double rotatescale, double originscale)
{
	out->m[0][0] *= rotatescale;
	out->m[0][1] *= rotatescale;
	out->m[0][2] *= rotatescale;
	out->m[1][0] *= rotatescale;
	out->m[1][1] *= rotatescale;
	out->m[1][2] *= rotatescale;
	out->m[2][0] *= rotatescale;
	out->m[2][1] *= rotatescale;
	out->m[2][2] *= rotatescale;
#ifdef MATRIX4x4_OPENGLORIENTATION
	out->m[3][0] *= originscale;
	out->m[3][1] *= originscale;
	out->m[3][2] *= originscale;
#else
	out->m[0][3] *= originscale;
	out->m[1][3] *= originscale;
	out->m[2][3] *= originscale;
#endif
}

void Matrix4x4_Abs (matrix4x4_t *out)
{
	out->m[0][0] = fabs(out->m[0][0]);
	out->m[0][1] = fabs(out->m[0][1]);
	out->m[0][2] = fabs(out->m[0][2]);
	out->m[1][0] = fabs(out->m[1][0]);
	out->m[1][1] = fabs(out->m[1][1]);
	out->m[1][2] = fabs(out->m[1][2]);
	out->m[2][0] = fabs(out->m[2][0]);
	out->m[2][1] = fabs(out->m[2][1]);
	out->m[2][2] = fabs(out->m[2][2]);
}

