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
#include "RenderDoc.h"

#include "../sys_public.h"
#include "renderdoc_app.h"
#include "../framework/Common.h"


RENDERDOC_API_1_6_0 *RenderDocApi = NULL;

#if defined(_WIN32)

	#include "windows.h"

	pRENDERDOC_GetAPI RenderDoc_LoadGetApiFunction() {
		if ( HMODULE mod = GetModuleHandle( "renderdoc.dll" ) ) {
			return (pRENDERDOC_GetAPI) GetProcAddress( mod, "RENDERDOC_GetAPI" );
		} else {
			return NULL;
		}
	}

#else

	#include <dlfcn.h>

	pRENDERDOC_GetAPI RenderDoc_LoadGetApiFunction() {
		if ( void *mod = dlopen( "librenderdoc.so", RTLD_NOW | RTLD_NOLOAD ) ) {
			return (pRENDERDOC_GetAPI) dlsym( mod, "RENDERDOC_GetAPI" );
		} else {
			return NULL;
		}
	}

#endif

idCVar renderdoc_block( "renderdoc_block", "0", CVAR_RENDERER | CVAR_BOOL, "If set to 1, then programmatic RenderDoc captures are blocked" );

static void RenderDoc_TriggerCapture_f( const idCmdArgs &args ) {
	int numFrames = 1;
	if ( args.Argc() >= 2 )
		numFrames = atoi( args.Argv( 1 ) );

	bool ok = RenderDoc_TriggerCapture(numFrames);

	if ( ok ) {
		common->Printf("RenderDoc capture done.\n");
	} else {
		common->Printf("RenderDoc is disabled.\n");
	}
}

void RenderDoc_Init() {
	cmdSystem->AddCommand( "renderdoc_triggercapture", RenderDoc_TriggerCapture_f, CMD_FL_RENDERER, "trigger RenderDoc capture as if you hit the hotkey (e.g. PrtScrn)" );

	RenderDocApi = NULL;

	pRENDERDOC_GetAPI pFuncGetApi = RenderDoc_LoadGetApiFunction();
	if ( !pFuncGetApi )
		return;
	common->Printf( "Detected RenderDoc DLL loaded\n" );

	int ret = ( *pFuncGetApi )( eRENDERDOC_API_Version_1_1_2, (void **)&RenderDocApi );
	if ( ret != 1 ) {
		common->Printf( "Failed to query RenderDoc API\n" );
		RenderDocApi = NULL;
		return;
	}

	int major, minor, patch;
	RenderDocApi->GetAPIVersion( &major, &minor, &patch );
	common->Printf( "RenderDoc version: %d.%d.%d\n", major, minor, patch );
}

bool RenderDoc_IsAvailable() {
	if ( !RenderDocApi )
		return false;
	return RenderDocApi->IsTargetControlConnected() == 1;
}

bool RenderDoc_IsCapturing() {
	if ( !RenderDocApi )
		return false;
	return RenderDocApi->IsFrameCapturing() == 1;
}

void RenderDoc_StartCapture() {
	if ( !RenderDocApi || renderdoc_block.GetBool() )
		return;
	// as far as I know, TDM works with only one window and one GL context
	RenderDocApi->StartFrameCapture( NULL, NULL );
}

void RenderDoc_EndCapture() {
	if ( !RenderDocApi || renderdoc_block.GetBool() )
		return;
	// as far as I know, TDM works with only one window and one GL context
	RenderDocApi->EndFrameCapture( NULL, NULL );
}

bool RenderDoc_TriggerCapture( int numFrames ) {
	if ( !RenderDocApi || renderdoc_block.GetBool() )
		return false;

	if ( numFrames == 1 ) {
		RenderDocApi->TriggerCapture();
	} else if ( numFrames > 1 ) {
		RenderDocApi->TriggerMultiFrameCapture( numFrames );
	}
	return true;
}

// I guess there is no need to check if capture is already in process / was already terminated
// we can just call start/end and let RenderDoc handle any mismatches
void RenderDoc_ScopedCapture::Start() {
	RenderDoc_StartCapture();
}
void RenderDoc_ScopedCapture::Finish() {
	RenderDoc_EndCapture();
}
RenderDoc_ScopedCapture::~RenderDoc_ScopedCapture() {
	Finish();
}
RenderDoc_ScopedCapture::RenderDoc_ScopedCapture( bool start ) {
	if ( start )
		Start();
}
