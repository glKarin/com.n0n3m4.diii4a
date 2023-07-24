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
#include "SnowEffect.h"
#include "../Player.h"
#include "../demos/DemoManager.h"


CLASS_DECLARATION( idEntity, sdSnowEffect )
END_CLASS

/*
==============
sdSnowEffect::Spawn
==============
*/
void				sdSnowEffect::Spawn( void ) {
	renderEntity_t *re = GetRenderEntity();

	re->numInsts = MAX_GROUPS;
	re->insts = new sdInstInfo[ re->numInsts ];

	idVec3 zero;
	idMat3 I;
	zero.Zero();
	I.Identity();
	this->SetPosition( zero, I );

	idBounds modelbb = re->hModel->Bounds();
	idVec3 extents = (modelbb.GetMaxs() - modelbb.GetMins()) * 0.5f;
	idPlayer *p = gameLocal.GetLocalViewPlayer();
	idVec3 const &v = p->GetViewPos();

	for (int i=0; i<MAX_GROUPS; i++) {
		groups[i].time = -1.f;
	}

	BecomeActive( TH_THINK );
	UpdateVisuals();
}

/*
==============
sdSnowEffect::Think
==============
*/
void		sdSnowEffect::Think( void ) {
	renderEntity_t *re = GetRenderEntity();
	if ( re->hModel == NULL ) {
		return;
	}

	idBounds modelbb = re->hModel->Bounds();
	idVec3 extents = (modelbb.GetMaxs() - modelbb.GetMins()) * 0.5f;

	idPlayer *p = gameLocal.GetLocalViewPlayer();
	idVec3 const &v = p->GetViewPos();

	for (int i=0; i<MAX_GROUPS; i++) {
		groups[i].time -= 1.f / 30.f;
		if ( groups[i].time < 0.f ) {
			groups[i].axis = idVec3( idRandom::StaticRandom().RandomFloat()- 0.15f , idRandom::StaticRandom().RandomFloat() - 0.5f , idRandom::StaticRandom().RandomFloat() - 0.5f );
			groups[i].axis.z *= 40.25f;
			groups[i].axis.Normalize();
			groups[i].rotate = 0.f;
			groups[i].rotateSpeed = idRandom::StaticRandom().RandomFloat() * 50.f + 50.f;
			groups[i].rotationPoint = idVec3( idRandom::StaticRandom().RandomFloat() * extents.x, 
				idRandom::StaticRandom().RandomFloat() * extents.y, 
				idRandom::StaticRandom().RandomFloat() * extents.z );

			groups[i].alpha = 0.f;
			groups[i].time = idRandom::StaticRandom().RandomFloat() * 1.f + 1.f;
			groups[i].worldPos = v + idVec3( (idRandom::StaticRandom().RandomFloat()-0.5f) * extents.x * 3, 
				(idRandom::StaticRandom().RandomFloat()-0.5f) * extents.y * 3, 
				(idRandom::StaticRandom().RandomFloat()-0.5f) * extents.z * 3 );
		} else {
			if ( groups[i].time > 0.25f ) {
				groups[i].alpha += 1.f / 7.5f;
				if ( groups[i].alpha > 1.f ) {
					groups[i].alpha = 1.f;
				}
			} else {
				groups[i].alpha = groups[i].time * 4.f;
				if ( groups[i].alpha < 0.f ) {
					groups[i].alpha = 0.f;
				}
			}
			groups[i].worldPos += idVec3( 0.f, 0.f, -600.f ) * 1.f / 30.f;
			groups[i].rotate += groups[i].rotateSpeed * 1.f / 30.f;
		}
	}


	int gridx = idMath::Ftoi( idMath::Floor(v.x / extents.x) );
	int gridy = idMath::Ftoi( idMath::Floor(v.y / extents.y) );

	idBounds bounds;
	bounds.Clear();
	sdInstInfo *inst = re->insts;
	for (int i=0; i<MAX_GROUPS; i++) {
		idRotation r( groups[i].rotationPoint, groups[i].axis, groups[i].rotate );
		
		idBounds bb2;
		inst->inst.color[0] = 0xff;
		inst->inst.color[1] = 0xff;
		inst->inst.color[2] = 0xff;
		inst->inst.color[3] = 0xff;
		inst->fadeOrigin = inst->inst.origin = groups[i].worldPos;
		inst->inst.axis = r.ToMat3();
		inst->maxVisDist = 0;
		inst->minVisDist = 0.f;
		bb2.FromTransformedBounds( modelbb, inst->inst.origin, inst->inst.axis );
		bounds.AddBounds( bb2 );
		inst++;
	}
	re->flags.overridenBounds = true;
	re->bounds = bounds;

	UpdateVisuals();
	Present();
}



