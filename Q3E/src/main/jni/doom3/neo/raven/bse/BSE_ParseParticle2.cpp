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

#include "BSE_Compat.h"

// ---------------------------------------------------------------------
//  Helper to reset rvParticleParms blocks
// ---------------------------------------------------------------------
ID_INLINE static void InitParms(rvParticleParms& p, int spawnType)
{
    p.mSpawnType = spawnType;
    p.mFlags = 0;
    p.mRange = 0.0f;
    p.mMisc = NULL;
    p.mMins = vec3_origin;
    p.mMaxs = vec3_origin;
}

//----------------------------------------------------------
//  Helper for shape keyword errors (minimises boilerplate)
//----------------------------------------------------------
ID_INLINE static void WarnBad(rvDeclEffect* effect, idLexer* src, const char* what)
{
    common->Warning("^4BSE:^1 Invalid %s parameter in '%s' "
                    "(file: %s, line: %d)",
                    what, effect->GetName(),
                    src->GetFileName(), src->GetLineNum());
}

static void SetDefaultEnv(rvEnvParms& env) { env.SetDefaultType(); }

/* ------------------------------------------------------------
   Spawn domains (vector compare helpers already exist)
   ------------------------------------------------------------ */

ID_INLINE static int C(float f) { return static_cast<int>(f * 255.0f); }
ID_INLINE static bool TintEqual(const rvParticleParms& a,
                    const rvParticleParms& b)
{
    return C(a.mMins.x) == C(b.mMins.x) && C(a.mMins.y) == C(b.mMins.y) &&
           C(a.mMins.z) == C(b.mMins.z) && C(a.mMaxs.x) == C(b.mMaxs.x) &&
           C(a.mMaxs.y) == C(b.mMaxs.y) && C(a.mMaxs.z) == C(b.mMaxs.z) &&
           a.mSpawnType == b.mSpawnType && a.mFlags == b.mFlags;
};

/* ------------------------------------------------------------
   Death domains are compared only when the envelope is active
   ------------------------------------------------------------ */
ID_INLINE static bool DeathNeeds(const rvEnvParms& e) { return e.GetType() > 0; };


/*
===============================================================================

    Lifetime helpers
    (small ones are placed first so they are easy to inline)

===============================================================================
*/
bool rvParticleTemplate::UsesEndOrigin(void) const {
    // Uses the line segment “origin -> endOrigin” for spawn / length domains?
    return (mSpawnPosition.mFlags & PPFLAG_USEENDORIGIN/* 2 */) != 0 ||
        (mSpawnLength.mFlags & PPFLAG_USEENDORIGIN/* 2 */) != 0;
}

/*
===============================================================================

    Parameter-count derivation  ( size / rotate envelopes )

    mType table
    ---------------------------------------------------------
      0 = sprite/quad    1 = beam          2 = billboard
      3 = axis-aligned   4 = ribbon        5 = ribbon-extruded
      6 = mesh-copy      7 = mesh-random   8 = decal
      9 = point-light “glare” helper
===============================================================================
*/
void rvParticleTemplate::SetParameterCounts(void) {
    int sizeParms = 0;
    int rotateParms = 0;
    int sizeSpawn = 7;   // default = “vector” envelope
    /*---------------------------------------------------------------------
        The original x86 code used two packed bit-fields:
            lower 2 bits – dimension count (0,1,2,3 axes)
            upper bits   – shape variants (point, unit, scaled, etc.)
    ---------------------------------------------------------------------*/
    switch (mType) {
	case PTYPE_SPRITE: // 1:                     // beam
	case PTYPE_DECAL: // 4:                     // ribbon
        sizeParms = 2;
        rotateParms = 1;
        sizeSpawn = 6;        // two-component envelope
        break;

	case PTYPE_LINE: // 2:                     // billboard
	case PTYPE_ELECTRIC: // 7:                     // mesh-random
	case PTYPE_LINKED: // 8:                     // decal
        sizeParms = 1;
        rotateParms = 0;
        sizeSpawn = 5;        // one-component envelope
        break;

	case PTYPE_ORIENTED: // 3:                     // axis-aligned (cone/cylinder)
        sizeParms = 2;
        rotateParms = 3;
        sizeSpawn = 6;
        break;

	case PTYPE_MODEL: // 5:                     // ribbon-extruded
        sizeParms = 3;
        rotateParms = 3;
        sizeSpawn = 7;
        break;

	case PTYPE_LIGHT: // 6:                     // mesh-copy
        sizeParms = 3;
        rotateParms = 0;
        sizeSpawn = 7;
        break;

	case PTYPE_DEBRIS: // 9:                     // glare helper
        sizeParms = 0;
        rotateParms = 3;
        sizeSpawn = 7;
        break;

    default:                    // sprites (0) – counts already correct
        return;
    }

    mNumSizeParms = sizeParms;
    mNumRotateParms = rotateParms;

    // Preserve editor intent – spawn/death domains must carry the same
    // “shape” meta-information the envelope expects.
    mSpawnSize.mSpawnType = sizeSpawn;
    mDeathSize.mSpawnType = sizeSpawn;
    mSpawnRotate.mSpawnType = rotateParms;
    mDeathRotate.mSpawnType = rotateParms;
}

/*
===============================================================================

    Runtime helpers
===============================================================================
*/
float rvParticleTemplate::GetSpawnVolume(rvBSE* fx) const {
    // Compute the effective diagonal extents ( ≈ bounding-box diagonal ).
    float xExtent;

    if (mSpawnPosition.mFlags & PPFLAG_USEENDORIGIN /* 2 END_ORIGIN */) {
        idVec3 delta = fx->mOriginalEndOrigin - fx->mOriginalOrigin;
        xExtent = delta.Length() - mSpawnPosition.mMins.x;
    }
    else {
        xExtent = mSpawnPosition.mMaxs.x - mSpawnPosition.mMins.x;
    }

    const float yExtent = mSpawnPosition.mMaxs.y - mSpawnPosition.mMins.y;
    const float zExtent = mSpawnPosition.mMaxs.z - mSpawnPosition.mMins.z;

    // Historical: the 0.01 factor dates back to the old “cm” → “m” scale.
    return (xExtent + yExtent + zExtent) * 0.01f;
}

// --------------------------------------------------------------------------

float rvParticleTemplate::CostTrail(float baseCost) const {
    switch (mTrailType) {
    case TRAIL_BURN: // 1: // burn
        return baseCost * mTrailCount.y * 2.0f;

	case TRAIL_MOTION: // 2: // motion blur
        return baseCost * mTrailCount.y * 1.5f + 20.0f;

    default:
        return baseCost;
    }
}

