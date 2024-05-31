/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// wad.h

#ifndef WAD_H
#define WAD_H

#include <stdint.h>
struct sizebuf_s;

//===============
//   TYPES
//===============

#define	CMP_NONE		0
#define	CMP_LZSS		1

#define	TYP_NONE		0
#define	TYP_LABEL		1

#define	TYP_LUMPY		64				// 64 + grab command number
#define	TYP_PALETTE		64
#define	TYP_QTEX		65
#define	TYP_QPIC		66
#define	TYP_SOUND		67
#define	TYP_MIPTEX		68

typedef struct qpic_s
{
	int			width, height;
	unsigned char		data[4];			// variably sized
} qpic_t;



typedef struct wadinfo_s
{
	char		identification[4];		// should be WAD2 or 2DAW
	int			numlumps;
	int			infotableofs;
} wadinfo_t;

typedef struct lumpinfo_s
{
	int			filepos;
	int			disksize;
	int			size;					// uncompressed
	char		type;
	char		compression;
	char		pad1, pad2;
	char		name[16];				// must be null terminated
} lumpinfo_t;

void W_UnloadAll(void);
unsigned char *W_GetLumpName(const char *name, int64_t *returnfilesize);

// halflife texture wads
void W_LoadTextureWadFile(char *filename, int complain);
unsigned char *W_GetTextureBGRA(char *name); // returns tempmempool allocated image data, width and height are in image_width and image_height
unsigned char *W_ConvertWAD3TextureBGRA(struct sizebuf_s *sb); // returns tempmempool allocated image data, width and height are in image_width and image_height

#endif

