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

#include "draw_glsl_shader.cpp"

static bool r_usePhong = true;
static float r_specularExponent = 4.0f;

static const float zero[4] = { 0, 0, 0, 0 };
static const float one[4] = { 1, 1, 1, 1 };
static const float negOne[4] = { -1, -1, -1, -1 };

#define HARM_INTERACTION_SHADER_PHONG "phong"
#define HARM_INTERACTION_SHADER_BLINNPHONG "blinn_phong"
const char *harm_r_lightModelArgs[]	= { HARM_INTERACTION_SHADER_PHONG, HARM_INTERACTION_SHADER_BLINNPHONG, NULL };
static idCVar harm_r_lightModel("harm_r_lightModel", harm_r_lightModelArgs[0], CVAR_RENDERER|CVAR_ARCHIVE, "[Harmattan]: Light model when draw interactions(`phong` - Phong(default), `blinn_phong` - Blinn-Phong.)", harm_r_lightModelArgs, idCmdSystem::ArgCompletion_String<harm_r_lightModelArgs>);
static idCVar harm_r_specularExponent("harm_r_specularExponent", "4.0", CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "[Harmattan]: Specular exponent in interaction light model(default is 4.0.)");

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

/*
====================
GL_SelectTextureNoClient
====================
*/
ID_INLINE static void GL_SelectTextureNoClient(int unit)
{
	backEnd.glState.currenttmu = unit;
	qglActiveTexture(GL_TEXTURE0 + unit);
	RB_LogComment("qglActiveTexture( %i )\n", unit);
}

ID_INLINE void			GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h )
{
	qglScissor( x, y, w, h );
}

ID_INLINE void			GL_Viewport( int x /* left */, int y /* bottom */, int w, int h )
{
	qglViewport( x, y, w, h );
}

ID_INLINE void	GL_Scissor( const idScreenRect& rect )
{
	GL_Scissor( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 );
}
ID_INLINE void	GL_Viewport( const idScreenRect& rect )
{
	GL_Viewport( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 );
}
ID_INLINE void	GL_ViewportAndScissor( int x, int y, int w, int h )
{
	GL_Viewport( x, y, w, h );
	GL_Scissor( x, y, w, h );
}
ID_INLINE void	GL_ViewportAndScissor( const idScreenRect& rect )
{
	GL_Viewport( rect );
	GL_Scissor( rect );
}

/*
================
RB_SetMVP
================
*/
ID_INLINE void RB_SetMVP( const float mvp[16] )
{
	GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mvp);
}

/*
====================
GL_PolygonOffset
====================
*/
ID_INLINE void GL_PolygonOffset( bool enabled, float scale = 0.0f, float bias = 0.0f )
{
	if(enabled)
	{
		qglPolygonOffset( scale, bias );
		qglEnable(GL_POLYGON_OFFSET_FILL);
	}
	else
		qglDisable(GL_POLYGON_OFFSET_FILL);
}

/*
====================
GL_Color
====================
*/
ID_INLINE void GL_Color( float r, float g, float b, float a )
{
	float parm[4];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, r );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, g );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, b );
	parm[3] = idMath::ClampFloat( 0.0f, 1.0f, a );
	GL_Uniform4fv(offsetof(shaderProgram_t, glColor), parm);
}

// RB begin
ID_INLINE void GL_Color( const idVec3& color )
{
	GL_Color( color[0], color[1], color[2], 1.0f );
}

ID_INLINE void GL_Color( const idVec4& color )
{
	GL_Color( color[0], color[1], color[2], color[3] );
}
// RB end

/*
====================
GL_Color
====================
*/
ID_INLINE void GL_Color( float r, float g, float b )
{
	GL_Color( r, g, b, 1.0f );
}

#ifdef _SHADOW_MAPPING
#include "draw_glsl_shadowmapping.cpp"
#endif

