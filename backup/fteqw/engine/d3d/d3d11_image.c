#include "quakedef.h"
#ifdef D3D11QUAKE
#include "winquake.h"
#include "shader.h"
#define COBJMACROS
#include <d3d11.h>
extern ID3D11Device *pD3DDev11;
extern ID3D11DeviceContext *d3ddevctx;

extern D3D_FEATURE_LEVEL d3dfeaturelevel;
#define D3D_HAVE_FULL_NPOT() (d3dfeaturelevel>=D3D_FEATURE_LEVEL_10_0)

void D3D11BE_UnbindAllTextures(void);


ID3D11ShaderResourceView *D3D11_Image_View(const texid_t id)
{
	if (!id || !id->ptr)
		return NULL;
	if (!id->ptr2)
		ID3D11Device_CreateShaderResourceView(pD3DDev11, (ID3D11Resource *)id->ptr, NULL, (ID3D11ShaderResourceView**)&id->ptr2);
	return id->ptr2;
}

void    D3D11_DestroyTexture (texid_t tex)
{
	if (!tex)
		return;

	if (tex->ptr2)
		ID3D11ShaderResourceView_Release((ID3D11ShaderResourceView*)tex->ptr2);
	tex->ptr2 = NULL;

	if (tex->ptr)
		ID3D11Texture2D_Release((ID3D11Texture2D*)tex->ptr);
	tex->ptr = NULL;
}

