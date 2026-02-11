/*
Copyright (C) 2002-2003 Victor Luchits

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
// r_shader.c - based on code by Stephen C. Taylor
// Ported to FTE from qfusion, there are numerous changes since then.


#include "quakedef.h"
#ifndef SERVERONLY
#include "glquake.h"
#ifdef VKQUAKE
#include "../vk/vkrenderer.h"
#endif
#include "shader.h"

#include "hash.h"



#include <ctype.h>

extern texid_t missing_texture;
texid_t r_whiteimage, r_blackimage;
qboolean shader_reload_needed;
static qboolean shader_rescan_needed;

sh_config_t sh_config;

//cvars that affect shader generation
cvar_t r_vertexlight = CVARFD("r_vertexlight", "0", CVAR_SHADERSYSTEM, "Hack loaded shaders to remove detail pass and lightmap sampling for faster rendering.");
cvar_t r_forceprogramify = CVARAFD("r_forceprogramify", "0", "dpcompat_makeshitup", CVAR_SHADERSYSTEM, "Reduce the shader to a single texture, and then make stuff up about its mother. The resulting fist fight results in more colour when you shine a light upon its face.\nSet to 2 to ignore 'depthfunc equal' and 'tcmod scale' in order to tolerate bizzare shaders made for a bizzare engine.\nBecause most shaders made for DP are by people who _clearly_ have no idea what the heck they're doing, you'll typically need the '2' setting.");
#ifdef HAVE_LEGACY
cvar_t dpcompat_nopremulpics = CVARFD("dpcompat_nopremulpics", "0", CVAR_SHADERSYSTEM, "By default FTE uses premultiplied alpha for hud/2d images, while DP does not (which results in halos with low-res content). Unfortunately DDS files would need to be recompressed, resulting in visible issues.");
#endif
cvar_t r_glsl_precache = CVARFD("r_glsl_precache", "0", CVAR_SHADERSYSTEM, "Force all relevant glsl permutations to load upfront.");
cvar_t r_halfrate = CVARFD("r_halfrate", "0", CVAR_ARCHIVE|CVAR_SHADERSYSTEM, "Use half-rate shading (where supported by gpu).");

extern cvar_t r_glsl_offsetmapping_reliefmapping;
extern cvar_t r_drawflat;
extern cvar_t r_shaderblobs;
extern cvar_t r_tessellation;
extern cvar_t gl_compress;

//backend fills this in to say the max fixed-function pass count (often 1, where its emulated by us, because we're too lazy).
int be_maxpasses;


#define Q_stricmp stricmp
#define Q_strnicmp strnicmp
#define clamp(v,min, max) (v) = (((v)<(min))?(min):(((v)>(max))?(max):(v)));

typedef union {
	float			f;
	unsigned int	i;
} float_int_t;
qbyte FloatToByte( float x )
{
	static float_int_t f2i;

	// shift float to have 8bit fraction at base of number
	f2i.f = x + 32768.0f;

	// then read as integer and kill float bits...
	return (qbyte) min(f2i.i & 0x7FFFFF, 255);
}



cvar_t r_detailtextures;

#define MAX_TOKEN_CHARS sizeof(com_token)

char *COM_ParseExt (const char **data_p, qboolean nl, qboolean comma)
{
	int		c;
	int		len;
	const char	*data;

	COM_AssertMainThread("COM_ParseExt");

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		*data_p = NULL;
		return "";
	}

// skip whitespace
skipwhite:
	while ((c = (unsigned char)*data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return "";
		}
		if (c == '\n' && !nl)
		{
			*data_p = data;
			return com_token;
		}
		data++;
	}

// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
// skip /* comments
	if (c == '/' && data[1] == '*')
	{
		const char *start = data;
		data+=2;
		for(;data[0];)
		{
			if (data[0] == '*' && data[1] == '/')
			{
				data+=2;
				break;
			}
			if (*data == '\n' && !nl)
			{
				*data_p = start;
				return com_token;
			}
			data++;
		}
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if (c == ',' && len && comma)
			break;
	} while (c>32);

	if (len == MAX_TOKEN_CHARS)
	{
		Con_DPrintf ("Token exceeded %i chars, discarded.\n", (int)MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}

static float Com_FloatArgument(const char *shadername, char *arg, size_t arglen, float def)
{
	const char *var;

	//grab an argument instead, otherwise 0
	var = shadername+1;
	if (*shadername)
	while((var = strchr(var, '#')))
	{
		var++;
		if (!strnicmp(var, arg, arglen))
		{
			if (var[arglen] == '=')
				return strtod(var+arglen+1, NULL);
			if (var[arglen] == '#' || !var[arglen])
				return 1;	//present, but no value
		}
		var++;
	}
	return def;	//not present.
}
#define Shader_FloatArgument(s,k) (Com_FloatArgument(s->name,k,strlen(k),0))





#define HASH_SIZE	128

#define SPF_DEFAULT		0u	/*quake3/fte internal*/
#define SPF_PROGRAMIFY	(1u<<0)	/*automatically replace known glsl, pulling in additional textures+effects from a single primary pass*/
#define SPF_DOOM3		(1u<<1)	/*any commands, args, etc, should be interpretted according to doom3's norms*/

typedef struct shaderparsestate_s
{
	shader_t *s;		//the shader we're parsing
	shaderpass_t *pass;	//the pass we're currently parsing
	const char *ptr;	//the src file pointer we're at
	char *sourcename;	//the name of the shader file being read (or '<code>')

	const char *forcedshader;
	unsigned int parseflags;	//SPF_*
	qboolean droppass;
	unsigned int oldflags;	//shader flags to revert to if the pass is dropped.

	//for dpwater compat, used to generate a program
	int dpwatertype;
	float reflectmin;
	float reflectmax;
	float reflectfactor;
	float refractfactor;
	vec3_t refractcolour;
	vec3_t reflectcolour;
	float wateralpha;

	char **saveshaderbody;

	//FIXME: rtlights can't respond to these
	int offsetmappingmode;
	float offsetmappingscale;
	float offsetmappingbias;
	float specularexpscale; //*32 ish
	float specularvalscale; //*1 ish
} parsestate_t;

typedef struct shaderkey_s
{
	char			*keyword;
	void			(*func)( parsestate_t *ps, const char **ptr );
	char			*prefix;
} shaderkey_t;
typedef struct shadercachefile_s {
	char *data;
	size_t length;
	unsigned int parseflags;
	char forcedshadername[64];
	struct shadercachefile_s *next;
	char name[1];
} shadercachefile_t;
typedef struct shadercache_s {
	shadercachefile_t *source;
	size_t offset;
	struct shadercache_s *hash_next;
	char name[1];
} shadercache_t;

static shadercachefile_t *shaderfiles;	//contents of a .shader file
static shadercache_t **shader_hash;		//locations of known inactive shaders.

unsigned int r_numshaders;	//number of active slots in r_shaders array.
unsigned int r_maxshaders;	//max length of r_shaders array. resized if exceeded.
shader_t	**r_shaders;	//list of active shaders for a id->shader lookup
static hashtable_t shader_active_hash;	//list of active shaders for a name->shader lookup
void *shader_active_hash_mem;

//static char		r_skyboxname[MAX_QPATH];
//static float	r_skyheight;

static const char *Shader_Skip(const char *file, const char *shadername, const char *ptr);
static qboolean Shader_Parsetok(parsestate_t *ps, shaderkey_t *keys, const char *token);
static void Shader_ParseFunc(parsestate_t *ps, const char *functype, const char **args, shaderfunc_t *func);
static void Shader_MakeCache(const char *path, unsigned int parseflags);
static qboolean Shader_LocateSource(const char *name, const char **buf, size_t *bufsize, size_t *offset, shadercachefile_t **sourcefile);
static void Shader_ReadShader(parsestate_t *ps, const char *shadersource, shadercachefile_t *sourcefile);
static qboolean Shader_ParseShader(parsestate_t *ps, const char *parsename);


static struct
{
	void *module;
	plugmaterialloaderfuncs_t *funcs;
} *materialloader;
static size_t		materialloader_count;
qboolean Material_RegisterLoader(void *module, plugmaterialloaderfuncs_t *driver)
{
	int i;
	if (!driver)
	{
		for (i = 0; i < materialloader_count; )
		{
			if (materialloader[i].module == module)
			{
				memmove(&materialloader[i], &materialloader[i+1], materialloader_count-(i+1));
				materialloader_count--;
			}
			else
				i++;
		}
		return true;
	}
	else
	{
		void *n = BZ_Malloc(sizeof(*materialloader)*(materialloader_count+1));
		memcpy(n, materialloader, sizeof(*materialloader)*materialloader_count);
		Z_Free(materialloader);
		materialloader = n;
		materialloader[materialloader_count].module = module;
		materialloader[materialloader_count].funcs = driver;
		materialloader_count++;
		return true;
	}
}

//===========================================================================

static qboolean Shader_EvaluateCondition(shader_t *shader, const char **ptr)
{
	char *token;
	cvar_t *cv;
	float lhs;
	qboolean conditiontrue = true;
	token = COM_ParseExt(ptr, false, false);
	if (*token == '!')
	{
		conditiontrue = false;
		token++;
		if (!*token)
			token = COM_ParseExt(ptr, false, false);
	}

	if (*token >= '0' && *token <= '9')
		lhs = strtod(token, NULL);
	else
	{
		if (*token == '$')
			token++;

		if (*token == '#')
			lhs = !!Shader_FloatArgument(shader, token+1);
		else if (!Q_stricmp(token, "lpp"))
			lhs = r_lightprepass;
		else if (!Q_stricmp(token, "lightmap"))
			lhs = !r_fullbright.value;
		else if (!Q_stricmp(token, "deluxmap") || !Q_stricmp(token, "deluxe"))
			lhs = r_deluxemapping;
		else if (!Q_stricmp(token, "softwarebanding"))
			lhs = r_softwarebanding;
		else if (!Q_stricmp(token, "unmaskedsky"))
			lhs = cls.allow_unmaskedskyboxes;	//can/should skip writing depth values for sky surfaces.

		//normalmaps are generated if they're not already known.
		else if (!Q_stricmp(token, "normalmap"))
			lhs = r_loadbumpmapping;

		else if (!Q_stricmp(token, "vulkan"))
			lhs = (qrenderer == QR_VULKAN);
		else if (!Q_stricmp(token, "opengl"))
			lhs = (qrenderer == QR_OPENGL);
		else if (!Q_stricmp(token, "d3d8"))
			lhs = (qrenderer == QR_DIRECT3D8);
		else if (!Q_stricmp(token, "d3d9"))
			lhs = (qrenderer == QR_DIRECT3D9);
		else if (!Q_stricmp(token, "d3d11"))
			lhs = (qrenderer == QR_DIRECT3D11);
		else if (!Q_stricmp(token, "gles"))
		{
#ifdef GLQUAKE
			lhs = ((qrenderer == QR_OPENGL) && gl_config.gles);
#else
			lhs = false;
#endif
		}
		else if (!Q_stricmp(token, "nofixed"))
			lhs = sh_config.progs_required;
		else if (!Q_stricmp(token, "glsl"))
			lhs = ((qrenderer == QR_OPENGL) && sh_config.progs_supported);
		else if (!Q_stricmp(token, "hlsl"))
			lhs = ((qrenderer == QR_DIRECT3D9 || qrenderer == QR_DIRECT3D11) && sh_config.progs_supported);	
		else if (!Q_stricmp(token, "haveprogram"))
			lhs = !!shader->prog;
		else if (!Q_stricmp(token, "programs"))
			lhs = sh_config.progs_supported;
		else if (!Q_stricmp(token, "diffuse"))
			lhs = true;
		else if (!Q_stricmp(token, "specular"))
			lhs = false;
		else if (!Q_stricmp(token, "fullbright"))
			lhs = false;
		else if (!Q_stricmp(token, "topoverlay"))
			lhs = false;
		else if (!Q_stricmp(token, "loweroverlay"))
			lhs = false;

		//these are for compat/documentation purposes with qfusion/warsow
		else if (!Q_stricmp(token, "maxTextureSize"))
			lhs = sh_config.texture2d_maxsize;
		else if (!Q_stricmp(token, "maxTextureCubemapSize"))
			lhs = sh_config.texturecube_maxsize;
		else if (!Q_stricmp(token, "maxTextureUnits"))
			lhs = 0;
		else if (!Q_stricmp(token, "textureCubeMap"))
			lhs = sh_config.havecubemaps;
//		else if (!Q_stricmp(token, "GLSL"))
//			lhs = 1;
		else if (!Q_stricmp(token, "deluxeMaps") || !Q_stricmp(token, "deluxe"))
			lhs = r_deluxemapping;
		else if (!Q_stricmp(token, "portalMaps"))
			lhs = false;
		//end qfusion

		else
		{
			cv = Cvar_Get(token, "", 0, "Shader Conditions");
			if (cv)
			{
				cv->flags |= CVAR_SHADERSYSTEM;
				lhs = cv->value;
			}
			else
			{
				Con_Printf("Shader_EvaluateCondition: '%s' is not a cvar\n", token);
				lhs = 0;
			}
		}
	}
	if (*token)
		token = COM_ParseExt(ptr, false, false);
	if (*token)
	{
		float rhs;
		char cmp[4];
		memcpy(cmp, token, 4);
		token = COM_ParseExt(ptr, false, false);
		rhs = atof(token);
		if (!strcmp(cmp, "!="))
			conditiontrue = lhs != rhs;
		else if (!strcmp(cmp, "=="))
			conditiontrue = lhs == rhs;
		else if (!strcmp(cmp, "<"))
			conditiontrue = lhs < rhs;
		else if (!strcmp(cmp, "<="))
			conditiontrue = lhs <= rhs;
		else if (!strcmp(cmp, ">"))
			conditiontrue = lhs > rhs;
		else if (!strcmp(cmp, ">="))
			conditiontrue = lhs >= rhs;
		else
			conditiontrue = false;
	}
	else
	{
		conditiontrue = conditiontrue == !!lhs;
	}
	if (*token)
		token = COM_ParseExt(ptr, false, false);
	if (!strcmp(token, "&&"))
		return Shader_EvaluateCondition(shader, ptr) && conditiontrue;
	if (!strcmp(token, "||"))
		return Shader_EvaluateCondition(shader, ptr) || conditiontrue;

	return conditiontrue;
}

static char *Shader_ParseExactString(const char **ptr)
{
	char *token;

	if (!ptr || !(*ptr))
		return "";
	if (!**ptr || **ptr == '}')
		return "";

	token = COM_ParseExt(ptr, false, false);
	return token;
}

static char *Shader_ParseString(const char **ptr)
{
	char *token;

	if (!ptr || !(*ptr))
		return "";
	while(**ptr == ' ' || **ptr == '\t')
		*ptr+=1;
	if (!**ptr || **ptr == '}')
		return "";

	token = COM_ParseExt(ptr, false, true);
	Q_strlwr ( token );

	return token;
}

static char *Shader_ParseSensString(const char **ptr)
{
	char *token;

	if (!ptr || !(*ptr))
		return "";
	if (!**ptr || **ptr == '}')
		return "";

	token = COM_ParseExt(ptr, false, true);

	return token;
}

static float Shader_ParseFloat(shader_t *shader, const char **ptr, float defaultval)
{
	char *token;
	if (!ptr || !(*ptr))
		return defaultval;
	if (!**ptr || **ptr == '}')
		return defaultval;

	token = COM_ParseExt(ptr, false, true);
	if (*token == '$')
	{
		if (token[1] == '#')
		{
			return Shader_FloatArgument(shader, token+2);
		}
		else
		{
			cvar_t *var;
			var = Cvar_FindVar(token+1);
			if (var)
			{
				if (*var->string)
					return var->value;
				else
					return defaultval;
			}
		}
	}
	if (!*token)
		return defaultval;
	return atof(token);
}

static void Shader_ParseVector(shader_t *shader, const char **ptr, vec3_t v)
{
	const char *scratch;
	char *token;
	qboolean bracket;
	qboolean fromcvar = false;

	token = Shader_ParseString(ptr);
	if (*token == '$')
	{
		cvar_t *var;
		var = Cvar_FindVar(token+1);
		if (!var)
		{
			v[0] = 1;
			v[1] = 1;
			v[2] = 1;
			return;
		}
		var->flags |= CVAR_SHADERSYSTEM;
		ptr = &scratch;
		scratch = var->string;

		token = Shader_ParseString(ptr);
		fromcvar = true;
	}
	if (!Q_stricmp (token, "("))
	{
		bracket = true;
		token = Shader_ParseString(ptr);
	}
	else if (token[0] == '(')
	{
		bracket = true;
		token = &token[1];
	}
	else
		bracket = false;

	if (!strncmp(token, "0x", 2))
	{	//0xRRGGBB
		unsigned int hex = strtoul(token, NULL, 0);
		v[0] = ((hex>>16)&255)/255.;
		v[1] = ((hex>> 8)&255)/255.;
		v[2] = ((hex>> 0)&255)/255.;
		return;
	}

	v[0] = atof ( token );
	
	token = Shader_ParseString ( ptr );
	if ( !token[0] ) {
		v[1] = fromcvar?v[0]:0;
	} else if (bracket &&  token[strlen(token)-1] == ')' ) {
		bracket = false;
		token[strlen(token)-1] = 0;
		v[1] = atof ( token );
	} else {
		v[1] = atof ( token );
	}

	token = Shader_ParseString ( ptr );
	if ( !token[0] ) {
		v[2] = fromcvar?v[1]:0;
	} else if (bracket && token[strlen(token)-1] == ')' ) {
		token[strlen(token)-1] = 0;
		v[2] = atof ( token );
	} else {
		v[2] = atof ( token );
		if ( bracket ) {
			Shader_ParseString ( ptr );
		}
	}

	/*
	if (v[0] > 5 || v[1] > 5 || v[2] > 5)
	{
		VectorScale(v, 1.0f/255, v);
	}
	*/
}

qboolean Shader_ParseSkySides (char *shadername, char *texturename, texid_t *images)
{	//FIXME: use Image_LoadCubemapTextureData to load the faces
	//if possible directly use a 7th/cubemap texture instead
	//this requires fixing the sky code to not do the random transforms thing though.
	qboolean allokay = true;
	int i, ss, sp;
	char path[MAX_QPATH];

	static char	*skyname_suffix[][6] = {
		{"rt", "bk", "lf", "ft", "up", "dn"},
//		{"px", "py", "nx", "ny", "pz", "nz"},
//		{"posx", "posy", "negx", "negy", "posz", "negz"},
//		{"_px", "_py", "_nx", "_ny", "_pz", "_nz"},
//		{"_posx", "_posy", "_negx", "_negy", "_posz", "_negz"},
		{"_rt", "_bk", "_lf", "_ft", "_up", "_dn"}
	};

	static char *skyname_pattern[] = {
		"%s_%s",
		"%s%s",
		"env/%s%s",
		"gfx/env/%s%s"
	};

	if (*texturename == '$')
	{
		cvar_t *v;
		v = Cvar_FindVar(texturename+1);
		if (v)
			texturename = v->string;
	}
	if (!*texturename)
		texturename = "-";

	for ( i = 0; i < 6; i++ )
	{
		if ( texturename[0] == '-' )
		{
			images[i] = r_nulltex;
			allokay = true;
		}
		else
		{
			for (sp = 0; sp < sizeof(skyname_pattern)/sizeof(skyname_pattern[0]); sp++)
			{
				for (ss = 0; ss < sizeof(skyname_suffix)/sizeof(skyname_suffix[0]); ss++)
				{
					Q_snprintfz ( path, sizeof(path), skyname_pattern[sp], texturename, skyname_suffix[ss][i] );
					images[i] = R_LoadHiResTexture ( path, NULL, IF_NOALPHA|IF_CLAMP|IF_LOADNOW);
					if (images[i]->width)
						break;
				}
				if (images[i]->width)
					break;
			}
			if (!images[i]->width)
			{
				Con_DPrintf("Sky \"%s\" missing texture: %s\n", shadername, path);
				images[i] = r_blackimage;
				allokay = false;
			}
		}
	}
	return allokay;
}

static void Shader_ParseFunc (parsestate_t *ps, const char *functype, const char **ptr, shaderfunc_t *func)
{
	shader_t *shader = ps->s;
	char *token;

	token = Shader_ParseString (ptr);
	if (!Q_stricmp (token, "sin"))
		func->type = SHADER_FUNC_SIN;
	else if (!Q_stricmp (token, "triangle"))
		func->type = SHADER_FUNC_TRIANGLE;
	else if (!Q_stricmp (token, "square"))
		func->type = SHADER_FUNC_SQUARE;
	else if (!Q_stricmp (token, "sawtooth"))
		func->type = SHADER_FUNC_SAWTOOTH;
	else if (!Q_stricmp (token, "inversesawtooth"))
		func->type = SHADER_FUNC_INVERSESAWTOOTH;
	else if (!Q_stricmp (token, "noise"))
		func->type = SHADER_FUNC_NOISE;
	else
	{
		if (!Q_stricmp (token, "distanceramp"))	//QFusion
			;
		else
			Con_Printf("%s: %s: unknown %s \"%s\"\n", ps->sourcename, shader->name, functype, token);
		func->type = SHADER_FUNC_CONSTANT;	//not supported...
		Shader_ParseFloat (shader, ptr, 0);
		Shader_ParseFloat (shader, ptr, 0);
		Shader_ParseFloat (shader, ptr, 0);
		Shader_ParseFloat (shader, ptr, 0);
		Vector4Set(func->args, 255, 255, 255, 255);
		return;
	}

	func->args[0] = Shader_ParseFloat (shader, ptr, 0);
	func->args[1] = Shader_ParseFloat (shader, ptr, 0);
	func->args[2] = Shader_ParseFloat (shader, ptr, 0);
	func->args[3] = Shader_ParseFloat (shader, ptr, 0);
}

//===========================================================================

static int Shader_SetImageFlags(parsestate_t *parsestate, shaderpass_t *pass, char **name, int flags)
{
	//fixme: pass flags should be handled elsewhere.
	for(;name;)
	{
		if (!Q_strnicmp(*name, "$rt:", 4))
		{
			*name += 4;
			flags |= IF_NOMIPMAP|IF_CLAMP|IF_RENDERTARGET;
			if (!(flags & (IF_NEAREST|IF_LINEAR)))
				flags |= IF_LINEAR;
		}
		else if (!Q_strnicmp(*name, "$clamp:", 7))
		{
			*name += 7;
			flags |= IF_CLAMP;
		}
		else if (!Q_strnicmp(*name, "$premul:", 8))
		{	//have the engine premultiply textures for you, instead of needing to do it in an editor.
			*name += 8;
			flags |= IF_PREMULTIPLYALPHA;
		}
		else if (!Q_strnicmp(*name, "$2d:", 4))
		{
			*name+=4;
			flags = (flags&~IF_TEXTYPEMASK) | IF_TEXTYPE_2D;
		}
		else if (!Q_strnicmp(*name, "$2darray:", 9))
		{
			*name+=9;
			flags = (flags&~IF_TEXTYPEMASK) | IF_TEXTYPE_2D_ARRAY;
		}
		else if (!Q_strnicmp(*name, "$3d:", 4))
		{
			*name+=4;
			flags = (flags&~IF_TEXTYPEMASK) | IF_TEXTYPE_3D;
		}
		else if (!Q_strnicmp(*name, "$cube:", 6))
		{
			*name+=6;
			flags = (flags&~IF_TEXTYPEMASK) | IF_TEXTYPE_CUBE;
		}
		else if (!Q_strnicmp(*name, "$cubearray:", 11))
		{
			*name+=11;
			flags = (flags&~IF_TEXTYPEMASK) | IF_TEXTYPE_CUBE_ARRAY;
		}
		else if (!Q_strnicmp(*name, "$srgb:", 6))
		{
			*name+=6;
			flags &= ~IF_NOSRGB;
			flags |= IF_SRGB;
		}
		else if (!Q_strnicmp(*name, "$nosrgb:", 8))
		{
			*name+=8;
			flags &= ~IF_SRGB;
			flags |= IF_NOSRGB;
		}
		else if (!Q_strnicmp(*name, "$nearest:", 9))
		{
			*name+=9;
			flags &= ~IF_LINEAR;
			flags |= IF_NEAREST;
		}
		else if (!Q_strnicmp(*name, "$linear:", 8))
		{
			*name+=8;
			flags &= ~IF_NEAREST;
			flags |= IF_LINEAR;
		}
		else if (!Q_strnicmp(*name, "$palettize:", 11))
		{
			*name+=11;
			flags |= IF_PALETTIZE;
		}
		else
			break;
	}

//	if (shader->flags & SHADER_SKY)
//		flags |= IF_SKY;
	if (parsestate->s->flags & SHADER_NOMIPMAPS)
		flags |= IF_NOMIPMAP;
	if (parsestate->s->flags & SHADER_NOPICMIP)
		flags |= IF_NOPICMIP;
	flags |= IF_MIPCAP;

	if (pass)
	{
		if (flags & IF_CLAMP)
			pass->flags |= SHADER_PASS_CLAMP;
		if (flags & IF_LINEAR)
		{
			pass->flags &= ~SHADER_PASS_NEAREST;
			pass->flags |= SHADER_PASS_LINEAR;
		}
		else if (flags & IF_NEAREST)
		{
			pass->flags &= ~SHADER_PASS_LINEAR;
			pass->flags |= SHADER_PASS_NEAREST;
		}
	}

	return flags;
}

texid_t R_LoadColourmapImage(void)
{
	//FIXME: cache the result, because this is abusive
	unsigned int w = 256, h = VID_GRADES;
	unsigned int x;
	unsigned int data[256*(VID_GRADES)];
	qbyte *colourmappal = (qbyte *)FS_LoadMallocFile ("gfx/colormap.lmp", NULL);
#if defined(Q2CLIENT) && defined(IMAGEFMT_PCX)
	if (!colourmappal)
	{
		size_t sz;
		qbyte *pcx = FS_LoadMallocFile("pics/colormap.pcx", &sz);
		if (pcx)
		{
			colourmappal = Z_Malloc(256*VID_GRADES);
			ReadPCXData(pcx, sz, 256, VID_GRADES, colourmappal);
			BZ_Free(pcx);
		}
	}
#endif
	if (colourmappal)
	{
		for (x = 0; x < sizeof(data)/sizeof(data[0]); x++)
			data[x] = d_8to24rgbtable[colourmappal[x]];
	}
	else
	{	//erk
		//fixme: generate a proper colourmap
		for (x = 0; x < sizeof(data)/sizeof(data[0]); x++)
		{
			int r, g, b;
			float l = 1.0-((x/256)/(float)VID_GRADES);
			r = d_8to24rgbtable[x & 0xff];
			g = (r>>16)&0xff;
			b = (r>>8)&0xff;
			r = (r>>0)&0xff;
			data[x] = d_8to24rgbtable[GetPaletteIndex(r*l,g*l,b*l)];
		}
	}
	BZ_Free(colourmappal);
	return R_LoadTexture("$colourmap", w, h, TF_RGBA32, data, IF_NOMIPMAP|IF_NOPICMIP|IF_NEAREST|IF_NOGAMMA|IF_CLAMP);
}

static texid_t Shader_FindImage (parsestate_t *parsestate, char *name, int flags )
{
	extern texid_t missing_texture_normal;
	if (parsestate->parseflags & SPF_DOOM3)
	{
		if (!Q_stricmp (name, "_default"))
			return r_whiteimage; /*fixme*/
		if (!Q_stricmp (name, "_white"))
			return r_whiteimage;
		if (!Q_stricmp (name, "_black"))
		{
			int wibuf[16] = {0};
			return R_LoadTexture("$blackimage", 4, 4, TF_RGBA32, wibuf, IF_NOMIPMAP|IF_NOPICMIP|IF_NEAREST|IF_NOGAMMA);
		}
	}
	else
	{
		if (!Q_stricmp (name, "$whiteimage"))
			return r_whiteimage;
		if (!Q_stricmp (name, "$blackimage"))
		{
			int wibuf[16] = {0};
			return R_LoadTexture("$blackimage", 4, 4, TF_RGBA32, wibuf, IF_NOMIPMAP|IF_NOPICMIP|IF_NEAREST|IF_NOGAMMA);
		}
		if (!Q_stricmp (name, "$identitynormal"))
			return missing_texture_normal;
		if (!Q_stricmp (name, "$colourmap"))
			return R_LoadColourmapImage();
	}
	if (flags & IF_RENDERTARGET)
		return R2D_RT_Configure(name, 0, 0, TF_INVALID, flags);
	return R_LoadHiResTexture(name, NULL, flags);
}


/****************** shader keyword functions ************************/

static void Shader_Cull (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	char *token;

	shader->flags &= ~(SHADER_CULL_FRONT|SHADER_CULL_BACK);

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "disable") || !Q_stricmp (token, "none") || !Q_stricmp (token, "twosided") ) {
	} else if ( !Q_stricmp (token, "front") ) {
		shader->flags |= SHADER_CULL_FRONT;
	} else if ( !Q_stricmp (token, "back") || !Q_stricmp (token, "backside") || !Q_stricmp (token, "backsided") ) {
		shader->flags |= SHADER_CULL_BACK;
	} else {
		shader->flags |= SHADER_CULL_FRONT;
	}
}

static void Shader_NoMipMaps (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->flags |= (SHADER_NOMIPMAPS|SHADER_NOPICMIP);
}

static void Shader_Affine (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	int i;
	for (i = 0; i < countof(shader->passes); i++)
		shader->passes[i].shaderbits |= SBITS_AFFINE;
}
static void Shader_FullRate (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	int i;
	for (i = 0; i < countof(shader->passes); i++)
		shader->passes[i].shaderbits |= SBITS_MISC_FULLRATE;
}

static void Shader_NoPicMip (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->flags |= SHADER_NOPICMIP;
}

static void Shader_DeformVertexes (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	char *token;
	deformv_t *deformv;

	if ( shader->numdeforms >= SHADER_DEFORM_MAX )
		return;

	deformv = &shader->deforms[shader->numdeforms];

	shader->flags |= SHADER_NOMARKS;	//just in case...

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "wave") )
	{
		deformv->type = DEFORMV_WAVE;
		deformv->args[0] = Shader_ParseFloat (shader, ptr, 0);
		if (deformv->args[0])
			deformv->args[0] = 1.0f / deformv->args[0];
		Shader_ParseFunc (ps, "deformvertexes wave", ptr, &deformv->func );
	}
	else if ( !Q_stricmp (token, "normal") )
	{
		deformv->type = DEFORMV_NORMAL;
		deformv->args[0] = Shader_ParseFloat (shader, ptr, 0);
		deformv->args[1] = Shader_ParseFloat (shader, ptr, 0);
	}
	else if ( !Q_stricmp (token, "bulge") )
	{
		deformv->type = DEFORMV_BULGE;
		Shader_ParseVector (shader, ptr, deformv->args);
	}
	else if ( !Q_stricmp (token, "move") )
	{
		deformv->type = DEFORMV_MOVE;
		Shader_ParseVector (shader, ptr, deformv->args );
		Shader_ParseFunc (ps, "deformvertexes move", ptr, &deformv->func );
	}
	else if ( !Q_stricmp (token, "autosprite") )
	{
		deformv->type = DEFORMV_AUTOSPRITE;
	}
	else if ( !Q_stricmp (token, "autosprite2") )
	{
		deformv->type = DEFORMV_AUTOSPRITE2;
	}
	else if ( !Q_stricmp (token, "projectionShadow") )
		deformv->type = DEFORMV_PROJECTION_SHADOW;
	else if ( !Q_strnicmp (token, "text", 4) )
	{
		deformv->type = DEFORMV_TEXT;
		deformv->args[0] = atoi(token+4);
	}
	else
		return;

	shader->numdeforms++;
}

static void Shader_ClutterParms(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	struct shader_clutter_s *clut;
	char *modelname;

	modelname = Shader_ParseString(ptr);
	clut = Z_Malloc(sizeof(*clut) + strlen(modelname));
	strcpy(clut->modelname, modelname);
	clut->spacing	= Shader_ParseFloat(shader, ptr, 1000);
	clut->scalemin	= Shader_ParseFloat(shader, ptr, 1);
	clut->scalemax	= Shader_ParseFloat(shader, ptr, 1);
	clut->zofs		= Shader_ParseFloat(shader, ptr, 0);
	clut->anglemin	= Shader_ParseFloat(shader, ptr, 0) * M_PI * 2 / 360.;
	clut->anglemax	= Shader_ParseFloat(shader, ptr, 360) * M_PI * 2 / 360.;

	clut->next = shader->clutter;
	shader->clutter = clut;
}

static void Shader_SkyParms(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	skydome_t *skydome;
//	float skyheight;
	char *boxname;

	if (shader->skydome)
	{
		Z_Free(shader->skydome);
	}

	skydome = (skydome_t *)Z_Malloc(sizeof(skydome_t));
	shader->skydome = skydome;

	boxname = Shader_ParseString(ptr);
	Shader_ParseSkySides(shader->name, boxname, skydome->farbox_textures);

	/*skyheight =*/ Shader_ParseFloat(shader, ptr, 512);

	boxname = Shader_ParseString(ptr);
//	Shader_ParseSkySides(shader->name, boxname, skydome->nearbox_textures);

	shader->flags |= SHADER_SKY;
	shader->sort = SHADER_SORT_SKY;
}

static void Shader_FogParms (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	float div;
	vec3_t color, fcolor;

//	if ( !r_ignorehwgamma->value )
//		div = 1.0f / pow(2, max(0, floor(r_overbrightbits->value)));
//	else
		div = 1.0f;

	Shader_ParseVector (shader, ptr, color );
	VectorScale ( color, div, color );
	ColorNormalize ( color, fcolor );

	shader->fog_color[0] = FloatToByte ( fcolor[0] );
	shader->fog_color[1] = FloatToByte ( fcolor[1] );
	shader->fog_color[2] = FloatToByte ( fcolor[2] );
	shader->fog_color[3] = 255;
	shader->fog_dist = Shader_ParseFloat (shader, ptr, 128);

	if ( shader->fog_dist <= 0.0f ) {
		shader->fog_dist = 128.0f;
	}
	shader->fog_dist = 1.0f / shader->fog_dist;

	shader->flags |= SHADER_NODLIGHT|SHADER_NOSHADOWS;
}

static void Shader_SurfaceParm (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	char *token;

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp( token, "nodraw" ) )
		shader->flags |= SHADER_NODRAW;
	else if ( !Q_stricmp( token, "nodraw2" ) )
		shader->flags |= SHADER_NODRAW;	//an alternative so that q3map2 won't see+strip it.
	else if ( !Q_stricmp( token, "nodlight" ) )
		shader->flags |= SHADER_NODLIGHT;
	else if ( !Q_stricmp( token, "noshadows" ) )
		shader->flags |= SHADER_NOSHADOWS;

	else if ( !Q_stricmp( token, "sky" ) )
		shader->flags |= SHADER_SKY;

	else if ( !Q_stricmp( token, "noimpact" ) )
		shader->flags |= SHADER_NOMARKS;	//wrong, but whatever.
	else if ( !Q_stricmp( token, "nomarks" ) )
		shader->flags |= SHADER_NOMARKS;

	//forceshader type things inherit certain textures from the original material
	//however, that original material might not need those textures and thus won't have them loaded, which breaks replacement.
	//these provide a way to override that.
	else if (!Q_stricmp( token, "hasdiffuse"))
		shader->flags |= SHADER_HASDIFFUSE;
	else if (!Q_stricmp( token, "hasnormalmap"))
		shader->flags |= SHADER_HASNORMALMAP;
	else if (!Q_stricmp( token, "hasgloss"))
		shader->flags |= SHADER_HASGLOSS;
	else if (!Q_stricmp( token, "hasfullbright"))
		shader->flags |= SHADER_HASFULLBRIGHT;
	else if (!Q_stricmp( token, "haspaletted"))
		shader->flags |= SHADER_HASPALETTED;
	else if (!Q_stricmp(token, "hastop") || !Q_stricmp(token, "hasbottom") || !Q_stricmp(token, "hastopbottom"))
		shader->flags |= SHADER_HASTOPBOTTOM;
	else
		Con_DLPrintf(2, "Shader %s, Unknown surface parm \"%s\"\n", ps->s->name, token);	//note that there are game-specific names used to override mod surfaceflags+contents
}

static void Shader_DP_Sort (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	char *token;

	token = Shader_ParseString ( ptr );

	if (!Q_stricmp(token, "sky"))
		shader->sort = SHADER_SORT_SKY;
	else if (!Q_stricmp(token, "hud"))
		shader->sort = SHADER_SORT_NEAREST;
//	else if (!Q_stricmp(token, "distance"))
//		shader->sort = SHADER_SORT_NONE;	//not really immplemented, could maybe force v_depthsortentities. just let q3 rules take over.
}

static void Shader_Sort (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	char *token;

	token = Shader_ParseString ( ptr );
	if (r_forceprogramify.ival >= 2)
	{
		Con_DPrintf("Shader %s, ignoring 'sort %s'\n", ps->s->name, token);
		return;	//dp ignores 'sort' entirely.
	}
//	else if ( !Q_stricmp( token, "none" ) )
//		shader->sort = SHADER_SORT_NONE;	//default, overwritten with an automatic choice.
	else if ( !Q_stricmp( token, "ripple" ) )	//fte, weird. drawn only to the ripplemap.
		shader->sort = SHADER_SORT_RIPPLE;
	else if ( !Q_stricmp( token, "deferredlight" ) )	//fte, weird. drawn only to prelight buffer.
		shader->sort = SHADER_SORT_DEFERREDLIGHT;
	else if ( !Q_stricmp( token, "portal" ) )
		shader->sort = SHADER_SORT_PORTAL;
	else if( !Q_stricmp( token, "sky" ) )
		shader->sort = SHADER_SORT_SKY;
	else if( !Q_stricmp( token, "opaque" ) )
		shader->sort = SHADER_SORT_OPAQUE;
	else if( !Q_stricmp( token, "decal" ) || !Q_stricmp( token, "litdecal" ) )
		shader->sort = SHADER_SORT_DECAL;
	else if( !Q_stricmp( token, "seethrough" ) )
		shader->sort = SHADER_SORT_SEETHROUGH;
	else if( !Q_stricmp( token, "unlitdecal" ) )
		shader->sort = SHADER_SORT_UNLITDECAL;
	else if( !Q_stricmp( token, "banner" ) )
		shader->sort = SHADER_SORT_BANNER;
	else if( !Q_stricmp( token, "underwater" ) )
		shader->sort = SHADER_SORT_UNDERWATER;
	else if( !Q_stricmp( token, "blend" ))
		shader->sort = SHADER_SORT_BLEND;
	else if( !Q_stricmp( token, "additive" ) )
		shader->sort = SHADER_SORT_ADDITIVE;
	else if( !Q_stricmp( token, "nearest" ) )
		shader->sort = SHADER_SORT_NEAREST;
	else
	{
		int q3 = atoi ( token );
		shadersort_t q3sorttofte[] =
		{
			/* 0*/SHADER_SORT_NONE,
			/* 1*/SHADER_SORT_PORTAL,
			/* 2*/SHADER_SORT_SKY,		//aka environment in q3
			/* 3*/SHADER_SORT_OPAQUE,
			/* 4*/SHADER_SORT_DECAL,
			/* 5*/SHADER_SORT_SEETHROUGH,
			/* 6*/SHADER_SORT_BANNER,
			/* 7*/SHADER_SORT_UNDERWATER/*SHADER_SORT_FOG*/,
			/* 8*/SHADER_SORT_UNDERWATER,
			/* 9*/SHADER_SORT_BLEND,		//blend0 in q3
			/*10*/SHADER_SORT_ADDITIVE,	//blend1 in q3
			/*11*/SHADER_SORT_ADDITIVE/*SHADER_SORT_BLEND2*/,
			/*12*/SHADER_SORT_ADDITIVE/*SHADER_SORT_BLEND3*/,
			/*13*/SHADER_SORT_ADDITIVE/*SHADER_SORT_BLEND6*/, //yes, 4+5 missing in q3...
			/*14*/SHADER_SORT_ADDITIVE/*SHADER_SORT_STENCIL*/,
			/*15*/SHADER_SORT_NEAREST/*SHADER_SORT_ALMOSTNEAREST*/,
			/*16*/SHADER_SORT_NEAREST
		};
		if (q3 >= 0 && q3 < countof(q3sorttofte))
			shader->sort = q3sorttofte[q3];
		else
			shader->sort = SHADER_SORT_NONE;	// :(
		clamp ( shader->sort, SHADER_SORT_NONE, SHADER_SORT_NEAREST );
	}
}

static void Shader_Deferredlight (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->sort = SHADER_SORT_DEFERREDLIGHT;
}

static void Shader_Portal (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->sort = SHADER_SORT_PORTAL;
}

static void Shader_PolygonOffset (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	float m = Shader_ParseFloat(shader, ptr, 1);

	shader->polyoffset.unit = -25 * m;
	shader->polyoffset.factor = -0.05;
	shader->flags |= SHADER_POLYGONOFFSET;	//some backends might be lazy and only allow simple values.
}

static void Shader_EntityMergable (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->flags |= SHADER_ENTITY_MERGABLE;
}

#if defined(GLQUAKE) || defined(D3DQUAKE)
static qboolean Shader_ParseProgramCvar(char *script, char **cvarnames, int *cvartypes, int cvartype)
{
	cvar_t *ref;
	char body[MAX_QPATH];
	char *out;
	char *namestart;
	while (*script == ' ' || *script == '\t')
		script++;
	namestart = script;
	while ((*script >= 'A' && *script <= 'Z') || (*script >= 'a' && *script <= 'z') || (*script >= '0' && *script <= '9') || *script == '_')
		script++;

	cvartypes[0] = cvartype;
	cvarnames[0] = Z_Malloc(script - namestart + 1);
	memcpy(cvarnames[0], namestart, script - namestart);
	cvarnames[0][script - namestart] = 0;

	while (*script == ' ' || *script == '\t')
		script++;
	if (*script == '=')
	{
		script++;
		while (*script == ' ' || *script == '\t')
			script++;

		out = body;
		while (out < com_token+countof(body)-1 && *script != '\n' && !(script[0] == '/' && script[1] == '/')) 
			*out++ = *script++;
		*out++ = 0;
		ref = Cvar_Get(cvarnames[0], body, 0, "GLSL Variables");
	}
	else
		ref = Cvar_Get(cvarnames[0], "", 0, "GLSL Variables");
	if (!ref)
	{
		Z_Free(cvarnames[0]);
		return false;
	}
	return true;
}
static qboolean Shader_ParseSemantic(const char *script, const char *shadername, char **cvarnames, int *cvartypes)
{
	int s;
	const char *namestart, *nameend;
	while (*script == ' ' || *script == '\t')
		script++;
	namestart = script;
	while ((*script >= 'A' && *script <= 'Z') || (*script >= 'a' && *script <= 'z') || (*script >= '0' && *script <= '9') || *script == '_')
		script++;
	nameend = script;
	if (*script == ' ' || *script == '\t')
	{
		while (*script == ' ' || *script == '\t')
			script++;
		while ((*script >= 'A' && *script <= 'Z') || (*script >= 'a' && *script <= 'z') || (*script >= '0' && *script <= '9') || *script == '_')
			script++;
	}
	else
		return false;

	cvarnames[0] = Z_Malloc(script - namestart + 1);
	memcpy(cvarnames[0], namestart, script - namestart);
	cvarnames[0][script - namestart] = 0;

	cvarnames[0][nameend-namestart] = 0;
	nameend = &cvarnames[0][nameend-namestart]+1;

	for (s = 0; shader_unif_names[s].name; s++)
	{
		if (!strcmp(shader_unif_names[s].name, nameend))
		{
			cvartypes[0] = shader_unif_names[s].ptype;
			return true;
		}
	}
	Con_Printf("%s: semantic %s not found\n", shadername, nameend);
	Z_Free(cvarnames[0]);
	return false;
}
static qboolean Shader_ParseProgramConst(char *script, char **cvarnames, int *cvartypes, int cvartype, unsigned short *numsamplers)
{
	char *namestart;
	char *nameend;
	while (*script == ' ' || *script == '\t')
		script++;
	namestart = script;
	while ((*script >= 'A' && *script <= 'Z') || (*script >= 'a' && *script <= 'z') || (*script >= '0' && *script <= '9') || *script == '_')
		script++;
	nameend = script;
	if (*script == ' ' || *script == '\t')
	{
		while (*script != '\n')
			script++;
	}

	cvartypes[0] = cvartype;
	cvarnames[0] = Z_Malloc(script - namestart + 1);
	memcpy(cvarnames[0], namestart, script - namestart);
	cvarnames[0][script - namestart] = 0;

	//not a cvar. data is baked weirdly into the name.
	if (nameend < script)
	{
		cvarnames[0][nameend-namestart] = '=';

		if (numsamplers)
		{	//this is a !!constt
			//make sure we know the max sampler id needed...
			unsigned short s;
			nameend = &cvarnames[0][nameend-namestart]+1;
			while (*nameend == ' ' || *nameend == '\t')
				nameend++;
			s = atoi(nameend)+1;
			if (*numsamplers < s)
				*numsamplers = s;
		}
	}
	return true;
}
#endif

const struct sh_defaultsamplers_s sh_defaultsamplers[] =
{
	{"s_shadowmap",		1u<<S_SHADOWMAP},
	{"s_projectionmap",	1u<<S_PROJECTIONMAP},
	{"s_diffuse",		1u<<S_DIFFUSE},
	{"s_normalmap",		1u<<S_NORMALMAP},
	{"s_specular",		1u<<S_SPECULAR},
	{"s_upper",			1u<<S_UPPERMAP},
	{"s_lower",			1u<<S_LOWERMAP},
	{"s_fullbright",	1u<<S_FULLBRIGHT},
	{"s_paletted",		1u<<S_PALETTED},
	{"s_reflectcube",	1u<<S_REFLECTCUBE},
	{"s_reflectmask",	1u<<S_REFLECTMASK},
	{"s_displacement",	1u<<S_DISPLACEMENT},
	{"s_occlusion",		1u<<S_OCCLUSION},
	{"s_transmission",	1u<<S_TRANSMISSION},
	{"s_thickness",		1u<<S_THICKNESS},
	{"s_lightmap",		1u<<S_LIGHTMAP0},
	{"s_deluxemap",		1u<<S_DELUXEMAP0},
#if MAXRLIGHTMAPS > 1
	{"s_lightmap1",		1u<<S_LIGHTMAP1},
	{"s_lightmap2",		1u<<S_LIGHTMAP2},
	{"s_lightmap3",		1u<<S_LIGHTMAP3},
	{"s_deluxemap1",	1u<<S_DELUXEMAP1},
	{"s_deluxemap2",	1u<<S_DELUXEMAP2},
	{"s_deluxemap3",	1u<<S_DELUXEMAP3},
#else
	{"s_lightmap1",		0},
	{"s_lightmap2",		0},
	{"s_lightmap3",		0},
	{"s_deluxemap1",	0},
	{"s_deluxemap2",	0},
	{"s_deluxemap3",	0},
#endif
	{NULL}
};

static struct
{
	char *name;
	unsigned int bitmask;
} permutations[] =
{
	{"BUMP", PERMUTATION_BUMPMAP},
	{"FULLBRIGHT", PERMUTATION_FULLBRIGHT},
	{"UPPERLOWER", PERMUTATION_UPPERLOWER},
	{"REFLECTCUBEMASK", PERMUTATION_REFLECTCUBEMASK},
	{"SKELETAL", PERMUTATION_SKELETAL},
	{"FOG", PERMUTATION_FOG},
	{"FRAMEBLEND", PERMUTATION_FRAMEBLEND},
	{"LIGHTSTYLED", PERMUTATION_LIGHTSTYLES}
};
#define MAXMODIFIERS 64

void VARGS Q_strlcatfz (char *dest, size_t *offset, size_t size, const char *fmt, ...) LIKEPRINTF(4);
void VARGS Q_strlcatfz (char *dest, size_t *offset, size_t size, const char *fmt, ...)
{
	va_list		argptr;

	dest += *offset;
	size -= *offset;

	va_start (argptr, fmt);
	Q_vsnprintfz(dest, size, fmt, argptr);
	va_end (argptr);
	*offset += strlen(dest);
}
struct programpermu_s *Shader_LoadPermutation(program_t *prog, unsigned int p)
{
	const char *permutationdefines[3];
	struct programpermu_s *pp;
	size_t n, pn = 0;
	char defines[8192];
	size_t offset;
	qboolean fail = false;

	extern cvar_t r_glsl_pbr, gl_specular, gl_specular_power;

	if (~prog->supportedpermutations & p)
		return NULL;	//o.O
	pp = Z_Malloc(sizeof(*pp));
	pp->permutation = p;
	*defines = 0;
	offset = 0;
	if (p & PERMUTATION_SKELETAL)
		Q_strlcatfz(defines, &offset, sizeof(defines), "#define MAX_GPU_BONES %i\n", sh_config.max_gpu_bones);
	if (gl_specular.value)
		Q_strlcatfz(defines, &offset, sizeof(defines), "#define SPECULAR\n#define SPECULAR_BASE_MUL %f\n#define SPECULAR_BASE_POW %f\n", 1.0*gl_specular.value, max(1,gl_specular_power.value));
	if (r_glsl_pbr.ival)
		Q_strlcatfz(defines, &offset, sizeof(defines), "#define PBR\n");
#ifdef RTLIGHTS
	if (r_fakeshadows)
		Q_strlcatfz(defines, &offset, sizeof(defines), "#define FAKESHADOWS\n%s",
#ifdef GLQUAKE
				gl_config.arb_shadow?"#define USE_ARB_SHADOW\n":
#endif
				"");
#endif

	for (n = 0; n < countof(permutations); n++)
	{
		if (p & permutations[n].bitmask)
			Q_strlcatfz(defines, &offset, sizeof(defines), "#define %s\n", permutations[n].name);
	}
	if (p & PERMUTATION_UPPERLOWER)
		Q_strlcatfz(defines, &offset, sizeof(defines), "#define UPPER\n#define LOWER\n");
	if (p & PERMUTATION_BUMPMAP)
	{
		if (r_glsl_offsetmapping.ival)
		{
			Q_strlcatfz(defines, &offset, sizeof(defines), "#define OFFSETMAPPING\n");
			if (r_glsl_offsetmapping_reliefmapping.ival && (p & PERMUTATION_BUMPMAP))
				Q_strlcatfz(defines, &offset, sizeof(defines), "#define RELIEFMAPPING\n");
		}

		if (r_deluxemapping)	//fixme: should be per-model really
			Q_strlcatfz(defines, &offset, sizeof(defines), "#define DELUXE\n");
	}
	permutationdefines[pn++] = defines;
	permutationdefines[pn++] = prog->preshade;
	permutationdefines[pn++] = NULL;

	if (!sh_config.pCreateProgram(prog, pp, prog->shaderver, permutationdefines, prog->shadertext, prog->tess?prog->shadertext:NULL, prog->tess?prog->shadertext:NULL, prog->geom?prog->shadertext:NULL, prog->shadertext, prog->warned, NULL))
		prog->warned = fail = true;

	//extra loop to validate the programs actually linked properly.
	//delaying it like this gives certain threaded drivers a chance to compile them all while we're messing around with other junk
	if (!fail && sh_config.pValidateProgram && !sh_config.pValidateProgram(prog, pp, prog->warned, NULL))
		prog->warned = fail = true;

	if (!fail && sh_config.pProgAutoFields)
	{
		char *cvarnames[64+1];
		int cvartypes[64];

		unsigned char *cvardata = prog->cvardata;
		size_t size = prog->cvardatasize, i;
		for (i = 0; i < countof(cvartypes) && size; i++)
		{
			memcpy(&cvartypes[i], cvardata, sizeof(int));
			cvarnames[i] = cvardata+sizeof(int);
			size -= sizeof(int)+strlen(cvarnames[i])+1;
			cvardata += sizeof(int)+strlen(cvarnames[i])+1;
		}
		cvarnames[i] = NULL; //no more
		sh_config.pProgAutoFields(prog, pp, cvarnames, cvartypes);
	}
	if (fail)
	{
		Z_Free(pp);
		return NULL;
	}
	return pp;
}

qboolean Shader_PermutationEnabled(unsigned int bit)
{
	if (bit == PERMUTATION_REFLECTCUBEMASK)
		return gl_load24bit.ival;
	if (bit == PERMUTATION_BUMPMAP)
		return r_loadbumpmapping;
	return true;
}
qboolean Com_PermuOrFloatArgument(const char *shadername, char *arg, size_t arglen, float def)
{
	extern cvar_t gl_specular;
	size_t p;
	//load-time-only permutations...
	if (arglen == 8 && !strncmp("SPECULAR", arg, arglen) && gl_specular.value)
		return true;
#ifdef RTLIGHTS
	if (arglen == 11 && !strncmp("FAKESHADOWS", arg, arglen) && r_fakeshadows)
		return true;
#endif
	if ((arglen==5||arglen==6) && !strncmp("DELUXE", arg, arglen) && r_deluxemapping && Shader_PermutationEnabled(PERMUTATION_BUMPMAP))
		return true;
	if (arglen == 13 && !strncmp("OFFSETMAPPING", arg, arglen) && r_glsl_offsetmapping.ival)
		return true;
	if (arglen == 13 && !strncmp("RELIEFMAPPING", arg, arglen) && r_glsl_offsetmapping.ival && r_glsl_offsetmapping_reliefmapping.ival)
		return true;

	//real permutations
	if (arglen == 5 && (!strncmp("UPPER", arg, arglen)||!strncmp("LOWER", arg, arglen)) && Shader_PermutationEnabled(PERMUTATION_BIT_UPPERLOWER))
		return true;
	for (p = 0; p < countof(permutations); p++)
	{
		if (arglen == strlen(permutations[p].name) && !strncmp(permutations[p].name, arg, arglen))
		{
			if (Shader_PermutationEnabled(permutations[p].bitmask))
				return true;
			break;
		}
	}

	return Com_FloatArgument(shadername, arg, arglen, def) != 0;
}

static qboolean Shader_LoadPermutations(char *name, program_t *prog, char *script, int qrtype, int ver, char *blobfilename)
{
#if defined(GLQUAKE) || defined(D3DQUAKE)
//	const char *permutationdefines[countof(permutations) + MAXMODIFIERS + 1];
	unsigned int nopermutation = PERMUTATIONS-1;
//	int nummodifiers = 0;
	int p;
	char *end;

	char *cvarnames[64];
	int cvartypes[64];
	size_t cvarcount = 0, i;
	extern cvar_t gl_specular, gl_specular_power;
	qboolean cantess = false;	//not forced.
	char prescript[8192];
	size_t offset = 0;
#endif

	if (qrenderer != qrtype)
	{
		return false;
	}

#ifdef VKQUAKE
	if (qrenderer == QR_VULKAN && qrtype == QR_VULKAN)
	{	//vulkan 'scripts' are just blobs. could maybe base64 the spirv but eww.
		return VK_LoadBlob(prog, script, name);
	}
#endif

#if defined(GLQUAKE) || defined(D3DQUAKE)
	ver = 0;

	if (!sh_config.pCreateProgram && !sh_config.pLoadBlob)
		return false;

	if (prog->name)
		return false;	//o.O

	*prescript = 0;
	offset = 0;
	memset(prog->permu, 0, sizeof(prog->permu));
	prog->name = Z_StrDup(name);
	prog->geom = false;
	prog->tess = false;
	prog->rayquery = false;
	prog->calcgens = false;
	prog->numsamplers = 0;
	prog->defaulttextures = 0;
	for(;;)
	{
		while (*script == ' ' || *script == '\r' || *script == '\n' || *script == '\t')
			script++;
		if (!strncmp(script, "!!fixed", 7))
		{
			prog->calcgens = true;
			script += 7;
		}
		else if (!strncmp(script, "!!explicit", 10))
		{
			prog->explicitsyms = true;
			script += 10;
		}
		else if (!strncmp(script, "!!geom", 6))
		{
			prog->geom = true;
			script += 6;
		}
		else if (!strncmp(script, "!!tess", 6))
		{
			prog->tess = true;
			script += 6;
		}
		else if (!strncmp(script, "!!samps", 7))
		{
			com_tokentype_t tt;
			qboolean ignore = false;
			script += 7;
			for(;;)
			{
				size_t len;
				int i;
				char *type, *idx, *next;
				char *token = com_token;

				next = COM_ParseTokenOut(script, "", com_token, sizeof(com_token), &tt);
				if (tt == TTP_LINEENDING || tt == TTP_EOF)
					break;
				script = next;

				if (*token == '=' || *token == '!')
				{
					len = strlen(token);
					if (*token == (Com_PermuOrFloatArgument(name, token+1, len-1, 0)?'!':'='))
						ignore = true;
					continue;
				}
				else if (ignore)
					continue;
#if 1//def HAVE_LEGACY
				else if (!strncmp(token, "deluxmap", 8))
				{	//FIXME: remove this some time.
					Con_DPrintf("Outdated texture name \"%s\" in program \"%s\"\n", token, name);
					token = va("deluxemap%s",token+8);
				}
#endif
				type = strchr(token, ':');
				idx = strchr(token, '=');
				if (type || idx)
				{	//name:type=idx
					if (type)
						*type++ = 0;
					else
						type = "2D";
					if (idx)
					{
						*idx++ = 0;
						i = atoi(idx);
					}
					else
						i = prog->numsamplers;
					if (prog->numsamplers < i+1)
						prog->numsamplers = i+1;

					/*for (j = 0; sh_defaultsamplers[j].name; j++)
					{
						if (!strcmp(token, sh_defaultsamplers[j].name+2))
						{
							Con_Printf("%s: %s is an internal texture name\n", name, token);
							break;
						}
					}*/

					//I really want to use layout(binding = %i) here, but its specific to the glsl version (which we don't really know yet)
					Q_strlcatfz(prescript, &offset, sizeof(prescript), "#define s_%s s_t%u\nuniform %s%s s_%s;\n", token, i, strncmp(type, "sampler", 7)?"sampler":"", type, token);
				}
				else
				{
					len = strlen(token);
					for (i = 0; sh_defaultsamplers[i].name; i++)
					{
						if (!strcmp(token, sh_defaultsamplers[i].name+2))
						{
							prog->defaulttextures |= sh_defaultsamplers[i].defaulttexbits;
							break;
						}
					}
					if (!sh_defaultsamplers[i].name)
					{	//this path is deprecated.
						i = atoi(token);
						if (i)
						{
							if (qrenderer == QR_OPENGL)
							{
								while (prog->numsamplers < i)
									Q_strlcatfz(prescript, &offset, sizeof(prescript), "uniform sampler2D s_t%u;\n", prog->numsamplers++);
							}
							else if (prog->numsamplers < i)
								prog->numsamplers = i;
						}
						else
							Con_Printf("Unknown texture name \"%s\" in program \"%s\"\n", token, name);
					}
				}
			}
		}
		else if (!strncmp(script, "!!cvardf", 8) || !strncmp(script, "!!cvard3", 8) || !strncmp(script, "!!cvard4", 8) || !strncmp(script, "!!cvard_srgb", 11))
		{
			qboolean srgb = false;
			float div = 1;
			char type = script[7];
			script+=8;
			if (type == '_')
			{
				if (*script == 's')
				{
					srgb = true;
					script+=1;
				}

				if (!strncmp(script, "rgba", 4))
				{
					type = '4';
					script+=4;
				}
				else if (!strncmp(script, "rgb", 3))
				{
					type = '3';
					script+=3;
				}
				else if (!strncmp(script, "rg", 2))
				{
					type = '2';
					script+=2;
				}
				else if (!strncmp(script, "r", 1) || !strncmp(script, "f", 1))
				{
					type = 'f';
					script+=1;
				}

				if (!strncmp(script, "_b", 2))
				{
					div = 255;
					script+=2;
				}
				else if (!strncmp(script, "_", 1))
					div = strtod(script, &script);
			}
			while (*script == ' ' || *script == '\t')
				script++;
			end = script;
			while ((*end >= 'A' && *end <= 'Z') || (*end >= 'a' && *end <= 'z') || (*end >= '0' && *end <= '9') || *end == '_')
				end++;
			if (end - script < 64)
			{
				cvar_t *var;
				char namebuf[64];
				char valuebuf[64];
				memcpy(namebuf, script, end - script);
				namebuf[end - script] = 0;
				while (*end == ' ' || *end == '\t')
					end++;
				if (*end == '=')
				{
					script = ++end;
					while (*end && *end != '\n' && *end != '\r' && end < script+sizeof(namebuf)-1)
						end++;
					memcpy(valuebuf, script, end - script);
					valuebuf[end - script] = 0;
				}
				else
					strcpy(valuebuf, "0");
				var = Cvar_Get(namebuf, valuebuf, CVAR_SHADERSYSTEM, "GLSL Variables");
				if (var)
				{
					var->flags |= CVAR_SHADERSYSTEM;
					if (srgb)
					{
						if (type == '4')
							Q_strlcatfz(prescript, &offset, sizeof(prescript), "#define %s %s(%g,%g,%g,%g)\n", namebuf, ((qrenderer == QR_OPENGL)?"vec4":"float4"), SRGBf(var->vec4[0]/div), SRGBf(var->vec4[1]/div), SRGBf(var->vec4[2]/div), var->vec4[3]/div);
						else if (type == '3')
							Q_strlcatfz(prescript, &offset, sizeof(prescript), "#define %s %s(%g,%g,%g)\n", namebuf, ((qrenderer == QR_OPENGL)?"vec3":"float3"), SRGBf(var->vec4[0]/div), SRGBf(var->vec4[1]/div), SRGBf(var->vec4[2]/div));
						else
							Q_strlcatfz(prescript, &offset, sizeof(prescript), "#define %s %g\n", namebuf, SRGBf(var->value/div));
					}
					else
					{
						if (type == '4')
							Q_strlcatfz(prescript, &offset, sizeof(prescript), "#define %s %s(%g,%g,%g,%g)\n", namebuf, ((qrenderer == QR_OPENGL)?"vec4":"float4"), var->vec4[0]/div, var->vec4[1]/div, var->vec4[2]/div, var->vec4[3]/div);
						else if (type == '3')
							Q_strlcatfz(prescript, &offset, sizeof(prescript), "#define %s %s(%g,%g,%g)\n", namebuf, ((qrenderer == QR_OPENGL)?"vec3":"float3"), var->vec4[0]/div, var->vec4[1]/div, var->vec4[2]/div);
						else
							Q_strlcatfz(prescript, &offset, sizeof(prescript), "#define %s %g\n", namebuf, var->value/div);
					}
				}
			}
			script = end;
		}
		else if (!strncmp(script, "!!cvarf", 7) || !strncmp(script, "!!cvari", 7) || !strncmp(script, "!!cvarv", 7) || !strncmp(script, "!!cvar3f", 8) || !strncmp(script, "!!cvar4f", 8))
		{
			int type;
			     if (script[6]=='f') type = SP_CVARF;
			else if (script[6]=='i') type = SP_CVARI;
			else if (script[6]=='v') type = SP_CVAR3F;
			else if (script[6]=='3') type = SP_CVAR3F;
			else if (script[6]=='4') type = SP_CVAR4F;
			else break;
			if (cvarcount != sizeof(cvarnames)/sizeof(cvarnames[0]))
				cvarcount += Shader_ParseProgramCvar(script+8, &cvarnames[cvarcount], &cvartypes[cvarcount], type);
		}
		else if (	!strncmp(script, "!!semantic", 10))
		{
			if (cvarcount != sizeof(cvarnames)/sizeof(cvarnames[0]))
				cvarcount += Shader_ParseSemantic(script+10, name, &cvarnames[cvarcount], &cvartypes[cvarcount]);
		}
		else if (	!strncmp(script, "!!const1f", 9) ||
					!strncmp(script, "!!const2f", 9) ||
					!strncmp(script, "!!const3f", 9) ||
					!strncmp(script, "!!const4f", 9) ||
					!strncmp(script, "!!constt", 8))
		{
			int type;
			if (script[8] == 'f')
				type = SP_CONST1F + (script[7]-'1');
			else if (script[8] == 'i')
				type = SP_CONST1I + (script[7]-'1');
			else if (script[7] == 't')
				type = SP_TEXTURE;
			else
				break;

			if (cvarcount != sizeof(cvarnames)/sizeof(cvarnames[0]))
				cvarcount += Shader_ParseProgramConst(script+9, &cvarnames[cvarcount], &cvartypes[cvarcount], type, (type==SP_TEXTURE)?&prog->numsamplers:NULL);
		}
		else if (!strncmp(script, "!!arg", 5))
		{	//compat with our vulkan glsl, generate (specialisation) constants from #args
			char namebuf[MAX_QPATH];
			char valuebuf[MAX_QPATH];
			char *out;
			char *namestart;
			char *atype;
			script+=5;
			if (*script == 'b')
			{
				atype = "bool";
				strcpy(valuebuf, "false");
			}
			else if (*script == 'f')
			{
				atype = "float";
				strcpy(valuebuf, "0.0");
			}
			else if (*script == 'd')
			{
				atype = "double";
				strcpy(valuebuf, "0.0");
			}
			else if (*script == 'i')
			{
				atype = "int";
				strcpy(valuebuf, "0");
			}
			else if (*script == 'u')
			{
				atype = "uint";
				strcpy(valuebuf, "0");
			}
			else
			{
				atype = "float";	//I guess
				strcpy(valuebuf, "0.0");
			}
			while (*script >= 'a' && *script <= 'z')
				script++;
			while (*script == ' ' || *script == '\t')
				script++;
			namestart = script;
			while ((*script >= 'A' && *script <= 'Z') || (*script >= 'a' && *script <= 'z') || (*script >= '0' && *script <= '9') || *script == '_')
				script++;

			if (script-namestart < countof(namebuf))
			{
				float def = 0;
				memcpy(namebuf, namestart, script - namestart);
				namebuf[script - namestart] = 0;

				while (*script == ' ' || *script == '\t')
					script++;
				if (*script == '=')
				{
					script++;
					while (*script == ' ' || *script == '\t')
						script++;

					out = valuebuf;
					while (out < com_token+countof(valuebuf)-1 && *script != '\n' && !(script[0] == '/' && script[1] == '/'))
						*out++ = *script++;
					*out++ = 0;
					if (!strcmp(valuebuf, "true"))
						def = 1;
					else
						def = atof(valuebuf);
				}
				Com_FloatArgument(name, valuebuf, sizeof(valuebuf), def);
				Q_strlcatfz(prescript, &offset, sizeof(prescript), "const %s arg_%s = %s(%s);\n", atype, namebuf, atype, valuebuf);
			}
		}
		else if (!strncmp(script, "!!permu", 7))
		{
			script += 7;
			while (*script == ' ' || *script == '\t')
				script++;
			end = script;
			while ((*end >= 'A' && *end <= 'Z') || (*end >= 'a' && *end <= 'z') || (*end >= '0' && *end <= '9') || *end == '_')
				end++;
			for (p = 0; p < countof(permutations); p++)
			{
				if (!strncmp(permutations[p].name, script, end - script) && permutations[p].name[end-script] == '\0')
				{
					if (Shader_PermutationEnabled(permutations[p].bitmask))
						nopermutation &= ~permutations[p].bitmask;
					break;
				}
			}
			if (p == countof(permutations))
			{
				//we 'recognise' ones that are force-defined, despite not being actual permutations.
				if (end - script == 4 && !strncmp("TESS", script, 4))
					cantess = true;
				else if (strncmp("SPECULAR", script, end - script))
				if (strncmp("DELUXE", script, end - script))
				if (strncmp("DELUX", script, end - script))
				if (strncmp("OFFSETMAPPING", script, end - script))
				if (strncmp("RELIEFMAPPING", script, end - script))
				if (strncmp("FAKESHADOWS", script, end - script))
					Con_DPrintf("Unknown pemutation in glsl program %s\n", name);
			}
			script = end;
		}
		else if (!strncmp(script, "!!ver", 5))
		{
			int minver, maxver;
			script += 5;
			while (*script == ' ' || *script == '\t')
				script++;
			end = script;
			while ((*end >= 'A' && *end <= 'Z') || (*end >= 'a' && *end <= 'z') || (*end >= '0' && *end <= '9') || *end == '_')
				end++;
			minver = strtol(script, &script, 0);
			while (*script == ' ' || *script == '\t')
				script++;
			maxver = strtol(script, NULL, 0);
			if (!maxver)
				maxver = minver;

			ver = maxver;
			if (ver > sh_config.maxver)
				ver = sh_config.maxver;
			if (ver < minver)
				ver = minver;	//some kind of error.

			script = end;
		}
		else if (!strncmp(script, "!!", 2))
		{
			Con_DPrintf("Unknown directinve in glsl program %s\n", name);
			script += 2;
			while (*script == ' ' || *script == '\t')
				script++;
		}
		else if (!strncmp(script, "//", 2))
		{
			script += 2;
			while (*script == ' ' || *script == '\t')
				script++;
		}
		else
			break;
		while (*script && *script != '\n')
			script++;
	}
	prog->shadertext = Z_StrDup(script);

	if (qrenderer == qrtype && ver < 150)
		prog->tess = cantess = false;	//GL_ARB_tessellation_shader requires glsl 150(gl3.2) (or glessl 3.1). nvidia complains about layouts if you try anyway

	if (!r_fog_permutation.ival)
		nopermutation |= PERMUTATION_BIT_FOG;
	if (!sh_config.max_gpu_bones)
		nopermutation |= PERMUTATION_SKELETAL;

	//multiple lightmaps is kinda hacky. if any are set, all must be.
#if MAXRLIGHTMAPS > 1
	if (prog->defaulttextures & ((1u<<S_LIGHTMAP1 ) | (1u<<S_LIGHTMAP2 ) | (1u<<S_LIGHTMAP3 )))
		prog->defaulttextures |=((1u<<S_LIGHTMAP1 ) | (1u<<S_LIGHTMAP2 ) | (1u<<S_LIGHTMAP3 ));
	if (prog->defaulttextures & ((1u<<S_DELUXEMAP1) | (1u<<S_DELUXEMAP2) | (1u<<S_DELUXEMAP3)))
		prog->defaulttextures |=((1u<<S_DELUXEMAP1) | (1u<<S_DELUXEMAP2) | (1u<<S_DELUXEMAP3));
#endif

	for (end = *name?strchr(name+1, '#'):NULL; end && *end; )
	{
		size_t startoffset=offset;
		char *start = end+1;
		end = strchr(start, '#');
		if (!end)
			end = start + strlen(start);
		if (end-start == 7 && !Q_strncasecmp(start, "usemods", 7))
			prog->calcgens = true;

		if (end-start == 4 && !Q_strncasecmp(start, "tess", 4))
			prog->tess |= cantess;

		Q_strlcatfz(prescript, &offset, sizeof(prescript), "#define ");
		while (offset < sizeof(prescript) && start < end)
		{
			if (*start == '=')
			{
				if (offset == startoffset+8)
					break;
				start++;
				prescript[offset++] = ' ';
				break;
			}
			if ((*start >='a'&&*start<='z')||(*start >='A'&&*start<='Z')||*start=='_'||(*start >='0'&&*start<='9'&&offset>startoffset+8))
				prescript[offset++] = toupper(*start++);
			else
			{	///invalid symbol name...
				offset = startoffset+8;
				prescript[offset] = 0;
				break;
			}
		}
		if (offset == startoffset+8)
		{	///invalid symbol name...
			offset = startoffset;
			prescript[offset] = 0;
			break;
		}
		while (offset < sizeof(prescript) && start < end)
			prescript[offset++] = toupper(*start++);
		Q_strlcatfz(prescript, &offset, sizeof(prescript), "\n");
	}

	prog->preshade = Z_StrDup(prescript);
	prog->supportedpermutations = (~nopermutation) & (PERMUTATIONS-1);
	prog->shaderver = ver;

	if (cvarcount)
	{
		*prescript = 0;
		offset = 0;
		for (i = 0; i < cvarcount && offset < sizeof(prescript); i++)
		{
			memcpy(prescript+offset, &cvartypes[i], sizeof(int));
			offset+=4;
			Q_strlcatfz(prescript, &offset, sizeof(prescript), "%s", cvarnames[i]);
			offset++;
		}
		prog->cvardata = Z_Malloc(offset);
		prog->cvardatasize = offset;
		memcpy(prog->cvardata, prescript, prog->cvardatasize);
	}

	while(cvarcount)
		Z_Free((char*)cvarnames[--cvarcount]);

	//ensure that permutation 0 works correctly as a fallback.
	//FIXME: add debug mode to compile all.
	prog->permu[0] = Shader_LoadPermutation(prog, 0);
	if (!prog->permu[0])
		return false;

	if (r_glsl_precache.ival)
	{
		for (p = prog->supportedpermutations; p > 0; )
		{
			if (p != (p&prog->supportedpermutations))
				p &= prog->supportedpermutations;	//this permutation isn't active, skip to the next one
			else
			{
				prog->permu[p] = Shader_LoadPermutation(prog, p);
				p--;
			}
		}
	}
	return true;
#else
	return false;
#endif
}

typedef struct sgeneric_s
{
	program_t prog;
	struct sgeneric_s *next;
	char *name;
	qboolean failed;
} sgeneric_t;
static sgeneric_t *sgenerics;
static struct sbuiltin_s sbuiltins[] =
{
#include "r_bishaders.h"
	{QR_NONE}
};
void Shader_UnloadProg(program_t *prog)
{
	if (sh_config.pDeleteProg)
		sh_config.pDeleteProg(prog);

	Z_Free(prog->name);
	Z_Free(prog->preshade);
	Z_Free(prog->shadertext);
	Z_Free(prog->cvardata);

	Z_Free(prog);
}
static void Shader_FlushGenerics(void)
{
	sgeneric_t *g;
	while (sgenerics)
	{
		g = sgenerics;
		sgenerics = g->next;

		if (g->prog.refs == 1)
		{
			g->prog.refs--;
			Shader_UnloadProg(&g->prog);
		}
		else
			Con_Printf("generic shader still used\n"); 
	}
}
static void Shader_LoadGeneric(sgeneric_t *g, int qrtype)
{
	unsigned int i;
	void *file;
	char basicname[MAX_QPATH];
	char blobname[MAX_QPATH];
	char *h;

	g->failed = true;

	TRACE(("Loading program %s...\n", g->name));

	basicname[1] = 0;
	Q_strncpyz(basicname, g->name, sizeof(basicname));
	h = strchr(basicname+1, '#');
	if (h)
		*h = '\0';

	if (strchr(basicname, '/') || strchr(basicname, '.'))
	{	//explicit path
		FS_LoadFile(basicname, &file);
		if (!file)
		{	//well that failed. try fixing up the extension in case they omitted that.
			Q_snprintfz(blobname, sizeof(blobname), COM_SkipPath(sh_config.progpath), basicname);
			FS_LoadFile(blobname, &file);
		}
		*blobname = 0;
	}
	else if (ruleset_allow_shaders.ival)
	{	//renderer-specific files
		if (sh_config.progpath)
		{
			Q_snprintfz(blobname, sizeof(blobname), sh_config.progpath, basicname);
			FS_LoadFile(blobname, &file);
		}
		else
			file = NULL;
		if (sh_config.blobpath && r_shaderblobs.ival)
			Q_snprintfz(blobname, sizeof(blobname), sh_config.blobpath, basicname);
		else
			*blobname = 0;
	}
	else
	{
		file = NULL;
		*blobname = 0;
	}

	if (sh_config.pDeleteProg)
	{
		sh_config.pDeleteProg(&g->prog);
	}
	Z_Free(g->prog.name);
	g->prog.name = NULL;
	Z_Free(g->prog.preshade);
	g->prog.preshade = NULL;
	Z_Free(g->prog.shadertext);
	g->prog.shadertext = NULL;
	Z_Free(g->prog.cvardata);
	g->prog.cvardata = NULL;

	if (file)
	{
		TRACE(("Loading from disk (%s)\n", g->name));
//		Con_DPrintf("Loaded %s from disk\n", sh_config.progpath?va(sh_config.progpath, basicname):basicname);
		g->failed = !Shader_LoadPermutations(g->name, &g->prog, file, qrtype, 0, blobname);
		FS_FreeFile(file);
		return;
	}
	else
	{
		int ver;
		const struct sbuiltin_s *progs;
		unsigned int l;
		for (l = 0; l <= materialloader_count; l++)
		{
			if (l == materialloader_count)
				progs = sbuiltins;
			else if (materialloader[l].funcs && materialloader[l].funcs->builtinshaders)
				progs = materialloader[l].funcs->builtinshaders;
			else
				continue;

			for (i = 0; *progs[i].name; i++)
			{
				if (progs[i].qrtype == qrtype && !strcmp(progs[i].name, basicname))
				{
					ver = progs[i].apiver;

					if (ver < sh_config.minver || ver > sh_config.maxver)
						if (!(qrenderer==QR_OPENGL&&ver==110))
							continue;

					TRACE(("Loading Embedded %s\n", g->name));
					g->failed = !Shader_LoadPermutations(g->name, &g->prog, progs[i].body, qrtype, ver, blobname);

					if (g->failed)
						continue;

					return;
				}
			}
		}
		TRACE(("Program unloadable %s\n", g->name));
	}
}

program_t *Shader_FindGeneric(char *name, int qrtype)
{
	sgeneric_t *g;

	for (g = sgenerics; g; g = g->next)
	{
		if (!strcmp(name, g->name))
		{
			if (g->failed)
				return NULL;
			g->prog.refs++;
			return &g->prog;
		}
	}

	//don't even try if we know it won't work.
	if (!sh_config.progs_supported)
		return NULL;

	g = BZ_Malloc(sizeof(*g) + strlen(name)+1);
	memset(g, 0, sizeof(*g));
	g->name = (char*)(g+1);
	strcpy(g->name, name);
	g->next = sgenerics;
	sgenerics = g;

	g->prog.refs = 1;

	Shader_LoadGeneric(g, qrtype);
	if (g->failed)
		return NULL;
	g->prog.refs++;
	return &g->prog;
}
static void Shader_ReloadGenerics(void)
{
	sgeneric_t *g;
	for (g = sgenerics; g; g = g->next)
	{
		//this happens if some cvar changed that affects the glsl itself. supposedly.
		Shader_LoadGeneric(g, qrenderer);
	}

	//this shader can take a while to load due to its number of permutations.
	//because this all happens on the main thread, try to avoid random stalls by pre-loading it.
	if (sh_config.progs_supported)
	{
		program_t *p = Shader_FindGeneric("defaultskin", qrenderer);
		if (p)	//generics get held on to in order to avoid so much churn. so we can just release the reference we just created and it'll be held until shutdown anyway.
			p->refs--;
	}
}
void Shader_WriteOutGenerics_f(void)
{
	int i;
	char *name;
	for (i = 0; *sbuiltins[i].name; i++)
	{
		name = NULL;
		if (sbuiltins[i].qrtype == QR_OPENGL)
		{
			if (sbuiltins[i].apiver == 100)
				name = va("gles/eg_%s.glsl", sbuiltins[i].name);
			else
				name = va("glsl/eg_%s.glsl", sbuiltins[i].name);
		}
		else if (sbuiltins[i].qrtype == QR_DIRECT3D9)
			name = va("hlsl/eg_%s.hlsl", sbuiltins[i].name);
		else if (sbuiltins[i].qrtype == QR_DIRECT3D11)
			name = va("hlsl11/eg_%s.hlsl", sbuiltins[i].name);

		if (name)
		{
			vfsfile_t *f = FS_OpenVFS(name, "rb", FS_GAMEONLY);
			if (f)
			{
				int len = VFS_GETLEN(f);
				char *buf = Hunk_TempAlloc(len);
				VFS_READ(f, buf, len);
				if (len != strlen(sbuiltins[i].body) || memcmp(buf, sbuiltins[i].body, len))
					Con_Printf("Not writing %s - modified version in the way\n", name);
				else
					Con_Printf("%s is unmodified\n", name);
				VFS_CLOSE(f);
			}
			else
			{
				Con_Printf("Writing %s\n", name);
				FS_WriteFile(name, sbuiltins[i].body, strlen(sbuiltins[i].body), FS_GAMEONLY);
			}
		}
	}
}

struct shader_field_names_s shader_attr_names[] =
{
	/*vertex attributes*/
	{"v_position1",				VATTR_VERTEX1},
	{"v_position2",				VATTR_VERTEX2},
	{"v_colour",				VATTR_COLOUR},
	{"v_texcoord",				VATTR_TEXCOORD},
	{"v_lmcoord",				VATTR_LMCOORD},
	{"v_normal",				VATTR_NORMALS},
	{"v_svector",				VATTR_SNORMALS},
	{"v_tvector",				VATTR_TNORMALS},
	{"v_bone",					VATTR_BONENUMS},
	{"v_weight",				VATTR_BONEWEIGHTS},
#if MAXRLIGHTMAPS > 1
	{"v_lmcoord1",				VATTR_LMCOORD},
	{"v_lmcoord2",				VATTR_LMCOORD2},
	{"v_lmcoord3",				VATTR_LMCOORD3},
	{"v_lmcoord4",				VATTR_LMCOORD4},
	{"v_colour1",				VATTR_COLOUR},
	{"v_colour2",				VATTR_COLOUR2},
	{"v_colour3",				VATTR_COLOUR3},
	{"v_colour4",				VATTR_COLOUR4},
#endif
	{NULL}
};

struct shader_field_names_s shader_unif_names[] =
{
	/**///tagged names are available to vulkan

	/*matricies*/
/**/{"m_model",					SP_M_MODEL},	//the model matrix
	{"m_view",					SP_M_VIEW},		//the view matrix
	{"m_modelview",				SP_M_MODELVIEW},//the combined modelview matrix
	{"m_projection",			SP_M_PROJECTION},//projection matrix
/**/{"m_modelviewprojection",	SP_M_MODELVIEWPROJECTION},//fancy mvp matrix. probably has degraded precision.
	{"m_bones_packed",			SP_M_ENTBONES_PACKED},	//bone matrix array. should normally be read via sys/skeletal.h
	{"m_bones_mat3x4",			SP_M_ENTBONES_MAT3X4},	//bone matrix array. should normally be read via sys/skeletal.h
	{"m_bones_mat4",			SP_M_ENTBONES_MAT4},	//bone matrix array. should normally be read via sys/skeletal.h
	{"m_invviewprojection",		SP_M_INVVIEWPROJECTION},//inverted vp matrix
	{"m_invmodelviewprojection",SP_M_INVMODELVIEWPROJECTION},//inverted mvp matrix.
	{"m_invmodelview",			SP_M_INVMODELVIEW},//inverted mv matrix.
/**///m_modelinv

	/*viewer properties*/
	{"v_eyepos",				SP_V_EYEPOS},	//eye pos in worldspace
/**/{"w_fog",					SP_W_FOG},		//read by sys/fog.h
	{"w_user",					SP_W_USER},		//set via VF_USERDATA

	/*ent properties*/
	{"e_vblend",				SP_E_VBLEND},	//v_position1+v_position2 scalers
/**/{"e_lmscale",				SP_E_LMSCALE},	/*overbright shifting*/
	{"e_vlscale",				SP_E_VLSCALE},	/*no lightmaps, no overbrights*/
	{"e_origin",				SP_E_ORIGIN},		//the ents origin in worldspace
/**/{"e_time",					SP_E_TIME},			//r_refdef.time - entity->time
/**/{"e_eyepos",				SP_E_EYEPOS},		//eye pos in modelspace
	{"e_colour",				SP_E_COLOURS},		//colormod/alpha, even if colormod isn't set
/**/{"e_colourident",			SP_E_COLOURSIDENT},	//colormod,alpha or 1,1,1,alpha if colormod isn't set
/**/{"e_glowmod",				SP_E_GLOWMOD},		//fullbright scalers (for hdr mostly)
/**/{"e_uppercolour",			SP_E_TOPCOLOURS},	//q1 player colours
/**/{"e_lowercolour",			SP_E_BOTTOMCOLOURS},//q1 player colours
/**/{"e_light_dir",				SP_E_L_DIR},		//lightgrid light dir. dotproducts should be clamped to 0-1.
/**/{"e_light_mul",				SP_E_L_MUL},		//lightgrid light scaler.
/**/{"e_light_ambient",			SP_E_L_AMBIENT},	//lightgrid light value for the unlit side.

	{"s_colour",				SP_S_COLOUR},	//the rgbgen/alphagen stuff. obviously doesn't work with per-vertex ones.

	/*rtlight properties, use with caution*/
	{"l_lightscreen",			SP_LIGHTSCREEN},	//screenspace position of the current rtlight
/**/{"l_lightradius",			SP_LIGHTRADIUS},	//radius of the current rtlight
/**/{"l_lightcolour",			SP_LIGHTCOLOUR},	//rgb values of the current rtlight
/**/{"l_lightposition",			SP_LIGHTPOSITION},	//light position in modelspace
	{"l_lightdirection",		SP_LIGHTDIRECTION},	//light direction in modelspace (ortho lights only, instead of position)
/**/{"l_lightcolourscale",		SP_LIGHTCOLOURSCALE},//ambient/diffuse/specular scalers
/**/{"l_cubematrix",			SP_LIGHTCUBEMATRIX},//matrix used to control the rtlight's cubemap projection
/**/{"l_shadowmapproj",			SP_LIGHTSHADOWMAPPROJ},	//compacted projection matrix for shadowmaps
/**/{"l_shadowmapscale",		SP_LIGHTSHADOWMAPSCALE},//should probably use a samplerRect instead...

	{"e_sourcesize",			SP_SOURCESIZE},			//size of VF_SOURCECOLOUR image
	{"e_rendertexturescale",	SP_RENDERTEXTURESCALE},	//scaler for $currentrender, when npot isn't supported.

	{NULL}
};

static char *Shader_ParseBody(char *debugname, const char **ptr)
{
	char *body;
	const char *start, *end;

	end = *ptr;
	while (*end == ' ' || *end == '\t' || *end == '\r')
		end++;
	if (*end == '\n')
	{
		int count;
		end++;
		while (*end == ' ' || *end == '\t')
			end++;
		if (*end != '{')
		{
			Con_Printf("shader \"%s\" missing program string\n", debugname);
		}
		else
		{
			end++;
			start = end;
			for (count = 1; *end; end++)
			{
				if (*end == '}')
				{
					count--;
					if (!count)
						break;
				}
				else if (*end == '{')
					count++;
			}
			body = BZ_Malloc(end - start + 1);
			memcpy(body, start, end-start);
			body[end-start] = 0;
			*ptr = end+1;/*skip over it all*/

			return body;
		}
	}
	return NULL;
}

static void Shader_SLProgramName (shader_t *shader, shaderpass_t *pass, const char **ptr, int qrtype)
{
	/*accepts:
	program
	{
		BLAH
	}
	where BLAH is both vertex+frag with #ifdefs
	or
	program fname
	on one line.
	*/
	char *programbody;
	char *hash;
	program_t *newprog;

	programbody = Shader_ParseBody(shader->name, ptr);
	if (programbody)
	{
		newprog = BZ_Malloc(sizeof(*newprog));
		memset(newprog, 0, sizeof(*newprog));
		newprog->refs = 1;
		if (!Shader_LoadPermutations(shader->name, newprog, programbody, qrtype, 0, NULL))
		{
			BZ_Free(newprog);
			newprog = NULL;
		}

		BZ_Free(programbody);
	}
	else
	{
		hash = strchr(shader->name, '#');
		if (hash)
		{
			//pass the # postfixes from the shader name onto the generic glsl to use
			char newname[512];
			Q_snprintfz(newname, sizeof(newname), "%s%s", Shader_ParseExactString(ptr), hash);
			newprog = Shader_FindGeneric(newname, qrtype);
		}
		else
			newprog = Shader_FindGeneric(Shader_ParseExactString(ptr), qrtype);
	}

	if (pass)
	{
		if (pass->numMergedPasses)
		{
			Shader_ReleaseGeneric(newprog);
			Con_DPrintf("shader %s: program defined after first texture map\n", shader->name);
		}
		else
		{
			Shader_ReleaseGeneric(pass->prog);
			pass->prog = newprog;
		}
	}
	else
	{
		Shader_ReleaseGeneric(shader->prog);
		shader->prog = newprog;
	}
}

static void Shader_GLSLProgramName (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	Shader_SLProgramName(shader,pass,ptr,QR_OPENGL);
}
static void Shader_ProgramName (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	Shader_SLProgramName(shader,pass,ptr,qrenderer);
}
static void Shader_HLSL9ProgramName (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	Shader_SLProgramName(shader,pass,ptr,QR_DIRECT3D9);
}
static void Shader_HLSL11ProgramName (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	Shader_SLProgramName(shader,pass,ptr,QR_DIRECT3D11);
}

static void Shaderpass_BlendFunc (parsestate_t *ps, const char **ptr);
static void Shader_ProgBlendFunc (parsestate_t *ps, const char **ptr)
{
	if (ps->s->prog)
	{
		ps->pass = ps->s->passes;
		Shaderpass_BlendFunc(ps, ptr);
		ps->pass = NULL;
	}
}

static void Shader_ReflectCube(parsestate_t *ps, const char **ptr)
{
	char *token = Shader_ParseSensString(ptr);
	unsigned int flags = Shader_SetImageFlags (ps, ps->pass, &token, IF_TEXTYPE_CUBE);
	ps->s->defaulttextures->reflectcube = Shader_FindImage(ps, token, flags);
}
static void Shader_ReflectMask(parsestate_t *ps, const char **ptr)
{
	char *token = Shader_ParseSensString(ptr);
	unsigned int flags = Shader_SetImageFlags (ps, ps->pass, &token, IF_NOSRGB);
	ps->s->defaulttextures->reflectmask = Shader_FindImage(ps, token, flags);
}

static void Shader_DiffuseMap(parsestate_t *ps, const char **ptr)
{
	char *token = Shader_ParseSensString(ptr);
	unsigned int flags = Shader_SetImageFlags (ps, ps->pass, &token, 0);
	ps->s->defaulttextures->base = Shader_FindImage(ps, token, flags);

	Q_strncpyz(ps->s->defaulttextures->mapname, token, sizeof(ps->s->defaulttextures->mapname));
}
static void Shader_SpecularMap(parsestate_t *ps, const char **ptr)
{
	char *token = Shader_ParseSensString(ptr);
	unsigned int flags = Shader_SetImageFlags (ps, ps->pass, &token, 0);
	ps->s->defaulttextures->specular = Shader_FindImage(ps, token, flags);
}
static void Shader_NormalMap(parsestate_t *ps, const char **ptr)
{
	char *token = Shader_ParseSensString(ptr);
	unsigned int flags = Shader_SetImageFlags (ps, ps->pass, &token, IF_TRYBUMP|IF_NOSRGB);
	ps->s->defaulttextures->bump = Shader_FindImage(ps, token, flags);
}
static void Shader_FullbrightMap(parsestate_t *ps, const char **ptr)
{
	char *token = Shader_ParseSensString(ptr);
	unsigned int flags = Shader_SetImageFlags (ps, ps->pass, &token, 0);
	ps->s->defaulttextures->fullbright = Shader_FindImage(ps, token, flags);
}
static void Shader_UpperMap(parsestate_t *ps, const char **ptr)
{
	char *token = Shader_ParseSensString(ptr);
	unsigned int flags = Shader_SetImageFlags (ps, ps->pass, &token, 0);
	ps->s->defaulttextures->upperoverlay = Shader_FindImage(ps, token, flags);
}
static void Shader_LowerMap(parsestate_t *ps, const char **ptr)
{
	char *token = Shader_ParseSensString(ptr);
	unsigned int flags = Shader_SetImageFlags (ps, ps->pass, &token, 0);
	ps->s->defaulttextures->loweroverlay = Shader_FindImage(ps, token, flags);
}
static void Shader_DisplacementMap(parsestate_t *ps, const char **ptr)
{
	char *token = Shader_ParseSensString(ptr);
	unsigned int flags = Shader_SetImageFlags (ps, ps->pass, &token, IF_NOSRGB);
	ps->s->defaulttextures->displacement = Shader_FindImage(ps, token, flags);
}
static void Shader_TransmissionMap(parsestate_t *ps, const char **ptr)
{
	char *token = Shader_ParseSensString(ptr);
	unsigned int flags = Shader_SetImageFlags (ps, ps->pass, &token, IF_NOSRGB);
	ps->s->defaulttextures->transmission = Shader_FindImage(ps, token, flags);
}
static void Shader_ThicknessMap(parsestate_t *ps, const char **ptr)
{
	char *token = Shader_ParseSensString(ptr);
	unsigned int flags = Shader_SetImageFlags (ps, ps->pass, &token, IF_NOSRGB);
	ps->s->defaulttextures->thickness = Shader_FindImage(ps, token, flags);
}

static void Shaderpass_QF_Material(parsestate_t *ps, const char **ptr)
{	//qf_material BASETEXTURE NORMALMAP SPECULARMAP
	unsigned int flags;
	char *progname = "defaultwall";
	char *token;
	char *hash = strchr(ps->s->name, '#');
	if (hash)
	{
		//pass the # postfixes from the shader name onto the generic glsl to use
		char newname[512];
		Q_snprintfz(newname, sizeof(newname), "%s%s", progname, hash);
		ps->pass->prog = Shader_FindGeneric(newname, qrenderer);
	}
	else
		ps->pass->prog = Shader_FindGeneric(progname, qrenderer);

	token = Shader_ParseSensString(ptr);
	if (*token && strcmp(token, "-"))
	{
		flags = Shader_SetImageFlags (ps, ps->pass, &token, 0);
		if (*token)
			ps->s->defaulttextures->base = Shader_FindImage(ps, token, flags);
		else
		{
			token = ps->s->name;
			if (hash)
				*hash = 0;
			ps->s->defaulttextures->base = Shader_FindImage(ps, token, flags);
			if (hash)
				*hash = '#';
		}
	}

	if (*token)
		token = Shader_ParseSensString(ptr);
	if (*token && strcmp(token, "-"))
	{
		flags = Shader_SetImageFlags (ps, NULL, &token, IF_TRYBUMP|IF_NOSRGB);
		ps->s->defaulttextures->bump = Shader_FindImage(ps, token, flags);
	}

	if (*token)
		token = Shader_ParseSensString(ptr);
	if (*token && strcmp(token, "-"))
	{
		flags = Shader_SetImageFlags (ps, NULL, &token, 0);
		ps->s->defaulttextures->specular = Shader_FindImage(ps, token, flags);
	}
}

static qboolean Shaderpass_MapGen (parsestate_t *ps, shaderpass_t *pass, char *tname);

static void Shader_Translucent(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->flags |= SHADER_BLEND;
}

static void Shader_PortalFBOScale(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->portalfboscale = Shader_ParseFloat(shader, ptr, 0);
	shader->portalfboscale = max(shader->portalfboscale, 0);
}

static void Shader_DP_Camera(parsestate_t *ps, const char **ptr)
{
	ps->s->sort = SHADER_SORT_PORTAL;
	ps->dpwatertype |= 4;
}
static void Shader_DP_Water(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	ps->parseflags |= SPF_PROGRAMIFY;

	ps->dpwatertype |= 3;
	ps->reflectmin = Shader_ParseFloat(shader, ptr, 0);
	ps->reflectmax = Shader_ParseFloat(shader, ptr, 0);
	ps->refractfactor = Shader_ParseFloat(shader, ptr, 0);
	ps->reflectfactor = Shader_ParseFloat(shader, ptr, 0);
	Shader_ParseVector(shader, ptr, ps->refractcolour);
	Shader_ParseVector(shader, ptr, ps->reflectcolour);
	ps->wateralpha = Shader_ParseFloat(shader, ptr, 0);
}
static void Shader_DP_Reflect(parsestate_t *ps, const char **ptr)
{
	ps->parseflags |= SPF_PROGRAMIFY;

	ps->dpwatertype |= 1;
	ps->reflectmin = 1;
	ps->reflectmax = 1;
	ps->reflectfactor = Shader_ParseFloat(ps->s, ptr, 0);
	Shader_ParseVector(ps->s, ptr, ps->reflectcolour);
}
static void Shader_DP_Refract(parsestate_t *ps, const char **ptr)
{
	ps->parseflags |= SPF_PROGRAMIFY;

	ps->dpwatertype |= 2;
	ps->refractfactor = Shader_ParseFloat(ps->s, ptr, 0);
	Shader_ParseVector(ps->s, ptr, ps->refractcolour);
}

static void Shader_DP_OffsetMapping(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	char *token = Shader_ParseString(ptr);
	if (!strcmp(token, "disable") || !strcmp(token, "none") || !strcmp(token, "off"))
		ps->offsetmappingmode = 0;
	else if (!strcmp(token, "default") || !strcmp(token, "normal"))
		ps->offsetmappingmode = -1;
	else if (!strcmp(token, "linear"))
		ps->offsetmappingmode = 1;
	else if (!strcmp(token, "relief"))
		ps->offsetmappingmode = 2;
	ps->offsetmappingscale = Shader_ParseFloat(shader, ptr, 1);
	token = Shader_ParseString(ptr);
	if (!strcmp(token, "bias"))
		ps->offsetmappingbias = Shader_ParseFloat(shader, ptr, 0.5);
	else if (!strcmp(token, "match"))
		ps->offsetmappingbias = 1.0 - Shader_ParseFloat(shader, ptr, 0.5);
	else if (!strcmp(token, "match8"))
		ps->offsetmappingbias = 1.0 - (1.0/255) * Shader_ParseFloat(shader, ptr, 128);
	else if (!strcmp(token, "match16"))
		ps->offsetmappingbias = 1.0 - (1.0/65535) * Shader_ParseFloat(shader, ptr, 32768);
}
static void Shader_DP_GlossScale(parsestate_t *ps, const char **ptr)
{
	ps->specularvalscale = Shader_ParseFloat(ps->s, ptr, 0);
}
static void Shader_DP_GlossExponent(parsestate_t *ps, const char **ptr)
{
	ps->specularexpscale = Shader_ParseFloat(ps->s, ptr, 0);
}

static void Shader_FactorBase(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->factors[MATERIAL_FACTOR_BASE][0] = Shader_ParseFloat(shader, ptr, 1);
	shader->factors[MATERIAL_FACTOR_BASE][1] = Shader_ParseFloat(shader, ptr, 1);
	shader->factors[MATERIAL_FACTOR_BASE][2] = Shader_ParseFloat(shader, ptr, 1);
	shader->factors[MATERIAL_FACTOR_BASE][3] = Shader_ParseFloat(shader, ptr, 1);
}
static void Shader_FactorSpec(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->factors[MATERIAL_FACTOR_SPEC][0] = Shader_ParseFloat(shader, ptr, 1);
	shader->factors[MATERIAL_FACTOR_SPEC][1] = Shader_ParseFloat(shader, ptr, 1);
	shader->factors[MATERIAL_FACTOR_SPEC][2] = Shader_ParseFloat(shader, ptr, 1);
	shader->factors[MATERIAL_FACTOR_SPEC][3] = Shader_ParseFloat(shader, ptr, 1);
}
static void Shader_FactorEmit(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->factors[MATERIAL_FACTOR_EMIT][0] = Shader_ParseFloat(shader, ptr, 1);
	shader->factors[MATERIAL_FACTOR_EMIT][1] = Shader_ParseFloat(shader, ptr, 1);
	shader->factors[MATERIAL_FACTOR_EMIT][2] = Shader_ParseFloat(shader, ptr, 1);
	shader->factors[MATERIAL_FACTOR_EMIT][3] = Shader_ParseFloat(shader, ptr, 1);
}
static void Shader_FactorTransmission(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->factors[MATERIAL_FACTOR_TRANSMISSION][0] = Shader_ParseFloat(shader, ptr, 1);
//	shader->factors[MATERIAL_FACTOR_TRANSMISSION][1] = the volume distance;
	shader->factors[MATERIAL_FACTOR_TRANSMISSION][2] = 0;
	shader->factors[MATERIAL_FACTOR_TRANSMISSION][3] = 0;
}
static void Shader_FactorVolume(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->factors[MATERIAL_FACTOR_VOLUME][0] = Shader_ParseFloat(shader, ptr, 1);	//r
	shader->factors[MATERIAL_FACTOR_VOLUME][1] = Shader_ParseFloat(shader, ptr, 1);	//g
	shader->factors[MATERIAL_FACTOR_VOLUME][2] = Shader_ParseFloat(shader, ptr, 1);	//b
	shader->factors[MATERIAL_FACTOR_VOLUME][3] = Shader_ParseFloat(shader, ptr, 1);	//factor
	shader->factors[MATERIAL_FACTOR_TRANSMISSION][1] = Shader_ParseFloat(shader, ptr, 1);	//distance
}

static void Shader_BEMode(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	char subname[1024];
	int mode;
	char tokencopy[1024];
	char *token;
	char *embed = NULL;
	token = Shader_ParseString(ptr);
	if (!Q_stricmp(token, "rtlight"))
		mode = -1;	//all light types
	else if (!Q_stricmp(token, "rtlight_only"))
		mode = LSHADER_STANDARD;
	else if (!Q_stricmp(token, "rtlight_smap"))
		mode = LSHADER_SMAP;
	else if (!Q_stricmp(token, "rtlight_spot"))
		mode = LSHADER_SPOT;
	else if (!Q_stricmp(token, "rtlight_cube"))
		mode = LSHADER_CUBE;
	else if (!Q_stricmp(token, "rtlight_cube_smap"))
		mode = LSHADER_CUBE|LSHADER_SMAP;
	else if (!Q_stricmp(token, "rtlight_cube_spot"))		//doesn't make sense.
		mode = LSHADER_CUBE|LSHADER_SPOT;
	else if (!Q_stricmp(token, "rtlight_spot_smap"))
		mode = LSHADER_SMAP|LSHADER_SPOT;
	else if (!Q_stricmp(token, "rtlight_cube_spot_smap"))	//doesn't make sense.
		mode = LSHADER_CUBE|LSHADER_SPOT|LSHADER_SMAP;
	else if (!Q_stricmp(token, "crepuscular"))
		mode = bemoverride_crepuscular;
	else if (!Q_stricmp(token, "depthonly"))
		mode = bemoverride_depthonly;
	else if (!Q_stricmp(token, "depthdark"))
		mode = bemoverride_depthdark;
	else if (!Q_stricmp(token, "gbuffer") || !Q_stricmp(token, "prelight"))
		mode = bemoverride_gbuffer;
	else if (!Q_stricmp(token, "fog"))
		mode = bemoverride_fog;
	else
	{
		Con_DPrintf(CON_WARNING "Shader %s specifies unknown bemode %s.\n", shader->name, token);
		return;	//not supported.
	}

	embed = Shader_ParseBody(shader->name, ptr);
	if (embed)
	{
		int l = strlen(embed) + 6;
		char *b = BZ_Malloc(l);
		Q_snprintfz(b, l, "{\n%s\n}\n", embed);
		BZ_Free(embed);
		embed = b;
		//generate a unique name
		Q_snprintfz(tokencopy, sizeof(tokencopy), "%s_mode%i", shader->name, mode);
	}
	else
	{
		token = Shader_ParseString(ptr);
		Q_strncpyz(tokencopy, token, sizeof(tokencopy));	//make sure things don't go squiff.
	}

	if (mode == -1)
	{
		//shorthand for rtlights
		for (mode = 0; mode < LSHADER_MODES; mode++)
		{
			if ((mode & LSHADER_RAYQUERY) && !r_shadow_raytrace.ival)
				continue;	//no. just no.
			if ((mode & LSHADER_SMAP) && r_shadow_raytrace.ival)
				continue;	//don't waste time.
			if ((mode & LSHADER_CUBE) && (mode & (LSHADER_SPOT|LSHADER_ORTHO)))
				continue;	//cube projections don't make sense when the light isn't projecting a cube
			if ((mode & LSHADER_ORTHO) && (mode & LSHADER_SPOT))
				continue;	//ortho+spot are mutually exclusive.
			Q_snprintfz(subname, sizeof(subname), "%s%s%s%s%s%s%s",
																(mode & LSHADER_RAYQUERY)?"rq_":"",
																tokencopy,
																(mode & LSHADER_SMAP)?"#PCF":"",
																(mode & LSHADER_SPOT)?"#SPOT":"",
																(mode & LSHADER_CUBE)?"#CUBE":"",
																(mode & LSHADER_ORTHO)?"#ORTHO":"",
#ifdef GLQUAKE
																(qrenderer == QR_OPENGL && gl_config.arb_shadow && (mode & LSHADER_SMAP))?"#USE_ARB_SHADOW":""
#else
																""
#endif
																);
			shader->bemoverrides[mode] = R_RegisterCustom(shader->model, subname, shader->usageflags|(embed?SUR_FORCEFALLBACK:0), embed?Shader_DefaultScript:NULL, embed);
		}
	}
	else
	{
		shader->bemoverrides[mode] = R_RegisterCustom(shader->model, tokencopy, shader->usageflags|(embed?SUR_FORCEFALLBACK:0), embed?Shader_DefaultScript:NULL, embed);
	}
	if (embed)
		BZ_Free(embed);
}

static shaderkey_t shaderkeys[] =
{
#define Q3 NULL
	{"cull",				Shader_Cull,				Q3},
	{"skyparms",			Shader_SkyParms,			Q3},
	{"fogparms",			Shader_FogParms,			Q3},
	{"surfaceparm",			Shader_SurfaceParm,			Q3},
	{"nomipmaps",			Shader_NoMipMaps,			Q3},
	{"nopicmip",			Shader_NoPicMip,			Q3},
	{"polygonoffset",		Shader_PolygonOffset,		Q3},
	{"sort",				Shader_Sort,				Q3},
	{"deformvertexes",		Shader_DeformVertexes,		Q3},
	{"portal",				Shader_Portal,				Q3},
	{"entitymergable",		Shader_EntityMergable,		Q3},

	//fte extensions
	{"clutter",				Shader_ClutterParms,		"fte"},
	{"deferredlight",		Shader_Deferredlight,		"fte"},	//(sort = prelight)
//	{"lpp_light",			Shader_Deferredlight,		"fte"},	//(sort = prelight)
	{"affine",				Shader_Affine,				"fte"},	//some hardware is horribly slow, and can benefit from certain hints.
	{"fullrate",			Shader_FullRate,			"fte"},	//blocks half-rate shading on this surface.

	{"bemode",				Shader_BEMode,				"fte"},

	{"diffusemap",			Shader_DiffuseMap,			"fte"},
	{"normalmap",			Shader_NormalMap,			"fte"},
	{"specularmap",			Shader_SpecularMap,			"fte"},
	{"fullbrightmap",		Shader_FullbrightMap,		"fte"},
	{"uppermap",			Shader_UpperMap,			"fte"},
	{"lowermap",			Shader_LowerMap,			"fte"},
	{"reflectmask",			Shader_ReflectMask,			"fte"},
	{"displacementmap",		Shader_DisplacementMap,		"fte"},
	{"transmissionmap",		Shader_TransmissionMap,		"fte"},
	{"thicknessmap",		Shader_ThicknessMap,		"fte"},

	{"portalfboscale",		Shader_PortalFBOScale,		"fte"},	//portal/mirror/refraction/reflection FBOs are resized by this scale
	{"basefactor",			Shader_FactorBase,			"fte"},	//material scalers for glsl
	{"specularfactor",		Shader_FactorSpec,			"fte"},	//material scalers for glsl
	{"fullbrightfactor",	Shader_FactorEmit,			"fte"},	//material scalers for glsl
	{"fte_transmissionfactor",Shader_FactorTransmission,"fte"},	//material scalers for glsl
	{"fte_volumefactor",	Shader_FactorVolume,		"fte"},	//material scalers for glsl

	//TODO: PBR textures...
//	{"albedomap",			Shader_DiffuseMap,			"fte"},	//rgb(a)
//	{"loweruppermap",		Shader_LowerUpperMap,		"fte"}, //r=lower, g=upper (team being more important than personal colours, this allows the texture to gracefully revert to red-only)
	//{"normalmap",			Shader_NormalMap,			"fte"},	//xy-h
//	{"ormmap",				Shader_SpecularMap,			"fte"},	//r=occlusion, g=metalness, b=roughness.
	//{"glowmap",			Shader_FullbrightMap,		"fte"}, //rgb

	/*program stuff at the material level is an outdated practise.*/
	{"program",				Shader_ProgramName,			"fte"},	//usable with any renderer that has a usable shader language...
	{"glslprogram",			Shader_GLSLProgramName,		"fte"},	//for renderers that accept embedded glsl
	{"hlslprogram",			Shader_HLSL9ProgramName,	"fte"},	//for d3d with embedded hlsl
	{"hlsl11program",		Shader_HLSL11ProgramName,	"fte"},	//for d3d with embedded hlsl
	{"progblendfunc",		Shader_ProgBlendFunc,		"fte"},	//specifies the blend mode (actually just overrides the first subpasses' blendmode.
//	{"progmap",				Shader_ProgMap,				"fte"},	//avoids needing extra subpasses (actually just inserts an extra pass).

	//dp compat
	{"reflectcube",			Shader_ReflectCube,			"dp"},
	{"camera",				Shader_DP_Camera,			"dp"},
	{"water",				Shader_DP_Water,			"dp"},
	{"reflect",				Shader_DP_Reflect,			"dp"},
	{"refract",				Shader_DP_Refract,			"dp"},
	{"offsetmapping",		Shader_DP_OffsetMapping,	"dp"},
	{"shadow",				NULL,						"dp"},
	{"noshadow",			NULL,						"dp"},
	{"polygonoffset",		NULL,						"dp"},
	{"glossintensitymod",	Shader_DP_GlossScale,		"dp"},	//scales r_shadow_glossintensity(=1), aka: gl_specular
	{"glossexponentmod",	Shader_DP_GlossExponent,	"dp"},	//scales r_shadow_glossexponent(=32)
	{"transparentsort",		Shader_DP_Sort,				"dp"},	//urgh...

	/*doom3 compat*/
	{"diffusemap",			Shader_DiffuseMap,			"doom3"},	//macro for "{\nstage diffusemap\nmap <map>\n}"
	{"bumpmap",				Shader_NormalMap,			"doom3"},	//macro for "{\nstage bumpmap\nmap <map>\n}"
	{"discrete",			NULL,						"doom3"},
	{"nonsolid",			NULL,						"doom3"},
	{"noimpact",			NULL,						"doom3"},
	{"translucent",			Shader_Translucent,			"doom3"},
	{"noshadows",			NULL,						"doom3"},
	{"nooverlays",			NULL,						"doom3"},
	{"nofragment",			NULL,						"doom3"},

	/*RTCW compat*/
	{"nocompress",			NULL,						"rtcw"},
	{"allowcompress",		NULL,						"rtcw"},
	{"nofog",				NULL,						"rtcw"},
	{"skyfogvars",			NULL,						"rtcw"},
	{"sunshader",			NULL,						"rtcw"},
	{"sun",					NULL,						"q3map2"},	//provides rgb and dir
	{"sunExt",				NULL,						"q3map2"},	//treated as an alias
	{"fogParms",			NULL,						"rtcw"},	//sets a cvar. *shudder*
	{"fogvars",				NULL,						"rtcw"},	//sets a cvar. *shudder*
	{"waterfogvars",		NULL,						"rtcw"},	//sets a cvar. *shudder*
	{"light",				NULL,						"rtcw"},	//for q3map2, not us
	{"lightgridmulamb",		NULL,						"rtcw"},	//urm
	{"lightgridmuldir",		NULL,						"rtcw"},	//not really sure how this is useful to us

	/*qfusion / warsow compat*/
//	{"skyparms2",			NULL,						"qf"},	//skyparms without the underscore.
//	{"skyparmssides",		NULL,						"qf"},	//skyparms with explicitly-named faces
//	{"nocompress",			NULL,						"qf"},	//disables opportunistic compression (doesn't affect compressed source images, apparently)
//	{"nofiltering",			NULL,						"qf"},	//misnomer. there is always 'filtering'. this means to use nearest filtering for min and mag, as well as no mipmaps.
//	{"smallestmipmapsize",	NULL,						"qf"},	//mips with a size less than the specified value are dropped.
//	{"stenciltest",			NULL,						"qf"},	//enables GL_STENCIL_TEST, which is special-case stuff that I see no reason to support
//	{"offsetmappingscale",	NULL,						"qf"},
//	{"glossexponent",		NULL,						"qf"},
//	{"glossintensity",		NULL,						"qf"},
//	{"template",			NULL,						"qf"},	//parses some other shader, with $3 etc arg expansion
	{"skip",				NULL,						"qf"},	//just skips the line. acts like a comment. no idea why they can't just use a comment.
//	{"softparticle",		NULL,						"qf"},	//uses screen depth, if possible. 
//	{"forceworldoutlines",	NULL,						"qf"},	//looks like an ugly hack to me.

	{NULL,				NULL}
};

static struct
{
	char *name;
	char *body;
} shadermacros[] =
{
	{"decal_macro", 	"polygonOffset 1\ndiscrete\nsort decal\nnoShadows"},
//	{"diffusemap", 		"{\nblend diffusemap\nmap %1\n}"},
//	{"bumpmap", 		"{\nblend bumpmap\nmap %1\n}"},
//	{"specularmap", 	"{\nblend specularmap\nmap %1\n}"},
	{NULL}
};

// ===============================================================

static qboolean Shaderpass_MapGen (parsestate_t *ps, shaderpass_t *pass, char *tname)
{
	shader_t *shader = ps->s;
	int tcgen = TC_GEN_BASE;
	if (!Q_stricmp (tname, "$lightmap"))
	{
		tcgen = TC_GEN_LIGHTMAP;
		pass->flags |= SHADER_PASS_LIGHTMAP | SHADER_PASS_NOMIPMAP;
		pass->texgen = T_GEN_LIGHTMAP;
		shader->flags |= SHADER_HASLIGHTMAP;
	}
	else if (!Q_stricmp (tname, "$deluxmap"))
	{
		tcgen = TC_GEN_LIGHTMAP;
		pass->flags |= SHADER_PASS_DELUXMAP | SHADER_PASS_NOMIPMAP;
		pass->texgen = T_GEN_DELUXMAP;
	}
	else if (!Q_stricmp (tname, "$diffuse"))
	{
		pass->texgen = T_GEN_DIFFUSE;
		shader->flags |= SHADER_HASDIFFUSE;
	}
	else if (!Q_stricmp (tname, "$paletted"))
	{
		pass->texgen = T_GEN_PALETTED;
		shader->flags |= SHADER_HASPALETTED;
	}
	else if (!Q_stricmp (tname, "$normalmap"))
	{
		pass->texgen = T_GEN_NORMALMAP;
		shader->flags |= SHADER_HASNORMALMAP;
	}
	else if (!Q_stricmp (tname, "$specular"))
	{
		pass->texgen = T_GEN_SPECULAR;
		shader->flags |= SHADER_HASGLOSS;
	}
	else if (!Q_stricmp (tname, "$fullbright"))
	{
		pass->texgen = T_GEN_FULLBRIGHT;
		shader->flags |= SHADER_HASFULLBRIGHT;
	}
	else if (!Q_stricmp (tname, "$upperoverlay"))
	{
		shader->flags |= SHADER_HASTOPBOTTOM;
		pass->texgen = T_GEN_UPPEROVERLAY;
	}
	else if (!Q_stricmp (tname, "$loweroverlay"))
	{
		shader->flags |= SHADER_HASTOPBOTTOM;
		pass->texgen = T_GEN_LOWEROVERLAY;
	}
	else if (!Q_stricmp (tname, "$reflectcube"))
	{
		shader->flags |= SHADER_HASREFLECTCUBE;
		pass->texgen = T_GEN_REFLECTCUBE;
	}
	else if (!Q_stricmp (tname, "$reflectmask"))
	{
		pass->texgen = T_GEN_REFLECTMASK;
	}
	else if (!Q_stricmp (tname, "$displacement"))
	{
		shader->flags |= SHADER_HASDISPLACEMENT;
		pass->texgen = T_GEN_DISPLACEMENT;
	}
	else if (!Q_stricmp (tname, "$shadowmap"))
	{
		pass->texgen = T_GEN_SHADOWMAP;
		pass->flags |= SHADER_PASS_DEPTHCMP;
	}
	else if (!Q_stricmp (tname, "$lightcubemap"))
	{
		pass->texgen = T_GEN_LIGHTCUBEMAP;
	}
	else if (!Q_stricmp (tname, "$currentrender"))
	{
		pass->texgen = T_GEN_CURRENTRENDER;
		shader->flags |= SHADER_HASCURRENTRENDER;
	}
	else if (!Q_stricmp (tname, "$sourcecolour"))
	{
		pass->texgen = T_GEN_SOURCECOLOUR;
	}
	else if (!Q_stricmp (tname, "$sourcecube"))
	{
		pass->texgen = T_GEN_SOURCECUBE;
	}
	else if (!Q_stricmp (tname, "$sourcedepth"))
	{
		pass->texgen = T_GEN_SOURCEDEPTH;
	}
	else if (!Q_strnicmp (tname, "$gbuffer", 8))
	{
		unsigned idx = strtoul(tname+8, &tname, 10);
		if (*tname || idx >= GBUFFER_COUNT)
			return false;
		pass->texgen = T_GEN_GBUFFER0 + idx;
	}
	else if (!Q_stricmp (tname, "$reflection"))
	{
		shader->flags |= SHADER_HASREFLECT;
		pass->texgen = T_GEN_REFLECTION;
	}
	else if (!Q_stricmp (tname, "$refraction"))
	{
		shader->flags |= SHADER_HASREFRACT;
		pass->texgen = T_GEN_REFRACTION;
	}
	else if (!Q_stricmp (tname, "$refractiondepth"))
	{
		shader->flags |= SHADER_HASREFRACT;
		pass->texgen = T_GEN_REFRACTIONDEPTH;
	}
	else if (!Q_stricmp (tname, "$ripplemap"))
	{
		shader->flags |= SHADER_HASRIPPLEMAP;
		pass->texgen = T_GEN_RIPPLEMAP;
	}
	else if (!Q_stricmp (tname, "$null"))
	{
		pass->flags |= SHADER_PASS_NOMIPMAP|SHADER_PASS_DETAIL;
		pass->texgen = T_GEN_SINGLEMAP;
	}
	else
		return false;

	if (pass->tcgen == TC_GEN_UNSPECIFIED)
		pass->tcgen = tcgen;
	return true;
}

shaderpass_t *Shaderpass_DefineMap(parsestate_t *ps, shaderpass_t *pass)
{
	//'map foo' works a bit differently when there's a program in the same pass.
	//instead of corrupting the previous one, it collects multiple maps so that {prog foo;map t0;map t1; map t2; blendfunc add} can work as expected
	if (pass->prog)
	{
		if (pass->numMergedPasses==0)
			pass->numMergedPasses++;
		else
		{	//FIXME: bounds check!
			if (ps->s->numpasses == SHADER_PASS_MAX || ps->s->numpasses == SHADER_TMU_MAX)
			{
				Con_DPrintf (CON_WARNING "Shader %s has too many texture passes.\n", ps->s->name);
				ps->droppass = true;
			}
//			else if (shader->numpasses == be_maxpasses)
//				ps->droppass = true;
			else
			{
				pass->numMergedPasses++;
				ps->s->numpasses++;
			}
			pass = ps->s->passes+ps->s->numpasses-1;
			memset(pass, 0, sizeof(*pass));
		}
	}
	else if (pass->numMergedPasses>1)
		pass = ps->s->passes+ps->s->numpasses-1;	//nextbundle stuff.
	else
		pass->numMergedPasses = 1;
	return pass;
}

static void Shaderpass_Map (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	int flags;
	char *token;

	pass = Shaderpass_DefineMap(ps, pass);

	pass->anim_frames[0] = r_nulltex;

	token = Shader_ParseSensString (ptr);

	/*cod compat*/
	if (!stricmp(token, "clamp"))
		token = Shader_ParseSensString (ptr);
	else if (!stricmp(token, "clampx"))
		token = Shader_ParseSensString (ptr);
	else if (!stricmp(token, "clampy"))
		token = Shader_ParseSensString (ptr);

	flags = Shader_SetImageFlags (ps, pass, &token, 0);
	if (!Shaderpass_MapGen(ps, pass, token))
	{
		pass->texgen = T_GEN_SINGLEMAP;
		if (pass->tcgen == TC_GEN_UNSPECIFIED)
			pass->tcgen = TC_GEN_BASE;
		if (!*shader->defaulttextures->mapname && *token != '$' && pass->tcgen == TC_GEN_BASE)
			Q_strncpyz(shader->defaulttextures->mapname, token, sizeof(shader->defaulttextures->mapname));
		pass->anim_frames[0] = Shader_FindImage (ps, token, flags);
	}
}

static void Shaderpass_AnimMap_Flag (parsestate_t *ps, const char **ptr, unsigned int flags)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	char *token;
	texid_t image;
	qboolean isdiffuse = false;

	flags |= Shader_SetImageFlags (ps, ps->pass, NULL, 0);

	if (pass->tcgen == TC_GEN_UNSPECIFIED)
		pass->tcgen = TC_GEN_BASE;
	pass->flags |= SHADER_PASS_ANIMMAP;
	pass->texgen = T_GEN_ANIMMAP;
	pass->anim_fps = Shader_ParseFloat (shader, ptr, 0);
	pass->anim_numframes = 0;

	for ( ; ; )
	{
		token = Shader_ParseString(ptr);
		if (!token[0])
		{
			break;
		}

		if (!pass->anim_numframes && !*shader->defaulttextures->mapname && *token != '$' && pass->tcgen == TC_GEN_BASE)
		{
			isdiffuse = true;
			shader->defaulttextures_fps = pass->anim_fps;
		}

		if (pass->anim_numframes < SHADER_MAX_ANIMFRAMES)
		{
			image = Shader_FindImage (ps, token, flags);

			if (isdiffuse)
			{
				if (shader->numdefaulttextures < pass->anim_numframes+1)
				{
					int newmax = pass->anim_numframes+1;
					shader->defaulttextures = BZ_Realloc(shader->defaulttextures, sizeof(*shader->defaulttextures) * (newmax));
					memset(shader->defaulttextures+shader->numdefaulttextures, 0, sizeof(*shader->defaulttextures) * (newmax-shader->numdefaulttextures));
					shader->numdefaulttextures = newmax;
				}
				Q_strncpyz(shader->defaulttextures[pass->anim_numframes].mapname, token, sizeof(shader->defaulttextures[pass->anim_numframes].mapname));
			}
			if (!TEXVALID(image))
			{
				pass->anim_frames[pass->anim_numframes++] = missing_texture;
				Con_DPrintf (CON_WARNING "Shader %s has an animmap with no image: %s.\n", shader->name, token );
			}
			else
			{
				pass->anim_frames[pass->anim_numframes++] = image;
			}
		}
	}
}
static void Shaderpass_AnimMap (parsestate_t *ps, const char **ptr)
{
	Shaderpass_AnimMap_Flag(ps, ptr, 0);
}
static void Shaderpass_QF_AnimClampMap (parsestate_t *ps, const char **ptr)
{
	Shaderpass_AnimMap_Flag(ps, ptr, IF_CLAMP);
}

static void Shaderpass_ClampMap (parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	int flags;
	char *token;

	token = Shader_ParseSensString (ptr);

	flags = Shader_SetImageFlags (ps, pass, &token, IF_CLAMP);
	if (!Shaderpass_MapGen(ps, pass, token))
	{
		if (pass->tcgen == TC_GEN_UNSPECIFIED)
			pass->tcgen = TC_GEN_BASE;
		pass->anim_frames[0] = Shader_FindImage (ps, token, flags);
		pass->texgen = T_GEN_SINGLEMAP;

		if (!TEXVALID(pass->anim_frames[0]))
		{
			if ((flags & IF_TEXTYPEMASK)!=IF_TEXTYPE_2D)
				pass->anim_frames[0] = r_nulltex;
			else
				pass->anim_frames[0] = missing_texture;
			Con_DPrintf (CON_WARNING "Shader %s has a stage with no image: %s.\n", ps->s->name, token);
		}
	}
}

static void Shaderpass_VideoMap (parsestate_t *ps, const char **ptr)
{
	char		*token = Shader_ParseSensString (ptr);

#ifndef HAVE_MEDIA_DECODER
	(void)token;
#else
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;

	if (pass->cin)
		Z_Free (pass->cin);

	pass->cin = Media_StartCin(token);
	if (!pass->cin)
	{
		Con_DPrintf (CON_WARNING "(shader %s) Couldn't load video %s\n", shader->name, token);
	}

	if (pass->cin)
	{
		pass->flags |= SHADER_PASS_VIDEOMAP;
		shader->flags |= SHADER_VIDEOMAP;
		pass->texgen = T_GEN_VIDEOMAP;
	}
	else
	{
		pass->texgen = T_GEN_DIFFUSE;
		pass->rgbgen = RGB_GEN_CONST;
		pass->rgbgen_func.type = SHADER_FUNC_CONSTANT;
		pass->rgbgen_func.args[0] = pass->rgbgen_func.args[1] = pass->rgbgen_func.args[2] = 0;
	}
#endif
}

static void Shaderpass_RTCW_Map_16bit (parsestate_t *ps, const char **ptr)
{
	if (!gl_load24bit.ival)	//urm, not sure if suitable choice of cvar
		Shaderpass_Map(ps, ptr);
}
static void Shaderpass_RTCW_Map_32bit (parsestate_t *ps, const char **ptr)
{
	if (gl_load24bit.ival)
		Shaderpass_Map(ps, ptr);
}
static void Shaderpass_RTCW_Map_s3tc (parsestate_t *ps, const char **ptr)
{
	if (sh_config.texfmt[PTI_BC3_RGBA] && gl_compress.ival)
		Shaderpass_Map(ps, ptr);
}
static void Shaderpass_RTCW_Map_nos3tc (parsestate_t *ps, const char **ptr)
{
	if (!(sh_config.texfmt[PTI_BC3_RGBA] && gl_compress.ival))
		Shaderpass_Map(ps, ptr);
}
static void Shaderpass_RTCW_AnimMap_s3tc (parsestate_t *ps, const char **ptr)
{
	if ((sh_config.texfmt[PTI_BC3_RGBA] && gl_compress.ival))
		Shaderpass_AnimMap(ps, ptr);
}
static void Shaderpass_RTCW_AnimMap_nos3tc (parsestate_t *ps, const char **ptr)
{
	if (!(sh_config.texfmt[PTI_BC3_RGBA] && gl_compress.ival))
		Shaderpass_AnimMap(ps, ptr);
}

static void Shader_BeginPass(parsestate_t *ps);
static void Shader_EndPass(parsestate_t *ps);
static void Shaderpass_CoD_NextBundle (parsestate_t *ps, const char **ptr)
{	//in a pass... end it and start the next. cos annoying.
	shaderpass_t *basepass = ps->pass;
	shaderpass_t *newpass;
	if (!basepass->numMergedPasses)
		basepass->numMergedPasses = 1;	//its explicit...
	if (ps->s->numpasses == SHADER_PASS_MAX || ps->s->numpasses == SHADER_TMU_MAX)
		ps->droppass = true;
	else
	{
		basepass->numMergedPasses++;
		ps->s->numpasses++;
	}
	newpass = ps->s->passes+ps->s->numpasses-1;
	memset(newpass, 0, sizeof(*newpass));
	newpass->numMergedPasses++;

	newpass->tcgen = TC_GEN_UNSPECIFIED;
	newpass->shaderbits |= SBITS_SRCBLEND_DST_COLOR | SBITS_DSTBLEND_ZERO;
	newpass->shaderbits |= SBITS_DEPTHFUNC_EQUAL;
/*
	qboolean depthwrite = !!(ps->pass->shaderbits & SBITS_MISC_DEPTHWRITE);
	qboolean dropping = ps->droppass;
	Shader_EndPass(ps);

	Shader_BeginPass(ps);
	ps->droppass = dropping;
	//make it modulate.
	ps->pass->shaderbits |= SBITS_SRCBLEND_DST_COLOR | SBITS_DSTBLEND_ZERO;
	if (depthwrite)	//and if the last one is doing weird alphatest crap, copy its depthfunc status.
		ps->pass->shaderbits |= SBITS_DEPTHFUNC_EQUAL;
*/
}
static void Shaderpass_CoD_Requires (parsestate_t *ps, const char **ptr)
{
	if (!Shader_EvaluateCondition(ps->s, ptr))
		ps->droppass = true;
}
static void Shaderpass_CoD_texEnvCombine (parsestate_t *ps, const char **ptr)
{
	int depth = 0;
	char *token;
	while (*(token = COM_ParseExt (&ps->ptr, true, true)))
	{	//extra parsing to even out this unexpected brace without extra warnings.
		if (token[0] == '}')
			depth--;
		else if (token[0] == '{')
			depth++;	//crap.
		if (!depth)
			break;
	}
	ps->droppass = true;
}
static void Shaderpass_CoD_nvRegCombiners (parsestate_t *ps, const char **ptr)
{
	int depth = 0;
	char *token;
	while (*(token = COM_ParseExt (&ps->ptr, true, true)))
	{	//extra parsing to even out this unexpected brace without extra warnings.
		if (token[0] == '}')
			depth--;
		else if (token[0] == '{')
			depth++;	//crap.
		if (!depth)
			break;
	}
	ps->droppass = true;
}
static void Shaderpass_CoD_atiFragmentShader (parsestate_t *ps, const char **ptr)
{
	int depth = 0;
	char *token;
	while (*(token = COM_ParseExt (&ps->ptr, true, true)))
	{	//extra parsing to even out this unexpected brace without extra warnings.
		if (token[0] == '}')
			depth--;
		else if (token[0] == '{')
			depth++;	//crap.
		if (!depth)
			break;
	}
	ps->droppass = true;
}

static void Shaderpass_SLProgramName (shader_t *shader, shaderpass_t *pass, const char **ptr, int qrtype)
{
	/*accepts:
	program
	{
		BLAH
	}
	where BLAH is both vertex+frag with #ifdefs
	or
	program fname
	on one line.
	*/
	//char *programbody;
	char *hash;

	/*programbody = Shader_ParseBody(shader->name, ptr);
	if (programbody)
	{
		shader->prog = BZ_Malloc(sizeof(*shader->prog));
		memset(shader->prog, 0, sizeof(*shader->prog));
		shader->prog->refs = 1;
		if (!Shader_LoadPermutations(shader->name, shader->prog, programbody, qrtype, 0, NULL))
		{
			BZ_Free(shader->prog);
			shader->prog = NULL;
		}

		BZ_Free(programbody);
		return;
	}*/

	hash = strchr(shader->name, '#');
	if (hash)
	{
		//pass the # postfixes from the shader name onto the generic glsl to use
		char newname[512];
		Q_snprintfz(newname, sizeof(newname), "%s%s", Shader_ParseExactString(ptr), hash);
		pass->prog = Shader_FindGeneric(newname, qrtype);
	}
	else
		pass->prog = Shader_FindGeneric(Shader_ParseExactString(ptr), qrtype);
}
static void Shaderpass_ProgramName (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	Shaderpass_SLProgramName(shader,pass,ptr,qrenderer);
}

static void Shaderpass_RGBGen (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	char		*token;

	token = Shader_ParseString (ptr);
	if (!Q_stricmp (token, "identitylighting"))
		pass->rgbgen = RGB_GEN_IDENTITY_LIGHTING;
	else if (!Q_stricmp (token, "identity"))
		pass->rgbgen = RGB_GEN_IDENTITY;
	else if (!Q_stricmp (token, "wave"))
	{
		pass->rgbgen = RGB_GEN_WAVE;
		Shader_ParseFunc (ps, "rgbGen wave", ptr, &pass->rgbgen_func);
	}
	else if (!Q_stricmp(token, "entity"))
		pass->rgbgen = RGB_GEN_ENTITY;
	else if (!Q_stricmp (token, "oneMinusEntity"))
		pass->rgbgen = RGB_GEN_ONE_MINUS_ENTITY;
	else if (!Q_stricmp (token, "vertex"))
	{
		pass->rgbgen = RGB_GEN_VERTEX_LIGHTING;
		if (pass->alphagen == ALPHA_GEN_UNDEFINED)	//matches Q3, and is a perf gain, even if its inconsistent.
			pass->alphagen = ALPHA_GEN_VERTEX;
	}
	else if (!Q_stricmp (token, "oneMinusVertex"))
		pass->rgbgen = RGB_GEN_ONE_MINUS_VERTEX;
	else if (!Q_stricmp (token, "lightingDiffuse"))
		pass->rgbgen = RGB_GEN_LIGHTING_DIFFUSE;
	else if (!Q_stricmp (token, "entitylighting"))
		pass->rgbgen = RGB_GEN_ENTITY_LIGHTING_DIFFUSE;
	else if (!Q_stricmp (token, "exactvertex"))
		pass->rgbgen = RGB_GEN_VERTEX_EXACT;
	else if (!Q_stricmp (token, "const") || !Q_stricmp (token, "constant")
		|| !Q_stricmp (token, "constLighting"))
	{
		pass->rgbgen = RGB_GEN_CONST;
		pass->rgbgen_func.type = SHADER_FUNC_CONSTANT;

		Shader_ParseVector (shader, ptr, pass->rgbgen_func.args);
	}
	else if (!Q_stricmp (token, "srgb") || !Q_stricmp (token, "srgbconst"))
	{
		pass->rgbgen = RGB_GEN_CONST;
		pass->rgbgen_func.type = SHADER_FUNC_CONSTANT;

		Shader_ParseVector (shader, ptr, pass->rgbgen_func.args);

		pass->rgbgen_func.args[0] = SRGBf(pass->rgbgen_func.args[0]);
		pass->rgbgen_func.args[1] = SRGBf(pass->rgbgen_func.args[1]);
		pass->rgbgen_func.args[2] = SRGBf(pass->rgbgen_func.args[2]);
	}
	else if (!Q_stricmp (token, "topcolor"))
		pass->rgbgen = RGB_GEN_TOPCOLOR;
	else if (!Q_stricmp (token, "bottomcolor"))
		pass->rgbgen = RGB_GEN_BOTTOMCOLOR;
}

static void Shaderpass_AlphaGen (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	char		*token;

	token = Shader_ParseString(ptr);
	if (!Q_stricmp (token, "portal"))
	{
		pass->alphagen = ALPHA_GEN_PORTAL;
		shader->portaldist = Shader_ParseFloat(shader, ptr, 256);
		if (!shader->portaldist)
			shader->portaldist = 256;
		shader->flags |= SHADER_AGEN_PORTAL;
	}
	else if (!Q_stricmp (token, "vertex"))
	{
		pass->alphagen = ALPHA_GEN_VERTEX;
	}
	else if (!Q_stricmp (token, "entity"))
	{
		pass->alphagen = ALPHA_GEN_ENTITY;
	}
	else if (!Q_stricmp (token, "wave"))
	{
		pass->alphagen = ALPHA_GEN_WAVE;

		Shader_ParseFunc (ps, "alphaGen wave", ptr, &pass->alphagen_func);
	}
	else if ( !Q_stricmp (token, "lightingspecular"))
	{
		pass->alphagen = ALPHA_GEN_SPECULAR;
	}
	else if ( !Q_stricmp (token, "const") || !Q_stricmp (token, "constant"))
	{
		pass->alphagen = ALPHA_GEN_CONST;
		pass->alphagen_func.type = SHADER_FUNC_CONSTANT;
		pass->alphagen_func.args[0] = fabs(Shader_ParseFloat(shader, ptr, 0));
	}
}
static void Shaderpass_AlphaShift (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	float speed;
	float min, max;
	pass->alphagen = ALPHA_GEN_WAVE;

	pass->alphagen_func.type = SHADER_FUNC_SIN;


	//arg0 = add
	//arg1 = scale
	//arg2 = timeshift
	//arg3 = timescale

	speed = Shader_ParseFloat(shader, ptr, 0);
	min = Shader_ParseFloat(shader, ptr, 0);
	max = Shader_ParseFloat(shader, ptr, 0);

	pass->alphagen_func.args[0] = min + (max - min)/2;
	pass->alphagen_func.args[1] = (max - min)/2;
	pass->alphagen_func.args[2] = 0;
	pass->alphagen_func.args[3] = 1/speed;
}

static int Shader_BlendFactor(char *name, qboolean dstnotsrc)
{
	int factor;
	if (!strnicmp(name, "gl_", 3))
		name += 3;

	if (!Q_stricmp(name, "zero"))
		factor = SBITS_SRCBLEND_ZERO;
	else if ( !Q_stricmp(name, "one"))
		factor = SBITS_SRCBLEND_ONE;
	else if (!Q_stricmp(name, "dst_color"))
		factor = SBITS_SRCBLEND_DST_COLOR;
	else if (!Q_stricmp(name, "one_minus_src_alpha"))
		factor = SBITS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	else if (!Q_stricmp(name, "src_alpha"))
		factor = SBITS_SRCBLEND_SRC_ALPHA;
	else if (!Q_stricmp(name, "src_color"))
		factor = SBITS_SRCBLEND_SRC_COLOR_INVALID;
	else if (!Q_stricmp(name, "one_minus_dst_color"))
		factor = SBITS_SRCBLEND_ONE_MINUS_DST_COLOR;
	else if (!Q_stricmp(name, "one_minus_src_color"))
		factor = SBITS_SRCBLEND_ONE_MINUS_SRC_COLOR_INVALID;
	else if (!Q_stricmp(name, "dst_alpha") )
		factor = SBITS_SRCBLEND_DST_ALPHA;
	else if (!Q_stricmp(name, "one_minus_dst_alpha"))
		factor = SBITS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	else
		factor = SBITS_SRCBLEND_NONE;

	if (dstnotsrc)
	{
		//dest factors are shifted
		factor <<= 4;

		/*gl doesn't accept dst_color for destinations*/
		if (factor == SBITS_DSTBLEND_NONE ||
			factor == SBITS_DSTBLEND_DST_COLOR_INVALID ||
			factor == SBITS_DSTBLEND_ONE_MINUS_DST_COLOR_INVALID ||
			factor == SBITS_DSTBLEND_ALPHA_SATURATE_INVALID)
		{
			Con_DPrintf("Invalid shader dst blend \"%s\"\n", name);
			factor = SBITS_DSTBLEND_ONE;
		}
	}
	else
	{
		/*gl doesn't accept src_color for sources*/
		if (factor == SBITS_SRCBLEND_NONE ||
			factor == SBITS_SRCBLEND_SRC_COLOR_INVALID ||
			factor == SBITS_SRCBLEND_ONE_MINUS_SRC_COLOR_INVALID)
		{
			Con_DPrintf("Unrecognised shader src blend \"%s\"\n", name);
			factor = SBITS_SRCBLEND_ONE;
		}
	}

	return factor;
}

static void Shaderpass_BlendFunc (parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	char		*token;

	if (pass->numMergedPasses>1)
		pass = ps->s->passes+ps->s->numpasses-1;	//nextbundle stuff.

	//reset to defaults
	pass->shaderbits &= ~(SBITS_BLEND_BITS);
	pass->stagetype = ST_AMBIENT;

	token = Shader_ParseString (ptr);
	if ( !Q_stricmp (token, "bumpmap"))				//doom3 is awkward...
		pass->stagetype = ST_BUMPMAP;
	else if ( !Q_stricmp (token, "specularmap"))	//doom3 is awkward...
		pass->stagetype = ST_SPECULARMAP;
	else if ( !Q_stricmp (token, "diffusemap"))		//doom3 is awkward...
		pass->stagetype = ST_DIFFUSEMAP;
	else if ( !Q_stricmp (token, "blend"))
		pass->shaderbits |= SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	else if ( !Q_stricmp (token, "premul"))			//gets rid of feathering.
		pass->shaderbits |= SBITS_SRCBLEND_ONE | SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	else if (!Q_stricmp (token, "filter"))
		pass->shaderbits |= SBITS_SRCBLEND_DST_COLOR | SBITS_DSTBLEND_ZERO;
	else if (!Q_stricmp (token, "add"))
		pass->shaderbits |= SBITS_SRCBLEND_ONE | SBITS_DSTBLEND_ONE;
	else if (!Q_stricmp (token, "replace"))
		pass->shaderbits |= SBITS_SRCBLEND_NONE | SBITS_DSTBLEND_NONE;
	else
	{
		pass->shaderbits |= Shader_BlendFactor(token, false);

		token = Shader_ParseString (ptr);
		if (*token == ',')
			token = Shader_ParseString (ptr);
		pass->shaderbits |= Shader_BlendFactor(token, true);
	}
}

static void Shaderpass_AlphaFunc (parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	char *token;

	pass->shaderbits &= ~SBITS_ATEST_BITS;

	token = Shader_ParseString (ptr);
	if (!Q_stricmp (token, "gt0"))
	{
		pass->shaderbits |= SBITS_ATEST_GT0;
	}
	else if (!Q_stricmp (token, "lt128"))
	{
		pass->shaderbits |= SBITS_ATEST_LT128;
	}
	else if (!Q_stricmp (token, "ge128"))
	{
		pass->shaderbits |= SBITS_ATEST_GE128;
	}
}

static void Shaderpass_DepthFunc (parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	char *token;

	pass->shaderbits &= ~(SBITS_DEPTHFUNC_BITS);

	token = Shader_ParseString (ptr);
	if (!Q_stricmp (token, "equal"))
		pass->shaderbits |= SBITS_DEPTHFUNC_EQUAL;
	else if (!Q_stricmp (token, "lequal"))
		pass->shaderbits |= SBITS_DEPTHFUNC_CLOSEREQUAL;	//default
	else if (!Q_stricmp (token, "less"))
		pass->shaderbits |= SBITS_DEPTHFUNC_CLOSER;
	else if (!Q_stricmp (token, "greater"))
		pass->shaderbits |= SBITS_DEPTHFUNC_FURTHER;
//	else if (!Q_stricmp (token, "gequal"))
//		pass->shaderbits |= SBITS_DEPTHFUNC_FURTHEREQUAL;
//	else if (!Q_stricmp (token, "nequal"))
//		pass->shaderbits |= SBITS_DEPTHFUNC_NOTEQUAL;
//	else if (!Q_stricmp (token, "never"))
//		pass->shaderbits |= SBITS_DEPTHFUNC_NEVER;
//	else if (!Q_stricmp (token, "always"))
//		pass->shaderbits |= SBITS_DEPTHFUNC_ALWAYS;
	else
		Con_DPrintf("Invalid depth func %s\n", token);
}

static void Shaderpass_DepthWrite (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	shader->flags |= SHADER_DEPTHWRITE;
	pass->shaderbits |= SBITS_MISC_DEPTHWRITE;
}

static void Shaderpass_NoDepthTest (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	shader->flags |= SHADER_DEPTHWRITE;
	pass->shaderbits |= SBITS_MISC_NODEPTHTEST;
}

static void Shaderpass_NoDepth (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shader->flags |= SHADER_DEPTHWRITE;
}

static void Shaderpass_TcMod (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	int i;
	tcmod_t *tcmod;
	char *token;

	if (pass->numMergedPasses>1)
		pass = ps->s->passes+ps->s->numpasses-1;	//nextbundle stuff.

	if (pass->numtcmods >= SHADER_MAX_TC_MODS)
	{
		return;
	}

	tcmod = &pass->tcmods[pass->numtcmods];

	token = Shader_ParseString (ptr);
	if (!Q_stricmp (token, "rotate"))
	{
		tcmod->args[0] = -Shader_ParseFloat(shader, ptr, 0) / 360.0f;
		if (!tcmod->args[0])
		{
			return;
		}

		tcmod->type = SHADER_TCMOD_ROTATE;
	}
	else if ( !Q_stricmp (token, "scale") )
	{
		tcmod->args[0] = Shader_ParseFloat (shader, ptr, 0);
		tcmod->args[1] = Shader_ParseFloat (shader, ptr, 0);
		tcmod->type = SHADER_TCMOD_SCALE;
	}
	else if ( !Q_stricmp (token, "scroll") )
	{
		tcmod->args[0] = Shader_ParseFloat (shader, ptr, 0);
		tcmod->args[1] = Shader_ParseFloat (shader, ptr, 0);
		tcmod->type = SHADER_TCMOD_SCROLL;
	}
	else if (!Q_stricmp(token, "stretch"))
	{
		shaderfunc_t func;

		Shader_ParseFunc(ps, "tcmod stretch", ptr, &func);

		tcmod->args[0] = func.type;
		for (i = 1; i < 5; ++i)
			tcmod->args[i] = func.args[i-1];
		tcmod->type = SHADER_TCMOD_STRETCH;
	}
	else if (!Q_stricmp (token, "transform"))
	{
		for (i = 0; i < 6; ++i)
			tcmod->args[i] = Shader_ParseFloat (shader, ptr, 0);
		tcmod->type = SHADER_TCMOD_TRANSFORM;
	}
	else if (!Q_stricmp (token, "turb"))
	{
		for (i = 0; i < 4; i++)
			tcmod->args[i] = Shader_ParseFloat (shader, ptr, 0);
		tcmod->type = SHADER_TCMOD_TURB;
	}
	else if (!Q_stricmp (token, "page"))
	{
		for (i = 0; i < 3; i++)
			tcmod->args[i] = Shader_ParseFloat (shader, ptr, 0);
		tcmod->type = SHADER_TCMOD_PAGE;
	}
//	else if (!Q_stricmp (token, "entityTranslate"))	//RTCW
//	else if (!Q_stricmp (token, "swap"))			//RTCW
	else
	{
		Con_DPrintf("Unknown tcmod %s in %s\n", token, shader->name);
		return;
	}

	pass->numtcmods++;
}

static void Shaderpass_Scale (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	//seperate x and y
	char *token;
	tcmod_t *tcmod;

	tcmod = &pass->tcmods[pass->numtcmods];

	tcmod->type = SHADER_TCMOD_SCALE;

	token = Shader_ParseString (ptr);
	if (!strcmp(token, "static"))
	{
		tcmod->args[0] = Shader_ParseFloat (shader, ptr, 0);
	}
	else
	{
		tcmod->args[0] = atof(token);
	}

	while (**ptr == ' ' || **ptr == '\t')
		*ptr+=1;
	if (**ptr == ',')
		*ptr+=1;

	token = Shader_ParseString (ptr);
	if (!strcmp(token, "static"))
	{
		tcmod->args[1] = Shader_ParseFloat (shader, ptr, 0);
	}
	else
	{
		tcmod->args[1] = atof(token);
	}

	pass->numtcmods++;
}

static void Shaderpass_Scroll (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	//seperate x and y
	char *token;
	tcmod_t *tcmod;

	tcmod = &pass->tcmods[pass->numtcmods];

	token = Shader_ParseString ( ptr );
	if (!strcmp(token, "static"))
	{
		tcmod->type = SHADER_TCMOD_SCROLL;
		tcmod->args[0] = Shader_ParseFloat (shader, ptr, 0);
	}
	else
	{
		Con_DPrintf("Bad shader scroll value\n");
		return;
	}

	token = Shader_ParseString ( ptr );
	if (!strcmp(token, "static"))
	{
		tcmod->type = SHADER_TCMOD_SCROLL;
		tcmod->args[1] = Shader_ParseFloat (shader, ptr, 0);
	}
	else
	{
		Con_DPrintf("Bad shader scroll value\n");
		return;
	}

	pass->numtcmods++;
}


static void Shaderpass_TcGen (parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	char *token;

	if (pass->numMergedPasses>1)
		pass = ps->s->passes+ps->s->numpasses-1;	//nextbundle stuff.

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "base") ) {
		pass->tcgen = TC_GEN_BASE;
	} else if ( !Q_stricmp (token, "lightmap") ) {
		pass->tcgen = TC_GEN_LIGHTMAP;
	} else if ( !Q_stricmp (token, "environment") ) {
		pass->tcgen = TC_GEN_ENVIRONMENT;
	} else if ( !Q_stricmp (token, "fireriseenv") ) {	//from RTCW
		pass->tcgen = TC_GEN_ENVIRONMENT;	//FIXME: not supported
	} else if ( !Q_stricmp (token, "vector") )
	{
		pass->tcgen = TC_GEN_VECTOR;
		Shader_ParseVector (shader, ptr, pass->tcgenvec[0]);
		Shader_ParseVector (shader, ptr, pass->tcgenvec[1]);
	} else if ( !Q_stricmp (token, "normal") ) {
		pass->tcgen = TC_GEN_NORMAL;
	} else if ( !Q_stricmp (token, "svector") ) {
		pass->tcgen = TC_GEN_SVECTOR;
	} else if ( !Q_stricmp (token, "tvector") ) {
		pass->tcgen = TC_GEN_TVECTOR;
	} else if ( !Q_stricmp (token, "skybox") ) {
		pass->tcgen = TC_GEN_SKYBOX;
	}
}
static void Shaderpass_EnvMap (parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	pass->tcgen = TC_GEN_ENVIRONMENT;
}

static void Shaderpass_Detail (parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	pass->flags |= SHADER_PASS_DETAIL;
}

static void Shaderpass_AlphaMask (parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	pass->shaderbits &= ~SBITS_ATEST_BITS;
	pass->shaderbits |= SBITS_ATEST_GE128;
}

static void Shaderpass_NoLightMap (parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	pass->rgbgen = RGB_GEN_IDENTITY;
}

static void Shaderpass_Red(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	pass->rgbgen = RGB_GEN_CONST;
	pass->rgbgen_func.args[0] = Shader_ParseFloat(shader, ptr, 0);
}
static void Shaderpass_Green(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	pass->rgbgen = RGB_GEN_CONST;
	pass->rgbgen_func.args[1] = Shader_ParseFloat(shader, ptr, 0);
}
static void Shaderpass_Blue(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	pass->rgbgen = RGB_GEN_CONST;
	pass->rgbgen_func.args[2] = Shader_ParseFloat(shader, ptr, 0);
}
static void Shaderpass_Alpha(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	pass->alphagen = ALPHA_GEN_CONST;
	pass->alphagen_func.args[0] = Shader_ParseFloat(shader, ptr, 0);
}
static void Shaderpass_MaskColor(parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	pass->shaderbits |= SBITS_MASK_RED|SBITS_MASK_GREEN|SBITS_MASK_BLUE;
}
static void Shaderpass_MaskRed(parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	pass->shaderbits |= SBITS_MASK_RED;
}
static void Shaderpass_MaskGreen(parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	pass->shaderbits |= SBITS_MASK_GREEN;
}
static void Shaderpass_MaskBlue(parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	pass->shaderbits |= SBITS_MASK_BLUE;
}
static void Shaderpass_MaskAlpha(parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	pass->shaderbits |= SBITS_MASK_ALPHA;
}
static void Shaderpass_AlphaTest(parsestate_t *ps, const char **ptr)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;
	if (Shader_ParseFloat(shader, ptr, 0) == 0.5)
		pass->shaderbits |= SBITS_ATEST_GE128;
	else
		Con_Printf("unsupported alphatest value\n");
}
static void Shaderpass_TexGen(parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	char *token = Shader_ParseString(ptr);
	if (!strcmp(token, "normal"))
		pass->tcgen = TC_GEN_NORMAL;
	else if (!strcmp(token, "skybox"))
		pass->tcgen = TC_GEN_SKYBOX;
	else if (!strcmp(token, "wobblesky"))
	{
		pass->tcgen = TC_GEN_WOBBLESKY;
		token = Shader_ParseString(ptr);
		token = Shader_ParseString(ptr);
		token = Shader_ParseString(ptr);
	}
	else if (!strcmp(token, "reflect"))
		pass->tcgen = TC_GEN_REFLECT;
	else
	{
		Con_Printf("texgen token not understood\n");
	}
}
static void Shaderpass_CubeMap(parsestate_t *ps, const char **ptr)
{
	shaderpass_t *pass = ps->pass;
	char *token = Shader_ParseString(ptr);

	if (pass->tcgen == TC_GEN_UNSPECIFIED)
		pass->tcgen = TC_GEN_SKYBOX;
	pass->anim_frames[0] = Shader_FindImage(ps, token, IF_TEXTYPE_CUBE);
	pass->texgen = T_GEN_SINGLEMAP;

	if (!TEXVALID(pass->anim_frames[0]))
	{
		pass->texgen = T_GEN_SINGLEMAP;
		pass->anim_frames[0] = missing_texture;
	}
}

static shaderkey_t shaderpasskeys[] =
{
#define Q3 NULL
	{"rgbgen",		Shaderpass_RGBGen,			Q3},
	{"alphagen",	Shaderpass_AlphaGen,		Q3},
	{"blendfunc",	Shaderpass_BlendFunc,		Q3},
	{"depthfunc",	Shaderpass_DepthFunc,		Q3},
	{"depthwrite",	Shaderpass_DepthWrite,		Q3},
	{"alphafunc",	Shaderpass_AlphaFunc,		Q3},
	{"tcmod",		Shaderpass_TcMod,			Q3},
	{"map",			Shaderpass_Map,				Q3},
	{"animmap",		Shaderpass_AnimMap,			Q3},
	{"clampmap",	Shaderpass_ClampMap,		Q3},
	{"videomap",	Shaderpass_VideoMap,		Q3},
	{"tcgen",		Shaderpass_TcGen,			Q3},
	{"texgen",		Shaderpass_TcGen,			Q3},
	{"detail",		Shaderpass_Detail,			Q3},

	{"nodepthtest",	Shaderpass_NoDepthTest,		NULL},
	{"nodepth",		Shaderpass_NoDepth,			NULL},

	{"envmap",		Shaderpass_EnvMap,			"rscript"},//for alienarena
	{"nolightmap",	Shaderpass_NoLightMap,		"rscript"},//for alienarena
	{"scale",		Shaderpass_Scale,			"rscript"},//for alienarena
	{"scroll",		Shaderpass_Scroll,			"rscript"},//for alienarena
	{"alphashift",	Shaderpass_AlphaShift,		"rscript"},//for alienarena
	{"alphamask",	Shaderpass_AlphaMask,		"rscript"},//for alienarena

	{"program",		Shaderpass_ProgramName,		"fte"},
	
	/*doom3 compat*/
	{"blend",		Shaderpass_BlendFunc,		"doom3"},
	{"maskcolor",	Shaderpass_MaskColor,		"doom3"},
	{"maskred",		Shaderpass_MaskRed,			"doom3"},
	{"maskgreen",	Shaderpass_MaskGreen,		"doom3"},
	{"maskblue",	Shaderpass_MaskBlue,		"doom3"},
	{"maskalpha",	Shaderpass_MaskAlpha,		"doom3"},
	{"alphatest",	Shaderpass_AlphaTest,		"doom3"},
	{"texgen",		Shaderpass_TexGen,			"doom3"},
	{"cubemap",		Shaderpass_CubeMap,			"doom3"},	//one of these is wrong
	{"cameracubemap",Shaderpass_CubeMap,		"doom3"},	//one of these is wrong
	{"red",			Shaderpass_Red,				"doom3"},
	{"green",		Shaderpass_Green,			"doom3"},
	{"blue",		Shaderpass_Blue,			"doom3"},
	{"alpha",		Shaderpass_Alpha,			"doom3"},


	//RTCW
	//fancy map lines use the map if that mode is active.
	//FIXME: actually check these to ensure there's no issues with any shaders overriding the pass's previously specified map
	//      (hopefully no shaders would actually do that due to the engine loading both textures, which would be wasteful)
	{"map16",		Shaderpass_RTCW_Map_16bit,		"rtcw"},
	{"map32",		Shaderpass_RTCW_Map_32bit,		"rtcw"},
	{"mapcomp",		Shaderpass_RTCW_Map_s3tc,		"rtcw"},
	{"mapnocomp",	Shaderpass_RTCW_Map_nos3tc,		"rtcw"},
	{"animcompmap",	Shaderpass_RTCW_AnimMap_s3tc,	"rtcw"},
	{"animnocompmap",Shaderpass_RTCW_AnimMap_nos3tc,"rtcw"},

	//qfusion/warsow compat
	{"material",	Shaderpass_QF_Material,		"qf"},
	{"animclampmap",Shaderpass_QF_AnimClampMap,	"qf"},
//	{"cubemap",		Shaderpass_QF_CubeMap,		"qf"},
//	{"shadecubemap",Shaderpass_QF_ShadeCubeMap,	"qf"},
	{"surroundmap",	Shaderpass_CubeMap,			"qf"},
//	{"distortion",	Shaderpass_QF_Distortion,	"qf"},
//	{"celshade",	Shaderpass_QF_Celshade,		"qf"},
//	{"grayscale",	Shaderpass_QF_Greyscale,	"qf"},
//	{"greyscale",	Shaderpass_QF_Greyscale,	"qf"},
//	{"skip",		Shaderpass_QF_Skip,			"qf"},

	{"nextbundle",			Shaderpass_CoD_NextBundle,			"cod"},
	{"requires",			Shaderpass_CoD_Requires,			"cod"},
	{"texEnvCombine",		Shaderpass_CoD_texEnvCombine,		"cod"},
	{"nvRegCombiners",		Shaderpass_CoD_nvRegCombiners,		"cod"},
	{"atiFragmentShader",	Shaderpass_CoD_atiFragmentShader,	"cod"},

	{NULL,			NULL}
};

// ===============================================================


void Shader_FreePass (shaderpass_t *pass)
{
#ifdef HAVE_MEDIA_DECODER
	if ( pass->flags & SHADER_PASS_VIDEOMAP )
	{
		Media_ShutdownCin(pass->cin);
		pass->cin = NULL;
	}
#endif

	if (pass->prog)
	{
		Shader_ReleaseGeneric(pass->prog);
		pass->prog = NULL;
	}
}

void Shader_ReleaseGeneric(program_t *prog)
{
	if (prog)
		if (prog->refs-- == 1)
			Shader_UnloadProg(prog);
}
void Shader_Free (shader_t *shader)
{
	int i;
	shaderpass_t *pass;

	if (shader->bucket.data == shader)
		Hash_RemoveData(&shader_active_hash, shader->name, shader);
	shader->bucket.data = NULL;

	Shader_ReleaseGeneric(shader->prog);
	shader->prog = NULL;

	if (shader->skydome)
		Z_Free (shader->skydome);
	shader->skydome = NULL;
	while (shader->clutter)
	{
		void *t = shader->clutter;
		shader->clutter = shader->clutter->next;
		Z_Free(t);
	}

	pass = shader->passes;
	for (i = 0; i < shader->numpasses; i++, pass++)
	{
		Shader_FreePass (pass);
	}
	shader->numpasses = 0;

	if (shader->genargs)
	{
		free(shader->genargs);
		shader->genargs = NULL;
	}
	shader->uses = 0;

	Z_Free(shader->defaulttextures);
	shader->defaulttextures = NULL;
}





int QDECL Shader_InitCallback (const char *name, qofs_t size, time_t mtime, void *param, searchpathfuncs_t *spath)
{
	Shader_MakeCache(name, SPF_DEFAULT);
	return true;
}
int QDECL Shader_InitCallback_Doom3 (const char *name, qofs_t size, time_t mtime, void *param, searchpathfuncs_t *spath)
{
	Shader_MakeCache(name, SPF_DOOM3);
	return true;
}

qboolean Shader_Init (void)
{
	int wibuf[16];

	if (!r_shaders)
	{
		r_numshaders = 0;
		r_maxshaders = 256;
		r_shaders = calloc(r_maxshaders, sizeof(*r_shaders));

		shader_hash = calloc (HASH_SIZE, sizeof(*shader_hash));

		shader_active_hash_mem = malloc(Hash_BytesForBuckets(1024));
		memset(shader_active_hash_mem, 0, Hash_BytesForBuckets(1024));
		Hash_InitTable(&shader_active_hash, 1024, shader_active_hash_mem);

		Shader_FlushGenerics();

		if (!sh_config.progs_supported)
			sh_config.max_gpu_bones = 0;
		else
		{
			extern cvar_t r_max_gpu_bones;
			if (!*r_max_gpu_bones.string)
			{
#ifdef FTE_TARGET_WEB
				sh_config.max_gpu_bones = 0;	//webgl tends to crap out if this is too high, so 32 is a good enough value to play safe. some browsers have really shitty uniform performance too, so lets just default to pure-cpu transforms. in javascript. yes, its that bad.
#else
				//some of our APIs will set their own guesses from queries. don't stomp on that.
				if (!sh_config.max_gpu_bones)
					sh_config.max_gpu_bones = 64;	//ATI drivers bug out and start to crash if you put this at 128.
#endif
			}
			else
				sh_config.max_gpu_bones = bound(0, r_max_gpu_bones.ival, MAX_BONES);
		}
	}
	
	if (!qrenderer)
		r_whiteimage = r_nulltex;
	else
	{
		memset(wibuf, 0xff, sizeof(wibuf));
		r_whiteimage = R_LoadTexture("$whiteimage", 4, 4, TF_RGBA32, wibuf, IF_NOMIPMAP|IF_NOPICMIP|IF_NEAREST|IF_NOGAMMA|IF_NOPURGE);
		memset(wibuf, 0, sizeof(wibuf));
		r_blackimage = R_LoadTexture("$blackimage", 4, 4, TF_RGBA32, wibuf, IF_NOMIPMAP|IF_NOPICMIP|IF_NEAREST|IF_NOGAMMA|IF_NOPURGE);
	}

	Shader_NeedReload(true);
	Shader_DoReload();
	return true;
}

void Shader_FlushCache(void)
{
	shadercachefile_t *sf;
	shadercache_t *cache, *cache_next;
	int i;

	for (i = 0; i < HASH_SIZE; i++)
	{
		cache = shader_hash[i];
		shader_hash[i] = NULL;

		for (; cache; cache = cache_next)
		{
			cache_next = cache->hash_next;
			cache->hash_next = NULL;
			Z_Free(cache);
		}
	}

	while(shaderfiles)
	{
		sf = shaderfiles;
		shaderfiles = sf->next;
		if (sf->data)
			FS_FreeFile(sf->data);
		Z_Free(sf);
	}
}

static void Shader_MakeCache(const char *path, unsigned int parseflags)
{
	unsigned int key;
	const char *buf, *ptr;
	char *token;
	shadercache_t *cache;
	shadercachefile_t *cachefile, *filelink = NULL;
	qofs_t size;

	for (cachefile = shaderfiles; cachefile; cachefile = cachefile->next)
	{
		if (!Q_stricmp(cachefile->name, path))
			return;	//already loaded. there's no source package or anything.
		filelink = cachefile;
	}

	
	Con_DPrintf ("...loading '%s'\n", path);

	cachefile = Z_Malloc(sizeof(*cachefile) + strlen(path));
	strcpy(cachefile->name, path);
	size = FS_LoadFile(path, (void **)&cachefile->data);
	cachefile->length = size;
	cachefile->parseflags = parseflags;
	if (filelink)
		filelink->next = cachefile;
	else
		shaderfiles = cachefile;

	if (qofs_Error(size))
	{
		Con_Printf("Unable to read %s\n", path);
		cachefile->length = 0;
		return;
	}
	if (size > 1024*1024*64)	//sanity limit
	{
		Con_Printf("Refusing to parse %s due to size\n", path);
		cachefile->length = 0;
		FS_FreeFile(cachefile->data);
		cachefile->data = NULL;
		return;
	}

	ptr = buf = cachefile->data;
	size = cachefile->length;

	//look for meta comments.
	while (1)
	{
		//parse metas
		while (*ptr == ' ' || *ptr == '\t')
			ptr++;
		if (ptr[0] == '\r' && ptr[1] == '\n')
			ptr+=2;	//blank line with dos ending
		else if (ptr[0] == '\r' || ptr[0] == '\n')
			ptr+=1;	//blank line with mac or unix ending
		else if (ptr[0] == '/' && ptr[1] == '/')
		{
			const char *e = strchr(ptr, '\n');
			if (e)
				e++;
			else
				e = ptr + strlen(ptr);

			ptr += 2;
			while (*ptr == ' ' || *ptr == '\t')
				ptr++;
			if (!strncmp(ptr, "meta:", 5))
			{
				ptr+=5;

				token = COM_ParseExt (&ptr, false, true);
				if (!strcmp(token, "forceprogramify"))
				{
					cachefile->parseflags |= SPF_PROGRAMIFY;
					token = COM_ParseExt (&ptr, false, true);
					if (*token)
						Q_strncpyz(cachefile->forcedshadername, token, sizeof(cachefile->forcedshadername));
				}
				else
					Con_DPrintf("unknown shader meta term \"%s\" in %s\n", token, path);

				while (*ptr == ' ' || *ptr == '\t')
					ptr++;
				if (*ptr != '\r' && *ptr != '\n')
				{
					while (*ptr && (*ptr != '\r' && *ptr != '\n'))
						ptr++;
					Con_DPrintf("junk after shader meta in %s\n", path);
				}
			}
			ptr = e;
		}
		else
			break;	//the actual shader started.
	}

	//now scan the file looking for each individual shader.
	do
	{
		if ( ptr - buf >= size )
			break;

		token = COM_ParseExt (&ptr, true, true);
		if ( !token[0] || ptr - buf >= size )
			break;

		COM_CleanUpPath(token);

		if (Shader_LocateSource(token, NULL, NULL, NULL, NULL))
		{
			ptr = Shader_Skip (path, token, ptr);
			continue;
		}

		key = Hash_Key ( token, HASH_SIZE );

		cache = ( shadercache_t * )Z_Malloc(sizeof(shadercache_t) + strlen(token));
		strcpy(cache->name, token);
		cache->hash_next = shader_hash[key];
		cache->source = cachefile;
		cache->offset = ptr - cachefile->data;

		shader_hash[key] = cache;

		ptr = Shader_Skip (path, cache->name, ptr);
	} while ( ptr );
}

static qboolean Shader_LocateSource(const char *name, const char **buf, size_t *bufsize, size_t *offset, shadercachefile_t **sourcefile)
{
	unsigned int key;
	shadercache_t *cache;

	key = Hash_Key ( name, HASH_SIZE );
	cache = shader_hash[key];

	for ( ; cache; cache = cache->hash_next )
	{
		if ( !Q_stricmp (cache->name, name) )
		{
			if (buf)
			{
				*buf = cache->source->data;
				*bufsize = cache->source->length;
				*offset = cache->offset;
				*sourcefile = cache->source;
			}
			return true;
		}
	}
	return false;
}

static const char *Shader_Skip(const char *file, const char *shadername, const char *ptr)
{
	char *tok;
	int brace_count;

    // Opening brace
	tok = COM_ParseExt(&ptr, true, true);

	if (!ptr)
		return NULL;

	if ( tok[0] != '{' )
	{
		tok = COM_ParseExt (&ptr, true, true);
	}

	for (brace_count = 1; brace_count > 0; )
	{
		tok = COM_ParseExt (&ptr, true, true);

		if ( !tok[0] )
		{
			Con_Printf(CON_WARNING"%s: unexpected EOF parsing %s\n", file, shadername);
			return NULL;
		}

		if (tok[0] == '{')
		{
			if (r_forceprogramify.ival > 1 && brace_count == 2)
				Con_Printf(CON_WARNING"%s: excess indentation depth while parsing shader \"%s\" (%s==%i)\n", file, shadername, r_forceprogramify.name, r_forceprogramify.ival);
			else
				brace_count++;
		} else if (tok[0] == '}')
		{
			brace_count--;
		}
	}

	return ptr;
}

static void Shader_Reset(parsestate_t *ps)
{
	shader_t *s = ps->s;
	extern cvar_t r_refractreflect_scale;
	char name[MAX_QPATH];
	int id = s->id;
	int uses = s->uses;
	shader_gen_t *defaultgen = s->generator;
	char *genargs = s->genargs;
	texnums_t *dt = s->defaulttextures;
	int dtcount = s->numdefaulttextures;
	float dtrate = s->defaulttextures_fps;	//FIXME!
	int w = s->width;
	int h = s->height;
	model_t *mod = s->model;
	unsigned int uf = s->usageflags;
	Q_strncpyz(name, s->name, sizeof(name));
	s->genargs = NULL;
	s->defaulttextures = NULL;
	Shader_Free(s);
	memset(s, 0, sizeof(*s));
	s->portalfboscale = r_refractreflect_scale.value;	//unless otherwise specified, this cvar specifies the value.

	s->remapto = s;
	s->id = id;
	s->width = w;
	s->height = h;
	s->model = mod;
	s->defaulttextures = dt;
	s->numdefaulttextures = dtcount;
	s->defaulttextures_fps = dtrate;
	s->generator = defaultgen;
	s->genargs = genargs;
	s->usageflags = uf;
	s->uses = uses;
	Q_strncpyz(s->name, name, sizeof(s->name));
	Hash_Add(&shader_active_hash, s->name, s, &s->bucket);
}

static void Shader_Regenerate(parsestate_t *ps, const char *shortname)
{
	Shader_Reset(ps);

	if (!strcmp(shortname, "textures/common/clip") || !strcmp(shortname, "textures/common/nodraw") || !strcmp(shortname, "common/nodraw"))
		Shader_DefaultScript(ps, shortname,
			"{\n"
				"surfaceparm nodraw\n"
				"surfaceparm nodlight\n"
			"}\n");
	else
		ps->s->generator(ps, shortname, ps->s->genargs);
}

void Shader_Shutdown (void)
{
	int i;
	shader_t *shader;

	if (!r_shaders)
		return;	/*nothing needs freeing yet*/
	for (i = 0; i < r_numshaders; i++)
	{
		shader = r_shaders[i];
		if (!shader)
			continue;

		Shader_Free(shader);
		Z_Free(r_shaders[i]);
		r_shaders[i] = NULL;
	}

	Shader_FlushCache();
	Shader_FlushGenerics();

	R_SkyShutdown();

	r_maxshaders = 0;
	r_numshaders = 0;

	free(r_shaders);
	r_shaders = NULL;
	free(shader_hash);
	shader_hash = NULL;
	free(shader_active_hash_mem);
	shader_active_hash_mem = NULL;

	shader_reload_needed = false;
}

static void Shader_SetBlendmode (shaderpass_t *pass, shaderpass_t *lastpass)
{
	qboolean lightmapoverbright;
	if (pass->texgen == T_GEN_DELUXMAP)
	{
		pass->blendmode = PBM_DOTPRODUCT;
		return;
	}

	if (pass->texgen < T_GEN_DIFFUSE && !TEXVALID(pass->anim_frames[0]) && !(pass->flags & SHADER_PASS_LIGHTMAP))
	{
		pass->blendmode = PBM_MODULATE;
		return;
	}

	if (!(pass->shaderbits & SBITS_BLEND_BITS))
	{
		if (pass->texgen == T_GEN_LIGHTMAP && lastpass)
			pass->blendmode = PBM_OVERBRIGHT;
		else if ((pass->rgbgen == RGB_GEN_IDENTITY) && (pass->alphagen == ALPHA_GEN_IDENTITY))
		{
			pass->blendmode = PBM_REPLACE;
			return;
		}
		else if ((pass->rgbgen == RGB_GEN_IDENTITY_LIGHTING) && (pass->alphagen == ALPHA_GEN_IDENTITY))
		{
			pass->shaderbits &= ~SBITS_BLEND_BITS;
			pass->shaderbits |= SBITS_SRCBLEND_ONE;
			pass->shaderbits |= SBITS_DSTBLEND_ZERO;
			pass->blendmode = PBM_REPLACELIGHT;
		}
		else
		{
			pass->shaderbits &= ~SBITS_BLEND_BITS;
			pass->shaderbits |= SBITS_SRCBLEND_ONE;
			pass->shaderbits |= SBITS_DSTBLEND_ZERO;
			pass->blendmode = PBM_MODULATE;
		}
		return;
	}

	lightmapoverbright = pass->texgen == T_GEN_LIGHTMAP || (lastpass && lastpass->texgen == T_GEN_LIGHTMAP && lastpass->blendmode != PBM_OVERBRIGHT);

	if (((pass->shaderbits&SBITS_BLEND_BITS) == (SBITS_SRCBLEND_ZERO|SBITS_DSTBLEND_SRC_COLOR)) ||
		((pass->shaderbits&SBITS_BLEND_BITS) == (SBITS_SRCBLEND_DST_COLOR|SBITS_DSTBLEND_ZERO)) ||
		((pass->shaderbits&SBITS_BLEND_BITS) == (SBITS_SRCBLEND_DST_COLOR|SBITS_DSTBLEND_ONE_MINUS_DST_ALPHA)))
		pass->blendmode = lightmapoverbright?PBM_OVERBRIGHT:PBM_MODULATE;
	else if ((pass->shaderbits&SBITS_BLEND_BITS) == (SBITS_SRCBLEND_ONE|SBITS_DSTBLEND_ONE))
		pass->blendmode = PBM_ADD;
	else if ((pass->shaderbits&SBITS_BLEND_BITS) == (SBITS_SRCBLEND_SRC_ALPHA|SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
		pass->blendmode = PBM_DECAL;
	else
		pass->blendmode = lightmapoverbright?PBM_OVERBRIGHT:PBM_MODULATE;
}

static void Shader_FixupProgPasses(parsestate_t *ps, shaderpass_t *pass)
{
	shader_t *shader = ps->s;
	int i;
	int maxpasses = SHADER_PASS_MAX - (pass-shader->passes);
	struct
	{
		int gen;
		unsigned int flags;
	} defaulttgen[] =
	{
		//light
		{T_GEN_SHADOWMAP,		0},						//0
		{T_GEN_LIGHTCUBEMAP,	0},						//1

		//material
		{T_GEN_DIFFUSE,			SHADER_HASDIFFUSE},		//2
		{T_GEN_NORMALMAP,		SHADER_HASNORMALMAP},	//3
		{T_GEN_SPECULAR,		SHADER_HASGLOSS},		//4
		{T_GEN_UPPEROVERLAY,	SHADER_HASTOPBOTTOM},	//5
		{T_GEN_LOWEROVERLAY,	SHADER_HASTOPBOTTOM},	//6
		{T_GEN_FULLBRIGHT,		SHADER_HASFULLBRIGHT},	//7
		{T_GEN_PALETTED,		SHADER_HASPALETTED},	//8
		{T_GEN_REFLECTCUBE,		SHADER_HASREFLECTCUBE},	//9
		{T_GEN_REFLECTMASK,		0},						//10
		{T_GEN_DISPLACEMENT,	SHADER_HASDISPLACEMENT},//11
		{T_GEN_OCCLUSION,		0},						//12
		{T_GEN_TRANSMISSION,	0},						//13
		{T_GEN_THICKNESS,		0},						//14
//			{T_GEN_REFLECTION,		SHADER_HASREFLECT},		//
//			{T_GEN_REFRACTION,		SHADER_HASREFRACT},		//
//			{T_GEN_REFRACTIONDEPTH,	SHADER_HASREFRACTDEPTH},//
//			{T_GEN_RIPPLEMAP,		SHADER_HASRIPPLEMAP},	//

		//batch
		{T_GEN_LIGHTMAP,		SHADER_HASLIGHTMAP},	//15
		{T_GEN_DELUXMAP,		0},						//16
		//more lightmaps								//17,18,19
		//mode deluxemaps								//20,21,22
	};

#ifdef HAVE_MEDIA_DECODER
	cin_t *cin = R_ShaderGetCinematic(shader);
#endif

	//if the glsl doesn't specify all samplers, just trim them.
	pass->numMergedPasses = pass->prog->numsamplers;

#ifdef HAVE_MEDIA_DECODER
	if (cin && R_ShaderGetCinematic(shader) == cin)
		cin = NULL;
#endif

	//if the glsl has specific textures listed, be sure to provide a pass for them.
	for (i = 0; i < sizeof(defaulttgen)/sizeof(defaulttgen[0]); i++)
	{
		if (pass->prog->defaulttextures & (1u<<i))
		{
			if (pass->numMergedPasses >= maxpasses)
			{	//panic...
				ps->droppass = true;
				break;
			}
			pass[pass->numMergedPasses].flags |= SHADER_PASS_NOCOLORARRAY;
			pass[pass->numMergedPasses].flags &= ~SHADER_PASS_DEPTHCMP;
			if (defaulttgen[i].gen == T_GEN_SHADOWMAP)
				pass[pass->numMergedPasses].flags |= SHADER_PASS_DEPTHCMP;
#ifdef HAVE_MEDIA_DECODER
			if (!i && cin)
			{
				pass[pass->numMergedPasses].texgen = T_GEN_VIDEOMAP;
				pass[pass->numMergedPasses].cin = cin;
				cin = NULL;
			}
			else
#endif
			{
				pass[pass->numMergedPasses].texgen = defaulttgen[i].gen;
#ifdef HAVE_MEDIA_DECODER
				pass[pass->numMergedPasses].cin = NULL;
#endif
			}
			pass->numMergedPasses++;
			shader->flags |= defaulttgen[i].flags;
		}
	}

	//must have at least one texture.
	if (!pass->numMergedPasses)
	{
#ifdef HAVE_MEDIA_DECODER
		pass[0].texgen = cin?T_GEN_VIDEOMAP:T_GEN_DIFFUSE;
		pass[0].cin = cin;
#else
		pass[0].texgen = T_GEN_DIFFUSE;
#endif
		pass->numMergedPasses = 1;
	}
#ifdef HAVE_MEDIA_DECODER
	else if (cin)
		Media_ShutdownCin(cin);
#endif

	shader->numpasses = (pass-shader->passes)+pass->numMergedPasses;
}

struct scondinfo_s
{
	int depth;
	int level[8];
#define COND_IGNORE			1
#define COND_IGNOREPARENT	2
#define COND_ALLOWELSE		4
#define COND_TAKEN			8
};
static qboolean Shader_Conditional_Read(parsestate_t *ps, struct scondinfo_s *cond, const char *token, const char **ptr)
{
	shader_t *shader = ps->s;
	if (ps->parseflags & SPF_DOOM3)
		return false;	//doom materials have conditionals that remove passes, without endifs. don't misparse here.
	else if (!Q_stricmp(token, "if"))
	{
		if (cond->depth+1 == countof(cond->level))
		{
			Con_Printf("if statements nest too deeply in shader %s\n", shader->name);
			*ptr += strlen(*ptr);
			return true;
		}
		cond->depth++;
		cond->level[cond->depth] = (Shader_EvaluateCondition(shader, ptr)?0:COND_IGNORE);
		cond->level[cond->depth] |= COND_ALLOWELSE;
		if (cond->level[cond->depth-1] & (COND_IGNORE|COND_IGNOREPARENT))
			cond->level[cond->depth] |= COND_IGNOREPARENT;	//if ignoring the parent, ignore this one too, even if valid
		if (!(cond->level[cond->depth] & (COND_IGNORE|COND_IGNOREPARENT)))
			cond->level[cond->depth] |= COND_TAKEN;		//if we're not ignoring the contained commands then flag it so we don't take any elifs/elses
	}
	else if (!Q_stricmp(token, "elif"))
	{
		if (cond->level[cond->depth] & COND_ALLOWELSE)
		{
			if (cond->level[cond->depth] & COND_TAKEN)
			{	//if we took the if/elif then don't take this elif either
				Shader_EvaluateCondition(shader, ptr);
				cond->level[cond->depth] = COND_ALLOWELSE|COND_TAKEN|COND_IGNORE;
			}
			else
			{
				cond->level[cond->depth] = (Shader_EvaluateCondition(shader, ptr)?0:COND_IGNORE);
				cond->level[cond->depth] |= COND_ALLOWELSE;
			}
			if (cond->level[cond->depth-1] & (COND_IGNORE|COND_IGNOREPARENT))
				cond->level[cond->depth] |= COND_IGNOREPARENT;
			if (!(cond->level[cond->depth] & (COND_IGNORE|COND_IGNOREPARENT)))
				cond->level[cond->depth] |= COND_TAKEN;
		}
		else
		{
			Con_Printf(CON_WARNING"unexpected elif statement in shader %s\n", shader->name);
			*ptr += strlen(*ptr);
		}
	}
	else if (!Q_stricmp(token, "endif"))
	{
		if (!cond->depth)
		{
			Con_Printf("endif without if in shader %s\n", shader->name);
			*ptr += strlen(*ptr);
			return true;
		}
		cond->depth--;
	}
	else if (!Q_stricmp(token, "else"))
	{
		if (cond->level[cond->depth] & COND_ALLOWELSE)
		{
			if (cond->level[cond->depth] & COND_TAKEN)
				cond->level[cond->depth] |= COND_IGNORE;
			else
				cond->level[cond->depth] ^= COND_IGNORE;
			cond->level[cond->depth] &= ~COND_ALLOWELSE;
		}
		else
		{
			Con_Printf(CON_WARNING"unexpected else statement in shader %s\n", shader->name);
			*ptr += strlen(*ptr);
		}
	}
	else if (cond->level[cond->depth] & (COND_IGNORE|COND_IGNOREPARENT))
	{
		//eat it
		while (ptr)
		{
			token = COM_ParseExt(ptr, false, true);
			if ( !token[0] )
				break;
		}
	}
	else
		return false;
	return true;
}

static void Shader_BeginPass(parsestate_t *ps)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass;
	static shader_t dummy;
	ps->oldflags = shader->flags;

	if ( shader->numpasses >= SHADER_PASS_MAX )
	{
		ps->droppass = true;
		shader = &dummy;
		shader->numpasses = 1;
		pass = shader->passes;
	}
	else
	{
		ps->droppass = false;
		pass = &shader->passes[shader->numpasses++];
	}

	// Set defaults
	pass->flags = 0;
	pass->anim_frames[0] = r_nulltex;
	pass->anim_numframes = 0;
	pass->rgbgen = RGB_GEN_UNKNOWN;
	pass->alphagen = ALPHA_GEN_UNDEFINED;
	pass->tcgen = TC_GEN_UNSPECIFIED;
	pass->numtcmods = 0;
	pass->stagetype = ST_AMBIENT;
	pass->numMergedPasses = 0;

	if (shader->flags & SHADER_NOMIPMAPS)
		pass->flags |= SHADER_PASS_NOMIPMAP;

	ps->pass = pass;
}
static void Shader_EndPass(parsestate_t *ps)
{
	shader_t *shader = ps->s;
	shaderpass_t *pass = ps->pass;

	if (pass->alphagen == ALPHA_GEN_UNDEFINED)
		pass->alphagen = ALPHA_GEN_IDENTITY;

	//if there was no texgen, then its too late now.
	if (!pass->numMergedPasses)
		pass->numMergedPasses = 1;

	if (pass->tcgen == TC_GEN_UNSPECIFIED)
		pass->tcgen = TC_GEN_BASE;

	if (!ps->droppass)
	{
		switch(pass->stagetype)
		{
		case ST_DIFFUSEMAP:
			if (pass->texgen == T_GEN_SINGLEMAP)
				shader->defaulttextures->base = pass->anim_frames[0];
			break;
		case ST_AMBIENT:
			break;
		case ST_BUMPMAP:
			if (pass->texgen == T_GEN_SINGLEMAP)
				shader->defaulttextures->bump = pass->anim_frames[0];
			ps->droppass = true;	//fixme: scrolling etc may be important. but we're not doom3.
			break;
		case ST_SPECULARMAP:
			if (pass->texgen == T_GEN_SINGLEMAP)
				shader->defaulttextures->specular = pass->anim_frames[0];
			ps->droppass = true;	//fixme: scrolling etc may be important. but we're not doom3.
			break;
		}
	}

	// check some things

	if (!ps->droppass)
		if ((pass->shaderbits&SBITS_BLEND_BITS) == (SBITS_SRCBLEND_ONE|SBITS_DSTBLEND_ZERO))
		{
			pass->shaderbits |= SBITS_MISC_DEPTHWRITE;
			shader->flags |= SHADER_DEPTHWRITE;
		}

	switch (pass->rgbgen)
	{
		case RGB_GEN_IDENTITY_LIGHTING:
		case RGB_GEN_IDENTITY:
		case RGB_GEN_CONST:
		case RGB_GEN_WAVE:
		case RGB_GEN_ENTITY:
		case RGB_GEN_ONE_MINUS_ENTITY:
		case RGB_GEN_UNKNOWN:	// assume RGB_GEN_IDENTITY or RGB_GEN_IDENTITY_LIGHTING

			switch (pass->alphagen)
			{
				case ALPHA_GEN_IDENTITY:
				case ALPHA_GEN_CONST:
				case ALPHA_GEN_WAVE:
				case ALPHA_GEN_ENTITY:
					pass->flags |= SHADER_PASS_NOCOLORARRAY;
					break;
				default:
					break;
			}

			break;
		default:
			break;
	}

	/*if ((shader->flags & SHADER_SKY) && (shader->flags & SHADER_DEPTHWRITE))
	{
#ifdef warningmsg
#pragma warningmsg("is this valid?")
#endif
		pass->shaderbits &= ~SBITS_MISC_DEPTHWRITE;
	}
	*/

	//if this pass specified a program, make sure it has all the textures that the program requires
	if (!ps->droppass && pass->prog)
		Shader_FixupProgPasses(ps, pass);

	if (ps->droppass)
	{
		while (pass->numMergedPasses > 0)
		{
			Shader_FreePass (pass+--pass->numMergedPasses);
			shader->numpasses--;
		}
		shader->flags = ps->oldflags;
	}
	ps->pass = NULL;
}
static void Shader_Readpass (parsestate_t *ps)
{
	shader_t *shader = ps->s;
	const char *token;
	struct scondinfo_s cond = {0};

	Shader_BeginPass(ps);

	while ( ps->ptr )
	{
		token = COM_ParseExt (&ps->ptr, true, true);

		if ( !token[0] )
		{
			continue;
		}
		else if (!Shader_Conditional_Read(ps, &cond, token, &ps->ptr))
		{
			if ( token[0] == '}' )
				break;
			else if (token[0] == '{')
			{
				int depth = 1;
				Con_Printf(CON_WARNING"%s: unexpected indentation in %s\n", ps->sourcename, shader->name);
				while (*(token = COM_ParseExt (&ps->ptr, true, true)))
				{	//extra parsing to even out this unexpected brace without extra warnings.
					if (token[0] == '}')
					{
						if (depth--==0)
							break;
					}
					else if (token[0] == '{')
						depth++;	//crap.
				}
			}
			else if ( Shader_Parsetok (ps, shaderpasskeys, token) )
				break;
		}
	}

	if (cond.depth)
	{
		Con_Printf("if statements without endif in shader %s\n", shader->name);
	}

	Shader_EndPass(ps);
}

//we've read the first token, now make sense of it and any args
static qboolean Shader_Parsetok(parsestate_t *ps, shaderkey_t *keys, const char *token)
{
	shaderkey_t *key;
	const char *prefix;
	qboolean toolchainprefix = false;

	if (*token == '_')
	{	//forward compat: make sure there's a way to shut stuff up if you're using future extensions in an outdated engine.
		token++;
		toolchainprefix = true;
	}

	//handle known prefixes.
	if		(!Q_strncasecmp(token, "fte", 3))		{prefix = token; token += 3; }
	else if	(!Q_strncasecmp(token, "dp", 2))		{prefix = token; token += 2; }
	else if (!Q_strncasecmp(token, "doom3", 5))		{prefix = token; token += 5; }
	else if (!Q_strncasecmp(token, "rscript", 7))	{prefix = token; token += 7; }
	else if (!Q_strncasecmp(token, "qer_", 4))		{prefix = token; token += 3; toolchainprefix = true; }
	else if (!Q_strncasecmp(token, "q3map_", 6))	{prefix = token; token += 5; toolchainprefix = true; }
	else if (!Q_strncasecmp(token, "vmap_", 6))		{prefix = token; token += 4; toolchainprefix = true; }
	else	prefix = NULL;
	if (prefix && *token == '_')
		token++;

	for (key = keys; key->keyword != NULL; key++)
	{
		if (!Q_stricmp (token, key->keyword))
		{
			if (!prefix || (prefix && key->prefix && !Q_strncasecmp(prefix, key->prefix, strlen(key->prefix))))
			{
				if (key->func)
					key->func ( ps, &ps->ptr );

				return (ps->ptr && *ps->ptr == '}' );
			}
		}
	}

	if (!toolchainprefix)	//we don't really give a damn about prefixes owned by various toolchains - they shouldn't affect us.
	{
		if (prefix)
			Con_DPrintf("Unknown shader directive parsing %s: \"%s\"\n", ps->s->name, prefix);
		else
			Con_DPrintf("Unknown shader directive parsing %s: \"%s\"\n", ps->s->name, token);
	}

	// Next Line
	while (ps->ptr)
	{
		token = COM_ParseExt(&ps->ptr, false, true);
		if ( !token[0] )
		{
			break;
		}
	}

	return false;
}

static void Shader_SetPassFlush (shaderpass_t *pass, shaderpass_t *pass2)
{
	if (((pass->flags & SHADER_PASS_DETAIL) && !r_detailtextures.value) ||
		((pass2->flags & SHADER_PASS_DETAIL) && !r_detailtextures.value) ||
		 (pass->flags & SHADER_PASS_VIDEOMAP) || (pass2->flags & SHADER_PASS_VIDEOMAP))
	{
		return;
	}

	//don't merge passes if they're got their own programs.
	if (pass->prog || pass2->prog)
		return;

	/*identity alpha is required for merging*/
	if (pass->alphagen != ALPHA_GEN_IDENTITY || pass2->alphagen != ALPHA_GEN_IDENTITY)
		return;

	/*don't merge passes if the hardware cannot support it*/
	if (pass->numMergedPasses >= be_maxpasses)
		return;

	/*rgbgen must be identity too except if the later pass is identity_ligting, in which case all is well and we can switch the first pass to identity_lighting instead*/
	if (pass2->rgbgen == RGB_GEN_IDENTITY_LIGHTING && (pass2->blendmode == PBM_OVERBRIGHT || pass2->blendmode == PBM_MODULATE) && (pass->rgbgen == RGB_GEN_IDENTITY||pass->rgbgen == RGB_GEN_VERTEX_EXACT))
	{
		if (pass->blendmode == PBM_REPLACE)
			pass->blendmode = PBM_REPLACELIGHT;
		pass->rgbgen = RGB_GEN_IDENTITY_LIGHTING;
		pass2->rgbgen = RGB_GEN_IDENTITY;
	}
	/*rgbgen must be identity (or the first is identity_lighting)*/
	else if (pass2->rgbgen != RGB_GEN_IDENTITY || (pass->rgbgen != RGB_GEN_IDENTITY && pass->rgbgen != RGB_GEN_IDENTITY_LIGHTING))
		return;

	/*if its alphatest, don't merge with anything other than lightmap (although equal stuff can be merged)*/
	if ((pass->shaderbits & SBITS_ATEST_BITS) && (((pass2->shaderbits & SBITS_DEPTHFUNC_BITS) != SBITS_DEPTHFUNC_EQUAL) || pass2->texgen != T_GEN_LIGHTMAP))
		return;

	if ((pass->shaderbits & SBITS_MASK_BITS) != (pass2->shaderbits & SBITS_MASK_BITS))
		return;

	// check if we can use multiple passes
	if (pass2->blendmode == PBM_DOTPRODUCT)
	{
		pass->numMergedPasses++;
	}
	else if (pass->numMergedPasses < be_maxpasses)
	{
		if (pass->blendmode == PBM_REPLACE || pass->blendmode == PBM_REPLACELIGHT)
		{
			if ((pass2->blendmode == PBM_DECAL && sh_config.tex_env_combine) ||
				(pass2->blendmode == PBM_ADD && sh_config.env_add) ||
				(pass2->blendmode && pass2->blendmode != PBM_ADD) ||	sh_config.nv_tex_env_combine4)
			{
				pass->numMergedPasses++;
			}
		}
		else if (pass->blendmode == PBM_ADD &&
			pass2->blendmode == PBM_ADD && sh_config.env_add)
		{
			pass->numMergedPasses++;
		}
		else if ((pass->blendmode == PBM_MODULATE || pass->blendmode == PBM_OVERBRIGHT) && (pass2->blendmode == PBM_MODULATE || pass2->blendmode == PBM_OVERBRIGHT))
		{
			pass->numMergedPasses++;
		}
		else if (pass->blendmode == PBM_DECAL && pass2->blendmode == PBM_OVERBRIGHT)
			pass->numMergedPasses++;	//HACK: allow modulating overbright passes with decal passes. overbright passes need to blend with something for their lighting to be correct, so this is a tradeoff.
		else
			return;
	}
	else return;

	/*if (pass->texgen == T_GEN_LIGHTMAP && (pass->blendmode == PBM_REPLACE || pass->blendmode == PBM_REPLACELIGHT) && pass2->blendmode == PBM_MODULATE && sh_config.tex_env_combine)
	{
//		if (pass->rgbgen == RGB_GEN_IDENTITY)
//			pass->rgbgen = RGB_GEN_IDENTITY_OVERBRIGHT;	//get the light levels right
//		pass2->blendmode = PBM_OVERBRIGHT;
	}
	if (pass2->texgen == T_GEN_LIGHTMAP && pass2->blendmode == PBM_OVERBRIGHT && sh_config.tex_env_combine)
	{
//		if (pass->rgbgen == RGB_GEN_IDENTITY)
//			pass->rgbgen = RGB_GEN_IDENTITY_OVERBRIGHT;	//get the light levels right
//		pass->blendmode = PBM_REPLACELIGHT;
//		pass2->blendmode = PBM_OVERBRIGHT;
		pass->blendmode = PBM_OVERBRIGHT;
	}
	*/
}

static const char *Shader_AlphaMaskProgArgs(shader_t *s)
{
	if (s->numpasses)
	{
		//alpha mask values ALWAYS come from the first pass.
		shaderpass_t *pass = &s->passes[0];
		switch(pass->shaderbits & SBITS_ATEST_BITS)
		{
		default:
			break;
		//cases inverted. the atest is to retain the pixel, glsl is to discard (alpha OP MASK).
		case SBITS_ATEST_GT0:
			return "#MASK=0.0#MASKLT=1";
		case SBITS_ATEST_LT128:
			return "#MASK=0.5";
		case SBITS_ATEST_GE128:
			return "#MASK=0.5#MASKLT=1";	//ignore the eq part.
		}
	}
	return "";
}

static void Shader_Programify (parsestate_t *ps)
{
	shader_t *s = ps->s;
	unsigned int reflectrefract = 0;
	const char *prog = NULL;
	const char *mask;
	char args[1024];
	qboolean eightbit = false;
/*	enum
	{
		T_UNKNOWN,
		T_WALL,
		T_MODEL
	} type = 0;*/
	int i;
	shaderpass_t *pass, *lightmap = NULL, *modellighting = NULL, *vertexlighting = NULL;
	for (i = 0; i < s->numpasses; i++)
	{
		pass = &s->passes[i];
		if (pass->rgbgen == RGB_GEN_LIGHTING_DIFFUSE || pass->rgbgen == RGB_GEN_ENTITY_LIGHTING_DIFFUSE)
		{
			if (s->usageflags & SUF_LIGHTMAP)
				lightmap = pass;
			else
				modellighting = pass;
		}
		else if (pass->rgbgen == RGB_GEN_ENTITY)
			modellighting = pass;
		else if (pass->rgbgen == RGB_GEN_VERTEX_LIGHTING || pass->rgbgen == RGB_GEN_VERTEX_EXACT)
		{
			if (s->usageflags & (SUF_LIGHTMAP|SUF_2D))
				vertexlighting = pass;
			else
				modellighting = pass;	//fucking DP morons who don't know what lightmaps are.
		}
		else if (pass->texgen == T_GEN_LIGHTMAP && pass->tcgen == TC_GEN_LIGHTMAP)
		{
			if (s->usageflags & SUF_LIGHTMAP)
				lightmap = pass;
			else
				modellighting = pass;	//fucking DP morons who don't know what lightmaps are.
		}

		/*if (pass->numtcmods || (pass->shaderbits & SBITS_ATEST_BITS))
			return;
		if (pass->texgen == T_GEN_LIGHTMAP && pass->tcgen == TC_GEN_LIGHTMAP)
			;
		else if (pass->texgen != T_GEN_LIGHTMAP && pass->tcgen == TC_GEN_BASE)
			;
		else
			return;*/
	}

	if (ps->forcedshader)
		prog = ps->forcedshader;
	else if (ps->dpwatertype)
	{
		prog = va("altwater%s#USEMODS#FRESNEL_EXP=2.0"
				//variable parts
				"#STRENGTH_REFR=%g#STRENGTH_REFL=%g"
				"#TINT_REFR=%g,%g,%g"
				"#TINT_REFL=%g,%g,%g"
				"#FRESNEL_MIN=%g#FRESNEL_RANGE=%g"
				"#ALPHA=%g",
				//those args
				(ps->dpwatertype&1)?"#REFLECT":"",
				ps->refractfactor*0.01, ps->reflectfactor*0.01,
				ps->refractcolour[0],ps->refractcolour[1],ps->refractcolour[2],
				ps->reflectcolour[0],ps->reflectcolour[1],ps->reflectcolour[2],
				ps->reflectmin, ps->reflectmax-ps->reflectmin,
				ps->wateralpha
			);
		//clear out blending and force regular depth.
		s->passes[0].shaderbits &= ~(SBITS_BLEND_BITS|SBITS_MISC_NODEPTHTEST|SBITS_DEPTHFUNC_BITS);
		s->passes[0].shaderbits |= SBITS_MISC_DEPTHWRITE;

		if (ps->dpwatertype & 1)
			reflectrefract |= SHADER_HASREFLECT;
		if (ps->dpwatertype & 2)
			reflectrefract |= SHADER_HASREFRACT;
		if (ps->dpwatertype & 4)
		{
			reflectrefract |= SHADER_HASREFRACT|SHADER_HASPORTAL;	//doubles up as a 'camera'
			if (s->sort == SHADER_SORT_PORTAL)
				s->sort = SHADER_SORT_OPAQUE;	//don't do it twice.
		}
	}
	else if (modellighting)
	{
		eightbit = r_softwarebanding && (qrenderer == QR_OPENGL) && sh_config.progs_supported;
		if (eightbit)
			prog = "defaultskin#EIGHTBIT";
		else
			prog = "defaultskin";
	}
	else if (lightmap)
	{
		eightbit = r_softwarebanding && (qrenderer == QR_OPENGL || qrenderer == QR_VULKAN) && sh_config.progs_supported;
		if (eightbit)
			prog = "defaultwall#EIGHTBIT";
		else
			prog = "defaultwall";
	}
	else if (vertexlighting)
	{
		if (r_forceprogramify.ival < 0)
			prog = "defaultfill";
		else
		{
			pass = vertexlighting;
			prog = "default2d";
		}
	}
	else
	{
		if (r_forceprogramify.ival < 0)
			prog = "defaultfill";
		else
			return;
	}

	args[0] = 0;
	if (ps->specularvalscale != 1)
		Q_strncatz(args, va("#specmul=%g", ps->specularvalscale), sizeof(args));
	if (ps->specularexpscale != 1)
		Q_strncatz(args, va("#specexp=%g", ps->specularexpscale), sizeof(args));
/*	switch(ps->offsetmappingmode)
	{
	case 0:	//force off.
		Q_strncatz(args, va("#NOOFFSETMAPPING", ps->specularexpscale), sizeof(args));
		break;
	case 1:	//force linear
		Q_strncatz(args, va("#NORELIEFMAPPING", ps->specularexpscale), sizeof(args));
		break;
	case 2:	//force relief
		Q_strncatz(args, va("#RELIEFMAPPING", ps->specularexpscale), sizeof(args));
		break;
	}
	if (ps->offsetmappingscale != 1)
		Q_strncatz(args, va("#OFFSETMAPPING_SCALE=%g", ps->offsetmappingscale), sizeof(args));
	if (ps->offsetmappingbias != 0)
		Q_strncatz(args, va("#OFFSETMAPPING_BIAS=%g", ps->offsetmappingbias), sizeof(args));
*/
	mask = strchr(s->name, '#');
	if (mask)
		Q_strncatz(args, mask, sizeof(args));

	mask = Shader_AlphaMaskProgArgs(s);

	s->prog = Shader_FindGeneric(va("%s%s%s", prog, mask, args), qrenderer);
	if (s->prog)
	{
		s->numpasses = 0;
		if (reflectrefract)
		{
			if (s->passes[0].numtcmods > 0 && s->passes[0].tcmods[0].type == SHADER_TCMOD_SCALE)
			{	//crappy workaround for DP bug.
				s->passes[0].tcmods[0].args[0] *= 4;
				s->passes[0].tcmods[0].args[1] *= 4;
			}
			s->passes[s->numpasses++].texgen = T_GEN_REFRACTION;
			s->passes[s->numpasses++].texgen = T_GEN_REFLECTION;
//			s->passes[s->numpasses++].texgen = T_GEN_RIPPLEMAP;
//			s->passes[s->numpasses++].texgen = T_GEN_REFRACTIONDEPTH;
			s->flags |= reflectrefract;
		}
		else
		{
			if (eightbit)
			{
				s->passes[s->numpasses].anim_frames[0] = R_LoadColourmapImage();
				s->passes[s->numpasses++].texgen = T_GEN_SINGLEMAP;
			}
			else			
				s->passes[s->numpasses++].texgen = T_GEN_DIFFUSE;
			s->flags |= SHADER_HASDIFFUSE;
		}
	}
}

static void Shader_Finish (parsestate_t *ps)
{
	shader_t *s = ps->s;
	int i;
	shaderpass_t *pass;
	
	//FIXME: reorder doom3 stages.
	//put diffuse first. give it a lightmap pass also, if we found a diffuse one with no lightmap.
	//then the ambient stages.
	//and forget about the bump/specular stages as we don't support them and already stripped them.

#if 0
	if (s->flags & SHADER_SKY)
	{
		/*skies go all black if fastsky is set*/
		if (r_fastsky.ival)
			s->flags = 0;
		/*or if its purely a skybox and has missing textures*/
//		if (!s->numpasses)
//			for (i = 0; i < 6; i++)
//				if (missing_texture.ref == s->skydome->farbox_textures[i].ref)
//					s->flags = 0;
		if (!(s->flags & SHADER_SKY))
		{
			Shader_Reset(s);

			Shader_DefaultScript(s->name, s,
						"{\n"
							"sort sky\n"
							"{\n"
								"map $whiteimage\n"
								"rgbgen srgb $r_fastskycolour\n"
							"}\n"
							"surfaceparm nodlight\n"
						"}\n"
					);
			return;
		}
	}
#endif

	if (s->prog && !s->numpasses)
	{
		pass = &s->passes[s->numpasses++];
		pass->tcgen = TC_GEN_BASE;
		pass->texgen = T_GEN_DIFFUSE;
		pass->shaderbits |= SBITS_MISC_DEPTHWRITE;
		pass->rgbgen = RGB_GEN_IDENTITY;
		pass->alphagen = ALPHA_GEN_IDENTITY;
		pass->numMergedPasses = 1;
		Shader_SetBlendmode(pass, NULL);
	}

	if (!s->numpasses && s->sort != SHADER_SORT_PORTAL && !(s->flags & (SHADER_NODRAW|SHADER_SKY)) && !s->fog_dist)
	{
		if (r_forceprogramify.ival >= 2)
		{
			Con_DPrintf("Shader %s with no passes, forcing nodraw\n", s->name);
			s->flags |= SHADER_NODRAW;
		}
		else
		{
			pass = &s->passes[s->numpasses++];
			pass = &s->passes[0];
			pass->tcgen = TC_GEN_BASE;
			if (TEXVALID(s->defaulttextures->base))
				pass->texgen = T_GEN_DIFFUSE;
			else
			{
				pass->texgen = T_GEN_SINGLEMAP;
				TEXASSIGN(pass->anim_frames[0], R_LoadHiResTexture(s->name, NULL, IF_NOALPHA));
				if (!TEXVALID(pass->anim_frames[0]))
				{
					Con_Printf("Shader %s failed to load default texture\n", s->name);
					pass->anim_frames[0] = missing_texture;
				}
				Con_Printf("Shader %s with no passes and no surfaceparm nodraw, inserting pass\n", s->name);
			}
			pass->shaderbits |= SBITS_MISC_DEPTHWRITE;
			pass->rgbgen = RGB_GEN_VERTEX_LIGHTING;
			pass->alphagen = ALPHA_GEN_IDENTITY;
			pass->numMergedPasses = 1;
			Shader_SetBlendmode(pass, NULL);
		}
	}

	if (!Q_stricmp (s->name, "flareShader"))
	{
		s->flags |= SHADER_FLARE;
		s->flags |= SHADER_NODRAW;
	}

	if (!s->numpasses && !s->sort)
	{
		s->sort = SHADER_SORT_ADDITIVE;
		return;
	}

	if (!s->sort && s->passes->texgen == T_GEN_CURRENTRENDER)
		s->sort = SHADER_SORT_NEAREST;


	if ((s->polyoffset.unit < 0) && !s->sort)
	{
		s->sort = SHADER_SORT_DECAL;
	}

	if ((r_vertexlight.value || !(s->usageflags & SUF_LIGHTMAP)) && !s->prog)
	{
		// do we have a lightmap pass?
		pass = s->passes;
		for (i = 0; i < s->numpasses; i++, pass++)
		{
			if (pass->flags & SHADER_PASS_LIGHTMAP)
				break;
		}

		if (i == s->numpasses)
		{
			goto done;
		}

		if (!r_vertexlight.value && pass->rgbgen == RGB_GEN_IDENTITY)
		{	//we found the lightmap pass. if we need a vertex-lit shader then just switch over the rgbgen+texture and hope other things work out
			pass->rgbgen = RGB_GEN_VERTEX_LIGHTING;
			pass->flags &= ~SHADER_PASS_LIGHTMAP;
			pass->tcgen = T_GEN_SINGLEMAP;
			goto done;
		}

		// try to find pass with rgbgen set to RGB_GEN_VERTEX
		pass = s->passes;
		for (i = 0; i < s->numpasses; i++, pass++)
		{
			if (pass->rgbgen == RGB_GEN_VERTEX_LIGHTING)
				break;
		}

		if (i < s->numpasses)
		{		// we found it
			pass->flags |= SHADER_CULL_FRONT;
			pass->flags &= ~SHADER_PASS_ANIMMAP;
			pass->shaderbits &= ~SBITS_BLEND_BITS;
			pass->blendmode = 0;
			pass->shaderbits |= SBITS_MISC_DEPTHWRITE;
			pass->alphagen = ALPHA_GEN_IDENTITY;
			pass->numMergedPasses = 1;
			s->flags |= SHADER_DEPTHWRITE;
			s->sort = SHADER_SORT_OPAQUE;
			s->numpasses = 1;
			memcpy(&s->passes[0], pass, sizeof(shaderpass_t));
		}
		else
		{	// we didn't find it - simply remove all lightmap passes
			pass = s->passes;
			for(i = 0; i < s->numpasses; i++, pass++)
			{
				if (pass->flags & SHADER_PASS_LIGHTMAP)
					break;
			}

			if ( i == s->numpasses -1 )
			{
				s->numpasses--;
			}
			else if ( i < s->numpasses - 1 )
			{
				for ( ; i < s->numpasses - 1; i++, pass++ )
				{
					memcpy ( pass, &s->passes[i+1], sizeof(shaderpass_t) );
				}
				s->numpasses--;
			}

			if ( s->passes[0].numtcmods )
			{
				pass = s->passes;
				for ( i = 0; i < s->numpasses; i++, pass++ )
				{
					if ( !pass->numtcmods )
						break;
				}

				memcpy ( &s->passes[0], pass, sizeof(shaderpass_t) );
			}

			s->passes[0].rgbgen = RGB_GEN_VERTEX_LIGHTING;
			s->passes[0].alphagen = ALPHA_GEN_IDENTITY;
			s->passes[0].blendmode = 0;
			s->passes[0].flags &= ~(SHADER_PASS_ANIMMAP|SHADER_PASS_NOCOLORARRAY);
			s->passes[0].shaderbits &= ~SBITS_BLEND_BITS;
			s->passes[0].shaderbits |= SBITS_MISC_DEPTHWRITE;
			s->passes[0].numMergedPasses = 1;
			s->numpasses = 1;
			s->flags |= SHADER_DEPTHWRITE;
		}
	}
done:;

	//if we've no specular map, try and find whatever the q3 syntax said. hopefully it'll be compatible...
 	if (!TEXVALID(s->defaulttextures->specular) && !(s->flags & SHADER_HASGLOSS))
	{
		for (pass = s->passes, i = 0; i < s->numpasses; i++, pass++)
		{
			if (pass->alphagen == ALPHA_GEN_SPECULAR)
				if (pass->texgen == T_GEN_ANIMMAP || pass->texgen == T_GEN_SINGLEMAP)
					s->defaulttextures->specular = pass->anim_frames[0];
		}
	}

	if (!TEXVALID(s->defaulttextures->base) && !(s->flags & SHADER_HASDIFFUSE) && !s->prog)
	{	//if one of the other passes specifies $diffuse, don't try and guess one, because that means that other pass's texture gets used for BOTH passes, which isn't good.
		//also, don't guess one if a program was specified.
		shaderpass_t *best = NULL;
		int bestweight = 9999999;
		int weight;

		for (pass = s->passes, i = 0; i < s->numpasses; i++, pass++)
		{
			weight = 0;
			if (pass->flags & SHADER_PASS_DETAIL)
				weight += 500;	//prefer not to use a detail pass. these are generally useless.
			if (pass->numtcmods || pass->tcgen != TC_GEN_BASE)
				weight += 200;
			if (pass->rgbgen != RGB_GEN_IDENTITY && pass->rgbgen != RGB_GEN_IDENTITY_OVERBRIGHT && pass->rgbgen != RGB_GEN_IDENTITY_LIGHTING)
				weight += 100;

			if (pass->texgen != T_GEN_ANIMMAP && pass->texgen != T_GEN_SINGLEMAP
#ifdef HAVE_MEDIA_DECODER
					&& pass->texgen != T_GEN_VIDEOMAP
#endif
					)
				weight += 1000;

			if ((pass->texgen == T_GEN_ANIMMAP || pass->texgen == T_GEN_SINGLEMAP) && pass->anim_frames[0] && *pass->anim_frames[0]->ident == '$')
				weight += 1500;
			
			if (weight < bestweight)
			{
				bestweight = weight;
				best = pass;
			}
		}

		if (best)
		{
			if (best->texgen == T_GEN_ANIMMAP || best->texgen == T_GEN_SINGLEMAP)
			{
				if (best->anim_frames[0] && *best->anim_frames[0]->ident != '$')
					s->defaulttextures->base = best->anim_frames[0];
			}
#ifdef HAVE_MEDIA_DECODER
			else if (pass->texgen == T_GEN_VIDEOMAP && pass->cin)
				s->defaulttextures->base = Media_UpdateForShader(best->cin);
#endif
		}
	}

	for (i = 0; i < s->numpasses; i += (pass->prog?pass->numMergedPasses:1))
	{
		pass = s->passes+i;
		if (!(pass->shaderbits & (SBITS_BLEND_BITS|SBITS_MASK_BITS)))
		{
			if (pass->texgen == T_GEN_LIGHTMAP && r_forceprogramify.ival==2)
				continue;	//pretend its blended.
			break;
		}
	}

	// all passes have blendfuncs
	if (i == s->numpasses)
	{
		int maskpass;
		qboolean isopaque = false;

		maskpass = -1;
		pass = s->passes;
		for (i = 0; i < s->numpasses; i++, pass++ )
		{
			if (pass->shaderbits & SBITS_ATEST_BITS)
			{
				maskpass = i;
			}
			else if ((pass->shaderbits & SBITS_MASK_BITS) == 0)
			{	//a few shaders use blendfunc one zero so that they're ignored when using r_vertexlight (while later alpha-masked surfs are not).
				if (/*(pass->shaderbits & (SBITS_SRCBLEND_BITS|SBITS_DSTBLEND_BITS)) == 0 ||*/
					(pass->shaderbits & (SBITS_SRCBLEND_BITS|SBITS_DSTBLEND_BITS)) == (SBITS_SRCBLEND_ONE|SBITS_DSTBLEND_ZERO))
					isopaque = true;
			}

			if (pass->rgbgen == RGB_GEN_UNKNOWN)
			{
				if (   (pass->shaderbits & SBITS_SRCBLEND_BITS) == 0
					|| (pass->shaderbits & SBITS_SRCBLEND_BITS) == SBITS_SRCBLEND_ONE
					|| (pass->shaderbits & SBITS_SRCBLEND_BITS) == SBITS_SRCBLEND_SRC_ALPHA)
					pass->rgbgen = RGB_GEN_IDENTITY_LIGHTING;
				else
					pass->rgbgen = RGB_GEN_IDENTITY;
			}

			Shader_SetBlendmode (pass, i?pass-1:NULL);

			if (pass->blendmode == PBM_ADD && !s->defaulttextures->fullbright)
				s->defaulttextures->fullbright = pass->anim_frames[0];
		}

		if (!(s->flags & SHADER_SKY ) && !s->sort)
		{
			if (isopaque)
				s->sort = SHADER_SORT_OPAQUE;
			else if (maskpass == -1)
			{
				if (s->numpasses && s->passes[0].blendmode == PBM_ADD)
					s->sort = SHADER_SORT_ADDITIVE;
				else
					s->sort = SHADER_SORT_BLEND;
			}
			else
				s->sort = SHADER_SORT_SEETHROUGH;
		}
	}
	else
	{
		int	j;
		shaderpass_t *sp;

		sp = s->passes;
		for (j = 0; j < s->numpasses; j++, sp++)
		{
			if (sp->rgbgen == RGB_GEN_UNKNOWN)
			{
				if (sp->flags & SHADER_PASS_LIGHTMAP)
					sp->rgbgen = RGB_GEN_IDENTITY_LIGHTING;
				else
					sp->rgbgen = RGB_GEN_IDENTITY;
			}

			Shader_SetBlendmode (sp, j?sp-1:NULL);
		}

		if (!s->sort)
		{
			if (i < s->numpasses && (s->passes[i].shaderbits & SBITS_ATEST_BITS))
				s->sort = SHADER_SORT_SEETHROUGH;
		}

		if (!( s->flags & SHADER_DEPTHWRITE) &&
			!(s->flags & SHADER_SKY))
		{
			s->passes->shaderbits |= SBITS_MISC_DEPTHWRITE;
			s->flags |= SHADER_DEPTHWRITE;
		}
	}

	if (s->numpasses >= 2)
	{
		int j;

		pass = s->passes;
		for (i = 0; i < s->numpasses;)
		{
			if (i == s->numpasses - 1)
				break;

			pass = s->passes + i;
			for (j = 1; j < s->numpasses-i && j == pass->numMergedPasses && j+1 < be_maxpasses; j++)
				Shader_SetPassFlush (pass, pass + j);

			i += pass->numMergedPasses;
		}
	}

	if (!s->sort)
	{
		extern cvar_t r_refract_fbo;
		if ((s->flags & SHADER_HASREFRACT) && !r_refract_fbo.ival)
			s->sort = SHADER_SORT_UNDERWATER;
		else
			s->sort = SHADER_SORT_OPAQUE;
	}

	if ((s->flags & SHADER_SKY) && (s->flags & SHADER_DEPTHWRITE))
	{
		s->flags &= ~SHADER_DEPTHWRITE;
	}

	if (!s->bemoverrides[bemoverride_depthonly])
	{
		const char *mask = Shader_AlphaMaskProgArgs(s);
		if (*mask || (s->prog&&s->prog->tess))
			s->bemoverrides[bemoverride_depthonly] = R_RegisterShader(va("depthonly%s%s", mask, (s->prog&&s->prog->tess)?"#TESS":""), SUF_NONE, 
				"{\n"
					"program depthonly\n"
					"{\n"
						"map $diffuse\n"
						"depthwrite\n"
						"maskcolor\n"
					"}\n"
				"}\n");
	}
	if (!s->bemoverrides[LSHADER_STANDARD] && (s->prog&&s->prog->tess))
	{
		int mode;
		for (mode = 0; mode < LSHADER_MODES; mode++)
		{
			if ((mode & LSHADER_CUBE) && (mode & LSHADER_SPOT))
				continue;
			if (s->bemoverrides[mode])
				continue;
			s->bemoverrides[mode] = R_RegisterShader(va("rtlight%s%s%s%s#TESS", 
																(mode & LSHADER_SMAP)?"#PCF":"",
																(mode & LSHADER_SPOT)?"#SPOT":"",
																(mode & LSHADER_CUBE)?"#CUBE":"",
#ifdef GLQUAKE
																(qrenderer == QR_OPENGL && gl_config.arb_shadow && (mode & (LSHADER_SMAP|LSHADER_SPOT)))?"#USE_ARB_SHADOW":""
#else
																""
#endif
																)
														, s->usageflags, 
				"{\n"
					"program rtlight\n"
					"{\n"
						"map $diffuse\n"
						"blendfunc add\n"
						"nodepth\n"
					"}\n"
				"}\n");
		}
	}

	if (!s->prog && sh_config.progs_supported && (r_forceprogramify.ival || (ps->parseflags & SPF_PROGRAMIFY)))
	{
		if (r_forceprogramify.ival >= 2)
		{
			if (s->passes[0].numtcmods == 1 && s->passes[0].tcmods[0].type == SHADER_TCMOD_SCALE)
				s->passes[0].numtcmods = 0;	//DP sucks and doesn't use normalized texture coords *if* there's a shader specified. so lets ignore any extra scaling that this imposes.
			if (s->passes[0].shaderbits & SBITS_ATEST_BITS)	//mimic DP's limited alphafunc support
				s->passes[0].shaderbits = (s->passes[0].shaderbits & ~SBITS_ATEST_BITS) | SBITS_ATEST_GE128;
			s->passes[0].shaderbits &= ~SBITS_DEPTHFUNC_BITS;	//DP ignores this too.
			if (s->flags & SHADER_CULL_BACK)	//DP has no back-face culling
				s->flags = (s->flags&~SHADER_CULL_BACK)|SHADER_CULL_FRONT;

			//disable rtlight stuff when blended.
			if ((s->passes[0].shaderbits & SBITS_SRCBLEND_BITS)==SBITS_SRCBLEND_ONE && (s->passes[0].shaderbits & SBITS_DSTBLEND_BITS)==SBITS_DSTBLEND_ZERO)
				;	//replace
			else if ((s->passes[0].shaderbits & SBITS_SRCBLEND_BITS)==SBITS_SRCBLEND_ONE && (s->passes[0].shaderbits & SBITS_DSTBLEND_BITS)==SBITS_DSTBLEND_ONE)
				s->flags |= SHADER_NOSHADOWS;	//add-ignore-alpha
			else if ((s->passes[0].shaderbits & SBITS_SRCBLEND_BITS)==SBITS_SRCBLEND_SRC_ALPHA && (s->passes[0].shaderbits & SBITS_DSTBLEND_BITS)==SBITS_DSTBLEND_ONE)
				s->flags |= SHADER_NOSHADOWS;	//add-with-alpha
			else if ((s->passes[0].shaderbits & SBITS_SRCBLEND_BITS)==SBITS_SRCBLEND_SRC_ALPHA && (s->passes[0].shaderbits & SBITS_DSTBLEND_BITS)==SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA)
				s->flags |= SHADER_NOSHADOWS;	//blended
			else
				s->flags |= SHADER_NOSHADOWS|SHADER_NODLIGHT;	//erk...

			if (s->passes[0].numtcmods>1)
			{	//reverse the order of tcmods (dp uses a texture matrix which it concats in reverse order)
				tcmod_t		tmp[SHADER_MAX_TC_MODS];
				int j;
				memcpy(tmp, s->passes[0].tcmods, sizeof(*tmp)*s->passes[0].numtcmods);
				for (j = 0; j < s->passes[0].numtcmods; j++)
					s->passes[0].tcmods[j] = tmp[s->passes[0].numtcmods-1-j];
			}
		}
		Shader_Programify(ps);
	}

	if(		s->numdeforms ||
			s->sort == SHADER_SORT_PORTAL ||	//q3-style portals (needed for pvs info)
			s->flags & (SHADER_HASREFLECT|SHADER_HASREFRACT)) //water effects (needed for pvs info)
		s->flags |= SHADER_NEEDSARRAYS;
	if (s->prog)
	{
		struct
		{
			int gen;
			unsigned int flags;
		} defaulttgen[] =
		{
			//light
			{T_GEN_SHADOWMAP,		0},						//0
			{T_GEN_LIGHTCUBEMAP,	0},						//1

			//material
			{T_GEN_DIFFUSE,			SHADER_HASDIFFUSE},		//2
			{T_GEN_NORMALMAP,		SHADER_HASNORMALMAP},	//3
			{T_GEN_SPECULAR,		SHADER_HASGLOSS},		//4
			{T_GEN_UPPEROVERLAY,	SHADER_HASTOPBOTTOM},	//5
			{T_GEN_LOWEROVERLAY,	SHADER_HASTOPBOTTOM},	//6
			{T_GEN_FULLBRIGHT,		SHADER_HASFULLBRIGHT},	//7
			{T_GEN_PALETTED,		SHADER_HASPALETTED},	//8
			{T_GEN_REFLECTCUBE,		SHADER_HASREFLECTCUBE},	//9
			{T_GEN_REFLECTMASK,		0},						//10
			{T_GEN_DISPLACEMENT,	SHADER_HASDISPLACEMENT},//11
			{T_GEN_OCCLUSION,		0},						//12
			{T_GEN_TRANSMISSION,	0},						//13
			{T_GEN_THICKNESS,		0},						//14
//			{T_GEN_REFLECTION,		SHADER_HASREFLECT},		//
//			{T_GEN_REFRACTION,		SHADER_HASREFRACT},		//
//			{T_GEN_REFRACTIONDEPTH,	SHADER_HASREFRACTDEPTH},//
//			{T_GEN_RIPPLEMAP,		SHADER_HASRIPPLEMAP},	//

			//batch
			{T_GEN_LIGHTMAP,		SHADER_HASLIGHTMAP},	//15
			{T_GEN_DELUXMAP,		0},						//16
			//more lightmaps								//17,18,19
			//mode deluxemaps								//20,21,22
		};

#ifdef HAVE_MEDIA_DECODER
		cin_t *cin = R_ShaderGetCinematic(s);
#endif

		//if the glsl doesn't specify all samplers, just trim them.
		s->numpasses = s->prog->numsamplers;

#ifdef HAVE_MEDIA_DECODER
		if (cin && R_ShaderGetCinematic(s) == cin)
			cin = NULL;
#endif

		//if the glsl has specific textures listed, be sure to provide a pass for them.
		for (i = 0; i < sizeof(defaulttgen)/sizeof(defaulttgen[0]); i++)
		{
			if (s->prog->defaulttextures & (1u<<i))
			{
				if (s->numpasses >= SHADER_PASS_MAX)
					break;	//panic...
				s->passes[s->numpasses].flags &= ~SHADER_PASS_DEPTHCMP;
				if (defaulttgen[i].gen == T_GEN_SHADOWMAP)
					s->passes[s->numpasses].flags |= SHADER_PASS_DEPTHCMP;
#ifdef HAVE_MEDIA_DECODER
				if (!i && cin)
				{
					s->passes[s->numpasses].texgen = T_GEN_VIDEOMAP;
					s->passes[s->numpasses].cin = cin;
					cin = NULL;
				}
				else
#endif
				{
					s->passes[s->numpasses].texgen = defaulttgen[i].gen;
#ifdef HAVE_MEDIA_DECODER
					s->passes[s->numpasses].cin = NULL;
#endif
				}
				s->numpasses++;
				s->flags |= defaulttgen[i].flags;
			}
		}

		//must have at least one texture.
		if (!s->numpasses)
		{
#ifdef HAVE_MEDIA_DECODER
			s->passes[0].texgen = cin?T_GEN_VIDEOMAP:T_GEN_DIFFUSE;
			s->passes[0].cin = cin;
#else
			s->passes[0].texgen = T_GEN_DIFFUSE;
#endif
			s->numpasses = 1;
		}
#ifdef HAVE_MEDIA_DECODER
		else if (cin)
			Media_ShutdownCin(cin);
#endif
		s->passes->numMergedPasses = s->numpasses;
	}
	else
	{
		for (i = 0; i < s->numpasses; i++)
		{
			pass = &s->passes[i];
			if (pass->prog)
				continue;
			if (pass->numtcmods || (s->passes[i].tcgen != TC_GEN_BASE && s->passes[i].tcgen != TC_GEN_LIGHTMAP) || !(s->passes[i].flags & SHADER_PASS_NOCOLORARRAY))
			{
				s->flags |= SHADER_NEEDSARRAYS;
				break;
			}
			if (pass->alphagen == ALPHA_GEN_PORTAL ||	//needs xyz
				pass->alphagen == ALPHA_GEN_SPECULAR)	//needs xyz+norm
			{
				s->flags |= SHADER_NEEDSARRAYS;
				break;
			}
			if (!(pass->flags & SHADER_PASS_NOCOLORARRAY))
			{
				if (!(((pass->rgbgen == RGB_GEN_VERTEX_LIGHTING) ||
					(pass->rgbgen == RGB_GEN_VERTEX_EXACT) ||
					(pass->alphagen == ALPHA_GEN_VERTEX) ||
					(pass->rgbgen == RGB_GEN_ONE_MINUS_VERTEX)) &&
					(pass->alphagen == ALPHA_GEN_VERTEX)))
				{
					s->flags |= SHADER_NEEDSARRAYS;
					break;
				}
			}
		}
	}

	if (!r_halfrate.ival)
	{
		for (i = 0; i < s->numpasses; i++)
		{
			s->passes[i].shaderbits |= SBITS_MISC_FULLRATE;
		}
	}
}
/*
static void Shader_UpdateRegistration (void)
{
	int i, j, l;
	shader_t *shader;
	shaderpass_t *pass;

	shader = r_shaders;
	for (i = 0; i < MAX_SHADERS; i++, shader++)
	{
		if (!shader->registration_sequence)
			continue;
		if (shader->registration_sequence != registration_sequence)
		{
			Shader_Free ( shader );
			shader->registration_sequence = 0;
			continue;
		}

		pass = shader->passes;
		for (j = 0; j < shader->numpasses; j++, pass++)
		{
			if (pass->flags & SHADER_PASS_ANIMMAP)
			{
				for (l = 0; l < pass->anim_numframes; l++)
				{
					if (pass->anim_frames[l])
						pass->anim_frames[l]->registration_sequence = registration_sequence;
				}
			}
			else if ( pass->flags & SHADER_PASS_VIDEOMAP )
			{
				// Shader_RunCinematic will do the job
//				pass->cin->frame = -1;
			}
			else if ( !(pass->flags & SHADER_PASS_LIGHTMAP) )
			{
				if ( pass->anim_frames[0] )
					pass->anim_frames[0]->registration_sequence = registration_sequence;
			}
		}
	}
}
*/

/*
	if (*shader_diffusemapname)
	{
		if (!s->defaulttextures.base)
			s->defaulttextures.base = Shader_FindImage (va("%s.tga", shader_diffusemapname), 0);
		if (!s->defaulttextures.bump)
			s->defaulttextures.bump = Shader_FindImage (va("%s_norm.tga", shader_diffusemapname), 0);
		if (!s->defaulttextures.fullbright)
			s->defaulttextures.fullbright = Shader_FindImage (va("%s_glow.tga", shader_diffusemapname), 0);
		if (!s->defaulttextures.specular)
			s->defaulttextures.specular = Shader_FindImage (va("%s_gloss.tga", shader_diffusemapname), 0);
		if (!s->defaulttextures.upperoverlay)
			s->defaulttextures.upperoverlay = Shader_FindImage (va("%s_shirt.tga", shader_diffusemapname), 0);
		if (!s->defaulttextures.loweroverlay)
			s->defaulttextures.loweroverlay = Shader_FindImage (va("%s_pants.tga", shader_diffusemapname), 0);	//stupid yanks...
	}
*/
void QDECL R_BuildDefaultTexnums(texnums_t *src, shader_t *shader, unsigned int imageflags)
{
	char *h;
	char imagename[MAX_QPATH];
	char mapname[MAX_QPATH];
	char *subpath = NULL;
	texnums_t *tex;
	unsigned int a, aframes;
	strcpy(imagename, shader->name);
	h = strchr(imagename, '#');
	if (h && !strchr(imagename, '@'))
		*h = 0;
	if (*imagename == '/' || strchr(imagename, ':'))
	{	//this is not security. this is anti-spam for the verbose security in the filesystem code.
		Con_Printf("Warning: shader has absolute path: %s\n", shader->name);
		*imagename = 0;
	}

	//skins can use an alternative path in certain cases, to work around dodgy models.
	if (shader->generator == Shader_DefaultSkin)
		subpath = shader->genargs;

	tex = shader->defaulttextures;
	aframes = max(1, shader->numdefaulttextures);
	//if any were specified explicitly, replicate that into all.
	//this means animmap can be used, with any explicit textures overriding all.
	if (!shader->numdefaulttextures && src)
	{
		//only do this if there wasn't an animmap thing to break everything.
		if (!TEXVALID(tex->base))
			tex->base			= src->base;
		if (!TEXVALID(tex->bump))
			tex->bump			= src->bump;
		if (!TEXVALID(tex->fullbright))
			tex->fullbright		= src->fullbright;
		if (!TEXVALID(tex->specular))
			tex->specular		= src->specular;
		if (!TEXVALID(tex->loweroverlay))
			tex->loweroverlay	= src->loweroverlay;
		if (!TEXVALID(tex->upperoverlay))
			tex->upperoverlay	= src->upperoverlay;
		if (!TEXVALID(tex->reflectmask))
			tex->reflectmask	= src->reflectmask;
		if (!TEXVALID(tex->reflectcube))
			tex->reflectcube	= src->reflectcube;
		if (!TEXVALID(tex->displacement))
			tex->displacement	= src->displacement;
	}
	for (a = 1; a < aframes; a++)
	{
		if (!TEXVALID(tex[a].base))
			tex[a].base			= tex[0].base;
		if (!TEXVALID(tex[a].bump))
			tex[a].bump			= tex[0].bump;
		if (!TEXVALID(tex[a].fullbright))
			tex[a].fullbright	= tex[0].fullbright;
		if (!TEXVALID(tex[a].specular))
			tex[a].specular		= tex[0].specular;
		if (!TEXVALID(tex[a].loweroverlay))
			tex[a].loweroverlay	= tex[0].loweroverlay;
		if (!TEXVALID(tex[a].upperoverlay))
			tex[a].upperoverlay	= tex[0].upperoverlay;
		if (!TEXVALID(tex[a].reflectmask))
			tex[a].reflectmask	= tex[0].reflectmask;
		if (!TEXVALID(tex[a].reflectcube))
			tex[a].reflectcube	= tex[0].reflectcube;
		if (!TEXVALID(tex[a].displacement))
			tex[a].displacement	= tex[0].displacement;
	}
	for (a = 0; a < aframes; a++, tex++)
	{
		COM_StripExtension(tex->mapname, mapname, sizeof(mapname));

		if (!TEXVALID(tex->base))
		{
			/*dlights/realtime lighting needs some stuff*/
			if (!TEXVALID(tex->base) && *tex->mapname)// && (shader->flags & SHADER_HASDIFFUSE))
				tex->base = R_LoadHiResTexture(tex->mapname, NULL, imageflags);

			if (!TEXVALID(tex->base))
				tex->base = R_LoadHiResTexture(imagename, subpath, (*imagename=='{')?0:IF_NOALPHA|imageflags);
		}

		if ((shader->flags & SHADER_HASPALETTED) && !TEXVALID(tex->paletted))
		{
			if (!TEXVALID(tex->paletted) && *tex->mapname)
				tex->paletted = R_LoadHiResTexture(va("%s", tex->mapname), NULL, 0|IF_NEAREST|IF_PALETTIZE|imageflags);
			if (!TEXVALID(tex->paletted))
				tex->paletted = R_LoadHiResTexture(va("%s", imagename), subpath, ((*imagename=='{')?0:IF_NOALPHA)|IF_NEAREST|IF_PALETTIZE|imageflags);
		}

		imageflags |= IF_LOWPRIORITY;

		COM_StripExtension(imagename, imagename, sizeof(imagename));

		if (!TEXVALID(tex->bump))
		{
			if (r_loadbumpmapping || (shader->flags & SHADER_HASNORMALMAP))
			{
				if (!TEXVALID(tex->bump) && *mapname && (shader->flags & SHADER_HASNORMALMAP))
					tex->bump = R_LoadHiResTexture(va("%s_norm", mapname), NULL, imageflags|IF_TRYBUMP|IF_NOSRGB|imageflags);
				if (!TEXVALID(tex->bump))
					tex->bump = R_LoadHiResTexture(va("%s_norm", imagename), subpath, imageflags|IF_TRYBUMP|IF_NOSRGB|imageflags);
			}
		}

		if (!TEXVALID(tex->loweroverlay))
		{
			if (shader->flags & SHADER_HASTOPBOTTOM)
			{
				if (!TEXVALID(tex->loweroverlay) && *mapname)
					tex->loweroverlay = R_LoadHiResTexture(va("%s_pants", mapname), NULL, imageflags);
				if (!TEXVALID(tex->loweroverlay))
					tex->loweroverlay = R_LoadHiResTexture(va("%s_pants", imagename), subpath, imageflags);	/*how rude*/
			}
		}

		if (!TEXVALID(tex->upperoverlay))
		{
			if (shader->flags & SHADER_HASTOPBOTTOM)
			{
				if (!TEXVALID(tex->upperoverlay) && *mapname)
					tex->upperoverlay = R_LoadHiResTexture(va("%s_shirt", mapname), NULL, imageflags);
				if (!TEXVALID(tex->upperoverlay))
					tex->upperoverlay = R_LoadHiResTexture(va("%s_shirt", imagename), subpath, imageflags);
			}
		}

		if (!TEXVALID(tex->specular))
		{
			extern cvar_t gl_specular;
			if ((shader->flags & SHADER_HASGLOSS) && gl_specular.value && gl_load24bit.value)
			{
				if (!TEXVALID(tex->specular) && *mapname)
					tex->specular = R_LoadHiResTexture(va("%s_gloss", mapname), NULL, imageflags);
				if (!TEXVALID(tex->specular))
					tex->specular = R_LoadHiResTexture(va("%s_gloss", imagename), subpath, imageflags);
			}
		}

		if (!TEXVALID(tex->fullbright))
		{
			extern cvar_t r_fb_bmodels;
			if ((shader->flags & SHADER_HASFULLBRIGHT) && r_fb_bmodels.value && gl_load24bit.value)
			{
				if (!TEXVALID(tex->fullbright) && *mapname)
					tex->fullbright = R_LoadHiResTexture(va("%s_luma:%s_glow", mapname, mapname), NULL, imageflags);
				if (!TEXVALID(tex->fullbright))
					tex->fullbright = R_LoadHiResTexture(va("%s_luma:%s_glow", imagename, imagename), subpath, imageflags);
			}
		}

		//if there's a reflectcube texture specified by the shader, make sure we have a reflectmask to go with it.
		if (tex->reflectcube)
		{
			if (!TEXVALID(tex->reflectmask) && *mapname)
				tex->reflectmask = R_LoadHiResTexture(va("%s_reflect", mapname), NULL, imageflags);
			if (!TEXVALID(tex->reflectmask))
				tex->reflectmask = R_LoadHiResTexture(va("%s_reflect", imagename), subpath, imageflags);
		}
	}
}

#if 0//def Q2BSPS
static qbyte *ReadRGBA8ImageFile(const char *fname, const char *subpath, int *width, int *height)
{
	qboolean hasalpha;
	qofs_t filesize;
	qbyte *img, *filedata = NULL;
	char *patterns[] = 
	{
		"*overrides/%s.tga",
		"*overrides/%s.jpg",
		"textures/%s.tga",
		"textures/%s.jpg",
	};
	char nname[MAX_QPATH];
	size_t p;
	for (p = 0; p < countof(patterns) && !filedata; p++)
	{
		if (*patterns[p] == '*')
			Q_snprintfz(nname, sizeof(nname), patterns[p]+1, COM_SkipPath(fname));
		else
			Q_snprintfz(nname, sizeof(nname), patterns[p], fname);
		filedata = FS_MallocFile(nname, FS_GAME, &filesize);
	}
	img = filedata?Read32BitImageFile(filedata, filesize, width, height, &hasalpha, fname):NULL;
	BZ_Free(filedata);
	return img;
}
#endif

//call this with some fallback textures to directly load some textures
void QDECL R_BuildLegacyTexnums(shader_t *shader, const char *fallbackname, const char *subpath, unsigned int loadflags, unsigned int imageflags, uploadfmt_t basefmt, size_t width, size_t height, qbyte *srcdata, qbyte *palette)
{
	char *h;
	char imagename[MAX_QPATH];
	char mapname[MAX_QPATH];	//as specified by the shader.
	//extern cvar_t gl_miptexLevel;
	texnums_t *tex = shader->defaulttextures;
	int a, aframes;
	/*else if (gl_miptexLevel.ival)
	{
		unsigned int miplevel = 0, i;
		for (i = 0; i < 3 && i < gl_miptexLevel.ival && mipdata[i]; i++)
			miplevel = i;
		for (i = 0; i < 3; i++)
			dontcrashme[i] = (miplevel+i)>3?NULL:mipdata[miplevel+i];
		width >>= miplevel;
		height >>= miplevel;
		mipdata = dontcrashme;
	}
	*/
	strcpy(imagename, shader->name);
	h = *imagename?strchr(imagename+1, '#'):NULL;
	if (h)
		*h = 0;
	if (*imagename == '/' || strchr(imagename, ':'))
	{	//this is not security. this is anti-spam for the verbose security in the filesystem code.
		Con_Printf("Warning: shader has absolute path: %s\n", shader->name);
		*imagename = 0;
	}

	//for water texture replacements
	while((h = strchr(imagename, '*')))
		*h = '#';

	loadflags &= shader->flags;

	//skins can use an alternative path in certain cases, to work around dodgy models.
	if (shader->generator == Shader_DefaultSkin)
		subpath = shader->genargs;

	//optimise away any palette info if we can...
	if (!palette || palette == host_basepal)
	{
		if (basefmt == TF_MIP4_8PAL24)
			basefmt = TF_MIP4_SOLID8;
//		if (basefmt == TF_MIP4_8PAL24_T255)
//			basefmt = TF_MIP4_TRANS8;
	}

	//make sure the noalpha thing is set properly.
	switch(basefmt)
	{
	case TF_MIP4_P8:
	case TF_MIP4_8PAL24:
	case TF_MIP4_SOLID8:
	case TF_SOLID8:
		imageflags |= IF_NOALPHA;
		//fallthrough
	case TF_MIP4_8PAL24_T255:
		if (!srcdata)
			basefmt = TF_SOLID8;
		break;
	default:
		break;
	}
	imageflags |= IF_MIPCAP;

	COM_StripExtension(imagename, imagename, sizeof(imagename));

	aframes = max(1, shader->numdefaulttextures);
	//if any were specified explicitly, replicate that into all.
	//this means animmap can be used, with any explicit textures overriding all.
	for (a = 1; a < aframes; a++)
	{
		if (!TEXVALID(tex[a].base))
			tex[a].base			= tex[0].base;
		if (!TEXVALID(tex[a].bump))
			tex[a].bump			= tex[0].bump;
		if (!TEXVALID(tex[a].fullbright))
			tex[a].fullbright	= tex[0].fullbright;
		if (!TEXVALID(tex[a].specular))
			tex[a].specular		= tex[0].specular;
		if (!TEXVALID(tex[a].loweroverlay))
			tex[a].loweroverlay	= tex[0].loweroverlay;
		if (!TEXVALID(tex[a].upperoverlay))
			tex[a].upperoverlay	= tex[0].upperoverlay;
		if (!TEXVALID(tex[a].reflectmask))
			tex[a].reflectmask	= tex[0].reflectmask;
		if (!TEXVALID(tex[a].reflectcube))
			tex[a].reflectcube	= tex[0].reflectcube;
		if (!TEXVALID(tex[a].displacement))
			tex[a].displacement	= tex[0].displacement;
	}
	for (a = 0; a < aframes; a++, tex++)
	{
		COM_StripExtension(tex->mapname, mapname, sizeof(mapname));

#if 0//def Q2BSPS
		if (gl_load24bit.ival == 2)
		{
			qbyte *base;
			int basewidth, baseheight;
			qbyte *norm;
			int normwidth, normheight;
			qbyte tmp;
			int x, y;
			base = ReadRGBA8ImageFile(imagename, subpath, &basewidth, &baseheight);
			//base contains diffuse RGB, and height
			if (base)
			{
				tex->base = Image_GetTexture(va("%s_diff", imagename), subpath, imageflags|IF_NOREPLACE, base, NULL, basewidth, baseheight, TF_RGBX32);	//queues the texture creation.
				norm = ReadRGBA8ImageFile(va("%s_bump", imagename), subpath, &normwidth, &normheight);
				if (norm)
				{
					if (normwidth != basewidth || normheight != baseheight)
					{
						//sucks...
						tex->bump = Image_GetTexture(va("%s_norm", imagename), subpath, imageflags|IF_NOREPLACE, norm, NULL, normwidth, normheight, TF_RGBX32);	//queues the texture creation.
					}
					else
					{
						//so we have a base texture, and normalmap
						//we already uploaded the diffuse, so now we can just pretend that the base is the specularmap.
						//we just need to swap the two alpha channels.
						for (y = 0; y < baseheight; y++)
						{
							for (x = 0; x < basewidth; x++)
							{
								tmp = base[(x+y*basewidth)*4+3];
								base[(x+y*basewidth)*4+3] = norm[(x+y*basewidth)*4+3];
								norm[(x+y*basewidth)*4+3] = ((x+y)&1)*255;//tmp;
							}
						}
						tex->bump = Image_GetTexture(va("%s_norm", imagename), subpath, imageflags|IF_NOREPLACE, norm, NULL, normwidth, normheight, TF_RGBA32);	//queues the texture creation.
						tex->specular = Image_GetTexture(va("%s_spec", imagename), subpath, imageflags|IF_NOREPLACE, base, NULL, basewidth, baseheight, TF_RGBA32);	//queues the texture creation.
					}
					BZ_Free(norm);
				}
				else
				{
					//generate a height8 image from the alpha channel. we  can do it in place
					for (y = 0; y < baseheight; y++)
					{
						for (x = 0; x < basewidth; x++)
							base[(x+y*basewidth)] = base[(x+y*basewidth)*4+3];
					}
					tex->bump = Image_GetTexture(va("%s_norm", imagename), subpath, imageflags|IF_NOREPLACE, base, NULL, basewidth, baseheight, TF_HEIGHT8);
				}
				BZ_Free(base);
			}
		}
#endif

		/*dlights/realtime lighting needs some stuff*/
		if (loadflags & SHADER_HASDIFFUSE)
		{
			if (!TEXVALID(tex->base) && *mapname)
				tex->base = R_LoadHiResTexture(mapname, NULL, imageflags);
			if (!TEXVALID(tex->base) && fallbackname)
			{
				if (gl_load24bit.ival)
				{
					tex->base = Image_GetTexture(imagename, subpath, imageflags|IF_NOWORKER, NULL, NULL, width, height, basefmt);
					if (!TEXLOADED(tex->base))
					{
						tex->base = Image_GetTexture(fallbackname, subpath, imageflags|IF_NOWORKER, NULL, NULL, width, height, basefmt);
						if (TEXLOADED(tex->base))
							Q_strncpyz(imagename, fallbackname, sizeof(imagename));
					}
				}
				if (!TEXLOADED(tex->base))
					tex->base = Image_GetTexture(imagename, subpath, imageflags, srcdata, palette, width, height, basefmt);
			}
			else if (!TEXVALID(tex->base))
				tex->base = Image_GetTexture(imagename, subpath, imageflags, srcdata, palette, width, height, basefmt);
		}

		if (loadflags & SHADER_HASPALETTED)
		{
			if (!TEXVALID(tex->paletted) && *mapname)
				tex->paletted = R_LoadHiResTexture(va("%s_pal", mapname), NULL, imageflags|IF_NEAREST|IF_PALETTIZE);
			if (!TEXVALID(tex->paletted))
				tex->paletted = Image_GetTexture(va("%s_pal", imagename), subpath, imageflags|IF_NEAREST|IF_NOSRGB|IF_PALETTIZE, srcdata, palette, width, height, (basefmt==TF_MIP4_SOLID8)?TF_MIP4_P8:PTI_P8);
		}

		imageflags |= IF_LOWPRIORITY;
		//all the rest need/want an alpha channel in some form.
		imageflags &= ~IF_NOALPHA;
		imageflags |= IF_NOGAMMA;

		if (loadflags & SHADER_HASNORMALMAP||*imagename=='#')
		{
			extern cvar_t r_shadow_bumpscale_basetexture;
			if (!TEXVALID(tex->bump) && *mapname)
				tex->bump = R_LoadHiResTexture(va("%s_norm", mapname), NULL, imageflags|IF_TRYBUMP|IF_NOSRGB);
			if (!TEXVALID(tex->bump) && (r_shadow_bumpscale_basetexture.ival||*imagename=='#'||gl_load24bit.ival))
			{
				qbyte *fallbackheight;
				if ((r_shadow_bumpscale_basetexture.ival||*imagename=='#') && !(basefmt&PTI_FULLMIPCHAIN))
					fallbackheight = srcdata;	//generate normalmap from assumed heights.
				else
					fallbackheight = NULL;		//disabled
				tex->bump = Image_GetTexture(va("%s_norm", imagename), subpath, imageflags|IF_TRYBUMP|IF_NOSRGB|(*imagename=='#'?IF_LINEAR:0), fallbackheight, palette, width, height, TF_HEIGHT8PAL);
			}
		}

		if (loadflags & SHADER_HASTOPBOTTOM)
		{
			if (!TEXVALID(tex->loweroverlay) && *mapname)
				tex->loweroverlay = R_LoadHiResTexture(va("%s_pants", mapname), NULL, imageflags);
			if (!TEXVALID(tex->loweroverlay))
				tex->loweroverlay = Image_GetTexture(va("%s_pants", imagename), subpath, imageflags, NULL, palette, width, height, 0);
		}
		if (loadflags & SHADER_HASTOPBOTTOM)
		{
			if (!TEXVALID(tex->upperoverlay) && *mapname)
				tex->upperoverlay = R_LoadHiResTexture(va("%s_shirt", mapname), NULL, imageflags);
			if (!TEXVALID(tex->upperoverlay))
				tex->upperoverlay = Image_GetTexture(va("%s_shirt", imagename), subpath, imageflags, NULL, palette, width, height, 0);
		}

		if (loadflags & SHADER_HASGLOSS)
		{
			if (!TEXVALID(tex->specular) && *mapname)
				tex->specular = R_LoadHiResTexture(va("%s_gloss", mapname), NULL, imageflags);
			if (!TEXVALID(tex->specular))
				tex->specular = Image_GetTexture(va("%s_gloss", imagename), subpath, imageflags, NULL, palette, width, height, 0);
		}
		
		if (tex->reflectcube)
		{
			if (!TEXVALID(tex->reflectmask) && *mapname)
				tex->reflectmask = R_LoadHiResTexture(va("%s_reflect", mapname), NULL, imageflags);
			if (!TEXVALID(tex->reflectmask))
				tex->reflectmask = Image_GetTexture(va("%s_reflect", imagename), subpath, imageflags, NULL, NULL, width, height, TF_INVALID);
		}

		if (loadflags & SHADER_HASFULLBRIGHT)
		{
			if (!TEXVALID(tex->fullbright) && *mapname)
				tex->fullbright = R_LoadHiResTexture(va("%s_luma", mapname), NULL, imageflags);
			if (!TEXVALID(tex->fullbright))
			{
				int s=-1;
				if (srcdata && !(basefmt&PTI_FULLMIPCHAIN) && (!palette || palette == host_basepal))
				for(s = width*height-1; s>=0; s--)
				{
					if (srcdata[s] >= 256-vid.fullbright)
						break;
				}
				tex->fullbright = Image_GetTexture(va("%s_luma:%s_glow", imagename,imagename), subpath, imageflags, (s>=0)?srcdata:NULL, palette, width, height, TF_TRANS8_FULLBRIGHT);
			}
		}
	}
}

shader_t *Mod_RegisterBasicShader(struct model_s *mod, const char *texname, unsigned int usageflags, const char *shadertext, uploadfmt_t pixelfmt, unsigned int width, unsigned int height, void *pixeldata, void *palettedata)
{
	extern cvar_t r_fb_bmodels, r_fb_models;
	extern cvar_t gl_specular;
	shader_t *s;
	unsigned int maps;
	char mapbase[64];
	if (shadertext)
		s = R_RegisterShader(texname, usageflags, shadertext);
	else if (mod->type != mod_brush)
		s = R_RegisterCustom(mod, texname, usageflags, Shader_DefaultSkin, NULL);
	else
		s = R_RegisterCustom(mod, texname, usageflags, Shader_DefaultBSPLM, NULL);

	maps = 0;
	maps |= SHADER_HASPALETTED;
	maps |= SHADER_HASDIFFUSE;
	if (mod->type == mod_alias)
		maps |= SHADER_HASTOPBOTTOM;
	if ((mod->type == mod_brush)?r_fb_bmodels.ival:r_fb_models.ival)
		maps |= SHADER_HASFULLBRIGHT;
	if (r_loadbumpmapping || s->defaulttextures->reflectcube)
		maps |= SHADER_HASNORMALMAP;
	if (gl_specular.ival)
		maps |= SHADER_HASGLOSS;
	COM_FileBase(mod->name, mapbase, sizeof(mapbase));
	R_BuildLegacyTexnums(s, texname, mapbase, maps, 0, pixelfmt, width, height, pixeldata, palettedata);
	return s;
}

void Shader_DefaultScript(parsestate_t *ps, const char *shortname, const void *args)
{
	const char *f = args;
	if (!args)
		return;
	while (*f == ' ' || *f == '\t' || *f == '\n' || *f == '\r')
		f++;
	if (*f == '{')
	{
		f++;
		Shader_ReadShader(ps, (void*)f, NULL);
	}
}

void Shader_DefaultBSPLM(parsestate_t *ps, const char *shortname, const void *args)
{
	shader_t *s = ps->s;
	char *builtin = NULL;
	if (Shader_ParseShader(ps, "defaultwall"))
		return;

	if (!builtin && r_softwarebanding && (qrenderer == QR_OPENGL || qrenderer == QR_VULKAN) && sh_config.progs_supported)
		builtin = (
				"{\n"
					"{\n"
						"program defaultwall#EIGHTBIT\n"
						"map $colourmap\n"
					"}\n"
				"}\n"
			);

	if (!builtin && r_lightmap.ival)
		builtin = (
				"{\n"
				"fte_program drawflat_wall#LM\n"
					"{\n"
						"map $lightmap\n"
						"tcgen lightmap\n"
					"}\n"
				"}\n"
			);

	if (!builtin && r_drawflat.ival)
		builtin = (
				"{\n"
					"fte_program drawflat_wall\n"
					"{\n"
						"map $lightmap\n"
						"tcgen lightmap\n"
						"rgbgen srgb $r_floorcolour\n"
					"}\n"
				"}\n"
			);


	if (!builtin && r_lightprepass)
	{
		builtin = (
			"{\n"
				"{\n"
					"fte_program lpp_wall\n"
					"map $gbuffer2\n"	//diffuse lighting info
					"map $gbuffer3\n"	//specular lighting info
				"}\n"

				//this is drawn during the initial gbuffer pass to prepare it
				"fte_bemode gbuffer\n"
				"{\n"
					"{\n"
						"fte_program lpp_depthnorm\n"
//						"map $normalmap\n"
						"tcgen base\n"
					"}\n"
				"}\n"
			"}\n"
		);
	}
	//d3d has no position-invariant. this results in all sorts of glitches, so try not to use it.
	if (!builtin && ((sh_config.progs_supported && (qrenderer == QR_OPENGL/*||qrenderer == QR_DIRECT3D9*/)) || sh_config.progs_required))
	{
			builtin = (
					"{\n"
						"fte_program defaultwall\n"
						"{\n"
							"map $diffuse\n"
						"}\n"
					"}\n"
				);
	}
	if (!builtin)
		builtin = (
				"{\n"
/*					"if $deluxmap\n"
						"{\n"
							"map $normalmap\n"
							"tcgen base\n"
							"depthwrite\n"
						"}\n"
						"{\n"
							"map $deluxmap\n"
							"tcgen lightmap\n"
						"}\n"
					"endif\n"
*///				"if !r_fullbright\n"
						"{\n"
							"map $lightmap\n"
//							"if $deluxmap\n"
//								"blendfunc gl_dst_color gl_zero\n"
//							"endif\n"
						"}\n"
//					"endif\n"
					"{\n"
						"map $diffuse\n"
						"tcgen base\n"
//						"if $deluxmap || !r_fullbright\n"
//							"blendfunc gl_dst_color gl_zero\n"
							"blendfunc filter\n"
//						"endif\n"
					"}\n"
					"if gl_fb_bmodels\n"
						"{\n"
							"map $fullbright\n"
							"blendfunc add\n"
							"depthfunc equal\n"
						"}\n"
					"endif\n"
				"}\n"
			);

	Shader_DefaultScript(ps, shortname, builtin);

	if (r_lightprepass)
		s->flags |= SHADER_HASNORMALMAP;
}

void Shader_DefaultCinematic(parsestate_t *ps, const char *shortname, const void *args)
{
	Shader_DefaultScript(ps, shortname,
		va(
			"{\n"
				"program default2d\n"
				"{\n"
					"videomap \"%s\"\n"
					"blendfunc gl_one gl_one_minus_src_alpha\n"
				"}\n"
			"}\n"
		, (const char*)args)
	);
}

/*shortname should begin with 'skybox_'*/
void Shader_DefaultSkybox(parsestate_t *ps, const char *shortname, const void *args)
{
	shader_t *s = ps->s;
	int i;
	Shader_DefaultScript(ps, shortname,
		va(
			"{\n"
				"sort sky\n"
				"surfaceparm nodlight\n"
				"skyparms %s - -\n"
			"}\n"
		, shortname+7)
	);

	for (i = 0; i < 6; i++)
	{
		if (s->skydome->farbox_textures[i] == missing_texture)
		{
			if (s->skydome)
				Z_Free(s->skydome);
			s->skydome = NULL;
			return;
		}
	}
}

char *Shader_DefaultBSPWater(parsestate_t *ps, const char *shortname, char *buffer, size_t buffersize)
{
	shader_t *s = ps->s;
	int wstyle;
	int type;
	float alpha;
	qboolean explicitalpha = false;
	cvar_t *alphavars[] = {	&r_wateralpha, &r_lavaalpha, &r_slimealpha, &r_telealpha};
	cvar_t *stylevars[] = {	&r_waterstyle, &r_lavastyle, &r_slimestyle, &r_telestyle};

	if (!strncmp(shortname, "*portal", 7))
	{
		return	"{\n"
					"portal\n"
				"}\n";
	}
	else if (!strncmp(shortname, "*lava", 5))
		type = 1;
	else if (!strncmp(shortname, "*slime", 6))
		type = 2;
	else if (!strncmp(shortname, "*tele", 5))
		type = 3;
	else
		type = 0;
	alpha = Shader_FloatArgument(s, "ALPHA");
	if (alpha)
		explicitalpha = true;
	else
	{
		if (cls.allow_watervis)
			alpha = *alphavars[type]->string?alphavars[type]->value:alphavars[0]->value;
		else
			alpha = 1;
	}

	if (alpha <= 0)
		wstyle = -1;
	else if (r_fastturb.ival)
		wstyle = 0;
	else if (*stylevars[type]->string)
		wstyle = stylevars[type]->ival;
	else if (stylevars[0]->ival > 0)
		wstyle = stylevars[0]->ival;
	else
		wstyle = 1;

	if ((wstyle < 0 || wstyle > 1) && !cls.allow_watervis)
		wstyle = 1;

	if (wstyle > 1 && !sh_config.progs_supported)
		wstyle = 1;

	//extra silly limitations
	switch(qrenderer)
	{
#ifdef GLQUAKE
	case QR_OPENGL:
		if (wstyle > 2 && !gl_config.ext_framebuffer_objects)
			wstyle = 2;
		break;
#endif
#ifdef VKQUAKE
	case QR_VULKAN:
		if (wstyle > 3)
			wstyle = 3;
		break;
#endif
	default:	//altwater not supported with other renderers
		if (wstyle > 1)
			wstyle = 1;
	}

	switch(wstyle)
	{
	case -1:	//invisible
		return (
			"{\n"
				"surfaceparm nodraw\n"
				"surfaceparm nodlight\n"
				"surfaceparm nomarks\n"
				"surfaceparm hasdiffuse\n"
			"}\n"
		);
	case -2:	//regular with r_wateralpha forced off.
		return (
			"{\n"
				"fte_program defaultwarp\n"
				"{\n"
					"map $diffuse\n"
					"tcmod turb 0.02 0.1 0.5 0.1\n"
				"}\n"
				"surfaceparm nodlight\n"
				"surfaceparm nomarks\n"
				"surfaceparm hasdiffuse\n"
			"}\n"
		);
	case 0:	//fastturb
		return (
			"{\n"
				"{\n"
					//"program defaultfill\n"
					"map $whiteimage\n"
					"rgbgen srgb $r_fastturbcolour\n"
				"}\n"
				"surfaceparm nodlight\n"
				"surfaceparm nomarks\n"
				"surfaceparm hasdiffuse\n"
			"}\n"
		);
	default:
	case 1:	//vanilla style
		Q_snprintfz(buffer, buffersize, 
				"{\n"
					"surfaceparm nodlight\n"
					"surfaceparm nomarks\n"
					"if %g < 1\n"
						"sort underwater\n"
					"endif\n"
					"fte_program defaultwarp%s\n"
					"{\n"
						"map $diffuse\n"
						"tcmod turb 0.02 0.1 0.5 0.1\n"
						"if %g < 1\n"
							"alphagen const %g\n"
							"blendfunc gl_src_alpha gl_one_minus_src_alpha\n"
						"endif\n"
					"}\n"
					"surfaceparm hasdiffuse\n"
				"}\n"
				, alpha, (explicitalpha||alpha==1)?"":va("#ALPHA=%g",alpha), alpha, alpha);
		return buffer;
	case 2:	//refraction of the underwater surface, with a fresnel
		return (
			"{\n"
				"surfaceparm nodlight\n"
				"surfaceparm nomarks\n"
				"{\n"
					"program altwater#FRESNEL=4\n"
					"map $refraction\n"
					"map $null\n"//$reflection
					"map $null\n"//$ripplemap
					"map $null\n"//$refractiondepth
				"}\n"
				"surfaceparm hasdiffuse\n"
			"}\n"
		);
	case 3:	//reflections
		return (
			"{\n"
				"surfaceparm nodlight\n"
				"surfaceparm nomarks\n"
				"{\n"
					"program altwater#REFLECT#FRESNEL=4\n"
					"map $refraction\n"
					"map $reflection\n"
					"map $null\n"//$ripplemap
					"map $null\n"//$refractiondepth
				"}\n"
				"surfaceparm hasdiffuse\n"
			"}\n"
		);
	case 4:	//ripples
		return (
			"{\n"
				"surfaceparm nodlight\n"
				"surfaceparm nomarks\n"
				"{\n"
					"program altwater#RIPPLEMAP#FRESNEL=4\n"
					"map $refraction\n"
					"map $null\n"//$reflection
					"map $ripplemap\n"
					"map $null\n"//$refractiondepth
				"}\n"
				"surfaceparm hasdiffuse\n"
			"}\n"
		);
	case 5:	//ripples+reflections
		return (
			"{\n"
				"surfaceparm nodlight\n"
				"surfaceparm nomarks\n"
				"{\n"
					"program altwater#REFLECT#RIPPLEMAP#FRESNEL=4\n"
					"map $refraction\n"
					"map $reflection\n"
					"map $ripplemap\n"
					"map $null\n"//$refractiondepth
				"}\n"
				"surfaceparm hasdiffuse\n"
			"}\n"
		);
	}
}

void Shader_DefaultWaterShader(parsestate_t *ps, const char *shortname, const void *args)
{
	char tmpbuffer[2048];
	Shader_DefaultScript(ps, shortname, Shader_DefaultBSPWater(ps, shortname, tmpbuffer, sizeof(tmpbuffer)));
}
void Shader_DefaultBSPQ2(parsestate_t *ps, const char *shortname, const void *args)
{
	if (!strncmp(shortname, "sky/", 4))
	{
		Shader_DefaultScript(ps, shortname,
				"{\n"
					"sort sky\n"
					"surfaceparm nodlight\n"
					"skyparms - - -\n"
				"}\n"
			);
	}
	else if (Shader_FloatArgument(ps->s, "WARP"))//!strncmp(shortname, "warp/", 5) || !strncmp(shortname, "warp33/", 7) || !strncmp(shortname, "warp66/", 7))
	{
		char tmpbuffer[2048];
		Shader_DefaultScript(ps, shortname, Shader_DefaultBSPWater(ps, shortname, tmpbuffer, sizeof(tmpbuffer)));
	}
	else if (Shader_FloatArgument(ps->s, "FLOW"))
	{
		if (Shader_FloatArgument(ps->s, "ALPHA")) {
			Shader_DefaultScript(ps, shortname,
					"{\n"
						"{\n"
							"map $diffuse\n"
							"alphagen const $#ALPHA\n"
							"tcmod scroll -1 0\n"
							"blendfunc blend\n"
						"}\n"
					"}\n"
				);
		} else {
			Shader_DefaultScript(ps, shortname,
				"{\n"
					"{\n"
						"map $diffuse\n"
						"tcmod scroll -1 0\n"
					"}\n"
					"{\n"
						"map $lightmap\n"
						"if gl_overbright > 1\n"
						"blendfunc gl_dst_color gl_src_color\n"
						"else\n"
						"blendfunc gl_dst_color gl_zero\n"
						"endif\n"
						"depthfunc equal\n"
					"}\n"
				"}\n"
				);
		}
	}
	else if (Shader_FloatArgument(ps->s, "ALPHA"))//   !strncmp(shortname, "trans/", 6))
	{
		Shader_DefaultScript(ps, shortname,
				"{\n"
					"{\n"
						"map $diffuse\n"
						"alphagen const $#ALPHA\n"
						"blendfunc blend\n"
					"}\n"
				"}\n"
			);
	}
	else
		Shader_DefaultBSPLM(ps, shortname, args);
}

void Shader_DefaultBSPQ1(parsestate_t *ps, const char *shortname, const void *args)
{
	char *builtin = NULL;
	char tmpbuffer[2048];

	if (!strcmp(shortname, "mirror_portal"))
	{
		builtin =	"{\n"
						"portal\n"
					"}\n";
	}
	else if (r_mirroralpha.value < 1 && (!strcmp(shortname, "window02_1") || !strncmp(shortname, "mirror", 6)))
	{
		if (r_mirroralpha.value < 0)
		{
			builtin =	"{\n"
							"portal\n"
							"{\n"
								"map $diffuse\n"
								"blendfunc blend\n"
								"alphagen portal 512\n"
								"depthwrite\n"
							"}\n"
						"}\n";
		}
		else
		{
			builtin =	"{\n"
							"portal\n"
							"{\n"
								"map $diffuse\n"
								"blendfunc blend\n"
								"alphagen const $r_mirroralpha\n"
								"depthwrite\n"
							"}\n"
							"surfaceparm nodlight\n"
						"}\n";
		}

	}

	if (!builtin && (*shortname == '*' || *shortname == '!'))
	{
		builtin = Shader_DefaultBSPWater(ps, shortname, tmpbuffer, sizeof(tmpbuffer));
	}
	if (!builtin && !strncmp(shortname, "sky", 3))
	{
		//q1 sky
		/*if (r_fastsky.ival)
		{
			builtin = (
					"{\n"
						"sort sky\n"
						"{\n"
							"map $whiteimage\n"
							"rgbgen srgb $r_fastskycolour\n"
						"}\n"
						"surfaceparm nodlight\n"
					"}\n"
				);
		}*/
		/*else if (*r_skyboxname.string || *cl.skyname)
		{
			qboolean okay;
			Z_Free(s->skydome);
			s->skydome = (skydome_t *)Z_Malloc(sizeof(skydome_t));

			okay = Shader_ParseSkySides(shortname, "", s->skydome->farbox_textures);
			s->flags |= SHADER_SKY|SHADER_NODLIGHT;
			s->sort = SHADER_SORT_SKY;

			if (okay)
				return;
			builtin = NULL;
			//if the r_skybox failed to load or whatever, reset and fall through and just use the regular sky
			Shader_Reset(s);
		}*/
		if (!builtin)
			builtin = (
				"{\n"
					"sort sky\n"
					"program defaultsky\n"
					"skyparms - 512 -\n"
					/*WARNING: these values are not authentic quake, only close aproximations*/
					"{\n"
						"map $diffuse\n"
						"tcmod scale 10 10\n"
						"tcmod scroll 0.04 0.04\n"
						"depthwrite\n"
					"}\n"
					"{\n"
						"map $fullbright\n"
						"blendfunc blend\n"
						"tcmod scale 10 10\n"
						"tcmod scroll 0.02 0.02\n"
					"}\n"
				"}\n"
			);
	}
	if (!builtin && *shortname == '{')
	{
		/*alpha test*/
		if (sh_config.progs_supported)
			builtin = (
				"{\n"
					"fte_program defaultwall#MASK=0.666#MASKLT\n"
				"}\n"
			);
		else
			builtin = (
				"{\n"
			/*		"if $deluxmap\n"
						"{\n"
							"map $normalmap\n"
							"tcgen base\n"
						"}\n"
						"{\n"
							"map $deluxmap\n"
							"tcgen lightmap\n"
						"}\n"
					"endif\n"*/
					"{\n"
						"map $diffuse\n"
						"tcgen base\n"
						"alphafunc ge128\n"
					"}\n"
	//				"if $lightmap\n"
						"{\n"
							"map $lightmap\n"
							"if gl_overbright > 1\n"
							"blendfunc gl_dst_color gl_src_color\n"	//scale it up twice. will probably still get clamped, but what can you do
							"else\n"
							"blendfunc gl_dst_color gl_zero\n"
							"endif\n"
							"depthfunc equal\n"
						"}\n"
	//				"endif\n"
					"{\n"
						"map $fullbright\n"
						"blendfunc add\n"
						"depthfunc equal\n"
					"}\n"
				"}\n"
			);
	}

	/* Half-Life requirement ~eukara */
	if (!builtin && !strncmp(shortname, "scroll", 6))
	{
		builtin = (
			"{\n"
				"fte_program defaultwall#SCROLL\n"
			"}\n"
		);
	}

	if (builtin)
		Shader_DefaultScript(ps, shortname, builtin);
	else
		Shader_DefaultBSPLM(ps, shortname, args);
}

void Shader_DefaultBSPVertex(parsestate_t *ps, const char *shortname, const void *args)
{
	char *builtin = NULL;

	if (Shader_ParseShader(ps, "defaultvertexlit"))
		return;

	if (!builtin)
	{
		builtin = (
			"{\n"
				"program defaultwall#VERTEXLIT\n"
				"{\n"
					"map $diffuse\n"
					"rgbgen vertex\n"
					"alphagen vertex\n"
				"}\n"
			"}\n"
		);
	}

	Shader_DefaultScript(ps, shortname, builtin);
}
void Shader_DefaultBSPFlare(parsestate_t *ps, const char *shortname, const void *args)
{
	shader_t *s = ps->s;
	shaderpass_t *pass;
	if (Shader_ParseShader(ps, "defaultflare"))
		return;

	pass = &s->passes[0];
	pass->flags = SHADER_PASS_NOCOLORARRAY;
	pass->shaderbits |= SBITS_SRCBLEND_ONE|SBITS_DSTBLEND_ONE;
	pass->anim_frames[0] = R_LoadHiResTexture(shortname, NULL, 0);
	pass->rgbgen = RGB_GEN_VERTEX_LIGHTING;
	pass->alphagen = ALPHA_GEN_IDENTITY;
	pass->numtcmods = 0;
	pass->tcgen = TC_GEN_BASE;
	pass->numMergedPasses = 1;
	Shader_SetBlendmode(pass, NULL);

	if (!TEXVALID(pass->anim_frames[0]))
	{
		Con_DPrintf (CON_WARNING "Shader %s has a stage with no image: %s.\n", s->name, shortname );
		pass->anim_frames[0] = missing_texture;
	}

	s->numpasses = 1;
	s->numdeforms = 0;
	s->flags = SHADER_FLARE;
	s->sort = SHADER_SORT_ADDITIVE;

	s->flags |= SHADER_NODRAW;
}
void Shader_DefaultSkin(parsestate_t *ps, const char *shortname, const void *args)
{
	if (Shader_ParseShader(ps, "defaultskin"))
		return;

	if (r_softwarebanding && qrenderer == QR_OPENGL && sh_config.progs_supported)
	{
		Shader_DefaultScript(ps, shortname,
			"{\n"
				"program defaultskin#EIGHTBIT\n"
				"affine\n"
				"{\n"
					"map $colourmap\n"
				"}\n"
			"}\n"
			);
		return;
	}
	if (r_lightprepass)
	{
		Shader_DefaultScript(ps, shortname,
			"{\n"
				"{\n"
					"fte_program lpp_wall\n"
					"map $gbuffer2\n"	//diffuse lighting info
					"map $gbuffer3\n"	//specular lighting info
				"}\n"

				//this is drawn during the initial gbuffer pass to prepare it
				"fte_bemode gbuffer\n"
				"{\n"
					"{\n"
						"fte_program lpp_depthnorm\n"
						"tcgen base\n"
					"}\n"
				"}\n"
			"}\n"
		);
		return;
	}

	if (r_tessellation.ival && sh_config.progs_supported)
	{
		Shader_DefaultScript(ps, shortname,
			"{\n"
				"program defaultskin#TESS\n"
			"}\n"
			);
		return;
	}

	Shader_DefaultScript(ps, shortname,
		"{\n"
			"if $lpp\n"
				"program lpp_skin\n"
			"else\n"
				"program defaultskin\n"
			"endif\n"
			"if gl_affinemodels\n"
				"affine\n"
			"endif\n"
			"{\n"
				"map $diffuse\n"
				"rgbgen lightingDiffuse\n"
				"if #BLEND\n"
					"blendfunc blend\n"	//straight blend
				"endif\n"
			"}\n"
			"{\n"
				"map $loweroverlay\n"
				"rgbgen bottomcolor\n"
				"blendfunc gl_src_alpha gl_one\n"
			"}\n"
			"{\n"
				"map $upperoverlay\n"
				"rgbgen topcolor\n"
				"blendfunc gl_src_alpha gl_one\n"
			"}\n"
			"{\n"
				"map $fullbright\n"
				"blendfunc add\n"
			"}\n"
		"}\n"
		);
}
void Shader_DefaultSkinShell(parsestate_t *ps, const char *shortname, const void *args)
{
	if (Shader_ParseShader(ps, "defaultskinshell"))
		return;

	Shader_DefaultScript(ps, shortname,
		"{\n"
			"sort seethrough\n"	//before blend, but after other stuff. should fix most issues with shotgun etc effects obscuring it.
//			"deformvertexes normal 1 1\n"
			//draw it with depth but no colours at all
			"{\n"
				"map $whiteimage\n"
				"maskcolor\n"
				"depthwrite\n"
			"}\n"
			//now draw it again, depthfunc = equal should fill only the near-side, avoiding any excess-brightness issues with overlapping triangles
			"{\n"
				"map $whiteimage\n"
				"rgbgen entity\n"
				"alphagen entity\n"
				"blendfunc blend\n"
			"}\n"
		"}\n"
		);
}
void Shader_Default2D(parsestate_t *ps, const char *shortname, const void *genargs)
{
	shader_t *s = ps->s;
	if (Shader_ParseShader(ps, "default2d"))
		return;
	if (sh_config.progs_supported && qrenderer != QR_DIRECT3D9
#ifdef HAVE_LEGACY
			&& !dpcompat_nopremulpics.ival
#endif
	)
	{
		//hexen2 needs premultiplied alpha to avoid looking ugly
		//but that results in problems where things are drawn with alpha not 0, so scale vertex colour by alpha in the fragment program
		Shader_DefaultScript(ps, shortname,
			"{\n"
				"affine\n"
				"nomipmaps\n"
				"program default2d#PREMUL\n"
				"{\n"
				"clampmap $diffuse\n"
				"blendfunc gl_one gl_one_minus_src_alpha\n"
				"}\n"
				"sort additive\n"
			"}\n"
			);
		TEXASSIGN(s->defaulttextures->base, R_LoadHiResTexture(s->name, genargs, IF_PREMULTIPLYALPHA|IF_UIPIC|IF_NOPICMIP|IF_NOMIPMAP|IF_CLAMP|IF_HIGHPRIORITY));
	}
	else
	{
		Shader_DefaultScript(ps, shortname,
			"{\n"
				"affine\n"
				"nomipmaps\n"
				"{\n"
					"clampmap $diffuse\n"
					"rgbgen vertex\n"
					"alphagen vertex\n"
					"blendfunc gl_src_alpha gl_one_minus_src_alpha\n"
				"}\n"
				"sort additive\n"
			"}\n"
			);
		TEXASSIGN(s->defaulttextures->base, R_LoadHiResTexture(s->name, genargs, IF_UIPIC|IF_NOPICMIP|IF_NOMIPMAP|IF_CLAMP|IF_HIGHPRIORITY));
	}
}
void Shader_PolygonShader(struct shaderparsestate_s *ps, const char *shortname, const void *args)
{
	shader_t *s = ps->s;
	Shader_DefaultScript(ps, shortname,
		"{\n"
			"if $lpp\n"
				"program lpp_skin\n"
			"else\n"
				"program defaultskin#NONORMALS\n"
			"endif\n"
			"{\n"
				"map $diffuse\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
			"}\n"
			"{\n"
				"map $fullbright\n"
				"blendfunc add\n"
			"}\n"
		"}\n"
		);

	if (!s->defaulttextures->base && (s->flags & SHADER_HASDIFFUSE))
		R_BuildDefaultTexnums(NULL, s, 0);
}

static qboolean Shader_ReadShaderTerms(parsestate_t *ps, struct scondinfo_s *cond)
{
	char *token;

	if (!ps->ptr)
		return false;

	token = COM_ParseExt (&ps->ptr, true, true);

	if ( !token[0] )
		return true;
	else if (!Shader_Conditional_Read(ps, cond, token, &ps->ptr))
	{
		int i;
		for (i = 0; shadermacros[i].name; i++)
		{
			if (!Q_stricmp (token, shadermacros[i].name))
			{
#define SHADER_MACRO_ARGS 8
				int argn = 0;
				const char *oldptr;
				char arg[SHADER_MACRO_ARGS][256];
				char tmp[4096], *out, *in;
				//parse args until the end of the line
				while (ps->ptr)
				{
					token = COM_ParseExt(&ps->ptr, false, true);
					if ( !token[0] )
					{
						break;
					}
					if (argn <= SHADER_MACRO_ARGS)
					{
						Q_strncpyz(arg[argn], token, sizeof(arg[argn]));
						argn++;
					}
				}
				for(out = tmp, in = shadermacros[i].body; *in; )
				{
					if (out == tmp+countof(tmp)-1)
						break;
					if (*in == '%' && in[1] == '%')
						in++;	//skip doubled up percents
					else if (*in == '%')
					{	//expand an arg
						char *e;
						int i = strtol(in+1, &e, 0);
						if (e != in+1)
						{
							i--;
							if (i >= 0 && i < countof(arg))
							{
								for (in = arg[i]; *in; )
								{
									if (out == tmp+countof(tmp)-1)
										break;
									*out++ = *in++;
								}
								in = e;
								continue;
							}
						}
					}
					*out++ = *in++;
				}
				*out = 0;

				oldptr = ps->ptr;
				ps->ptr = tmp;
				Shader_ReadShaderTerms(ps, cond);
				ps->ptr = oldptr;
				return true;
			}
		}
		if (token[0] == '}')
			return false;
		else if (token[0] == '{')
			Shader_Readpass(ps);
		else if (Shader_Parsetok(ps, shaderkeys, token))
			return false;
	}
	return true;
}

//loads a shader string into an existing shader object, and finalises it and stuff
static void Shader_ReadShader(parsestate_t *ps, const char *shadersource, shadercachefile_t *sourcefile)
{
	struct scondinfo_s cond = {0};
	char **savebody = ps->saveshaderbody;
	shader_t *s = ps->s;

	memset(ps, 0, sizeof(*ps));
	if (sourcefile)
	{
		ps->forcedshader = *sourcefile->forcedshadername?sourcefile->forcedshadername:NULL;
		ps->parseflags = sourcefile->parseflags;
		ps->sourcename = sourcefile->name;
	}
	else
	{
		ps->parseflags = 0;
		ps->sourcename = "<code>";
	}
	ps->specularexpscale = 1;
	ps->specularvalscale = 1;
	ps->ptr = shadersource;
	ps->s = s;

	if (!s->defaulttextures)
	{
		s->defaulttextures = Z_Malloc(sizeof(*s->defaulttextures));
		s->numdefaulttextures = 0;
	}

// set defaults
	s->flags = SHADER_CULL_FRONT;

	while (Shader_ReadShaderTerms(ps, &cond))
	{
	}

	if (cond.depth)
	{
		Con_Printf("if statements without endif in shader %s\n", s->name);
	}

	Shader_Finish (ps);

	//querying the shader body often requires generating the shader, which then gets parsed.
	if (savebody)
	{
		size_t l = ps->ptr?ps->ptr - shadersource:0;
		Z_Free(*savebody);
		*savebody = BZ_Malloc(l+1);
		(*savebody)[l] = 0;
		memcpy(*savebody, shadersource, l);
	}
}

static void Shader_LoadMaterialString(parsestate_t *ps, const char *shadertext)
{	//callback for our external material loaders.
	Shader_Reset(ps);
	Shader_ReadShader(ps, shadertext, NULL);
}

static qboolean Shader_ParseShader(parsestate_t *ps, const char *parsename)
{
	size_t offset = 0, length;
	const char *buf = NULL;
	shadercachefile_t *sourcefile = NULL;
	const char *file;
	const char *token=".";
	size_t i;

	if (!strchr(parsename, ':'))
	{
		//if the named shader is a .shader file then just directly load it.
		token = COM_GetFileExtension(parsename, NULL);
		if (!strcmp(token, ".mat") || !*token)
		{
			char shaderfile[MAX_QPATH];
			if (!*token)
			{
				for (i = 0; i < materialloader_count; i++)
				{
					if (materialloader[i].funcs->ReadMaterial(ps, parsename, Shader_LoadMaterialString))
						return true;
				}

				Q_snprintfz(shaderfile, sizeof(shaderfile), "%s.mat", parsename);
				file = COM_LoadTempMoreFile(shaderfile, &length);
			}
			else
				file = COM_LoadTempMoreFile(parsename, &length);
			if (file)
			{
				Shader_Reset(ps);
				token = COM_ParseExt (&file, true, false);	//we need to skip over the leading {.
				if (*token != '{')
					token = COM_ParseExt (&file, true, false);	//try again, in case we found some legacy name.
				if (*token == '{')
				{
					Shader_ReadShader(ps, file, NULL);
					return true;
				}
				else
					Con_Printf("file %s.shader does not appear to contain a shader\n", shaderfile);
			}
		}
	}

	if (Shader_LocateSource(parsename, &buf, &length, &offset, &sourcefile))
	{
		// the shader is in the shader scripts
		if (buf && offset < length )
		{
			file = buf + offset;
			token = COM_ParseExt (&file, true, true);
			if ( !file || token[0] != '{' )
			{
				FS_FreeFile((char*)buf);
				return false;
			}

			Shader_Reset(ps);

			Shader_ReadShader(ps, file, sourcefile);

			return true;
		}
	}

	if (*token)
	{
		for (i = 0; i < materialloader_count; i++)
		{
			if (materialloader[i].funcs->ReadMaterial(ps, parsename, Shader_LoadMaterialString))
				return true;
		}
	}

	return false;
}
void R_UnloadShader(shader_t *shader)
{
	if (!shader)
		return;
	if (shader->uses <= 0)
	{
		Con_Printf("Shader double free (%p %s %i)\n", shader, shader->name, shader->usageflags);
		return;
	}
	if (--shader->uses == 0)
		Shader_Free(shader);
}
static shader_t *R_LoadShader (model_t *mod, const char *name, unsigned int usageflags, shader_gen_t *defaultgen, const char *genargs)
{
	int i, f = -1;
	char cleanname[MAX_QPATH];
	shader_t *s;

	if (!*name)
		name = "gfx/unspecified";

	COM_AssertMainThread("R_LoadShader");

	Q_strncpyz(cleanname, name, sizeof(cleanname));
	COM_CleanUpPath(cleanname);

	// check the hash first
	s = Hash_Get(&shader_active_hash, cleanname);
	while (s)
	{
		if (!mod || s->model == mod)
		//make sure the same texture can be used as either a lightmap or vertexlit shader
		//if it has an explicit shader overriding it then that still takes precidence. we might just have multiple copies of it.
		//q3 has a separate (internal) shader for every lightmap.
		if (!((s->usageflags ^ usageflags) & SUF_LIGHTMAP))
		{
			if (!s->uses)
				break;
			s->uses++;
			return s;
		}
		if (s->generator == Shader_DefaultScript)
		{	//if someone shaderfornamed and then needed a different usageflag later, just borrow from the existing one.
			defaultgen = s->generator;
			genargs = s->genargs;
		}
		s = Hash_GetNext(&shader_active_hash, cleanname, s);
	}

	// not loaded, find a free slot
	for (i = 0; i < r_numshaders; i++)
	{
		if (!r_shaders[i] || !r_shaders[i]->uses)
		{
			if ( f == -1 )	// free shader
			{
				f = i;
				break;
			}
		}
	}

	if (f == -1)
	{
		shader_t **n;
		int nm;	
		if (strlen(cleanname) >= sizeof(s->name))
		{
			Sys_Error( "R_LoadShader: Shader name too long.");
			return NULL;
		}
		f = r_numshaders;
		if (f == r_maxshaders)
		{
			if (!r_maxshaders)
				Sys_Error( "R_LoadShader: shader system not inited.");

			nm = r_maxshaders * 2;
			n = realloc(r_shaders, nm*sizeof(*n));
			if (!n)
			{
				Sys_Error( "R_LoadShader: Shader limit exceeded.");
				return NULL;
			}
			memset(n+r_maxshaders, 0, (nm - r_maxshaders)*sizeof(*n));
			r_shaders = n;
			r_maxshaders = nm;
		}
	}

	{
		parsestate_t ps;
		char shortname[MAX_QPATH];
		char *argsstart;
		s = r_shaders[f];
		if (!s)
		{
			s = r_shaders[f] = Z_Malloc(sizeof(*s));
		}
		ps.saveshaderbody = NULL;
		ps.s = s;
		s->id = f;
		if (r_numshaders < f+1)
			r_numshaders = f+1;

		if (!s->defaulttextures)
			s->defaulttextures = Z_Malloc(sizeof(*s->defaulttextures));
		else
			memset(s->defaulttextures, 0, sizeof(*s->defaulttextures));
		s->numdefaulttextures = 0;
		Q_strncpyz(s->name, cleanname, sizeof(s->name));
		s->model = mod;
		s->usageflags = usageflags;
		s->generator = defaultgen;
		s->width = 0;
		s->height = 0;
		s->uses = 1;
		if (genargs)
			s->genargs = strdup(genargs);
		else
			s->genargs = NULL;

		//now determine the 'short name'. ie: the shader that is loaded off disk (no args, no extension)
		argsstart = *cleanname?strchr(cleanname+1, '#'):NULL;
		if (argsstart)
			*argsstart = 0;
		COM_StripExtension (cleanname, shortname, sizeof(shortname));

		if (ruleset_allow_shaders.ival && !(usageflags & SUR_FORCEFALLBACK))
		{
			if (sh_config.shadernamefmt)
			{
				char drivername[MAX_QPATH];
				Q_snprintfz(drivername, sizeof(drivername), sh_config.shadernamefmt, cleanname);
				if (Shader_ParseShader(&ps, drivername))
					return s;
			}
			if (Shader_ParseShader(&ps, cleanname))
				return s;
			if (strcmp(cleanname, shortname))
				if (Shader_ParseShader(&ps, shortname))
					return s;
		}

		// make a default shader

		if (s->generator)
		{
			Shader_Regenerate(&ps, shortname);
			return s;
		}
		else
		{
			Shader_Free(s);
		}
	}
	return NULL;
}

#ifdef _DEBUG
static char *Shader_DecomposePass(char *o, shaderpass_t *p, qboolean simple)
{
	if (!simple)
	{
		switch(p->rgbgen)
		{
		default: sprintf(o, "RGB_GEN_? "); break;
		case RGB_GEN_ENTITY: sprintf(o, "RGB_GEN_ENTITY "); break;
		case RGB_GEN_ONE_MINUS_ENTITY: sprintf(o, "RGB_GEN_ONE_MINUS_ENTITY "); break;
		case RGB_GEN_VERTEX_LIGHTING: sprintf(o, "RGB_GEN_VERTEX_LIGHTING "); break;
		case RGB_GEN_VERTEX_EXACT: sprintf(o, "RGB_GEN_VERTEX_EXACT "); break;
		case RGB_GEN_ONE_MINUS_VERTEX: sprintf(o, "RGB_GEN_ONE_MINUS_VERTEX "); break;
		case RGB_GEN_IDENTITY_LIGHTING: sprintf(o, "RGB_GEN_IDENTITY_LIGHTING "); break;
		case RGB_GEN_IDENTITY_OVERBRIGHT: sprintf(o, "RGB_GEN_IDENTITY_OVERBRIGHT "); break;
		case RGB_GEN_IDENTITY: sprintf(o, "RGB_GEN_IDENTITY "); break;
		case RGB_GEN_CONST: sprintf(o, "RGB_GEN_CONST "); break;
		case RGB_GEN_ENTITY_LIGHTING_DIFFUSE: sprintf(o, "RGB_GEN_ENTITY_LIGHTING_DIFFUSE "); break;
		case RGB_GEN_LIGHTING_DIFFUSE: sprintf(o, "RGB_GEN_LIGHTING_DIFFUSE "); break;
		case RGB_GEN_WAVE: sprintf(o, "RGB_GEN_WAVE "); break;
		case RGB_GEN_TOPCOLOR: sprintf(o, "RGB_GEN_TOPCOLOR "); break;
		case RGB_GEN_BOTTOMCOLOR: sprintf(o, "RGB_GEN_BOTTOMCOLOR "); break;
		case RGB_GEN_UNKNOWN: sprintf(o, "RGB_GEN_UNKNOWN "); break;
		}
		o+=strlen(o);
		sprintf(o, "\n"); o+=strlen(o);

		switch(p->alphagen)
		{
		default: sprintf(o, "ALPHA_GEN_? "); break;
		case ALPHA_GEN_ENTITY: sprintf(o, "ALPHA_GEN_ENTITY "); break;
		case ALPHA_GEN_WAVE: sprintf(o, "ALPHA_GEN_WAVE "); break;
		case ALPHA_GEN_PORTAL: sprintf(o, "ALPHA_GEN_PORTAL "); break;
		case ALPHA_GEN_SPECULAR: sprintf(o, "ALPHA_GEN_SPECULAR "); break;
		case ALPHA_GEN_IDENTITY: sprintf(o, "ALPHA_GEN_IDENTITY "); break;
		case ALPHA_GEN_VERTEX: sprintf(o, "ALPHA_GEN_VERTEX "); break;
		case ALPHA_GEN_CONST: sprintf(o, "ALPHA_GEN_CONST "); break;
		}
		o+=strlen(o);
		sprintf(o, "\n"); o+=strlen(o);
	}

	if (p->prog)
	{
		sprintf(o, "program %s\n", p->prog->name);
		o+=strlen(o);
	}

	if (p->shaderbits & SBITS_MISC_DEPTHWRITE)		{	sprintf(o, "SBITS_MISC_DEPTHWRITE\n"); o+=strlen(o); }
	if (p->shaderbits & SBITS_MISC_NODEPTHTEST)		{	sprintf(o, "SBITS_MISC_NODEPTHTEST\n"); o+=strlen(o); }
	else switch (p->shaderbits & SBITS_DEPTHFUNC_BITS)
	{
	case SBITS_DEPTHFUNC_EQUAL:			sprintf(o, "depthfunc equal\n");	break;
	case SBITS_DEPTHFUNC_CLOSER:		sprintf(o, "depthfunc less\n");		break;
	case SBITS_DEPTHFUNC_CLOSEREQUAL:	sprintf(o, "depthfunc lequal\n");	break;
	case SBITS_DEPTHFUNC_FURTHER:		sprintf(o, "depthfunc greater\n");	break;
	}
	if (p->shaderbits & SBITS_TESSELLATION)			{	sprintf(o, "SBITS_TESSELLATION\n"); o+=strlen(o); }
	if (p->shaderbits & SBITS_AFFINE)				{	sprintf(o, "SBITS_AFFINE\n"); o+=strlen(o); }
	if (p->shaderbits & SBITS_MASK_BITS)			{	sprintf(o, "SBITS_MASK_BITS\n"); o+=strlen(o); }

	if (p->shaderbits & SBITS_BLEND_BITS)
	{
		sprintf(o, "blendfunc"); 
		o+=strlen(o);
		switch(p->shaderbits & SBITS_SRCBLEND_BITS)
		{
		case SBITS_SRCBLEND_NONE:							sprintf(o, " SBITS_SRCBLEND_NONE"); break;
		case SBITS_SRCBLEND_ZERO:							sprintf(o, " SBITS_SRCBLEND_ZERO"); break;
		case SBITS_SRCBLEND_ONE:							sprintf(o, " SBITS_SRCBLEND_ONE"); break;
		case SBITS_SRCBLEND_DST_COLOR:						sprintf(o, " SBITS_SRCBLEND_DST_COLOR"); break;
		case SBITS_SRCBLEND_ONE_MINUS_DST_COLOR:			sprintf(o, " SBITS_SRCBLEND_ONE_MINUS_DST_COLOR"); break;
		case SBITS_SRCBLEND_SRC_ALPHA:						sprintf(o, " SBITS_SRCBLEND_SRC_ALPHA"); break;
		case SBITS_SRCBLEND_ONE_MINUS_SRC_ALPHA:			sprintf(o, " SBITS_SRCBLEND_ONE_MINUS_SRC_ALPHA"); break;
		case SBITS_SRCBLEND_DST_ALPHA:						sprintf(o, " SBITS_SRCBLEND_DST_ALPHA"); break;
		case SBITS_SRCBLEND_ONE_MINUS_DST_ALPHA:			sprintf(o, " SBITS_SRCBLEND_ONE_MINUS_DST_ALPHA"); break;
		case SBITS_SRCBLEND_SRC_COLOR_INVALID:				sprintf(o, " SBITS_SRCBLEND_SRC_COLOR_INVALID"); break;
		case SBITS_SRCBLEND_ONE_MINUS_SRC_COLOR_INVALID:	sprintf(o, " SBITS_SRCBLEND_ONE_MINUS_SRC_COLOR_INVALID"); break;
		case SBITS_SRCBLEND_ALPHA_SATURATE:					sprintf(o, " SBITS_SRCBLEND_ALPHA_SATURATE"); break;
		default:											sprintf(o, " SBITS_SRCBLEND_INVALID"); break;
		}
		o+=strlen(o);
		switch(p->shaderbits & SBITS_DSTBLEND_BITS)
		{
		case SBITS_DSTBLEND_NONE:							sprintf(o, " SBITS_DSTBLEND_NONE"); break;
		case SBITS_DSTBLEND_ZERO:							sprintf(o, " SBITS_DSTBLEND_ZERO"); break;
		case SBITS_DSTBLEND_ONE:							sprintf(o, " SBITS_DSTBLEND_ONE"); break;
		case SBITS_DSTBLEND_DST_COLOR_INVALID:				sprintf(o, " SBITS_DSTBLEND_DST_COLOR_INVALID"); break;
		case SBITS_DSTBLEND_ONE_MINUS_DST_COLOR_INVALID:	sprintf(o, " SBITS_DSTBLEND_ONE_MINUS_DST_COLOR_INVALID"); break;
		case SBITS_DSTBLEND_SRC_ALPHA:						sprintf(o, " SBITS_DSTBLEND_SRC_ALPHA"); break;
		case SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA:			sprintf(o, " SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA"); break;
		case SBITS_DSTBLEND_DST_ALPHA:						sprintf(o, " SBITS_DSTBLEND_DST_ALPHA"); break;
		case SBITS_DSTBLEND_ONE_MINUS_DST_ALPHA:			sprintf(o, " SBITS_DSTBLEND_ONE_MINUS_DST_ALPHA"); break;
		case SBITS_DSTBLEND_SRC_COLOR:						sprintf(o, " SBITS_DSTBLEND_SRC_COLOR"); break;
		case SBITS_DSTBLEND_ONE_MINUS_SRC_COLOR:			sprintf(o, " SBITS_DSTBLEND_ONE_MINUS_SRC_COLOR"); break;
		case SBITS_DSTBLEND_ALPHA_SATURATE_INVALID:			sprintf(o, " SBITS_DSTBLEND_ALPHA_SATURATE_INVALID"); break;
		default:											sprintf(o, " SBITS_DSTBLEND_INVALID"); break;
		}
		o+=strlen(o);

		sprintf(o, "\n"); 
		o+=strlen(o);
	}


	switch(p->shaderbits & SBITS_ATEST_BITS)
	{
	case SBITS_ATEST_NONE:	break;
	case SBITS_ATEST_GE128: sprintf(o, "SBITS_ATEST_GE128\n"); break;
	case SBITS_ATEST_LT128: sprintf(o, "SBITS_ATEST_LT128\n"); break;
	case SBITS_ATEST_GT0: sprintf(o, "SBITS_ATEST_GT0\n"); break;
	}
	o+=strlen(o);

	return o;
}
static void Shader_DecomposeSubPassMap(char *o, shader_t *s, char *name, texid_t tex)
{
	if (tex)
	{
		unsigned int flags = tex->flags;
		sprintf(o, "%s \"%s\" %ix%i%s %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", name, tex->ident, tex->width, tex->height,
			(tex->status == TEX_FAILED)?" FAILED":"",
			Image_FormatName(tex->format),
			(flags&IF_CLAMP)?" clamp":"",
			(flags&IF_NOMIPMAP)?" nomipmap":"",
			(flags&IF_NEAREST)?" nearest":"",
			(flags&IF_LINEAR)?" linear":"",
			(flags&IF_UIPIC)?" uipic":"",
			(flags&IF_SRGB)?" srgb":"",

			(flags&IF_NOPICMIP)?" nopicmip":"",
			(flags&IF_NOALPHA)?" noalpha":"",
			(flags&IF_NOGAMMA)?" noalpha":"",
			(flags&IF_TEXTYPEMASK)?" non-2d":"",
			(flags&IF_MIPCAP)?"":" nomipcap",
			(flags&IF_PREMULTIPLYALPHA)?" premultiply":"",

			(flags&IF_NOSRGB)?" nosrgb":"",

			(flags&IF_PALETTIZE)?" palettize":"",
			(flags&IF_NOPURGE)?" nopurge":"",
			(flags&IF_HIGHPRIORITY)?" highpri":"",
			(flags&IF_LOWPRIORITY)?" lowpri":"",
			(flags&IF_LOADNOW)?" loadnow":"",
			(flags&IF_TRYBUMP)?" trybump":"",
			(flags&IF_RENDERTARGET)?" rendertarget":"",
			(flags&IF_EXACTEXTENSION)?" exactext":"",
			(flags&IF_NOREPLACE)?" noreplace":"",
			(flags&IF_NOWORKER)?" noworker":""
			);
	}
	else
		sprintf(o, "%s (%s)", name, "UNDEFINED");
}
static char *Shader_DecomposeSubPass(char *o, shader_t *s, shaderpass_t *p, qboolean simple)
{
	int i;
	if (!simple)
	{
		switch(p->tcgen)
		{
		default: sprintf(o, "TC_GEN_? "); break;
		case TC_GEN_BASE: sprintf(o, "TC_GEN_BASE "); break;
		case TC_GEN_LIGHTMAP: sprintf(o, "TC_GEN_LIGHTMAP "); break;
		case TC_GEN_ENVIRONMENT: sprintf(o, "TC_GEN_ENVIRONMENT "); break;
		case TC_GEN_DOTPRODUCT: sprintf(o, "TC_GEN_DOTPRODUCT "); break;
		case TC_GEN_VECTOR: sprintf(o, "TC_GEN_VECTOR "); break;
		case TC_GEN_NORMAL: sprintf(o, "TC_GEN_NORMAL "); break;
		case TC_GEN_SVECTOR: sprintf(o, "TC_GEN_SVECTOR "); break;
		case TC_GEN_TVECTOR: sprintf(o, "TC_GEN_TVECTOR "); break;
		case TC_GEN_SKYBOX: sprintf(o, "TC_GEN_SKYBOX "); break;
		case TC_GEN_WOBBLESKY: sprintf(o, "TC_GEN_WOBBLESKY "); break;
		case TC_GEN_REFLECT: sprintf(o, "TC_GEN_REFLECT "); break;
		case TC_GEN_UNSPECIFIED: sprintf(o, "TC_GEN_UNSPECIFIED "); break;
		}
		o+=strlen(o);
		sprintf(o, "\n"); o+=strlen(o);

		for (i = 0; i < p->numtcmods; i++)
		{
			switch(p->tcmods[i].type)
			{
				default: sprintf(o, "TCMOD_GEN_? "); break;
				case SHADER_TCMOD_NONE: sprintf(o, "SHADER_TCMOD_NONE "); break;
				case SHADER_TCMOD_SCALE: sprintf(o, "SHADER_TCMOD_SCALE "); break;
				case SHADER_TCMOD_SCROLL: sprintf(o, "SHADER_TCMOD_SCROLL "); break;
				case SHADER_TCMOD_STRETCH: sprintf(o, "SHADER_TCMOD_STRETCH "); break;
				case SHADER_TCMOD_ROTATE: sprintf(o, "SHADER_TCMOD_ROTATE "); break;
				case SHADER_TCMOD_TRANSFORM: sprintf(o, "SHADER_TCMOD_TRANSFORM "); break;
				case SHADER_TCMOD_TURB: sprintf(o, "SHADER_TCMOD_TURB "); break;
				case SHADER_TCMOD_PAGE: sprintf(o, "SHADER_TCMOD_PAGE "); break;
			}
			o+=strlen(o);
			sprintf(o, "\n"); o+=strlen(o);
		}


		switch(p->blendmode)
		{
		default: sprintf(o, "PBM_? "); break;
		case PBM_MODULATE: sprintf(o, "PBM_MODULATE "); break;
		case PBM_OVERBRIGHT: sprintf(o, "PBM_OVERBRIGHT "); break;
		case PBM_DECAL: sprintf(o, "PBM_DECAL "); break;
		case PBM_ADD: sprintf(o, "PBM_ADD "); break;
		case PBM_DOTPRODUCT: sprintf(o, "PBM_DOTPRODUCT "); break;
		case PBM_REPLACE: sprintf(o, "PBM_REPLACE "); break;
		case PBM_REPLACELIGHT: sprintf(o, "PBM_REPLACELIGHT "); break;
		case PBM_MODULATE_PREV_COLOUR: sprintf(o, "PBM_MODULATE_PREV_COLOUR "); break;
		}
		o+=strlen(o);
	}

	switch(p->texgen)
	{
	default: sprintf(o, "T_GEN_? "); break;
	case T_GEN_SINGLEMAP:		Shader_DecomposeSubPassMap(o, s, "map", p->anim_frames[0]);	break;
	case T_GEN_ANIMMAP:			Shader_DecomposeSubPassMap(o, s, "animmap", p->anim_frames[0]);	break;
#ifdef HAVE_MEDIA_DECODER
	case T_GEN_VIDEOMAP:		Shader_DecomposeSubPassMap(o, s, "videomap", Media_UpdateForShader(p->cin)); break;
#endif
	case T_GEN_LIGHTMAP:		sprintf(o, "map $lightmap "); break;
	case T_GEN_DELUXMAP:		sprintf(o, "map $deluxemap "); break;
	case T_GEN_SHADOWMAP:		sprintf(o, "map $shadowmap "); break;
	case T_GEN_LIGHTCUBEMAP: 	sprintf(o, "map $lightcubemap "); break;
	case T_GEN_DIFFUSE:			Shader_DecomposeSubPassMap(o, s, "map $diffuse", s->defaulttextures[0].base); break;
	case T_GEN_NORMALMAP:		Shader_DecomposeSubPassMap(o, s, "map $normalmap", s->defaulttextures[0].bump); break;
	case T_GEN_SPECULAR:		Shader_DecomposeSubPassMap(o, s, "map $specular", s->defaulttextures[0].specular); break;
	case T_GEN_UPPEROVERLAY:	Shader_DecomposeSubPassMap(o, s, "map $upper", s->defaulttextures[0].upperoverlay); break;
	case T_GEN_LOWEROVERLAY:	Shader_DecomposeSubPassMap(o, s, "map $lower", s->defaulttextures[0].loweroverlay); break;
	case T_GEN_FULLBRIGHT:		Shader_DecomposeSubPassMap(o, s, "map $fullbright", s->defaulttextures[0].fullbright); break;
	case T_GEN_PALETTED:		Shader_DecomposeSubPassMap(o, s, "map $paletted", s->defaulttextures[0].paletted); break;
	case T_GEN_REFLECTCUBE:		Shader_DecomposeSubPassMap(o, s, "map $reflectcube", s->defaulttextures[0].reflectcube); break;
	case T_GEN_REFLECTMASK:		Shader_DecomposeSubPassMap(o, s, "map $reflectmask", s->defaulttextures[0].reflectmask); break;
	case T_GEN_DISPLACEMENT:	Shader_DecomposeSubPassMap(o, s, "map $displacement", s->defaulttextures[0].displacement); break;
	case T_GEN_OCCLUSION:		Shader_DecomposeSubPassMap(o, s, "map $occlusion", s->defaulttextures[0].occlusion); break;
	case T_GEN_TRANSMISSION:	Shader_DecomposeSubPassMap(o, s, "map $transmission", s->defaulttextures[0].transmission); break;
	case T_GEN_THICKNESS:		Shader_DecomposeSubPassMap(o, s, "map $thickness", s->defaulttextures[0].thickness); break;
	case T_GEN_CURRENTRENDER:	sprintf(o, "map $currentrender "); break;
	case T_GEN_SOURCECOLOUR:	sprintf(o, "map $sourcecolour"); break;
	case T_GEN_SOURCEDEPTH:		sprintf(o, "map $sourcedepth"); break;
	case T_GEN_REFLECTION:		sprintf(o, "map $reflection"); break;
	case T_GEN_REFRACTION:		sprintf(o, "map $refraction"); break;
	case T_GEN_REFRACTIONDEPTH:	sprintf(o, "map $refractiondepth"); break;
	case T_GEN_RIPPLEMAP:		sprintf(o, "map $ripplemap"); break;
	case T_GEN_SOURCECUBE:		sprintf(o, "map $sourcecube"); break;
	case T_GEN_GBUFFERCASE:		sprintf(o, "map $gbuffer%i ",p->texgen-T_GEN_GBUFFER0); break;
	}
	o+=strlen(o);

	sprintf(o, "\n"); o+=strlen(o);
	return o;
}
char *Shader_Decompose(shader_t *s)
{
	static char decomposebuf[32768];
	char *o = decomposebuf;
	shaderpass_t *p;
	unsigned int i, j;

	sprintf(o, "\n<---\n"); o+=strlen(o);

	switch (s->sort)
	{
	default:						sprintf(o, "sort %i\n", s->sort); break;
	case SHADER_SORT_NONE:			sprintf(o, "sort %i (SHADER_SORT_NONE)\n", s->sort); break;
	case SHADER_SORT_RIPPLE:		sprintf(o, "sort %i (SHADER_SORT_RIPPLE)\n", s->sort); break;
	case SHADER_SORT_DEFERREDLIGHT:	sprintf(o, "sort %i (SHADER_SORT_DEFERREDLIGHT)\n", s->sort); break;
	case SHADER_SORT_PORTAL:		sprintf(o, "sort %i (SHADER_SORT_PORTAL)\n", s->sort); break;
	case SHADER_SORT_SKY:			sprintf(o, "sort %i (SHADER_SORT_SKY)\n", s->sort); break;
	case SHADER_SORT_OPAQUE:		sprintf(o, "sort %i (SHADER_SORT_OPAQUE)\n", s->sort); break;
	case SHADER_SORT_DECAL:			sprintf(o, "sort %i (SHADER_SORT_DECAL)\n", s->sort); break;
	case SHADER_SORT_SEETHROUGH:	sprintf(o, "sort %i (SHADER_SORT_SEETHROUGH)\n", s->sort); break;
	case SHADER_SORT_BANNER:		sprintf(o, "sort %i (SHADER_SORT_BANNER)\n", s->sort); break;
	case SHADER_SORT_UNDERWATER:	sprintf(o, "sort %i (SHADER_SORT_UNDERWATER)\n", s->sort); break;
	case SHADER_SORT_BLEND:			sprintf(o, "sort %i (SHADER_SORT_BLEND)\n", s->sort); break;
	case SHADER_SORT_ADDITIVE:		sprintf(o, "sort %i (SHADER_SORT_ADDITIVE)\n", s->sort); break;
	case SHADER_SORT_NEAREST:		sprintf(o, "sort %i (SHADER_SORT_NEAREST)\n", s->sort); break;
	}
	o+=strlen(o);

	if (s->prog)
	{
		sprintf(o, "program %s\n", s->prog->name);
		o+=strlen(o);

		p = s->passes;
		o = Shader_DecomposePass(o, p, true);
		for (j = 0; j < s->numpasses; j++)
			o = Shader_DecomposeSubPass(o, s, p+j, true);
	}
	else
	{
		for (i = 0; i < s->numpasses; i+= p->numMergedPasses)
		{
			p = &s->passes[i];
			sprintf(o, "{\n"); o+=strlen(o);

			o = Shader_DecomposePass(o, p, false);
			for (j = 0; j < p->numMergedPasses; j++)
				o = Shader_DecomposeSubPass(o, s, p+j, !!p->prog);
			sprintf(o, "}\n"); o+=strlen(o);
		}
	}
	sprintf(o, "--->\n"); o+=strlen(o);
	return decomposebuf;
}
#endif

char *Shader_GetShaderBody(shader_t *s, char *fname, size_t fnamesize)
{
	parsestate_t ps;
	char *adr, *parsename=NULL, *argsstart;
	char cleanname[MAX_QPATH];
	char shortname[MAX_QPATH];
	char drivername[MAX_QPATH];
	int oldsort;
	qboolean resort = false;

	if (!s || !s->uses)
		return NULL;
	ps.s = s;

	adr = Z_StrDup("UNKNOWN BODY");
	ps.saveshaderbody = &adr;

	strcpy(cleanname, s->name);
	argsstart = strchr(cleanname, '#');
	if (argsstart)
		*argsstart = 0;
	COM_StripExtension (cleanname, shortname, sizeof(shortname));
	if (ruleset_allow_shaders.ival && !(s->usageflags & SUR_FORCEFALLBACK))
	{
		if (sh_config.shadernamefmt)
		{
			Q_snprintfz(drivername, sizeof(drivername), sh_config.shadernamefmt, cleanname);
			if (!parsename && Shader_ParseShader(&ps, drivername))
				parsename = drivername;
		}
		if (!parsename && Shader_ParseShader(&ps, cleanname))
			parsename = cleanname;
		if (!parsename && Shader_ParseShader(&ps, shortname))
			parsename = shortname;
	}
	if (!parsename && s->generator)
	{
		oldsort = s->sort;
		Shader_Regenerate(&ps, shortname);

		if (s->sort != oldsort)
			resort = true;
	}

	if (resort)
	{
		Mod_ResortShaders();
	}

	if (fnamesize)
	{
		*fname = 0;

		if (parsename)
		{
			unsigned int key;
			shadercache_t *cache;
			key = Hash_Key (parsename, HASH_SIZE);
			cache = shader_hash[key];
			for ( ; cache; cache = cache->hash_next)
			{
				if (!Q_stricmp (cache->name, parsename))
				{
					char *c, *stop;
					int line = 1;
					//okay, this is the shader we're looking for, we know where it came from too, so there's handy.
					//figure out the line index now, by just counting the \ns up to the offset
					for (c = cache->source->data, stop = c+cache->offset; c < stop; c++)
					{
						if (*c == '\n')
							line++;
					}
					Q_snprintfz(fname, fnamesize, "%s:%i", cache->source->name, line);
					break;
				}
			}

			if (!strchr(parsename, ':'))
			{
				//if the named shader is a .shader file then just directly load it.
				const char *token = COM_GetFileExtension(parsename, NULL);
				if (!strcmp(token, ".shader") || !*token)
				{
					char shaderfile[MAX_QPATH];
					if (!*token)
					{
						Q_snprintfz(shaderfile, sizeof(shaderfile), "%s.shader", parsename);
						if (COM_FCheckExists(shaderfile))
							Q_snprintfz(fname, fnamesize, "%s:%i", shaderfile, 1);
					}
					else if (COM_FCheckExists(parsename))
						Q_snprintfz(fname, fnamesize, "%s:%i", parsename, 1);
				}
			}
		}
	}

#ifdef _DEBUG
	{
		char *add, *ret;
		add = Shader_Decompose(s);
		if (*add)
		{
			ret = Z_Malloc(strlen(add) + strlen(adr) + 1);
			strcpy(ret, adr);
			strcpy(ret + strlen(ret), add);
			Z_Free(adr);
			adr = ret;
		}
	}
#endif

	return adr;
}

void Shader_ShowShader_f(void)
{
	char *sourcename = Cmd_Argv(1);
	shader_t *o = R_LoadShader(NULL, sourcename, SUF_NONE, NULL, NULL);
	if (!o)
		o = R_LoadShader(NULL, sourcename, SUF_LIGHTMAP, NULL, NULL);
	if (!o)
		o = R_LoadShader(NULL, sourcename, SUF_2D, NULL, NULL);
	if (o)
	{
		char fname[256];
		char *body = Shader_GetShaderBody(o, fname, sizeof(fname));
		if (body)
		{
			Con_Printf("^h(%s)^h\n%s\n{%s\n", fname, o->name, body);
			Z_Free(body);
		}
		else
		{
			Con_Printf("Shader \"%s\" is not in use\n", o->name);
		}
	}
	else
		Con_Printf("Shader \"%s\" is not loaded\n", sourcename);
}

void Shader_ShaderList_f(void)
{
	unsigned int i;
	// not loaded, find a free slot
	for (i = 0; i < r_numshaders; i++)
	{
		if (!r_shaders[i])
			continue;	//gap?
		Con_Printf("^[\\img\\%s\\imgtype\\%i\\s\\64^] ^2%s^7 [%i]", r_shaders[i]->name, r_shaders[i]->usageflags, r_shaders[i]->name, r_shaders[i]->usageflags);
		if (r_shaders[i]->width || r_shaders[i]->height)
			Con_Printf(" Size:%ix%i", r_shaders[i]->width, r_shaders[i]->height);
		if (r_shaders[i]->model)
			Con_Printf(" ^[%s\\modelviewer\\%s^]", r_shaders[i]->model->name, r_shaders[i]->model->name);
		Con_Printf("\n");
	}
}

void Shader_TouchTexnums(texnums_t *t)
{
	if (t->base)
		t->base->regsequence = r_regsequence;
	if (t->bump)
		t->bump->regsequence = r_regsequence;
	if (t->specular)
		t->specular->regsequence = r_regsequence;
	if (t->upperoverlay)
		t->upperoverlay->regsequence = r_regsequence;
	if (t->loweroverlay)
		t->loweroverlay->regsequence = r_regsequence;
	if (t->paletted)
		t->paletted->regsequence = r_regsequence;
	if (t->fullbright)
		t->fullbright->regsequence = r_regsequence;
	if (t->reflectcube)
		t->reflectcube->regsequence = r_regsequence;
	if (t->reflectmask)
		t->reflectmask->regsequence = r_regsequence;
	if (t->displacement)
		t->displacement->regsequence = r_regsequence;
	if (t->occlusion)
		t->occlusion->regsequence = r_regsequence;
}
void Shader_TouchTextures(void)
{
	int i, j, k;
	shader_t *s;
	shaderpass_t *p;
	for (i = 0; i < r_numshaders; i++)
	{
		s = r_shaders[i];
		if (!s || !s->uses)
			continue;

		for (j = 0; j < s->numpasses; j++)
		{
			p = &s->passes[j];
			for (k = 0; k < countof(p->anim_frames); k++)
				if (p->anim_frames[k])
					p->anim_frames[k]->regsequence = r_regsequence;
		}
		for (j = 0; j < max(1,s->numdefaulttextures); j++)
			Shader_TouchTexnums(&s->defaulttextures[j]);
	}
}

void Shader_DoReload(void)
{
	shader_t *s;
	unsigned int i;
	char shortname[MAX_QPATH];
	char cleanname[MAX_QPATH], *argsstart;
	int oldsort;
	qboolean resort = false;
	parsestate_t ps;

	//don't spam shader reloads while we're connecting, as that's just wasteful.
	if (cls.state && cls.state < ca_active)
		return;
	if (!r_shaders)
		return;	//err, not ready yet

	if (shader_rescan_needed)
	{
		Shader_FlushCache();

		if (ruleset_allow_shaders.ival)
		{
			COM_EnumerateFiles("materials/*.mtr", Shader_InitCallback_Doom3, NULL);
			COM_EnumerateFiles("shaders/*.shader", Shader_InitCallback, NULL);
			COM_EnumerateFiles("scripts/*.shader", Shader_InitCallback, NULL);
			COM_EnumerateFiles("scripts/*.rscript", Shader_InitCallback, NULL);
		}

		shader_reload_needed = true;
		shader_rescan_needed = false;

		Con_DPrintf("Rescanning shaders\n");
	}
	else
	{
		if (!shader_reload_needed)
			return;
		Con_DPrintf("Reloading shaders\n");
	}
	shader_reload_needed = false;
	R2D_ImageColours(1,1,1,1);
	TRACE(("Reloading generics\n"));
	Shader_ReloadGenerics();

	for (i = 0; i < r_numshaders; i++)
	{
		s = r_shaders[i];
		if (!s || !s->uses)
			continue;
		ps.s = s;
		ps.saveshaderbody = NULL;

		strcpy(cleanname, s->name);
		argsstart = *cleanname?strchr(cleanname+1, '#'):NULL;
		if (argsstart)
			*argsstart = 0;
		COM_StripExtension (cleanname, shortname, sizeof(shortname));
		TRACE(("reparsing %s\n", s->name));
		if (ruleset_allow_shaders.ival && !(s->usageflags & SUR_FORCEFALLBACK))
		{
			if (sh_config.shadernamefmt)
			{
				char drivername[MAX_QPATH];
				Q_snprintfz(drivername, sizeof(drivername), sh_config.shadernamefmt, cleanname);
				if (Shader_ParseShader(&ps, drivername))
					continue;
			}
			if (Shader_ParseShader(&ps, cleanname))
				continue;
			if (strcmp(cleanname, shortname))
				if (Shader_ParseShader(&ps, shortname))
					continue;
		}
		if (s->generator)
		{
			oldsort = s->sort;
			Shader_Regenerate(&ps, shortname);
			if (s->sort != oldsort)
				resort = true;
		}
	}

	TRACE(("Resorting shaders\n"));

	if (resort)
	{
		Mod_ResortShaders();
	}
}

void Shader_NeedReload(qboolean rescanfs)
{
	if (rescanfs)
		shader_rescan_needed = true;
	shader_reload_needed = true;
}

cin_t *QDECL R_ShaderGetCinematic(shader_t *s)
{
#ifdef HAVE_MEDIA_DECODER
	int j;
	if (!s)
		return NULL;
	for (j = 0; j < s->numpasses; j++)
		if (s->passes[j].cin)
			return s->passes[j].cin;
#endif
	/*no cinematic in this shader!*/
	return NULL;
}

shader_t *R_ShaderFind(const char *name)
{
	int i;
	char shortname[MAX_QPATH];
	shader_t *s;

	if (!r_shaders)
		return NULL;

	COM_StripExtension ( name, shortname, sizeof(shortname));

	COM_CleanUpPath(shortname);

	//try and find it
	for (i = 0; i < r_numshaders; i++)
	{
		s = r_shaders[i];
		if (!s || !s->uses)
			continue;

		if (!Q_stricmp (shortname, s->name) )
			return s;
	}
	return NULL;
}

cin_t *R_ShaderFindCinematic(const char *name)
{
#ifdef HAVE_MEDIA_DECODER
	return R_ShaderGetCinematic(R_ShaderFind(name));
#else
	return NULL;
#endif
}

void Shader_ResetRemaps(void)
{
	shader_t *s;
	int i;
	for (i = 0; i < r_numshaders; i++)
	{
		s = r_shaders[i];
		if (!s)
			continue;
		s->remapto = s;
		s->remaptime = 0;
	}
}

void R_RemapShader(const char *sourcename, const char *destname, float timeoffset)
{
	shader_t *o;
	shader_t *n;
	int i;
	size_t l;

	char cleansrcname[MAX_QPATH];
	Q_strncpyz(cleansrcname, sourcename, sizeof(cleansrcname));
	COM_CleanUpPath(cleansrcname);
	l = strlen(cleansrcname);

	for (i = 0; i < r_numshaders; i++)
	{
		o = r_shaders[i];
		if (o && o->uses)
		{
			if (!strncmp(o->name, cleansrcname, l) && (!o->name[l] || o->name[l]=='#'))
			{
				n = R_LoadShader (o->model, va("%s%s", destname, o->name+l), o->usageflags, NULL, NULL);
				if (!n)
				{	//if it isn't actually available on disk then don't care about usageflags, just find ANY that's already loaded.
					// check the hash first
					char cleandstname[MAX_QPATH];
					Q_strncpyz(cleandstname, destname, sizeof(cleandstname));
					COM_CleanUpPath(cleandstname);
					n = Hash_Get(&shader_active_hash, cleandstname);

					// if one of our shaders is made for lightmaps, check through the rest until we find one more suitable
					if ((n->usageflags ^ o->usageflags) & SUF_LIGHTMAP)
					{
						shader_t *n_f = n;
						while (n)
						{
							if (!((n->usageflags ^ o->usageflags) & SUF_LIGHTMAP))
								break;

							n = Hash_GetNext(&shader_active_hash, cleandstname, n);
						}

						if (!n)
							n = n_f;
					}

					if (!n || !n->uses)
						n = o;
				}
				o->remapto = n;
				o->remaptime = timeoffset;	//this just feels wrong.
			}
		}
	}
}

void Shader_RemapShader_f(void)
{
	char *sourcename = Cmd_Argv(1);
	char *destname = Cmd_Argv(2);
	float timeoffset = atof(Cmd_Argv(3));
	
	if (!Cmd_FromGamecode() && strcmp(InfoBuf_ValueForKey(&cl.serverinfo, "*cheats"), "ON"))
	{
		Con_Printf("%s may only be used from gamecode, or when cheats are enabled\n", Cmd_Argv(0));
		return;
	}
	if (!*sourcename)
	{
		Con_Printf("%s originalshader remappedshader starttime\n", Cmd_Argv(0));
		return;
	}
	R_RemapShader(sourcename, destname, timeoffset);
}

//blocks
int R_GetShaderSizes(shader_t *shader, int *width, int *height, qboolean blocktillloaded)
{
	if (!shader)
		return false;
	if (!shader->width && !shader->height)
	{
		int i;
		if (width)
			*width = 0;
		if (height)
			*height = 0;
		if ((shader->flags & SHADER_HASDIFFUSE) && shader->defaulttextures->base)
		{
			if (shader->defaulttextures->base->status == TEX_LOADING)
			{
				if (!blocktillloaded)
					return -1;
				COM_WorkerPartialSync(shader->defaulttextures->base, &shader->defaulttextures->base->status, TEX_LOADING);
			}
			if (shader->defaulttextures->base->status == TEX_LOADED)
			{
				shader->width = shader->defaulttextures->base->width;
				shader->height = shader->defaulttextures->base->height;
			}
		}
		else if ((shader->flags & SHADER_HASPALETTED) && shader->defaulttextures->paletted)
		{
			if (shader->defaulttextures->paletted->status == TEX_LOADING)
			{
				if (!blocktillloaded)
					return -1;
				COM_WorkerPartialSync(shader->defaulttextures->paletted, &shader->defaulttextures->paletted->status, TEX_LOADING);
			}
			if (shader->defaulttextures->paletted->status == TEX_LOADED)
			{
				shader->width = shader->defaulttextures->paletted->width;
				shader->height = shader->defaulttextures->paletted->height;
			}
		}
		else
		{
			for (i = 0; i < shader->numpasses; i++)
			{
				if (shader->passes[i].texgen == T_GEN_SINGLEMAP && shader->passes[i].anim_frames[0] && shader->passes[i].anim_frames[0]->status == TEX_LOADING)
				{
					if (!blocktillloaded)
						return -1;
					COM_WorkerPartialSync(shader->passes[i].anim_frames[0], &shader->passes[i].anim_frames[0]->status, TEX_LOADING);
				}
				if (shader->passes[i].texgen == T_GEN_DIFFUSE && (shader->defaulttextures->base && shader->defaulttextures->base->status == TEX_LOADING))
				{
					if (!blocktillloaded)
						return -1;
					COM_WorkerPartialSync(shader->defaulttextures->base, &shader->defaulttextures->base->status, TEX_LOADING);
				}
				if (shader->passes[i].texgen == T_GEN_PALETTED && (shader->defaulttextures->paletted && shader->defaulttextures->paletted->status == TEX_LOADING))
				{
					if (!blocktillloaded)
						return -1;
					COM_WorkerPartialSync(shader->defaulttextures->paletted, &shader->defaulttextures->paletted->status, TEX_LOADING);
				}
			}

			for (i = 0; i < shader->numpasses; i++)
			{
				if (shader->passes[i].texgen == T_GEN_SINGLEMAP)
				{
					if (shader->passes[i].anim_frames[0] && shader->passes[i].anim_frames[0]->status == TEX_LOADED)
					{
						shader->width = shader->passes[i].anim_frames[0]->width;
						shader->height = shader->passes[i].anim_frames[0]->height;
					}
					break;
				}
				if (shader->passes[i].texgen == T_GEN_DIFFUSE)
				{
					if (shader->defaulttextures->base && shader->defaulttextures->base->status == TEX_LOADED)
					{
						shader->width = shader->defaulttextures->base->width;
						shader->height = shader->defaulttextures->base->height;
					}
					break;
				}
				if (shader->passes[i].texgen == T_GEN_PALETTED)
				{
					if (shader->defaulttextures->paletted && shader->defaulttextures->paletted->status == TEX_LOADED)
					{
						shader->width = shader->defaulttextures->paletted->width;
						shader->height = shader->defaulttextures->paletted->height;
					}
					break;
				}
			}
			if (i == shader->numpasses)
			{	//this shader has no textures from which to source a width and height
				if (!shader->width)
					shader->width = 64;
				if (!shader->height)
					shader->height = 64;
			}
		}
	}
	if (shader->width && shader->height)
	{
		if (width)
			*width = shader->width;
		if (height)
			*height = shader->height;
		return true;	//final size
	}
	else
	{
		//fill with dummy values
		if (width)
			*width = 64;
		if (height)
			*height = 64;
		return false;
	}
}
shader_t *R_RegisterPic (const char *name, const char *subdirs)
{
	shader_t *shader;
	shader = R_LoadShader (NULL, name, SUF_2D, Shader_Default2D, subdirs);
	return shader;
}

shader_t *QDECL R_RegisterShader (const char *name, unsigned int usageflags, const char *shaderscript)
{
	return R_LoadShader (NULL, name, usageflags, Shader_DefaultScript, shaderscript);
}

shader_t *R_RegisterShader_Lightmap (model_t *mod, const char *name)
{
	return R_LoadShader (mod, name, SUF_LIGHTMAP, Shader_DefaultBSPLM, NULL);
}

shader_t *R_RegisterShader_Vertex (model_t *mod, const char *name)
{
	return R_LoadShader (mod, name, 0, Shader_DefaultBSPVertex, NULL);
}

shader_t *R_RegisterShader_Flare (model_t *mod, const char *name)
{
	return R_LoadShader (mod, name, 0, Shader_DefaultBSPFlare, NULL);
}

shader_t *QDECL R_RegisterSkin (model_t *mod, const char *shadername)
{
	char newsname[MAX_QPATH];
	shader_t *shader;
#ifdef _DEBUG
	if (shadername == com_token)
		Con_Printf("R_RegisterSkin was passed com_token. that will bug out.\n");
#endif

	newsname[0] = 0;
	if (mod && !strchr(shadername, '/') && *shadername)
	{
		char *modname = mod->name;
		char *b = COM_SkipPath(modname);
		if (b != modname && b-modname + strlen(shadername)+1 < sizeof(newsname))
		{
			b--;	//no trailing /
			memcpy(newsname, modname, b - modname);
			newsname[b-modname] = 0;
		}
	}
	if (*newsname)
	{
		int l = strlen(newsname);
		Q_strncpyz(newsname+l, ":models", sizeof(newsname)-l);
	}
	else
		Q_strncpyz(newsname, "models", sizeof(newsname));
	shader = R_LoadShader (mod, shadername, 0, Shader_DefaultSkin, newsname);
	return shader;
}
shader_t *R_RegisterCustom (model_t *mod, const char *name, unsigned int usageflags, shader_gen_t *defaultgen, const void *args)
{
	return R_LoadShader (mod, name, usageflags, defaultgen, args);
}
#endif //SERVERONLY