/*
===============================================================================

    Parameter sanitiser
    (rewrites spawnType so the renderer can make fast decisions)

===============================================================================
*/
void rvParticleTemplate::FixupParms(rvParticleParms& p) {
    const int axisBits = p.mSpawnType & 3;      // 0..3
    const int shapeBits = (p.mSpawnType & ~3);    // 0,4,8,(...)  etc.

    // Explicit editor values “point” (0), “unit” (4) and the two tapered
    // cone/cylinder variants (43,47) are left untouched.
    if (shapeBits == 0 || shapeBits == 4 ||
        p.mSpawnType == 43 || p.mSpawnType == 47) {
        return;
    }

    // ---------------------------------------------------------------------
    //  Detect degenerate cases where mins == maxs on the active axes.
    //  These collapse to either POINT / UNIT / SCALE depending on size.
    // ---------------------------------------------------------------------
    const idVec3& mins = p.mMins;
    const idVec3& maxs = p.mMaxs;

    const bool equalX = (maxs.x == mins.x);
    const bool equalY = (axisBits < 2) || (maxs.y == mins.x);
    const bool equalZ = (axisBits != 3) || (maxs.z == mins.x);
	//karin: TODO Q4D
    const bool equalY_min = (axisBits < 2) || (mins.y == mins.x);
    const bool equalZ_min = (axisBits != 3) || (mins.z == mins.x);

    if (equalY_min && equalZ_min && (shapeBits == 8 && equalX && equalY && equalZ)) {      // a “box”
        if (mins.x == 0.0f) {
            p.mSpawnType = axisBits;           // true POINT
        }
        else if (idMath::Fabs(mins.x - 1.0f) < idMath::FLT_EPSILON) {
            p.mSpawnType = axisBits + 4;       // UNIT-sized
        }
        else {
            p.mSpawnType = axisBits + 8;       // general SCALE
        }
    }
    else if (shapeBits == 8) {
        // Mixed extents – still a SCALE box, but make sure the type’s
        // “shape” bits say so even when the editor didn’t.
        p.mSpawnType = axisBits + 8;
    }

    // ---------------------------------------------------------------------
    //  Remove unused components ( renderer relies on 0 to skip work )
    // ---------------------------------------------------------------------
    if (p.mSpawnType >= 8) {
        if (axisBits == 1) {                 // X/Y only
            p.mMins.y = p.mMaxs.y = 0.0f;
        }
        else if (axisBits == 2) {          // X/Z only
            p.mMins.z = p.mMaxs.z = 0.0f;
        }
    }
    else {                                   // pure POINT / UNIT
        p.mMins = vec3_origin;
        p.mMaxs = vec3_origin;
    }

    // Enforce maxs == mins for POINT / UNIT / SCALE
    if (p.mSpawnType <= 11) {
        p.mMaxs = p.mMins;
    }

    // If this parameter references endOrigin, upgrade it to the
    // corresponding “vector” (12-15) shape so the renderer knows it varies.
    if ((p.mFlags & PPFLAG_USEENDORIGIN/* 2 */) && p.mSpawnType <= 12) {
        p.mSpawnType = axisBits + 12;
    }
}

/*
===============================================================================

    Master initialiser  (called by the default ctor)

===============================================================================
*/
void rvParticleTemplate::Init(void) {
    // ---------------------------------------------------------------------
    //  Basic meta
    // ---------------------------------------------------------------------
    mFlags = 0;
    mType = 0;

    mMaterialName = "_default";
    mMaterial = declManager->FindMaterial("_default", false);

    mModelName = "_default";
    mTraceModelIndex = -1;

    mGravity.Zero();
    mSoundVolume.Zero();
    mFreqShift.Zero();
    mDuration.Set(0.002f, 0.002f);

    mBounce = 0.0f;
    mTiling = 8.0f;

    // ---------------------------------------------------------------------
    //  Trail parameters
    // ---------------------------------------------------------------------
    mTrailType = 0;
    mTrailMaterial = declManager->FindMaterial(
        "gfx/effects/particles_shapes/motionblur", false);
    mTrailTime.Zero();
    mTrailCount.Zero();

    // ---------------------------------------------------------------------
    //  Fork / jitter
    // ---------------------------------------------------------------------
    mNumForks = 0;
    mForkSizeMins.Set(-20, -20, -20);
    mForkSizeMaxs = -mForkSizeMins;
    mJitterSize.Set(3, 7, 7);
    mJitterRate = 0.0f;
    mJitterTable = static_cast<const idDeclTable*>(
        declManager->FindType(DECL_TABLE,
            "halfsintable",
            false));

    // ---------------------------------------------------------------------
    //  Derived constants
    // ---------------------------------------------------------------------
    mNumSizeParms = 2;
    mNumRotateParms = 1;
    mVertexCount = 4;
    mIndexCount = 6;
    mCentre = vec3_origin;

    // ---------------------------------------------------------------------
    //  Helper to reset rvParticleParms blocks
    // ---------------------------------------------------------------------

    // ---------------------------------------------------------------------
    //  Spawn domains
    // ---------------------------------------------------------------------
    InitParms(mSpawnPosition, 3);
    InitParms(mSpawnDirection, 3);
    InitParms(mSpawnVelocity, 3);
    InitParms(mSpawnAcceleration, 3);
    InitParms(mSpawnFriction, 3);
    InitParms(mSpawnTint, 7);
    InitParms(mSpawnFade, 5);
    InitParms(mSpawnSize, 7);
    InitParms(mSpawnRotate, 3);
    InitParms(mSpawnAngle, 3);
    InitParms(mSpawnOffset, 3);
    InitParms(mSpawnLength, 3);

    // ---------------------------------------------------------------------
    //  Per-frame envelopes
    // ---------------------------------------------------------------------
    mTintEnvelope.Init();
    mFadeEnvelope.Init();
    mSizeEnvelope.Init();
    mRotateEnvelope.Init();
    mAngleEnvelope.Init();
    mOffsetEnvelope.Init();
    mLengthEnvelope.Init();

    // ---------------------------------------------------------------------
    //  Death domains
    // ---------------------------------------------------------------------
    InitParms(mDeathTint, 3);
    InitParms(mDeathFade, 1);
    InitParms(mDeathSize, 7);
    InitParms(mDeathRotate, 3);
    InitParms(mDeathAngle, 3);
    InitParms(mDeathOffset, 3);
    InitParms(mDeathLength, 3);

    // ---------------------------------------------------------------------
    //  Impact / timeout effects
    // ---------------------------------------------------------------------
    mNumImpactEffects = 0;
    mNumTimeoutEffects = 0;
    memset(mImpactEffects, 0, sizeof(mImpactEffects));
    memset(mTimeoutEffects, 0, sizeof(mTimeoutEffects));
}

