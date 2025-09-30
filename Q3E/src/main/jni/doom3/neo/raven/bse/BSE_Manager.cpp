/*
===========================================================================

QUAKE 4 BSE CODE RECREATION EFFORT - (c) 2025 by Justin Marshall(IceColdDuke).

QUAKE 4 BSE CODE RECREATION EFFORT is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QUAKE 4 BSE CODE RECREATION EFFORT is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QUAKE 4 BSE CODE RECREATION EFFORT.  If not, see <http://www.gnu.org/licenses/>.

In addition, the QUAKE 4 BSE CODE RECREATION EFFORT is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "BSE.h"

rvBSEManagerLocal bseLocal;
rvBSEManager* bse = &bseLocal;

static idCVar _g_decals(              "g_decals",                                       "1",         CVAR_GAME | PC_CVAR_ARCHIVE | CVAR_BOOL, "show decals such as bullet holes" );
idMat3  rvBSEManagerLocal::mModelToBSE;
float rvBSEManagerLocal::mEffectRates[EC_MAX];
float rvBSEManagerLocal::effectCosts[EC_MAX];
unsigned int rvBSEManagerLocal::mPerfCounters[NUM_PERF_COUNTERS];
idCVar * rvBSEManagerLocal::g_decals = &_g_decals;

const char* rvBSEManagerLocal::mSegmentNames[SEG_COUNT] = {
        "NONE",
        "EFFECT",
        "EMITTER",
        "SPAWNER",
        "TRAIL",
        "SOUND",
        "DECAL",
        "LIGHT",
        "DELAY",
        "DV",
        "SHAKE",
        "TUNNEL",
};

static const char	*bse_singleSegmentTypeArgs[]	= {
        "",
        "effect",
        "emitter",
        "spawner",
        "trail",
        "sound",
        "decal",
        "light",
        "doublevision",
        "shake",
        "tunnel",

        NULL
};

static const char	*bse_singleParticleTypeArgs[]	= {
        "",
        "sprite",
        "line",
        "oriented",
        "decal",
        "model",
        "light",
        "electric",
        "linked",
        "debris",
        "sound",
        "particle",

        NULL
};

namespace BSE
{
    static const char *SEGMENT_TYPE_NAMES[] = {
            "unknown", // 0
            "effect", // 1
            "emitter", // 2
            "spawner", // 3
            "trail", // 4
            "sound", // 5
            "decal", // 6
            "light", // 7
            "delay", // 8
            "doubleVision", // 9
            "shake", // 10
            "tunnel", // 11
    };

    const char * SegmentTypeName(int segType)
    {
        if(segType >= 0 && segType < SEG_COUNT)
            return SEGMENT_TYPE_NAMES[segType];
        else
            return "invalid";
    }

    static const char *PARTICLE_TYPE_NAMES[] = {
            "unknown", // 0
            "sprite", // 1
            "line", // 2
            "oriented", // 3
            "decal", // 4
            "model", // 5
            "light", // 6
            "electric", // 7
            "linked", // 8
            "debris", // 9
            "sound", // 10
    };

    const char * ParticleTypeName(int pType)
    {
        if(pType >= 0 && pType < PTYPE_COUNT)
            return PARTICLE_TYPE_NAMES[pType];
        else
            return "invalid";
    }
};

idCVar bse_render("bse_render", "1", CVAR_BOOL, "disable effect rendering");
idCVar bse_enabled("bse_enabled", "1", CVAR_BOOL | CVAR_ARCHIVE, "set to false to disable all effects");
idCVar bse_debug("bse_debug", "0", CVAR_INTEGER, "display debug info about effect");
idCVar bse_singleEffect("bse_singleEffect", "", 0, "set to the name of the effect that is only played", idCmdSystem::ArgCompletion_Decl<DECL_EFFECT>);
idCVar bse_rateLimit("bse_rateLimit", "1", CVAR_FLOAT, "rate limit for spawned effects");
idCVar bse_showBounds("bse_showbounds", "0", CVAR_BOOL, "display debug bounding boxes effect");
idCVar bse_physics("bse_physics", "1", CVAR_BOOL, "disable effect physics");
idCVar bse_debris("bse_debris", "1", CVAR_BOOL, "disable effect debris");
idCVar bse_speeds("bse_speeds", "0", CVAR_INTEGER, "print bse frame statistics");
idCVar bse_rateCost("bse_rateCost", "1", CVAR_FLOAT, "rate cost multiplier for spawned effects");
idCVar bse_scale("bse_scale", "1.2", CVAR_FLOAT, "effect scalability amount");
idCVar bse_maxParticles("bse_maxParticles", "2048", CVAR_INTEGER, "max number of particles allowed per segment");

#if BSE_DEV
//karin: render single segment/particle type effect
idCVar bse_singleSegmentType("bse_singleSegmentType", "", 0, "set to the name of the segment that is only played", idCmdSystem::ArgCompletion_String<bse_singleSegmentTypeArgs>);
idCVar bse_singleParticleType("bse_singleParticleType", "", 0, "set to the name of the segment particle that is only played", idCmdSystem::ArgCompletion_String<bse_singleParticleTypeArgs>);
idCVar bse_log("bse_log", "0", CVAR_INTEGER, "display developer debug log about effect");
#endif

// ──────────────────────────────────────────────────────────────────────────────
//  Implementation
// ──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::Init()
{
    common->Printf("----------------- BSE Init ------------------\n");

    // warm-up default resources so play-time is hitch-free
    declManager->FindEffect("_default", true);
    declManager->FindMaterial("_default", true);
    declManager->FindMaterial("gfx/effects/particles_shapes/motionblur", true);
    declManager->FindType(DECL_TABLE, "halfsintable", true, false);
    renderModelManager->FindModel("_default");

	//karin: null if game dll not loaded(it should not happen)
	idCVar *cvar_g_decals = cvarSystem->Find("g_decals");
	if(cvar_g_decals)
		g_decals = cvar_g_decals;

	for(int i = 0; i < sizeof(mEffectRates) / sizeof(mEffectRates[0]); i++)
		mEffectRates[i] = 0.0f;

    // register console commands
    cmdSystem->AddCommand("bseStats", &rvBSEManagerLocal::BSE_Stats_f,
        NULL, "Dumps the stats of every registered effect - use all to force parse every effect");
    cmdSystem->AddCommand("bseLog", &rvBSEManagerLocal::BSE_Log_f,
        NULL, "Dumps the number of times an effect has been played since game start");

    common->Printf("--------- BSE Created Successfully ----------\n");
    return true;
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::Shutdown()
{
    common->Printf("--------------- BSE Shutdown ----------------\n");

    for (int i = 0; i < traceModels.Num(); i++)
    {
        delete traceModels[i];
    }
    traceModels.Clear();    

	for(int i = 0; i < sizeof(mEffectRates) / sizeof(mEffectRates[0]); i++)
		mEffectRates[i] = 0.0f;

    common->Printf("---------------------------------------------\n");
    return true;
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::EndLevelLoad()
{
    common->Printf("----- rvBSEManagerLocal::EndLevelLoad -----\n");
	for(int i = 0; i < sizeof(mEffectRates) / sizeof(mEffectRates[0]); i++)
		mEffectRates[i] = 0.0f;
   // effectCredits.fill(0.0f);
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::StartFrame()
{
  if ( DebugHudActive() )
  {
	  for(int i = 0; i < sizeof(mPerfCounters) / sizeof(mPerfCounters[0]); i++)
		  mPerfCounters[i] = 0;
	  /*
    dword_1137DDAC = 0;
    dword_1137DDB0 = 0;
    dword_1137DDB4 = 0;
    dword_1137DDB8 = 0;
	*/
  }
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::EndFrame()
{
	if ( DebugHudActive() )
	{
		game->DebugSetInt("fx_num_active", mPerfCounters[PERF_NUM_BSE]);
		game->DebugSetInt("fx_num_particles", mPerfCounters[PERF_NUM_PARTICLES]/* dword_1137DDB0 */);
		game->DebugSetInt("fx_num_traces", mPerfCounters[PERF_NUM_TRACES]/* dword_1137DDAC */);
		float v1 = mPerfCounters[PERF_NUM_TEXELS]/* dword_1137DDB4 */ / (float)(1 << 20)/*k??? TODO Q4D * 0.00000095367432 */;
		game->DebugSetFloat("fx_num_texels", v1);
		game->DebugSetInt("fx_num_segments", mPerfCounters[PERF_NUM_SEGMENTS]/* dword_1137DDB8 */);
	}

    //game->DebugSetFloat("fx_num_texels",
    //    static_cast<float>(perfCounters_[3]) / (1 << 20)); // 2^20 == 1 048 576
    //game->DebugSetInt("fx_num_segments", perfCounters_[4]);
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::UpdateRateTimes()
{
	for(int i = EC_IGNORE; i < EC_MAX; i++)
	{
		float v1 = mEffectRates[i] - 0.1f;
		mEffectRates[i] = v1;
		if(v1 < 0.0f)
			mEffectRates[i] = 0.0f;
	}
//    for (float& credit : effectCredits_)
//        credit = max(0.0f, credit - kDecayPerFrame);
}
//──────────────────────────────────────────────────────────────────────────────
float rvBSEManagerLocal::EffectDuration(const rvRenderEffectLocal* def)
{
    return (def && def->index >= 0 && def->effect)
        ? def->effect->mDuration
        : 0.0f;
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::CheckDefForSound(const renderEffect_t* def)
{
    rvDeclEffect* decl = (rvDeclEffect*)def->declEffect;    
    return (decl->mFlags & ETFLAG_HAS_SOUND/* 1u */) != 0;
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::SetDoubleVisionParms(float t, float s)
{
    game->StartViewEffect(0, t, s);
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::SetShakeParms(float t, float s)
{
    game->StartViewEffect(1, t, s);
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::SetTunnelParms(float t, float s)
{
    game->StartViewEffect(2, t, s);
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::Filtered(const char* name, effectCategory_t cat)
{
    const char* filter = bse_singleEffect.GetString();        // CVAR helper
    const bool  filterEmpty = (filter[0] == '\0');

    const bool namePasses = filterEmpty || (strstr(name, filter) != NULL);
    const bool ratePasses = CanPlayRateLimited(cat);

    return !(namePasses && ratePasses);
    //return false;
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::CanPlayRateLimited(effectCategory_t c)
{
    if (c == EC_IGNORE || bse_rateLimit.GetFloat() <= 0.1f)
        return true;
    
	float &bucket = mEffectRates[c];
    const float cost = effectCosts[c] * bse_rateCost.GetFloat();
    const float limit = bse_rateLimit.GetFloat();
    
    // simple leaky-bucket: if >50 % full and the random test fails, refuse
    if (limit * 0.5f < bucket &&
        cost + bucket > rvRandom::flrand(0.0f, limit))
        return false;
    
    bucket += cost;
    return true;
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::StopEffect(rvRenderEffectLocal* def)
{
    if (!def || def->index < 0 || !def->effect) return;

    if (bse_debug.GetBool())
        common->Printf("BSE: Stop: %s\n",
            def->parms.declEffect->GetName());

    def->effect->mFlags |= F_STOP_REQUESTED;
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::FreeEffect(rvRenderEffectLocal* def)
{
    if (!def || def->index < 0 || !def->effect) return;

    if (bse_debug.GetBool())
        common->Printf("BSE: Free: %s\n",
            def->parms.declEffect->GetName());

    def->effect->Destroy();                     // returns memory to pool
    effects_.Free(def->effect);
    def->effect = NULL;
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::ServiceEffect(rvRenderEffectLocal* def, float now)
{
    if (!def || !def->effect) return true;            // nothing to do → finished

// jmarshall
    if (Filtered(def->parms.declEffect->GetName()))
        return true;                                   // filtered out – treat as finished

    bool ret = def->effect->Service(&def->parms, now);
	BSE_LOGFI(ServiceEffect, "%s: -> %d, referenceBounds=(%s | %s), mCurrentLocalBounds=(%s | %s)", def->effect->mDeclEffect->GetName(), ret, def->referenceBounds[0].ToString(), def->referenceBounds[1].ToString(), def->effect->mCurrentLocalBounds[0].ToString(), def->effect->mCurrentLocalBounds[1].ToString())
	if(ret)
        return true;                                   // still alive

    // effect finished – copy its final bounds for spatial culling
    def->referenceBounds = def->effect->mCurrentLocalBounds;

    if (DebugHudActive())
        ++mPerfCounters[PERF_NUM_BSE];

    if (/*common->IsMultiplayer() || */bse_debug.GetBool())
        def->effect->EvaluateCost(-1);

    return false;                                      // tell caller to destroy def
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::PlayEffect(rvRenderEffectLocal* def, float now)
{
    rvDeclEffect* decl = (rvDeclEffect *)def->parms.declEffect;

    if (Filtered(decl->GetName()))
        return false;

    if (bse_debug.GetBool())
	{
		static unsigned int count = 0;
        common->Printf("BSE: Play %d: %s at %g\n",
            ++count, decl->GetName(), now);
	}

    ++decl->mPlayCount;

    def->effect = effects_.Alloc();
    def->effect->Init(decl, &def->parms, now);
    return true;
}
//──────────────────────────────────────────────────────────────────────────────
int rvBSEManagerLocal::AddTraceModel(idTraceModel* m)
{
    traceModels.Append(m);
    return traceModels.Num();
}
idTraceModel* rvBSEManagerLocal::GetTraceModel(int idx)
{
    return traceModels[idx];
}
void rvBSEManagerLocal::FreeTraceModel(int idx)
{
    if (idx < 0 || idx >= traceModels.Num()) 
        return;
    delete traceModels[idx];
    traceModels[idx] = NULL;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Console command glue
// ──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::BSE_Stats_f(const idCmdArgs& args)
{
    /*  The original Hex-Rays dump walks the whole declManager list, counts segments,
        reports average particles, etc.  That code was extremely ugly and
        completely tied to idTech internals.

        Re-implementing it *verbatim* adds no instructional value, so here we only
        keep the behaviour (same console text) while writing it clearly.         */

    const bool forceParseAll = (args.Argc() > 1 && !idStr::Icmp(args.Argv(1), "all"));
   
   const int numDecls = declManager->GetNumDecls(DECL_EFFECT);
   
   common->Printf("... processing %d registered effects\n", numDecls);
   
   int loaded = 0;
   int neverReferenced = 0;
   int segments = 0;
   int segmentsWithParticles = 0;
   int particlesTotal = 0;
   
   for (int i = 0; i < numDecls; ++i)
   {
       const rvDeclEffect* effect = declManager->EffectByIndex(i, forceParseAll);
	   if(!effect)
		   continue;

       if (effect->GetState() == DS_PARSED)
	   {
		   ++loaded;
		   const int segCount = effect->mSegmentTemplates.Num();
		   segments += segCount;

		   for (int s = 0; s < segCount; ++s)
		   {
			   const rvSegmentTemplate* seg = effect->GetSegmentTemplate(s);
			   const bool hasParticles = seg->mFlags & STFLAG_HASPARTICLES; //4
			   if (hasParticles)
			   {
				   ++segmentsWithParticles;
				   particlesTotal += static_cast<int>(seg->mCount.y);
			   }
		   }
	   }
#if 0
	   else if ( !effect->EverReferenced() )
	   {
		   ++haveParts;
	   }
#endif
   }
   
   common->Printf("%d segments in %d loaded effects (%d never referenced)\n",
       segments, loaded, neverReferenced);
   
   //if (loaded > 0)
   {
       common->Printf("%.2f segments per effect\n",
           loaded > 0 ? static_cast<float>(segments) / (float)loaded : 0.0f);
   }
   //if (segments > 0)
   {
       common->Printf("%.2f of segments have particles\n",
           segments > 0 ? static_cast<float>(segmentsWithParticles) / (float)segments : 0.0f);
       common->Printf("%.2f particles per segment with particles\n",
           segmentsWithParticles > 0 ? static_cast<float>(particlesTotal) / (float)segmentsWithParticles : 0.0f);
   }
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::BSE_Log_f(const idCmdArgs& /*args*/)
{
    const int numDecls = declManager->GetNumDecls(DECL_EFFECT);

    common->Printf("Processing %d effect decls...\n", numDecls);

    int playedOrLooped = 0;

    for (int i = 0; i < numDecls; ++i)
    {
        const rvDeclEffect* e = declManager->EffectByIndex(i, false);
        if (!e) continue;

        const int plays = e->mPlayCount;
        const int loops = e->mLoopCount;

        if (plays || loops)
        {
            common->Printf("%d plays (%d loops): '%s'\n",
                plays, loops, e->GetName());
            ++playedOrLooped;
        }
    }

    common->Printf("%d effects played or looped out of %d\n",
        playedOrLooped, numDecls);
}
//──────────────────────────────────────────────────────────────────────────────
