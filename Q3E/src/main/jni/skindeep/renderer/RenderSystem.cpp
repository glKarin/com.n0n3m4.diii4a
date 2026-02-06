/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "sys/platform.h"
#include "idlib/containers/List.h"
#include "framework/EventLoop.h"
#include "framework/Session.h"
#include "framework/DemoFile.h"
#include "renderer/ModelManager.h"
#include "renderer/Material.h"
#include "renderer/GuiModel.h"
#include "renderer/VertexCache.h"
#include "renderer/RenderWorld_local.h"

#include "renderer/tr_local.h"



idRenderSystemLocal	tr;
idRenderSystem	*renderSystem = &tr;


/*
=====================
R_PerformanceCounters

This prints both front and back end counters, so it should
only be called when the back end thread is idle.
=====================
*/
static void R_PerformanceCounters( void ) {
	if ( r_showPrimitives.GetInteger() != 0 ) {

		float megaBytes = globalImages->SumOfUsedImages() / ( 1024*1024.0 );

		if ( r_showPrimitives.GetInteger() == 2 )
		{
			common->Printf( "v:%i ds:%i t:%i/%i v:%i/%i st:%i sv:%i image:%5.1f MB\n",
				tr.pc.c_numViews,
				backEnd.pc.c_drawElements + backEnd.pc.c_shadowElements,
				backEnd.pc.c_drawIndexes / 3,
				( backEnd.pc.c_drawIndexes - backEnd.pc.c_drawRefIndexes ) / 3,
				backEnd.pc.c_drawVertexes,
				( backEnd.pc.c_drawVertexes - backEnd.pc.c_drawRefVertexes ),
				backEnd.pc.c_shadowIndexes / 3,
				backEnd.pc.c_shadowVertexes,
				megaBytes
				);
		}
		else if (r_showPrimitives.GetInteger() == 3)
		{
			common->Printf("views:%i draws:%i tris:%i shdw:%i vbo:%i image:%5.1f MB\n",
				tr.pc.c_numViews,
				backEnd.pc.c_drawElements + backEnd.pc.c_shadowElements,
				(backEnd.pc.c_drawIndexes + backEnd.pc.c_shadowIndexes) / 3,
				backEnd.pc.c_shadowIndexes / 3,
				backEnd.pc.c_vboIndexes / 3,
				megaBytes
			);
		}
		else
		{
			common->Printf("drawcalls:%i/4000 tris:%i shadow:%i/800000 \n",
				backEnd.pc.c_drawElements + backEnd.pc.c_shadowElements,
				(backEnd.pc.c_drawIndexes + backEnd.pc.c_shadowIndexes) / 3,
				backEnd.pc.c_shadowIndexes / 3
			);			
		}
	}

	if ( r_showDynamic.GetBool() ) {
		common->Printf( "callback:%i md5:%i dfrmVerts:%i dfrmTris:%i tangTris:%i guis:%i\n",
			tr.pc.c_entityDefCallbacks,
			tr.pc.c_generateMd5,
			tr.pc.c_deformedVerts,
			tr.pc.c_deformedIndexes/3,
			tr.pc.c_tangentIndexes/3,
			tr.pc.c_guiSurfs
			);
	}

	if ( r_showCull.GetBool() ) {
		common->Printf( "%i sin %i sclip  %i sout %i bin %i bout\n",
			tr.pc.c_sphere_cull_in, tr.pc.c_sphere_cull_clip, tr.pc.c_sphere_cull_out,
			tr.pc.c_box_cull_in, tr.pc.c_box_cull_out );
	}

	if ( r_showAlloc.GetBool() ) {
		common->Printf( "alloc:%i free:%i\n", tr.pc.c_alloc, tr.pc.c_free );
	}

	if ( r_showInteractions.GetBool() ) {
		common->Printf( "createInteractions:%i createLightTris:%i createShadowVolumes:%i\n",
			tr.pc.c_createInteractions, tr.pc.c_createLightTris, tr.pc.c_createShadowVolumes );
	}
	if ( r_showDefs.GetBool() ) {
		common->Printf( "viewEntities:%i  shadowEntities:%i  viewLights:%i\n", tr.pc.c_visibleViewEntities,
			tr.pc.c_shadowViewEntities, tr.pc.c_viewLights );
	}
	if ( r_showUpdates.GetBool() ) {
		common->Printf( "entityUpdates:%i  entityRefs:%i  lightUpdates:%i  lightRefs:%i\n",
			tr.pc.c_entityUpdates, tr.pc.c_entityReferences,
			tr.pc.c_lightUpdates, tr.pc.c_lightReferences );
	}
	if ( r_showMemory.GetBool() ) {
		int	m1 = frameData ? frameData->memoryHighwater : 0;
		common->Printf( "frameData: %i (%i)\n", R_CountFrameData(), m1 );
	}
	if ( r_showLightScale.GetBool() ) {
		common->Printf( "lightScale: %f\n", backEnd.pc.maxLightValue );
	}

	memset( &tr.pc, 0, sizeof( tr.pc ) );
	memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}



