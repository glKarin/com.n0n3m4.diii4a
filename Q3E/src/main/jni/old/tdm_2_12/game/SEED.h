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

// Copyright (C) 2010-2011 Tels (Donated to The Dark Mod)

#ifndef __GAME_SEED_H__
#define __GAME_SEED_H__

/*===============================================================================

  System for Environmental Entity Distribution (SEED, formerly known as LODE)
  
  Automatically creates/culls entities based on distance from player.

  Can spawn entities based on templates in random places, or manage already
  existing entities. The rules where to spawn what are quite flexible, either
  random, with distribution functions, based on underlying texture etc.

  Inhibitors can also be placed, these inhibit spawning of either all entities
  from one class or all entitites.

  Can also combine spawned entities into "megamodels", where the model consists
  of all the surfaces of the combined models, plus a physics model with multiple
  clipmodels, to reduce entity count and number of drawcalls. These combined
  entities are of the CStaticMulti class.

  ============================================================================= */

#include "Game_local.h"
#include "StaticMulti.h"
#include "Objectives/MissionData.h"
#include "Func_Shooter.h"
#include "Emitter.h"
#include "../idlib/containers/List.h"

#define SEED_DEBUG_MATERIAL_COUNT 13
/** List of debug materials to use for the SEED megamodels */
static const char* seed_debug_materials[SEED_DEBUG_MATERIAL_COUNT] = {
	"debug_red",
	"debug_blue",
	"debug_green",
	"debug_dark_green",
	"debug_pale_green",
	"debug_yellow",
	"debug_purple",
	"debug_cyan",
	"debug_dark_blue",
	"debug_pale_blue",
	"debug_dark_red",
	"debug_orange",
	"debug_brown",
};

/** To sort a list of offsets by distance, but still keep the info which offset
    belongs to which entity so we can take the N nearest: */
struct seed_sort_ofs_t {
	model_ofs_t	ofs;					//!< the offset data
	int			entity;					//!< Index into m_Entities
};

/** Defines one material class that modulates how often entities appear on it: */
struct seed_material_t {
	idStr					name;			//!< a part, like "grass", or the full name like "sand_dark"
	float					probability;	//!< 0 .. 1.0
};

/* Defines one entity class for the SEED system */
struct seed_class_t {
	idStr					classname;		//!< Entity class to respawn entities
	idList< int >			skins;			//!< index into skins array
	idRenderModel*			hModel;			//!< When you turn a brush inside DR into a idStaticEntity and use it as template,
   											//!< this is used to keep their renderModel (as it is not loadable by name).
	idClipModel*			clip;			//!< If the rendermodel is not a named model, but instead static geometry from brushes/patches,
											//!< we need to store this, so we can use it later even after freeing the original target entity.
	idStr					modelname;		//!< To load the rendermodel for combining it w/o spawning
											//!< the entity first. Used to calculate f.i. how many models
											//!< can be combined at most (as this model is the high-poly version).
	idStr					lowestLOD;		//!< Name of the model with the lowest LOD, used as clipmodel

	bool					pseudo;			//!< if true, this class is a pseudo-class, and describes an
											//!< entity with a megamodel (a combined model from many entities),
											//!< the model is still stored in hModel.
											//!< These classes will be skipped when recreating the entities.

	bool					noshadows;		//!< entities of this class do not have a shadow, even if the model has one
	bool					watch;			//!< if true, this class is just used to watch over a certain entity
	idStr					combine_as;		//!< If watch is true, this is the value from "seed_combine_as"

	idStr					materialName;	//!< Override material for debug_colors.
	idList< model_ofs_t >	offsets;		//!< if pseudo: List of enitity offsets to construct a combined model

