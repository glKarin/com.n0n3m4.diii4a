/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

#include "renderer/tr_local.h"
#include "game/Grabber.h"


idCVar r_subviewMaxDepth(
	"r_subviewMaxDepth", "4", CVAR_RENDERER | CVAR_INTEGER,
	"How many nested subviews to generate at most (#6434).\n"
	"For instance: value = 1 means that direct mirrors and remotes are rendered, but all subviews nested inside them are black."
);
idCVar r_remoteLimitResolutionByScreenSize(
	"r_remoteLimitResolutionByScreenSize", "1.0", CVAR_RENDERER | CVAR_FLOAT,
	"Reduce resolution of remote subview when it is small on screen.\n"
	"Size ratio (screen pixel / remote pixel) does not exceed X.\n"
	"Value X = 0 disable this optimization.",
	0.0f, 10.0f
);


typedef struct {
	idVec3		origin;
	idMat3		axis;
} orientation_t;

/*
=================
R_MirrorPoint
=================
*/
static void R_MirrorPoint( const idVec3 in, orientation_t *surface, orientation_t *camera, idVec3 &out ) {
	int		i;
	idVec3	local;
	idVec3	transformed;
	float	d;

	local = in - surface->origin;

	transformed = vec3_origin;
	for ( i = 0 ; i < 3 ; i++ ) {
		d = local * surface->axis[i];
		transformed += d * camera->axis[i];
	}

	out = transformed + camera->origin;
}

/*
=================
R_MirrorVector
=================
*/
static void R_MirrorVector( const idVec3 in, orientation_t *surface, orientation_t *camera, idVec3 &out ) {
	int		i;
	float	d;

	out = vec3_origin;
	for ( i = 0 ; i < 3 ; i++ ) {
		d = in * surface->axis[i];
		out += d * camera->axis[i];
	}
}

/*
=============
R_EstimatePositionByTexcoordDerivsOfSurface

Computes derivatives of 3D position by texcoord.
We combine them with "pixels per doom-unit" kind of resolution to known resolution in pixels.
=============
*/
static idVec2 R_EstimatePositionByTexcoordDerivsOfSurface ( const srfTriangles_t *tri ) {
	idVec2 maxDerivs;
	maxDerivs.Zero();

	for ( int i = 0; i < tri->numIndexes / 3; i++ ) {
		const idDrawVert &v0 = tri->verts[tri->indexes[3 * i + 0]];
		const idDrawVert &v1 = tri->verts[tri->indexes[3 * i + 1]];
		const idDrawVert &v2 = tri->verts[tri->indexes[3 * i + 2]];

		idMat2 dt( v1.st - v0.st, v2.st - v0.st );
		dt.TransposeSelf();
		dt.InverseSelf();

		idVec3 dp01 = v1.xyz - v0.xyz;
		idVec3 dp02 = v2.xyz - v0.xyz;

		idVec3 derivS = dt[0][0] * dp01 + dt[1][0] * dp02;
		idVec3 derivT = dt[0][1] * dp01 + dt[1][1] * dp02;

		maxDerivs.x = idMath::Fmax( maxDerivs.x, derivS.LengthSqr() );
		maxDerivs.y = idMath::Fmax( maxDerivs.y, derivT.LengthSqr() );
	}
	maxDerivs.x = idMath::Sqrt( maxDerivs.x );
	maxDerivs.y = idMath::Sqrt( maxDerivs.y );

	return maxDerivs;
}

/*
=============
R_PlaneForSurface

Returns the plane for the first triangle in the surface
FIXME: check for degenerate triangle?
=============
*/
static void R_PlaneForSurface( const srfTriangles_t *tri, idPlane &plane ) {
	idDrawVert		*v1, *v2, *v3;

	v1 = tri->verts + tri->indexes[0];
	v2 = tri->verts + tri->indexes[1];
	v3 = tri->verts + tri->indexes[2];
	plane.FromPoints( v1->xyz, v2->xyz, v3->xyz );
}

