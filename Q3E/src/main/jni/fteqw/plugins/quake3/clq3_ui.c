#include "q3common.h"
#ifdef VM_UI
#include "clq3defs.h"
#include "ui_public.h"
#include "cl_master.h"
#include "shader.h"

static int keycatcher;

#include "botlib/botlib.h"
void SV_InitBotLib(void);
extern botlib_export_t *botlib;
qboolean CG_GetLimboString(int index, char *outbuf);

#define TT_STRING					1			// string
#define TT_LITERAL					2			// literal
#define TT_NUMBER					3			// number
#define TT_NAME						4			// name
#define TT_PUNCTUATION				5			// punctuation

#define SCRIPT_MAXDEPTH 64
#define SCRIPT_DEFINELENGTH 256
typedef struct {
	char *filestack[SCRIPT_MAXDEPTH];
	char *originalfilestack[SCRIPT_MAXDEPTH];
	char *lastreadptr;
	int lastreaddepth;
	char filename[MAX_QPATH][SCRIPT_MAXDEPTH];
	int stackdepth;

	char *defines;
	int numdefines;
} script_t;
static script_t *scripts;
static int maxscripts;
static float ui_size[2];	//to track when it needs to be restarted (the api has no video mode changed event)
static menu_t uimenu;

void Q3_SetKeyCatcher(int newcatcher)
{
	int delta = newcatcher^keycatcher;
	keycatcher = newcatcher;
	if (delta & 2)
	{
		uimenu.isopaque = false; //no surprises.
		if (newcatcher&2)
			inputfuncs->Menu_Push(&uimenu, false);
		else
			inputfuncs->Menu_Unlink(&uimenu, false);
	}
}
int Q3_GetKeyCatcher(void)
{
	return keycatcher;
}
#define Q3SCRIPTPUNCTUATION "(,{})(\':;=!><&|+-\""
void StripCSyntax (char *s)
{
	while(*s)
	{
		if (*s == '\\')
		{
			memmove(s, s+1, strlen(s+1)+1);
			switch (*s)
			{
			case 'r':
				*s = '\r';
				break;
			case 'n':
				*s = '\n';
				break;
			case '\\':
				*s = '\\';
				break;
			default:
				*s = '?';
				break;
			}
		}
		s++;
	}
}
int Script_Read(int handle, struct pc_token_s *token)
{
	char *s;
	char readstring[8192];
	int i;
	script_t *sc = scripts+handle-1;
	char thetoken[1024];
	com_tokentype_t tokentype;

	for(;;)
	{
		if (!sc->stackdepth)
		{
			memset(token, 0, sizeof(*token));
			return 0;
		}

		s = sc->filestack[sc->stackdepth-1];
		sc->lastreadptr = s;
		sc->lastreaddepth = sc->stackdepth;

		s = (char *)cmdfuncs->ParsePunctuation(s, Q3SCRIPTPUNCTUATION, thetoken, sizeof(thetoken), &tokentype);
		Q_strncpyz(readstring, thetoken, sizeof(readstring));
		if (tokentype == TTP_STRING)
		{
			while(s)
			{
				while (*s > '\0' && *s <= ' ')
					s++;
				if (*s == '/' && s[1] == '/')
				{
					while(*s && *s != '\n')
						s++;
					continue;
				}
				while (*s > '\0' && *s <= ' ')
					s++;
				if (*s == '\"')
				{
					s = (char*)cmdfuncs->ParsePunctuation(s, Q3SCRIPTPUNCTUATION, thetoken, sizeof(thetoken), &tokentype);
					Q_strncatz(readstring, thetoken, sizeof(readstring));
				}
				else
					break;
			}
		}
		sc->filestack[sc->stackdepth-1] = s;
		if (tokentype == TTP_LINEENDING)
			continue;	//apparently we shouldn't stop on linebreaks

		if (!strcmp(readstring, "#include"))
		{
			sc->filestack[sc->stackdepth-1] = (char *)cmdfuncs->ParsePunctuation(sc->filestack[sc->stackdepth-1], Q3SCRIPTPUNCTUATION, thetoken, sizeof(thetoken), &tokentype);

			if (sc->stackdepth == SCRIPT_MAXDEPTH)	//just don't enter it
				continue;

			if (sc->originalfilestack[sc->stackdepth])
				plugfuncs->Free(sc->originalfilestack[sc->stackdepth]);
			sc->filestack[sc->stackdepth] = sc->originalfilestack[sc->stackdepth] = fsfuncs->LoadFile(thetoken, NULL);
			Q_strncpyz(sc->filename[sc->stackdepth], thetoken, MAX_QPATH);
			sc->stackdepth++;
			continue;
		}
		if (!strcmp(readstring, "#define"))
		{
			sc->numdefines++;
			sc->defines = plugfuncs->Realloc(sc->defines, sc->numdefines*SCRIPT_DEFINELENGTH*2);
			sc->filestack[sc->stackdepth-1] = (char *)cmdfuncs->ParsePunctuation(sc->filestack[sc->stackdepth-1], Q3SCRIPTPUNCTUATION, thetoken, sizeof(thetoken), &tokentype);
			Q_strncpyz(sc->defines+SCRIPT_DEFINELENGTH*2*(sc->numdefines-1), thetoken, SCRIPT_DEFINELENGTH);
			sc->filestack[sc->stackdepth-1] = (char *)cmdfuncs->ParsePunctuation(sc->filestack[sc->stackdepth-1], Q3SCRIPTPUNCTUATION, thetoken, sizeof(thetoken), &tokentype);
			Q_strncpyz(sc->defines+SCRIPT_DEFINELENGTH*2*(sc->numdefines-1)+SCRIPT_DEFINELENGTH, thetoken, SCRIPT_DEFINELENGTH);

			continue;
		}
		if (!*readstring && tokentype != TTP_STRING)
		{
			if (sc->stackdepth==0)
			{
				memset(token, 0, sizeof(*token));
				return 0;
			}

			sc->stackdepth--;
			continue;
		}
		break;
	}
	if (tokentype == TTP_STRING)
	{
		i = sc->numdefines;
	}
	else
	{
		for (i = 0; i < sc->numdefines; i++)
		{
			if (!strcmp(readstring, sc->defines+SCRIPT_DEFINELENGTH*2*i))
			{
				Q_strncpyz(token->string, sc->defines+SCRIPT_DEFINELENGTH*2*i+SCRIPT_DEFINELENGTH, sizeof(token->string));
				break;
			}
		}
	}
	if (i == sc->numdefines)	//otherwise
		Q_strncpyz(token->string, readstring, sizeof(token->string));

	StripCSyntax(token->string);

	if (token->string[0] == '0' && (token->string[1] == 'x'||token->string[1] == 'X'))
	{
		token->intvalue = strtoul(token->string, NULL, 16);
		token->floatvalue = token->intvalue;
		token->type = TT_NUMBER;
		token->subtype = 0x100;//TT_HEX;
	}
	else
	{
		token->intvalue = atoi(token->string);
		token->floatvalue = atof(token->string);
		if (token->floatvalue || *token->string == '0' || *token->string == '.')
		{
			token->type = TT_NUMBER;
			token->subtype = 0;
		}
		else if (tokentype == TTP_STRING)
		{
			token->type = TT_STRING;
			token->subtype = strlen(token->string);
		}
		else
		{
			if (token->string[1] == '\0')
			{
				token->type = TT_PUNCTUATION;
				token->subtype = token->string[0];
			}
			else
			{
				token->type = TT_NAME;
				token->subtype = strlen(token->string);
			}
		}
	}

//	Con_Printf("Found %s (%i, %i)\n", token->string, token->type, token->subtype);
	return tokentype != TTP_EOF;
}

int Script_LoadFile(char *filename)
{
	int i;
	script_t *sc;
	for (i = 0; i < maxscripts; i++)
		if (!scripts[i].stackdepth)
			break;
	if (i == maxscripts)
	{
		maxscripts++;
		scripts = plugfuncs->Realloc(scripts, sizeof(script_t)*maxscripts);
	}
	
	sc = scripts+i;
	memset(sc, 0, sizeof(*sc));
	sc->filestack[0] = sc->originalfilestack[0] = fsfuncs->LoadFile(filename, NULL);
	Q_strncpyz(sc->filename[sc->stackdepth], filename, MAX_QPATH);
	sc->stackdepth = 1;

	return i+1;
}

void Script_Free(int handle)
{
	int i;
	script_t *sc = scripts+handle-1;
	if (sc->defines)
		plugfuncs->Free(sc->defines);

	for (i = 0; i < sc->stackdepth; i++)
		plugfuncs->Free(sc->originalfilestack[i]);

	sc->stackdepth = 0;
}

void Script_Get_File_And_Line(int handle, char *filename, int *line)
{
	script_t *sc = scripts+handle-1;
	char *src;
	char *start;

	if (!sc->lastreaddepth)
	{
		*line = 0;
		Q_strncpyz(filename, sc->filename[0], MAX_QPATH);
		return;
	}
	*line = 1;

	src = sc->lastreadptr;
	start = sc->originalfilestack[sc->lastreaddepth-1];

	while(start < src)
	{
		if (*start == '\n')
			(*line)++;
		start++;
	}

	Q_strncpyz(filename, sc->filename[sc->lastreaddepth-1], MAX_QPATH);

}











