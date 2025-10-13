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

idList<idFramebuffer*>	Framebuffer::framebuffers;

#ifdef _SHADOW_MAPPING
globalFramebuffers_t globalFramebuffers;
#endif

static void R_ListFramebuffers_f( const idCmdArgs& args )
{
	if( !glConfig.framebufferObjectAvailable )
	{
		common->Printf( "GL_EXT_framebuffer_object is not available.\n" );
		return;
	}
}

idFramebuffer::idFramebuffer( const char* name, int w, int h )
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

    printf("idFramebuffer create: %s -> %d\n", fboName.c_str(), frameBuffer);
}

void idFramebuffer::Bind()
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

void idFramebuffer::Unbind()
{
#if 1
	if( r_logFile.GetBool() )
{
		RB_LogComment( "--- Framebuffer::Unbind( name = '%s', handle = %d ) ---\n", fboName.c_str(), frameBuffer );
	}
#endif
	
	if( backEnd.glState.currentFramebuffer == this )
	{
		qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
		backEnd.glState.currentFramebuffer = NULL;
	}
}

void idFramebuffer::BindDirectly()
{
#if 1
	if( r_logFile.GetBool() )
	{
		RB_LogComment( "--- Framebuffer::BindDirectly( name = '%s', handle = %d ) ---\n", fboName.c_str(), frameBuffer );
	}
#endif

	qglBindFramebuffer( GL_FRAMEBUFFER, frameBuffer );
	backEnd.glState.currentFramebuffer = this;
}

void idFramebuffer::UnbindDirectly()
{
#if 1
	if( r_logFile.GetBool() )
	{
		RB_LogComment( "--- Framebuffer::BindDirectly( name = '%s', handle = %d ) ---\n", fboName.c_str(), frameBuffer );
	}
#endif

	qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	backEnd.glState.currentFramebuffer = NULL;
}

void idFramebuffer::AddColorBuffer( int format, int index )
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

void idFramebuffer::AddDepthBuffer( int format )
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

void idFramebuffer::AddDepthStencilBuffer( int format )
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
		qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer );
	}
	
	GL_CheckErrors();
}

void idFramebuffer::AddStencilBuffer( int format )
{
	stencilFormat = format;
	
	bool notCreatedYet = stencilBuffer == 0;
	if( notCreatedYet )
	{
		qglGenRenderbuffers( 1, &stencilBuffer );
	}
	
	qglBindRenderbuffer( GL_RENDERBUFFER, stencilBuffer );
	qglRenderbufferStorage( GL_RENDERBUFFER, format, width, height );
	
	if( notCreatedYet )
	{
		qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilBuffer );
	}
	
	GL_CheckErrors();
}

void idFramebuffer::AttachColorBuffer( void )
{
	qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuffers[0] );
}

void idFramebuffer::AttachDepthBuffer( void )
{
	qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer );
}

void idFramebuffer::AttachDepthStencilBuffer( void )
{
	qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer );
}

void idFramebuffer::AttachStencilBuffer( void )
{
	qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilBuffer );
}

void idFramebuffer::AttachImage2D( int target, const idImage* image, int index )
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

void idFramebuffer::AttachImageDepth( const idImage* image )
{
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, image->texnum, 0 );
}

void idFramebuffer::AttachImageDepthStencil( const idImage* image )
{
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, image->texnum, 0 );
}

void idFramebuffer::AttachImageDepthLayer( const idImage* image, int layer )
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

void idFramebuffer::AttachImageDepthSide( const idImage* image, int side )
{
    qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, image->texnum, 0 );
}

void idFramebuffer::AttachImage2D( const idImage* image )
{
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, image->texnum, 0 );
}

void idFramebuffer::AttachImage2DLayer( const idImage* image, int layer )
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

void idFramebuffer::AttachImage2DSide( const idImage* image, int side )
{
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, image->texnum, 0 );
}

void idFramebuffer::Check()
{
	int prev;
	qglGetIntegerv( GL_FRAMEBUFFER_BINDING, &prev );
	
	qglBindFramebuffer( GL_FRAMEBUFFER, frameBuffer );
	
	int status = qglCheckFramebufferStatus( GL_FRAMEBUFFER );
	if( status == GL_FRAMEBUFFER_COMPLETE )
	{
		// PrintFramebuffer();
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

void idFramebuffer::Purge()
{
    if(frameBuffer > 0 && qglIsFramebuffer(frameBuffer))
    {
        qglBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        qglBindRenderbuffer( GL_RENDERBUFFER, 0 );

        int count = sizeof(colorBuffers) / sizeof(colorBuffers[0]);
        for(int i = 0; i < count; i++)
        {
            qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, 0 );

            uint32_t &handle = colorBuffers[i];
            if(handle > 0 && qglIsRenderbuffer(handle))
            {
                qglDeleteRenderbuffers(1, &handle);
            }
        }

        qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0 );
        qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0 );
        qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0 );

        if(depthBuffer && qglIsRenderbuffer(depthBuffer))
        {
            qglDeleteRenderbuffers(1, &depthBuffer);
        }
        if(stencilBuffer && qglIsRenderbuffer(stencilBuffer))
        {
            qglDeleteRenderbuffers(1, &stencilBuffer);
        }

        qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0 );
        qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0 );
        for(int i = 0; i < 6; i++)
        {
#ifdef GL_ES_VERSION_3_0
            if(USING_GLES3)
            {
                qglFramebufferTextureLayer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 0, 0, i );
                qglFramebufferTextureLayer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, i );
            }
            else
