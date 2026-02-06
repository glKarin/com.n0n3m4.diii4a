/*
Copyright notice from dzip (zlib license):

 (C) 2000-2002 Stefan Schwoon, Nolan Pflug

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Stefan Schwoon	Nolan Pflug
  schwoon@in.tum.de	radix@planetquake.com
*/
/*
Additionally, portions of this code are copy-righted by id software and provided under the gpl v2 or later.
*/

/*
dzip has two primary components:
the first/outer part is that it is some alternative to a .zip, one that stores paks weirdly...
the second part is some entropy differences that just allows zlib to work more effectively, or something.

I've rewritten the archive/outer part to plug it in to fte more nicely
the demo/inner part should mostly be the same as dzip, just with some minor tweaks to make it thread-safe (the 'dc' pointer stuff, in case that's ever an issue).
I have explicitly changed longs to ints, to ensure there's no issues with 64bit builds
*/

#include "quakedef.h"
#include "fs.h"

#ifdef PACKAGE_DZIP

//handle to a file's metadata
typedef struct
{
	fsbucket_t bucket;

	char	name[MAX_QPATH];
	qofs_t	filepos;
	size_t	filelen;	//aka: memory size
	size_t	isize;
	size_t	csize;
	unsigned int ztype;
	time_t mtime;
	unsigned int subfiles;

	//FIXME: flag files as in paks or whatever.
} mdzfile_t;

//handle to the archive itself.
typedef struct
{
	searchpathfuncs_t pub;
	char	descname[MAX_OSPATH];
	int		numfiles;
	mdzfile_t	*files;

	int major_version;

	void		*mutex;
	vfsfile_t	*handle;
	unsigned int filepos;	//the pos the subfiles left it at (to optimize calls to vfs_seek)
	int references;	//seeing as all vfiles from a pak file use the parent's vfsfile, we need to keep the parent open until all subfiles are closed.
} dzarchive_t;

//a file that's actively being read
typedef struct {
	vfsfile_t funcs;
	size_t length;
	size_t currentpos;

	qbyte data[1];
} vfsdz_t;

//
// on disk
//
typedef struct
{
	//v1
	unsigned int offset;
	unsigned int size;
	unsigned int realsize;
	unsigned short namelen;
	unsigned short pak;
	unsigned int crc;
	unsigned int type;

	//only in v2
	unsigned int date;
	unsigned int intersize;

	//name follows for namelen bytes
} dpackfile_t;

typedef struct
{
	char	id[2];	//DZ
	unsigned char major_ver;	//2
	unsigned char minor_ver;	//9
	int		dirofs;
	int		dirlen;
} dpackheader_t;


//defs copied from dzip.h
enum { TYPE_NORMAL, TYPE_DEMV1, TYPE_TXT, TYPE_PAK, TYPE_DZ, TYPE_DEM, TYPE_NEHAHRA, TYPE_DIR, TYPE_STORE };

//stuff to decode
enum {
	DEM_bad, DEM_nop, DEM_disconnect, DEM_updatestat, DEM_version,
	DEM_setview, DEM_sound, DEM_time, DEM_print, DEM_stufftext,
	DEM_setangle, DEM_serverinfo, DEM_lightstyle, DEM_updatename,
	DEM_updatefrags, DEM_clientdata, DEM_stopsound, DEM_updatecolors,
	DEM_particle, DEM_damage, DEM_spawnstatic, DEM_spawnbinary,
	DEM_spawnbaseline, DEM_temp_entity, DEM_setpause, DEM_signonnum,
	DEM_centerprint, DEM_killedmonster, DEM_foundsecret,
	DEM_spawnstaticsound, DEM_intermission, DEM_finale,
	DEM_cdtrack, DEM_sellscreen, DEM_cutscene, DZ_longtime,
/* nehahra */
	DEM_showlmp = 35, DEM_hidelmp, DEM_skybox, DZ_showlmp
};

//basic types
typedef unsigned int uInt;	//gah!
typedef qbyte uchar;

typedef struct {
	uchar voz, pax;
	uchar ang0, ang1, ang2;
	uchar vel0, vel1, vel2;
	int items;
	uchar uk10, uk11, invbit;
	uchar wpf, av, wpm;
	int health;
	uchar am, sh, nl, rk, ce, wp;
	int force;
} cdata_t;
typedef struct {
	uchar modelindex, frame;
	uchar colormap, skin;
	uchar effects;
	uchar ang0, ang1, ang2;
	uchar newbit, present, active;
	uchar fullbright;	/* nehahra */
	int org0, org1, org2;
	int od0, od1, od2;
	int force;
	float alpha;		/* nehahra */
} ent_t;

//the 'globals'
typedef struct
{
	int dem_decode_type;
	qbyte *out;
	qbyte *outend;

#define MAX_ENT 1024
#define p_blocksize 32768

	uchar dem_updateframe;
	uchar copybaseline;
	int maxent, lastent, sble;
	int entlink[MAX_ENT];
	int dem_gametime;
	int outlen;
	int cam0, cam1, cam2;
	uchar inblk[p_blocksize], outblk[p_blocksize], *inptr;
	cdata_t oldcd, newcd;
	ent_t base[MAX_ENT], oldent[MAX_ENT], newent[MAX_ENT];

} decodectx_t;

static void Outfile_Write(decodectx_t *dc, void *outblk, size_t outlen)
{	//throw the data at the file
	if (dc->out + outlen <= dc->outend)
	{
		memcpy(dc->out, outblk, outlen);
		dc->out += outlen;
	}
}

static void copy_msg(decodectx_t *dc, size_t bytes)
{	//just copy the data over
	memcpy(dc->outblk+dc->outlen, dc->inptr, bytes);
	dc->outlen += bytes;
	dc->inptr += bytes;
}
static void insert_msg (decodectx_t *dc, void *data, size_t bytes)
{
	memcpy(dc->outblk+dc->outlen, data, bytes);
	dc->outlen += bytes;
}
static void discard_msg (decodectx_t *dc, size_t bytes)
{
	dc->inptr += bytes;
}

