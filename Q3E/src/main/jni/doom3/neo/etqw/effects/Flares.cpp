// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../Atmosphere.h"
#include "Flares.h"

/*
===============================================================================

  sdHardcodedParticleFlare

===============================================================================
*/


/*
==============
sdHardcodedParticleFlare::SetupParams
==============
*/
void sdHardcodedParticleFlare::SetupParams( sdFlareParameters &params, renderEntity_t &renderEntity, const idDict& args ) {
	params.depth = args.GetFloat( "depth", "100" );
	params.end_height = args.GetFloat( "end_height", "100" ) / 2.f;
	params.end_width= args.GetFloat( "end_width", "100" ) / 2.f;
	params.height = args.GetFloat( "height", "100" ) / 2.f;
	params.width = args.GetFloat( "width", "100" ) / 2.f;
	params.slices = args.GetInt( "slices", "10" );


	float overBright = args.GetFloat( "_overbright", "1.0f" );
	idVec4 color;
	args.GetVec4( "_color", "1 1 1 1", color );
	renderEntity.shaderParms[ SHADERPARM_RED ]	 = color[0] * overBright;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = color[1] * overBright;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]	 = color[2] * overBright;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ] = color[3];
	renderEntity.flags.noShadow = true;
	renderEntity.flags.noSelfShadow = true;
	renderEntity.flags.noDynamicInteractions = true;
	renderEntity.flags.pushByOrigin = true;
	renderEntity.maxVisDist = args.GetInt( "maxVisDist" );
	idStr gpuSpecParam = args.GetString( "drawSpec", "low" );
	if ( gpuSpecParam.Icmp( "high" ) == 0 ) {
		renderEntity.drawSpec = 2;
	} else if ( gpuSpecParam.Icmp( "med" ) == 0 || gpuSpecParam.Icmp( "medium" ) == 0 ) {
		renderEntity.drawSpec = 1;
	} else if ( gpuSpecParam.Icmp( "low" ) == 0 ) {
		renderEntity.drawSpec = 0;
	} else {
		renderEntity.drawSpec = 0;
	}

	if ( renderEntity.hModel->NumSurfaces() == 0 ) {
		modelSurface_t surface;
		memset( &surface, 0, sizeof( surface ) );
		surface.geometry = renderEntity.hModel->AllocSurfaceTriangles(  params.slices * 4, params.slices * 6 );
		surface.material = declHolder.declMaterialType.LocalFind(args.GetString( "material" ));
		surface.id = renderEntity.hModel->NumSurfaces();
		renderEntity.hModel->AddSurface( surface );
	} else {
		modelSurface_t *mdlsurf = (modelSurface_t *)renderEntity.hModel->Surface( 0 );
		srfTriangles_t *trisurf = mdlsurf->geometry;
		renderEntity.hModel->FreeSurfaceTriangles( trisurf );
		mdlsurf->geometry = renderEntity.hModel->AllocSurfaceTriangles(  params.slices * 4, params.slices * 6 );
		mdlsurf->material = declHolder.declMaterialType.LocalFind(args.GetString( "material" ));
	}

	srfTriangles_t *surf = renderEntity.hModel->Surface( 0 )->geometry;

	idBounds bounds;
	bounds.Clear();
	float diffw = params.end_width - params.width;
	float diffh = params.end_height - params.height;
	for (int i=0; i<params.slices; i++) {
		float factor = i / (float)params.slices;
		float w, h;
		switch ( i % 4 ) {
			case 0:
				w = -(factor * diffw + params.width);
				h = -(factor * diffh + params.height);
				break;
			case 1:
				w =  (factor * diffw + params.width);
				h = -(factor * diffh + params.height);
				break;
			case 2:
				w =  (factor * diffw + params.width);
				h =  (factor * diffh + params.height);
				break;
			case 3:
				w = -(factor * diffw + params.width);
				h =  (factor * diffh + params.height);
				break;
		};
		idVec3 xyz = idVec3(params.depth * factor,0.f,0.f);
		idBounds bb;
		bb.Zero();
		bb.ExpandSelf( idMath::Sqrt( w * w + h * h ) );
		bb.TranslateSelf( xyz );
		bounds.AddBounds( bb );
	}
	renderEntity.bounds = bounds;

	for (int i=0; i<params.slices; i++) {
		surf->indexes[ i*6 + 0 ] = i*4+0;
		surf->indexes[ i*6 + 1 ] = i*4+1;
		surf->indexes[ i*6 + 2 ] = i*4+2;
		surf->indexes[ i*6 + 3 ] = i*4+0;
		surf->indexes[ i*6 + 4 ] = i*4+2;
		surf->indexes[ i*6 + 5 ] = i*4+3;
	}
	surf->bounds = bounds;
	surf->numIndexes = params.slices * 6;
	surf->numVerts = 0;

	params.surfid = 0;
}