/*
=========================
R_PreciseCullSurface

Check the surface for visibility on a per-triangle basis
for cases when it is going to be VERY expensive to draw (subviews)

If not culled, also returns the bounding box of the surface in 
Normalized Device Coordinates, so it can be used to crop the scissor rect.

OPTIMIZE: we could also take exact portal passing into consideration
=========================
*/
bool R_PreciseCullSurface( const drawSurf_t *drawSurf, idBounds &ndcBounds ) {
	const srfTriangles_t *tri;
    int numTriangles;
	idPlane clip, eye;
	unsigned int pointOr;
	unsigned int pointAnd;
	idVec3 localView;
	idFixedWinding w;

	tri = drawSurf->frontendGeo;

	pointOr = 0;
	pointAnd = (unsigned int)~0;

	// get an exact bounds of the triangles for scissor cropping
	ndcBounds.Clear();

	for ( int i = 0; i < tri->numVerts; i++ ) {
		unsigned int pointFlags;

		R_TransformModelToClip( tri->verts[i].xyz, drawSurf->space->modelViewMatrix,
			tr.viewDef->projectionMatrix, eye, clip );

		pointFlags = 0;
		for ( int j = 0; j < 3; j++ ) {
			if ( clip[j] >= clip[3] ) {
				pointFlags |= (1 << (j*2));
			} else if ( clip[j] <= -clip[3] ) {
				pointFlags |= ( 1 << (j*2+1));
			}
		}

		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if ( pointAnd ) {
		return true;
	}

	// backface and frustum cull
    numTriangles = tri->numIndexes / 3;

	R_GlobalPointToLocal( drawSurf->space->modelMatrix, tr.viewDef->renderView.vieworg, localView );

	for ( int i = 0; i < tri->numIndexes; i += 3 ) {
		idVec3	dir, normal;
		float	dot;
		idVec3	d1, d2;

		const idVec3 &v1 = tri->verts[tri->indexes[i]].xyz;
		const idVec3 &v2 = tri->verts[tri->indexes[i+1]].xyz;
		const idVec3 &v3 = tri->verts[tri->indexes[i+2]].xyz;

		// this is a hack, because R_GlobalPointToLocal doesn't work with the non-normalized
		// axis that we get from the gui view transform.  It doesn't hurt anything, because
		// we know that all gui generated surfaces are front facing
		if ( tr.guiRecursionLevel == 0 ) {
			// we don't care that it isn't normalized,
			// all we want is the sign
			d1 = v2 - v1;
			d2 = v3 - v1;
			normal = d2.Cross( d1 );

			dir = v1 - localView;

			dot = normal * dir;
			if ( dot >= 0.0f ) {
				return true;
			}
		}

		// now find the exact screen bounds of the clipped triangle
		w.SetNumPoints( 3 );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v1, w[0].ToVec3() );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v2, w[1].ToVec3() );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v3, w[2].ToVec3() );
		w[0].s = w[0].t = w[1].s = w[1].t = w[2].s = w[2].t = 0.0f;

		for ( int j = 0; j < 4; j++ ) {
			if ( !w.ClipInPlace( -tr.viewDef->frustum[j], 0.1f ) ) {
				break;
			}
		}
		for ( int j = 0; j < w.GetNumPoints(); j++ ) {
			idVec3	screen;

			R_GlobalToNormalizedDeviceCoordinates( w[j].ToVec3(), screen );
			ndcBounds.AddPoint( screen );
		}
	}

	// if we don't enclose any area, return
	if ( ndcBounds.IsCleared() ) {
		return true;
	}

	return false;
}

/*
========================
R_MirrorViewBySurface
========================
*/
static viewDef_t *R_MirrorViewBySurface( drawSurf_t *drawSurf ) {
	viewDef_t		*parms;
	orientation_t	surface, camera;
	idPlane			originalPlane, plane;

	// copy the viewport size from the original
	parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;
	parms->renderView.viewID = VID_SUBVIEW;	// clear to allow player bodies to show up, and suppress view weapons
	parms->unlockedRenderView = nullptr;

	parms->isSubview = true;
	parms->isMirrorGen = true;
	parms->isMirrorInverted = true;
	parms->isPortalSky = false;
	parms->isXray = false;
	parms->xrayEntityMask = XR_IGNORE;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->frontendGeo, originalPlane );
	R_LocalPlaneToGlobal( drawSurf->space->modelMatrix, originalPlane, plane );

	surface.origin = plane.Normal() * -plane[3];
	surface.axis[0] = plane.Normal();
	surface.axis[0].NormalVectors( surface.axis[1], surface.axis[2] );
	surface.axis[2] = -surface.axis[2];

	camera.origin = surface.origin;
	camera.axis[0] = -surface.axis[0];
	camera.axis[1] = surface.axis[1];
	camera.axis[2] = surface.axis[2];

	// set the mirrored origin and axis
	R_MirrorPoint( tr.viewDef->renderView.vieworg, &surface, &camera, parms->renderView.vieworg );

	R_MirrorVector( tr.viewDef->renderView.viewaxis[0], &surface, &camera, parms->renderView.viewaxis[0] );
	R_MirrorVector( tr.viewDef->renderView.viewaxis[1], &surface, &camera, parms->renderView.viewaxis[1] );
	R_MirrorVector( tr.viewDef->renderView.viewaxis[2], &surface, &camera, parms->renderView.viewaxis[2] );

	// make the view origin 16 units away from the center of the surface
	idVec3	viewOrigin = (drawSurf->frontendGeo->bounds[0] + drawSurf->frontendGeo->bounds[1]) * 0.5;
	viewOrigin += ( originalPlane.Normal() * 16 );

	R_LocalPointToGlobal( drawSurf->space->modelMatrix, viewOrigin, parms->initialViewAreaOrigin );

	// set the mirror clip plane
	parms->numClipPlanes = 1;
	parms->clipPlane[0] = plane;

	return parms;
}

