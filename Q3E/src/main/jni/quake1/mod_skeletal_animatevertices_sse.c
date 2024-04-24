#include "mod_skeletal_animatevertices_sse.h"

#ifdef SSE_POSSIBLE

#ifdef MATRIX4x4_OPENGLORIENTATION
#error "SSE skeletal requires D3D matrix layout"
#endif

#include <xmmintrin.h>

void Mod_Skeletal_AnimateVertices_SSE(const model_t * RESTRICT model, const frameblend_t * RESTRICT frameblend, const skeleton_t *skeleton, float * RESTRICT vertex3f, float * RESTRICT normal3f, float * RESTRICT svector3f, float * RESTRICT tvector3f)
{
	// vertex weighted skeletal
	int i, k;
	int blends;
	matrix4x4_t *bonepose;
	matrix4x4_t *boneposerelative;
	const blendweights_t * RESTRICT weights;
	int num_vertices_minus_one;

	num_vertices_minus_one = model->surfmesh.num_vertices - 1;

	//unsigned long long ts = rdtsc();
	bonepose = (matrix4x4_t *) Mod_Skeletal_AnimateVertices_AllocBuffers(sizeof(matrix4x4_t) * (model->num_bones*2 + model->surfmesh.num_blends));
	boneposerelative = bonepose + model->num_bones;

	if (skeleton && !skeleton->relativetransforms)
		skeleton = NULL;

	// interpolate matrices
	if (skeleton)
	{
		for (i = 0;i < model->num_bones;i++)
		{
			const float * RESTRICT n = model->data_baseboneposeinverse + i * 12;
			matrix4x4_t * RESTRICT s = &skeleton->relativetransforms[i];
			matrix4x4_t * RESTRICT b = &bonepose[i];
			matrix4x4_t * RESTRICT r = &boneposerelative[i];
			__m128 b0, b1, b2, b3, r0, r1, r2, r3, nr;
			if (model->data_bones[i].parent >= 0)
			{
				const matrix4x4_t * RESTRICT p = &bonepose[model->data_bones[i].parent];
				__m128 s0 = _mm_loadu_ps(s->m[0]), s1 = _mm_loadu_ps(s->m[1]), s2 = _mm_loadu_ps(s->m[2]);
#ifdef OPENGLORIENTATION
				__m128 s3 = _mm_loadu_ps(s->m[3]);
#define SKELETON_MATRIX(r, c) _mm_shuffle_ps(s##c, s##c, _MM_SHUFFLE(r, r, r, r))
#else
#define SKELETON_MATRIX(r, c) _mm_shuffle_ps(s##r, s##r, _MM_SHUFFLE(c, c, c, c))
#endif
				__m128 pr = _mm_load_ps(p->m[0]);
				b0 = _mm_mul_ps(pr, SKELETON_MATRIX(0, 0));
				b1 = _mm_mul_ps(pr, SKELETON_MATRIX(0, 1));
				b2 = _mm_mul_ps(pr, SKELETON_MATRIX(0, 2));
				b3 = _mm_mul_ps(pr, SKELETON_MATRIX(0, 3));
				pr = _mm_load_ps(p->m[1]);
				b0 = _mm_add_ps(b0, _mm_mul_ps(pr, SKELETON_MATRIX(1, 0)));
				b1 = _mm_add_ps(b1, _mm_mul_ps(pr, SKELETON_MATRIX(1, 1)));
				b2 = _mm_add_ps(b2, _mm_mul_ps(pr, SKELETON_MATRIX(1, 2)));
				b3 = _mm_add_ps(b3, _mm_mul_ps(pr, SKELETON_MATRIX(1, 3)));
				pr = _mm_load_ps(p->m[2]);
				b0 = _mm_add_ps(b0, _mm_mul_ps(pr, SKELETON_MATRIX(2, 0)));
				b1 = _mm_add_ps(b1, _mm_mul_ps(pr, SKELETON_MATRIX(2, 1)));
				b2 = _mm_add_ps(b2, _mm_mul_ps(pr, SKELETON_MATRIX(2, 2)));
				b3 = _mm_add_ps(b3, _mm_mul_ps(pr, SKELETON_MATRIX(2, 3)));
				b3 = _mm_add_ps(b3, _mm_load_ps(p->m[3]));
			}
			else
			{
				b0 = _mm_loadu_ps(s->m[0]);
				b1 = _mm_loadu_ps(s->m[1]);
				b2 = _mm_loadu_ps(s->m[2]);
				b3 = _mm_loadu_ps(s->m[3]);
#ifndef OPENGLORIENTATION
				_MM_TRANSPOSE4_PS(b0, b1, b2, b3);
#endif
			}
			_mm_store_ps(b->m[0], b0);
			_mm_store_ps(b->m[1], b1);
			_mm_store_ps(b->m[2], b2);
			_mm_store_ps(b->m[3], b3);
			nr = _mm_loadu_ps(n);
			r0 = _mm_mul_ps(b0, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(0, 0, 0, 0)));
			r1 = _mm_mul_ps(b0, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(1, 1, 1, 1)));
			r2 = _mm_mul_ps(b0, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(2, 2, 2, 2)));
			r3 = _mm_mul_ps(b0, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(3, 3, 3, 3)));
			nr = _mm_loadu_ps(n+4);
			r0 = _mm_add_ps(r0, _mm_mul_ps(b1, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(0, 0, 0, 0))));
			r1 = _mm_add_ps(r1, _mm_mul_ps(b1, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(1, 1, 1, 1))));
			r2 = _mm_add_ps(r2, _mm_mul_ps(b1, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(2, 2, 2, 2))));
			r3 = _mm_add_ps(r3, _mm_mul_ps(b1, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(3, 3, 3, 3))));
			nr = _mm_loadu_ps(n+8);
			r0 = _mm_add_ps(r0, _mm_mul_ps(b2, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(0, 0, 0, 0))));
			r1 = _mm_add_ps(r1, _mm_mul_ps(b2, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(1, 1, 1, 1))));
			r2 = _mm_add_ps(r2, _mm_mul_ps(b2, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(2, 2, 2, 2))));
			r3 = _mm_add_ps(r3, _mm_mul_ps(b2, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(3, 3, 3, 3))));
			r3 = _mm_add_ps(r3, b3);
			_mm_store_ps(r->m[0], r0);
			_mm_store_ps(r->m[1], r1);
			_mm_store_ps(r->m[2], r2);
			_mm_store_ps(r->m[3], r3);
		}
	}
	else
	{
		for (i = 0;i < model->num_bones;i++)
		{
			float m[12];
			const short * RESTRICT firstpose7s = model->data_poses7s + 7 * (frameblend[0].subframe * model->num_bones + i);
			float firstlerp = frameblend[0].lerp,
				firsttx = firstpose7s[0], firstty = firstpose7s[1], firsttz = firstpose7s[2],
				rx = firstpose7s[3] * firstlerp,
				ry = firstpose7s[4] * firstlerp,
				rz = firstpose7s[5] * firstlerp,
				rw = firstpose7s[6] * firstlerp,
				dx = firsttx*rw + firstty*rz - firsttz*ry,
				dy = -firsttx*rz + firstty*rw + firsttz*rx,
				dz = firsttx*ry - firstty*rx + firsttz*rw,
				dw = -firsttx*rx - firstty*ry - firsttz*rz,
				scale, sx, sy, sz, sw;
			for (blends = 1;blends < MAX_FRAMEBLENDS && frameblend[blends].lerp > 0;blends++)
			{
				const short * RESTRICT blendpose7s = model->data_poses7s + 7 * (frameblend[blends].subframe * model->num_bones + i);
				float blendlerp = frameblend[blends].lerp,
					blendtx = blendpose7s[0], blendty = blendpose7s[1], blendtz = blendpose7s[2],
					qx = blendpose7s[3], qy = blendpose7s[4], qz = blendpose7s[5], qw = blendpose7s[6];
				if(rx*qx + ry*qy + rz*qz + rw*qw < 0) blendlerp = -blendlerp;
				qx *= blendlerp;
				qy *= blendlerp;
				qz *= blendlerp;
				qw *= blendlerp;
				rx += qx;
				ry += qy;
				rz += qz;
				rw += qw;
				dx += blendtx*qw + blendty*qz - blendtz*qy;
				dy += -blendtx*qz + blendty*qw + blendtz*qx;
				dz += blendtx*qy - blendty*qx + blendtz*qw;
				dw += -blendtx*qx - blendty*qy - blendtz*qz;
			}
			scale = 1.0f / (rx*rx + ry*ry + rz*rz + rw*rw);
			sx = rx * scale;
			sy = ry * scale;
			sz = rz * scale;
			sw = rw * scale;
			m[0] = sw*rw + sx*rx - sy*ry - sz*rz;
			m[1] = 2*(sx*ry - sw*rz);
			m[2] = 2*(sx*rz + sw*ry);
			m[3] = model->num_posescale*(dx*sw - dy*sz + dz*sy - dw*sx);
			m[4] = 2*(sx*ry + sw*rz);
			m[5] = sw*rw + sy*ry - sx*rx - sz*rz;
			m[6] = 2*(sy*rz - sw*rx);
			m[7] = model->num_posescale*(dx*sz + dy*sw - dz*sx - dw*sy);
			m[8] = 2*(sx*rz - sw*ry);
			m[9] = 2*(sy*rz + sw*rx);
			m[10] = sw*rw + sz*rz - sx*rx - sy*ry;
			m[11] = model->num_posescale*(dy*sx + dz*sw - dx*sy - dw*sz);
			if (i == r_skeletal_debugbone.integer)
				m[r_skeletal_debugbonecomponent.integer % 12] += r_skeletal_debugbonevalue.value;
			m[3] *= r_skeletal_debugtranslatex.value;
			m[7] *= r_skeletal_debugtranslatey.value;
			m[11] *= r_skeletal_debugtranslatez.value;
			{
				const float * RESTRICT n = model->data_baseboneposeinverse + i * 12;
				matrix4x4_t * RESTRICT b = &bonepose[i];
				matrix4x4_t * RESTRICT r = &boneposerelative[i];
				__m128 b0, b1, b2, b3, r0, r1, r2, r3, nr;
				if (model->data_bones[i].parent >= 0)
				{
					const matrix4x4_t * RESTRICT p = &bonepose[model->data_bones[i].parent];
					__m128 pr = _mm_load_ps(p->m[0]);
					b0 = _mm_mul_ps(pr, _mm_set1_ps(m[0]));
					b1 = _mm_mul_ps(pr, _mm_set1_ps(m[1]));
					b2 = _mm_mul_ps(pr, _mm_set1_ps(m[2]));
					b3 = _mm_mul_ps(pr, _mm_set1_ps(m[3]));
					pr = _mm_load_ps(p->m[1]);
					b0 = _mm_add_ps(b0, _mm_mul_ps(pr, _mm_set1_ps(m[4])));
					b1 = _mm_add_ps(b1, _mm_mul_ps(pr, _mm_set1_ps(m[5])));
					b2 = _mm_add_ps(b2, _mm_mul_ps(pr, _mm_set1_ps(m[6])));
					b3 = _mm_add_ps(b3, _mm_mul_ps(pr, _mm_set1_ps(m[7])));
					pr = _mm_load_ps(p->m[2]);
					b0 = _mm_add_ps(b0, _mm_mul_ps(pr, _mm_set1_ps(m[8])));
					b1 = _mm_add_ps(b1, _mm_mul_ps(pr, _mm_set1_ps(m[9])));
					b2 = _mm_add_ps(b2, _mm_mul_ps(pr, _mm_set1_ps(m[10])));
					b3 = _mm_add_ps(b3, _mm_mul_ps(pr, _mm_set1_ps(m[11])));
					b3 = _mm_add_ps(b3, _mm_load_ps(p->m[3]));
				}
				else
				{
					b0 = _mm_setr_ps(m[0], m[4], m[8], 0.0f);
					b1 = _mm_setr_ps(m[1], m[5], m[9], 0.0f);
					b2 = _mm_setr_ps(m[2], m[6], m[10], 0.0f);
					b3 = _mm_setr_ps(m[3], m[7], m[11], 1.0f);
				}
				_mm_store_ps(b->m[0], b0);
				_mm_store_ps(b->m[1], b1);
				_mm_store_ps(b->m[2], b2);
				_mm_store_ps(b->m[3], b3);
				nr = _mm_loadu_ps(n);
				r0 = _mm_mul_ps(b0, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(0, 0, 0, 0)));
				r1 = _mm_mul_ps(b0, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(1, 1, 1, 1)));
				r2 = _mm_mul_ps(b0, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(2, 2, 2, 2)));
				r3 = _mm_mul_ps(b0, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(3, 3, 3, 3)));
				nr = _mm_loadu_ps(n+4);
				r0 = _mm_add_ps(r0, _mm_mul_ps(b1, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(0, 0, 0, 0))));
				r1 = _mm_add_ps(r1, _mm_mul_ps(b1, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(1, 1, 1, 1))));
				r2 = _mm_add_ps(r2, _mm_mul_ps(b1, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(2, 2, 2, 2))));
				r3 = _mm_add_ps(r3, _mm_mul_ps(b1, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(3, 3, 3, 3))));
				nr = _mm_loadu_ps(n+8);
				r0 = _mm_add_ps(r0, _mm_mul_ps(b2, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(0, 0, 0, 0))));
				r1 = _mm_add_ps(r1, _mm_mul_ps(b2, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(1, 1, 1, 1))));
				r2 = _mm_add_ps(r2, _mm_mul_ps(b2, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(2, 2, 2, 2))));
				r3 = _mm_add_ps(r3, _mm_mul_ps(b2, _mm_shuffle_ps(nr, nr, _MM_SHUFFLE(3, 3, 3, 3))));
				r3 = _mm_add_ps(r3, b3);
				_mm_store_ps(r->m[0], r0);
				_mm_store_ps(r->m[1], r1);
				_mm_store_ps(r->m[2], r2);
				_mm_store_ps(r->m[3], r3);
			}	
		}
	}

	// generate matrices for all blend combinations
	weights = model->surfmesh.data_blendweights;
	for (i = 0;i < model->surfmesh.num_blends;i++, weights++)
	{
		float * RESTRICT b = &boneposerelative[model->num_bones + i].m[0][0];
		const float * RESTRICT m = &boneposerelative[weights->index[0]].m[0][0];
		float f = weights->influence[0] * (1.0f / 255.0f);
		__m128 fv = _mm_set_ps1(f);
		__m128 b0 = _mm_load_ps(m);
		__m128 b1 = _mm_load_ps(m+4);
		__m128 b2 = _mm_load_ps(m+8);
		__m128 b3 = _mm_load_ps(m+12);
		__m128 m0, m1, m2, m3;
		b0 = _mm_mul_ps(b0, fv);
		b1 = _mm_mul_ps(b1, fv);
		b2 = _mm_mul_ps(b2, fv);
		b3 = _mm_mul_ps(b3, fv);
		for (k = 1;k < 4 && weights->influence[k];k++)
		{
			m = &boneposerelative[weights->index[k]].m[0][0];
			f = weights->influence[k] * (1.0f / 255.0f);
			fv = _mm_set_ps1(f);
			m0 = _mm_load_ps(m);
			m1 = _mm_load_ps(m+4);
			m2 = _mm_load_ps(m+8);
			m3 = _mm_load_ps(m+12);
			m0 = _mm_mul_ps(m0, fv);
			m1 = _mm_mul_ps(m1, fv);
			m2 = _mm_mul_ps(m2, fv);
			m3 = _mm_mul_ps(m3, fv);
			b0 = _mm_add_ps(m0, b0);
			b1 = _mm_add_ps(m1, b1);
			b2 = _mm_add_ps(m2, b2);
			b3 = _mm_add_ps(m3, b3);
		}
		_mm_store_ps(b, b0);
		_mm_store_ps(b+4, b1);
		_mm_store_ps(b+8, b2);
		_mm_store_ps(b+12, b3);
	}

