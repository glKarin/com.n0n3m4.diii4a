#include "quakedef.h"
#ifdef D3D11QUAKE
#include "glquake.h"
#include "gl_draw.h"
#include "shader.h"

#define COBJMACROS
#include <d3d11.h>

extern ID3D11Device *pD3DDev11;
extern ID3D11DeviceContext *d3ddevctx;
extern ID3D11DepthStencilView *fb_backdepthstencil;

extern cvar_t r_shadow_realtime_world_lightmaps;
extern cvar_t gl_overbright;
extern cvar_t r_portalrecursion, r_wireframe;

extern cvar_t r_polygonoffset_shadowmap_offset, r_polygonoffset_shadowmap_factor;

void D3D11_TerminateShadowMap(void);
void D3D11BE_BeginShadowmapFace(void);

#ifdef _DEBUG
#define d3dcheck(foo) do{HRESULT err = foo; if (FAILED(err)) Sys_Error("D3D11 reported error on backend line %i - error 0x%x\n", __LINE__, (unsigned int)err);} while(0)
#else
#define d3dcheck(foo) foo
#endif

#define MAX_TMUS 16

static void BE_RotateForEntity (const entity_t *e, const model_t *mod);
void D3D11BE_SetupLightCBuffer(dlight_t *l, vec3_t colour);
texid_t D3D11_GetShadowMap(int id);

/*========================================== tables for deforms =====================================*/
#define frand() (rand()*(1.0/RAND_MAX))
#define FTABLE_SIZE		1024
#define FTABLE_CLAMP(x)	(((int)((x)*FTABLE_SIZE) & (FTABLE_SIZE-1)))
#define FTABLE_EVALUATE(table,x) (table ? table[FTABLE_CLAMP(x)] : frand()*((x)-floor(x)))
#define R_FastSin(x) r_sintable[FTABLE_CLAMP(x)]

static	float	r_sintable[FTABLE_SIZE];
static	float	r_triangletable[FTABLE_SIZE];
static	float	r_squaretable[FTABLE_SIZE];
static	float	r_sawtoothtable[FTABLE_SIZE];
static	float	r_inversesawtoothtable[FTABLE_SIZE];

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

/*================================================*/

//global constant-buffer
typedef struct
{
	float m_view[16];
	float m_projection[16];
	vec3_t v_eyepos; float v_time;
	vec3_t e_light_ambient; float pad1;
	vec3_t e_light_dir; float pad2;
	vec3_t e_light_mul; float pad3;
} dx11_cbuf_view_t;

typedef struct
{
	float l_cubematrix[16];
	vec3_t l_lightposition; float padl1;
	vec3_t l_colour; float pad2;
	vec3_t l_lightcolourscale; float l_lightradius;
	vec4_t l_shadowmapproj;
	vec2_t l_shadowmapscale; vec2_t pad3;
} dx11_cbuf_light_t;

//entity-specific constant-buffer
typedef struct
{
	float m_model[16];
	vec3_t e_eyepos;
	float e_time;
	vec3_t e_light_ambient; float pad1;
	vec3_t e_light_dir; float pad2;
	vec3_t e_light_mul; float pad3;
	vec4_t e_lmscale[4];
	vec4_t e_uppercolour;
	vec4_t e_lowercolour;
	vec4_t e_colourmod;
	vec4_t e_glowmod;
} dx11_cbuf_entity_t;

//vertex attributes
typedef struct
{
	vecV_t coord;
	vec2_t tex;
	vec2_t lm;
	vec3_t ndir;
	vec3_t sdir;
	vec3_t tdir;
	byte_vec4_t colorsb;
} dx11_vbovdata_t;

enum
{
	//pos1 is always used
	D3D_VDEC_COL4B = 1<<0,
	D3D_VDEC_ST0 = 1<<1,	//base or tcgen/tcmod
	D3D_VDEC_ST1 = 1<<2,	//lm or so
//	D3D_VDEC_ST2 = 1<<3,
//	D3D_VDEC_ST3 = 1<<4,
	D3D_VDEC_NORM = 1<<3,	//normal+sdir+tdir
//	D3D_VDEC_SKEL = 1<<4,
//	D3D_VDEC_POS2 = 1<<5,
	D3D_VDEC_MAX = 1<<5,
};

typedef struct blendstates_s
{
	struct blendstates_s *next;
	ID3D11BlendState *val;
	unsigned int bits;
} blendstates_t;

/*
typedef struct d3d11renderqueue_s
{
	ID3D11SamplerState *cursamplerstate[MAX_TMUS];
	ID3D11ShaderResourceView *pendingtextures[MAX_TMUS];
	unsigned int shaderbits;

	ID3D11Buffer		*stream_buffer[D3D11_BUFF_MAX];
	unsigned int		stream_stride[D3D11_BUFF_MAX];
	unsigned int		stream_offset[D3D11_BUFF_MAX];
} d3d11renderqueue_t;
*/

typedef struct
{
	unsigned int inited;

	backendmode_t mode;
	unsigned int flags;

	float	identitylighting;
	float		curtime;
	const entity_t	*curentity;
	const dlight_t	*curdlight;
	vec3_t		curdlight_colours;
	shader_t	*curshader;
	shader_t	*depthonly;
	texnums_t	*curtexnums;
	int			curvertdecl;
	unsigned int shaderbits;
	unsigned int curcull;
	float depthbias;
	float depthfactor;
	float m_model[16];
	unsigned int lastpasscount;
	vbo_t *batchvbo;
	batch_t *curbatch;
	batch_t dummybatch;
	vec4_t lightshadowmapproj;
	vec2_t lightshadowmapscale;

	unsigned int curlmode;
	shader_t	*shader_rtlight[LSHADER_MODES];
	unsigned int texflags[MAX_TMUS];
	unsigned int tmuflags[MAX_TMUS];
	ID3D11SamplerState *cursamplerstate[MAX_TMUS];
	ID3D11SamplerState *sampstate[SHADER_PASS_IMAGE_FLAGS_D3D11+1];
	ID3D11DepthStencilState *depthstates[1u<<4];	//index, its fairly short.
	blendstates_t *blendstates;	//list. this could get big.

	//pending buffer state
	ID3D11Buffer		*stream_buffer[D3D11_BUFF_MAX];
	unsigned int		stream_stride[D3D11_BUFF_MAX];
	unsigned int		stream_offset[D3D11_BUFF_MAX];
	qboolean			stream_rgbaf;

	program_t			*programfixedemu[4];

	mesh_t		**meshlist;
	unsigned int nummeshes;

#define NUMECBUFFERS 8
	ID3D11Buffer *lcbuffer;
	ID3D11Buffer *vcbuffer;
	ID3D11Buffer *ecbuffers[NUMECBUFFERS];
	int ecbufferidx;

	unsigned int wbatch;
	unsigned int maxwbatches;
	batch_t *wbatches;

	qboolean textureschanged;
	ID3D11ShaderResourceView *pendingtextures[MAX_TMUS];

	float depthrange;

	qboolean purgevertexstream;
#define NUMVBUFFERS 3
	ID3D11Buffer *vertexstream[NUMVBUFFERS];
	int vertexstreamcycle;
	int vertexstreamoffset;
	qboolean purgeindexstream;
#define NUMIBUFFERS 3
	ID3D11Buffer *indexstream[NUMIBUFFERS];
	int indexstreamcycle;
	int indexstreamoffset;

	texid_t currentrender;


	int numlivevbos;
	int numliveshadowbuffers;
} d3d11backend_t;
extern D3D_FEATURE_LEVEL d3dfeaturelevel;

#define VERTEXSTREAMSIZE (1024*1024*2)	//2mb = 1 PAE jumbo page

#define DYNVBUFFSIZE 65536
#define DYNIBUFFSIZE 65536*3

static vecV_t tmpbuf[65536];	//max verts per mesh

static d3d11backend_t shaderstate;

extern int be_maxpasses;

void D3D11_UpdateFiltering(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float lodbias, float anis)
{
	D3D11_SAMPLER_DESC sampdesc;
	int flags;
	for (flags = 0; flags <= (SHADER_PASS_IMAGE_FLAGS_D3D11); flags++)
	{
		int *filter;
		sampdesc.Filter = 0;
		filter = (flags & SHADER_PASS_UIPIC)?filterpic:filtermip;
		if ((filter[2] && !(flags & SHADER_PASS_NEAREST)) || (flags & SHADER_PASS_LINEAR))
			sampdesc.Filter |= D3D11_FILTER_TYPE_LINEAR<<D3D11_MAG_FILTER_SHIFT;
		else
			sampdesc.Filter |= D3D11_FILTER_TYPE_POINT<<D3D11_MAG_FILTER_SHIFT;
		//d3d11 has no no-mip feature
		if ((filter[1]==1 && !(flags & SHADER_PASS_NEAREST)) || (flags & SHADER_PASS_LINEAR))
			sampdesc.Filter |= D3D11_FILTER_TYPE_LINEAR<<D3D11_MIP_FILTER_SHIFT;
		else
			sampdesc.Filter |= D3D11_FILTER_TYPE_POINT<<D3D11_MIP_FILTER_SHIFT;
		if ((filter[0] && !(flags & SHADER_PASS_NEAREST)) || (flags & SHADER_PASS_LINEAR))
			sampdesc.Filter |= D3D11_FILTER_TYPE_LINEAR<<D3D11_MIN_FILTER_SHIFT;
		else
			sampdesc.Filter |= D3D11_FILTER_TYPE_POINT<<D3D11_MIN_FILTER_SHIFT;
		//switch to anisotropic filtering if all three filters are linear and anis is set
		if (sampdesc.Filter == D3D11_FILTER_MIN_MAG_MIP_LINEAR && anis > 1)
			sampdesc.Filter = D3D11_FILTER_ANISOTROPIC;
		if (flags & SHADER_PASS_DEPTHCMP)
			sampdesc.Filter |= D3D11_COMPARISON_FILTERING_BIT;

		if (flags & SHADER_PASS_DEPTHCMP)
			sampdesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		else
			sampdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		if (flags & SHADER_PASS_CLAMP)
		{
			sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		}
		else
		{
			sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		}
		sampdesc.MipLODBias = lodbias;
		sampdesc.MaxAnisotropy = bound(1, anis, 16);
		sampdesc.BorderColor[0] = 0;
		sampdesc.BorderColor[1] = 0;
		sampdesc.BorderColor[2] = 0;
		sampdesc.BorderColor[3] = 0;
		if (flags & SHADER_PASS_NOMIPMAP)
		{	//only ever use the biggest mip if multiple are present
			sampdesc.MinLOD = 0;
			sampdesc.MaxLOD = 0;
		}
		else
		{
			sampdesc.MinLOD = mipcap[0];
			sampdesc.MaxLOD = mipcap[1];
		}

		if (d3dfeaturelevel < D3D_FEATURE_LEVEL_10_0)	//9.* sucks
			sampdesc.MaxLOD = FLT_MAX;

		if (shaderstate.sampstate[flags])
			ID3D11SamplerState_Release(shaderstate.sampstate[flags]);
		shaderstate.sampstate[flags] = NULL;

		//returns the same pointer for dupes, supposedly, so no need to check that
		ID3D11Device_CreateSamplerState(pD3DDev11, &sampdesc, &shaderstate.sampstate[flags]);
	}
}
static void BE_DestroyVariousStates(void)
{
	blendstates_t *bs;
	int flags;
	int i;
	
	for (i = 0; i < MAX_TMUS/*shaderstate.lastpasscount*/; i++)
	{
		shaderstate.cursamplerstate[i] = NULL;
	}
	if (d3ddevctx && i)
		ID3D11DeviceContext_PSSetSamplers(d3ddevctx, 0, i, shaderstate.cursamplerstate);

	for (flags = 0; flags <= SHADER_PASS_IMAGE_FLAGS_D3D11; flags++)
	{
		if (shaderstate.sampstate[flags])
			ID3D11SamplerState_Release(shaderstate.sampstate[flags]);
		shaderstate.sampstate[flags] = NULL;
	}

	if (d3ddevctx)
		ID3D11DeviceContext_OMSetDepthStencilState(d3ddevctx, NULL, 0);
	for (i = 0; i < (1u<<4); i++)
	{
		if (shaderstate.depthstates[i])
			ID3D11DepthStencilState_Release(shaderstate.depthstates[i]);
		shaderstate.depthstates[i] = NULL;
	}

	if (d3ddevctx)
		ID3D11DeviceContext_OMSetBlendState(d3ddevctx, NULL, NULL, 0xffffffff);
	//hopefully the caches inside shaders should get flushed too...
	while(shaderstate.blendstates)
	{
		bs = shaderstate.blendstates;
		shaderstate.blendstates = bs->next;

		if (bs->val)
			ID3D11BlendState_Release(bs->val);
		BZ_Free(bs);
	}

	for (i = 0; i < NUMIBUFFERS; i++)
	{
		if (shaderstate.indexstream[i])
			ID3D11Buffer_Release(shaderstate.indexstream[i]);
		shaderstate.indexstream[i] = NULL;
	}
	
	for (i = 0; i < NUMVBUFFERS; i++)
	{
		if (shaderstate.vertexstream[i])
			ID3D11Buffer_Release(shaderstate.vertexstream[i]);
		shaderstate.vertexstream[i] = NULL;
	}

	if (shaderstate.lcbuffer)
		ID3D11Buffer_Release(shaderstate.lcbuffer);
	shaderstate.lcbuffer = NULL;

	if (shaderstate.vcbuffer)
		ID3D11Buffer_Release(shaderstate.vcbuffer);
	shaderstate.vcbuffer = NULL;

	for (i = 0; i < NUMECBUFFERS; i++)
	{
		if (shaderstate.ecbuffers[i])
			ID3D11Buffer_Release(shaderstate.ecbuffers[i]);
		shaderstate.ecbuffers[i] = NULL;
	}

	D3D11_DestroyTexture(shaderstate.currentrender);

	//make sure the device doesn't have any textures still referenced.
	for (i = 0; i < MAX_TMUS/*shaderstate.lastpasscount*/; i++)
	{
		shaderstate.pendingtextures[i] = NULL;
	}
	if (d3ddevctx && i)
		ID3D11DeviceContext_PSSetShaderResources(d3ddevctx, 0, i, shaderstate.pendingtextures);
}

