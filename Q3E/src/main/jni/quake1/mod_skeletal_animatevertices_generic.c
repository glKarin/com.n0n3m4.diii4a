#include "mod_skeletal_animatevertices_generic.h"

void Mod_Skeletal_AnimateVertices_Generic(const model_t * RESTRICT model, const frameblend_t * RESTRICT frameblend, const skeleton_t *skeleton, float * RESTRICT vertex3f, float * RESTRICT normal3f, float * RESTRICT svector3f, float * RESTRICT tvector3f)
{
	// vertex weighted skeletal
	int i, k;
	float *bonepose;
	float *boneposerelative;
	const blendweights_t * RESTRICT weights;

	//unsigned long long ts = rdtsc();
	bonepose = (float *) Mod_Skeletal_AnimateVertices_AllocBuffers(sizeof(float[12]) * (model->num_bones*2 + model->surfmesh.num_blends));
	boneposerelative = bonepose + model->num_bones * 12;

	Mod_Skeletal_BuildTransforms(model, frameblend, skeleton, bonepose, boneposerelative);

	// generate matrices for all blend combinations
	weights = model->surfmesh.data_blendweights;
	for (i = 0;i < model->surfmesh.num_blends;i++, weights++)
	{
		float * RESTRICT b = boneposerelative + 12 * (model->num_bones + i);
		const float * RESTRICT m = boneposerelative + 12 * (unsigned int)weights->index[0];
		float f = weights->influence[0] * (1.0f / 255.0f);
		b[ 0] = f*m[ 0]; b[ 1] = f*m[ 1]; b[ 2] = f*m[ 2]; b[ 3] = f*m[ 3];
		b[ 4] = f*m[ 4]; b[ 5] = f*m[ 5]; b[ 6] = f*m[ 6]; b[ 7] = f*m[ 7];
		b[ 8] = f*m[ 8]; b[ 9] = f*m[ 9]; b[10] = f*m[10]; b[11] = f*m[11];
		for (k = 1;k < 4 && weights->influence[k];k++)
		{
			m = boneposerelative + 12 * (unsigned int)weights->index[k];
			f = weights->influence[k] * (1.0f / 255.0f);
			b[ 0] += f*m[ 0]; b[ 1] += f*m[ 1]; b[ 2] += f*m[ 2]; b[ 3] += f*m[ 3];
			b[ 4] += f*m[ 4]; b[ 5] += f*m[ 5]; b[ 6] += f*m[ 6]; b[ 7] += f*m[ 7];
			b[ 8] += f*m[ 8]; b[ 9] += f*m[ 9]; b[10] += f*m[10]; b[11] += f*m[11];
		}
	}

#define LOAD_MATRIX_SCALAR() const float * RESTRICT m = boneposerelative + 12 * (unsigned int)*b

#define LOAD_MATRIX3() \
	LOAD_MATRIX_SCALAR()
#define LOAD_MATRIX4() \
	LOAD_MATRIX_SCALAR()

#define TRANSFORM_POSITION_SCALAR(in, out) \
	(out)[0] = ((in)[0] * m[0] + (in)[1] * m[1] + (in)[2] * m[ 2] + m[3]); \
	(out)[1] = ((in)[0] * m[4] + (in)[1] * m[5] + (in)[2] * m[ 6] + m[7]); \
	(out)[2] = ((in)[0] * m[8] + (in)[1] * m[9] + (in)[2] * m[10] + m[11]);
#define TRANSFORM_VECTOR_SCALAR(in, out) \
	(out)[0] = ((in)[0] * m[0] + (in)[1] * m[1] + (in)[2] * m[ 2]); \
	(out)[1] = ((in)[0] * m[4] + (in)[1] * m[5] + (in)[2] * m[ 6]); \
	(out)[2] = ((in)[0] * m[8] + (in)[1] * m[9] + (in)[2] * m[10]);

#define TRANSFORM_POSITION(in, out) \
	TRANSFORM_POSITION_SCALAR(in, out)
#define TRANSFORM_VECTOR(in, out) \
	TRANSFORM_VECTOR_SCALAR(in, out)

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
				for (i = 0; i < model->surfmesh.num_vertices; i++, v += 3, n += 3, svec += 3, tvec += 3, b++,
						vertex3f += 3, normal3f += 3, svector3f += 3, tvector3f += 3)
				{
					LOAD_MATRIX4();
					TRANSFORM_POSITION(v, vertex3f);
					TRANSFORM_VECTOR(n, normal3f);
					TRANSFORM_VECTOR(svec, svector3f);
					TRANSFORM_VECTOR(tvec, tvector3f);
				}

				return;
			}

			for (i = 0;i < model->surfmesh.num_vertices; i++, v += 3, n += 3, b++, vertex3f += 3, normal3f += 3)
			{
				LOAD_MATRIX4();
				TRANSFORM_POSITION(v, vertex3f);
				TRANSFORM_VECTOR(n, normal3f);
			}
		}
		else
		{
			for (i = 0;i < model->surfmesh.num_vertices; i++, v += 3, b++, vertex3f += 3)
			{
				LOAD_MATRIX4();
				TRANSFORM_POSITION(v, vertex3f);
			}
		}
	}

	else if (normal3f)
	{
		const float * RESTRICT n = model->surfmesh.data_normal3f;
		const unsigned short * RESTRICT b = model->surfmesh.blends;
		for (i = 0; i < model->surfmesh.num_vertices; i++, n += 3, b++, normal3f += 3)
		{
			LOAD_MATRIX3();
			TRANSFORM_VECTOR(n, normal3f);
		}
	}

	if (svector3f)
	{
		const float * RESTRICT svec = model->surfmesh.data_svector3f;
		const unsigned short * RESTRICT b = model->surfmesh.blends;
		for (i = 0; i < model->surfmesh.num_vertices; i++, svec += 3, b++, svector3f += 3)
		{
			LOAD_MATRIX3();
			TRANSFORM_VECTOR(svec, svector3f);
		}
	}

	if (tvector3f)
	{
		const float * RESTRICT tvec = model->surfmesh.data_tvector3f;
		const unsigned short * RESTRICT b = model->surfmesh.blends;
		for (i = 0; i < model->surfmesh.num_vertices; i++, tvec += 3, b++, tvector3f += 3)
		{
			LOAD_MATRIX3();
			TRANSFORM_VECTOR(tvec, tvector3f);
		}
	}
}
