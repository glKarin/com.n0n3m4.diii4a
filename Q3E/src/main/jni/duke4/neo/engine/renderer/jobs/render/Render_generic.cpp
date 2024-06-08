// Render_generic.cpp
//

#include "../../RenderSystem_local.h"


/*
==================
RB_STD_T_RenderShaderPasses

This is also called for the generated 2D rendering
==================
*/
void RB_STD_T_RenderShaderPasses(const drawSurf_t* surf) {
	int			stage;
	const idMaterial* shader;
	const shaderStage_t* pStage;
	const float* regs;
	float		color[4];
	const srfTriangles_t* tri;

	tri = surf->geo;
	shader = surf->material;

	if (!shader->HasAmbient()) {
		return;
	}

	if (shader->IsPortalSky()) {
		return;
	}

	RB_SetMVP(surf->space->mvp);

	// change the matrix if needed
	if (surf->space != backEnd.currentSpace) {
#if !defined(__ANDROID__) //karin: GLES not support matrix functions
		glLoadMatrixf(surf->space->modelViewMatrix);
#else
		RB_SetModelMatrix(surf->space->modelViewMatrix);
		RB_SetupMVP_DrawSurf(surf);
#endif
		backEnd.currentSpace = surf->space;
	}

	// change the scissor if needed
	if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(surf->scissorRect)) {
		backEnd.currentScissor = surf->scissorRect;
		glScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
			backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
			backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
			backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
	}

	// some deforms may disable themselves by setting numIndexes = 0
	if (!tri->numIndexes) {
		return;
	}

	if (!tri->ambientCache) {
		common->Printf("RB_T_RenderShaderPasses: !tri->ambientCache\n");
		return;
	}

	// get the expressions for conditionals / color / texcoords
	regs = surf->shaderRegisters;

	// set face culling appropriately
	GL_Cull(shader->GetCullType());

	// set polygon offset if necessary
	if (shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset());
	}

	if (surf->space->weaponDepthHack) {
		RB_EnterWeaponDepthHack();
	}

	if (surf->space->modelDepthHack != 0.0f) {
		RB_EnterModelDepthHack(surf->space->modelDepthHack);
	}

	idDrawVert* ac = (idDrawVert*)vertexCache.Position(tri->ambientCache);
#ifdef __ANDROID__ //karin: GLES is programming pipeline
	glVertexAttribPointer(PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 2, GL_FLOAT, false, sizeof(idDrawVert), reinterpret_cast<void*>(&ac->st));
