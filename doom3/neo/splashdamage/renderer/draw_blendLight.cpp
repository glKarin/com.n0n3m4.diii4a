#include "idlib/precompiled.h"

#include "renderer/tr_local.h"

#include "RenderProgram.h"
#include "RenderProgramManager.h"

static const sdRenderProgram *blendLightProgram = NULL;

static void RB_T_BlendLight_external(const drawSurf_t *surf)
{
	const srfTriangles_t *tri;
	const viewLight_t *vLight = backEnd.vLight;

	tri = surf->geo;

	// Setup the fogMatrix as being the local Light Projection
	// Only do this once per space
	if (backEnd.currentSpace != surf->space) {
		idPlane lightProject[4];

		int i;
		for (i = 0; i < 4; i++) {
			R_GlobalPlaneToLocal(surf->space->modelMatrix, vLight->lightProject[i], lightProject[i]);
		}

		blendLightProgram->BindVector("lightFalloff_0", lightProject[0].ToVec4());
		blendLightProgram->BindVector("lightFalloff_1", lightProject[1].ToVec4());
		blendLightProgram->BindVector("lightFalloff_2", lightProject[2].ToVec4());
		blendLightProgram->BindVector("lightFalloff_3", lightProject[3].ToVec4());
	}

	// This gets used for both blend lights and shadow draws
	if (tri->ambientCache) {
		idDrawVert *ac = (idDrawVert *) vertexCache.Position(tri->ambientCache);
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	} else if (tri->shadowCache) {
		shadowCache_t *sc = (shadowCache_t *) vertexCache.Position(tri->shadowCache);
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), sc->xyz.ToFloatPtr());
	}

#if 1
	if (surf->space->fadeFraction > 0.0f) {
		const float fade = 1.0f - surf->space->fadeFraction;
		blendLightProgram->BindVector("fadeFraction", fade);
	}
	else
#endif
	blendLightProgram->BindVector("fadeFraction", 1.0f);

	RB_DrawElementsWithCounters(tri);
}

void RB_BlendLight_external(const shaderStage_t *pStage, const drawSurf_t *drawSurfs,  const drawSurf_t *drawSurfs2)
{
	const viewLight_t *vLight = backEnd.vLight;
	const idMaterial * const lightShader = vLight->lightShader;
	const float * const regs = vLight->shaderRegisters;

	assert(pStage->renderProgram);

	if (!pStage->renderProgram->IsValid())
		return;

	shaderProgram_t *lastProgram = backEnd.glState.currentProgram;

	if(!pStage->renderProgram->Bind(pStage, lightShader, regs))
		return;

	blendLightProgram = pStage->renderProgram;

	int oldDrawBits = backEnd.glState.glStateBits;

	// Setup the drawState
	GL_State(GLS_DEPTHMASK | pStage->drawStateBits | GLS_DEPTHFUNC_EQUAL);
	//GL_State( pStage->drawStateBits );

	blendLightProgram->SetupState();

	GL_EnableVertexAttribArray(SHADER_PARM_ADDR(attr_Vertex));

	// Texture 1 will get the falloff texture
	blendLightProgram->BindImage("lightFalloffMap", vLight->falloffImage);

	// Bind the projected texture
	blendLightProgram->BindImage("lightProjectionMap", pStage->texture.image);

	// Setup the Fog Color
	float lightColor[4];
	lightColor[0] = regs[pStage->color.registers[0]];
	lightColor[1] = regs[pStage->color.registers[1]];
	lightColor[2] = regs[pStage->color.registers[2]];
	lightColor[3] = regs[pStage->color.registers[3]];
	blendLightProgram->BindVector("diffuseColor", lightColor);

	RB_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_BlendLight_external);
	RB_RenderDrawSurfChainWithFunction(drawSurfs2, RB_T_BlendLight_external);

	GL_DisableVertexAttribArray(SHADER_PARM_ADDR(attr_Vertex));

	if(oldDrawBits)
		GL_State(oldDrawBits);

	blendLightProgram->Unbind(/*pStage*/);

	blendLightProgram = NULL;

	GL_UseProgram(lastProgram);
}