/*
========================
R_XrayViewBySurface
========================
*/
static viewDef_t *R_XrayView() {
	viewDef_t		*parms;

	// copy the viewport size from the original
	parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;
	parms->renderView.viewID = VID_PLAYER_VIEW;	// clear to allow player bodies to show up, and suppress view weapons
	parms->unlockedRenderView = nullptr;

	parms->isXray = true;
	parms->isSubview = true;
	parms->isPortalSky = false;
	parms->isMirrorGen = false;
	parms->xrayEntityMask = XR_ONLY;

	return parms;
}

/*
===============
R_RemoteRender
===============
*/
static void R_RemoteRender( drawSurf_t *surf, textureStage_t *stage, const idBounds *ndcBounds ) {
	// if the entity doesn't have a remoteRenderView, do nothing
	if ( !surf->space->entityDef->parms.remoteRenderView ) 
		return;

	TRACE_CPU_SCOPE( "R_RemoteRender" );

	// copy the viewport size from the original
	viewDef_t* parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;
	parms->unlockedRenderView = nullptr;

	parms->isSubview = true;
	parms->isMirrorGen = false;
	parms->isMirrorInverted = false;
	parms->isPortalSky = false;
	parms->isXray = false;
	// if we see remote screen in mirror, drop mirror's clip plane
	parms->numClipPlanes = 0;
	parms->xrayEntityMask = XR_IGNORE;

	parms->renderView = *surf->space->entityDef->parms.remoteRenderView;
	parms->renderView.viewID = VID_SUBVIEW;	// clear to allow player bodies to show up, and suppress view weapons
	parms->initialViewAreaOrigin = parms->renderView.vieworg;

	idVec2 positionByTexcoordDerivs = R_EstimatePositionByTexcoordDerivsOfSurface( surf->frontendGeo );

	// #5485: if 3D resolution is set, multiply it by position/texcoord derivatives magnitude
	idVec2 resolution = idVec2( FLT_MAX, FLT_MAX );
	if ( stage->remoteResolutionWorld >= 0.0f ) {
		resolution = positionByTexcoordDerivs * stage->remoteResolutionWorld;
	}

	if ( r_remoteLimitResolutionByScreenSize.GetFloat() > 0.0 && ndcBounds ) {
		// estimate upper bound on "screen pixel to 3D distance" derivatives
		const idRenderMatrix &proj = tr.viewDef->projectionRenderMatrix;
		// note: R_GlobalToNormalizedDeviceCoordinates actually returns depth in [0..1]
		float minDepth = (*ndcBounds)[0].z;
		float minViewDistance = proj.DepthToZ( minDepth );	// viewZ negated!
		float minClipW = proj[3][3] - proj[3][2] * minViewDistance;
		idVec2 ndcMaxDeriv = idVec2( proj[0][0], proj[1][1] ) / minViewDistance;
		int cropW, cropH;
		tr.GetCurrentRenderCropSize( cropW, cropH );
		idVec2 pixelMaxDeriv = ndcMaxDeriv * 0.5f;
		pixelMaxDeriv.MulCW( idVec2( cropW, cropH ) );
		// limit remote subview resolution by our current screen resolution
		idVec2 pixelByTexcoordDeriv = idMath::Fmax( pixelMaxDeriv[0], pixelMaxDeriv[1] ) * positionByTexcoordDerivs;
		idVec2 capResolution = pixelByTexcoordDeriv * r_remoteLimitResolutionByScreenSize.GetFloat();
		if ( idMath::Fmax( capResolution.x, capResolution.y ) < (1 << 16)) {
			// round vertical resolution to power-of-two so that resolution does not change all the time on move
			// constant changes in remote resolution increase aliasing effects
			int vertPot = idMath::CeilPowerOfTwo( int( capResolution.y ) );
			capResolution *= vertPot / capResolution.x;
			resolution.MinCW( capResolution );
		}
	}

	// limit effective resolution by the explicit numbers
	resolution.Clamp( idVec2( 1, 1 ), idVec2( stage->remoteWidth, stage->remoteHeight ) );
	int resW = (int) idMath::Ceil( resolution.x );
	int resH = (int) idMath::Ceil( resolution.y );

	tr.CropRenderSize( resW, resH, false, true );

	parms->renderView.x = 0;
	parms->renderView.y = 0;
	parms->renderView.width = SCREEN_WIDTH;
	parms->renderView.height = SCREEN_HEIGHT;

	tr.RenderViewToViewport( parms->renderView, parms->viewport );

	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// generate render commands for it
	R_RenderView(*parms);

	// copy this rendering to the image
	stage->image = nullptr;
	idImageScratch *outputTexture = tr.CreateImageForSubview();
	tr.CaptureRenderToImage( *outputTexture );
	surf->dynamicImageOverride = outputTexture;

	tr.UnCrop();
}

