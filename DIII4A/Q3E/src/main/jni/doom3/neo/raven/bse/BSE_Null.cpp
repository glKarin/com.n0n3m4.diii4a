// BSE_Manager.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop


#include "BSE.h"

rvBSEManagerLocal bseLocal;
rvBSEManager* bse = &bseLocal;

#if 0
idCVar bse_speeds("bse_speeds", "0", CVAR_INTEGER, "print bse frame statistics");
idCVar bse_enabled("bse_enabled", "1", CVAR_BOOL, "set to false to disable all effects");
idCVar bse_render("bse_render", "1", CVAR_BOOL, "disable effect rendering");
idCVar bse_debug("bse_debug", "0", CVAR_INTEGER, "display debug info about effect");
idCVar bse_showBounds("bse_showbounds", "0", CVAR_BOOL, "display debug bounding boxes effect");
idCVar bse_physics("bse_physics", "1", CVAR_BOOL, "disable effect physics");
idCVar bse_debris("bse_debris", "1", CVAR_BOOL, "disable effect debris");
idCVar bse_singleEffect("bse_singleEffect", "", 0, "set to the name of the effect that is only played");
idCVar bse_rateLimit("bse_rateLimit", "1", CVAR_FLOAT, "rate limit for spawned effects");
idCVar bse_rateCost("bse_rateCost", "1", CVAR_FLOAT, "rate cost multiplier for spawned effects");
#endif

float effectCosts[EC_MAX] = { 0, 2, 0.1 }; // dd 0.0, 2 dup(0.1)

#ifdef _RAVEN_FX
idBlockAlloc<rvBSE, 256/*, 0 //k*/>	rvBSEManagerLocal::effects;
#endif

// rvBSEManagerLocal
/*
====================
vBSEManagerLocal::EffectDuration
====================
*/
float rvBSEManagerLocal::EffectDuration(const rvRenderEffectLocal* def)
{
	return 0.0f;
}

/*
====================
rvBSEManagerLocal::CanPlayRateLimited
====================
*/
bool rvBSEManagerLocal::CanPlayRateLimited(effectCategory_t category)
{
	return false;
}

/*
====================
rvBSEManagerLocal::Init
====================
*/
bool rvBSEManagerLocal::Init(void) {
	common->Printf("----------------- BSE Init ------------------\n");

	return true;
}

/*
====================
rvBSEManagerLocal::Filtered
====================
*/
bool rvBSEManagerLocal::Filtered(const char* name, effectCategory_t category)
{
	return true;
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
	return false;
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
rvBSEManagerLocal::StopEffect
=======================
*/
bool rvBSEManagerLocal::ServiceEffect(class rvRenderEffectLocal* def, float time, bool& forcePush)
{
	return false;
}

/*
=======================
rvBSEManagerLocal::StopEffect
=======================
*/
void rvBSEManagerLocal::StopEffect(rvRenderEffectLocal* def) {
}

/*
=======================
rvBSEManagerLocal::FreeEffect
=======================
*/
void rvBSEManagerLocal::FreeEffect(rvRenderEffectLocal* def)
{
}

bool rvBSEManagerLocal::CheckDefForSound(const renderEffect_t* def) {
	return true;
}

void rvBSEManagerLocal::BeginLevelLoad(void) {

}



// rvDeclEffect
bool rvDeclEffect::SetDefaultText()
{
	char generated[1024]; // [esp+4h] [ebp-404h]

	idStr::snPrintf(generated, sizeof(generated), "effect %s // IMPLICITLY GENERATED\n");
	SetText(generated);
	return false;
}

size_t rvDeclEffect::Size(void) const {
	return sizeof(rvDeclEffect);
}

void rvDeclEffect::FreeData()
{
}

const char* rvDeclEffect::DefaultDefinition() const
{
	return "{\n}\n";
}

bool rvDeclEffect::Parse(const char* text, const int textLength) {
	return true;
}
