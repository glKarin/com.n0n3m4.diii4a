// Copyright (C) 2007 Id Software, Inc.
//

#pragma once
#ifndef _BSE_SPAWN_DOMAINS_H_INC_
#define _BSE_SPAWN_DOMAINS_H_INC_

enum
{
	SPF_NONE_0 = 0,
	SPF_NONE_1,
	SPF_NONE_2,
	SPF_NONE_3,

	SPF_ONE_0,
	SPF_ONE_1,
	SPF_ONE_2,
	SPF_ONE_3,

	SPF_POINT_0,
	SPF_POINT_1,
	SPF_POINT_2,
	SPF_POINT_3,

	SPF_LINEAR_0,
	SPF_LINEAR_1,
	SPF_LINEAR_2,
	SPF_LINEAR_3,

	SPF_BOX_0,
	SPF_BOX_1,
	SPF_BOX_2,
	SPF_BOX_3,

	SPF_SURFACE_BOX_0,
	SPF_SURFACE_BOX_1,
	SPF_SURFACE_BOX_2,
	SPF_SURFACE_BOX_3,

	SPF_SPHERE_0,
	SPF_SPHERE_1,
	SPF_SPHERE_2,
	SPF_SPHERE_3,	

	SPF_SURFACE_SPHERE_0,
	SPF_SURFACE_SPHERE_1,
	SPF_SURFACE_SPHERE_2,
	SPF_SURFACE_SPHERE_3,

 	SPF_CYLINDER_0,
 	SPF_CYLINDER_1,
 	SPF_CYLINDER_2,
 	SPF_CYLINDER_3,
 
 	SPF_SURFACE_CYLINDER_0,
 	SPF_SURFACE_CYLINDER_1,
 	SPF_SURFACE_CYLINDER_2,
 	SPF_SURFACE_CYLINDER_3,
 
	SPF_SPIRAL_0,
	SPF_SPIRAL_1,
	SPF_SPIRAL_2,
	SPF_SPIRAL_3,	

	SPF_MODEL_0,
	SPF_MODEL_1,
	SPF_MODEL_2,
	SPF_MODEL_3,

	SPF_COUNT
};
   
typedef void ( *TSpawnFunc )( float *, const class rvParticleParms &, idVec3 *, const idVec3 * );

#define	PPFLAG_SURFACE			( 1 << 0 )
#define	PPFLAG_USEENDORIGIN		( 1 << 1 )
#define	PPFLAG_CONE				( 1 << 2 )
#define	PPFLAG_RELATIVE			( 1 << 3 )
#define	PPFLAG_LINEARSPACING	( 1 << 4 )
#define	PPFLAG_ATTENUATE		( 1 << 5 )
#define	PPFLAG_INV_ATTENUATE	( 1 << 6 )

class idRenderModel;

// Parameters that define how a particle spawns. These are generic to all fields
struct sdModelInfo {
	static const int NUM_SURF_REMAP = 10;
	idRenderModel *model;
	int surfRemap[NUM_SURF_REMAP];	//Will have a nuber of elements set to a certain surface's index based on the number of triangles in that surface
									// so surfaces with only like a few triangles don't have very dense particles generated for them.

	void CalculateSurfRemap( void );
};

extern const char sdPoolAllocator_rvParticleParms[];
class rvParticleParms //: public sdPoolAllocator< rvParticleParms, sdPoolAllocator_rvParticleParms, 128 >
{
	friend class rvParticle;
	friend class rvLineParticle;
	friend void SpawnStub( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

	friend void SpawnNone1( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnNone2( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnNone3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

	friend void SpawnOne1( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnOne2( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnOne3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

	friend void SpawnPoint1( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnPoint2( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnPoint3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

	friend void SpawnLinear1( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnLinear2( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnLinear3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

	friend void SpawnBox1( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnBox2( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnBox3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

	friend void SpawnSurfaceBox1( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnSurfaceBox2( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnSurfaceBox3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

	friend void SpawnSphere1( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnSphere2( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnSphere3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

	friend void SpawnSurfaceSphere1( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnSurfaceSphere2( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnSurfaceSphere3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

	friend void SpawnCylinder3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnSurfaceCylinder3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

	friend void SpawnSpiral2( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );
	friend void SpawnSpiral3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

	friend void SpawnModel3( float *result, const rvParticleParms &parms, idVec3 *normal, const idVec3 * );

public:
				rvParticleParms( void ) { mModelInfo = NULL; mStatic = 0;}
				~rvParticleParms( void ) { delete mModelInfo; }
				
	void operator= ( const rvParticleParms &other ) {
		memcpy( this, &other, sizeof( rvParticleParms ) );
		if ( other.mModelInfo ) {
			mModelInfo = new sdModelInfo;
			*mModelInfo = *other.mModelInfo;
		}
		mStatic = 0;
	}

	bool		operator== ( const rvParticleParms &comp ) const { return( Compare( comp ) ); }
	bool		operator!= ( const rvParticleParms &comp ) const { return( !Compare( comp ) ); }

	void		Init( int spawnType = SPF_NONE_0 ) { mSpawnType = spawnType; mFlags = 0; mRange = 0.0f; mModelInfo = NULL; mMins.Zero(); mMaxs.Zero(); }
	ID_INLINE void		Spawn( float *dest, const rvParticleParms &parms, idVec3 *normal, const idVec3 *centre ) { ( *spawnFunctions[mSpawnType] )( dest, parms, normal, centre ); }
	ID_INLINE void		Spawn( float *dest, const rvParticleParms &parms ) { ( *spawnFunctions[mSpawnType] )( dest, parms, NULL, NULL ); }
	void		HandleRelativeParms( float *death, float *init, int count );
	void		GetMinsMaxs( idVec3 &mins, idVec3 &maxs );

	ID_INLINE int GetFlags( void ) const { return mFlags; }
	ID_INLINE const idVec3 & GetMins( void ) const { return mMins; }
	ID_INLINE const idVec3 & GetMaxs( void ) const { return mMaxs; }

//private:
	byte		mSpawnType;				// Type of spawn domain (eg. box, spiral)
	byte		mFlags;					// PPFLAG_
	byte		mStatic;
	byte		mPad;
	float		mRange;					// Repeat length of spiral
	sdModelInfo		*mModelInfo;					// idRenderModel *
	idVec3		mMins;					// Box mins, line start etc
	idVec3		mMaxs;					// Box maxs, line end etc

private:
	bool		Compare( const rvParticleParms &comp ) const;

	static		TSpawnFunc			spawnFunctions[SPF_COUNT];
};

void SpawnStub( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

void SpawnNone1( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnNone2( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnNone3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

void SpawnOne1( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnOne2( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnOne3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

void SpawnPoint1( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnPoint2( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnPoint3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

void SpawnLinear1( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnLinear2( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnLinear3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

void SpawnBox1( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnBox2( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnBox3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

void SpawnSurfaceBox1( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnSurfaceBox2( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnSurfaceBox3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

void SpawnSphere1( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnSphere2( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnSphere3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

void SpawnSurfaceSphere1( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnSurfaceSphere2( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnSurfaceSphere3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

void SpawnCylinder3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnSurfaceCylinder3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

void SpawnSpiral2( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );
void SpawnSpiral3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

void SpawnModel3( float *result, const rvParticleParms &parms, idVec3 *normal = NULL, const idVec3 * = NULL );

#endif // _BSE_SPAWN_DOMAINS_H_INC_
