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

#ifndef __DARKMOD_STATICMULTI_H__
#define __DARKMOD_STATICMULTI_H__

#include "Misc.h"
#include "StimResponse/StimResponseCollection.h"
#include "ModelGenerator.h"

/*
===============================================================================

  StaticMulti - An entity with a physics object consisting of multiple
  				clipmodels and a megamodel as rendermodel. Used by the SEED
				system to combine multiple entities.

===============================================================================
*/

// Defines info about a change to a combined model. E.g. if a combined model was
// combined from 2 times model A, and we want to change the second model from A
// to B, we use this struct:
typedef struct {
	int				entity;					// the entity index in the offsets list this change applies to
	int				oldLOD;					// the original model combined into the megamodel
	int				newLOD;					// the new model to be combined into the megamodel
	int				oldFlags;				// the original flags for this offset
	int				newFlags;				// the new flags (f.i. noshadows)
} model_changeinfo_t;

class CStaticMulti : public idStaticEntity {
public:
	CLASS_PROTOTYPE( CStaticMulti );

						CStaticMulti( void );
	virtual				~CStaticMulti() override;

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );

	void				SetLODData( const idVec3 &origin, const unsigned int lodHandle, idStr modelName, idList<model_ofs_t>* offsets, idStr materialName, const idRenderModel* hModel, const idClipModel* clip);

//	virtual void		Hide( void ) override;
//	virtual void		Show( void ) override;
	virtual void		Think( void ) override;

//	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const override;
//	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg ) override;

private:
	void						Event_Activate( idEntity *activator );
	
	/**
	* Marks the model for the entity #entity (offset into Offsets list) to be changed to newLOD, and the
	* new flags set in newFlags (f.i. noShadows).
	*/
	void						AddChange( const int entity, const int newLOD, const int newFlags );

	/**
	* The entity presenting/using this model is going to get culled, so stop all updates.
	*/
	void						StopUpdating();

	/**
	* The entity presenting/using this model is going to get culled, so remove all changes.
	*/
	void						ClearChanges();

	/**
	* The entity presenting/using this model is spawned again, so start updates.
	*/
	void						StartUpdating();

	/**
	* If nec, create a new renderModel (and return true if done so). If force is true,
	* the model will be recreated, regardless of whether there where enough changes or not.
	* #4168 -- "force" is no longer used. The model is always created afresh, due to heap 
	* corruption when the step was skipped. See notes in UpdateRenderModel(). 
	*/
	bool						UpdateRenderModel( const bool force = false);

	bool						active;
	// if none of our offsets have LOD stages, we don't need to update the model
	bool						m_bNeedModelUpdates;

	idPhysics_StaticMulti		physics;

	/** override material for debug_colors: */
	idStr						m_MaterialName;

	idList<model_ofs_t>*		m_Offsets;			//!< pt to the list of the individual entity combined into the model
	idList<model_changeinfo_t>	m_Changes;			//!< list with changes accumulated before a rendermodel update

	const idRenderModel*		m_hModel;			//!< if we need to combine from a func_static, this is its renderModel
	idStr						m_modelName;		//!< in case we have no LOD struct, this is our model name

	int							m_iMaxChanges;		//!< maximum number of changes before we update
	int							m_iVisibleModels;	//!< how many of our models are currently visible? if == 0, we are invisible, too

	bool						m_bFree_hModel;		//!< The initial hModel cannot be freed as it is used by the engine, but once we create our own, we need to free it.

	bool						m_bNoshadows;		//!< if true, completely disable shadows

	// TODO: use these from m_LOD:
	int							m_DistCheckTimeStamp;
	int							m_DistCheckInterval;
	float						m_fHideDistance;
	bool						m_bDistCheckXYOnly;

	lod_handle					m_LODHandle;		//!< to retrieve the LOD Data from the ModelGenerator

	//stgatilov: LOD properties which were moved from idEntity to LodComponent
	//note: this component is not registered in LodSystem (whole SEED is a mess)
	LodComponent		m_LodComponent;

};

#endif /* !__DARKMOD_STATICMULTI_H__ */