#define Outfile_Write(d,b) Outfile_Write(dc,d,b)
#define copy_msg(b)		copy_msg(dc,b)
#define insert_msg(d,b)	insert_msg(dc,d,b)
#define discard_msg(b)	discard_msg(dc,b)
#define dem_decode_type dc->dem_decode_type
#define copybaseline	dc->copybaseline
#define maxent			dc->maxent
#define lastent			dc->lastent
#define sble			dc->sble
#define entlink			dc->entlink
#define dem_gametime	dc->dem_gametime
#define outlen			dc->outlen
#define cam0			dc->cam0
#define cam1			dc->cam1
#define cam2			dc->cam2
#define inblk			dc->inblk
#define outblk			dc->outblk
#define inptr			dc->inptr
#define oldcd			dc->oldcd
#define newcd			dc->newcd
#define base			dc->base
#define oldent			dc->oldent
#define newent			dc->newent
#define dem_updateframe	dc->dem_updateframe

#define GUI	//because it disables v1

#define getshort(x) LittleShort(*(short*)(x))
#define getlong(x) LittleLong(*(int*)(x))
#define getfloat(x) LittleFloat(*(float*)(x))
#define cnvlong(x) LittleLong(x)

void dem_copy_ue(decodectx_t *dc)
{
	uchar mask = inptr[0] & 0x7f;
	uchar topmask;
	int len = 1;

	topmask = (mask & 0x01)? inptr[len++] : 0x00;
	if (topmask & 0x40) len += 2; else len++;
	if (topmask & 0x04) len++;
	if (mask & 0x40) len++;
	if (topmask & 0x08) len++;
	if (topmask & 0x10) len++;
	if (topmask & 0x20) len++;
	if (mask & 0x02) len += 2;
	if (topmask & 0x01) len++;
	if (mask & 0x04) len += 2;
	if (mask & 0x10) len++;
	if (mask & 0x08) len += 2;
	if (topmask & 0x02) len++;
//	if (topmask & 0x80) /* this should be a bailout */
//		error("dem_copy_ue(): topmask & 0x80");
	copy_msg(len);
}
const uchar te_size[] = {8, 8,  8,  8, 8, 16, 16, 8, 8, 16,
				  8, 8, 10, 16, 8,  8, 14};

/////////////////////////////////////////////////////////////////////////
//Start decode.c

void demx_nop(decodectx_t *dc)
{
	copy_msg(1);
}

void demx_disconnect(decodectx_t *dc)
{
	copy_msg(1);
}

void demx_updatestat(decodectx_t *dc)
{
	copy_msg(6);
}

void demx_version(decodectx_t *dc)
{
	copy_msg(5);
}

void demx_setview(decodectx_t *dc)
{
	copy_msg(3);
}

void demx_sound(decodectx_t *dc)
{
	int c, len;
	uInt entity;
	uchar mask = inptr[1];
	uchar channel;

	if (*inptr > DEM_sound)
	{
		len = 10;
		mask = *inptr & 3;
	}
	else
		len = 11;
	if (mask & 0x01) len++;
	if (mask & 0x02) len++;

#ifndef GUI
	if (dem_decode_type == TYPE_DEMV1) { copy_msg(len); return; }
#endif
	*inptr = DEM_sound;
	insert_msg(inptr,1);

	*inptr = mask;
	channel = inptr[len-9] & 7;
	inptr[len-9] = (inptr[len-9] & 0xf8) + ((2 - channel) & 7);

	if ((entity = getshort(inptr+len-9) >> 3) < MAX_ENT)
	{
		c = getshort(inptr+len-6); c += newent[entity].org0;
		c = cnvlong(c); memcpy(inptr+len-6,&c,2);
		c = getshort(inptr+len-4); c += newent[entity].org1;
		c = cnvlong(c); memcpy(inptr+len-4,&c,2);
		c = getshort(inptr+len-2); c += newent[entity].org2;
		c = cnvlong(c); memcpy(inptr+len-2,&c,2);
	}

	copy_msg(len);
}

void demx_longtime(decodectx_t *dc)
{
	int tmp = getlong(inptr+1);
	dem_gametime += tmp;
	tmp = cnvlong(dem_gametime);
	*inptr = DEM_time;
	memcpy(inptr+1,&tmp,4);
	copy_msg(5);
}

void demx_time(decodectx_t *dc)
{
	uchar buf[5];
	int tmp = getshort(inptr+1) & 0xffff;

#ifndef GUI
	if (dem_decode_type == TYPE_DEMV1) { demx_longtime(); return; }
#endif
	dem_gametime += tmp;
	tmp = cnvlong(dem_gametime);
	buf[0] = DEM_time;
	memcpy(buf+1,&tmp,4);
	insert_msg(buf,5);
	discard_msg(3);
}

/* used by lots of msgs */
void demx_string(decodectx_t *dc)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	copy_msg(ptr-inptr);
}

void demx_setangle(decodectx_t *dc)
{
	copy_msg(4);
}

void demx_serverinfo(decodectx_t *dc)
{
	uchar *ptr = inptr + 7;
	uchar *start_ptr;
	while (*ptr++);
	do {
		start_ptr = ptr;
		while (*ptr++);
	} while (ptr - start_ptr > 1);
	do {
		start_ptr = ptr;
		while (*ptr++);
	} while (ptr - start_ptr > 1);
	copy_msg(ptr-inptr);
	sble = 0;
}

void demx_lightstyle(decodectx_t *dc)
{
	uchar *ptr = inptr + 2;
	while (*ptr++);
	copy_msg(ptr-inptr);
}

void demx_updatename(decodectx_t *dc)
{
	uchar *ptr = inptr + 2;
	while (*ptr++);
	copy_msg(ptr-inptr);
}