/*
==================
RB_GLSL_DrawInteraction
==================
*/
void	RB_GLSL_DrawInteraction(const drawInteraction_t *din)
{
	// load all the vertex program parameters
	GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, localLightOrigin), din->localLightOrigin.ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, localViewOrigin), din->localViewOrigin.ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, lightProjectionS), din->lightProjection[0].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, lightProjectionT), din->lightProjection[1].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, lightProjectionQ), din->lightProjection[2].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, lightFalloff), din->lightProjection[3].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, bumpMatrixS), din->bumpMatrix[0].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, bumpMatrixT), din->bumpMatrix[1].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, diffuseMatrixS), din->diffuseMatrix[0].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, diffuseMatrixT), din->diffuseMatrix[1].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, specularMatrixS), din->specularMatrix[0].ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, specularMatrixT), din->specularMatrix[1].ToFloatPtr());

	switch (din->vertexColor) {
		case SVC_MODULATE:
			GL_Uniform4fv(offsetof(shaderProgram_t, colorModulate), one);
			GL_Uniform4fv(offsetof(shaderProgram_t, colorAdd), zero);
			break;
		case SVC_INVERSE_MODULATE:
			GL_Uniform4fv(offsetof(shaderProgram_t, colorModulate), negOne);
			GL_Uniform4fv(offsetof(shaderProgram_t, colorAdd), one);
			break;
		case SVC_IGNORE:
		default:
			GL_Uniform4fv(offsetof(shaderProgram_t, colorModulate), zero);
			GL_Uniform4fv(offsetof(shaderProgram_t, colorAdd), one);
			break;
	}

	// set the constant colors
	GL_Uniform4fv(offsetof(shaderProgram_t, diffuseColor), din->diffuseColor.ToFloatPtr());
	GL_Uniform4fv(offsetof(shaderProgram_t, specularColor), din->specularColor.ToFloatPtr());

	// material may be NULL for shadow volumes
	GL_Uniform1fv(offsetof(shaderProgram_t, specularExponent), &r_specularExponent);

	// set the textures

	// texture 0 will be the per-surface bump map
	GL_SelectTextureNoClient(0);
	din->bumpImage->Bind();

	// texture 1 will be the light falloff texture
	GL_SelectTextureNoClient(1);
	din->lightFalloffImage->Bind();

	// texture 2 will be the light projection texture
	GL_SelectTextureNoClient(2);
	din->lightImage->Bind();

	// texture 3 is the per-surface diffuse map
	GL_SelectTextureNoClient(3);
	din->diffuseImage->Bind();

	// texture 4 is the per-surface specular map
	GL_SelectTextureNoClient(4);
	din->specularImage->Bind();

#ifdef _SHADOW_MAPPING
	if(r_shadowMapping)
	{
		RB_ShadowMappingInteraction_setupMVP(din);

		// texture 6 is the shadow map
		GL_SelectTextureNoClient(6);
		RB_ShadowMappingInteraction_bindTexture();
	}
#endif

	GL_SelectTextureNoClient(0); //k2023

	// draw it
	RB_DrawElementsWithCounters(din->surf->geo);
}


/*
=============
RB_GLSL_CreateDrawInteractions

=============
*/
void RB_GLSL_CreateDrawInteractions(const drawSurf_t *surf)
{
	if (!surf) {
		return;
	}

	// perform setup here that will be constant for all interactions
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE |
			GLS_DEPTHMASK | //k: fix translucent interactions
			backEnd.depthFunc);

	// bind the vertex and fragment shader
	if(r_usePhong)
		GL_UseProgram(&interactionShader);
	else
		GL_UseProgram(&interactionBlinnPhongShader);

	// enable the vertex arrays
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));	// gl_Vertex
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color

	// texture 5 is the specular lookup table
	GL_SelectTextureNoClient(5);
	globalImages->specularTableImage->Bind();

	backEnd.currentSpace = NULL; //k2023

	for (; surf ; surf=surf->nextOnLight) {
		// perform setup here that will not change over multiple interaction passes

		// set the modelview matrix for the viewer
		/*float   mat[16];
		myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);*/ //k2023

		// set the vertex pointers
		idDrawVert	*ac = (idDrawVert *)vertexCache.Position(surf->geo->ambientCache);

		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Normal), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Bitangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[1].ToFloatPtr());
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Tangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[0].ToFloatPtr());
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());

		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), ac->color);

		// this may cause RB_GLSL_DrawInteraction to be exacuted multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions(surf, RB_GLSL_DrawInteraction);
	}

	backEnd.currentSpace = NULL; //k2023

	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));	// gl_Vertex
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color

	// disable features
	GL_SelectTextureNoClient(5);
	globalImages->BindNull();

	GL_SelectTextureNoClient(4);
	globalImages->BindNull();

	GL_SelectTextureNoClient(3);
	globalImages->BindNull();

	GL_SelectTextureNoClient(2);
	globalImages->BindNull();

	GL_SelectTextureNoClient(1);
	globalImages->BindNull();

	backEnd.glState.currenttmu = -1;
	GL_SelectTexture(0);

	GL_UseProgram(NULL);
}