#define LOAD_MATRIX_SCALAR() const float * RESTRICT m = &boneposerelative[*b].m[0][0]

#define LOAD_MATRIX3() \
	const float * RESTRICT m = &boneposerelative[*b].m[0][0]; \
	/* bonepose array is 16 byte aligned */ \
	__m128 m1 = _mm_load_ps((m)); \
	__m128 m2 = _mm_load_ps((m)+4); \
	__m128 m3 = _mm_load_ps((m)+8);
#define LOAD_MATRIX4() \
	const float * RESTRICT m = &boneposerelative[*b].m[0][0]; \
	/* bonepose array is 16 byte aligned */ \
	__m128 m1 = _mm_load_ps((m)); \
	__m128 m2 = _mm_load_ps((m)+4); \
	__m128 m3 = _mm_load_ps((m)+8); \
	__m128 m4 = _mm_load_ps((m)+12)

	/* Note that matrix is 4x4 and transposed compared to non-USE_SSE codepath */
#define TRANSFORM_POSITION_SCALAR(in, out) \
	(out)[0] = ((in)[0] * m[0] + (in)[1] * m[4] + (in)[2] * m[ 8] + m[12]); \
	(out)[1] = ((in)[0] * m[1] + (in)[1] * m[5] + (in)[2] * m[ 9] + m[13]); \
	(out)[2] = ((in)[0] * m[2] + (in)[1] * m[6] + (in)[2] * m[10] + m[14]);
