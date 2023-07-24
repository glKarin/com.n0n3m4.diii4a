// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserInterfaceManager.h"
#include "UIWindow.h"
#include "UserInterfaceLocal.h"
#include "UICinematic.h"

SD_UI_IMPLEMENT_CLASS( sdUICinematic, sdUIWindow )

/*
============
sdUICinematic::sdUICinematic
============
*/
sdUICinematic::sdUICinematic() :
	active( 0.0f ),
	looping( 0.0f ),
	soundShader( NULL ),
	sw( NULL ),
	soundEmitter( NULL ) {

	scriptState.GetProperties().RegisterProperty( "soundShader", soundShaderName );
	scriptState.GetProperties().RegisterProperty( "active", active );
	scriptState.GetProperties().RegisterProperty( "looping", looping );

	UI_ADD_STR_CALLBACK( soundShaderName, sdUICinematic, OnSoundShaderChanged )
	UI_ADD_FLOAT_CALLBACK( active, sdUICinematic, OnActiveChanged )
}

/*
============
sdUICinematic::sdUICinematic
============
*/
sdUICinematic::~sdUICinematic() {
	if ( soundEmitter != NULL ) {
		soundEmitter->Free( true );
		soundEmitter = NULL;
	}
}

/*
============
sdUICinematic::DrawLocal
============
*/
void sdUICinematic::DrawLocal() {
	if( !PreDraw() ) {
		return;
	}

	if ( soundEmitter == NULL ) {
		PostDraw();
		return;
	}

	int handle;
	uiMaterialCache_t::Iterator iter = GetUI()->FindCachedMaterial( va( "%p_mat", this ), handle );

	if( !GetUI()->IsValidMaterial( iter ) ) {
		PostDraw();
		return;
	}

	uiCachedMaterial_t& cached = *iter->second;

	deviceContext->SetColor( backColor );
	deviceContext->DrawCinematic( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w,
									0.0f, 0.0f, 1.0f, 1.0f,
									cached.material.material, soundEmitter, rotation );

	if ( !soundEmitter->CurrentlyPlaying() && active == 1.0f ) {
		active = 0.0f;
	}
	PostDraw();
}

/*
============
sdUICinematic::OnSoundShaderChanged
============
*/
void sdUICinematic::OnSoundShaderChanged( const idStr& oldValue, const idStr& newValue ) {
	if ( active != 0.0f ) {
		active = 0.0f;
	}

	soundShader = declHolder.FindSoundShader( GetUI()->GetSound( newValue.c_str() ) );
}

/*
============
sdUICinematic::OnActiveChanged
============
*/
void sdUICinematic::OnActiveChanged( const float oldValue, const float newValue ) {
	if ( newValue == 1.0f && oldValue == 0.0f ) {
		if ( sw == NULL ) {
			sw = soundSystem->GetPlayingSoundWorld();

			if ( sw == NULL ) {
				return;
			}

			soundEmitter = sw->AllocSoundEmitter();

			int shaderFlags = 0;
			if ( looping > 0.0f ) {
				shaderFlags |= SSF_LOOPING;
			}

			//gameLocal.Printf( "Starting sound %s\n", soundShader->GetName() );
			soundEmitter->StartSound( soundShader, SND_ANY, 0, 0.0f, shaderFlags );
		}
	} else if ( newValue == 0.0f && oldValue == 1.0f ) {
		if ( sw == NULL ) {
			return;
		}

		if ( soundEmitter != NULL ) {
			soundEmitter->Free( true );
			soundEmitter = NULL;
		}

        sw = NULL;
	}
}
