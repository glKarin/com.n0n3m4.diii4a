/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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

#include "sys/platform.h"
#include "renderer/VertexCache.h"

#include "renderer/tr_local.h"
#include "ShaderGL.h"
#include "idlib/containers/HashTable.h"

static struct Uniforms_t
{
	idMat4 projectionMatrix;
	idMat4 modelViewMatrix;
	idMat4 textureMatrix;

	idVec4 programEnv[NUM_PROG_ENV];
	idVec4 programLocal[NUM_PROG_LOCAL];

	unsigned int customPostValue;
} Uniforms;

static idShaderGL* currentShader = nullptr;

void R_GLSL_SetProgramEnv(GLuint index, const idVec4& params)
{
	Uniforms.programEnv[index] = params;
	if (currentShader)
	{
		currentShader->uniforms.programEnv[index] = Uniforms.programEnv[index];
	}
}

void R_GLSL_SetProgramLocal(GLuint index, const idVec4& params)
{
	Uniforms.programLocal[index] = params;
	if (currentShader)
	{
		currentShader->uniforms.programLocal[index] = Uniforms.programLocal[index];
	}
}

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
static void GLSL_SelectTextureNoClient( int unit ) {
	backEnd.glState.currenttmu = unit;
	qglActiveTexture( GL_TEXTURE0 + unit );
}

/*
==================
RB_GLSL_DrawInteraction
==================
*/
void	RB_GLSL_DrawInteraction( const drawInteraction_t *din ) {
	// load all the vertex program parameters
	R_GLSL_SetProgramEnv(PP_LIGHT_ORIGIN, din->localLightOrigin);
	R_GLSL_SetProgramEnv(PP_VIEW_ORIGIN, din->localViewOrigin);
	R_GLSL_SetProgramEnv(PP_LIGHT_PROJECT_S, din->lightProjection[0]);
	R_GLSL_SetProgramEnv(PP_LIGHT_PROJECT_T, din->lightProjection[1]);
	R_GLSL_SetProgramEnv(PP_LIGHT_PROJECT_Q, din->lightProjection[2]);
	R_GLSL_SetProgramEnv(PP_LIGHT_FALLOFF_S, din->lightProjection[3]);
	R_GLSL_SetProgramEnv(PP_BUMP_MATRIX_S, din->bumpMatrix[0]);
	R_GLSL_SetProgramEnv(PP_BUMP_MATRIX_T, din->bumpMatrix[1]);
	R_GLSL_SetProgramEnv(PP_DIFFUSE_MATRIX_S, din->diffuseMatrix[0]);
	R_GLSL_SetProgramEnv(PP_DIFFUSE_MATRIX_T, din->diffuseMatrix[1]);
	R_GLSL_SetProgramEnv(PP_SPECULAR_MATRIX_S, din->specularMatrix[0]);
	R_GLSL_SetProgramEnv(PP_SPECULAR_MATRIX_T, din->specularMatrix[1]);
	
	// <blendo> eric: ambient lighting shader frag param, i.e normal independent
	R_GLSL_SetProgramEnv(PP_AMBIENT_LEVEL, din->isAmbientLight);

	// testing fragment based normal mapping
	if ( r_testARBProgram.GetBool() ) {
		R_GLSL_SetProgramEnv(2, din->localLightOrigin);
		R_GLSL_SetProgramEnv(3, din->localViewOrigin);
	}

	R_GLSL_SetVertexColorEnv(din->vertexColor);

	// set the constant colors
	R_GLSL_SetProgramEnv(0, din->diffuseColor);
	R_GLSL_SetProgramEnv(1, din->specularColor);

	// set the textures

	// texture 1 will be the per-surface bump map
	GLSL_SelectTextureNoClient( 1 );
	din->bumpImage->Bind();

	// texture 2 will be the light falloff texture
	GLSL_SelectTextureNoClient( 2 );
	din->lightFalloffImage->Bind();

	// texture 3 will be the light projection texture
	GLSL_SelectTextureNoClient( 3 );
	din->lightImage->Bind();

	// texture 4 is the per-surface diffuse map
	GLSL_SelectTextureNoClient( 4 );
	din->diffuseImage->Bind();

	// texture 5 is the per-surface specular map
	GLSL_SelectTextureNoClient( 5 );
	din->specularImage->Bind();

	// draw it
	RB_DrawElementsWithCounters( din->surf->geo );
}