	int						seed;			//!< per-class seed so each class generates the same sequence of
											//!< entities independ from the other classes, helps when the menu
   											//!> setting changes
	int						maxEntities;	//!< to find out how many entities. If != 0, this is the maximum count.
	int						numEntities;	//!< Either maxEntities, or calculated from density
	idVec3					origin;			//!< origin of the original target entity, useful for "flooring"
	idVec3					offset;			//!< offset to displace the final entity, used to correct for models with their
											//!< origin not at the base
	float					cullDist;		//!< distance after where we remove the entity from the world
	float					spawnDist;		//!< distance where we respawn the entity
	float					spacing;		//!< min. distance between entities of this class
	float					bunching;		//!< bunching threshold (0 - none, 1.0 - all)
	float					sink_min;		//!< sink into floor at minimum
	float					sink_max;		//!< sink into floor at maximum
	bool					floor;			//!< if true, the entities will be floored (on by default, use
											//!< "seed_floor" "0" to disable, then entities will be positioned
											//!< at "z" where the are in the editor
	bool					floating;		//!< if true, entities that hit the bottom of the SEED will float, if false, they will be removed
	bool					stack;			//!< if true, the entities can stack on top of each other
	bool					noinhibit;		//!< if true, the entities of this class will not be inhibited
	bool					nocombine;		//!< if true, the entities of this class will never be combined into megamodels
	bool					solid;			//!< if true, is solid and has thus a clipmodel
	idVec3					color_min;		//!< random color minimum value
	idVec3					color_max;		//!< random color maximum value
	idVec3					impulse_min;	//!< random impulse on spawn for moveables
	idVec3					impulse_max;	//!< random impulse on spawn for moveables
	float					defaultProb;	//!< Probabiliy with that entity class will appear. Only used if
											//!< materialNames is not empty, and then used as the default when
											//!< no entry in this list matches the texture the entity stands on.
	idList<seed_material_t>	materials;		//!< List of material name parts that we can match against
	int						nocollide;		//!< should this entity collide with:
   											//!< 1 other auto-generated entities from the same class?
											//!< 2 other auto-generated entities (other classes)
											//!< 4 other static entities already present
											//!< 8 world geometry
	idVec3					size;			//!< size of the model for collision tests during placement
	float					avgSize;		//!< Avg. size of a model to compute entity count
	int						score;			//!< Only used when max_entitites of the SEED != 0

	idVec3					scale_min;		//!< X Y Z min factors for randomly scaling rendermodels
	idVec3					scale_max;		//!< X Y Z max factor for randomly scaling rendermodels

	int						falloff;		//!< Entity random distribution method
											//!< 0 - none, 1 - cutoff, 2 - power, 3 - root, 4 - linear, 5 - func
	float					func_x;			//!< only used when falloff == 5
	float					func_y;
	float					func_s;
	float					func_a;
	int						func_Xt;		//!< 1 => X, 2 => X*X
	int						func_Yt;		//!< 1 => X, 2 => X*X
	int						func_f;			//!< 1 => Clamp, 0 => Zeroclamp
	float					func_min;
	float					func_max;

	idImageAsset*			imgmap;			//!< Handle of the image map

	bool					map_invert;		//!< if map != "": should the image map be inverted?
	float					map_scale_x;	//!< if map != "": scale the map in x direction
	float					map_scale_y;	//!< if map != "": scale the map in y direction
	float					map_ofs_x;		//!< x offset for the map (0..1.0)
	float					map_ofs_y;		//!< y offset for the map (0..1.0)

	float					z_min;			//!< depends on z_invert
	float					z_max;			//!< depends on z_invert
	float					z_fadein;		//!< depends on z_invert
	float					z_fadeout;		//!< depends on z_invert
	bool					z_invert;		//!< false => entities spawn between z_min => z_max, otherwise outside

	lod_handle				m_LODHandle;	//!< handle to shared, constant LOD data

	idDict					*spawnArgs;		//!< pointer to a dictionary with additional spawnargs that were present in the map file
};

/** Defines one area that inhibits entity spawning */
struct seed_inhibitor_t {
	idVec3					origin;			//!< origin of the area
	idVec3					size;			//!< size of the area (for falloff computation)
	idBox					box;			//!< oriented box of the area
	idList< idStr >			classnames;		//!< Contains a list of classes to inhibit/allow, depending on "inhibit_only"
	bool					inhibit_only;	//!< If false, classes in 'classnames' are allowed instead of inhibited
	int						falloff;		//!< Entity random inhibition method
											//!< 0 - none, 1 - cutoff, 2 - power, 3 - root, 4 - linear
	float					factor;			//!< if falloff == 2: X ** factor, if falloff == 3: factor'th root of X
};

#define SEED_ENTITY_FLAGMASK 0x00FFFFFF
#define SEED_ENTITY_FLAGSHIFT 24

enum seed_entity_flags {
	SEED_ENTITY_HIDDEN		= 0x0001,		//!< the entity is currently not existing (e.g. was culled or never spawned)
	SEED_ENTITY_EXISTS		= 0x0002,		//!< the entity is currently existing (e.g. was spawned)
	SEED_ENTITY_WAS_SPAWNED	= 0x0004,		//!< entity was spawned at least once (to trigger actions on first spawn)
	SEED_ENTITY_PSEUDO		= 0x0008,		//!< the entity is a pseudo-class entity, e.g. a multi-static
	SEED_ENTITY_WAITING		= 0x0010,		//!< the entity is still waiting for a timer before being spawned
	SEED_ENTITY_WATCHED		= 0x0020,		//!< Set on entities that are merely watched, so we do not cull
											//!< them unnec., f.i. when the menu setting changes
	SEED_ENTITY_COMBINED	= 0x0040		//!< Set on entities combined into other entities already, these will be removed afterwards.
};