/*
=================
R_MirrorRender
=================
*/
void R_MirrorRender( drawSurf_t *surf, textureStage_t *stage, idScreenRect& scissor ) {
	// issue a new view command
	viewDef_t *parms = R_MirrorViewBySurface( surf );
	if ( !parms ) {
		return;
	}

	TRACE_CPU_SCOPE( "R_MirrorRender" );

	// #5485: find the 'root' view to read main resolution from
	// it is usually the main view, but it can be remote subview as well
	int rootW = 0, rootH = 0;
	for ( viewDef_t *view = tr.viewDef; view; view = view->superView )
		if ( !view->isMirrorGen ) {
			rootW = view->viewport.GetWidth();
			rootH = view->viewport.GetHeight();
			break;
		}
	// compute resolution based on this subview parameters
	float resolutionFactor = stage->mirrorResolutionFactor;
	int thisW = (int) idMath::Ceil( resolutionFactor * rootW );
	int thisH = (int) idMath::Ceil( resolutionFactor * rootH );
	// but never make it more detailed than the current view
	// so, full-resolution mirror in low-res water surface will still be rendered low-res
	int cropW, cropH;
	tr.GetCurrentRenderCropSize( cropW, cropH );
	thisW = idMath::Imin( thisW, cropW );
	thisH = idMath::Imin( thisH, cropH );

	tr.CropRenderSize( thisW, thisH, false, true );

	parms->renderView.x = 0;
	parms->renderView.y = 0;
	parms->renderView.width = SCREEN_WIDTH;
	parms->renderView.height = SCREEN_HEIGHT;

	tr.RenderViewToViewport( parms->renderView, parms->viewport );

	parms->scissor.x1 = scissor.x1 * thisW / cropW;
	parms->scissor.y1 = scissor.y1 * thisH / cropH;
	parms->scissor.x2 = scissor.x2 * thisW / cropW;
	parms->scissor.y2 = scissor.y2 * thisH / cropH;

	renderCrop_t copyScissor;
	copyScissor.x = parms->scissor.x1;
	copyScissor.y = parms->scissor.y1;
	copyScissor.width = parms->scissor.GetWidth();
	copyScissor.height = parms->scissor.GetHeight();

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// triangle culling order changes with mirroring
	parms->isMirrorInverted = (((int)parms->isMirrorInverted ^ (int)tr.viewDef->isMirrorInverted) != 0);

	// generate render commands for it
	R_RenderView( *parms );

	// copy this rendering to the image
	stage->image = nullptr;
	idImageScratch *outputTexture = tr.CreateImageForSubview();
	// TODO: optimize by copying only the scissor
	tr.CaptureRenderToImage( *outputTexture, &copyScissor );
	surf->dynamicImageOverride = outputTexture;

	tr.UnCrop();
}

