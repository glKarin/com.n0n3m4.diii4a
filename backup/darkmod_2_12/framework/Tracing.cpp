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
#include "Tracing.h"

// stgatilov: if you update Tracy and this header is missing in new artefacts
// then you need to implement proper way of querying the version of Tracy
// see this issue: https://github.com/wolfpld/tracy/issues/449
#if !defined(_NO_TRACY) // __ANDROID__
#include <common/TracyVersion.hpp>
#endif

idCVar r_useDebugGroups(
	// note: we cannot enable these for players, because they are slow as shit in AMD driver:
	// https://forums.thedarkmod.com/index.php?/topic/21682-r_usedebuggroups-kills-performance
	"r_useDebugGroups", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE,
	"Emit GL debug groups during rendering. "
	"Useful for frame debugging and analysis with e.g. nSight, "
	"which will group render calls accordingly."
);
idCVar com_enableTracing(
	"com_enableTracing", "0", CVAR_SYSTEM | CVAR_INTEGER,
	"Enable the tracy profiler. If set to 2, will stall until the Tracy Profiler app is connected"
);
idCVar com_tracingAllocStacks(
	"com_tracingAllocStacks", "0", CVAR_SYSTEM | CVAR_BOOL,
	"Collect call stacks for all memory allocations (wastes time)"
);

bool g_tracingEnabled = false;
bool g_glTraceInitialized = false;

void GL_SetDebugLabel(GLenum identifier, GLuint name, const idStr &label ) {
	if( GLAD_GL_KHR_debug && r_useDebugGroups.GetBool() ) {
		qglObjectLabel( identifier, name, std::min(label.Length(), 256), label.c_str() );
	}
}

void GL_SetDebugLabel(void *ptr, const idStr &label ) {
	if( GLAD_GL_KHR_debug && r_useDebugGroups.GetBool() ) {
		qglObjectPtrLabel( ptr, std::min(label.Length(), 256), label.c_str() );
	}
}

void InitTracing() {
	if ( !g_tracingEnabled && com_enableTracing.GetBool() ) {
#ifdef TRACY_ENABLE
		tracy::StartupProfiler();
		g_tracingEnabled = true;

		// print used version of Tracy so that people know which TracyViewer to download
		common->Printf(
			"Use Tracy viewer of version %d.%d.%d\n",
			int( tracy::Version::Major ),
			int( tracy::Version::Minor ),
			int( tracy::Version::Patch )
		);

		if ( com_enableTracing.GetInteger() == 2 ) {
			// wait until Tracy Profiler app is connected
			while ( !tracy::GetProfiler().IsConnected() ) {
				Sys_Yield();
			}
		}
#else
		common->Printf("Tracy profiling not included in this build\n");
		common->Printf("Note that Tracy does not work in Debug configurations\n");
		g_tracingEnabled = true;
#endif
	}
	g_tracingAllocStacks = com_tracingAllocStacks.GetBool();
}

void InitOpenGLTracing() {
#ifdef TRACY_ENABLE
	if ( g_glTraceInitialized ) {
		// must recreate the queries for tracy's GpuCtx. If we instead create a new GpuCtx,
		// the tracy Profiler app will crash...
		tracy::GetGpuCtx().ptr->RecreateQueries();
	}
	if ( g_tracingEnabled && !g_glTraceInitialized ) {
		TracyGpuContext;
		g_glTraceInitialized = true;
	}
#endif
}

void TracingEndFrame() {
	InitTracing();

#ifdef TRACY_ENABLE
	if ( g_tracingEnabled ) {
		if ( !g_glTraceInitialized ) {
			TracyGpuContext;
			g_glTraceInitialized = true;
		}

		FrameMark;
		TracyGpuCollect;
	}
#endif
}
