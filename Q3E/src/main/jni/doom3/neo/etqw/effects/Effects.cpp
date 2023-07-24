// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Effects.h"

//===============================================================
//
//	sdEffect
//
//===============================================================

/*
================
sdEffect::Init
================
*/
void sdEffect::Init( void ) {
	memset( &effect, 0, sizeof( effect ) );
	effectHandle = -1;
	effectStopped = true;
	waitingToDie = false;
	effectNode.SetOwner( this );
}

void sdEffect::Init( const char* effectName ) {
	Init();

	effect.declEffect							= gameLocal.FindEffect( effectName );
	effect.axis									= mat3_identity;

	effect.attenuation							= 1.f;
	effect.hasEndOrigin							= false;
	effect.loop									= false;

	effect.shaderParms[SHADERPARM_RED]			= 1.0f;
	effect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
	effect.shaderParms[SHADERPARM_BLUE]			= 1.0f;
	effect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
	effect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;
}

/*
================
sdEffect::Free
================
*/
void sdEffect::Free( void ) {
	if ( effectHandle != -1 ) {
		gameRenderWorld->FreeEffectDef( effectHandle );
		effectHandle = -1;
		effectStopped = true;
		waitingToDie = false;
	}
}

/*
================
sdEffect::Start
================
*/
bool sdEffect::Start( int startTime ) {
	assert( !waitingToDie );

	if ( effect.declEffect != NULL ) {
		if ( effectHandle == -1 ) {
			effect.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
			effectHandle = gameRenderWorld->AddEffectDef( &effect, startTime );
			effectStopped = false;
		} else if ( effectStopped ) {
			for ( int i = 0; i < waitingToDieList.Num(); i++ ) {
				if ( waitingToDieList[ i ].effectHandle == effectHandle ) {
					waitingToDieList.RemoveIndexFast( i );
					break;
				}
			}
			waitingToDie = false;
			gameRenderWorld->RestartEffectDef( effectHandle );
			effectStopped = false;
		}
	}

	if ( effectHandle != -1 ) {
		effectStopped = false;
		return true;
	} else {
		return false;
	}
}

/*
================
sdEffect::Stop
================
*/
void sdEffect::Stop( bool appendToDieList ) {
	if ( effectHandle != -1 && !effectStopped ) {
		gameRenderWorld->StopEffectDef( effectHandle );
		effectStopped = true;
		if ( appendToDieList ) {
			waitingToDie = true;
			waitingToDieList.Append( *this );
			waitingToDie = false;
		}
	}
}

/*
================
sdEffect::StopDetach
================
*/
void sdEffect::StopDetach( void ) {
	// Stop the current one and let it die naturally
	Stop( true );

	// Make sure we start a new effect next time start is called
	effectHandle = -1;
	effectStopped = true;
}

/*
================
sdEffect::Update
================
*/
void sdEffect::Update( void ) {
	if ( effectHandle != -1 && effect.declEffect ) {
		effect.windVector = gameLocal.GetWindVector( effect.origin );
		if ( gameRenderWorld->UpdateEffectDef( effectHandle, &effect, gameLocal.time ) ) {
			Free();
		}
	}
}

// This is a list of effects waiting until their update returns false
// meaning all the particles in the effect died naturally and it
// can be unregistered from the renderer
idList<sdEffect::DeadEffect> sdEffect::waitingToDieList;

/*
================
sdEffect::UpdateDieingEffects
================
*/
void sdEffect::UpdateDeadEffects( void ) {
	for ( int i = 0; i < waitingToDieList.Num(); ) {
		DeadEffect &eff = waitingToDieList[i];

		if ( gameRenderWorld->UpdateEffectDef( eff.effectHandle, &eff.effect, gameLocal.time ) ) {
			gameRenderWorld->FreeEffectDef( eff.effectHandle );
			waitingToDieList.RemoveIndexFast( i );
		} else {
			i++;
		}
	}
}

/*
================
sdEffect::FrieeDieingEffects
================
*/
void sdEffect::FreeDeadEffects( void ) {
	while ( waitingToDieList.Num() ) {
		gameRenderWorld->FreeEffectDef( waitingToDieList[0].effectHandle );
		waitingToDieList.RemoveIndexFast( 0 );
	}
}