/*
====================
R_IssueRenderCommands

Called by R_EndFrame each frame
====================
*/
static void R_IssueRenderCommands( void ) {
	if ( frameData->cmdHead->commandId == RC_NOP
		&& !frameData->cmdHead->next ) {
		// nothing to issue
		return;
	}

	// r_skipBackEnd allows the entire time of the back end
	// to be removed from performance measurements, although
	// nothing will be drawn to the screen.  If the prints
	// are going to a file, or r_skipBackEnd is later disabled,
	// usefull data can be received.

	// r_skipRender is usually more usefull, because it will still
	// draw 2D graphics
	if ( !r_skipBackEnd.GetBool() ) {
		RB_ExecuteBackEndCommands( frameData->cmdHead );
	}

	R_ClearCommandChain();
}

/*
============
R_GetCommandBuffer

Returns memory for a command buffer (stretchPicCommand_t,
drawSurfsCommand_t, etc) and links it to the end of the
current command chain.
============
*/
void *R_GetCommandBuffer( int bytes ) {
	emptyCommand_t	*cmd;

	cmd = (emptyCommand_t *)R_FrameAlloc( bytes );
	cmd->next = NULL;
	frameData->cmdTail->next = &cmd->commandId;
	frameData->cmdTail = cmd;

	return (void *)cmd;
}


/*
====================
R_ClearCommandChain

Called after every buffer submission
and by R_ToggleSmpFrame
====================
*/
void R_ClearCommandChain( void ) {
	// clear the command chain
	frameData->cmdHead = frameData->cmdTail = (emptyCommand_t *)R_FrameAlloc( sizeof( *frameData->cmdHead ) );
	frameData->cmdHead->commandId = RC_NOP;
	frameData->cmdHead->next = NULL;
}

/*
=================
R_ViewStatistics
=================
*/
static void R_ViewStatistics( viewDef_t *parms ) {
	// report statistics about this view
	if ( !r_showSurfaces.GetBool() ) {
		return;
	}
	common->Printf( "view:%p surfs:%i\n", parms, parms->numDrawSurfs );
}

