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

static bool r_usePhong = true;
static float r_specularExponent = 4.0f;

#if 1
#define ENABLE_STENCIL_TEST() qglEnable(GL_STENCIL_TEST);
#define DISABLE_STENCIL_TEST() qglDisable(GL_STENCIL_TEST);
#else
#define ENABLE_STENCIL_TEST() // qglStencilFunc(GL_GEQUAL, 128, 255);
#define DISABLE_STENCIL_TEST() qglStencilFunc(GL_ALWAYS, 128, 255);
#endif

#define HARM_INTERACTION_SHADER_PHONG "phong"
#define HARM_INTERACTION_SHADER_BLINNPHONG "blinn_phong"
const char *harm_r_lightModelArgs[]	= { HARM_INTERACTION_SHADER_PHONG, HARM_INTERACTION_SHADER_BLINNPHONG, NULL };
static idCVar harm_r_lightModel("harm_r_lightModel", harm_r_lightModelArgs[0], CVAR_RENDERER|CVAR_ARCHIVE, "[Harmattan]: Light model when draw interactions(`phong` - Phong(default), `blinn_phong` - Blinn-Phong.)", harm_r_lightModelArgs, idCmdSystem::ArgCompletion_String<harm_r_lightModelArgs>);
static idCVar harm_r_specularExponent("harm_r_specularExponent", "4.0", CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "[Harmattan]: Specular exponent in interaction light model(Phong default is 4, Blinn-Phong default is 12.)");

#include "glsl/draw_glsl_backend.cpp"

#ifdef _SHADOW_MAPPING
#include "glsl/draw_glsl_shadowmapping.cpp"
#endif
#ifdef _TRANSLUCENT_STENCIL_SHADOW
#include "glsl/draw_glsl_stencilshadow.cpp"
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

	const char *lightModel = harm_r_lightModel.GetString();
	r_usePhong = !(lightModel && !idStr::Icmp(HARM_INTERACTION_SHADER_BLINNPHONG, lightModel));

	f = harm_r_specularExponent.GetFloat();
	if(f <= 0.0f)
		f = 4.0f;
	r_specularExponent = f;

#ifdef _SHADOW_MAPPING
	r_shadowMapping = r_useShadowMapping.GetBool();
	r_shadowMapPerforated = r_forceShadowMapsOnAlphaTestedSurfaces.GetBool();
#ifdef _CONTROL_SHADOW_MAPPING_RENDERING
	int i = harm_r_shadowMappingScheme.GetInteger();
    if(i < 0 || i > SHADOW_MAPPING_NON_PRELIGHT)
        i = SHADOW_MAPPING_PURE;
    r_shadowMappingScheme = i;
#endif
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
		common->Printf("[Harmattan]: Shadow mapping in OpenGLES2.0: \n");
		if(r_useCubeDepthTexture)
        {
            common->Printf("[Harmattan]: Using depth cubemap texture on point light.\n");
            depthShader_cube = &depthShader;
            depthPerforatedShader_cube = &depthPerforatedShader;
        }
		else
        {
            common->Printf("[Harmattan]: Using color cubemap texture on point light.\n");
            depthShader_cube = &depthShader_color;
            depthPerforatedShader_cube = &depthPerforatedShader_color;
        }
		if(r_useDepthTexture)
        {
            common->Printf("[Harmattan]: Using depth texture on non-point light.\n");
            depthShader_2d = &depthShader;
            depthPerforatedShader_2d = &depthPerforatedShader;
        }
		else
        {
            common->Printf("[Harmattan]: Using color texture on non-point light.\n");
            depthShader_2d = &depthShader_color;
            depthPerforatedShader_2d = &depthPerforatedShader_color;
        }
		if(r_usePackColorAsDepth)
			common->Printf("[Harmattan]: Store depth value with pack to RGBA if not using depth texture.\n");
		else
			common->Printf("[Harmattan]: Store depth value with RED component if not using depth texture.\n");
	}
#endif
#endif

#ifdef _TRANSLUCENT_STENCIL_SHADOW
	r_stencilShadowTranslucent = harm_r_stencilShadowTranslucent.GetBool();
	f = harm_r_stencilShadowAlpha.GetFloat();
	if(f < 0.0f)
		f = 0.0f;
	if(f > 1.0f)
		f = 1.0f;
	r_stencilShadowAlpha = f;
#endif
	r_stencilShadowCombine = harm_r_stencilShadowCombine.GetBool();
}

void R_CheckBackEndCvars(void)
{
	if(harm_r_lightModel.IsModified())
	{
		const char *lightModel = harm_r_lightModel.GetString();
		r_usePhong = !(lightModel && !idStr::Icmp(HARM_INTERACTION_SHADER_BLINNPHONG, lightModel));
		harm_r_lightModel.ClearModified();
	}

	if(harm_r_specularExponent.IsModified())
	{
		float f = harm_r_specularExponent.GetFloat();
		if(f <= 0.0f)
			f = 4.0f;
		r_specularExponent = f;
		harm_r_specularExponent.ClearModified();
	}

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
#ifdef _CONTROL_SHADOW_MAPPING_RENDERING
	if(harm_r_shadowMappingScheme.IsModified())
	{
        int i = harm_r_shadowMappingScheme.GetInteger();
        if(i < 0 || i > SHADOW_MAPPING_NON_PRELIGHT)
            i = SHADOW_MAPPING_PURE;
        r_shadowMappingScheme = i;
		harm_r_shadowMappingScheme.ClearModified();
	}
#endif
#endif

#ifdef _TRANSLUCENT_STENCIL_SHADOW
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
#endif
	if(harm_r_stencilShadowCombine.IsModified())
	{
		r_stencilShadowCombine = harm_r_stencilShadowCombine.GetBool();
		harm_r_stencilShadowCombine.ClearModified();
	}
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
