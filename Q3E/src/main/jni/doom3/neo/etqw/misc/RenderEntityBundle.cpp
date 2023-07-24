// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "RenderEntityBundle.h"

/*
===============================================================================

	sdRenderEntityBundle

===============================================================================
*/

/*
================
sdRenderEntityBundle::sdRenderEntityBundle
================
*/
sdRenderEntityBundle::sdRenderEntityBundle( void ) {
	handle = -1;
	memset( &entity, 0, sizeof( entity ) );
}

/*
================
sdRenderEntityBundle::~sdRenderEntityBundle
================
*/
sdRenderEntityBundle::~sdRenderEntityBundle( void ) {
	Hide();
}

/*
================
sdRenderEntityBundle::Copy
================
*/
void sdRenderEntityBundle::Copy( const renderEntity_t& rhs ) {
	entity = rhs;
	Update();
}

/*
================
sdRenderEntityBundle::Update
================
*/
void sdRenderEntityBundle::Update( void ) {
	if ( handle == -1 ) {
		return;
	}

	gameRenderWorld->UpdateEntityDef( handle, &entity );
}

/*
================
sdRenderEntityBundle::Show
================
*/
void sdRenderEntityBundle::Show( void ) {
	if ( handle != -1 ) {
		return;
	}

	handle = gameRenderWorld->AddEntityDef( &entity );
}

/*
================
sdRenderEntityBundle::Hide
================
*/
void sdRenderEntityBundle::Hide( void ) {
	if ( handle == -1 ) {
		return;
	}

	gameRenderWorld->FreeEntityDef( handle );
	handle = -1;
}
