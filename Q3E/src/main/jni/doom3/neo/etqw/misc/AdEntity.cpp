// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "AdEntity.h"
#include "WorldToScreen.h"
#include "../Player.h"

/*
===============================================================================

	sdAdEntityCallback

===============================================================================
*/

/*
================
sdAdEntityCallback::OnImageLoaded
================
*/
void sdAdEntityCallback::OnImageLoaded( idImage* image ) {
	owner->OnImageLoaded( image );
}

/*
================
sdAdEntityCallback::OnDestroyed
================
*/
void sdAdEntityCallback::OnDestroyed( void ) {
	owner->OnAdDestroyed();
}

/*
================
sdAdEntityCallback::UpdateImpression
================
*/
void sdAdEntityCallback::UpdateImpression( impressionInfo_t& impression, const renderView_t& view, const sdBounds2D& viewPort ) {
	owner->UpdateImpression( impression, view, viewPort );
}

/*
===============================================================================

	sdAdEntity

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdAdEntity )
END_CLASS

/*
================
sdAdEntity::sdAdEntity
================
*/
sdAdEntity::sdAdEntity( void ) : adSurface( NULL ), adObject( NULL ) {
	adSurfaceNormal.Zero();

	adCallback.Init( this );

	memset( &lastImpression, 0, sizeof( lastImpression ) );
}

/*
================
sdAdEntity::~sdAdEntity
================
*/
sdAdEntity::~sdAdEntity( void ) {
	if ( adObject ) {
		adObject->Free();
	}
}

/*
================
sdAdEntity::Spawn
================
*/
void sdAdEntity::Spawn( void ) {
	if ( gameLocal.DoClientSideStuff() ) {
		if ( !renderEntity.hModel ) {
			gameLocal.Error( "sdAdEntity::Spawn No Model!" );
		}

		for ( int i = 0; i < renderEntity.hModel->NumSurfaces(); i++ ) {
			const modelSurface_t* surface = renderEntity.hModel->Surface( i );		
			if ( !surface->material->TestMaterialFlag( MF_ADVERT ) ) {
				continue;
			}

			adSurface = surface;
			break;
		}

		if ( !adSurface ) {
			gameLocal.Error( "sdAdEntity::Spawn No Ad Surface Found!" );
		}

		for ( int i = 0; i < adSurface->geometry->numIndexes / 3; i++ ) {
			idPlane& plane = adSurface->geometry->facePlanes[ i ];
			adSurfaceNormal += plane.Normal();
		}
		adSurfaceNormal.Normalize();

		const char* billboardName = spawnArgs.GetString( "billboard_name" );
		if ( !*billboardName ) {
			gameLocal.Error( "sdAdEntity::Spawn No Billboard Supplied" );
		}

		adObject = adManager->AllocAdSubscriber( billboardName, &adCallback );

//		BecomeActive( TH_THINK );
	}
}

/*
================
sdAdEntity::OnImageLoaded
================
*/
void sdAdEntity::OnImageLoaded( idImage* image ) {
	if ( !adSurface ) {
		return;
	}

	materialStage_t* stage = const_cast< materialStage_t* >( adSurface->material->GetStage( 0 ) );
	stage->textures[ 0 ].image = image;
}

/*
================
sdAdEntity::OnAdDestroyed
================
*/
void sdAdEntity::OnAdDestroyed( void ) {
	adObject = NULL;
}

/*
================
sdAdEntity::UpdateImpression
================
*/
void sdAdEntity::UpdateImpression( impressionInfo_t& impression, const renderView_t& view, const sdBounds2D& viewPort ) {
	assert( adSurface );

	sdWorldToScreenConverter converter( view );

	sdBounds2D	screenBounds;
	idVec2		screenCenter;

	const idBounds& surfaceBounds	= adSurface->geometry->bounds;
	idVec3 sufaceCenter				= renderEntity.origin + ( surfaceBounds.GetCenter() * renderEntity.axis );

	impression.screenWidth	= viewPort.GetWidth();
	impression.screenHeight	= viewPort.GetHeight();

	converter.SetExtents( idVec2( impression.screenWidth, impression.screenHeight ) );
	converter.Transform( surfaceBounds, renderEntity.axis, renderEntity.origin, screenBounds );
	converter.Transform( sufaceCenter, screenCenter );

	sdBounds2D clippedImpressionBounds;
	idFrustum f;
	f.SetAxis( view.viewaxis );
	f.SetOrigin( view.vieworg );
	float dNear = 0.0f;
	float dFar	= MAX_WORLD_SIZE;
	float dLeft = idMath::Tan( DEG2RAD( view.fov_x * 0.5f ) ) * dFar;
	float dUp	= idMath::Tan( DEG2RAD( view.fov_y * 0.5f ) ) * dFar;
	f.SetSize( dNear, dFar, dLeft, dUp );
	sdWorldToScreenConverter::TransformClipped( surfaceBounds, renderEntity.axis, renderEntity.origin, clippedImpressionBounds, f, idVec2( impression.screenWidth, impression.screenHeight ) );
	impression.size		= ( clippedImpressionBounds.GetMaxs() - clippedImpressionBounds.GetMins() ).Length();

	// Angle calculation	
	idVec3 objectNormal = adSurfaceNormal * renderEntity.axis;

	impression.angle	= objectNormal * -view.viewaxis[ 0 ];

	// Offscreen determination
	impression.inView	= true;

	// If the center point is off screen, and if either of the two bounding points are offscreen
	// then the entire object is considered to be offscreen
	if ( !viewPort.ContainsPoint( screenCenter ) ) {
		if ( !viewPort.ContainsPoint( screenBounds.GetMins() ) || !viewPort.ContainsPoint( screenBounds.GetMaxs() ) ) {
			impression.inView = false;
		}
	}

	lastImpression = impression;
}

/*
================
sdAdEntity::Think
================
*/
/*void sdAdEntity::Think( void ) {
	idEntity::Think();

	if ( adSurface ) {
		gameRenderWorld->DebugBounds( colorRed, adSurface->geometry->bounds, renderEntity.origin, renderEntity.axis );		

		idPlayer* localPlayer = gameLocal.GetLocalPlayer();
		if ( localPlayer ) {
			idVec3 org = renderEntity.origin + ( adSurface->geometry->bounds.GetCenter() * renderEntity.axis );
			gameRenderWorld->DrawText( va( "Angle: %f\n%s\nSize: %d", lastImpression.angle, lastImpression.inView ? "In View" : "Not In View", lastImpression.size ), org + idVec3( 0.f, 0.f, 64.f ), 0.25f, colorYellow, localPlayer->renderView.viewaxis );
		}
	}
}*/

/*
================
sdAdEntity::Damage
================
*/
void sdAdEntity::OnBulletImpact( idEntity* attacker, const trace_t& trace ) {
	if ( !gameLocal.DoClientSideStuff() ) {
		return;
	}

	if ( !adObject ) {
		return;
	}

	if ( attacker != gameLocal.GetLocalPlayer() ) {
		return;
	}

	adObject->Activate();
}
