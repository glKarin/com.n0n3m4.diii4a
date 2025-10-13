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
// wad.c

#include "quakedef.h"
#include "shader.h"

void *wadmutex;

#ifndef PACKAGE_TEXWAD
void Wads_Flush (void){}
qboolean Wad_NextDownload (void){return true;}
void *W_GetLumpName (const char *name, size_t *size, qbyte *type) {return NULL;}
qbyte *W_GetTexture(const char *name, int *width, int *height, uploadfmt_t *format){return NULL;}
void W_LoadWadFile (char *filename){}
void W_Shutdown (void){}
void CL_Skygroup_f(void){}
#else

int			wad_numlumps;
lumpinfo_t	*wad_lumps;
qbyte		*wad_base;

static char wads[4096];

void SwapPic (qpic_t *pic);

/*
==================
W_CleanupName

Lowercases name and pads with spaces and a terminating 0 to the length of
lumpinfo_t->name.
Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
Space padding is so names can be printed nicely in tables.
Can safely be performed in place.
==================
*/
void W_CleanupName (const char *in, char *out)
{
	int		i;
	int		c;

	if (!strncmp(in, "textures/", 9))
		in += 9;
	
	for (i=0 ; i<16 ; i++ )
	{
		c = in[i];
		if (!c)
			break;
			
		if (c >= 'A' && c <= 'Z')
			c += ('a' - 'A');
		if (c == '*')	//not a valid filesystem char
			c = '#';
		out[i] = c;
	}
	
	for ( ; i< 16 ; i++ )
		out[i] = 0;
}


void W_Shutdown (void)
{
	if (wad_base)
		Z_Free(wad_base);
	wad_base = NULL;
}

/*
====================
W_LoadWadFile
====================
*/
void W_LoadWadFile (char *filename)
{
	lumpinfo_t		*lump_p;
	wadinfo_t		*header;
	unsigned		i;
	int				infotableofs;

	if (wad_base)
		Z_Free(wad_base);
	
	wad_base = COM_LoadFile (filename, 0, 0, NULL);
	if (!wad_base)
	{
		wad_numlumps = 0;
		Con_DPrintf ("W_LoadWadFile: couldn't load %s\n", filename);
		return;
	}

	header = (wadinfo_t *)wad_base;
	
	if (header->identification[0] != 'W'
	|| header->identification[1] != 'A'
	|| header->identification[2] != 'D'
	|| (header->identification[3] != '2' && header->identification[3] != '3'))
	{
		Con_Printf ("W_LoadWadFile: Wad file %s doesn't have WAD2 id\n",filename);
		wad_numlumps = 0;
		Z_Free(wad_base);
		wad_base = NULL;
		return;
	}
		
	wad_numlumps = LittleLong(header->numlumps);
	infotableofs = LittleLong(header->infotableofs);
	wad_lumps = (lumpinfo_t *)(wad_base + infotableofs);
	
	for (i=0, lump_p = wad_lumps ; i<wad_numlumps ; i++,lump_p++)
	{
		lump_p->filepos = LittleLong(lump_p->filepos);
		lump_p->size = LittleLong(lump_p->size);
		W_CleanupName (lump_p->name, lump_p->name);
		if (lump_p->type == TYP_QPIC)
			SwapPic ( (qpic_t *)(wad_base + lump_p->filepos));
	}
}

/*
=============
W_GetLumpName
=============
*/
void *W_GetLumpName (const char *name, size_t *size, qbyte *type)
{
	int		i;
	lumpinfo_t	*lump_p;
	char	clean[16];

	*type = 0;
	*size = 0;

	W_CleanupName (name, clean);

	for (lump_p=wad_lumps, i=0 ; i<wad_numlumps ; i++,lump_p++)
	{
		if (!strcmp(clean, lump_p->name))
		{
			*type = lump_p->type;
			*size = lump_p->disksize;
			return (void *)(wad_base+lump_p->filepos);
		}
	}
	return NULL;
}

/*
=============================================================================

automatic qbyte swapping

=============================================================================
*/

void SwapPic (qpic_t *pic)
{
	pic->width = LittleLong(pic->width);
	pic->height = LittleLong(pic->height);	
}















































// based on original code by LordHavoc

//FIXME: convert to linked list. is hunk possible?
//hash tables?
#define TEXWAD_MAXIMAGES 16384