#define TRANSFORM_VECTOR_SCALAR(in, out) \
	(out)[0] = ((in)[0] * m[0] + (in)[1] * m[4] + (in)[2] * m[ 8]); \
	(out)[1] = ((in)[0] * m[1] + (in)[1] * m[5] + (in)[2] * m[ 9]); \
	(out)[2] = ((in)[0] * m[2] + (in)[1] * m[6] + (in)[2] * m[10]);

#define TRANSFORM_POSITION(in, out) { \
		__m128 pin = _mm_loadu_ps(in); /* we ignore the value in the last element (x from the next vertex) */ \
		__m128 x = _mm_shuffle_ps(pin, pin, 0x0); \
		__m128 t1 = _mm_mul_ps(x, m1); \
		\
		/* y, + x */ \
		__m128 y = _mm_shuffle_ps(pin, pin, 0x55); \
		__m128 t2 = _mm_mul_ps(y, m2); \
		__m128 t3 = _mm_add_ps(t1, t2); \
		\
		/* z, + (y+x) */ \
		__m128 z = _mm_shuffle_ps(pin, pin, 0xaa); \
		__m128 t4 = _mm_mul_ps(z, m3); \
		__m128 t5 = _mm_add_ps(t3, t4); \
		\
		/* + m3 */ \
		__m128 pout = _mm_add_ps(t5, m4); \
		_mm_storeu_ps((out), pout); \
	}