static vm_t *uivm;

#define MAX_PINGREQUESTS 32

static struct
{
	unsigned int startms;
	netadr_t adr;
	char adrstring[64];
	const char *broker;
} ui_pings[MAX_PINGREQUESTS];

#define UITAGNUM 2452

struct q3refEntity_s {
	refEntityType_t	reType;
	int			renderfx;

	int hModel;				// opaque type outside refresh

	// most recent data
	vec3_t		lightingOrigin;		// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float		shadowPlane;		// projection shadows go here, stencils go slightly lower

	vec3_t		axis[3];			// rotation vectors
	qboolean	nonNormalizedAxes;	// axis are not normalized, i.e. they have scale
	float		origin[3];			// also used as MODEL_BEAM's "from"
	int			frame;				// also used as MODEL_BEAM's diameter

	// previous data for frame interpolation
	float		oldorigin[3];		// also used as MODEL_BEAM's "to"
	int			oldframe;
	float		backlerp;			// 0.0 = current, 1.0 = old

	// texturing
	int			skinNum;			// inline skin index
	skinid_t	customSkin;			// NULL for default skin
	int			customShader;		// use one image for the entire thing

	// misc
	qbyte		shaderRGBA[4];		// colors used by rgbgen entity shaders
	float		shaderTexCoord[2];	// texture coordinates used by tcMod entity modifiers
	float		shaderTime;			// subtracted from refdef time to control effect start times

	// extra sprite information
	float		radius;
	float		rotation;
};

struct q3polyvert_s
{
	vec3_t org;
	vec2_t tcoord;
	qbyte colours[4];
};

#define Q3RF_MINLIGHT			1
#define	Q3RF_THIRD_PERSON		2		// don't draw through eyes, only mirrors (player bodies, chat sprites)
#define	Q3RF_FIRST_PERSON		4		// only draw through eyes (view weapon, damage blood blob)
#define	Q3RF_DEPTHHACK			8		// for view weapon Z crunching
#define Q3RF_NOSHADOW			64
#define Q3RF_LIGHTING_ORIGIN	128

#define MAX_VMQ3_CACHED_STRINGS 2048
static char *stringcache[MAX_VMQ3_CACHED_STRINGS];

void VMQ3_FlushStringHandles(void)
{
	int i;
	for (i = 0; i < MAX_VMQ3_CACHED_STRINGS; i++)
	{
		if (stringcache[i])
		{
			Z_Free(stringcache[i]);
			stringcache[i] = NULL;
		}
	}
}

char *VMQ3_StringFromHandle(int handle)
{
	if (!handle)
		return "";
	handle--;
	if ((unsigned) handle >= MAX_VMQ3_CACHED_STRINGS)
		return "";
	return stringcache[handle];
}

int VMQ3_StringToHandle(char *str)
{
	int i;
	for (i = 0; i < MAX_VMQ3_CACHED_STRINGS; i++)
	{
		if (!stringcache[i])
			break;
		if (!strcmp(str, stringcache[i]))
			return i+1;
	}
	if (i == MAX_VMQ3_CACHED_STRINGS)
	{
		Con_Printf("Q3VM out of string handle space\n");
		return 0;
	}
	stringcache[i] = Z_Malloc(strlen(str)+1);
	strcpy(stringcache[i], str);
	return i+1;
}

#define VM_TOSTRCACHE(a) VMQ3_StringToHandle(VM_POINTER(a))
#define VM_FROMSTRCACHE(a) VMQ3_StringFromHandle(a)

void VQ3_AddEntity(const q3refEntity_t *q3)
{
	entity_t ent;
	memset(&ent, 0, sizeof(ent));
	ent.model = scenefuncs->ModelFromId(q3->hModel);
	ent.framestate.g[FS_REG].frame[0] = q3->frame;
	ent.framestate.g[FS_REG].frame[1] = q3->oldframe;
	memcpy(ent.axis, q3->axis, sizeof(q3->axis));
	ent.framestate.g[FS_REG].lerpweight[1] = q3->backlerp;
	ent.framestate.g[FS_REG].lerpweight[0] = 1 - ent.framestate.g[FS_REG].lerpweight[1];
	if (q3->reType == RT_SPRITE)
	{
		ent.scale = q3->radius;
		ent.rotation = q3->rotation;
	}
	else
		ent.scale = 1;
	ent.rtype = q3->reType;

	ent.customskin = q3->customSkin;
	ent.skinnum = q3->skinNum;

	ent.shaderRGBAf[0] = q3->shaderRGBA[0]/255.0f;
	ent.shaderRGBAf[1] = q3->shaderRGBA[1]/255.0f;
	ent.shaderRGBAf[2] = q3->shaderRGBA[2]/255.0f;
	ent.shaderRGBAf[3] = q3->shaderRGBA[3]/255.0f;

	/*don't set force-translucent etc, the shader is meant to already be correct*/
//	if (ent.shaderRGBAf[3] <= 0)
//		return;

	ent.forcedshader = drawfuncs->ShaderFromId(q3->customShader);
	ent.shaderTime = q3->shaderTime;

	if (q3->renderfx & Q3RF_FIRST_PERSON)
		ent.flags |= RF_FIRSTPERSON;
	if (q3->renderfx & Q3RF_DEPTHHACK)
		ent.flags |= RF_DEPTHHACK;
	if (q3->renderfx & Q3RF_THIRD_PERSON)
		ent.flags |= RF_EXTERNALMODEL;
	if (q3->renderfx & Q3RF_NOSHADOW)
		ent.flags |= RF_NOSHADOW;

	ent.topcolour = TOP_DEFAULT;
	ent.bottomcolour = BOTTOM_DEFAULT;
	ent.playerindex = -1;


	if ((q3->renderfx & Q3RF_LIGHTING_ORIGIN) && ent.model)
	{
		VectorCopy(q3->lightingOrigin, ent.origin);
		scenefuncs->CalcModelLighting(&ent, ent.model);
	}

	VectorCopy(q3->origin, ent.origin);
	VectorCopy(q3->oldorigin, ent.oldorigin);

	scenefuncs->AddEntity(&ent);
}

void VQ3_AddPolys(shader_t *s, int numverts, q3polyvert_t *verts, size_t polycount)
{
	unsigned int v;

	for (; polycount-->0; verts += numverts)
	{
		vecV_t *vertcoord;
		vec2_t *texcoord;
		vec4_t *colour;
		index_t *indexes;
		index_t indexbias = scenefuncs->AddPolydata(s, BEF_NODLIGHT|BEF_NOSHADOWS, numverts, (numverts-2)*3, &vertcoord, &texcoord, &colour, &indexes);

		//and splurge the data out.
		for (v = 0; v < numverts; v++)
		{
			VectorCopy(verts[v].org, vertcoord[v]);
			Vector2Copy(verts[v].tcoord, texcoord[v]);
			Vector4Scale(verts[v].colours, (1/255.0f), colour[v]);
		}
		for (v = 2; v < numverts; v++)
		{
			*indexes++ = indexbias + 0;
			*indexes++ = indexbias + (v-1);
			*indexes++ = indexbias + v;
		}
	}
}

int VM_LerpTag(void *out, model_t *model, int f1, int f2, float l2, char *tagname)
{
	int tagnum;
	float *ang;
	float *org;

	float tr[12];
	qboolean found;
	framestate_t fstate;

	org = (float*)out;
	ang = ((float*)out+3);

	memset(&fstate, 0, sizeof(fstate));
	fstate.g[FS_REG].frame[0] = f1;
	fstate.g[FS_REG].frame[1] = f2;
	fstate.g[FS_REG].lerpweight[0] = 1 - l2;
	fstate.g[FS_REG].lerpweight[1] = l2;

	tagnum = scenefuncs->TagNumForName(model, tagname, 0);
	found = scenefuncs->GetTag(model, tagnum, &fstate, tr);

	if (found && tagnum)
	{
		ang[0] = tr[0];
		ang[1] = tr[1];
		ang[2] = tr[2];
		org[0] = tr[3];

		ang[3] = tr[4];
		ang[4] = tr[5];
		ang[5] = tr[6];
		org[1] = tr[7];

		ang[6] = tr[8];
		ang[7] = tr[9];
		ang[8] = tr[10];
		org[2] = tr[11];

		return true;
	}
	else
	{
		org[0] = 0;
		org[1] = 0;
		org[2] = 0;

		ang[0] = 1;
		ang[1] = 0;
		ang[2] = 0;

		ang[3] = 0;
		ang[4] = 1;
		ang[5] = 0;

		ang[6] = 0;
		ang[7] = 0;
		ang[8] = 1;

		return false;
	}
}

#define	MAX_RENDER_STRINGS			8
#define	MAX_RENDER_STRING_LENGTH	32