void demx_updatefrags(decodectx_t *dc)
{
	copy_msg(4);
}

static int bplus(int x, int y)
{
	if (x >= 128) x -= 256;
	return y + x;
}

void create_clientdata_msg(decodectx_t *dc)
{
	uchar buf[32];
	uchar *ptr = buf+3;
	int mask = newcd.invbit? 0 : 0x0200;
	int tmp;

	buf[0] = DEM_clientdata;

	#define CMADD(x,def,bit,bit2) if (newcd.x != def || newcd.force & bit2)\
		{ mask |= bit; *ptr++ = newcd.x; }

	CMADD(voz,22,0x0001,0x0800);
	CMADD(pax,0,0x0002,0x1000);
	CMADD(ang0,0,0x0004,0x0100);
	CMADD(vel0,0,0x0020,0x0001);
	CMADD(ang1,0,0x0008,0x0200);
	CMADD(vel1,0,0x0040,0x0002);
	CMADD(ang2,0,0x0010,0x0400);
	CMADD(vel2,0,0x0080,0x0004);
	tmp = cnvlong(newcd.items); memcpy(ptr,&tmp,4); ptr += 4;
	if (newcd.uk10) mask |= 0x0400;
	if (newcd.uk11) mask |= 0x0800;
	CMADD(wpf,0,0x1000,0x2000);
	CMADD(av,0,0x2000,0x4000);
	CMADD(wpm,0,0x4000,0x8000);
	tmp = cnvlong(newcd.health); memcpy(ptr,&tmp,2); ptr += 2;
	*ptr++ = newcd.am;
	*ptr++ = newcd.sh;
	*ptr++ = newcd.nl;
	*ptr++ = newcd.rk;
	*ptr++ = newcd.ce;
	*ptr++ = newcd.wp;
	mask = cnvlong(mask);
	memcpy(buf+1,&mask,2);
	insert_msg(buf,ptr-buf);

	oldcd = newcd;
}

#define CPLUS(x,bit) if (mask & bit) { newcd.x = bplus(*ptr++,oldcd.x); }

void demx_clientdata(decodectx_t *dc)
{
	uchar *ptr = inptr;
	int mask = *ptr++;

	newcd = oldcd;

#ifndef GUI
	if (dem_decode_type == TYPE_DEMV1) { demv1_clientdata(); return; }
#endif
	if (mask & 0x08) mask += *ptr++ << 8;
	if (mask & 0x8000) mask += *ptr++ << 16;
	if (mask & 0x800000) mask += *ptr++ << 24;

	CPLUS(vel2,0x00000001);
	CPLUS(vel0,0x00000002);
	CPLUS(vel1,0x00000004);

	CPLUS(wpf,0x00000100);
	if (mask & 0x00000200) newcd.uk10 = !oldcd.uk10;
	CPLUS(ang0,0x00000400);
	CPLUS(am,0x00000800);
	if (mask & 0x00001000) { newcd.health += getshort(ptr); ptr += 2; }
	if (mask & 0x00002000) { newcd.items ^= getlong(ptr); ptr += 4; }
	CPLUS(av,0x00004000);

	CPLUS(pax,0x00010000);
	CPLUS(sh,0x00020000);
	CPLUS(nl,0x00040000);
	CPLUS(rk,0x00080000);
	CPLUS(wpm,0x00100000);
	CPLUS(wp,0x00200000);
	if (mask & 0x00400000) newcd.uk11 = !oldcd.uk11;

	CPLUS(voz,0x01000000);
	CPLUS(ce,0x02000000);
	CPLUS(ang1,0x04000000);
	CPLUS(ang2,0x08000000);
	if (mask & 0x10000000) newcd.invbit = !oldcd.invbit;

	discard_msg(ptr-inptr);

	if ((*ptr & 0xf0) == 0x50)
	{
		mask = *ptr++;
		if (mask & 0x08) mask |= *ptr++ << 8;
		newcd.force ^= mask & 0xff07;
		discard_msg(ptr-inptr);
	}

	create_clientdata_msg(dc);
}

void demx_stopsound(decodectx_t *dc)
{
	copy_msg(3);
}

void demx_updatecolors(decodectx_t *dc)
{
	copy_msg(3);
}

void demx_particle(decodectx_t *dc)
{
	copy_msg(12);
}

void demx_damage(decodectx_t *dc)
{
	copy_msg(9);
}

void demx_spawnstatic(decodectx_t *dc)
{
	copy_msg(14);
}

void demx_spawnbinary(decodectx_t *dc)
{
	copy_msg(1);
}

void demx_spawnbaseline(decodectx_t *dc)
{
	uchar buf[16], *ptr = inptr+3;
	ent_t ent;
	int mask = getshort(inptr+1);
	
	sble = (sble + (mask & 0x03ff)) % 0x400;
	memset(&ent,0,sizeof(ent_t));
	ent.modelindex = *ptr++;
	if (mask & 0x0400) ent.frame = *ptr++;
	if (mask & 0x0800) ent.colormap = *ptr++;
	if (mask & 0x1000) ent.skin = *ptr++;
	if (mask & 0x2000)
	{
		ent.org0 = getshort(ptr); ptr += 2;
		ent.org1 = getshort(ptr); ptr += 2;
		ent.org2 = getshort(ptr); ptr += 2;
	}
	if (mask & 0x4000) ent.ang1 = *ptr++;
	if (mask & 0x8000) { ent.ang0 = *ptr++; ent.ang2 = *ptr++; }
	discard_msg(ptr-inptr);

	buf[0] = DEM_spawnbaseline;
	mask = cnvlong(sble); memcpy(buf+1,&mask,2);
	buf[3] = ent.modelindex;
	buf[4] = ent.frame;
	buf[5] = ent.colormap;
	buf[6] = ent.skin;
	mask = cnvlong(ent.org0); memcpy(buf+7,&mask,2);
	buf[9] = ent.ang0;
	mask = cnvlong(ent.org1); memcpy(buf+10,&mask,2);
	buf[12] = ent.ang1;
	mask = cnvlong(ent.org2); memcpy(buf+13,&mask,2);
	buf[15] = ent.ang2;
	insert_msg(buf,16);

	base[sble] = ent;
	copybaseline = 1;
}

