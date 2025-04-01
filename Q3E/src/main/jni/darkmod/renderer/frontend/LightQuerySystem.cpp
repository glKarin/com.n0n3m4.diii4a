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
#include "renderer/frontend/LightQuerySystem.h"

// samples on the wall up to this tolerance should be treated as not occluded by the wall
static const float POS_TOLERANCE = 0.1f;

idCVar r_lqsParallel(
	"r_lqsParallel", "0", CVAR_BOOL | CVAR_RENDERER, 
	"Process queries in LightQuerySystem in parallel?"
);

// ==================================================================================

idRenderWorld::lightQuery_t LightQuerySystem::AddQuery( const idRenderEntityLocal *onEntity, const samplePointOnModel_t &point, const idList<qhandle_t> &ignoredEntities ) {
	int i;
#if 0
	for ( i = 0; i < queries.Num(); i++ ) {
		if ( queries[i].status < 0 )
			break;
	}
#else
	if ( deadQueryList.Num() > 0 ) {
		i = deadQueryList.Pop();
	} else {
		i = queries.Num();
	}
#endif
	if ( i == queries.Num() ) {
		queries.Alloc();
	}

	LightQuery &qr = queries[i];
	qr.status = 0;
	qr.entity = onEntity;
	qr.model = qr.entity->parms.hModel;
	qr.sample = point;
	qr.ignoredEntities = ignoredEntities;
	qr.resultValue = idVec3(0);

	return i;
}

bool LightQuerySystem::CheckResult( lightQuery_t query, idVec3 &outputValue, idVec3& outputPosition ) const {
	const LightQuery &qr = queries[query];
	if ( qr.status > 0 ) {
		outputValue = qr.resultValue;
		outputPosition = qr.position;
		return true;
	}
	return false;
}

void LightQuerySystem::Forget( lightQuery_t query ) {
	LightQuery &qr = queries[query];
	qr.status = -1;
	deadQueryList.AddGrow(query);
}

void LightQuerySystem::Think( const viewDef_t *viewDef ) {
	TRACE_CPU_SCOPE( "LQS::Think" );

	struct Job {
		const LightQuerySystem *system;
		const viewDef_t *viewDef;
		LightQuery *query;

		void Run() const {
			system->RecomputeQuery( *query, viewDef );
		}
		static void Invoke( void *param ) {
			const Job *job = (const Job*)(param);
			job->Run();
		}
	};

	idList<Job> jobs;
	jobs.Reserve( queries.Num() );

	int count[3] = {0};
	for ( int i = 0; i < queries.Num(); i++ ) {
		LightQuery &qr = queries[i];
		count[qr.status + 1]++;
		if ( qr.status != 0 )
			continue;
		jobs.AddGrow( { this, viewDef, &qr } );
	}

	if ( r_lqsParallel.GetBool() ) {
		RegisterJob( Job::Invoke, "lightQuery" );
		idParallelJobList *joblist = tr.frontEndJobList;
		for ( Job &j : jobs )
			joblist->AddJob( Job::Invoke, &j );
		joblist->Submit( nullptr, JOBLIST_PARALLELISM_REALTIME );
		joblist->Wait();
	} else {
		for ( Job &j : jobs )
			j.Run();
	}

	TRACE_ATTACH_FORMAT( "dead: %d\npending: %d\nfinished: %d\n", count[0], count[1], count[2] )
}

// ==================================================================================

void LightQuerySystem::RecomputeQuery( LightQuery &query, const viewDef_t *viewDef ) const {
	TRACE_CPU_SCOPE_TEXT( "LQS::RecomputeQuery", GetTraceLabel( query.entity->parms ) )
	query.position = UpdateSamplePos( query );
	// this temporary variable must be independent in each parallel thread
	static thread_local Context ctx;
	ctx.viewDef = viewDef;
	query.resultValue = ComputeQuery( query, ctx );
	query.status = 1;
}

idVec3 LightQuerySystem::UpdateSamplePos( const LightQuery &query ) const {
	idVec3 position;
	if ( query.model ) {
		// point moves on animated model
		position = query.model->GetSamplePosition( &query.entity->parms, query.sample );
	} else {
		position = query.sample.staticPosition;
	}
	if ( query.entity ) {
		// transform from model space into world space
		const renderEntity_t &parms = query.entity->parms;
		position = parms.origin + position * parms.axis;
	}
	return position;
}

