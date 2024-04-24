/*
	Copyright (C) 2006  Serge "(515)" Ziryukin

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

#ifndef PNG_H
#define PNG_H

#include "qtypes.h"

qbool PNG_OpenLibrary (void);
void PNG_CloseLibrary (void);
unsigned char* PNG_LoadImage_BGRA (const unsigned char *f, int filesize, int *miplevel);
qbool PNG_SaveImage_preflipped (const char *filename, int width, int height, qbool has_alpha, unsigned char *data);

#endif

