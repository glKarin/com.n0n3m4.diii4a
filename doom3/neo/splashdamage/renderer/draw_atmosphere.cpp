#include "idlib/precompiled.h"

#include "renderer/tr_local.h"

#include "RenderProgram.h"
#include "RenderProgramManager.h"

idCVar harm_r_skipAtmosphere("harm_r_skipAtmosphere", "0", CVAR_BOOL | CVAR_RENDERER, "skip atmosphere rendering after draw interactions");
idCVar harm_r_atmosphereShader("harm_r_atmosphereShader", "0", CVAR_INTEGER | CVAR_RENDERER | CVAR_ARCHIVE, "atmosphere shader. 0 = auto, 1 = atmosphereLinear, 2 = atmosphere");

static const sdRenderProgram *atmosphereShader = NULL;

ID_INLINE static void RB_BindAtmosphereConstants(void)
{
    // sun
    // program->BindVector("sunDirectionWorld", backEnd.parms.sunDir);
    // program->BindVector("sunColor", backEnd.parms.sunColor);
    atmosphereShader->BindVector("sunHaloParameters", backEnd.parms.sunHaloParameters);
    // fog
    atmosphereShader->BindVector("fogColor", backEnd.parms.fogColor);
    atmosphereShader->BindVector("fogParams", backEnd.parms.fogParams);
    atmosphereShader->BindVector("fogDepths", backEnd.parms.fogDepths);
    atmosphereShader->BindVector("fogRotation_x", 1.0f, 0.0f, 0.0f);
    atmosphereShader->BindVector("fogRotation_y", 0.0f, 1.0f, 0.0f);
    atmosphereShader->BindVector("fogRotation_z", 0.0f, 0.0f, 1.0f);
	// view
    atmosphereShader->BindVector("viewOriginWorld", backEnd.viewDef->renderView.vieworg);
}

static void RB_DrawSurfAtmosphere(const drawSurf_t *surf)
{
	int			stage;
	const idMaterial	*shader;
	const shaderStage_t *pStage;
	const float	*regs;
	float		color[4];

    if (!surf || !surf->geo->ambientCache) {
        return;
    }
    if (surf->material->TestMaterialFlag(MF_NOATMOSPHERE)) {
        return;
    }

	shader = surf->material;
	if (!shader->IsDrawn()) {
		return;
	}

	regs = surf->shaderRegisters;

	for (stage = 0; stage < shader->GetNumStages() ; stage++) {
		pStage = shader->GetStage(stage);

		// check the enable condition
		if (regs[ pStage->conditionRegister ] == 0) {
			continue;
		}

		// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
		if ((pStage->drawStateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE)) {
			continue;
		}

		// set the color
		color[0] = regs[ pStage->color.registers[0] ];
		color[1] = regs[ pStage->color.registers[1] ];
		color[2] = regs[ pStage->color.registers[2] ];
		color[3] = regs[ pStage->color.registers[3] ];

		// skip the entire stage if an add would be black
		if ((pStage->drawStateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE)
		    && color[0] <= 0 && color[1] <= 0 && color[2] <= 0) {
			continue;
		}

		if (pStage->hasAlphaTest && color[3] <= 0) {
			continue;
		}

		break;
	}

	if(stage == shader->GetNumStages())
		return;

    // set the vertex pointers
    idDrawVert	*ac = (idDrawVert *)vertexCache.Position(surf->geo->ambientCache);

    GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());

	// sun
    idVec3 localSunDir;
    R_GlobalVectorToLocal(surf->space->modelMatrix, backEnd.parms.sunDir, localSunDir);
    atmosphereShader->BindVector("sunDirection", localSunDir);
    // view
    idVec4 localViewOrigin;
    R_GlobalPointToLocal(surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, localViewOrigin.ToVec3());
    localViewOrigin[3] = 1.0f;
    atmosphereShader->BindVector("viewOrigin", localViewOrigin);

    idMat4 modelMatrix;
    memcpy(&modelMatrix, surf->space->modelMatrix, sizeof(modelMatrix));
    modelMatrix.TransposeSelf();
    atmosphereShader->BindVector("transposedModelMatrix_x", modelMatrix[0]);
    atmosphereShader->BindVector("transposedModelMatrix_y", modelMatrix[1]);
    atmosphereShader->BindVector("transposedModelMatrix_z", modelMatrix[2]);

    idImage *skyGradientCubeMap = backEnd.parms.skyGradientCubeMap;
    if(!skyGradientCubeMap)
        skyGradientCubeMap = globalImages->blackCubeMapImage;

    atmosphereShader->BindImage("skyGradientCubeMap", skyGradientCubeMap);

    // change the matrix if needed
    if( surf->space != backEnd.currentSpace )
    {
		RB_LoadProjectionMatrix();
		backEnd.currentSpace = surf->space;
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
	RB_SetupDrawSurfMVP(surf);

	if (shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset());
	}

	// draw it
	RB_DrawElementsWithCounters(surf->geo);

	if (shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}

	if ( surf->space->weaponDepthHack || surf->space->modelDepthHack != 0.0f ) {
		RB_LeaveDepthHack();
	}
}

void RB_DrawAtmosphere( drawSurf_t **drawSurfs, int numDrawSurfs )
{
    if(harm_r_skipAtmosphere.GetBool())
        return;

    if( numDrawSurfs == 0 )
    {
        return;
    }

    if( !drawSurfs )
    {
        return;
    }

    // if we are just doing 2D rendering, no need to fill the depth buffer
    if( backEnd.viewDef->viewEntitys == NULL )
    {
        return;
    }

	bool usingLinear = harm_r_atmosphereShader.GetInteger() == 1 || (harm_r_atmosphereShader.GetInteger() == 0 && r_megaDrawMethod.GetInteger() != 0);
	if(usingLinear)
	{
		atmosphereShader = renderProgramManager->LoadProgram("atmosphereLinear");
	}
	else
	{
		atmosphereShader = renderProgramManager->LoadProgram("atmosphere");
	}
    if( !atmosphereShader || !atmosphereShader->Bind() )
    {
        return;
    }

    RB_LogComment("---------- RB_DrawAtmosphere ----------\n");

    // bind the vertex and fragment shader

    // enable the vertex arrays
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));

    // force MVP change on first surface
    backEnd.currentSpace = NULL;
    int glStateBits = backEnd.glState.glStateBits;

	int shaderStateBits;
	if(usingLinear)
	{
		atmosphereShader = renderProgramManager->LoadProgram("atmosphereLinear");
		shaderStateBits = GLS_DEPTHFUNC_EQUAL;
	}
	else
	{
		atmosphereShader = renderProgramManager->LoadProgram("atmosphere");
		shaderStateBits = GLS_COLORMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	GL_State(shaderStateBits);
	atmosphereShader->SetupState();

	RB_BindAtmosphereConstants();

    for( int i = 0; i < numDrawSurfs; i++ )
    {
        RB_DrawSurfAtmosphere(drawSurfs[i]);
    }

	if(glStateBits != backEnd.glState.glStateBits)
		GL_State(glStateBits);

    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));

    atmosphereShader->Unbind();

    backEnd.currentSpace = NULL;
	
    //k GL_SelectTexture( 0 );
}
