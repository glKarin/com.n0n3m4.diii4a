// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "GuiSurface.h"
#include "UserInterfaceLocal.h"
#include "../../renderer/DeviceContext.h"

/*
===============================================================================

sdGuiSurface

===============================================================================
*/

CLASS_DECLARATION( rvClientEntity, sdGuiSurface )
END_CLASS

/*
============
sdGuiSurface::CleanUp
============
*/
void sdGuiSurface::CleanUp( void ) {
	renderable.Clear();

	rvClientEntity::CleanUp();
}

/*
============
sdGuiSurface::Init
============
*/
void sdGuiSurface::Init( idEntity* master, const guiSurface_t& surface, const guiHandle_t& handle, const int allowInViewID, const bool weaponDepthHack ) {
	Bind( master, surface.joint );
	renderable.Init( surface, handle, allowInViewID, weaponDepthHack );
	renderable.SetPosition( worldOrigin, worldAxis );
}

/*
============
sdGuiSurface::Init
============
*/
void sdGuiSurface::Init( rvClientEntity* master, const guiSurface_t& surface, const guiHandle_t& handle, const int allowInViewID, const bool weaponDepthHack ) {
	Bind( master, surface.joint );
	renderable.Init( surface, handle, allowInViewID, weaponDepthHack );
	renderable.SetPosition( worldOrigin, worldAxis );
}

/*
============
sdGuiSurface::Think
============
*/
void sdGuiSurface::Think() {
	rvClientEntity::Think();
	renderable.SetPosition( worldOrigin, worldAxis );
}

/*
============
sdGuiSurface::DrawCulled
============
*/
void sdGuiSurface::DrawCulled( const pvsHandle_t pvsHandle, const idFrustum& viewFrustum ) const {
	if ( bindMaster.IsValid() && bindMaster->IsHidden() ) {
		return;
	} else if ( bindMasterClient.IsValid() && bindMasterClient->IsHidden() ) {
		return;
	}

	renderable.DrawCulled( gameLocal.pvs, pvsHandle, viewFrustum );
}
