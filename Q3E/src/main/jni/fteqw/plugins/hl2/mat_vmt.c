#include "../plugin.h"
#include "shader.h"
static plugfsfuncs_t *fsfuncs;

typedef struct shaderparsestate_s parsestate_t;

typedef struct
{
	char **savefile;
	char *sourcefile;
	char type[MAX_QPATH];
	char normalmap[MAX_QPATH];
	struct
	{
		char name[MAX_QPATH];
	} tex[5];
	char fullbrightmap[MAX_QPATH];
	char envmap[MAX_QPATH];
	char envmapmask[MAX_QPATH];
	char dudvmap[MAX_QPATH];
	char refracttinttexture[MAX_QPATH];
	char color[MAX_QPATH];
	char envfrombase;
	char envfromnorm;
	char halflambert;

	float envmaptint_r;
	float envmaptint_g;
	float envmaptint_b;
	float envmapsat_r;
	float envmapsat_g;
	float envmapsat_b;
	float alphatestref;
	char *blendfunc;
	qboolean alphatest;
	qboolean culldisable;
	qboolean ignorez;
	char *replaceblock;

	char vertexcolor;
	char vertexalpha;
	char nodraw;
	char additive;
	char translucent;
	char selfillum;
	char decal;
	char nofog;
	char mod2x; /* modulate only */
	char water_cheap; /* water only */
	char water_expensive; /* water only */
} vmtstate_t;


static void VARGS Q_strlcatfz (char *dest, size_t *offset, size_t size, const char *fmt, ...) LIKEPRINTF(4);
static void VARGS Q_strlcatfz (char *dest, size_t *offset, size_t size, const char *fmt, ...)
{
	va_list		argptr;

	dest += *offset;
	size -= *offset;

	va_start (argptr, fmt);
	Q_vsnprintfz(dest, size, fmt, argptr);
	va_end (argptr);
	*offset += strlen(dest);
}
static void Q_StrCat(char **ptr, const char *append)
{
	size_t oldlen = *ptr?strlen(*ptr):0;
	size_t newlen = strlen(append);
	char *newptr = plugfuncs->Malloc(oldlen+newlen+1);
	memcpy(newptr, *ptr, oldlen);
	memcpy(newptr+oldlen, append, newlen);
	newptr[oldlen+newlen] = 0;
	plugfuncs->Free(*ptr);
	*ptr = newptr;
}

//case comparisons are specific to ascii only, so this should be 'safe' for utf-8 strings too.
int Q_strncasecmp (const char *s1, const char *s2, int n)
{
	int		c1, c2;

	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
			{	// strings not equal
				if (c1 > c2)
					return 1;		// strings not equal
				return -1;
			}
		}
		if (!c1)
			return 0;		// strings are equal
//		s1++;
//		s2++;
	}

	return -1;
}
int Q_strcasecmp (const char *s1, const char *s2)
{
	return Q_strncasecmp (s1, s2, 0x7fffffff);
}