/*
=============
R_AddDrawViewCmd

This is the main 3D rendering command.  A single scene may
have multiple views if a mirror, portal, or dynamic texture is present.
=============
*/
void	R_AddDrawViewCmd( viewDef_t *parms ) {
	drawSurfsCommand_t	*cmd;

	cmd = (drawSurfsCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_DRAW_VIEW;

	cmd->viewDef = parms;


	tr.pc.c_numViews++;

	R_ViewStatistics( parms );
}


//=================================================================================



/*
=============
R_CheckCvars

See if some cvars that we watch have changed
=============
*/
static void R_CheckCvars( void ) {
	globalImages->CheckCvars();

	// gamma stuff
	if ( r_gamma.IsModified() || r_brightness.IsModified() ) {
		r_gamma.ClearModified();
		r_brightness.ClearModified();
		// SM: Disable this because it changes the whole window, instead do in post process
		//R_SetColorMappings();
	}
}

/*
=============
idRenderSystemLocal::idRenderSystemLocal
=============
*/
idRenderSystemLocal::idRenderSystemLocal( void ) {
	Clear();
}

/*
=============
idRenderSystemLocal::~idRenderSystemLocal
=============
*/
idRenderSystemLocal::~idRenderSystemLocal( void ) {
}

/*
=============
SetColor

This can be used to pass general information to the current material, not
just colors
=============
*/
void idRenderSystemLocal::SetColor( const idVec4 &rgba ) {
	guiModel->SetColor( rgba[0], rgba[1], rgba[2], rgba[3] );
}


/*
=============
SetColor4
=============
*/
void idRenderSystemLocal::SetColor4( float r, float g, float b, float a ) {
	guiModel->SetColor( r, g, b, a );
}

/*
=============
DrawStretchPic
=============
*/
void idRenderSystemLocal::DrawStretchPic( const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *material,
									   bool clip, float min_x, float min_y, float max_x, float max_y ) {
	guiModel->DrawStretchPic( verts, indexes, vertCount, indexCount, material,
		clip, min_x, min_y, max_x, max_y );
}

/*
=============
DrawStretchPic

x/y/w/h are in the 0,0 to 640,480 range
=============
*/
void idRenderSystemLocal::DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material ) {
	guiModel->DrawStretchPic( x, y, w, h, s1, t1, s2, t2, material );
}


void idRenderSystemLocal::DrawStretchPicRotated(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const idMaterial *mat, float angle)
{
	guiModel->DrawStretchPicRotated(x, y, w, h, s0, t0, s1, t1, mat, angle);
}

/*
=============
DrawStretchTri

x/y/w/h are in the 0,0 to 640,480 range
=============
*/
void idRenderSystemLocal::DrawStretchTri( idVec2 p1, idVec2 p2, idVec2 p3, idVec2 t1, idVec2 t2, idVec2 t3, const idMaterial *material ) {
	tr.guiModel->DrawStretchTri( p1, p2, p3, t1, t2, t3, material );
}

/*
=============
GlobalToNormalizedDeviceCoordinates
=============
*/
void idRenderSystemLocal::GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc ) {
	R_GlobalToNormalizedDeviceCoordinates( global, ndc );
}

/*
=============
GlobalToNormalizedDeviceCoordinates
=============
*/
void idRenderSystemLocal::GetGLSettings( int& width, int& height ) {
	width = glConfig.vidWidth;
	height = glConfig.vidHeight;
}

/*
=====================
idRenderSystemLocal::DrawSmallChar

small chars are drawn at native screen resolution
=====================
*/
void idRenderSystemLocal::DrawSmallChar( float x, float y, int ch, const idMaterial *material ) {
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -SMALLCHAR_HEIGHT ) {
		return;
	}

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	DrawStretchPic( x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT,
					   fcol, frow,
					   fcol + size, frow + size,
					   material );
}


void idRenderSystemLocal::DrawMicroChar(float x, float y, int ch, const idMaterial *material) {
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if (ch == ' ') {
		return;
	}

	if (y < -MICROCHAR_HEIGHT) {
		return;
	}

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	DrawStretchPic(x, y, MICROCHAR_WIDTH, MICROCHAR_HEIGHT,
		fcol, frow,
		fcol + size, frow + size,
		material);
}