struct q3refdef_s {
	int			x, y, width, height;
	float		fov_x, fov_y;
	vec3_t		vieworg;
	vec3_t		viewaxis[3];		// transformation matrix

	// time in milliseconds for shader effects and other time dependent rendering issues
	int			time;

	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	qbyte		areamask[MAX_MAP_AREA_BYTES];

	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
};

void VQ3_RenderView(const q3refdef_t *ref)
{
	plugrefdef_t scene;

	scene.flags = 0;


	scene.rect.x = ref->x;
	scene.rect.y = ref->y;
	scene.rect.w = ref->width;
	scene.rect.h = ref->height;
	scene.fov[0] = scene.fov_viewmodel[0] = ref->fov_x;
	scene.fov[1] = scene.fov_viewmodel[1] = ref->fov_y;
	VectorCopy(ref->viewaxis[0], scene.viewaxisorg[0]);
	VectorCopy(ref->viewaxis[1], scene.viewaxisorg[1]);
	VectorCopy(ref->viewaxis[2], scene.viewaxisorg[2]);
	VectorCopy(ref->vieworg, scene.viewaxisorg[3]);
	scene.time = ref->time/1000.0f;

	if (ref->rdflags & 1/*Q3RDF_NOWORLDMODEL*/)
		scene.flags |= RDF_NOWORLDMODEL;

	scenefuncs->RenderScene(&scene, sizeof(ref->areamask), ref->areamask);
}

static void *Little4Block(void *out, qbyte *in, size_t count)
{
	while (count-->0)
	{
		*(unsigned int*)out = (in[0]<<0)|(in[1]<<8)|(in[2]<<16)|(in[3]<<24);
		out = (qbyte*)out + 4;
		in+=4;
	}
	return in;
}
void UI_RegisterFont(char *fontName, int pointSize, fontInfo_t *font)
{
	union 
	{
		char *c;
		int *i;
		float *f;
	} in;
	int i;
	char name[MAX_QPATH];
	void *filedata;
	size_t sz;

	snprintf(name, sizeof(name), "fonts/fontImage_%i.dat",pointSize);

	in.c = filedata = fsfuncs->LoadFile(name, &sz);
	if (sz == sizeof(fontInfo_t))
	{
		for(i=0; i<GLYPHS_PER_FONT; i++)
		{
			in.c = Little4Block(&font->glyphs[i].height, in.c, (int*)font->glyphs[i].shaderName-&font->glyphs[i].height);
			memcpy(font->glyphs[i].shaderName, in.i, 32);
			in.c += 32;
		}
		in.c = Little4Block(&font->glyphScale, in.c, 1);
		memcpy(font->name, in.i, sizeof(font->name));

//		Com_Memcpy(font, faceData, sizeof(fontInfo_t));
		Q_strncpyz(font->name, name, sizeof(font->name));
		for (i = GLYPH_START; i < GLYPH_END; i++)
			font->glyphs[i].glyph = drawfuncs->LoadImage(font->glyphs[i].shaderName);
	}
	plugfuncs->Free(filedata);
}

static struct
{
	int shaderhandle;
	int x, y, w, h;
	qboolean loop;
} uicinematics[16];
int UI_Cin_Play(const char *name, int x, int y, int w, int h, unsigned int flags)
{
	int idx;
	qhandle_t mediashader;
	cin_t *cin;
	for (idx = 0; ; idx++)
	{
		if (idx == countof(uicinematics))
			return -1;	//out of handles
		if (uicinematics[idx].shaderhandle)
			continue;	//slot in use
		break;	//this slot is usable
	}

	mediashader = drawfuncs->LoadImageShader(name, va(
			"{\n"
				"program default2d\n"
				"{\n"
					"videomap \"video/%s\"\n"
					"blendfunc gl_one gl_one_minus_src_alpha\n"
				"}\n"
			"}\n", name));
	if (!mediashader)
		return -1;	//wtf?
	cin = drawfuncs->media.GetCinematic(drawfuncs->ShaderFromId(mediashader));
	if (cin)
		drawfuncs->media.SetState(cin, CINSTATE_PLAY);
	else
		return -1;	//FAIL!

	uicinematics[idx].x = x;
	uicinematics[idx].y = y;
	uicinematics[idx].w = w;
	uicinematics[idx].h = h;
	uicinematics[idx].loop = !!(flags&1);
	uicinematics[idx].shaderhandle = mediashader;

	return idx;
}
int UI_Cin_Stop(int idx)
{
	if (idx >= 0 && idx < countof(uicinematics))
	{
		drawfuncs->UnloadImage(uicinematics[idx].shaderhandle);
		uicinematics[idx].shaderhandle = 0;
	}
	return 0;
}
int UI_Cin_Run(int idx)
{
    enum {
		FMV_IDLE,
		FMV_PLAY,       // play
		FMV_EOF,        // all other conditions, i.e. stop/EOF/abort
		FMV_ID_BLT,
		FMV_ID_IDLE,
		FMV_LOOPED,
		FMV_ID_WAIT
    };
	int ret = FMV_IDLE;


	if (idx >= 0 && idx < countof(uicinematics))
	{
		shader_t *shader = drawfuncs->ShaderFromId(uicinematics[idx].shaderhandle);
		cin_t *cin = drawfuncs->media.GetCinematic(shader);
		if (cin)
		{
			switch((cinstates_t)drawfuncs->media.GetState(cin))
			{
			case CINSTATE_INVALID:	ret = FMV_IDLE;	break;
			case CINSTATE_PLAY:		ret = FMV_PLAY; break;
			case CINSTATE_LOOP:		ret = FMV_PLAY; break;
			case CINSTATE_PAUSE:	ret = FMV_PLAY; break;
			case CINSTATE_ENDED:
				drawfuncs->media.SetState(cin, CINSTATE_FLUSHED);
				ret = FMV_EOF;
				break;
			case CINSTATE_FLUSHED:
				//FIXME: roq decoder has no reset method!
				drawfuncs->media.Reset(cin);
				ret = FMV_LOOPED;
				break;
			}
		}
	}
	return ret;
}
int UI_Cin_Draw(int idx)
{
	if (idx >= 0 && idx < countof(uicinematics))
	{
		unsigned int size[2];
		qhandle_t shader = uicinematics[idx].shaderhandle;
		float x = uicinematics[idx].x;
		float y = uicinematics[idx].y;
		float w = uicinematics[idx].w;
		float h = uicinematics[idx].h;

		drawfuncs->GetVideoSize(NULL, size);

		//gah! q3 compat sucks!
		x *= size[0]/640.0;
		w *= size[0]/640.0;
		y *= size[1]/480.0;
		h *= size[1]/480.0;

		drawfuncs->Image(x, y, w, h, 0, 0, 1, 1, shader);
	}
	return 0;
}
int UI_Cin_SetExtents(int idx, int x, int y, int w, int h)
{
	if (idx >= 0 && idx < countof(uicinematics))
	{
		uicinematics[idx].x = x;
		uicinematics[idx].y = y;
		uicinematics[idx].w = w;
		uicinematics[idx].h = h;
	}
	return 0;
}

static const char *Cvar_FixQ3Name (const char *var_name)
{
	struct {
		const char *q3;
		const char *fte;
	} cvarremaps[] =
	{
		{"s_musicvolume",	"bgmvolume"},
		{"r_gamma",			"gamma"},
		{"s_sdlSpeed",		"s_khz"},
		{"r_fullscreen",	"vid_fullscreen"},
		{"r_picmip",		"gl_picmip"},
		{"r_textureMode",	"gl_texturemode"},
		{"r_lodBias",		"d_lodbias"},
		{"r_colorbits",		"vid_bpp"},
		{"r_dynamiclight",	"r_dynamic"},
		{"r_finish",		"gl_finish"},
		{"protocol",		"com_protocolversion"},
//		{"r_glDriver",		NULL},
//		{"r_depthbits",		NULL},
//		{"r_stencilbits",	NULL},
//		{"s_compression",	NULL},
//		{"r_texturebits",	NULL},
//		{"r_allowExtensions",NULL},
//		{"s_useOpenAL",		NULL},
//		{"sv_running",		NULL},
//		{"sv_killserver",	NULL},
//		{"color1",			NULL},
//		{"in_joystick",		NULL},
//		{"joy_threshold",	NULL},
//		{"cl_freelook",		NULL},
//		{"color1",			NULL},
//		{"r_availableModes",NULL},
//		{"r_mode",			NULL},
	};
	size_t i;
	for (i = 0; i < countof(cvarremaps); i++)
	{
		if (!strcmp(cvarremaps[i].q3, var_name))
			return cvarremaps[i].fte;
	}
	return var_name;
}

static void UI_SimulateTextEntry(void *cb, const char *utf8)
{
	const char *line = utf8;
	unsigned int unicode;
	int err;
	while(*line)
	{
		unicode = inputfuncs->utf8_decode(&err, line, &line);
		if (uivm)
			vmfuncs->Call(uivm, UI_KEY_EVENT, unicode|1024, true);
	}
}

