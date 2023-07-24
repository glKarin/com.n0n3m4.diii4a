// Copyright (C) 2007 Id Software, Inc.
//

//----------------------------------------------------------------
// ClientEffect.cpp
//----------------------------------------------------------------

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "ClientEffect.h"

#include "../../bse/BSEInterface.h"
#include "../../bse/BSE_Envelope.h"
#include "../../bse/BSE_SpawnDomains.h"
#include "../../bse/BSE_Particle.h"
#include "../../bse/BSE.h"

#include "../Player.h"

/*
===============================================================================

rvClientEffect

===============================================================================
*/

const idEventDef EV_SetEffectEndOrigin( "setEffectEndOrigin", '\0', DOC_TEXT( "Sets the end point of the effect, for use with beam effects." ), 1, NULL, "v", "position", "Position to set as the end point, in world space." );
const idEventDef EV_SetEffectLooping( "setEffectLooping", '\0', DOC_TEXT( "Sets or clears the looping flag for the effect." ), 1, NULL, "b", "value", "Value to set the looping flag to." );
const idEventDef EV_UseRenderBounds( "useRenderBounds", '\0', DOC_TEXT( "Sets or clears the useRenderBounds flag for the effect." ), 1, NULL, "b", "value", "Value to set the useRenderBounds flag to." );
const idEventDef EV_EndEffect( "endEffect", '\0', DOC_TEXT( "Stops the effect, and optionally removes any existing particles." ), 1, NULL, "b", "killParticles", "Whether to remove existing particles or not." );	// called endEffect, not stopEffect due to confict with idEntity

CLASS_DECLARATION( sdClientScriptEntity, rvClientEffect )
	EVENT( EV_SetEffectEndOrigin,			rvClientEffect::Event_SetEffectEndOrigin )	
	EVENT( EV_SetEffectLooping,				rvClientEffect::Event_SetEffectLooping )
	EVENT( EV_UseRenderBounds,				rvClientEffect::Event_UseRenderBounds )
	EVENT( EV_EndEffect,					rvClientEffect::Event_EndEffect )
END_CLASS

/*
================
rvClientEffect::rvClientEffect
================
*/
rvClientEffect::rvClientEffect( void ) {
	Init ( -1 );
}

rvClientEffect::rvClientEffect( int _effectIndex ) {
	Init ( _effectIndex );
}

void rvClientEffect::Init( int _effectIndex ) {
	memset( &renderEffect, 0, sizeof( renderEffect ) );
	
	effectIndex		= _effectIndex;
	effectDefHandle = -1;
	startTime		= -1;
	endOriginJoint	= INVALID_JOINT;
	viewSuppress    = true;

	monitorSpawnId  = -1;
}

/*
================
rvClientEffect::~rvClientEffect
================
*/
rvClientEffect::~rvClientEffect( void ) {
	FreeEffectDef();
}

/*
================
rvClientEffect::FreeEffectDef
================
*/
void rvClientEffect::FreeEffectDef( void ) {
	if ( effectDefHandle != -1 && gameRenderWorld ) {
		gameRenderWorld->FreeEffectDef( effectDefHandle );
	}
	effectDefHandle = -1;
}

/*
================
rvClientEffect::UpdateBind
================
*/
void rvClientEffect::UpdateBind( bool skipModelUpdate ) {
	rvClientEntity::UpdateBind( skipModelUpdate );

	renderEffect.origin = worldOrigin;
	renderEffect.windVector = gameLocal.GetWindVector( worldOrigin );
	
	if ( endOriginJoint != INVALID_JOINT && bindMaster ) {
		idVec3 endOrigin;		
		idVec3 dir;

		bindMaster->GetWorldOrigin( endOriginJoint, endOrigin );
		SetEndOrigin ( endOrigin );
		
		dir = ( endOrigin - worldOrigin );
		dir.Normalize ();		
		renderEffect.axis = dir.ToMat3();
	} else {
		renderEffect.axis = worldAxis;
	}

	if ( bindMaster ) {
		if ( viewSuppress ) {
			renderEffect.allowSurfaceInViewID = bindMaster->GetRenderEntity()->allowSurfaceInViewID;
			renderEffect.suppressSurfaceInViewID = bindMaster->GetRenderEntity()->suppressSurfaceInViewID;
		} else {
			renderEffect.allowSurfaceInViewID = 0;
			renderEffect.suppressSurfaceInViewID = 0;
		}
		renderEffect.weaponDepthHackInViewID = bindMaster->GetRenderEntity()->flags.weaponDepthHack;
		renderEffect.modelDepthHack = bindMaster->GetRenderEntity()->modelDepthHack;

		renderEffect.groupID = bindMaster->entityNumber + 1;
	} else {
		renderEffect.groupID = 0;
	}
}

