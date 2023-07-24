// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __LIBQGL_GL_H__
#define __LIBQGL_GL_H__

#if defined( _WIN32 )

	#ifdef _XBOX
		#include "../Xenon/Sys/Xenon_GLStub.h"
	#else
		#include <gl/gl.h>
	#endif

#elif defined( __APPLE__ ) && defined( __MACH__ )

	// magic flag to keep tiger gl.h from loading glext.h
	#define GL_GLEXT_LEGACY
	#include <OpenGL/gl.h>

#elif defined( __linux__ )

	// use our local glext.h
	// http://oss.sgi.com/projects/ogl-sample/ABI/
	#define GL_GLEXT_LEGACY
	#define GLX_GLXEXT_LEGACY
	#include <GL/gl.h>
	#include <GL/glx.h>

	// the X11 headers have the VERY NASTY habit of using #define on some very common keywords..
	#undef CurrentTime
#else

	#include <gl.h>

#endif

#endif /* !__LIBQGL_GL_H__ */
