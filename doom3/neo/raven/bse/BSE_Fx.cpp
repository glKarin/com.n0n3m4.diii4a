#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "BSE.h"
#include "../../sound/snd_local.h"

static int bseSegmentNameToEnum(const char *name)
{
	if(!name || !name[0])
		return SEG_NONE;
#define SEG_CASE(x) \
	if(!idStr::Icmp(name, #x)) \
		return SEG_##x;
	//SEG_CASE(EFFECT);
	SEG_CASE(EMITTER);
	SEG_CASE(SPAWNER);
	SEG_CASE(TRAIL);
	SEG_CASE(SOUND);
	SEG_CASE(DECAL);
	SEG_CASE(LIGHT);
	//SEG_CASE(DELAY);
	SEG_CASE(SHAKE);
	//SEG_CASE(TUNNEL);
	//SEG_CASE(COUNT);
	return SEG_NONE;
#undef SEG_CASE
}

static int bseParticleNameToEnum(const char *name)
{
	if(!name || !name[0])
		return SEG_NONE;
#define PTYPE_CASE(x) \
	if(!idStr::Icmp(name, #x)) \
		return PTYPE_##x;
	PTYPE_CASE(SPRITE);
	PTYPE_CASE(LINE);
	PTYPE_CASE(ORIENTED);
	PTYPE_CASE(DECAL);
	PTYPE_CASE(MODEL);
	PTYPE_CASE(LIGHT);
	PTYPE_CASE(ELECTRICITY);
	PTYPE_CASE(LINKED);
	PTYPE_CASE(ORIENTEDLINKED);
	//PTYPE_CASE(DEBRIS);
	//PTYPE_CASE(COUNT);
	return PTYPE_NONE;
#undef PTYPE_CASE
}

/*
===============================================================================

	rvBSE

===============================================================================
*/

/*
================
rvBSE::Setup
================
*/
void rvBSE::Setup(const char *fx)
{

	if (started >= 0) {
		return;					// already started
	}

	// early during MP Spawn() with no information. wait till we ReadFromSnapshot for more
	if ((!fx || fx[0] == '\0')) {
		return;
	}

	systemName = fx;
	started = 0;

	fxEffect = static_cast<const rvDeclEffect *>(declManager->FindType(DECL_EFFECT, systemName.c_str()));

	if (fxEffect) {
		idFXLocalAction localAction;

		memset(&localAction, 0, sizeof(idFXLocalAction));

		actions.AssureSize(fxEffect->events.Num(), localAction);

		for (int i = 0; i<fxEffect->events.Num(); i++) {
			const rvFXSingleAction &fxaction = fxEffect->events[i];

			idFXLocalAction &laction = actions[i];

			if (fxaction.random1 || fxaction.random2) {
				laction.delay = fxaction.random1 + rvBSEManagerLocal::random.RandomFloat() * (fxaction.random2 - fxaction.random1);
			} else {
				laction.delay = fxaction.delay;
			}

			laction.start = -1;
			laction.lightDefHandle = -1;
			laction.modelDefHandle = -1;
			laction.particleSystem = -1;
			laction.shakeStarted = false;
			laction.decalDropped = false;
			laction.launched = false;
		}
	}
}

/*
================
rvBSE::EffectName
================
*/
const char *rvBSE::EffectName(void)
{
	return fxEffect ? fxEffect->GetName() : NULL;
}

/*
================
rvBSE::Joint
================
*/
const char *rvBSE::Joint(void)
{
	return fxEffect ? fxEffect->joint.c_str() : NULL;
}

/*
================
rvBSE::CleanUp
================
*/
void rvBSE::CleanUp(void)
{
	if (!fxEffect) {
		return;
	}

	for (int i = 0; i < fxEffect->events.Num(); i++) {
		const rvFXSingleAction &fxaction = fxEffect->events[i];
		idFXLocalAction &laction = actions[i];
		CleanUpSingleAction(fxaction, laction);
	}
	SetReferenceSound(-1);
}

/*
================
rvBSE::CleanUpSingleAction
================
*/
void rvBSE::CleanUpSingleAction(const rvFXSingleAction &fxaction, idFXLocalAction &laction)
{
	if (laction.lightDefHandle != -1 && fxaction.sibling == -1 && fxaction.type != FX_ATTACHLIGHT) {
		gameRenderWorld->FreeLightDef(laction.lightDefHandle);
		laction.lightDefHandle = -1;
	}

	if (laction.modelDefHandle != -1 && fxaction.sibling == -1 && fxaction.type != FX_ATTACHENTITY) {
		gameRenderWorld->FreeEntityDef(laction.modelDefHandle);
		laction.modelDefHandle = -1;
	}

	laction.start = -1;
}

/*
================
rvBSE::Start
================
*/
void rvBSE::Start(int time)
{
	if (!fxEffect) {
		return;
	}

	started = time;

	for (int i = 0; i < fxEffect->events.Num(); i++) {
		idFXLocalAction &laction = actions[i];
		laction.start = time;
		laction.soundStarted = false;
		laction.shakeStarted = false;
		laction.particleSystem = -1;
		laction.decalDropped = false;
		laction.launched = false;
	}
}

/*
================
rvBSE::Stop
================
*/
void rvBSE::Stop(void)
{
	CleanUp();
	started = -1;
}

/*
================
rvBSE::Duration
================
*/
const int rvBSE::Duration(void)
{
	int max = 0;

	if (!fxEffect) {
		return max;
	}

	for (int i = 0; i < fxEffect->events.Num(); i++) {
		const rvFXSingleAction &fxaction = fxEffect->events[i];
		int d = (fxaction.delay + fxaction.duration) * 1000.0f;

		if (d > max) {
			max = d;
		}
	}

	return max;
}


/*
================
rvBSE::Done
================
*/
const bool rvBSE::Done()
{
	if(started < 0 && time > 0) // started and stopped
		return true;

	if(parms.loop)
	{
		for (int i = 0; i < fxEffect->events.Num(); i++) {
			const rvFXSingleAction &fxaction = fxEffect->events[i];
			if(fxaction.restart > 0.0)
				return false;
		}
	}

	if (started > 0 && time > started + Duration()) {
		return true;
	}

	return false;
}

/*
================
rvBSE::ApplyFade
================
*/
void rvBSE::ApplyFade(const rvFXSingleAction &fxaction, idFXLocalAction &laction, const int time, const int actualStart)
{
	if (fxaction.fadeInTime || fxaction.fadeOutTime) {
		float fadePct = (float)(time - actualStart) / (1000.0f * ((fxaction.fadeInTime != 0) ? fxaction.fadeInTime : fxaction.fadeOutTime));

		if (fadePct > 1.0) {
			fadePct = 1.0;
		}

		if (laction.modelDefHandle != -1) {
			laction.renderEntity.shaderParms[SHADERPARM_RED] = (fxaction.fadeInTime) ? fadePct : 1.0f - fadePct;
			laction.renderEntity.shaderParms[SHADERPARM_GREEN] = (fxaction.fadeInTime) ? fadePct : 1.0f - fadePct;
			laction.renderEntity.shaderParms[SHADERPARM_BLUE] = (fxaction.fadeInTime) ? fadePct : 1.0f - fadePct;

			gameRenderWorld->UpdateEntityDef(laction.modelDefHandle, &laction.renderEntity);
		}

		if (laction.lightDefHandle != -1) {
			laction.renderLight.shaderParms[SHADERPARM_RED] = fxaction.lightColor.x * ((fxaction.fadeInTime) ? fadePct : 1.0f - fadePct);
			laction.renderLight.shaderParms[SHADERPARM_GREEN] = fxaction.lightColor.y * ((fxaction.fadeInTime) ? fadePct : 1.0f - fadePct);
			laction.renderLight.shaderParms[SHADERPARM_BLUE] = fxaction.lightColor.z * ((fxaction.fadeInTime) ? fadePct : 1.0f - fadePct);

			gameRenderWorld->UpdateLightDef(laction.lightDefHandle, &laction.renderLight);
		}
	}
	else if(fxaction.trackOrigin)
	{
		if (laction.modelDefHandle != -1) {
			gameRenderWorld->UpdateEntityDef(laction.modelDefHandle, &laction.renderEntity);
		}
		else if (laction.lightDefHandle != -1) {
			gameRenderWorld->UpdateLightDef(laction.lightDefHandle, &laction.renderLight);
		}
	}
}

/*
================
rvBSE::Run
================
*/
void rvBSE::Run(int time)
{
	int ieff, j;

	if (!fxEffect) {
		return;
	}

	UpdateSound();

	//LOGI("------------------ %p %s %d", fxEffect, fxEffect->GetName(), fxEffect->events.Num());
	//fxEffect->Print();
	idEntity *ent = NULL;
	const idDict *projectileDef = NULL;
	//idProjectile *projectile = NULL;
	for (ieff = 0; ieff < fxEffect->events.Num(); ieff++) {
		const rvFXSingleAction &fxaction = fxEffect->events[ieff];
		idFXLocalAction &laction = actions[ieff];

		//
		// if we're currently done with this one
		//
		if (laction.start == -1) {
			continue;
		}

		//
		// see if it's delayed
		//
		if (laction.delay) {
			if (laction.start + (time - laction.start) < laction.start + (laction.delay * 1000)) {
				continue;
			}
		}

		//
		// each event can have it's own delay and restart
		//
		int actualStart = laction.delay ? laction.start + (int)(laction.delay * 1000) : laction.start;
		float pct = (float)(time - actualStart) / (1000 * fxaction.duration);

		if (pct >= 1.0f) {
			laction.start = -1;
			float totalDelay = 0.0f;

			if (parms.loop && fxaction.restart) {
				if (fxaction.random1 || fxaction.random2) {
					totalDelay = fxaction.random1 + rvBSEManagerLocal::random.RandomFloat() * (fxaction.random2 - fxaction.random1);
				} else {
					totalDelay = fxaction.delay;
				}

				laction.delay = totalDelay;
				laction.start = time;

				continue;
			}

			CleanUpSingleAction(fxaction, laction); //karin: remove action
			continue;
		}

		if (fxaction.fire.Length()) {
			for (j = 0; j < fxEffect->events.Num(); j++) {
				if (fxEffect->events[j].name.Icmp(fxaction.fire) == 0) {
					actions[j].delay = 0;
				}
			}
		}

		idFXLocalAction *useAction;

		if (fxaction.sibling == -1) {
			useAction = &laction;
		} else {
			useAction = &actions[fxaction.sibling];
		}

		assert(useAction);

		// single debug
		const char *bseSingleEffect = bse_singleEffect.GetString();
		if(bseSingleEffect && bseSingleEffect[0] && bseSingleEffect[0] != '0')
		{
			int segType = bseSegmentNameToEnum(bseSingleEffect);
			if(segType)
			{
				if(segType != fxaction.seg)
					continue;
			}
			else
			{
				int ptype = bseParticleNameToEnum(bseSingleEffect);
				if(ptype)
				{
					if(ptype != fxaction.ptype)
						continue;
				}
			}
		}

		switch (fxaction.type) {
			case FX_ATTACHLIGHT:
			case FX_LIGHT: {
				if (useAction->lightDefHandle == -1) {
					if (fxaction.type == FX_LIGHT) {
						memset(&useAction->renderLight, 0, sizeof(renderLight_t));
						useAction->renderLight.origin = parms.origin + fxaction.offset;
						useAction->renderLight.axis = parms.axis;
						useAction->renderLight.lightRadius[0] = fxaction.lightRadius;
						useAction->renderLight.lightRadius[1] = fxaction.lightRadius;
						useAction->renderLight.lightRadius[2] = fxaction.lightRadius;
						useAction->renderLight.shader = declManager->FindMaterial(fxaction.data, false);
						useAction->renderLight.shaderParms[ SHADERPARM_RED ]	= fxaction.lightColor.x;
						useAction->renderLight.shaderParms[ SHADERPARM_GREEN ]	= fxaction.lightColor.y;
						useAction->renderLight.shaderParms[ SHADERPARM_BLUE ]	= fxaction.lightColor.z;
						useAction->renderLight.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;
						useAction->renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC(time);
						useAction->renderLight.referenceSoundHandle = parms.referenceSoundHandle;
						useAction->renderLight.pointLight = true;

						if (fxaction.noshadows) {
							useAction->renderLight.noShadows = true;
						}

						useAction->lightDefHandle = gameRenderWorld->AddLightDef(&useAction->renderLight);
					}

					if (fxaction.noshadows) {
						for (j = 0; j < fxEffect->events.Num(); j++) {
							idFXLocalAction &laction2 = actions[j];

							if (laction2.modelDefHandle != -1) {
								laction2.renderEntity.noShadow = true;
							}
						}
					}
				}
				else if (fxaction.trackOrigin) {
					useAction->renderLight.origin = parms.origin + fxaction.offset;
					useAction->renderLight.axis = parms.axis;

					useAction->renderLight.referenceSoundHandle = parms.referenceSoundHandle;
				}

				ApplyFade(fxaction, *useAction, time, actualStart);
				break;
			}
			case FX_SOUND: {
				if (!useAction->soundStarted
						&& parms.referenceSoundHandle > 0
						) {
					useAction->soundStarted = true;
					const idSoundShader *shader = declManager->FindSound(fxaction.data);
					StartSoundShader(shader, SND_CHANNEL_ANY, 0, false, NULL);

					for (j = 0; j < fxEffect->events.Num(); j++) {
						idFXLocalAction &laction2 = actions[j];

						if (laction2.lightDefHandle != -1) {
							laction2.renderLight.referenceSoundHandle = parms.referenceSoundHandle;
							gameRenderWorld->UpdateLightDef(laction2.lightDefHandle, &laction2.renderLight);
						}
					}
				}

				break;
			}
			case FX_DECAL: {
				if (!useAction->decalDropped) {
					useAction->decalDropped = true;
					ProjectDecal(parms.origin, -parms.axis[0], 8.0f, true, fxaction.size, fxaction.data);
				}

				break;
			}
			case FX_SHAKE: {
				if (!useAction->shakeStarted) {
					if(!idStr::Icmp(fxaction.data, "doubleVision"))
						game->StartViewEffect(VIEWEFFECT_DOUBLEVISION, MS2SEC(time) + fxaction.duration, fxaction.shakeAmplitude);
					else
					{
						idVec3 playerOrigin;
						idMat3 playerAxis;
						game->GetPlayerView(playerOrigin, playerAxis);
						if ((playerOrigin - parms.origin).LengthSqr() < Square(fxaction.shakeDistance)) {
							game->StartViewEffect(VIEWEFFECT_SHAKE, MS2SEC(time) + fxaction.duration, fxaction.shakeAmplitude);
						}
					}
					useAction->shakeStarted = true;
				}
				break;
			}
			case FX_ATTACHENTITY:
			case FX_PARTICLE:
			case FX_MODEL: {
				if (useAction->modelDefHandle == -1) {
					memset(&useAction->renderEntity, 0, sizeof(renderEntity_t));
					useAction->renderEntity.origin = parms.origin + fxaction.offset;
					useAction->renderEntity.axis = (fxaction.explicitAxis) ? fxaction.axis : parms.axis;
					useAction->renderEntity.hModel = renderModelManager->FindModel(fxaction.data);
					useAction->renderEntity.shaderParms[ SHADERPARM_RED ]		= 1.0f;
					useAction->renderEntity.shaderParms[ SHADERPARM_GREEN ]		= 1.0f;
					useAction->renderEntity.shaderParms[ SHADERPARM_BLUE ]		= 1.0f;
					useAction->renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC(time);
					useAction->renderEntity.shaderParms[3] = 1.0f;
					useAction->renderEntity.shaderParms[5] = 0.0f;

					if (useAction->renderEntity.hModel) {
						useAction->renderEntity.bounds = useAction->renderEntity.hModel->Bounds(&useAction->renderEntity);
					}

					useAction->renderEntity.shaderParms[ SHADERPARM_BEAM_WIDTH ]		= parms.endOrigin.x; // SHADERPARM_BEAM_END_X used by idRenderModelPrt
					useAction->renderEntity.shaderParms[ SHADERPARM_BEAM_END_Y ]		= parms.endOrigin.y;
					useAction->renderEntity.shaderParms[ SHADERPARM_BEAM_END_Z ]		= parms.endOrigin.z;
					useAction->renderEntity.shaderParms[ SHADERPARM_PARTICLE_STOPTIME ]		= 0.0f;

					useAction->modelDefHandle = gameRenderWorld->AddEntityDef(&useAction->renderEntity);
				} else if (fxaction.trackOrigin) {
					useAction->renderEntity.origin = parms.origin + fxaction.offset;
					useAction->renderEntity.axis = (fxaction.explicitAxis) ? fxaction.axis : parms.axis;
					useAction->renderEntity.shaderParms[ SHADERPARM_BEAM_WIDTH ]		= parms.endOrigin.x; // SHADERPARM_BEAM_END_X used by idRenderModelPrt
					useAction->renderEntity.shaderParms[ SHADERPARM_BEAM_END_Y ]		= parms.endOrigin.y;
					useAction->renderEntity.shaderParms[ SHADERPARM_BEAM_END_Z ]		= parms.endOrigin.z;
				}

				useAction->renderEntity.shaderParms[ SHADERPARM_RED ]		= parms.shaderParms[SHADERPARM_RED];
				useAction->renderEntity.shaderParms[ SHADERPARM_GREEN ]		= parms.shaderParms[SHADERPARM_GREEN];
				useAction->renderEntity.shaderParms[ SHADERPARM_BLUE ]		= parms.shaderParms[SHADERPARM_BLUE];
				useAction->renderEntity.shaderParms[ SHADERPARM_ALPHA ]		= parms.shaderParms[SHADERPARM_ALPHA];

				useAction->renderEntity.suppressSurfaceInViewID = parms.suppressSurfaceInViewID;
				useAction->renderEntity.allowSurfaceInViewID = parms.allowSurfaceInViewID;
				useAction->renderEntity.weaponDepthHackInViewID = parms.weaponDepthHackInViewID;
				useAction->renderEntity.modelDepthHack = parms.modelDepthHack;

				ApplyFade(fxaction, *useAction, time, actualStart);
				break;
			}
			case FX_LAUNCH: {
#if 0
				if (gameLocal.isClient) {
					// client never spawns entities outside of ClientReadSnapshot
					useAction->launched = true;
					break;
				}

				if (!useAction->launched) {
					useAction->launched = true;
					projectile = NULL;
					// FIXME: may need to cache this if it is slow
					projectileDef = gameLocal.FindEntityDefDict(fxaction.data, false);

					if (!projectileDef) {
						gameLocal.Warning("projectile \'%s\' not found", fxaction.data.c_str());
					} else {
						gameLocal.SpawnEntityDef(*projectileDef, &ent, false);

						if (ent && ent->IsType(idProjectile::Type)) {
							projectile = (idProjectile *)ent;
							projectile->Create(this, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis()[0]);
							projectile->Launch(GetPhysics()->GetOrigin(), GetPhysics()->GetAxis()[0], vec3_origin);
						}
					}
				}
#endif
				break;
			}
		}
	}
}

/*
================
rvBSE::rvBSE
================
*/
rvBSE::rvBSE()
	: time(0),
	gameRenderWorld(NULL)
{
	fxEffect = NULL;
	started = -1;
	nextTriggerTime = -1;

	Sync(NULL);
}

/*
================
rvBSE::~rvBSE
================
*/
rvBSE::~rvBSE()
{
	Event_Remove();
}

/*
================
rvBSE::Spawn
================
*/
void rvBSE::Spawn(void)
{
	systemName = "<none>";
	time = 0;
	nextTriggerTime = 0;
	gameRenderWorld = NULL;
	fxEffect = NULL;
	actions.Clear();
	started = -1;

	Sync(NULL);
}

void rvBSE::Event_Remove(void)
{
	//LOGI("rvBSE::~ %p %s", this, fxEffect ? fxEffect->GetName() : "<NULL>");
	CleanUp();
#if 0 //karin: see in quake4/client/ClientEffect.cpp::~rvClientEffect
	if(parms.referenceSoundHandle > 0)
	{
		idSoundEmitter *referenceSound = soundSystem->EmitterForIndex(SOUNDWORLD_GAME, parms.referenceSoundHandle);
		referenceSound->Free(true);
	}
#endif
	started = -1;
	parms.referenceSoundHandle = -1;
	gameRenderWorld = NULL;
	fxEffect = NULL;
}

/*
================
rvBSE::Think

  Clears any visual fx started when {item,mob,player} was spawned
================
*/
void rvBSE::Think(void)
{
	if (!bse_render.GetBool()) {
		return;
	}

	Run(time);
}

void rvBSE::ProjectDecal(const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float size, const char *material, float angle)
{
	float s, c;
	idMat3 axis, axistemp;
	idFixedWinding winding;
	idVec3 windingOrigin, projectionOrigin;

	static idVec3 decalWinding[4] = {
		idVec3(1.0f,  1.0f, 0.0f),
		idVec3(-1.0f,  1.0f, 0.0f),
		idVec3(-1.0f, -1.0f, 0.0f),
		idVec3(1.0f, -1.0f, 0.0f)
	};

#if 0
	if (!g_decals.GetBool()) {
		return;
	}
#endif

	// randomly rotate the decal winding
	idMath::SinCos16((angle) ? angle : rvBSEManagerLocal::random.RandomFloat() * idMath::TWO_PI, s, c);

	// winding orientation
	axis[2] = dir;
	axis[2].Normalize();
	axis[2].NormalVectors(axistemp[0], axistemp[1]);
	axis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * -s;
	axis[1] = axistemp[ 0 ] * -s + axistemp[ 1 ] * -c;

	windingOrigin = origin + depth * axis[2];

	if (parallel) {
		projectionOrigin = origin - depth * axis[2];
	} else {
		projectionOrigin = origin;
	}

	size *= 0.5f;

	winding.Clear();
	winding += idVec5(windingOrigin + (axis * decalWinding[0]) * size, idVec2(1, 1));
	winding += idVec5(windingOrigin + (axis * decalWinding[1]) * size, idVec2(0, 1));
	winding += idVec5(windingOrigin + (axis * decalWinding[2]) * size, idVec2(0, 0));
	winding += idVec5(windingOrigin + (axis * decalWinding[3]) * size, idVec2(1, 0));
	gameRenderWorld->ProjectDecalOntoWorld(winding, projectionOrigin, parallel, depth * 0.5f, declManager->FindMaterial(material), time);
}

bool rvBSE::StartSoundShader(const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length)
{
	float diversity;
	int len;

	if (length) {
		*length = 0;
	}

	if(parms.referenceSoundHandle <= 0)
		return false;

	if (!shader) {
		return false;
	}

	idSoundEmitter *referenceSound = soundSystem->EmitterForIndex(SOUNDWORLD_GAME, parms.referenceSoundHandle);
	if(!referenceSound)
		return false;

	// set a random value for diversity unless one was parsed from the entity
	//if (refSound.diversity < 0.0f) {
		diversity = rvBSEManagerLocal::random.RandomFloat();
	//} else {
	//	diversity = refSound.diversity;
	//}

	UpdateSound();

	diversity = 0;
	len = referenceSound->StartSound(shader, channel, diversity, soundShaderFlags);

	if (length) {
		*length = len;
	}

	// set reference to the sound for shader synced effects
	//renderEntity.referenceSound = refSound.referenceSound;

	return true;
}

void rvBSE::UpdateSound(void)
{
	if(parms.referenceSoundHandle <= 0)
		return;

	idSoundEmitter *referenceSound = soundSystem->EmitterForIndex(SOUNDWORLD_GAME, parms.referenceSoundHandle);
	if(!referenceSound)
		return;

	idSoundEmitterLocal *emitter = static_cast<idSoundEmitterLocal *>(referenceSound);
	//emitter->origin = parms.origin;
	//emitter->listenerId = parms.groupID;
	soundShaderParms_t shaderParms = emitter->parms;
	emitter->UpdateEmitter(parms.origin, parms.groupID, &shaderParms); // for fixing noise
}

void rvBSE::Init(const rvDeclEffect* declEffect, renderEffect_s* parms, idRenderWorld *world, int time)
{
	SetReferenceSound(parms->referenceSoundHandle);
	Sync(parms);
	this->gameRenderWorld = world;
	this->started = -1;
	//LOGI("Fx::Init %d %p %s %d", started, declEffect, declEffect ? declEffect->GetName() : "<NULL>", declEffect ? declEffect->events.Num() : -1);
	Setup(declEffect);
	Start(time);
}

void rvBSE::Update(renderEffect_s* parms, int time)
{
	Sync(parms);
	this->time = time;
}

void rvBSE::Sync(renderEffect_s* parms)
{
	if(parms)
		this->parms = *parms;
	else
	{
		memset( &parms, 0, sizeof( parms ) );
		this->parms.origin.Zero();
		this->parms.axis.Identity();
		this->parms.gravity.Set(0.0f, 0.0f, -1.0f);
		this->parms.endOrigin.Zero();
		this->parms.referenceSoundHandle = -1;
		this->parms.loop = false;
		this->parms.shaderParms[SHADERPARM_RED] = 1.0;
		this->parms.shaderParms[SHADERPARM_GREEN] = 1.0;
		this->parms.shaderParms[SHADERPARM_BLUE] = 1.0;
		this->parms.shaderParms[SHADERPARM_ALPHA] = 1.0;
	}
}

void rvBSE::Setup(const rvDeclEffect *fx)
{

	if (started >= 0) {
		return;					// already started
	}

	// early during MP Spawn() with no information. wait till we ReadFromSnapshot for more
	if (!fx) {
		return;
	}

	this->fxEffect = fx;
	systemName = fx->GetName();
	started = 0;

	if (fxEffect) {
		idFXLocalAction localAction;

		memset(&localAction, 0, sizeof(idFXLocalAction));

		actions.AssureSize(fxEffect->events.Num(), localAction);

		for (int i = 0; i<fxEffect->events.Num(); i++) {
			const rvFXSingleAction &fxaction = fxEffect->events[i];

			idFXLocalAction &laction = actions[i];

			if (fxaction.random1 || fxaction.random2) {
				laction.delay = fxaction.random1 + rvBSEManagerLocal::random.RandomFloat() * (fxaction.random2 - fxaction.random1);
			} else {
				laction.delay = fxaction.delay;
			}

			laction.start = -1;
			laction.lightDefHandle = -1;
			laction.modelDefHandle = -1;
			laction.particleSystem = -1;
			laction.shakeStarted = false;
			laction.decalDropped = false;
			laction.launched = false;
		}
	}
}

void rvBSE::SetReferenceSound(int handle)
{
	idSoundEmitter *referenceSound;

	if(handle <= 0)
		handle = -1;
#if 0
	else
	{
		idSoundWorld *soundWorld = soundSystem->GetSoundWorldFromId(SOUNDWORLD_GAME);
		if(handle >= static_cast<idSoundWorldLocal *>(soundWorld)->emitters.Num()) //??? safety ???
		handle = -1;
	}
#endif

	if(parms.referenceSoundHandle == handle)
		return;

	if(parms.referenceSoundHandle > 0)
	{
		referenceSound = soundSystem->EmitterForIndex(SOUNDWORLD_GAME, parms.referenceSoundHandle);
		if(referenceSound)
			referenceSound->StopSound(SND_CHANNEL_ANY);
	}
	parms.referenceSoundHandle = handle;
}
