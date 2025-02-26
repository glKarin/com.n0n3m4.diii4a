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
// r_misc.c

#include "quakedef.h"
#ifdef GLQUAKE
#include "glquake.h"
#include "gl_draw.h"

/*
==================
R_InitTextures
==================
*
void	GLR_InitTextures (void)
{
	int		x,y, m;
	qbyte	*dest;

// create a simple checkerboard texture for the default
	r_notexture_mip = Hunk_AllocName (sizeof(texture_t) + 16*16+8*8+4*4+2*2, "notexture");
	
	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16*16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8*8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4*4;
	
	for (m=0 ; m<4 ; m++)
	{
		dest = (qbyte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y=0 ; y< (16>>m) ; y++)
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}	
}*/










#ifdef RTLIGHTS
texid_t GenerateNormalisationCubeMap(void)
{
	texid_t normalisationCubeMap;
	unsigned char data[32*32*3];

	//some useful variables
	int size=32;
	float offset=0.5f;
	float halfSize=16.0f;
	vec3_t tempVector;
	unsigned char * bytePtr;

	int i, j;
	
	normalisationCubeMap = Image_CreateTexture("normalisationcubemap", NULL, IF_TEXTYPE_CUBE);
	qglGenTextures(1, &normalisationCubeMap->num);
	GL_MTBind(0, GL_TEXTURE_CUBE_MAP_ARB, normalisationCubeMap);

	//positive x
	bytePtr=data;

	for(j=0; j<size; j++)
	{
		for(i=0; i<size; i++)
		{
			tempVector[0] = halfSize;			
			tempVector[1] = -(j+offset-halfSize);
			tempVector[2] = -(i+offset-halfSize);

			VectorNormalize(tempVector);

			bytePtr[0]=(unsigned char)((tempVector[0]/2 + 0.5)*255);
			bytePtr[1]=(unsigned char)((tempVector[1]/2 + 0.5)*255);
			bytePtr[2]=(unsigned char)((tempVector[2]/2 + 0.5)*255);

			bytePtr+=3;
		}
	}
	qglTexImage2D(	GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
					0, GL_RGBA, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	//negative x
	bytePtr=data;

	for(j=0; j<size; j++)
	{
		for(i=0; i<size; i++)
		{
			tempVector[0] = (-halfSize);
			tempVector[1] = (-(j+offset-halfSize));
			tempVector[2] = ((i+offset-halfSize));

			VectorNormalize(tempVector);

			bytePtr[0]=(unsigned char)((tempVector[0]/2 + 0.5)*255);
			bytePtr[1]=(unsigned char)((tempVector[1]/2 + 0.5)*255);
			bytePtr[2]=(unsigned char)((tempVector[2]/2 + 0.5)*255);

			bytePtr+=3;
		}
	}
	qglTexImage2D(	GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
					0, GL_RGBA, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	//positive y
	bytePtr=data;

	for(j=0; j<size; j++)
	{
		for(i=0; i<size; i++)
		{
			tempVector[0] = (i+offset-halfSize);
			tempVector[1] = (halfSize);
			tempVector[2] = ((j+offset-halfSize));

			VectorNormalize(tempVector);

			bytePtr[0]=(unsigned char)((tempVector[0]/2 + 0.5)*255);
			bytePtr[1]=(unsigned char)((tempVector[1]/2 + 0.5)*255);
			bytePtr[2]=(unsigned char)((tempVector[2]/2 + 0.5)*255);

			bytePtr+=3;
		}
	}
	qglTexImage2D(	GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
					0, GL_RGBA, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	//negative y
	bytePtr=data;

	for(j=0; j<size; j++)
	{
		for(i=0; i<size; i++)
		{
			tempVector[0] = (i+offset-halfSize);
			tempVector[1] = (-halfSize);
			tempVector[2] = (-(j+offset-halfSize));

			VectorNormalize(tempVector);

			bytePtr[0]=(unsigned char)((tempVector[0]/2 + 0.5)*255);
			bytePtr[1]=(unsigned char)((tempVector[1]/2 + 0.5)*255);
			bytePtr[2]=(unsigned char)((tempVector[2]/2 + 0.5)*255);

			bytePtr+=3;
		}
	}
	qglTexImage2D(	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
					0, GL_RGBA, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	//positive z
	bytePtr=data;

	for(j=0; j<size; j++)
	{
		for(i=0; i<size; i++)
		{
			tempVector[0] = (i+offset-halfSize);
			tempVector[1] = (-(j+offset-halfSize));
			tempVector[2] = (halfSize);

			VectorNormalize(tempVector);

			bytePtr[0]=(unsigned char)((tempVector[0]/2 + 0.5)*255);
			bytePtr[1]=(unsigned char)((tempVector[1]/2 + 0.5)*255);
			bytePtr[2]=(unsigned char)((tempVector[2]/2 + 0.5)*255);

			bytePtr+=3;
		}
	}
	qglTexImage2D(	GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
					0, GL_RGBA, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	//negative z
	bytePtr=data;

	for(j=0; j<size; j++)
	{
		for(i=0; i<size; i++)
		{
			tempVector[0] = (-(i+offset-halfSize));
			tempVector[1] = (-(j+offset-halfSize));
			tempVector[2] = (-halfSize);

			VectorNormalize(tempVector);

			bytePtr[0]=(unsigned char)((tempVector[0]/2 + 0.5)*255);
			bytePtr[1]=(unsigned char)((tempVector[1]/2 + 0.5)*255);
			bytePtr[2]=(unsigned char)((tempVector[2]/2 + 0.5)*255);

			bytePtr+=3;
		}
	}
	qglTexImage2D(	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB,
					0, GL_RGBA, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data);	
		

	qglTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return normalisationCubeMap;
}

texid_t normalisationCubeMap;
#endif

#if 0
typedef struct
{
   long offset;                 	// Position of the entry in WAD
   long dsize;                  	// Size of the entry in WAD file
   long size;                   	// Size of the entry in memory
   char type;                   	// type of entry
   char cmprs;                  	// Compression. 0 if none.
   short dummy;                 	// Not used
   char name[16];               	// we use only first 8
} wad2entry_t;
typedef struct
{
   char magic[4]; 			//should be WAD2
   long num;				//number of entries
   long offset;				//location of directory
} wad2_t;
void R_MakeTexWad_f(void)
{
	//this function is written as little endian. nothing will fix that.
	miptex_t dummymip = {"", 0, 0, {0, 0, 0, 0}};
	wad2_t wad2 = {"WAD2",0,0};
	wad2entry_t entry[2048];
	int entries = 0, i;
	vfsfile_t *f;
	char base[128];
//	qbyte b;
	qboolean hasalpha;
	int width, height;

	qbyte *buf, *outmip;
	qbyte *mip, *stack;

	char *wadname = Cmd_Argv(1);
	char *imagename = Cmd_Argv(2);
	float scale = atof(Cmd_Argv(3));

	if (!scale)
		scale = 2;

	if (!*wadname || !*imagename)
		return;
	f=FS_OpenVFS(wadname, "w+b", FS_GAMEONLY);
	if (!f)
		return;

	mip = BZ_Malloc(1024*1024);
//	initbuf = BZ_Malloc(1024*1024*4);
	stack = BZ_Malloc(1024*1024*4+1024);

	VFS_SEEK(f, 0);
	VFS_READ(f, &wad2, sizeof(wad2_t));

	VFS_SEEK(f, wad2.offset);
	VFS_READ(f, entry, sizeof(entry[0]) * wad2.num);

	//find the end of the data.
	wad2.offset = sizeof(wad2_t);
	for (entries = 0; entries < wad2.num; entries++)
		if (wad2.offset < entry[entries].offset + entry[entries].dsize)
			wad2.offset = entry[entries].offset + entry[entries].dsize;
	VFS_SEEK(f, wad2.offset);

	{
		COM_StripExtension(imagename, base, sizeof(base));
		base[15]=0;
		for (i =0; i < entries; i++)
			if (!stricmp(entry[i].name, base))
				break;
		if (i != entries)
			Con_Printf("Replacing %s, you'll want to compact your wad at some point.\n", base);	//this will leave a gap. we don't support compacting.
		else
			entries++;
		entry[i].offset = VFS_TELL(f);
		entry[i].dsize = entry[i].size = 0;
		entry[i].type = TYP_MIPTEX;
		entry[i].cmprs = 0;
		entry[i].dummy = 0;
		strcpy(entry[i].name, base);

		strcpy(dummymip.name, base);

		{
	
			qbyte *data;
			int h;
			float x, xi;
			float y, yi;			

			char *path[] ={
		"%s",
		"override/%s.tga",

		"textures/%s.png",
		"textures/%s.tga",

		"%s.png",
		"%s.tga",
		"progs/%s"};
			for (h = 0, buf=NULL; h < sizeof(path)/sizeof(char *); h++)
			{			
				buf = COM_LoadStackFile(va(path[h], imagename), stack, 1024*1024*4+1024);
				if (buf)
					break;
			}
			width = 16;
			height = 16;
			if (buf)
				data = Read32BitImageFile(buf, com_filesize, &width, &height, &hasalpha, imagename);
			else
				data = NULL;
			if (!data)
				data = Z_Malloc(width*height*4);

			dummymip.width = (int)(width/scale) & ~0xf;
			dummymip.height = (int)(height/scale) & ~0xf;
			if (dummymip.width<=0)
				dummymip.width=16;
			if (dummymip.height<=0)
				dummymip.height=16;
			if (dummymip.width > 1024)
				dummymip.width = 1024;
			if (dummymip.height > 1024)
				dummymip.height = 1024;

			dummymip.offsets[0] = sizeof(dummymip);
			dummymip.offsets[1] = dummymip.offsets[0]+dummymip.width*dummymip.height;
			dummymip.offsets[2] = dummymip.offsets[1]+dummymip.width/2*dummymip.height/2;
			dummymip.offsets[3] = dummymip.offsets[2]+dummymip.width/4*dummymip.height/4;
			entry[entries].dsize = entry[entries].size = dummymip.offsets[3]+dummymip.width/8*dummymip.height/8;

			xi = (float)width/dummymip.width;
			yi = (float)height/dummymip.height;


			VFS_WRITE(f, &dummymip, sizeof(dummymip));
			outmip=mip;
			for (outmip=mip, y = 0; y < height; y+=yi)
			for (x = 0; x < width; x+=xi)
			{
				*outmip++ = GetPaletteIndex(	data[(int)(x+y*width)*4+0],
								data[(int)(x+y*width)*4+1],
								data[(int)(x+y*width)*4+2]);
			}
			VFS_WRITE(f, mip, dummymip.width * dummymip.height);
			for (outmip=mip, y = 0; y < height; y+=yi*2)
			for (x = 0; x < width; x+=xi*2)
			{
				*outmip++ = GetPaletteIndex(	data[(int)(x+y*width)*4+0],
								data[(int)(x+y*width)*4+1],
								data[(int)(x+y*width)*4+2]);				
			}
			VFS_WRITE(f, mip, (dummymip.width/2) * (dummymip.height/2));
			for (outmip=mip, y = 0; y < height; y+=yi*4)
			for (x = 0; x < width; x+=xi*4)
			{
				*outmip++ = GetPaletteIndex(	data[(int)(x+y*width)*4+0],
								data[(int)(x+y*width)*4+1],
								data[(int)(x+y*width)*4+2]);				
			}
			VFS_WRITE(f, mip, (dummymip.width/4) * (dummymip.height/4));
			for (outmip=mip, y = 0; y < height; y+=yi*8)
			for (x = 0; x < width; x+=xi*8)
			{
				*outmip++ = GetPaletteIndex(	data[(int)(x+y*width)*4+0],
								data[(int)(x+y*width)*4+1],
								data[(int)(x+y*width)*4+2]);
			}
			VFS_WRITE(f, mip, (dummymip.width/8) * (dummymip.height/8));

			BZ_Free(data);
		}
		entry[i].dsize = VFS_TELL(f) - entry[i].offset;
		Con_Printf("Added %s\n", base);
	}

	wad2.offset = VFS_TELL(f);
	wad2.num = entries;
	VFS_WRITE(f, entry, entries*sizeof(wad2entry_t));
	VFS_SEEK(f, 0);
	VFS_WRITE(f, &wad2, sizeof(wad2_t));
	VFS_CLOSE(f);


	BZ_Free(mip);
//	BZ_Free(initbuf);
	BZ_Free(stack);

	Con_Printf("%s now has %i entries\n", wadname, entries);
}
#endif
void GLR_TimeRefresh_f (void);
void GLV_Gamma_Callback(struct cvar_s *var, char *oldvalue);

void GLR_DeInit (void)
{
	Cmd_RemoveCommand ("timerefresh");

//	Cmd_RemoveCommand ("makewad");

//	Cvar_Unhook(&v_gamma);
//	Cvar_Unhook(&v_contrast);
//	Cvar_Unhook(&v_brightness);

	Surf_DeInit();

	GLDraw_DeInit();
}

void GLR_Init (void)
{	
	Cmd_AddCommand ("timerefresh", GLR_TimeRefresh_f);

//	Cmd_AddCommand ("makewad", R_MakeTexWad_f);
}

/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void GLR_TimeRefresh_f (void)
{
	int			i;
	float		start, stop, time;
	int			finish;
	int			frames = 128;

	finish = atoi(Cmd_Argv(1));
	frames = atoi(Cmd_Argv(2));
	if (frames < 1)
		frames = 128;

	if (!r_refdef.playerview)
		r_refdef.playerview = &cl.playerview[0];

	if (finish == 2)
	{
		qglFinish ();
		start = Sys_DoubleTime ();
		for (i=0 ; i<frames ; i++)
		{
			r_refdef.viewangles[1] = i/(float)frames*360.0;
			R_RenderView ();
			VID_SwapBuffers();
		}
	}
	else
	{
		if (qglDrawBuffer)
			qglDrawBuffer  (GL_FRONT);
		qglFinish ();

		start = Sys_DoubleTime ();
		for (i=0 ; i<frames ; i++)
		{
			r_refdef.viewangles[1] = i/(float)frames*360.0;
			R_RenderView ();
			if (finish)
				qglFinish ();
		}
	}
	qglFinish ();
	stop = Sys_DoubleTime ();
	time = stop-start;
	Con_Printf ("%f seconds (%f fps)\n", time, frames/time);

	if (R2D_Flush)
		R2D_Flush();
	if (qglDrawBuffer)
		qglDrawBuffer  (GL_BACK);
	VID_SwapBuffers();
}

#endif