#define TRANSFORM_VECTOR(in, out) { \
		__m128 vin = _mm_loadu_ps(in); \
		\
		/* x */ \
		__m128 x = _mm_shuffle_ps(vin, vin, 0x0); \
		__m128 t1 = _mm_mul_ps(x, m1); \
		\
		/* y, + x */ \
		__m128 y = _mm_shuffle_ps(vin, vin, 0x55); \
		__m128 t2 = _mm_mul_ps(y, m2); \
		__m128 t3 = _mm_add_ps(t1, t2); \
		\
		/* nz, + (ny + nx) */ \
		__m128 z = _mm_shuffle_ps(vin, vin, 0xaa); \
		__m128 t4 = _mm_mul_ps(z, m3); \
		__m128 vout = _mm_add_ps(t3, t4); \
		_mm_storeu_ps((out), vout); \
	}

	// transform vertex attributes by blended matrices
	if (vertex3f)
	{
		const float * RESTRICT v = model->surfmesh.data_vertex3f;
		const unsigned short * RESTRICT b = model->surfmesh.blends;
		// special case common combinations of attributes to avoid repeated loading of matrices
		if (normal3f)
		{
			const float * RESTRICT n = model->surfmesh.data_normal3f;
			if (svector3f && tvector3f)
			{
				const float * RESTRICT svec = model->surfmesh.data_svector3f;
				const float * RESTRICT tvec = model->surfmesh.data_tvector3f;

				// Note that for SSE each iteration stores one element past end, so we break one vertex short
				// and handle that with scalars in that case
				for (i = 0; i < num_vertices_minus_one; i++, v += 3, n += 3, svec += 3, tvec += 3, b++,
						vertex3f += 3, normal3f += 3, svector3f += 3, tvector3f += 3)
				{
					LOAD_MATRIX4();
					TRANSFORM_POSITION(v, vertex3f);
					TRANSFORM_VECTOR(n, normal3f);
					TRANSFORM_VECTOR(svec, svector3f);
					TRANSFORM_VECTOR(tvec, tvector3f);
				}

				// Last vertex needs to be done with scalars to avoid reading/writing 1 word past end of arrays
				{
					LOAD_MATRIX_SCALAR();
					TRANSFORM_POSITION_SCALAR(v, vertex3f);
					TRANSFORM_VECTOR_SCALAR(n, normal3f);
					TRANSFORM_VECTOR_SCALAR(svec, svector3f);
					TRANSFORM_VECTOR_SCALAR(tvec, tvector3f);
				}
				//printf("elapsed ticks: %llu\n", rdtsc() - ts); // XXX
				return;
			}

			for (i = 0;i < num_vertices_minus_one; i++, v += 3, n += 3, b++, vertex3f += 3, normal3f += 3)
			{
				LOAD_MATRIX4();
				TRANSFORM_POSITION(v, vertex3f);
				TRANSFORM_VECTOR(n, normal3f);
			}
			{
				LOAD_MATRIX_SCALAR();
				TRANSFORM_POSITION_SCALAR(v, vertex3f);
				TRANSFORM_VECTOR_SCALAR(n, normal3f);
			}
		}
		else
		{
			for (i = 0;i < num_vertices_minus_one; i++, v += 3, b++, vertex3f += 3)
			{
				LOAD_MATRIX4();
				TRANSFORM_POSITION(v, vertex3f);
			}
			{
				LOAD_MATRIX_SCALAR();
				TRANSFORM_POSITION_SCALAR(v, vertex3f);
			}
		}
	}

	else if (normal3f)
	{
		const float * RESTRICT n = model->surfmesh.data_normal3f;
		const unsigned short * RESTRICT b = model->surfmesh.blends;
		for (i = 0; i < num_vertices_minus_one; i++, n += 3, b++, normal3f += 3)
		{
			LOAD_MATRIX3();
			TRANSFORM_VECTOR(n, normal3f);
		}
		{
			LOAD_MATRIX_SCALAR();
			TRANSFORM_VECTOR_SCALAR(n, normal3f);
		}
	}

	if (svector3f)
	{
		const float * RESTRICT svec = model->surfmesh.data_svector3f;
		const unsigned short * RESTRICT b = model->surfmesh.blends;
		for (i = 0; i < num_vertices_minus_one; i++, svec += 3, b++, svector3f += 3)
		{
			LOAD_MATRIX3();
			TRANSFORM_VECTOR(svec, svector3f);
		}
		{
			LOAD_MATRIX_SCALAR();
			TRANSFORM_VECTOR_SCALAR(svec, svector3f);
		}
	}

	if (tvector3f)
	{
		const float * RESTRICT tvec = model->surfmesh.data_tvector3f;
		const unsigned short * RESTRICT b = model->surfmesh.blends;
		for (i = 0; i < num_vertices_minus_one; i++, tvec += 3, b++, tvector3f += 3)
		{
			LOAD_MATRIX3();
			TRANSFORM_VECTOR(tvec, tvector3f);
		}
		{
			LOAD_MATRIX_SCALAR();
			TRANSFORM_VECTOR_SCALAR(tvec, tvector3f);
		}
	}

#undef LOAD_MATRIX3
#undef LOAD_MATRIX4
#undef TRANSFORM_POSITION
#undef TRANSFORM_VECTOR
#undef LOAD_MATRIX_SCALAR
#undef TRANSFORM_POSITION_SCALAR
#undef TRANSFORM_VECTOR_SCALAR
}

#endif
