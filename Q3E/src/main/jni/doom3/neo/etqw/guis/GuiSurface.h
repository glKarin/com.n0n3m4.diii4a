// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GUISURFACE_H__
#define __GUISURFACE_H__

#include "../client/ClientEntity.h"
#include "GuiSurfaceRenderable.h"

class sdGuiSurface : public rvClientEntity {
public:
	CLASS_PROTOTYPE( sdGuiSurface );

	virtual void					CleanUp( void );

	void							Init( idEntity* master, const guiSurface_t& surface, const guiHandle_t& handle, const int allowInViewID = 0, const bool weaponDepthHack = false );
	void							Init( rvClientEntity* master, const guiSurface_t& surface, const guiHandle_t& handle, const int allowInViewID = 0, const bool weaponDepthHack = false );

	virtual void					Think();

	void							DrawCulled( const pvsHandle_t pvsHandle, const idFrustum& viewFrustum ) const;

	sdGuiSurfaceRenderable&			GetRenderable() { return renderable; }

	virtual const char*				GetName( void ) const { return "sdGuiSurface"; }

protected:
	sdGuiSurfaceRenderable			renderable;
};

#endif /* __GUISURFACE_H__ */