void demx_temp_entity(decodectx_t *dc)
{
	if (inptr[1] == 17)
		copy_msg(strlen(inptr + 2) + 17);
	else
		copy_msg(te_size[inptr[1]]);
}

void demx_setpause(decodectx_t *dc)
{
	copy_msg(2);
}

void demx_signonnum(decodectx_t *dc)
{
	copy_msg(2);
}

void demx_killedmonster(decodectx_t *dc)
{
	copy_msg(1);
}

void demx_foundsecret(decodectx_t *dc)
{
	copy_msg(1);
}

void demx_spawnstaticsound(decodectx_t *dc)
{
	copy_msg(10);
}

void demx_intermission(decodectx_t *dc)
{
	copy_msg(1);
}

void demx_cdtrack(decodectx_t *dc)
{
	copy_msg(3);
}

void demx_sellscreen(decodectx_t *dc)
{
	copy_msg(1);
}

/* nehahra */
void demx_showlmp(decodectx_t *dc)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	while (*ptr++);
	ptr += 2;
	*inptr = DEM_showlmp;
	copy_msg(ptr-inptr);
}

void demx_updateentity(decodectx_t *dc)
{
	uchar buf[32], *ptr;
	int mask, i, entity;
	int baseval = 0, prev;
	ent_t n, o;
	int tmp;

#ifndef GUI
	if (dem_decode_type == TYPE_DEMV1) { demv1_updateentity(); return; }
#endif
	lastent = 0;
	for (ptr = inptr+1; *ptr; ptr++)
	{
		if (*ptr == 0xff) { baseval += 0xfe; continue; }
		entity = baseval + *ptr;
		newent[entity].active = 1;
		while (entlink[lastent] <= entity) lastent = entlink[lastent];
		if (lastent < entity)
		{
			entlink[entity] = entlink[lastent];
			entlink[lastent] = entity;
		}
	}

	for (prev = 0, i = entlink[0], ptr++; i < MAX_ENT; i = entlink[i])
	{
		newent[i].org0 += newent[i].od0;
		newent[i].org1 += newent[i].od1;
		newent[i].org2 += newent[i].od2;

		if (!newent[i].active) { prev = i; continue; }

		mask = *ptr++;

		if (mask == 0x80)
		{
			oldent[i] = newent[i] = base[i];
			entlink[prev] = entlink[i];
			continue;
		}

		prev = i;

		if (mask == 0x00) { newent[i].active = 0; continue; }

		if (mask & 0x80) mask += (*ptr++) << 8;
		if (mask & 0x8000) mask += (*ptr++) << 16;

		n = newent[i];
		o = oldent[i];

		if (mask & 0x000001) { n.od2 = bplus(*ptr++,o.od2);
				       n.org2 = o.org2 + n.od2; }
		if (mask & 0x000800) { n.org2 = getshort(ptr); ptr += 2;
				       n.od2 = n.org2 - o.org2; }
		if (mask & 0x000002) { n.od1 = bplus(*ptr++,o.od1);
				       n.org1 = o.org1 + n.od1; }
		if (mask & 0x000400) { n.org1 = getshort(ptr); ptr += 2;
				       n.od1 = n.org1 - o.org1; }
		if (mask & 0x000004) { n.od0 = bplus(*ptr++,o.od0);
				       n.org0 = o.org0 + n.od0; }
		if (mask & 0x000200) { n.org0 = getshort(ptr); ptr += 2;
				       n.od0 = n.org0 - o.org0; }

		if (mask & 0x000008) n.ang0 = bplus(*ptr++,o.ang0);
		if (mask & 0x000010) n.ang1 = bplus(*ptr++,o.ang1);
		if (mask & 0x000020) n.ang2 = bplus(*ptr++,o.ang2);
		if (mask & 0x000040) n.frame = o.frame+1;
		if (mask & 0x000100) n.frame = bplus(*ptr++,o.frame);

		if (mask & 0x001000) n.effects = *ptr++;
		if (mask & 0x002000) n.modelindex = *ptr++;
		if (mask & 0x004000) n.newbit = !o.newbit;
		if (mask & 0x010000) n.colormap = *ptr++;
		if (mask & 0x020000) n.skin = *ptr++;
	/* nehahra */
		if (mask & 0x040000) { n.alpha = getfloat(ptr); ptr += 4; }
		if (mask & 0x080000) n.fullbright = *ptr++;

		newent[i] = n;
	}

	if (*ptr == 0x31)
	{
		ptr++;
		while ((mask = getshort(ptr)))
		{
			ptr += 2;
			mask &= 0xffff;
			if (mask & 0x8000) mask |= *ptr++ << 16;
			entity = mask & 0x3ff;
			newent[entity].force ^= mask & 0xfffc00;
		}
		ptr += 2;
	}

	discard_msg(ptr-inptr);

	for (i = entlink[0]; i < MAX_ENT; i = entlink[i])
	{
		ent_t n = newent[i], b = base[i];

		ptr = buf+2;
		mask = 0x80;

		if (i > 0xff || (n.force & 0x400000))
		{
			tmp = cnvlong(i);
			memcpy(ptr,&tmp,2);
			ptr += 2;
			mask |= 0x4000;
		}
		else
			*ptr++ = i;

		#define BDIFF(x,bit,bit2) \
			if (n.x != b.x || n.force & bit2) \
				{ *ptr++ = n.x; mask |= bit; }

		BDIFF(modelindex,0x0400,0x040000);
		BDIFF(frame,0x0040,0x4000);
		BDIFF(colormap,0x0800,0x080000);
		BDIFF(skin,0x1000,0x100000);
		BDIFF(effects,0x2000,0x200000);
		if (n.org0 != b.org0 || n.force & 0x010000)
		    { mask |= 0x0002; tmp = cnvlong(n.org0); 
		      memcpy(ptr,&tmp,2); ptr += 2; }
		BDIFF(ang0,0x0100,0x0800);
		if (n.org1 != b.org1 || n.force & 0x0400)
		    { mask |= 0x0004; tmp = cnvlong(n.org1); 
		      memcpy(ptr,&tmp,2); ptr += 2; }
		BDIFF(ang1,0x0010,0x1000);
		if (n.org2 != b.org2 || n.force & 0x020000)
		    { mask |= 0x0008; tmp = cnvlong(n.org2); 
		      memcpy(ptr,&tmp,2); ptr += 2; }
		BDIFF(ang2,0x0200,0x2000);
/* nehahra */
		if (n.force & 0x800000)
		{
			float f = 1;

			if (n.fullbright)
				f = 2;
			tmp = cnvlong(*(int *)&f);
			memcpy(ptr, &tmp, 4);
			tmp = cnvlong(*(int *)&n.alpha);
			memcpy(ptr + 4, &tmp, 4);
			ptr += 8;
			if (f == 2)
			{
				f = (char)(n.fullbright - 1);
				tmp = cnvlong(*(int *)&f);
				memcpy(ptr, &tmp, 4);
				ptr += 4;
			}
			mask |= 0x8000;
		}
		
		if (n.newbit) mask |= 0x20;
		if (mask & 0xff00) mask |= 0x01;
		buf[0] = mask & 0xff;
		buf[1] = (mask & 0xff00) >> 8;
		if (!(mask & 0x01)) { memcpy(buf+1,buf+2,ptr-buf-2); ptr--; }
		insert_msg(buf,ptr-buf);

		oldent[i] = newent[i];
	}

}

