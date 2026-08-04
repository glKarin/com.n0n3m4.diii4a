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

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

enum GameDisplayMode
{
	kInactive,
	kFullScreen,
	kWindow
};

typedef long LONG;

typedef struct tagPOINT
{
	LONG  x;
	LONG  y;
} POINT;

typedef struct 
{
	GameDisplayMode		mode;				// Indicates if the game is in full screen mode or window mode.
	CGDirectDisplayID	displayID;			// Display used for the full screen mode.
	short				width;				// Width of screen and/or window.
	short				height;				// Height of screen and/or window.
	short				depth;				// Screen bit depth used for full screen mode.
	Fixed				frequency;			// Screen refresh rate in MHz for full screen mode. If zero, then a default will be used.
	POINT				windowLoc;			// Device-local coordinate of top left corner for window mode. Expressed as a Win32 POINT. Coordiantes may be CW_USEDEFAULT indicating no location has yet been established.
	unsigned long		flags;				// kBlankingWindow, kDontRepositionWindow, etc.
	UInt32				resFlags;			// boolean bits to mark special modes for each resolution, e.g. stretched
} GameDisplayInfo;

typedef bool(*ValidModeCallbackProc)(CGDirectDisplayID displayID, int width, int height, int depth, Fixed freq);

OSStatus CreateGameDisplayPreferencesDialog(
							const GameDisplayInfo *inGDInfo,
							WindowRef *outWindow,
							ValidModeCallbackProc inCallback = NULL);

OSStatus RunGameDisplayPreferencesDialog(
							GameDisplayInfo *outGDInfo, 
							WindowRef inWindow);


#endif // PREFERENCESDIALOG_H
