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
#pragma once

#include "renderer/tr_local.h"
#include "renderer/frontend/RenderWorld_local.h"

typedef int lightQuery_t;

class LightQuerySystem {
public:
	void Init( const idRenderWorldLocal *world ) {
		this->world = world;
	}

	lightQuery_t AddQuery( const idRenderEntityLocal *onEntity, const samplePointOnModel_t &point, const idList<qhandle_t> &ignoredEntities );
	bool CheckResult( lightQuery_t query, idVec3 &outputValue, idVec3& outputPosition ) const;
	void Forget( lightQuery_t query );
	void Think( const viewDef_t *viewDef );

private:
	struct LightQuery {
		// -1 = dead, 0 = pending, 1 = finished
		int status = -1;

		// input:
		const idRenderEntityLocal *entity = nullptr;
		const idRenderModel *model = nullptr;
		samplePointOnModel_t sample = {};
		idList<qhandle_t> ignoredEntities;

		// output:
		idVec3 resultValue = idVec3(0);
		idVec3 position = idVec3(0);
	};
	struct Context {
		const viewDef_t *viewDef = nullptr;
		idList<float> lightRegisters;
		viewLight_t viewLight;		// note: we only fill a few members here
	};

	void RecomputeQuery( LightQuery &query, const viewDef_t *viewDef ) const;
	idVec3 ComputeQuery( const LightQuery &query, Context &ctx ) const;
	idVec3 ComputeQueryLight( const LightQuery &query, Context &ctx, const idRenderLightLocal *light ) const;
	idVec3 ComputeQueryLightStage( const LightQuery &query, Context &ctx, const idRenderLightLocal *light, const shaderStage_t *lightStage ) const;
	idVec3 UpdateSamplePos( const LightQuery &query ) const;

	const idRenderWorldLocal *world = nullptr;
	idList<LightQuery> queries;
	idList<int> deadQueryList;		// to accelerate AddQuery
};

