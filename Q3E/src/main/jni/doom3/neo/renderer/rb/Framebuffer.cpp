/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2014 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../tr_local.h"
#include "Framebuffer.h"

idList<Framebuffer*>	Framebuffer::framebuffers;

globalFramebuffers_t globalFramebuffers;

static void R_ListFramebuffers_f( const idCmdArgs& args )
{
	if( !glConfig.framebufferObjectAvailable )
	{
		common->Printf( "GL_EXT_framebuffer_object is not available.\n" );
		return;
	}
}

Framebuffer::Framebuffer( const char* name, int w, int h )
{
	fboName = name;
	
	frameBuffer = 0;
	
	memset( colorBuffers, 0, sizeof( colorBuffers ) );
	colorFormat = 0;
	
	depthBuffer = 0;
	depthFormat = 0;
	
	stencilBuffer = 0;
	stencilFormat = 0;
	
	width = w;
	height = h;
	
	qglGenFramebuffers( 1, &frameBuffer );
	
	framebuffers.Append( this );
}

void Framebuffer::Init()
{
	cmdSystem->AddCommand( "listFramebuffers", R_ListFramebuffers_f, CMD_FL_RENDERER, "lists framebuffers" );
	
	backEnd.glState.currentFramebuffer = NULL;
	
	int width, height;
	width = height = r_shadowMapImageSize.GetInteger();
	
	for( int i = 0; i < MAX_SHADOWMAP_RESOLUTIONS; i++ )
	{
		width = height = shadowMapResolutions[i];

		char name[32];
		sprintf(name, "_shadowMap_%d", i);
		globalFramebuffers.shadowFBO[i] = new Framebuffer( name , width, height );
		globalFramebuffers.shadowFBO[i]->Bind();
#ifdef GL_ES_VERSION_3_0
		if(USING_GLES3)
		{
#ifdef SHADOW_MAPPING_DEBUG
			globalFramebuffers.shadowFBO[i]->AddColorBuffer(GL_RGBA8, 0); // for debug, not need render color buffer with OpenGLES3.0
#endif
			//globalFramebuffers.shadowFBO[i]->AddDepthBuffer(GL_DEPTH_COMPONENT24);
			//qglDrawBuffers( 0, NULL );
		}
		else
#endif
		{
			globalFramebuffers.shadowFBO[i]->AddColorBuffer(GL_RGBA8, 0); // GL_RGBA4 TODO: check OpenGLES2.0 support GL_RGBA8
			globalFramebuffers.shadowFBO[i]->AddDepthBuffer(glConfig.depth24Available ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT16);
		}
		common->Printf( "--- Framebuffer::Bind( name = '%s', handle = %d ) ---\n", globalFramebuffers.shadowFBO[i]->fboName.c_str(), globalFramebuffers.shadowFBO[i]->frameBuffer );
	}
//	globalFramebuffers.shadowFBO->AddColorBuffer( GL_RGBA8, 0 );
//	globalFramebuffers.shadowFBO->AddDepthBuffer( GL_DEPTH_COMPONENT24 );
//	globalFramebuffers.shadowFBO->Check();

	BindNull();
}

void Framebuffer::Shutdown()
{
	// TODO
}

void Framebuffer::Bind()
{
#if 1
	if( r_logFile.GetBool() )
	{
		RB_LogComment( "--- Framebuffer::Bind( name = '%s', handle = %d ) ---\n", fboName.c_str(), frameBuffer );
	}
#endif
	
	if( backEnd.glState.currentFramebuffer != this )
	{
		qglBindFramebuffer( GL_FRAMEBUFFER, frameBuffer );
		backEnd.glState.currentFramebuffer = this;
	}
}

void Framebuffer::BindNull()
{
	//if(backEnd.glState.framebuffer != NULL)
	{
		qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
		qglBindRenderbuffer( GL_RENDERBUFFER, 0 );
		backEnd.glState.currentFramebuffer = NULL;
	}
}

void Framebuffer::AddColorBuffer( int format, int index )
{
	if( index < 0 || index >= glConfig.maxColorAttachments )
	{
		common->Warning( "Framebuffer::AddColorBuffer( %s ): bad index = %i", fboName.c_str(), index );
		return;
	}
	
	colorFormat = format;
	
	bool notCreatedYet = colorBuffers[index] == 0;
	if( notCreatedYet )
	{
		qglGenRenderbuffers( 1, &colorBuffers[index] );
	}
	
	qglBindRenderbuffer( GL_RENDERBUFFER, colorBuffers[index] );
	qglRenderbufferStorage( GL_RENDERBUFFER, format, width, height );
	
	if( notCreatedYet )
	{
		qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_RENDERBUFFER, colorBuffers[index] );
	}
	
	GL_CheckErrors();
}

