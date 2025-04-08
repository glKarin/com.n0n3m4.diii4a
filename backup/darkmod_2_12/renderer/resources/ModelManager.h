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

#ifndef __MODELMANAGER_H__
#define __MODELMANAGER_H__

/*
===============================================================================

	Model Manager

	Temporarily created models do not need to be added to the model manager.

===============================================================================
*/

class idRenderModelManager {
public:
	virtual					~idRenderModelManager() {}

	// registers console commands and clears the list
	virtual	void			Init() = 0;

	// frees all the models
	virtual	void			Shutdown() = 0;

	// called only by renderer::BeginLevelLoad
	virtual void			BeginLevelLoad() = 0;

	// called only by renderer::EndLevelLoad
	virtual void			EndLevelLoad() = 0;

	// allocates a new empty render model.
	virtual idRenderModel *	AllocModel() = 0;

	// frees a render model
	virtual void			FreeModel( idRenderModel *model ) = 0;

	// returns NULL if modelName is NULL or an empty string, otherwise
	// it will create a default model if not loadable
	virtual	idRenderModel *	FindModel( const char *modelName ) = 0;

	// returns NULL if not loadable
	virtual	idRenderModel *	CheckModel( const char *modelName ) = 0;

	// returns the default cube model
	virtual	idRenderModel *	DefaultModel() = 0;

	// world map parsing will add all the inline models with this call
	virtual	void			AddModel( idRenderModel *model ) = 0;

	// when a world map unloads, it removes its internal models from the list
	// before freeing them.
	// There may be an issue with multiple renderWorlds that share data...
	virtual	void			RemoveModel( idRenderModel *model ) = 0;

	// the reloadModels console command calls this, but it can
	// also be explicitly invoked
	virtual	void			ReloadModels( bool forceAll = false ) = 0;

	// write "touchModel <model>" commands for each non-world-map model
	virtual	void			WritePrecacheCommands( idFile *f ) = 0;

	// called during vid_restart
	virtual	void			FreeModelVertexCaches() = 0;

	// print memory info
	virtual	void			PrintMemInfo( MemInfo_t *mi ) = 0;
};

// this will be statically pointed at a private implementation
extern	idRenderModelManager	*renderModelManager;

#endif /* !__MODELMANAGER_H__ */
