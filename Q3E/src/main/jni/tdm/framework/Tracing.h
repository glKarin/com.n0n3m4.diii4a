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

#pragma once

#include "renderer/backend/qgl/qgl.h"
#include <TracyOpenGL.hpp>
#include <common/TracySystem.hpp>

extern idCVar r_useDebugGroups;

void GL_SetDebugLabel(GLenum identifier, GLuint name, const idStr &label );
void GL_SetDebugLabel(void *ptr, const idStr &label );

void InitTracing();
void InitOpenGLTracing();
void TracingEndFrame();

extern bool g_tracingEnabled;
extern bool g_tracingAllocStacks;
extern bool g_glTraceInitialized;

#define TRACE_THREAD_NAME( name ) if ( g_tracingEnabled ) tracy::SetThreadName( name );
#define TRACE_PLOT_NUMBER( name, value ) if ( g_tracingEnabled ) { TracyPlot( name, value ); TracyPlotConfig( name, tracy::PlotFormatType::Number ); }
#define TRACE_PLOT_BYTES( name, value ) if ( g_tracingEnabled ) { TracyPlot( name, value ); TracyPlotConfig( name, tracy::PlotFormatType::Memory ); }
#define TRACE_PLOT_FRACTION( name, value ) if ( g_tracingEnabled ) { TracyPlot( name, value*100 ); TracyPlotConfig( name, tracy::PlotFormatType::Percentage ); }

#define TRACE_COLOR_IDLE 0x808080

//zones/scopes to measure and display as interval task
#define TRACE_CPU_SCOPE( section ) ZoneNamedN( __tracy_scoped_zone, section, g_tracingEnabled )
#define TRACE_CPU_SCOPE_COLOR( section, color ) ZoneNamedNC( __tracy_scoped_zone, section, color, g_tracingEnabled )

//set text of the currently active zone (overwrite)
#define TRACE_ATTACH_TEXT( text ) if ( g_tracingEnabled ) { \
	const char *__tmp_cstr = text; \
	ZoneTextV( __tracy_scoped_zone, __tmp_cstr, strlen(__tmp_cstr) ) \
}
#define TRACE_ATTACH_STR( text ) if ( g_tracingEnabled ) { \
	const idStr &__tmp_str = text; \
	ZoneTextV( __tracy_scoped_zone, __tmp_str.c_str(), __tmp_str.Length() ) \
}
#define TRACE_ATTACH_FORMAT( ... ) if ( g_tracingEnabled ) { \
	char __tracy_scoped_buffer[1024]; \
	int __tracy_scoped_len = idStr::snPrintf(__tracy_scoped_buffer, 1024, __VA_ARGS__); \
	ZoneTextV( __tracy_scoped_zone, __tracy_scoped_buffer, __tracy_scoped_len ) \
}

//create zone with text attached immediately
#define TRACE_CPU_SCOPE_TEXT( section, text_cstr ) \
	TRACE_CPU_SCOPE( section ) \
	TRACE_ATTACH_TEXT( text_cstr )
#define TRACE_CPU_SCOPE_STR( section, text_idstr ) \
	TRACE_CPU_SCOPE( section ) \
	TRACE_ATTACH_STR( text_idstr )
#define TRACE_CPU_SCOPE_FORMAT( section, ... ) \
	TRACE_CPU_SCOPE( section ) \
	TRACE_ATTACH_FORMAT( __VA_ARGS__ )

//DSCOPE versions can have name generated at runtime
#define TRACE_CPU_DSCOPE( section ) ZoneTransientN( __tracy_scoped_zone, section, g_tracingEnabled )
#define TRACE_CPU_DSCOPE_TEXT( section, text_cstr ) \
	TRACE_CPU_DSCOPE( section ) \
	TRACE_ATTACH_TEXT( text_cstr )


class GlDebugGroupScope {
public:
	GlDebugGroupScope(const char* section) {
		if( GLAD_GL_KHR_debug && r_useDebugGroups.GetBool() )
			qglPushDebugGroup( GL_DEBUG_SOURCE_APPLICATION, 1, -1, section );
	}

	~GlDebugGroupScope() {
		if( GLAD_GL_KHR_debug && r_useDebugGroups.GetBool() )
			qglPopDebugGroup();
	}
};

#define TRACE_GL_SCOPE( section ) GlDebugGroupScope __glDebugGroupCurentScope(section); TracyGpuNamedZone( __tracy_gpu_zone, section, g_tracingEnabled && g_glTraceInitialized ); // printf("TRACE_GL_SCOPE %s\n", section);
#define TRACE_GL_SCOPE_COLOR( section, color ) GlDebugGroupScope __glDebugGroupCurentScope(section); TracyGpuNamedZoneC( __tracy_gpu_zone, section, color, g_tracingEnabled && g_glTraceInitialized );
