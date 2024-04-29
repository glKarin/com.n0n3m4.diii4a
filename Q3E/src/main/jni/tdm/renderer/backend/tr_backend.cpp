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
#include "renderer/backend/FrameBuffer.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/RenderBackend.h"
#include "renderer/backend/stages/BloomStage.h"
#include "renderer/backend/FrameBufferManager.h"

backEndState_t	backEnd;
idCVarBool image_showBackgroundLoads( "image_showBackgroundLoads", "0", CVAR_RENDERER, "1 = print outstanding background loads" );

/*
======================
RB_SetDefaultGLState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
======================
*/
void RB_SetDefaultGLState( void ) {
	TRACE_GL_SCOPE( "RB_SetDefaultGLState" );
	GL_CheckErrors();

#ifdef __ANDROID__ //karin: GLES
	qglClearDepthf( 1.0f );
#else
	qglClearDepth( 1.0f );
#endif
	//GL_FloatColor( 1.0f, 1.0f, 1.0f, 1.0f );

	// the vertex arrays are always enabled. FIXME: Not exactly a 'default GL state'
	const int attrib_indices[] = { 0,2,3,8,9,10 };
	for ( auto attr_index : attrib_indices )
		qglEnableVertexAttribArray( attr_index );

	// make sure our GL state vector is set correctly
	memset( &backEnd.glState, 0, sizeof( backEnd.glState ) );
	backEnd.glState.forceGlState = true;

	qglColorMask( 1, 1, 1, 1 );

	qglEnable( GL_DEPTH_TEST );
	qglEnable( GL_BLEND );
	qglEnable( GL_SCISSOR_TEST );
	qglEnable( GL_CULL_FACE );
	qglDisable( GL_STENCIL_TEST );

	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	qglDepthMask( GL_TRUE );
	qglDepthFunc( GL_ALWAYS );

	qglCullFace( GL_FRONT_AND_BACK );
	//qglShadeModel( GL_SMOOTH );

	// stgatilov #5875: clean ALL texture units just to be sure
	for (int i = 0; i < MAX_MULTITEXTURE_UNITS; i++) {
		qglActiveTexture(GL_TEXTURE0 + i);
		qglBindTexture(GL_TEXTURE_2D, 0);
	}
	// and set active TMU to 0 to ensure GL_SelectTexture is synced
	assert( backEnd.glState.currenttmu == 0 );
	qglActiveTexture( GL_TEXTURE0 );

	if ( r_useScissor.GetBool() ) {
		GL_ScissorRelative( 0, 0, 1, 1 );
	}

	GL_CheckErrors();
}

/*
====================
RB_LogComment
====================
*/
/*void RB_LogComment( const char *comment, ... ) {
	if ( !tr.logFile ) {
		return;
	}
	va_list marker;

	fprintf( tr.logFile, "// " );
	va_start( marker, comment );
	vfprintf( tr.logFile, comment, marker );
	va_end( marker );
}*/


//=============================================================================

/*
====================
GL_SelectTexture
====================
*/
void GL_SelectTexture( const int unit ) {
	if ( backEnd.glState.currenttmu == unit ) {
		return;
	}

	if ( unit < 0 || unit >= MAX_MULTITEXTURE_UNITS ) {
		common->Warning( "GL_SelectTexture: unit = %i", unit );
		return;
	}
	qglActiveTexture( GL_TEXTURE0 + unit );

	backEnd.glState.currenttmu = unit;
}

/*
====================
GL_Cull

This handles the flipping needed when the view being
rendered is a mirored view.
====================
*/
void GL_Cull( const int cullType ) {
	if ( backEnd.glState.faceCulling == cullType ) {
		return;
	}

	if ( cullType == CT_TWO_SIDED ) {
		qglDisable( GL_CULL_FACE );
	} else {
		if ( backEnd.glState.faceCulling == CT_TWO_SIDED ) {
			qglEnable( GL_CULL_FACE );
		}

		if ( cullType == CT_BACK_SIDED ) {
			if ( backEnd.viewDef->isMirror ) {
				qglCullFace( GL_FRONT );
			} else {
				qglCullFace( GL_BACK );
			}
		} else {
			if ( backEnd.viewDef->isMirror ) {
				qglCullFace( GL_BACK );
			} else {
				qglCullFace( GL_FRONT );
			}
		}
	}
	backEnd.glState.faceCulling = cullType;
}

