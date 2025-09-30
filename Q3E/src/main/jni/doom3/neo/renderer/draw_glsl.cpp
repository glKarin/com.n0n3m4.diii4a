/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"

#include "glsl/draw_glsl_shader.cpp"

#if 1
#define ENABLE_STENCIL_TEST() qglEnable(GL_STENCIL_TEST);
#define DISABLE_STENCIL_TEST() qglDisable(GL_STENCIL_TEST);
#else
#define ENABLE_STENCIL_TEST() // qglStencilFunc(GL_GEQUAL, 128, 255);
#define DISABLE_STENCIL_TEST() qglStencilFunc(GL_ALWAYS, 128, 255);
#endif

idCVar harm_r_lightingModel("harm_r_lightingModel", "1", CVAR_RENDERER|CVAR_ARCHIVE|CVAR_INTEGER, "Lighting model when draw interactions(1 = Phong(default); 2 = Blinn-Phong; 3 = PBR; 4 = Ambient; 0 = No lighting.)", HARM_INTERACTION_SHADER_NOLIGHTING, HARM_INTERACTION_SHADER_AMBIENT, idCmdSystem::ArgCompletion_Integer<HARM_INTERACTION_SHADER_NOLIGHTING, HARM_INTERACTION_SHADER_AMBIENT>);
static idCVar harm_r_specularExponent("harm_r_specularExponent", "3.0"/* "4.0"*/, CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "Specular exponent in Phong interaction lighting model");
static idCVar harm_r_specularExponentBlinnPhong("harm_r_specularExponentBlinnPhong", "12.0", CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "Specular exponent in Blinn-Phong interaction lighting model");
static idCVar harm_r_specularExponentPBR("harm_r_specularExponentPBR", "5.0", CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "Specular exponent in PBR interaction lighting model"); // 3.0
static idCVar harm_r_PBRNormalCorrection("harm_r_PBRNormalCorrection", "0.25", CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "Vertex normal correction in PBR interaction lighting model(1 = pure using bump texture; 0 = pure using vertex normal; 0.0 - 1.0 = bump texture * harm_r_normalCorrectionPBR + vertex normal * (1 - harm_r_normalCorrectionPBR))", 0, 1); // 0.2
static idCVar harm_r_PBRRMAOSpecularMap("harm_r_PBRRMAOSpecularMap", "0", CVAR_BOOL|CVAR_RENDERER|CVAR_ARCHIVE, "Specular map is standard PBR RAMO texture or old non-PBR texture");
static idCVar harm_r_PBRRoughnessCorrection("harm_r_PBRRoughnessCorrection", "0.55", CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "max roughness for old specular texture in PBR. 0 = disable; else = roughness = harm_r_PBRRoughnessCorrection - texture(specularTexture, st).r", -1, 1); // 0.6 0.5
static idCVar harm_r_PBRMetallicCorrection("harm_r_PBRMetallicCorrection", "0", CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "min metallic for old specular texture in PBR. 0 = disable; else = metallic = texture(specularTexture, st).r + harm_r_PBRMetallicCorrection", -1, 1);
static idCVar harm_r_ambientLightingBrightness("harm_r_ambientLightingBrightness", "1.0", CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "Lighting brightness in ambient lighting");

#include "glsl/draw_glsl_backend.cpp"

#ifdef _SHADOW_MAPPING
#include "glsl/draw_glsl_shadowmapping.cpp"
#endif
#ifdef _STENCIL_SHADOW_IMPROVE
#include "glsl/draw_glsl_stencilshadow.cpp"
#endif
#ifdef _GLOBAL_ILLUMINATION
#include "glsl/draw_glsl_globalIllumination.cpp"
#endif

#include "glsl/draw_glsl_interaction.cpp"

/*
==================
R_InitGLSLCvars
==================
*/
static void R_InitGLSLCvars(void)
{
	float f;

#ifdef _SHADOW_MAPPING
	r_shadowMapping = r_useShadowMapping.GetBool();
	r_shadowMapPerforated = r_forceShadowMapsOnAlphaTestedSurfaces.GetBool();
    r_shadowMapCombine = harm_r_shadowMapCombine.GetBool();
    r_shadowMapParallelSplitFrustums = r_shadowMapSplits.GetInteger();
#ifdef GL_ES_VERSION_3_0
	if(!USING_GLES3) {
#endif
		switch (harm_r_shadowMapDepthBuffer.GetInteger())
		{
			case 1:
				r_useDepthTexture = glConfig.depthTextureAvailable;
				r_useCubeDepthTexture = glConfig.depthTextureCubeMapAvailable;
				r_usePackColorAsDepth = false;
				break;
			case 2:
				r_useDepthTexture = false;
				r_useCubeDepthTexture = false;
				r_usePackColorAsDepth = false;
				break;
			case 3:
				r_useDepthTexture = false;
				r_useCubeDepthTexture = false;
				r_usePackColorAsDepth = true;
				break;
			case 0:
			default:
				r_useDepthTexture = glConfig.depthTextureAvailable;
				r_useCubeDepthTexture = glConfig.depthTextureCubeMapAvailable;
				if (!r_useDepthTexture || !r_useCubeDepthTexture)
					r_usePackColorAsDepth = true;
				break;
		}
#ifdef GL_ES_VERSION_3_0
		common->Printf("Shadow mapping in OpenGLES2.0: \n");
		if(r_useCubeDepthTexture)
        {
            common->Printf("Using depth cubemap texture on point light.\n");
            depthShader_cube = &depthShader;
            depthPerforatedShader_cube = &depthPerforatedShader;
        }
		else
        {
            common->Printf("Using color cubemap texture on point light.\n");
            depthShader_cube = &depthShader_color;
            depthPerforatedShader_cube = &depthPerforatedShader_color;
        }
		if(r_useDepthTexture)
        {
            common->Printf("Using depth texture on non-point light.\n");
            depthShader_2d = &depthShader;
            depthPerforatedShader_2d = &depthPerforatedShader;
        }
		else
        {
            common->Printf("Using color texture on non-point light.\n");
            depthShader_2d = &depthShader_color;
            depthPerforatedShader_2d = &depthPerforatedShader_color;
        }
		if(r_usePackColorAsDepth)
			common->Printf("Store depth value with pack to RGBA if not using depth texture.\n");
		else
			common->Printf("Store depth value with RED component if not using depth texture.\n");
	}
#endif
#endif

#ifdef _STENCIL_SHADOW_IMPROVE
	r_stencilShadowTranslucent = harm_r_stencilShadowTranslucent.GetBool();
	f = harm_r_stencilShadowAlpha.GetFloat();
	if(f < 0.0f)
		f = 0.0f;
	if(f > 1.0f)
		f = 1.0f;
	r_stencilShadowAlpha = f;

	r_stencilShadowCombine = harm_r_stencilShadowCombine.GetBool();

#ifdef _SOFT_STENCIL_SHADOW
	// if(USING_GLES31)
	if(idStencilTexture::IsAvailable())
	{
		r_stencilShadowSoft = harm_r_stencilShadowSoft.GetBool();
		r_stencilShadowSoftCopyStencilBuffer = harm_r_stencilShadowSoftCopyStencilBuffer.GetBool();
	}
	else
	{
		common->Warning("Unsupport soft stencil shadow on OpenGL ES2.0!");
		r_stencilShadowSoft = false;
		harm_r_stencilShadowSoft.SetBool(false);
		harm_r_stencilShadowSoft.ClearModified();
		CVAR_READONLY(harm_r_stencilShadowSoft);
	}
#endif
#endif
}

