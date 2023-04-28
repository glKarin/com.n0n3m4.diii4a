// BSE_Manager.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop


#include "BSE.h"

rvBSEManagerLocal bseLocal;
rvBSEManager* bse = &bseLocal;

const char	*bse_singleEffectArgs[]	= {
	"0",
	"emitter",
	"spawner",
	"trail",
	"sound",
	"decal",
	"light",
	"shake",

	"sprite",
	"line",
	"oriented",
	"model",
	"electricity",
	"linked",
	"orientedlinked",

	NULL
};
idCVar bse_render("bse_render", "1", CVAR_BOOL, "disable effect rendering");
idCVar bse_enabled("bse_enabled", "1", CVAR_BOOL | CVAR_ARCHIVE, "set to false to disable all effects");
idCVar bse_debug("bse_debug", "0", CVAR_INTEGER, "display debug info about effect");
idCVar bse_singleEffect("bse_singleEffect", "", 0, "set to the name of the effect that is only played", bse_singleEffectArgs, idCmdSystem::ArgCompletion_String<bse_singleEffectArgs>);
//idCVar bse_rateLimit("bse_rateLimit", "1", CVAR_FLOAT, "rate limit for spawned effects");
#if 0
idCVar bse_showBounds("bse_showbounds", "0", CVAR_BOOL, "display debug bounding boxes effect");
idCVar bse_physics("bse_physics", "1", CVAR_BOOL, "disable effect physics");
idCVar bse_debris("bse_debris", "1", CVAR_BOOL, "disable effect debris");
idCVar bse_speeds("bse_speeds", "0", CVAR_INTEGER, "print bse frame statistics");
idCVar bse_rateCost("bse_rateCost", "1", CVAR_FLOAT, "rate cost multiplier for spawned effects");

float effectCosts[EC_MAX] = { 0, 2, 0.1 }; // dd 0.0, 2 dup(0.1)
#endif

idRandom	rvBSEManagerLocal::random;
idBlockAlloc<rvBSE, 256/*, 0 //k*/>	rvBSEManagerLocal::effects;
#if 0
idVec3						rvBSEManagerLocal::mCubeNormals[6];
idMat3						rvBSEManagerLocal::mModelToBSE;
idList<idTraceModel*>		rvBSEManagerLocal::mTraceModels;
const char* rvBSEManagerLocal::mSegmentNames[SEG_COUNT];
int							rvBSEManagerLocal::mPerfCounters[NUM_PERF_COUNTERS];
float						rvBSEManagerLocal::mEffectRates[EC_MAX];
#endif

/*
====================
vBSEManagerLocal::EffectDuration
====================
*/
float rvBSEManagerLocal::EffectDuration(const rvRenderEffectLocal* def)
{
	if(def && def->effect)
		return SEC2MS(def->effect->Duration());
	return 0.0f;
}

/*
====================
rvBSEManagerLocal::CanPlayRateLimited
====================
*/
bool rvBSEManagerLocal::CanPlayRateLimited(effectCategory_t category)
{
	//return bse_rateLimit.GetFloat() > 0.0;
	return false;
}

/*
====================
rvBSEManagerLocal::Init
====================
*/
bool rvBSEManagerLocal::Init(void) {
	common->Printf("----------------- BSE Init ------------------\n");

	random.SetSeed(Sys_Milliseconds() ^ 2003 ^ 2005 ^ 2009 ^ 2010 ^ 2014 ^ 2015 ^ 2020);
	random.SetSeed(random.RandomInt() ^ 2020 ^ 2015 ^ 2014 ^ 2010 ^ 2009 ^ 2005 ^ 2003);
	common->Printf("--------- BSE Created Successfully ----------\n");
	return true;
}

/*
====================
rvBSEManagerLocal::Filtered
====================
*/
bool rvBSEManagerLocal::Filtered(const char* name, effectCategory_t category)
{
	return !bse_enabled.GetBool();
}

/*
====================
rvBSEManagerLocal::StartFrame
====================
*/
void rvBSEManagerLocal::StartFrame()
{
}

/*
====================
rvBSEManagerLocal::EndFrame
====================
*/
void rvBSEManagerLocal::EndFrame()
{
}

/*
====================
rvBSEManagerLocal::EndLevelLoad
====================
*/
void rvBSEManagerLocal::EndLevelLoad()
{
	common->Printf("----- rvBSEManager::EndLevelLoad -----\n");
}

/*
====================
rvBSEManagerLocal::Shutdown
====================
*/
bool rvBSEManagerLocal::Shutdown(void) {
	common->Printf("--------------- BSE Shutdown ----------------\n");

	return true;
}

bool rvBSEManagerLocal::PlayEffect(class rvRenderEffectLocal* def, float time) {
	const rvDeclEffect* effectDecl;

	effectDecl = (const rvDeclEffect *)def->parms.declEffect;
	idStr effectName = def->parms.declEffect->GetName();

	if (Filtered(effectName, EC_IGNORE))
		return false;
	if (bse_debug.GetInteger())
	{
		common->Printf("Playing effect: %s at %g\n", effectName.c_str(), time);
	}
	rvBSE* bse = effects.Alloc();
	bse->Spawn();
	def->effect = bse;
	bse->Init(effectDecl, &def->parms, (idRenderWorld *)def->world, SEC2MS(time));
	return true;
}

/*
=======================
rvBSEManagerLocal::RestartEffect
=======================
*/
void rvBSEManagerLocal::RestartEffect(rvRenderEffectLocal* def)
{
}

/*
=======================
rvBSEManagerLocal::ServiceEffect
=======================
*/
bool rvBSEManagerLocal::ServiceEffect(class rvRenderEffectLocal* def, float time, bool& forcePush)
{
	// return true will remove effect
	rvBSE* fx;
	idStr effectName;

	fx = def->effect;
	if (!fx)
		return true;
	effectName = def->parms.declEffect->GetName();
	if (Filtered(effectName, EC_IGNORE))
		return true;
	int ms = SEC2MS(time);
	fx->Update(&def->parms, ms);
	if(fx->Done())
		return true;
	fx->Think();
	def->serviceTime = time;

	return false;
}

/*
=======================
rvBSEManagerLocal::StopEffect
=======================
*/
void rvBSEManagerLocal::StopEffect(rvRenderEffectLocal* def) {
	if(!def)
		return;
	if (def->effect)
	{
		if (bse_debug.GetInteger())
		{
			idStr effectName = def->parms.declEffect->GetName();
			common->Printf("Stopping effect %s\n", effectName.c_str());
		}
		def->effect->Stop();
	}
	else
	{
		def->newEffect = 0;
		def->expired = 1;
	}
}

/*
=======================
rvBSEManagerLocal::FreeEffect
=======================
*/
void rvBSEManagerLocal::FreeEffect(rvRenderEffectLocal* def)
{
	rvBSE* bse;

	if(!def)
		return;
	if (def->effect)
	{
		if (bse_debug.GetInteger())
		{
			idStr effectName = def->parms.declEffect->GetName();
			common->Printf("Freeing effect %s\n", effectName.c_str());
		}
		bse = def->effect;
		//bse->Stop();
		bse->Event_Remove();
		effects.Free(def->effect);
		def->effect = NULL;
	}
}

bool rvBSEManagerLocal::CheckDefForSound(const renderEffect_t* def) {
	if(!def)
		return false;

	const rvDeclEffect *effect = (const rvDeclEffect *)def->declEffect;
	return effect && effect->GetHasSound();
}

void rvBSEManagerLocal::BeginLevelLoad(void) {

}