// Defines one entity to be spawned/culled
struct seed_entity_t {
	int						skinIdx;		//!< index into skin list, the final skin for this entity (might be randomly choosen)
	idVec3					origin;			//!< (semi-random) origin
	idAngles				angles;			//!< zyx (yaw, pitch, roll) (semi-random) angles
	idVec3					scale;			//!< XYZ scale factor
	dword					color;			//!< (semi-random) color, computed from base/min/max colors of the class
	int						flags;			/*!< flags & 0x00FFFFFF:
												  0x01 hidden? 1 = hidden, 0 => visible
												  0x02 exists? 1 => exists, 0 => culled
												  0x04 0 => never spawned before, 1 => already spawned at least once
												  0x08 if 1, this entity has a pseudo class (e.g. it is a combined entity)
												 flags >> 24:
												  Current LOD (0 - normal, 1,2,3,4,5 LOD, 6 hidden)
											 */
	int						entity;			//!< nr of the entity if exists == true
	int						classIdx;		//!< index into m_Classes
};

extern const idEventDef EV_Disable;
extern const idEventDef EV_Enable;
extern const idEventDef EV_Deactivate;
extern const idEventDef EV_CullAll;

class Seed : public idStaticEntity {
public:
	CLASS_PROTOTYPE( Seed );

						Seed( void );
	virtual				~Seed( void ) override;

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	virtual void		Think( void ) override;

	/**
	* Clear the m_Classes list and also free any allocated models.
	*/
	void				ClearClasses( void );

	/**
	* Stop thinking and no longer cull/spawn entities.
	*/
	void				Event_Disable( void );

	/**
	* Start thinking and cull/spawn entities again.
	*/
	void				Event_Enable( void );

	/*
	* Cull all entities, including the watched-over ones (this is false in
	* case the menu changes, because then we do not want to cull+respawn them).
	*/
	void				CullAll( bool includingWatched = false );

	/*
	* Cull all entities (including watched ones). Only useful after Deactivate().
	*/
	void				Event_CullAll( void );

	void				Event_Activate( idEntity *activator );

private:

	/**
	* Look at our targets and create the entity classes. Calls PrepareEntities().
	*/
	void				Prepare( void );

	/**
	* Create the entity (pseudo-randomly choosen) positions.
	*/
	void				PrepareEntities( void );

	/**
	* Create the entity positions based on entities we watch.
	*/
	void				CreateWatchedList( void );

	/**
	* Compute the LOD level for this entity based on distance to player.
	*/
	int					ComputeLODLevel( const lod_data_t* m_LOD, const idVec3 dist ) const;

	/**
	* Combine entity models into "megamodels". Called automatically by PrepareEntities().
	*/
	void				CombineEntities( void );

	/**
	* Return a random int independedn from RandomFloat/RandomFloatSqr/RandomFloatExp, so we
	* can seed the other random generator per class.
	*/
	int					RandomSeed( void );

	/**
	* In the range 0.. 1.0, using our own m_iSeed value.
	*/
	float				RandomFloat( void );

	/**
	* Spawn the entity with the given index, return true if it could be spawned.
	* If managed is true, the SEED will take care of this entity for LOD changes.
	*/
	bool				SpawnEntity( const int idx, const bool managed );

	/**
	* Cull the entity with the given index, if it exists, return true if it could
	* be culled.
	*/
	bool				CullEntity( const int idx );

	/**
	* Parse the falloff spawnarg and return an integer representing it.
	*/
	int					ParseFalloff(idDict const *dict, idStr defaultName, idStr defaultFactor, float *func_a) const;

	/**
	* Parses a (potentially cached copy) mapfile and looks for the entity with the name and class, to find out
	* which spawnargs were set on it in the editor, since we need to preserve these. Returns a dict with these,
	* where common ones like "classname", "editor_", "seed_" etc. are already removed.
	*/
	idDict* 			LoadSpawnArgsFromMap(const idMapFile* mapFile, const idStr &entityName, const idStr &entityClass) const;

	/**
	* Take the given entity as template and add a template class from its values.
	*/
	void				AddClassFromEntity( idEntity *ent, const bool watch = false, const bool getSpawnArgs = true );

	/**
	* Take the given spawn_class or spawn_model spawnarg and add a template class based on it.
	*/
	void				AddTemplateFromEntityDef(idStr base, const idList<idStr> *sa);

	/**
	* Add an entry to the skin list unless it is there already. Return the index.
	*/
	int 				AddSkin( const idStr *skin );

	/**
	* Generate a scaling factor depending on the GUI setting.
	*/
	float				LODBIAS ( void ) const;

	/**
	* Set m_iNumEntities from spawnarg, or density, taking GUI setting into account.
	*/
	void				ComputeEntityCount( void );


	/* *********************** Members *********************/

	bool				active;

	/**
	* If true, we need still to prepare our entity list.
	*/
	bool				m_bPrepared;