/*
================
rvClientEffect::Think
================
*/
void rvClientEffect::Think( void ) {
	// If there is a valid effect handle and we havent started playing
	// and effect yet then see if its time
	if( effectDefHandle < 0 && effectIndex >= 0 ) {
		if( startTime >= 0 ) {
			// Make sure our origins are all straight before starting the effect
			UpdateBind( false );
			renderEffect.declEffect = declHolder.FindEffectByIndex( effectIndex );
			renderEffect.attenuation = 1.0f;
			
			// Add the render effect
			effectDefHandle = gameRenderWorld->AddEffectDef( &renderEffect, startTime );
			if ( effectDefHandle < 0 ) {
				Dispose();
			}
		}
		
		return;
	} 

	if ( monitorSpawnId >= 0 ) {
		idEntity *ent = gameLocal.EntityForSpawnId( monitorSpawnId );
		if ( ent == NULL || ent->IsHidden() ) {
			Dispose();
			return;
		}
	}

	// If we lost our effect def handle then just remove ourself
	if( effectDefHandle < 0 ) {
		Dispose();
		return;
	}

	// Dont do anything else if its not a new client frame
	if( !gameLocal.isNewFrame ) {
		return;
	}

	// Update the bind
	if ( bindMaster.IsValid() && bindMaster->IsInterpolated() ) {
		UpdateBind( true );
	} else {
		UpdateBind( false );
	}

	// Update the actual render effect now
	if( gameRenderWorld->UpdateEffectDef( effectDefHandle, &renderEffect, gameLocal.time ) ) {
		FreeEffectDef();
		Dispose();
		return;
	}
}

/*
================
rvClientEffect::ClientUpdateView
================
*/
void rvClientEffect::ClientUpdateView( void ) {
	if ( effectDefHandle < 0 ) {
		return;
	}

	UpdateBind( true );
	gameRenderWorld->UpdateEffectDef( effectDefHandle, &renderEffect, gameLocal.time );
}

/*
================
rvClientEffect::Play
================
*/
bool rvClientEffect::Play( int _startTime, bool _loop, const idVec3& endOrigin ) {
	if ( effectIndex < 0 ) {
		return false;
	}

	// Initialize the render entity
	if ( bindMaster ) {
		renderEntity_t* renderEnt = bindMaster->GetRenderEntity ( );
		assert( renderEnt );
		
		// Copy suppress values from parent entity
		if ( viewSuppress ) {
			renderEffect.allowSurfaceInViewID = bindMaster->GetRenderEntity()->allowSurfaceInViewID;
			renderEffect.suppressSurfaceInViewID = bindMaster->GetRenderEntity()->suppressSurfaceInViewID;
		} else {
			renderEffect.allowSurfaceInViewID = 0;
			renderEffect.suppressSurfaceInViewID = 0;
		}
		renderEffect.weaponDepthHackInViewID = renderEnt->flags.weaponDepthHack;
  	}

	renderEffect.shaderParms[SHADERPARM_RED] = 1.0f;
	renderEffect.shaderParms[SHADERPARM_GREEN] = 1.0f;
	renderEffect.shaderParms[SHADERPARM_BLUE] = 1.0f;
	renderEffect.shaderParms[SHADERPARM_ALPHA] = 1.0f;
	renderEffect.shaderParms[SHADERPARM_BRIGHTNESS] = 1.0f; 
	renderEffect.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
	renderEffect.hasEndOrigin = ( endOrigin != vec3_origin );
	renderEffect.endOrigin	  = endOrigin;		
	renderEffect.loop		  = _loop;

	assert( effectDefHandle < 0 );

	startTime = _startTime;

	return true;	
}

/*
================
rvClientEffect::Stop
================
*/
void rvClientEffect::Stop ( bool destroyParticles ) {
	if( effectDefHandle < 0 ) {
		startTime = -1.0f;
		renderEffect.declEffect = NULL;

		FreeEffectDef();
		Dispose();
		return;
	}
	
	if ( destroyParticles ) {
		// Clear the effect index to make sure the effect isn't started again.  This is 
		// an indirect way of making the effect not think
		renderEffect.declEffect = NULL;
		effectIndex = -1;
		
		FreeEffectDef();
		Dispose();
	} else {
		gameRenderWorld->StopEffectDef( effectDefHandle );
		// this will ensure the effect doesn't re-up when loaded from a save.
		startTime = -1.0f;
		Unbind();
	}
}

/*
================
rvClientEffect::Restart
================
*/
void rvClientEffect::Restart ( void ) {
	FreeEffectDef();
	
	if ( renderEffect.loop ) {
		Play ( gameLocal.time, true, renderEffect.endOrigin );
	}
}

/*
================
rvClientEffect::Attenuate
================
*/
void rvClientEffect::Attenuate ( float attenuation ) {
	renderEffect.attenuation = attenuation;
}

/*
================
rvClientEffect::DrawDebugInfo
================
*/
void rvClientEffect::DrawDebugInfo ( void ) const {
	rvClientEntity::DrawDebugInfo();

	if ( !gameLocal.GetLocalPlayer() ) {	
		return;
	}
		
	if ( !renderEffect.declEffect ) {
		return;
	}

	idMat3 axis = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();
	idVec3 up = axis[ 2 ] * 5.0f;

	gameRenderWorld->DrawText ( declHolder.declEffectsType.LocalFindByIndex( effectIndex )->GetName(), worldOrigin + up, 0.1f, colorWhite, axis, 1 );
}

