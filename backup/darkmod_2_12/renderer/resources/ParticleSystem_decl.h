/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
//no include guard: include ParticleSystem.h instead

//---------------------------------------------------------------------

float idRandom_RandomFloat(PINOUT(int) seed);
float idRandom_CRandomFloat(PINOUT(int) seed);


struct idParticleParm {
	const idDeclTable *		table;	//TODO: remove?
	float					from;
	float					to;
};
void idParticleParm_Clear(POUT(idParticleParm) self);
float idParticleParm_Eval(PIN(idParticleParm) self, float frac);
float idParticleParm_Integrate(PIN(idParticleParm) self, float frac);

//---------------------------------------------------------------------

//TODO: turn enums into integer constants for GLSL?

typedef enum {
	PDIST_RECT,				// ( sizeX sizeY sizeZ )
	PDIST_CYLINDER,			// ( sizeX sizeY sizeZ ringFraction)
	PDIST_SPHERE			// ( sizeX sizeY sizeZ ringFraction )
							// a ringFraction of zero allows the entire sphere, 0.9 would only
							// allow the outer 10% of the sphere
} prtDistribution_t;

typedef enum {
	PDIR_CONE,				// parm0 is the solid cone angle
	PDIR_OUTWARD			// direction is relative to offset from origin, parm0 is an upward bias
} prtDirection_t;

typedef enum {
	PPATH_STANDARD,
	PPATH_HELIX,			// ( sizeX sizeY sizeZ radialSpeed climbSpeed )
	PPATH_FLIES,
	PPATH_ORBIT,
	PPATH_DRIP
} prtCustomPth_t;

typedef enum {
	POR_VIEW,
	POR_AIMED,				// angle and aspect are disregarded
	POR_X,
	POR_Y,
	POR_Z
} prtOrientation_t;

typedef enum {
	PML_LINEAR,
	PML_TEXTURE
} prtMapLayout_t;

//---------------------------------------------------------------------

//particle system: per-declstage data
//this data is embedded into idParticleStage class
//stays constant across time and entities with same decl
struct idPartStageData {

	int totalParticles;				// total number of particles, although some may be invisible at a given time
	float cycles;					// allows things to oneShot ( 1 cycle ) or run for a set number of cycles (on per stage basis)
	int diversityPeriod;			// stgatilov: affects how random seed depends on cycle number:
									//    if K > 0, the every i-th cycle looks exactly the same as (i+K)-th cycle
									//    if K <= 0 (infinite), then every cycle looks unique

	int cycleMsec;					// ( particleLife + deadTime ) in msec

	float spawnBunching;			// 0.0 = all come out at first instant, 1.0 = evenly spaced over cycle time
	float particleLife;				// total seconds of life for each particle
	float timeOffset;				// time offset from system start for the first particle to spawn
	float deadTime;					// time after particleLife before respawning

	//------ standard path parms

	prtDistribution_t distributionType;
	float distributionParms[4];

	prtDirection_t directionType;
	float directionParms[4];

	idParticleParm speed;
	float gravity;					// can be negative to float up
	bool worldGravity;				// apply gravity in world space
	bool worldAxis;					// apply offset in world space -- SteveL #3950
	bool randomDistribution;		// randomly orient the quad on emission ( defaults to true ) 
	bool entityColor;				// force color from render entity ( fadeColor is still valid )

	//------ custom path will completely replace the standard path calculations

	prtCustomPth_t customPathType;	// use custom C code routines for determining the origin
	float customPathParms[8];

	//------

	idVec3 offset;					// offset from origin to spawn all particles, also applies to customPath

	int animationFrames;			// if > 1, subdivide the texture S axis into frames and crossfade
	float animationRate;			// frames per second

	float initialAngle;				// in degrees, random angle is used if zero ( default ) 
	idParticleParm rotationSpeed;	// half the particles will have negative rotation speeds

	prtOrientation_t orientation;	// view, aimed, or axis fixed
	float orientationParms[4];

	idParticleParm size;
	idParticleParm aspect;			// greater than 1 makes the T axis longer

