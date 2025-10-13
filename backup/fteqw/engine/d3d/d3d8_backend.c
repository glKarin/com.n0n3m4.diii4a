#include "quakedef.h"
#include "glquake.h"
#include "gl_draw.h"
#ifdef D3D8QUAKE
//#define FIXME //for Eukara to fix, if he ever wants to get the d3d8 renderer working fully.
#include "shader.h"
#if !defined(HMONITOR_DECLARED) && (WINVER < 0x0500)
    #define HMONITOR_DECLARED
    DECLARE_HANDLE(HMONITOR);
#endif

#ifdef _XBOX
	#include <xtl.h>
	#define D3DLOCK_DISCARD 0
#endif

#include <d3d8.h>

/*
d3d8 supports no per-buffer offsets, and the fixed function stuff only supports a single vbo anyway.
to work around this, anything with any mods that can't be expressed by the vbo will result in everything being pulled over into a temporary buffer
*/



//#define FORCESTATE

#ifdef FORCESTATE
#pragma warningmsg("D3D8 FORCESTATE is active")
#endif


extern LPDIRECT3DDEVICE8 pD3DDev8;

//#define d3dcheck(foo) foo
#ifdef _DEBUG
#define d3dcheck(foo) do{HRESULT err = foo; if (FAILED(err)) Sys_Error("D3D reported error on backend line %i - error 0x%x\n", __LINE__, (unsigned int)err);} while(0)
#else
#define d3dcheck(foo) foo
#endif

#define MAX_TMUS 1

#define MAX_TC_TMUS 1

extern float d3d_trueprojection[16];

static void BE_RotateForEntity (const entity_t *e, const model_t *mod);

/*========================================== tables for deforms =====================================*/
#define R_FastSin(x) sin((x)*(2*M_PI))
#define frand() (rand()*(1.0/RAND_MAX))
#define FTABLE_SIZE		1024
#define FTABLE_CLAMP(x)	(((int)((x)*FTABLE_SIZE) & (FTABLE_SIZE-1)))
#define FTABLE_EVALUATE(table,x) (table ? table[FTABLE_CLAMP(x)] : frand()*((x)-floor(x)))

static	float	r_sintable[FTABLE_SIZE];
static	float	r_triangletable[FTABLE_SIZE];
static	float	r_squaretable[FTABLE_SIZE];
static	float	r_sawtoothtable[FTABLE_SIZE];
static	float	r_inversesawtoothtable[FTABLE_SIZE];

#if FIXME
static float *FTableForFunc ( unsigned int func )
{
	switch (func)
	{
		case SHADER_FUNC_SIN:
			return r_sintable;

		case SHADER_FUNC_TRIANGLE:
			return r_triangletable;

		case SHADER_FUNC_SQUARE:
			return r_squaretable;

		case SHADER_FUNC_SAWTOOTH:
			return r_sawtoothtable;

		case SHADER_FUNC_INVERSESAWTOOTH:
			return r_inversesawtoothtable;
	}

	//bad values allow us to crash (so I can debug em)
	return NULL;
}
#endif

static void FTable_Init(void)
{
	unsigned int i;
	double t;
	for (i = 0; i < FTABLE_SIZE; i++)
	{
		t = (double)i / (double)FTABLE_SIZE;

		r_sintable[i] = sin(t * 2*M_PI);

		if (t < 0.25)
			r_triangletable[i] = t * 4.0;
		else if (t < 0.75)
			r_triangletable[i] = 2 - 4.0 * t;
		else
			r_triangletable[i] = (t - 0.75) * 4.0 - 1.0;

		if (t < 0.5)
			r_squaretable[i] = 1.0f;
		else
			r_squaretable[i] = -1.0f;

		r_sawtoothtable[i] = t;
		r_inversesawtoothtable[i] = 1.0 - t;
	}
}

#if FIXME
typedef vec3_t mat3_t[3];
static mat3_t axisDefault={{1, 0, 0},
					{0, 1, 0},
					{0, 0, 1}};

static void Matrix3_Transpose (mat3_t in, mat3_t out)
{
	out[0][0] = in[0][0];
	out[1][1] = in[1][1];
	out[2][2] = in[2][2];

	out[0][1] = in[1][0];
	out[0][2] = in[2][0];
	out[1][0] = in[0][1];
	out[1][2] = in[2][1];
	out[2][0] = in[0][2];
	out[2][1] = in[1][2];
}
static void Matrix3_Multiply_Vec3 (const mat3_t a, const vec3_t b, vec3_t product)
{
	product[0] = a[0][0]*b[0] + a[0][1]*b[1] + a[0][2]*b[2];
	product[1] = a[1][0]*b[0] + a[1][1]*b[1] + a[1][2]*b[2];
	product[2] = a[2][0]*b[0] + a[2][1]*b[1] + a[2][2]*b[2];
}

static int Matrix3_Compare(const mat3_t in, const mat3_t out)
{
	return !memcmp(in, out, sizeof(mat3_t));
}
#endif

/*================================================*/

typedef struct
{
	backendmode_t mode;
	unsigned int flags;

	float		curtime;
	const entity_t	*curentity;
	const dlight_t	*curdlight;
	batch_t		*curbatch, dummybatch;
	vec3_t		curdlight_colours;
	shader_t	*curshader;
	texnums_t	*curtexnums;
	texid_t		curlightmap;
	texid_t		curdeluxmap;
	unsigned int shaderbits;
	unsigned int curcull;
	float depthbias;
	float depthfactor;
	float m_model[16];
	unsigned int lastpasscount;
	vbo_t *batchvbo;

	shader_t	*shader_rtlight;
	IDirect3DTexture8	*curtex[MAX_TMUS];
	unsigned int curtexflags[MAX_TMUS];
	unsigned int tmuflags[MAX_TMUS];

	mesh_t		**meshlist;
	unsigned int nummeshes;

	D3DCOLOR	passcolour;
	qboolean	passsinglecolour;

	/*FIXME: we shouldn't lock these so much - we need to cache which batches have been submitted and set up streams separately from the vertex data*/
	IDirect3DVertexBuffer8 *dynvbo_buff;
	unsigned int dynvbo_offs;
	unsigned int dynvbo_size;

	IDirect3DIndexBuffer8 *dynidx_buff;
	unsigned int dynidx_offs;
	unsigned int dynidx_size;

	unsigned int wbatch;
	unsigned int maxwbatches;
	batch_t *wbatches;

	int mipfilter[3];
	int picfilter[3];
} d3d8backend_t;

typedef struct
{
	vec3_t coord;
	byte_vec4_t colorsb;
	vec2_t tc[2];
} vbovdata_t;
#define D3DFVF_QVBO (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX2)

#define DYNVBUFFSIZE 65536
#define DYNIBUFFSIZE 65536

static d3d8backend_t shaderstate;

extern int be_maxpasses;

static void BE_ApplyTMUState(unsigned int tu, unsigned int flags)
{
	unsigned int delta = shaderstate.tmuflags[tu] ^ flags;
	if (!delta)
		return;
	if (delta & SHADER_PASS_CLAMP)
	{
		if (flags & SHADER_PASS_CLAMP)
		{
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);
		}
		else
		{
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ADDRESSW, D3DTADDRESS_WRAP);
		}
	}
	if (delta & (SHADER_PASS_NOMIPMAP|SHADER_PASS_NEAREST|SHADER_PASS_LINEAR|SHADER_PASS_UIPIC))
	{
		int min, mip, mag;
		int *filter = (flags & SHADER_PASS_UIPIC)?shaderstate.picfilter:shaderstate.mipfilter;

		if ((filter[2] && !(flags & SHADER_PASS_NEAREST)) || (flags & SHADER_PASS_LINEAR))
			mag = D3DTEXF_ANISOTROPIC;//D3DTEXF_LINEAR;
		else
			mag = D3DTEXF_POINT;
		if (filter[1] == -1 || (flags & IF_NOMIPMAP))
			mip = D3DTEXF_NONE;
		else if ((filter[1] && !(flags & SHADER_PASS_NEAREST)) || (flags & SHADER_PASS_LINEAR))
			mip = D3DTEXF_ANISOTROPIC;//D3DTEXF_LINEAR;
		else
			mip = D3DTEXF_POINT;
		if ((filter[0] && !(flags & SHADER_PASS_NEAREST)) || (flags & SHADER_PASS_LINEAR))
			min = D3DTEXF_ANISOTROPIC;//D3DTEXF_LINEAR;
		else
			min = D3DTEXF_POINT;

		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_MINFILTER, min);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_MIPFILTER, mip);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_MAGFILTER, mag);
	}
	shaderstate.tmuflags[tu] = flags;
}

//d3d8 is all sampler state
void D3D8_UpdateFiltering(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float lodbias, float anis)
{
	int i;
	memcpy(shaderstate.mipfilter, filtermip, sizeof(shaderstate.mipfilter));
	memcpy(shaderstate.picfilter, filterpic, sizeof(shaderstate.picfilter));

	for (i = 0; i < MAX_TMUS; i++)
	{
		shaderstate.tmuflags[i] = ~shaderstate.tmuflags[i];
		BE_ApplyTMUState(i, ~shaderstate.tmuflags[i]);

		IDirect3DDevice8_SetTextureStageState(pD3DDev8, i, D3DTSS_MAXANISOTROPY, anis);
	}
}

static void BE_ApplyShaderBits(unsigned int bits)
{
	unsigned int delta;

	if (shaderstate.flags & (BEF_FORCEADDITIVE|BEF_FORCETRANSPARENT|BEF_FORCENODEPTH|BEF_FORCEDEPTHTEST|BEF_FORCEDEPTHWRITE))
	{
		if (shaderstate.flags & BEF_FORCEADDITIVE)
			bits = (bits & ~(SBITS_MISC_DEPTHWRITE|SBITS_BLEND_BITS|SBITS_ATEST_BITS))
						| (SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE);
		else if (shaderstate.flags & BEF_FORCETRANSPARENT)
		{
			if ((bits & SBITS_BLEND_BITS) == (SBITS_SRCBLEND_ONE|SBITS_DSTBLEND_ZERO) || !(bits & SBITS_BLEND_BITS)) 	/*if transparency is forced, clear alpha test bits*/
				bits = (bits & ~(SBITS_MISC_DEPTHWRITE|SBITS_BLEND_BITS|SBITS_ATEST_BITS))
							| (SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		}

		if (shaderstate.flags & BEF_FORCENODEPTH) 	/*EF_NODEPTHTEST dp extension*/
			bits |= SBITS_MISC_NODEPTHTEST;
		else
		{
			if (shaderstate.flags & BEF_FORCEDEPTHTEST)
				bits &= ~SBITS_MISC_NODEPTHTEST;
			if (shaderstate.flags & BEF_FORCEDEPTHWRITE)
				bits |= SBITS_MISC_DEPTHWRITE;
		}
	}

#ifdef FORCESTATE
	delta = ~0;
#else
	delta = bits ^ shaderstate.shaderbits;
#endif
	if (!delta)
		return;
	shaderstate.shaderbits = bits;

	if (delta & SBITS_BLEND_BITS)
	{
		if (bits & SBITS_BLEND_BITS)
		{
			D3DBLEND src;
			D3DBLEND dst;

			switch(bits & SBITS_SRCBLEND_BITS)
			{
			case SBITS_SRCBLEND_ZERO:					src = D3DBLEND_ZERO; break;
			case SBITS_SRCBLEND_ONE:					src = D3DBLEND_ONE; break;
			case SBITS_SRCBLEND_DST_COLOR:				src = D3DBLEND_DESTCOLOR; break;
			case SBITS_SRCBLEND_ONE_MINUS_DST_COLOR:	src = D3DBLEND_INVDESTCOLOR; break;
			case SBITS_SRCBLEND_SRC_ALPHA:				src = D3DBLEND_SRCALPHA; break;
			case SBITS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	src = D3DBLEND_INVSRCALPHA; break;
			case SBITS_SRCBLEND_DST_ALPHA:				src = D3DBLEND_DESTALPHA; break;
			case SBITS_SRCBLEND_ONE_MINUS_DST_ALPHA:	src = D3DBLEND_INVDESTALPHA; break;
			case SBITS_SRCBLEND_ALPHA_SATURATE:			src = D3DBLEND_SRCALPHASAT; break;
			default:	Sys_Error("Bad shader blend src\n"); return;
			}
			switch(bits & SBITS_DSTBLEND_BITS)
			{
			case SBITS_DSTBLEND_ZERO:					dst = D3DBLEND_ZERO; break;
			case SBITS_DSTBLEND_ONE:					dst = D3DBLEND_ONE; break;
			case SBITS_DSTBLEND_SRC_ALPHA:				dst = D3DBLEND_SRCALPHA; break;
			case SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	dst = D3DBLEND_INVSRCALPHA; break;
			case SBITS_DSTBLEND_DST_ALPHA:				dst = D3DBLEND_DESTALPHA; break;
			case SBITS_DSTBLEND_ONE_MINUS_DST_ALPHA:	dst = D3DBLEND_INVDESTALPHA; break;
			case SBITS_DSTBLEND_SRC_COLOR:				dst = D3DBLEND_SRCCOLOR; break;
			case SBITS_DSTBLEND_ONE_MINUS_SRC_COLOR:	dst = D3DBLEND_INVSRCCOLOR; break;
			default:	Sys_Error("Bad shader blend dst\n"); return;
			}
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHABLENDENABLE, TRUE);
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_SRCBLEND, src);
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_DESTBLEND, dst);
		}
		else
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHABLENDENABLE, FALSE);
	}

	if (delta & SBITS_ATEST_BITS)
	{
		switch(bits & SBITS_ATEST_BITS)
		{
		case SBITS_ATEST_NONE:
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHATESTENABLE, FALSE);
	//		IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHAREF, 0);
	//		IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHAFUNC, 0);
			break;
		case SBITS_ATEST_GT0:
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHATESTENABLE, TRUE);
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHAREF, 0);
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHAFUNC, D3DCMP_GREATER);
			break;
		case SBITS_ATEST_LT128:
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHATESTENABLE, TRUE);
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHAREF, 128);
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHAFUNC, D3DCMP_LESS);
			break;
		case SBITS_ATEST_GE128:
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHATESTENABLE, TRUE);
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHAREF, 128);
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
			break;
		}
	}

	if (delta & SBITS_MISC_DEPTHWRITE)
	{
		if (bits & SBITS_MISC_DEPTHWRITE)
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ZWRITEENABLE, TRUE);
		else
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ZWRITEENABLE, FALSE);
	}

	if(delta & SBITS_MISC_NODEPTHTEST)
	{
		if(bits & SBITS_MISC_NODEPTHTEST)
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ZENABLE, FALSE);
		else
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ZENABLE, TRUE);
	}

	if (delta & SBITS_DEPTHFUNC_BITS)
	{
		switch(bits & SBITS_DEPTHFUNC_BITS)
		{
		default:
		case SBITS_DEPTHFUNC_CLOSEREQUAL:
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
			break;
		case SBITS_DEPTHFUNC_EQUAL:
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ZFUNC, D3DCMP_EQUAL);
			break;
		case SBITS_DEPTHFUNC_CLOSER:
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ZFUNC, D3DCMP_LESS);
			break;
		case SBITS_DEPTHFUNC_FURTHER:
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_ZFUNC, D3DCMP_GREATER);
			break;
		}
	}

	if (delta & (SBITS_MASK_BITS))
	{
		IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_COLORWRITEENABLE,
				((bits&SBITS_MASK_RED)?0:D3DCOLORWRITEENABLE_RED) |
				((bits&SBITS_MASK_GREEN)?0:D3DCOLORWRITEENABLE_GREEN) |
				((bits&SBITS_MASK_BLUE)?0:D3DCOLORWRITEENABLE_BLUE) |
				((bits&SBITS_MASK_ALPHA)?0:D3DCOLORWRITEENABLE_ALPHA));
	}
}

