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

// Copyright (C) 2010 Tels (Donated to The Dark Mod)

#ifndef __DARKMOD_MODELGENERATOR_H__
#define __DARKMOD_MODELGENERATOR_H__

/*
===============================================================================

  Model Generator - Generate/combine/scale models at run-time

  This class is a singleton and initiated/destroyed from gameLocal.

  At the moment it does not use any memory, but this might change later.

===============================================================================
*/

/* Tels: Define the number of supported LOD levels (0..LOD_LEVELS + hide)
 *		 About higher than 10 starts take performance instead of gaining it.
 *		 Do not go higher than 31, as each level needs 1 bit in a 32bit int.
 */
#define LOD_LEVELS 7


// Tels: If set to 2 << 20, it crashes on my system
#define MAX_MODEL_VERTS		(2 << 18)		// never combine more than this into one model
#define MAX_MODEL_INDEXES	(2 << 18)		// never combine more than this into one model

enum seed_model_flags {
	SEED_MODEL_NOSHADOW		= 0x0001,		// remove common/shadow surfaces
	SEED_MODEL_NOCLIP		= 0x0002,		// remove common/collision or tdm_collision_X surfaces
};

// Defines offset, rotation, vertex color etc. for a model combine operation
typedef struct {
	idVec3				offset;
	idVec3				scale;
	idAngles			angles;
	dword				color;	// packed color (including alpha)
	int					lod; 	// which LOD model stage to use?
	int					flags; 	// flags for each model, see seed_model_flags
} model_ofs_t;

typedef unsigned int lod_handle;

// When combining different models (e.g. different LOD stages), every model
// can have different source surfaces, that map to different target surfaces.
// For each of these target surfaces we need to track a few information and
// this struct holds them.

typedef struct {
	int					numVerts;
	int					numIndexes;
	modelSurface_s		surf;		// the actual new surface
} model_target_surf;

/**
* Tels: Info structure for LOD data, can be shared between many entities.
* The size of this struct is 192 bytes, which is neatly dividable by 16:
**/
struct lod_data_t
{
	/**
	* If true, the LOD distance check will only consider distance
	* orthogonal to gravity.  This can be useful for things like
	* turning on rainclouds high above the player.
	**/
	bool				bDistCheckXYOnly;
	
	/**
	* Interval between distance checks, in milliseconds.
	**/
	int					DistCheckInterval;
	
	/**
	* Distance squared beyond which the entity switches to LOD model/skin #1,#2,#3
	* if it is distance dependent
	* The last number is the distance squared beyond which the entity hides.
	**/
	float				DistLODSq[ LOD_LEVELS ];

	/**
	* Models and skins to be used for the different LOD distances
	* for level 0 we use "model" and "skin"
	**/
	idStr				ModelLOD[ LOD_LEVELS ];
	idStr				SkinLOD[ LOD_LEVELS ];

	/**
	* Different LOD models might need different offsets to match
	* the position of the LOD 0 level.
	*/
	idVec3				OffsetLOD[ LOD_LEVELS ];

	/** one bit for each LOD level, telling noshadows (1) or not (0) */
	int					noshadowsLOD;

	/**
	* Fade out and fade in range in D3 units.
	**/
	float				fLODFadeOutRange;
	float				fLODFadeInRange;

	/**
	* If the mapper sets a minimum distance, respect this even when the
	* menu would try a lower dist. E.g. "lod_normal_distance" "700" means
	* the entity ignores a lod_bias under 1.0f if it is closer than 700
	* units. E.g. a "hide_distance" of 800 units will with lod_bias 0.5
	* not cause the entity to disappear at 400 units, but only from 700
	* units onwards.
	*/
	float				fLODNormalDistance;
};

/** Describes one entry in the global lod_data_t list, and tracks how
*	many entities use this data. If the number of users drops to zero,
*	the data can be freed.
*/
struct lod_entry_t
{
	int					users;		//!< Number of users of this data
	lod_data_t			*LODPtr;	//!< The actual data
};

/** Describes a model stage used at a specific origin. Used internally to figure
*	out which surface to skip, or where to append it.
*/
struct model_stage_info_t {
	bool						couldCastShadow;	// if false - no shadow casting surfaces at all
	unsigned int				usedShadowless;		// how often is this model used without shadows (or doesn't have shadow casting surfaces)
	unsigned int				usedShadowing;		// how often is this model used with shadow casting on?
	const idRenderModel* 		source;				// the actual render model to use