static void BE_ApplyTMUState(unsigned int tu, unsigned int flags)
{
	ID3D11SamplerState *nstate;

	flags = (flags & SHADER_PASS_IMAGE_FLAGS_D3D11);
	flags |= shaderstate.texflags[tu];
	nstate = shaderstate.sampstate[flags];
	if (nstate != shaderstate.cursamplerstate[tu])
	{
		shaderstate.cursamplerstate[tu] = nstate;

		//fixme: is it significant to bulk-apply this later?
		ID3D11DeviceContext_PSSetSamplers(d3ddevctx, tu, 1, &nstate);
	}
	/*
	if ((flags ^ shaderstate.tmuflags[tu]) & (SHADER_PASS_NEAREST|SHADER_PASS_CLAMP))
	{
		D3D11_SAMPLER_DESC sampdesc;
		ID3D11SamplerState *sstate;

		shaderstate.tmuflags[tu] = flags;

		if (flags & SHADER_PASS_NEAREST)
			sampdesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		else
			sampdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		if (flags & SHADER_PASS_CLAMP)
		{
			sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		}
		else
		{
			sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		}
		sampdesc.MipLODBias = 0.0f;
		sampdesc.MaxAnisotropy = 1;
		sampdesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampdesc.BorderColor[0] = 0;
		sampdesc.BorderColor[1] = 0;
		sampdesc.BorderColor[2] = 0;
		sampdesc.BorderColor[3] = 0;
		sampdesc.MinLOD = 0;
		sampdesc.MaxLOD = D3D11_FLOAT32_MAX;

		if (!FAILED(ID3D11Device_CreateSamplerState(pD3DDev11, &sampdesc, &sstate)))
		{
			ID3D11DeviceContext_PSSetSamplers(d3ddevctx, tu, 1, &sstate);
			ID3D11SamplerState_Release(sstate);
		}
	}
	*/
}

static void *D3D11BE_GenerateBlendState(unsigned int bits)
{
	D3D11_BLEND_DESC  blend = {0};
	ID3D11BlendState *newblendstate;
	blend.IndependentBlendEnable = FALSE;
	blend.AlphaToCoverageEnable = FALSE;	//FIXME

	if (bits & SBITS_BLEND_BITS)
	{
		switch(bits & SBITS_SRCBLEND_BITS)
		{
		case SBITS_SRCBLEND_ZERO:					blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;				blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;				break;
		case SBITS_SRCBLEND_ONE:					blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;				blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;				break;
		case SBITS_SRCBLEND_DST_COLOR:				blend.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;		blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;		break;
		case SBITS_SRCBLEND_ONE_MINUS_DST_COLOR:	blend.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;	blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;	break;
		case SBITS_SRCBLEND_SRC_ALPHA:				blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;			blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;		break;
		case SBITS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	blend.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_SRC_ALPHA;		blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;	break;
		case SBITS_SRCBLEND_DST_ALPHA:				blend.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_ALPHA;		blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;		break;
		case SBITS_SRCBLEND_ONE_MINUS_DST_ALPHA:	blend.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_ALPHA;	blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;	break;
		case SBITS_SRCBLEND_ALPHA_SATURATE:			blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA_SAT;		blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA_SAT;	break;
		default:	Sys_Error("Bad shader blend src\n"); return NULL;
		}
		switch(bits & SBITS_DSTBLEND_BITS)
		{
		case SBITS_DSTBLEND_ZERO:					blend.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;				blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;			break;
		case SBITS_DSTBLEND_ONE:					blend.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;				blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;				break;
		case SBITS_DSTBLEND_SRC_ALPHA:				blend.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;		blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;		break;
		case SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;	blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;	break;
		case SBITS_DSTBLEND_DST_ALPHA:				blend.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_ALPHA;		blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;		break;
		case SBITS_DSTBLEND_ONE_MINUS_DST_ALPHA:	blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_DEST_ALPHA;	blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;	break;
		case SBITS_DSTBLEND_SRC_COLOR:				blend.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;		blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;		break;
		case SBITS_DSTBLEND_ONE_MINUS_SRC_COLOR:	blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;	blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;	break;
		default:	Sys_Error("Bad shader blend dst\n"); return NULL;
		}
		blend.RenderTarget[0].BlendEnable = TRUE;
	}
	else
	{
		blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blend.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blend.RenderTarget[0].BlendEnable = FALSE;
	}
	blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	if (bits&SBITS_MASK_BITS)
	{
		blend.RenderTarget[0].RenderTargetWriteMask = 0;
		if (!(bits&SBITS_MASK_RED))
			blend.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
		if (!(bits&SBITS_MASK_GREEN))
			blend.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
		if (!(bits&SBITS_MASK_BLUE))
			blend.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
		if (!(bits&SBITS_MASK_ALPHA))
			blend.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
	}
	else
		blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (!FAILED(ID3D11Device_CreateBlendState(pD3DDev11, &blend, &newblendstate)))
		return newblendstate;
	return NULL;
}