typedef struct wadfile_s
{
	vfsfile_t *file;
	struct wadfile_s *next;
	char name[1];
} wadfile_t;

typedef struct
{
	char name[16];
	vfsfile_t *file;
	int position;
	int size;
} texwadlump_t;
static int numwadtextures;
static texwadlump_t texwadlump[TEXWAD_MAXIMAGES];

static wadfile_t *openwadfiles;

void Wads_Flush (void)
{
	wadfile_t *wf;
	if (wadmutex)
		Sys_LockMutex(wadmutex);
	while(openwadfiles)
	{
		VFS_CLOSE(openwadfiles->file);

		wf = openwadfiles->next;
		Z_Free(openwadfiles);
		openwadfiles = wf;
	}

	numwadtextures=0;
	if (wadmutex)
		Sys_UnlockMutex(wadmutex);
}
/*
====================
W_LoadTextureWadFile
====================
*/
void W_LoadTextureWadFile (char *filename, int complain)
{
	lumpinfo_t		*lumps, *lump_p;
	wadinfo_t		header;
	int				i, j;
	int				infotableofs;
	vfsfile_t		*file;
	int				numlumps;

	wadfile_t *wf = openwadfiles;
	while(wf)
	{
		if (!strcmp(wf->name, filename))	//already loaded
			return;

		wf = wf->next;
	}

	file = FS_OpenVFS(filename, "rb", FS_GAME);
	if (!file)
		file = FS_OpenVFS(va("textures/halflife/%s", filename), "rb", FS_GAME);
	if (!file)
	{
		if (complain)
			Con_Printf ("W_LoadTextureWadFile: couldn't find %s", filename);
		return;
	}

	if (VFS_READ(file, &header, sizeof(wadinfo_t)) != sizeof(wadinfo_t))
	{Con_Printf ("W_LoadTextureWadFile: unable to read wad header");return;}

	if (memcmp(header.identification, "WAD3", 4) && memcmp(header.identification, "WAD2", 4))
	{Con_Printf ("W_LoadTextureWadFile: Wad file %s doesn't have WAD3 id\n",filename);return;}

	numlumps = LittleLong(header.numlumps);
	if (numlumps < 1 || numlumps > TEXWAD_MAXIMAGES)
	{Con_Printf ("W_LoadTextureWadFile: invalid number of lumps (%i)\n", numlumps);return;}
	infotableofs = LittleLong(header.infotableofs);
	if (!VFS_SEEK(file, infotableofs))
	{Con_Printf ("W_LoadTextureWadFile: unable to seek to lump table");return;}
	if (!((lumps = Hunk_TempAlloc(sizeof(lumpinfo_t)*numlumps))))
	{Con_Printf ("W_LoadTextureWadFile: unable to allocate temporary memory for lump table");return;}

	if (VFS_READ(file, lumps, sizeof(lumpinfo_t)*numlumps) != (int)sizeof(lumpinfo_t) * numlumps)
	{Con_Printf ("W_LoadTextureWadFile: unable to read lump table");return;}

	for (i=0, lump_p = lumps ; i<numlumps ; i++,lump_p++)
	{
		W_CleanupName (lump_p->name, lump_p->name);
		for (j = 0;j < numwadtextures;j++)
		{
			if (!strcmp(lump_p->name, texwadlump[j].name)) // name match, replace old one
				break;
		}
		if (j >= TEXWAD_MAXIMAGES)
			break; // abort loading
		if (j == numwadtextures)
		{
			W_CleanupName (lump_p->name, texwadlump[j].name);
			texwadlump[j].file = file;
			texwadlump[j].position = LittleLong(lump_p->filepos);
			texwadlump[j].size = LittleLong(lump_p->disksize);
			numwadtextures++;
		}
	}	
	// leaves the file open

	wf = BZ_Malloc(sizeof(*wf) + strlen(filename));
	strcpy(wf->name, filename);
	wf->file = file;
	wf->next = openwadfiles;
	openwadfiles = wf;
}