void D3D8BE_Reset(qboolean before)
{
	int i;
	if (before)
	{
		IDirect3DDevice8_SetVertexShader(pD3DDev8, 0);
		IDirect3DDevice8_SetStreamSource(pD3DDev8, 0, NULL, 0);
		IDirect3DDevice8_SetIndices(pD3DDev8, NULL, 0);

		if (shaderstate.dynvbo_buff)
			IDirect3DVertexBuffer8_Release(shaderstate.dynvbo_buff);
		shaderstate.dynvbo_buff = NULL;
		if (shaderstate.dynidx_buff)
			IDirect3DIndexBuffer8_Release(shaderstate.dynidx_buff);
		shaderstate.dynidx_buff = NULL;
	}
	else
	{
		if (shaderstate.dynidx_buff)
			return;

		IDirect3DDevice8_CreateVertexBuffer(pD3DDev8, shaderstate.dynvbo_size, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &shaderstate.dynvbo_buff);
		IDirect3DDevice8_CreateIndexBuffer(pD3DDev8, shaderstate.dynidx_size, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY, D3DFMT_QINDEX, D3DPOOL_DEFAULT, &shaderstate.dynidx_buff);

		for (i = 0; i < MAX_TMUS; i++)
		{
			shaderstate.tmuflags[i] = ~0;
			BE_ApplyTMUState(i, 0);
		}

		/*force all state to change, thus setting a known state*/
		shaderstate.shaderbits = ~0;
		BE_ApplyShaderBits(0);
	}
}

static const char LIGHTPASS_SHADER[] = "\
{\n\
	program rtlight\n\
	{\n\
		map $diffuse\n\
		blendfunc add\n\
	}\n\
	{\n\
		map $normalmap\n\
	}\n\
	{\n\
		map $specular\n\
	}\n\
}";

void D3D8BE_Init(void)
{
	be_maxpasses = MAX_TMUS;
	memset(&shaderstate, 0, sizeof(shaderstate));

	FTable_Init();

	shaderstate.dynvbo_size = sizeof(vbovdata_t) * DYNVBUFFSIZE;
	shaderstate.dynidx_size = sizeof(index_t) * DYNIBUFFSIZE;

	D3D8BE_Reset(false);

	shaderstate.shader_rtlight = R_RegisterShader("rtlight", SUF_NONE, LIGHTPASS_SHADER);

	R_InitFlashblends();
}

void D3D8BE_Set2D(void)
{	//start of some 2d rendering code.
	r_refdef.time = realtime;
	shaderstate.curtime = r_refdef.time;
}

static void allocvertexbuffer(IDirect3DVertexBuffer8 *buff, unsigned int bmaxsize, unsigned int *offset, void **data, unsigned int bytes)
{
	unsigned int boff;
	if (*offset + bytes > bmaxsize)
	{
		boff = 0;
		*offset = bytes;
	}
	else
	{
		boff = *offset;
		*offset += bytes;
	}
	d3dcheck(IDirect3DVertexBuffer8_Lock(buff, boff, bytes, (BYTE**)data, boff?D3DLOCK_NOOVERWRITE:D3DLOCK_DISCARD));
}

static unsigned int allocindexbuffer(void **dest, unsigned int entries)
{
	unsigned int bytes = entries*sizeof(index_t);
	unsigned int offset;

	if (shaderstate.dynidx_offs + bytes > DYNIBUFFSIZE)
	{
		offset = 0;
		shaderstate.dynidx_offs = 0;
	}
	else
	{
		offset = shaderstate.dynidx_offs;
		shaderstate.dynidx_offs += bytes;
	}

	d3dcheck(IDirect3DIndexBuffer8_Lock(shaderstate.dynidx_buff, offset, (unsigned int)entries, (BYTE**)dest, offset?D3DLOCK_NOOVERWRITE:D3DLOCK_DISCARD));
	return offset/sizeof(index_t);
}

static void BindTexture(unsigned int tu, texid_t tex)
{
	IDirect3DTexture8 *dt;
	if (tex)
	{
		dt = tex->ptr;
		shaderstate.curtexflags[tu] = tex->flags & SHADER_PASS_IMAGE_FLAGS_D3D8;
	}
	else
		dt = NULL;
	if (shaderstate.curtex[tu] != dt)
	{
		shaderstate.curtex[tu] = dt;
		IDirect3DDevice8_SetTexture (pD3DDev8, tu, (void*)dt);
	}
}

static void SelectPassTexture(unsigned int tu, shaderpass_t *pass)
{
	int last;
	extern texid_t missing_texture;

	switch(pass->texgen)
	{
	default:
	case T_GEN_DIFFUSE:
		if (shaderstate.curtexnums->base)
			BindTexture(tu, shaderstate.curtexnums->base);
		else
			BindTexture(tu, missing_texture);
		break;
	case T_GEN_NORMALMAP:
		BindTexture( tu, shaderstate.curtexnums->bump);
		break;
	case T_GEN_SPECULAR:
		BindTexture(tu, shaderstate.curtexnums->specular);
		break;
	case T_GEN_UPPEROVERLAY:
		BindTexture(tu, shaderstate.curtexnums->upperoverlay);
		break;
	case T_GEN_LOWEROVERLAY:
		BindTexture(tu, shaderstate.curtexnums->loweroverlay);
		break;
	case T_GEN_FULLBRIGHT:
		BindTexture(tu, shaderstate.curtexnums->fullbright);
		break;
	case T_GEN_ANIMMAP:
		BindTexture(tu, pass->anim_frames[(int)(pass->anim_fps * shaderstate.curtime) % pass->anim_numframes]);
		break;
	case T_GEN_SINGLEMAP:
		BindTexture(tu, pass->anim_frames[0]);
		break;
	case T_GEN_DELUXMAP:
		BindTexture(tu, shaderstate.curdeluxmap);
		break;
	case T_GEN_LIGHTMAP:
		BindTexture(tu, shaderstate.curlightmap);
		break;

	/*case T_GEN_CURRENTRENDER:
		FIXME: no code to grab the current screen and convert to a texture
		break;*/
	case T_GEN_VIDEOMAP:
#ifdef HAVE_MEDIA_DECODER
		BindTexture(tu, Media_UpdateForShader(pass->cin));
#else
		BindTexture(tu, missing_texture);
#endif
		break;
	}

	BE_ApplyTMUState(tu, shaderstate.curtexflags[tu]|pass->flags);

	if (tu == 0)
	{
		/*if (shaderstate.passsinglecolour)
		{
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_CONSTANT, shaderstate.passcolour);
			last = D3DTA_CONSTANT;
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_COLORVERTEX, FALSE);
		}
		else*/
		{
			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_COLORVERTEX, TRUE);
//			IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);	//default state anyway.
			last = D3DTA_DIFFUSE;
		}
	}
	else
		last = D3DTA_CURRENT;

	switch (pass->blendmode)
	{
	case PBM_DOTPRODUCT:
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG1, last);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLOROP, D3DTOP_DOTPRODUCT3);

		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG1, last);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
		break;
	case PBM_REPLACE:
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG1, last);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLOROP, D3DTOP_SELECTARG2);

		if (shaderstate.flags & (BEF_FORCETRANSPARENT | BEF_FORCEADDITIVE))
		{
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG1, last);
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		}
		else
		{
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG1, last);
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
			IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		}
		break;
	case PBM_ADD:
		if (tu == 0)
			goto forcemod;
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG1, last);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLOROP, D3DTOP_ADD);

		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG1, last);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		break;
	case PBM_DECAL:
		if (!tu)
			goto forcemod;
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG2, last);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLOROP, D3DTOP_BLENDTEXTUREALPHA);

		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG1, last);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		break;
	case PBM_OVERBRIGHT:
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG1, last);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		{
			extern cvar_t gl_overbright;
			switch (gl_overbright.ival)
			{
			case 1:
				IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
				break;
			case 2:
			case 3:
				IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLOROP, D3DTOP_MODULATE4X);
				break;
			default:
				IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLOROP, D3DTOP_MODULATE);
				break;
			}
		}
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG1, last);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		break;
	default:
	case PBM_MODULATE:
	forcemod:
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG1, last);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_COLOROP, D3DTOP_MODULATE);

		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG1, last);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
		IDirect3DDevice8_SetTextureStageState(pD3DDev8, tu, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		break;
	}
}

#if FIXME
static void colourgenbyte(const shaderpass_t *pass, int cnt, byte_vec4_t *srcb, vec4_t *srcf, byte_vec4_t *dst, const mesh_t *mesh)
{
	D3DCOLOR block;
	switch (pass->rgbgen)
	{
	case RGB_GEN_ENTITY:
		block = D3DCOLOR_COLORVALUE(shaderstate.curentity->shaderRGBAf[0], shaderstate.curentity->shaderRGBAf[1], shaderstate.curentity->shaderRGBAf[2], shaderstate.curentity->shaderRGBAf[3]);
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
		break;
	case RGB_GEN_ONE_MINUS_ENTITY:
		block = D3DCOLOR_COLORVALUE(1-shaderstate.curentity->shaderRGBAf[0], 1-shaderstate.curentity->shaderRGBAf[1], 1-shaderstate.curentity->shaderRGBAf[2], 1-shaderstate.curentity->shaderRGBAf[3]);
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
		break;
	case RGB_GEN_VERTEX_LIGHTING:
	case RGB_GEN_VERTEX_EXACT:
		if (srcb)
		{
			while((cnt)--)
			{
				qbyte r, g, b;
				r=srcb[cnt][0];
				g=srcb[cnt][1];
				b=srcb[cnt][2];
				dst[cnt][0] = b;
				dst[cnt][1] = g;
				dst[cnt][2] = r;
			}
		}
		else if (srcf)
		{
			while((cnt)--)
			{
				int r, g, b;
				r=srcf[cnt][0]*255;
				g=srcf[cnt][1]*255;
				b=srcf[cnt][2]*255;
				dst[cnt][0] = bound(0, b, 255);
				dst[cnt][1] = bound(0, g, 255);
				dst[cnt][2] = bound(0, r, 255);
			}
		}
		else
			goto identity;
		break;
	case RGB_GEN_ONE_MINUS_VERTEX:
		if (srcb)
		{
			while((cnt)--)
			{
				qbyte r, g, b;
				r=255-srcb[cnt][0];
				g=255-srcb[cnt][1];
				b=255-srcb[cnt][2];
				dst[cnt][0] = b;
				dst[cnt][1] = g;
				dst[cnt][2] = r;
			}
		}
		else if (srcf)
		{
			while((cnt)--)
			{
				int r, g, b;
				r=255-srcf[cnt][0]*255;
				g=255-srcf[cnt][1]*255;
				b=255-srcf[cnt][2]*255;
				dst[cnt][0] = bound(0, b, 255);
				dst[cnt][1] = bound(0, g, 255);
				dst[cnt][2] = bound(0, r, 255);
			}
		}
		else
			goto identity;
		break;
	case RGB_GEN_IDENTITY_LIGHTING:
		//compensate for overbrights
		block = D3DCOLOR_RGBA(255, 255, 255, 255); //shaderstate.identitylighting
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
		break;
	default:
	identity:
	case RGB_GEN_IDENTITY:
		block = D3DCOLOR_RGBA(255, 255, 255, 255);
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
		break;
	case RGB_GEN_CONST:
		block = D3DCOLOR_COLORVALUE(pass->rgbgen_func.args[0], pass->rgbgen_func.args[1], pass->rgbgen_func.args[2], 1);
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
		break;
	case RGB_GEN_LIGHTING_DIFFUSE:
		//collect lighting details for mobile entities
		if (!mesh->normals_array)
		{
			block = D3DCOLOR_RGBA(255, 255, 255, 255);
			while((cnt)--)
			{
				((D3DCOLOR*)dst)[cnt] = block;
			}
		}
		else
		{
			R_LightArraysByte_BGR(shaderstate.curentity , mesh->xyz_array, dst, cnt, mesh->normals_array);
		}
		break;
	case RGB_GEN_WAVE:
		{
			float *table;
			float c;

			table = FTableForFunc(pass->rgbgen_func.type);
			c = pass->rgbgen_func.args[2] + shaderstate.curtime * pass->rgbgen_func.args[3];
			c = FTABLE_EVALUATE(table, c) * pass->rgbgen_func.args[1] + pass->rgbgen_func.args[0];
			c = bound(0.0f, c, 1.0f);
			block = D3DCOLOR_COLORVALUE(c, c, c, 1);

			while((cnt)--)
			{
				((D3DCOLOR*)dst)[cnt] = block;
			}
		}
		break;

	case RGB_GEN_TOPCOLOR:
	case RGB_GEN_BOTTOMCOLOR:
#ifdef warningmsg
#pragma warningmsg("fix 24bit player colours")
#endif
		block = D3DCOLOR_RGBA(255, 255, 255, 255);
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
	//	Con_Printf("RGB_GEN %i not supported\n", pass->rgbgen);
		break;
	}
}

static void alphagenbyte(const shaderpass_t *pass, int cnt, byte_vec4_t *srcb, vec4_t *srcf, byte_vec4_t *dst, const mesh_t *mesh)
{
	/*FIXME: Skip this if the rgbgen did it*/
	float *table;
	unsigned char t;
	float f;
	vec3_t v1, v2;

	switch (pass->alphagen)
	{
	default:
	case ALPHA_GEN_IDENTITY:
		if (shaderstate.flags & BEF_FORCETRANSPARENT)
		{
			f = shaderstate.curentity->shaderRGBAf[3];
			if (f < 0)
				t = 0;
			else if (f >= 1)
				t = 255;
			else
				t = f*255;
			while(cnt--)
				dst[cnt][3] = t;
		}
		else
		{
			while(cnt--)
				dst[cnt][3] = 255;
		}
		break;

	case ALPHA_GEN_CONST:
		t = pass->alphagen_func.args[0]*255;
		while(cnt--)
			dst[cnt][3] = t;
		break;

	case ALPHA_GEN_WAVE:
		table = FTableForFunc(pass->alphagen_func.type);
		f = pass->alphagen_func.args[2] + shaderstate.curtime * pass->alphagen_func.args[3];
		f = FTABLE_EVALUATE(table, f) * pass->alphagen_func.args[1] + pass->alphagen_func.args[0];
		t = bound(0.0f, f, 1.0f)*255;
		while(cnt--)
			dst[cnt][3] = t;
		break;

	case ALPHA_GEN_PORTAL:
		//FIXME: should this be per-vert?
		VectorAdd(mesh->xyz_array[0], shaderstate.curentity->origin, v1);
		VectorSubtract(r_origin, v1, v2);
		f = VectorLength(v2) * (1.0 / 255.0);
		t = bound(0.0f, f, 1.0f)*255;

		while(cnt--)
			dst[cnt][3] = t;
		break;

	case ALPHA_GEN_VERTEX:
		if (srcb)
		{
			while(cnt--)
			{
				dst[cnt][3] = srcb[cnt][3];
			}
		}
		else if (srcf)
		{
			while(cnt--)
			{
				dst[cnt][3] = bound(0, srcf[cnt][3]*255, 255);
			}
		}
		else
		{
			while(cnt--)
			{
				dst[cnt][3] = 255;
			}
		}
		break;

	case ALPHA_GEN_ENTITY:
		t = bound(0, shaderstate.curentity->shaderRGBAf[3], 1)*255;
		while(cnt--)
		{
			dst[cnt][3] = t;
		}
		break;

	case ALPHA_GEN_SPECULAR:
		{
			int i;
			VectorSubtract(r_origin, shaderstate.curentity->origin, v1);

			if (!Matrix3_Compare(shaderstate.curentity->axis, (void *)axisDefault))
			{
				Matrix3_Multiply_Vec3(shaderstate.curentity->axis, v1, v2);
			}
			else
			{
				VectorCopy(v1, v2);
			}

			for (i = 0; i < cnt; i++)
			{
				VectorSubtract(v2, mesh->xyz_array[i], v1);
				f = DotProduct(v1, mesh->normals_array[i]) * Q_rsqrt(DotProduct(v1,v1));
				f = f * f * f * f * f;
				dst[i][3] = bound (0, (int)(f*255), 255);
			}
		}
		break;
	}
}