/*
===============================================================================

    Misc query helpers

===============================================================================
*/
idTraceModel* rvParticleTemplate::GetTraceModel(void) const {
    return (mTraceModelIndex >= 0)
        ? bseLocal.traceModels[mTraceModelIndex]
        : NULL;
}

// --------------------------------------------------------------------------

int rvParticleTemplate::GetTrailCount(void) const {
    const int count = static_cast<int>(rvRandom::flrand(
        mTrailCount.x,
        mTrailCount.y));
    return count < 0 ? 0 : count;
}

static inline void UpdateExtents(const idVec3& p, idVec3& minE, idVec3& maxE) {
    minE.x = Min(minE.x, p.x);
    minE.y = Min(minE.y, p.y);
    minE.z = Min(minE.z, p.z);
    maxE.x = Max(maxE.x, p.x);
    maxE.y = Max(maxE.y, p.y);
    maxE.z = Max(maxE.z, p.z);
}

void rvParticleTemplate::EvaluateSimplePosition(
    idVec3* pos,
    float time,
    float lifeTime,
    const idVec3* initPos,
    const idVec3* velocity,
    const idVec3* acceleration,
    const idVec3* friction
) const {
    // 1) Linear motion + constant acceleration term: x = x₀ + v·t + ½·a·t²
    float t = time;
    float t2 = t * t;
    float halfT2 = 0.5f * t2;

    pos->x = initPos->x + velocity->x * t + acceleration->x * halfT2;
    pos->y = initPos->y + velocity->y * t + acceleration->y * halfT2;
    pos->z = initPos->z + velocity->z * t + acceleration->z * halfT2;

    // 2) Friction/damping integration term:
    //    Derived from original: v12 = halfT2 * ((2^v9 - 1) * halfT2) / 3
    //    where 2^v9 == exp((lifeTime - halfT2)/lifeTime)
    float expFactor = exp((lifeTime - halfT2) / lifeTime) - 1.0f;
    float frictionScalar = (halfT2 * halfT2 * expFactor) / 3.0f;

    pos->x += friction->x * frictionScalar;
    pos->y += friction->y * frictionScalar;
    pos->z += friction->z * frictionScalar;
}

float rvParticleTemplate::GetFurthestDistance() const {
    // 1) Gather min/max for each spawn parameter
    idVec3 minPos, maxPos;
    mSpawnPosition.GetMinsMaxs(minPos, maxPos);

    idVec3 minVel, maxVel;
    mSpawnVelocity.GetMinsMaxs(minVel, maxVel);

    idVec3 minAccel, maxAccel;
    mSpawnAcceleration.GetMinsMaxs(minAccel, maxAccel);

    idVec3 minFric, maxFric;
    mSpawnFriction.GetMinsMaxs(minFric, maxFric);

    // 2) Choose appropriate gravity
    float grav = game->IsMultiplayer()
        ? cvarSystem->GetCVarFloat("g_mp_gravity")
        : cvarSystem->GetCVarFloat("g_gravity");

    // 3) Build gravity offset vector and scale by template gravity.x
    idVec3 gravVec(0.0f, 0.0f, -grav);
    float gx = mGravity.x;
    gravVec.x *= gx;
    gravVec.y *= gx;
    gravVec.z *= gx;

    // 4) Apply gravity offset to acceleration bounds
    minAccel.x -= gravVec.x;  minAccel.y -= gravVec.y;  minAccel.z -= gravVec.z;
#if 0 //k??? TODO Q4D is it
	gravVec.x *= mGravity.y;
	gravVec.y *= mGravity.y;
	gravVec.z *= mGravity.y;
    maxAccel.x += gravVec.x;  maxAccel.y += gravVec.y;  maxAccel.z += gravVec.z;
#else
    maxAccel.x -= gravVec.x;  maxAccel.y -= gravVec.y;  maxAccel.z -= gravVec.z;
#endif

    // 5) Prepare overall min/max trackers
#if 1 //k??? TODO using inf
#define ID_FLT_MAX idMath::INFINITY
#else
#define ID_FLT_MAX          3.402823466e+38F        // max value
#endif
    idVec3 overallMin(ID_FLT_MAX, ID_FLT_MAX, ID_FLT_MAX);
    idVec3 overallMax(-ID_FLT_MAX, -ID_FLT_MAX, -ID_FLT_MAX);
#undef ID_FLT_MAX

    // 6) Sample 8 time steps and all 16 combinations of parameter extremes
    const float duration = mDuration.y;
    const float step = duration * 0.125f;  // eighths
    idVec3 pos;

    for (int i = 0; i < 8; ++i) {
        float t = i * step;
        for (int p = 0; p < 2; ++p) {
            const idVec3& initP = (p ? maxPos : minPos);
            for (int v = 0; v < 2; ++v) {
                const idVec3& vel = (v ? maxVel : minVel);
                for (int a = 0; a < 2; ++a) {
                    const idVec3& acc = (a ? maxAccel : minAccel);
                    for (int f = 0; f < 2; ++f) {
                        const idVec3& fr = (f ? maxFric : minFric);

                        // Evaluate position for this combination
                        EvaluateSimplePosition(&pos, t, duration,
                            &initP, &vel, &acc, &fr);
                        UpdateExtents(pos, overallMin, overallMax);
                    }
                }
            }
        }
    }

    // 7) Compute half‐extents distances
    float distMin = overallMin.Length() * 0.5f;
    float distMax = overallMax.Length() * 0.5f;

    return (distMax > distMin) ? distMax : distMin;
}

/*
===============================================================================

    Static lexer helpers

===============================================================================
*/
bool rvParticleTemplate::GetVector(idLexer* src, int components,
    idVec3& out) {
    assert(components >= 1 && components <= 3);

    out.x = src->ParseFloat();

    if (components > 1) {
        if (!src->ExpectTokenString(","))
            return false;
        out.y = src->ParseFloat();

        if (components > 2) {
            if (!src->ExpectTokenString(","))
                return false;
            out.z = src->ParseFloat();
        }
    }
    return true;
}