void Framebuffer::AddDepthBuffer( int format )
{
	depthFormat = format;
	
	bool notCreatedYet = depthBuffer == 0;
	if( notCreatedYet )
	{
		qglGenRenderbuffers( 1, &depthBuffer );
	}
	
	qglBindRenderbuffer( GL_RENDERBUFFER, depthBuffer );
	qglRenderbufferStorage( GL_RENDERBUFFER, format, width, height );
	
	if( notCreatedYet )
	{
		qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer );
	}
	
	GL_CheckErrors();
}

void Framebuffer::AttachColorBuffer( void )
{
	qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuffers[0] );
}

void Framebuffer::AttachDepthBuffer( void )
{
	qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer );
}

void Framebuffer::AttachImage2D( int target, const idImage* image, int index )
{
	if( ( target != GL_TEXTURE_2D ) && ( target < GL_TEXTURE_CUBE_MAP_POSITIVE_X || target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ) )
	{
		common->Warning( "Framebuffer::AttachImage2D( %s ): invalid target", fboName.c_str() );
		return;
	}
	
	if( index < 0 || index >= glConfig.maxColorAttachments )
	{
		common->Warning( "Framebuffer::AttachImage2D( %s ): bad index = %i", fboName.c_str(), index );
		return;
	}
	
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target, image->texnum, 0 );
}

void Framebuffer::AttachImageDepth( const idImage* image )
{
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, image->texnum, 0 );
}

void Framebuffer::AttachImageDepthLayer( const idImage* image, int layer )
{
#ifdef GL_ES_VERSION_3_0
	if(USING_GLES3)
	{
		qglFramebufferTextureLayer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, image->texnum, 0, layer );
	}
	else
#endif
    qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, image->texnum, 0 );
}

void Framebuffer::AttachImageDepthSide( const idImage* image, int side )
{
    qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, image->texnum, 0 );
}

void Framebuffer::AttachImage2D( const idImage* image )
{
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, image->texnum, 0 );
}

void Framebuffer::AttachImage2DLayer( const idImage* image, int layer )
{
#ifdef GL_ES_VERSION_3_0
	if(USING_GLES3)
	{
		qglFramebufferTextureLayer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, image->texnum, 0, layer );
	}
	else
#endif
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, image->texnum, 0 );
}

void Framebuffer::AttachImage2DSide( const idImage* image, int side )
{
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, image->texnum, 0 );
}

void Framebuffer::Check()
{
	int prev;
	qglGetIntegerv( GL_FRAMEBUFFER_BINDING, &prev );
	
	qglBindFramebuffer( GL_FRAMEBUFFER, frameBuffer );
	
	int status = qglCheckFramebufferStatus( GL_FRAMEBUFFER );
	if( status == GL_FRAMEBUFFER_COMPLETE )
	{
#if 0
		int type, handle;

		qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
		qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &handle);
		Sys_Printf("FrameBuffer[%s]: Color-0 current bind(0x%x %s), object handle(%d)\n", fboName.c_str(), type, (type == GL_RENDERBUFFER ? "RENDERBUFFER" : (type == GL_TEXTURE ? "TEXTURE" : "NONE")), handle);

		qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
		qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &handle);
		Sys_Printf("FrameBuffer[%s]: Depth current bind(0x%x %s), object handle(%d)\n", fboName.c_str(), type, (type == GL_RENDERBUFFER ? "RENDERBUFFER" : (type == GL_TEXTURE ? "TEXTURE" : "NONE")), handle);
#endif
		qglBindFramebuffer( GL_FRAMEBUFFER, prev );
		return;
	}
	
	// something went wrong
	switch( status )
	{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, incomplete attachment", fboName.c_str() );
			break;
			
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing attachment", fboName.c_str() );
			break;
#if 0
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing draw buffer", fboName.c_str() );
			break;
			
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing read buffer", fboName.c_str() );
			break;
			
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing layer targets", fboName.c_str() );
			break;
			
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing multisample", fboName.c_str() );
			break;
#endif
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, incomplete dimensions", fboName.c_str() );
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			common->Error( "Framebuffer::Check( %s ): Unsupported framebuffer format", fboName.c_str() );
			break;
		default:
			common->Error( "Framebuffer::Check( %s ): Unknown error 0x%X", fboName.c_str(), status );
			break;
	};
	
	qglBindFramebuffer( GL_FRAMEBUFFER, prev );
}