static void D3D11BE_ApplyShaderBits(unsigned int bits, void **blendstatecache)
{
	unsigned int delta;

	if (shaderstate.flags & (BEF_FORCEADDITIVE|BEF_FORCETRANSPARENT|BEF_FORCENODEPTH|BEF_FORCEDEPTHTEST|BEF_FORCEDEPTHWRITE))
	{
		blendstatecache = NULL;

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

	delta = bits ^ shaderstate.shaderbits;
	if (!delta)
		return;
	shaderstate.shaderbits = bits;

	if (delta & (SBITS_BLEND_BITS|SBITS_MASK_BITS))
	{
		int sbits = bits & (SBITS_BLEND_BITS|SBITS_MASK_BITS);
		if (blendstatecache && *blendstatecache)
			ID3D11DeviceContext_OMSetBlendState(d3ddevctx, *blendstatecache, NULL, 0xffffffff);
		else
		{
			blendstates_t *bs;
			for (bs = shaderstate.blendstates; bs; bs = bs->next)
			{
				if (bs->bits == sbits)
					break;
			}
			if (!bs)
			{
				bs = BZ_Malloc(sizeof(*bs));
				bs->next = shaderstate.blendstates;
				shaderstate.blendstates = bs;
				bs->bits = sbits;
				bs->val = D3D11BE_GenerateBlendState(sbits);
			}
			ID3D11DeviceContext_OMSetBlendState(d3ddevctx, bs->val, NULL, 0xffffffff);
			if (blendstatecache)
				*blendstatecache = bs->val;
		}
	}

	if (delta & SBITS_ATEST_BITS)
	{
/*
		switch(bits & SBITS_ATEST_BITS)
		{
		case SBITS_ATEST_NONE:
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHATESTENABLE, FALSE);
	//		IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAREF, 0);
	//		IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAFUNC, 0);
			break;
		case SBITS_ATEST_GT0:
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHATESTENABLE, TRUE);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAREF, 0);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAFUNC, D3DCMP_GREATER);
			break;
		case SBITS_ATEST_LT128:
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHATESTENABLE, TRUE);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAREF, 128);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAFUNC, D3DCMP_LESS);
			break;
		case SBITS_ATEST_GE128:
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHATESTENABLE, TRUE);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAREF, 128);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
			break;
		}
*/
	}

	if (delta & (SBITS_DEPTHFUNC_BITS|SBITS_MISC_NODEPTHTEST|SBITS_MISC_DEPTHWRITE))
	{
		unsigned int key = (bits&(SBITS_DEPTHFUNC_BITS|SBITS_MISC_NODEPTHTEST|SBITS_MISC_DEPTHWRITE))>>16;
		if (shaderstate.depthstates[key])
			ID3D11DeviceContext_OMSetDepthStencilState(d3ddevctx, shaderstate.depthstates[key], 0);
		else
		{
			D3D11_DEPTH_STENCIL_DESC depthdesc;
			if (bits & SBITS_MISC_NODEPTHTEST)
				depthdesc.DepthEnable = false;
			else
				depthdesc.DepthEnable = true;
			if (bits & SBITS_MISC_DEPTHWRITE)
				depthdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			else
				depthdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

			switch(bits & SBITS_DEPTHFUNC_BITS)
			{
			default:
			case SBITS_DEPTHFUNC_CLOSEREQUAL:
				depthdesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
				break;
			case SBITS_DEPTHFUNC_EQUAL:
				depthdesc.DepthFunc = D3D11_COMPARISON_EQUAL;
				break;
			case SBITS_DEPTHFUNC_CLOSER:
				depthdesc.DepthFunc = D3D11_COMPARISON_LESS;
				break;
			case SBITS_DEPTHFUNC_FURTHER:
				depthdesc.DepthFunc = D3D11_COMPARISON_GREATER;
				break;
			}

			//make sure the stencil part is actually valid, even if we're not using it.
			depthdesc.StencilEnable = false;
			depthdesc.StencilReadMask = 0xFF;
			depthdesc.StencilWriteMask = 0xFF;
			depthdesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			depthdesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			//and change it
			if (!FAILED(ID3D11Device_CreateDepthStencilState(pD3DDev11, &depthdesc, &shaderstate.depthstates[key])))
				ID3D11DeviceContext_OMSetDepthStencilState(d3ddevctx, shaderstate.depthstates[key], 0);
		}
	}
}

void D3D11BE_Reset(qboolean before)
{
	int i;
	if (!shaderstate.inited)
		return;

	if (before)
	{
		/*backbuffer is going away, release stuff so it can be destroyed cleanly*/
	}
	else
	{
		/*we have a new backbuffer etc, reassert state*/
		for (i = 0; i < MAX_TMUS; i++)
		{
			shaderstate.tmuflags[i] = ~0;
			BE_ApplyTMUState(i, 0);
		}

		/*force all state to change, thus setting a known state*/
		shaderstate.shaderbits = ~0;
		D3D11BE_ApplyShaderBits(0, NULL);
	}
}

static const char LIGHTPASS_SHADER[] = "\
{\n\
	program rtlight\n\
	{\n\
		blendfunc add\n\
	}\n\
}";

void D3D11BE_Init(void)
{
	D3D11_BUFFER_DESC bd;
	int i;

	be_maxpasses = 1;
	memset(&shaderstate, 0, sizeof(shaderstate));
	shaderstate.inited = true;
	shaderstate.curvertdecl = -1;
	for (i = 0; i < MAXRLIGHTMAPS; i++)
		shaderstate.dummybatch.lightmap[i] = -1;

	FTable_Init();

//	BE_CreateSamplerStates();

//	FTable_Init();

/*	shaderstate.dynxyz_size = sizeof(vecV_t) * DYNVBUFFSIZE;
	shaderstate.dyncol_size = sizeof(byte_vec4_t) * DYNVBUFFSIZE;
	shaderstate.dynst_size = sizeof(vec2_t) * DYNVBUFFSIZE;
	shaderstate.dynidx_size = sizeof(index_t) * DYNIBUFFSIZE;
*/
	D3D11BE_Reset(false);

	//set up the constant buffers
	for (i = 0; i < NUMECBUFFERS; i++)
	{
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(dx11_cbuf_entity_t);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		if (FAILED(ID3D11Device_CreateBuffer(pD3DDev11, &bd, NULL, &shaderstate.ecbuffers[i])))
			return;
	}
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(dx11_cbuf_view_t);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	if (FAILED(ID3D11Device_CreateBuffer(pD3DDev11, &bd, NULL, &shaderstate.vcbuffer)))
		return;

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(dx11_cbuf_light_t);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	if (FAILED(ID3D11Device_CreateBuffer(pD3DDev11, &bd, NULL, &shaderstate.lcbuffer)))
		return;

	//generate the streaming buffers for stuff that doesn't provide info in nice static vbos
	for (i = 0; i < NUMIBUFFERS; i++)
	{
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.ByteWidth = VERTEXSTREAMSIZE;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		if (FAILED(ID3D11Device_CreateBuffer(pD3DDev11, &bd, NULL, &shaderstate.indexstream[i])))
			return;
	}
	for (i = 0; i < NUMVBUFFERS; i++)
	{
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.ByteWidth = VERTEXSTREAMSIZE;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		if (FAILED(ID3D11Device_CreateBuffer(pD3DDev11, &bd, NULL, &shaderstate.vertexstream[i])))
			return;
	}

	/*
	for (i = 0; i < LSHADER_MODES; i++)
	{
		if ((i & LSHADER_CUBE) && (i & LSHADER_SPOT))
			continue;
		shaderstate.shader_rtlight[i] = R_RegisterShader(va("rtlight%s%s%s", 
															(i & LSHADER_SMAP)?"#PCF":"",
															(i & LSHADER_SPOT)?"#SPOT":"",
															(i & LSHADER_CUBE)?"#CUBE":"")
														, SUF_NONE, LIGHTPASS_SHADER);
	}
	*/
//	shaderstate.shader_rtlight = R_RegisterShader("rtlight", SUF_NONE, LIGHTPASS_SHADER);
	shaderstate.depthonly = R_RegisterShader("depthonly", SUF_NONE, 
				"{\n"
					"program depthonly\n"
					"{\n"
						"depthwrite\n"
						"maskcolor\n"
					"}\n"
				"}\n");

	R_InitFlashblends();
}

void D3D11BE_Shutdown(void)
{
	unsigned int i;
	shaderstate.inited = false;
#ifdef RTLIGHTS
	D3D11_TerminateShadowMap();
#endif
	BE_DestroyVariousStates();
	Z_Free(shaderstate.wbatches);
	shaderstate.wbatches = NULL;

	for (i = 0; i < countof(shaderstate.programfixedemu); i++)
		Shader_ReleaseGeneric(shaderstate.programfixedemu[i]);
}

static void allocvertexbuffer(ID3D11Buffer **buf, unsigned int *offset, void **dest, unsigned int sz)
{
	D3D11_MAPPED_SUBRESOURCE msr;
	unsigned int maptype;
	sz = (sz+63)&~63;	//try to align it to a cacheline. I assume the memory is write-through cached.
	if (shaderstate.purgevertexstream || shaderstate.vertexstreamoffset + sz > VERTEXSTREAMSIZE)
	{
		shaderstate.purgevertexstream = false;
		shaderstate.vertexstreamoffset = 0;
		maptype = D3D11_MAP_WRITE_DISCARD;
		shaderstate.vertexstreamcycle++;
		if (shaderstate.vertexstreamcycle == NUMVBUFFERS)
			shaderstate.vertexstreamcycle = 0;
	}
	else
	{
		maptype = D3D11_MAP_WRITE_NO_OVERWRITE;
	}
	*buf = shaderstate.vertexstream[shaderstate.vertexstreamcycle];
	if (FAILED(ID3D11DeviceContext_Map(d3ddevctx, (ID3D11Resource*)*buf, 0, maptype, 0, &msr)))
	{
		*offset = 0;
		*buf = NULL;
		*dest = NULL;
		Con_Printf("allocvertexbuffer: failed to map vertex buffer\n");
		return;
	}

	*dest = (index_t*)((qbyte*)msr.pData + shaderstate.vertexstreamoffset);
	*offset = shaderstate.vertexstreamoffset;
	shaderstate.vertexstreamoffset += sz;
	return;
}

static unsigned int allocindexbuffer(index_t **dest, ID3D11Buffer **buf, unsigned int entries)
{
	D3D11_MAPPED_SUBRESOURCE msr;
	unsigned int sz = sizeof(index_t) * entries;
	unsigned int maptype;
	unsigned int ret;
	if (shaderstate.purgeindexstream || shaderstate.indexstreamoffset + sz > VERTEXSTREAMSIZE)
	{
		shaderstate.purgeindexstream = false;
		shaderstate.indexstreamoffset = 0;
		maptype = D3D11_MAP_WRITE_DISCARD;
		shaderstate.indexstreamcycle++;
		if (shaderstate.indexstreamcycle == NUMIBUFFERS)
			shaderstate.indexstreamcycle = 0;
	}
	else
	{
		maptype = D3D11_MAP_WRITE_NO_OVERWRITE;
	}
	*buf = shaderstate.indexstream[shaderstate.indexstreamcycle];
	if (FAILED(ID3D11DeviceContext_Map(d3ddevctx, (ID3D11Resource*)*buf, 0, maptype, 0, &msr)))
	{
		*buf = NULL;
		*dest = NULL;
		Con_Printf("allocindexbuffer: failed to map index buffer\n");
		return 0;
	}

	*dest = (index_t*)((qbyte*)msr.pData + shaderstate.indexstreamoffset);
	ret = shaderstate.indexstreamoffset;
	shaderstate.indexstreamoffset += sz;
	return ret;
}

ID3D11ShaderResourceView *D3D11_Image_View(const texid_t id);
static void BindTexture(unsigned int tu, const texid_t id)
{
	ID3D11ShaderResourceView *view = D3D11_Image_View(id);
	if (shaderstate.pendingtextures[tu] != view)
	{
		shaderstate.textureschanged = true;
		shaderstate.pendingtextures[tu] = view;
		if (id)
			shaderstate.texflags[tu] = id->flags&SHADER_PASS_IMAGE_FLAGS_D3D11;
	}
}

void D3D11BE_UnbindAllTextures(void)
{
	int i;
	for (i = 0; i < shaderstate.lastpasscount; i++)
		shaderstate.pendingtextures[i] = NULL;
	if (i)
	{
		ID3D11DeviceContext_PSSetShaderResources(d3ddevctx, 0, i, shaderstate.pendingtextures);
		shaderstate.lastpasscount = 0;
	}
}

#define IID_ID3D11Texture2D gahzomgwtf
static const GUID IID_ID3D11Texture2D = { 0x6f15aaf2, 0xd208, 0x4e89, { 0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c } };

static texid_t T_Gen_CurrentRender(void)
{
//	int vwidth, vheight;
//	int pwidth = vid.fbpwidth;
//	int pheight = vid.fbpheight;

	ID3D11Texture2D *backbuf;
#ifdef WINRT
	extern IDXGISwapChain1 *d3dswapchain;
#else
	extern IDXGISwapChain *d3dswapchain;
#endif
	D3D11_TEXTURE2D_DESC tdesc = {0};

	if (r_refdef.recurse)
		return r_nulltex;

	//FIXME: should be willing to use the current rendertargets instead
	IDXGISwapChain_GetBuffer(d3dswapchain, 0, &IID_ID3D11Texture2D, (LPVOID*)&backbuf);
	ID3D11Texture2D_GetDesc(backbuf, &tdesc);

	// copy the scene to texture
	if (!TEXVALID(shaderstate.currentrender) || tdesc.Width != shaderstate.currentrender->width || tdesc.Height != shaderstate.currentrender->height)
	{
		if (!shaderstate.currentrender)
			shaderstate.currentrender = Image_CreateTexture("***$currentrender***", NULL, 0);
		if (shaderstate.currentrender)
		{
			shaderstate.currentrender->width = tdesc.Width;
			shaderstate.currentrender->height = tdesc.Height;

			tdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			tdesc.CPUAccessFlags = 0;

			if (shaderstate.currentrender->ptr)
				ID3D11Texture2D_Release((ID3D11Texture2D*)shaderstate.currentrender->ptr);
			shaderstate.currentrender->ptr = NULL;
			ID3D11Device_CreateTexture2D(pD3DDev11, &tdesc, NULL, (ID3D11Texture2D**)&shaderstate.currentrender->ptr);
		}
	}
	ID3D11DeviceContext_CopyResource(d3ddevctx, (ID3D11Resource*)shaderstate.currentrender->ptr, (ID3D11Resource*)backbuf);

	if (shaderstate.currentrender->ptr2)
		ID3D11ShaderResourceView_Release((ID3D11ShaderResourceView*)shaderstate.currentrender->ptr2);
	shaderstate.currentrender->ptr2 = NULL;

	ID3D11Texture2D_Release(backbuf);

	return shaderstate.currentrender;
}

static void SelectPassTexture(unsigned int tu, const shaderpass_t *pass)
{
	extern texid_t r_whiteimage, missing_texture_gloss, missing_texture_normal;
	switch(pass->texgen)
	{
	default:

	case T_GEN_DIFFUSE:
		BindTexture(tu, shaderstate.curtexnums->base);
		break;
	case T_GEN_NORMALMAP:
		if (TEXLOADED(shaderstate.curtexnums->bump))
			BindTexture(tu, shaderstate.curtexnums->bump);
		else
			BindTexture(tu, missing_texture_normal);
		break;
	case T_GEN_SPECULAR:
		if (TEXLOADED(shaderstate.curtexnums->specular))
			BindTexture(tu, shaderstate.curtexnums->specular);
		else
			BindTexture(tu, missing_texture_gloss);
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
	case T_GEN_REFLECTCUBE:
		BindTexture(tu, shaderstate.curtexnums->reflectcube);
		break;
	case T_GEN_REFLECTMASK:
		BindTexture(tu, shaderstate.curtexnums->reflectmask);
		break;
	case T_GEN_ANIMMAP:
		BindTexture(tu, pass->anim_frames[(int)(pass->anim_fps * shaderstate.curtime) % pass->anim_numframes]);
		break;
	case T_GEN_SINGLEMAP:
		BindTexture(tu, pass->anim_frames[0]);
		break;
	case T_GEN_DELUXMAP:
		{
			int lmi = shaderstate.curbatch->lightmap[0];
			if (lmi < 0 || !lightmap[lmi]->hasdeluxe)
				BindTexture(tu, r_nulltex);
			else
			{
				lmi+=1;
				BindTexture(tu, lightmap[lmi]->lightmap_texture);
			}
		}
		break;
	case T_GEN_LIGHTMAP:
		{
			int lmi = shaderstate.curbatch->lightmap[0];
			if (lmi < 0)
				BindTexture(tu, r_whiteimage);
			else
				BindTexture(tu, lightmap[lmi]->lightmap_texture);
		}
		break;

	case T_GEN_CURRENTRENDER:
		BindTexture(tu, T_Gen_CurrentRender());
		break;
	case T_GEN_VIDEOMAP:
#ifdef HAVE_MEDIA_DECODER
		if (pass->cin)
		{
			BindTexture(tu, Media_UpdateForShader(pass->cin));
			break;
		}
#endif
		BindTexture(tu, r_nulltex);
		break;

	case T_GEN_LIGHTCUBEMAP:	//light's projected cubemap
		if (shaderstate.curdlight)
			BindTexture(tu, shaderstate.curdlight->cubetexture);
		else
			BindTexture(tu, r_nulltex);
		break;

	case T_GEN_SHADOWMAP:	//light's depth values.
#ifdef RTLIGHTS
		if (shaderstate.curdlight)
		{
			BindTexture(tu, D3D11_GetShadowMap(shaderstate.curdlight->fov>0));
			break;
		}
#endif
		BindTexture(tu, r_nulltex);
		break;

	case T_GEN_SOURCECOLOUR: //used for render-to-texture targets
	case T_GEN_SOURCEDEPTH:	//used for render-to-texture targets

	case T_GEN_REFLECTION:	//reflection image (mirror-as-fbo)
	case T_GEN_REFRACTION:	//refraction image (portal-as-fbo)
	case T_GEN_REFRACTIONDEPTH:	//refraction image (portal-as-fbo)
	case T_GEN_RIPPLEMAP:	//ripplemap image (water surface distortions-as-fbo)

	case T_GEN_SOURCECUBE:	//used for render-to-texture targets
		BindTexture(tu, r_nulltex);
		break;
	}

	BE_ApplyTMUState(tu, pass->flags);

	//pass blend modes are skipped - they're really only useful for fixed function. we should just use blend modes instead.
}

static void colourgenbyte(const shaderpass_t *pass, int cnt, byte_vec4_t *srcb, vec4_t *srcf, byte_vec4_t *dst, const mesh_t *mesh)
{
#define D3DCOLOR unsigned int
#define D3DCOLOR_ARGB(a,r,g,b)	((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_COLORVALUE(r,g,b,a)	D3DCOLOR_RGBA((DWORD)((r)*255.f),(DWORD)((g)*255.f),(DWORD)((b)*255.f),(DWORD)((a)*255.f))
#define D3DCOLOR_RGBA(r,g,b,a)	D3DCOLOR_ARGB(a,r,g,b)
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
	case RGB_GEN_IDENTITY_OVERBRIGHT:
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
			R_LightArraysByte_BGR(shaderstate.curentity , mesh->xyz_array, dst, cnt, mesh->normals_array, pass->rgbgen==RGB_GEN_ENTITY_LIGHTING_DIFFUSE);
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

//true if we used an array (flag to use uniforms for it instead if false)
static void BE_GenerateColourMods(unsigned int vertcount, const shaderpass_t *pass)
{
	const mesh_t *m = shaderstate.meshlist[0];
	if (pass->flags & SHADER_PASS_NOCOLORARRAY)
	{
#if 1
		ID3D11Buffer *buf;
		unsigned char *map;
		unsigned int offset;
		byte_vec4_t passcolour;
		static byte_vec4_t fakesource = {0xff,0xff,0xff,0xff};
		allocvertexbuffer(&buf, &offset, (void**)&map, sizeof(byte_vec4_t));

		colourgenbyte(pass, 1, (byte_vec4_t*)&fakesource, NULL, (byte_vec4_t*)&passcolour, m);
		alphagenbyte(pass, 1, (byte_vec4_t*)&fakesource, NULL, (byte_vec4_t*)&passcolour, m);
		*(int*)map = *(int*)passcolour;

		ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
		shaderstate.stream_buffer[D3D11_BUFF_COL] = buf;
		shaderstate.stream_offset[D3D11_BUFF_COL] = offset;
		shaderstate.stream_stride[D3D11_BUFF_COL] = 0;	//omg that's so lame!
		shaderstate.stream_rgbaf = false;
#else
		ID3D11Buffer *buf;
		unsigned char *map;
		unsigned int offset;
		vec4_t passcolour;
		static vec4_t fakesource = {1,1,1,1};
		allocvertexbuffer(&buf, &offset, (void**)&map, sizeof(fakesource));

		colourgen(pass, 1, (vec4_t*)&fakesource, NULL, (vec4_t*)&passcolour, m);
		alphagen(pass, 1, (vec4_t*)&fakesource, NULL, (vec4_t*)&passcolour, m);
		Vector4Copy(passcolour, map);

		ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
		shaderstate.stream_buffer[D3D11_BUFF_COL] = buf;
		shaderstate.stream_offset[D3D11_BUFF_COL] = offset;
		shaderstate.stream_stride[D3D11_BUFF_COL] = 0;	//omg that's so lame!
		shaderstate.stream_rgbaf = true;
#endif
	}
	else
	{
		if (shaderstate.batchvbo && (m->colors4f_array[0] &&
						((pass->rgbgen == RGB_GEN_VERTEX_LIGHTING) ||
						(pass->rgbgen == RGB_GEN_VERTEX_EXACT) ||
						(pass->rgbgen == RGB_GEN_ONE_MINUS_VERTEX)) &&
						(pass->alphagen == ALPHA_GEN_VERTEX)))
		{
			shaderstate.stream_buffer[D3D11_BUFF_COL] = shaderstate.batchvbo->colours[0].d3d.buff;
			shaderstate.stream_offset[D3D11_BUFF_COL] = shaderstate.batchvbo->colours[0].d3d.offs;
			shaderstate.stream_stride[D3D11_BUFF_COL] = sizeof(dx11_vbovdata_t);
			shaderstate.stream_rgbaf = false;
		}
		else
		{
			ID3D11Buffer *buf;
			byte_vec4_t *map;
			unsigned int mno;
			unsigned int offset;
			allocvertexbuffer(&buf, &offset, (void**)&map, vertcount*sizeof(byte_vec4_t));
			for (mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				m = shaderstate.meshlist[mno];
				colourgenbyte(pass, m->numvertexes, m->colors4b_array, m->colors4f_array[0], map, m);
				alphagenbyte(pass, m->numvertexes, m->colors4b_array, m->colors4f_array[0], map, m);
				map += m->numvertexes;
			}
			ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
			shaderstate.stream_buffer[D3D11_BUFF_COL] = buf;
			shaderstate.stream_offset[D3D11_BUFF_COL] = offset;
			shaderstate.stream_stride[D3D11_BUFF_COL] = sizeof(byte_vec4_t);
			shaderstate.stream_rgbaf = false;
		}
	}
}

/*********************************************************************************************************/
/*========================================== texture coord generation =====================================*/
static void tcgen_environment(float *st, unsigned int numverts, float *xyz, float *normal)
{
	int			i;
	vec3_t		viewer, reflected;
	float		d;

	vec3_t		rorg;

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

static void BE_GenerateTCMods(const shaderpass_t *pass, float *dest)
{
	mesh_t *mesh;
	unsigned int mno;
	int i;
	float *src;
	for (mno = 0; mno < shaderstate.nummeshes; mno++)
	{
		mesh = shaderstate.meshlist[mno];
		src = tcgen(pass, mesh->numvertexes, dest, mesh);
		//tcgen might return unmodified info
		if (pass->numtcmods)
		{
			tcmod(&pass->tcmods[0], mesh->numvertexes, src, dest, mesh);
			for (i = 1; i < pass->numtcmods; i++)
			{
				tcmod(&pass->tcmods[i], mesh->numvertexes, dest, dest, mesh);
			}
		}
		else if (src != dest)
		{
			memcpy(dest, src, sizeof(vec2_t)*mesh->numvertexes);
		}
		dest += mesh->numvertexes*2;
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


static void BE_ApplyUniforms(program_t *prog, struct programpermu_s *permu)
{
	ID3D11Buffer *cbuf[3] =
	{
		shaderstate.ecbuffers[shaderstate.ecbufferidx],	//entity buffer
		shaderstate.vcbuffer,							//view buffer that changes rarely
		shaderstate.lcbuffer							//light buffer that changes rarelyish
	};
	//FIXME: how many of these calls can we avoid?
	ID3D11DeviceContext_IASetInputLayout(d3ddevctx, permu->h.hlsl.layouts[shaderstate.stream_rgbaf]);
	ID3D11DeviceContext_VSSetShader(d3ddevctx, permu->h.hlsl.vert, NULL, 0);
	ID3D11DeviceContext_HSSetShader(d3ddevctx, permu->h.hlsl.hull, NULL, 0);
	ID3D11DeviceContext_DSSetShader(d3ddevctx, permu->h.hlsl.domain, NULL, 0);
	ID3D11DeviceContext_GSSetShader(d3ddevctx, permu->h.hlsl.geom, NULL, 0);
	ID3D11DeviceContext_PSSetShader(d3ddevctx, permu->h.hlsl.frag, NULL, 0);
	ID3D11DeviceContext_IASetPrimitiveTopology(d3ddevctx, permu->h.hlsl.topology);

	ID3D11DeviceContext_VSSetConstantBuffers(d3ddevctx, 0, 3, cbuf);
	if (permu->h.hlsl.hull)
		ID3D11DeviceContext_HSSetConstantBuffers(d3ddevctx, 0, 3, cbuf);
	if (permu->h.hlsl.domain)
		ID3D11DeviceContext_DSSetConstantBuffers(d3ddevctx, 0, 3, cbuf);
	if (permu->h.hlsl.geom)
		ID3D11DeviceContext_GSSetConstantBuffers(d3ddevctx, 0, 3, cbuf);
	ID3D11DeviceContext_PSSetConstantBuffers(d3ddevctx, 0, 3, cbuf);
}

static void BE_RenderMeshProgram(program_t *p, shaderpass_t *pass, unsigned int vertcount, unsigned int idxfirst, unsigned int idxcount)
{
	int passno;
	int perm = 0;

	struct programpermu_s *pp;

	if (TEXLOADED(shaderstate.curtexnums->bump))
		perm |= PERMUTATION_BUMPMAP;
	if (TEXLOADED(shaderstate.curtexnums->fullbright))
		perm |= PERMUTATION_FULLBRIGHT;
	if ((TEXLOADED(shaderstate.curtexnums->upperoverlay) || TEXLOADED(shaderstate.curtexnums->loweroverlay)))
		perm |= PERMUTATION_UPPERLOWER;
	if (r_refdef.globalfog.density)
		perm |= PERMUTATION_FOG;
//	if (r_glsl_offsetmapping.ival && TEXLOADED(shaderstate.curtexnums->bump))
//		perm |= PERMUTATION_OFFSET;

	perm &= p->supportedpermutations;
	pp = p->permu[perm];
	if (!pp)
	{
		p->permu[perm] = pp = Shader_LoadPermutation(p, perm);
		if (!pp)
		{	//failed? copy from 0 so we don't keep re-failing
			pp = p->permu[perm] = p->permu[0];
		}
	}

	BE_ApplyUniforms(p, pp);

	D3D11BE_ApplyShaderBits(pass->shaderbits, &pass->becache);

	/*activate tmus*/
	for (passno = 0; passno < pass->numMergedPasses; passno++)
	{
		SelectPassTexture(passno, pass+passno);
	}
	/*deactivate any extras*/
	for (; passno < shaderstate.lastpasscount; passno++)
	{
		shaderstate.pendingtextures[passno] = NULL;
		shaderstate.textureschanged = true;
	}
	if (shaderstate.textureschanged)
		ID3D11DeviceContext_PSSetShaderResources(d3ddevctx, 0, max(passno, pass->numMergedPasses), shaderstate.pendingtextures);
	shaderstate.lastpasscount = pass->numMergedPasses;

	ID3D11DeviceContext_DrawIndexed(d3ddevctx, idxcount, idxfirst, 0);
	RQuantAdd(RQUANT_DRAWS, 1);
}

static void D3D11BE_Cull(unsigned int cullflags, int depthbias, float depthfactor)
{
	HRESULT hr;
	D3D11_RASTERIZER_DESC rasterdesc;
	ID3D11RasterizerState *newrasterizerstate;

	if (shaderstate.flags & BEF_FORCETWOSIDED)
		cullflags = 0;
	else if (cullflags)
		cullflags ^= r_refdef.flipcull;

#define SHADER_CULL_WIREFRAME SHADER_NODRAW //borrow a flag...
	if (shaderstate.mode == BEM_WIREFRAME)
		cullflags |= SHADER_CULL_WIREFRAME;

	if (shaderstate.curcull != cullflags || shaderstate.depthbias != depthbias || shaderstate.depthfactor != depthfactor)
	{
		shaderstate.curcull = cullflags;
		shaderstate.depthbias = depthbias;
		shaderstate.depthfactor = depthfactor;


		rasterdesc.AntialiasedLineEnable = false;

		if (shaderstate.curcull & 1)
		{
			if (shaderstate.curcull & SHADER_CULL_FRONT)
				rasterdesc.CullMode = D3D11_CULL_FRONT;
			else if (shaderstate.curcull & SHADER_CULL_BACK)
				rasterdesc.CullMode = D3D11_CULL_BACK;
			else
				rasterdesc.CullMode = D3D11_CULL_NONE;
		}
		else
		{
			if (shaderstate.curcull & SHADER_CULL_FRONT)
				rasterdesc.CullMode = D3D11_CULL_BACK;
			else if (shaderstate.curcull & SHADER_CULL_BACK)
				rasterdesc.CullMode = D3D11_CULL_FRONT;
			else
				rasterdesc.CullMode = D3D11_CULL_NONE;
		}


		rasterdesc.DepthBias = shaderstate.depthbias;
		rasterdesc.SlopeScaledDepthBias = shaderstate.depthfactor;
		rasterdesc.DepthBiasClamp = 0.0f;
		rasterdesc.DepthClipEnable = true;
		rasterdesc.FillMode = (shaderstate.curcull&SHADER_CULL_WIREFRAME)?D3D11_FILL_WIREFRAME:D3D11_FILL_SOLID;
		rasterdesc.FrontCounterClockwise = false;
		rasterdesc.MultisampleEnable = false;
		rasterdesc.ScissorEnable = true;

		if (FAILED(hr=ID3D11Device_CreateRasterizerState(pD3DDev11, &rasterdesc, &newrasterizerstate)))
		{
			if (hr == DXGI_ERROR_DEVICE_REMOVED)
			{
				hr = ID3D11Device_GetDeviceRemovedReason(pD3DDev11);
				switch(hr)
				{
				case DXGI_ERROR_DEVICE_HUNG:
					Sys_Error("DXGI_ERROR_DEVICE_HUNG\nThe application's device failed due to badly formed commands sent by the application.\n");
					break;
				case DXGI_ERROR_DEVICE_REMOVED:
					Sys_Error("DXGI_ERROR_DEVICE_REMOVED\nThe video card has been physically removed from the system, or a driver upgrade for the video card has occurred.\n");
					break;
				case DXGI_ERROR_DEVICE_RESET:
					Sys_Error("DXGI_ERROR_DEVICE_RESET\nThe device failed due to a badly formed command.\n");
					break;
				case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
					Sys_Error("DXGI_ERROR_DRIVER_INTERNAL_ERROR\nThe driver encountered a problem and was put into the device removed state.\n");
					break;
				case DXGI_ERROR_INVALID_CALL:
					Sys_Error("invalid call! oh noes!\n");
					break;
				default:
					break;
				}
			}
			else
				Con_Printf("ID3D11Device_CreateRasterizerState failed\n");
			return;
		}
		ID3D11DeviceContext_RSSetState(d3ddevctx, newrasterizerstate);
		ID3D11RasterizerState_Release(newrasterizerstate);
	}
}

static void BE_DrawMeshChain_Internal(void)
{
	shader_t *altshader;
	unsigned int vertcount, idxcount, idxfirst;
	mesh_t *m;
	qboolean vblends;	//software
//	void *map;
//	int i;
	unsigned int mno;
	unsigned int passno;
//	extern cvar_t r_polygonoffset_submodel_factor;
//	float pushdepth;
//	float pushfactor;

	altshader = shaderstate.curshader;
	switch (shaderstate.mode)
	{
	case BEM_LIGHT:
		altshader = shaderstate.shader_rtlight[shaderstate.curlmode];
		break;
	case BEM_DEPTHONLY:
		altshader = shaderstate.curshader->bemoverrides[bemoverride_depthonly];
		if (!altshader)
			altshader = shaderstate.depthonly;
		break;
	default:
	case BEM_STANDARD:
		altshader = shaderstate.curshader;
		break;
	case BEM_WIREFRAME:
		altshader = R_RegisterShader("wireframe", SUF_NONE, 
			"{\n"
				"{\n"
					"map $whiteimage\n"
				"}\n"
			"}\n"
			);
		break;
	}
	if (!altshader)
		return;

	if (0)//shaderstate.force2d)
	{
		RQuantAdd(RQUANT_2DBATCHES, 1);
	}
	else if (shaderstate.curentity == &r_worldentity)
	{
		RQuantAdd(RQUANT_WORLDBATCHES, 1);
	}
	else
	{
		RQuantAdd(RQUANT_ENTBATCHES, 1);
	}

	D3D11BE_Cull(shaderstate.curshader->flags & (SHADER_CULL_FRONT | SHADER_CULL_BACK), shaderstate.curshader->polyoffset.unit, shaderstate.curshader->polyoffset.factor);

	memset(shaderstate.stream_buffer, 0, sizeof(shaderstate.stream_buffer));

	//if this flag is set, then we have to generate our own arrays. to avoid processing extra verticies this may require that we re-pack the verts
	if (shaderstate.meshlist[0]->xyz2_array)// && !altshader->prog)
	{
		vblends = true;
		shaderstate.batchvbo = NULL;
	}
	else
	{
		vblends = false;
		if (altshader->flags & SHADER_NEEDSARRAYS)
			shaderstate.batchvbo = NULL;
	}


	/*so are index buffers*/
	if (shaderstate.batchvbo && shaderstate.nummeshes == 1)
	{
		m = shaderstate.meshlist[0];

		ID3D11DeviceContext_IASetIndexBuffer(d3ddevctx, shaderstate.batchvbo->indicies.d3d.buff, DXGI_FORMAT_INDEX_UINT, shaderstate.batchvbo->indicies.d3d.offs);
		idxfirst = m->vbofirstelement;

		vertcount = m->vbofirstvert + m->numvertexes;
		idxcount = m->numindexes;
	}
	else if (shaderstate.batchvbo)
	{	/*however, we still want to try to avoid discontinuities, because that would otherwise be more draw calls. we can have gaps in verts though*/
		index_t *map;
		ID3D11Buffer *buf;
		unsigned int i;
		unsigned int byteofs;
		vertcount = shaderstate.batchvbo->vertcount;
		for (mno = 0, idxcount = 0; mno < shaderstate.nummeshes; mno++)
		{
			m = shaderstate.meshlist[mno];
			idxcount += m->numindexes;
		}
		byteofs = allocindexbuffer(&map, &buf, idxcount);
		for (mno = 0; mno < shaderstate.nummeshes; mno++)
		{
			m = shaderstate.meshlist[mno];
			for (i = 0; i < m->numindexes; i++)
				map[i] = m->indexes[i]+m->vbofirstvert;
			map += m->numindexes;
		}
		ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
		ID3D11DeviceContext_IASetIndexBuffer(d3ddevctx, buf, DXGI_FORMAT_INDEX_UINT, byteofs);
		idxfirst = 0;
	}
	else
	{	/*we're going to be using dynamic array stuff here, so generate an index array list that has no vertex gaps*/
		index_t *map;
		ID3D11Buffer *buf;
		unsigned int i;
		unsigned int byteofs;
		for (mno = 0, vertcount = 0, idxcount = 0; mno < shaderstate.nummeshes; mno++)
		{
			m = shaderstate.meshlist[mno];
			vertcount += m->numvertexes;
			idxcount += m->numindexes;
		}

		byteofs = allocindexbuffer(&map, &buf, idxcount);
		for (mno = 0, vertcount = 0; mno < shaderstate.nummeshes; mno++)
		{
			m = shaderstate.meshlist[mno];
			for (i = 0; i < m->numindexes; i++)
				map[i] = m->indexes[i]+vertcount;
			map += m->numindexes;
			vertcount += m->numvertexes;
		}
		ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
		ID3D11DeviceContext_IASetIndexBuffer(d3ddevctx, buf, DXGI_FORMAT_INDEX_UINT, byteofs);
		idxfirst = 0;
	}

	/*vertex buffers are common to all passes*/
	if (shaderstate.batchvbo && !vblends)
	{
		shaderstate.stream_buffer[D3D11_BUFF_POS] = shaderstate.batchvbo->coord.d3d.buff;
		shaderstate.stream_offset[D3D11_BUFF_POS] = shaderstate.batchvbo->coord.d3d.offs;
		shaderstate.stream_stride[D3D11_BUFF_POS] = sizeof(dx11_vbovdata_t);
	}
	else
	{
		ID3D11Buffer *buf;
		vecV_t *map;
		const mesh_t *m;
		unsigned int mno;
		unsigned int offset, i;
		allocvertexbuffer(&buf, &offset, (void**)&map, vertcount*sizeof(vecV_t));

		if (vblends)
		{
			for (mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				const mesh_t *m = shaderstate.meshlist[mno];
				vecV_t *ov = shaderstate.curshader->numdeforms?tmpbuf:map;
				vecV_t *iv1 = m->xyz_array;
				vecV_t *iv2 = m->xyz2_array;
				float w1 = m->xyz_blendw[0];
				float w2 = m->xyz_blendw[1];
				for (i = 0; i < m->numvertexes; i++)
				{
					ov[i][0] = iv1[i][0]*w1 + iv2[i][0]*w2;
					ov[i][1] = iv1[i][1]*w1 + iv2[i][1]*w2;
					ov[i][2] = iv1[i][2]*w1 + iv2[i][2]*w2;
				}
				if (shaderstate.curshader->numdeforms)
				{
					for (i = 0; i < shaderstate.curshader->numdeforms-1; i++)
						deformgen(&shaderstate.curshader->deforms[i], m->numvertexes, tmpbuf, tmpbuf, m);
					deformgen(&shaderstate.curshader->deforms[i], m->numvertexes, tmpbuf, (vecV_t*)map, m);
				}
				map += m->numvertexes;
			}
		}
		else if (shaderstate.curshader->numdeforms > 1)
		{	//horrible code, because multiple deforms would otherwise READ from the gpu memory
			for (mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				m = shaderstate.meshlist[mno];
				deformgen(&shaderstate.curshader->deforms[0], m->numvertexes, m->xyz_array, tmpbuf, m);
				for (i = 1; i < shaderstate.curshader->numdeforms-1; i++)
					deformgen(&shaderstate.curshader->deforms[i], m->numvertexes, tmpbuf, tmpbuf, m);
				deformgen(&shaderstate.curshader->deforms[i], m->numvertexes, tmpbuf, (vecV_t*)map, m);
				map += m->numvertexes;
			}
		}
		else
		{
			for (mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				m = shaderstate.meshlist[mno];
				deformgen(&shaderstate.curshader->deforms[0], m->numvertexes, m->xyz_array, (vecV_t*)map, m);
				map += m->numvertexes;
			}
		}
		ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
		shaderstate.stream_buffer[D3D11_BUFF_POS] = buf;
		shaderstate.stream_offset[D3D11_BUFF_POS] = offset;
		shaderstate.stream_stride[D3D11_BUFF_POS] = sizeof(vecV_t);
	}

	if (altshader->prog)
	{
		if (shaderstate.batchvbo)
		{
			shaderstate.stream_buffer[D3D11_BUFF_COL] = shaderstate.batchvbo->colours[0].d3d.buff;
			shaderstate.stream_offset[D3D11_BUFF_COL] = shaderstate.batchvbo->colours[0].d3d.offs;
			shaderstate.stream_stride[D3D11_BUFF_COL] = sizeof(dx11_vbovdata_t);
			shaderstate.stream_rgbaf = false;
			shaderstate.stream_buffer[D3D11_BUFF_TC] = shaderstate.batchvbo->texcoord.d3d.buff;
			shaderstate.stream_offset[D3D11_BUFF_TC] = shaderstate.batchvbo->texcoord.d3d.offs;
			shaderstate.stream_stride[D3D11_BUFF_TC] = sizeof(dx11_vbovdata_t);
			shaderstate.stream_buffer[D3D11_BUFF_LMTC] = shaderstate.batchvbo->lmcoord[0].d3d.buff;
			shaderstate.stream_offset[D3D11_BUFF_LMTC] = shaderstate.batchvbo->lmcoord[0].d3d.offs;
			shaderstate.stream_stride[D3D11_BUFF_LMTC] = sizeof(dx11_vbovdata_t);
			shaderstate.stream_buffer[D3D11_BUFF_NORM] = shaderstate.batchvbo->normals.d3d.buff;
			shaderstate.stream_offset[D3D11_BUFF_NORM] = shaderstate.batchvbo->normals.d3d.offs;
			shaderstate.stream_stride[D3D11_BUFF_NORM] = sizeof(dx11_vbovdata_t);
		}
		else
		{
			ID3D11Buffer *buf;
			vec2_t *map;
			vec2_t *lmmap;
			const mesh_t *m;
			unsigned int mno;
			unsigned int offset, i;

			if (shaderstate.meshlist[0]->colors4f_array[0])
			{
				vec4_t *map;
				allocvertexbuffer(&buf, &offset, (void**)&map, vertcount*sizeof(*map));
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					memcpy(map, m->colors4f_array[0], sizeof(*map)*m->numvertexes);
					map += m->numvertexes;
				}
				ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
				shaderstate.stream_buffer[D3D11_BUFF_COL] = buf;
				shaderstate.stream_offset[D3D11_BUFF_COL] = offset;
				shaderstate.stream_stride[D3D11_BUFF_COL] = sizeof(*map);
				shaderstate.stream_rgbaf = true;
			}
			else if (shaderstate.meshlist[0]->colors4b_array)
			{
				byte_vec4_t *map;
				allocvertexbuffer(&buf, &offset, (void**)&map, vertcount*sizeof(byte_vec4_t));
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					for (i = 0; i < m->numvertexes; i++)
						memcpy(map, m->colors4b_array, vertcount*sizeof(byte_vec4_t));
					map += m->numvertexes;
				}
				ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
				shaderstate.stream_buffer[D3D11_BUFF_COL] = buf;
				shaderstate.stream_offset[D3D11_BUFF_COL] = offset;
				shaderstate.stream_stride[D3D11_BUFF_COL] = sizeof(byte_vec4_t);
				shaderstate.stream_rgbaf = false;
			}
			else
			{
				shaderstate.stream_buffer[D3D11_BUFF_COL] = 0;
				shaderstate.stream_offset[D3D11_BUFF_COL] = 0;
				shaderstate.stream_stride[D3D11_BUFF_COL] = 0;
				shaderstate.stream_rgbaf = false;
			}

			if (shaderstate.meshlist[0]->lmst_array[0])
			{
				allocvertexbuffer(&buf, &offset, (void**)&map, vertcount*sizeof(vec4_t));
				lmmap = map + vertcount;
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					for (i = 0; i < m->numvertexes; i++)
					{
						map[i][0] = m->st_array[i][0];
						map[i][1] = m->st_array[i][1];
						lmmap[i][0] = m->lmst_array[0][i][0];
						lmmap[i][1] = m->lmst_array[0][i][1];
					}
					map += m->numvertexes;
					lmmap += m->numvertexes;
				}
				ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
				shaderstate.stream_buffer[D3D11_BUFF_TC] = buf;
				shaderstate.stream_offset[D3D11_BUFF_TC] = offset;
				shaderstate.stream_stride[D3D11_BUFF_TC] = sizeof(vec2_t);
				shaderstate.stream_buffer[D3D11_BUFF_LMTC] = buf;
				shaderstate.stream_offset[D3D11_BUFF_LMTC] = offset+vertcount*sizeof(vec2_t);
				shaderstate.stream_stride[D3D11_BUFF_LMTC] = sizeof(vec2_t);
			}
			else
			{
				allocvertexbuffer(&buf, &offset, (void**)&map, vertcount*sizeof(vec2_t));
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					for (i = 0; i < m->numvertexes; i++)
					{
						map[i][0] = m->st_array[i][0];
						map[i][1] = m->st_array[i][1];
					}
					map += m->numvertexes;
				}
				ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
				shaderstate.stream_buffer[D3D11_BUFF_TC] = buf;
				shaderstate.stream_offset[D3D11_BUFF_TC] = offset;
				shaderstate.stream_stride[D3D11_BUFF_TC] = sizeof(vec2_t);

				shaderstate.stream_buffer[D3D11_BUFF_LMTC] = NULL;
				shaderstate.stream_offset[D3D11_BUFF_LMTC] = 0;
				shaderstate.stream_stride[D3D11_BUFF_LMTC] = 0;
			}
		}

		ID3D11DeviceContext_IASetVertexBuffers(d3ddevctx, 0, D3D11_BUFF_MAX, (ID3D11Buffer**)shaderstate.stream_buffer, shaderstate.stream_stride, shaderstate.stream_offset);
		BE_RenderMeshProgram(altshader->prog, altshader->passes, vertcount, idxfirst, idxcount);
	}
	else if (1)
	{
		shaderpass_t *p;

		//d3d11 has no fixed function pipeline. we emulate it.

		for (passno = 0; passno < altshader->numpasses; passno += p->numMergedPasses)
		{
			int emumode;
			p = &altshader->passes[passno];

			emumode = 0;
			emumode = (p->shaderbits & SBITS_ATEST_BITS) >> SBITS_ATEST_SHIFT;

			BE_GenerateColourMods(vertcount, p);

			if (shaderstate.batchvbo)
			{	//texcoords are all compatible with static arrays, supposedly
				if (p->tcgen == TC_GEN_LIGHTMAP)
				{
					shaderstate.stream_buffer[D3D11_BUFF_TC] = shaderstate.batchvbo->lmcoord[0].d3d.buff;
					shaderstate.stream_offset[D3D11_BUFF_TC] = shaderstate.batchvbo->lmcoord[0].d3d.offs;
					shaderstate.stream_stride[D3D11_BUFF_TC] = sizeof(dx11_vbovdata_t);
				}
				else if (p->tcgen == TC_GEN_BASE)
				{
					shaderstate.stream_buffer[D3D11_BUFF_TC] = shaderstate.batchvbo->texcoord.d3d.buff;
					shaderstate.stream_offset[D3D11_BUFF_TC] = shaderstate.batchvbo->texcoord.d3d.offs;
					shaderstate.stream_stride[D3D11_BUFF_TC] = sizeof(dx11_vbovdata_t);
				}
				else
				{
					Con_Printf("should be unreachable\n");
				}
			}
			else
			{
				ID3D11Buffer *buf;
				float *map;
				unsigned int offset;
				allocvertexbuffer(&buf, &offset, (void**)&map, vertcount*sizeof(vec2_t));
				BE_GenerateTCMods(p, map);
				ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
				shaderstate.stream_buffer[D3D11_BUFF_TC] = buf;
				shaderstate.stream_offset[D3D11_BUFF_TC] = offset;
				shaderstate.stream_stride[D3D11_BUFF_TC] = sizeof(vec2_t);
			}

			if (!shaderstate.programfixedemu[emumode])
			{
				char *modes[] = {
					"","#ALPHATEST=>0.0","#ALPHATEST=<0.5","#ALPHATEST=>=0.5"
				};
				shaderstate.programfixedemu[emumode] = Shader_FindGeneric(va("fixedemu%s", modes[emumode]), QR_DIRECT3D11);
				if (!shaderstate.programfixedemu[emumode])
					break;
			}

			ID3D11DeviceContext_IASetVertexBuffers(d3ddevctx, 0, D3D11_BUFF_MAX, (ID3D11Buffer**)shaderstate.stream_buffer, shaderstate.stream_stride, shaderstate.stream_offset);
			BE_RenderMeshProgram(shaderstate.programfixedemu[emumode], p, vertcount, idxfirst, idxcount);
		}
	}
}

void D3D11BE_SelectMode(backendmode_t mode)
{
	shaderstate.mode = mode;

	if (mode == BEM_STENCIL)
		D3D11BE_ApplyShaderBits(SBITS_MASK_BITS, NULL);
}
qboolean D3D11BE_GenerateRTLightShader(unsigned int lmode)
{
	if (!shaderstate.shader_rtlight[lmode])
	{
		shaderstate.shader_rtlight[lmode] = R_RegisterShader(va("rtlight%s%s%s", 
															(lmode & LSHADER_SMAP)?"#PCF":"",
															(lmode & LSHADER_SPOT)?"#SPOT":"",
															(lmode & LSHADER_CUBE)?"#CUBE":"")
														, SUF_NONE, LIGHTPASS_SHADER);
	}
	if (!shaderstate.shader_rtlight[lmode]->prog)
		return false;
	return true;
}
qboolean D3D11BE_SelectDLight(dlight_t *dl, vec3_t colour, vec3_t axis[3], unsigned int lmode)
{
	if (!D3D11BE_GenerateRTLightShader(lmode))
	{
		lmode &= ~(LSHADER_SMAP|LSHADER_CUBE);
		if (!D3D11BE_GenerateRTLightShader(lmode))
			return false;
	}
	shaderstate.curdlight = dl;
	shaderstate.curlmode = lmode;
	VectorCopy(colour, shaderstate.curdlight_colours);

	D3D11BE_SetupLightCBuffer(dl, colour);
	return true;
}

#ifdef RTLIGHTS
void D3D11BE_SetupForShadowMap(dlight_t *dl, int texwidth, int texheight, float shadowscale)
{
#define SHADOWMAP_SIZE 512
	extern cvar_t r_shadow_shadowmapping_nearclip, r_shadow_shadowmapping_bias;
	float nc = r_shadow_shadowmapping_nearclip.value;
	float bias = r_shadow_shadowmapping_bias.value;

	//much of the projection matrix cancels out due to symmetry and stuff
	//we need to scale between -0.5,0.5 within the sub-image. the fragment shader will center on the subimage based upon the major axis.
	//in d3d, the depth value is scaled between 0 and 1 (gl is -1 to 1).
	//d3d's framebuffer is upside down or something annoying like that.
	shaderstate.lightshadowmapproj[0] = shadowscale * (1.0-(1.0/texwidth)) * 0.5/3.0;	//pinch x inwards
	shaderstate.lightshadowmapproj[1] = -shadowscale * (1.0-(1.0/texheight)) * 0.5/2.0;	//pinch y inwards
	shaderstate.lightshadowmapproj[2] = 0.5*(dl->radius+nc)/(nc-dl->radius);	//proj matrix 10
	shaderstate.lightshadowmapproj[3] = (dl->radius*nc)/(nc-dl->radius) - bias*nc*(1024/texheight);	//proj matrix 14	

	shaderstate.lightshadowmapscale[0] = 1.0/(SHADOWMAP_SIZE*3);
	shaderstate.lightshadowmapscale[1] = -1.0/(SHADOWMAP_SIZE*2);
}
#endif

void D3D11BE_SelectEntity(entity_t *ent)
{
	BE_RotateForEntity(ent, ent->model);
}

void D3D11BE_GenBatchVBOs(vbo_t **vbochain, batch_t *firstbatch, batch_t *stopbatch)
{
	int maxvboelements;
	int maxvboverts;
	int vert = 0, idx = 0;
	batch_t *batch;
	vbo_t *vbo;
	int i, j;
	mesh_t *m;
	ID3D11Buffer *vbuff;
	ID3D11Buffer *ebuff;
	index_t *vboedata, *vboedatastart;
	dx11_vbovdata_t *vbovdata, *vbovdatastart;
	D3D11_BUFFER_DESC vbodesc;
	D3D11_BUFFER_DESC ebodesc;
	D3D11_SUBRESOURCE_DATA srd;

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

	vbovdatastart = vbovdata = BZ_Malloc(sizeof(*vbovdata) * maxvboverts);
	vboedatastart = vboedata = BZ_Malloc(sizeof(*vboedata) * maxvboelements);

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
				vbovdata->coord[3] = 1;
				Vector2Copy(m->st_array[i],			vbovdata->tex);
				if (m->lmst_array[0])
					Vector2Copy(m->lmst_array[0][i],		vbovdata->lm);
				else
					Vector2Copy(m->st_array[i],			vbovdata->tex);
				if (m->normals_array)
					VectorCopy(m->normals_array[i],		vbovdata->ndir);
				else
					VectorSet(vbovdata->ndir, 0, 0, 1);
				if (m->snormals_array)
					VectorCopy(m->snormals_array[i],	vbovdata->sdir);
				else
					VectorSet(vbovdata->sdir, 1, 0, 0);
				if (m->tnormals_array)
					VectorCopy(m->tnormals_array[i],	vbovdata->tdir);
				else
					VectorSet(vbovdata->tdir, 0, 1, 0);
				if (m->colors4f_array[0])
					Vector4Scale(m->colors4f_array[0][i],	255, vbovdata->colorsb);
				else if (m->colors4b_array)
					Vector4Copy(m->colors4b_array[i],	vbovdata->colorsb);
				else
					Vector4Set(vbovdata->colorsb, 255, 255, 255, 255);

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

	//generate the ebo, and submit the data to the driver
	ebodesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ebodesc.ByteWidth = sizeof(*vboedata) * maxvboelements;
	ebodesc.CPUAccessFlags = 0;
	ebodesc.MiscFlags = 0;
	ebodesc.StructureByteStride = 0;
	ebodesc.Usage = D3D11_USAGE_DEFAULT;
	srd.pSysMem = vboedatastart;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;
	ID3D11Device_CreateBuffer(pD3DDev11, &ebodesc, &srd, &ebuff);
	shaderstate.numlivevbos++;
	BZ_Free(vboedatastart);

	//generate the vbo, and submit the data to the driver
	vbodesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbodesc.ByteWidth = sizeof(*vbovdata) * maxvboverts;
	vbodesc.CPUAccessFlags = 0;
	vbodesc.MiscFlags = 0;
	vbodesc.StructureByteStride = 0;
	vbodesc.Usage = D3D11_USAGE_DEFAULT;
	srd.pSysMem = vbovdatastart;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;
	ID3D11Device_CreateBuffer(pD3DDev11, &vbodesc, &srd, &vbuff);
	shaderstate.numlivevbos++;
	BZ_Free(vbovdatastart);

	vbovdata = NULL;
	vbo->coord.d3d.buff = vbuff;
	vbo->coord.d3d.offs = (quintptr_t)&vbovdata->coord;
	vbo->texcoord.d3d.buff = vbuff;
	vbo->texcoord.d3d.offs = (quintptr_t)&vbovdata->tex;
	vbo->lmcoord[0].d3d.buff = vbuff;
	vbo->lmcoord[0].d3d.offs = (quintptr_t)&vbovdata->lm;
	vbo->normals.d3d.buff = vbuff;
	vbo->normals.d3d.offs = (quintptr_t)&vbovdata->ndir;
	vbo->svector.d3d.buff = vbuff;
	vbo->svector.d3d.offs = (quintptr_t)&vbovdata->sdir;
	vbo->tvector.d3d.buff = vbuff;
	vbo->tvector.d3d.offs = (quintptr_t)&vbovdata->tdir;
	vbo->colours[0].d3d.buff = vbuff;
	vbo->colours[0].d3d.offs = (quintptr_t)&vbovdata->colorsb;
	vbo->indicies.d3d.buff = ebuff;
	vbo->indicies.d3d.offs = 0;

	vbo->indexcount = maxvboelements;
	vbo->vertcount = maxvboverts;

	vbo->next = *vbochain;
	*vbochain = vbo;
}

void D3D11BE_GenBrushModelVBO(model_t *mod)
{
	unsigned int vcount, cvcount;


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
			for (i = 0, cvcount = 0; i < batch->maxmeshes; i++)
				cvcount += batch->mesh[i]->numvertexes;

			//firstmesh got reused as the number of verticies in each batch
			if (vcount + cvcount > MAX_INDICIES)
			{
				D3D11BE_GenBatchVBOs(&mod->vbos, fbatch, batch);
				fbatch = batch;
				vcount = 0;
			}
			vcount += cvcount;
		}

		D3D11BE_GenBatchVBOs(&mod->vbos, fbatch, batch);
	}
}

