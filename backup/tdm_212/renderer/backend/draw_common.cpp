/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"
#pragma hdrstop

#include "renderer/tr_local.h"
#include "renderer/backend/glsl.h"
#include "renderer/backend/FrameBuffer.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/GLSLUniforms.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/stages/AmbientOcclusionStage.h"
#include "renderer/backend/FrameBufferManager.h"
#include "renderer/backend/stages/VolumetricStage.h"

struct CubemapUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( CubemapUniforms );

	DEFINE_UNIFORM( int, skybox );
	DEFINE_UNIFORM( vec3, viewOrigin );
	DEFINE_UNIFORM( mat4, modelMatrix );
};

struct FogUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( FogUniforms );

	DEFINE_UNIFORM( vec4, tex0PlaneS );
	DEFINE_UNIFORM( vec4, tex1PlaneT );
	DEFINE_UNIFORM( vec3, fogColor );
	DEFINE_UNIFORM( float, fogEnter );
	DEFINE_UNIFORM( float, fogAlpha );
};

struct BlendUniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( BlendUniforms );

	DEFINE_UNIFORM( vec4, tex0PlaneS );
	DEFINE_UNIFORM( vec4, tex0PlaneT );
	DEFINE_UNIFORM( vec4, tex0PlaneQ );
	DEFINE_UNIFORM( vec4, tex1PlaneS );
	DEFINE_UNIFORM( vec4, blendColor );
};

/*
================
RB_PrepareStageTexturing_ReflectCube
Extracted from RB_PrepareStageTexturing
================
*/
ID_NOINLINE void RB_PrepareStageTexturing_ReflectCube( const shaderStage_t *pStage, const drawSurf_t *surf ) {
	// see if there is also a bump map specified
	const shaderStage_t *bumpStage = surf->material->GetBumpStage();
	if ( bumpStage ) {
		// per-pixel reflection mapping with bump mapping
		GL_SelectTexture( 1 );
		bumpStage->texture.image->Bind();
		GL_SelectTexture( 0 );

		programManager->bumpyEnvironment->Activate();
		programManager->bumpyEnvironment->GetUniformGroup<Uniforms::Global>()->Set( surf->space );
	} else {
		programManager->environment->Activate();
		programManager->environment->GetUniformGroup<Uniforms::Global>()->Set( surf->space );
	}
}

/*
================
RB_PrepareStageTexturing
================
*/
void RB_PrepareStageTexturing( const shaderStage_t *pStage, const drawSurf_t *surf ) {
	// set privatePolygonOffset if necessary
	if ( pStage->privatePolygonOffset ) {
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
	}

	// set the texture matrix if needed
	RB_LoadShaderTextureMatrix( surf->shaderRegisters, pStage );

	// texgens
	switch ( pStage->texture.texgen ) {
	case TG_SCREEN:
		programManager->oldStageShader->GetUniformGroup<OldStageUniforms>()->screenTex.Set( 1 );
		break;
	case TG_REFLECT_CUBE:
		RB_PrepareStageTexturing_ReflectCube( pStage, surf );
		break;
	}
}

/*
================
RB_FinishStageTexturing
================
*/
void RB_FinishStageTexturing( const shaderStage_t *pStage, const drawSurf_t *surf ) {
	// unset privatePolygonOffset if necessary
	if ( pStage->privatePolygonOffset && !surf->material->TestMaterialFlag( MF_POLYGONOFFSET ) ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}

	switch ( pStage->texture.texgen ) {
	case TG_SCREEN:
		programManager->oldStageShader->GetUniformGroup<OldStageUniforms>()->screenTex.Set( 0 );
		break;
	case TG_REFLECT_CUBE:
		const shaderStage_t *bumpStage = surf->material->GetBumpStage();
		if ( bumpStage ) {
			GL_SelectTexture( 0 );
		}
		GLSLProgram::Deactivate();

		break;
	}

	RB_LoadShaderTextureMatrix( NULL, pStage );
}

/*
=============================================================================================

SHADER PASSES

=============================================================================================
*/