/*
=================
R_PortalRender
=================
*/
void R_PortalRender() {
	TRACE_CPU_SCOPE( "R_PortalRender" );

	viewDef_t		*parms;
	parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;
	parms->renderView.viewID = VID_SUBVIEW;
	parms->unlockedRenderView = nullptr;

	parms->numClipPlanes = 0;
	parms->superView = tr.viewDef;
	parms->subviewSurface = nullptr;
	parms->isPortalSky = true;
	parms->isMirrorGen = false;

	parms->renderView.viewaxis = parms->renderView.viewaxis * gameLocal.GetLocalPlayer()->playerView.ShakeAxis();

	// grayman #3108 - contributed by neuro & 7318
	idVec3 diff, currentEyePos, PSOrigin, Zero;

	Zero.Zero();

	if ( (gameLocal.CheckGlobalPortalSky()) || (gameLocal.GetCurrentPortalSkyType() == PORTALSKY_LOCAL) ) {
		// in a case of a moving portalSky

		currentEyePos = parms->renderView.vieworg;

		if ( gameLocal.playerOldEyePos == Zero ) {
			// Initialize playerOldEyePos. This will only happen in one tick.
			gameLocal.playerOldEyePos = currentEyePos;
		}

		diff = (currentEyePos - gameLocal.playerOldEyePos) / gameLocal.portalSkyScale;
		gameLocal.portalSkyGlobalOrigin += diff; // This is for the global portalSky.
		// It should keep going even when not active.
	}

		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_STANDARD ) {
			PSOrigin = gameLocal.portalSkyOrigin;
		}

		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_GLOBAL ) {
			PSOrigin = gameLocal.portalSkyGlobalOrigin;
		}

		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_LOCAL ) {
			gameLocal.portalSkyOrigin += diff;
			PSOrigin = gameLocal.portalSkyOrigin;
		}

		gameLocal.playerOldEyePos = currentEyePos;
		// end neuro & 7318

		parms->renderView.vieworg = PSOrigin;	// grayman #3108 - contributed by neuro & 7318
		parms->renderView.viewaxis = tr.viewDef->renderView.viewaxis * gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetAxis();

		// set up viewport, adjusted for resolution and OpenGL style 0 at the bottom
		tr.RenderViewToViewport( parms->renderView, parms->viewport );

		if ( tr.viewDef->isMirrorInverted ) {
			parms->scissor = tr.viewDef->scissor; // mirror in an area that has sky, limit to mirror rect only
		} else {
			parms->scissor.x1 = 0;
			parms->scissor.y1 = 0;
			parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
			parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;
		}

		parms->isSubview = true;
		parms->initialViewAreaOrigin = parms->renderView.vieworg;
		parms->floatTime = parms->renderView.time * 0.001f;

		tr.frameShaderTime = parms->floatTime;

		idVec3	cross;
		cross = parms->renderView.viewaxis[1].Cross( parms->renderView.viewaxis[2] );
		if ( cross * parms->renderView.viewaxis[0] > 0 ) {
			parms->isMirrorInverted = false;
		} else {
			parms->isMirrorInverted = true;
		}

		R_RenderView( *parms );

	if ( g_enablePortalSky.GetInteger() == 1 ) {
		tr.CaptureRenderToImage( *globalImages->currentRenderImage );
	} else {
		// stgatilov: in this mode, next view should be rendered on top of skybox
		// thus don't clear color buffer when rendering the next view
		parms->outputColorIsBackground = true;
	}
}

/*
=================
R_XrayRender
=================
*/
void R_XrayRender( drawSurf_t *surf, textureStage_t *stage, idImageScratch **imageOverride ) {
	TRACE_CPU_SCOPE( "R_XrayRender" );

	// issue a new view command
	viewDef_t *parms = R_XrayView();
	assert( parms );

	if ( stage->xrayInclusive ) {
		parms->xrayEntityMask = XR_SUBSTITUTE;
	}

	//tr.CropRenderSize( stage->width, stage->height, true );

	parms->renderView.x = 0;
	parms->renderView.y = 0;
	parms->renderView.width = SCREEN_WIDTH;
	parms->renderView.height = SCREEN_HEIGHT;

	tr.RenderViewToViewport( parms->renderView, parms->viewport );

	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// triangle culling order changes with mirroring
	parms->isMirrorInverted = ( ( (int)parms->isMirrorInverted ^ (int)tr.viewDef->isMirrorInverted ) != 0 );

	// generate render commands for it
	R_RenderView( *parms );

	stage->image = nullptr;
	idImageScratch *outputTexture = tr.CreateImageForSubview();
	tr.CaptureRenderToImage( *outputTexture );
	if ( surf )
		surf->dynamicImageOverride = outputTexture;
	if ( imageOverride )
		*imageOverride = outputTexture;

	//tr.UnCrop();
}