/*
===============================================================================

    Motion-envelope parser   ( {   envelope <table>
                                   rate     <vec>
                                   count    <vec>
                                   offset   <vec>
                                 } )
===============================================================================
*/
bool rvParticleTemplate::ParseMotionParms(idLexer* src,
    int           vecCount,
    rvEnvParms& env)
{
    if (!src->ExpectTokenString("{"))
        return false;

	idStr tableName; //karin: record last parsed table name
    idToken tok;
    while (src->ReadToken(&tok)) {
        if (!idStr::Cmp(tok, "}"))
            break;

        if (!idStr::Icmp(tok, "envelope")) {

            src->ReadToken(&tok);
            env.mTable = static_cast<const idDeclTable*>(
                declManager->FindType(DECL_TABLE,
                    tok,
                    false));
            if(!env.mTable)
				tableName = tok.c_str(); //karin: read table name token        

        }
        else if (!idStr::Icmp(tok, "rate")) {

            if (!GetVector(src, vecCount, env.mRate))
                return false;
            env.mIsCount = false;
            
            tableName.Clear(); //karin: not read envelope any more
        }
        else if (!idStr::Icmp(tok, "count")) {

            if (!GetVector(src, vecCount, env.mRate))
                return false;
            env.mIsCount = true;

            tableName.Clear(); //karin: not read envelope any more
        }
        else if (!idStr::Icmp(tok, "offset")) {

            if (!GetVector(src, vecCount, env.mEnvOffset))
                return false;

            tableName.Clear(); //karin: not read envelope any more
        }
        else {
			if(!env.mTable && !tableName.IsEmpty()) //karin: only if table is invalid and envelope token readed
			{
				tableName.Append(tok); // append unexpected token as table name and refind it
				env.mTable = static_cast<const idDeclTable*>(
						declManager->FindType(DECL_TABLE,
							tableName,
							false));
	            if(env.mTable)
					tableName.Clear(); //karin: read table name token
			}
			else
			{
				common->Warning("^4BSE:^1 Invalid motion parameter '%s' "
						"(file: %s, line: %d)",
						tok.c_str(), src->GetFileName(), src->GetLineNum());
				src->SkipBracedSection(true);
			}
        }
    }

    return true;
}

// ==========================================================================
//  Motion-domain block
// ==========================================================================
bool rvParticleTemplate::ParseMotionDomains(rvDeclEffect* effect,
    idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        //--------------------------------------------------------------
        //  Dispatch to the right envelope; vector-count depends on type
        //--------------------------------------------------------------
        if (!idStr::Icmp(tok, "tint")) ParseMotionParms(src, 3, mTintEnvelope);
        else if (!idStr::Icmp(tok, "fade")) ParseMotionParms(src, 1, mFadeEnvelope);
        else if (!idStr::Icmp(tok, "size")) ParseMotionParms(src, mNumSizeParms,
            mSizeEnvelope);
        else if (!idStr::Icmp(tok, "rotate")) ParseMotionParms(src, mNumRotateParms,
            mRotateEnvelope);
        else if (!idStr::Icmp(tok, "angle")) ParseMotionParms(src, 3, mAngleEnvelope);
        else if (!idStr::Icmp(tok, "offset")) ParseMotionParms(src, 3, mOffsetEnvelope);
        else if (!idStr::Icmp(tok, "length")) ParseMotionParms(src, 3, mLengthEnvelope);
        else {
            // Unknown keyword – skip nested section so parsing can continue
            common->Warning("^4BSE:^1 Invalid motion domain '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(),
                effect->GetName(),
                src->GetFileName(),
                src->GetLineNum());
            src->SkipBracedSection(true);
        }
    }

    return true;
}

/*
========================
rvParticleTemplate::GetMaxParmValue
========================
*/
float rvParticleTemplate::GetMaxParmValue(const rvParticleParms* spawn, const rvParticleParms* death, const rvEnvParms* envelope) const {
    float     minScale, maxScale;   // <- what Hex-Rays called “min” and (wrongly) “spawn”
    idBounds  sBounds, dBounds;

    // Raw bounds for the particle at spawn
    spawn->GetMinsMaxs(sBounds[0], sBounds[1]);

    // Envelope supplies two scalar multipliers (usually 0–1)
    if (envelope->GetMinMax(minScale, maxScale))
    {
        // Scale the spawn bounds
        sBounds[0] *= minScale;
        sBounds[1] *= maxScale;

        // Bounds at death, scaled in the same way
        death->GetMinsMaxs(dBounds[0], dBounds[1]);
        dBounds[0] *= minScale;
        dBounds[1] *= maxScale;

        // Expand sBounds so it encloses dBounds
        for (int axis = 0; axis < 3; ++axis)
        {
            if (dBounds[0][axis] < sBounds[0][axis]) sBounds[0][axis] = dBounds[0][axis];
            if (dBounds[1][axis] > sBounds[1][axis]) sBounds[1][axis] = dBounds[1][axis];
        }
    }

    // Length of the two opposite corners from the origin
    float cornerMin = sBounds[0].Length();
    float cornerMax = sBounds[1].Length();

    return /*idMath::*/Max(cornerMin, cornerMax);
}

// ==========================================================================
//  Flag parsing shared by *every* spawn shape
// ==========================================================================
bool rvParticleTemplate::CheckCommonParms(idLexer* src,
    rvParticleParms& p)
{
    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        if (!idStr::Icmp(tok, "surface")) p.mFlags |= PPFLAG_SURFACE; // 0x01;
        else if (!idStr::Icmp(tok, "useEndOrigin")) p.mFlags |= PPFLAG_USEENDORIGIN; // 0x02;
        else if (!idStr::Icmp(tok, "cone")) p.mFlags |= PPFLAG_CONE; // 0x04;
        else if (!idStr::Icmp(tok, "relative")) p.mFlags |= PPFLAG_RELATIVE; // 0x08;
        else if (!idStr::Icmp(tok, "linearSpacing")) p.mFlags |= PPFLAG_LINEARSPACING; // 0x10;
        else if (!idStr::Icmp(tok, "attenuate")) p.mFlags |= PPFLAG_ATTENUATE; // 0x20;
        else if (!idStr::Icmp(tok, "inverseAttenuate")) p.mFlags |= PPFLAG_INV_ATTENUATE; // 0x40;
        else {
            common->Warning("^4BSE:^1 Unknown spawn flag '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), src->GetFileName(), src->GetLineNum());
        }
    }
    return true;        // nothing fatal here – bad keywords are soft-warnings
}