/*
==================
RB_STD_T_RenderShaderPasses_OldStage

Extracted from the giantic loop in RB_STD_T_RenderShaderPasses
==================
*/
void RB_STD_T_RenderShaderPasses_OldStage( const shaderStage_t *pStage, const drawSurf_t *surf ) {
	if ( backEnd.viewDef->viewEntitys && r_skipAmbient.GetInteger() & 1 )
		return;
	// set the color
	float		color[4];
	const float	*regs = surf->shaderRegisters;
	color[0] = regs[pStage->color.registers[0]];
	color[1] = regs[pStage->color.registers[1]];
	color[2] = regs[pStage->color.registers[2]];
	color[3] = regs[pStage->color.registers[3]];

	// skip the entire stage if an add would be black
	if ( ( pStage->drawStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE ) && color[0] <= 0 && color[1] <= 0 && color[2] <= 0 ) {
		return;
	}

	// skip the entire stage if a blend would be completely transparent
	if ( ( pStage->drawStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) && color[3] <= 0 ) {
		return;
	}
	const float zero[4] = { 0, 0, 0, 0 };
	static const float one[4] = { 1, 1, 1, 1 };
	const float negOne[4] = { -color[0], -color[1], -color[2], -1 };

	GLColorOverride colorOverride;

	switch ( pStage->texture.texgen ) {
	case TG_SKYBOX_CUBE:
	case TG_WOBBLESKY_CUBE:
		//qglEnableVertexAttribArray( 8 );
		//qglVertexAttribPointer( 8, 3, GL_FLOAT, false, 0, vertexCache.VertexPosition( surf->dynamicTexCoords ) );
		programManager->cubeMapShader->Activate();
		{
			auto uniforms = programManager->cubeMapShader->GetUniformGroup<CubemapUniforms>();
			uniforms->skybox.Set( 1 );
			uniforms->modelMatrix.Set( surf->space->modelMatrix );
			idVec3 localViewOrigin;
			R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, localViewOrigin );
			uniforms->viewOrigin.Set( localViewOrigin );
		}
		break;
	case TG_REFLECT_CUBE:
		colorOverride.Enable( color );
		break;
	case TG_SCREEN:
		colorOverride.Enable( color );
	default:
		programManager->oldStageShader->Activate();
		OldStageUniforms *oldStageUniforms = programManager->oldStageShader->GetUniformGroup<OldStageUniforms>();
		switch ( pStage->vertexColor )	 {
		case SVC_IGNORE:
			oldStageUniforms->colorMul.Set( zero );
			oldStageUniforms->colorAdd.Set( color );
			break;
		case SVC_MODULATE:
			// select the vertex color source
			oldStageUniforms->colorMul.Set( color );
			oldStageUniforms->colorAdd.Set( zero );
			break;
		case SVC_INVERSE_MODULATE:
			// select the vertex color source
			oldStageUniforms->colorMul.Set( negOne );
			oldStageUniforms->colorAdd.Set( color );
			break;
		}
	}
	auto prog = GLSLProgram::GetCurrentProgram();
	if ( prog ) {
		Uniforms::Global* transformUniforms = prog->GetUniformGroup<Uniforms::Global>();
		transformUniforms->Set( surf->space );
	}
	RB_PrepareStageTexturing( pStage, surf );

	// bind the texture
	RB_BindVariableStageImage( &pStage->texture, regs );

	// set the state
	GL_State( pStage->drawStateBits );

	// draw it
	RB_DrawElementsWithCounters( surf );

	RB_FinishStageTexturing( pStage, surf );

	switch ( pStage->texture.texgen ) {
	case TG_REFLECT_CUBE:
		break;
	case TG_SKYBOX_CUBE:
	case TG_WOBBLESKY_CUBE:
		programManager->cubeMapShader->GetUniformGroup<CubemapUniforms>()->skybox.Set( 0 );
	case TG_SCREEN:
	default:
		GLSLProgram::Deactivate();
	}
}

/*
==================
RB_STD_T_RenderShaderPasses_New

Extracted from the giantic loop in RB_STD_T_RenderShaderPasses
==================
*/
void RB_STD_T_RenderShaderPasses_GLSL( const shaderStage_t *pStage, const drawSurf_t *surf ) {
	if ( r_skipNewAmbient & 1 ) 
		return;

	newShaderStage_t *newStage = pStage->newStage;
	GL_State( pStage->drawStateBits );
	newStage->glslProgram->Activate();

	newStage->glslProgram->GetUniformGroup<Uniforms::Global>()->Set( surf->space );
	newStage->glslProgram->GetUniformGroup<Uniforms::MaterialStage>()->Set( pStage, surf );
	{
		using namespace Attributes::Default;
		//Attributes::Default::SetDrawVert( (size_t)ac, (1<<Position) | (1<<TexCoord) | (1<<Normal) | (1<<Tangent) | (1<<Bitangent));
	}
	RB_DrawElementsWithCounters( surf );

	GL_SelectTexture( 0 );
	GLSLProgram::Deactivate();
}