static unsigned int BE_GenerateColourMods(unsigned int vertcount, const shaderpass_t *pass)
{
	unsigned int ret = 0;
#ifdef FIXME
	unsigned char *map;
	const mesh_t *m;
	unsigned int mno;

	m = shaderstate.meshlist[0];

	if (pass->flags & SHADER_PASS_NOCOLORARRAY)
	{
		shaderstate.passsinglecolour = true;
		shaderstate.passcolour = D3DCOLOR_RGBA(255,255,255,255);
		colourgenbyte(pass, 1, (byte_vec4_t*)&shaderstate.passcolour, NULL, (byte_vec4_t*)&shaderstate.passcolour, m);
		alphagenbyte(pass, 1, (byte_vec4_t*)&shaderstate.passcolour, NULL, (byte_vec4_t*)&shaderstate.passcolour, m);
		/*FIXME: just because there's no rgba set, there's no reason to assume it should be a single colour (unshaded ents)*/
		d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_COL, NULL, 0, 0));
	}
	else
	{
		shaderstate.passsinglecolour = false;

		ret |= D3D_VDEC_COL4B;
		if (shaderstate.batchvbo && (m->colors4f_array[0] &&
						((pass->rgbgen == RGB_GEN_VERTEX_LIGHTING) ||
						(pass->rgbgen == RGB_GEN_VERTEX_EXACT) ||
						(pass->rgbgen == RGB_GEN_ONE_MINUS_VERTEX)) &&
						(pass->alphagen == ALPHA_GEN_VERTEX)))
		{
			//fixme
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_COL, shaderstate.batchvbo->colours[0].d3d.buff, shaderstate.batchvbo->colours[0].d3d.offs, sizeof(vbovdata_t)));
		}
		else
		{
			allocvertexbuffer(shaderstate.dyncol_buff, shaderstate.dyncol_size, &shaderstate.dyncol_offs, (void**)&map, vertcount*sizeof(D3DCOLOR));
			for (mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				m = shaderstate.meshlist[mno];
				colourgenbyte(pass, m->numvertexes, m->colors4b_array, m->colors4f_array[0], (byte_vec4_t*)map, m);
				alphagenbyte(pass, m->numvertexes, m->colors4b_array, m->colors4f_array[0], (byte_vec4_t*)map, m);
				map += m->numvertexes*4;
			}
			d3dcheck(IDirect3DVertexBuffer8_Unlock(shaderstate.dyncol_buff));
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_COL, shaderstate.dyncol_buff, shaderstate.dyncol_offs - vertcount*sizeof(D3DCOLOR), sizeof(D3DCOLOR)));
		}
	}
#endif
	return ret;
}
#endif
/*********************************************************************************************************/
/*========================================== texture coord generation =====================================*/

#if FIXME
static void tcgen_environment(float *st, unsigned int numverts, float *xyz, float *normal)
{
	int			i;
	vec3_t		viewer, reflected;
	float		d;

	vec3_t		rorg;

	if (!normal)
	{
		for (i = 0 ; i < numverts ; i++, st += 2 )
		{
			st[0] = xyz[0];
			st[1] = xyz[1];
		}
		return;
	}

	RotateLightVector(shaderstate.curentity->axis, shaderstate.curentity->origin, r_origin, rorg);

	for (i = 0 ; i < numverts ; i++, xyz += sizeof(vecV_t)/sizeof(vec_t), normal += 3, st += 2 )
	{
		VectorSubtract (rorg, xyz, viewer);
		VectorNormalizeFast (viewer);

		d = DotProduct (normal, viewer);

		reflected[0] = normal[0]*2*d - viewer[0];
		reflected[1] = normal[1]*2*d - viewer[1];
		reflected[2] = normal[2]*2*d - viewer[2];

		st[0] = 0.5 + reflected[1] * 0.5;
		st[1] = 0.5 - reflected[2] * 0.5;
	}
}

static float *tcgen(const shaderpass_t *pass, int cnt, float *dst, const mesh_t *mesh)
{
	int i;
	vecV_t *src;
	switch (pass->tcgen)
	{
	default:
	case TC_GEN_BASE:
		return (float*)mesh->st_array;
	case TC_GEN_LIGHTMAP:
		return (float*)mesh->lmst_array[0];
	case TC_GEN_NORMAL:
		return (float*)mesh->normals_array;
	case TC_GEN_SVECTOR:
		return (float*)mesh->snormals_array;
	case TC_GEN_TVECTOR:
		return (float*)mesh->tnormals_array;
	case TC_GEN_ENVIRONMENT:
		tcgen_environment(dst, cnt, (float*)mesh->xyz_array, (float*)mesh->normals_array);
		return dst;

	case TC_GEN_DOTPRODUCT:
		return dst;//mesh->st_array[0];
	case TC_GEN_VECTOR:
		src = mesh->xyz_array;
		for (i = 0; i < cnt; i++, dst += 2)
		{
			dst[0] = DotProduct(pass->tcgenvec[0], src[i]);
			dst[1] = DotProduct(pass->tcgenvec[1], src[i]);
		}
		return dst;
	}
}

/*src and dst can be the same address when tcmods are chained*/
static void tcmod(const tcmod_t *tcmod, int cnt, const float *src, float *dst, const mesh_t *mesh)
{
	float *table;
	float t1, t2;
	float cost, sint;
	int j;
	switch (tcmod->type)
	{
		case SHADER_TCMOD_ROTATE:
			cost = tcmod->args[0] * shaderstate.curtime;
			sint = R_FastSin(cost);
			cost = R_FastSin(cost + 0.25);

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				t1 = cost * (src[0] - 0.5f) - sint * (src[1] - 0.5f) + 0.5f;
				t2 = cost * (src[1] - 0.5f) + sint * (src[0] - 0.5f) + 0.5f;
				dst[0] = t1;
				dst[1] = t2;
			}
			break;

		case SHADER_TCMOD_SCALE:
			t1 = tcmod->args[0];
			t2 = tcmod->args[1];

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] * t1;
				dst[1] = src[1] * t2;
			}
			break;

		case SHADER_TCMOD_TURB:
			t1 = tcmod->args[2] + shaderstate.curtime * tcmod->args[3];
			t2 = tcmod->args[1];

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] + R_FastSin (src[0]*t2+t1) * t2;
				dst[1] = src[1] + R_FastSin (src[1]*t2+t1) * t2;
			}
			break;

		case SHADER_TCMOD_STRETCH:
			table = FTableForFunc(tcmod->args[0]);
			t2 = tcmod->args[3] + shaderstate.curtime * tcmod->args[4];
			t1 = FTABLE_EVALUATE(table, t2) * tcmod->args[2] + tcmod->args[1];
			t1 = t1 ? 1.0f / t1 : 1.0f;
			t2 = 0.5f - 0.5f * t1;
			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] * t1 + t2;
				dst[1] = src[1] * t1 + t2;
			}
			break;

		case SHADER_TCMOD_SCROLL:
			t1 = tcmod->args[0] * shaderstate.curtime;
			t2 = tcmod->args[1] * shaderstate.curtime;

			for (j = 0; j < cnt; j++, dst += 2, src+=2)
			{
				dst[0] = src[0] + t1;
				dst[1] = src[1] + t2;
			}
			break;

		case SHADER_TCMOD_TRANSFORM:
			for (j = 0; j < cnt; j++, dst+=2, src+=2)
			{
				t1 = src[0];
				t2 = src[1];
				dst[0] = t1 * tcmod->args[0] + t2 * tcmod->args[2] + tcmod->args[4];
				dst[1] = t1 * tcmod->args[1] + t2 * tcmod->args[3] + tcmod->args[5];
			}
			break;

		default:
			break;
	}
}

static void GenerateTCMods(const shaderpass_t *pass, float *dest)
{
	mesh_t *mesh;
	unsigned int mno;
	// unsigned int fvertex = 0; //unused variable
//	int i;
	float *src;
	float *out;
	for (mno = 0; mno < shaderstate.nummeshes; mno++)
	{
		mesh = shaderstate.meshlist[mno];

#if 0
		out = dest + mesh->vbofirstvert*2;
#else
		out = dest;
		dest += mesh->numvertexes*2;
#endif

		src = tcgen(pass, mesh->numvertexes, out, mesh);
		//tcgen might return unmodified info
		/*if (pass->numtcmods)
		{
			for (i = 0; i < pass->numtcmods; i++)
			{
				tcmod(&pass->tcmods[i], mesh->numvertexes, src, out, mesh);
				src = out;
			}
		}
		else*/ if (src != out)
		{
			//memcpy(out, src, sizeof(vec2_t)*mesh->numvertexes);
		}
	}
}

//end texture coords
/*******************************************************************************************************************/

static void deformgen(const deformv_t *deformv, int cnt, vecV_t *src, vecV_t *dst, const mesh_t *mesh)
{
	float *table;
	int j, k;
	float args[4];
	float deflect;
	switch (deformv->type)
	{
	default:
	case DEFORMV_NONE:
		if (src != dst)
			memcpy(dst, src, sizeof(*src)*cnt);
		break;

	case DEFORMV_WAVE:
		if (!mesh->normals_array)
		{
			if (src != dst)
				memcpy(dst, src, sizeof(*src)*cnt);
			return;
		}
		args[0] = deformv->func.args[0];
		args[1] = deformv->func.args[1];
		args[3] = deformv->func.args[2] + deformv->func.args[3] * shaderstate.curtime;
		table = FTableForFunc(deformv->func.type);

		for ( j = 0; j < cnt; j++ )
		{
			deflect = deformv->args[0] * (src[j][0]+src[j][1]+src[j][2]) + args[3];
			deflect = FTABLE_EVALUATE(table, deflect) * args[1] + args[0];

			// Deflect vertex along its normal by wave amount
			VectorMA(src[j], deflect, mesh->normals_array[j], dst[j]);
		}
		break;

	case DEFORMV_NORMAL:
		//normal does not actually move the verts, but it does change the normals array
		//we don't currently support that.
		if (src != dst)
			memcpy(dst, src, sizeof(*src)*cnt);
/*
		args[0] = deformv->args[1] * shaderstate.curtime;

		for ( j = 0; j < cnt; j++ )
		{
			args[1] = normalsArray[j][2] * args[0];

			deflect = deformv->args[0] * R_FastSin(args[1]);
			normalsArray[j][0] *= deflect;
			deflect = deformv->args[0] * R_FastSin(args[1] + 0.25);
			normalsArray[j][1] *= deflect;
			VectorNormalizeFast(normalsArray[j]);
		}
*/		break;

	case DEFORMV_MOVE:
		table = FTableForFunc(deformv->func.type);
		deflect = deformv->func.args[2] + shaderstate.curtime * deformv->func.args[3];
		deflect = FTABLE_EVALUATE(table, deflect) * deformv->func.args[1] + deformv->func.args[0];

		for ( j = 0; j < cnt; j++ )
			VectorMA(src[j], deflect, deformv->args, dst[j]);
		break;

	case DEFORMV_BULGE:
		args[0] = deformv->args[0]/(2*M_PI);
		args[1] = deformv->args[1];
		args[2] = shaderstate.curtime * deformv->args[2]/(2*M_PI);

		for (j = 0; j < cnt; j++)
		{
			deflect = R_FastSin(mesh->st_array[j][0]*args[0] + args[2])*args[1];
			dst[j][0] = src[j][0]+deflect*mesh->normals_array[j][0];
			dst[j][1] = src[j][1]+deflect*mesh->normals_array[j][1];
			dst[j][2] = src[j][2]+deflect*mesh->normals_array[j][2];
		}
		break;

	case DEFORMV_AUTOSPRITE:
		if (mesh->numindexes < 6)
			break;

		for (j = 0; j < cnt-3; j+=4, src+=4, dst+=4)
		{
			vec3_t mid, d;
			float radius;
			mid[0] = 0.25*(src[0][0] + src[1][0] + src[2][0] + src[3][0]);
			mid[1] = 0.25*(src[0][1] + src[1][1] + src[2][1] + src[3][1]);
			mid[2] = 0.25*(src[0][2] + src[1][2] + src[2][2] + src[3][2]);
			VectorSubtract(src[0], mid, d);
			radius = 2*VectorLength(d);

			for (k = 0; k < 4; k++)
			{
				dst[k][0] = mid[0] + radius*((mesh->st_array[j+k][0]-0.5)*r_refdef.m_view[0+0]-(mesh->st_array[j+k][1]-0.5)*r_refdef.m_view[0+1]);
				dst[k][1] = mid[1] + radius*((mesh->st_array[j+k][0]-0.5)*r_refdef.m_view[4+0]-(mesh->st_array[j+k][1]-0.5)*r_refdef.m_view[4+1]);
				dst[k][2] = mid[2] + radius*((mesh->st_array[j+k][0]-0.5)*r_refdef.m_view[8+0]-(mesh->st_array[j+k][1]-0.5)*r_refdef.m_view[8+1]);
			}
		}
		break;

	case DEFORMV_AUTOSPRITE2:
		if (mesh->numindexes < 6)
			break;

		for (k = 0; k < mesh->numindexes; k += 6)
		{
			int long_axis, short_axis;
			vec3_t axis;
			float len[3];
			mat3_t m0, m1, m2, result;
			float *quad[4];
			vec3_t rot_centre, tv, tv2;

			quad[0] = (float *)(src + mesh->indexes[k+0]);
			quad[1] = (float *)(src + mesh->indexes[k+1]);
			quad[2] = (float *)(src + mesh->indexes[k+2]);

			for (j = 2; j >= 0; j--)
			{
				quad[3] = (float *)(src + mesh->indexes[k+3+j]);
				if (!VectorEquals (quad[3], quad[0]) &&
					!VectorEquals (quad[3], quad[1]) &&
					!VectorEquals (quad[3], quad[2]))
				{
					break;
				}
			}

			// build a matrix were the longest axis of the billboard is the Y-Axis
			VectorSubtract(quad[1], quad[0], m0[0]);
			VectorSubtract(quad[2], quad[0], m0[1]);
			VectorSubtract(quad[2], quad[1], m0[2]);
			len[0] = DotProduct(m0[0], m0[0]);
			len[1] = DotProduct(m0[1], m0[1]);
			len[2] = DotProduct(m0[2], m0[2]);

			if ((len[2] > len[1]) && (len[2] > len[0]))
			{
				if (len[1] > len[0])
				{
					long_axis = 1;
					short_axis = 0;
				}
				else
				{
					long_axis = 0;
					short_axis = 1;
				}
			}
			else if ((len[1] > len[2]) && (len[1] > len[0]))
			{
				if (len[2] > len[0])
				{
					long_axis = 2;
					short_axis = 0;
				}
				else
				{
					long_axis = 0;
					short_axis = 2;
				}
			}
			else //if ( (len[0] > len[1]) && (len[0] > len[2]) )
			{
				if (len[2] > len[1])
				{
					long_axis = 2;
					short_axis = 1;
				}
				else
				{
					long_axis = 1;
					short_axis = 2;
				}
			}

			if (DotProduct(m0[long_axis], m0[short_axis]))
			{
				VectorNormalize2(m0[long_axis], axis);
				VectorCopy(axis, m0[1]);

				if (axis[0] || axis[1])
				{
					VectorVectors(m0[1], m0[2], m0[0]);
				}
				else
				{
					VectorVectors(m0[1], m0[0], m0[2]);
				}
			}
			else
			{
				VectorNormalize2(m0[long_axis], axis);
				VectorNormalize2(m0[short_axis], m0[0]);
				VectorCopy(axis, m0[1]);
				CrossProduct(m0[0], m0[1], m0[2]);
			}

			for (j = 0; j < 3; j++)
				rot_centre[j] = (quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j]) * 0.25;

			if (shaderstate.curentity)
			{
				VectorAdd(shaderstate.curentity->origin, rot_centre, tv);
			}
			else
			{
				VectorCopy(rot_centre, tv);
			}
			VectorSubtract(r_origin, tv, tv);

			// filter any longest-axis-parts off the camera-direction
			deflect = -DotProduct(tv, axis);

			VectorMA(tv, deflect, axis, m1[2]);
			VectorNormalizeFast(m1[2]);
			VectorCopy(axis, m1[1]);
			CrossProduct(m1[1], m1[2], m1[0]);

			Matrix3_Transpose(m1, m2);
			Matrix3_Multiply(m2, m0, result);

			for (j = 0; j < 4; j++)
			{
				int v = ((vecV_t*)quad[j]-src);
				VectorSubtract(quad[j], rot_centre, tv);
				Matrix3_Multiply_Vec3((void *)result, tv, tv2);
				VectorAdd(rot_centre, tv2, dst[v]);
			}
		}
		break;

//	case DEFORMV_PROJECTION_SHADOW:
//		break;
	}
}
#endif


