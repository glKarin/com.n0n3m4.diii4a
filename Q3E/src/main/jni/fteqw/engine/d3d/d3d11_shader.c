#include "quakedef.h"

#ifdef D3D11QUAKE
#include "shader.h"
#include "winquake.h"
#define COBJMACROS
#include <d3d11.h>
extern ID3D11Device *pD3DDev11;

//#include <D3D11Shader.h>	//apparently requires win8 sdk, despite being a win7 thing.

#ifndef IID_ID3DBlob
	//microsoft can be such a pain sometimes.
	typedef struct _D3D_SHADER_MACRO
	{
		LPCSTR Name;
		LPCSTR Definition;

	} D3D_SHADER_MACRO, *LPD3D_SHADER_MACRO;

	typedef enum _D3D_INCLUDE_TYPE { 
	  D3D_INCLUDE_LOCAL         = 0,
	  D3D_INCLUDE_SYSTEM        = ( D3D_INCLUDE_LOCAL + 1 ),
	  D3D_INCLUDE_FORCE_DWORD   = 0x7fffffff 
	} D3D_INCLUDE_TYPE;

	#undef INTERFACE
	#define INTERFACE ID3DInclude
	DECLARE_INTERFACE_(INTERFACE, IUnknown)
	{
		STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) PURE;
		STDMETHOD(Close)(THIS_ LPCVOID pData) PURE;
	};

	#undef INTERFACE
	#define INTERFACE ID3DBlob
	DECLARE_INTERFACE_(INTERFACE, IUnknown)
	{
		STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
		STDMETHOD_(ULONG, AddRef)(THIS) PURE;
		STDMETHOD_(ULONG, Release)(THIS) PURE;
		STDMETHOD_(LPVOID, GetBufferPointer)(THIS) PURE;
		STDMETHOD_(SIZE_T, GetBufferSize)(THIS) PURE;
	};
	#undef INTERFACE
/*
	#define D3D11_SHADER_VARIABLE_DESC void
	typedef unsigned int D3D_SHADER_INPUT_TYPE;
	typedef unsigned int D3D_RESOURCE_RETURN_TYPE;
	typedef unsigned int D3D_SRV_DIMENSION;
	typedef struct D3D11_SHADER_INPUT_BIND_DESC {
		LPCSTR						Name;
		D3D_SHADER_INPUT_TYPE		Type;
		UINT						BindPoint;
		UINT						BindCount;
		UINT						uFlags;
		D3D_RESOURCE_RETURN_TYPE	ReturnType;
		D3D_SRV_DIMENSION			Dimension;
		UINT						NumSamples;
	} D3D11_SHADER_INPUT_BIND_DESC;
	#define ID3D11ShaderReflectionConstantBuffer void
	#define ID3D11ShaderReflectionType void
	#define INTERFACE ID3D11ShaderReflectionVariable
	DECLARE_INTERFACE(ID3D11ShaderReflectionVariable)
	{
		STDMETHOD(GetDesc)(THIS_ D3D11_SHADER_VARIABLE_DESC *pDesc) PURE;

		STDMETHOD_(ID3D11ShaderReflectionType*, GetType)(THIS) PURE;
		STDMETHOD_(ID3D11ShaderReflectionConstantBuffer*, GetBuffer)(THIS) PURE;

		STDMETHOD_(UINT, GetInterfaceSlot)(THIS_ UINT uArrayIndex) PURE;
	};
	#undef INTERFACE
	#define D3D11_SHADER_DESC void
	#define D3D11_SIGNATURE_PARAMETER_DESC void
	#define INTERFACE ID3D11ShaderReflection
	DECLARE_INTERFACE_(INTERFACE, IUnknown)
	{
		STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
		STDMETHOD_(ULONG, AddRef)(THIS) PURE;
		STDMETHOD_(ULONG, Release)(THIS) PURE;

		STDMETHOD(GetDesc)(THIS_ D3D11_SHADER_DESC *pDesc) PURE;

		STDMETHOD_(ID3D11ShaderReflectionConstantBuffer*, GetConstantBufferByIndex)(THIS_ UINT Index) PURE;
		STDMETHOD_(ID3D11ShaderReflectionConstantBuffer*, GetConstantBufferByName)(THIS_ LPCSTR Name) PURE;

		STDMETHOD(GetResourceBindingDesc)(THIS_ UINT ResourceIndex,
										  D3D11_SHADER_INPUT_BIND_DESC *pDesc) PURE;

		STDMETHOD(GetInputParameterDesc)(THIS_ UINT ParameterIndex,
										 D3D11_SIGNATURE_PARAMETER_DESC *pDesc) PURE;
		STDMETHOD(GetOutputParameterDesc)(THIS_ UINT ParameterIndex,
										  D3D11_SIGNATURE_PARAMETER_DESC *pDesc) PURE;
		STDMETHOD(GetPatchConstantParameterDesc)(THIS_ UINT ParameterIndex,
												 D3D11_SIGNATURE_PARAMETER_DESC *pDesc) PURE;

		STDMETHOD_(ID3D11ShaderReflectionVariable*, GetVariableByName)(THIS_ LPCSTR Name) PURE;
		STDMETHOD(GetResourceBindingDescByName)(THIS_ LPCSTR Name, D3D11_SHADER_INPUT_BIND_DESC *pDesc) PURE;
		//more stuff
	};
	#define ID3D11ShaderReflection_GetVariableByName(r,v) r->lpVtbl->GetVariableByName(r,v)
	#undef INTERFACE
	*/