/*
void W_ApplyGamma (qbyte *data, int len, int skipalpha)
{
	int		i, inf;
	qbyte gammatable[256];
	
	if (v_gamma.value == 1.0)
	{
		for (i=0 ; i<256 ; i++)
			gammatable[i] = i;		
	}
	else
	{
		for (i=0 ; i<256 ; i++)
		{
			inf = 255 * pow ( (i+0.5)/255.5 , v_gamma.value ) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}
}
*/
qbyte *W_ConvertWAD3Texture(miptex_t *tex, size_t lumpsize, int *width, int *height, uploadfmt_t *format)	//returns rgba
{	
	qbyte *in, *data, *out, *pal;
	int d, p;

	int alpha = 0;

	if (tex->name[0] == '{')
		alpha = 1;

	if (tex->width > 0x10000 || tex->height > 0x10000)
		return NULL;

	//use malloc here if you want, but you'll have to free it again... NUR!
	data = out = BZ_Malloc(tex->width * tex->height * 4);

	if (!data)
		return NULL;

	in = (qbyte *)tex + tex->offsets[0];

	*width = tex->width;
	*height = tex->height;

	//halflife wads have palettes embedded in them. but make sure everything else is packed because some quake wads are weird.
	if (tex->offsets[0] == sizeof(*tex) &&
		tex->offsets[1] == tex->offsets[0] + (tex->width)*(tex->height) &&
		tex->offsets[2] == tex->offsets[1] + (tex->width>>1)*(tex->height>>1) && 
		tex->offsets[3] == tex->offsets[2] + (tex->width>>2)*(tex->height>>2) && 
		((lumpsize+3)&~3) >= ((tex->offsets[3] + (tex->width>>3)*(tex->height>>3) + 2 + 768+3)&~3))
	{
		short fl = *(short*)((qbyte *)tex + tex->offsets[3] + (tex->width>>3)*(tex->height>>3));
		if (fl == 256)
			pal = (qbyte *)tex + tex->offsets[3] + (tex->width>>3)*(tex->height>>3) + 2;
		else
			pal = host_basepal;
	}
	else
		pal = host_basepal;

	/* handle decals type textures -eukara */
	if (alpha == 1 && (pal[765] == 255 && pal[766] == 255 && pal[767] == 255))
		alpha = 3;
	if (alpha == 1 && !(pal[765] == 0 && pal[766] == 0 && pal[767] == 255))
		alpha = 2;

	if (tex->offsets[0] + tex->width * tex->height > lumpsize)
	{	//fucked texture.
		for (d = 0;d < tex->width * tex->height;d++)	
		{
			out[0] = 0;
			out[1] = 255;
			out[2] = 0;
			out[3] = 255;
			out += 4;
		}
	}
	else for (d = 0;d < tex->width * tex->height;d++)
	{
		p = *in++;
		if (alpha == 1 && p == 255) {
			out[0] = out[1] = out[2] = out[3] = 0;
		} else if (alpha == 2) {
			p *= 3;
			/* this will be a blended decal -eukara */
			out[0] = pal[765];
			out[1] = pal[766];
			out[2] = pal[767];
			out[3] = 255 - pal[p];
		} else if (alpha == 3) {
			p *= 3;
			/* this is used for glass and so on -eukara */
			out[0] = 255;
			out[1] = 255;
			out[2] = 255;
			out[3] = pal[p];
		} else {
			p *= 3;
			/* non transparent -eukara */
			out[0] = pal[p];
			out[1] = pal[p+1];
			out[2] = pal[p+2];
			out[3] = 255;
		}
		out += 4;
	}
	*format = alpha?PTI_RGBA8:PTI_RGBX8;
	if (!vid_hardwaregamma.value)
		BoostGamma(data, tex->width, tex->height, *format);
	return data;
}

