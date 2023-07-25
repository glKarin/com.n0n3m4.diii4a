// Copyright (C) 2004 Id Software, Inc.
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "PlayerIcon.h"

static const char * iconKeys[ ICON_NONE ] = {
	"mtr_icon_lag",
	"mtr_icon_chat"
#ifdef CTF
	,"mtr_icon_redteam",
	"mtr_icon_blueteam"
#endif
};

/*
===============
idPlayerIcon::idPlayerIcon
===============
*/
idPlayerIcon::idPlayerIcon() {
	iconHandle	= -1;
	iconType	= ICON_NONE;
}

/*
===============
idPlayerIcon::~idPlayerIcon
===============
*/
idPlayerIcon::~idPlayerIcon() {
	FreeIcon();
}

/*
===============
idPlayerIcon::Draw
===============
*/
void idPlayerIcon::Draw( idPlayer *player, jointHandle_t joint ) {
	idVec3 origin;
	idMat3 axis;

	if ( joint == INVALID_JOINT ) {
		FreeIcon();
		return;
	}

	player->GetJointWorldTransform( joint, gameLocal.time, origin, axis );
	origin.z += 16.0f;

	Draw( player, origin );
}

/*
===============
idPlayerIcon::Draw
===============
*/
void idPlayerIcon::Draw( idPlayer *player, const idVec3 &origin ) {
	idPlayer *localPlayer = gameLocal.GetLocalPlayer();
	if ( !localPlayer || !localPlayer->GetRenderView() ) {
		FreeIcon();
		return;
	}

	idMat3 axis = localPlayer->GetRenderView()->viewaxis;

	if ( player->isLagged && !player->spectating ) {
		// create the icon if necessary, or update if already created
		if ( !CreateIcon( player, ICON_LAG, origin, axis ) ) {
			UpdateIcon( player, origin, axis );
		}
	} else if ( player->isChatting && !player->spectating ) {
		if ( !CreateIcon( player, ICON_CHAT, origin, axis ) ) {
			UpdateIcon( player, origin, axis );
		}
#ifdef CTF
	} else if ( g_CTFArrows.GetBool() && gameLocal.mpGame.IsGametypeFlagBased() && gameLocal.GetLocalPlayer() && player->team == gameLocal.GetLocalPlayer()->team && !player->IsHidden() && !player->AI_DEAD ) {
		int icon = ICON_TEAM_RED + player->team;

		if ( icon != ICON_TEAM_RED && icon != ICON_TEAM_BLUE )
			return;

		if ( !CreateIcon( player, ( playerIconType_t )icon, origin, axis ) ) {
			UpdateIcon( player, origin, axis );
		}
#endif
	} else {
		FreeIcon();
	}
}

/*
===============
idPlayerIcon::FreeIcon
===============
*/
void idPlayerIcon::FreeIcon( void ) {
	if ( iconHandle != - 1 ) {
		gameRenderWorld->FreeEntityDef( iconHandle );
		iconHandle = -1;
	}
	iconType = ICON_NONE;
}

/*
===============
idPlayerIcon::CreateIcon
===============
*/
bool idPlayerIcon::CreateIcon( idPlayer *player, playerIconType_t type, const idVec3 &origin, const idMat3 &axis ) {
	assert( type != ICON_NONE );
	const char *mtr = player->spawnArgs.GetString( iconKeys[ type ], "_default" );
	return CreateIcon( player, type, mtr, origin, axis );
}

/*
===============
idPlayerIcon::CreateIcon
===============
*/
bool idPlayerIcon::CreateIcon( idPlayer *player, playerIconType_t type, const char *mtr, const idVec3 &origin, const idMat3 &axis ) {
	assert( type != ICON_NONE );

	if ( type == iconType ) {
		return false;
	}

	FreeIcon();

	memset( &renderEnt, 0, sizeof( renderEnt ) );
	renderEnt.origin	= origin;
	renderEnt.axis		= axis;
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

	iconHandle = gameRenderWorld->AddEntityDef( &renderEnt );
	iconType = type;

	return true;
}

/*
===============
idPlayerIcon::UpdateIcon
===============
*/
void idPlayerIcon::UpdateIcon( idPlayer *player, const idVec3 &origin, const idMat3 &axis ) {
	assert( iconHandle >= 0 );

	renderEnt.origin = origin;
	renderEnt.axis	= axis;
	gameRenderWorld->UpdateEntityDef( iconHandle, &renderEnt );
}