qboolean D3D11_LoadTextureMips(image_t *tex, const struct pendingtextureinfo *mips)
{
	unsigned int blockbytes, blockwidth, blockheight, blockdepth;
	HRESULT hr;
	D3D11_TEXTURE2D_DESC tdesc = {0};
	D3D11_SUBRESOURCE_DATA *subresdesc;
	int i, layer;

	if (!sh_config.texfmt[mips->encoding])
	{
		Con_Printf("Texture encoding %i not supported by d3d11\n", mips->encoding);
		return false;
	}
	if (mips->type != (tex->flags & IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT)
		return false;

	tdesc.Width = mips->mip[0].width;
	tdesc.Height = mips->mip[0].height;
	tdesc.ArraySize = 1;
	tdesc.SampleDesc.Count = 1;
	tdesc.SampleDesc.Quality = 0;
	tdesc.Usage = mips->mip[0].data?D3D11_USAGE_IMMUTABLE:D3D11_USAGE_DYNAMIC;
	tdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tdesc.CPUAccessFlags = (mips->mip[0].data)?0:D3D11_CPU_ACCESS_WRITE;
	tdesc.MiscFlags = 0;
	tdesc.Format = DXGI_FORMAT_UNKNOWN;

	if (tex->flags & IF_RENDERTARGET)
	{
		tdesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		tdesc.Usage = D3D11_USAGE_DEFAULT;
		tdesc.CPUAccessFlags = 0;
	}

	if (mips->type == PTI_CUBE)
	{
		tdesc.ArraySize *= 6;
		tdesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
	}
//	else if (mips->type == PTI_2D_ARRAY)
//	{
//		tdesc.ArraySize *= mips->mip[0].depth;
//	}
	else if (mips->type != PTI_2D)
		return false;	//nyi

//d3d11.1 formats
#define DXGI_FORMAT_B4G4R4A4_UNORM 115

	//dxgi formats are expressed in little-endian bit order. byte-aligned formats are always in byte order and are thus little-endian even on big-endian machines.
	//so byte aligned have the same order, while misligned need reversed order.
	switch(mips->encoding)
	{
	case PTI_DEPTH16:
		tdesc.Format = DXGI_FORMAT_D16_UNORM;
		tdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		break;
	case PTI_DEPTH24:
		tdesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		tdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		break;
	case PTI_DEPTH32:
		tdesc.Format = DXGI_FORMAT_D32_FLOAT;
		tdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		break;
	case PTI_DEPTH24_8:
		tdesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		tdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		break;
	case PTI_RGB565:
		tdesc.Format = DXGI_FORMAT_B5G6R5_UNORM;
		break;
	case PTI_ARGB1555:
		tdesc.Format = DXGI_FORMAT_B5G5R5A1_UNORM;
		break;
	case PTI_RGBA5551:
//		tdesc.Format = DXGI_FORMAT_A1B5G5R5_UNORM;
		break;
	case PTI_ARGB4444:
		tdesc.Format = DXGI_FORMAT_B4G4R4A4_UNORM;	//DX11.1
		break;
	case PTI_RGBA4444:
//		tdesc.Format = DXGI_FORMAT_A4B4G4R4_UNORM;
		break;
	case PTI_RGB8:
//		tdesc.Format = DXGI_FORMAT_R8G8B8_UNORM;
		break;
	case PTI_BGR8:
//		tdesc.Format = DXGI_FORMAT_B8G8R8_UNORM;
		break;
	case PTI_RGB8_SRGB:
//		tdesc.Format = DXGI_FORMAT_R8G8B8_SRGB;
		break;
	case PTI_BGR8_SRGB:
//		tdesc.Format = DXGI_FORMAT_B8G8R8_SRGB;
		break;
	case PTI_RGBA8:
		tdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case PTI_RGBX8:	//d3d11 has no alphaless format. be sure to proprly disable alpha in the shader. 
		tdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case PTI_BGRA8:
		tdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		break;
	case PTI_BGRX8:
		tdesc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
		break;
	case PTI_A2BGR10:	//mostly for rendertargets, might also be useful for overbight lightmaps.
		tdesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		break;
	case PTI_E5BGR9:
		tdesc.Format = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
		break;
	case PTI_B10G11R11F:
		tdesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
		break;
	case PTI_RGBA8_SRGB:
		tdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		break;
	case PTI_RGBX8_SRGB:	//d3d11 has no alphaless format. be sure to proprly disable alpha in the shader. 
		tdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		break;
	case PTI_BGRA8_SRGB:
		tdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		break;
	case PTI_BGRX8_SRGB:
		tdesc.Format = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		break;

	case PTI_BC1_RGB:	//d3d11 provides no way to disable alpha with dxt1. be sure to proprly disable alpha in the shader. 
	case PTI_BC1_RGBA:
		tdesc.Format = DXGI_FORMAT_BC1_UNORM;
		break;
	case PTI_BC2_RGBA:
		tdesc.Format = DXGI_FORMAT_BC2_UNORM;
		break;
	case PTI_BC3_RGBA:
		tdesc.Format = DXGI_FORMAT_BC3_UNORM;
		break;
	case PTI_BC1_RGB_SRGB:	//d3d11 provides no way to disable alpha with dxt1. be sure to proprly disable alpha in the shader. 
	case PTI_BC1_RGBA_SRGB:
		tdesc.Format = DXGI_FORMAT_BC1_UNORM_SRGB;
		break;
	case PTI_BC2_RGBA_SRGB:
		tdesc.Format = DXGI_FORMAT_BC2_UNORM_SRGB;
		break;
	case PTI_BC3_RGBA_SRGB:
		tdesc.Format = DXGI_FORMAT_BC3_UNORM_SRGB;
		break;
	case PTI_BC4_R:
		tdesc.Format = DXGI_FORMAT_BC4_UNORM;
		break;
	case PTI_BC4_R_SNORM:
		tdesc.Format = DXGI_FORMAT_BC4_SNORM;
		break;
	case PTI_BC5_RG:
		tdesc.Format = DXGI_FORMAT_BC5_UNORM;
		break;
	case PTI_BC5_RG_SNORM:
		tdesc.Format = DXGI_FORMAT_BC5_SNORM;
		break;
	case PTI_BC6_RGB_UFLOAT:
		tdesc.Format = DXGI_FORMAT_BC6H_UF16;
		break;
	case PTI_BC6_RGB_SFLOAT:
		tdesc.Format = DXGI_FORMAT_BC6H_SF16;
		break;
	case PTI_BC7_RGBA:
		tdesc.Format = DXGI_FORMAT_BC7_UNORM;
		break;
	case PTI_BC7_RGBA_SRGB:
		tdesc.Format = DXGI_FORMAT_BC7_UNORM_SRGB;
		break;

	case PTI_R16:
		tdesc.Format = DXGI_FORMAT_R16_UNORM;
		break;
	case PTI_RGBA16:
		tdesc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		break;

	case PTI_R16F:
		tdesc.Format = DXGI_FORMAT_R16_FLOAT;
		break;
	case PTI_R32F:
		tdesc.Format = DXGI_FORMAT_R32_FLOAT;
		break;
	case PTI_RGBA16F:
		tdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	case PTI_RGBA32F:
		tdesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	case PTI_RGB32F:
		tdesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		break;
	case PTI_L8:	//UNSUPPORTED
	case PTI_P8:	//R8, but different usage.
	case PTI_R8:
		tdesc.Format = DXGI_FORMAT_R8_UNORM;
		break;
	case PTI_L8A8: //UNSUPPORTED
	case PTI_RG8:
		tdesc.Format = DXGI_FORMAT_R8G8_UNORM;
		break;
	case PTI_R8_SNORM:
		tdesc.Format = DXGI_FORMAT_R8_SNORM;
		break;
	case PTI_RG8_SNORM:
		tdesc.Format = DXGI_FORMAT_R8G8_SNORM;
		break;

	case PTI_L8_SRGB:			//no swizzles / single-channel srgb
	case PTI_L8A8_SRGB:			//no swizzles / single-channel srgb
	case PTI_ETC1_RGB8:			//not invented here...
	case PTI_ETC2_RGB8:
	case PTI_ETC2_RGB8A1:
	case PTI_ETC2_RGB8A8:
	case PTI_ETC2_RGB8_SRGB:
	case PTI_ETC2_RGB8A1_SRGB:
	case PTI_ETC2_RGB8A8_SRGB:
	case PTI_EAC_R11:
	case PTI_EAC_R11_SNORM:
	case PTI_EAC_RG11:
	case PTI_EAC_RG11_SNORM:
	case PTI_ASTC_4X4_LDR:			//not invented here...
	case PTI_ASTC_5X4_LDR:
	case PTI_ASTC_5X5_LDR:
	case PTI_ASTC_6X5_LDR:
	case PTI_ASTC_6X6_LDR:
	case PTI_ASTC_8X5_LDR:
	case PTI_ASTC_8X6_LDR:
	case PTI_ASTC_10X5_LDR:
	case PTI_ASTC_10X6_LDR:
	case PTI_ASTC_8X8_LDR:
	case PTI_ASTC_10X8_LDR:
	case PTI_ASTC_10X10_LDR:
	case PTI_ASTC_12X10_LDR:
	case PTI_ASTC_12X12_LDR:
	case PTI_ASTC_4X4_HDR:
	case PTI_ASTC_5X4_HDR:
	case PTI_ASTC_5X5_HDR:
	case PTI_ASTC_6X5_HDR:
	case PTI_ASTC_6X6_HDR:
	case PTI_ASTC_8X5_HDR:
	case PTI_ASTC_8X6_HDR:
	case PTI_ASTC_10X5_HDR:
	case PTI_ASTC_10X6_HDR:
	case PTI_ASTC_8X8_HDR:
	case PTI_ASTC_10X8_HDR:
	case PTI_ASTC_10X10_HDR:
	case PTI_ASTC_12X10_HDR:
	case PTI_ASTC_12X12_HDR:
	case PTI_ASTC_4X4_SRGB:
	case PTI_ASTC_5X4_SRGB:
	case PTI_ASTC_5X5_SRGB:
	case PTI_ASTC_6X5_SRGB:
	case PTI_ASTC_6X6_SRGB:
	case PTI_ASTC_8X5_SRGB:
	case PTI_ASTC_8X6_SRGB:
	case PTI_ASTC_10X5_SRGB:
	case PTI_ASTC_10X6_SRGB:
	case PTI_ASTC_8X8_SRGB:
	case PTI_ASTC_10X8_SRGB:
	case PTI_ASTC_10X10_SRGB:
	case PTI_ASTC_12X10_SRGB:
	case PTI_ASTC_12X12_SRGB:
#ifdef FTE_TARGET_WEB
	case PTI_WHOLEFILE:			//basically webgl only...
#endif
	case PTI_MAX:				//not actually valid...
	case PTI_EMULATED:			//not hardware-compatible.
		break;
	}
	if (tdesc.Format == DXGI_FORMAT_UNKNOWN)
	{
		return false;
	}

	Image_BlockSizeForEncoding(mips->encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);

	if (!mips->mip[0].data)
	{
		subresdesc = alloca(sizeof(*subresdesc));
		subresdesc[0].pSysMem = NULL;
		subresdesc[0].SysMemPitch = 0;
		subresdesc[0].SysMemSlicePitch = 0;
		//one mip, but no data. happens with rendertargets
		tdesc.MipLevels = 1;
	}
	else
	{
		subresdesc = alloca(tdesc.ArraySize*mips->mipcount*sizeof(*subresdesc));
		for (layer = 0; layer < tdesc.ArraySize; layer++)
		{
			for (i = 0; i < mips->mipcount; i++)
			{
				subresdesc[i+layer*mips->mipcount].SysMemPitch = ((mips->mip[i].width+blockwidth-1)/blockwidth) * blockbytes;
				subresdesc[i+layer*mips->mipcount].SysMemSlicePitch = subresdesc[i].SysMemPitch * ((mips->mip[i].width+blockheight-1)/blockheight);
				subresdesc[i+layer*mips->mipcount].pSysMem = (qbyte*)mips->mip[i].data + subresdesc[i+layer*mips->mipcount].SysMemSlicePitch*layer;
			}
		}
		tdesc.MipLevels = mips->mipcount;
	}

	D3D11_DestroyTexture(tex);
	hr = ID3D11Device_CreateTexture2D(pD3DDev11, &tdesc, (mips->mip[0].data?subresdesc:NULL), (ID3D11Texture2D**)&tex->ptr);

	return !FAILED(hr);
}
void D3D11_UploadLightmap(lightmapinfo_t *lm)
{
	extern cvar_t r_lightmap_nearest;
	struct pendingtextureinfo mips;
	image_t *tex;
	lm->modified = false;
	if (!TEXVALID(lm->lightmap_texture))
	{
		lm->lightmap_texture = Image_CreateTexture("***lightmap***", NULL, (r_lightmap_nearest.ival?IF_NEAREST:IF_LINEAR));
		if (!lm->lightmap_texture)
			return;
	}
	tex = lm->lightmap_texture;

	mips.extrafree = NULL;
	mips.type = PTI_2D;
	mips.mip[0].data = lm->lightmaps;
	mips.mip[0].needfree = false;
	mips.mip[0].width = lm->width;
	mips.mip[0].height = lm->height;
	mips.mip[0].datasize = lm->width*lm->height*4;
	switch (lm->fmt)
	{
	default:
	case PTI_A2BGR10:
	case PTI_E5BGR9:
	case PTI_B10G11R11F:
	case PTI_RGBA16F:
	case PTI_RGBA32F:
		mips.encoding = lm->fmt;
		break;
	case PTI_BGRA8:
		mips.encoding = PTI_BGRX8;
		break;
	case PTI_RGBA8:
		mips.encoding = PTI_RGBX8;
		break;
	case PTI_L8:
		mips.encoding = PTI_R8;	//FIXME: unspported
		break;
	}
	mips.mipcount = 1;
	D3D11_LoadTextureMips(tex, &mips);
	tex->width = lm->width;
	tex->height = lm->height;

	lm->lightmap_texture = tex;
}

#ifdef RTLIGHTS
static const int shadowfmt = 1;
static const int shadowfmts[][3] =
{
	//sampler,				creation,			render
	{DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT},
	{DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_D16_UNORM},
	{DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM}
};
image_t shadowmap_texture[2];
ID3D11DepthStencilView *shadowmap_dsview[2];
ID3D11RenderTargetView *shadowmap_rtview[2];
texid_t D3D11_GetShadowMap(int id)
{
	texid_t tex = &shadowmap_texture[id];
	if (!tex->ptr)
	{
		return r_nulltex;
	}
	if (!tex->ptr2)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		desc.Format = shadowfmts[shadowfmt][0];
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.MipLevels = -1;
		ID3D11Device_CreateShaderResourceView(pD3DDev11, (ID3D11Resource *)tex->ptr, &desc, (ID3D11ShaderResourceView**)&tex->ptr2);
	}
	return tex;
}
void D3D11_TerminateShadowMap(void)
{
	int i;
	for (i = 0; i < sizeof(shadowmap_texture)/sizeof(shadowmap_texture[0]); i++)
	{
		if (shadowmap_dsview[i])
			ID3D11DepthStencilView_Release(shadowmap_dsview[i]);
		shadowmap_dsview[i] = NULL;
		D3D11_DestroyTexture(&shadowmap_texture[i]);
	}
}
qboolean D3D11_BeginShadowMap(int id, int w, int h)
{
	D3D11_TEXTURE2D_DESC texdesc;
	HRESULT hr;

	if (!shadowmap_dsview[id] && !shadowmap_rtview[id])
	{
		memset(&texdesc, 0, sizeof(texdesc));

		texdesc.Width = w;
		texdesc.Height = h;
		texdesc.MipLevels = 1;
		texdesc.ArraySize = 1;
		texdesc.Format = shadowfmts[shadowfmt][1];
		texdesc.SampleDesc.Count = 1;
		texdesc.SampleDesc.Quality = 0;
		texdesc.Usage = D3D11_USAGE_DEFAULT;
		texdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		texdesc.CPUAccessFlags = 0;
		texdesc.MiscFlags = 0;

		if (shadowfmt == 2)
			texdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		// Create the texture
		if (!shadowmap_texture[id].ptr)
		{
			hr = ID3D11Device_CreateTexture2D(pD3DDev11, &texdesc, NULL, (ID3D11Texture2D **)&shadowmap_texture[id].ptr);
			if (FAILED(hr))
				return false;
		}


		if (shadowfmt == 2)
		{
			hr = ID3D11Device_CreateRenderTargetView(pD3DDev11, (ID3D11Resource *)shadowmap_texture[id].ptr, NULL, &shadowmap_rtview[id]);
		}
		else
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC rtdesc;
			rtdesc.Format = shadowfmts[shadowfmt][2];
			rtdesc.Flags = 0;
			rtdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			rtdesc.Texture2D.MipSlice = 0;
			hr = ID3D11Device_CreateDepthStencilView(pD3DDev11, (ID3D11Resource *)shadowmap_texture[id].ptr, &rtdesc, &shadowmap_dsview[id]);
		}
		if (FAILED(hr))
			return false;
	}
	D3D11BE_UnbindAllTextures();
	if (shadowfmt == 2)
	{
		float colours[4] = {0, 1, 0, 0};
		colours[0] = frandom();
		colours[1] = frandom();
		colours[2] = frandom();
		ID3D11DeviceContext_OMSetRenderTargets(d3ddevctx, 1, &shadowmap_rtview[id], shadowmap_dsview[id]);
		ID3D11DeviceContext_ClearRenderTargetView(d3ddevctx, shadowmap_rtview[id], colours);
	}
	else
	{
		ID3D11DeviceContext_OMSetRenderTargets(d3ddevctx, 0, NULL, shadowmap_dsview[id]);
		ID3D11DeviceContext_ClearDepthStencilView(d3ddevctx, shadowmap_dsview[id], D3D11_CLEAR_DEPTH, 1.0f, 0);
	}
	return true;
}
void D3D11_EndShadowMap(void)
{
	extern ID3D11RenderTargetView *fb_backbuffer;
	extern ID3D11DepthStencilView *fb_backdepthstencil;
	ID3D11DeviceContext_OMSetRenderTargets(d3ddevctx, 1, &fb_backbuffer, fb_backdepthstencil);
}
#endif

#endif