qbyte *W_GetTexture(const char *name, int *width, int *height, uploadfmt_t *format)//returns rgba
{
	char texname[17];
	int i, j;
	vfsfile_t *file;
	miptex_t *tex;
	qbyte *data;

	if (!strncmp(name, "gfx/", 4) || !strncmp(name, "wad/", 4))
	{
		qpic_t *p;
		size_t lumpsize;
		qbyte lumptype;
		p = W_GetLumpName(name+4, &lumpsize, &lumptype);
		if (p)
		{
			if (/*lumptype == TYP_MIPTEX && */!strcmp(name+4, "conchars") && (lumpsize==128*128
#ifdef HAVE_LEGACY
							|| (lumptype == TYP_QPIC&&lumpsize==8+128*128)
#endif
							))
			{	//conchars has no header.
				qbyte *lump = (qbyte*)p;
				extern cvar_t con_ocranaleds;

				if (lumpsize==8+128*128)
					Con_Printf(CON_WARNING"WARNING: gfx.wad conchars lump has incorrect lump size.\n");

				if (con_ocranaleds.ival)
				{
					if (con_ocranaleds.ival != 2 || CalcHashInt(&hash_crc16, lump, 128*128) == 798)
						AddOcranaLEDsIndexed (lump, 128, 128);
				}

				*width = 128;
				*height = 128;
				*format = PTI_RGBA8;

				data = BZ_Malloc(128 * 128 * 4);
				for (i = 0; i < 128 * 128; i++)
				{
					qbyte b = lump[i];
					if (b == 0)
						((unsigned int*)data)[i] = 0;
					else
						((unsigned int*)data)[i] = d_8to24rgbtable[lump[i]] | 0xff000000;
				}

				return data;
			}
			else if (lumptype == TYP_QPIC && lumpsize == 8+p->width*p->height)
			{
				qboolean alpha = false;
				qbyte pal;
				*width = p->width;
				*height = p->height;

				data = BZ_Malloc(p->width * p->height * 4);
				for (i = 0; i < p->width * p->height; i++)
				{
					pal = p->data[i];
					((unsigned int*)data)[i] = d_8to24rgbtable[pal];
					if (pal == 0xff)
						alpha = true;
				}
				*format = alpha?PTI_RGBA8:PTI_RGBX8;
				return data;
			}
			else if (lumptype == TYP_QPIC && lumpsize == 8+p->width*p->height+4+768)
			{	//halflife, 24bit palette at end
				qbyte *pal = p->data+p->width*p->height+4;
				*width = p->width;
				*height = p->height;
				*format = PTI_RGBA8;

				data = BZ_Malloc(p->width * p->height * 4);
				for (i = 0; i < p->width * p->height; i++)
				{
					qbyte *rgb = pal + p->data[i]*3;
					data[i*4+0] = rgb[0];
					data[i*4+1] = rgb[1];
					data[i*4+2] = rgb[2];
					data[i*4+3] = ((p->data[i]==255)?0:255);
				}
				return data;
			}
			else if (lumptype == TYP_HLFONT)
				; //FIXME... gah
			else
				Con_Printf("W_GetTexture: unknown lump type\n");
		}
	}

	texname[16] = 0;
	W_CleanupName (name, texname);
	Sys_LockMutex(wadmutex);
	for (i = 0;i < numwadtextures;i++)
	{
		if (!strcmp(texname, texwadlump[i].name)) // found it
		{
			file = texwadlump[i].file;

			if (VFS_SEEK(file, texwadlump[i].position))
			{
				tex = BZ_Malloc(texwadlump[i].size);	//temp buffer for disk info (was hunk_tempalloc, but that wiped loading maps and the like
				if (tex && VFS_READ(file, tex, texwadlump[i].size) == texwadlump[i].size)
				{
					Sys_UnlockMutex(wadmutex);
					tex->width = LittleLong(tex->width);
					tex->height = LittleLong(tex->height);
					for (j = 0;j < MIPLEVELS;j++)
						tex->offsets[j] = LittleLong(tex->offsets[j]);

					data = W_ConvertWAD3Texture(tex, texwadlump[i].size, width, height, format);
					BZ_Free(tex);
					return data;
				}
			}
			Con_Printf("W_GetTexture: corrupt WAD3 file\n");
			break;
		}
	}	
	Sys_UnlockMutex(wadmutex);
	return NULL;
}

miptex_t *W_GetMipTex(const char *name)
{
	char texname[17];
	int i, j;
	vfsfile_t *file;
	miptex_t *tex;

	texname[16] = 0;
	W_CleanupName (name, texname);
	Sys_LockMutex(wadmutex);
	for (i = 0;i < numwadtextures;i++)
	{
		if (!strcmp(texname, texwadlump[i].name)) // found it
		{
			file = texwadlump[i].file;
			if (VFS_SEEK(file, texwadlump[i].position))
			{
				tex = BZ_Malloc(texwadlump[i].size);	//temp buffer for disk info (was hunk_tempalloc, but that wiped loading maps and the like
				if (tex && VFS_READ(file, tex, texwadlump[i].size) == texwadlump[i].size)
				{
					Sys_UnlockMutex(wadmutex);
					tex->width = LittleLong(tex->width);
					tex->height = LittleLong(tex->height);
					for (j = 0;j < MIPLEVELS;j++)
						tex->offsets[j] = LittleLong(tex->offsets[j]);
					return tex;
				}
			}
			Con_Printf("W_GetTexture: corrupt WAD3 file\n");
			break;
		}
	}	
	Sys_UnlockMutex(wadmutex);
	return NULL;
}