void (* const demx_message[])(decodectx_t *dc) = {
	demx_nop, demx_disconnect, demx_updatestat, demx_version,
	demx_setview, demx_sound, demx_time, demx_string, demx_string,
	demx_setangle, demx_serverinfo, demx_lightstyle, demx_updatename,
	demx_updatefrags, demx_clientdata, demx_stopsound, demx_updatecolors,
	demx_particle, demx_damage, demx_spawnstatic, demx_spawnbinary,
	demx_spawnbaseline, demx_temp_entity, demx_setpause, demx_signonnum,
	demx_string, demx_killedmonster, demx_foundsecret,
	demx_spawnstaticsound, demx_intermission, demx_string,
	demx_cdtrack, demx_sellscreen, demx_string, demx_longtime,
	demx_string, demx_string, demx_showlmp	/* nehahra */
};

void dem_uncompress_init (decodectx_t *dc, int type)
{
	dem_decode_type = -type;
	memset(&base,0,sizeof(ent_t)*MAX_ENT);
	memset(&oldent,0,sizeof(ent_t)*MAX_ENT);
	memset(&oldcd,0,sizeof(cdata_t));
	oldcd.voz = 22;
	oldcd.items = 0x4001;
	entlink[0] = MAX_ENT;
	cam0 = cam1 = cam2 = 0;
	copybaseline = 0;
	dem_gametime = 0;
	maxent = 0;
	sble = 0;
}
	
uInt dem_uncompress_block(decodectx_t *dc)
{
	int a1;
	uchar cfields;
#ifdef GUI
	int uemask = 0x30, cdmask = 0x40;
#else
	int uemask = (dem_decode_type == TYPE_DEMV1)? 0x80 : 0x30;
	int cdmask = (dem_decode_type == TYPE_DEMV1)? 0xf0 : 0x40;
#endif
	cfields = *inptr++;

	if (cfields & 1) { cam0 += getlong(inptr); inptr += 4; }
	if (cfields & 2) { cam1 += getlong(inptr); inptr += 4; }
	if (cfields & 4) { cam2 += getlong(inptr); inptr += 4; }

	outlen = 0;
	a1 = 0/*length*/; insert_msg(&a1,4);
	a1 = cnvlong(cam0); insert_msg(&a1,4);
	a1 = cnvlong(cam1); insert_msg(&a1,4);
	a1 = cnvlong(cam2); insert_msg(&a1,4);

	dem_updateframe = 0;
	while (*inptr)
	{		
		if ((*inptr & 0xf8) == uemask)
			demx_updateentity(dc);
		else
		{
#ifndef GUI
			if (dem_updateframe)
				{ demv1_dxentities(); dem_updateframe = 0; }
#endif
			if (*inptr && *inptr <= DZ_showlmp)
				demx_message[*inptr - 1](dc);
			else if ((*inptr & 0xf0) == cdmask) 
				demx_clientdata(dc);
			else if ((*inptr & 0xf8) == 0x38)
				demx_sound(dc);
			else if (*inptr >= 0x80)
				dem_copy_ue(dc);
			else
				return 0;
		}
	}
#ifndef GUI
	if (dem_updateframe) demv1_dxentities();
#endif
	outlen -= 16;
	outlen = cnvlong(outlen);
	memcpy(outblk,&outlen,4);
	Outfile_Write(outblk,cnvlong(outlen)+16);

	if (copybaseline)
	{
		copybaseline = 0;
		memcpy(oldent,base,sizeof(ent_t)*MAX_ENT);
		memcpy(newent,base,sizeof(ent_t)*MAX_ENT);
	}

	return inptr-inblk+1;
}

