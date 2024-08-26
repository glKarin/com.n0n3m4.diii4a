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

#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

class Framebuffer;

class idFramebuffer
{
public:

	idFramebuffer( const char* name, int width, int height );
	
	// deletes OpenGL object but leaves structure intact for reloading
	void					Purge();
	
	void					Bind();
	void					Unbind();
	void					BindDirectly();
	void					UnbindDirectly();
	
	void					AddColorBuffer( int format, int index );
	void					AddDepthBuffer( int format );
	void					AddDepthStencilBuffer( int format );
	void					AddStencilBuffer( int format );
	void					AttachColorBuffer( void );
	void					AttachDepthBuffer( void );
	void					AttachDepthStencilBuffer( void );
	void					AttachStencilBuffer( void );
	
	void					AttachImage2D( int target, const idImage* image, int index = 0 );
	void					AttachImageDepth( const idImage* image );
	void					AttachImageDepthStencil( const idImage* image );
	void					AttachImageDepthLayer( const idImage* image, int layer );
	void					AttachImage2DLayer( const idImage* image, int layer );
	void					AttachImage2D( const idImage* image );
	void					AttachImageDepthSide( const idImage* image, int side );
	void					AttachImage2DSide( const idImage* image, int side );
	
	// check for OpenGL errors
	void					Check();
	uint32_t				GetFramebuffer() const
	{
		return frameBuffer;
	}
	const char *			GetName() const {
		return fboName.c_str();
	}

private:
	void					PrintFramebuffer(void);
	
public:
	idStr					fboName;
	
	// FBO object
	uint32_t				frameBuffer;

	uint32_t				colorBuffers[16];
	int						colorFormat;

	uint32_t				depthBuffer;
	int						depthFormat;

	uint32_t				stencilBuffer;
	int						stencilFormat;
	
	int						width;
	int						height;
	
	friend class Framebuffer;
};

class Framebuffer
{
public:
	static void				Init();
	static void				Shutdown();
	static void				BindNull();
	static void				Default();
	static void				Append(idFramebuffer *fb);
	static idFramebuffer *	Alloc(const char *name, int width, int height);
	static idFramebuffer *	Find(const char *name);

	static idList<idFramebuffer*>	framebuffers;
};

#ifdef _SHADOW_MAPPING
#if 1
static	int shadowMapResolutions[MAX_SHADOWMAP_RESOLUTIONS] = { 2048, 1024, 512, 512, 256 };
#else
static	int shadowMapResolutions[MAX_SHADOWMAP_RESOLUTIONS] = { 1024, 1024, 1024, 1024, 1024 };
#endif
struct globalFramebuffers_t
{
	idFramebuffer*				shadowFBO[MAX_SHADOWMAP_RESOLUTIONS];
};
extern globalFramebuffers_t globalFramebuffers;
#endif

#endif // __FRAMEBUFFER_H__