/*does not do the draw call, does not consider indicies (except for billboard generation) */
static qboolean BE_DrawMeshChain_SetupPass(shaderpass_t *pass, unsigned int vertcount, unsigned int *vertfirst)
{
	int vdec;
	vbovdata_t *map;
	int i, mno;
	unsigned int passno = 0, tmu;
	mesh_t *mesh;

	int lastpass = pass->numMergedPasses;

	for (i = 0; i < lastpass; i++)
	{
		if (pass[i].texgen == T_GEN_UPPEROVERLAY && !TEXLOADED(shaderstate.curtexnums->upperoverlay))
			continue;
		if (pass[i].texgen == T_GEN_LOWEROVERLAY && !TEXLOADED(shaderstate.curtexnums->loweroverlay))
			continue;
		if (pass[i].texgen == T_GEN_FULLBRIGHT && !TEXLOADED(shaderstate.curtexnums->fullbright))
			continue;
		break;
	}
	if (i == lastpass)
		return false;

	/*all meshes in a chain must have the same features*/
	vdec = D3DFVF_QVBO;

	allocvertexbuffer(shaderstate.dynvbo_buff, shaderstate.dynvbo_size, &shaderstate.dynvbo_offs, (void**)&map, vertcount*sizeof(*map));
	*vertfirst = (shaderstate.dynvbo_offs - vertcount*sizeof(*map))/sizeof(*map);


	for (mno = 0; mno < shaderstate.nummeshes; mno++)
	{
		mesh = shaderstate.meshlist[mno];

		for(i = 0; i < mesh->numvertexes; i++, map++)
		{
			VectorCopy(mesh->xyz_array[i], map->coord);
			if (mesh->colors4f_array[0])
			{
				map->colorsb[0] = mesh->colors4f_array[0][i][2]*255;
				map->colorsb[1] = mesh->colors4f_array[0][i][1]*255;
				map->colorsb[2] = mesh->colors4f_array[0][i][0]*255;
				map->colorsb[3] = mesh->colors4f_array[0][i][3]*255;
			}
			else if (mesh->colors4b_array)
			{
				map->colorsb[0] = mesh->colors4b_array[i][2];
				map->colorsb[1] = mesh->colors4b_array[i][1];
				map->colorsb[2] = mesh->colors4b_array[i][0];
				map->colorsb[3] = mesh->colors4b_array[i][3];
			}
			else
				Vector4Set(map->colorsb, 255, 255, 255, 255);
			Vector2Copy(mesh->st_array[i], map->tc[0]);
			if (mesh->lmst_array[0])
				Vector2Copy(mesh->lmst_array[0][i], map->tc[1]);
		}
	}
	d3dcheck(IDirect3DVertexBuffer8_Unlock(shaderstate.dynvbo_buff));

	d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, 0, shaderstate.dynvbo_buff, sizeof(vbovdata_t)));
	d3dcheck(IDirect3DDevice8_SetIndices(pD3DDev8, shaderstate.dynidx_buff, *vertfirst));	//base vertex is considered part of ebo state in d3d8, I guess

	/*we only use one colour, generated from the first pass*/
	//vdec |= BE_GenerateColourMods(vertcount, pass);

	tmu = 0;
	/*activate tmus*/
	for (passno = 0; passno < lastpass; passno++)
	{
		if (pass[passno].texgen == T_GEN_UPPEROVERLAY && !TEXLOADED(shaderstate.curtexnums->upperoverlay))
			continue;
		if (pass[passno].texgen == T_GEN_LOWEROVERLAY && !TEXLOADED(shaderstate.curtexnums->loweroverlay))
			continue;
		if (pass[passno].texgen == T_GEN_FULLBRIGHT && !TEXLOADED(shaderstate.curtexnums->fullbright))
			continue;

		SelectPassTexture(tmu, pass+passno);

		if (pass[passno].tcgen == TC_GEN_BASE)
			d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, tmu, D3DTSS_TEXCOORDINDEX, 0));
		else
			d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, tmu, D3DTSS_TEXCOORDINDEX, 1));


/*		vdec |= D3D_VDEC_ST0<<tmu;
		if (shaderstate.batchvbo && pass[passno].tcgen == TC_GEN_BASE && !pass[passno].numtcmods)
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_TC0+tmu, shaderstate.batchvbo->texcoord.d3d.buff, sizeof(vbovdata_t)));
		else if (shaderstate.batchvbo && pass[passno].tcgen == TC_GEN_LIGHTMAP && !pass[passno].numtcmods)
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_TC0+tmu, shaderstate.batchvbo->lmcoord[0].d3d.buff, sizeof(vbovdata_t)));
		else
		{
			allocvertexbuffer(shaderstate.dynst_buff[tmu], shaderstate.dynst_size, &shaderstate.dynst_offs[tmu], &map, vertcount*sizeof(vec2_t));
			GenerateTCMods(pass+passno, map);
			d3dcheck(IDirect3DVertexBuffer8_Unlock(shaderstate.dynst_buff[tmu]));
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_TC0+tmu, shaderstate.dynst_buff[tmu], shaderstate.dynst_offs[tmu] - vertcount*sizeof(vec2_t), sizeof(vec2_t)));
		}*/
		tmu++;
	}
	/*deactivate any extras*/
	for (; tmu < shaderstate.lastpasscount; )
	{
		BindTexture(tmu, NULL);
		d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, tmu, D3DTSS_COLOROP, D3DTOP_DISABLE));
		d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, tmu, D3DTSS_ALPHAOP, D3DTOP_DISABLE));
		tmu++;
	}
	shaderstate.lastpasscount = tmu;

	IDirect3DDevice8_SetVertexShader(pD3DDev8, vdec);

	BE_ApplyShaderBits(pass->shaderbits);
	return true;
}

static void BE_SubmitMeshChain(unsigned int firstvert, unsigned int vertcount, unsigned int idxfirst, int unsigned idxcount)
{
	if (shaderstate.flags & BEF_LINES)
		IDirect3DDevice8_DrawIndexedPrimitive(pD3DDev8, D3DPT_LINELIST, firstvert, vertcount, idxfirst, idxcount/2);
	else
		IDirect3DDevice8_DrawIndexedPrimitive(pD3DDev8, D3DPT_TRIANGLELIST, firstvert, vertcount, idxfirst, idxcount/3);

	RQuantAdd(RQUANT_DRAWS, 1);

	RQuantAdd(RQUANT_PRIMITIVEINDICIES, idxcount);
}

#ifdef FIXME
static void R_FetchPlayerColour(unsigned int cv, vec3_t rgb)
{
	int i;

	if (cv >= 16)
	{
		rgb[0] = (((cv&0xff0000)>>16)**((unsigned char*)&d_8to24rgbtable[15]+0)) / (256.0*256);
		rgb[1] = (((cv&0x00ff00)>>8)**((unsigned char*)&d_8to24rgbtable[15]+1)) / (256.0*256);
		rgb[2] = (((cv&0x0000ff)>>0)**((unsigned char*)&d_8to24rgbtable[15]+2)) / (256.0*256);
		return;
	}
	i = cv;
	if (i >= 8)
	{
		i<<=4;
	}
	else
	{
		i<<=4;
		i+=15;
	}
	i*=3;
	rgb[0] = host_basepal[i+0] / 255.0;
	rgb[1] = host_basepal[i+1] / 255.0;
	rgb[2] = host_basepal[i+2] / 255.0;
/*	if (!gammaworks)
	{
		*retred = gammatable[*retred];
		*retgreen = gammatable[*retgreen];
		*retblue = gammatable[*retblue];
	}*/
}

static void BE_ApplyUniforms(program_t *prog, int permu)
{
	struct programpermu_s *perm = &prog->permu[permu];
	shaderprogparm_t *pp;
	vec4_t param4;
	int h;
	int i;
	IDirect3DDevice8_SetVertexShader(pD3DDev8, perm->h.hlsl.vert);
	IDirect3DDevice8_SetPixelShader(pD3DDev8, perm->h.hlsl.frag);
	for (i = 0, pp = perm->parm; i < perm->numparms; i++, pp++)
	{
		h = pp->handle;
		switch (pp->type)
		{
		case SP_M_PROJECTION:
			IDirect3DDevice8_SetVertexShaderConstantF(pD3DDev8, h, d3d_trueprojection, 4);
			break;
		case SP_M_VIEW:
			IDirect3DDevice8_SetVertexShaderConstantF(pD3DDev8, h, r_refdef.m_view, 4);
			break;
		case SP_M_MODEL:
			IDirect3DDevice8_SetVertexShaderConstantF(pD3DDev8, h, shaderstate.m_model, 4);
			break;
		case SP_E_VBLEND:
			IDirect3DDevice8_SetVertexShaderConstantF(pD3DDev8, h, shaderstate.meshlist[0]->xyz_blendw, 2);
			break;


		case SP_V_EYEPOS:
			IDirect3DDevice8_SetPixelShaderConstantF(pD3DDev8, h, r_origin, 1);
			break;
		case SP_E_EYEPOS:
			{
				vec4_t t2;
				float m16[16];
				Matrix4x4_CM_ModelMatrixFromAxis(m16, shaderstate.curentity->axis[0], shaderstate.curentity->axis[1], shaderstate.curentity->axis[2], shaderstate.curentity->origin);
				Matrix4x4_CM_Transform3(m16, r_origin, t2);
				IDirect3DDevice8_SetPixelShaderConstantF(pD3DDev8, h, t2, 1);
			}
			break;
		case SP_E_TIME:
			{
				vec4_t t1 = {shaderstate.curtime};
				IDirect3DDevice8_SetPixelShaderConstantF(pD3DDev8, h, t1, 1);
			}
			break;
		case SP_M_MODELVIEWPROJECTION:
			{
				float mv[16], mvp[16];
				Matrix4_Multiply(r_refdef.m_view, shaderstate.m_model, mv);
				Matrix4_Multiply(d3d_trueprojection, mv, mvp);
				IDirect3DDevice8_SetVertexShaderConstantF(pD3DDev8, h, mvp, 4);
			}
			break;

		case SP_LIGHTPOSITION:
			{
				/*light position in model space*/
				float inv[16];
				vec3_t t2;
				qboolean Matrix4_Invert(const float *m, float *out);

				Matrix4_Invert(shaderstate.m_model, inv);
				Matrix4x4_CM_Transform3(inv, shaderstate.curdlight->origin, t2);
				IDirect3DDevice8_SetVertexShaderConstantF(pD3DDev8, h, t2, 1);
				break;
			}

		case SP_LIGHTRADIUS:
			IDirect3DDevice8_SetPixelShaderConstantF(pD3DDev8, h, &shaderstate.curdlight->radius, 1);
			break;
		case SP_LIGHTCOLOUR:
			IDirect3DDevice8_SetPixelShaderConstantF(pD3DDev8, h, shaderstate.curdlight_colours, 1);
			break;

		case SP_E_L_DIR:
			IDirect3DDevice8_SetVertexShaderConstantF(pD3DDev8, h, shaderstate.curentity->light_dir, 1);
			break;
		case SP_E_L_MUL:
			IDirect3DDevice8_SetVertexShaderConstantF(pD3DDev8, h, shaderstate.curentity->light_range, 1);
			break;
		case SP_E_L_AMBIENT:
			IDirect3DDevice8_SetVertexShaderConstantF(pD3DDev8, h, shaderstate.curentity->light_avg, 1);
			break;

		case SP_E_COLOURS:
			IDirect3DDevice8_SetPixelShaderConstantF(pD3DDev8, h, shaderstate.curentity->shaderRGBAf, 1);
			break;
		case SP_E_COLOURSIDENT:
			if (shaderstate.flags & BEF_FORCECOLOURMOD)
				IDirect3DDevice8_SetPixelShaderConstantF(pD3DDev8, h, shaderstate.curentity->shaderRGBAf, 1);
			else
			{
				vec4_t tmp = {1, 1, 1, shaderstate.curentity->shaderRGBAf[3]};
				IDirect3DDevice8_SetPixelShaderConstantF(pD3DDev8, h, tmp, 1);
			}
			break;

		case SP_E_TOPCOLOURS:
			R_FetchPlayerColour(shaderstate.curentity->topcolour, param4);
			IDirect3DDevice8_SetPixelShaderConstantF(pD3DDev8, h, param4, 3);
			break;
		case SP_E_BOTTOMCOLOURS:
			R_FetchPlayerColour(shaderstate.curentity->bottomcolour, param4);
			IDirect3DDevice8_SetPixelShaderConstantF(pD3DDev8, h, param4, 3);
			break;

		case SP_E_LMSCALE:
			Vector4Set(param4, 1, 1, 1, 1);
			if (shaderstate.curentity->model && (shaderstate.curentity->model->engineflags & MDLF_NEEDOVERBRIGHT))
			{
				extern cvar_t gl_overbright;
				const float identitylighting = 1;
				float sc = (1<<bound(0, gl_overbright.ival, 2)) * identitylighting;
				VectorSet(param4, sc, sc, sc);
			}
			else
			{
				const float identitylighting = 1;
				VectorSet(param4, identitylighting, identitylighting, identitylighting);
			}
			param4[3] = 1;
			IDirect3DDevice8_SetVertexShaderConstantF(pD3DDev8, h, param4, 3);
			break;

		case SP_M_ENTBONES:
		case SP_M_MODELVIEW:
		case SP_E_VLSCALE:
		case SP_E_ORIGIN:
		case SP_E_GLOWMOD:
		case SP_W_FOG:
		case SP_M_INVVIEWPROJECTION:
		case SP_M_INVMODELVIEWPROJECTION:
		case SP_SOURCESIZE:
		case SP_S_COLOUR:
		case SP_LIGHTCOLOURSCALE:
		case SP_LIGHTSCREEN:
		case SP_LIGHTCUBEMATRIX:
		case SP_LIGHTSHADOWMAPPROJ:
		case SP_LIGHTSHADOWMAPSCALE:

		case SP_RENDERTEXTURESCALE:

		case SP_FIRSTIMMEDIATE:
		case SP_CONSTI:
		case SP_CONSTF:
		case SP_CVARI:
		case SP_CVARF:
		case SP_CVAR3F:
		case SP_TEXTURE:
		case SP_BAD:
			Con_Printf("shader property %i not implemented\n", pp->type);
			break;
		}
	}
}