	/**
	* Set to true if whether this static entity should hide
	* or change models/skins when outside a certain distance from the player.
	* If true, this entity will manage the LOD settings for entities, which
	* means these entities should have a "m_DistCheckInterval" of 0.
	**/
	bool				m_bDistDependent;

	/**
	* Current seed value for the random generator, which generates the sequence used to
	* place entities. Same seed value gives same sequence, thus same placing every time.
	**/
	int					m_iSeed;

	/**
	* Current seed value for the second random generator, which generates the sequence used
	* to initialize the first generator.
	**/
	int					m_iSeed_2;

	/**
	* Seed start value for the second random generator, used to restart the sequence.
	**/
	int					m_iOrgSeed;

	/**
	* Number of entities to manage overall.
	**/
	int					m_iNumEntities;

	/**
	* Number of entities currently visible.
	**/
	int					m_iNumVisible;

	/**
	* Number of entities currently existing (visible or not).
	**/
	int					m_iNumExisting;

	/**
	* If true, the LOD distance check will only consider distance
	* orthogonal to gravity.  This can be useful for things like
	* turning on rainclouds high above the player.
	**/
	bool				m_bDistCheckXYOnly;

	/**
	* Timestamp and interval between distance checks, in milliseconds
	* stgatilov: timestamp is for the !next! think
	**/
	int					m_DistCheckTimeStamp;
	int					m_DistCheckInterval;

	//stgatilov: LOD properties which were moved from idEntity to LodComponent
	//note: this component is not registered in LodSystem (whole SEED is a mess)
	LodComponent		m_LodComponent;

	/**
	* The classes of entities that we need to construct.
	**/
	idList<seed_class_t>		m_Classes;

	/**
	* The entities that inhibit us from spawning inside their area.
	**/
	idList<seed_inhibitor_t>	m_Inhibitors;

	/**
	* Info about each entitiy that we spawn or cull.
	**/
	idList<seed_entity_t>		m_Entities;

	/**
	* Info about each entitiy that we watch (e.g. that already existed and
	* that we just cloned).
	**/
	idList<seed_entity_t>		m_Watched;

	/**
	* List of entities that we need to remove along with our targets, this
	* are entity numbers for things that we watch over. Gets filled by
	* CreateWatchList() and emptied when we remove our targets.
	**/
	idList< int >				m_Remove;

	/**
	* Names of all different skins.
	**/
	idList<idStr>				m_Skins;

	/**
	* A copy of cv_lod_bias, to detect changes during runtime.
	*/
	float						m_fLODBias;

	/**
	* The PVS this SEED spans - can be more than one when it crosses a visportal.
	* TODO: What if it are more than 64?
	*/
	int							m_iNumPVSAreas;
	int							m_iPVSAreas[64];

	/**
	* If we are outside the PVS area, only think every Nth time. This counter is set
	* to 0 evertime we think and increased if outside the PVS.
	*/
	int							m_iThinkCounter;

	/**
	* If true, wait until triggered before spawning entities.
	*/
	bool						m_bWaitForTrigger;

	/**
	* Debug level. Default 0. Higher values will print more debug info.
	*/
	int							m_iDebug;

	/**
	* The origin of the SEED brush.
	*/
	idVec3						m_origin;

	/**
	* If true, debug colors are used instead of normal skins.
	*/
	bool						m_bDebugColors;

	/**
	* If true, this SEED will combine entities into StaticMulti entities
	*/
	bool						m_bCombine;

	/**
	* How many StaticMulti entities do we have that require a SetLODData?
	*/
	int							m_iNumStaticMulties;

	/**
	* If Restore() just finished, the next Think() does need to do this.
	*/
	bool 						m_bRestoreLOD;

	/**
	* Number of currently existing entities, to see if we reached the spawn limit.
	*/
	int 						m_iNumEntitiesInGame;

	static const unsigned int IEEE_ONE  = 0x3f800000U;
	static const unsigned int IEEE_MASK = 0x007fffffU;

	static const unsigned int NOCOLLIDE_SAME   = 0x01;
	static const unsigned int NOCOLLIDE_OTHER  = 0x02;
	static const unsigned int NOCOLLIDE_STATIC = 0x04;
	static const unsigned int NOCOLLIDE_WORLD  = 0x08;
	static const unsigned int NOCOLLIDE_ATALL  = NOCOLLIDE_WORLD + NOCOLLIDE_STATIC + NOCOLLIDE_OTHER + NOCOLLIDE_SAME;
	static const unsigned int COLLIDE_WORLD    = NOCOLLIDE_STATIC + NOCOLLIDE_OTHER + NOCOLLIDE_SAME;
	static const unsigned int COLLIDE_ALL  	 = 0x00;
};

#endif /* !__GAME_SEED_H__ */