/*
==================
RB_STD_T_RenderShaderPasses_SoftParticle

Extracted from the giantic loop in RB_STD_T_RenderShaderPasses
==================
*/
void RB_STD_T_RenderShaderPasses_SoftParticle( const shaderStage_t *pStage, const drawSurf_t *surf ) {
	// determine the blend mode (used by soft particles #3878)
	const int src_blend = pStage->drawStateBits & GLS_SRCBLEND_BITS;
	if ( (r_skipNewAmbient & 2) || !( src_blend == GLS_SRCBLEND_ONE || src_blend == GLS_SRCBLEND_SRC_ALPHA ) ) {
		return;
	}

	GLColorOverride colorOverride;

	// SteveL #3878. Particles are automatically softened by the engine, unless they have shader programs of
	// their own (i.e. are "newstages" handled above). This section comes after the newstage part so that if a
	// designer has specified their own shader programs, those will be used instead of the soft particle program.
	if ( pStage->vertexColor == SVC_IGNORE ) {
		// Ignoring vertexColor is not recommended for particles. The particle system uses vertexColor for fading.
		// However, there are existing particle effects that don't use it, in which case we default to using the
		// rgb color modulation specified in the material like the "old stages" do below.
		const float	*regs = surf->shaderRegisters;
		float		color[4];
		color[0] = regs[pStage->color.registers[0]];
		color[1] = regs[pStage->color.registers[1]];
		color[2] = regs[pStage->color.registers[2]];
		color[3] = regs[pStage->color.registers[3]];
		colorOverride.Enable( color );
	}

	// Disable depth clipping. The fragment program will handle it to allow overdraw.
	GL_State( pStage->drawStateBits | GLS_DEPTHFUNC_ALWAYS );

	programManager->softParticleShader->Activate();
	programManager->softParticleShader->GetUniformGroup<Uniforms::Global>()->Set( surf->space );

	// Bind image and _currentDepth
	GL_SelectTexture( 0 );
	pStage->texture.image->Bind();
	GL_SelectTexture( 1 );

	globalImages->currentDepthImage->Bind();

	// Set up parameters for fragment program
	// program.env[5] is the particle radius, given as { radius, 1/(faderange), 1/radius }
	float fadeRange;

	// fadeRange is the particle diameter for alpha blends (like smoke), but the particle radius for additive
	// blends (light glares), because additive effects work differently. Fog is half as apparent when a wall
	// is in the middle of it. Light glares lose no visibility when they have something to reflect off. See
	// issue #3878 for diagram
	if ( src_blend == GLS_SRCBLEND_SRC_ALPHA ) { // an alpha blend material
		fadeRange = surf->particle_radius * 2.0f;
	} else if ( src_blend == GLS_SRCBLEND_ONE ) { // an additive (blend add) material
		fadeRange = surf->particle_radius;
	}

	float params[4] = {
		surf->particle_radius,
		1.0f / ( fadeRange ),
		1.0f / surf->particle_radius,
		0.0f
	};

	// program.env[6] is the color channel mask. It gets added to the fade multiplier, so adding 1
	//    to a channel will make sure it doesn't get faded at all. Particles with additive blend
	//    need their RGB channels modifying to blend them out. Particles with an alpha blend need
	//    their alpha channel modifying.
	float blend[4];
	if ( src_blend == GLS_SRCBLEND_SRC_ALPHA ) { // an alpha blend material
		blend[0] = blend[1] = blend[2] = 1.0f; // Leave the rgb channels at full strength when fading
		blend[3] = 0.0f;						// but fade the alpha channel
	} else if ( src_blend == GLS_SRCBLEND_ONE ) { // an additive (blend add) material
		blend[0] = blend[1] = blend[2] = 0.0f; // Fade the rgb channels but
		blend[3] = 1.0f;						// leave the alpha channel at full strength
	}
	auto group = programManager->softParticleShader->GetUniformGroup<Uniforms::SoftParticle>();
	group->softParticleParams.Set(params);
	group->softParticleBlend.Set(blend);
	//note: we need only u_scaleDepthCoords, but this is the easiest way to reuse code
	programManager->softParticleShader->GetUniformGroup<Uniforms::MaterialStage>()->Set(pStage, surf);

	// draw it
	RB_DrawElementsWithCounters( surf );

	GL_SelectTexture( 0 );
}