/*Wipes a vbo*/
void D3D11BE_ClearVBO(vbo_t *vbo, qboolean dataonly)
{
	ID3D11Buffer *vbuff = vbo->coord.d3d.buff;
	ID3D11Buffer *ebuff = vbo->indicies.d3d.buff;
	if (vbuff)
	{
		ID3D11Buffer_Release(vbuff);
		shaderstate.numlivevbos--;
	}
	if (ebuff)
	{
		ID3D11Buffer_Release(ebuff);
		shaderstate.numlivevbos--;
	}
	vbo->coord.d3d.buff = NULL;
	vbo->indicies.d3d.buff = NULL;

	if (!dataonly)
		BZ_Free(vbo);
}

/*upload all lightmaps at the start to reduce lags*/
static void BE_UploadLightmaps(qboolean force)
{
	int i;

	for (i = 0; i < numlightmaps; i++)
	{
		if (!lightmap[i])
			continue;

		if (force && !lightmap[i]->external)
		{
			lightmap[i]->rectchange.l = 0;
			lightmap[i]->rectchange.t = 0;
			lightmap[i]->rectchange.r = lightmap[i]->width;
			lightmap[i]->rectchange.b = lightmap[i]->height;
			lightmap[i]->modified = true;
		}

		if (lightmap[i]->modified)
		{
			D3D11_UploadLightmap(lightmap[i]);
		}
	}
}