#endif
            {
                qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0 );
                qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0 );
            }
        }

        qglDeleteFramebuffers(1, &frameBuffer);

        qglBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    printf("idFramebuffer purge: %s -> %d\n", fboName.c_str(), frameBuffer);

    frameBuffer = 0;

    memset( colorBuffers, 0, sizeof( colorBuffers ) );
    colorFormat = 0;

    depthBuffer = 0;
    depthFormat = 0;

    stencilBuffer = 0;
    stencilFormat = 0;
}

void idFramebuffer::PrintFramebuffer(void)
{
	printf("Get framebuffer: %s\n", fboName.c_str());
	GLint value[4] = {0};
	for(int i = 0; i < 6; i++)
	{
		qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, value);
		if(value[0] != GL_NONE)
			qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, value + 1);
		else
			value[1] = 0;
		printf("GL_COLOR_ATTACHMENT%d::OBJECT_TYPE -> %x %d\n", i, value[0], value[1]);
	}

	qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, value);
	if(value[0] != GL_NONE)
		qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, value + 1);
	else
		value[1] = 0;
	printf("GL_DEPTH_ATTACHMENT::OBJECT_TYPE -> %x %d\n", value[0], value[1]);

	qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, value);
	if(value[0] != GL_NONE)
		qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, value + 1);
	else
		value[1] = 0;
	printf("GL_STENCIL_ATTACHMENT::OBJECT_TYPE -> %x %d\n", value[0], value[1]);

	qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, value);
	if(value[0] != GL_NONE)
		qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, value + 1);
	else
		value[1] = 0;
	printf("GL_DEPTH_STENCIL_ATTACHMENT::OBJECT_TYPE -> %x %d\n", value[0], value[1]);
}



void Framebuffer::Init()
{
	cmdSystem->AddCommand( "listFramebuffers", R_ListFramebuffers_f, CMD_FL_RENDERER, "lists framebuffers" );
	
	backEnd.glState.currentFramebuffer = NULL;
	
#ifdef _SHADOW_MAPPING
	int width, height;
	
	for( int i = 0; i < MAX_SHADOWMAP_RESOLUTIONS; i++ )
	{
		width = height = shadowMapResolutions[i];

		char name[32];
		sprintf(name, "_shadowMap_%d", i);
		globalFramebuffers.shadowFBO[i] = new idFramebuffer( name , width, height );

		framebuffers.Append( globalFramebuffers.shadowFBO[i] );

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
		common->Printf( "--- Framebuffer::Bind( name = '%s', handle = %d, size = %d x %d ) ---\n", globalFramebuffers.shadowFBO[i]->fboName.c_str(), globalFramebuffers.shadowFBO[i]->frameBuffer, globalFramebuffers.shadowFBO[i]->width, globalFramebuffers.shadowFBO[i]->height );
	}
//	globalFramebuffers.shadowFBO->AddColorBuffer( GL_RGBA8, 0 );
//	globalFramebuffers.shadowFBO->AddDepthBuffer( GL_DEPTH_COMPONENT24 );
//	globalFramebuffers.shadowFBO->Check();
#endif

	BindNull();

    // if(USING_GLES31)
    if(idStencilTexture::IsAvailable())
    {
        printf("Stencil texture creating: %d x %d\n", glConfig.vidWidth, glConfig.vidHeight);
        stencilTexture.Init(glConfig.vidWidth, glConfig.vidHeight);
    }

    BindNull();
}

void Framebuffer::Shutdown()
{
    printf("Framebuffer shutdown: %d\n", framebuffers.Num());
	for(int i = 0; i < framebuffers.Num(); i++)
	{
		idFramebuffer *fb = framebuffers[i];
		if(fb)
		{
			fb->Purge();
			delete fb;
		}
	}
	framebuffers.Clear();

    printf("Shadow map framebuffer purged\n");
    memset(globalFramebuffers.shadowFBO, 0, sizeof(globalFramebuffers.shadowFBO));

    // if(USING_GLES31)
    if(idStencilTexture::IsAvailable())
    {
        printf("Stencil texture purged\n");
        stencilTexture.Shutdown();
    }
}

void Framebuffer::BindNull()
{
	{
		qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
		qglBindRenderbuffer( GL_RENDERBUFFER, 0 );
		backEnd.glState.currentFramebuffer = NULL;
	}
}

void Framebuffer::Default()
{
	qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	backEnd.glState.currentFramebuffer = NULL;
}

void Framebuffer::Append(idFramebuffer *fb)
{
	if(!fb)
		return;
	if(framebuffers.FindIndex(fb) != -1)
		return;
	framebuffers.Append(fb);
}

idFramebuffer *	Framebuffer::Find(const char *name)
{
	for(int i = 0; i < framebuffers.Num(); i++)
	{
		idFramebuffer *fb = framebuffers[i];
		if(!idStr::Cmp(fb->GetName(), name))
			return fb;
	}
	return NULL;
}

idFramebuffer * Framebuffer::Alloc(const char *name, int width, int height)
{
	idFramebuffer *fb = Find(name);
	if(fb)
		return fb;

	fb = new idFramebuffer( name, width, height );
	framebuffers.Append( fb );
	return fb;
}