/*
=============
RB_GLSL_CreateDrawInteractions

=============
*/
void RB_GLSL_CreateDrawInteractions( const drawSurf_t *surf ) {
	if ( !surf ) {
		return;
	}

	bool bBlendLit = surf->material->TestMaterialFlag(MF_BLENDLIT);
	// blendo eric: use interaction shader with alpha and offset
    if (bBlendLit) {
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc);

		R_GLSL_SetActiveProgram("interactionLightBlend");

		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * surf->material->GetPolygonOffset());
	} else {
		// perform setup here that will be constant for all interactions
		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc);

		if (r_testARBProgram.GetBool()) {
			R_GLSL_SetActiveProgram("test");
		}
		else {
			R_GLSL_SetActiveProgram("interaction");
		}
	}

	R_GLSL_EnableVertexAttribs( 6 );

	// texture 0 is the normalization cube map for the vector towards the light
	GLSL_SelectTextureNoClient( 0 );
	if ( backEnd.vLight->lightShader->IsAmbientLight() || backEnd.vLight->lightDef->parms.isAmbient ) {
		globalImages->ambientNormalMap->Bind();
	} else {
		globalImages->normalCubeMapImage->Bind();
	}

	// texture 6 is the specular lookup table
	GLSL_SelectTextureNoClient( 6 );
	if ( r_testARBProgram.GetBool() ) {
		globalImages->specular2DTableImage->Bind();	// variable specularity in alpha channel
	} else {
		globalImages->specularTableImage->Bind();
	}


	for ( ; surf ; surf=surf->nextOnLight ) {
		// perform setup here that will not change over multiple interaction passes

		// set the vertex pointers
		idDrawVert	*ac = (idDrawVert *)vertexCache.Position( surf->geo->ambientCache );
		R_GLSL_SetVertexAttribs(ac);

		// this may cause RB_GLSL_DrawInteraction to be executed multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf, RB_GLSL_DrawInteraction );
	}

	// disable features
	GLSL_SelectTextureNoClient( 6 );
	globalImages->BindNull();

	GLSL_SelectTextureNoClient( 5 );
	globalImages->BindNull();

	GLSL_SelectTextureNoClient( 4 );
	globalImages->BindNull();

	GLSL_SelectTextureNoClient( 3 );
	globalImages->BindNull();

	GLSL_SelectTextureNoClient( 2 );
	globalImages->BindNull();

	GLSL_SelectTextureNoClient( 1 );
	globalImages->BindNull();

	backEnd.glState.currenttmu = -1;
	GLSL_SelectTextureNoClient( 0 );

	// blendo eric: reset offset on interaction blend
	if (bBlendLit) {
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}

	R_GLSL_DisableVertexAttribs();
}