// ==========================================================================
//  Spawn-parameter parser  (point, line, box … model)
// ==========================================================================
bool rvParticleTemplate::ParseSpawnParms(rvDeclEffect* effect,
    idLexer* src,
    rvParticleParms& p,
    int            vecCount)
{
    //----------------------------------------------------------
    //  Opening brace + first keyword
    //----------------------------------------------------------
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    if (!src->ReadToken(&tok))
        return false;

    //======================================================================
    //          1.  POINT  (axis-aligned point or offset)
    //======================================================================
    if (!idStr::Icmp(tok, "point"))
    {
        p.mSpawnType = vecCount + 8;                           // 1-D/2-D/3-D
        GetVector(src, vecCount, p.mMins);                   // mins == point

        if (!CheckCommonParms(src, p))
            WarnBad(effect, src, "point");
    }

    //======================================================================
    //          2.  LINE  (pairs of points)
    //======================================================================
    else if (!idStr::Icmp(tok, "line"))
    {
        p.mSpawnType = vecCount + 12;
        GetVector(src, vecCount, p.mMins);
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);

        if (!CheckCommonParms(src, p))
            WarnBad(effect, src, "line");
    }

    //======================================================================
    //          3.  BOX  (AABB)
    //======================================================================
    else if (!idStr::Icmp(tok, "box"))
    {
        p.mSpawnType = vecCount + 16;
        GetVector(src, vecCount, p.mMins);
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);

        if (!CheckCommonParms(src, p))
            WarnBad(effect, src, "box");

        // If “surface” flag set – upgrade to “hollow box”
        if (p.mFlags & PPFLAG_SURFACE/* 0x01 */) {
            p.mSpawnType = vecCount + 20;
            FixupParms(p);
        }
    }

    //======================================================================
    //          4.  SPHERE
    //======================================================================
    else if (!idStr::Icmp(tok, "sphere"))
    {
        p.mSpawnType = vecCount + 24;
        GetVector(src, vecCount, p.mMins);                   // centre
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);                   // radii per-axis

        if (!CheckCommonParms(src, p))
            WarnBad(effect, src, "sphere");

        if (p.mFlags & PPFLAG_SURFACE/* 0x01 */) {          // surface == “shell”
            p.mSpawnType = vecCount + 28;
            FixupParms(p);
        }
    }

    //======================================================================
    //          5.  CYLINDER
    //======================================================================
    else if (!idStr::Icmp(tok, "cylinder"))
    {
        p.mSpawnType = vecCount + 32;
        GetVector(src, vecCount, p.mMins);                   // base
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);                   // top

        if (!CheckCommonParms(src, p))
            WarnBad(effect, src, "cylinder");

        if (p.mFlags & PPFLAG_SURFACE/* 0x01 */) {          // surface only
            p.mSpawnType = vecCount + 36;
            FixupParms(p);
        }
    }

    //======================================================================
    //          6.  SPIRAL  (mins / maxs / pitch)
    //======================================================================
    else if (!idStr::Icmp(tok, "spiral"))
    {
        p.mSpawnType = vecCount + 40;
        GetVector(src, vecCount, p.mMins);
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);
        src->ExpectTokenString(",");
        p.mRange = src->ParseFloat();                          // pitch Δ

        if (!CheckCommonParms(src, p))
            WarnBad(effect, src, "spiral");

        FixupParms(p);   // surfaces always collapse to SCALE variants
    }

    //======================================================================
    //          7.  MODEL  (arbitrary mesh sampling)
    //======================================================================
    else if (!idStr::Icmp(tok, "model"))
    {
        p.mSpawnType = vecCount + 44;

        // model name
        src->ReadToken(&tok);
        idRenderModel* mdl = renderModelManager->FindModel(tok);

        if (mdl->NumSurfaces() == 0) {
            common->Warning("^4BSE:^1 No surfaces in model '%s' "
                "– falling back to _default",
                tok.c_str());
            mdl = renderModelManager->FindModel("_default");
        }
        p.mMisc = mdl;

        // mins / maxs
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMins);
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);

        if (!CheckCommonParms(src, p))
            WarnBad(effect, src, "model");
    }

    //======================================================================
    //          8.  Unknown keyword
    //======================================================================
    else {
        common->Warning("^4BSE:^1 Unknown spawn keyword '%s' in '%s' "
            "(file: %s, line: %d)",
            tok.c_str(), effect->GetName(),
            src->GetFileName(), src->GetLineNum());
        src->SkipBracedSection(true);
        return false;
    }

    //----------------------------------------------------------------------
    //  Canonicalise for fast runtime use
    //----------------------------------------------------------------------
    FixupParms(p);
    return true;
}

// ==========================================================================
//  1.  SPAWN-DOMAIN BLOCK
// ==========================================================================
bool rvParticleTemplate::ParseSpawnDomains(rvDeclEffect* effect,
    idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        //----------------------------------------------------------
        //  Dispatch – every keyword maps to one rvParticleParms slot
        //----------------------------------------------------------
        if (!idStr::Icmp(tok, "position")) ParseSpawnParms(effect, src, mSpawnPosition, 3);
        else if (!idStr::Icmp(tok, "direction")) {
            ParseSpawnParms(effect, src, mSpawnDirection, 3);
            mFlags |= PTFLAG_CALCED_NORMAL; // 0x4000 //k??? TODO Q4BSE is 0x00000040;      /* marks “hasDir” */ BYTE1(this->mFlags) |= 0x40u;
        }
        else if (!idStr::Icmp(tok, "velocity")) ParseSpawnParms(effect, src, mSpawnVelocity, 3);
        else if (!idStr::Icmp(tok, "acceleration")) ParseSpawnParms(effect, src, mSpawnAcceleration, 3);
        else if (!idStr::Icmp(tok, "friction")) ParseSpawnParms(effect, src, mSpawnFriction, 3);
        else if (!idStr::Icmp(tok, "tint")) ParseSpawnParms(effect, src, mSpawnTint, 3);
        else if (!idStr::Icmp(tok, "fade")) ParseSpawnParms(effect, src, mSpawnFade, 1);
        else if (!idStr::Icmp(tok, "size")) ParseSpawnParms(effect, src, mSpawnSize, mNumSizeParms);
        else if (!idStr::Icmp(tok, "rotate")) ParseSpawnParms(effect, src, mSpawnRotate, mNumRotateParms);
        else if (!idStr::Icmp(tok, "angle")) ParseSpawnParms(effect, src, mSpawnAngle, 3);
        else if (!idStr::Icmp(tok, "offset")) ParseSpawnParms(effect, src, mSpawnOffset, 3);
        else if (!idStr::Icmp(tok, "length")) ParseSpawnParms(effect, src, mSpawnLength, 3);
        else {
            common->Warning("^4BSE:^1 Invalid spawn keyword '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
            src->SkipBracedSection(true);
        }
    }
    return true;
}


