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

// ───────── helpers ───────────────────────────────────────────────────────────
void rvLightParticle::ClampRadius() {
    mLight.lightRadius.x = /*idMath::*/Max(mLight.lightRadius.x, 1.f);
    mLight.lightRadius.y = /*idMath::*/Max(mLight.lightRadius.y, 1.f);
    mLight.lightRadius.z = /*idMath::*/Max(mLight.lightRadius.z, 1.f);
}
void rvLightParticle::SetOriginFromLocal(const idVec3& p) {
    mLight.origin = p;
}
void rvLightParticle::SetAxis(const idMat3& m) {
    mLight.axis = m;
}

// ───────── Destroy ───────────────────────────────────────────────────────────
bool rvLightParticle::Destroy() {
    if (mLightDefHandle != -1) {
        session->rw->FreeLightDef(mLightDefHandle);
        mLightDefHandle = -1;
    }
    return true;
}

// ───────── InitLight ─────────────────────────────────────────────────────────
bool rvLightParticle::InitLight(rvBSE* effect,
                                const rvSegmentTemplate* st,
    float              time) {
    float evalTime;
    if (!GetEvaluationTime(time, evalTime, /*looping*/false))
        return false;

    // clear, then evaluate env-parms
    idVec4 tint;
    idVec3 size;
    idVec3 pos;

    memset(&mLight, 0, sizeof(mLight));

    mTintEnv.Evaluate(evalTime, tint.ToFloatPtr());  // rgb
    mFadeEnv.Evaluate(evalTime, &tint.w);            // alpha

   // EvaluateSize(evalTime, size); // jmarshall - fix me
    const float localT = time - mMotionStartTime;
    EvaluatePosition(effect, pos, localT);

    // Transform from local-segment space to world
    // TODO: replace these raw accesses with proper data when template defined
    const idMat3& segAxis = effect->mCurrentAxis;   
    const idVec3& segOrigin = effect->mCurrentOrigin; 

    idVec3 worldPos = pos * segAxis + segOrigin;
    SetOriginFromLocal(worldPos);

    mLight.lightRadius = size;
    ClampRadius();

    SetAxis(segAxis);

    mLight.shaderParms[0] = tint.x;
    mLight.shaderParms[1] = tint.y;
    mLight.shaderParms[2] = tint.z;
    mLight.shaderParms[3] = tint.w;

    mLight.pointLight = true;
    mLight.noShadows = (st->mFlags & PTFLAG_SHADOWS) == 0;
    mLight.noSpecular = (st->mFlags & PTFLAG_SPECULAR) == 0;
    mLight.detailLevel = 10.f;
    mLight.shader = st->mParticleTemplate.mMaterial; //k??? TODO set particle material for light // assumption jmarshall - fix me

    mLightDefHandle = session->rw->AddLightDef(&mLight);
    return true;
}

// ───────── PresentLight ──────────────────────────────────────────────────────
bool rvLightParticle::PresentLight(rvBSE* segState, const rvParticleTemplate* pt, float evalTime, bool infinite) {
    float t;
    if (!GetEvaluationTime(evalTime, t, infinite))
        return false;

    idVec4 tint;
    idVec3 size;
    idVec3 pos;

    mTintEnv.Evaluate(t, tint.ToFloatPtr());
    mFadeEnv.Evaluate(t, &tint.w);

    EvaluateSize(t, size.ToFloatPtr());
    const float localT = evalTime - mMotionStartTime;
    EvaluatePosition(segState, pos, localT);

    idVec3 worldPos = pos * segState->mCurrentAxis + segState->mCurrentOrigin;
    SetOriginFromLocal(worldPos);

    mLight.lightRadius = size;
    ClampRadius();
    SetAxis(segState->mCurrentAxis);

    mLight.shaderParms[0] = tint.x;
    mLight.shaderParms[1] = tint.y;
    mLight.shaderParms[2] = tint.z;
    mLight.shaderParms[3] = tint.w;

    session->rw->UpdateLightDef(mLightDefHandle, &mLight);
    return true;
}

bool rvSegment::HandleLight(rvBSE *effect, const rvSegmentTemplate *st, float time)
{
    rvParticle *usedHead; // ecx
    unsigned int v6; // edx
    rvParticle *v7; // ecx

    usedHead = this->mUsedHead;
    if (!usedHead)
        return false;
    v6 = (unsigned int) st->mFlags >> 5;
    bool infinite = (st->mFlags & STFLAG_INFINITE_DURATION/* 0x20 */) != 0;
    usedHead->PresentLight(
            effect,
            &st->mParticleTemplate,
            time,
            infinite
            );
    if ((st->mFlags & STFLAG_INFINITE_DURATION/* 0x20 */) != 0)
        return false;
    v7 = mUsedHead;
    if (v7->mEndTime - BSE_TIME_EPSILON/* 0.0020000001f */ > time)
        return false;
    v7->Destroy();
    mFreeHead = mUsedHead;
    mUsedHead = NULL;
    return true;
}