#else
#include <d3d11shader.h>
#endif

//const GUID IID_ID3D11ShaderReflection = {0x8d536ca1, 0x0cca, 0x4956, {0xa8, 0x37, 0x78, 0x69, 0x63, 0x75, 0x55, 0x84}};
//const GUID IID_ID3D11ShaderReflection = {0x0a233719, 0x3960, 0x4578, {0x9d, 0x7c, 0x20, 0x3b, 0x8b, 0x1d, 0x9c, 0xc1}};
#define ID3DBlob_GetBufferPointer(b) b->lpVtbl->GetBufferPointer(b)
#define ID3DBlob_Release(b) b->lpVtbl->Release(b)
#define ID3DBlob_GetBufferSize(b) b->lpVtbl->GetBufferSize(b)
//#define ID3D11ShaderReflection_Release IUnknown_Release

HRESULT (WINAPI *pD3DCompile) (
	LPCVOID pSrcData,
	SIZE_T SrcDataSize,
	LPCSTR pSourceName,
	const D3D_SHADER_MACRO *pDefines,
	ID3DInclude *pInclude,
	LPCSTR pEntrypoint,
	LPCSTR pTarget,
	UINT Flags1,
	UINT Flags2,
	ID3DBlob **ppCode,
	ID3DBlob **ppErrorMsgs
);

static dllhandle_t *shaderlib;


D3D_FEATURE_LEVEL d3dfeaturelevel;