// ==========================================================================
//  2.  DEATH-DOMAIN BLOCK   (plus automatic envelope fallbacks)
// ==========================================================================
bool rvParticleTemplate::ParseDeathDomains(rvDeclEffect* effect,
    idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        if (!idStr::Icmp(tok, "tint")) { ParseSpawnParms(effect, src, mDeathTint, 3); SetDefaultEnv(mTintEnvelope); }
        else if (!idStr::Icmp(tok, "fade")) { ParseSpawnParms(effect, src, mDeathFade, 1); SetDefaultEnv(mFadeEnvelope); }
        else if (!idStr::Icmp(tok, "size")) { ParseSpawnParms(effect, src, mDeathSize, mNumSizeParms); SetDefaultEnv(mSizeEnvelope); }
        else if (!idStr::Icmp(tok, "rotate")) { ParseSpawnParms(effect, src, mDeathRotate, mNumRotateParms); SetDefaultEnv(mRotateEnvelope); }
        else if (!idStr::Icmp(tok, "angle")) { ParseSpawnParms(effect, src, mDeathAngle, 3); SetDefaultEnv(mAngleEnvelope); }
        else if (!idStr::Icmp(tok, "offset")) { ParseSpawnParms(effect, src, mDeathOffset, 3); SetDefaultEnv(mOffsetEnvelope); }
        else if (!idStr::Icmp(tok, "length")) { ParseSpawnParms(effect, src, mDeathLength, 3); SetDefaultEnv(mLengthEnvelope); }
        else {
            common->Warning("^4BSE:^1 Invalid end keyword '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
            src->SkipBracedSection(true);
        }
    }
    return true;
}


// ==========================================================================
//  3.  IMPACT BLOCK     (bounce / remove / effect)
// ==========================================================================
bool rvParticleTemplate::ParseImpact(rvDeclEffect* effect,
    idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    mFlags |= PTFLAG_HAS_PHYSICS; // 0x200 //k??? TODO Q4BSE is 0x00000002;     // “hasImpact” bit if ( idLexer::ExpectTokenString(lexer, "{") && (BYTE1(this->mFlags) |= 2u

    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        if (!idStr::Icmp(tok, "effect"))            // ----- effect list
        {
            src->ReadToken(&tok);                     // grab effect name
            if (mNumImpactEffects >= 4) {
                common->Warning("^4BSE:^1 Too many impact effects '%s' in '%s' "
                    "(file: %s, line: %d)",
                    tok.c_str(), effect->GetName(),
                    src->GetFileName(), src->GetLineNum());
            }
            else {
                mImpactEffects[mNumImpactEffects++] =
                    declManager->FindEffect(tok, false);
            }
        }
        else if (!idStr::Icmp(tok, "remove"))       // ----- remove flag
        {
            const bool remove = src->ParseInt() != 0;
            if (remove) mFlags |= PTFLAG_DELETE_ON_IMPACT; else mFlags &= ~PTFLAG_DELETE_ON_IMPACT; // 0x400 //k??? TODO Q4BSE is 0x00000004; BYTE1(this->mFlags) |= 4u;
        }
        else if (!idStr::Icmp(tok, "bounce"))       // ----- elasticity
        {
            mBounce = src->ParseFloat();
        }
        else                                            // ----- unknown key
        {
            common->Warning("^4BSE:^1 Invalid impact keyword '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
            src->SkipBracedSection(true);
        }
    }
    return true;
}