/*
=================
GL_ClearStateDelta

Clears the state delta bits, so the next GL_State
will set every item
=================
*/
void GL_ClearStateDelta( void ) {
	backEnd.glState.forceGlState = true;
}

/*
====================
GL_State

This routine is responsible for setting the most commonly changed state
====================
*/
void GL_State( const int stateBits ) {

	int diff;

	if ( !r_useStateCaching.GetBool() || backEnd.glState.forceGlState ) {
		// make sure everything is set all the time, so we
		// can see if our delta checking is screwing up
		diff = -1;
		backEnd.glState.forceGlState = false;
	} else {
		diff = stateBits ^ backEnd.glState.glStateBits;
		if ( !diff ) {
			return;
		}
	}

	// check depthFunc bits
	if ( diff & ( GLS_DEPTHFUNC_EQUAL | GLS_DEPTHFUNC_LESS | GLS_DEPTHFUNC_ALWAYS ) ) {
		if ( stateBits & GLS_DEPTHFUNC_EQUAL ) {
			qglDepthFunc( GL_EQUAL );
		} else if ( stateBits & GLS_DEPTHFUNC_ALWAYS ) {
			qglDepthFunc( GL_ALWAYS );
		} else {
			qglDepthFunc( GL_LEQUAL );
		}
	}

	// check blend bits
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) {
		GLenum srcFactor, dstFactor;

		switch ( stateBits & GLS_SRCBLEND_BITS ) {
		case GLS_SRCBLEND_ONE:
			srcFactor = GL_ONE;
			break;
		case GLS_SRCBLEND_ZERO:
			srcFactor = GL_ZERO;
			break;
		case GLS_SRCBLEND_DST_COLOR:
			srcFactor = GL_DST_COLOR;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
			srcFactor = GL_ONE_MINUS_DST_COLOR;
			break;
		case GLS_SRCBLEND_SRC_ALPHA:
			srcFactor = GL_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
			srcFactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_DST_ALPHA:
			srcFactor = GL_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
			srcFactor = GL_ONE_MINUS_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ALPHA_SATURATE:
			srcFactor = GL_SRC_ALPHA_SATURATE;
			break;
		default:
			srcFactor = GL_ONE;		// to get warning to shut up
			common->Error( "GL_State: invalid src blend state bits\n" );
			break;
		}

		switch ( stateBits & GLS_DSTBLEND_BITS ) {
		case GLS_DSTBLEND_ZERO:
			dstFactor = GL_ZERO;
			break;
		case GLS_DSTBLEND_ONE:
			dstFactor = GL_ONE;
			break;
		case GLS_DSTBLEND_SRC_COLOR:
			dstFactor = GL_SRC_COLOR;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
			dstFactor = GL_ONE_MINUS_SRC_COLOR;
			break;
		case GLS_DSTBLEND_SRC_ALPHA:
			dstFactor = GL_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
			dstFactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_DST_ALPHA:
			dstFactor = GL_DST_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
			dstFactor = GL_ONE_MINUS_DST_ALPHA;
			break;
		default:
			dstFactor = GL_ONE;		// to get warning to shut up
			common->Error( "GL_State: invalid dst blend state bits\n" );
			break;
		}
		qglBlendFunc( srcFactor, dstFactor );
	}

	// check depthmask
	if ( diff & GLS_DEPTHMASK ) {
		if ( stateBits & GLS_DEPTHMASK ) {
			qglDepthMask( GL_FALSE );
		} else {
			qglDepthMask( GL_TRUE );
		}
	}

	// check colormask
	if ( diff & ( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK ) ) {
		qglColorMask(
		    !( stateBits & GLS_REDMASK ),
		    !( stateBits & GLS_GREENMASK ),
		    !( stateBits & GLS_BLUEMASK ),
		    !( stateBits & GLS_ALPHAMASK )
		);
	}

	// fill/line mode
	if ( diff & GLS_POLYMODE_LINE ) {
		if ( stateBits & GLS_POLYMODE_LINE ) {
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		} else {
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}
	backEnd.glState.glStateBits = stateBits;
}

