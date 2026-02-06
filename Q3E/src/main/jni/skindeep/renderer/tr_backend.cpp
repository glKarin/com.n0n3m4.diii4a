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

#include "renderer/tr_local.h"



frameData_t		*frameData;
backEndState_t	backEnd;

/*
======================
RB_SetDefaultGLState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
======================
*/
void RB_SetDefaultGLState( void ) {
	int		i;

	qglClearDepth( 1.0f );
	qglColor4f (1,1,1,1);

	// the vertex array is always enabled
	qglEnableClientState( GL_VERTEX_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglDisableClientState( GL_COLOR_ARRAY );

	//
	// make sure our GL state vector is set correctly
	//
	memset( &backEnd.glState, 0, sizeof( backEnd.glState ) );
	backEnd.glState.forceGlState = true;

	qglColorMask( 1, 1, 1, 1 );

	qglEnable( GL_DEPTH_TEST );
	qglEnable( GL_BLEND );
	qglEnable( GL_SCISSOR_TEST );
	qglEnable( GL_CULL_FACE );
#if !defined(_GLES) //karin: no GL_LIGHTING/GL_LINE_STIPPLE in programming pipeline
	qglDisable( GL_LIGHTING );
	qglDisable( GL_LINE_STIPPLE );
#endif
	qglDisable( GL_STENCIL_TEST );

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask( GL_TRUE );
	qglDepthFunc( GL_ALWAYS );

	qglCullFace( GL_FRONT_AND_BACK );
	qglShadeModel( GL_SMOOTH );

	if ( r_useScissor.GetBool() ) {
		RB_SetScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	}

	for ( i = glConfig.maxTextureUnits - 1 ; i >= 0 ; i-- ) {
		GL_SelectTexture( i );

		// object linear texgen is our default
// 		qglTexGenf( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
// 		qglTexGenf( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
// 		qglTexGenf( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
// 		qglTexGenf( GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );

		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		if ( glConfig.texture3DAvailable ) {
			qglDisable( GL_TEXTURE_3D );
		}
		if ( glConfig.cubeMapAvailable ) {
			qglDisable( GL_TEXTURE_CUBE_MAP_EXT );
		}
	}
}




//=============================================================================



/*
====================
GL_SelectTexture
====================
*/
void GL_SelectTexture( int unit ) {
	if ( backEnd.glState.currenttmu == unit ) {
		return;
	}

	if ( unit < 0 || (unit >= glConfig.maxTextureUnits && unit >= glConfig.maxTextureImageUnits) ) {
		common->Warning( "GL_SelectTexture: unit = %i", unit );
		return;
	}

	qglActiveTextureARB( GL_TEXTURE0_ARB + unit );
	//qglClientActiveTextureARB( GL_TEXTURE0_ARB + unit );

	backEnd.glState.currenttmu = unit;
}


/*
====================
GL_Cull

This handles the flipping needed when the view being
rendered is a mirored view.
====================
*/
void GL_Cull( int cullType ) {
	if ( backEnd.glState.faceCulling == cullType ) {
		return;
	}

	if ( cullType == CT_TWO_SIDED ) {
		qglDisable( GL_CULL_FACE );
	} else  {
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
====================
GL_TexEnv
====================
*/
void GL_TexEnv( int env ) {
	tmu_t	*tmu;

	tmu = &backEnd.glState.tmu[backEnd.glState.currenttmu];
	if ( env == tmu->texEnv ) {
		return;
	}

	tmu->texEnv = env;

	switch ( env ) {
	case GL_COMBINE_EXT:
	case GL_MODULATE:
	case GL_REPLACE:
	case GL_DECAL:
	case GL_ADD:
		//qglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, env );
		break;
	default:
		common->Error( "GL_TexEnv: invalid env '%d' passed\n", env );
		break;
	}
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
void GL_State( int stateBits ) {
	int	diff;

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

	//
	// check depthFunc bits
	//
	if ( diff & ( GLS_DEPTHFUNC_EQUAL | GLS_DEPTHFUNC_LESS | GLS_DEPTHFUNC_ALWAYS ) ) {
		if ( stateBits & GLS_DEPTHFUNC_EQUAL ) {
			qglDepthFunc( GL_EQUAL );
		} else if ( stateBits & GLS_DEPTHFUNC_ALWAYS ) {
			qglDepthFunc( GL_ALWAYS );
		} else {
			qglDepthFunc( GL_LEQUAL );
		}
	}


	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) {
		GLenum srcFactor, dstFactor;

		switch ( stateBits & GLS_SRCBLEND_BITS ) {
		case GLS_SRCBLEND_ZERO:
			srcFactor = GL_ZERO;
			break;
		case GLS_SRCBLEND_ONE:
			srcFactor = GL_ONE;
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

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK ) {
		if ( stateBits & GLS_DEPTHMASK ) {
			qglDepthMask( GL_FALSE );
		} else {
			qglDepthMask( GL_TRUE );
		}
	}

	//
	// check colormask
	//
	if ( diff & (GLS_REDMASK|GLS_GREENMASK|GLS_BLUEMASK|GLS_ALPHAMASK) ) {
		GLboolean		r, g, b, a;
		r = ( stateBits & GLS_REDMASK ) ? 0 : 1;
		g = ( stateBits & GLS_GREENMASK ) ? 0 : 1;
		b = ( stateBits & GLS_BLUEMASK ) ? 0 : 1;
		a = ( stateBits & GLS_ALPHAMASK ) ? 0 : 1;
		qglColorMask( r, g, b, a );
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE ) {
		if ( stateBits & GLS_POLYMODE_LINE ) {
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		} else {
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// alpha test
	//
	if ( diff & GLS_ATEST_BITS ) {
		switch ( stateBits & GLS_ATEST_BITS ) {
		case 0:
			qglDisable( GL_ALPHA_TEST );
			break;
		case GLS_ATEST_EQ_255:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_EQUAL, 1 );
			break;
		case GLS_ATEST_LT_128:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_LESS, 0.5 );
			break;
		case GLS_ATEST_GE_128:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.5 );
			break;
		default:
			assert( 0 );
			break;
		}
	}

	backEnd.glState.glStateBits = stateBits;
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
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	if ( r_useScissor.GetBool() ) {
		RB_SetScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
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

	cmd = (const setBufferCommand_t *)data;

	backEnd.frameCount = cmd->frameCount;

	//SM: Don't actually do this here because it's handled by the FBO
	//qglDrawBuffer( cmd->buffer );

	// Before doing the clear, make all attachments writable
	GLenum colorBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	qglDrawBuffers(3, colorBuffers);

	// clear screen for debugging
	// automatically enable this with several other debug tools
	// that might leave unrendered portions of the screen
	if ( r_clear.GetFloat() || idStr::Length( r_clear.GetString() ) != 1 || r_lockSurfaces.GetBool() || r_singleArea.GetBool() || r_showOverDraw.GetBool() ) {
		float c[4];
		c[3] = 1.0f;
		if ( sscanf( r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2] ) == 3 ) {
		} else if ( r_clear.GetInteger() == 2 ) {
			c[0] = c[1] = c[2] = 0.0f; // Black
		} else if ( r_showOverDraw.GetBool() ) {
			c[0] = c[1] = c[2] = 1.0f; // White
		} else {
			// Purple
			c[0] = c[2] = 1.0f;
			c[1] = 0.0f;
		}

		qglClearColor(c[0], c[1], c[2], c[3]);
		qglClearBufferfv(GL_COLOR, 0, c);
	}

	// Clear post process mask and light texture
	GLuint clearValues[] = { 0, 0, 0, 0 };
	qglClearBufferuiv(GL_COLOR, 1, clearValues);

	GLfloat clearFloat[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	qglClearBufferfv(GL_COLOR, 2, clearFloat);
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.
===============
*/
void RB_ShowImages( void ) {
#if !defined(_GLES) //karin: disable r_showImages debug rendering
	int		i;
	idImage	*image;
	float	x, y, w, h;
	int		start, end;

	RB_SetGL2D();

	//qglClearColor( 0.2, 0.2, 0.2, 1 );
	//qglClear( GL_COLOR_BUFFER_BIT );

	qglFinish();

	start = Sys_Milliseconds();

	for ( i = 0 ; i < globalImages->images.Num() ; i++ ) {
		image = globalImages->images[i];

		if ( image->texnum == idImage::TEXTURE_NOT_LOADED && image->partialImage == NULL ) {
			continue;
		}

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages.GetInteger() == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		image->Bind();
		qglBegin (GL_QUADS);
		qglTexCoord2f( 0, 0 );
		qglVertex2f( x, y );
		qglTexCoord2f( 1, 0 );
		qglVertex2f( x + w, y );
		qglTexCoord2f( 1, 1 );
		qglVertex2f( x + w, y + h );
		qglTexCoord2f( 0, 1 );
		qglVertex2f( x, y + h );
		qglEnd();
	}

	qglFinish();

	end = Sys_Milliseconds();
	common->Printf( "%i msec to draw all images\n", end - start );
#endif
}


/*
=============
RB_SwapBuffers

=============
*/
const void	RB_SwapBuffers( const void *data ) {
	// texture swapping test
	if ( r_showImages.GetInteger() != 0 ) {
		RB_ShowImages();
	}

	// force a gl sync if requested
	if ( r_finish.GetBool() ) {
		qglFinish();
	}

	qglBindFramebuffer(GL_READ_FRAMEBUFFER, backEnd.frameBuffersIdsAllocated[backEnd.frameBufferId]);
	qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	qglBlitFramebuffer(0, 0, glConfig.vidWidth, glConfig.vidHeight,
		0, 0, glConfig.vidWidth, glConfig.vidHeight,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	backEnd.frameBufferId = FRAME_DEFAULT_BACKBUFFER;

	GLimp_SwapBuffers();
}

/*
=============
RB_CopyRender

Copy part of the current framebuffer to an image
=============
*/
const void	RB_CopyRender( const void *data ) {
	const copyRenderCommand_t	*cmd;

	cmd = (const copyRenderCommand_t *)data;

	if ( r_skipCopyTexture.GetBool() ) {
		return;
	}

	if (cmd->image) {
		cmd->image->CopyFramebuffer( cmd->x, cmd->y, cmd->imageWidth, cmd->imageHeight, false );
	}
}



/*
=============
RB_SetFrameBuffer

blendo eric: targets the frame buffer id
=============
*/
static void	RB_SetFrameBuffer(const void *data)
{
	const setFrameBufferCommand_t	*cmd;
	cmd = (const setFrameBufferCommand_t *)data;

	FrameBufferIds_t frameBufferIdIndex = cmd->frameBufferId;

	// check if swap required
	if (backEnd.frameBufferId != frameBufferIdIndex)
	{
		if ( frameBufferIdIndex != 0
			&& backEnd.frameBuffersIdsAllocated[(int)frameBufferIdIndex] == 0 )
		{ // alloc new frame buffer
			unsigned int fbo = 0;
			unsigned int texture = 0;
			unsigned int depthStencil = 0;
			unsigned int customMaskTexture = 0;
			unsigned int lightTexture = 0;
			qglGenFramebuffers(1, &fbo);
			qglBindFramebuffer(GL_FRAMEBUFFER, fbo);

			// create buffer texture
			qglGenTextures(1, &texture);
			qglBindTexture(GL_TEXTURE_2D, texture);
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glConfig.vidWidth, glConfig.vidHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#ifdef _DEBUG
			if (glConfig.khrDebugAvailable && r_debugGLContext.GetBool()) {
				idStr name = idStr::Format("Color FBO #%d", fbo);
				qglObjectLabel(GL_TEXTURE, texture, name.Length(), name.c_str());
			}
#endif

			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

			// create buffer depth/stencil
			// (except for the post-process fbo which just needs a color target)
			if (frameBufferIdIndex != FRAME_POSTPROCESSVIEW) {
				qglGenTextures(1, &depthStencil);
				qglBindTexture(GL_TEXTURE_2D, depthStencil);
				qglTexImage2D(
					GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, glConfig.vidWidth, glConfig.vidHeight, 0,
					GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
				);

				qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#ifdef _DEBUG
				if (glConfig.khrDebugAvailable && r_debugGLContext.GetBool()) {
					idStr name = idStr::Format("Depth/Stencil FBO #%d", fbo);
					qglObjectLabel(GL_TEXTURE, texture, name.Length(), name.c_str());
				}
#endif
				qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthStencil, 0);

				if (frameBufferIdIndex == FRAME_MAINVIEW)
				{
					globalImages->currentDepthImage->texnum = depthStencil;

					// Create the texture for the customPostMask
					qglGenTextures(1, &customMaskTexture);
					globalImages->currentCustomImage->texnum = customMaskTexture;

					qglBindTexture(GL_TEXTURE_2D, customMaskTexture);
					qglTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, glConfig.vidWidth, glConfig.vidHeight, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					// 5-25-2025 NOTE: because this is an integer texture, it must be the highest numbered
					//       color attachment we use, to work around a Mesa bug that broke
					//       blending when it thought an integer color attachment was bound
					//       due to a buggy check (this works around the buggy check)
					//       https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/34990
					qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, customMaskTexture, 0);
					
					qglGenTextures(1, &lightTexture);
					globalImages->currentLightImage->texnum = lightTexture;

					qglBindTexture(GL_TEXTURE_2D, lightTexture);
					qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, glConfig.vidWidth, glConfig.vidHeight, 0, GL_RGB, GL_FLOAT, NULL);
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lightTexture, 0);
				}
			}

			assert(qglCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

			backEnd.frameBuffersIdsAllocated[(int)frameBufferIdIndex] = fbo;
			backEnd.frameBuffersTextureIdsAllocated[(int)frameBufferIdIndex] = texture;
			backEnd.frameBuffersDepthIdsAllocated[(int)frameBufferIdIndex] = depthStencil;
			backEnd.frameBuffersCustomMaskAllocated[(int)frameBufferIdIndex] = customMaskTexture;
			backEnd.frameBuffersLightAllocated[(int)frameBufferIdIndex] = lightTexture;
		}
		else
		{ // bind already allocd buffer
			qglBindFramebuffer(GL_FRAMEBUFFER, backEnd.frameBuffersIdsAllocated[(int)frameBufferIdIndex]);
		}

		// If we're changing between main and postprocess, we need to make sure
		// we update the texture number the next postprocess pass reads from
		if (frameBufferIdIndex == FRAME_MAINVIEW || frameBufferIdIndex == FRAME_POSTPROCESSVIEW)
		{
			globalImages->currentPostProcessImage->texnum = backEnd.frameBuffersTextureIdsAllocated[(int)backEnd.frameBufferId];
		}
		backEnd.frameBufferId = frameBufferIdIndex;
	}
}

/*
====================
RB_ExecuteBackEndCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
====================
*/
uint64		backEndStartTime, backEndFinishTime;
void RB_ExecuteBackEndCommands( const emptyCommand_t *cmds ) {
	// r_debugRenderToTexture
	int	c_draw3d = 0, c_draw2d = 0, c_setBuffers = 0, c_swapBuffers = 0, c_copyRenders = 0, c_setFrameBuffers = 0;

	if ( cmds->commandId == RC_NOP && !cmds->next ) {
		return;
	}

	backEndStartTime = Sys_GetPerformanceCounter();

	// needed for editor rendering
	RB_SetDefaultGLState();


	// upload any image loads that have completed
	globalImages->CompleteBackgroundImageLoads();

	for ( ; cmds ; cmds = (const emptyCommand_t *)cmds->next ) {
		switch ( cmds->commandId ) {
		case RC_NOP:
			break;
		case RC_DRAW_VIEW:
			RB_DrawView( cmds );
			if ( ((const drawSurfsCommand_t *)cmds)->viewDef->viewEntitys ) {
				c_draw3d++;
			}
			else {
				c_draw2d++;
			}
			break;
		case RC_SET_BUFFER:
			RB_SetBuffer( cmds );
			c_setBuffers++;
			break;
		case RC_SWAP_BUFFERS:



			RB_SwapBuffers( cmds );
			c_swapBuffers++;
			break;
		case RC_COPY_RENDER:
			RB_CopyRender( cmds );
			c_copyRenders++;
			break;
		case RC_SET_FRAME_BUFFER:
			RB_SetFrameBuffer(cmds);
			c_setFrameBuffers++;
			break;
		case RC_POST_EFFECT:
		{	
			// Swap to the correct framebuffer
			setFrameBufferCommand_t swapCmd;
			swapCmd.frameBufferId = backEnd.frameBufferId == FRAME_MAINVIEW ? FRAME_POSTPROCESSVIEW : FRAME_MAINVIEW;
			RB_SetFrameBuffer(&swapCmd);
			break;
		}
		default:
			common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
			break;
		}
	}

	// go back to the default texture so the editor doesn't mess up a bound image
	qglBindTexture( GL_TEXTURE_2D, 0 );
	backEnd.glState.tmu[0].current2DMap = -1;

	// stop rendering on this thread
	backEndFinishTime = Sys_GetPerformanceCounter();
	backEnd.pc.msec =Sys_GetPerformanceTimeMS(backEndFinishTime - backEndStartTime);

	if ( r_debugRenderToTexture.GetInteger() == 1 ) {
		common->Printf( "3d: %i, 2d: %i, SetBuf: %i, SwpBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_swapBuffers, c_copyRenders, backEnd.c_copyFrameBuffer );
		backEnd.c_copyFrameBuffer = 0;
	}
}