HRESULT STDMETHODCALLTYPE d3dinclude_Close(ID3DInclude *this, LPCVOID pData)
{
	free((void*)pData);
	return S_OK;
}
HRESULT STDMETHODCALLTYPE d3dinclude_Open(ID3DInclude *this, D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
{
	if (IncludeType == D3D_INCLUDE_SYSTEM)
	{
		if (!strcmp(pFileName, "ftedefs.h") || !strcmp(pFileName, "sys/defs.h"))
		{
			static const char *defstruct =
				"cbuffer ftemodeldefs : register(b0)\n"
				"{\n"
					"matrix m_model;\n"
					"float3 e_eyepos; float e_time;\n"
					"float3 e_light_ambient; float pad1;\n"
					"float3 e_light_dir; float pad2;\n"
					"float3 e_light_mul; float pad3;\n"
					"float4 e_lmscale[4];\n"
					"float4 e_uppercolour;\n"
					"float4 e_lowercolour;\n"
					"float4 e_colourmod;\n"
					"float4 e_glowmod;\n"
				"};\n"
				"cbuffer fteviewdefs : register(b1)\n"
				"{\n"
					"matrix m_view;\n"
					"matrix m_projection;\n"
					"float3 v_eyepos; float v_time;\n"
				"};\n"
				"cbuffer ftelightdefs : register(b2)\n"
				"{\n"
					"matrix l_cubematrix;\n"
					"float3 l_lightposition; float padl1;\n"
					"float3 l_colour; float padl2;\n"
					"float3 l_lightcolourscale;float l_lightradius;\n"
					"float4 l_shadowmapproj;\n"
					"float2 l_shadowmapscale; float2 padl3;\n"
				"};\n"
				;
			*ppData = strdup(defstruct);
			*pBytes = strlen(*ppData);
			return S_OK;
		}
		if (!strcmp(pFileName, "sys/skeletal.h"))
		{
			static const char *defstruct =
				""
				;
			*ppData = strdup(defstruct);
			*pBytes = strlen(*ppData);
			return S_OK;
		}
		//fog
	}
	else
	{
		
	}
	return E_FAIL;
}
ID3DIncludeVtbl myd3dincludetab =
{
	d3dinclude_Open,
	d3dinclude_Close
};
ID3DInclude myd3dinclude =
{
	&myd3dincludetab
};

void D3D11Shader_DeleteProg(program_t *prog)
{
	ID3D11InputLayout *layout;
	ID3D11PixelShader *frag;
	ID3D11VertexShader *vert;
	unsigned int permu, l;
	struct programpermu_s *pp;
	for (permu = countof(prog->permu); permu-- > 0; )
	{
		pp = prog->permu[permu];
		if (!pp)
			continue;
		prog->permu[permu] = NULL;
		if (pp == prog->permu[0] && permu)
			continue;	//entry 0 (only) can get copied to avoid constant recompile failures (0 is always precompiled)

		vert = pp->h.hlsl.vert;
		frag = pp->h.hlsl.frag;
		if (vert)
			ID3D11VertexShader_Release(vert);
		if (frag)
			ID3D11PixelShader_Release(frag);
		for (l = 0; l < countof(pp->h.hlsl.layouts); l++)
		{
			layout = pp->h.hlsl.layouts[l];
			if (layout)
				ID3D11InputLayout_Release(layout);
		}
		Z_Free(pp);
	}
}

//create a program from two blobs.
static qboolean D3D11Shader_CreateShaders(program_t *prog, const char *name, struct programpermu_s *permu,
										  void *vblob, size_t vsize,
										  void *hblob, size_t hsize,
										  void *dblob, size_t dsize,
										  void *gblob, size_t gsize,
										  void *fblob, size_t fsize)
{
	int l;
	qboolean success = true;

	if (FAILED(ID3D11Device_CreateVertexShader(pD3DDev11, vblob, vsize, NULL, (ID3D11VertexShader**)&permu->h.hlsl.vert)))
		success = false;

	if (hblob || dblob)
	{
		permu->h.hlsl.topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
		if (FAILED(ID3D11Device_CreateHullShader(pD3DDev11, hblob, hsize, NULL, (ID3D11HullShader**)&permu->h.hlsl.hull)))
			success = false;

		if (FAILED(ID3D11Device_CreateDomainShader(pD3DDev11, dblob, dsize, NULL, (ID3D11DomainShader**)&permu->h.hlsl.domain)))
			success = false;
	}
	else
		permu->h.hlsl.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	if (gblob && FAILED(ID3D11Device_CreateGeometryShader(pD3DDev11, gblob, gsize, NULL, (ID3D11GeometryShader**)&permu->h.hlsl.geom)))
		success = false;

	if (FAILED(ID3D11Device_CreatePixelShader(pD3DDev11, fblob, fsize, NULL, (ID3D11PixelShader**)&permu->h.hlsl.frag)))
		success = false;

	for (l = 0; l < 2 && success; l++)
	{
		D3D11_INPUT_ELEMENT_DESC decl[13];
		int elements = 0;

		decl[elements].SemanticName = "POSITION";
		decl[elements].SemanticIndex = 0;
		decl[elements].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		decl[elements].InputSlot = D3D11_BUFF_POS;
		decl[elements].AlignedByteOffset = 0;
		decl[elements].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		decl[elements].InstanceDataStepRate = 0;
		elements++;

		decl[elements].SemanticName = "TEXCOORD";
		decl[elements].SemanticIndex = 0;
		decl[elements].Format = DXGI_FORMAT_R32G32_FLOAT;
		decl[elements].InputSlot = D3D11_BUFF_TC;
		decl[elements].AlignedByteOffset = 0;
		decl[elements].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		decl[elements].InstanceDataStepRate = 0;
		elements++;

		decl[elements].SemanticName = "TEXCOORD";
		decl[elements].SemanticIndex = 1;
		decl[elements].Format = DXGI_FORMAT_R32G32_FLOAT;
		decl[elements].InputSlot = D3D11_BUFF_LMTC;
		decl[elements].AlignedByteOffset = 0;
		decl[elements].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		decl[elements].InstanceDataStepRate = 0;
		elements++;

		decl[elements].SemanticName = "COLOR";
		decl[elements].SemanticIndex = 0;
		decl[elements].Format = l?DXGI_FORMAT_R32G32B32A32_FLOAT:DXGI_FORMAT_R8G8B8A8_UNORM;
		decl[elements].InputSlot = D3D11_BUFF_COL;
		decl[elements].AlignedByteOffset = 0;
		decl[elements].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		decl[elements].InstanceDataStepRate = 0;
		elements++;

		decl[elements].SemanticName = "NORMAL";
		decl[elements].SemanticIndex = 0;
		decl[elements].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		decl[elements].InputSlot = D3D11_BUFF_NORM;
		decl[elements].AlignedByteOffset = sizeof(vec3_t)*0;
		decl[elements].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		decl[elements].InstanceDataStepRate = 0;
		elements++;

		decl[elements].SemanticName = "TANGENT";
		decl[elements].SemanticIndex = 0;
		decl[elements].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		decl[elements].InputSlot = D3D11_BUFF_NORM;
		decl[elements].AlignedByteOffset = sizeof(vec3_t)*1;
		decl[elements].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		decl[elements].InstanceDataStepRate = 0;
		elements++;

		decl[elements].SemanticName = "BINORMAL";
		decl[elements].SemanticIndex = 0;
		decl[elements].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		decl[elements].InputSlot = D3D11_BUFF_NORM;
		decl[elements].AlignedByteOffset = sizeof(vec3_t)*2;
		decl[elements].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		decl[elements].InstanceDataStepRate = 0;
		elements++;

/*
		decl[elements].SemanticName = "BLENDWEIGHT";
		decl[elements].SemanticIndex = 0;
		decl[elements].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		decl[elements].InputSlot = D3D11_BUFF_SKEL;
		decl[elements].AlignedByteOffset = 0;
		decl[elements].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		decl[elements].InstanceDataStepRate = 0;
		elements++;

		decl[elements].SemanticName = "BLENDINDICIES";
		decl[elements].SemanticIndex = 0;
		decl[elements].Format = DXGI_FORMAT_R8G8B8A8_UINT;
		decl[elements].InputSlot = D3D11_BUFF_SKEL;
		decl[elements].AlignedByteOffset = 0;
		decl[elements].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		decl[elements].InstanceDataStepRate = 0;
		elements++;
*/
		if (FAILED(ID3D11Device_CreateInputLayout(pD3DDev11, decl, elements, vblob, vsize, (ID3D11InputLayout**)&permu->h.hlsl.layouts[l])))
		{
			Con_Printf("HLSL Shader %s requires unsupported inputs\n", name);
			success = false;
		}
	}
	return success;
}

static qboolean D3D11Shader_LoadBlob(program_t *prog, unsigned int permu, vfsfile_t *blobfile)
{
	qboolean success;
	char *vblob, *hblob, *dblob, *gblob, *fblob;
	unsigned int vsz, hsz, dsz, gsz, fsz;

	VFS_READ(blobfile, &vsz, sizeof(vsz));
	vblob = Z_Malloc(vsz);
	VFS_READ(blobfile, vblob, vsz);

	VFS_READ(blobfile, &hsz, sizeof(hsz));
	if (hsz != ~0u)
	{
		hblob = Z_Malloc(hsz);
		VFS_READ(blobfile, hblob, hsz);
	}
	else
		hblob = NULL;

	VFS_READ(blobfile, &dsz, sizeof(dsz));
	if (dsz != ~0u)
	{
		dblob = Z_Malloc(dsz);
		VFS_READ(blobfile, dblob, dsz);
	}
	else
		dblob = NULL;

	VFS_READ(blobfile, &gsz, sizeof(gsz));
	if (dsz != ~0u)
	{
		gblob = Z_Malloc(gsz);
		VFS_READ(blobfile, gblob, gsz);
	}
	else
		gblob = NULL;

	VFS_READ(blobfile, &fsz, sizeof(fsz));
	fblob = Z_Malloc(fsz);
	VFS_READ(blobfile, fblob, fsz);


	success = D3D11Shader_CreateShaders(prog, prog->name, prog->permu[permu], vblob, vsz, hblob, hsz, dblob, dsz, gblob, gsz, fblob, fsz);
	Z_Free(vblob);
	Z_Free(hblob);
	Z_Free(dblob);
	Z_Free(gblob);
	Z_Free(fblob);
	return success;
}

qboolean D3D11Shader_CreateProgram (program_t *prog, struct programpermu_s *permu, int ver, const char **precompilerconstants, const char *vert, const char *hull, const char *domain, const char *geom, const char *frag, qboolean silenterrors, vfsfile_t *blobfile)
{
	char *vsformat;
	char *hsformat = NULL;
	char *dsformat = NULL;
	char *fsformat;
	char *gsformat = NULL;
	char *tmp;
	D3D_SHADER_MACRO defines[64];
	ID3DBlob *vcode = NULL, *hcode = NULL, *dcode = NULL, *gcode = NULL, *fcode = NULL, *errors = NULL;
	qboolean success = false;
//	ID3D11ShaderReflection *freflect;
//	int i;

	if (d3dfeaturelevel >= D3D_FEATURE_LEVEL_11_0)	//and 11.1
	{
		vsformat = "vs_5_0";
		hsformat = "hs_5_0";
		dsformat = "ds_5_0";
		gsformat = "gs_5_0";
		fsformat = "ps_5_0";
	}
	else if (d3dfeaturelevel >= D3D_FEATURE_LEVEL_10_1)
	{
		vsformat = "vs_4_1";
		gsformat = "gs_4_1";
		fsformat = "ps_4_1";
	}
	else if (d3dfeaturelevel >= D3D_FEATURE_LEVEL_10_0)
	{
		vsformat = "vs_4_0";
		gsformat = "gs_4_0";
		fsformat = "ps_4_0";
	}
	else if (d3dfeaturelevel >= D3D_FEATURE_LEVEL_9_3)
	{	//dx10-compatible output for 9.3 hardware
		vsformat = "vs_4_0_level_9_3";
		fsformat = "ps_4_0_level_9_3";
	}
	else
	{	//dx10-compatible output for 9.1|9.2 hardware
		vsformat = "vs_4_0_level_9_1";
		fsformat = "ps_4_0_level_9_1";
	}

	permu->h.hlsl.vert = NULL;
	permu->h.hlsl.frag = NULL;
	permu->h.hlsl.hull = NULL;
	permu->h.hlsl.domain = NULL;
	permu->h.hlsl.geom = NULL;
	memset(permu->h.hlsl.layouts, 0, sizeof(permu->h.hlsl.layouts));

	if (pD3DCompile)
	{
		int consts;
		for (consts = 0; precompilerconstants[consts]; consts++)
			;
		if (consts+3 >= sizeof(defines) / sizeof(defines[0]))
			return success;

		consts = 0;
		defines[consts].Name = NULL; /*shader type*/
		defines[consts].Definition = "1";
		consts++;

		defines[consts].Name = "ENGINE_"DISTRIBUTION;
#ifdef SVNREVISION
		defines[consts].Definition = STRINGIFY(SVNREVISION);
#else
		defines[consts].Definition = "0";
#endif
		consts++;

		tmp = Z_Malloc(64);
		Q_snprintfz(tmp, 64, "0x%x", d3dfeaturelevel);
		defines[consts].Name = Z_StrDup("LEVEL");
		defines[consts].Definition = tmp;
		consts++;

		for (; *precompilerconstants; precompilerconstants++)
		{
			const char *t, *nl;
			char *v;
			for (t = *precompilerconstants; *t; t++)
			{
				t = COM_Parse(t);
				t = COM_Parse(t);
				defines[consts].Name = Z_StrDup(com_token);
				while (*t == ' ' || *t == '\t')
					t++;
				nl = strchr(t, '\n');
				if (!nl)
					nl = t+strlen(t);

				if (nl && nl != t)
				{
					v = BZ_Malloc(nl-t+1);
					memcpy(v, t, nl-t);
					v[nl-t] = 0;
					defines[consts].Definition = v;
				}
				else
					defines[consts].Definition = t?Z_StrDup(t):NULL;
				consts++;
				t = nl;
			}
		}

		defines[consts].Name = NULL;
		defines[consts].Definition = NULL;

		success = true;

		defines[0].Name = "VERTEX_SHADER";
		if (FAILED(pD3DCompile(vert, strlen(vert), prog->name, defines, &myd3dinclude, "main", vsformat, 0, 0, &vcode, &errors)))
			success = false;
		if (errors && !silenterrors)
		{
			char *messages = ID3DBlob_GetBufferPointer(errors);
			Con_Printf("vertex shader %s:\n%s", prog->name, messages);
			ID3DBlob_Release(errors);
		}

		if (hull)
		{
			if (!hsformat)
				success = false;
			else
			{
				defines[0].Name = "HULL_SHADER";
				if (FAILED(pD3DCompile(hull, strlen(hull), prog->name, defines, &myd3dinclude, "main", hsformat, 0, 0, &hcode, &errors)))
					success = false;
				if (errors && !silenterrors)
				{
					char *messages = ID3DBlob_GetBufferPointer(errors);
					Con_Printf("hull shader %s:\n%s", prog->name, messages);
					ID3DBlob_Release(errors);
				}
			}
		}

		if (domain)
		{
			if (!dsformat)
				success = false;
			else
			{
				defines[0].Name = "DOMAIN_SHADER";
				if (FAILED(pD3DCompile(domain, strlen(domain), prog->name, defines, &myd3dinclude, "main", dsformat, 0, 0, &dcode, &errors)))
					success = false;
				if (errors && !silenterrors)
				{
					char *messages = ID3DBlob_GetBufferPointer(errors);
					Con_Printf("domain shader %s:\n%s", prog->name, messages);
					ID3DBlob_Release(errors);
				}
			}
		}

		if (geom)
		{
			if (!dsformat)
				success = false;
			else
			{
				defines[0].Name = "GEOMETRY_SHADER";
				if (FAILED(pD3DCompile(domain, strlen(domain), prog->name, defines, &myd3dinclude, "main", gsformat, 0, 0, &gcode, &errors)))
					success = false;
				if (errors && !silenterrors)
				{
					char *messages = ID3DBlob_GetBufferPointer(errors);
					Con_Printf("geometry shader %s:\n%s", prog->name, messages);
					ID3DBlob_Release(errors);
				}
			}
		}

		defines[0].Name = "FRAGMENT_SHADER";
		if (FAILED(pD3DCompile(frag, strlen(frag), prog->name, defines, &myd3dinclude, "main", fsformat, 0, 0, &fcode, &errors)))
			success = false;
		if (errors && !silenterrors)
		{
			char *messages = ID3DBlob_GetBufferPointer(errors);
			Con_Printf("fragment shader %s:\n%s", prog->name, messages);
			ID3DBlob_Release(errors);
		}

		while(consts-->2)
		{
			Z_Free((void*)defines[consts].Name);
			Z_Free((void*)defines[consts].Definition);
		}

		if (success)
			success = D3D11Shader_CreateShaders(prog, prog->name, permu,
				ID3DBlob_GetBufferPointer(vcode), ID3DBlob_GetBufferSize(vcode),
				hcode?ID3DBlob_GetBufferPointer(hcode):NULL, hcode?ID3DBlob_GetBufferSize(hcode):0,
				dcode?ID3DBlob_GetBufferPointer(dcode):NULL, dcode?ID3DBlob_GetBufferSize(dcode):0,
				gcode?ID3DBlob_GetBufferPointer(gcode):NULL, gcode?ID3DBlob_GetBufferSize(gcode):0,
				ID3DBlob_GetBufferPointer(fcode), ID3DBlob_GetBufferSize(fcode));

		if (success && blobfile)
		{
			unsigned int sz;
			sz = ID3DBlob_GetBufferSize(vcode);
			VFS_WRITE(blobfile, &sz, sizeof(sz));
			VFS_WRITE(blobfile, ID3DBlob_GetBufferPointer(vcode), sz);

			if (!hcode)
			{
				sz = ~0u;
				VFS_WRITE(blobfile, &sz, sizeof(sz));
			}
			else
			{
				sz = ID3DBlob_GetBufferSize(hcode);
				VFS_WRITE(blobfile, &sz, sizeof(sz));
				VFS_WRITE(blobfile, ID3DBlob_GetBufferPointer(hcode), sz);
			}

			if (!dcode)
			{
				sz = ~0u;
				VFS_WRITE(blobfile, &sz, sizeof(sz));
			}
			else
			{
				sz = ID3DBlob_GetBufferSize(dcode);
				VFS_WRITE(blobfile, &sz, sizeof(sz));
				VFS_WRITE(blobfile, ID3DBlob_GetBufferPointer(dcode), sz);
			}

			if (!gcode)
			{
				sz = ~0u;
				VFS_WRITE(blobfile, &sz, sizeof(sz));
			}
			else
			{
				sz = ID3DBlob_GetBufferSize(gcode);
				VFS_WRITE(blobfile, &sz, sizeof(sz));
				VFS_WRITE(blobfile, ID3DBlob_GetBufferPointer(gcode), sz);
			}

			sz = ID3DBlob_GetBufferSize(fcode);
			VFS_WRITE(blobfile, &sz, sizeof(sz));
			VFS_WRITE(blobfile, ID3DBlob_GetBufferPointer(fcode), sz);
		}


/*		if (fcode)
		{
			pD3DReflect(ID3DBlob_GetBufferPointer(fcode), ID3DBlob_GetBufferSize(fcode), &IID_ID3D11ShaderReflection, (void**)&freflect);
			if (freflect)
			{
				int tmu;
				D3D11_SHADER_INPUT_BIND_DESC bdesc = {0};
				for (i = prog->numsamplers; i < 16; i++)
				{
					if (SUCCEEDED(freflect->lpVtbl->GetResourceBindingDescByName(freflect, va("t_t%i", i), &bdesc)))
						prog->numsamplers = i+1;
				}

				tmu = prog->numsamplers;
				for (i = 0; sh_defaultsamplers[i]; i++)
				{
//					if (prog->defaulttextures & (1u<<i))
//						continue;
					if (SUCCEEDED(freflect->lpVtbl->GetResourceBindingDescByName(freflect, va("t%s", sh_defaultsamplers[i]+1), &bdesc)))
						prog->defaulttextures |= (1u<<i);
					if (!(prog->defaulttextures & (1u<<i)))
						continue;
					tmu++;
				}
				ID3D11ShaderReflection_Release(freflect);
			}
			else
				Con_Printf("%s: D3DReflect failed, unable to get reflection info\n", name);
		}
*/
		if (vcode)
			ID3DBlob_Release(vcode);
		if (hcode)
			ID3DBlob_Release(hcode);
		if (dcode)
			ID3DBlob_Release(dcode);
		if (gcode)
			ID3DBlob_Release(gcode);
		if (fcode)
			ID3DBlob_Release(fcode);
	}
	return success;
}

qboolean D3D11Shader_Init(unsigned int flevel)
{
	//FIXME: if the feature level is below 10, make sure the compiler supports all the right targets etc
	int ver;
	dllfunction_t funcsold[] =
	{
		{(void**)&pD3DCompile, "D3DCompileFromMemory"},
		{NULL,NULL}
	};
	dllfunction_t funcsnew[] =
	{
		{(void**)&pD3DCompile, "D3DCompile"},
		{NULL,NULL}
	};

	//FIXME: wine's d3dcompiler dlls don't work properly right now, and winetricks installs ms' only up to 43 (which works, but we loaded 47 instead)
	for (ver = 47; ver >= 33; ver--)
	{
		shaderlib = Sys_LoadLibrary(va("D3dcompiler_%i.dll", ver), (ver>=40)?funcsnew:funcsold);
		if (shaderlib)
			break;
	}

	if (!shaderlib)
	{
		//no shader library available. at least make sure that there's a 2d blob that we can use.
		if (!COM_FCheckExists("hlsl11/default2d.blob"))
			return false;
	}

	sh_config.minver = 11;
	sh_config.maxver = 11;
	sh_config.blobpath = "hlsl11/%s.blob";
	sh_config.progpath = shaderlib?"hlsl11/%s.hlsl":NULL;
	sh_config.shadernamefmt = "%s_hlsl11";

	sh_config.progs_supported	= true;
	sh_config.progs_required	= true;

	sh_config.pDeleteProg		= D3D11Shader_DeleteProg;
	sh_config.pLoadBlob			= D3D11Shader_LoadBlob;
	sh_config.pCreateProgram	= D3D11Shader_CreateProgram;
	sh_config.pProgAutoFields	= NULL;

	sh_config.can_mipcap		= true;	//at creation time
	sh_config.havecubemaps		= true;

//	sh_config.tex_env_combine		= 1;
//	sh_config.nv_tex_env_combine4	= 1;
//	sh_config.env_add				= 1;

	d3dfeaturelevel = flevel;
	return true;
}

#endif