/*
========================
ApplyDepthTweaks
========================
*/

ApplyDepthTweaks::ApplyDepthTweaks( const drawSurf_t *surf ) : drawSurf(surf) {
	if ( drawSurf ) {
		if ( drawSurf->material && drawSurf->material->TestMaterialFlag( MF_POLYGONOFFSET ) ) {
			qglEnable( GL_POLYGON_OFFSET_FILL );
			qglPolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * drawSurf->material->GetPolygonOffset() );
		}
		if ( drawSurf->space && drawSurf->space->weaponDepthHack ) {
			RB_EnterWeaponDepthHack();
		}
	}
}

ApplyDepthTweaks::~ApplyDepthTweaks() {
	if ( drawSurf ) {
		if ( drawSurf->material && drawSurf->material->TestMaterialFlag( MF_POLYGONOFFSET ) ) {
			qglDisable( GL_POLYGON_OFFSET_FILL );
		}
		if ( drawSurf->space && drawSurf->space->weaponDepthHack ) {
			RB_LeaveDepthHack();
		}
	}
}

/*
========================
DepthBoundsTest
========================
*/
DepthBoundsTest::DepthBoundsTest( const idScreenRect &scissorRect ) {
	if ( !glConfig.depthBoundsTestAvailable || !r_useDepthBoundsTest.GetBool() )
		return;
	qglEnable( GL_DEPTH_BOUNDS_TEST_EXT );
	Update( scissorRect );
}

void DepthBoundsTest::Update( const idScreenRect &scissorRect ) {
	if ( !glConfig.depthBoundsTestAvailable || !r_useDepthBoundsTest.GetBool() )
		return;
	// note: the bounds should have been expanded for precision in R_ScreenRectFromViewFrustumBounds
	assert( scissorRect.zmin <= scissorRect.zmax );
	qglDepthBoundsEXT( scissorRect.zmin, scissorRect.zmax );
}

DepthBoundsTest::~DepthBoundsTest() {
	if ( !glConfig.depthBoundsTestAvailable || !r_useDepthBoundsTest.GetBool() )
		return;
	qglDisable( GL_DEPTH_BOUNDS_TEST_EXT );
}

/*
============================================================================

RENDER BACK END COLOR WRAPPERS

============================================================================
*/

/*
====================
GL_Color

Vector color 3 component (clamped)
====================
*/
/*GL_FloatColor( const idVec3 &color ) {
	GLfloat parm[3];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, color[0] );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, color[1] );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, color[2] );
	qglColor3f( parm[0], parm[1], parm[2] );
}*/

/*
====================
GL_Color

Vector color 4 component (clamped)
====================
*/
GLColorOverride::GLColorOverride( const idVec4 &color ) {
	Enable( color.ToFloatPtr() );
}

/*
====================
GL_Color

Float to vector color 3 or 4 component (clamped)
====================
*/
GLColorOverride::GLColorOverride( const float *color ) {
	Enable( color );
}

/*
====================
GL_Color

Float color 3 component (clamped)
====================
*/
GLColorOverride::GLColorOverride( float r, float g, float b ) {
	GLfloat parm[4] = { r,g,b,1 };
	Enable( parm );
}

/*
====================
GL_Color

Float color 4 component (clamped)
====================
*/
GLColorOverride::GLColorOverride( float r, float g, float b, float a ) {
	GLfloat parm[4] = {r,g,b,a};
	Enable( parm );
}

GLColorOverride::~GLColorOverride() {
	if ( !enabled )
		return;
	qglEnableVertexAttribArray( 3 );
}

void GLColorOverride::Enable( const float* color ) {
	GLfloat parm[4];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, color[0] );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, color[1] );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, color[2] );
	parm[3] = idMath::ClampFloat( 0.0f, 1.0f, color[3] );
	//qglColor4f( parm[0], parm[1], parm[2], parm[3] );
	qglDisableVertexAttribArray( 3 );
	qglVertexAttrib4fv( 3, parm );
	enabled = true;
}