idVec3 LightQuerySystem::ComputeQuery( const LightQuery &query, Context &ctx ) const {
	int areaList[16];
	int areaCnt;

	// sample might be on area boundary, so query for area with tolerance
	idBounds box( query.position );
	box.ExpandSelf( POS_TOLERANCE );
	areaCnt = world->FindAreasInBounds( box, areaList, 16 );
	if ( areaCnt == 0 )
		return vec3_zero;	// surely in solid

	idVec3 totalLight( 0.0f );

	idFlexList<int, 128> processedLights;
	for ( int i = 0; i < areaCnt; i++ ) {
		int areaIdx = areaList[i];
		const portalArea_t &area = world->portalAreas[areaIdx];

		for ( int lightIdx : area.lightRefs ) {
			const idRenderLightLocal *light = world->lightDefs[lightIdx];
			if ( processedLights.Find( lightIdx ) )
				continue;
			processedLights.AddGrow( lightIdx );

			idVec3 addedColor = ComputeQueryLight( query, ctx, light );
			totalLight += addedColor;
		}
	}

	return totalLight;
}

idVec3 LightQuerySystem::ComputeQueryLight( const LightQuery &query, Context &ctx, const idRenderLightLocal *light ) const {
	if ( !light->globalLightBounds.ContainsPoint( query.position ) )
		return vec3_zero;

	const idMaterial *lightShader = light->lightShader;

	if ( !lightShader )
		return vec3_zero;
	if ( lightShader->IsFogLight() || lightShader->IsBlendLight() )
		return vec3_zero;
	if ( light->parms.suppressLightInViewID == VID_LIGHTGEM )
		return vec3_zero;	// TODO: idLight::IsSeenByAI() is false

	// follows code from R_AddLightSurfaces
	// (where registers are evaluated during rendering)
	ctx.lightRegisters.SetNum( lightShader->GetNumRegisters(), false );
	lightShader->EvaluateRegisters( ctx.lightRegisters.Ptr(), light->parms.shaderParms, ctx.viewDef, light->parms.referenceSound );

	// hack in order to call register-using methods...
	ctx.viewLight.shaderRegisters = ctx.lightRegisters.Ptr();

	idVec3 res = vec3_zero;
	for ( int lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ ) {
		const shaderStage_t	*lightStage = lightShader->GetStage( lightStageNum );

		// ignore stages that fail the condition
		if ( !ctx.viewLight.IsStageEnabled( lightStage ) ) {
			continue;
		}

		res += ComputeQueryLightStage( query, ctx, light, lightStage );
	}

	return res;
}