/*
=================
R_Lightgem_Render
=================
*/
bool R_Lightgem_Render() {
	TRACE_CPU_SCOPE( "R_Lightgem_Render" );

	// copy the viewport size from the original
	auto &parms = *(viewDef_t *)R_FrameAlloc( sizeof( viewDef_t ) );
	parms = *tr.viewDef;
	parms.unlockedRenderView = nullptr;
	parms.isSubview = true;
	parms.isMirrorGen = false;

	// Get position for lg
	idEntity* lg = gameLocal.m_lightGem.m_LightgemSurface.GetEntity();
	// duzenko #4408 - this happens at map start if no game tics ran in background yet
	if ( !lg || lg->GetModelDefHandle() == -1 )
		return false;
	renderEntity_t* lgent = lg->GetRenderEntity();

	auto player = gameLocal.GetLocalPlayer();
	// don't render lightgem If player is hidden (i.e the whole player entity is actually hidden)
	if ( player->GetModelDefHandle() == -1 ) {
		return false;
	}

	const idVec3& Cam = player->GetEyePosition();
	idVec3 LGPos = player->GetPhysics()->GetOrigin();// Set the lightgem position to that of the player

	// Move the lightgem out a fraction along the leaning vector
	LGPos.x += (Cam.x - LGPos.x) * 0.3f + cv_lg_oxoffs.GetFloat(); 
	LGPos.y += (Cam.y - LGPos.y) * 0.3f + cv_lg_oyoffs.GetFloat();

	// Prevent lightgem from clipping into the floor while crouching
	if ( player->GetPlayerPhysics()->IsCrouching() ) {
		LGPos.z += 50.0f + cv_lg_ozoffs.GetFloat();
	} else {
		LGPos.z = Cam.z + cv_lg_ozoffs.GetFloat(); // Set the lightgem's Z-axis position to that of the player's eyes
	}

	lg->SetOrigin( LGPos ); // Move the lightgem testmodel to the players feet based on the eye position

	gameRenderWorld->UpdateEntityDef( lg->GetModelDefHandle(), lgent ); // Make sure the lg is in the updated position
	auto &lightgemRv = parms.renderView;
	// Tinkerton lightgem fix
	lightgemRv.vieworg = lg->GetPhysics()->GetOrigin();
	lightgemRv.width = SCREEN_WIDTH;
	lightgemRv.height = SCREEN_HEIGHT;
	lightgemRv.fov_x = lightgemRv.fov_y = DARKMOD_LG_RENDER_FOV;	// square, TODO: investigate lowering the value to increase performance on tall maps
	lightgemRv.x = lightgemRv.y = 0;
	lightgemRv.viewID = VID_LIGHTGEM;
	lightgemRv.viewaxis = idMat3(
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		-1.0f, 0.0f, 0.0f
	); 

	// Give the rv the current ambient light values (obsolete #5449)
	lightgemRv.shaderParms[2] = 0.0f;
	lightgemRv.shaderParms[3] = 0.0f;
	lightgemRv.shaderParms[4] = 0.0f;

	// angua: render view needs current time, otherwise it will be unable to see time-dependent changes in light shaders such as flickering torches
	lightgemRv.time = gameLocal.GetTime();
	static int lgSplit;
	if(lgSplit++ & 1)
		lightgemRv.viewaxis.TransposeSelf();

	// Make sure the player model is hidden in the lightgem renders
	renderEntity_t* prent = player->GetRenderEntity();
	const int pdef = player->GetModelDefHandle();
	const int playerid = prent->suppressSurfaceInViewID;
	const int psid = prent->suppressShadowInViewID;
	prent->suppressShadowInViewID = VID_LIGHTGEM;
	prent->suppressSurfaceInViewID = VID_LIGHTGEM;

	// And the player's head 
	renderEntity_t* hrent = player->GetHeadEntity()->GetRenderEntity();
	const int hdef = player->GetHeadEntity()->GetModelDefHandle();
	const int headid = hrent->suppressSurfaceInViewID;
	const int hsid = hrent->suppressShadowInViewID;
	hrent->suppressShadowInViewID = VID_LIGHTGEM;
	hrent->suppressSurfaceInViewID = VID_LIGHTGEM;

	// Let the game know about the changes
	gameRenderWorld->UpdateEntityDef( pdef, prent );
	if ( hdef >= 0 )
		gameRenderWorld->UpdateEntityDef( hdef, hrent );

	// Currently grabbed entities should not cast a shadow on the lightgem to avoid exploits
	int heldDef = 0;
	int heldSurfID = 0;
	int heldShadID = 0;
	renderEntity_t *heldRE = NULL;
	idEntity *heldEnt = gameLocal.m_Grabber->GetSelected();
	if ( heldEnt ) {
		heldDef = heldEnt->GetModelDefHandle();

		// tels: #3286: Only update the entityDef if it is valid
		if ( heldDef >= 0 )
		{
			heldRE = heldEnt->GetRenderEntity();
			heldSurfID = heldRE->suppressSurfaceInViewID;
			heldShadID = heldRE->suppressShadowInViewID;
			heldRE->suppressShadowInViewID = VID_LIGHTGEM;
			heldRE->suppressSurfaceInViewID = VID_LIGHTGEM;
			gameRenderWorld->UpdateEntityDef( heldDef, heldRE );
		}
	}

	tr.RenderViewToViewport( parms.renderView, parms.viewport );

	parms.superView = tr.viewDef;

	// generate render commands for it
	emptyCommand_t *lastCmd = frameData->cmdTail;
	R_RenderView( parms );
	drawSurfsCommand_t *noppedCmd = (drawSurfsCommand_t *)frameData->cmdTail;
	assert(noppedCmd->commandId == RC_DRAW_VIEW);
	assert(lastCmd->next == noppedCmd);

	// hack: we replace the drawview command with our own lightgem draw command
	//noppedCmd->commandId = RC_NOP;
	frameData->cmdTail = lastCmd;
	drawLightgemCommand_t *newCmd = (drawLightgemCommand_t *)R_GetCommandBuffer( sizeof(drawLightgemCommand_t) );
	newCmd->commandId = RC_DRAW_LIGHTGEM;
	newCmd->viewDef = noppedCmd->viewDef;
	// the frontend buffer has already been analyzed this frame and will become the backend buffer in the next frame
	newCmd->dataBuffer = gameLocal.m_lightGem.m_LightgemImgBufferFrontend;

	// and switch back our normal render definition - player model and head are returned
	prent->suppressSurfaceInViewID = playerid;
	prent->suppressShadowInViewID = psid;
	hrent->suppressSurfaceInViewID = headid;
	hrent->suppressShadowInViewID = hsid;
	gameRenderWorld->UpdateEntityDef( pdef, prent );
	if ( hdef >= 0 )
		gameRenderWorld->UpdateEntityDef( hdef, hrent );

	// switch back currently grabbed entity settings
	if ( heldEnt ) {
		// tels: #3286: Only update the entityDef if it is valid
		if ( heldDef >= 0 )
		{
			heldRE->suppressSurfaceInViewID = heldSurfID;
			heldRE->suppressShadowInViewID = heldShadID;
			gameRenderWorld->UpdateEntityDef( heldDef, heldRE );
		}
	}

	return true;
}