	idVec4 color;
	idVec4 fadeColor;				// either 0 0 0 0 for additive, or 1 1 1 0 for blended materials
	float fadeInFraction;			// in 0.0 to 1.0 range
	float fadeOutFraction;			// in 0.0 to 1.0 range
	float fadeIndexFraction;		// in 0.0 to 1.0 range, causes later index smokes to be more faded 

	float boundsExpansion;			// user tweak to fix poorly calculated bounds

	//------

	bool useCutoffTimeMap;
	//stgatilov: if set to true, then cutoffTimeMap is auto-generated when map is compiled
	bool collisionStatic;
	bool collisionStaticWorldOnly;
	//stgatilov: configuration of cutoff texture (and possibly other textures added in future)
	prtMapLayout_t mapLayoutType;
	int mapLayoutSizes[2];
	int collisionStaticTimeSteps;
};

//particle system: per-system modeling data
//if many entities use same particle declaration, each has its own data
struct idPartSysData {
	idMat3 entityAxis;				// (renderEnt->axis) entity model space -> world space
	idMat3 viewAxis;				// (renderView->viewaxis) view space -> world space (see R_SetViewMatrix)
	idVec4 entityParmsColor;		// (renderEnt->shaderParms[0-3]) specify entity color
	int totalParticles;				//stgatilov #5130: fixes R_ParticleDeform with useArea = true and fadeIndex
};

//particle system: per-particle individual modeling data
struct idParticleData {
	int index;				// particle number in the system
	float frac;				// 0.0 to 1.0
	int randomSeed;			// initial seed for idRandom
	idVec3 origin;			// dynamic smoke particles can have individual origins and axis
	idMat3 axis;			// particle emitter space -> entity model space (identity for particle models)
};


//computes particle origin in "standard" coordinate system --- specified transformation and world gravity not applied yet
//this function is needed to compute bounding box of particle-deform effect
idVec3 idParticle_ParticleOriginStdSys(PIN(idPartStageData) stg, PIN(idParticleData) part, PINOUT(int) random);
//computes particle origin in entity model space --- all transformations applied
//fo get final world-space position, just multiply on entityAxis
idVec3 idParticle_ParticleOrigin(PIN(idPartStageData) stg, PIN(idPartSysData) psys, PIN(idParticleData) part, PINOUT(int) random);

//generates fixed number of quads for one specified particle
//for each quad, vertex order is:
//  0 1
//  2 3
void idParticle_CreateParticle(PIN(idPartStageData) stg, PIN(idPartSysData) psys, PIN(idParticleData) part, EMITTER emitter);

//---------------------------------------------------------------------

//particle system: per-system emit/generation info
//if many entities use same particle declaration, each has its own data
struct idPartSysEmit {
	int viewTimeMs;
	float entityParmsTimeOffset;
	float entityParmsStopTime;
	int totalParticles;
	float randomizer;
};

//emits one particle with specified index and default emit location
//custom "origin" and "axis" should be assigned immediately afterwards if needed
//the result can be passed into idParticle_CreateParticle to compute characteristics and produce geometry
//"cycleNumber" is the number of cycle particle belongs to (modulo diversityPeriod if set)
bool idParticle_EmitParticle(
	PIN(idPartStageData) stg, PIN(idPartSysEmit) psys, PIN(int) index,
	POUT(idParticleData) res, POUT(int) cycleNumber
);

//get random seed of a particle
//note: better use idParticle_EmitParticle instead
//this function is exposed for runRarticle preprocessing tool (collisions with mapLayout linear)
int idParticle_GetRandomSeed(PIN(int) index, PIN(int) cycleNumber, PIN(float) randomizer);

//---------------------------------------------------------------------

//precomputed auxilliary information about cutoff map with "texture" layout
//defines which subregion in texcoord space is saved in the image
struct idPartSysCutoffTextureInfo {
	//resolution of virtual texture
	//texcoords [0..1) x [0..1) map to virtual texel grid [0..resX) x [0..resY)
	int resX, resY;
	//(baseX + x, baseY + y) texel in virtual texture is stored in texel (x, y) of physical image
	int baseX, baseY;
	//physical image has dimensions sizeX x sizeY
	int sizeX, sizeY;
};