/*
==============
sdHardcodedParticleFlare::SetupRenderEntity
==============
*/
void sdHardcodedParticleFlare::SetupRenderEntity( sdFlareParameters const &params, renderEntity_t *renderEntity, const renderView_t *renderView ) {
	srfTriangles_t *surf = renderEntity->hModel->Surface( params.surfid )->geometry;
	float diffw = params.end_width - params.width;
	float diffh = params.end_height - params.height;
	idMat3 mat = renderView->viewaxis;
	idMat3 const &entmat = renderEntity->axis;
	idMat3 inventmat = entmat.Inverse();
	mat = mat * inventmat;
	surf->bounds.Clear();
	for (int i=0; i<params.slices * 4; i++) {
		float factor = (i/4) / (float)params.slices;
		surf->verts[ i ].Clear();
		float w, h;
		switch ( i % 4 ) {
			case 0:
				surf->verts[ i ].SetST( 0.f, 0.f );
				w = -(factor * diffw + params.width);
				h = -(factor * diffh + params.height);
				break;
			case 1:
				surf->verts[ i ].SetST( 1.f, 0.f );
				w =  (factor * diffw + params.width);
				h = -(factor * diffh + params.height);
				break;
			case 2:
				surf->verts[ i ].SetST( 1.f, 1.f );
				w =  (factor * diffw + params.width);
				h =  (factor * diffh + params.height);
				break;
			case 3:
				surf->verts[ i ].SetST( 0.f, 1.f );
				w = -(factor * diffw + params.width);
				h =  (factor * diffh + params.height);
				break;
		};
		surf->verts[ i ].xyz = idVec3(params.depth * factor,0.f,0.f) + mat[1] * w + mat[2] * h;
		surf->bounds.AddPoint( surf->verts[ i ].xyz );
	}
	surf->numVerts = params.slices * 4;
	renderEntity->hModel->DirtyVertexAmbientCache();
}

/*
==============
sdHardcodedParticleFlare::RenderEntityCallback
==============
*/
bool sdHardcodedParticleFlare::RenderEntityCallback( renderEntity_t *renderEntity, const renderView_t *renderView, int& lastGameModifiedTime ) {
	if ( renderView == NULL ) {
		assert( false );
		return false;
	}
	SetupRenderEntity( params, renderEntity, renderView );
	return true;
}

CLASS_DECLARATION( idEntity, sdMiscFlare )
END_CLASS

/*
===============================================================================

  sdMiscFlare

===============================================================================
*/

/*
==============
sdMiscFlare::Spawn
==============
*/
void sdMiscFlare::Spawn( void ) {

	if ( renderSystem->IsOpenGLRunning() ) {
		renderEntity_t &renderEntity = system.GetRenderEntity();
		sdHardcodedParticleFlare::SetupParams( system.params, renderEntity, spawnArgs );
		renderEntity.origin = GetPhysics()->GetOrigin();
		renderEntity.axis = GetPhysics()->GetAxis();

		BecomeActive( TH_THINK );
	}
}

/*
==============
sdMiscFlare::Think
==============
*/
void sdMiscFlare::Think( void ) {
	if ( renderSystem->IsOpenGLRunning() ) {
		system.PresentRenderEntity();
	}
}


/*
============
idGameEdit::FlareUpdate
============
*/
void idGameEdit::FlareUpdate( const idDict& args, renderEntity_t& renderEntity, const struct renderView_s* renderView ) {
	sdFlareParameters params;
	sdHardcodedParticleFlare::SetupParams( params, renderEntity, args );
	sdHardcodedParticleFlare::SetupRenderEntity( params, &renderEntity, renderView );
}