/*
==================
R_GenerateSurfaceSubview
==================
*/
bool	R_GenerateSurfaceSubview( drawSurf_t *drawSurf ) {
	idBounds		ndcBounds;
	const idMaterial		*shader;

	// for testing the performance hit
	if ( r_skipSubviews ) 
		return false;

	if ( R_PreciseCullSurface( drawSurf, ndcBounds ) ) 
		return false;

	shader = drawSurf->material;

	// crop the scissor bounds based on the precise cull
	idScreenRect	scissor;

	idScreenRect	*v = &tr.viewDef->viewport;
	scissor.x1 = v->x1 + (int)( (v->x2 - v->x1 + 1 ) * 0.5f * ( ndcBounds[0][0] + 1.0f ));
	scissor.y1 = v->y1 + (int)( (v->y2 - v->y1 + 1 ) * 0.5f * ( ndcBounds[0][1] + 1.0f ));
	scissor.x2 = v->x1 + (int)( (v->x2 - v->x1 + 1 ) * 0.5f * ( ndcBounds[1][0] + 1.0f ));
	scissor.y2 = v->y1 + (int)( (v->y2 - v->y1 + 1 ) * 0.5f * ( ndcBounds[1][1] + 1.0f ));

	// nudge a bit for safety
	scissor.Expand();

	scissor.Intersect( tr.viewDef->scissor );

	if ( scissor.IsEmpty() ) // cropped out
		return false;

	// see what kind of subview we are making
	//if ( shader->GetSort() != SS_SUBVIEW ) {
		for ( int i = 0 ; i < shader->GetNumStages() ; i++ ) {
			const shaderStage_t	*stage = shader->GetStage( i );
			switch ( stage->texture.dynamic ) {
			case DI_REMOTE_RENDER:
				R_RemoteRender( drawSurf, const_cast<textureStage_t *>(&stage->texture), &ndcBounds );
				break;
			case DI_MIRROR_RENDER:
				R_MirrorRender( drawSurf, const_cast<textureStage_t *>(&stage->texture), scissor );
				break;
			case DI_XRAY_RENDER:
				R_XrayRender( drawSurf, const_cast<textureStage_t *>(&stage->texture), nullptr );
				break;
			case DI_PORTAL_RENDER:
				common->Error("Portal subview error");	// never happens, see R_GenerateSubViews
				R_PortalRender();
				break;
			}
		}
		return true;
	//}
}

