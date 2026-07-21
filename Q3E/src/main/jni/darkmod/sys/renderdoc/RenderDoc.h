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

void RenderDoc_Init();

// is TDM connected to RenderDoc and ready to shine?
bool RenderDoc_IsAvailable();

// are we inside frame capture right now?
bool RenderDoc_IsCapturing();

// programmatically start/end capture (useful for one-time screenshots, saves, etc.)
void RenderDoc_StartCapture();
void RenderDoc_EndCapture();

// programmatically trigger capture as if the user has pressed PrtScrn hotkey
bool RenderDoc_TriggerCapture(int numFrames = 1);

// scope-based wrapper arround StartCapture / EndCapture
struct RenderDoc_ScopedCapture {
	~RenderDoc_ScopedCapture();
	RenderDoc_ScopedCapture( bool start = true );
	void Start();
	void Finish();
};