static void BE_RenderMeshProgram(shader_t *s, unsigned int vertbase, unsigned int vertfirst, unsigned int vertcount, unsigned int idxfirst, unsigned int idxcount)
{
	int vdec = D3D_VDEC_ST0|D3D_VDEC_ST1|D3D_VDEC_NORM;
	int passno;
	int perm = 0;

	program_t *p = s->prog;

#ifdef SKELETALMODELS
	if (shaderstate.batchvbo && shaderstate.batchvbo->numbones)
	{
		if (p->permu[perm|PERMUTATION_SKELETAL].h.loaded)
			perm |= PERMUTATION_SKELETAL;
		else
			return;
	}
#endif
	if (TEXVALID(shaderstate.curtexnums->bump) && p->permu[perm|PERMUTATION_BUMPMAP].h.loaded)
		perm |= PERMUTATION_BUMPMAP;
	if (TEXVALID(shaderstate.curtexnums->fullbright) && p->permu[perm|PERMUTATION_FULLBRIGHT].h.loaded)
		perm |= PERMUTATION_FULLBRIGHT;
	if (p->permu[perm|PERMUTATION_UPPERLOWER].h.loaded && (TEXLOADED(shaderstate.curtexnums->upperoverlay) || TEXLOADED(shaderstate.curtexnums->loweroverlay)))
		perm |= PERMUTATION_UPPERLOWER;
	if (r_refdef.globalfog.density && p->permu[perm|PERMUTATION_FOG].h.loaded)
		perm |= PERMUTATION_FOG;
#ifdef NONSKELETALMODELS
	if (p->permu[perm|PERMUTATION_FRAMEBLEND].h.loaded && shaderstate.batchvbo && shaderstate.batchvbo->coord2.d3d.buff)
	{
		perm |= PERMUTATION_FRAMEBLEND;
		vdec |= D3D_VDEC_POS2;
	}
#endif
//	if (p->permu[perm|PERMUTATION_DELUXE].h.loaded && TEXVALID(shaderstate.curtexnums->bump) && shaderstate.curbatch->lightmap[0] >= 0 && lightmap[shaderstate.curbatch->lightmap[0]]->hasdeluxe)
//		perm |= PERMUTATION_DELUXE;
#if MAXRLIGHTMAPS > 1
	if (shaderstate.curbatch->lightmap[1] >= 0 && p->permu[perm|PERMUTATION_LIGHTSTYLES].h.loaded)
		perm |= PERMUTATION_LIGHTSTYLES;
#endif

	BE_ApplyUniforms(p, perm);


	BE_ApplyShaderBits(s->passes->shaderbits);

	/*activate tmus*/
	for (passno = 0; passno < s->numpasses; passno++)
	{
		SelectPassTexture(passno, s->passes+passno);
	}
	/*deactivate any extras*/
	for (; passno < shaderstate.lastpasscount; passno++)
	{
		BindTexture(passno, NULL);
		d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, passno, D3DTSS_COLOROP, D3DTOP_DISABLE));
		d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, passno, D3DTSS_ALPHAOP, D3DTOP_DISABLE));
	}
	shaderstate.lastpasscount = passno;

	/*colours*/
	if (vdec & D3D_VDEC_COL4B)
	{
		if (shaderstate.batchvbo)
		{
			//fixme
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_COL, shaderstate.batchvbo->colours[0].d3d.buff, shaderstate.batchvbo->colours[0].d3d.offs, sizeof(vbovdata_t)));
		}
		else
		{
			int mno,v;
			void *map;
			mesh_t *m;
			allocvertexbuffer(shaderstate.dynst_buff[0], shaderstate.dynst_size, &shaderstate.dynst_offs[0], &map, vertcount*sizeof(byte_vec4_t));
			for (mno = 0, vertcount = 0; mno < shaderstate.nummeshes; mno++)
			{
				byte_vec4_t *dest = (byte_vec4_t*)((char*)map+vertcount*sizeof(byte_vec4_t));
				m = shaderstate.meshlist[mno];
				if (m->colors4f_array[0])
				{
					for (v = 0; v < m->numvertexes; v++)
					{
						//fixme:
						dest[v][0] = bound(0, m->colors4f_array[0][v][0] * 255, 255);
						dest[v][1] = bound(0, m->colors4f_array[0][v][1] * 255, 255);
						dest[v][2] = bound(0, m->colors4f_array[0][v][2] * 255, 255);
						dest[v][3] = bound(0, m->colors4f_array[0][v][3] * 255, 255);
					}
				}
				else if (m->colors4b_array)
					memcpy(dest, m->colors4b_array, m->numvertexes*sizeof(byte_vec4_t));
				else
					memset(dest, 0, m->numvertexes*sizeof(byte_vec4_t));
				vertcount += m->numvertexes;
			}
			d3dcheck(IDirect3DVertexBuffer8_Unlock(shaderstate.dynst_buff[0]));
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_COL, shaderstate.dynst_buff[0], shaderstate.dynst_offs[0] - vertcount*sizeof(byte_vec4_t), sizeof(byte_vec4_t)));
		}
	}

	/*texture coords*/
	if (vdec & D3D_VDEC_ST0)
	{
		if (shaderstate.batchvbo)
		{
			if (shaderstate.batchvbo && !shaderstate.batchvbo->vaodynamic)
				d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_TC0, shaderstate.batchvbo->texcoord.d3d.buff, shaderstate.batchvbo->texcoord.d3d.offs, sizeof(vbovdata_t)));
			else
				d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_TC0, shaderstate.batchvbo->texcoord.d3d.buff, shaderstate.batchvbo->texcoord.d3d.offs, sizeof(vec2_t)));
		}
		else
		{
			int mno;
			void *map;
			mesh_t *m;
			allocvertexbuffer(shaderstate.dynst_buff[0], shaderstate.dynst_size, &shaderstate.dynst_offs[0], &map, vertcount*sizeof(vec2_t));
			for (mno = 0, vertcount = 0; mno < shaderstate.nummeshes; mno++)
			{
				vec2_t *dest = (vec2_t*)((char*)map+vertcount*sizeof(vec2_t));
				m = shaderstate.meshlist[mno];
				memcpy(dest, m->st_array, m->numvertexes*sizeof(vec2_t));
				vertcount += m->numvertexes;
			}
			d3dcheck(IDirect3DVertexBuffer8_Unlock(shaderstate.dynst_buff[0]));
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_TC0, shaderstate.dynst_buff[0], shaderstate.dynst_offs[0] - vertcount*sizeof(vec2_t), sizeof(vec2_t)));
		}
	}
	/*lm coords*/
	if (vdec & D3D_VDEC_ST1)
	{
		if (shaderstate.batchvbo)
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_TC1, shaderstate.batchvbo->lmcoord[0].d3d.buff, shaderstate.batchvbo->lmcoord[0].d3d.offs, sizeof(vbovdata_t)));
		else
		{
			int mno;
			void *map;
			mesh_t *m;
			allocvertexbuffer(shaderstate.dynst_buff[1], shaderstate.dynst_size, &shaderstate.dynst_offs[1], &map, vertcount*sizeof(vec2_t));
			for (mno = 0, vertcount = 0; mno < shaderstate.nummeshes; mno++)
			{
				vec2_t *dest = (vec2_t*)((char*)map+vertcount*sizeof(vec2_t));
				m = shaderstate.meshlist[mno];
				memcpy(dest, m->lmst_array, m->numvertexes*sizeof(vec2_t));
				vertcount += m->numvertexes;
			}
			d3dcheck(IDirect3DVertexBuffer8_Unlock(shaderstate.dynst_buff[1]));
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_TC1, shaderstate.dynst_buff[1], shaderstate.dynst_offs[1] - vertcount*sizeof(vec2_t), sizeof(vec2_t)));
		}
	}

	/*normals/tangents/bitangents*/
	if (vdec & D3D_VDEC_NORM)
	{
		if (shaderstate.batchvbo)
		{
			if (shaderstate.batchvbo && !shaderstate.batchvbo->vaodynamic)
			{
				d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_NORM, shaderstate.batchvbo->normals.d3d.buff, shaderstate.batchvbo->normals.d3d.offs, sizeof(vbovdata_t)));
				d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_NORMS, shaderstate.batchvbo->svector.d3d.buff, shaderstate.batchvbo->svector.d3d.offs, sizeof(vbovdata_t)));
				d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_NORMT, shaderstate.batchvbo->tvector.d3d.buff, shaderstate.batchvbo->tvector.d3d.offs, sizeof(vbovdata_t)));
			}
			else
			{
				d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_NORM, shaderstate.batchvbo->normals.d3d.buff, shaderstate.batchvbo->normals.d3d.offs, sizeof(vec3_t)));
				d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_NORMS, shaderstate.batchvbo->svector.d3d.buff, shaderstate.batchvbo->svector.d3d.offs, sizeof(vec3_t)));
				d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_NORMT, shaderstate.batchvbo->tvector.d3d.buff, shaderstate.batchvbo->tvector.d3d.offs, sizeof(vec3_t)));
			}
		}
		else if (shaderstate.meshlist[0]->normals_array && shaderstate.meshlist[0]->snormals_array && shaderstate.meshlist[0]->tnormals_array)
		{
			int mno;
			void *map;
			mesh_t *m;
			int tv = vertcount;

			allocvertexbuffer(shaderstate.dynnorm_buff, shaderstate.dynnorm_size, &shaderstate.dynnorm_offs, &map, vertcount*3*sizeof(vec3_t));
			for (mno = 0, vertcount = 0; mno < shaderstate.nummeshes; mno++)
			{
				float *dest;
				m = shaderstate.meshlist[mno];

				dest = (float*)((char*)map+vertcount*sizeof(vec3_t));
				memcpy(dest, m->normals_array, m->numvertexes*sizeof(vec3_t));

				dest += tv*3;
				memcpy(dest, m->snormals_array, m->numvertexes*sizeof(vec3_t));

				dest += tv*3;
				memcpy(dest, m->tnormals_array, m->numvertexes*sizeof(vec3_t));

				vertcount += m->numvertexes;
			}
			d3dcheck(IDirect3DVertexBuffer8_Unlock(shaderstate.dynnorm_buff));
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_NORM, shaderstate.dynnorm_buff, shaderstate.dynnorm_offs - vertcount*sizeof(vec3_t)*3, sizeof(vec3_t)));
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_NORMS, shaderstate.dynnorm_buff, shaderstate.dynnorm_offs - vertcount*sizeof(vec3_t)*2, sizeof(vec3_t)));
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_NORMT, shaderstate.dynnorm_buff, shaderstate.dynnorm_offs - vertcount*sizeof(vec3_t)*1, sizeof(vec3_t)));
		}
		else
			vdec &= ~D3D_VDEC_NORM;
	}

	/*bone weights+indexes*/
	if (vdec & D3D_VDEC_SKEL)
	{
		/*FIXME*/
		vdec &= ~D3D_VDEC_SKEL;
	}

	if (vdec != shaderstate.curvertdecl)
	{
		shaderstate.curvertdecl = vdec;
		d3dcheck(IDirect3DDevice8_SetVertexDeclaration(pD3DDev8, vertexdecls[shaderstate.curvertdecl]));
	}

//	IDirect3DDevice8_SetVertexShaderConstantF(pD3DDev8,
	BE_SubmitMeshChain(vertbase, vertfirst, vertcount, idxfirst, idxcount);

	IDirect3DDevice8_SetVertexShader(pD3DDev8, NULL);
	IDirect3DDevice8_SetPixelShader(pD3DDev8, NULL);
}
#endif

void D3D8BE_Cull(unsigned int cullflags)
{
	if (shaderstate.flags & BEF_FORCETWOSIDED)
		cullflags = 0;
	else if (cullflags)
		cullflags ^= r_refdef.flipcull;

	if (shaderstate.curcull != cullflags)
	{
		shaderstate.curcull = cullflags;
		if (shaderstate.curcull & 1)
		{
			if (shaderstate.curcull & SHADER_CULL_FRONT)
				IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_CULLMODE, D3DCULL_CW);
			else if (shaderstate.curcull & SHADER_CULL_BACK)
				IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_CULLMODE, D3DCULL_CCW);
			else
				IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_CULLMODE, D3DCULL_NONE);
		}
		else
		{
			if (shaderstate.curcull & SHADER_CULL_FRONT)
				IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_CULLMODE, D3DCULL_CCW);
			else if (shaderstate.curcull & SHADER_CULL_BACK)
				IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_CULLMODE, D3DCULL_CW);
			else
				IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_CULLMODE, D3DCULL_NONE);
		}
	}
}

static void BE_DrawMeshChain_Internal(void)
{
	unsigned int vertfirst, vertcount, idxcount, idxfirst;
	mesh_t *m;
	void *map;
	int i;
	unsigned int mno;
	unsigned int passno = 0;
	shaderpass_t *pass;
	shader_t *useshader = shaderstate.curshader;
	float pushdepth = shaderstate.curshader->polyoffset.factor;
//	float pushfactor;

#ifdef BEF_PUSHDEPTH
	if (shaderstate.flags & BEF_PUSHDEPTH)
	{
		extern cvar_t r_polygonoffset_submodel_factor;
		pushdepth += r_polygonoffset_submodel_factor.value;
	}
#endif
	pushdepth /= 0xffff;

	D3D8BE_Cull(shaderstate.curshader->flags & (SHADER_CULL_FRONT | SHADER_CULL_BACK));

#ifdef FIXME
	if (pushdepth != shaderstate.depthbias)
	{
		shaderstate.depthbias = pushdepth;
		IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_DEPTHBIAS, *(DWORD*)&shaderstate.depthbias);
	}
//	pushdepth = shaderstate.curshader->polyoffset.unit/-1;// + ((shaderstate.flags & BEF_PUSHDEPTH)?8:0);
//	pushfactor = shaderstate.curshader->polyoffset.factor/-1;
//	if (pushfactor != shaderstate.depthfactor)
//	{
//		shaderstate.depthfactor = pushfactor;
//		IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&shaderstate.depthfactor);
//	}
#endif

	switch (shaderstate.mode)
	{
	case BEM_LIGHT:
//		useshader = shaderstate.curshader->bemoverrides[shaderstate.lightmode];
//		if (!useshader)
			useshader = shaderstate.shader_rtlight;
		if (!useshader->prog)
			return;
		break;
	case BEM_DEPTHDARK:
//		useshader = shaderstate.shader_depthblack;
		return;
		break;
	case BEM_STENCIL:
		return;
	case BEM_DEPTHONLY:
//		useshader = shaderstate.shader_depthonly;
		return;
		break;
	default:
		useshader = shaderstate.curshader;
		break;	//no need to switch the shader
	}

	//if anything is dynamic ALL must be dynamic
	//might want to flag this for multi-mesh batches on pre-t&l cards too, so that there's no gaps.
//	if ((useshader->flags & SHADER_NEEDSARRAYS) && shaderstate.nummeshes > 0)
		shaderstate.batchvbo = NULL;

	if (shaderstate.batchvbo)
	{
		vertfirst = 0;
		vertcount = shaderstate.batchvbo->vertcount;
		idxfirst = 0;
		idxcount = shaderstate.batchvbo->indexcount;
	}
	else
	{
		vertfirst = 0;
		for (mno = 0, vertcount = 0, idxcount = 0; mno < shaderstate.nummeshes; mno++)
		{
			m = shaderstate.meshlist[mno];
			vertcount += m->numvertexes;
			idxcount += m->numindexes;
		}
	}

	/*vertex buffers are common to all passes*/
