// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclAOR.h"
#include "../demos/DemoManager.h"

idCVar aor_physicsCutoffScale( "aor_physicsCutoffScale", "1", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "scale the aor physics cutoff distance" );
idCVar aor_animationCutoffScale( "aor_animationCutoffScale", "1", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "scale the aor animation cutoff distance" );
idCVar aor_ikCutoffScale( "aor_ikCutoffScale", "1", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "scale the aor ik cutoff distance" );

idCVar aor_physicsLod1StartScale( "aor_physicsLod1StartScale", "1", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "scale the aor physics lod 1 distance" );
idCVar aor_physicsLod2StartScale( "aor_physicsLod2StartScale", "1", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "scale the aor physics lod 2 distance" );
idCVar aor_physicsLod3StartScale( "aor_physicsLod3StartScale", "1", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "scale the aor physics lod 3 distance" );

/*
===============================================================================

	DeclAOR

===============================================================================
*/

/*
================
sdDeclAOR::sdDeclAOR
================
*/
sdDeclAOR::sdDeclAOR( void ) {
	Clear();
}

/*
================
sdDeclAOR::Clear
================
*/
void sdDeclAOR::Clear( void ) {
	spreadScale = 1.0f;
	spreadStart = 0.0f;
	spreadDistance = 0.0f;

	// set the cutoffs massive so we don't need to check if they're valid before comparing with them
	userCommandCutoffSqr = idMath::INFINITY;

	baseCutoffs.animationCutoffSqr = idMath::INFINITY;
	baseCutoffs.ikCutoffSqr = idMath::INFINITY;
	baseCutoffs.physicsCutoffSqr = idMath::INFINITY;

	baseCutoffs.boxDecayClipStartSqr = idMath::INFINITY;
	baseCutoffs.pointDecayClipStartSqr = idMath::INFINITY;
	baseCutoffs.heightMapDecayClipStartSqr = idMath::INFINITY;
	baseCutoffs.decayClipEndSqr = idMath::INFINITY;

	baseCutoffs.physicsLOD1StartSqr = idMath::INFINITY;
	baseCutoffs.physicsLOD2StartSqr = idMath::INFINITY;
	baseCutoffs.physicsLOD3StartSqr = idMath::INFINITY;

	UpdateCutoffs();
}

/*
================
sdDeclAOR::~sdDeclAOR
================
*/
sdDeclAOR::~sdDeclAOR( void ) {

}

/*
================
sdDeclAOR::DefaultDefinition
================
*/
const char* sdDeclAOR::DefaultDefinition( void ) const {
	return						\
			"{\n"				\
			"}\n";
}

/*
================
sdDeclAOR::Parse
================
*/
bool sdDeclAOR::Parse( const char* text, const int textLength ) {
	idToken token;
	idLexer src;

	src.SetFlags( DECL_LEXER_FLAGS );
	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );

	src.SkipUntilString( "{" );

	Clear();

	idDict temp;
	float spreadEnd = 0.0f;
	while( true ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}

		//
		// Packet spreading parms
		//
		if ( !token.Icmp( "spreadStart" ) ) {
			bool error;
			spreadStart = src.ParseFloat( &error );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'spreadStart'" );
				return false;
			}
			if ( spreadStart < 0.0f ) {
				src.Warning( "sdDeclAOR::Parse - spreadStart < 0" );
				spreadStart = 0.0f;
			}
		} else if ( !token.Icmp( "spreadEnd" ) ) {
			bool error;
			spreadEnd = src.ParseFloat( &error );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'spreadEnd'" );
				return false;
			}
			if ( spreadEnd < spreadStart ) {
				src.Warning( "sdDeclAOR::Parse - spreadEnd < spreadStar" );
			}
		} else if ( !token.Icmp( "spreadScale" ) ) {
			bool error;
			spreadScale = src.ParseFloat( &error );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'spreadScale'" );
				return false;
			}
		}
		//
		// Cutoff distances
		//
		else if ( !token.Icmp( "userCommandCutoff" ) ) {
			bool error;
			userCommandCutoffSqr = Square( src.ParseFloat( &error ) );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'userCommandCutoff'" );
				return false;
			}
		} else if ( !token.Icmp( "animationCutoff" ) ) {
			bool error;
			baseCutoffs.animationCutoffSqr = Square( src.ParseFloat( &error ) );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'animationCutoff'" );
				return false;
			}
		} else if ( !token.Icmp( "ikCutoff" ) ) {
			bool error;
			baseCutoffs.ikCutoffSqr = Square( src.ParseFloat( &error ) );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'ikCutoff'" );
				return false;
			}
		} else if ( !token.Icmp( "physicsCutoff" ) ) {
			bool error;
			baseCutoffs.physicsCutoffSqr = Square( src.ParseFloat( &error ) );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'physicsCutoff'" );
				return false;
			}
		}
		//
		// Prediction error decay distances
		//
		else if ( !token.Icmp( "boxDecayClipStart" ) ) {
			bool error;
			baseCutoffs.boxDecayClipStartSqr = Square( src.ParseFloat( &error ) );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'boxDecayClipStart'" );
				return false;
			}
		} else if ( !token.Icmp( "pointDecayClipStart" ) ) {
			bool error;
			baseCutoffs.pointDecayClipStartSqr = Square( src.ParseFloat( &error ) );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'pointDecayClipStart'" );
				return false;
			}
		} else if ( !token.Icmp( "heightMapDecayClipStart" ) ) {
			bool error;
			baseCutoffs.heightMapDecayClipStartSqr = Square( src.ParseFloat( &error ) );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'heightMapDecayClipStart'" );
				return false;
			}
		} else if ( !token.Icmp( "decayClipEnd" ) ) {
			bool error;
			baseCutoffs.decayClipEndSqr = Square( src.ParseFloat( &error ) );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'decayClipEndSqr'" );
				return false;
			}
		}
		//
		// Physics LOD start distances
		//
		else if ( !token.Icmp( "physicsLOD1Start" ) ) {
			bool error;
			baseCutoffs.physicsLOD1StartSqr = Square( src.ParseFloat( &error ) );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'physicsLOD1Start'" );
				return false;
			}
		} else if ( !token.Icmp( "physicsLOD2Start" ) ) {
			bool error;
			baseCutoffs.physicsLOD2StartSqr = Square( src.ParseFloat( &error ) );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'physicsLOD2Start'" );
				return false;
			}
		} else if ( !token.Icmp( "physicsLOD3Start" ) ) {
			bool error;
			baseCutoffs.physicsLOD3StartSqr = Square( src.ParseFloat( &error ) );

			if ( error ) {
				src.Error( "sdDeclAOR::Parse Invalid Parms for 'physicsLOD3Start'" );
				return false;
			}
		} else if( !token.Cmp( "}" ) ) {
			break;
		} else {
			src.Error( "sdDeclAOR::Parse Invalid Token %s", token.c_str() );
			return false;
		}
	}

	spreadDistance = spreadEnd - spreadStart;
	if ( spreadDistance < idMath::FLT_EPSILON ) {
		spreadDistance = 0.0f;
	}

	UpdateCutoffs();

	return true;
}

