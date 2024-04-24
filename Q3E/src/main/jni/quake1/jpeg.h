/*
	Copyright (C) 2002  Mathieu Olivier

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

*/

#ifndef JPEG_H
#define JPEG_H

#include <stddef.h>
#include "qtypes.h"

qbool JPEG_OpenLibrary (void);
void JPEG_CloseLibrary (void);
unsigned char* JPEG_LoadImage_BGRA (const unsigned char *f, int filesize, int *miplevel);
qbool JPEG_SaveImage_preflipped (const char *filename, int width, int height, unsigned char *data);

/*! \returns 0 if failed, or the size actually used.
 */
size_t JPEG_SaveImage_to_Buffer (char *jpegbuf, size_t jpegsize, int width, int height, unsigned char *data);
qbool Image_Compress(const char *imagename, size_t maxsize, void **buf, size_t *size);


#endif
