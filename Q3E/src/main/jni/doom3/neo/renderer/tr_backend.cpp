/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

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
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"


frameData_t		*frameData;
backEndState_t	backEnd;


/*
======================
RB_SetDefaultGLState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
======================
*/
void RB_SetDefaultGLState(void)
{
	int		i;

	RB_LogComment("--- R_SetDefaultGLState ---\n");

	//
	// make sure our GL state vector is set correctly
	//
	memset(&backEnd.glState, 0, sizeof(backEnd.glState));
	backEnd.glState.forceGlState = true;

	GL_UseProgram(NULL);

	glClearDepthf(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
#if !defined(GL_ES_VERSION_2_0)
	glColor4f(1,1,1,1);
#endif

	glColorMask(1, 1, 1, 1);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glDisable(GL_DITHER);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_CULL_FACE);
#if !defined(GL_ES_VERSION_2_0)
	glDisable(GL_LIGHTING);
#endif
	glDisable(GL_STENCIL_TEST);

	glDepthMask(GL_TRUE);
	glDepthFunc(GL_ALWAYS);

	glCullFace(GL_FRONT_AND_BACK);
#if !defined(GL_ES_VERSION_2_0)
	glShadeModel(GL_SMOOTH);
#endif

	if (r_useScissor.GetBool()) {
		glScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	}

#if !defined(GL_ES_VERSION_2_0)
	for (i = glConfig.maxTextureUnits - 1 ; i >= 0 ; i--) {
		GL_SelectTexture(i);

		glDisable(GL_TEXTURE_2D);

		if (glConfig.texture3DAvailable) {
			glDisable(GL_TEXTURE_3D);
		}

		if (glConfig.cubeMapAvailable) {
			glDisable(GL_TEXTURE_CUBE_MAP);
		}
	}
#endif
}


/*
====================
RB_LogComment
====================
*/
void RB_LogComment(const char *comment, ...)
{
	va_list marker;

	if (!tr.logFile) {
		return;
	}

	fprintf(tr.logFile, "// ");
	va_start(marker, comment);
	vfprintf(tr.logFile, comment, marker);
	va_end(marker);
}


//=============================================================================



/*
====================
GL_SelectTexture
====================
*/
void GL_SelectTexture(int unit)
{
	if (backEnd.glState.currenttmu == unit) {
		return;
	}

#if 0
	if (unit < 0 || unit >= glConfig.maxTextureUnits && unit >= glConfig.maxTextureImageUnits) {
		common->Warning("GL_SelectTexture: unit = %i", unit);
		return;
	}
#endif

	glActiveTexture(GL_TEXTURE0 + unit);
#if !defined(GL_ES_VERSION_2_0)
	glClientActiveTexture(GL_TEXTURE0 + unit);
#endif
	RB_LogComment("glActiveTextureARB( %i );\nglClientActiveTextureARB( %i );\n", unit, unit);

	backEnd.glState.currenttmu = unit;
}

/*
====================
GL_UseProgram
====================
*/
void GL_UseProgram(shaderProgram_t *program)
{
	if (backEnd.glState.currentProgram == program) {
		return;
	}

#ifdef _HARM_SHADER_NAME
	RB_LogComment("Last shader program: %s, %p\n", backEnd.glState.currentProgram ? backEnd.glState.currentProgram->name : "NULL", backEnd.glState.currentProgram);
	RB_LogComment("Current shader program: %s, %p\n", program ? program->name : "NULL", program);
#endif
	glUseProgram(program ? program->program : 0);
	backEnd.glState.currentProgram = program;

	GL_CheckErrors();
}

/*
====================
GL_Uniform1fv
====================
*/
void GL_Uniform1fv(GLint location, const GLfloat *value)
{
	if (!backEnd.glState.currentProgram) {
		common->Printf("GL_Uniform1fv: no current program object\n");
		__builtin_trap();
		return;
	}

	glUniform1fv(*(GLint *)((char *)backEnd.glState.currentProgram + location), 1, value);

	GL_CheckErrors();
}