uInt dem_uncompress (decodectx_t *dc, uInt maxsize)
{
	uInt blocksize = 0;
	inptr = inblk;
	if (dem_decode_type < 0) 
	{
		dem_decode_type = -dem_decode_type;
		while (inptr[blocksize] != '\n' && blocksize < 12)
			blocksize++;

		if (blocksize == 12)	/* seriously corrupt! */
			return 0;

		Outfile_Write(inblk, ++blocksize);
		inptr += blocksize;
	}
	while (blocksize < 16000 && blocksize < maxsize)
	{
		if (*inptr == 0xff)
		{
			uInt len = getlong(inptr+1);
			if (p_blocksize - blocksize - 5 < len)
				return blocksize;
			Outfile_Write(inptr + 5,len);
			blocksize = inptr - inblk + len + 5;
		}
		else
			blocksize = dem_uncompress_block(dc);
		if (!blocksize)
			return 0;	/* corrupt encoding */
		inptr++;
	}
	return blocksize;
}


//End decode.c
/////////////////////////////////////////////////////////////////////////
#undef outlen

#undef copy_msg
#undef insert_msg
#undef discard_msg
#undef inptr
#undef dem_decode_type
#undef copybaseline
#undef maxent
#undef lastent
#undef sble
#undef entlink
#undef dem_gametime
#undef outlen
#undef cam0
#undef cam1
#undef cam2
#undef inblk
#undef outblk
#undef inptr
#undef oldcd
#undef newcd
#undef base
#undef oldent
#undef newent
#undef dem_updateframe

#include <zlib.h>
//pack mutex must be held for this function.
qboolean FSDZ_ExtractFile(qbyte *out, size_t outsize, dzarchive_t *pack, mdzfile_t *src)
{
	switch(src->ztype)
	{
	case TYPE_PAK:
		{
			unsigned int i;
			unsigned int dirsize = src->subfiles * 64;
			unsigned int diroffset = src->filelen - dirsize;
			size_t ofs;
			qbyte *ftab = out + diroffset;
			out[0] = 'P';
			out[1] = 'A';
			out[2] = 'C';
			out[3] = 'K';
			((int*)out)[2] = LittleLong(dirsize);//size;
			((int*)out)[1] = LittleLong(src->filelen - dirsize);//offset;

			for (ofs = 12, i = 1; i <= src->subfiles; i++)
			{
				if (ofs + src[i].filelen > diroffset)
					return false;	//panic!
				FSDZ_ExtractFile(out+ofs, src[i].filelen, pack, src+i);

				Q_strncpyz(ftab, src[i].name, 56);
				*(int*)&(ftab[56]) = ofs;
				*(int*)&(ftab[60]) = src[i].filelen;
				ftab += 64;

				ofs += src[i].filelen;
			}
		}
		return true;

	case TYPE_STORE:	//no compression or anything
		VFS_SEEK(pack->handle, src->filepos);
		return outsize == VFS_READ(pack->handle, out, outsize);

	//not actually a file... we shouldn't be here.
	case TYPE_DIR:
		return false;
	case TYPE_DEMV1:	//dz v1 == solid archive. really messy, we don't support them.
		return false;

	case TYPE_NORMAL:
	case TYPE_TXT:
	case TYPE_DZ:	//its defined. its weird, but its defined.
	case TYPE_DEM:
	case TYPE_NEHAHRA:
		{
			//decodectx_t *dc = NULL;
			unsigned char inbuffer[p_blocksize];
			int ret;
			size_t inremaining = src->csize;
			decodectx_t *dc = NULL;

			z_stream strm = {
				inbuffer,
				0,
				0,

				out,
				src->isize,
				0
			};
			strm.data_type = Z_UNKNOWN;

			if (src->ztype == TYPE_DEM||src->ztype == TYPE_NEHAHRA)
			{
				dc = Z_Malloc(sizeof(*dc));
				dc->out = out;
				dc->outend = out + outsize;
				dem_uncompress_init(dc, src->ztype);

				strm.next_out = dc->inblk;
				strm.avail_out = sizeof(dc->inblk);
			}

			VFS_SEEK(pack->handle, src->filepos);

			strm.avail_in = 0;
			strm.next_in = inbuffer;

			inflateInit(&strm);

			while ((ret=inflate(&strm, Z_SYNC_FLUSH)) != Z_STREAM_END)
			{
				if (strm.avail_in == 0 || strm.avail_out == 0)
				{	//keep feeding the beast
					if (strm.avail_in == 0)
					{
						size_t chunk = inremaining;
						if (chunk > sizeof(inbuffer))
							chunk = sizeof(inbuffer);
						strm.avail_in = VFS_READ(pack->handle, inbuffer, chunk);
						inremaining -= strm.avail_in;
						strm.next_in = inbuffer;
						if (!strm.avail_in)
							break;
					}

					//and cleaning up its excrement
					if (strm.avail_out == 0)
					{
						if (dc)
						{
							int chunk = dem_uncompress(dc, strm.next_out - dc->inblk);
							int remaining = strm.next_out-(dc->inblk+chunk);
							if (!chunk)
								break;	//made no progress. that's bad
							memmove(dc->inblk, dc->inblk+chunk, remaining);

							strm.next_out = dc->inblk+remaining;
							strm.avail_out = sizeof(dc->inblk)-remaining;
						}
						else
							break;	//eep
					}
					continue;
				}

				//doh, it terminated for no reason
				if (ret != Z_STREAM_END)
					break;
			}

			if (dc)
			{
				while(1)
				{
					int chunk = dem_uncompress(dc, strm.next_out - dc->inblk);
					int remaining = strm.next_out-(dc->inblk+chunk);
					if (!chunk || !remaining)
						break;	//made no progress. that's bad
					memmove(dc->inblk, dc->inblk+chunk, remaining);
					strm.next_out = dc->inblk+remaining;
				}
				Z_Free(dc);
				dc = NULL;
			}

			inflateEnd(&strm);
			return strm.total_out == src->isize && !inremaining && ret == Z_STREAM_END;
		}
		return false;

	default:	//unknown file types can just fail.
		return false;
	}
}