#define VALIDATEPOINTER(o,l) if ((quintptr_t)o + l >= mask || VM_POINTER(o) < offset) plugfuncs->EndGame("Call to ui trap %i passes invalid pointer\n", (int)fn);	//out of bounds.

static qintptr_t UI_SystemCalls(void *offset, quintptr_t mask, qintptr_t fn, const qintptr_t *arg)
{
	static int overstrikemode;
	int ret=0;

	//Remember to range check pointers.
	//The QVM must not be allowed to write to anything outside it's memory.
	//This includes getting the exe to copy it for it.

	//don't bother with reading, as this isn't a virus risk.
	//could be a cheat risk, but hey.

	//make sure that any called functions are also range checked.
	//like reading from files copies names into alternate buffers, allowing stack screwups.

	switch((uiImport_t)fn)
	{
	case UI_CVAR_CREATE:						Con_Printf("Q3UI: Not implemented system trap: %s\n", "UI_CVAR_CREATE"); return 0;
	case UI_CVAR_INFOSTRINGBUFFER:				Con_Printf("Q3UI: Not implemented system trap: %s\n", "UI_CVAR_INFOSTRINGBUFFER"); return 0;
	case UI_R_ADDPOLYTOSCENE:					Con_Printf("Q3UI: Not implemented system trap: %s\n", "UI_R_ADDPOLYTOSCENE"); return 0;
	case UI_R_REMAP_SHADER:						Con_Printf("Q3UI: Not implemented system trap: %s\n", "UI_R_REMAP_SHADER"); return 0;
	case UI_UPDATESCREEN:						Con_Printf("Q3UI: Not implemented system trap: %s\n", "UI_UPDATESCREEN"); return 0;
	case UI_CM_LOADMODEL:						Con_Printf("Q3UI: Not implemented system trap: %s\n", "UI_CM_LOADMODEL"); return 0;
	case UI_ERROR:
		Con_Printf("%s", (char*)VM_POINTER(arg[0]));
		UI_Stop();
		plugfuncs->EndGame("UI error");
		break;
	case UI_PRINT:
		Con_Printf("%s", (char*)VM_POINTER(arg[0]));
		break;

	case UI_MILLISECONDS:
		VM_LONG(ret) = plugfuncs->GetMilliseconds();
		break;

	case UI_ARGC:
		VM_LONG(ret) = cmdfuncs->Argc();
		break;
	case UI_ARGV:
//		VALIDATEPOINTER(arg[1], arg[2]);
		cmdfuncs->Argv(VM_LONG(arg[0]), VM_POINTER(arg[1]), VM_LONG(arg[2]));
		break;
/*	case UI_ARGS:
		VALIDATEPOINTER(arg[0], arg[1]);
		Q_strncpyz(VM_POINTER(arg[0]), Cmd_Args(), VM_LONG(arg[1]));
		break;
*/

	case UI_CVAR_SET:
		cvarfuncs->SetString(Cvar_FixQ3Name(VM_POINTER(arg[0])), VM_POINTER(arg[1]));
		break;
	case UI_CVAR_VARIABLEVALUE:
		VM_FLOAT(ret) = cvarfuncs->GetFloat(Cvar_FixQ3Name(VM_POINTER(arg[0])));
		break;
	case UI_CVAR_VARIABLESTRINGBUFFER:
		if (arg[1] + arg[2] >= mask || VM_POINTER(arg[1]) < offset)
			VM_LONG(ret) = -2;	//out of bounds.
		cvarfuncs->GetString(Cvar_FixQ3Name(VM_POINTER(arg[0])), VM_POINTER(arg[1]), VM_LONG(arg[2]));
		break;

	case UI_CVAR_SETVALUE:
		cvarfuncs->SetFloat(Cvar_FixQ3Name(VM_POINTER(arg[0])), VM_FLOAT(arg[1]));
		break;

	case UI_CVAR_RESET:	//cvar reset
		cvarfuncs->SetString(Cvar_FixQ3Name(VM_POINTER(arg[0])), NULL);
		break;

	case UI_CMD_EXECUTETEXT:
		{
			char *cmdtext = VM_POINTER(arg[1]);
#ifdef CL_MASTER
			if (!strncmp(cmdtext, "ping ", 5))
			{
				char token[1024];
				int i;
				for (i = 0; i < MAX_PINGREQUESTS; i++)
					if (ui_pings[i].adr.type == NA_INVALID && ui_pings[i].adr.prot == NP_INVALID)
					{
						const char *p = NULL;
						cmdfuncs->ParseToken(cmdtext + 5, token, sizeof(token), NULL);
						ui_pings[i].startms = plugfuncs->GetMilliseconds();
						Q_strncpyz(ui_pings[i].adrstring, token, sizeof(ui_pings[i].adrstring));
						if (masterfuncs->StringToAdr(ui_pings[i].adrstring, 0, &ui_pings[i].adr, 1, &p))
						{
#ifdef HAVE_PACKET
							serverinfo_t *info = masterfuncs->InfoForServer(&ui_pings[i].adr, p);
							if (info)
							{
								info->special |= SS_KEEPINFO;
								info->sends++;
								masterfuncs->QueryServer(info);
							}
#endif
						}
						ui_pings[i].broker = p;
						break;
					}
			}
			else
#endif
				cmdfuncs->AddText(cmdtext, false);
		}
		break;

	case UI_FS_FOPENFILE: //fopen
		if ((int)arg[1] + 4 >= mask || VM_POINTER(arg[1]) < offset)
			break;	//out of bounds.
		VM_LONG(ret) = VM_fopen(VM_POINTER(arg[0]), VM_POINTER(arg[1]), VM_LONG(arg[2]), 0);
		break;

	case UI_FS_READ:	//fread
		if ((int)arg[0] + VM_LONG(arg[1]) >= mask || VM_POINTER(arg[0]) < offset)
			break;	//out of bounds.

		VM_LONG(ret) = VM_FRead(VM_POINTER(arg[0]), VM_LONG(arg[1]), VM_LONG(arg[2]), 0);
		break;
	case UI_FS_WRITE:	//fwrite
		break;
	case UI_FS_SEEK:
		return VM_FSeek(arg[0], arg[1], arg[2], 0);
	case UI_FS_FCLOSEFILE:	//fclose
		VM_fclose(VM_LONG(arg[0]), 0);
		break;

	case UI_FS_GETFILELIST:	//fs listing
		if ((int)arg[2] + arg[3] >= mask || VM_POINTER(arg[2]) < offset)
			break;	//out of bounds.
		return VM_GetFileList(VM_POINTER(arg[0]), VM_POINTER(arg[1]), VM_POINTER(arg[2]), VM_LONG(arg[3]));

	case UI_R_REGISTERMODEL:	//precache model
		{
			char *name = VM_POINTER(arg[0]);
			model_t *mod = scenefuncs->LoadModel(name, MLV_SILENTSYNC);
			if (!mod || mod->loadstate != MLS_LOADED || mod->type == mod_dummy)
				VM_LONG(ret) = 0;
			else
				VM_LONG(ret) = scenefuncs->ModelToId(mod);
		}
		break;
	case UI_R_MODELBOUNDS:
		{
			VALIDATEPOINTER(arg[1], sizeof(vec3_t));
			VALIDATEPOINTER(arg[2], sizeof(vec3_t));
			{
				model_t *mod = scenefuncs->ModelFromId(arg[0]);
				if (mod)
				{
					VectorCopy(mod->mins, ((float*)VM_POINTER(arg[1])));
					VectorCopy(mod->maxs, ((float*)VM_POINTER(arg[2])));
				}
				else
				{
					VectorClear(((float*)VM_POINTER(arg[1])));
					VectorClear(((float*)VM_POINTER(arg[2])));
				}
			}
		}
		break;

	case UI_R_REGISTERSKIN:
		VM_LONG(ret) = scenefuncs->RegisterSkinFile(VM_POINTER(arg[0]));
		break;

	case UI_R_REGISTERFONT:	//register font
		UI_RegisterFont(VM_POINTER(arg[0]), arg[1], VM_POINTER(arg[2]));
		break;
	case UI_R_REGISTERSHADERNOMIP:
		if (!*(char*)VM_POINTER(arg[0]))
			VM_LONG(ret) = 0;
		else
			VM_LONG(ret) = drawfuncs->LoadImage(VM_POINTER(arg[0]));
		if (!drawfuncs->ImageSize(VM_LONG(ret), NULL, NULL))
			VM_LONG(ret) = 0;	//not found...
		break;

	case UI_R_CLEARSCENE:	//clear scene
		scenefuncs->ClearScene();
		break;
	case UI_R_ADDREFENTITYTOSCENE:	//add ent to scene
		VQ3_AddEntity(VM_POINTER(arg[0]));
		break;
	case UI_R_ADDLIGHTTOSCENE:	//add light to scene.
		break;
	case UI_R_RENDERSCENE:	//render scene
		VQ3_RenderView(VM_POINTER(arg[0]));
		break;

	case UI_R_SETCOLOR:	//setcolour float*
		{
			float *fl =VM_POINTER(arg[0]);
			if (!fl)
				drawfuncs->Colour4f(1, 1, 1, 1);
			else
				drawfuncs->Colour4f(fl[0], fl[1], fl[2], fl[3]);
		}
		break;

	case UI_R_DRAWSTRETCHPIC:
		drawfuncs->Image(VM_FLOAT(arg[0]), VM_FLOAT(arg[1]), VM_FLOAT(arg[2]), VM_FLOAT(arg[3]), VM_FLOAT(arg[4]), VM_FLOAT(arg[5]), VM_FLOAT(arg[6]), VM_FLOAT(arg[7]), VM_LONG(arg[8]));
		break;

	case UI_CM_LERPTAG:	//Lerp tag...
		VALIDATEPOINTER(arg[0], sizeof(float)*12);
		VM_LONG(ret) = VM_LerpTag(VM_POINTER(arg[0]), scenefuncs->ModelFromId(arg[1]), VM_LONG(arg[2]), VM_LONG(arg[3]), VM_FLOAT(arg[4]), VM_POINTER(arg[5]));
		break;

	case UI_S_REGISTERSOUND:
		{
			sfx_t *sfx;
			sfx = audiofuncs->PrecacheSound(va("%s", (char*)VM_POINTER(arg[0])));
			if (sfx)
				VM_LONG(ret) = VM_TOSTRCACHE(arg[0]);	//return handle is the parameter they just gave
			else
				VM_LONG(ret) = -1;
		}
		break;
	case UI_S_STARTLOCALSOUND:
		if (VM_LONG(arg[0]) != -1 && arg[0])
			audiofuncs->LocalSound(VM_FROMSTRCACHE(arg[0]), CHAN_AUTO, 1.0);	//now we can fix up the sound name
		break;

	case UI_S_STARTBACKGROUNDTRACK:
		audiofuncs->ChangeMusicTrack(VM_POINTER(arg[0]), VM_POINTER(arg[1]));
		break;
	case UI_S_STOPBACKGROUNDTRACK:
		audiofuncs->ChangeMusicTrack(NULL, NULL);
		break;

	//q3 shares insert mode between its ui and console. whereas fte doesn't support it (FIXME: add to Key_EntryInsert).
	case UI_KEY_SETOVERSTRIKEMODE:
		overstrikemode = arg[0];
		return 0;
	case UI_KEY_GETOVERSTRIKEMODE:
		return overstrikemode;

	case UI_GETCLIPBOARDDATA:
		if (VM_OOB(arg[0], VM_LONG(arg[1])))
			break;	//out of bounds.
		//our clipboard doesn't allow us to simply query without blocking (would result in stalls on x11/wayland)
		//so just return nothing - we can send the text as if it were regular text entry for a similar result.
		Q_strncpyz(VM_POINTER(arg[0]), "", VM_LONG(arg[1]));

		//but do we really want to let mods read the system clipboard? I suppose it SHOULD be okay if the UI was manually installed by the user.
		//side note: q3's text entry logic is kinda flawed.
		inputfuncs->ClipboardGet(CBT_CLIPBOARD, UI_SimulateTextEntry, NULL);
		break;

	case UI_KEY_KEYNUMTOSTRINGBUF:
		if (VM_LONG(arg[0]) < 0 || VM_LONG(arg[0]) >= K_MAX || (int)arg[1] + VM_LONG(arg[2]) >= mask || VM_POINTER(arg[1]) < offset || VM_LONG(arg[2]) < 1)
			break;	//out of bounds.

		Q_strncpyz(VM_POINTER(arg[1]), inputfuncs->GetKeyName(VM_LONG(arg[0]), 0), VM_LONG(arg[2]));
		break;

	case UI_KEY_GETBINDINGBUF:
		if (VM_LONG(arg[0]) < 0 || VM_LONG(arg[0]) >= K_MAX || (int)arg[1] + VM_LONG(arg[2]) >= mask || VM_POINTER(arg[1]) < offset || VM_LONG(arg[2]) < 1)
			break;	//out of bounds.

		{
			const char *binding = inputfuncs->GetKeyBind(0, VM_LONG(arg[0]), 0);
			if (binding)
				Q_strncpyz(VM_POINTER(arg[1]), binding, VM_LONG(arg[2]));
			else
				*(char *)VM_POINTER(arg[1]) = '\0';
		}
		break;

	case UI_KEY_SETBINDING:
		inputfuncs->SetKeyBind(0, VM_LONG(arg[0]), 0, VM_POINTER(arg[1]));
		break;

	case UI_KEY_ISDOWN:
		VM_LONG(ret) = inputfuncs->IsKeyDown(VM_LONG(arg[0]));
		break;

	case UI_KEY_CLEARSTATES:
		inputfuncs->ClearKeyStates();
		break;
	case UI_KEY_GETCATCHER:
		if (inputfuncs->GetKeyDest() & kdm_console)
			VM_LONG(ret) = keycatcher | 1;
		else
			VM_LONG(ret) = keycatcher;
		break;
	case UI_KEY_SETCATCHER:
		Q3_SetKeyCatcher(VM_LONG(arg[0]));
		break;

	case UI_GETGLCONFIG:	//get glconfig
		{
			q3glconfig_t *cfg;
			float vsize[2] = {0,0};
			if ((int)arg[0] + sizeof(q3glconfig_t) >= mask || VM_POINTER(arg[0]) < offset)
				break;	//out of bounds.
			drawfuncs->GetVideoSize(vsize, NULL);
			cfg = VM_POINTER(arg[0]);

			//do any needed work
			memset(cfg, 0, sizeof(*cfg));

			Q_strncpyz(cfg->renderer_string, "", sizeof(cfg->renderer_string));
			Q_strncpyz(cfg->vendor_string, "", sizeof(cfg->vendor_string));
			Q_strncpyz(cfg->version_string, "", sizeof(cfg->version_string));
			Q_strncpyz(cfg->extensions_string, "", sizeof(cfg->extensions_string));

			cfg->colorBits = 32;
			cfg->depthBits = 24;
			cfg->stencilBits = 8;//sh_config.stencilbits;
			cfg->textureCompression = true;//!!sh_config.texfmt[PTI_BC1_RGBA];
			cfg->textureEnvAddAvailable = true;//sh_config.env_add;

			//these are the only three that really matter.
			cfg->vidWidth = vsize[0];
			cfg->vidHeight = vsize[1];
			cfg->windowAspect = vsize[0]/vsize[1];;
#if 0
			char *cfg;
			if ((int)arg[0] + 11332/*sizeof(glconfig_t)*/ >= mask || VM_POINTER(arg[0]) < offset)
				break;	//out of bounds.
			cfg = VM_POINTER(arg[0]);
		

			//do any needed work
			memset(cfg, 0, 11304);
			*(int *)(cfg+11304) = vsize[0];
			*(int *)(cfg+11308) = vid.height;
			*(float *)(cfg+11312) = (float)vsize[0]/vid.height;
			memset(cfg+11316, 0, 11332-11316);
#endif
		}
		break;

	case UI_GETCLIENTSTATE:	//get client state
		//fixme: we need to fill in a structure.
//		Con_Printf("ui_getclientstate\n");
		VALIDATEPOINTER(arg[0], sizeof(uiClientState_t));
		{
			uiClientState_t *state = VM_POINTER(arg[0]);
			state->connectPacketCount = 0;//clc.connectPacketCount;

			cvarfuncs->GetString("cl_servername", state->servername, sizeof(state->servername));	//server we're connected to
			switch(ccs.state)
			{
			case ca_disconnected:
				if (*state->servername)
					state->connState = Q3CA_CONNECTING;
				else
					state->connState = Q3CA_DISCONNECTED;
				break;
			case ca_demostart:
				state->connState = Q3CA_CONNECTING;
				break;
			case ca_connected:
				state->connState = Q3CA_CONNECTED;
				break;
			case ca_onserver:
				state->connState = Q3CA_PRIMED;
				break;
			case ca_active:
				state->connState = Q3CA_ACTIVE;
				break;
			}
			Q_strncpyz(state->updateInfoString, "FTE!", sizeof(state->updateInfoString));	//warning/motd message from update server
			cvarfuncs->GetString("com_errorMessage", state->messageString, sizeof(state->messageString));	//error message from game server
			state->clientNum = ccs.playernum;
		}
		break;

	case UI_GETCONFIGSTRING:
		if (arg[1] + VM_LONG(arg[2]) >= mask || VM_POINTER(arg[1]) < offset || VM_LONG(arg[2]) < 1)
		{
			VM_LONG(ret) = 0;
			break;	//out of bounds.
		}
#ifdef VM_CG
		Q_strncpyz(VM_POINTER(arg[1]), CG_GetConfigString(VM_LONG(arg[0])), VM_LONG(arg[2]));
#endif
		break;

#ifdef CL_MASTER
	case UI_LAN_GETPINGQUEUECOUNT:	//these four are master server polling.
		{
			int i;
			for (i = 0; i < MAX_PINGREQUESTS; i++)
				if (ui_pings[i].adr.type != NA_INVALID || ui_pings[i].adr.prot != NP_INVALID)
					VM_LONG(ret)++;
		}
		break;
	case UI_LAN_CLEARPING:	//clear ping
		//void (int pingnum)
		if (VM_LONG(arg[0])>= 0 && VM_LONG(arg[0]) < MAX_PINGREQUESTS)
			ui_pings[VM_LONG(arg[0])].adr.type = NA_INVALID, ui_pings[VM_LONG(arg[0])].adr.prot = NP_INVALID;
		break;
	case UI_LAN_GETPING:
		//void (int pingnum, char *buffer, int buflen, int *ping)
		if ((int)arg[1] + VM_LONG(arg[2]) >= mask || VM_POINTER(arg[1]) < offset)
			break;	//out of bounds.
		if ((int)arg[3] + sizeof(int) >= mask || VM_POINTER(arg[3]) < offset)
			break;	//out of bounds.

		masterfuncs->CheckPollSockets();
		if (VM_LONG(arg[0])>= 0 && VM_LONG(arg[0]) < MAX_PINGREQUESTS)
		{
			int i = VM_LONG(arg[0]);
			char *buf = VM_POINTER(arg[1]);
			size_t bufsize = VM_LONG(arg[2]);
			int *ping = VM_POINTER(arg[3]);
			serverinfo_t *info;
			if (ui_pings[i].adr.type != NA_INVALID || ui_pings[i].adr.prot != NP_INVALID)
				info = masterfuncs->InfoForServer(&ui_pings[i].adr, ui_pings[i].broker);
			else
				info = NULL;
			Q_strncpyz(buf, ui_pings[i].adrstring, bufsize);

			if (info && /*(info->status & SRVSTATUS_ALIVE) &&*/ info->moreinfo)
			{
				VM_LONG(ret) = true;
				*ping = (info->ping == PING_UNKNOWN)?1:info->ping;
				break;
			}
			i = plugfuncs->GetMilliseconds()-ui_pings[i].startms;
			if (i < 999)
				i = 0;	//don't time out yet.
			*ping = i;
		}
		break;
	case UI_LAN_GETPINGINFO:
		//void (int pingnum, char *buffer, int buflen)
		if ((int)arg[1] + VM_LONG(arg[2]) >= mask || VM_POINTER(arg[1]) < offset)
			break;	//out of bounds.
		if ((int)arg[3] + sizeof(int) >= mask || VM_POINTER(arg[3]) < offset)
			break;	//out of bounds.

		masterfuncs->CheckPollSockets();
		if (VM_LONG(arg[0])>= 0 && VM_LONG(arg[0]) < MAX_PINGREQUESTS)
		{
			int i = VM_LONG(arg[0]);
			char *buf = VM_POINTER(arg[1]);
			size_t bufsize = VM_LONG(arg[2]);
			char *adr;
			serverinfo_t *info = masterfuncs->InfoForServer(&ui_pings[i].adr, ui_pings[i].broker);

			if (info && /*(info->status & SRVSTATUS_ALIVE) &&*/ info->moreinfo)
			{
				adr = info->moreinfo->info;
				if (!adr)
					adr = "";
				if (strlen(adr) < bufsize)
				{
					strcpy(buf, adr);
					worldfuncs->SetInfoKey(buf, "mapname", info->map, bufsize);
					worldfuncs->SetInfoKey(buf, "hostname", info->name, bufsize);
					worldfuncs->SetInfoKey(buf, "g_humanplayers", va("%i", info->numhumans), bufsize);
					worldfuncs->SetInfoKey(buf, "sv_maxclients", va("%i", info->maxplayers), bufsize);
					worldfuncs->SetInfoKey(buf, "clients", va("%i", info->players), bufsize);
					if (info->adr.type == NA_IPV6 && info->adr.prot == NP_DGRAM)
						worldfuncs->SetInfoKey(buf, "nettype", "2", bufsize);
					else if (info->adr.type == NA_IP && info->adr.prot == NP_DGRAM)
						worldfuncs->SetInfoKey(buf, "nettype", "1", bufsize);
					else
						worldfuncs->SetInfoKey(buf, "nettype", "", bufsize);
					VM_LONG(ret) = true;
				}	
			}
			else
				strcpy(buf, "");
		}
		break;

	case UI_LAN_UPDATEVISIBLEPINGS:
		return masterfuncs->QueryServers();	//return true while we're still going.
	case UI_LAN_RESETPINGS:
		return 0;
	case UI_LAN_ADDSERVER:
		return 0;
	case UI_LAN_REMOVESERVER:
		return 0;
	case UI_LAN_SERVERSTATUS:
		//*address, *status, statuslen
		return 0;
	case UI_LAN_COMPARESERVERS:
		{
			hostcachekey_t q3tofte[] = {SLKEY_NAME, SLKEY_MAP, SLKEY_NUMHUMANS, SLKEY_GAMEDIR, SLKEY_PING, 0};
			qboolean keyisstring[] = {true, true, false, true, false, false};
			//int source = VM_LONG(arg[0]);
			int key = bound(0, VM_LONG(arg[1]), countof(q3tofte));
			int sortdir = VM_LONG(arg[2]);
			serverinfo_t *s1 = masterfuncs->InfoForNum(VM_LONG(arg[3]));
			serverinfo_t *s2 = masterfuncs->InfoForNum(VM_LONG(arg[4]));
			if (keyisstring[key])
				ret = strcasecmp(masterfuncs->ReadKeyString(s2, q3tofte[key]), masterfuncs->ReadKeyString(s1, q3tofte[key]));
			else
			{
				ret = masterfuncs->ReadKeyFloat(s2, q3tofte[key]) - masterfuncs->ReadKeyFloat(s1, q3tofte[key]);
				if (ret < 0)
					ret = -1;
				else if (ret > 0)
					ret = 1;
			}
			if (sortdir)
				ret = -ret;
		}
		return ret;

	case UI_LAN_GETSERVERCOUNT:	//LAN Get server count
		//int (int source)
		masterfuncs->CheckPollSockets();
		VM_LONG(ret) = masterfuncs->TotalCount();
		break;
	case UI_LAN_GETSERVERADDRESSSTRING:	//LAN get server address
		//void (int source, int svnum, char *buffer, int buflen)
		if ((int)arg[2] + VM_LONG(arg[3]) >= mask || VM_POINTER(arg[2]) < offset)
			break;	//out of bounds.
		{
			char *buf = VM_POINTER(arg[2]);
			char *adr;
			char adrbuf[MAX_ADR_SIZE];
			serverinfo_t *info = masterfuncs->InfoForNum(VM_LONG(arg[1]));
			strcpy(buf, "");
			if (info)
			{
				adr = masterfuncs->ServerToString(adrbuf, sizeof(adrbuf), info);
				if (strlen(adr) < VM_LONG(arg[3]))
				{
					strcpy(buf, adr);
					VM_LONG(ret) = true;
				}
			}
		}
		break;
	case UI_LAN_LOADCACHEDSERVERS:
		break;
	case UI_LAN_SAVECACHEDSERVERS:
		break;
	case UI_LAN_GETSERVERPING:
		return 50;
	case UI_LAN_GETSERVERINFO:
		if (VM_OOB(arg[2], arg[3]) || !arg[3])
			break;	//out of bounds.
		{
			//int source = VM_LONG(arg[0]);
			int servernum = VM_LONG(arg[1]);
			char *out = VM_POINTER(arg[2]);
			int maxsize = VM_LONG(arg[3]);
			char adr[MAX_ADR_SIZE];
			serverinfo_t *info = masterfuncs->InfoForNum(servernum);
			*out = 0;
			if (info)
			{
				worldfuncs->SetInfoKey(out, "hostname", info->name, maxsize);
				worldfuncs->SetInfoKey(out, "mapname", info->map, maxsize);
				worldfuncs->SetInfoKey(out, "clients", va("%i", info->players), maxsize);
				worldfuncs->SetInfoKey(out, "sv_maxclients", va("%i", info->maxplayers), maxsize);
				worldfuncs->SetInfoKey(out, "ping", va("%i", info->ping), maxsize);
//				worldfuncs->SetInfoKey(out, "minping", info->map, maxsize);
//				worldfuncs->SetInfoKey(out, "maxping", info->map, maxsize);
//				worldfuncs->SetInfoKey(out, "game", info->map, maxsize);
//				worldfuncs->SetInfoKey(out, "gametype", info->map, maxsize);
//				worldfuncs->SetInfoKey(out, "nettype", info->map, maxsize);
				worldfuncs->SetInfoKey(out, "addr", masterfuncs->AdrToString(adr, sizeof(adr), &info->adr), maxsize);
//				worldfuncs->SetInfoKey(out, "punkbuster", info->map, maxsize);
//				worldfuncs->SetInfoKey(out, "g_needpass", info->map, maxsize);
//				worldfuncs->SetInfoKey(out, "g_humanplayers", info->map, maxsize);
			}
		}
		break;
	case UI_LAN_MARKSERVERVISIBLE:
		/*not implemented*/
		return 0;
	case UI_LAN_SERVERISVISIBLE:
		return 1;
#endif

	case UI_CVAR_REGISTER:
		if (VM_OOB(arg[0], sizeof(q3vmcvar_t)))
			break;	//out of bounds.
		return VMQ3_Cvar_Register(VM_POINTER(arg[0]), VM_POINTER(arg[1]), VM_POINTER(arg[2]), VM_LONG(arg[3]));

	case UI_CVAR_UPDATE:
		if (VM_OOB(arg[0], sizeof(q3vmcvar_t)))
			break;	//out of bounds.
		return VMQ3_Cvar_Update(VM_POINTER(arg[0]));

	case UI_MEMORY_REMAINING:
		VM_LONG(ret) = 1024*1024*8;//Hunk_LowMemAvailable();
		break;

	case UI_GET_CDKEY:	//get cd key
		{
			cvar_t *cvar = cvarfuncs->GetNVFDG("cl_cdkey", "", CVAR_ARCHIVE, "Quake3 auth", "Q3 Compat");
			char *keydest = VM_POINTER(arg[0]);
			if (!cvar || (int)arg[0] + VM_LONG(arg[1]) >= mask || VM_POINTER(arg[0]) < offset)
				break;	//out of bounds.
			Q_strncpyz(keydest, cvar->string, VM_LONG(arg[1]));
		}
		break;
	case UI_SET_CDKEY:	//set cd key
		{
			char *keysrc = VM_POINTER(arg[0]);
			if ((int)arg[0] + strlen(keysrc) >= mask || VM_POINTER(arg[0]) < offset)
				break;	//out of bounds.
			cvarfuncs->SetString("cl_cdkey", keysrc);
		}
		break;

	case UI_REAL_TIME:
		VALIDATEPOINTER(arg[0], sizeof(q3time_t));
		return Q3VM_GetRealtime(VM_POINTER(arg[0]));

	case UI_VERIFY_CDKEY:
		VM_LONG(ret) = true;
		break;

	case UI_SET_PBCLSTATUS:
		break;

// standard Q3
	case UI_MEMSET:
		{
			void *dest = VM_POINTER(arg[0]);
			if ((int)arg[0] + arg[2] >= mask || dest < offset)
				break;	//out of bounds.
			memset(dest, arg[1], arg[2]);
		}
		break;
	case UI_MEMCPY:
		{
			void *dest = VM_POINTER(arg[0]);
			void *src = VM_POINTER(arg[1]);
			if ((int)arg[0] + arg[2] >= mask || VM_POINTER(arg[0]) < offset)
				break;	//out of bounds.
			memcpy(dest, src, arg[2]);
		}
		break;
	case UI_STRNCPY:
		{
			void *dest = VM_POINTER(arg[0]);
			void *src = VM_POINTER(arg[1]);
			if (arg[0] + arg[2] >= mask || VM_POINTER(arg[0]) < offset)
				break;	//out of bounds.
			Q_strncpyS(dest, src, arg[2]);
		}
		break;
	case UI_SIN:
		VM_FLOAT(ret)=(float)sin(VM_FLOAT(arg[0]));
		break;
	case UI_COS:
		VM_FLOAT(ret)=(float)cos(VM_FLOAT(arg[0]));
		break;
	case UI_ATAN2:
		VM_FLOAT(ret)=(float)atan2(VM_FLOAT(arg[0]), VM_FLOAT(arg[1]));
		break;
	case UI_SQRT:
		VM_FLOAT(ret)=(float)sqrt(VM_FLOAT(arg[0]));
		break;
	case UI_FLOOR:
		VM_FLOAT(ret)=(float)floor(VM_FLOAT(arg[0]));
		break;
	case UI_CEIL:
		VM_FLOAT(ret)=(float)ceil(VM_FLOAT(arg[0]));
		break;
/*
	case UI_GETPLAYERINFO:
		if (arg[1] + sizeof(vmuiclientinfo_t) >= mask || VM_POINTER(arg[1]) < offset)
			break;	//out of bounds.
		if (VM_LONG(arg[0]) < -1 || VM_LONG(arg[0] ) >= MAX_CLIENTS)
			break;
		{
			int i = VM_LONG(arg[0]);
			vmuiclientinfo_t *vci = VM_POINTER(arg[1]);
			if (i == -1)
			{
				i = cl.playernum[0];
				if (i < 0)
				{
					memset(vci, 0, sizeof(*vci));
					return 0;
				}	
			}
			vci->bottomcolour = cl.players[i].rbottomcolor;
			vci->frags = cl.players[i].frags;
			Q_strncpyz(vci->name, cl.players[i].name, UIMAX_SCOREBOARDNAME);
			vci->ping = cl.players[i].ping;
			vci->pl = cl.players[i].pl;
			vci->starttime = cl.players[i].entertime;
			vci->topcolour = cl.players[i].rtopcolor;
			vci->userid = cl.players[i].userid;
			Q_strncpyz(vci->userinfo, cl.players[i].userinfo, sizeof(vci->userinfo));
		}
		break;
	case UI_GETSTAT:
		if (VM_LONG(arg[0]) < 0 || VM_LONG(arg[0]) >= MAX_CL_STATS)
			VM_LONG(ret) = 0;	//invalid stat num.
		else
			VM_LONG(ret) = cl.stats[0][VM_LONG(arg[0])];
		break;
	case UI_GETVIDINFO:
		{
			vidinfo_t *vi;
			if (arg[0] + VM_LONG(arg[1]) >= mask || VM_POINTER(arg[1]) < offset)
			{
				VM_LONG(ret) = 0;
				break;	//out of bounds.
			}
			vi = VM_POINTER(arg[0]);
			if (VM_LONG(arg[1]) < sizeof(vidinfo_t))
			{
				VM_LONG(ret) = 0;
				break;
			}
			VM_LONG(ret) = sizeof(vidinfo_t);
			vi->width = vsize[0];
			vi->height = vsize[1];
			vi->refreshrate = 60;
			vi->fullscreen = 1;
			Q_strncpyz(vi->renderername, q_renderername, sizeof(vi->renderername));
		}
		break;
*/

#ifdef HAVE_SERVER
	case UI_PC_ADD_GLOBAL_DEFINE:
		return botlib->PC_AddGlobalDefine(VM_POINTER(arg[0]));
	case UI_PC_LOAD_SOURCE:
		return botlib->PC_LoadSourceHandle(VM_POINTER(arg[0]));
	case UI_PC_FREE_SOURCE:
		return botlib->PC_FreeSourceHandle(VM_LONG(arg[0]));
	case UI_PC_READ_TOKEN:
		return botlib->PC_ReadTokenHandle(VM_LONG(arg[0]), VM_POINTER(arg[1]));
	case UI_PC_SOURCE_FILE_AND_LINE:
		return botlib->PC_SourceFileAndLine(VM_LONG(arg[0]), VM_POINTER(arg[1]), VM_POINTER(arg[2]));
#else
	case UI_PC_ADD_GLOBAL_DEFINE:
		Con_Printf("UI_PC_ADD_GLOBAL_DEFINE not supported\n");
		break;
	case UI_PC_SOURCE_FILE_AND_LINE:
		Script_Get_File_And_Line(arg[0], VM_POINTER(arg[1]), VM_POINTER(arg[2]));
		break;

	case UI_PC_LOAD_SOURCE:
		return Script_LoadFile(VM_POINTER(arg[0]));
	case UI_PC_FREE_SOURCE:
		Script_Free(arg[0]);
		break;
	case UI_PC_READ_TOKEN:
		//fixme: memory protect.
		return Script_Read(arg[0], VM_POINTER(arg[1]));
#endif

	case UI_CIN_PLAYCINEMATIC:
		return UI_Cin_Play(VM_POINTER(arg[0]), VM_LONG(arg[1]), VM_LONG(arg[2]), VM_LONG(arg[3]), VM_LONG(arg[4]), VM_LONG(arg[5]));
	case UI_CIN_STOPCINEMATIC:
		return UI_Cin_Stop(VM_LONG(arg[0]));
	case UI_CIN_RUNCINEMATIC:
		return UI_Cin_Run(VM_LONG(arg[0]));
	case UI_CIN_DRAWCINEMATIC:
		return UI_Cin_Draw(VM_LONG(arg[0]));
	case UI_CIN_SETEXTENTS:
		return UI_Cin_SetExtents(VM_LONG(arg[0]), VM_LONG(arg[1]), VM_LONG(arg[2]), VM_LONG(arg[3]), VM_LONG(arg[4]));









#ifndef _DEBUG
	default:
#endif
		Con_Printf("Q3UI: Unknown system trap: %i\n", (int)fn);
		return 0;
	}

	return ret;
}

