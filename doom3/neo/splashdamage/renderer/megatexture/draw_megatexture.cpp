#include "idlib/precompiled.h"

#include "renderer/tr_local.h"

#include "renderer/RenderProgram.h"

idCVar harm_r_megatextureAmbient("harm_r_megatextureAmbient", "0", CVAR_RENDERER | CVAR_BOOL, "don't render meta texture interaction");
extern idCVar harm_r_skipAreaAmbient;

static const sdRenderProgram *megaTextureProgram = NULL;
#ifdef _STENCIL_SHADOW_IMPROVE
extern bool stencilShadowWithoutStencilTest;
#endif

ID_INLINE static void R_SetMetaTextureDrawInteraction(const shaderStage_t *surfaceStage, const float *surfaceRegs, float color[4])
{
	if (color) {
		for (int i = 0 ; i < 4 ; i++) {
			color[i] = surfaceRegs[surfaceStage->color.registers[i]];

			// clamp here, so card with greater range don't look different.
			// we could perform overbrighting like we do for lights, but
			// it doesn't currently look worth it.
			if (color[i] < 0) {
				color[i] = 0;
			} else if (color[i] > 1.0) {
				color[i] = 1.0;
			}
		}
	}
}

// must bind render program first
ID_INLINE static void RB_MegaTexture_Update(const drawSurf_t *surf, idMegaTexture *megaTexture) {
	megaTexture->UpdateMapping( backEnd.viewDef->renderWorld );
	megaTexture->SetMappingForSurface( surf->geo );
	idVec3	localViewer;
	R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, localViewer );
	megaTexture->BindForViewOrigin( localViewer, megaTextureProgram );
}

ID_INLINE static void RB_SubmitMetaTextureInteraction(drawInteraction_t *din, void (*DrawInteraction)(const drawInteraction_t *))
{
	RB_SetupDrawSurfMVP(din->surf);

	megaTextureProgram->SetupState();

	// enable the vertex arrays
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));

	// set the vertex pointers
	idDrawVert	*ac = (idDrawVert *)vertexCache.Position(din->surf->geo->ambientCache);

	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Normal), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());

	DrawInteraction(din);

	// disable the vertex arrays
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
}

ID_INLINE static int RB_MegaTexture_FilterInteractionStages(const drawSurf_t *surf, int drawStages[MAX_SHADER_STAGES]) {
	const idMaterial	*surfaceShader = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	const shaderStage_t	*surfaceStage;
	int numDrawStages = 0;

	// if all stages of a material have been conditioned off, don't do anything
	int stage = 0;
	for( ; stage < surfaceShader->GetNumStages(); stage++ )
	{
		surfaceStage = surfaceShader->GetStage( stage );

		if (surfaceStage->lighting != SL_AMBIENT)
			continue;

		// no megatexture
		if (!surfaceStage->newStage || !surfaceStage->newStage->megaTexture) {
			continue;
		}

		// check the stage enable condition
		if( !surfaceRegs[ surfaceStage->conditionRegister ] )
		{
			continue;
		}

		if(!surfaceStage->renderProgram)
			continue;

		if (!surfaceStage->renderProgram->IsValid())
			continue;

		drawStages[numDrawStages++] = stage;
	}

	return numDrawStages;
}