// blendo eric: shader params, set state, render pass for ambient mood lights
/*
==================
RB_GLSL_DrawInteractions
==================
*/
void RB_GLSL_DrawInteractions( void ) {
	viewLight_t		*vLight;

	GLSL_SelectTextureNoClient( 0 );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	//
	// for each light, perform adding and shadowing
	//
	for ( vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
		backEnd.vLight = vLight;

		// do fogging later
		if ( vLight->lightShader->IsFogLight() ) {
			continue;
		}
		if ( vLight->lightShader->IsBlendLight() ) {
			continue;
		}

		if ( !vLight->localInteractions && !vLight->globalInteractions
			&& !vLight->translucentInteractions ) {
			continue;
		}
		
		bool bAmbient = vLight->lightDef->parms.isAmbient;

		// blendo eric todo: shouldn't need stencil at all for ambient lights?
		// clear the stencil buffer if needed
		if ( vLight->globalShadows || vLight->localShadows || bAmbient) {
			backEnd.currentScissor = vLight->scissorRect;
			if ( r_useScissor.GetBool() ) {
				RB_SetScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
					backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
					backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
					backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
			}
			qglClear( GL_STENCIL_BUFFER_BIT );
		} else {
			// no shadows, so no need to read or write the stencil buffer
			// we might in theory want to use GL_ALWAYS instead of disabling
			// completely, to satisfy the invarience rules
			qglStencilFunc( GL_ALWAYS, 128, 255 );
		}

		if (!bAmbient)
		{
			R_GLSL_SetActiveProgram("shadow");
			RB_StencilShadowPass(vLight->globalShadows);
		}
		RB_GLSL_CreateDrawInteractions( vLight->localInteractions );

		if (!bAmbient)
		{
			R_GLSL_SetActiveProgram("shadow");
			RB_StencilShadowPass(vLight->localShadows);
		}
		RB_GLSL_CreateDrawInteractions( vLight->globalInteractions );

		// translucent surfaces never get stencil shadowed
		if ( r_skipTranslucent.GetBool() ) {
			continue;
		}

		qglStencilFunc( GL_ALWAYS, 128, 255 );

		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_GLSL_CreateDrawInteractions( vLight->translucentInteractions );

		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}

	// disable stencil shadow test
	qglStencilFunc( GL_ALWAYS, 128, 255 );

	GLSL_SelectTextureNoClient( 0 );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	R_GLSL_DisablePrograms();
}

// blendo eric: new blend pass for blended lit interactions (wall decals or possibly future blended objects)
/*
==================
RB_GLSL_DrawInteractionsBlend
==================
*/
void RB_GLSL_DrawInteractionsBlend() {
	viewLight_t		*vLight;

	GLSL_SelectTextureNoClient(0);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	// blendo eric: blended interaction pass
	for (vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next) {
		backEnd.vLight = vLight;

		// do fogging later
		if (vLight->lightShader->IsFogLight()) {
			continue;
		}
		if (vLight->lightShader->IsBlendLight()) {
			continue;
		}
		if ( !vLight->blendLitInteractions) {
			continue;
		}

		bool bAmbient = vLight->lightDef->parms.isAmbient;

		// blendo eric todo: shouldn't need stencil at all for ambient lights?
		// clear the stencil buffer if needed
		if (vLight->globalShadows || vLight->localShadows || bAmbient) {
			backEnd.currentScissor = vLight->scissorRect;
			if (r_useScissor.GetBool()) {
				RB_SetScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
					backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
					backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
					backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
			}
			qglClear(GL_STENCIL_BUFFER_BIT);
		}
		else {
			// no shadows, so no need to read or write the stencil buffer
			// we might in theory want to use GL_ALWAYS instead of disabling
			// completely, to satisfy the invarience rules
			qglStencilFunc(GL_ALWAYS, 128, 255);
		}

		if (!bAmbient)
		{
			R_GLSL_SetActiveProgram("shadow");
			RB_StencilShadowPass(vLight->globalShadows);
			RB_StencilShadowPass(vLight->localShadows);
		}

		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_GLSL_CreateDrawInteractions(vLight->blendLitInteractions);
		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;

		qglStencilFunc(GL_ALWAYS, 128, 255);
	}

	backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;

	// disable stencil shadow test
	qglStencilFunc(GL_ALWAYS, 128, 255);

	GLSL_SelectTextureNoClient(0);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	R_GLSL_DisablePrograms();
}

//===================================================================================

idHashTable<idShaderGL*> shaderTable;
idStr globalShaders[] = {
	"test",
	"interaction",
	"bumpyEnvironment",
	"ambientLight",
	"shadow",
	"environment",
	"glasswarp",
	"interactionLightBlend",
	"depthOnly",
	"basicSurf",
	"basicCube",
	"basicObjectLinear",
	"fogLight",
	"customMaskPass"
};