/*
====================
GL_Uniform4fv
====================
*/
void GL_Uniform4fv(GLint location, const GLfloat *value)
{
	if (!backEnd.glState.currentProgram) {
		common->Printf("GL_Uniform4fv: no current program object\n");
		__builtin_trap();
		return;
	}

	glUniform4fv(*(GLint *)((char *)backEnd.glState.currentProgram + location), 1, value);

	GL_CheckErrors();
}

/*
====================
GL_Uniform3fv
====================
*/
void GL_Uniform3fv(GLint location, const GLfloat *value)
{
	if (!backEnd.glState.currentProgram) {
		common->Printf("GL_Uniform4fv: no current program object\n");
		__builtin_trap();
		return;
	}

	glUniform3fv(*(GLint *)((char *)backEnd.glState.currentProgram + location), 1, value);

	GL_CheckErrors();
}

/*
====================
GL_UniformMatrix4fv
====================
*/
void GL_UniformMatrix4fv(GLint location, const GLfloat *value)
{
	if (!backEnd.glState.currentProgram) {
		common->Printf("GL_Uniform4fv: no current program object\n");
		__builtin_trap();
		return;
	}

	glUniformMatrix4fv(*(GLint *)((char *)backEnd.glState.currentProgram + location), 1, GL_FALSE, value);

	GL_CheckErrors();
}

/*
====================
GL_Uniform1f
====================
*/
void GL_Uniform1f(GLint location, GLfloat value)
{
	if (!backEnd.glState.currentProgram) {
		common->Printf("GL_Uniform1f: no current program object\n");
		__builtin_trap();
		return;
	}

	glUniform1f(*(GLint *)((char *)backEnd.glState.currentProgram + location), value);

	GL_CheckErrors();
}

/*
====================
GL_EnableVertexAttribArray
====================
*/
void GL_EnableVertexAttribArray(GLuint index)
{
	if (!backEnd.glState.currentProgram) {
		common->Printf("GL_EnableVertexAttribArray: no current program object\n");
		__builtin_trap();
		return;
	}

	if ((*(GLint *)((char *)backEnd.glState.currentProgram + index)) == -1) {
		common->Printf("GL_EnableVertexAttribArray: unbound attribute index\n");
#ifdef _HARM_SHADER_NAME
		RB_LogComment("Current shader program: %s, index: %d\n", backEnd.glState.currentProgram->name, index);
#endif
		__builtin_trap();
		return;
	}

	//RB_LogComment("glEnableVertexAttribArray( %i );\n", index);
	glEnableVertexAttribArray(*(GLint *)((char *)backEnd.glState.currentProgram + index));

	GL_CheckErrors();
}

/*
====================
GL_DisableVertexAttribArray
====================
*/
void GL_DisableVertexAttribArray(GLuint index)
{
	if (!backEnd.glState.currentProgram) {
		common->Printf("GL_DisableVertexAttribArray: no current program object\n");
		__builtin_trap();
		return;
	}

	if ((*(GLint *)((char *)backEnd.glState.currentProgram + index)) == -1) {
		common->Printf("GL_DisableVertexAttribArray: unbound attribute index\n");
#ifdef _HARM_SHADER_NAME
		RB_LogComment("Current shader program: %s, index: %d\n", backEnd.glState.currentProgram->name, index);
#endif
		__builtin_trap();
		return;
	}

	glDisableVertexAttribArray(*(GLint *)((char *)backEnd.glState.currentProgram + index));

	GL_CheckErrors();
}

/*
====================
GL_VertexAttribPointer
====================
*/
void GL_VertexAttribPointer(GLuint index, GLint size, GLenum type,
			    GLboolean normalized, GLsizei stride,
			    const GLvoid *pointer)
{
	if (!backEnd.glState.currentProgram) {
		common->Printf("GL_VertexAttribPointer: no current program object\n");
		__builtin_trap();
		return;
	}

	if ((*(GLint *)((char *)backEnd.glState.currentProgram + index)) == -1) {
		common->Printf("GL_VertexAttribPointer: unbound attribute index\n");
		__builtin_trap();
		return;
	}

	// RB_LogComment("glVertexAttribPointer( %i, %i, %i, %i, %i, %p );\n", index, size, type, normalized, stride, pointer);
	glVertexAttribPointer(*(GLint *)((char *)backEnd.glState.currentProgram + index),
	                      size, type, normalized, stride, pointer);

	GL_CheckErrors();
}

