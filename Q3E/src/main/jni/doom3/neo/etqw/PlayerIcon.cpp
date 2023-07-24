// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "PlayerIcon.h"
#include "Player.h"
#include "misc/WorldToScreen.h"
#include "../decllib/declTypeHolder.h"
#include "rules/GameRules.h"
#include "vehicles/JetPack.h"

/*
===============
idPlayerIcon::idPlayerIcon
===============
*/
idPlayerIcon::idPlayerIcon( void ) {
	activeIcon	= -1;
}

/*
===============
idPlayerIcon::~idPlayerIcon
===============
*/
idPlayerIcon::~idPlayerIcon( void ) {
}

/*
===============
idPlayerIcon::Init
===============
*/
void idPlayerIcon::Init( const idDict& dict ) {
}

/*
===============
idPlayerIcon::GetPosition
===============
*/
void idPlayerIcon::GetPosition( idPlayer *player, jointHandle_t joint, float offset, idVec3& origin ) {
	idEntity* entity = player->GetProxyEntity();
	if ( entity != NULL && !entity->IsType( sdJetPack::Type ) ) {
		jointHandle_t proxyJoint = entity->GetUsableInterface()->GetPlayerIconJoint( player );
		if ( proxyJoint != INVALID_JOINT ) {
			entity->GetAnimator()->GetJointTransform( proxyJoint, gameLocal.time, origin );
			origin = entity->GetLastPushedOrigin() + ( origin * entity->GetLastPushedAxis() );
			return;
		}
	}

	if ( joint != INVALID_JOINT ) {
		idMat3 tempAxis;
		player->GetWorldOriginAxisNoUpdate( joint, origin, tempAxis );
	} else {
		origin = player->GetLastPushedOrigin();
		origin.z += player->GetPhysics()->GetBounds().GetMaxs().z;
	}

	origin.z += offset;
}

/*
===============
idPlayerIcon::GetActiveIcon
===============
*/
const idMaterial* idPlayerIcon::GetActiveIcon( void ) {
	UpdateIcons();
	if ( activeIcon == -1 ) {
		return NULL;
	}

	return icons[ activeIcon ].material;
}

/*
===============
idPlayerIcon::FreeIcon
===============
*/
void idPlayerIcon::FreeIcon( qhandle_t handle ) {
	if ( handle < 0 || handle >= icons.Num() ) {
		return;
	}

	icons[ handle ].material = NULL;

	if ( handle == activeIcon ) {
		FindActiveIcon();
	}
}

/*
===============
idPlayerIcon::CreateIcon
===============
*/
qhandle_t idPlayerIcon::CreateIcon( const idMaterial* material, int priority, int timeout ) {
	if ( !material ) {
		return -1;
	}

	int i;
	for ( i = 0; i < icons.Num(); i++ ) {
		if ( icons[ i ].material == NULL || icons[ i ].material == material ) {
			break;
		}
	}
	if ( i >= icons.Num() ) {
		i = icons.Num();
		icons.Alloc();
	}

	icons[ i ].material = material;
	icons[ i ].priority = priority;
	if ( timeout <= 0 ) {
		icons[ i ].timeout = 0;
	} else {
		icons[ i ].timeout = gameLocal.time + timeout;
	}

	FindActiveIcon();

	return i;
}

/*
===============
idPlayerIcon::FindActiveIcon
===============
*/
void idPlayerIcon::FindActiveIcon( void ) {
	int	bestValue	= -1;
	activeIcon		= -1;

	for ( int i = 0; i < icons.Num(); i++ ) {
		if ( !icons[ i ].material ) {
			continue;
		}

		if ( icons[ i ].priority > bestValue ) {
			activeIcon = i;
			bestValue = icons[ i ].priority;
		}
	}
}

/*
===============
idPlayerIcon::UpdateIcons
===============
*/
void idPlayerIcon::UpdateIcons( void ) {
	bool changed = false;

	for ( int i = 0; i < icons.Num(); i++ ) {
		if ( icons[ i ].timeout != 0 && icons[ i ].timeout < gameLocal.time ) {
			icons[ i ].material = NULL;
			changed = true;
		}
	}

	if ( changed ) {
		FindActiveIcon();
	}
}