void R_CheckBackEndCvars(void)
{
#ifdef _SHADOW_MAPPING
	if(r_useShadowMapping.IsModified())
	{
		r_shadowMapping = r_useShadowMapping.GetBool();
		r_useShadowMapping.ClearModified();
	}
	if(r_forceShadowMapsOnAlphaTestedSurfaces.IsModified())
	{
		r_shadowMapPerforated = r_forceShadowMapsOnAlphaTestedSurfaces.GetBool();
		r_forceShadowMapsOnAlphaTestedSurfaces.ClearModified();
	}
    if(harm_r_shadowMapCombine.IsModified())
    {
        r_shadowMapCombine = harm_r_shadowMapCombine.GetBool();
        harm_r_shadowMapCombine.ClearModified();
    }
	if(r_shadowMapSplits.IsModified())
	{
		r_shadowMapParallelSplitFrustums = r_shadowMapSplits.GetInteger();
		r_shadowMapSplits.ClearModified();
	}
#endif

#ifdef _STENCIL_SHADOW_IMPROVE
	if(harm_r_stencilShadowTranslucent.IsModified())
	{
		r_stencilShadowTranslucent = harm_r_stencilShadowTranslucent.GetBool();
		harm_r_stencilShadowTranslucent.ClearModified();
	}
	if(harm_r_stencilShadowAlpha.IsModified())
	{
		float f = harm_r_stencilShadowAlpha.GetFloat();
		if(f < 0.0f)
			f = 0.0f;
		if(f > 1.0f)
			f = 1.0f;
		r_stencilShadowAlpha = f;
		harm_r_stencilShadowAlpha.ClearModified();
	}

	if(harm_r_stencilShadowCombine.IsModified())
	{
		r_stencilShadowCombine = harm_r_stencilShadowCombine.GetBool();
		harm_r_stencilShadowCombine.ClearModified();
	}

#ifdef _SOFT_STENCIL_SHADOW
	// if(USING_GLES31)
	if(idStencilTexture::IsAvailable())
	{
		if(harm_r_stencilShadowSoft.IsModified())
		{
			r_stencilShadowSoft = harm_r_stencilShadowSoft.GetBool();
			harm_r_stencilShadowSoft.ClearModified();
		}
		if(harm_r_stencilShadowSoftCopyStencilBuffer.IsModified())
		{
			r_stencilShadowSoftCopyStencilBuffer = harm_r_stencilShadowSoftCopyStencilBuffer.GetBool();
			harm_r_stencilShadowSoftCopyStencilBuffer.ClearModified();
			if(r_stencilShadowSoftCopyStencilBuffer)
				common->Printf("Copy stencil buffer directly for soft stencil shadow.\n");
			else
				common->Printf("Copy depth buffer and render stencil buffer for soft stencil shadow.\n");
		}
		if(harm_r_stencilShadowSoftBias.IsModified())
		{
			r_stencilShadowSoftBias = RB_StencilShadowSoft_calcBIAS();
			harm_r_stencilShadowSoftBias.ClearModified();
		}
	}
	else
	{
		if(harm_r_stencilShadowSoft.IsModified())
		{
			r_stencilShadowSoft = false;
			harm_r_stencilShadowSoft.SetBool(false);
			harm_r_stencilShadowSoft.ClearModified();
			CVAR_READONLY(harm_r_stencilShadowSoft);
		}
	}
#endif
#endif
}

void R_GLSL_Init(void)
{
	glConfig.allowGLSLPath = false;

	common->Printf("---------- R_GLSL_Init ----------\n");

	if (!glConfig.GLSLAvailable) {
		common->Printf("Not available.\n");
		return;
	}

	common->Printf("Available.\n");

	R_InitGLSLCvars();
	common->Printf("---------------------------------\n");
}
