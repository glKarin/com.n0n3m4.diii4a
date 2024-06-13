
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../tr_local.h"

#ifdef _STENCIL_SHADOW_IMPROVE

static idCVar harm_r_stencilShadowCombine( "harm_r_stencilShadowCombine", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "combine local and global stencil shadow" );
static bool r_stencilShadowCombine = false;

static bool r_stencilShadowTranslucent = false;
static float r_stencilShadowAlpha = 0.5f;

static void RB_GLSL_CreateDrawInteractions_translucentStencilShadow(const drawSurf_t *surf, bool noStencilTest);

void RB_GLSL_CreateDrawInteractions_translucentStencilShadow(const drawSurf_t *surf, bool noStencilTest)
{
	if (!surf) {
		return;
	}

	if(noStencilTest)
		qglDisable(GL_STENCIL_TEST);
	// perform setup here that will be constant for all interactions
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE |
			 GLS_DEPTHMASK | //k: fix translucent interactions
			 backEnd.depthFunc);

	// bind the vertex and fragment shader
	if(r_usePhong)
		GL_UseProgram(&interactionTranslucentShader);
	else
		GL_UseProgram(&interactionBlinnPhongTranslucentShader);

	// enable the vertex arrays
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));	// gl_Vertex
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color

	GL_Uniform1f(offsetof(shaderProgram_t, u_uniformParm[0]), noStencilTest ? r_stencilShadowAlpha : 1.0f - r_stencilShadowAlpha);

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

	if(noStencilTest)
		qglEnable(GL_STENCIL_TEST);
}
#endif

#ifdef _SOFT_STENCIL_SHADOW
static bool r_stencilShadowSoft = false;

ID_INLINE static float RB_StencilShadowSoft_calcBIAS(void)
{
    float f = harm_r_stencilShadowSoftBias.GetFloat();
    if(f < 0)
    {
        float min = 1.0f;
        float val = idMath::Ceil((float)stencilTexture.Width() / 1024.0);
        return val > min ? val : min;
    }
    else
        return f;
}

static void RB_StencilShadowSoftInteraction_setupUniform(void)
{
	int iw = stencilTexture.UploadWidth();
	int ih = stencilTexture.UploadHeight();
	float	parm[4];
	int		pot;

	// screen power of two correction factor, assuming the copy to _currentRender
	// also copied an extra row and column for the bilerp
	//int	 w = backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1;
	int	 w = stencilTexture.Width();
	pot = iw;
	parm[0] = (float)w / pot;

	//int	 h = backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1;
	int	 h = stencilTexture.Height();
	pot = ih;
	parm[1] = (float)h / pot;

	parm[2] = 1.0 / iw;
	parm[3] = 1.0 / ih;

	GL_Uniform4fv(offsetof(shaderProgram_t, nonPowerOfTwo), parm);

	// window coord to 0.0 to 1.0 conversion
	parm[0] = 1.0 / w;
	parm[1] = 1.0 / h;
	parm[2] = 0;
	parm[3] = 1;
	GL_Uniform4fv(offsetof(shaderProgram_t, windowCoords), parm);

    // alpha
    GL_Uniform1f(offsetof(shaderProgram_t, u_uniformParm[0]), 1.0 - r_stencilShadowAlpha);

    // bias
    GL_Uniform1f(offsetof(shaderProgram_t, u_uniformParm[1]), RB_StencilShadowSoft_calcBIAS());
}

void RB_GLSL_CreateDrawInteractions_softStencilShadow(const drawSurf_t *surf, int mask)
{
	if (!surf) {
		return;
	}

	if(mask & 1)
		qglDisable(GL_STENCIL_TEST);
	// perform setup here that will be constant for all interactions
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE |
			 GLS_DEPTHMASK | //k: fix translucent interactions
			 backEnd.depthFunc);

	// bind the vertex and fragment shader
	if(r_usePhong)
		GL_UseProgram(&interactionSoftShader);
	else
		GL_UseProgram(&interactionBlinnPhongSoftShader);

	// enable the vertex arrays
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));	// gl_Vertex
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color

	RB_StencilShadowSoftInteraction_setupUniform();

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
	GL_SelectTextureNoClient(6);
	globalImages->BindNull();

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

	if(mask & 2)
		qglEnable(GL_STENCIL_TEST);
}

ID_INLINE static void RB_StencilShadowSoft_copyStencilBuffer(void)
{
	stencilTexture.Blit(); // copy stencil buffer
	qglBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ID_INLINE static void RB_StencilShadowSoftInteraction_bindTexture(void)
{
	stencilTexture.Select();
}
#endif