/*
=================
R_LoadGLSLProgram
=================
*/
bool R_LoadGLSLProgram( const idStr& progName ) {
	idShaderGL** oldShader = nullptr;
	if (shaderTable.Get(progName, &oldShader))
	{
		// Delete the old one
		(*oldShader)->Unload();
		delete (*oldShader);
		shaderTable.Remove(progName);
	}

	idShaderGL* shader = new idShaderGL();
	if (shader->LoadCombined(progName))
	{
		shaderTable.Set(progName, shader);
	}
	else
	{
		shader->Unload();
		delete shader;
		return false;
	}

	return true;
}

/*
==================
R_FindGLSLProgram

Returns true if successfully finds the program of the specified name
==================
*/
bool R_FindGLSLProgram( const idStr& program ) {
	if (shaderTable.Get(program, nullptr))
	{
		// Already loaded
		return true;
	}

	return R_LoadGLSLProgram(program);
}

/*
==================
R_ReloadGLSLPrograms_f
==================
*/
void R_ReloadGLSLPrograms_f( const idCmdArgs &args ) {
	common->Printf( "----- R_ReloadGLSLPrograms -----\n" );
	for (auto& name : globalShaders)
	{
		R_LoadGLSLProgram(name);
	}
}

/*
==================
R_GLSL_Init

==================
*/
void R_GLSL_Init( void ) {
	glConfig.allowGLSLPath = false;

	common->Printf( "GLSL renderer: " );

	if ( !glConfig.GLSLAvailable ) {
		common->Printf( "Not available.\n" );
		return;
	}

	common->Printf( "Available.\n" );

	glConfig.allowGLSLPath = true;

	// Set the defaults for our cache
	Uniforms.projectionMatrix = mat4_identity;
	Uniforms.modelViewMatrix = mat4_identity;
	Uniforms.textureMatrix = mat4_identity;
	for (int i = 0; i < NUM_PROG_ENV; i++)
	{
		Uniforms.programEnv[i] = vec4_zero;
	}

	for (int i = 0; i < NUM_PROG_LOCAL; i++)
	{
		Uniforms.programLocal[i] = vec4_zero;
	}

	Uniforms.customPostValue = 0;
}

void R_GLSL_Shutdown( void ) {
	for (int i = 0; i < shaderTable.Num(); i++) {
		delete *shaderTable.GetIndex(i);
	}
	shaderTable.Clear();
}

static void R_GLSL_CopyUniformsToCurrent()
{
	currentShader->uniforms.projectionMatrix = Uniforms.projectionMatrix;
	currentShader->uniforms.modelviewMatrix = Uniforms.modelViewMatrix;
	currentShader->uniforms.textureMatrix = Uniforms.textureMatrix;

	for (int i = 0; i < NUM_PROG_ENV; i++)
	{
		currentShader->uniforms.programEnv[i] = Uniforms.programEnv[i];
	}

	for (int i = 0; i < NUM_PROG_LOCAL; i++)
	{
		currentShader->uniforms.programLocal[i] = Uniforms.programLocal[i];
	}

	currentShader->uniforms.customPostValue = Uniforms.customPostValue;
}

void R_GLSL_SetActiveProgram(const idStr& program)
{
	idShaderGL** shader = nullptr;
	if (shaderTable.Get(program, &shader))
	{
		idShaderGL* newShader = *shader;
		if (newShader != currentShader)
		{
#if 0 //karin: debug current shader
			printf("Use program : %s\n", newShader->Name());
#endif
			currentShader = newShader;
			(*shader)->SetActive();
			R_GLSL_CopyUniformsToCurrent();
		}
	}
	else
	{
		common->Warning("R_GLSL_SetActiveProgram trying to activate program %s which does not exist!\n", program.c_str());
		qglUseProgram(0);
		currentShader = nullptr;
	}
}

