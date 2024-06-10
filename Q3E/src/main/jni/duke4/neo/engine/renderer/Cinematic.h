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

#ifndef __CINEMATIC_H__
#define __CINEMATIC_H__

/*
===============================================================================

	RoQ cinematic

	Multiple idCinematics can run simultaniously.
	A single idCinematic can be reused for multiple files if desired.

===============================================================================
*/

// cinematic states
typedef enum {
	FMV_IDLE,
	FMV_PLAY,			// play
	FMV_EOF,			// all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} cinStatus_t;

// a cinematic stream generates an image buffer, which the caller will upload to a texture
typedef struct {
	int					imageWidth, imageHeight;	// will be a power of 2
	const byte *		image;						// RGBA format, alpha will be 255
	int					status;
} cinData_t;

class idCinematic {
public:
	// initialize cinematic play back data
	static void			InitCinematic( void );

	// shutdown cinematic play back data
	static void			ShutdownCinematic( void );

	// allocates and returns a private subclass that implements the methods
	// This should be used instead of new
	static idCinematic	*Alloc();

	// returns false if it failed to load
	virtual bool		InitFromFile( const char *qpath, bool looping ) = 0;

	virtual bool		IsDone() = 0;

	// the pointers in cinData_t will remain valid until the next UpdateForTime() call
	virtual cinData_t	ImageForTime( int milliseconds ) = 0;

	// closes the file and frees all allocated memory
	virtual void		Close() = 0;

	// closes the file and frees all allocated memory
	virtual void		ResetTime(int time) = 0;

	// returns the render image for this cinematic.
	virtual idImage*	GetRenderImage() = 0;
};

#endif /* !__CINEMATIC_H__ */