/*
====================
GL_Color

Byte to vector color 3 or 4 component (clamped)
====================
*/
void GL_ByteColor( const byte *color ) {
	GLubyte parm[4] = { 255, 255, 255, 255 };
	parm[0] = idMath::ClampByte( 0, 255, color[0] );
	parm[1] = idMath::ClampByte( 0, 255, color[1] );
	parm[2] = idMath::ClampByte( 0, 255, color[2] );
	if ( color[3] ) {
		parm[3] = idMath::ClampByte( 0, 255, color[3] );
	}
//	qglColor3ub( parm[0], parm[1], parm[2] );
	qglVertexAttrib4ubv( 3, parm );
}


/*
====================
GL_Color

Byte color 3 component (clamped)
====================
*/
void GL_ByteColor( byte r, byte g, byte b ) {
	GLubyte parm[4] = { 255, 255, 255, 255 };
	parm[0] = idMath::ClampByte( 0, 255, r );
	parm[1] = idMath::ClampByte( 0, 255, g );
	parm[2] = idMath::ClampByte( 0, 255, b );
	//qglColor3ub( parm[0], parm[1], parm[2] );
	qglVertexAttrib4ubv( 3, parm );
}

/*
====================
GL_Color

Byte color 4 component (clamped)
====================
*/
void GL_ByteColor( byte r, byte g, byte b, byte a ) {
	GLubyte parm[4] = { 255, 255, 255, 255 };
	parm[0] = idMath::ClampByte( 0, 255, r );
	parm[1] = idMath::ClampByte( 0, 255, g );
	parm[2] = idMath::ClampByte( 0, 255, b );
	parm[3] = idMath::ClampByte( 0, 255, a );
	//qglColor4ub( parm[0], parm[1], parm[2], parm[3] );
	qglVertexAttrib4ubv( 3, parm );
}

void GL_SetProjection( float* matrix ) {
	qglBindBuffer( GL_UNIFORM_BUFFER, programManager->uboHandle );
	qglBufferData( GL_UNIFORM_BUFFER, sizeof( backEnd.viewDef->projectionMatrix ), matrix, GL_DYNAMIC_DRAW );
}

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
=============
RB_SetGL2D

