/*
Copyright (C) 2002-2012 DarkPlaces contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef DPVSIMPLEDECODE_H
#define DPVSIMPLEDECODE_H

#include "qtypes.h"
#include "qdefs.h"
#include "cvar.h"
#include "cl_video.h"
#include "draw.h"

#define DPVSIMPLEDECODEERROR_NONE 0
#define DPVSIMPLEDECODEERROR_EOF 1
#define DPVSIMPLEDECODEERROR_READERROR 2
#define DPVSIMPLEDECODEERROR_SOUNDBUFFERTOOSMALL 3
#define DPVSIMPLEDECODEERROR_INVALIDRMASK 4
#define DPVSIMPLEDECODEERROR_INVALIDGMASK 5
#define DPVSIMPLEDECODEERROR_INVALIDBMASK 6
#define DPVSIMPLEDECODEERROR_COLORMASKSOVERLAP 7
#define DPVSIMPLEDECODEERROR_COLORMASKSEXCEEDBPP 8
#define DPVSIMPLEDECODEERROR_UNSUPPORTEDBPP 9

// opening and closing streams

// opens a stream
void *dpvsimpledecode_open(clvideo_t *video, char *filename, const char **errorstring);

// closes a stream
void dpvsimpledecode_close(void *stream);

// utilitarian functions

// returns the current error number for the stream, and resets the error
// number to DPVDECODEERROR_NONE
// if the supplied string pointer variable is not NULL, it will be set to the
// error message
int dpvsimpledecode_error(void *stream, const char **errorstring);

// returns the width of the image data
unsigned int dpvsimpledecode_getwidth(void *stream);

// returns the height of the image data
unsigned int dpvsimpledecode_getheight(void *stream);

// returns the framerate of the stream
double dpvsimpledecode_getframerate(void *stream);

// returns aspect ratio of the stream
double dpvsimpledecode_getaspectratio(void *stream);

// decodes a video frame to the supplied output pixels
int dpvsimpledecode_video(void *stream, void *imagedata, unsigned int Rmask, unsigned int Gmask, unsigned int Bmask, unsigned int bytesperpixel, int imagebytesperrow);

#endif