/*
==================
idRenderSystemLocal::DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void idRenderSystemLocal::DrawSmallStringExt( float  x, float y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material ) {
	idVec4		color;
	const unsigned char	*s;
	float		xx;

	// draw the colored text
	s = (const unsigned char*)string;
	xx = x;
	SetColor( setColor );
	while ( *s ) {
		if ( idStr::IsColor( (const char*)s ) ) {
			if ( !forceColor ) {
				if ( *(s+1) == C_COLOR_DEFAULT ) {
					SetColor( setColor );
				} else {
					color = idStr::ColorForIndex( *(s+1) );
					color[3] = setColor[3];
					SetColor( color );
				}
			}
			s += 2;
			continue;
		}
		DrawSmallChar( xx, y, *s, material );
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	SetColor( colorWhite );
}


//BC
void idRenderSystemLocal::DrawMicroStringExt(float x, float y, const char *string, const idVec4 &setColor, const idMaterial *material) {
	idVec4		color;
	const unsigned char	*s;
	float		xx;

	// draw the colored text
	s = (const unsigned char*)string;
	xx = x;
	SetColor(setColor);
	while (*s) {
		if (idStr::IsColor((const char*)s))
		{			
			s += 2;
			continue;
		}
		DrawMicroChar(xx, y, *s, material);
		xx += MICROCHAR_WIDTH - .5f; //bc tighter kerning
		s++;
	}
	SetColor(colorWhite);
}




/*
=====================
idRenderSystemLocal::DrawBigChar
=====================
*/
void idRenderSystemLocal::DrawBigChar( int x, int y, int ch, const idMaterial *material ) {
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -BIGCHAR_HEIGHT ) {
		return;
	}

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	DrawStretchPic( x, y, BIGCHAR_WIDTH, BIGCHAR_HEIGHT,
					   fcol, frow,
					   fcol + size, frow + size,
					   material );
}

/*
==================
idRenderSystemLocal::DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void idRenderSystemLocal::DrawBigStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material ) {
	idVec4		color;
	const char	*s;
	int			xx;

	// draw the colored text
	s = string;
	xx = x;
	SetColor( setColor );
	while ( *s ) {
		if ( idStr::IsColor( s ) ) {
			if ( !forceColor ) {
				if ( *(s+1) == C_COLOR_DEFAULT ) {
					SetColor( setColor );
				} else {
					color = idStr::ColorForIndex( *(s+1) );
					color[3] = setColor[3];
					SetColor( color );
				}
			}
			s += 2;
			continue;
		}
		DrawBigChar( xx, y, *s, material );
		xx += BIGCHAR_WIDTH;
		s++;
	}
	SetColor( colorWhite );
}

//======================================================================================

/*
==================
SetBackEndRenderer

Check for changes in the back end renderSystem, possibly invalidating cached data
==================
*/
void idRenderSystemLocal::SetBackEndRenderer() {
	if ( !r_renderer.IsModified() ) {
		return;
	}

	bool oldVPstate = backEndRendererHasVertexPrograms;

	backEndRenderer = BE_BAD;

	if ( idStr::Icmp( r_renderer.GetString(), "glsl" ) == 0 ) {
		if ( glConfig.allowGLSLPath ) {
			backEndRenderer = BE_GLSL;
		}
	}

	// fallback
	if ( backEndRenderer == BE_BAD ) {
		// choose the best
		if ( glConfig.allowGLSLPath ) {
			backEndRenderer = BE_GLSL;
		}
	}

	backEndRendererHasVertexPrograms = false;
	backEndRendererMaxLight = 1.0;

	switch( backEndRenderer ) {
	case BE_GLSL:
		common->Printf( "using GLSL renderSystem\n" );
		backEndRendererHasVertexPrograms = true;
		backEndRendererMaxLight = 999;
		cmdSystem->AddCommand( "reloadGLSLprograms", R_ReloadGLSLPrograms_f, CMD_FL_RENDERER, "reloads GLSL programs" );
		R_ReloadGLSLPrograms_f( idCmdArgs() );
		break;
	default:
		common->FatalError( "SetbackEndRenderer: bad back end" );
	}

	// clear the vertex cache if we are changing between
	// using vertex programs and not, because specular and
	// shadows will be different data
	if ( oldVPstate != backEndRendererHasVertexPrograms ) {
		vertexCache.PurgeAll();
		if ( primaryWorld ) {
			primaryWorld->FreeInteractions();
		}
	}

	r_renderer.ClearModified();
}