This is not used by the normal game paths, just by some tools
=============
*/
void RB_SetGL2D( void ) {
	// set 2D virtual screen size
	GL_ViewportVidSize( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	if ( r_useScissor.GetBool() ) {
		GL_ScissorVidSize( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	}
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity();
	qglOrtho( 0, 640, 480, 0, 0, 1 );		// always assume 640x480 virtual coordinates
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();

	GL_State( GLS_DEPTHFUNC_ALWAYS |
	          GLS_SRCBLEND_SRC_ALPHA |
	          GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Cull( CT_TWO_SIDED );

	qglDisable( GL_DEPTH_TEST );
	qglDisable( GL_STENCIL_TEST );
}

/*
=============
RB_SetBuffer
=============
*/
static void	RB_SetBuffer( const void *data ) {
	const setBufferCommand_t	*cmd;

	// see which draw buffer we want to render the frame to
	cmd = ( const setBufferCommand_t * )data;

	backEnd.frameCount = cmd->frameCount;

#if !defined(__ANDROID__)
	qglDrawBuffer( r_frontBuffer.GetBool() ? GL_FRONT : GL_BACK );
#endif
#ifdef _OPENGLES3
	qglReadBuffer( r_frontBuffer.GetBool() ? GL_FRONT : GL_BACK ); //karin: OpenGLES3.0
#endif

	// note: clear was moved to RB_BeginDrawingView
}

/*
=============
RB_DumpFramebuffer

Bloom related debug tool
=============
*/
void RB_DumpFramebuffer( const char *fileName ) {
	renderCrop_t r;

	qglGetIntegerv( GL_VIEWPORT, &r.x );

	// calculate pitch of buffer that will be returned by qglReadPixels()
	int alignment;
	qglGetIntegerv( GL_PACK_ALIGNMENT, &alignment );

	int pitch = r.width * 4 + alignment - 1;
	pitch = pitch - pitch % alignment;

	byte *data = (byte *)R_StaticAlloc( pitch * r.height );

	// GL_RGBA/GL_UNSIGNED_BYTE seems to be the safest option
	qglReadPixels( r.x, r.y, r.width, r.height, GL_RGBA, GL_UNSIGNED_BYTE, data );

	byte *data2 = (byte *)R_StaticAlloc( r.width * r.height * 4 );

	for ( int y = 0; y < r.height; y++ ) {
		memcpy( data2 + y * r.width * 4, data + y * pitch, r.width * 4 );
	}
	R_WriteTGA( fileName, data2, r.width, r.height, true );

	R_StaticFree( data );
	R_StaticFree( data2 );
}

/*
=============
RB_CheckTools

Revelator: Check for any rendertool and mark it
because it might potentially break with post processing.
=============
*/
static bool RB_CheckTools( int width, int height ) {
	// this has actually happened
	if ( !width || !height ) {
		return true;
	}
	// nbohr1more add checks for render tools
	// revelator: added some more
	if ( r_showLightCount.GetBool() ||
	     r_showShadows.GetBool() ||
	     r_showVertexColor.GetBool() ||
	     r_showShadowCount.GetBool() ||
	     r_showTris.GetBool() ||
	     r_showTexturePolarity.GetBool() ||
	     r_showTangentSpace.GetBool() ||
	     r_showDepth.GetBool() ) {
		return true;
	}
	return false;
}

/*
=============
RB_DrawFullScreenQuad

Moved to backend: Revelator
=============
*/
void RB_DrawFullScreenQuad( float e ) {
#if 1
	vertexCache.VertexPosition( vertexCache.screenRectSurf.ambientCache );
	RB_DrawElementsWithCounters( &vertexCache.screenRectSurf );
#else
	qglBegin( GL_QUADS );
	qglTexCoord2f( 0, 0 );
	qglVertex2f( -e, -e );
	qglTexCoord2f( 0, 1 );
	qglVertex2f( -e, e );
	qglTexCoord2f( 1, 1 );
	qglVertex2f( e, e );
	qglTexCoord2f( 1, 0 );
	qglVertex2f( e, -e );
	qglEnd();
#endif
}

void RB_DrawFullScreenTri() {
	int oldCulling = backEnd.glState.faceCulling;
	GL_Cull( CT_TWO_SIDED );
	qglDrawArrays( GL_TRIANGLES, 0, 3 );
	GL_Cull( oldCulling );
}


bool RB_Bloom( bloomCommand_t *cmd ) {
	if ( !r_bloom.GetBool() ) {
		return false;
	}
	if ( RB_CheckTools( globalImages->currentRenderImage->uploadWidth, globalImages->currentRenderImage->uploadHeight ) ) {
		return false;
	}

	TRACE_GL_SCOPE("Postprocess")
	frameBuffers->UpdateCurrentRenderCopy();
	bloom->ComputeBloomFromRenderImage();
	frameBuffers->LeavePrimary( false );
	bloom->ApplyBloom();
	return true;
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.
===============
*/
void RB_ShowImages( void ) {
	idImage	*image;
	float	x=-1, y=-1, w, h;

	GLSLProgram::Deactivate();
	GL_State( GLS_DEFAULT );
	GL_Cull( CT_TWO_SIDED );

	for ( int i = 0 ; i < globalImages->images.Num() ; i++ ) {
		image = globalImages->images[i];

		if ( image->texnum == idImage::TEXTURE_NOT_LOADED ) {
			continue;
		}
		w = h = 0.1f;
		// show in proportional size in mode 2
		if ( r_showImages.GetInteger() == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}
		qglEnable( GL_TEXTURE_2D );
		qglActiveTexture( 0 );
		image->Bind();
		qglBegin( GL_QUADS );
		qglTexCoord2f( 0, 0 );
		qglVertex2f( x, y );
		qglTexCoord2f( 1, 0 );
		qglVertex2f( x + w, y );
		qglTexCoord2f( 1, 1 );
		qglVertex2f( x + w, y + h );
		qglTexCoord2f( 0, 1 );
		qglVertex2f( x, y + h );
		qglEnd();
		x += w;
		if ( x >= 1 ) {
			y += 0.1f;
			x = -1;
		}
	}
	qglBindTexture( GL_TEXTURE_2D, 0 );
}

/*
=============
RB_SwapBuffers
=============
*/
void RB_SwapBuffers() {
	TRACE_CPU_SCOPE_COLOR( "SwapBuffers", TRACE_COLOR_IDLE )
	TRACE_GL_SCOPE( "SwapBuffers" )

	// texture swapping test
	if ( r_showImages.GetInteger() != 0 ) {
		RB_ShowImages();
	}

	// force a gl sync if requested
	if ( r_finish.GetBool() ) {
		qglFinish();
	}

	// don't flip if drawing to front buffer
	if ( !r_frontBuffer.GetBool() ) {
		GLimp_SwapBuffers();
	}
}

/*
=============
RB_CopyRender

Copy part of the current framebuffer to an image
=============
*/
bool RB_CopyRender( const void *data ) {
	if ( r_skipCopyTexture.GetBool() ) {
		return false;
	}
	const copyRenderCommand_t &cmd = *( copyRenderCommand_t * )data;

	if ( cmd.imageWidth * cmd.imageHeight == 0 )
		return false;
	frameBuffers->CopyRender( cmd );
	return true;
}

/*
====================
RB_ExecuteBackEndCommands

Always runs on the main thread
====================
*/
void RB_ExecuteBackEndCommands( const emptyCommand_t *cmds ) {
	static int backEndStartTime, backEndFinishTime;

	if ( cmds->commandId == RC_NOP && !cmds->next ) {
		return;
	}

	// r_debugRenderToTexture
	// revelator: added bloom to counters.
	int	c_draw3d = 0, c_draw2d = 0, c_setBuffers = 0, c_drawBloom = 0, c_copyRenders = 0;

	backEndStartTime = Sys_Milliseconds();

	// needed for editor rendering
	RB_SetDefaultGLState();

	// true: we currently render views to "primary framebuffer", which can be e.g. multisampled
	// false: we finished main rendering and now render to GUI framebuffer (or to default one directly)
	bool drawToPrimary = true;
	// if true, then the color output of the last view render should NOT be cleared at the start of the next view render
	bool outputColorIsBackground = false;

	while ( cmds ) {
		switch ( cmds->commandId ) {
		case RC_NOP:
			break;
		case RC_DRAW_VIEW: {
			backEnd.viewDef = ( ( const drawSurfsCommand_t * )cmds )->viewDef;
			bool isv3d = ( backEnd.viewDef->viewEntitys != nullptr );	// view is 2d or 3d
			if ( drawToPrimary ) {									// don't switch to FBO if bloom or some 2d has happened
				if ( isv3d ) {
					frameBuffers->EnterPrimary();
				} else {
					frameBuffers->LeavePrimary();	// switch to GUI or default FBO to render UI elements at native resolution
					FB_DebugShowContents();
					drawToPrimary = false;
				}
			}
			renderBackend->DrawView( backEnd.viewDef, outputColorIsBackground );
			GL_CheckErrors();
			outputColorIsBackground = backEnd.viewDef->outputColorIsBackground;
			if ( isv3d ) {
				c_draw3d++;
			} else {
				c_draw2d++;
			}
			break;
		}
		case RC_DRAW_LIGHTGEM:
			backEnd.viewDef = ( ( const drawLightgemCommand_t * )cmds )->viewDef;
			renderBackend->DrawLightgem( backEnd.viewDef, ( ( const drawLightgemCommand_t * )cmds )->dataBuffer );
			break;			
		case RC_SET_BUFFER:
			RB_SetBuffer( cmds );
			c_setBuffers++;
			break;
		case RC_BLOOM:
			if ( RB_Bloom( (bloomCommand_t*)cmds ) ) {
				c_drawBloom++;
				FB_DebugShowContents();
				frameBuffers->LeavePrimary();
				drawToPrimary = false;
			}
			break;
		case RC_COPY_RENDER:
			RB_CopyRender( cmds );
			c_copyRenders++;
			break;
		case RC_TONEMAP:
			// duzenko #4425: display the fbo content
			frameBuffers->LeavePrimary();
			renderBackend->Tonemap();
			break;
		default:
			common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
			break;
		}
		cmds = ( const emptyCommand_t * )cmds->next;
	}

	// go back to the default texture so the editor doesn't mess up a bound image
	qglBindTexture( GL_TEXTURE_2D, 0 );
	GL_CheckErrors();
	backEnd.glState.tmu[0].current2DMap = -1;

	// stop rendering on this thread
	backEndFinishTime = Sys_Milliseconds();
	backEnd.pc.msecLast = backEndFinishTime - backEndStartTime;
	backEnd.pc.msec += backEnd.pc.msecLast;

	// revelator: added depthcopy to counters
	if ( r_showRenderToTexture.GetBool() ) {
		common->Printf( "3d: %i, 2d: %i, SetBuf: %i, drwBloom: %i, CpyRenders: %i, CpyFrameBuf: %i, CpyDepthBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_drawBloom, c_copyRenders, backEnd.pc.c_copyFrameBuffer, backEnd.pc.c_copyDepthBuffer );
	}

	if ( image_showBackgroundLoads && backEnd.pc.textureLoads ) {
		common->Printf( "%i/%i loads in %i/%i ms\n", backEnd.pc.textureLoads, backEnd.pc.textureBackgroundLoads, backEnd.pc.textureLoadTime, backEnd.pc.textureUploadTime );
	}
}

#if 0
static void DumpDrawFramebuffer(const char *file)
{
	GLint read, draw, buffer;
	qglGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
	qglGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
	qglGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &buffer);
	qglBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	qglBindFramebuffer(GL_READ_FRAMEBUFFER, draw);
	RB_DumpFramebuffer(file);
	qglBindFramebuffer(GL_READ_FRAMEBUFFER, read);
	qglBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
}

void RB_DumpDrawFramebuffer(int i, const char *folder)
{
	if(!(r_ignore2.GetInteger() == i))
		return;
	static int ii=0;
	idStr str = va("%s_%s/%d.tga", folder, frameBuffers->activeDrawFbo->Name(), ii++);
	DumpDrawFramebuffer(str.c_str());
}

void RB_DumpReadFramebuffer(int i, const char *folder)
{
	if(!(r_ignore2.GetInteger() == i))
		return;
	static int ii=0;
	idStr str = va("%s_%s/%d.tga", folder, frameBuffers->activeFbo->Name(), ii++);
	GLint read, draw, buffer;
	qglGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &buffer);
	qglBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	RB_DumpFramebuffer(str.c_str());
	qglBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
}

#include "renderer/backend/glsl.h"
void RB_DumpTexImage(int i, const char *folder)
{
	if(!(r_ignore2.GetInteger() == i))
		return;

	GL_ViewportRelative( 0, 0, 1, 1 );
	GL_ScissorRelative( 0, 0, 1, 1 );

	GL_SetProjection( mat4_identity.ToFloatPtr() );

	GL_State( GLS_DEFAULT );

	programManager->oldStageShader->Activate();
	Uniforms::Global* transformUniforms = programManager->oldStageShader->GetUniformGroup<Uniforms::Global>();
	idMat4 ninety = mat4_identity * .9f;
	ninety[3][3] = 1;
	transformUniforms->modelViewMatrix.Set( ninety );

	GL_SelectTexture( 0 );
	/*
	switch ( r_showFBO.GetInteger() ) {
	case 1:
		globalImages->shadowAtlas->Bind();
		break;
	case 2:
		globalImages->currentDepthImage->Bind();
		break;
	case 3:
		globalImages->shadowDepthFbo->Bind();
		qglTexParameteri( GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT );
		break;
	case 4:
		ambientOcclusion->ShowSSAO();
		break;
	case 5:
		bloom->BindBloomTexture();
		break;
	default:
		globalImages->currentRenderImage->Bind();
	}
	*/
	globalImages->currentRenderImage->Bind();
	RB_DrawFullScreenQuad();
	transformUniforms->modelViewMatrix.Set( mat4_identity );
	GLSLProgram::Deactivate();

	static int ii=0;
	idStr str = va("%s_%s/%d.tga", folder, frameBuffers->activeDrawFbo->Name(), ii++);
	DumpDrawFramebuffer(str.c_str());
}
#endif