/*
==================
RB_GLSL_DrawInteractions
==================
*/
void RB_GLSL_DrawInteractions(void)
{
	viewLight_t		*vLight;
	const idMaterial	*lightShader;

	//GL_SelectTexture(0); //k2023

#ifdef _SHADOW_MAPPING
	const bool shadowMapping = r_shadowMapping && r_shadows.GetBool();
	float clearColor[4];
	if(shadowMapping)
        RB_getClearColor(clearColor);
#endif
	//
	// for each light, perform adding and shadowing
	//
	for (vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next) 
	{
		backEnd.vLight = vLight;

		// do fogging later
		if (vLight->lightShader->IsFogLight()) {
			continue;
		}

		if (vLight->lightShader->IsBlendLight()) {
			continue;
		}

		if (!vLight->localInteractions && !vLight->globalInteractions
		    && !vLight->translucentInteractions) {
			continue;
		}

		lightShader = vLight->lightShader;

		// clear the stencil buffer if needed
		if ((vLight->globalShadows || vLight->localShadows)&&(r_shadows.GetBool())) {
			backEnd.currentScissor = vLight->scissorRect;

			if (r_useScissor.GetBool()) {
				qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
				          backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
				          backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
				          backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
			}
			qglClear(GL_STENCIL_BUFFER_BIT);
		} else {
			// no shadows, so no need to read or write the stencil buffer
			// we might in theory want to use GL_ALWAYS instead of disabling
			// completely, to satisfy the invarience rules
			if (r_shadows.GetBool())
			qglStencilFunc(GL_ALWAYS, 128, 255);
		}

#ifdef _SHADOW_MAPPING
		if(shadowMapping && vLight->shadowLOD >= 0)
		{
            int	side, sideStop;

            if( vLight->parallel )
            {
                side = 0;
                //sideStop = r_shadowMapSplits.GetInteger() + 1;
				sideStop = 1;
            }
            else if( vLight->pointLight )
            {
                if( r_shadowMapSingleSide.GetInteger() != -1 )
                {
                    side = r_shadowMapSingleSide.GetInteger();
                    sideStop = side + 1;
                }
                else
                {
                    side = 0;
                    sideStop = 6;
                }
            }
            else
            {
                side = -1;
                sideStop = 0;
            }

			if(vLight->globalShadows || vLight->localShadows)
			{
                qglDisable(GL_STENCIL_TEST);
				extern char RB_ShadowMapPass_T;

				RB_ShadowMapPass_T = 'G';
				for( int m = side; m < sideStop ; m++ )
				{
					RB_ShadowMapPass( vLight->globalShadows, m, true );
				}

				RB_ShadowMapPass_T = 'L';
				for( int m = side; m < sideStop ; m++ )
				{
					RB_ShadowMapPass( vLight->localShadows, m, false );
				}

				RB_GLSL_CreateDrawInteractions_shadowMapping(vLight->localInteractions);
				RB_GLSL_CreateDrawInteractions_shadowMapping(vLight->globalInteractions);

                qglEnable(GL_STENCIL_TEST);
			}
			else
			{
				RB_GLSL_CreateDrawInteractions(vLight->localInteractions);
				RB_GLSL_CreateDrawInteractions(vLight->globalInteractions);
			}
		}
		else
#endif
		{
            RB_StencilShadowPass(vLight->globalShadows);
            RB_GLSL_CreateDrawInteractions(vLight->localInteractions);

			RB_StencilShadowPass(vLight->localShadows);
			RB_GLSL_CreateDrawInteractions(vLight->globalInteractions);
		}

		//k GL_UseProgram(NULL);	// if there weren't any globalInteractions, it would have stayed on
		// translucent surfaces never get stencil shadowed
		if (r_skipTranslucent.GetBool()) {
			continue;
		}
		if (r_shadows.GetBool())
		qglStencilFunc(GL_ALWAYS, 128, 255);

		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_GLSL_CreateDrawInteractions(vLight->translucentInteractions);

		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}

	// disable stencil shadow test
	if (r_shadows.GetBool())
	qglStencilFunc(GL_ALWAYS, 128, 255);

	//GL_SelectTexture(0); //k2023
#ifdef _SHADOW_MAPPING
	if(shadowMapping)
        qglClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
#endif
}

//===================================================================================

/*
==================
R_InitGLSLCvars
==================
*/
static void R_InitGLSLCvars(void)
{
	const char *lightModel = harm_r_lightModel.GetString();
	r_usePhong = !(lightModel && !idStr::Icmp(HARM_INTERACTION_SHADER_BLINNPHONG, lightModel));

	float f = harm_r_specularExponent.GetFloat();
	if(f <= 0.0f)
		f = 4.0f;
	r_specularExponent = f;

#ifdef _SHADOW_MAPPING
	r_shadowMapping = r_useShadowMapping.GetBool();
    r_shadowMapPointLight = harm_r_shadowMapPointLight.GetInteger();
#endif
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
    if(harm_r_shadowMapPointLight.IsModified())
    {
        r_shadowMapPointLight = harm_r_shadowMapPointLight.GetInteger();
        harm_r_shadowMapPointLight.ClearModified();
    }
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