/*	if (shaderstate.batchvbo && !shaderstate.batchvbo->vaodynamic)
	{
		d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_VERT, shaderstate.batchvbo->coord.d3d.buff, sizeof(vbovdata_t)));
		vertfirst = 0;
	}
	else if (shaderstate.batchvbo)
	{
		d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_VERT, shaderstate.batchvbo->coord.d3d.buff, shaderstate.batchvbo->coord.d3d.offs, sizeof(vecV_t)));
		vertfirst = 0;
	}
	else
	{
		vertfirst = 0;
		allocvertexbuffer(shaderstate.dynxyz_buff, shaderstate.dynxyz_size, &shaderstate.dynxyz_offs, &map, vertcount*sizeof(vecV_t));
		for (mno = 0, vertcount = 0; mno < shaderstate.nummeshes; mno++)
		{
			vecV_t *dest = (vecV_t*)((char*)map+vertcount*sizeof(vecV_t));
			m = shaderstate.meshlist[mno];
			deformgen(&shaderstate.curshader->deforms[0], m->numvertexes, m->xyz_array, dest, m);
			for (i = 1; i < shaderstate.curshader->numdeforms; i++)
			{
				deformgen(&shaderstate.curshader->deforms[i], m->numvertexes, dest, dest, m);
			}
			vertcount += m->numvertexes;
		}
		d3dcheck(IDirect3DVertexBuffer8_Unlock(shaderstate.dynxyz_buff));
		d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_VERT, shaderstate.dynxyz_buff, shaderstate.dynxyz_offs - vertcount*sizeof(vecV_t), sizeof(vecV_t)));
		d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_VERT2, NULL, 0, sizeof(vecV_t)));
	}
*/
	/*index buffers are also common (note that we may still need to stream these when dealing with bsp geometry, to cope with gaps. this is faster than using multiple draw calls.)*/
	if (shaderstate.batchvbo)
	{
		if (shaderstate.nummeshes != 1 || (useshader->flags & SHADER_NEEDSARRAYS))
		{	//in this case, the vertex data is static, but the index data can have gaps.
			//we're streaming index buffer data only so that we can avoid repeated draw calls. if this stuff was properly built in the first place we wouldn't need to do this. :s
			idxcount = 0;
			for (mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				m = shaderstate.meshlist[mno];
				idxcount += m->numindexes;
			}
			idxfirst = allocindexbuffer(&map, idxcount);
			for (mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				m = shaderstate.meshlist[mno];
				for (i = 0; i < m->numindexes; i++)
					((index_t*)map)[i] = m->vbofirstvert + m->indexes[i];
				map = (char*)map + m->numindexes*sizeof(index_t);
			}
			d3dcheck(IDirect3DIndexBuffer8_Unlock(shaderstate.dynidx_buff));
			d3dcheck(IDirect3DDevice8_SetIndices(pD3DDev8, shaderstate.dynidx_buff, idxfirst));

			//we could constrain vertfirst+vertcount, but I suspect those only matter on pre t&l cards, of which there are very few.
		}
		else
		{
			m = shaderstate.meshlist[0];
			idxfirst = m->vbofirstelement;
			idxcount = m->numindexes;
			vertfirst = m->vbofirstvert;
			vertcount = m->numvertexes;

			d3dcheck(IDirect3DDevice8_SetIndices(pD3DDev8, shaderstate.batchvbo->indicies.d3d.buff, 0));
		}
	}
	else
	{
		idxfirst = allocindexbuffer(&map, idxcount);
		for (mno = 0, vertcount = 0; mno < shaderstate.nummeshes; mno++)
		{
			m = shaderstate.meshlist[mno];
			for (i = 0; i < m->numindexes; i++)
				((index_t*)map)[i] = m->indexes[i]+vertcount;
			map = (char*)map + m->numindexes*sizeof(index_t);
			vertcount += m->numvertexes;
		}
		d3dcheck(IDirect3DIndexBuffer8_Unlock(shaderstate.dynidx_buff));
	}

	switch (shaderstate.mode)
	{
#if 0
	case BEM_LIGHT:
		if (shaderstate.shader_rtlight->prog)
			BE_RenderMeshProgram(shaderstate.shader_rtlight, vertbase, vertfirst, vertcount, idxfirst, idxcount);
		break;
	case BEM_DEPTHDARK:
		shaderstate.lastpasscount = 0;
		i = 0;
		if (i != shaderstate.curvertdecl)
		{
			shaderstate.curvertdecl = i;
			d3dcheck(IDirect3DDevice8_SetVertexDeclaration(pD3DDev8, vertexdecls[shaderstate.curvertdecl]));
		}
		/*deactivate any extras*/
		for (passno = 1; passno < shaderstate.lastpasscount; )
		{
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_TC0+passno, NULL, 0, 0));
			BindTexture(passno, NULL);
			d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, passno, D3DTSS_COLOROP, D3DTOP_DISABLE));
			d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, passno, D3DTSS_ALPHAOP, D3DTOP_DISABLE));
			passno++;
		}
		BindTexture(passno, NULL);
		d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1));
		d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, 0, D3DTSS_COLORARG1, D3DTA_CONSTANT));
		d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, 0, D3DTSS_CONSTANT, D3DCOLOR_RGBA(0,0,0,255)));
		d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE));
		shaderstate.lastpasscount = 1;
		BE_ApplyShaderBits(SBITS_MISC_DEPTHWRITE);

		BE_SubmitMeshChain(vertbase, vertfirst, vertcount, idxfirst, idxcount);
		break;
	case BEM_STENCIL:
		break;
	case BEM_DEPTHONLY:
		shaderstate.lastpasscount = 0;
		i = 0;
		if (i != shaderstate.curvertdecl)
		{
			shaderstate.curvertdecl = i;
			d3dcheck(IDirect3DDevice8_SetVertexDeclaration(pD3DDev8, vertexdecls[shaderstate.curvertdecl]));
		}
		IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_COLORWRITEENABLE, 0);
		/*deactivate any extras*/
		for (passno = 0; passno < shaderstate.lastpasscount; )
		{
			d3dcheck(IDirect3DDevice8_SetStreamSource(pD3DDev8, STRM_TC0+passno, NULL, 0, 0));
			BindTexture(passno, NULL);
			d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, passno, D3DTSS_COLOROP, D3DTOP_DISABLE));
			d3dcheck(IDirect3DDevice8_SetTextureStageState(pD3DDev8, passno, D3DTSS_ALPHAOP, D3DTOP_DISABLE));
			passno++;
		}
		shaderstate.lastpasscount = 0;
		BE_ApplyShaderBits(SBITS_MISC_DEPTHWRITE);
		BE_SubmitMeshChain(vertbase, vertfirst, vertcount, idxfirst, idxcount);
		IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA);
		break;
#endif
	default:
		break;
	case BEM_STANDARD:
#ifdef FIXME
		if (useshader->prog)
		{
			BE_RenderMeshProgram(useshader, vertfirst, vertcount, idxfirst, idxcount);
		}
		else
#endif
		{
			/*now go through and flush each pass*/
			for (passno = 0, pass = useshader->passes; passno < shaderstate.curshader->numpasses; passno += pass->numMergedPasses)
			{
				if (!BE_DrawMeshChain_SetupPass(pass+passno, vertcount, &vertfirst))
					continue;
		#ifdef BENCH
				shaderstate.bench.draws++;
				if (shaderstate.bench.clamp && shaderstate.bench.clamp < shaderstate.bench.draws)
					continue;
		#endif
				BE_SubmitMeshChain(vertfirst, vertcount, idxfirst, idxcount);
//				d3dcheck(IDirect3DDevice8_DrawIndexedPrimitive(pD3DDev8, D3DPT_TRIANGLELIST, 0, 0, vertcount, idxfirst, idxcount/3));
			}
		}
		break;
	}
}

void D3D8BE_SelectMode(backendmode_t mode)
{
	shaderstate.mode = mode;

	if (mode == BEM_STENCIL)
		BE_ApplyShaderBits(SBITS_MASK_BITS);
}

qboolean D3D8BE_SelectDLight(dlight_t *dl, vec3_t colour, vec3_t axis[3], unsigned int lmode)
{
	shaderstate.curdlight = dl;
	VectorCopy(colour, shaderstate.curdlight_colours);

	if (lmode != LSHADER_STANDARD)
		return false;
	return true;
}

void D3D8BE_SelectEntity(entity_t *ent)
{
	shaderstate.curentity = ent;
	BE_RotateForEntity(ent, ent->model);
}

#if 1
static void D3D8BE_GenBatchVBOs(vbo_t **vbochain, batch_t *firstbatch, batch_t *stopbatch)
{
	int maxvboelements;
	int maxvboverts;
	int vert = 0, idx = 0;
	batch_t *batch;
	vbo_t *vbo;
	int i, j;
	mesh_t *m;
	IDirect3DVertexBuffer8 *vbuff;
	IDirect3DIndexBuffer8 *ebuff;
	index_t *vboedata;
	vbovdata_t *vbovdata;

	vbo = Z_Malloc(sizeof(*vbo));

	maxvboverts = 0;
	maxvboelements = 0;
	for(batch = firstbatch; batch != stopbatch; batch = batch->next)
	{
		for (i=0 ; i<batch->maxmeshes ; i++)
		{
			m = batch->mesh[i];
			maxvboelements += m->numindexes;
			maxvboverts += m->numvertexes;
		}
	}

	IDirect3DDevice8_CreateIndexBuffer(pD3DDev8, sizeof(index_t) * maxvboelements, 0,					D3DFMT_QINDEX, D3DPOOL_MANAGED, &ebuff);
	IDirect3DDevice8_CreateVertexBuffer(pD3DDev8, sizeof(*vbovdata) * maxvboverts, D3DUSAGE_WRITEONLY,	D3DFVF_QVBO, D3DPOOL_MANAGED, &vbuff);

	vbovdata = NULL;
	vbo->vaodynamic = 0;
	vbo->coord.d3d.buff = vbuff;
	vbo->coord.d3d.offs = (quintptr_t)&vbovdata->coord;
	vbo->indicies.d3d.buff = ebuff;
	vbo->indicies.d3d.offs = 0;

	IDirect3DIndexBuffer8_Lock(ebuff, 0, sizeof(index_t) * maxvboelements, (BYTE**)&vboedata, D3DLOCK_DISCARD);
	IDirect3DVertexBuffer8_Lock(vbuff, 0, sizeof(*vbovdata) * maxvboverts, (BYTE**)&vbovdata, D3DLOCK_DISCARD);

	for(batch = firstbatch; batch != stopbatch; batch = batch->next)
	{
		batch->vbo = vbo;
		for (j=0 ; j<batch->maxmeshes ; j++)
		{
			m = batch->mesh[j];
			m->vbofirstvert = vert;
			for (i = 0; i < m->numvertexes; i++)
			{
				VectorCopy(m->xyz_array[i],			vbovdata->coord);
				//vbovdata->coord[3] = 1;
				Vector2Copy(m->st_array[i],			vbovdata->tc[0]);
				Vector2Copy(m->lmst_array[0][i],		vbovdata->tc[1]);
				Vector4Scale(m->colors4f_array[0][i],	255, vbovdata->colorsb);

				vbovdata++;
			}

			m->vbofirstelement = idx;
			for (i = 0; i < m->numindexes; i++)
			{
				*vboedata++ = vert + m->indexes[i];
			}
			idx += m->numindexes;
			vert += m->numvertexes;
		}
	}

	vbo->indexcount = idx;
	vbo->vertcount = vert;

	IDirect3DIndexBuffer8_Unlock(ebuff);
	IDirect3DVertexBuffer8_Unlock(vbuff);

	vbo->next = *vbochain;
	*vbochain = vbo;
}

void D3D8BE_GenBrushModelVBO(model_t *mod)
{
	unsigned int vcount;


	batch_t *batch, *fbatch;
	int sortid;
	int i;

	fbatch = NULL;
	vcount = 0;
	for (sortid = 0; sortid < SHADER_SORT_COUNT; sortid++)
	{
		if (!mod->batches[sortid])
			continue;

		for (fbatch = batch = mod->batches[sortid]; batch != NULL; batch = batch->next)
		{
			//firstmesh got reused as the number of verticies in each batch
			if (vcount + batch->firstmesh > MAX_INDICIES)
			{
				D3D8BE_GenBatchVBOs(&mod->vbos, fbatch, batch);
				fbatch = batch;
				vcount = 0;
			}

			for (i = 0; i < batch->maxmeshes; i++)
				vcount += batch->mesh[i]->numvertexes;
		}

		D3D8BE_GenBatchVBOs(&mod->vbos, fbatch, batch);
	}
}
#else
/*Generates an optimised vbo for each of the given model's textures*/
void D3D8BE_GenBrushModelVBO(model_t *mod)
{
#if 1
	unsigned int maxvboverts;
	unsigned int maxvboelements;

	unsigned int t;
	unsigned int i;
	unsigned int v;
	unsigned int vcount, ecount;
	unsigned int pervertsize;	//erm, that name wasn't intentional
	unsigned int meshes;

	vbo_t *vbo;
	index_t *vboedata;
	mesh_t *m;
	char *vbovdata;
	IDirect3DVertexBuffer8 *vbuff;
	IDirect3DIndexBuffer8 *ebuff;

	vecV_t *coord;
	vec2_t *texcoord;
	vec2_t *lmcoord;
	vec3_t *normals;
	vec3_t *svector;
	vec3_t *tvector;
	byte_vec4_t *colours;

	if (!mod->numsurfaces)
		return;

	for (t = 0; t < mod->numtextures; t++)
	{
		if (!mod->textures[t])
			continue;
		vbo = &mod->textures[t]->vbo;
		BE_ClearVBO(vbo);

		maxvboverts = 0;
		maxvboelements = 0;
		meshes = 0;
		for (i=0 ; i<mod->numsurfaces ; i++)
		{
			if (mod->surfaces[i].texinfo->texture != mod->textures[t])
				continue;
			m = mod->surfaces[i].mesh;
			if (!m)
				continue;

			meshes++;
			maxvboelements += m->numindexes;
			maxvboverts += m->numvertexes;
		}
#if sizeof_index_t == 2
		if (maxvboverts > (1u<<(sizeof(index_t)*8))-1)
			continue;
#endif
		if (!maxvboverts)
			continue;

		//fixme: stop this from leaking!
		vcount = 0;
		ecount = 0;

		pervertsize =	sizeof(vecV_t)+	//coord
					sizeof(vec2_t)+	//tex
					sizeof(vec2_t)+	//lm
					sizeof(vec3_t)+	//normal
					sizeof(vec3_t)+	//sdir
					sizeof(vec3_t)+	//tdir
					sizeof(vec4_t);	//colours

		IDirect3DDevice8_CreateIndexBuffer(pD3DDev8, sizeof(index_t) * maxvboelements, 0, D3DFMT_QINDEX, D3DPOOL_MANAGED, &ebuff, NULL);
		IDirect3DDevice8_CreateVertexBuffer(pD3DDev8, pervertsize * maxvboverts, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &vbuff, NULL);

		IDirect3DIndexBuffer8_Lock(ebuff, 0, sizeof(index_t) * maxvboelements, &vboedata, D3DLOCK_DISCARD);
		IDirect3DVertexBuffer8_Lock(vbuff, 0, pervertsize * maxvboverts, &vbovdata, D3DLOCK_DISCARD);

		vbo->vertdata = vbuff;
		vbo->indexcount = maxvboelements;
		vbo->vertcount = maxvboverts;

		vbo->coord.d3d.buff = vbuff;
		vbo->coord.d3d.offs = 0;
		vbo->texcoord.d3d.buff = vbuff;
		vbo->texcoord.d3d.offs = vbo->coord.d3d.offs+maxvboverts*sizeof(vecV_t);
		vbo->lmcoord.d3d.buff = vbuff;
		vbo->lmcoord.d3d.offs = vbo->texcoord.d3d.offs+maxvboverts*sizeof(vec2_t);
		vbo->normals.d3d.buff = vbuff;
		vbo->normals.d3d.offs = vbo->lmcoord.d3d.offs+maxvboverts*sizeof(vec2_t);
		vbo->svector.d3d.buff = vbuff;
		vbo->svector.d3d.offs = vbo->normals.d3d.offs+maxvboverts*sizeof(vec3_t);
		vbo->tvector.d3d.buff = vbuff;
		vbo->tvector.d3d.offs = vbo->svector.d3d.offs+maxvboverts*sizeof(vec3_t);
		vbo->colours.d3d.buff = vbuff;
		vbo->colours.d3d.offs = vbo->tvector.d3d.offs+maxvboverts*sizeof(vec3_t);
		vbo->indicies.d3d.buff = ebuff;
		vbo->indicies.d3d.offs = 0;

		coord = (void*)(vbovdata + vbo->coord.d3d.offs);
		texcoord = (void*)(vbovdata + vbo->texcoord.d3d.offs);
		lmcoord = (void*)(vbovdata + vbo->lmcoord.d3d.offs);
		normals = (void*)(vbovdata + vbo->normals.d3d.offs);
		svector = (void*)(vbovdata + vbo->svector.d3d.offs);
		tvector = (void*)(vbovdata + vbo->tvector.d3d.offs);
		colours = (void*)(vbovdata + vbo->colours.d3d.offs);

		vbo->meshcount = meshes;
		vbo->meshlist = BZ_Malloc(meshes*sizeof(*vbo->meshlist));

		meshes = 0;

		for (i=0 ; i<mod->numsurfaces ; i++)
		{
			if (mod->surfaces[i].texinfo->texture != mod->textures[t])
				continue;
			m = mod->surfaces[i].mesh;
			if (!m)
				continue;

			mod->surfaces[i].mark = &vbo->meshlist[meshes++];
			*mod->surfaces[i].mark = NULL;

			m->vbofirstvert = vcount;
			m->vbofirstelement = ecount;
			for (v = 0; v < m->numindexes; v++)
				vboedata[ecount++] = vcount + m->indexes[v];
			for (v = 0; v < m->numvertexes; v++)
			{
				coord[vcount+v][0] = m->xyz_array[v][0];
				coord[vcount+v][1] = m->xyz_array[v][1];
				coord[vcount+v][2] = m->xyz_array[v][2];
				coord[vcount+v][3] = 1;
				if (m->st_array)
				{
					texcoord[vcount+v][0] = m->st_array[v][0];
					texcoord[vcount+v][1] = m->st_array[v][1];
				}
				if (m->lmst_array)
				{
					lmcoord[vcount+v][0] = m->lmst_array[v][0];
					lmcoord[vcount+v][1] = m->lmst_array[v][1];
				}
				if (m->normals_array)
				{
					normals[vcount+v][0] = m->normals_array[v][0];
					normals[vcount+v][1] = m->normals_array[v][1];
					normals[vcount+v][2] = m->normals_array[v][2];
				}
				if (m->snormals_array)
				{
					svector[vcount+v][0] = m->snormals_array[v][0];
					svector[vcount+v][1] = m->snormals_array[v][1];
					svector[vcount+v][2] = m->snormals_array[v][2];
				}
				if (m->tnormals_array)
				{
					tvector[vcount+v][0] = m->tnormals_array[v][0];
					tvector[vcount+v][1] = m->tnormals_array[v][1];
					tvector[vcount+v][2] = m->tnormals_array[v][2];
				}
				if (m->colors4b_array)
				{
					colours[vcount+v][0] = m->colors4b_array[v][0];
					colours[vcount+v][1] = m->colors4b_array[v][1];
					colours[vcount+v][2] = m->colors4b_array[v][2];
					colours[vcount+v][3] = m->colors4b_array[v][3];
				}
				if (m->colors4f_array[0])
				{
					colours[vcount+v][0] = m->colors4f_array[0][v][0] * 255;
					colours[vcount+v][1] = m->colors4f_array[0][v][1] * 255;
					colours[vcount+v][2] = m->colors4f_array[0][v][2] * 255;
					colours[vcount+v][3] = m->colors4f_array[0][v][3] * 255;
				}
			}
			vcount += v;
		}

		IDirect3DIndexBuffer8_Unlock(ebuff);
		IDirect3DVertexBuffer8_Unlock(vbuff);
	}
#endif
}
#endif
/*Wipes a vbo*/
void D3D8BE_ClearVBO(vbo_t *vbo, qboolean dataonly)
{
	IDirect3DVertexBuffer8 *vbuff = vbo->coord.d3d.buff;
	IDirect3DIndexBuffer8 *ebuff = vbo->indicies.d3d.buff;
	if (vbuff)
		IDirect3DVertexBuffer8_Release(vbuff);
	if (ebuff)
		IDirect3DIndexBuffer8_Release(ebuff);
	vbo->coord.d3d.buff = NULL;
	vbo->indicies.d3d.buff = NULL;

	if (!dataonly)
		free(vbo);
}