/*
==============
sdSnowPrecipitation::sdSnowPrecipitation
==============
*/
sdSnowPrecipitation::sdSnowPrecipitation( sdPrecipitationParameters const &_parms ) : parms(_parms) {
	renderEntity_t *re = GetRenderEntity();

	renderEntityHandle = -1;
	memset( re, 0, sizeof( renderEntity ) );
	re->hModel = parms.model;

	re->axis.Identity();
	re->origin.Zero();

	re->numInsts = MAX_GROUPS;
	re->insts = new sdInstInfo[ re->numInsts ];

	idBounds modelbb = re->hModel->Bounds();
	idVec3 extents = (modelbb.GetMaxs() - modelbb.GetMins()) * 0.5f;
	idPlayer *p = gameLocal.GetLocalViewPlayer();
	idVec3 const &v = p->GetViewPos();

	for (int i=0; i<MAX_GROUPS; i++) {
		groups[i].time = -1.f;
	}

	SetupEffect();
}

/*
==============
sdSnowPrecipitation::~sdSnowPrecipitation
==============
*/
sdSnowPrecipitation::~sdSnowPrecipitation() {
	FreeRenderEntity();
	delete []renderEntity.insts;
}

/*
==============
sdSnowPrecipitation::SetupEffect
==============
*/
void sdSnowPrecipitation::SetupEffect( void ) {
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
sdSnowPrecipitation::SetMaxActiveParticles
==============
*/
void sdSnowPrecipitation::SetMaxActiveParticles( int num ) {
}

/*
==============
sdSnowPrecipitation::Update
==============
*/
void sdSnowPrecipitation::Update( void ) {
	renderEntity_t *re = GetRenderEntity();
	if ( re->hModel == NULL ) {
		return;
	}

	idBounds modelbb = re->hModel->Bounds();
	idVec3 extents = (modelbb.GetMaxs() - modelbb.GetMins()) * 0.5f;

	idPlayer *p = gameLocal.GetLocalViewPlayer();
	idVec3 const &v = p->GetViewPos();

	for (int i=0; i<MAX_GROUPS; i++) {
		groups[i].time -= 1.f / 30.f;
		if ( groups[i].time < 0.f ) {
			groups[i].axis = idVec3( idRandom::StaticRandom().RandomFloat()- 0.15f , idRandom::StaticRandom().RandomFloat() - 0.5f , idRandom::StaticRandom().RandomFloat() - 0.5f );
			groups[i].axis.z *= 40.25f;
			groups[i].axis.Normalize();
			groups[i].rotate = 0.f;
			groups[i].rotateSpeed = idRandom::StaticRandom().RandomFloat() * 50.f + 50.f;
			groups[i].rotationPoint = idVec3( idRandom::StaticRandom().RandomFloat() * extents.x, 
				idRandom::StaticRandom().RandomFloat() * extents.y, 
				idRandom::StaticRandom().RandomFloat() * extents.z );

			groups[i].alpha = 0.f;
			groups[i].time = idRandom::StaticRandom().RandomFloat() * 1.f + 1.f;
			groups[i].worldPos = v + idVec3( (idRandom::StaticRandom().RandomFloat()-0.5f) * extents.x * 3, 
				(idRandom::StaticRandom().RandomFloat()-0.5f) * extents.y * 3, 
				(idRandom::StaticRandom().RandomFloat()-0.5f) * extents.z * 3 );
		} else {
			if ( groups[i].time > 0.25f ) {
				groups[i].alpha += 1.f / 7.5f;
				if ( groups[i].alpha > 1.f ) {
					groups[i].alpha = 1.f;
				}
			} else {
				groups[i].alpha = groups[i].time * 4.f;
				if ( groups[i].alpha < 0.f ) {
					groups[i].alpha = 0.f;
				}
			}
			groups[i].worldPos += idVec3( 0.f, 0.f, -600.f ) * 1.f / 30.f;
			groups[i].rotate += groups[i].rotateSpeed * 1.f / 30.f;
		}
	}


	int gridx = idMath::Ftoi( idMath::Floor(v.x / extents.x) );
	int gridy = idMath::Ftoi( idMath::Floor(v.y / extents.y) );

	idBounds bounds;
	bounds.Clear();
	sdInstInfo *inst = re->insts;
	for (int i=0; i<MAX_GROUPS; i++) {
		idRotation r( groups[i].rotationPoint, groups[i].axis, groups[i].rotate );
		
		idBounds bb2;
		inst->inst.color[0] = 0xff;
		inst->inst.color[1] = 0xff;
		inst->inst.color[2] = 0xff;
		inst->inst.color[3] = 0xff;
		inst->fadeOrigin = inst->inst.origin = groups[i].worldPos;
		inst->inst.axis = r.ToMat3();
		inst->maxVisDist = 0;
		inst->minVisDist = 0.f;
		bb2.FromTransformedBounds( modelbb, inst->inst.origin, inst->inst.axis );
		bounds.AddBounds( bb2 );
		inst++;
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
sdSnowPrecipitation::Init
==============
*/
void sdSnowPrecipitation::Init( void ) {
}

/*
==============
sdSnowPrecipitation::FreeRenderEntity
==============
*/
void sdSnowPrecipitation::FreeRenderEntity( void ) {
	if ( renderEntityHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( renderEntityHandle );
		renderEntityHandle = -1;
	}
	if ( !effect.GetRenderEffect().declEffect ) return;

	effect.FreeRenderEffect();
}