/*
============
rvClientEffect::Event_SetEffectEndOrigin
============
*/
void rvClientEffect::Event_SetEffectEndOrigin( const idVec3& endOrg ) {
	SetEndOrigin( endOrg );	
}


/*
============
rvClientEffect::Event_SetEffectLooping
============
*/
void rvClientEffect::Event_SetEffectLooping( bool looping ) {
	renderEffect.loop = looping;
}

/*
============
rvClientEffect::Event_UseRenderBounds
============
*/
void rvClientEffect::Event_UseRenderBounds( bool rb ) {
	renderEffect.useRenderBounds = rb;
}

/*
============
rvClientEffect::Event_EndEffect
============
*/
void rvClientEffect::Event_EndEffect( bool destroyParticles ) {
	Stop( destroyParticles );
}

/*
================
rvClientEffect::FreeEntityDef
================
*/
void rvClientEffect::FreeEntityDef( void ) {
	FreeEffectDef();
}

const char * rvClientEffect::GetName( void ) const {
	return va( "rvClientEffect %s", effectIndex != -1 ? declHolder.declEffectsType.LocalFindByIndex( effectIndex )->GetName() : "<NOT SET>");
}

void rvClientEffect::Monitor( idEntity *ent ) {
	if ( ent != NULL ) {
		monitorSpawnId = ( gameLocal.spawnIds[ ent->entityNumber ] << GENTITYNUM_BITS ) | ent->entityNumber;
	} else {
		monitorSpawnId = -1;
	}
}


/*
===============================================================================

rvClientCrawlEffect

===============================================================================
*/

CLASS_DECLARATION( rvClientEffect, rvClientCrawlEffect )
END_CLASS

/*
================
rvClientCrawlEffect::rvClientCrawlEffect
================
*/
rvClientCrawlEffect::rvClientCrawlEffect( void ) {
}

rvClientCrawlEffect::rvClientCrawlEffect( int _effectIndex, idEntity* ent, int _crawlTime, idList<jointHandle_t>* joints ) : rvClientEffect ( _effectIndex ) {
	int i;
	
	// Crawl effects require an animated entity
	crawlEnt = ent->Cast< idAnimatedEntity >();
	if ( !crawlEnt) {
		return;
	}
	
	// Specific joint list provided?
	if ( joints && joints->Num () ) {
		crawlJoints.Clear ( );
		for ( i = 0; i < joints->Num(); i ++ ) {
			crawlJoints.Append ( (*joints)[i] );
		}
	} else {
		// Use only parent joints and skip joint zero which is presumed to be the origin
		for ( i = ent->GetAnimator()->NumJoints() - 1; i > 0; i -- ) {
			if ( i != ent->GetAnimator()->GetFirstChild ( (jointHandle_t)i ) ) {
				crawlJoints.Append ( (jointHandle_t)i );
			}
		}
	}
	
	//no joints?  abort!
	if ( !crawlJoints.Num() ) {
		return;
	}

	jointStart = gameLocal.random.RandomInt( crawlJoints.Num() );
	crawlDir   = gameLocal.random.RandomInt( 2 ) > 0 ? 1 : -1;		
	jointEnd   = ( jointStart + crawlDir + crawlJoints.Num() ) % crawlJoints.Num();
	crawlTime  = _crawlTime;	
	nextCrawl  = gameLocal.time + crawlTime;
}

/*
================
rvClientCrawlEffect::Think
================
*/
void rvClientCrawlEffect::Think ( void ) {
	
	// If there is no crawl entity or no crawl joints then just free ourself
	if ( !crawlEnt || !crawlJoints.Num() ) {
		Dispose();
		return;
	}
			
	// Move to the next joint if its time
	if ( gameLocal.time > nextCrawl ) {
		jointStart = (jointStart + crawlDir + crawlJoints.Num() ) % crawlJoints.Num();
		jointEnd   = (jointStart + crawlDir + crawlJoints.Num() ) % crawlJoints.Num();
		nextCrawl  = gameLocal.time + crawlTime;
	}

	idVec3 offsetStart;
	idVec3 offsetEnd;
	idVec3 dir;
	idMat3 axis;
	
	// Get the start origin
	crawlEnt->GetJointWorldTransform( crawlJoints[jointStart], gameLocal.time, offsetStart, axis );
	SetOrigin( offsetStart );

	// Get the end origin
	crawlEnt->GetJointWorldTransform( crawlJoints[jointEnd], gameLocal.time, offsetEnd, axis );
	SetEndOrigin( offsetEnd );

	// Update the axis to point at the bone
	dir = offsetEnd - offsetStart;
	dir.Normalize();
	SetAxis( dir.ToMat3() );
		
	rvClientEffect::Think();
}