static void QDECL FSDZ_GetPathDetails(searchpathfuncs_t *handle, char *out, size_t outlen)
{
	dzarchive_t *pak = (dzarchive_t*)handle;

	*out = 0;
	if (pak->references != 1)
		Q_snprintfz(out, outlen, "(%i)", pak->references-1);
}
static void QDECL FSDZ_ClosePath(searchpathfuncs_t *handle)
{
	qboolean stillopen;
	dzarchive_t *pak = (void*)handle;

	if (!Sys_LockMutex(pak->mutex))
		return;	//ohnoes
	stillopen = --pak->references > 0;
	Sys_UnlockMutex(pak->mutex);
	if (stillopen)
		return;	//not free yet


	VFS_CLOSE (pak->handle);

	Sys_DestroyMutex(pak->mutex);
	if (pak->files)
		Z_Free(pak->files);
	Z_Free(pak);
}
static void QDECL FSDZ_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	dzarchive_t *pak = (void*)handle;
	int i;

	for (i = 0; i < pak->numfiles; i++)
	{
		AddFileHash(depth, pak->files[i].name, &pak->files[i].bucket, &pak->files[i]);
	}
}
static unsigned int QDECL FSDZ_FLocate(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	mdzfile_t *pf = hashedresult;
	int i;
	dzarchive_t		*pak = (void*)handle;

// look through all the pak file elements

	if (pf)
	{	//is this a pointer to a file in this pak?
		if (pf < pak->files || pf > pak->files + pak->numfiles)
			return FF_NOTFOUND;	//was found in a different path
	}
	else
	{
		for (i=0 ; i<pak->numfiles ; i++)	//look for the file
		{
			if (!Q_strcasecmp (pak->files[i].name, filename))
			{
				pf = &pak->files[i];
				break;
			}
		}
	}

	if (pf)
	{
		if (loc)
		{
			loc->fhandle = pf;
			snprintf(loc->rawname, sizeof(loc->rawname), "%s", pak->descname);
			loc->offset = pf->filepos;
			loc->len = pf->filelen;
		}
		return FF_FOUND;
	}
	return FF_NOTFOUND;
}
static int QDECL FSDZ_EnumerateFiles (searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	dzarchive_t	*pak = (dzarchive_t*)handle;
	int		num;

	for (num = 0; num<(int)pak->numfiles; num++)
	{
		if (wildcmp(match, pak->files[num].name))
		{
			if (!func(pak->files[num].name, pak->files[num].filelen, pak->files[num].mtime, parm, handle))
				return false;
		}
	}

	return true;
}

static int QDECL FSDZ_GeneratePureCRC(searchpathfuncs_t *handle, const int *seed)
{
	dzarchive_t *pak = (void*)handle;

	int result;
	int *filecrcs;
	int numcrcs=0;
	int i;

	filecrcs = BZ_Malloc((pak->numfiles+1)*sizeof(int));
	if (seed)
		filecrcs[numcrcs++] = *seed;

	for (i = 0; i < pak->numfiles; i++)
	{
		if (pak->files[i].filelen > 0)
		{
			filecrcs[numcrcs++] = pak->files[i].filepos ^ pak->files[i].filelen ^ CalcHashInt(&hash_crc16, pak->files[i].name, sizeof(56));
		}
	}

	result = CalcHashInt(&hash_md4, filecrcs, numcrcs*sizeof(int));

	BZ_Free(filecrcs);
	return result;
}

static int QDECL VFSDZ_ReadBytes (struct vfsfile_s *vfs, void *buffer, int bytestoread)
{
	vfsdz_t *vfsp = (vfsdz_t*)vfs;

	if (bytestoread == 0)
		return 0;

	if (vfsp->currentpos + bytestoread > vfsp->length)
		bytestoread = vfsp->length - vfsp->currentpos;
	if (bytestoread <= 0)
		return -1;

	memcpy(buffer, vfsp->data + vfsp->currentpos, bytestoread);
	vfsp->currentpos += bytestoread;

	return bytestoread;
}
static int QDECL VFSDZ_WriteBytes (struct vfsfile_s *vfs, const void *buffer, int bytestoread)
{	//not supported.
	Sys_Error("Cannot write to dz files\n");
	return 0;
}
static qboolean QDECL VFSDZ_Seek (struct vfsfile_s *vfs, qofs_t pos)
{
	vfsdz_t *vfsp = (vfsdz_t*)vfs;
	if (pos > vfsp->length)
		return false;
	vfsp->currentpos = pos;

	return true;
}
static qofs_t QDECL VFSDZ_Tell (struct vfsfile_s *vfs)
{
	vfsdz_t *vfsp = (vfsdz_t*)vfs;
	return vfsp->currentpos;
}
static qofs_t QDECL VFSDZ_GetLen (struct vfsfile_s *vfs)
{
	vfsdz_t *vfsp = (vfsdz_t*)vfs;
	return vfsp->length;
}
static qboolean QDECL VFSDZ_Close(vfsfile_t *vfs)
{
	vfsdz_t *vfsp = (vfsdz_t*)vfs;
	Z_Free(vfsp);	//free ourselves.
	return true;
}
static vfsfile_t *QDECL FSDZ_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	dzarchive_t *pack = (dzarchive_t*)handle;
	vfsdz_t *vfs;
	mdzfile_t *pf = loc->fhandle;

	if (strcmp(mode, "rb") && strcmp(mode, "r") && strcmp(mode, "rt"))
		return NULL; //urm, unable to write/append

	vfs = Z_Malloc(sizeof(vfsdz_t) + pf->filelen);

	if (!Sys_LockMutex(pack->mutex))
	{
		Z_Free(vfs);
		return NULL;
	}

	if (!FSDZ_ExtractFile(vfs->data, pf->filelen, pack, pf))
	{
		Sys_UnlockMutex(pack->mutex);
		Z_Free(vfs);
		return NULL;
	}
	Sys_UnlockMutex(pack->mutex);

	vfs->length = loc->len;
	vfs->currentpos = 0;

