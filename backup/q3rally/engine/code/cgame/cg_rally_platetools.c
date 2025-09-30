/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_local.h"

#define POOLSIZE	(512 * 1024)

static char		memoryPool[POOLSIZE];
static int		allocPoint = 0;

static void *CG_Alloc( int size ) {
	char	*p;

	if ( allocPoint + size > POOLSIZE ) {
		CG_Error( "CG_Alloc: failed on allocation of %i bytes\n", size );
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 31 ) & ~31;

	return p;
}

/*
static void CG_Free( int size ) {
	allocPoint -= ( size + 31 ) & ~31;
}
*/

typedef	struct
{
	byte	*imageData;			// Image Data (Up To 32 Bits)
	int		bpp;				// Image Color Depth In Bits Per Pixel
	int		width;				// Image Width
	int		height;				// Image Height
} TextureImage;

/*
This function is based on code in the Tokens, Extensions, Scissor Testing And TGA Loading
tutorial by Jeff Molofee at NeHe.

NeHe URL: http://nehe.gamedev.net
Tutorial URL: http://nehe.gamedev.net/data/lessons/lesson.asp?lesson=24
*/
qboolean LoadTGA(TextureImage *texture, const char *filename){    
	byte		TGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0};			// Uncompressed TGA Header
	byte		TGAcompare[12];										// Used To Compare TGA Header
	byte		header[6];											// First 6 Useful Bytes From The Header
	int			bytesPerPixel;										// Holds Number Of Bytes Per Pixel Used In The TGA File
	int			imageSize;											// Used To Store The Image Size When Setting Aside Ram
	//int			temp;												// Temporary Variable
	fileHandle_t	imageFile;
	//int			i;

	// Read tga file

	trap_FS_FOpenFile( filename, &imageFile, FS_READ );

	if ( !imageFile ){
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not open %s for license plate.\n", filename);
		return qfalse;
	}

	trap_FS_Read(TGAcompare, sizeof(TGAcompare), imageFile);

	if ( memcmp(TGAheader, TGAcompare, sizeof(TGAheader)) != 0){		// Does The Header Match What We Want?
		trap_FS_FCloseFile( imageFile );
		if (TGAcompare[2] == 10) {
			Com_Printf( S_COLOR_YELLOW "Q3R Warning: Cannot load %s, Run-Length Encoded TGAs are unsupported.\n", filename);
		} else {
			Com_Printf( S_COLOR_YELLOW "Q3R Warning: Header of %s does not match known header format.\n", filename);
		}
		return qfalse;
	}

	trap_FS_Read(header, sizeof(header), imageFile);				// Read in the rest of the 6 bytes
	
	texture->width = header[1] * 256 + header[0];					// Determine The TGA Width	(highbyte*256+lowbyte)
	texture->height = header[3] * 256 + header[2];					// Determine The TGA Height	(highbyte*256+lowbyte)
    
 	if(	texture->width <=0	||						// Is The Width Less Than Or Equal To Zero
		texture->height <=0	||						// Is The Height Less Than Or Equal To Zero
		(header[4]!=24 && header[4]!=32))			// Is The TGA 24 or 32 Bit?
	{
		trap_FS_FCloseFile( imageFile );
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: %s has invalid dimensions or bpps.\n", filename);
		return qfalse;								// Return False
	}

	texture->bpp = header[4];							// Grab The TGA's Bits Per Pixel (24 or 32)
	bytesPerPixel = texture->bpp / 8;						// Divide By 8 To Get The Bytes Per Pixel
	imageSize = texture->width * texture->height * bytesPerPixel;			// Calculate The Memory Required For The TGA Data

	texture->imageData = (unsigned char*)CG_Alloc(imageSize);				// Reserve Memory To Hold The TGA Data

	trap_FS_Read(texture->imageData, imageSize, imageFile);

	if(	texture->imageData == NULL )						// Does The Storage Memory Exist?
	{
		trap_FS_FCloseFile( imageFile );
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Not enough memory to load %s.\n", filename);
		return qfalse;								// Return False
	}
/*
	for(i = 0; i < imageSize; i += bytesPerPixel)				// Loop Through The Image Data
	{										// Swaps The 1st And 3rd Bytes ('R'ed and 'B'lue)
		temp = texture->imageData[i];						// Temporarily Store The Value At Image Data 'i'
		texture->imageData[i] = texture->imageData[i + 2];			// Set The 1st Byte To The Value Of The 3rd Byte
		texture->imageData[i + 2] = temp;					// Set The 3rd Byte To The Value In 'temp' (1st Byte Value)
	}
*/
	trap_FS_FCloseFile( imageFile );

	return qtrue;									// Texture Building Went Ok, Return True
}