/*
====================
BeginFrame
====================
*/
void idRenderSystemLocal::BeginFrame( int windowWidth, int windowHeight ) {
	setBufferCommand_t	*cmd;

	if ( !glConfig.isInitialized ) {
		return;
	}

	//r_lock dhewm3
	if (r_lockSurfaces.IsModified())
	{
		static bool origUseScissor = true;
		r_lockSurfaces.ClearModified();
		if (r_lockSurfaces.GetBool()) {
			origUseScissor = r_useScissor.GetBool();
			r_useScissor.SetBool(false);
		}
		else {
			r_useScissor.SetBool(origUseScissor);
		}
	}

	// determine which back end we will use
	SetBackEndRenderer();

	guiModel->Clear();

	// for the larger-than-window tiled rendering screenshots
	if ( tiledViewport[0] ) {
		windowWidth = tiledViewport[0];
		windowHeight = tiledViewport[1];
	}

	glConfig.vidWidth = windowWidth;
	glConfig.vidHeight = windowHeight;

	renderCrops[0].x = 0;
	renderCrops[0].y = 0;
	renderCrops[0].width = windowWidth;
	renderCrops[0].height = windowHeight;
	currentRenderCrop = 0;

	// screenFraction is just for quickly testing fill rate limitations
	if ( r_screenFraction.GetInteger() != 100 ) {
		int	w = SCREEN_WIDTH * r_screenFraction.GetInteger() / 100.0f;
		int h = SCREEN_HEIGHT * r_screenFraction.GetInteger() / 100.0f;
		CropRenderSize( w, h );
	}


	// this is the ONLY place this is modified
	frameCount++;

	// just in case we did a common->Error while this
	// was set
	guiRecursionLevel = 0;

	// the first rendering will be used for commands like
	// screenshot, rather than a possible subsequent remote
	// or mirror render
//	primaryWorld = NULL;

	// set the time for shader effects in 2D rendering
	frameShaderTime = eventLoop->Milliseconds() * 0.001;

	R_AddSetFrameBufferCmd(FrameBufferIds_t::FRAME_MAINVIEW);

	//
	// draw buffer stuff
	//
	cmd = (setBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_SET_BUFFER;
	cmd->frameCount = frameCount;
}

void idRenderSystemLocal::WriteDemoPics() {
	session->writeDemo->WriteInt( DS_RENDER );
	session->writeDemo->WriteInt( DC_GUI_MODEL );
	guiModel->WriteToDemo( session->writeDemo );
}

void idRenderSystemLocal::DrawDemoPics() {
	demoGuiModel->EmitFullScreen();
}

/*
=============
EndFrame

Returns the number of msec spent in the back end
=============
*/
void idRenderSystemLocal::EndFrame( double *frontEndMsec, double *backEndMsec ) {
	emptyCommand_t *cmd;

	if ( !glConfig.isInitialized ) {
		return;
	}
#ifdef _DIII4A //karin: check/wait EGL context
    if(!GLimp_CheckGLInitialized())
		return;
#endif

	// close any gui drawing
	guiModel->EmitFullScreen();
	guiModel->Clear();

	// save out timing information
	if ( frontEndMsec ) {
		*frontEndMsec = pc.frontEndMsec;
	}
	if ( backEndMsec ) {
		*backEndMsec = backEnd.pc.msec;
	}

	

	// print any other statistics and clear all of them
	R_PerformanceCounters();

	// check for dynamic changes that require some initialization
	R_CheckCvars();

	// check for errors
	GL_CheckErrors();

	// add the swapbuffers command
	cmd = (emptyCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_SWAP_BUFFERS;

	// start the back end up again with the new command list
	R_IssueRenderCommands();

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current buffers
	R_ToggleSmpFrame();

	// we can now release the vertexes used this frame
	vertexCache.EndFrame();

	if ( session->writeDemo ) {
		session->writeDemo->WriteInt( DS_RENDER );
		session->writeDemo->WriteInt( DC_END_FRAME );
		if ( r_showDemo.GetBool() ) {
			common->Printf( "write DC_END_FRAME\n" );
		}
	}

}

/*
=====================
RenderViewToViewport

Converts from SCREEN_WIDTH / SCREEN_HEIGHT coordinates to current cropped pixel coordinates
=====================
*/
void idRenderSystemLocal::RenderViewToViewport( const renderView_t *renderView, idScreenRect *viewport ) {
	renderCrop_t	*rc = &renderCrops[currentRenderCrop];

	float wRatio = (float)rc->width / SCREEN_WIDTH;
	float hRatio = (float)rc->height / SCREEN_HEIGHT;

	viewport->x1 = idMath::Ftoi( rc->x + renderView->x * wRatio );
	viewport->x2 = idMath::Ftoi( rc->x + floor( ( renderView->x + renderView->width ) * wRatio + 0.5f ) - 1 );
	viewport->y1 = idMath::Ftoi( ( rc->y + rc->height ) - floor( ( renderView->y + renderView->height ) * hRatio + 0.5f ) );
	viewport->y2 = idMath::Ftoi( ( rc->y + rc->height ) - floor( renderView->y * hRatio + 0.5f ) - 1 );
}

static int RoundDownToPowerOfTwo( int v ) {
	int	i;

	for ( i = 0 ; i < 20 ; i++ ) {
		if ( ( 1 << i ) == v ) {
			return v;
		}
		if ( ( 1 << i ) > v ) {
			return 1 << ( i-1 );
		}
	}
	return 1<<i;
}

/*
================
CropRenderSize

This automatically halves sizes until it fits in the current window size,
so if you specify a power of two size for a texture copy, it may be shrunk
down, but still valid.
================
*/
void	idRenderSystemLocal::CropRenderSize( int width, int height, bool makePowerOfTwo, bool forceDimensions ) {
	if ( !glConfig.isInitialized ) {
		return;
	}

	// close any gui drawing before changing the size
	guiModel->EmitFullScreen();
	guiModel->Clear();

	if ( width < 1 || height < 1 ) {
		common->Error( "CropRenderSize: bad sizes" );
	}

	if ( session->writeDemo ) {
		session->writeDemo->WriteInt( DS_RENDER );
		session->writeDemo->WriteInt( DC_CROP_RENDER );
		session->writeDemo->WriteInt( width );
		session->writeDemo->WriteInt( height );
		session->writeDemo->WriteInt( makePowerOfTwo );

		if ( r_showDemo.GetBool() ) {
			common->Printf( "write DC_CROP_RENDER\n" );
		}
	}

	// convert from virtual SCREEN_WIDTH/SCREEN_HEIGHT coordinates to physical OpenGL pixels
	renderView_t renderView;
	renderView.x = 0;
	renderView.y = 0;
	renderView.width = width;
	renderView.height = height;

	idScreenRect	r;
	RenderViewToViewport( &renderView, &r );

	width = r.x2 - r.x1 + 1;
	height = r.y2 - r.y1 + 1;

	if ( forceDimensions ) {
		// just give exactly what we ask for
		width = renderView.width;
		height = renderView.height;
	}

	// if makePowerOfTwo, drop to next lower power of two after scaling to physical pixels
	if ( makePowerOfTwo ) {
		width = RoundDownToPowerOfTwo( width );
		height = RoundDownToPowerOfTwo( height );
		// FIXME: megascreenshots with offset viewports don't work right with this yet
	}

	renderCrop_t	*rc = &renderCrops[currentRenderCrop];

	// we might want to clip these to the crop window instead
	while ( width > glConfig.vidWidth ) {
		width >>= 1;
	}
	while ( height > glConfig.vidHeight ) {
		height >>= 1;
	}

	if ( currentRenderCrop == MAX_RENDER_CROPS ) {
		common->Error( "idRenderSystemLocal::CropRenderSize: currentRenderCrop == MAX_RENDER_CROPS" );
	}

	currentRenderCrop++;

	rc = &renderCrops[currentRenderCrop];

	rc->x = 0;
	rc->y = 0;
	rc->width = width;
	rc->height = height;
}

/*
================
UnCrop
================
*/
void idRenderSystemLocal::UnCrop() {
	if ( !glConfig.isInitialized ) {
		return;
	}

	if ( currentRenderCrop < 1 ) {
		common->Error( "idRenderSystemLocal::UnCrop: currentRenderCrop < 1" );
	}

	// close any gui drawing
	guiModel->EmitFullScreen();
	guiModel->Clear();

	currentRenderCrop--;

	if ( session->writeDemo ) {
		session->writeDemo->WriteInt( DS_RENDER );
		session->writeDemo->WriteInt( DC_UNCROP_RENDER );

		if ( r_showDemo.GetBool() ) {
			common->Printf( "write DC_UNCROP\n" );
		}
	}
}

/*
================
CaptureRenderToImage
================
*/
void idRenderSystemLocal::CaptureRenderToImage( const char *imageName ) {
	if ( !glConfig.isInitialized ) {
		return;
	}
	guiModel->EmitFullScreen();
	guiModel->Clear();

	if ( session->writeDemo ) {
		session->writeDemo->WriteInt( DS_RENDER );
		session->writeDemo->WriteInt( DC_CAPTURE_RENDER );
		session->writeDemo->WriteHashString( imageName );

		if ( r_showDemo.GetBool() ) {
			common->Printf( "write DC_CAPTURE_RENDER: %s\n", imageName );
		}
	}

	// look up the image before we create the render command, because it
	// may need to sync to create the image
	idImage	*image = globalImages->ImageFromFile(imageName, TF_DEFAULT, true, TR_REPEAT, TD_DEFAULT);

	renderCrop_t *rc = &renderCrops[currentRenderCrop];

	copyRenderCommand_t *cmd = (copyRenderCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_COPY_RENDER;
	cmd->x = rc->x;
	cmd->y = rc->y;
	cmd->imageWidth = rc->width;
	cmd->imageHeight = rc->height;
	cmd->image = image;

	guiModel->Clear();
}

void idRenderSystemLocal::PrepareForPostProcessPass()
{
	if (!glConfig.isInitialized) {
		return;
	}
	guiModel->EmitFullScreen();
	guiModel->Clear();

	R_AddPostEffectCmd();

	guiModel->Clear();
}

/*
==============
CaptureRenderToFile

==============
*/
void idRenderSystemLocal::CaptureRenderToFile( const char *fileName, bool fixAlpha ) {
	if ( !glConfig.isInitialized ) {
		return;
	}

	renderCrop_t *rc = &renderCrops[currentRenderCrop];

	guiModel->EmitFullScreen();
	guiModel->Clear();
	R_IssueRenderCommands();

#ifdef _GLES //karin: using GL_RGBA on glReadPixels
	// include extra space for OpenGL padding to word boundaries
	int	c = ( rc->width + 4 ) * rc->height;
	byte *data = (byte *)R_StaticAlloc( c * 4 );

	qglReadPixels( rc->x, rc->y, rc->width, rc->height, GL_RGBA, GL_UNSIGNED_BYTE, data );
#else
	qglReadBuffer( GL_BACK );

	// include extra space for OpenGL padding to word boundaries
	int	c = ( rc->width + 3 ) * rc->height;
	byte *data = (byte *)R_StaticAlloc( c * 3 );

	qglReadPixels( rc->x, rc->y, rc->width, rc->height, GL_RGB, GL_UNSIGNED_BYTE, data );
#endif

	byte *data2 = (byte *)R_StaticAlloc( c * 4 );

	for ( int i = 0 ; i < c ; i++ ) {
#ifdef _GLES //karin: using GL_RGBA on glReadPixels
		data2[ i * 4 ] = data[ i * 4 ];
		data2[ i * 4 + 1 ] = data[ i * 4 + 1 ];
		data2[ i * 4 + 2 ] = data[ i * 4 + 2 ];
#else
		data2[ i * 4 ] = data[ i * 3 ];
		data2[ i * 4 + 1 ] = data[ i * 3 + 1 ];
		data2[ i * 4 + 2 ] = data[ i * 3 + 2 ];
#endif
		data2[ i * 4 + 3 ] = 0xff;
	}

	R_WriteTGA( fileName, data2, rc->width, rc->height, true );

	R_StaticFree( data );
	R_StaticFree( data2 );
}


/*
==============
AllocRenderWorld
==============
*/
idRenderWorld *idRenderSystemLocal::AllocRenderWorld() {
	idRenderWorldLocal *rw;
	rw = new idRenderWorldLocal;
	worlds.Append( rw );
	return rw;
}

/*
==============
FreeRenderWorld
==============
*/
void idRenderSystemLocal::FreeRenderWorld( idRenderWorld *rw ) {
	if ( primaryWorld == rw ) {
		primaryWorld = NULL;
	}
	worlds.Remove( static_cast<idRenderWorldLocal *>(rw) );
	delete rw;
}

/*
==============
PrintMemInfo
==============
*/
void idRenderSystemLocal::PrintMemInfo( MemInfo_t *mi ) {
	// sum up image totals
	globalImages->PrintMemInfo( mi );

	// sum up model totals
	renderModelManager->PrintMemInfo( mi );

	// compute render totals

}

/*
===============
idRenderSystemLocal::UploadImage
===============
*/
bool idRenderSystemLocal::UploadImage( const char *imageName, const byte *data, int width, int height  ) {
	idImage *image = globalImages->GetImage( imageName );
	if ( !image ) {
		return false;
	}
	image->UploadScratch( data, width, height );
	image->SetImageFilterAndRepeat();
	return true;
}
/*
===============
idRenderSystemLocal::GetImageBytes

SW: Slightly hacky method for extracting the bytes of a known image from OpenGL. 
The caller is expected to allocate and manage the memory for it.
===============
*/
int idRenderSystemLocal::GetImageBytes(idImage* img, byte** bytes)
{
	qglGetError(); // Clear error flags, if any
	img->Bind();
	qglGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, *bytes);
	return qglGetError();
}

/*
=============
idRenderSystemLocal::PrimaryViewExists

	SW: For the first few frames of game time, it seems that the primary view hasn't yet been initialized.
	This causes problems for some features that jump the gun and try to use it to (e.g) translate worldspace to screenspace coordinates.
	This method lets us safely check if the view is non-null and let the calling functions make up their mind about how to handle it.
=============
*/
bool idRenderSystemLocal::PrimaryViewExists() {
	return tr.primaryView != NULL;
}

void idRenderSystemLocal::UpdateScreenWidth()
{
	renderCrops[0].x = 0;
	renderCrops[0].y = 0;
	renderCrops[0].width = glConfig.vidWidth;
	renderCrops[0].height = glConfig.vidHeight;
	currentRenderCrop = 0;
}

/*
=============
R_AddSetFrameBufferCmd

blendo eric: adds a frame buffer swap command (changes current render buffer if different)
=============
*/
void	R_AddSetFrameBufferCmd(FrameBufferIds_t id) {
	setFrameBufferCommand_t	*cmd;

	cmd = (setFrameBufferCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	cmd->commandId = RC_SET_FRAME_BUFFER;
	cmd->frameBufferId = id;

	tr.pc.c_frameBufferSwaps++; //naive, doesn't consider ignored swaps
}

void R_AddPostEffectCmd() {
	postEffectCommand_t* cmd;
	cmd = (postEffectCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	cmd->commandId = RC_POST_EFFECT;
}