static int UI_SystemCallsVM(void *offset, quintptr_t mask, int fn, const int *arg)
{	//this is so we can use edit and continue properly (vc doesn't like function pointers for edit+continue)
#if __WORDSIZE == 32
	return UI_SystemCalls(offset, mask, fn, (qintptr_t*)arg);
#else
	qintptr_t args[9];

	args[0]=arg[0];
	args[1]=arg[1];
	args[2]=arg[2];
	args[3]=arg[3];
	args[4]=arg[4];
	args[5]=arg[5];
	args[6]=arg[6];
	args[7]=arg[7];
	args[8]=arg[8];

	return UI_SystemCalls(offset, mask, fn, args);
#endif
}

//I'm not keen on this.
//but dlls call it without saying what sort of vm it comes from, so I've got to have them as specifics
static qintptr_t EXPORT_FN UI_SystemCallsNative(qintptr_t arg, ...)
{
	qintptr_t args[9];
	va_list argptr;

	va_start(argptr, arg);
	args[0]=va_arg(argptr, qintptr_t);
	args[1]=va_arg(argptr, qintptr_t);
	args[2]=va_arg(argptr, qintptr_t);
	args[3]=va_arg(argptr, qintptr_t);
	args[4]=va_arg(argptr, qintptr_t);
	args[5]=va_arg(argptr, qintptr_t);
	args[6]=va_arg(argptr, qintptr_t);
	args[7]=va_arg(argptr, qintptr_t);
	args[8]=va_arg(argptr, qintptr_t);
	va_end(argptr);

	return UI_SystemCalls(NULL, ~(quintptr_t)0, arg, args);
}