void D3D11BE_UploadAllLightmaps(void)
{
	BE_UploadLightmaps(true);
}

qboolean D3D11BE_LightCullModel(vec3_t org, model_t *model)
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

batch_t *D3D11BE_GetTempBatch(void)
{
	if (shaderstate.wbatch >= shaderstate.maxwbatches)
	{
		shaderstate.wbatch++;
		return NULL;
	}
	return &shaderstate.wbatches[shaderstate.wbatch++];
}

/*static float projd3dtogl[16] = 
{
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 2.0, 0.0,
	0.0, 0.0, -1.0, 1.0
};*/
static float projgltod3d[16] = 
{
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 0.5, 0.0,
	0.0, 0.0, 0.5, 1.0
};
static void D3D11BE_SetupViewCBuffer(void)
{
	dx11_cbuf_view_t *cbv;
	D3D11_MAPPED_SUBRESOURCE msr;
	if (FAILED(ID3D11DeviceContext_Map(d3ddevctx, (ID3D11Resource*)shaderstate.vcbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
	{
		Con_Printf("BE_RotateForEntity: failed to map constant buffer\n");
		return;
	}
	cbv = (dx11_cbuf_view_t*)msr.pData;

	//we internally use gl-style projection matricies.
	//gl's viewport is based upon -1 to 1 depth.
	//d3d uses 0 to 1 depth.
	//so we scale the projection matrix by a bias
#if 1
	Matrix4_Multiply(projgltod3d, (shaderstate.depthrange<1)?r_refdef.m_projection_view:r_refdef.m_projection_std, cbv->m_projection);
#else
	memcpy(cbv->m_projection, r_refdef.m_projection_std, sizeof(cbv->m_projection));
	cbv->m_projection[10] = r_refdef.m_projection_std[10] * 0.5;
#endif
	memcpy(cbv->m_view, r_refdef.m_view, sizeof(cbv->m_view));
	VectorCopy(r_origin, cbv->v_eyepos);
	cbv->v_time = r_refdef.time;

	ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)shaderstate.vcbuffer, 0);
}
void D3D11BE_Set2D(void)
{
	D3D11_VIEWPORT vport;
	vport.TopLeftX = 0;
	vport.TopLeftY = 0;
	vport.Width = vid.pixelwidth;
	vport.Height = vid.pixelheight;
	vport.MinDepth = 0;
	vport.MaxDepth = shaderstate.depthrange = 1;

	ID3D11DeviceContext_RSSetViewports(d3ddevctx, 1, &vport);
	D3D11BE_SetupViewCBuffer();
	D3D11BE_Scissor(NULL);
}
void D3D11BE_SetupLightCBuffer(dlight_t *l, vec3_t colour)
{
	extern cvar_t gl_specular;
	dx11_cbuf_light_t *cbl;
	D3D11_MAPPED_SUBRESOURCE msr;
	if (FAILED(ID3D11DeviceContext_Map(d3ddevctx, (ID3D11Resource*)shaderstate.lcbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
	{
		Con_Printf("BE_RotateForEntity: failed to map constant buffer\n");
		return;
	}
	cbl = (dx11_cbuf_light_t*)msr.pData;

	cbl->l_lightradius = l->radius;

	Matrix4x4_CM_LightMatrixFromAxis(cbl->l_cubematrix, l->axis[0], l->axis[1], l->axis[2], l->origin);
	VectorCopy(l->origin, cbl->l_lightposition);
	cbl->padl1 = 0;
	VectorCopy(colour, cbl->l_colour);
#ifdef RTLIGHTS
	VectorCopy(l->lightcolourscales, cbl->l_lightcolourscale);
	cbl->l_lightcolourscale[0] = l->lightcolourscales[0];
	cbl->l_lightcolourscale[1] = l->lightcolourscales[1];
	cbl->l_lightcolourscale[2] = l->lightcolourscales[2] * gl_specular.value;
#endif
	cbl->l_lightradius = l->radius;
	Vector4Copy(shaderstate.lightshadowmapproj, cbl->l_shadowmapproj);
	Vector2Copy(shaderstate.lightshadowmapscale, cbl->l_shadowmapscale);

	ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)shaderstate.lcbuffer, 0);
}

static void R_FetchPlayerColour(unsigned int cv, vec4_t rgba)
{
	int i;

	if (cv >= 16)
	{
		rgba[0] = (((cv&0xff0000)>>16)**((unsigned char*)&d_8to24rgbtable[15]+0)) / (256.0*256);
		rgba[1] = (((cv&0x00ff00)>>8)**((unsigned char*)&d_8to24rgbtable[15]+1)) / (256.0*256);
		rgba[2] = (((cv&0x0000ff)>>0)**((unsigned char*)&d_8to24rgbtable[15]+2)) / (256.0*256);
		rgba[3] = 1.0;
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
	rgba[0] = host_basepal[i+0] / 255.0;
	rgba[1] = host_basepal[i+1] / 255.0;
	rgba[2] = host_basepal[i+2] / 255.0;
	rgba[3] = 1.0;
}

//also updates the entity constant buffer
static void BE_RotateForEntity (const entity_t *e, const model_t *mod)
{
	int i;
	float ndr;
	float modelinv[16];
	float *m = shaderstate.m_model;
	dx11_cbuf_entity_t *cbe;
	D3D11_MAPPED_SUBRESOURCE msr;
	shaderstate.ecbufferidx = (shaderstate.ecbufferidx + 1) & (NUMECBUFFERS-1);
	if (FAILED(ID3D11DeviceContext_Map(d3ddevctx, (ID3D11Resource*)shaderstate.ecbuffers[shaderstate.ecbufferidx], 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
	{
		Con_Printf("BE_RotateForEntity: failed to map constant buffer\n");
		return;
	}
	cbe = (dx11_cbuf_entity_t*)msr.pData;


	shaderstate.curentity = e;


	if ((e->flags & RF_WEAPONMODEL) && r_refdef.playerview->viewentity > 0)
	{
		float em[16];
		float vm[16];

		if (e->flags & RF_WEAPONMODELNOBOB)
		{
			vm[0] = r_refdef.weaponmatrix[0][0];
			vm[1] = r_refdef.weaponmatrix[0][1];
			vm[2] = r_refdef.weaponmatrix[0][2];
			vm[3] = 0;

			vm[4] = r_refdef.weaponmatrix[1][0];
			vm[5] = r_refdef.weaponmatrix[1][1];
			vm[6] = r_refdef.weaponmatrix[1][2];
			vm[7] = 0;

			vm[8] = r_refdef.weaponmatrix[2][0];
			vm[9] = r_refdef.weaponmatrix[2][1];
			vm[10] = r_refdef.weaponmatrix[2][2];
			vm[11] = 0;

			vm[12] = r_refdef.weaponmatrix[3][0];
			vm[13] = r_refdef.weaponmatrix[3][1];
			vm[14] = r_refdef.weaponmatrix[3][2];
			vm[15] = 1;
		}
		else
		{
			vm[0] = r_refdef.weaponmatrix_bob[0][0];
			vm[1] = r_refdef.weaponmatrix_bob[0][1];
			vm[2] = r_refdef.weaponmatrix_bob[0][2];
			vm[3] = 0;

			vm[4] = r_refdef.weaponmatrix_bob[1][0];
			vm[5] = r_refdef.weaponmatrix_bob[1][1];
			vm[6] = r_refdef.weaponmatrix_bob[1][2];
			vm[7] = 0;

			vm[8] = r_refdef.weaponmatrix_bob[2][0];
			vm[9] = r_refdef.weaponmatrix_bob[2][1];
			vm[10] = r_refdef.weaponmatrix_bob[2][2];
			vm[11] = 0;

			vm[12] = r_refdef.weaponmatrix_bob[3][0];
			vm[13] = r_refdef.weaponmatrix_bob[3][1];
			vm[14] = r_refdef.weaponmatrix_bob[3][2];
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

	memcpy(cbe->m_model, m, sizeof(cbe->m_model));

	Matrix4_Invert(shaderstate.m_model, modelinv);

	cbe->e_time = r_refdef.time - shaderstate.curentity->shaderTime;

	VectorCopy(e->light_avg, cbe->e_light_ambient);
	VectorCopy(e->light_dir, cbe->e_light_dir);
	VectorCopy(e->light_range, cbe->e_light_mul);

	R_FetchPlayerColour(e->topcolour, cbe->e_uppercolour);
	R_FetchPlayerColour(e->bottomcolour, cbe->e_lowercolour);
	R_FetchPlayerColour(e->bottomcolour, cbe->e_colourmod);
	if (shaderstate.flags & BEF_FORCECOLOURMOD)
		Vector4Copy(e->shaderRGBAf, cbe->e_colourmod);
	else
		Vector4Set(cbe->e_colourmod, 1, 1, 1, e->shaderRGBAf[3]);
	VectorCopy(e->glowmod, cbe->e_glowmod);cbe->e_glowmod[3] = 1;

	//various stuff in modelspace
	Matrix4x4_CM_Transform3(modelinv, r_origin, cbe->e_eyepos);

	for (i = 0; i < MAXRLIGHTMAPS ; i++)
	{
		extern cvar_t gl_overbright;
		unsigned char s = shaderstate.curbatch?shaderstate.curbatch->lmlightstyle[i]:0;
		float sc;
		if (s == 255)
		{
			if (i == 0)
			{
				if (shaderstate.curentity->model && shaderstate.curentity->model->engineflags & MDLF_NEEDOVERBRIGHT)
					sc = (1<<bound(0, gl_overbright.ival, 2)) * shaderstate.identitylighting;
				else
					sc = shaderstate.identitylighting;
				cbe->e_lmscale[i][0] = sc;
				cbe->e_lmscale[i][1] = sc;
				cbe->e_lmscale[i][2] = sc;
				cbe->e_lmscale[i][3] = 1;
				i++;
			}
			for (; i < MAXRLIGHTMAPS ; i++)
			{
				cbe->e_lmscale[i][0] = 0;
				cbe->e_lmscale[i][1] = 0;
				cbe->e_lmscale[i][2] = 0;
				cbe->e_lmscale[i][3] = 1;
			}
			break;
		}
		if (shaderstate.curentity->model && shaderstate.curentity->model->engineflags & MDLF_NEEDOVERBRIGHT)
			sc = (1<<bound(0, gl_overbright.ival, 2)) * shaderstate.identitylighting;
		else
			sc = shaderstate.identitylighting;
		sc *= d_lightstylevalue[s]/256.0f;
		Vector4Set(cbe->e_lmscale[i], sc, sc, sc, 1);
	}

	ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)shaderstate.ecbuffers[shaderstate.ecbufferidx], 0);

	ndr = (e->flags & RF_DEPTHHACK)?0.333:1;
	if (ndr != shaderstate.depthrange)
	{
		D3D11_VIEWPORT vport;

		shaderstate.depthrange = ndr;

		vport.TopLeftX = r_refdef.pxrect.x;
		vport.TopLeftY = r_refdef.pxrect.y;
		vport.Width = r_refdef.pxrect.width;
		vport.Height = r_refdef.pxrect.height;
		vport.MinDepth = 0;
		vport.MaxDepth = shaderstate.depthrange;
		ID3D11DeviceContext_RSSetViewports(d3ddevctx, 1, &vport);

		D3D11BE_SetupViewCBuffer();
	}
}

void D3D11BE_SubmitBatch(batch_t *batch)
{
	shader_t *shader = batch->shader;
	shaderstate.nummeshes = batch->meshes - batch->firstmesh;
	if (!shaderstate.nummeshes)
		return;
	shaderstate.curbatch = batch;
	shaderstate.batchvbo = batch->vbo;
	shaderstate.meshlist = batch->mesh + batch->firstmesh;
	shaderstate.curshader = shader;
	if (shaderstate.curentity != batch->ent)
	{
		BE_RotateForEntity(batch->ent, batch->ent->model);
		shaderstate.curtime = r_refdef.time - shaderstate.curentity->shaderTime;
	}
	if (batch->skin)
		shaderstate.curtexnums = batch->skin;
	else if (shader->numdefaulttextures)
		shaderstate.curtexnums = shader->defaulttextures + ((int)(shader->defaulttextures_fps * shaderstate.curtime) % shader->numdefaulttextures);
	else
		shaderstate.curtexnums = shader->defaulttextures;
	shaderstate.flags = batch->flags;

	BE_DrawMeshChain_Internal();
}

void D3D11BE_DrawMesh_List(shader_t *shader, int nummeshes, mesh_t **meshlist, vbo_t *vbo, texnums_t *texnums, unsigned int beflags)
{
	shaderstate.curbatch = &shaderstate.dummybatch;
	shaderstate.batchvbo = vbo;
	shaderstate.curshader = shader;
	if (texnums)
		shaderstate.curtexnums = texnums;
	else if (shader->numdefaulttextures)
		shaderstate.curtexnums = shader->defaulttextures + ((int)(shader->defaulttextures_fps * shaderstate.curtime) % shader->numdefaulttextures);
	else
		shaderstate.curtexnums = shader->defaulttextures;
	shaderstate.meshlist = meshlist;
	shaderstate.nummeshes = nummeshes;
	shaderstate.flags = beflags;

	BE_DrawMeshChain_Internal();
}

void D3D11BE_DrawMesh_Single(shader_t *shader, mesh_t *meshchain, vbo_t *vbo, unsigned int beflags)
{
	shaderstate.curbatch = &shaderstate.dummybatch;
	shaderstate.batchvbo = vbo;
	shaderstate.curtime = realtime;
	shaderstate.curshader = shader;
	if (shader->numdefaulttextures)
		shaderstate.curtexnums = shader->defaulttextures + ((int)(shader->defaulttextures_fps * shaderstate.curtime) % shader->numdefaulttextures);
	else
		shaderstate.curtexnums = shader->defaulttextures;
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
			if (shaderstate.mode == BEM_STANDARD || shaderstate.mode == BEM_DEPTHDARK)
			{
				if (R_DrawSkyChain (batch))
					continue;
			}
			else if (shaderstate.mode != BEM_FOG && shaderstate.mode != BEM_CREPUSCULAR && shaderstate.mode != BEM_WIREFRAME)
				continue;
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
	//for q3-compat, portals on world scan for a visedict to use for their view.
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

static void R_DrawPortal(batch_t *batch, batch_t **blist)
{
	entity_t *view;
//	float glplane[4];
	plane_t plane;
	refdef_t oldrefdef;
	mesh_t *mesh = batch->mesh[batch->firstmesh];
	int sort;

	if (r_refdef.recurse)// || !r_portalrecursion.ival)
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



	R_SetFrustum (r_refdef.m_projection_std, r_refdef.m_view);
	/*FIXME: we need to borrow pretty much everything about portal positioning from the gl renderer. make it common code or something, because this is horrendous.
	if (r_refdef.frustum_numplanes < MAXFRUSTUMPLANES)
	{
		extern int SignbitsForPlane (mplane_t *out);
		mplane_t fp;
		VectorCopy(plane.normal, fp.normal);
		fp.dist = plane.dist;

		fp.type = PLANE_ANYZ;
		fp.signbits = SignbitsForPlane (&fp);

		if (portaltype == 1 || portaltype == 2)
			R_ObliqueNearClip(r_refdef.m_view, &fp);

		//our own culling should be an epsilon forwards so we don't still draw things behind the line due to precision issues.
		fp.dist += 0.01;
		r_refdef.frustum[r_refdef.frustum_numplanes++] = fp;
	}
	*/

	shaderstate.curentity = NULL;
	Surf_SetupFrame();
	Surf_DrawWorld();

	for (sort = 0; sort < SHADER_SORT_COUNT; sort++)
	for (batch = blist[sort]; batch; batch = batch->next)
	{
		batch->firstmesh = 0;
	}
	r_refdef = oldrefdef;

	/*broken stuff*/
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
	VectorCopy (r_refdef.vieworg, r_origin);

	R_SetFrustum (r_refdef.m_projection_std, r_refdef.m_view);
	shaderstate.curentity = NULL;

	D3D11BE_SetupViewCBuffer();

	//fixme: should be some other variable...
	ID3D11DeviceContext_ClearDepthStencilView(d3ddevctx, fb_backdepthstencil, D3D11_CLEAR_DEPTH, 1, 0);	//is it faster to clear the stencil too?
	//fixme: mask the batch's depth so that later batches don't break it.
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
			}
		}
	}
}

void D3D11BE_SubmitMeshes (batch_t **worldbatches, batch_t **blist, int first, int stop)
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
void D3D11BE_BaseEntTextures(const qbyte *worldpvs, const int *worldareas)
{
	batch_t *batches[SHADER_SORT_COUNT];
	BE_GenModelBatches(batches, shaderstate.curdlight, shaderstate.mode, worldpvs, worldareas);
	D3D11BE_SubmitMeshes(NULL, batches, SHADER_SORT_PORTAL, SHADER_SORT_SEETHROUGH+1);
	BE_SelectEntity(&r_worldentity);
}

void D3D11BE_GenerateShadowBuffer(void **vbuf_out, vecV_t *verts, int numverts, void **ibuf_out, index_t *indicies, int numindicies)
{
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA srd;
	ID3D11Buffer *vbuf;
	ID3D11Buffer *ibuf;


	//generate the ebo, and submit the data to the driver
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = sizeof(*verts) * numverts;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	srd.pSysMem = verts;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;
	ID3D11Device_CreateBuffer(pD3DDev11, &desc, &srd, &vbuf);

	//generate the vbo, and submit the data to the driver
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.ByteWidth = sizeof(*indicies) * numindicies;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	srd.pSysMem = indicies;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;
	ID3D11Device_CreateBuffer(pD3DDev11, &desc, &srd, &ibuf);

	shaderstate.numliveshadowbuffers++;

	*vbuf_out = vbuf;
	*ibuf_out = ibuf;
}
void D3D11_DestroyShadowBuffer(void *vbuf_in, void *ibuf_in)
{
	ID3D11Buffer *vbuf = vbuf_in;
	ID3D11Buffer *ibuf = ibuf_in;

	if (vbuf && ibuf)
	{
		ID3D11Buffer_Release(vbuf);
		ID3D11Buffer_Release(ibuf);
		shaderstate.numliveshadowbuffers--;
	}
}
//draws all depth-only surfaces from the perspective of the light.
void D3D11BE_RenderShadowBuffer(unsigned int numverts, void *vbuf, unsigned int numindicies, void *ibuf)
{
	ID3D11Buffer *vbufs[] = {vbuf};
	int vstrides[] = {sizeof(vecV_t)};
	int voffsets[] = {0};
	int i;

	if (!shaderstate.depthonly->prog)
		return;

	D3D11BE_SetupViewCBuffer();

	D3D11BE_Cull(SHADER_CULL_FRONT, r_polygonoffset_shadowmap_offset.ival, r_polygonoffset_shadowmap_factor.value);

	for (i = 0; i < shaderstate.lastpasscount; i++)
	{
		shaderstate.pendingtextures[i] = NULL;
		shaderstate.textureschanged = true;
	}
	if (shaderstate.textureschanged)
		ID3D11DeviceContext_PSSetShaderResources(d3ddevctx, 0, shaderstate.lastpasscount, shaderstate.pendingtextures);
	shaderstate.lastpasscount = 0;

	ID3D11DeviceContext_IASetVertexBuffers(d3ddevctx, 0, 1, vbufs, vstrides, voffsets);
	ID3D11DeviceContext_IASetIndexBuffer(d3ddevctx, ibuf, DXGI_FORMAT_INDEX_UINT, 0);

	BE_ApplyUniforms(shaderstate.depthonly->prog, shaderstate.depthonly->prog->permu[0]);

	ID3D11DeviceContext_DrawIndexed(d3ddevctx, numindicies, 0, 0);
}
void D3D11BE_DoneShadows(void)
{
	D3D11BE_SetupViewCBuffer();
	BE_SelectEntity(&r_worldentity);

	D3D11BE_BeginShadowmapFace();
}
#endif

void D3D11BE_DrawWorld (batch_t **worldbatches)
{
	batch_t *batches[SHADER_SORT_COUNT];
	RSpeedLocals();

	shaderstate.curentity = NULL;

	shaderstate.depthrange = 0;

	if (!r_refdef.recurse)
	{
		if (shaderstate.wbatch > shaderstate.maxwbatches)
		{
			int newm = shaderstate.wbatch;
			Z_Free(shaderstate.wbatches);
			shaderstate.wbatches = Z_Malloc(newm * sizeof(*shaderstate.wbatches));
			memset(shaderstate.wbatches + shaderstate.maxwbatches, 0, (newm - shaderstate.maxwbatches) * sizeof(*shaderstate.wbatches));
			shaderstate.maxwbatches = newm;
		}
		shaderstate.wbatch = 0;
	}

	D3D11BE_SetupViewCBuffer();

	shaderstate.curdlight = NULL;
	BE_GenModelBatches(batches, shaderstate.curdlight, BEM_STANDARD, r_refdef.scenevis, r_refdef.sceneareas);

	if (r_refdef.scenevis)
	{
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
		if (r_refdef.scenevis && r_shadow_realtime_world.ival)
			shaderstate.identitylighting = r_shadow_realtime_world_lightmaps.value;
		else
#endif
			shaderstate.identitylighting = r_lightmap_scale.value;
		shaderstate.identitylighting *= r_refdef.hdr_value;
//		shaderstate.identitylightmap = shaderstate.identitylighting / (1<<gl_overbright.ival);

		BE_SelectMode(BEM_STANDARD);

		RSpeedRemark();
		D3D11BE_SubmitMeshes(worldbatches, batches, SHADER_SORT_PORTAL, SHADER_SORT_SEETHROUGH+1);
		RSpeedEnd(RSPEED_OPAQUE);

#ifdef RTLIGHTS
		RSpeedRemark();
		D3D11BE_SelectEntity(&r_worldentity);
		Sh_DrawLights(r_refdef.scenevis);
		RSpeedEnd(RSPEED_RTLIGHTS);
#endif

		RSpeedRemark();
		D3D11BE_SubmitMeshes(worldbatches, batches, SHADER_SORT_SEETHROUGH+1, SHADER_SORT_COUNT);
		RSpeedEnd(RSPEED_TRANSPARENTS);

		if (r_wireframe.ival)
		{
			D3D11BE_SelectMode(BEM_WIREFRAME);
			D3D11BE_SubmitMeshes(worldbatches, batches, SHADER_SORT_PORTAL, SHADER_SORT_NEAREST);
			D3D11BE_SelectMode(BEM_STANDARD);
		}
	}
	else
	{
		RSpeedRemark();
		shaderstate.identitylighting = 1;
		D3D11BE_SubmitMeshes(NULL, batches, SHADER_SORT_PORTAL, SHADER_SORT_COUNT);
		RSpeedEnd(RSPEED_OPAQUE);
	}

	R_RenderDlights ();

	BE_RotateForEntity(&r_worldentity, NULL);
}

void D3D11BE_VBO_Begin(vbobctx_t *ctx, size_t maxsize)
{
}
void D3D11BE_VBO_Data(vbobctx_t *ctx, void *data, size_t size, vboarray_t *varray)
{
}
void D3D11BE_VBO_Finish(vbobctx_t *ctx, void *edata, size_t esize, vboarray_t *earray, void **vbomem, void **ebomem)
{
}
void D3D11BE_VBO_Destroy(vboarray_t *vearray, void *mem)
{
}

void D3D11BE_Scissor(srect_t *rect)
{
	D3D11_RECT drect;
	if (rect)
	{
		drect.left = (rect->x)*vid.fbpwidth;
		drect.right = (rect->x + rect->width)*vid.fbpwidth;
		drect.bottom = (1-(rect->y))*vid.fbpheight;
		drect.top = (1-(rect->y + rect->height))*vid.fbpheight;

		if (drect.left < 0)
			drect.left = 0;
		if (drect.top < 0)
			drect.top = 0;
	}
	else
	{
		drect.left = 0;
		drect.right = vid.fbpwidth;
		drect.top = 0;
		drect.bottom = vid.fbpheight;
	}
	ID3D11DeviceContext_RSSetScissorRects(d3ddevctx, 1, &drect);
}

#ifdef RTLIGHTS
void D3D11BE_BeginShadowmapFace(void)
{
	D3D11_VIEWPORT vport;

	vport.TopLeftX = r_refdef.pxrect.x;
	vport.TopLeftY = r_refdef.pxrect.y;
	vport.Width = r_refdef.pxrect.width;
	vport.Height = r_refdef.pxrect.height;
	vport.MinDepth = 0;
	vport.MaxDepth = 1;
	ID3D11DeviceContext_RSSetViewports(d3ddevctx, 1, &vport);

	D3D11BE_Cull(SHADER_CULL_FRONT, r_polygonoffset_shadowmap_offset.ival, r_polygonoffset_shadowmap_factor.value);
}
#endif

#endif