#ifdef _DEBUG
	Q_strncpyz(vfs->funcs.dbgname, pf->name, sizeof(vfs->funcs.dbgname));
#endif
	vfs->funcs.Close = VFSDZ_Close;
	vfs->funcs.GetLen = VFSDZ_GetLen;
	vfs->funcs.ReadBytes = VFSDZ_ReadBytes;
	vfs->funcs.Seek = VFSDZ_Seek;
	vfs->funcs.Tell = VFSDZ_Tell;
	vfs->funcs.WriteBytes = VFSDZ_WriteBytes;	//not supported

	return (vfsfile_t *)vfs;
}

static void QDECL FSDZ_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = FSDZ_OpenVFS(handle, loc, "rb");
	if (!f)	//err...
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}


/*
=================
COM_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
searchpathfuncs_t *QDECL FSDZ_LoadArchive (vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	dpackheader_t	header;
	int				i;
//	int				j;
	mdzfile_t		*newfiles;
	int				numpackfiles;
	dzarchive_t		*pack;
	vfsfile_t		*packhandle;
	dpackfile_t		info;
	int read;
	struct tm t;
//	unsigned short		crc;

	memset(&t, 0, sizeof(t));

	packhandle = file;
	if (packhandle == NULL)
		return NULL;

	if (prefix && *prefix)
		return NULL;	//not supported at this time

	read = VFS_READ(packhandle, &header, sizeof(header));
	if (read < sizeof(header) || header.id[0] != 'D' || header.id[1] != 'Z')
	{
		Con_Printf("%s is not a dz - %c%c\n", desc, header.id[0], header.id[1]);
		return NULL;
	}
	if (header.major_ver > 2/* || (header.major_ver == 2 && header.minor_ver > 9)*/)
	{	//ignore minor versions, assume they've got only additions.
		Con_Printf("%s uses too recent a version. %i.%i > 2.9\n", desc, header.major_ver, header.minor_ver);
		return NULL;
	}
	if (header.major_ver < 2)
	{
		Con_Printf("%s uses too old a version. %i.%i < 2.0\n", desc, header.major_ver, header.minor_ver);
		return NULL;
	}
	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	numpackfiles = header.dirlen;

	newfiles = (mdzfile_t*)Z_Malloc (numpackfiles * sizeof(mdzfile_t));

	VFS_SEEK(packhandle, header.dirofs);

	pack = (dzarchive_t*)Z_Malloc (sizeof (dzarchive_t));
// parse the directory
	for (i=0 ; i<numpackfiles ; i++)
	{
		if (header.major_ver == 1)
			read = VFS_READ(packhandle, &info, sizeof(info)-8) == sizeof(info)-8;
		else
			read = VFS_READ(packhandle, &info, sizeof(info)) == sizeof(info);
		if (!read)
		{
			Con_Printf("DZIP file table truncated, only found %i files out of %i\n", i, numpackfiles);
			numpackfiles = i;
			break;
		}
/*
		for (j=0 ; j<sizeof(info) ; j++)
			CRC_ProcessByte(&crc, ((qbyte *)&info)[j]);
*/
		info.type = LittleLong(info.type);
		info.namelen = LittleShort(info.namelen);
		if (info.namelen >= sizeof(newfiles[i].name) || info.type==TYPE_PAK)
		{
			//ignore dzip's paks. this allows us to just directly read the demos inside without extra subdirs.
			VFS_SEEK(packhandle, VFS_TELL(packhandle)+info.namelen);
			numpackfiles--;
			i--;	//counter the ++
			continue;
		}
		newfiles[i].name[info.namelen] = 0; //paranoid
		if (info.namelen != VFS_READ(packhandle, &newfiles[i].name, info.namelen))
		{
			Con_Printf("DZIP file table truncated, only found %i files out of %i\n", i, numpackfiles);
			numpackfiles = i;
			break;
		}
		COM_CleanUpPath(newfiles[i].name);	//this fixes silly people using backslashes in paths.
		newfiles[i].filepos  = LittleLong(info.offset);
		newfiles[i].filelen  = LittleLong(info.realsize);
		newfiles[i].isize    = LittleLong(info.intersize);
		newfiles[i].csize    = LittleLong(info.size);
		newfiles[i].ztype    = info.type;
		newfiles[i].subfiles = (unsigned short)LittleShort(info.pak);

		//lame, but whatever
		//fixme: make sure they're all correct...
		t.tm_year = ((info.date >> 25) & 0x7f) + 1980 - 1900;
		t.tm_mon = ((info.date >> 21) & 0x0f);
		t.tm_mday = (info.date >> 16) & 0x1f;
		t.tm_hour = ((info.date >> 11) & 0x1f)-1;
		t.tm_min = (info.date >> 5) & 0x3f;
		t.tm_sec = (info.date & 0x1f) << 1;
		newfiles[i].mtime = mktime(&t);
	}

	strcpy (pack->descname, desc);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;
	pack->filepos = 0;
	VFS_SEEK(packhandle, pack->filepos);

	pack->references++;

	pack->mutex = Sys_CreateMutex();

//	Con_TPrintf ("Added packfile %s (%i files)\n", desc, numpackfiles);

	pack->pub.fsver           = FSVER;
	pack->pub.GetPathDetails  = FSDZ_GetPathDetails;
	pack->pub.ClosePath       = FSDZ_ClosePath;
	pack->pub.BuildHash       = FSDZ_BuildHash;
	pack->pub.FindFile	      = FSDZ_FLocate;
	pack->pub.ReadFile        = FSDZ_ReadFile;
	pack->pub.EnumerateFiles  = FSDZ_EnumerateFiles;
	pack->pub.GeneratePureCRC = FSDZ_GeneratePureCRC;
	pack->pub.OpenVFS         = FSDZ_OpenVFS;
	return &pack->pub;
}

#endif