/*
====================
GL_Cull

This handles the flipping needed when the view being
rendered is a mirored view.
====================
*/
void GL_Cull(int cullType)
{
	if (backEnd.glState.faceCulling == cullType) {
		return;
	}

	if (cullType == CT_TWO_SIDED) {
		glDisable(GL_CULL_FACE);
	} else  {
		if (backEnd.glState.faceCulling == CT_TWO_SIDED) {
			glEnable(GL_CULL_FACE);
		}

		if (cullType == CT_BACK_SIDED) {
			if (backEnd.viewDef->isMirror) {
				glCullFace(GL_FRONT);
			} else {
				glCullFace(GL_BACK);
			}
		} else {
			if (backEnd.viewDef->isMirror) {
				glCullFace(GL_BACK);
			} else {
				glCullFace(GL_FRONT);
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
void GL_ClearStateDelta(void)
{
	backEnd.glState.forceGlState = true;
}

/*
====================
GL_State

This routine is responsible for setting the most commonly changed state
====================
*/
void GL_State(int stateBits)
{
	int	diff;

	if (!r_useStateCaching.GetBool() || backEnd.glState.forceGlState) {
		// make sure everything is set all the time, so we
		// can see if our delta checking is screwing up
		diff = -1;
		backEnd.glState.forceGlState = false;
	} else {
		diff = stateBits ^ backEnd.glState.glStateBits;

		if (!diff) {
			return;
		}
	}

	//
	// check depthFunc bits
	//
	if (diff & (GLS_DEPTHFUNC_EQUAL | GLS_DEPTHFUNC_LESS | GLS_DEPTHFUNC_ALWAYS)) {
		if (stateBits & GLS_DEPTHFUNC_EQUAL) {
			glDepthFunc(GL_EQUAL);
		} else if (stateBits & GLS_DEPTHFUNC_ALWAYS) {
			glDepthFunc(GL_ALWAYS);
		} else {
			glDepthFunc(GL_LEQUAL);
		}
	}


	//
	// check blend bits
	//
	if (diff & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) {
		GLenum srcFactor, dstFactor;

		switch (stateBits & GLS_SRCBLEND_BITS) {
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
#ifdef _RAVEN //k: quake4 blend
			case GLS_SRCBLEND_SRC_COLOR:
				srcFactor = GL_SRC_COLOR;
				break;
#endif
			default:
				srcFactor = GL_ONE;		// to get warning to shut up
				common->Error("GL_State: invalid src blend state bits\n");
				break;
		}

		switch (stateBits & GLS_DSTBLEND_BITS) {
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
				common->Error("GL_State: invalid dst blend state bits\n");
				break;
		}

		glBlendFunc(srcFactor, dstFactor);
	}

	//
	// check depthmask
	//
	if (diff & GLS_DEPTHMASK) {
		if (stateBits & GLS_DEPTHMASK) {
			glDepthMask(GL_FALSE);
		} else {
			glDepthMask(GL_TRUE);
		}
	}

	//
	// check colormask
	//
	if (diff & (GLS_REDMASK|GLS_GREENMASK|GLS_BLUEMASK|GLS_ALPHAMASK)) {
		GLboolean		r, g, b, a;
		r = (stateBits & GLS_REDMASK) ? 0 : 1;
		g = (stateBits & GLS_GREENMASK) ? 0 : 1;
		b = (stateBits & GLS_BLUEMASK) ? 0 : 1;
		a = (stateBits & GLS_ALPHAMASK) ? 0 : 1;
		glColorMask(r, g, b, a);
	}

	//
	// fill/line mode
	//
#if !defined(GL_ES_VERSION_2_0)
	if (diff & GLS_POLYMODE_LINE) {
		if (stateBits & GLS_POLYMODE_LINE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		} else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}
#endif

	//
	// alpha test
	//
#if !defined(GL_ES_VERSION_2_0)
	if (diff & GLS_ATEST_BITS) {
		switch (stateBits & GLS_ATEST_BITS) {
			case 0:
				glDisable(GL_ALPHA_TEST);
				break;
			case GLS_ATEST_EQ_255:
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_EQUAL, 1);
				break;
			case GLS_ATEST_LT_128:
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_LESS, 0.5);
				break;
			case GLS_ATEST_GE_128:
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GEQUAL, 0.5);
				break;
			default:
				assert(0);
				break;
		}
	}
#endif

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
void RB_SetGL2D(void)
{
	// set 2D virtual screen size
	glViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);

	if (r_useScissor.GetBool()) {
		glScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	}

	// always assume 640x480 virtual coordinates
#if !defined(GL_ES_VERSION_2_0)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif

	esOrtho((ESMatrix *)backEnd.viewDef->projectionMatrix, 0, 640, 480, 0, 0, 1);
	esMatrixLoadIdentity((ESMatrix *)backEnd.viewDef->worldSpace.modelViewMatrix);

	float	mat[16];
	myGlMultMatrix(backEnd.viewDef->worldSpace.modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
	GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);

	GL_State(GLS_DEPTHFUNC_ALWAYS |
	         GLS_SRCBLEND_SRC_ALPHA |
	         GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	GL_Cull(CT_TWO_SIDED);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
}



/*
=============
RB_SetBuffer

=============
*/
static void	RB_SetBuffer(const void *data)
{
	const setBufferCommand_t	*cmd;

	// see which draw buffer we want to render the frame to

	cmd = (const setBufferCommand_t *)data;

	backEnd.frameCount = cmd->frameCount;

#if !defined(GL_ES_VERSION_2_0)
	glDrawBuffer(cmd->buffer);
#endif

	// clear screen for debugging
	// automatically enable this with several other debug tools
	// that might leave unrendered portions of the screen
	if (r_clear.GetFloat() || idStr::Length(r_clear.GetString()) != 1 || r_lockSurfaces.GetBool() || r_singleArea.GetBool() || r_showOverDraw.GetBool()) {
		float c[3];

		if (sscanf(r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2]) == 3) {
			glClearColor(c[0], c[1], c[2], 1);
		} else if (r_clear.GetInteger() == 2) {
			glClearColor(0.0f, 0.0f,  0.0f, 1.0f);
		} else if (r_showOverDraw.GetBool()) {
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		} else {
			glClearColor(0.4f, 0.0f, 0.25f, 1.0f);
		}

		glClear(GL_COLOR_BUFFER_BIT);
	}
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.
===============
*/
void RB_ShowImages(void)
{
#if !defined(GL_ES_VERSION_2_0)
	int		i;
	idImage	*image;
	float	x, y, w, h;
	int		start, end;

	RB_SetGL2D();

	//glClearColor( 0.2, 0.2, 0.2, 1 );
	//glClear( GL_COLOR_BUFFER_BIT );

	glFinish();

	start = Sys_Milliseconds();

	for (i = 0 ; i < globalImages->images.Num() ; i++) {
		image = globalImages->images[i];

		if (image->texnum == idImage::TEXTURE_NOT_LOADED && image->partialImage == NULL) {
			continue;
		}

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if (r_showImages.GetInteger() == 2) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		image->Bind();
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(x, y);
		glTexCoord2f(1, 0);
		glVertex2f(x + w, y);
		glTexCoord2f(1, 1);
		glVertex2f(x + w, y + h);
		glTexCoord2f(0, 1);
		glVertex2f(x, y + h);
		glEnd();
	}

	glFinish();

	end = Sys_Milliseconds();
	common->Printf("%i msec to draw all images\n", end - start);
#endif
}


/*
=============
RB_SwapBuffers

=============
*/
const void	RB_SwapBuffers(const void *data)
{
	// texture swapping test
	if (r_showImages.GetInteger() != 0) {
		RB_ShowImages();
	}

	// force a gl sync if requested
	if (r_finish.GetBool()) {
		glFinish();
	}

	RB_LogComment("***************** RB_SwapBuffers *****************\n\n\n");

	// don't flip if drawing to front buffer
	if (!r_frontBuffer.GetBool()) {
		GLimp_SwapBuffers();
	}
}

/*
=============
RB_CopyRender

Copy part of the current framebuffer to an image
=============
*/
const void	RB_CopyRender(const void *data)
{
	const copyRenderCommand_t	*cmd;

	cmd = (const copyRenderCommand_t *)data;

	if (r_skipCopyTexture.GetBool()) {
		return;
	}

	RB_LogComment("***************** RB_CopyRender *****************\n");

	if (cmd->image) {
		cmd->image->CopyFramebuffer(cmd->x, cmd->y, cmd->imageWidth, cmd->imageHeight, false);
	}
}

/*
====================
RB_ExecuteBackEndCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
====================
*/
int		backEndStartTime, backEndFinishTime;
void RB_ExecuteBackEndCommands(const emptyCommand_t *cmds)
{
	// r_debugRenderToTexture
	int	c_draw3d = 0, c_draw2d = 0, c_setBuffers = 0, c_swapBuffers = 0, c_copyRenders = 0;

	if (cmds->commandId == RC_NOP && !cmds->next) {
		return;
	}

	backEndStartTime = Sys_Milliseconds();

	// needed for editor rendering
	RB_SetDefaultGLState();

	// upload any image loads that have completed
	globalImages->CompleteBackgroundImageLoads();

	for (; cmds ; cmds = (const emptyCommand_t *)cmds->next) {
		switch (cmds->commandId) {
			case RC_NOP:
				break;
			case RC_DRAW_VIEW:
				RB_DrawView(cmds);

				if (((const drawSurfsCommand_t *)cmds)->viewDef->viewEntitys) {
					c_draw3d++;
				} else {
					c_draw2d++;
				}

				break;
			case RC_SET_BUFFER:
				RB_SetBuffer(cmds);
				c_setBuffers++;
				break;
			case RC_SWAP_BUFFERS:
				RB_SwapBuffers(cmds);
				c_swapBuffers++;
				break;
			case RC_COPY_RENDER:
				RB_CopyRender(cmds);
				c_copyRenders++;
				break;
			default:
				common->Error("RB_ExecuteBackEndCommands: bad commandId");
				break;
		}
	}

	// go back to the default texture so the editor doesn't mess up a bound image
	glBindTexture(GL_TEXTURE_2D, 0);
	backEnd.glState.tmu[0].current2DMap = -1;

	// stop rendering on this thread
	backEndFinishTime = Sys_Milliseconds();
	backEnd.pc.msec = backEndFinishTime - backEndStartTime;

	if (r_debugRenderToTexture.GetInteger() == 1) {
		common->Printf("3d: %i, 2d: %i, SetBuf: %i, SwpBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_swapBuffers, c_copyRenders, backEnd.c_copyFrameBuffer);
		backEnd.c_copyFrameBuffer = 0;
	}
}

/*
=================
RB_ComputeMVP

Compute the model view matrix, with eventually required projection matrix depth hacks
=================
*/
void RB_ComputeMVP( const drawSurf_t * const surf, float mvp[16] ) {
	// Get the projection matrix
	float localProjectionMatrix[16];
	memcpy(localProjectionMatrix, backEnd.viewDef->projectionMatrix, sizeof(localProjectionMatrix));

	// Quick and dirty hacks on the projection matrix
	if ( surf->space->weaponDepthHack ) {
		localProjectionMatrix[14] = backEnd.viewDef->projectionMatrix[14] * 0.25;
	}
	if ( surf->space->modelDepthHack != 0.0 ) {
		localProjectionMatrix[14] = backEnd.viewDef->projectionMatrix[14] - surf->space->modelDepthHack;
	}

	// precompute the MVP
	myGlMultMatrix(surf->space->modelViewMatrix, localProjectionMatrix, mvp);
}

/*
====================
GL_UniformMatrix4fv
====================
*/
void GL_UniformMatrix4fv(GLint location, const GLsizei n, const GLfloat *value)
{
	if (!backEnd.glState.currentProgram) {
		common->Printf("GL_Uniform4fv: no current program object\n");
		__builtin_trap();
		return;
	}

	glUniformMatrix4fv(*(GLint *)((char *)backEnd.glState.currentProgram + location), n, GL_FALSE, value);

	GL_CheckErrors();
}