idVec3 LightQuerySystem::ComputeQueryLightStage( const LightQuery &query, Context &ctx, const idRenderLightLocal *light, const shaderStage_t *lightStage ) const {
	// this matrix is passed to shader by backend
	// (see InteractionStage)
	const idPlane *lightProjectionFalloff = light->lightProject;
	idMat4 texMatrix = ctx.viewLight.GetTextureMatrix( lightStage );
	idVec4 lightColor = ctx.viewLight.GetStageColor( lightStage );
	//lightColor.ToVec3() *= backEnd.lightScale;	// eye adjustment?

	// clamp negative light color to zero
	// (see R_AddLightSurfaces)
	lightColor.ToVec3().MaxCW( vec3_zero );

	// follows tdm_lightproject.glsl
	idVec4 texCoord(
		lightProjectionFalloff[0].Distance( query.position ),
		lightProjectionFalloff[1].Distance( query.position ),
		lightProjectionFalloff[3].Distance( query.position ),	// note: z/w swapped
		lightProjectionFalloff[2].Distance( query.position )
	);

	idVec3 addedColor;
	if ( light->lightShader->IsCubicLight() ) {

		idVec3 cubeTC = texCoord.ToVec3() * 2.0f - idVec3( 1.0f );

		// addedColor = texture(lightProjectionCubemap, cubeTC);
		idVec3 projLight = idVec3( 0.0f );
		if ( idImage *imgAny = lightStage->texture.image ) {
			if ( idImageAsset *img = imgAny->AsAsset() ) {
				if ( !( img->residency & IR_CPU ) ) {
					globalImages->ExecuteWhenSingleThreaded( [img] {
						globalImages->EnsureImageCpuResident( img );
					} );
				}
				if ( !img->cpuData.IsValid() )
					return vec3_zero;	// maybe next frame

				projLight = img->Sample( cubeTC.x, cubeTC.y, cubeTC.z ).ToVec3();
			}
		}
		addedColor = projLight;

		float att = idMath::ClampFloat(0.0f, 1.0f, 1.0f - cubeTC.LengthFast());
		addedColor *= att * att;

	} else {

		if (
			texCoord.w <= 0 ||									// anything with inversed W
			texCoord.x < 0 || texCoord.x > texCoord.w ||		// proj U outside [0..1]
			texCoord.y < 0 || texCoord.y > texCoord.w ||		// proj V outside [0..1]
			texCoord.z < 0 || texCoord.z > 1.0					// falloff outside [0..1]
		) {
			return vec3_zero;
		}

		float falloffCoord = texCoord.z;
		idVec4 projCoords = texCoord;     //divided by last component
		projCoords.z = 0.0;

		idVec3 projTexCoords;
		projTexCoords.x = projCoords * texMatrix[0];
		projTexCoords.y = projCoords * texMatrix[1];
		projTexCoords.z = projCoords.w;

		// vec4 lightProjection = textureProj(lightProjectionTexture, projTexCoords);
		idVec3 projLight = idVec3( 1.0f );
		if ( idImage *imgAny = lightStage->texture.image ) {
			if ( idImageAsset *img = imgAny->AsAsset() ) {
				if ( !( img->residency & IR_CPU ) ) {
					globalImages->ExecuteWhenSingleThreaded( [img] {
						globalImages->EnsureImageCpuResident( img );
					} );
				}
				if ( !img->cpuData.IsValid() )
					return vec3_zero;	// maybe next frame

				projTexCoords /= projTexCoords.z;
				projLight = img->Sample( projTexCoords.x, projTexCoords.y ).ToVec3();
			}
		}

		// vec4 lightFalloff = texture(lightFalloffTexture, vec2(falloffCoord, 0.5));
		idVec3 falloff = idVec3( 1.0f );
		if ( idImageAsset *img = light->falloffImage ) {
			if ( !( img->residency & IR_CPU ) ) {
				globalImages->ExecuteWhenSingleThreaded( [img] {
					globalImages->EnsureImageCpuResident( img );
				} );
			}
			if ( !img->cpuData.IsValid() )
				return vec3_zero;	// maybe next turn

			falloff = light->falloffImage->Sample( falloffCoord, 0.5f ).ToVec3();
		}

		// return lightProjection * lightFalloff;
		addedColor = projLight;
		addedColor.MulCW( falloff );
	}

	// multiply by color from material/entity
	addedColor.MulCW( lightColor.ToVec3() );

	if ( addedColor.Max() == 0.0f )
		return vec3_zero;

	bool lightNoShadows = light->parms.noShadows || !light->lightShader->LightCastsShadows();
	if ( !lightNoShadows ) {
		auto Filter = [&](const qhandle_t *rhandle, const renderEntity_t *rent, const idRenderModel *rmodel, const idMaterial *material) -> bool {
			// ignore entity? (e.g. checking illumination of body with attachments)
			if ( rhandle ) {
				if ( query.ignoredEntities.Find( *rhandle ) )
					return false;
			}

			if ( rent ) {
				// noshadows on entity?
				if ( rent->noShadow )
					return false;
				// suppress settings (e.g. player shadow)
				// note: see similar code in idInteraction::AddActiveInteraction
				if ( rent->suppressShadowInViewID && rent->suppressShadowInViewID == ctx.viewDef->renderView.viewID )
					return false;
				if ( rent->suppressShadowInLightID && rent->suppressShadowInLightID == light->parms.lightId )
					return false;
			}

			if ( material ) {
				// noshadows on material?
				if ( !material->SurfaceCastsShadow() )
					return false;
			}

			return true;
		};

		// cast shadow ray from light source
		// but remove a bit of length near ray end
		idVec3 start = light->globalLightOrigin;
		idVec3 end = query.position;
		float lenSq = ( end - start ).LengthSqr();
		if ( lenSq > POS_TOLERANCE * POS_TOLERANCE ) {
			end += ( start - end ).Normalized() * POS_TOLERANCE;
			modelTrace_t trace;
			if ( world->TraceAll( trace, start, end, true, 0.0f, LambdaToFuncPtr( Filter ), &Filter ) )
				return vec3_zero;
		}
	}

	return addedColor;
}
