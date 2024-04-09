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

// Copyright (C) 2004 Id Software, Inc.
// Copyright (C) 2011 The Dark Mod

#ifndef __GAME_TDM_EMITTER_H__
#define __GAME_TDM_EMITTER_H__

#include "Misc.h"

/*
===============================================================================

idFuncEmitter

===============================================================================
*/

/** Defines additional data for emitters with more than one particle */
typedef struct {
	int						defHandle;
	idVec3					offset;
	idRenderModel *			handle;
	idStr					name;		// empty when this model equals the first model
	int						flags;		// 0 => visible, 1 => hidden
} emitter_models_t;

class idFuncEmitter : public idStaticEntity {
public:
	CLASS_PROTOTYPE( idFuncEmitter );

						idFuncEmitter( void );
	virtual				~idFuncEmitter( void ) override;

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	void				On( void );
	void				Off( void );
	void				Event_Activate( idEntity *activator );
	void				Event_On( void );
	void				Event_Off( void );

	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg ) override;

	virtual void		Think( void ) override;
	virtual void		Present( void ) override;

	// switch to a new model
	virtual void		SetModel( const char *modelname ) override;

	// public events
	void				Event_EmitterGetNumModels( void ) const;
	void				Event_EmitterAddModel( idStr const &modelName, idVec3 const &modelOffset );

protected:

	// add an extra model
	void				SetModel( int id, const idStr &modelName, const idVec3 &offset );

private:
	bool						hidden;

	idList<emitter_models_t> 	m_models;			//! struct with data for additional models
};

/*
===============================================================================

idFuncSplat

===============================================================================
*/

class idFuncSplat : public idFuncEmitter {
public:
	CLASS_PROTOTYPE( idFuncSplat );

	idFuncSplat( void );

	void				Spawn( void );

private:
	void				Event_Activate( idEntity *activator );
	void				Event_Splat();
};


#endif /* !__GAME_TDM_EMITTER_H__ */