/*
=============
RB_GLSL_CreateDrawMegaTextureInteractions
=============
*/
void RB_CreateSingleMegaTextureDrawInteractions(const drawSurf_t *surf, void (*DrawInteraction)(const drawInteraction_t *))
{
	const idMaterial	*surfaceShader = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	const viewLight_t	*vLight = backEnd.vLight;
	const idMaterial	*lightShader = vLight->lightShader;
	const float			*lightRegs = vLight->shaderRegisters;
	drawInteraction_t	inter;

	if (!surf->geo || !surf->geo->ambientCache) {
		return;
	}

	int drawStages[MAX_SHADER_STAGES];

	int numDrawStages = RB_MegaTexture_FilterInteractionStages(surf, drawStages);
	if( numDrawStages == 0 )
	{
		return;
	}

	if (tr.logFile) {
		RB_LogComment("---------- RB_CreateSingleMegaTextureDrawInteractions %s on %s ----------\n", lightShader->GetName(), surfaceShader->GetName());
	}

	// change the matrix and light projection vectors if needed
	if (surf->space != backEnd.currentSpace) {

		RB_LoadProjectionMatrix();
	}

	// change the scissor if needed
	if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(surf->scissorRect)) {
		backEnd.currentScissor = surf->scissorRect;
		qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
	}

	// hack depth range if needed
	if (surf->space->weaponDepthHack) {
		RB_EnterWeaponDepthHack(/*surf*/);
	}

	if (surf->space->modelDepthHack) {
		RB_EnterModelDepthHack(surf);
	}

	inter.surf = surf;
	inter.lightFalloffImage = vLight->falloffImage;

	R_GlobalPointToLocal(surf->space->modelMatrix, vLight->globalLightOrigin, inter.localLightOrigin.ToVec3());
	R_GlobalPointToLocal(surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, inter.localViewOrigin.ToVec3());
	inter.localLightOrigin[3] = 0;
	inter.localViewOrigin[3] = 1;
	inter.ambientLight = lightShader->IsAmbientLight();

	// the base projections may be modified by texture matrix on light stages
	idPlane lightProject[4];

	for (int i = 0 ; i < 4 ; i++) {
		R_GlobalPlaneToLocal(surf->space->modelMatrix, backEnd.vLight->lightProject[i], lightProject[i]);
	}

	for (int lightStageNum = 0 ; lightStageNum < lightShader->GetNumStages() ; lightStageNum++) {
		const shaderStage_t	*lightStage = lightShader->GetStage(lightStageNum);

		// ignore stages that fail the condition
		if (!lightRegs[ lightStage->conditionRegister ]) {
			continue;
		}

		inter.lightImage = lightStage->texture.image;

		memcpy(inter.lightProjection, lightProject, sizeof(inter.lightProjection));

		// now multiply the texgen by the light texture matrix
		if (lightStage->texture.hasMatrix) {
			RB_GetShaderTextureMatrix(lightRegs, &lightStage->texture, backEnd.lightTextureMatrix);
			RB_BakeTextureMatrixIntoTexgen(reinterpret_cast<class idPlane *>(inter.lightProjection), backEnd.lightTextureMatrix); //k2023
		}

		inter.diffuseImage = NULL;
		inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;

		float lightColor[4];

		// backEnd.lightScale is calculated so that lightColor[] will never exceed
		// tr.backEndRendererMaxLight
		lightColor[0] = backEnd.lightScale * lightRegs[ lightStage->color.registers[0] ];
		lightColor[1] = backEnd.lightScale * lightRegs[ lightStage->color.registers[1] ];
		lightColor[2] = backEnd.lightScale * lightRegs[ lightStage->color.registers[2] ];
		lightColor[3] = lightRegs[ lightStage->color.registers[3] ];

		// go through the individual stages
		for (int surfaceStageNum = 0 ; surfaceStageNum < numDrawStages ; surfaceStageNum++) {
			const shaderStage_t	*surfaceStage = surfaceShader->GetStage(drawStages[surfaceStageNum]);

			R_SetMetaTextureDrawInteraction(surfaceStage, surfaceRegs, inter.diffuseColor.ToFloatPtr());
			if (inter.diffuseColor[0] > 0 || inter.diffuseColor[1] > 0 || inter.diffuseColor[2] > 0) {
				inter.diffuseColor[0] *= lightColor[0];
				inter.diffuseColor[1] *= lightColor[1];
				inter.diffuseColor[2] *= lightColor[2];
				inter.diffuseColor[3] *= lightColor[3];

#ifdef _NO_GAMMA //karin: r_brightness when unsupport gamma
				if(RB_overbright > 1.0f)
				{
					inter.diffuseColor[0] *= RB_overbright;
					inter.diffuseColor[1] *= RB_overbright;
					inter.diffuseColor[2] *= RB_overbright;
				}
#endif

				if(!surfaceStage->renderProgram->Bind(surfaceStage, surfaceShader, surfaceRegs))
					continue;

				megaTextureProgram = surfaceStage->renderProgram;

				RB_MegaTexture_Update(surf, surfaceStage->newStage->megaTexture);

				int oldDrawBits = backEnd.glState.glStateBits;

				RB_SubmitMetaTextureInteraction(&inter, DrawInteraction);

				megaTextureProgram->Unbind(true);

				//if(oldDrawBits)
				GL_State(oldDrawBits);

				megaTextureProgram = NULL;
			}
		}
	}

	// unhack depth range if needed
	if (surf->space->weaponDepthHack || surf->space->modelDepthHack != 0.0f) {
		RB_LeaveDepthHack(/*surf*/);
	}

	backEnd.currentSpace = surf->space; //k2023
}