/*
==================
RB_STD_T_RenderShaderPasses

This is also called for the generated 2D rendering
==================
*/
void RB_STD_T_RenderShaderPasses( const drawSurf_t *surf ) {
	int						stage;
	const idMaterial		*shader;
	const shaderStage_t		*pStage;
	const float				*regs;

	shader = surf->material;

	if ( !shader->HasAmbient() ) {
		return;
	}

	if ( shader->IsPortalSky() ) { // NB TDM portal sky does not use this flag or whatever mechanism
		return;    // it used to support. Our portalSky is drawn in this procedure using
	}

	// the skybox image captured in _currentRender. -- SteveL working on #4182
	if ( surf->material->GetSort() == SS_PORTAL_SKY && g_enablePortalSky.GetInteger() == 2 ) {
		return;
	}

	GL_CheckErrors();

	// change the scissor if needed
	backEnd.currentScissor = surf->scissorRect;
	GL_ScissorVidSize(
		backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1
	);
	GL_CheckErrors();

	// some deforms may disable themselves by setting numIndexes = 0
	if ( !surf->numIndexes ) {
		return;
	}

	if ( !surf->ambientCache.IsValid() ) {
		common->Printf( "RB_T_RenderShaderPasses: !tri->ambientCache\n" );
		return;
	}

	// check whether we're drawing a soft particle surface #3878
	const bool soft_particle = ( surf->dsFlags & DSF_SOFT_PARTICLE ) != 0;

	// get the expressions for conditionals / color / texcoords
	regs = surf->shaderRegisters;

	// set face culling appropriately
	GL_Cull( shader->GetCullType() );
	GL_CheckErrors();

	// set polygon offset if necessary
	if ( shader->TestMaterialFlag( MF_POLYGONOFFSET ) ) {
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
		GL_CheckErrors();
	}

	if ( surf->space->weaponDepthHack ) {
		RB_EnterWeaponDepthHack();
		GL_CheckErrors();
	}

	if ( surf->space->modelDepthHack != 0.0f && !soft_particle ) { // #3878 soft particles don't want modelDepthHack, which is
		// an older way to slightly "soften" particles
		RB_EnterModelDepthHack( surf->space->modelDepthHack );
		GL_CheckErrors();
	}
	vertexCache.VertexPosition( surf->ambientCache );

	for ( stage = 0; stage < shader->GetNumStages() ; stage++ ) {
		pStage = shader->GetStage( stage );

		// check the enable condition
		if ( regs[ pStage->conditionRegister ] == 0 ) {
			continue;
		}

		// skip the stages involved in lighting
		if ( pStage->lighting != SL_AMBIENT ) {
			continue;
		}

		// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
		if ( ( pStage->drawStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE ) ) {
			continue;
		}

		// see if we are a new-style stage
		newShaderStage_t *newStage = pStage->newStage;

		if ( newStage ) {
			RB_STD_T_RenderShaderPasses_GLSL( pStage, surf );
			continue;
		}

		if ( soft_particle && surf->particle_radius > 0.0f ) {
			RB_STD_T_RenderShaderPasses_SoftParticle( pStage, surf );
			continue;
		}
		RB_STD_T_RenderShaderPasses_OldStage( pStage, surf );
	}

	// reset polygon offset
	if ( shader->TestMaterialFlag( MF_POLYGONOFFSET ) ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}

	if ( surf->space->weaponDepthHack || ( !soft_particle && surf->space->modelDepthHack != 0.0f ) ) {
		RB_LeaveDepthHack();
	}
	GL_CheckErrors();
}

