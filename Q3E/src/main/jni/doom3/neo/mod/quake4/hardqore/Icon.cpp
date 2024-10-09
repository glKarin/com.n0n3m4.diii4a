//----------------------------------------------------------------
// Icon.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "Icon.h"

/*
===============
rvIcon::rvIcon
===============
*/
rvIcon::rvIcon() {
	iconHandle	= -1;
}

/*
===============
rvIcon::~rvIcon
===============
*/
rvIcon::~rvIcon() {
	FreeIcon();
}

/*
===============
rvIcon::FreeIcon
===============
*/
void rvIcon::FreeIcon( void ) {
	if ( iconHandle != - 1 ) {
		gameRenderWorld->FreeEntityDef( iconHandle );
		iconHandle = -1;
	}
}

/*
===============
rvIcon::CreateIcon
===============
*/
qhandle_t rvIcon::CreateIcon( const char *mtr, int suppressViewID ) {
	FreeIcon();

	memset( &renderEnt, 0, sizeof( renderEnt ) );
	renderEnt.origin	= vec3_origin;
	renderEnt.axis		= mat3_identity;
	renderEnt.shaderParms[ SHADERPARM_RED ]				= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_GREEN ]			= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_BLUE ]			= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_ALPHA ]			= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_SPRITE_WIDTH ]	= 16.0f;
	renderEnt.shaderParms[ SHADERPARM_SPRITE_HEIGHT ]	= 16.0f;
	renderEnt.hModel = renderModelManager->FindModel( "_sprite" );
	renderEnt.callback = NULL;
	renderEnt.numJoints = 0;
	renderEnt.joints = NULL;
	renderEnt.customSkin = 0;
	renderEnt.noShadow = true;
	renderEnt.noSelfShadow = true;
	renderEnt.customShader = declManager->FindMaterial( mtr );
	renderEnt.referenceShader = 0;
	renderEnt.bounds = renderEnt.hModel->Bounds( &renderEnt );
	renderEnt.suppressSurfaceInViewID = suppressViewID;
	
	iconHandle = gameRenderWorld->AddEntityDef( &renderEnt );

	return iconHandle;
}

/*
===============
rvIcon::UpdateIcon
===============
*/
void rvIcon::UpdateIcon( const idVec3 &origin, const idMat3 &axis ) {
	assert( iconHandle >= 0 );

	renderEnt.origin = origin;
	renderEnt.axis	= axis;
	gameRenderWorld->UpdateEntityDef( iconHandle, &renderEnt );
}

int rvIcon::GetWidth( void ) const {
	return renderEnt.shaderParms[ SHADERPARM_SPRITE_WIDTH ];
}

int rvIcon::GetHeight( void ) const {
	return renderEnt.shaderParms[ SHADERPARM_SPRITE_HEIGHT ];
}