void WAD_ImageList_f(void)
{
	wadfile_t *wad;
	int i;
	char *match = Cmd_Argv(1);
	Sys_LockMutex(wadmutex);
	for (i = 0;i < numwadtextures;i++)
	{
		if (*match && !wildcmp(match, texwadlump[i].name))
			continue;
		for (wad = openwadfiles; wad; wad = wad->next)
			if (wad->file == texwadlump[i].file)
				break;
		Con_Printf("^[\\img\\%s\\s\\%i\\tip\\From inside %s^] %s\n", texwadlump[i].name, 64, wad?wad->name:"<unknown>", texwadlump[i].name);
	}
	Sys_UnlockMutex(wadmutex);
}

typedef struct mapgroup_s {
	char *mapname;
	char *skyname;
	struct mapgroup_s *next;
} mapskys_t;
static mapskys_t *mapskies;
void CL_Skygroup_f(void)
{
	mapskys_t **link;
	mapskys_t *ms;
	char *skyname;
	char *mapname;
	int i;
	int remove;

	skyname = Cmd_Argv(1);

	if (!*skyname)
	{
		skyname = NULL;
		for (ms = mapskies; ms; ms = ms->next)
		{
			if (!skyname || strcmp(skyname, ms->skyname))
			{
				Con_Printf("%s%s:", skyname?"\n":"", ms->skyname);
				skyname=ms->skyname;
			}
			Con_Printf(" %s", ms->mapname);
		}
		if (skyname)
			Con_Printf("\n");
		else
			Con_Printf("No skygroups defined\n");
		return;
	}

	if (!strcmp(skyname, "clear") && Cmd_Argc() == 2)
	{
		while (mapskies)
		{
			ms = mapskies->next;
			Z_Free(mapskies);
			mapskies = ms;
		}
		return;
	}

	if (*skyname == '-')
	{
		skyname++;
		for (link = &mapskies; *link; )
		{
			if (!strcmp((*link)->mapname, skyname) || !strcmp((*link)->skyname, skyname))
			{
				ms = *link;
				*link = ms->next;
				Z_Free(ms);
			}
			else
				link = &(*link)->next;
		}
		return;
	}

	for (i = 2; i < Cmd_Argc(); i++)
	{
		mapname = Cmd_Argv(i);

		remove = *mapname == '-';
		mapname += remove;

		for (link = &mapskies; *link; link = &(*link)->next)
		{
			if (!strcmp((*link)->mapname, mapname))
			{
				ms = *link;
				*link = ms->next;
				Z_Free(ms);
				break;
			}
		}
		if (remove)
			continue;

		ms = Z_Malloc(sizeof(*ms) + strlen(mapname) + strlen(skyname) + 2);

		ms->mapname = (char*)(ms+1);
		ms->skyname = ms->mapname + strlen(mapname)+1;
		ms->next = mapskies;

		strcpy(ms->mapname, mapname);
		strcpy(ms->skyname, skyname);

		mapskies = ms;
	}
}

//textures/fred.wad is the DP standard - I wanna go for that one.
//textures/halfline/fred.wad is what fuhquake can use (yuck). 
//fred.wad is what half-life supports.