static void UI_Release(menu_t *m, qboolean forced)
{
	keycatcher &= ~2;
}
static void UI_DrawMenu(menu_t *m)
{
	if (uivm)
	{
		float vsize[2];

		if (drawfuncs->GetVideoSize(vsize, NULL) && (ui_size[0] != vsize[0] || ui_size[1] != vsize[1]))
		{	//should probably just rescale stuff instead.
			qboolean hadfocus = keycatcher&2;
			keycatcher &= ~2;
			ui_size[0] = vsize[0];
			ui_size[1] = vsize[1];
			vmfuncs->Call(uivm, UI_INIT);
			if (hadfocus)
			{
				if (ccs.state)
					vmfuncs->Call(uivm, UI_SET_ACTIVE_MENU, 2);
				else
					vmfuncs->Call(uivm, UI_SET_ACTIVE_MENU, 1);
			}
		}
		vmfuncs->Call(uivm, UI_REFRESH, plugfuncs->GetMilliseconds());


		uimenu.isopaque = vmfuncs->Call(uivm, UI_IS_FULLSCREEN);
	}
}
qboolean UI_KeyPress(struct menu_s *m, qboolean isdown, unsigned int devid, int key, int unicode)
{
//	qboolean result;
	if (!uivm)
		return false;
//	if (key_dest == key_menu)
//		return false;
	if (!(keycatcher&2))
	{
		if (key == K_ESCAPE && isdown)
		{
			UI_OpenMenu();
			return true;
		}
		return false;
	}

	/* if you change this here, it'll have to be changed in cl_cg.c too */
	switch (key)
	{
		/* all these get interpreted as enter in Q3's UI... */
		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
		case K_AUX1:
		case K_AUX2:
		case K_AUX3:
		case K_AUX4:
		case K_AUX5:
		case K_AUX6:
		case K_AUX7:
		case K_AUX8:
		case K_AUX9:
		case K_AUX10:
		case K_AUX11:
		case K_AUX14:
		case K_AUX15:
		case K_AUX16:
			return true;
			break;
		/* Q3 doesn't know about these keys, remap them */
		case K_GP_START:
			key = K_ESCAPE;
			break;
		case K_GP_DPAD_UP:
			key = K_UPARROW;
			break;
		case K_GP_DPAD_DOWN:
			key = K_DOWNARROW;
			break;
		case K_GP_DPAD_LEFT:
			key = K_LEFTARROW;
			break;
		case K_GP_DPAD_RIGHT:
			key = K_RIGHTARROW;
			break;
		case K_GP_A:
			key = K_ENTER;
			break;
		case K_GP_B:
			key = K_ESCAPE;
			break;
		case K_GP_X:
			key = K_BACKSPACE;
			break;
	}

	if (key && key < 1024)
		/*result = */vmfuncs->Call(uivm, UI_KEY_EVENT, key, isdown);
	if (unicode && unicode < 1024)
		/*result = */vmfuncs->Call(uivm, UI_KEY_EVENT, unicode|1024, isdown);
	return true;
}