/*
================
R_GenerateSubViews

If we need to render another view to complete the current view,
generate it first.

It is important to do this after all drawSurfs for the current
view have been generated, because it may create a subview which
would change tr.viewCount.
================
*/
bool R_GenerateSubViews( void ) {
	TRACE_CPU_SCOPE( "R_GenerateSubViews" )

	// for testing the performance hit
	if ( r_skipSubviews || tr.viewDef->areaNum < 0 ) 
		return false;

	// duzenko #4420: no mirrors on lightgem stage
	if ( tr.viewDef->IsLightGem() )
		return false;

	// nbohr1more: fix compass render error with Xray
	if (tr.viewDef->renderWorld->mapName.IsEmpty()) {
		return false;
	}

	bool subviews = false;

	extern idCVar cv_lg_interleave;								// FIXME a better way to check for RenderWindow views? (compass, etc)
	if ( !tr.viewDef->isSubview && cv_lg_interleave.GetBool() && !tr.viewDef->renderWorld->mapName.IsEmpty() ) {
		R_Lightgem_Render();
		subviews = true;
	}

	int depth = 0;
	bool isInsidePortalSky = false;
	for ( viewDef_s *view = tr.viewDef; view; view = view->superView ) {
		depth++;
		if ( view->isPortalSky )
			isInsidePortalSky = true;
	}
	if ( depth > r_subviewMaxDepth.GetInteger() )
		return false;

	// scan the surfaces until we either find a subview, or determine
	// there are no more subview surfaces.
	for ( int i = 0; i < tr.viewDef->numDrawSurfs; i++ ) {
		drawSurf_t *drawSurf = tr.viewDef->drawSurfs[i];
		const idMaterial *shader = drawSurf->material;

		if ( !shader || !shader->HasSubview() )
			continue;

		if ( shader->GetSort() == SS_PORTAL_SKY ) {
			// portal sky needs to be the last one and only once
			continue;
		}

		if ( R_GenerateSurfaceSubview( drawSurf ) ) {
			subviews = true;
		} else {
			// #6434: probably blocked due to limits: render as black image
			drawSurf->dynamicImageOverride = nullptr;
		}
	}

	// generate subviews for GUI overlays (from main view only)
	if ( !tr.viewDef->isSubview ) {
		// note: the image which will receive Xray subview is passed into idGuiModel::ProcessOverlaySubviews
		// that function will substitute virtual dynamic Xray texture with the actual scratch texture to display
		tr.xrayGuiImageOverride = nullptr;

		if ( const textureStage_t *textureStage = tr.viewDef->renderWorld->xrayGuiOverlayStage ) {
			idImageScratch *imageOverride;
			R_XrayRender( nullptr, const_cast<textureStage_t *>(textureStage), &imageOverride );
			tr.xrayGuiImageOverride = imageOverride;
			subviews = true;
		}
	}

	// generate portalsky only in main 3D view (not in compass), and never nest portalsky subviews in each other
	if ( !isInsidePortalSky && tr.viewDef->renderWorld == gameRenderWorld ) {
		if ( gameLocal.portalSkyEnt.GetEntity() && ( gameLocal.IsPortalSkyActive() || g_stopTime.GetBool() ) && g_enablePortalSky.GetBool() ) {
			R_PortalRender();
			subviews = true;
		}
	}

	return subviews;
}
