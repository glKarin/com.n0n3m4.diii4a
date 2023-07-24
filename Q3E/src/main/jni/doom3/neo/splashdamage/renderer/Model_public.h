// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __MODEL_PUBLIC_H__
#define __MODEL_PUBLIC_H__

struct silEdge_t {
	// NOTE: making this a glIndex is dubious, as there can be 2x the faces as verts
	glIndex_t					p1, p2;					// planes defining the edge
	glIndex_t					v1, v2;					// verts defining the edge
};

#if SD_SUPPORT_UNSMOOTHEDTANGENTS
// this is used for calculating unsmoothed normals and tangents for deformed models
typedef struct dominantTri_s {
	glIndex_t					v2, v3;
	float						normalizationScale[3];
} dominantTri_t;
#endif // SD_SUPPORT_UNSMOOTHEDTANGENTS

struct jointWeight_t {
	float					weight;					// joint weight
	int						jointMatOffset;			// offset in bytes to the joint matrix
	int						nextVertexOffset;		// offset in bytes to the first weight for the next vertex
};

// offsets for SIMD code
#define BASEVECTOR_SIZE							(4*4)		// sizeof( idVec4 )
#define JOINTWEIGHT_SIZE						(3*4)		// sizeof( jointWeight_t )
#define JOINTWEIGHT_WEIGHT_OFFSET				(0*4)		// offsetof( jointWeight_t, weight )
#define JOINTWEIGHT_JOINTMATOFFSET_OFFSET		(1*4)		// offsetof( jointWeight_t, jointMatOffset )
#define JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET		(2*4)		// offsetof( jointWeight_t, nextVertexOffset )

assert_sizeof( idVec4, BASEVECTOR_SIZE );
assert_sizeof( jointWeight_t, JOINTWEIGHT_SIZE );
assert_offsetof( jointWeight_t, weight, JOINTWEIGHT_WEIGHT_OFFSET );
assert_offsetof( jointWeight_t, jointMatOffset, JOINTWEIGHT_JOINTMATOFFSET_OFFSET );
assert_offsetof( jointWeight_t, nextVertexOffset, JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET );

const int MAX_WEIGHTS_PER_VERT = 4;					// skinning limitation
const int MAX_JOINTS_PER_MESH = 70;					// hardware skinning limitation

struct vertWeight_t {
	byte	index[MAX_WEIGHTS_PER_VERT];
	byte	weight[MAX_WEIGHTS_PER_VERT];
};

#endif // __MODEL_PUBLIC_H__