static qboolean UI_MousePosition(struct menu_s *m, qboolean abs, unsigned int devid, float x, float y)
{	//q3ui is a peice of poo and only accepts relative mouse movements.
	//which it then clamps arbitrarily
	//which means we can't use hardware cursors
	//which results in clumsyness when switching between q3ui and everything else.

	if (uivm && !abs)
	{
		int px = x, py = y;
		vmfuncs->Call(uivm, UI_MOUSE_EVENT, px, py);
		return true;
	}
	return false;
}

void UI_Reset(void)
{
	inputfuncs->Menu_Unlink(&uimenu, true);

	if (!drawfuncs->GetVideoSize(NULL,NULL))	//no renderer loaded
		UI_Stop();
	else if (uivm)
		vmfuncs->Call(uivm, UI_INIT);
}

void UI_Stop (void)
{
	inputfuncs->Menu_Unlink(&uimenu, true);

	if (uivm)
	{
		vmfuncs->Call(uivm, UI_SHUTDOWN);
		vmfuncs->Destroy(uivm);
		VM_fcloseall(0);
		uivm = NULL;

		//mimic Q3 and save the config if anything got changed.
		//note that q3 checks every frame. we only check when the ui is closed.
		cmdfuncs->AddText("cfg_save_ifmodified\n", true);
#if defined(CL_MASTER)
		masterfuncs->WriteServers();
#endif
	}
}