// interaction
static void RB_MegaTexture_DrawInteraction(const drawInteraction_t *din)
{
	// vertex shader
	megaTextureProgram->BindVector("lightProject_s", din->lightProjection[0].ToFloatPtr());
	megaTextureProgram->BindVector("lightProject_t", din->lightProjection[1].ToFloatPtr());
	megaTextureProgram->BindVector("lightProject_q", din->lightProjection[2].ToFloatPtr());
	megaTextureProgram->BindVector("lightFalloff_s", din->lightProjection[3].ToFloatPtr());
	megaTextureProgram->BindVector("lightOrigin", din->localLightOrigin); // in model coord

	// fragment shader
	megaTextureProgram->BindImage("lightFalloffMap", din->lightFalloffImage);
	megaTextureProgram->BindImage("lightProjectionMap", din->lightImage);
	megaTextureProgram->BindVector("diffuseColor", din->diffuseColor);

	// draw it
	RB_DrawElementsWithCounters(din->surf->geo);
}

/*
=============
RB_GLSL_CreateDrawMegaTextureInteractions

=============
*/
void RB_GLSL_CreateDrawMegaTextureInteractions(const drawSurf_t *surf)
{
	return;
	if (!surf) {
		return;
	}
	if (harm_r_megatextureAmbient.GetBool())
		return;
	if (r_skipInteractions.GetBool())
		return;

	// perform setup here that will be constant for all interactions
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE |
			GLS_DEPTHMASK | //k: fix translucent interactions
			backEnd.depthFunc);

	backEnd.currentSpace = NULL; //k2023

	for (; surf ; surf=surf->nextOnLight) {
		// perform setup here that will not change over multiple interaction passes

		// this may cause RB_GLSL_DrawInteraction to be exacuted multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleMegaTextureDrawInteractions(surf, RB_MegaTexture_DrawInteraction);
	}

	backEnd.currentSpace = NULL; //k2023
}

/*
=============
RB_DrawMegaTextureInteraction

=============
*/
bool RB_DrawMegaTextureInteraction(const drawInteraction_t *din, const shaderStage_t *lightStage, const shaderStage_t *surfaceStage)
{
	if (harm_r_megatextureAmbient.GetBool())
		return false;

#ifdef _STENCIL_SHADOW_IMPROVE //skip if fill transucent color first
	if(stencilShadowWithoutStencilTest)
		return false;
#endif

	if (!surfaceStage->newStage || !surfaceStage->newStage->megaTexture) {
		return false;
	}

	if(!surfaceStage->renderProgram || !surfaceStage->renderProgram->IsValid())
		return false;

	const drawSurf_t *surf = din->surf;
	const idMaterial	*surfaceShader = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	const viewLight_t	*vLight = backEnd.vLight;
	const float			*lightRegs = vLight->shaderRegisters;

	// ignore stages that fail the condition
	if (!surfaceRegs[ surfaceStage->conditionRegister ]) {
		return false;
	}

	drawInteraction_t inter = *din;
	R_SetMetaTextureDrawInteraction(surfaceStage, surfaceRegs, inter.diffuseColor.ToFloatPtr());
	float lightColor[4];

	// backEnd.lightScale is calculated so that lightColor[] will never exceed
	// tr.backEndRendererMaxLight
	lightColor[0] = backEnd.lightScale * lightRegs[ lightStage->color.registers[0] ];
	lightColor[1] = backEnd.lightScale * lightRegs[ lightStage->color.registers[1] ];
	lightColor[2] = backEnd.lightScale * lightRegs[ lightStage->color.registers[2] ];
	lightColor[3] = lightRegs[ lightStage->color.registers[3] ];
	if (inter.diffuseColor[0] > 0 || inter.diffuseColor[1] > 0 || inter.diffuseColor[2] > 0) {
		inter.diffuseColor[0] *= lightColor[0];
		inter.diffuseColor[1] *= lightColor[1];
		inter.diffuseColor[2] *= lightColor[2];
		inter.diffuseColor[3] *= lightColor[3];

#ifdef _NO_GAMMA //karin: r_brightness when unsupport gamma
		if(RB_overbright > 1.0f)
		{
			inter.diffuseColor[0] *= RB_overbright;
			inter.diffuseColor[1] *= RB_overbright;
			inter.diffuseColor[2] *= RB_overbright;
		}
#endif
	}
	else
		return false;

	shaderProgram_t *lastProgram = backEnd.glState.currentProgram;
	if(!surfaceStage->renderProgram->Bind(surfaceStage, surfaceShader, surfaceRegs))
		return false;

	megaTextureProgram = surfaceStage->renderProgram;

	int oldDrawBits = backEnd.glState.glStateBits;

	RB_MegaTexture_Update(surf, surfaceStage->newStage->megaTexture);

	// perform setup here that will be constant for all interactions
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE |
			GLS_DEPTHMASK | //k: fix translucent interactions
			backEnd.depthFunc);

	RB_SetupDrawSurfMVP(surf);

	megaTextureProgram->SetupState();

	RB_MegaTexture_DrawInteraction(&inter);

	megaTextureProgram->Unbind(true);

	//if(oldDrawBits)
	GL_State(oldDrawBits);

	megaTextureProgram = NULL;

	GL_UseProgram(lastProgram);

	return true;
}



