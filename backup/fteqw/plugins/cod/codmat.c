#include "../plugin.h"
#include "shader.h"
typedef struct shaderparsestate_s parsestate_t;
static plugfsfuncs_t *fsfuncs;

static qboolean COD2_DecodeMaterial(parsestate_t *ps, const char *filename, void (*LoadMaterialString)(parsestate_t *ps, const char *script))
{
	size_t sz;
	qbyte *base = fsfuncs->LoadFile(va("materials/%s", filename), &sz);
	struct
	{
		unsigned int ofs_materialname;	//usually matches filename. dunno why this needs to be here. for aliases? does the qbsp read this instead?
		unsigned int ofs_texturename;	//simplification for tools? dunno.
		unsigned int z1;
		qbyte unk10;
		qbyte sort;	//sort:
					//0x00: distortion
					//0x01: opaquewater
					//0x02: boathull
					//0x03: opaque
					//0x04: sky
					//0x04: skybox_sunmoon
					//0x06: skybox_clouds
					//0x08: decal_bottom1
					//0x09: decal_bottom2
					//0x0a: decal_bottom3
					//0x0b: decal_world
					//0x0c: decal_middle1
					//0x0d: decal_middle2
					//0x0e: decal_middle3
					//0x0f: decal_gunimpact
					//0x10: decal_top1
					//0x11: decal_top2
					//0x12: decal_top3
					//0x12: decal_multiplicative
					//0x14: banner
					//0x15: hair
					//0x16: underwater
					//0x17: transparentwater
					//0x18: corona
					//0x19: windowinside
					//0x1a: windowoutside
					//0x1b: blend
					//0x1c: viewmodel
		qbyte unk12;
		qbyte unk13;
		unsigned int z2;
		unsigned short unk2[2];
		unsigned int unk3;
		unsigned short width, height;
		unsigned int z3;
		unsigned int surfaceflags;	//lower bits seem like they might be wrong. upper bits are consistentish with cod1 surfaceflags though.
		unsigned int contentbits;	//pure guess. probably wrong.
		unsigned int blendbits;		//0x00ff00ff bits are src/dst color/alpha blendfuncs matching the D3DBLEND enum.
		unsigned int unk7;
		unsigned int unk8;
		unsigned int ofs_program;
		unsigned int ofs_table; //0x44
		unsigned int ofs_table_end;

		//regarding the various unknowns, we can expect editor locale+usage (which are useless to us), blendfunc+alphafunc++cullface+depthtest+depthwrite+polygonoffset, maybe some tessellation settings and envmapping stuff.

		struct
		{
			unsigned int ofs_sampler;
			unsigned int flags; //maybe? 0x200 for rgb, 0x300 for normals, 0x400 for spec...? there's probably clamping options here.
								//&7==0: default
								//&7==1: nearest... or trilinear?!?
								//&7==2: linear
								//&7==3: ???
								//&7==4: ???
								//&7==5: ???
								//&7==6: bilinear
								//&7==7: anisotropic
			unsigned int ofs_texname;
		} maps[1];
	} *header = (void*)base;
	unsigned int m;
	if (!base)
		return false;	//nope, bad filename
	if (header->ofs_table == 0x44)	//make sure we know what it is...
	{
		size_t ofs = 0;
		char shad[8192];
		const char *srcfac, *dstfac;

		//no initial { cos we're weird.
		Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\n"),ofs+=strlen(shad+ofs);	//not actually useful to us. maybe for the hud so it doesn't have to wait for the image data too?
		Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\t//material='%s'\n\t//tooltex='%s'\n", base+header->ofs_materialname, base+header->ofs_texturename),ofs+=strlen(shad+ofs);
		Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\t//z1=%#x z2=%#x z3=%#x\n\t//unk1=%#x,%#x,%#x,%#x\n\t//unk2=%#x,%#x\n\t//unk3=%#x\n\t//surfaceflags=%#x\n\t//contentbits=%#x\n\t//unk6=%#x\n\t//unk7=%#x\n\t//unk8=%#x\n", header->z1, header->z2, header->z3, header->unk10,header->sort,header->unk12,header->unk13, header->unk2[0],header->unk2[1], header->unk3, header->surfaceflags, header->contentbits, header->blendbits, header->unk7, header->unk8),ofs+=strlen(shad+ofs);
		Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\timagesize %i %i\n", header->width, header->height),ofs+=strlen(shad+ofs);	//not actually useful to us. maybe for the hud so it doesn't have to wait for the image data too?

		for (m = 0; m < (header->ofs_table_end-header->ofs_table)/sizeof(header->maps[0]); m++)
		{
			const char *sampler = base+header->maps[m].ofs_sampler;
			if (!strcmp(sampler, "colorMap"))
				Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\tdiffusemap images/%s.iwi //%#x\n", base+header->maps[m].ofs_texname, header->maps[m].flags),ofs+=strlen(shad+ofs);
			else if (!strcmp(sampler, "normalMap"))
				Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\tnormalmap images/%s.iwi //%#x\n", base+header->maps[m].ofs_texname, header->maps[m].flags),ofs+=strlen(shad+ofs);
			else if (!strcmp(sampler, "detailMap"))
				Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\tdisplacementmap images/%s.iwi //%#x\n", base+header->maps[m].ofs_texname, header->maps[m].flags),ofs+=strlen(shad+ofs);	//hack. might as well use this one as detail.
			else if (!strcmp(sampler, "specularMap"))
				Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\tspecularmap images/%s.iwi //%#x\n", base+header->maps[m].ofs_texname, header->maps[m].flags),ofs+=strlen(shad+ofs);
			else
				Con_Printf("\t%s:%#x<-%s\n", base+header->maps[m].ofs_sampler, header->maps[m].flags, base+header->maps[m].ofs_texname);
		}

		//spit out a pass...
		Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\t{\n"),ofs+=strlen(shad+ofs);
		Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\t\tprogram %s\n", base+header->ofs_program),ofs+=strlen(shad+ofs);
		Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\t\tmap $diffuse\n"),ofs+=strlen(shad+ofs);	//just in case.

		switch((header->blendbits>>0)&0xf)
		{	//source factors
		default:
		case 1: srcfac="zero";	break;
		case 2: srcfac="one";	break;
		case 3: srcfac="src_color";	break;
		case 4: srcfac="one_minus_src_color";	break;
		case 5: srcfac="src_alpha";	break;
		case 6: srcfac="one_minus_src_alpha";	break;
		case 7: srcfac="dst_alpha";	break;
		case 8: srcfac="one_minus_dst_alpha";	break;
		case 9: srcfac="dst_color";	break;
		case 10: srcfac="one_minus_dst_color";	break;
		case 11: srcfac="src_alpha_sat";	break;
		case 12: srcfac="both_src_alpha";	break;
		case 13: srcfac="both_one_minus_src_alpha";	break;
		case 14: srcfac="blend_factor";	break;
		case 15: srcfac="one_minus_blend_factor";	break;
		}
		switch((header->blendbits>>4)&0xf)
		{	//dest factors
		default:
		case 1:  dstfac="zero";	break;
		case 2:  dstfac="one";	break;
		case 3:  dstfac="src_color";	break;
		case 4:  dstfac="one_minus_src_color";	break;
		case 5:  dstfac="src_alpha";	break;
		case 6:  dstfac="one_minus_src_alpha";	break;
		case 7:  dstfac="dst_alpha";	break;
		case 8:  dstfac="one_minus_dst_alpha";	break;
		case 9:  dstfac="dst_color";	break;
		case 10: dstfac="one_minus_dst_color";	break;
		case 11: dstfac="src_alpha_sat";	break;
		case 12: dstfac="both_src_alpha";	break;
		case 13: dstfac="both_one_minus_src_alpha";	break;
		case 14: dstfac="blend_factor";	break;
		case 15: dstfac="one_minus_blend_factor";	break;
		}

		Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\t\tblendfunc %s %s\n", srcfac, dstfac),ofs+=strlen(shad+ofs);	//just in case.

		Q_snprintf(shad+ofs,sizeof(shad)-ofs, "\t}\n"),ofs+=strlen(shad+ofs);

		Q_snprintf(shad+ofs,sizeof(shad)-ofs, "}\n");
		plugfuncs->Free(base);
		LoadMaterialString(ps, shad);
		return true;
	}
	else
		Con_Printf(CON_WARNING"%s doesn't seem to be a material? table=%#x..%#x\n", filename, header->ofs_table, header->ofs_table_end);
	plugfuncs->Free(base);
	return false;
}

static qboolean SType_LoadShader(parsestate_t *ps, const char *filename, void (*LoadMaterialString)(parsestate_t *ps, const char *script))
{
	char stypefname[MAX_QPATH];
	char *path;
	const char *sep;
	const char *at;
	size_t pre;
	char *base, *in, *out;

	sep = strrchr(filename, '/');
	if (!sep++)
		return COD2_DecodeMaterial(ps, filename, LoadMaterialString);
	at = strrchr(sep, '@');
	if (!at)
		return false;	//nope, no shadertype specified. depend on the engine/loader/etc defaults.

	if (!strncmp(filename, "skins/", 5))
		path = "shadertypes/model/";
	else if (!strncmp(filename, "textures/", 9))
		path = "shadertypes/world/";
	else if (!strncmp(filename, "gfx/", 4))
		path = "shadertypes/2d/";
	else
		return false;	//nope, not gonna try.

	pre = strlen(path);
	if (pre+(at-sep)+7 > sizeof(stypefname))
		return false;	//nope, too long...
	memcpy(stypefname, path, pre);
	memcpy(stypefname+pre, sep, at-sep);
	memcpy(stypefname+pre+(at-sep), ".stype", 7);

	in = out = base = fsfuncs->LoadFile(stypefname, &pre);
	if (!base)
		return false;	//nope, bad filename
	//yay we got something, but we need to fix up the $texturename strings to $diffuse because $reasons
	while (*in)
	{
		if (*in == '$' && !strncmp(in, "$texturename", 12))
		{
			memcpy(out, "$diffuse", 8);
			out += 8;
			in += 12;
		}
		else
			*out++ = *in++;
	}
	*out = 0;

	//now hand over the fteised shader script.
	in = base;
	while(*in == ' ' || *in == '\t' || *in == '\n' || *in == '\r')
		in++;
	if (*in == '{')
		in++;
	LoadMaterialString(ps, in);
	plugfuncs->Free(base);	//done with it now.
	return true;
}

/*static struct sbuiltin_s codprograms[] =
{
	{QR_NONE}
};*/
static plugmaterialloaderfuncs_t stypefuncs =
{
	"Cod Shader Types",
	SType_LoadShader,

//	codprograms,
};
qboolean STypes_Init(void)
{
	fsfuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*fsfuncs));
	if (!fsfuncs)
		return false;
	return plugfuncs->ExportInterface(plugmaterialloaderfuncs_name, &stypefuncs, sizeof(stypefuncs));
}