//we only try one download, for textures/fred.wad
//but we will load wads from the other two paths if we have them locally.
qboolean Wad_NextDownload (void)
{
	char wadname[4096+9]="textures/";
	int i, j, k;
	model_t *wmodel = cl.worldmodel;
	if (wmodel && (wmodel->fromgame == fg_halflife || wmodel->type == mod_heightmap) && *wads)	//now go about checking the wads
	{
		j = 0;
		wads[4095] = '\0';
		for (i = 0;i < 4095;i++)
			if (wads[i] != ';' && wads[i] != '\\' && wads[i] != '/' && wads[i] != ':')
				break;
		if (wads[i])
		{
			j=i;
			for (;i < 4095;i++)
			{
				// ignore path...
				if (wads[i] == '\\' || wads[i] == '/' || wads[i] == ':')
					j = i+1;
				else if (wads[i] == ';' || wads[i] == 0)
				{
					k = wads[i];
					wads[i] = 0;
					strcpy(wadname+9, &wads[j]);
					if (wadname[9])
					{
						if (!COM_FCheckExists(wadname+9))	//wad is in root dir, so we don't need to try textures.
							CL_CheckOrEnqueDownloadFile(wadname, wadname, DLLF_REQUIRED);	//don't skip this one, or the world is white.
					}
					wads[i] = k;
					
					j = i+1;
					if (!k)
						break;
				}
			}
		}
	}
	Wads_Flush();
	if (*wads)	//now go about loading the wads, we are now safe from tempallocs
	{
		j = 0;
		wads[4095] = '\0';
		for (i = 0;i < 4095;i++)
			if (wads[i] != ';' && wads[i] != '\\' && wads[i] != '/' && wads[i] != ':')
				break;
		if (wads[i])
		{
			j=i;
			for (;i < 4095;i++)
			{
				// ignore path...
				if (wads[i] == '\\' || wads[i] == '/' || wads[i] == ':')
					j = i+1;
				else if (wads[i] == ';' || wads[i] == 0)
				{
					k = wads[i];
					wads[i] = 0;
					Q_strncpyz(wadname+9, &wads[j], sizeof(wadname)-9);
					if (wadname[9])
					{
						if (COM_FCheckExists(wadname+9))
							W_LoadTextureWadFile (wadname+9, false);
						else
							W_LoadTextureWadFile (wadname, false);
					}
					j = i+1;
					if (!k)
						break;
				}
			}
		}
	}
	return true;
}
#endif