/*
=====================
RB_STD_DrawShaderPasses

Draw non-light dependent passes
=====================
*/
int RB_STD_DrawShaderPasses( drawSurf_t **drawSurfs, int numDrawSurfs, bool postProcessing ) {
	int	 i;

	// only obey skipAmbient if we are rendering a view
	if ( !numDrawSurfs ) {
		return numDrawSurfs;
	}
	TRACE_GL_SCOPE( "STD_DrawShaderPasses" );

	idList<const idMaterial*> renderCopies;

	GL_SelectTexture( 0 );

	GL_CheckErrors();

	// we don't use RB_RenderDrawSurfListWithFunction()
	// because we want to defer the matrix load because many
	// surfaces won't draw any ambient passes
	backEnd.currentSpace = NULL;

	// stgatilov: disabled for now
	static idCVar r_testPostprocessMultiple(
		"r_testPostprocessMultiple", "1", CVAR_INTEGER | CVAR_RENDERER,
		"Set above 1 to allow rendering more than one postprocessing surface with frame copy.\n"
		"See also: https://forums.thedarkmod.com/index.php?/topic/21477-water-effects-not-rendered-through-warp-glass/page/4"
	);
	
	for ( i = 0  ; i < numDrawSurfs ; i++ ) {
		if ( drawSurfs[i]->sort >= SS_POST_PROCESS ) {
			if ( !postProcessing )
				break; // we need to draw the post process shaders after we have drawn the fog lights
			if ( !renderCopies.Find( drawSurfs[i]->material ) && renderCopies.Num() < r_testPostprocessMultiple.GetInteger() ) {
				renderCopies.Append( drawSurfs[i]->material );
				frameBuffers->UpdateCurrentRenderCopy();
			}
		}
		RB_STD_T_RenderShaderPasses( drawSurfs[i] );
		GL_CheckErrors();
	}
	GL_Cull( CT_FRONT_SIDED );

	return i;
}

/*
=============================================================================================

BLEND LIGHT PROJECTION

=============================================================================================
*/

/*
=====================
RB_T_BlendLight

=====================
*/
static void RB_T_BlendLight( const drawSurf_t *surf ) {
	if ( backEnd.currentSpace != surf->space ) {
		idPlane	lightProject[4];
		int		i;

		for ( i = 0 ; i < 4 ; i++ ) {
			R_GlobalPlaneToLocal( surf->space->modelMatrix, backEnd.vLight->lightProject[i], lightProject[i] );
		}
		BlendUniforms *blendUniforms = programManager->blendShader->GetUniformGroup<BlendUniforms>();
		blendUniforms->tex0PlaneS.Set( lightProject[0] );
		blendUniforms->tex0PlaneT.Set( lightProject[1] );
		blendUniforms->tex0PlaneQ.Set( lightProject[2] );
		blendUniforms->tex1PlaneS.Set( lightProject[3] );
	}

	// this gets used for both blend lights and shadow draws
	if ( surf->ambientCache.IsValid() ) {
		vertexCache.VertexPosition( surf->ambientCache );
	} else if ( surf->shadowCache.IsValid() ) {
		vertexCache.VertexPosition( surf->shadowCache, ATTRIB_SHADOW );
	}
	RB_DrawElementsWithCounters( surf );
}