static qboolean WriteNameOnTexture(TextureImage *texture, const char *name, int maxChars ) {
	vec4_t			color;
	const char		*s;
	unsigned char	ch;
	float			ax, ay, aw, ah;
	float			frow, fcol, fwidth, fheight;
	int				i, j;
	int				bytesPerPixelF, bytesPerPixelT;
	float			a;
	int				t, f, len, cnt;
	TextureImage	font;

	if (!LoadTGA(&font, "gfx/2d/bigchars_plates.tga"))
		return qfalse;
//	LoadTGA(&font, "menu/art/font1_prop.tga");

	bytesPerPixelF = font.bpp / 8;
	bytesPerPixelT = texture->bpp / 8;

	// use image for maxChars
	len = (int)(texture->width / SMALLCHAR_WIDTH) - 1;
	if (len < maxChars)
		maxChars = len;

	len = strlen(name);
	if (len > maxChars)
		len = maxChars;

//	ax = (int)((texture->width / 2.0f) - (len * 9 / 2.0f) - 3.5);
	ax = (int)((texture->width / 2.0f) - (len * (SMALLCHAR_WIDTH+1) / 2.0f) - 3);
	if (ax < -3)
		ax = -3;
	ay = 11;
	ah = SMALLCHAR_HEIGHT;

	color[0] = 0;
	color[1] = 0;
	color[2] = 0;
	color[3] = 1.0f;

	s = name;
	cnt = 0;
	while ( *s && cnt < maxChars)
	{
		if ( Q_IsColorString( s ) ) {
			memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );

			s += 2;
			continue;
		}

		ch = *s & 127;

		if ( ch == ' ' ) {
			aw = SMALLCHAR_WIDTH + 8;
		} else if ( propMap[ch][2] != -1 ) {
			frow = (ch >> 4) * 16.0f;
			fcol = (ch & 15) * 16.0f;
			fwidth = 16.0f;
			fheight = 16.0f;
//			fcol = (float)propMap[ch][0];
//			frow = (float)propMap[ch][1];
//			fwidth = (float)propMap[ch][2];
//			fheight = (float)PROP_HEIGHT;
			aw = SMALLCHAR_WIDTH + 8;

			for (i = 0; i < ah; i++){
				t = (int)((texture->height - (ay + ah - i)) * texture->width + ax) * bytesPerPixelT;
				f = (int)(font.height - (frow + fheight - fheight * (i / (float)ah))) * font.width;

				for (j = 0; j < aw; j++){
					a = font.imageData[(int)((f + (int)(fcol + fwidth * (j / (float)aw))) * bytesPerPixelF + 3)];

					texture->imageData[t] = (byte)(texture->imageData[t] * (1.0f - (a / 255.0f)) + color[2] * a);
					texture->imageData[t+1] = (byte)(texture->imageData[t+1] * (1.0f - (a / 255.0f)) + color[1] * a);
					texture->imageData[t+2] = (byte)(texture->imageData[t+2] * (1.0f - (a / 255.0f)) + color[0] * a);

					t += bytesPerPixelT;
				}
			}
		} else {
			aw = 0;
		}

		ax += aw-7;
		cnt++;
		s++;
	}

	return qtrue;
}


qboolean SaveTGA(TextureImage *texture, const char *filename){
	byte		TGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0};			// Uncompressed TGA Header
	byte		header[6];											// First 6 Useful Bytes From The Header
	int			bytesPerPixel;										// Holds Number Of Bytes Per Pixel Used In The TGA File
	int			imageSize;											// Used To Store The Image Size When Setting Aside Ram
	//int			temp;												// Temporary Variable
	fileHandle_t	imageFile;
	//int			i;

	// Write tga file

	trap_FS_FOpenFile( filename, &imageFile, FS_WRITE );

	if ( !imageFile ){
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not open %s for texture output.\n", filename);
		return qfalse;
	}


	bytesPerPixel = texture->bpp / 8;						// Divide By 8 To Get The Bytes Per Pixel
	imageSize = texture->width * texture->height * bytesPerPixel;			// Calculate The Memory Required For The TGA Data

	header[0] = texture->width % 256;
	header[1] = texture->width / 256;
	header[2] = texture->height % 256;
	header[3] = texture->height / 256;
	header[4] = texture->bpp;
	header[5] = 0;

	trap_FS_Write(TGAheader, sizeof(TGAheader), imageFile);
	trap_FS_Write(header, sizeof(header), imageFile);
/*
	for(i = 0; i < imageSize; i += bytesPerPixel)				// Loop Through The Image Data
	{																// Swaps The 3st And 1rd Bytes ('R'ed and 'B'lue)
		temp = texture->imageData[i];								// Temporarily Store The Value At Image Data 'i'
		texture->imageData[i] = texture->imageData[i + 2];			// Set The 3st Byte To The Value Of The 1rd Byte
		texture->imageData[i + 2] = temp;							// Set The 1rd Byte To The Value In 'temp' (3st Byte Value)
	}
*/
	trap_FS_Write(texture->imageData, imageSize, imageFile);

	trap_FS_FCloseFile( imageFile );

	return qtrue;
}

void CreateLicensePlateImage(const char *input, const char *output, const char *name, int maxChars){
	TextureImage	tga;

	allocPoint = 0;

	if (!LoadTGA(&tga, input))
		return;
	if (!WriteNameOnTexture(&tga, name, maxChars))
		return;
	SaveTGA(&tga, output);
}