/*
================
sdDeclAOR::GetSpreadForDistanceSqr
================
*/
float sdDeclAOR::GetSpreadForDistanceSqr( float distanceSqr ) const {

	// Comparison with zero valid here as it clamps to 0 if under FLT_EPSILON in Parse()
	if ( spreadDistance == 0.0f ) {
		return 0.0f;
	}

	const float distance = idMath::Sqrt( distanceSqr );

	// calculate the spread time for this distance
	// hooray for magic numbers! this basically makes a nice ramping curve
	// it was generated by matching a cubic graph to a curve generated with
	// the function: spreadValue = sin( 0.5*pi*distFraction^0.25)^5
	const float A = 1.2092303f;
	const float B = -3.2394898f;
	const float C = 3.0424027f;
	const float D = -0.0031071f;
	float distFraction = ( distance - spreadStart ) / spreadDistance;
	float spreadValue = ( ( A*distFraction + B )* distFraction + C )* distFraction + D;
	spreadValue *= spreadScale;

	spreadValue = idMath::ClampFloat( 0.0f, 0.5f, spreadValue );

	return spreadValue;
}

/*
================
sdDeclAOR::GetFlagsForDistanceSqr
================
*/
int sdDeclAOR::GetFlagsForDistanceSqr( float distanceSqr ) const {
	int flags = 0;
	if ( distanceSqr > userCommandCutoffSqr ) {
		flags |= AOR_INHIBIT_USERCMDS;
	}

	if ( distanceSqr > activeCutoffs.animationCutoffSqr ) {
		flags |= AOR_INHIBIT_ANIMATION;
	}
	if ( distanceSqr > activeCutoffs.ikCutoffSqr ) {
		flags |= AOR_INHIBIT_IK;
	}
	if ( distanceSqr > activeCutoffs.physicsCutoffSqr ) {
		flags |= AOR_INHIBIT_PHYSICS;
	}

	if ( distanceSqr < activeCutoffs.decayClipEndSqr ) {
		if ( distanceSqr > activeCutoffs.heightMapDecayClipStartSqr ) {
			flags |= AOR_HEIGHTMAP_DECAY_CLIP;
		} else if ( distanceSqr > activeCutoffs.pointDecayClipStartSqr ) {
			flags |= AOR_POINT_DECAY_CLIP;
		} else if ( distanceSqr > activeCutoffs.boxDecayClipStartSqr ) {
			flags |= AOR_BOX_DECAY_CLIP;
		}
	}

	// no physics lodding on the server
	if ( gameLocal.isClient || sdDemoManager::GetInstance().InPlayBack() ) {
		if ( distanceSqr > activeCutoffs.physicsLOD3StartSqr ) {
			flags |= AOR_PHYSICS_LOD_3;
		} else if ( distanceSqr > activeCutoffs.physicsLOD2StartSqr ) {
			flags |= AOR_PHYSICS_LOD_2;
		} else if ( distanceSqr > activeCutoffs.physicsLOD1StartSqr ) {
			flags |= AOR_PHYSICS_LOD_1;
		}
	}

	return flags;
}