// ambient
static void RB_MegaTexture_DrawAmbient(const drawInteraction_t *din)
{
	megaTextureProgram->BindVector("diffuseColor", din->diffuseColor);

	// ambient shader
	megaTextureProgram->BindVector("ambientScale", backEnd.parms.ambientScale);
	megaTextureProgram->BindVector("sunDirectionWorld", backEnd.parms.sunDir);
	// texture is the cube map
	idImage *ambientCubeMap = NULL;
	ambientCubeMap = din->surf->space->areaAmbient->GetAmbientCubeMap();
	if(!ambientCubeMap)
		ambientCubeMap = globalImages->blackCubeMapImage;
	megaTextureProgram->BindImage("ambientCubeMap", ambientCubeMap);

	// draw it
	RB_DrawElementsWithCounters(din->surf->geo);
}

ID_INLINE static int RB_MegaTexture_FilterAmbientStages(const drawSurf_t *surf, int drawStages[MAX_SHADER_STAGES]) {
	const idMaterial	*surfaceShader = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	const shaderStage_t	*surfaceStage;
	int numDrawStages = 0;

	// if all stages of a material have been conditioned off, don't do anything
	int stage = 0;
	for( ; stage < surfaceShader->GetNumStages(); stage++ )
	{
		surfaceStage = surfaceShader->GetStage( stage );

		if (surfaceStage->lighting != SL_AMBIENT)
			continue;

		// no megatexture
		if (!surfaceStage->newStage || !surfaceStage->newStage->megaTexture) {
			continue;
		}

		// check the stage enable condition
		if( !surfaceRegs[ surfaceStage->conditionRegister ] )
		{
			continue;
		}

		if(!surfaceStage->renderProgram)
			continue;

		if (!surfaceStage->renderProgram->IsValid())
			continue;

		drawStages[numDrawStages++] = stage;
	}

	return numDrawStages;
}

/*
=============
RB_CreateSingleMegaTextureDrawAmbients
=============
*/
void RB_CreateSingleMegaTextureDrawAmbients(const drawSurf_t *surf, void (*DrawInteraction)(const drawInteraction_t *))
{
	drawInteraction_t	inter;

	if (!surf->geo || !surf->geo->ambientCache) {
		return;
	}

	const idMaterial	*surfaceShader = surf->material;

	// translucent surfaces don't put anything in the depth buffer and don't
	// test against it, which makes them fail the mirror clip plane operation
	if( surfaceShader->Coverage() == MC_TRANSLUCENT )
	{
		return;
	}

	const float			*surfaceRegs = surf->shaderRegisters;
	const shaderStage_t	*surfaceStage;
	int drawStages[MAX_SHADER_STAGES];

	int numDrawStages = RB_MegaTexture_FilterAmbientStages(surf, drawStages);
	if( numDrawStages == 0 )
	{
		return;
	}

	if (tr.logFile) {
		RB_LogComment("---------- RB_CreateSingleMegaTextureDrawAmbients on %s ----------\n", surfaceShader->GetName());
	}

	// change the matrix and light projection vectors if needed
	if (surf->space != backEnd.currentSpace) {

		RB_LoadProjectionMatrix();
	}

	// change the scissor if needed
	if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(surf->scissorRect)) {
		backEnd.currentScissor = surf->scissorRect;
		qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
	}

	// hack depth range if needed
	if (surf->space->weaponDepthHack) {
		RB_EnterWeaponDepthHack(/*surf*/);
	}

	if (surf->space->modelDepthHack) {
		RB_EnterModelDepthHack(surf);
	}

	inter.surf = surf;
	inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;

	// go through the individual stages
	for (int surfaceStageNum = 0 ; surfaceStageNum < numDrawStages ; surfaceStageNum++) {
		surfaceStage = surfaceShader->GetStage(drawStages[surfaceStageNum]);

		R_SetMetaTextureDrawInteraction(surfaceStage, surfaceRegs, inter.diffuseColor.ToFloatPtr());
		if (inter.diffuseColor[0] > 0 || inter.diffuseColor[1] > 0 || inter.diffuseColor[2] > 0) {
#ifdef _NO_GAMMA //karin: r_brightness when unsupport gamma
			if(RB_overbright > 1.0f)
			{
				inter.diffuseColor[0] *= RB_overbright;
				inter.diffuseColor[1] *= RB_overbright;
				inter.diffuseColor[2] *= RB_overbright;
			}
#endif

			// find ambient version
			const sdRenderProgram *ambientProgram = surfaceStage->renderProgram->GetDeclRenderProgram()->AmbientVersion();
			if(!ambientProgram)
				continue;
			if (!ambientProgram->IsValid())
				continue;
			if(!ambientProgram->Bind(surfaceStage, surfaceShader, surfaceRegs))
				continue;

			megaTextureProgram = ambientProgram;

			int oldDrawBits = backEnd.glState.glStateBits;

			RB_MegaTexture_Update(surf, surfaceStage->newStage->megaTexture);

			RB_SubmitMetaTextureInteraction(&inter, DrawInteraction);

			megaTextureProgram->Unbind(true);

			//if(oldDrawBits)
			GL_State(oldDrawBits);

			megaTextureProgram = NULL;
		}
	}

	// unhack depth range if needed
	if (surf->space->weaponDepthHack || surf->space->modelDepthHack != 0.0f) {
		RB_LeaveDepthHack(/*surf*/);
	}

	backEnd.currentSpace = surf->space; //k2023
}