/*upload all lightmaps at the start to reduce lags*/
static void BE_UploadLightmaps(qboolean force)
{
	int i;
	lightmapinfo_t *lm;

	for (i = 0; i < numlightmaps; i++)
	{
		lm = lightmap[i];
		if (!lm)
			continue;

		if (force)
		{
			lm->rectchange.l = 0;
			lm->rectchange.t = 0;
			lm->rectchange.r = lm->width;
			lm->rectchange.b = lm->height;
		}

		if (lightmap[i]->modified)
		{
			extern cvar_t r_lightmap_nearest;
			IDirect3DTexture8 *tex;
			D3DLOCKED_RECT lock;
			RECT rect;
			glRect_t *theRect = &lm->rectchange;
			int r;
			int w;

			if (!TEXLOADED(lm->lightmap_texture))
				lm->lightmap_texture = Image_CreateTexture("***lightmap***", NULL, (r_lightmap_nearest.ival?IF_NEAREST:IF_LINEAR)|IF_NOMIPMAP);
			tex = lm->lightmap_texture->ptr;
			if (lm->fmt != PTI_BGRA8)
				continue;	//erk!
			if (!tex)
			{
				IDirect3DDevice8_CreateTexture(pD3DDev8, lm->width, lm->height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex);
				if (!tex)
					continue;
				lm->lightmap_texture->ptr = tex;
				lm->lightmap_texture->status = TEX_LOADED;
			}
			
			lm->modified = 0;
			rect.left = theRect->l;
			rect.right = theRect->r;
			rect.top = theRect->t;
			rect.bottom = theRect->b;

			IDirect3DTexture8_LockRect(tex, 0, &lock, &rect, 0);
			for (r = 0, w = theRect->r-theRect->l; r < lightmap[i]->rectchange.b-lightmap[i]->rectchange.t; r++)
			{
				memcpy((char*)lock.pBits + r*lock.Pitch, lightmap[i]->lightmaps+(theRect->l+((r+theRect->t)*lm->width))*4, w*4);
			}
			IDirect3DTexture8_UnlockRect(tex, 0);
			theRect->l = lm->width;
			theRect->t = lm->height;
			theRect->r = 0;
			theRect->b = 0;
		}
	}
}

void D3D8BE_UploadAllLightmaps(void)
{
	BE_UploadLightmaps(true);
}

qboolean D3D8BE_LightCullModel(vec3_t org, model_t *model)
{
#ifdef RTLIGHTS
	if ((shaderstate.mode == BEM_LIGHT || shaderstate.mode == BEM_STENCIL))
	{
		/*true if hidden from current light*/
		/*we have no rtlight support, so mneh*/
	}
#endif
	return false;
}

batch_t *D3D8BE_GetTempBatch(void)
{
	if (shaderstate.wbatch >= shaderstate.maxwbatches)
	{
		shaderstate.wbatch++;
		return NULL;
	}
	return &shaderstate.wbatches[shaderstate.wbatch++];
}

static void BE_RotateForEntity (const entity_t *e, const model_t *mod)
{
//	float mv[16];
	float *m = shaderstate.m_model;

	shaderstate.curentity = e;

	if ((e->flags & RF_WEAPONMODEL) && r_refdef.playerview->viewentity > 0)
	{
		float em[16];
		float vm[16];

		if (e->flags & RF_WEAPONMODELNOBOB)
		{
			vm[0] = vpn[0];
			vm[1] = vpn[1];
			vm[2] = vpn[2];
			vm[3] = 0;

			vm[4] = -vright[0];
			vm[5] = -vright[1];
			vm[6] = -vright[2];
			vm[7] = 0;

			vm[8] = vup[0];
			vm[9] = vup[1];
			vm[10] = vup[2];
			vm[11] = 0;

			vm[12] = r_refdef.vieworg[0];
			vm[13] = r_refdef.vieworg[1];
			vm[14] = r_refdef.vieworg[2];
			vm[15] = 1;
		}
		else
		{
			vm[0] = r_refdef.playerview->vw_axis[0][0];
			vm[1] = r_refdef.playerview->vw_axis[0][1];
			vm[2] = r_refdef.playerview->vw_axis[0][2];
			vm[3] = 0;

			vm[4] = r_refdef.playerview->vw_axis[1][0];
			vm[5] = r_refdef.playerview->vw_axis[1][1];
			vm[6] = r_refdef.playerview->vw_axis[1][2];
			vm[7] = 0;

			vm[8] = r_refdef.playerview->vw_axis[2][0];
			vm[9] = r_refdef.playerview->vw_axis[2][1];
			vm[10] = r_refdef.playerview->vw_axis[2][2];
			vm[11] = 0;

			vm[12] = r_refdef.playerview->vw_origin[0];
			vm[13] = r_refdef.playerview->vw_origin[1];
			vm[14] = r_refdef.playerview->vw_origin[2];
			vm[15] = 1;
		}

		em[0] = e->axis[0][0];
		em[1] = e->axis[0][1];
		em[2] = e->axis[0][2];
		em[3] = 0;

		em[4] = e->axis[1][0];
		em[5] = e->axis[1][1];
		em[6] = e->axis[1][2];
		em[7] = 0;

		em[8] = e->axis[2][0];
		em[9] = e->axis[2][1];
		em[10] = e->axis[2][2];
		em[11] = 0;

		em[12] = e->origin[0];
		em[13] = e->origin[1];
		em[14] = e->origin[2];
		em[15] = 1;

		Matrix4_Multiply(vm, em, m);
	}
	else
	{
		m[0] = e->axis[0][0];
		m[1] = e->axis[0][1];
		m[2] = e->axis[0][2];
		m[3] = 0;

		m[4] = e->axis[1][0];
		m[5] = e->axis[1][1];
		m[6] = e->axis[1][2];
		m[7] = 0;

		m[8] = e->axis[2][0];
		m[9] = e->axis[2][1];
		m[10] = e->axis[2][2];
		m[11] = 0;

		m[12] = e->origin[0];
		m[13] = e->origin[1];
		m[14] = e->origin[2];
		m[15] = 1;
	}

	if (e->scale != 1 && e->scale != 0)	//hexen 2 stuff
	{
#ifdef HEXEN2
		float z;
		float escale;
		escale = e->scale;
		switch(e->drawflags&SCALE_TYPE_MASK)
		{
		default:
		case SCALE_TYPE_UNIFORM:
			VectorScale((m+0), escale, (m+0));
			VectorScale((m+4), escale, (m+4));
			VectorScale((m+8), escale, (m+8));
			break;
		case SCALE_TYPE_XYONLY:
			VectorScale((m+0), escale, (m+0));
			VectorScale((m+4), escale, (m+4));
			break;
		case SCALE_TYPE_ZONLY:
			VectorScale((m+8), escale, (m+8));
			break;
		}
		if (mod && (e->drawflags&SCALE_TYPE_MASK) != SCALE_TYPE_XYONLY)
		{
			switch(e->drawflags&SCALE_ORIGIN_MASK)
			{
			case SCALE_ORIGIN_CENTER:
				z = ((mod->maxs[2] + mod->mins[2]) * (1-escale))/2;
				VectorMA((m+12), z, e->axis[2], (m+12));
				break;
			case SCALE_ORIGIN_BOTTOM:
				VectorMA((m+12), mod->mins[2]*(1-escale), e->axis[2], (m+12));
				break;
			case SCALE_ORIGIN_TOP:
				VectorMA((m+12), -mod->maxs[2], e->axis[2], (m+12));
				break;
			}
		}
#else
		VectorScale((m+0), e->scale, (m+0));
		VectorScale((m+4), e->scale, (m+4));
		VectorScale((m+8), e->scale, (m+8));
#endif
	}
	else if (mod && !strcmp(mod->name, "progs/eyes.mdl"))
	{
		/*resize eyes, to make them easier to see*/
		m[14] -= (22 + 8);
		VectorScale((m+0), 2, (m+0));
		VectorScale((m+4), 2, (m+4));
		VectorScale((m+8), 2, (m+8));
	}
	if (mod && !ruleset_allow_larger_models.ival && mod->clampscale != 1)
	{	//possibly this should be on a per-frame basis, but that's a real pain to do
		Con_DPrintf("Rescaling %s by %f\n", mod->name, mod->clampscale);
		VectorScale((m+0), mod->clampscale, (m+0));
		VectorScale((m+4), mod->clampscale, (m+4));
		VectorScale((m+8), mod->clampscale, (m+8));
	}

	IDirect3DDevice8_SetTransform(pD3DDev8, D3DTS_WORLD, (D3DMATRIX*)m);

	{
	D3DVIEWPORT8 vport;
	IDirect3DDevice8_GetViewport(pD3DDev8, &vport);
	vport.MaxZ = (e->flags & RF_DEPTHHACK)?0.333:1;
	IDirect3DDevice8_SetViewport(pD3DDev8, &vport);
	}
}

void D3D8BE_SubmitBatch(batch_t *batch)
{
	shader_t *shader = batch->shader;
	shaderstate.nummeshes = batch->meshes - batch->firstmesh;
	if (!shaderstate.nummeshes)
		return;
	if (shaderstate.curentity != batch->ent)
	{
		BE_RotateForEntity(batch->ent, batch->ent->model);
		shaderstate.curtime = r_refdef.time - shaderstate.curentity->shaderTime;
	}
	shaderstate.batchvbo = batch->vbo;
	shaderstate.meshlist = batch->mesh + batch->firstmesh;
	shaderstate.curshader = shader;
	if (batch->skin)
		shaderstate.curtexnums = batch->skin;
	else if (shader->numdefaulttextures)
		shaderstate.curtexnums = shader->defaulttextures + ((int)(shader->defaulttextures_fps * shaderstate.curtime) % shader->numdefaulttextures);
	else
		shaderstate.curtexnums = shader->defaulttextures;
	shaderstate.curbatch = batch;
	shaderstate.flags = batch->flags;
	if ((unsigned)batch->lightmap[0] < (unsigned)numlightmaps)
		shaderstate.curlightmap = lightmap[batch->lightmap[0]]->lightmap_texture;
	else
		shaderstate.curlightmap = r_nulltex;

	BE_DrawMeshChain_Internal();
}

void D3D8BE_DrawMesh_List(shader_t *shader, int nummeshes, mesh_t **meshlist, vbo_t *vbo, texnums_t *texnums, unsigned int beflags)
{
	shaderstate.batchvbo = vbo;
	shaderstate.curshader = shader;
	if (texnums)
		shaderstate.curtexnums = texnums;
	else if (shader->numdefaulttextures)
		shaderstate.curtexnums = shader->defaulttextures + ((int)(shader->defaulttextures_fps * shaderstate.curtime) % shader->numdefaulttextures);
	else
		shaderstate.curtexnums = shader->defaulttextures;
	shaderstate.curlightmap = r_nulltex;
	shaderstate.curbatch = &shaderstate.dummybatch;
	shaderstate.meshlist = meshlist;
	shaderstate.nummeshes = nummeshes;
	shaderstate.flags = beflags;

	BE_DrawMeshChain_Internal();
}

void D3D8BE_DrawMesh_Single(shader_t *shader, mesh_t *meshchain, vbo_t *vbo, unsigned int beflags)
{
	shaderstate.batchvbo = vbo;
	shaderstate.curtime = realtime;
	shaderstate.curshader = shader;
	if (shader->numdefaulttextures)
		shaderstate.curtexnums = shader->defaulttextures + ((int)(shader->defaulttextures_fps * shaderstate.curtime) % shader->numdefaulttextures);
	else
		shaderstate.curtexnums = shader->defaulttextures;
	shaderstate.curlightmap = r_nulltex;
	shaderstate.meshlist = &meshchain;
	shaderstate.nummeshes = 1;
	shaderstate.flags = beflags;

	BE_DrawMeshChain_Internal();
}

static void BE_SubmitMeshesSortList(batch_t *sortlist)
{
	batch_t *batch;
	for (batch = sortlist; batch; batch = batch->next)
	{
		if (batch->meshes == batch->firstmesh)
			continue;

		if (batch->buildmeshes)
			batch->buildmeshes(batch);

		if (batch->shader->flags & SHADER_NODLIGHT)
			if (shaderstate.mode == BEM_LIGHT)
				continue;

		if (batch->shader->flags & SHADER_SKY)
		{
			if (!batch->shader->prog)
			{
				if (shaderstate.mode == BEM_STANDARD)
					R_DrawSkyChain (batch);
				continue;
			}
		}

		BE_SubmitBatch(batch);
	}
}


/*generates a new modelview matrix, as well as vpn vectors*/
static void R_MirrorMatrix(plane_t *plane)
{
	float mirror[16];
	float view[16];
	float result[16];

	vec3_t pnorm;
	VectorNegate(plane->normal, pnorm);

	mirror[0] = 1-2*pnorm[0]*pnorm[0];
	mirror[1] = -2*pnorm[0]*pnorm[1];
	mirror[2] = -2*pnorm[0]*pnorm[2];
	mirror[3] = 0;

	mirror[4] = -2*pnorm[1]*pnorm[0];
	mirror[5] = 1-2*pnorm[1]*pnorm[1];
	mirror[6] = -2*pnorm[1]*pnorm[2] ;
	mirror[7] = 0;

	mirror[8]  = -2*pnorm[2]*pnorm[0];
	mirror[9]  = -2*pnorm[2]*pnorm[1];
	mirror[10] = 1-2*pnorm[2]*pnorm[2];
	mirror[11] = 0;

	mirror[12] = -2*pnorm[0]*plane->dist;
	mirror[13] = -2*pnorm[1]*plane->dist;
	mirror[14] = -2*pnorm[2]*plane->dist;
	mirror[15] = 1;

	view[0] = vpn[0];
	view[1] = vpn[1];
	view[2] = vpn[2];
	view[3] = 0;

	view[4] = -vright[0];
	view[5] = -vright[1];
	view[6] = -vright[2];
	view[7] = 0;

	view[8]  = vup[0];
	view[9]  = vup[1];
	view[10] = vup[2];
	view[11] = 0;

	view[12] = r_refdef.vieworg[0];
	view[13] = r_refdef.vieworg[1];
	view[14] = r_refdef.vieworg[2];
	view[15] = 1;

	VectorMA(r_refdef.vieworg, 0.25, plane->normal, r_refdef.pvsorigin);

	Matrix4_Multiply(mirror, view, result);

	vpn[0] = result[0];
	vpn[1] = result[1];
	vpn[2] = result[2];

	vright[0] = -result[4];
	vright[1] = -result[5];
	vright[2] = -result[6];

	vup[0] = result[8];
	vup[1] = result[9];
	vup[2] = result[10];

	r_refdef.vieworg[0] = result[12];
	r_refdef.vieworg[1] = result[13];
	r_refdef.vieworg[2] = result[14];
}
static entity_t *R_NearestPortal(plane_t *plane)
{
	int i;
	entity_t *best = NULL;
	float dist, bestd = 0;
	for (i = 0; i < cl_numvisedicts; i++)
	{
		if (cl_visedicts[i].rtype == RT_PORTALSURFACE)
		{
			dist = DotProduct(cl_visedicts[i].origin, plane->normal)-plane->dist;
			dist = fabs(dist);
			if (dist < 64 && (!best || dist < bestd))
				best = &cl_visedicts[i];
		}
	}
	return best;
}