/*
=====================
RB_BlendLight

Dual texture together the falloff and projection texture with a blend
mode to the framebuffer, instead of interacting with the surface texture
=====================
*/
static void RB_BlendLight( const drawSurf_t *drawSurfs,  const drawSurf_t *drawSurfs2 ) {
	const idMaterial	*lightShader;
	const shaderStage_t	*stage;
	int					i;
	const float	*regs;

	if ( !drawSurfs ) {
		return;
	}
	if ( r_skipBlendLights.GetBool() ) {
		return;
	}

	lightShader = backEnd.vLight->lightShader;
	regs = backEnd.vLight->shaderRegisters;

	// texture 1 will get the falloff texture
	GL_SelectTexture( 1 );
	backEnd.vLight->falloffImage->Bind();

	// texture 0 will get the projected texture
	programManager->blendShader->Activate();
	BlendUniforms *blendUniforms = programManager->blendShader->GetUniformGroup<BlendUniforms>();

	for ( i = 0 ; i < lightShader->GetNumStages() ; i++ ) {
		stage = lightShader->GetStage( i );

		if ( !regs[ stage->conditionRegister ] ) {
			continue;
		}
		GL_State( GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_EQUAL );

		GL_SelectTexture( 0 );
		stage->texture.image->Bind();

		RB_LoadShaderTextureMatrix( regs, stage );

		// get the modulate values from the light, including alpha, unlike normal lights
		backEnd.lightColor[0] = regs[ stage->color.registers[0] ];
		backEnd.lightColor[1] = regs[ stage->color.registers[1] ];
		backEnd.lightColor[2] = regs[ stage->color.registers[2] ];
		backEnd.lightColor[3] = regs[ stage->color.registers[3] ];
		blendUniforms->blendColor.Set( backEnd.lightColor );

		RB_RenderDrawSurfChainWithFunction( drawSurfs, RB_T_BlendLight );
		RB_RenderDrawSurfChainWithFunction( drawSurfs2, RB_T_BlendLight );

		RB_LoadShaderTextureMatrix( NULL, stage );
		/*if ( stage->texture.hasMatrix ) {
			GL_SelectTexture( 0 );
			qglMatrixMode( GL_TEXTURE );
			qglLoadIdentity();
			qglMatrixMode( GL_MODELVIEW );
		}*/
	}
	GL_SelectTexture( 0 );
	GLSLProgram::Deactivate();
}

//========================================================================

static idPlane	fogPlanes[2];

/*
=====================
RB_T_BasicFog
=====================
*/
static void RB_T_BasicFog( const drawSurf_t *surf ) {
	FogUniforms* fogUniforms = programManager->fogShader->GetUniformGroup<FogUniforms>();
	if ( backEnd.currentSpace != surf->space ) {

		idPlane	local;

		R_GlobalPlaneToLocal( surf->space->modelMatrix, fogPlanes[0], local );
		local[3] += 0.5;
		fogUniforms->tex0PlaneS.Set( local );

		R_GlobalPlaneToLocal( surf->space->modelMatrix, fogPlanes[1], local );
		local[3] += FOG_ENTER;
		fogUniforms->tex1PlaneT.Set( local );
	}
	if ( surf->material && surf->material->Coverage() == MC_TRANSLUCENT && surf->material->ReceivesLighting() )
		fogUniforms->fogAlpha.Set( surf->material->FogAlpha() );

	RB_T_RenderTriangleSurface( surf );
}