// ==========================================================================
//  4.  TIMEOUT BLOCK    (effect list only)
// ==========================================================================
bool rvParticleTemplate::ParseTimeout(rvDeclEffect* effect,
    idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        if (idStr::Icmp(tok, "effect")) {
            common->Warning("^4BSE:^1 Invalid timeout keyword '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
            src->SkipBracedSection(true);
            continue;
        }

        src->ReadToken(&tok);                   // effect name
        if (mNumTimeoutEffects >= 4) {
            common->Warning("^4BSE:^1 Too many timeout effects '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
        }
        else {
            mTimeoutEffects[mNumTimeoutEffects++] =
                declManager->FindEffect(tok, false);
        }
    }
    return true;
}


// ==========================================================================
//  5.  BLEND-MODE PARSER  (currently only “add” is recognised)
// ==========================================================================
bool rvParticleTemplate::ParseBlendParms(rvDeclEffect* effect,
    idLexer* src)
{
    idToken tok;
    if (!src->ReadToken(&tok))
        return false;

    if (!idStr::Icmp(tok, "add")) {
        mFlags |= PTFLAG_ADDITIVE; // 0x8000 //k??? TODO Q4BSE is 0x00000080;       // additive flag BYTE1(this->mFlags) |= 0x80u;
    }
    else {
        common->Warning("^4BSE:^1 Invalid blend type '%s' in '%s' "
            "(file: %s, line: %d)",
            tok.c_str(), effect->GetName(),
            src->GetFileName(), src->GetLineNum());
    }
    return true;
}

/* ========================================================================
   1.  Deep-equality check
   ======================================================================== */
bool rvParticleTemplate::Compare(const rvParticleTemplate& rhs) const {

    /* ------------------------------------------------------------
       Cheap tests first – bitfields, type, durations, material etc.
       ------------------------------------------------------------ */
    if (mFlags != rhs.mFlags ||
        mType != rhs.mType ||
        mDuration != rhs.mDuration ||
        idStr::Cmp(mMaterialName, rhs.mMaterialName) ||
        idStr::Cmp(mModelName, rhs.mModelName) ||
        mGravity != rhs.mGravity ||
        !(idMath::Fabs(mBounce - rhs.mBounce) < idMath::FLT_EPSILON) || //k??? TODO wrap with !(condition)
        mNumSizeParms != rhs.mNumSizeParms ||
        mNumRotateParms != rhs.mNumRotateParms ||
        mVertexCount != rhs.mVertexCount ||
        mIndexCount != rhs.mIndexCount)
        return false;

    if (mSpawnPosition != rhs.mSpawnPosition ||
        mSpawnVelocity != rhs.mSpawnVelocity ||
        mSpawnAcceleration != rhs.mSpawnAcceleration ||
        mSpawnFriction != rhs.mSpawnFriction ||
        mSpawnDirection != rhs.mSpawnDirection ||
        !TintEqual(mSpawnTint, rhs.mSpawnTint) ||
        mSpawnFade != rhs.mSpawnFade ||
        mSpawnSize != rhs.mSpawnSize ||
        mSpawnRotate != rhs.mSpawnRotate ||
        mSpawnAngle != rhs.mSpawnAngle ||
        mSpawnOffset != rhs.mSpawnOffset ||
        mSpawnLength != rhs.mSpawnLength)
    {
        return false;
    }

    if (DeathNeeds(mTintEnvelope) && !TintEqual(mDeathTint, rhs.mDeathTint))   return false;
    if (DeathNeeds(mFadeEnvelope) && (mDeathFade != rhs.mDeathFade))         return false;
    if (DeathNeeds(mSizeEnvelope) && (mDeathSize != rhs.mDeathSize))         return false;
    if (DeathNeeds(mRotateEnvelope) && (mDeathRotate != rhs.mDeathRotate))       return false;
    if (DeathNeeds(mAngleEnvelope) && (mDeathAngle != rhs.mDeathAngle))        return false;
    if (DeathNeeds(mOffsetEnvelope) && (mDeathOffset != rhs.mDeathOffset))       return false;
    if (DeathNeeds(mLengthEnvelope) && (mDeathLength != rhs.mDeathLength))       return false;


    /* ------------------------------------------------------------
       Impact / timeout effect arrays
       ------------------------------------------------------------ */
    if (mNumImpactEffects != rhs.mNumImpactEffects ||
        mNumTimeoutEffects != rhs.mNumTimeoutEffects)
        return false;

    for (int i = 0; i < mNumImpactEffects; ++i)
        if (mImpactEffects[i] != rhs.mImpactEffects[i])
            return false;

    for (int i = 0; i < mNumTimeoutEffects; ++i)
        if (mTimeoutEffects[i] != rhs.mTimeoutEffects[i])
            return false;

    /* ------------------------------------------------------------
       Envelope bodies
       ------------------------------------------------------------ */
    if (mRotateEnvelope != rhs.mRotateEnvelope ||
        mSizeEnvelope != rhs.mSizeEnvelope ||
        mFadeEnvelope != rhs.mFadeEnvelope ||
        mTintEnvelope != rhs.mTintEnvelope ||
        mAngleEnvelope != rhs.mAngleEnvelope ||
        mOffsetEnvelope != rhs.mOffsetEnvelope ||
        mLengthEnvelope != rhs.mLengthEnvelope)
    {
        return false;
    }

    /* ------------------------------------------------------------
       Trail data
       ------------------------------------------------------------ */
    if (mTrailType != rhs.mTrailType)
        return false;

    switch (mTrailType)
    {
    case TRAIL_NONE: break; // 0
	case TRAIL_BURN: // 1: // burn
	case TRAIL_MOTION: // 2: // motion
        if (mTrailTime != rhs.mTrailTime || mTrailCount != rhs.mTrailCount)
            return false;
        break;
	case TRAIL_PARTICLE: // 3: // custom
        if (idStr::Cmp(mTrailTypeName, rhs.mTrailTypeName))
            return false;
        break;
    }
    return true;
}



/* ========================================================================
   2.  Finish() – post-parse sanity passes & derived-data setup
   ======================================================================== */
void rvParticleTemplate::Finish(void)
{
    mFlags |= PTFLAG_PARSED; // 0x100 //k??? TODO Q4BSE is 0x0001;                     // “parsed” bit BYTE1(this->mFlags) |= 1u;

    /* --------------------------------------------------------------------
       2-A  trail sanitiser
       -------------------------------------------------------------------- */
    if (mTrailType == 0 || mTrailType == TRAIL_PARTICLE/* 3 */) {
        mTrailTime.Zero();
        mTrailCount.Zero();
    }

    /* --------------------------------------------------------------------
       2-B  vertex / index budgets by particle type
       -------------------------------------------------------------------- */
    switch (mType)
    {
        /* billboards / beams / ribbons / decals ................................ */
    case PTYPE_SPRITE/* 1 */: case PTYPE_LINE/* 2 */: case PTYPE_DECAL/* 4 */: case PTYPE_LIGHT/* 6 */: case PTYPE_LINKED/* 8 */:
	case PTYPE_ORIENTED/* 3 */: //k??? TODO should add oriented type???
        mVertexCount = 4;
        mIndexCount = 6;
        if (mTrailType == TRAIL_BURN/* 1 */ && mTrailCount.y > 0.0f) {
            const int maxTrail = GetMaxTrailCount();
            mVertexCount *= maxTrail;
            mIndexCount *= maxTrail;
        }
        break;

        /* ribbon-extruded mesh .................................................. */
    case PTYPE_MODEL: { // 5
        // grab first surface of model for vert/idx counts and material
        idRenderModel* mdl = renderModelManager->FindModel(mModelName);
        if (mdl->NumSurfaces())
        {
            const modelSurface_t * s = mdl->Surface(0);
            const srfTriangles_t* tri = s->geometry;
            mVertexCount = tri ? tri->numVerts : 0; //k??? TODO Q4BSE duplicate: s->geometry->numVerts == tri->numVerts;
            mIndexCount = tri ? tri->numIndexes : 0; //k??? TODO Q4BSE duplicate: s->geometry->numIndexes == tri->numIndexes;
            mMaterial = s->shader;
            mMaterialName = mMaterial->GetName();
        }

        /* generate AABB trace-model for ribbon collisions */
        idBounds box;
		BSE::GetBounds(mdl, box);
        idTraceModel* tm = new idTraceModel;
        tm->InitBox();
        tm->SetupBox(box);
        mTraceModelIndex = bseLocal.traceModels.Append(tm);
        break;
    }

          /* forked sprite ........................................................ */
	case PTYPE_ELECTRIC: // 7:
        mVertexCount = 4 * (5 * mNumForks + 5);
        mIndexCount = 60 * (mNumForks + 1);
        break;

        /* glare helper .......................................................... */
	case PTYPE_DEBRIS: // 9:
        mVertexCount = mIndexCount = 0;
        break;

    default:
        mVertexCount = 0; mIndexCount = 0;          // should not occur
    }

    /* --------------------------------------------------------------------
       2-C  clamp & sort min/max ranges
       -------------------------------------------------------------------- */
    float y;
    if (mDuration.x >= mDuration.y)
    {
        y = mDuration.y;
        mDuration.y = mDuration.x;
        mDuration.x = y;
    }
    if (mGravity.x >= mGravity.y)
    {
        y = mGravity.y;
        mGravity.y = mGravity.x;
        mGravity.x = y;
    }
    if (mTrailTime.x >= mTrailTime.y)
    {
        y = mTrailTime.y;
        mTrailTime.y = mTrailTime.x;
        mTrailTime.x = y;
    }
    if (mTrailCount.x >= mTrailCount.y)
    {
        y = mTrailCount.y;
        mTrailCount.y = mTrailCount.x;
        mTrailCount.x = y;
    }

    /* --------------------------------------------------------------------
       2-D  pre-compute the spawn-centre for helpers that need it
       -------------------------------------------------------------------- */
    if (!(mFlags & PTFLAG_GENERATED_ORG_NORMAL/* 0x1000 */)) {
        switch (mSpawnPosition.mSpawnType)
        {
        case 0x0B:                               // explicit point
            mCentre = mSpawnPosition.mMins;
            break;

        case 0x0F: case 0x13: case 0x17:         // boxes
        case 0x1B: case 0x1F: case 0x23:
        case 0x27: case 0x2B: case 0x2F:
            mCentre = (mSpawnPosition.mMaxs + mSpawnPosition.mMins) * 0.5f;
            break;

        default: break;                          // others treat centre ≡ 0
        }
    }
}



/* ========================================================================
   3.  Parse() – master lexer loop
   ======================================================================== */
bool rvParticleTemplate::Parse(rvDeclEffect* effect, idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    while (src->ReadToken(&tok))
    {
        if (!idStr::Cmp(tok, "}"))
            break;

        /* ----------------------------------------------------------------
           Hand each keyword to its mini-parser or immediate setter
           ---------------------------------------------------------------- */
        if (!idStr::Icmp(tok, "start"))      ParseSpawnDomains(effect, src);
        else if (!idStr::Icmp(tok, "end"))      ParseDeathDomains(effect, src);
        else if (!idStr::Icmp(tok, "motion"))      ParseMotionDomains(effect, src);

        /*  generated* flags ------------------------------------------------*/
        else if (!idStr::Icmp(tok, "generatedNormal")) mFlags |= PTFLAG_GENERATED_NORMAL; // 0x800 //k??? TODO Q4BSE is 0x0100; BYTE1(this->mFlags) |= 8u;
        else if (!idStr::Icmp(tok, "generatedOriginNormal")) mFlags |= PTFLAG_GENERATED_ORG_NORMAL; // 0x1000 //k??? TODO Q4BSE is 0x0200; BYTE1(this->mFlags) |= 0x10u;
        else if (!idStr::Icmp(tok, "flipNormal")) mFlags |= PTFLAG_FLIPPED_NORMAL; // 0x2000 //k??? TODO Q4BSE is 0x0400; BYTE1(this->mFlags) |= 0x20u;
        else if (!idStr::Icmp(tok, "generatedLine")) mFlags |= PTFLAG_GENERATED_LINE; // 0x10000; BYTE2(this->mFlags) |= 1u;

        /*  persist / tiling / duration / gravity -------------------------- */
        else if (!idStr::Icmp(tok, "persist"))  mFlags |= PTFLAG_PERSIST; // 0x200000; BYTE2(this->mFlags) |= 0x20u;

        else if (!idStr::Icmp(tok, "tiling")) {
            mFlags |= PTFLAG_TILED; // 0x100000; BYTE2(this->mFlags) |= 0x10u;
            mTiling = idMath::ClampFloat(0.002f, 1024.f, src->ParseFloat());
        }
        else if (!idStr::Icmp(tok, "duration")) {
            mDuration.x = idMath::ClampFloat(0.002f, 60.f, src->ParseFloat());
            src->ExpectTokenString(",");
            mDuration.y = idMath::ClampFloat(0.002f, 60.f, src->ParseFloat());
        }
        else if (!idStr::Icmp(tok, "gravity")) {
            mGravity.x = src->ParseFloat();  src->ExpectTokenString(",");
            mGravity.y = src->ParseFloat();
        }

        /*  trail section --------------------------------------------------- */
        else if (!idStr::Icmp(tok, "trailType"))
        {
            src->ReadToken(&tok);
            if (!idStr::Icmp(tok, "burn")) mTrailType = TRAIL_BURN; // 1;
            else if (!idStr::Icmp(tok, "motion")) mTrailType = TRAIL_MOTION; // 2;
            else { mTrailType = TRAIL_PARTICLE/* 3 */;  mTrailTypeName = tok.c_str(); }
        }
        else if (!idStr::Icmp(tok, "trailMaterial")) {
            src->ReadToken(&tok);
            mTrailMaterial = declManager->FindMaterial(tok, false);
            mTrailMaterialName = tok.c_str();
        }
        else if (!idStr::Icmp(tok, "trailTime")) {
            mTrailTime.x = src->ParseFloat();  src->ExpectTokenString(",");
            mTrailTime.y = src->ParseFloat();
        }
        else if (!idStr::Icmp(tok, "trailCount")) {
            mTrailCount.x = src->ParseFloat(); src->ExpectTokenString(",");
            mTrailCount.y = src->ParseFloat();
        }

        /*  material / entityDef / fork etc. ------------------------------- */
        else if (!idStr::Icmp(tok, "material")) {
            src->ReadToken(&tok);
            mMaterial = declManager->FindMaterial(tok, true);
            mMaterialName = tok.c_str();
        }
        else if (!idStr::Icmp(tok, "entityDef")) {
            src->ReadToken(&tok);
            mEntityDefName = tok.c_str();
            // warm-cache its dict if it exists
            if (const idDecl* d = declManager->FindType(DECL_ENTITYDEF,
                mEntityDefName,
                false))
            {
                if(d)
                {
                    const idDeclEntityDef *def = static_cast<const idDeclEntityDef *>(d);
                    game->CacheDictionaryMedia(&def->dict);
                }
            }
        }
        else if (!idStr::Icmp(tok, "fork")) {
            mNumForks = idMath::ClampInt(0, 16, src->ParseInt());
        }
        else if (!idStr::Icmp(tok, "forkMins")) GetVector(src, 3, mForkSizeMins);
        else if (!idStr::Icmp(tok, "forkMaxs")) GetVector(src, 3, mForkSizeMaxs);
        else if (!idStr::Icmp(tok, "jitterSize")) GetVector(src, 3, mJitterSize);
        else if (!idStr::Icmp(tok, "jitterRate")) mJitterRate = src->ParseFloat();
        else if (!idStr::Icmp(tok, "jitterTable")) {
            src->ReadToken(&tok);
            const idDeclTable* t = static_cast<const idDeclTable *>(declManager->FindType(DECL_TABLE, tok, true));
            if (!t->IsImplicit()) mJitterTable = t;
        }

        /*  blend / shadow / specular flags -------------------------------- */
        else if (!idStr::Icmp(tok, "blend")) ParseBlendParms(effect, src);
        else if (!idStr::Icmp(tok, "shadows")) mFlags |= PTFLAG_SHADOWS; // 0x020000;
        else if (!idStr::Icmp(tok, "specular")) mFlags |= PTFLAG_SPECULAR; // 0x040000;

        /*  model-specific (ribbon-extrude) -------------------------------- */
        else if (!idStr::Icmp(tok, "model")) {
            src->ReadToken(&tok);
            mModelName = tok.c_str();

            if (renderModelManager->FindModel(tok)->NumSurfaces() == 0) {
                mModelName = "_default";
                common->Warning("^4BSE:^1 Model '%s' has no surfaces – using _default",
                    tok.c_str());
            }
        }

        /*  impact / timeout blocks ---------------------------------------- */
        else if (!idStr::Icmp(tok, "impact")) ParseImpact(effect, src);
        else if (!idStr::Icmp(tok, "timeout")) ParseTimeout(effect, src);

        /*  unknown keyword ------------------------------------------------- */
        else {
            common->Warning("^4BSE:^1 Invalid particle keyword '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
            src->SkipBracedSection(true);
        }
    }

    /* --------------------------------------------------------------------
       Final consistency pass
       -------------------------------------------------------------------- */
    Finish();
    return true;
}