/*
================
sdDeclAOR::UpdateCutoffs
================
*/
void sdDeclAOR::UpdateCutoffs( void ) const {
	activeCutoffs.animationCutoffSqr			= baseCutoffs.animationCutoffSqr;
	if ( activeCutoffs.animationCutoffSqr < idMath::INFINITY ) {
		activeCutoffs.animationCutoffSqr *= aor_animationCutoffScale.GetFloat();
	}

	activeCutoffs.ikCutoffSqr					= baseCutoffs.ikCutoffSqr;
	if ( activeCutoffs.ikCutoffSqr < idMath::INFINITY ) {
		activeCutoffs.ikCutoffSqr *= aor_ikCutoffScale.GetFloat();
	}

	activeCutoffs.physicsCutoffSqr				= baseCutoffs.physicsCutoffSqr;
	if ( activeCutoffs.physicsCutoffSqr < idMath::INFINITY ) {
		 activeCutoffs.physicsCutoffSqr *= Square( aor_physicsCutoffScale.GetFloat() );
	}

	activeCutoffs.boxDecayClipStartSqr			= baseCutoffs.boxDecayClipStartSqr;
	activeCutoffs.pointDecayClipStartSqr		= baseCutoffs.pointDecayClipStartSqr;
	activeCutoffs.heightMapDecayClipStartSqr	= baseCutoffs.heightMapDecayClipStartSqr;
	activeCutoffs.decayClipEndSqr				= baseCutoffs.decayClipEndSqr;

	activeCutoffs.physicsLOD1StartSqr			= baseCutoffs.physicsLOD1StartSqr;
	if ( activeCutoffs.physicsLOD1StartSqr < idMath::INFINITY ) {
		activeCutoffs.physicsLOD1StartSqr *= Square( aor_physicsLod1StartScale.GetFloat() );
	}

	activeCutoffs.physicsLOD2StartSqr			= baseCutoffs.physicsLOD2StartSqr;
	if ( activeCutoffs.physicsLOD2StartSqr < idMath::INFINITY ) {
		activeCutoffs.physicsLOD2StartSqr *= Square( aor_physicsLod2StartScale.GetFloat() );
	}

	activeCutoffs.physicsLOD3StartSqr			= baseCutoffs.physicsLOD3StartSqr;
	if ( activeCutoffs.physicsLOD3StartSqr < idMath::INFINITY ) {
		activeCutoffs.physicsLOD3StartSqr *= Square( aor_physicsLod3StartScale.GetFloat() );
	}
}

class sdAORCutoffCVarCallback : public idCVarCallback {
public:
	sdAORCutoffCVarCallback( idCVar& _cvar ) {
		cvar = &_cvar;
	}

	virtual void OnChanged( void ) {
		sdDeclAOR::OnCutoffChanged();
	}

	void Register( void ) {
		cvar->RegisterCallback( this );
	}

	void UnRegister( void ) {
		cvar->UnRegisterCallback( this );
	}

	idCVar* cvar;
};

sdAORCutoffCVarCallback	aor_physicsCutoffScale_Callback( aor_physicsCutoffScale );
sdAORCutoffCVarCallback	aor_animationCutoffScale_Callback( aor_animationCutoffScale );
sdAORCutoffCVarCallback	aor_ikCutoffScale_Callback( aor_ikCutoffScale );
sdAORCutoffCVarCallback	aor_physicsLod1StartScale_Callback( aor_physicsLod1StartScale );
sdAORCutoffCVarCallback	aor_physicsLod2StartScale_Callback( aor_physicsLod2StartScale );
sdAORCutoffCVarCallback	aor_physicsLod3StartScale_Callback( aor_physicsLod3StartScale );

/*
================
sdDeclAOR::InitCVars
================
*/
void sdDeclAOR::InitCVars( void ) {
	aor_physicsCutoffScale_Callback.Register();
	aor_animationCutoffScale_Callback.Register();
	aor_ikCutoffScale_Callback.Register();

	aor_physicsLod1StartScale_Callback.Register();
	aor_physicsLod2StartScale_Callback.Register();
	aor_physicsLod3StartScale_Callback.Register();
}

/*
================
sdDeclAOR::ShutdownCVars
================
*/
void sdDeclAOR::ShutdownCVars( void ) {
	aor_physicsCutoffScale_Callback.UnRegister();
	aor_animationCutoffScale_Callback.UnRegister();
	aor_ikCutoffScale_Callback.UnRegister();

	aor_physicsLod1StartScale_Callback.UnRegister();
	aor_physicsLod2StartScale_Callback.UnRegister();
	aor_physicsLod3StartScale_Callback.UnRegister();
}

/*
================
sdDeclAOR::OnCutoffChanged
================
*/
void sdDeclAOR::OnCutoffChanged( void ) {
	for ( int i = 0; i < gameLocal.declAORType.Num(); i++ ) {
		const sdDeclAOR* aor = gameLocal.declAORType.LocalFindByIndex( i, false );
		if ( aor == NULL || aor->GetState() != DS_PARSED ) {
			continue;
		}

		aor->UpdateCutoffs();
	}
}