void UI_Start (void)
{
	int i;
	int apiversion;
	if (!cl_shownet_ptr)
		return;	//make sure UI_Init was called. if it wasn't then something messed up and we can't do client stuff.
	if (!drawfuncs->GetVideoSize(ui_size,NULL))
		return;

	UI_Stop();

	for (i = 0; i < MAX_PINGREQUESTS; i++)
		ui_pings[i].adr.type = NA_INVALID;

	uimenu.drawmenu = UI_DrawMenu;
	uimenu.mousemove = UI_MousePosition;
	uimenu.keyevent = UI_KeyPress;
	uimenu.release = UI_Release;
	uimenu.lowpriority = true;

#ifdef HAVE_SERVER
	SV_InitBotLib();
#endif
	uivm = vmfuncs->Create("ui", cvarfuncs->GetFloat("com_gamedirnativecode")?UI_SystemCallsNative:NULL, "vm/ui", UI_SystemCallsVM);
	if (uivm)
	{
		apiversion = vmfuncs->Call(uivm, UI_GETAPIVERSION, 6);
		if (apiversion != 4 && apiversion != 6)	//make sure we can run the thing
		{
			Con_Printf("User-Interface VM uses incompatible API version (%i)\n", apiversion);
			vmfuncs->Destroy(uivm);
			VM_fcloseall(0);
			uivm = NULL;
			return;
		}
		vmfuncs->Call(uivm, UI_INIT);

		UI_OpenMenu();
	}
}

void UI_Restart_f(void)
{
	char arg1[256];
	cmdfuncs->Argv(1, arg1, sizeof(arg1));
	UI_Stop();
	if (strcmp(arg1, "off"))
	{
		UI_Start();

		if (uivm)
		{
			if (ccs.state)
				vmfuncs->Call(uivm, UI_SET_ACTIVE_MENU, 2);
			else
				vmfuncs->Call(uivm, UI_SET_ACTIVE_MENU, 1);
		}
	}
}

qboolean UI_OpenMenu(void)
{
	if (!uivm)
		UI_Start();
	if (uivm)
	{
		if (ccs.state)
			vmfuncs->Call(uivm, UI_SET_ACTIVE_MENU, 2);
		else
			vmfuncs->Call(uivm, UI_SET_ACTIVE_MENU, 1);
		return true;
	}
	return false;
}

qboolean UI_ConsoleCommand(void)
{
	if (uivm)
		return vmfuncs->Call(uivm, UI_CONSOLE_COMMAND, plugfuncs->GetMilliseconds());
	return false;
}

qboolean UI_IsRunning(void)
{
	if (uivm)
		return true;
	return false;
}

vm_t *UI_GetUIVM(void)
{
	return uivm;
}


void UI_Init (void)
{
	cl_shownet_ptr = cvarfuncs->GetNVFDG("cl_shownet", "", 0, NULL, "Q3 Compat");
	cl_c2sdupe_ptr = cvarfuncs->GetNVFDG("cl_c2sdupe", "", 0, NULL, "Q3 Compat");
	cl_nodelta_ptr = cvarfuncs->GetNVFDG("cl_nodelta", "", 0, NULL, "Q3 Compat");

	cmdfuncs->AddCommand("ui_restart", UI_Restart_f, "Reload the Q3-based User Interface module");
}
#endif

