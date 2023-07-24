// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../Atmosphere.h"
#include "RainEffect.h"
#include "../Player.h"
#include "../demos/DemoManager.h"


CLASS_DECLARATION( idEntity, sdRainEffect )
END_CLASS

/*
==============
sdRainEffect::Spawn
==============
*/
void		sdRainEffect::Spawn( void ) {
	renderEntity_t *re = GetRenderEntity();

	re->numInsts = 9;
	re->insts = new sdInstInfo[ re->numInsts ];

	idVec3 zero;
	idMat3 I;
	zero.Zero();
	I.Identity();
	this->SetPosition( zero, I );


	BecomeActive( TH_THINK );
	UpdateVisuals();

}

/*
==============
sdRainEffect::Think
==============
*/
void		sdRainEffect::Think( void ) {
	renderEntity_t *re = GetRenderEntity();
	if ( re->hModel == NULL ) {
		return;
	}

	idBounds modelbb = re->hModel->Bounds();
	idVec3 extents = (modelbb.GetMaxs() - modelbb.GetMins()) * 0.5f;

	idPlayer *p = gameLocal.GetLocalViewPlayer();
	idVec3 const &v = p->GetViewPos();
	int gridx = idMath::Ftoi( idMath::Floor(v.x / extents.x) );
	int gridy = idMath::Ftoi( idMath::Floor(v.y / extents.y) );

	idBounds bounds;
	bounds.Clear();
	sdInstInfo *inst = re->insts;
	for (int y=-1; y<=1; y++) {
		for (int x=-1; x<=1; x++) {
			idBounds bb2;
			inst->fadeOrigin = inst->inst.origin = idVec3( (x + gridx) * extents.x, (y + gridy) * extents.y, v.z );
			inst->inst.axis.Identity();
			inst->maxVisDist = 0;
			inst->minVisDist = 0.f;
			bb2 = modelbb.Translate( inst->inst.origin );
			bounds.AddBounds( bb2 );
			inst++;
		}
	}
	re->flags.overridenBounds = true;
	re->bounds = bounds;

	UpdateVisuals();
	Present();
}




/*
==============
sdRainPrecipitation::sdRainPrecipitation
==============
*/
sdRainPrecipitation::sdRainPrecipitation( sdPrecipitationParameters const &_parms ) : parms( _parms ) {
	renderEntityHandle = -1;
	memset( &renderEntity, 0, sizeof( renderEntity ) );
	renderEntity.hModel = parms.model;

	renderEntity.numInsts = 9;
	renderEntity.insts = new sdInstInfo[ renderEntity.numInsts ];

	renderEntity.axis.Identity();
	renderEntity.origin.Zero();

	SetupEffect();
}

/*
==============
sdRainPrecipitation::~sdRainPrecipitation
==============
*/
sdRainPrecipitation::~sdRainPrecipitation() {
	FreeRenderEntity();
	delete []renderEntity.insts;
	//renderModelManager->FreeModel( renderEntity.hModel );
}


/*
==============
sdRainPrecipitation::SetupEffect
==============
*/
void sdRainPrecipitation::SetupEffect( void ) {
	renderEffect_t &renderEffect = effect.GetRenderEffect();
	renderEffect.declEffect = parms.effect;
	renderEffect.axis.Identity();
	renderEffect.loop = true;
	renderEffect.shaderParms[SHADERPARM_RED]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_BLUE]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;

	effectRunning = false;
}

/*
==============
sdRainPrecipitation::SetMaxActiveParticles
==============
*/
void sdRainPrecipitation::SetMaxActiveParticles( int num ) {
}

/*
==============
sdRainPrecipitation::Update
==============
*/
void sdRainPrecipitation::Update( void ) {
	renderEntity_t *re = GetRenderEntity();
	if ( re->hModel == NULL ) {
		return;
	}

	idBounds modelbb = re->hModel->Bounds();
	idVec3 extents = (modelbb.GetMaxs() - modelbb.GetMins()) * 0.5f;

	idPlayer *p = gameLocal.GetLocalViewPlayer();
	idVec3 const &v = p->GetViewPos();
	int gridx = idMath::Ftoi( idMath::Floor(v.x / extents.x) );
	int gridy = idMath::Ftoi( idMath::Floor(v.y / extents.y) );

	idBounds bounds;
	bounds.Clear();
	sdInstInfo *inst = re->insts;
	for (int y=-1; y<=1; y++) {
		for (int x=-1; x<=1; x++) {
			idBounds bb2;
			inst->fadeOrigin = inst->inst.origin = idVec3( (x + gridx) * extents.x, (y + gridy) * extents.y, v.z );
			inst->inst.axis.Identity();
			inst->maxVisDist = 0;
			inst->minVisDist = 0.f;
			bb2 = modelbb.Translate( inst->inst.origin );
			bounds.AddBounds( bb2 );
			inst++;
		}
	}
	re->flags.overridenBounds = true;
	re->bounds = bounds;

	if ( renderEntityHandle == -1 ) {
		renderEntityHandle = gameRenderWorld->AddEntityDef( re );
	} else {
		gameRenderWorld->UpdateEntityDef( renderEntityHandle, re );
	}


	if ( !effect.GetRenderEffect().declEffect ) return;

	idVec3 viewOrg;
	renderView_t view;
	if ( sdDemoManager::GetInstance().CalculateRenderView( &view ) ) {
		viewOrg = view.vieworg;
	} else {
		// If we are inside don't run the bacground effect
		idPlayer* player = gameLocal.GetLocalViewPlayer();

		if ( player == NULL ) {
			return;
		}
		viewOrg = player->GetRenderView()->vieworg;
	}

	int area = gameRenderWorld->PointInArea( viewOrg );
	bool runEffect = false;
	if ( area >= 0 ) {
		if ( gameRenderWorld->GetAreaPortalFlags( area ) & ( 1 << PORTAL_OUTSIDE ) ) {
			runEffect = true && !g_skipLocalizedPrecipitation.GetBool();
		}
	}

	// Update the background effect
	if ( runEffect ) {
		effect.GetRenderEffect().origin = viewOrg;
		if ( !effectRunning ) {
			effect.Start( gameLocal.time );
			effectRunning = true;
		} else {
			effect.Update();
		}
	} else {
		effect.StopDetach();
		effectRunning = false;
	}
}

/*
==============
sdRainPrecipitation::Init
==============
*/
void sdRainPrecipitation::Init( void ) {
}

/*
==============
sdRainPrecipitation::FreeRenderEntity
==============
*/
void sdRainPrecipitation::FreeRenderEntity( void ) {
	if ( renderEntityHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( renderEntityHandle );
		renderEntityHandle = -1;
	}
	if ( !effect.GetRenderEffect().declEffect ) return;

	effect.FreeRenderEffect();
}
