// copy from framework/DeclParticle.h

#ifndef __BSE_DECLPARTICLE_H__
#define __BSE_DECLPARTICLE_H__

/*
===============================================================================

	rvBSEParticle

===============================================================================
*/

/*typedef*/ enum {
	PDIST_POINT = 3, // PDIST_SPHERE + 1
};

/*typedef*/ enum {
	POR_DIR = 5, // POR_Z + 1
};

class idRenderModel;

class rvBSEParticleParm
{
	public:
		rvBSEParticleParm(void) {
			table = NULL;
			from = to = 0.0f;
		}

		const idDeclTable 		*table;
		float					from;
		float					to;

		float					Eval(float frac, idRandom &rand) const;
		float					Integrate(float frac, idRandom &rand) const;
};


typedef struct renderEntity_s renderEntity_t;
typedef struct renderView_s renderView_t;

typedef struct {
	const renderEntity_t 	*renderEnt;			// for shaderParms, etc
	const renderView_t 	*renderView;
	int						index;				// particle number in the system
	float					frac;				// 0.0 to 1.0
	idRandom				random;
	idVec3					origin;				// dynamic smoke particles can have individual origins and axis
	idMat3					axis;


	float					age;				// in seconds, calculated as fraction * stage->particleLife
	idRandom				originalRandom;		// needed so aimed particles can reset the random for another origin calculation
	float					animationFrameFrac;	// set by ParticleTexCoords, used to make the cross faded version
} rvBSE_particleGen_t;


//
// single particle stage
//
class rvBSEParticleStage
{
	public:
		rvBSEParticleStage(void);
		virtual					~rvBSEParticleStage(void) {}

		void					Default();
		virtual int				NumQuadsPerParticle() const;	// includes trails and cross faded animations
		// returns the number of verts created, which will range from 0 to 4*NumQuadsPerParticle()
		virtual int				CreateParticle(rvBSE_particleGen_t *g, idDrawVert *verts) const;

		void					ParticleOrigin(rvBSE_particleGen_t *g, idVec3 &origin) const;
		int						ParticleVerts(rvBSE_particleGen_t *g, const idVec3 origin, idDrawVert *verts) const;
		void					ParticleTexCoords(rvBSE_particleGen_t *g, idDrawVert *verts) const;
		void					ParticleColors(rvBSE_particleGen_t *g, idDrawVert *verts) const;

		const char 			*GetCustomPathName();
		const char 			*GetCustomPathDesc();
		int						NumCustomPathParms();
		void					SetCustomPathType(const char *p);
		void					operator=(const rvBSEParticleStage &src);


		//------------------------------

		const idMaterial 		*material;

		int						totalParticles;		// total number of particles, although some may be invisible at a given time
		float					cycles;				// allows things to oneShot ( 1 cycle ) or run for a set number of cycles
		// on a per stage basis

		int						cycleMsec;			// ( particleLife + deadTime ) in msec

		float					spawnBunching;		// 0.0 = all come out at first instant, 1.0 = evenly spaced over cycle time
		float					particleLife;		// total seconds of life for each particle
		float					timeOffset;			// time offset from system start for the first particle to spawn
		float					deadTime;			// time after particleLife before respawning

		//-------------------------------	// standard path parms

		/*prtDistribution_t*/int		distributionType;
		float					distributionParms[4];

		/*prtDirection_t*/int			directionType;
		float					directionParms[4];

		rvBSEParticleParm			speed;
		float					gravity;				// can be negative to float up
		bool					worldGravity;			// apply gravity in world space
		bool					randomDistribution;		// randomly orient the quad on emission ( defaults to true )
		bool					entityColor;			// force color from render entity ( fadeColor is still valid )

		//------------------------------	// custom path will completely replace the standard path calculations

		/*prtCustomPth_t*/int			customPathType;		// use custom C code routines for determining the origin
		float					customPathParms[8];

		//--------------------------------

		idVec3					offset;				// offset from origin to spawn all particles, also applies to customPath

		int						animationFrames;	// if > 1, subdivide the texture S axis into frames and crossfade
		float					animationRate;		// frames per second

		float					initialAngle;		// in degrees, random angle is used if zero ( default )
		rvBSEParticleParm			rotationSpeed;		// half the particles will have negative rotation speeds

		/*prtOrientation_t*/int		orientation;	// view, aimed, or axis fixed
		float					orientationParms[4];

		rvBSEParticleParm			size;
		rvBSEParticleParm			aspect;				// greater than 1 makes the T axis longer

		idVec4					color;
		idVec4					fadeColor;			// either 0 0 0 0 for additive, or 1 1 1 0 for blended materials
		float					fadeInFraction;		// in 0.0 to 1.0 range
		float					fadeOutFraction;	// in 0.0 to 1.0 range
		float					fadeIndexFraction;	// in 0.0 to 1.0 range, causes later index smokes to be more faded

		bool					hidden;				// for editor use
		//-----------------------------------

		float					boundsExpansion;	// user tweak to fix poorly calculated bounds

		idBounds				bounds;				// derived

		int rvptype;
		idRenderModel	*model;
};


//
// group of particle stages
//
class rvBSEParticle : public idDecl
{
	public:

		virtual size_t			Size(void) const;
		virtual const char 	*DefaultDefinition(void) const;
		virtual bool			Parse(const char *text, const int textLength, bool noCaching = false);
		virtual void			FreeData(void);

		bool					Save(const char *fileName = NULL);

		idList<rvBSEParticleStage *>stages;
		idBounds				bounds;
		float					depthHack;

	private:
		bool					RebuildTextSource(void);
	public:
		void					GetStageBounds(rvBSEParticleStage *stage);
	private:
		rvBSEParticleStage 		*ParseParticleStage(idLexer &src);
		void					ParseParms(idLexer &src, float *parms, int maxParms);
		void					ParseParametric(idLexer &src, rvBSEParticleParm *parm);
	public:
		void					WriteStage(idFile *f, rvBSEParticleStage *stage);
	private:
		void					WriteParticleParm(idFile *f, rvBSEParticleParm *parm, const char *name);
};

#endif /* !__BSE_DECLPARTICLE_H__ */