static qboolean VMT_ReadVMT(const char *materialname, vmtstate_t *st);	//this is made more complicated on account of includes allowing recursion
static char *VMT_ParseBlock(const char *fname, vmtstate_t *st, char *line)
{	//assumes the open { was already parsed, but will parse the close.
	char *replace = NULL;
	com_tokentype_t ttype;
	char key[MAX_OSPATH];
	char value[MAX_OSPATH];
	char *qmark;
	qboolean cond;
	for(;line;)
	{
		line = cmdfuncs->ParseToken(line, key, sizeof(key), &ttype);
		if (ttype == TTP_RAWTOKEN && !strcmp(key, "}"))
			break;	//end-of-block
		line = cmdfuncs->ParseToken(line, value, sizeof(value), &ttype);
		if (ttype == TTP_RAWTOKEN && !strcmp(value, "{"))
		{	//sub block. we don't go into details here.
			if (!Q_strcasecmp(key, "replace"))
				replace = line;
			else
				Con_DPrintf("%s: Unknown block \"%s\"\n", fname, key);
			line = VMT_ParseBlock(fname, NULL, line);
			continue;
		}

		while ((qmark = strchr(key, '?')))
		{
			*qmark++ = 0;
			if (!Q_strcasecmp(key, "srgb"))
				cond = false;//!!(vid.flags & VID_SRGBAWARE);
			else
			{
				Con_DPrintf("%s: Unknown vmt conditional \"%s\"\n", fname, key);
				cond = false;
			}
			if (!cond)
			{
				*key = 0;
				break;
			}
			else
				memmove(key, qmark, strlen(qmark)+1);
		}

		if (!*key || !st)
			;
		else if (!Q_strcasecmp(key, "include"))
		{
			if (!VMT_ReadVMT(value, st))
				return NULL;
		}
		else if (!Q_strcasecmp(key, "$basetexture") || !Q_strcasecmp(key, "$hdrbasetexture"))	//fixme: hdr version should probably override the other. order matters.
			Q_strlcpy(st->tex[0].name, value, sizeof(st->tex[0].name));
		else if (!Q_strcasecmp(key, "$hdrcompressedtexture"))	//named texture is R8G8B8E8 and needs to be decompressed manually... should probably just use e5bgr9 but we don't have a way to transcode it here.
			;
		else if (!Q_strcasecmp(key, "$basetexturetransform"))
			;
		else if (!Q_strcasecmp(key, "$bumpmap")) // same as normalmap ~eukara
			Q_strlcpy(st->normalmap, value, sizeof(st->normalmap));
		else if (!Q_strcasecmp(key, "$dudvmap")) // refractions only
		{
			Q_strlcpy(st->dudvmap, value, sizeof(st->dudvmap));
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		}
		else if (!Q_strcasecmp(key, "$ssbump"))
			;
		else if (!Q_strcasecmp(key, "$ssbumpmathfix"))
			;
		else if (!Q_strcasecmp(key, "$basetexture2") || !strcmp(key, "$texture2"))
			Q_strlcpy(st->tex[1].name, value, sizeof(st->tex[1].name));
		else if (!Q_strcasecmp(key, "$basetexturetransform2"))
			;
		else if (!Q_strcasecmp(key, "$surfaceprop"))
			;

		else if (!Q_strcasecmp(key, "$ignorez"))
			st->ignorez = !!atoi(value);
		else if (!Q_strcasecmp(key, "$nocull") && (!strcmp(value, "1")||!strcmp(value, "0")))
			st->culldisable = atoi(value);
		else if (!Q_strcasecmp(key, "$alphatest") && (!strcmp(value, "1")||!strcmp(value, "0")))
			st->alphatest = atoi(value);
		else if (!Q_strcasecmp(key, "$alphatestreference"))
			st->alphatestref = atof(value);
		else if (!Q_strcasecmp(key, "$alphafunc"))
		{
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		}
		else if (!Q_strcasecmp(key, "$alpha"))
		{
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		}
		else if (!Q_strcasecmp(key, "$translucent"))
		{
			st->translucent = 1;
		}
		else if (!Q_strcasecmp(key, "$additive"))
		{
			if (atoi(value))
				st->additive = 1;
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		}
		else if (!Q_strcasecmp(key, "$halflambert"))
			st->halflambert = 1;
		else if (!Q_strcasecmp(key, "%compiletrigger"))
			st->nodraw = 1;
		else if (!Q_strcasecmp(key, "lampbeam"))
			st->additive = 1;
		else if (!Q_strcasecmp(key, "$color"))
			Q_strlcpy(st->color, value, sizeof(st->color));
		else if (!Q_strcasecmp(key, "$vertexcolor"))
			st->vertexcolor = 1;
		else if (!Q_strcasecmp(key, "$vertexalpha"))
			st->vertexalpha = 1;
		else if (!Q_strcasecmp(key, "$decal"))
			st->decal = atoi(value);
		else if (!Q_strcasecmp(key, "$decalscale"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$decalsize"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$envmap"))
			Q_strlcpy(st->envmap, value, sizeof(st->envmap));
		else if (!Q_strcasecmp(key, "$envmapmask"))
			Q_strlcpy(st->envmapmask, value, sizeof(st->envmapmask));
		else if (!Q_strcasecmp(key, "$envmapcontrast"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$envmaptint"))
		{
			char tok[64];
			char *tintline;
			tintline = cmdfuncs->ParsePunctuation(value, "[", tok, sizeof(tok), 0);

			if (!strcmp(tok, "[")) {
				tintline = cmdfuncs->ParseToken(tintline, tok, sizeof(tok), 0);
				st->envmaptint_r = strtof(tok, NULL);
				tintline = cmdfuncs->ParseToken(tintline, tok, sizeof(tok), 0);
				st->envmaptint_g = strtof(tok, NULL);
				tintline = cmdfuncs->ParsePunctuation(tintline, "]", tok, sizeof(tok), 0);
				st->envmaptint_b = strtof(tok, NULL);
			} else {
				st->envmaptint_r = st->envmaptint_g = st->envmaptint_b = atof(value);
			}
		}
		else if (!Q_strcasecmp(key, "$envmapsaturation"))
		{
			char tok[64];
			char *tintline;
			tintline = cmdfuncs->ParsePunctuation(value, "[", tok, sizeof(tok), 0);

			if (!strcmp(tok, "[")) {
				tintline = cmdfuncs->ParseToken(tintline, tok, sizeof(tok), 0);
				st->envmapsat_r = strtof(tok, NULL);
				tintline = cmdfuncs->ParseToken(tintline, tok, sizeof(tok), 0);
				st->envmapsat_g = strtof(tok, NULL);
				tintline = cmdfuncs->ParsePunctuation(tintline, "]", tok, sizeof(tok), 0);
				st->envmapsat_b = strtof(tok, NULL);
			} else {
				st->envmapsat_r = st->envmapsat_g = st->envmapsat_b = atof(value);
			}
		}
		else if (!Q_strcasecmp(key, "$basealphaenvmapmask"))
			st->envfrombase=1;
		else if (!Q_strcasecmp(key, "$normalmapalphaenvmapmask"))
			st->envfromnorm=1;
		else if (!Q_strcasecmp(key, "$crackmaterial"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$selfillum"))
			st->selfillum = 1;
		else if (!Q_strcasecmp(key, "$selfillummask"))
			Q_strlcpy(st->fullbrightmap, value, sizeof(st->fullbrightmap));
		else if (!Q_strcasecmp(key, "$selfillumtint"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$nofog"))
			st->nofog = 1;
		else if (!Q_strcasecmp(key, "$nomip"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$nodecal"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$detail"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$detailscale"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$detailtint"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$detailblendfactor"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$detailblendmode"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);

		else if (!Q_strcasecmp(key, "$surfaceprop2"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$AllowAlphaToCoverage"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$blendmodulatetexture"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);

		//water/reflection stuff
		else if (!Q_strcasecmp(key, "$REFRACTTINTTEXTURE"))
			Q_strlcpy(st->refracttinttexture, value, sizeof(st->refracttinttexture));
		else if (!Q_strcasecmp(key, "$refracttexture"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$refractamount"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$refracttint"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$reflecttexture"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$reflectamount"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$reflecttint"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$fresnelpower"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$minreflectivity"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$maxreflectivity"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$mod2x"))
			st->mod2x = 1;
		else if (!Q_strcasecmp(key, "$forcecheap"))
			st->water_cheap = 1;
		else if (!Q_strcasecmp(key, "$forceexpensive"))
			st->water_expensive = 1;
		else if (!Q_strcasecmp(key, "$normalmap"))
		{
			Q_strlcpy(st->normalmap, value, sizeof(st->normalmap));
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		}
		else if (!Q_strcasecmp(key, "$bumpframe"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$fogenable"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$fogcolor"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$fogstart"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$fogend"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$abovewater"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$underwateroverlay"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$reflectentities"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$scale"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$bottommaterial"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$scroll1"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);
		else if (!Q_strcasecmp(key, "$scroll2"))
			Con_DPrintf("%s: %s \"%s\"\n", fname, key, value);

		else if (*key == '%')
			;	//editor lines
		else
			Con_DPrintf("%s: Unknown field \"%s\"\n", fname, key);
	}
	if (replace)
		VMT_ParseBlock(fname, st, replace);
	return line;
}
static void Shader_GenerateFromVMT(parsestate_t *ps, vmtstate_t *st, const char *shortname, void (*LoadMaterialString)(parsestate_t *ps, const char *script))
{
	size_t offset = 0;
	char script[8192];
	char envmaptint[128];
	char envmapsat[128];
	char *progargs = "";

	if (!*st->tex[0].name)	//fill in a default...
		Q_strlcpy(st->tex[0].name, shortname, sizeof(st->tex[0].name));

	if (st->alphatest)
		progargs = "#MASK=0.5#MASKLT";	//alphamask has to be handled by glsl (when glsl is used)

	if (st->nofog)
		progargs = "#NOFOG";

	/* FIXME: check proper */
	if (st->envmaptint_b > 0.0f) {
		Q_snprintfz(envmaptint, sizeof(envmaptint), "#ENVTINT=%f,%f,%f", st->envmaptint_r, st->envmaptint_g, st->envmaptint_b);
	} else {
		Q_snprintfz(envmaptint, sizeof(envmaptint), "");
	}

	/* FIXME: check proper */
	if (st->envmapsat_r > 0.0f) {
		Q_snprintfz(envmapsat, sizeof(envmapsat), "#ENVSAT=%f,%f,%f", st->envmapsat_r, st->envmapsat_g, st->envmapsat_b);
	} else {
		Q_snprintfz(envmapsat, sizeof(envmapsat), "");
	}

	Q_strlcatfz(script, &offset, sizeof(script), "\n");

	if (st->nodraw)
	{
		Q_strlcatfz(script, &offset, sizeof(script), "\tsurfaceparm nodraw\n");
	}
	else if (!Q_strcasecmp(st->type, "UnlitGeneric"))
	{
		Q_strlcatfz(script, &offset, sizeof(script), "{\n");
		Q_strlcatfz(script, &offset, sizeof(script),	"\tmap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);

		if (st->vertexcolor)
			Q_strlcatfz(script, &offset, sizeof(script),	"\rrgbGen vertex\n");
		if (st->vertexalpha)
			Q_strlcatfz(script, &offset, sizeof(script),	"\ralphaGen vertex\n");

		if (st->additive)
			Q_strlcatfz(script, &offset, sizeof(script),	"\tblendFunc add\n");
		else if (st->translucent)
			Q_strlcatfz(script, &offset, sizeof(script),	"\tblendFunc blend\n");
		else if (st->alphatest)
			Q_strlcatfz(script, &offset, sizeof(script),	"\talphaFunc GE128\n");


		Q_strlcatfz(script, &offset, sizeof(script), "}\n");
	}
	else if (!Q_strcasecmp(st->type, "WorldVertexTransition"))
	{
		if (*st->envmap && st->envfrombase)
			Q_strlcpy(st->type, "vmt/transition#ENVFROMBASE", sizeof(st->type));
		else if (*st->envmap && st->envfromnorm)
			Q_strlcpy(st->type, "vmt/transition#ENVFROMNORM", sizeof(st->type));
		else if (*st->envmap && *st->envmapmask) /* dedicated reflectmask */
			Q_strlcpy(st->type, "vmt/transition#ENVFROMMASK", sizeof(st->type));
		else /* take from normalmap */
			Q_strlcpy(st->type, "vmt/transition", sizeof(st->type));

		Q_strlcatfz(script, &offset, sizeof(script),	"\tprogram \"%s%s\"\n", st->type, progargs);
		Q_strlcatfz(script, &offset, sizeof(script),	"\tdiffusemap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);
		Q_strlcatfz(script, &offset, sizeof(script),	"\tuppermap \"%s%s.vtf\"\n", strcmp(st->tex[1].name, "materials/")?"materials/":"", st->tex[1].name);

		/* there's also bumpmap2, but rtlight glsl doesn't respect it anyway. */
		if (*st->normalmap)
			Q_strlcatfz(script, &offset, sizeof(script),	"\tnormalmap \"%s%s.vtf\"\n", strcmp(st->normalmap, "materials/")?"materials/":"", st->normalmap);
	}
	else if (!Q_strcasecmp(st->type, "UnlitTwoTexture"))
	{

		Q_strlcatfz(script, &offset, sizeof(script), "{\n");
		Q_strlcatfz(script, &offset, sizeof(script),	"\tmap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);

		if (st->mod2x) {
			Q_strlcatfz(script, &offset, sizeof(script),"\t\tblendFunc gl_dst_color gl_src_color\n");
		} else if (st->additive) {
			Q_strlcatfz(script, &offset, sizeof(script),"\t\tblendFunc add\n");
		}

		Q_strlcatfz(script, &offset, sizeof(script), "}\n");
	}
	else if (!Q_strcasecmp(st->type, "Sprite"))
	{
		Q_strlcatfz(script, &offset, sizeof(script), "{\n");
		Q_strlcatfz(script, &offset, sizeof(script),	"\tmap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);
		Q_strlcatfz(script, &offset, sizeof(script),	"\rrgbGen vertex\n");
		Q_strlcatfz(script, &offset, sizeof(script),	"\tblendFunc add\n");
		Q_strlcatfz(script, &offset, sizeof(script), "}\n");
	}
	else if (!Q_strcasecmp(st->type, "Decal"))
	{
		Q_strlcpy(st->type, "vmt/vertexlit", sizeof(st->type));
		Q_strlcatfz(script, &offset, sizeof(script),	"\tprogram \"%s%s\"\n", st->type, progargs);
		Q_strlcatfz(script, &offset, sizeof(script),	"\tdiffusemap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);
		Q_strlcatfz(script, &offset, sizeof(script),	"\tpolygonOffset 1\n");
		st->decal = 0;
	}
	else if (!Q_strcasecmp(st->type, "DecalModulate"))
	{
		Q_strlcatfz(script, &offset, sizeof(script),	"\tpolygonOffset 1\n");
		Q_strlcatfz(script, &offset, sizeof(script),	"\t{\n");
		Q_strlcatfz(script, &offset, sizeof(script),	"\t\tmap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);

		if (!st->mod2x) {
			Q_strlcatfz(script, &offset, sizeof(script),"\t\tblendFunc gl_dst_color gl_src_color\n");
		} else {
			Q_strlcatfz(script, &offset, sizeof(script),"\t\tblendFunc gl_dst_color gl_one_minus_src_alpha\n");
		}

		Q_strlcatfz(script, &offset, sizeof(script),	"\t}\n");
		st->decal = 0;
		st->translucent = 0;
	}
	else if (!Q_strcasecmp(st->type, "Modulate"))
	{

		Q_strlcatfz(script, &offset, sizeof(script),	"\t{\n");
		Q_strlcatfz(script, &offset, sizeof(script),	"\t\tmap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);

		if (!st->mod2x) {
			Q_strlcatfz(script, &offset, sizeof(script),"\t\tblendFunc gl_dst_color gl_src_color\n");
		} else {
			Q_strlcatfz(script, &offset, sizeof(script),"\t\tblendFunc gl_dst_color gl_one_minus_src_alpha\n");
		}
		Q_strlcatfz(script, &offset, sizeof(script),	"\trgbGen vertex\n");

		Q_strlcatfz(script, &offset, sizeof(script),	"\t}\n");
		st->translucent = 0;
	}
	else if (!Q_strcasecmp(st->type, "Water"))
	{
		if (st->water_cheap)
			progargs = "#LQWATER";
		if (st->water_expensive)
			progargs = "#HQWATER";

		Q_strlcatfz(script, &offset, sizeof(script),
			"\t{\n"
				"\t\tprogram \"vmt/water%s\"\n"
				"\t\tmap $refraction\n"
				"\t\tmap $reflection\n"
			"\t}\n", progargs);
		Q_strlcatfz(script, &offset, sizeof(script),	"\tdiffusemap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);
		Q_strlcatfz(script, &offset, sizeof(script),	"\tnormalmap \"%s%s.vtf\"\n", strcmp(st->normalmap, "materials/")?"materials/":"", st->normalmap);
		Q_strlcatfz(script, &offset, sizeof(script),	"\tsurfaceparm nodlight\n");
		Q_strlcatfz(script, &offset, sizeof(script),	"\tsurfaceparm trans\n");
		Q_strlcatfz(script, &offset, sizeof(script),	"\tsurfaceparm alphashadow\n");
	}
	else if (!Q_strcasecmp(st->type, "Refract"))
	{
		Q_strlcatfz(script, &offset, sizeof(script),
			"\t{\n"
				"\t\tprogram \"vmt/refract%s\"\n"
				"\t\tmap $currentrender\n"
				"\t\tmap \"%s%s.vtf\"\n"
			"\t}\n", progargs, strcmp(st->normalmap, "materials/")?"materials/":"", st->normalmap);

		if (*st->refracttinttexture)
			Q_strlcatfz(script, &offset, sizeof(script),	"\tdiffusemap \"%s%s.vtf\"\n", strcmp(st->refracttinttexture, "materials/")?"materials/":"", st->refracttinttexture);

		Q_strlcatfz(script, &offset, sizeof(script),	"\tnormalmap \"%s%s.vtf\"\n", strcmp(st->normalmap, "materials/")?"materials/":"", st->normalmap);
	}
	else if (!Q_strcasecmp(st->type, "VertexlitGeneric") || st->decal)
	{

			if (*st->envmap && st->envfrombase)
			{
				if (st->halflambert)
					Q_strlcpy(st->type, "vmt/vertexlit#ENVFROMBASE#HALFLAMBERT", sizeof(st->type));
				else
					Q_strlcpy(st->type, "vmt/vertexlit#ENVFROMBASE", sizeof(st->type));
			}
			else if (*st->envmap && st->envfromnorm)
			{
				if (st->halflambert)
					Q_strlcpy(st->type, "vmt/vertexlit#ENVFROMNORM#HALFLAMBERT", sizeof(st->type));
				else
					Q_strlcpy(st->type, "vmt/vertexlit#ENVFROMNORM", sizeof(st->type));
			}
			else
			{
				if (st->halflambert)
					Q_strlcpy(st->type, "vmt/vertexlit#HALFLAMBERT", sizeof(st->type));
				else
					Q_strlcpy(st->type, "vmt/vertexlit", sizeof(st->type));
			}

			Q_strlcatfz(script, &offset, sizeof(script),	"\tprogram \"%s%s%s%s\"\n", st->type, progargs, envmaptint, envmapsat);


		Q_strlcatfz(script, &offset, sizeof(script),	"\tdiffusemap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);

		if (*st->normalmap) {
			Q_strlcatfz(script, &offset, sizeof(script),	"\tnormalmap \"%s%s.vtf\"\n", strcmp(st->normalmap, "materials/")?"materials/":"", st->normalmap);
		}
#if 0
		if (st->additive)
			st->blendfunc = "src_one dst_one";
		else if (st->translucent)
			st->blendfunc = "src_alpha one_minus_src_alpha";
#endif

		Q_strlcatfz(script, &offset, sizeof(script),	"\treflectcube $cube:materials/skybox/sky_day03_06\n");

	}
	else if (!Q_strcasecmp(st->type, "LightmappedGeneric"))
	{
		/* reflectmask from diffuse map alpha */

			if (*st->envmap && st->envfrombase)
				Q_strlcpy(st->type, "vmt/lightmapped#ENVFROMBASE", sizeof(st->type));
			else if (*st->envmap && st->envfromnorm)
				Q_strlcpy(st->type, "vmt/lightmapped#ENVFROMNORM", sizeof(st->type));
			else if (*st->envmap && *st->envmapmask) /* dedicated reflectmask */
				Q_strlcpy(st->type, "vmt/lightmapped#ENVFROMMASK", sizeof(st->type));
			else /* take from normalmap */
				Q_strlcpy(st->type, "vmt/lightmapped", sizeof(st->type));

			Q_strlcatfz(script, &offset, sizeof(script),	"\tprogram \"%s%s%s%s\"\n", st->type, progargs, envmaptint, envmapsat);


		Q_strlcatfz(script, &offset, sizeof(script),	"\tdiffusemap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);

		if (*st->normalmap)
			Q_strlcatfz(script, &offset, sizeof(script),	"\tnormalmap \"%s%s.vtf\"\n", strcmp(st->normalmap, "materials/")?"materials/":"", st->normalmap);
	}
	else
	{
		/* render-target camera/monitor - eukara*/
		if (!Q_strcasecmp(st->tex[0].name, "_rt_Camera"))
			Q_strlcatfz(script, &offset, sizeof(script),
			"\t{\n"
				"\t\tprogram vmt/rt\n"
				"\t\tmap $rt:base\n"
			"\t}\n"/*, progargs*/);
		else
		{
			/* the default should just be unlit, let's not make any assumptions - eukara*/
			Q_strlcpy(st->type, "vmt/unlit", sizeof(st->type));
			Q_strlcatfz(script, &offset, sizeof(script),	"\tprogram \"%s%s\"\n", st->type, progargs);
			Q_strlcatfz(script, &offset, sizeof(script),	"\tdiffusemap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);
		}
	}

	if (st->translucent) {
		st->blendfunc = "src_alpha one_minus_src_alpha";
	}

	if (st->decal) {
		Q_strlcatfz(script, &offset, sizeof(script),	"\tpolygonOffset 1\n");
		Q_strlcatfz(script, &offset, sizeof(script),	"\trgbGen vertex\n");
	}

	if (*st->fullbrightmap)
		Q_strlcatfz(script, &offset, sizeof(script), "\tfullbrightmap \"%s%s.vtf\"\n", strcmp(st->fullbrightmap, "materials/")?"materials/":"", st->fullbrightmap);
	else if (st->selfillum == 1)
		Q_strlcatfz(script, &offset, sizeof(script), "\tfullbrightmap \"%s%s.vtf\"\n", strcmp(st->tex[0].name, "materials/")?"materials/":"", st->tex[0].name);

	if (*st->envmapmask)
		Q_strlcatfz(script, &offset, sizeof(script),	"\treflectmask \"%s%s.vtf\"\n", strcmp(st->envmapmask, "materials/")?"materials/":"", st->envmapmask);
	if (*st->envmap && strcmp(st->envmap, "env_cubemap"))
		Q_strlcatfz(script, &offset, sizeof(script),	"\treflectcube \"%s%s.vtf\"\n", strcmp(st->envmap, "materials/")?"materials/":"", st->envmap);
	if (st->alphatest)
		Q_strlcatfz(script, &offset, sizeof(script), "\talphatest ge128\n");
	if (st->culldisable)
		Q_strlcatfz(script, &offset, sizeof(script), "\tcull disable\n");
	if (st->ignorez)
		Q_strlcatfz(script, &offset, sizeof(script), "\tnodepth\n");
	if (st->blendfunc)
		Q_strlcatfz(script, &offset, sizeof(script), "\tprogblendfunc %s\n", st->blendfunc);
	if (*st->color)
		Q_strlcatfz(script, &offset, sizeof(script), "\trgbGen const %s\n", st->color);

	Q_strlcatfz(script, &offset, sizeof(script), "}\n");

	LoadMaterialString(ps, script);

	if (st->sourcefile)
	{	//cat the original file on there...
		if (st->savefile)
		{
			char *winsucks;	//strip any '\r' chars in there that like to show as ugly glyphs.
			for (winsucks = st->sourcefile; *winsucks; winsucks++)
				if (*winsucks=='\r')
					*winsucks = ' ';

			Q_StrCat(st->savefile, "\n/*\n");
			Q_StrCat(st->savefile, st->sourcefile);
			Q_StrCat(st->savefile, "*/");
		}
		plugfuncs->Free(st->sourcefile);
	}
}
static qboolean VMT_ReadVMT(const char *fname, vmtstate_t *st)
{
	char *line, *file = NULL;
	com_tokentype_t ttype;
	char token[MAX_QPATH*2];
	char *prefix="", *postfix="";

	if (strstr(fname, "://"))
		return false;	//don't try to handle urls.

	//don't dupe the mandatory materials/ prefix
	if (strncmp(fname, "materials/", 10))
		prefix = "materials/";
	if (strcmp(fsfuncs->GetExtension(fname, NULL), ".vmt"))
		postfix = ".vmt";
	Q_snprintfz(token, sizeof(token), "%s%s%s", prefix, fname, postfix);

	file = fsfuncs->LoadFile(token, NULL);
	if (file)
	{
		if (st->savefile)
		{
			if (st->sourcefile)
			{
				Q_StrCat(&st->sourcefile, fname);
				Q_StrCat(&st->sourcefile, ":\n");
				Q_StrCat(&st->sourcefile, file);
			}
			else
				Q_StrCat(&st->sourcefile, file);
		}

		line = file;
		line = cmdfuncs->ParseToken(line, st->type, sizeof(st->type), &ttype);
		line = cmdfuncs->ParseToken(line, token, sizeof(token), &ttype);
		if (!strcmp(token, "{"))
		{
			line = VMT_ParseBlock(fname, st, line);
		}

		plugfuncs->Free(file);
		return !!line;
	}
	return false;
}
static qboolean Shader_LoadVMT(parsestate_t *ps, const char *filename, void (*LoadMaterialString)(parsestate_t *ps, const char *script))
{
	vmtstate_t st;
	memset(&st, 0, sizeof(st));
	st.savefile = NULL;//ps->saveshaderbody;
	if (!VMT_ReadVMT(filename, &st))
	{
		if (st.sourcefile)
			plugfuncs->Free(st.sourcefile);
		return false;
	}

	Shader_GenerateFromVMT(ps, &st, filename, LoadMaterialString);
	return true;
}

static struct sbuiltin_s vmtprograms[] =
{
//we don't know what renderer the engine will need...
#ifdef FTEPLUGIN
	#ifndef GLQUAKE
		#define GLQUAKE
	#endif
	#ifndef VKQUAKE
		#define VKQUAKE
	#endif
	#ifndef D3DQUAKE
		#define D3DQUAKE
	#endif
#endif
#include "mat_vmt_progs.h"
	{QR_NONE}
};
static plugmaterialloaderfuncs_t vmtfuncs =
{
	"HL2 VMT",
	Shader_LoadVMT,

	vmtprograms,
};

qboolean VMT_Init(void)
{
	fsfuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*fsfuncs));
	if (!fsfuncs)
		return false;
	return plugfuncs->ExportInterface(plugmaterialloaderfuncs_name, &vmtfuncs, sizeof(vmtfuncs));
}