void R_GLSL_DisablePrograms()
{
	qglUseProgram(0);
	currentShader = nullptr;
}

void R_GLSL_SetVertexAttribs(idDrawVert* verts)
{
	qglVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(idDrawVert), verts->xyz.ToFloatPtr());
	qglVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(idDrawVert), verts->st.ToFloatPtr());
	qglVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(idDrawVert), verts->normal.ToFloatPtr());
	qglVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(idDrawVert), verts->tangents[0].ToFloatPtr());
	qglVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(idDrawVert), verts->tangents[1].ToFloatPtr());
	qglVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, true, sizeof(idDrawVert), verts->color);
}

void R_GLSL_LoadMatrix(const float* matrix, GLenum matrixMode)
{
	switch (matrixMode)
	{
	case GL_MODELVIEW:
		memcpy(&Uniforms.modelViewMatrix[0], matrix, 4 * 4 * sizeof(float));
		if (currentShader)
		{
			currentShader->uniforms.modelviewMatrix = Uniforms.modelViewMatrix;
		}
		break;
	case GL_PROJECTION:
		memcpy(&Uniforms.projectionMatrix[0], matrix, 4 * 4 * sizeof(float));
		if (currentShader)
		{
			currentShader->uniforms.projectionMatrix = Uniforms.projectionMatrix;
		}
		break;
	case GL_TEXTURE:
		memcpy(&Uniforms.textureMatrix[0], matrix, 4 * 4 * sizeof(float));
		if (currentShader)
		{
			currentShader->uniforms.textureMatrix = Uniforms.textureMatrix;
		}
		break;
	default:
		break;
	}
}

void R_GLSL_SetCustomPostValue(unsigned int value)
{
	Uniforms.customPostValue = value;
	if (currentShader)
	{
		currentShader->uniforms.customPostValue = Uniforms.customPostValue;
	}
}

void R_GLSL_EnableVertexAttribs(int count)
{
	if (count > 6)
	{
		common->Warning("R_GLSL_EnableVertexAttribs doesn't support more than 6 attributes\n");
		count = 6;
	}

	for (int i = 0; i < count; i++)
	{
		qglEnableVertexAttribArray(i);
	}
}

void R_GLSL_DisableVertexAttribs()
{
	for (int i = 0; i < 6; i++)
	{
		qglDisableVertexAttribArray(i);
	}
}

// Second parameter is what you pass to glColor4fv in fixed-function
void R_GLSL_SetVertexColorEnv(stageVertexColor_t vertexColor, idVec4 color /*= idVec4(1.0f, 1.0f, 1.0f, 1.0f)*/)
{
	static const idVec4 zero(0, 0, 0, 0);
	static const idVec4 one(1, 1, 1, 1);
	static const idVec4 negOne(-1, -1, -1, -1);

	switch (vertexColor) {
	case SVC_IGNORE:
		Uniforms.programEnv[PP_COLOR_MODULATE] = zero;
		Uniforms.programEnv[PP_COLOR_ADD] = one;
		break;
	case SVC_MODULATE:
		Uniforms.programEnv[PP_COLOR_MODULATE] = one;
		Uniforms.programEnv[PP_COLOR_ADD] = zero;
		break;
	case SVC_INVERSE_MODULATE:
		Uniforms.programEnv[PP_COLOR_MODULATE] = negOne;
		Uniforms.programEnv[PP_COLOR_ADD] = one;
		break;
	}

	Uniforms.programEnv[PP_COLOR_GLSL] = color;

	if (currentShader)
	{
		currentShader->uniforms.programEnv[PP_COLOR_MODULATE] = Uniforms.programEnv[PP_COLOR_MODULATE];
		currentShader->uniforms.programEnv[PP_COLOR_ADD] = Uniforms.programEnv[PP_COLOR_ADD];
		currentShader->uniforms.programEnv[PP_COLOR_GLSL] = Uniforms.programEnv[PP_COLOR_GLSL];
	}
}