static void TransformCoord(vec3_t in, vec3_t planea[3], vec3_t planeo, vec3_t viewa[3], vec3_t viewo, vec3_t result)
{
	int		i;
	vec3_t	local;
	vec3_t	transformed;
	float	d;

	local[0] = in[0] - planeo[0];
	local[1] = in[1] - planeo[1];
	local[2] = in[2] - planeo[2];

	VectorClear(transformed);
	for ( i = 0 ; i < 3 ; i++ )
	{
		d = DotProduct(local, planea[i]);
		VectorMA(transformed, d, viewa[i], transformed);
	}

	result[0] = transformed[0] + viewo[0];
	result[1] = transformed[1] + viewo[1];
	result[2] = transformed[2] + viewo[2];
}
static void TransformDir(vec3_t in, vec3_t planea[3], vec3_t viewa[3], vec3_t result)
{
	int		i;
	float	d;
	vec3_t tmp;

	VectorCopy(in, tmp);

	VectorClear(result);
	for ( i = 0 ; i < 3 ; i++ )
	{
		d = DotProduct(tmp, planea[i]);
		VectorMA(result, d, viewa[i], result);
	}
}
static void R_RenderScene(void)
{
	IDirect3DDevice8_SetTransform(pD3DDev8, D3DTS_PROJECTION, (D3DMATRIX*)d3d_trueprojection);
	IDirect3DDevice8_SetTransform(pD3DDev8, D3DTS_VIEW, (D3DMATRIX*)r_refdef.m_view);
	R_SetFrustum (r_refdef.m_projection_std, r_refdef.m_view);
	Surf_DrawWorld();
}

static void R_DrawPortal(batch_t *batch, batch_t **blist)
{
	entity_t *view;
	float glplane[4];
	plane_t plane;
	refdef_t oldrefdef;
	mesh_t *mesh = batch->mesh[batch->firstmesh];
	int sort;

	if (r_refdef.recurse)
		return;

	VectorCopy(mesh->normals_array[0], plane.normal);
	plane.dist = DotProduct(mesh->xyz_array[0], plane.normal);

	//if we're too far away from the surface, don't draw anything
	if (batch->shader->flags & SHADER_AGEN_PORTAL)
	{
		/*there's a portal alpha blend on that surface, that fades out after this distance*/
		if (DotProduct(r_refdef.vieworg, plane.normal)-plane.dist > batch->shader->portaldist)
			return;
	}
	//if we're behind it, then also don't draw anything.
	if (DotProduct(r_refdef.vieworg, plane.normal)-plane.dist < 0)
		return;

	view = R_NearestPortal(&plane);
	//if (!view)
	//	return;

	oldrefdef = r_refdef;
	r_refdef.recurse = true;

	r_refdef.externalview = true;

	if (!view || VectorCompare(view->origin, view->oldorigin))
	{
		r_refdef.flipcull ^= SHADER_CULL_FLIP;
		R_MirrorMatrix(&plane);
	}
	else
	{
		float d;
		vec3_t paxis[3], porigin, vaxis[3], vorg;
		void PerpendicularVector( vec3_t dst, const vec3_t src );

		/*calculate where the surface is meant to be*/
		VectorCopy(mesh->normals_array[0], paxis[0]);
		PerpendicularVector(paxis[1], paxis[0]);
		CrossProduct(paxis[0], paxis[1], paxis[2]);
		d = DotProduct(view->origin, plane.normal) - plane.dist;
		VectorMA(view->origin, -d, paxis[0], porigin);

		/*grab the camera origin*/
		VectorNegate(view->axis[0], vaxis[0]);
		VectorNegate(view->axis[1], vaxis[1]);
		VectorCopy(view->axis[2], vaxis[2]);
		VectorCopy(view->oldorigin, vorg);

		VectorCopy(vorg, r_refdef.pvsorigin);

		/*rotate it a bit*/
		RotatePointAroundVector(vaxis[1], vaxis[0], view->axis[1], sin(realtime)*4);
		CrossProduct(vaxis[0], vaxis[1], vaxis[2]);

		TransformCoord(oldrefdef.vieworg, paxis, porigin, vaxis, vorg, r_refdef.vieworg);
		TransformDir(vpn, paxis, vaxis, vpn);
		TransformDir(vright, paxis, vaxis, vright);
		TransformDir(vup, paxis, vaxis, vup);
	}
	Matrix4x4_CM_ModelViewMatrixFromAxis(r_refdef.m_view, vpn, vright, vup, r_refdef.vieworg);
	VectorAngles(vpn, vup, r_refdef.viewangles, true);
	VectorCopy(r_refdef.vieworg, r_origin);

/*FIXME: the batch stuff should be done in renderscene*/

	/*fixup the first mesh index*/
	for (sort = 0; sort < SHADER_SORT_COUNT; sort++)
	for (batch = blist[sort]; batch; batch = batch->next)
	{
		batch->firstmesh = batch->meshes;
	}

	/*FIXME: can we get away with stenciling the screen?*/
	/*Add to frustum culling instead of clip planes?*/
	glplane[0] = plane.normal[0];
	glplane[1] = plane.normal[1];
	glplane[2] = plane.normal[2];
	glplane[3] = -plane.dist;
#ifndef _XBOX
	IDirect3DDevice8_SetClipPlane(pD3DDev8, 0, glplane);
	IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0);
#endif
	Surf_SetupFrame();
	R_RenderScene();

#ifndef _XBOX
	IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_CLIPPLANEENABLE, 0);
#endif
	for (sort = 0; sort < SHADER_SORT_COUNT; sort++)
	for (batch = blist[sort]; batch; batch = batch->next)
	{
		batch->firstmesh = 0;
	}
	r_refdef = oldrefdef;

	/*broken stuff*/
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
	VectorCopy (r_refdef.vieworg, r_origin);

	IDirect3DDevice8_SetTransform(pD3DDev8, D3DTS_PROJECTION, (D3DMATRIX*)d3d_trueprojection);
	IDirect3DDevice8_SetTransform(pD3DDev8, D3DTS_VIEW, (D3DMATRIX*)r_refdef.m_view);
	R_SetFrustum (r_refdef.m_projection_std, r_refdef.m_view);
}

static void BE_SubmitMeshesPortals(batch_t **worldlist, batch_t *dynamiclist)
{
	batch_t *batch, *old;
	int i;
	/*attempt to draw portal shaders*/
	if (shaderstate.mode == BEM_STANDARD)
	{
		for (i = 0; i < 2; i++)
		{
			for (batch = i?dynamiclist:worldlist[SHADER_SORT_PORTAL]; batch; batch = batch->next)
			{
				if (batch->meshes == batch->firstmesh)
					continue;

				if (batch->buildmeshes)
					batch->buildmeshes(batch);

				/*draw already-drawn portals as depth-only, to ensure that their contents are not harmed*/
				BE_SelectMode(BEM_DEPTHONLY);
				for (old = worldlist[SHADER_SORT_PORTAL]; old && old != batch; old = old->next)
				{
					if (old->meshes == old->firstmesh)
						continue;
					BE_SubmitBatch(old);
				}
				if (!old)
				{
					for (old = dynamiclist; old != batch; old = old->next)
					{
						if (old->meshes == old->firstmesh)
							continue;
						BE_SubmitBatch(old);
					}
				}
				BE_SelectMode(BEM_STANDARD);

				R_DrawPortal(batch, worldlist);

				/*clear depth again*/
				IDirect3DDevice8_Clear(pD3DDev8, 0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1, 0);
			}
		}
	}
}

void D3D8BE_SubmitMeshes (batch_t **worldbatches, batch_t **blist, int first, int stop)
{
	int i;

	for (i = first; i < stop; i++)
	{
		if (worldbatches)
		{
			if (i == SHADER_SORT_PORTAL /*&& !r_noportals.ival*/ && !r_refdef.recurse)
				BE_SubmitMeshesPortals(worldbatches, blist[i]);

			BE_SubmitMeshesSortList(worldbatches[i]);
		}
		BE_SubmitMeshesSortList(blist[i]);
	}
}

#ifdef RTLIGHTS
void D3D8BE_BaseEntTextures(void)
{
	batch_t *batches[SHADER_SORT_COUNT];
	BE_GenModelBatches(batches, shaderstate.curdlight, shaderstate.mode, r_refdef.scenevis);
	D3D8BE_SubmitMeshes(NULL, batches, SHADER_SORT_PORTAL, SHADER_SORT_SEETHROUGH+1);
	BE_SelectEntity(&r_worldentity);
}

void D3D8BE_RenderShadowBuffer(unsigned int numverts, IDirect3DVertexBuffer8 *vbuf, unsigned int numindicies, IDirect3DIndexBuffer8 *ibuf)
{
#ifdef FIXME
	float pushdepth = shaderstate.curshader->polyoffset.factor;
#ifdef BEF_PUSHDEPTH
	extern cvar_t r_polygonoffset_submodel_factor;
//	if (shaderstate.flags & BEF_PUSHDEPTH)
		pushdepth += r_polygonoffset_submodel_factor.value;
#endif
//	D3D8BE_Cull(0);//shaderstate.curshader->flags & (SHADER_CULL_FRONT | SHADER_CULL_BACK));
	pushdepth /= 0xffff;

	if (pushdepth != shaderstate.depthbias)
	{
		shaderstate.depthbias = pushdepth;
		IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_DEPTHBIAS, *(DWORD*)&shaderstate.depthbias);
	}
#endif


	IDirect3DDevice8_SetStreamSource(pD3DDev8, 0, vbuf, sizeof(vbovdata_t));
	IDirect3DDevice8_SetIndices(pD3DDev8, ibuf, 0);

	IDirect3DDevice8_DrawIndexedPrimitive(pD3DDev8, D3DPT_TRIANGLELIST, 0, numverts, 0, numindicies/3);
}
#endif

void D3D8BE_DrawWorld (batch_t **worldbatches)
{
	batch_t *batches[SHADER_SORT_COUNT];
	RSpeedLocals();

	shaderstate.curentity = NULL;

	if (!r_refdef.recurse)
	{
		if (shaderstate.wbatch > shaderstate.maxwbatches)
		{
			int newm = shaderstate.wbatch;
			shaderstate.wbatches = BZ_Realloc(shaderstate.wbatches, newm * sizeof(*shaderstate.wbatches));
			memset(shaderstate.wbatches + shaderstate.maxwbatches, 0, (newm - shaderstate.maxwbatches) * sizeof(*shaderstate.wbatches));
			shaderstate.maxwbatches = newm;
		}
		shaderstate.wbatch = 0;
	}

	shaderstate.curdlight = NULL;
	BE_GenModelBatches(batches, shaderstate.curdlight, BEM_STANDARD, r_refdef.scenevis);

	if (worldbatches)
	{
		float shaderstate_identitylighting;
		BE_UploadLightmaps(false);

		//make sure the world draws correctly
		r_worldentity.shaderRGBAf[0] = 1;
		r_worldentity.shaderRGBAf[1] = 1;
		r_worldentity.shaderRGBAf[2] = 1;
		r_worldentity.shaderRGBAf[3] = 1;
		r_worldentity.axis[0][0] = 1;
		r_worldentity.axis[1][1] = 1;
		r_worldentity.axis[2][2] = 1;

#ifdef RTLIGHTS
		if (worldbatches && r_shadow_realtime_world.ival)
			shaderstate_identitylighting = r_shadow_realtime_world_lightmaps.value;
		else
#endif
			shaderstate_identitylighting = r_lightmap_scale.value;
		shaderstate_identitylighting *= r_refdef.hdr_value;
//		shaderstate_identitylightmap = shaderstate.identitylighting / (1<<gl_overbright.ival);

		if (shaderstate_identitylighting == 0)
			BE_SelectMode(BEM_DEPTHDARK);
		else
			BE_SelectMode(BEM_STANDARD);

		RSpeedRemark();
		D3D8BE_SubmitMeshes(worldbatches, batches, SHADER_SORT_PORTAL, SHADER_SORT_SEETHROUGH+1);
		RSpeedEnd(RSPEED_OPAQUE);

#ifdef RTLIGHTS
		if (r_refdef.scenevis)
		{
			RSpeedRemark();
			D3D8BE_SelectEntity(&r_worldentity);
			Sh_DrawLights(r_refdef.scenevis);
			RSpeedEnd(RSPEED_RTLIGHTS);
		}
#endif

		BE_SelectMode(BEM_STANDARD);

		RSpeedRemark();
		D3D8BE_SubmitMeshes(worldbatches, batches, SHADER_SORT_SEETHROUGH+1, SHADER_SORT_COUNT);
		RSpeedEnd(RSPEED_TRANSPARENTS);
	}
	else
	{
		RSpeedRemark();
		D3D8BE_SubmitMeshes(NULL, batches, SHADER_SORT_PORTAL, SHADER_SORT_COUNT);
		RSpeedEnd(RSPEED_OPAQUE);
	}

	R_RenderDlights ();

	BE_RotateForEntity(&r_worldentity, NULL);
}
void D3D8BE_VBO_Begin(vbobctx_t *ctx, size_t maxsize)
{
	IDirect3DVertexBuffer8 *buf;
	IDirect3DDevice8_CreateVertexBuffer(pD3DDev8, maxsize, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &buf);
	ctx->vboptr[0] = buf;

	IDirect3DVertexBuffer8_Lock(buf, 0, maxsize, (BYTE**)&ctx->fallback, D3DLOCK_DISCARD);

	ctx->pos = 0;
}
void D3D8BE_VBO_Data(vbobctx_t *ctx, void *data, size_t size, vboarray_t *varray)
{
	IDirect3DVertexBuffer8 *buf = ctx->vboptr[0];
	memcpy((char*)ctx->fallback + ctx->pos, data, size);
	varray->d3d.buff = buf;
	varray->d3d.offs = ctx->pos;
	ctx->pos += size;
}
void D3D8BE_VBO_Finish(vbobctx_t *ctx, void *edata, size_t esize, vboarray_t *earray, void **vbomem, void **ebomem)
{
	IDirect3DIndexBuffer8 *buf;
	IDirect3DVertexBuffer8 *vbuf = ctx->vboptr[0];
	IDirect3DVertexBuffer8_Unlock(vbuf);
	ctx->fallback = NULL;

	IDirect3DDevice8_CreateIndexBuffer(pD3DDev8, esize, 0, D3DFMT_QINDEX, D3DPOOL_MANAGED, &buf);
	ctx->vboptr[1] = buf;
	IDirect3DIndexBuffer8_Lock(buf, 0, esize, (BYTE**)&ctx->fallback, D3DLOCK_DISCARD);
	memcpy(ctx->fallback, edata, esize);
	IDirect3DIndexBuffer8_Unlock(buf);
	ctx->fallback = NULL;

	earray->d3d.buff = buf;
	earray->d3d.offs = 0;
}
void D3D8BE_VBO_Destroy(vboarray_t *vearray, void *mem)
{
	IUnknown *ebuf = vearray->d3d.buff;
	if (ebuf)
		ebuf->lpVtbl->Release(ebuf);
}

void D3D8BE_Scissor(srect_t *srect)
{
	//d3d8 has no scissor
	//FIXME: we can supposedly emulate it with some projection matrix tweaks and viewport stuff
#if 0
	RECT rect;
	if (srect)
	{
		rect.left = (srect->x) * vid.fbpwidth;
		rect.right = (srect->x + srect->width) * vid.fbpwidth;
		rect.top = (srect->y) * vid.fbpheight;
		rect.bottom = (srect->y + srect->height) * vid.fbpheight;
	}
	else
	{
		rect.left = 0;
		rect.right = vid.pixelwidth;
		rect.top = 0;
		rect.bottom = vid.pixelheight;
	}
	IDirect3DDevice8_SetScissorRect(pD3DDev8, &rect);
	IDirect3DDevice8_SetRenderState(pD3DDev8, D3DRS_SCISSORTESTENABLE, TRUE);
#endif
}

#endif