void Mod_ParseInfoFromEntityLump(model_t *wmodel)	//actually, this should be in the model code.
{
	char token[4096];
	char key[128];
	char skyname[128];
	const char *data = Mod_GetEntitiesString(wmodel);
#ifdef PACKAGE_TEXWAD
	mapskys_t *msky;

	if (wmodel && wmodel->fromgame == fg_halflife)
		strcpy(wads, "decals.wad");
	else
		wads[0] = '\0';
#endif

	cl.skyrotate = 0;
	VectorClear(cl.skyaxis);

#ifndef CLIENTONLY
	if (isDedicated)	//don't bother
		return;
#endif

	// this hack is necessary to ensure Quake 2 maps get their default skybox, without breaking q1 etc
	if (wmodel && wmodel->fromgame == fg_quake2)
		strcpy(skyname, "unit1_");
	else
		skyname[0] = '\0';

	if (data)
	if ((data=COM_ParseOut(data, token, sizeof(token))))	//read the map info.
	if (token[0] == '{')
	while (1)
	{
		if (!(data=COM_ParseOut(data, token, sizeof(token))))
			break; // error
		if (token[0] == '}')
			break; // end of worldspawn
		if (token[0] == '_')
			Q_strncpyz(key, token + 1, sizeof(key));	//_ vars are for comments/utility stuff that arn't visible to progs. Ignore them.
		else
			Q_strncpyz(key, token, sizeof(key));
		if (!((data=COM_ParseOut(data, token, sizeof(token)))))
			break; // error
		if (!strcmp("wad", key)) // for HalfLife maps
		{
#ifdef PACKAGE_TEXWAD
			Q_strncatz(wads, ";", sizeof(wads));	//cache it for later (so that we don't play with any temp memory yet)
			Q_strncatz(wads, token, sizeof(wads));	//cache it for later (so that we don't play with any temp memory yet)
#endif
		}
		else if (!strcmp("skyfog", key)) //override cvars so mappers don't end up hacking cvars and fucking over configs (at least in other engines).
			Cvar_LockFromServer(&r_skyfog, token);
		else if (!strcmp("fog", key) || !strcmp("airfog", key))	//q1 extension. FIXME: should be made temporary.
		{
			key[0] = 'f';
			key[1] = 'o';
			key[2] = 'g';
			key[3] = ' ';
			Q_strncpyz(key+4, token, sizeof(key)-4);
			Cbuf_AddText(key, RESTRICT_INSECURE);
			Cbuf_AddText("\n", RESTRICT_INSECURE);
		}
		else if (!strcmp("waterfog", key))	//q1 extension. FIXME: should be made temporary.
		{
			memcpy(key, "waterfog ", 9);
			Q_strncpyz(key+9, token, sizeof(key)-9);
			Cbuf_AddText(key, RESTRICT_INSECURE);
			Cbuf_AddText("\n", RESTRICT_INSECURE);
		}
		else if (!strcmp("skyroomfog", key))	//q1 extension. FIXME: should be made temporary.
		{
			memcpy(key, "skyroomfog ", 11);
			Q_strncpyz(key+11, token, sizeof(key)-11);
			Cbuf_AddText(key, RESTRICT_INSECURE);
			Cbuf_AddText("\n", RESTRICT_INSECURE);
		}
		else if (!strncmp("cvar_", key, 5)) //override cvars so mappers don't end up hacking cvars and fucking over configs (at least in other engines).
		{
			cvar_t *var = Cvar_FindVar(key+5);
			if (var && !(var->flags & CVAR_NOTFROMSERVER))
				Cvar_LockFromServer(var, token);
		}
		else if (!strcmp("wateralpha", key)) //override cvars so mappers don't end up hacking cvars and fucking over configs (at least in other engines).
		{
			Cvar_LockFromServer(&r_wateralpha, token);
			Cvar_LockFromServer(&r_waterstyle, "1");	//force vanilla-style water too.
		}
		else if (!strcmp("slimealpha", key))
		{
			Cvar_LockFromServer(&r_slimealpha, token);
			Cvar_LockFromServer(&r_slimestyle, "1");
		}
		else if (!strcmp("lavaalpha", key))
		{
			Cvar_LockFromServer(&r_lavaalpha, token);
			Cvar_LockFromServer(&r_lavastyle, "1");
		}
		else if (!strcmp("telealpha", key))
		{
			Cvar_LockFromServer(&r_telealpha, token);
			Cvar_LockFromServer(&r_telestyle, "1");
		}
		else if (!strcmp("skyroom", key)) // for Quake mappers that lack the proper tools
		{
			extern cvar_t v_skyroom;
			Cvar_LockFromServer(&v_skyroom, token);
		}
		else if (!strcmp("skyname", key)) // for HalfLife maps
		{
			Q_strncpyz(skyname, token, sizeof(skyname));
		}
		else if (!strcmp("sky", key)) // for Quake2 maps
		{
			Q_strncpyz(skyname, token, sizeof(skyname));
		}
		else if (!strcmp("skyrotate", key))	//q2 feature
		{
			cl.skyrotate = atof(token);
		}
		else if (!strcmp("skyautorotate", key))	//q2ex feature
		{
			cl.skyautorotate = atof(token);
		}
		else if (!strcmp("skyaxis", key))	//q2 feature
		{
			char *s;
			s = COM_ParseOut(token, key, sizeof(key));
			if (s)
			{
				cl.skyaxis[0] = atof(key);
				s = COM_ParseOut(s, key, sizeof(key));
				if (s)
				{
					cl.skyaxis[1] = atof(key);
					COM_ParseOut(s, key, sizeof(key));
					if (s)
						cl.skyaxis[2] = atof(key);
				}
			}
		}
	}

	if (cl.skyrotate)
	{
		if (cl.skyaxis[0]||cl.skyaxis[1]||cl.skyaxis[2])
			Cvar_Set(&r_skybox_orientation, va("%g %g %g %g", cl.skyaxis[0], cl.skyaxis[1], cl.skyaxis[2], cl.skyrotate));
		else
			Cvar_Set(&r_skybox_orientation, va("0 0 1 %g", cl.skyrotate));
	}
	else
		Cvar_Set(&r_skybox_orientation, "");
	Cvar_SetValue(&r_skybox_autorotate, cl.skyautorotate);

	if (wmodel)
	{
		COM_FileBase (wmodel->name, token, sizeof(token));

#ifdef PACKAGE_TEXWAD
		//map-specific sky override feature
		for (msky = mapskies; msky; msky = msky->next)
		{
			if (!strcmp(msky->mapname, token))
			{
				Q_strncpyz(skyname, msky->skyname, sizeof(skyname));
				break;
			}
		}
#endif
	}

	R_SetSky(skyname);
}