/*
==================
RB_FogPass
==================
*/
static void RB_FogPass( bool translucent ) {
	const srfTriangles_t *frustumTris;
	drawSurf_t			ds;
	const idMaterial	*lightShader;
	const shaderStage_t	*stage;
	const float			*regs;

	// create a surface for the light frustom triangles, which are oriented drawn side out
	frustumTris = backEnd.vLight->frustumTris;

	// if we ran out of vertex cache memory, skip it
	if ( !frustumTris->ambientCache.IsValid() ) {
		return;
	}
	memset( &ds, 0, sizeof( ds ) );

	if ( !backEnd.vLight->noFogBoundary ) { // No need to create the drawsurf if we're not fogging the bounding box -- #3664
		ds.space = &backEnd.viewDef->worldSpace;
		//ds.backendGeo = frustumTris;
		ds.frontendGeo = frustumTris; // FIXME JIC
		ds.numIndexes = frustumTris->numIndexes;
		ds.indexCache = frustumTris->indexCache;
		ds.ambientCache = frustumTris->ambientCache;
		ds.scissorRect = backEnd.viewDef->scissor;
	}

	// find the current color and density of the fog
	lightShader = backEnd.vLight->lightShader;
	regs = backEnd.vLight->shaderRegisters;

	// assume fog shaders have only a single stage
	stage = lightShader->GetStage( 0 );

	backEnd.lightColor[0] = regs[ stage->color.registers[0] ];
	backEnd.lightColor[1] = regs[ stage->color.registers[1] ];
	backEnd.lightColor[2] = regs[ stage->color.registers[2] ];
	backEnd.lightColor[3] = regs[ stage->color.registers[3] ];

	// calculate the falloff planes
	float	a;

	// if they left the default value on, set a fog distance of 500
	if ( backEnd.lightColor[3] <= 1.0 ) {
		a = -0.5f / DEFAULT_FOG_DISTANCE;
	} else {
		// otherwise, distance = alpha color
		a = -0.5f / backEnd.lightColor[3];
	}
	GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );

	// texture 0 is the falloff image
	GL_SelectTexture( 0 );
	globalImages->fogImage->Bind();

	programManager->fogShader->Activate();
	FogUniforms *fogUniforms = programManager->fogShader->GetUniformGroup<FogUniforms>();
	fogUniforms->fogColor.Set( backEnd.lightColor );

	fogPlanes[0][0] = a * backEnd.viewDef->worldSpace.modelViewMatrix[2];
	fogPlanes[0][1] = a * backEnd.viewDef->worldSpace.modelViewMatrix[6];
	fogPlanes[0][2] = a * backEnd.viewDef->worldSpace.modelViewMatrix[10];
	fogPlanes[0][3] = a * backEnd.viewDef->worldSpace.modelViewMatrix[14];

	// texture 1 is the entering plane fade correction
	GL_SelectTexture( 1 );
	globalImages->fogEnterImage->Bind();

	// T will get a texgen for the fade plane, which is always the "top" plane on unrotated lights
	fogPlanes[1][0] = 0.001f * backEnd.vLight->fogPlane[0];
	fogPlanes[1][1] = 0.001f * backEnd.vLight->fogPlane[1];
	fogPlanes[1][2] = 0.001f * backEnd.vLight->fogPlane[2];
	fogPlanes[1][3] = 0.001f * backEnd.vLight->fogPlane[3];

	// S is based on the view origin
	float s = backEnd.viewDef->renderView.vieworg * fogPlanes[1].Normal() + fogPlanes[1][3];
	fogUniforms->fogEnter.Set( FOG_ENTER + s );
	fogUniforms->fogAlpha.Set( 1 );

	if ( translucent ) {
		GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS );
		GL_Cull( CT_TWO_SIDED );
		if ( !( r_skipFogLights & 2 ) ) {
			RB_RenderDrawSurfChainWithFunction( backEnd.vLight->translucentInteractions, RB_T_BasicFog );
		}
	} else {
		// draw it
		if ( !( r_skipFogLights & 1 ) ) {
			RB_RenderDrawSurfChainWithFunction( backEnd.vLight->globalInteractions, RB_T_BasicFog );
			RB_RenderDrawSurfChainWithFunction( backEnd.vLight->localInteractions, RB_T_BasicFog );
		}

		if ( !backEnd.vLight->noFogBoundary ) { // Let mappers suppress fogging the bounding box -- SteveL #3664
			// the light frustum bounding planes aren't in the depth buffer, so use depthfunc_less instead
			// of depthfunc_equal
			GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS );
			GL_Cull( CT_BACK_SIDED );
			if ( !( r_skipFogLights & 4 ) ) {
				RB_RenderDrawSurfChainWithFunction( &ds, RB_T_BasicFog );
			}
		}
	}

	GL_Cull( CT_FRONT_SIDED );

	GL_SelectTexture( 0 );
	GLSLProgram::Deactivate();
}

/*
==================
RB_STD_FogAllLights
==================
*/
void RB_STD_FogAllLights( bool translucent ) {
	if ( r_showOverDraw.GetInteger() != 0 ) {
		return;
	}
	TRACE_GL_SCOPE( "STD_FogAllLights" );

	for ( backEnd.vLight = backEnd.viewDef->viewLights ; backEnd.vLight; backEnd.vLight = backEnd.vLight->next ) {
		if ( backEnd.vLight->volumetricDust > 0.0f && !backEnd.viewDef->IsLightGem() && !translucent ) {
			volumetric->RenderLight( backEnd.viewDef, backEnd.vLight );
			continue;
		}
		if ( !backEnd.vLight->lightShader->IsFogLight() && !backEnd.vLight->lightShader->IsBlendLight() ) {
			continue;
		}
		qglDisable( GL_STENCIL_TEST );

		if ( backEnd.vLight->lightShader->IsFogLight() ) {
			RB_FogPass( translucent );
		} 
		if ( translucent )
			continue;
		if ( backEnd.vLight->lightShader->IsBlendLight() ) {
			RB_BlendLight( backEnd.vLight->globalInteractions, backEnd.vLight->localInteractions );
		}
	}
	qglEnable( GL_STENCIL_TEST );
}