	// these lists give for each source surface the target (or -1 for skip)
	idList< int > noshadowSurfaces;					// in case used shadowless
	idList< int > shadowSurfaces;					// The same as noshadowSurfaces, but in case of actual shadow casting

	// this list gives for each source surface on the model whether this is a backside or a pure shadow caster
	// * backsides are automatically created flipped copies of two-sided materials
	// * pure shadow casters have no diffuse, just a shadow
	idList< int > surface_info;						// 0 => no pure shadow caster and no backside
													// 1 => no pure shadow caster, but is backside
													// 2 => pure shadow caster, and not a backside
													// 3 => pure shadow caster and a backside, too (?)
};

class CModelGenerator {
public:
	//CLASS_PROTOTYPE( CModelGenerator );

						CModelGenerator( void );

						~CModelGenerator();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	/**
	* Called by gameLocal.
	*/
	void				Init ( void );
	void				Clear ( void );

	/** Given a rendermodel and a surface index, checks if that surface is two-sided,
	*   and if, tries to find the bakside for this surface, e.g. the surface which
	*	was copied and flipped. Returns either the surface index, or -1 for
	*	"not twosided or not found":
	*/
	int					GetBacksideForSurface( const idRenderModel * source, const int surfaceIdx ) const;

	/** Returns true if the model has at least one surface that casts a shadow */
	bool				ModelHasShadow( const idRenderModel * source ) const;
	
	/**
	* Given a pointer to a render model, calls AllocModel() on the rendermanager, then
	* copies all surface data from the old model to the new model. Used to construct a
	* copy of an existing model, so it can then be used as blue-print for other models,
	* which will share the same data. If dupData is true, memory for verts and indexes
	* is duplicated, otherwise the new model shares the data of the old model. In this
	* case the memory of the new model needs to be freed differently, of course :)
	* If shader is != NULL, all shaders of the models will be switched to this shader.
	*/
	idRenderModel*			DuplicateLODModels( const idList<const idRenderModel*> *LODs, const char* snapshotName,
												const idList<model_ofs_t>* offsets, const idVec3 *origin = NULL,
												const idMaterial *shader = NULL, idRenderModel* hModel = NULL) const;

	/**
	* Copies the surfaces of the source model to a new model. If noshadow = true, will
	* try to eliminate shadow casting surfaces and also not build a shadow hull. If
	* target is NULL, a new model will be allocated. Returns target or the newly
	* allocated model.
	*/
	idRenderModel*			DuplicateModel( const idRenderModel* source, const char* snapshotName, idRenderModel* target = NULL, const idVec3 *scale = NULL, const bool noshadow = false) const;

	/**
	* Returns the maximum number of models that can be combined from this model:
	*/
	unsigned int			GetMaxModelCount( const idRenderModel* hModel ) const;

	/**
	* Given the info CombineModels(), sep. the given model out again.
	*/
	//void					RemoveModel( const idRenderModel *source, const model_combineinfo_t *info);

	/**
	* Presents the ModelManager with the LOD data for this entity, and returns
	* a handle (>= 0) with that the entity can later access this data.
	*/
	lod_handle				RegisterLODData( const lod_data_t *mLOD );

	/**
	* Asks the ModelManager to register a new user for this handle. Returns
	* the same handle, or 0 in case of errors.
	*/
	lod_handle				RegisterLODData( const lod_handle h );

	/**
	* Unregister the LOD data for this handle (returned by RegisterLODData).
	* Returns true for success, and false in case of errors.
	*/
	bool					UnregisterLODData( const lod_handle h );

	/**
	* Get a pointer to the LOD data for this handle.
	*/
	const lod_data_t*		GetLODDataPtr( const lod_handle h ) const;

	/**
	* Print memory usage info.
	*/
	void					Print( void ) const;

private:
	// Called by the destructor
	void					Shutdown();

	void					SaveLOD( idSaveGame *savefile, const lod_data_t * m_LOD ) const;
	void					RestoreLOD( idRestoreGame *savefile, lod_data_t * m_LOD );
	bool					CompareLODData( const lod_data_t *mLOD, const lod_data_t *mLOD2 ) const;

	// used to identify textures that are pure shadow casting
	idStr					m_shadowTexturePrefix;

	/**
	* A list with (possible shared) LOD data.
	*/
	idList<lod_entry_t>		m_LODList;
};

#endif /* !__DARKMOD_MODELGENERATOR_H__ */