#else
	glVertexPointer(3, GL_FLOAT, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	glTexCoordPointer(2, GL_FLOAT, sizeof(idDrawVert), reinterpret_cast<void*>(&ac->st));
#endif

	for (stage = 0; stage < shader->GetNumStages(); stage++) {
		pStage = shader->GetStage(stage);

		// check the enable condition
		if (regs[pStage->conditionRegister] == 0) {
			continue;
		}

		// skip the stages involved in lighting
		if (pStage->lighting != SL_AMBIENT) {
			continue;
		}

		// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
		if ((pStage->drawStateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE)) {
			continue;
		}

		// see if we are a new-style stage
		newShaderStage_t* newStage = pStage->newStage;	

		idVec4 genericinfo;
		genericinfo.Zero();

		if (!newStage) {
			//--------------------------
			//
			// old style stages
			//
			//--------------------------

			// set the color
			color[0] = regs[pStage->color.registers[0]];
			color[1] = regs[pStage->color.registers[1]];
			color[2] = regs[pStage->color.registers[2]];
			color[3] = regs[pStage->color.registers[3]];

			// skip the entire stage if an add would be black
			if ((pStage->drawStateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE)
				&& color[0] <= 0 && color[1] <= 0 && color[2] <= 0) {
				continue;
			}

			// skip the entire stage if a blend would be completely transparent
			if ((pStage->drawStateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)
				&& color[3] <= 0) {
				continue;
			}

			// select the vertex color source
			if (pStage->vertexColor == SVC_IGNORE) {				
				genericinfo.x = 1;

				if (surf->forceColor != vec4_one)
				{
					tr.vertexColorParm->SetVectorValue(surf->forceColor);
				}
				else
				{
					tr.vertexColorParm->SetVectorValue(color);
				}				
			}
			else {
				tr.vertexColorParm->SetVectorValue(color);

#ifdef __ANDROID__ //karin: GLES is programming pipeline
				glVertexAttribPointer(PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), (void*)&ac->color);
				glEnableVertexAttribArray(PC_ATTRIB_INDEX_COLOR);	
#else
				glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(idDrawVert), (void*)&ac->color);
				glEnableClientState(GL_COLOR_ARRAY);			
#endif	
			}
		}

		if (!newStage) {
			// bind the texture
			RB_BindVariableStageImage(&pStage->texture, regs);			
		}

		// set the state
		GL_State(pStage->drawStateBits);

		//RB_PrepareStageTexturing(pStage, surf, ac);

		glEnableVertexAttribArrayARB(PC_ATTRIB_INDEX_COLOR);
		glVertexAttribPointerARB(PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(idDrawVert), (void*)&ac->color);

		tr.genericShaderParam->SetVectorValue(genericinfo);

		// draw it
		if (newStage) {
			for (int i = 0; i < newStage->numStageParms; i++) {
				switch (newStage->stageParms[i].param->GetType())
				{
					case RENDERPARM_TYPE_IMAGE:
						newStage->stageParms[i].param->SetImage(newStage->stageParms[i].imageValue);
						break;
				}
			}

			if (pStage->texture.cinematic)
			{
				RB_BindVariableStageImage(&pStage->texture, nullptr);
			}

			newStage->renderProgram->Bind();

			if (newStage->renderTexture)
			{
				newStage->renderTexture->MakeCurrent();
				glClear(GL_DEPTH_BUFFER_BIT);
			}
		}
		else {
			tr.guiTextureProgram->Bind();
		}

		

		RB_DrawElementsWithCounters(tri);

		if (newStage) {
			if (newStage->renderTexture)
			{
				newStage->renderTexture->BindNull();
			}

			newStage->renderProgram->BindNull();
		}
		else {
			tr.guiTextureProgram->BindNull();
		}

		glDisableVertexAttribArrayARB(PC_ATTRIB_INDEX_COLOR);

		//RB_FinishStageTexturing(pStage, surf, ac);

		if (pStage->vertexColor != SVC_IGNORE) {
#ifdef __ANDROID__ //karin: GLES is programming pipeline
			glDisableVertexAttribArray(PC_ATTRIB_INDEX_COLOR);
#else
			glDisableClientState(GL_COLOR_ARRAY);
#endif
		}
	}

	// reset polygon offset
	if (shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
	if (surf->space->weaponDepthHack || surf->space->modelDepthHack != 0.0f) {
		RB_LeaveDepthHack();
	}
}

/*
=====================
idRender::DrawShaderPasses

Draw non-light dependent passes
=====================
*/
int idRender::DrawShaderPasses(drawSurf_t** drawSurfs, int numDrawSurfs) {
	int				i;

	// only obey skipAmbient if we are rendering a view
	if (backEnd.viewDef->viewEntitys && r_skipAmbient.GetBool()) {
		return numDrawSurfs;
	}

	RB_LogComment("---------- RB_STD_DrawShaderPasses ----------\n");

	// if we are about to draw the first surface that needs
	// the rendering in a texture, copy it over
	if (drawSurfs[0]->material->GetSort() >= SS_POST_PROCESS) {
		if (r_skipPostProcess.GetBool()) {
			return 0;
		}

		// only dump if in a 3d view
		if (backEnd.viewDef->viewEntitys && tr.backEndRenderer == BE_ARB2) {
			globalImages->currentRenderImage->CopyFramebuffer(backEnd.viewDef->viewport.x1,
				backEnd.viewDef->viewport.y1, backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1,
				backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1);
		}
		backEnd.currentRenderCopied = true;
	}

	GL_SelectTexture(1);
	globalImages->BindNull();

	GL_SelectTexture(0);
#ifdef __ANDROID__ //karin: GLES is programming pipeline
	glEnableVertexAttribArray(PC_ATTRIB_INDEX_COLOR);
#else
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

//	RB_SetProgramEnvironment();

	// we don't use RB_RenderDrawSurfListWithFunction()
	// because we want to defer the matrix load because many
	// surfaces won't draw any ambient passes
	backEnd.currentSpace = NULL;
	for (i = 0; i < numDrawSurfs; i++) {
		if (drawSurfs[i]->material->SuppressInSubview()) {
			continue;
		}

		if (backEnd.viewDef->isXraySubview && drawSurfs[i]->space->entityDef) {
			if (drawSurfs[i]->space->entityDef->parms.xrayIndex != 2) {
				continue;
			}
		}

		// we need to draw the post process shaders after we have drawn the fog lights
		if (drawSurfs[i]->material->GetSort() >= SS_POST_PROCESS
			&& !backEnd.currentRenderCopied) {
			break;
		}

		RB_STD_T_RenderShaderPasses(drawSurfs[i]);
	}

	GL_Cull(CT_FRONT_SIDED);
	tr.vertexColorParm->SetVectorValue(idVec4(1, 1, 1, 1));

	return i;
}