/*
=============
RB_GLSL_CreateDrawMegaTextureAmbients

=============
*/
void RB_GLSL_CreateDrawMegaTextureAmbients(const drawSurf_t *surf)
{
	if (!surf) {
		return;
	}
	if (harm_r_skipAreaAmbient.GetBool())
		return;
	if (harm_r_megatextureAmbient.GetBool())
		return;
	if(!surf->space->areaAmbient)
		return;
	if (surf->material->TestMaterialFlag(MF_NOAMBIENT)) {
		return;
	}

	// perform setup here that will be constant for all interactions
    GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

	backEnd.currentSpace = NULL; //k2023

	RB_CreateSingleMegaTextureDrawAmbients(surf, RB_MegaTexture_DrawAmbient);

	backEnd.currentSpace = NULL; //k2023
}

/*
=============
RB_DrawMegaTextureAmbient

=============
*/
bool RB_DrawMegaTextureAmbient(const drawSurf_t *surf, const shaderStage_t *surfaceStage)
{
	if (harm_r_megatextureAmbient.GetBool())
		return false;

	if (!surfaceStage->newStage || !surfaceStage->newStage->megaTexture) {
		return false;
	}

	if(!surfaceStage->renderProgram || !surfaceStage->renderProgram->IsValid())
		return false;

	const idMaterial	*surfaceShader = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;

	// ignore stages that fail the condition
	if (!surfaceRegs[ surfaceStage->conditionRegister ]) {
		return false;
	}

	drawInteraction_t inter;
	inter.surf = surf;
	R_SetMetaTextureDrawInteraction(surfaceStage, surfaceRegs, inter.diffuseColor.ToFloatPtr());
	if (inter.diffuseColor[0] > 0 || inter.diffuseColor[1] > 0 || inter.diffuseColor[2] > 0) {
#ifdef _NO_GAMMA //karin: r_brightness when unsupport gamma
		if(RB_overbright > 1.0f)
		{
			inter.diffuseColor[0] *= RB_overbright;
			inter.diffuseColor[1] *= RB_overbright;
			inter.diffuseColor[2] *= RB_overbright;
		}
#endif
	}
	else
		return false;

	const sdRenderProgram *ambientProgram = surfaceStage->renderProgram->GetDeclRenderProgram()->AmbientVersion();
	if(!ambientProgram || !ambientProgram->IsValid())
		return false;

	shaderProgram_t *lastProgram = backEnd.glState.currentProgram;
	if(!ambientProgram->Bind(surfaceStage, surfaceShader, surfaceRegs))
		return false;

	megaTextureProgram = ambientProgram;

	int oldDrawBits = backEnd.glState.glStateBits;

	RB_MegaTexture_Update(surf, surfaceStage->newStage->megaTexture);

	// perform setup here that will be constant for all interactions
    GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

	RB_SetupDrawSurfMVP(surf);

	megaTextureProgram->SetupState();

	RB_MegaTexture_DrawAmbient(&inter);

	megaTextureProgram->Unbind(true);

	//if(oldDrawBits)
	GL_State(oldDrawBits);

	megaTextureProgram = NULL;

	GL_UseProgram(lastProgram);

	return true;
}
