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

//karin: render single segment/particle type effect
#if BSE_DEV
#define SINGLE_EFFECT(de, i) if(BSE::SingleEffect(de, i)) continue;

extern idCVar bse_singleSegmentType;
extern idCVar bse_singleParticleType;

namespace BSE {
ID_INLINE static bool SingleEffect(rvDeclEffect* mDeclEffect, int i)
{
    const char *single = bse_singleSegmentType.GetString();
    if(single && single[0])
    {
        const rvSegmentTemplate *st = mDeclEffect->GetSegmentTemplate(i);
        if(st)
        {
#define SINGLE_SEG_TYPE(stype) if(st->mSegType == SEG_##stype && idStr::Icmp(single, #stype)) return true;
            SINGLE_SEG_TYPE(EFFECT)
            SINGLE_SEG_TYPE(EMITTER)
            SINGLE_SEG_TYPE(SPAWNER)
            SINGLE_SEG_TYPE(TRAIL)
            SINGLE_SEG_TYPE(SOUND)
            SINGLE_SEG_TYPE(DECAL)
            SINGLE_SEG_TYPE(LIGHT)
            SINGLE_SEG_TYPE(DV)
            SINGLE_SEG_TYPE(SHAKE)
            SINGLE_SEG_TYPE(TUNNEL)
#undef SINGLE_SEG_TYPE
        }
    }

    single = bse_singleParticleType.GetString();
    if(single && single[0])
    {
        const rvSegmentTemplate *st = mDeclEffect->GetSegmentTemplate(i);
        if(st)
        {
#define SINGLE_SEG_TYPE(stype) if(st->mParticleTemplate.mType == PTYPE_##stype && idStr::Icmp(single, #stype)) return true;
            SINGLE_SEG_TYPE(SPRITE)
            SINGLE_SEG_TYPE(LINE)
            SINGLE_SEG_TYPE(ORIENTED)
            SINGLE_SEG_TYPE(DECAL)
            SINGLE_SEG_TYPE(MODEL)
            SINGLE_SEG_TYPE(LIGHT)
            SINGLE_SEG_TYPE(ELECTRIC)
            SINGLE_SEG_TYPE(LINKED)
            SINGLE_SEG_TYPE(DEBRIS)
            SINGLE_SEG_TYPE(SOUND)
            SINGLE_SEG_TYPE(PARTICLE)
#undef SINGLE_SEG_TYPE
        }
    }

    return false;
}
};
#else
#define SINGLE_EFFECT(de, i)
#endif

/*------------------------------------------------------------------------
    Distance–based attenuation helpers
  ----------------------------------------------------------------------*/
float rvBSE::GetAttenuation(const rvSegmentTemplate* st) const
{
    const idVec2& range = st->mAttenuation;        // x == near, y == far

    // No attenuation specified, or still inside the “near + 1” guard-band
    if ((range.x <= 0.0f && range.y <= 0.0f) ||
        (range.x + 1.0f > mShortestDistanceToCamera))
    {
        return mAttenuation;
    }

    // Fully inside the attenuation ramp?
    if (range.y - 1.0f >= mShortestDistanceToCamera)
    {
        const float t = (mShortestDistanceToCamera - range.x) /
            (range.y - range.x);     // 0 @ near … 1 @ far
        return (1.0f - t) * mAttenuation;
    }

    // Beyond the far distance → completely faded
    return 0.0f;
}
//-----------------------------------------------------------------------
float rvBSE::GetOriginAttenuation(const rvSegmentTemplate* st) const
{
    const idVec2& range = st->mAttenuation;

    if ((range.x <= 0.0f && range.y <= 0.0f) ||
        (range.x + 1.0f > mOriginDistanceToCamera))
    {
        return st->mScale * mAttenuation;
    }

    if (range.y - 1.0f >= mOriginDistanceToCamera)
    {
        const float t = (mOriginDistanceToCamera - range.x) /
            (range.y - range.x);
        return (1.0f - t) * st->mScale * mAttenuation;
    }

    return 0.0f;
}

/*------------------------------------------------------------------------
    Sound emitter upkeep for a single segment
  ----------------------------------------------------------------------*/
void rvBSE::UpdateSoundEmitter(const rvSegmentTemplate* st, rvSegment* seg)
{
    idSoundEmitter* emitter = soundSystem->EmitterForIndex( /*channel*/SOUNDWORLD_GAME/* 1 */, mReferenceSoundHandle);
    if (!emitter) {
        return;
    }

    // Global no-sound flag?
    if (mFlags & EF_SOUND/* 0x0008 */)
    {
        // Stop the looping sound (but only if this segment actually started it)
        if (st->GetSoundLooping() && (seg->mFlags & 0x0002))
        {
            emitter->StopSound(seg->mSegmentTemplateHandle + 1);
        }
        return;
    }

    // Still audible – update spatialisation and pitch
    soundShaderParms_t parms;          // zero initialise
    memset(&parms, 0, sizeof(parms));
    parms.volume = seg->mSoundVolume;
    parms.frequencyShift = seg->mFreqShift;

    emitter->UpdateEmitter(mCurrentOrigin,mCurrentVelocity, /*listener number*/0, &parms);
}

/*------------------------------------------------------------------------
    Interpolated offset between last & current origin
  ----------------------------------------------------------------------*/
idVec3 rvBSE::GetInterpolatedOffset(idVec3 *result, float time) const
{
    /*//k??? TODO not return reference // static thread_local*/ idVec3 offset(0.0f, 0.0f, 0.0f);      // safe: only one thread in id Tech 4
    //offset.Zero();

    const float dt = mCurrentTime - mLastTime;
    if (dt >= 0.002f)                     // ignore tiny steps (< 2 ms)
    {
        const float lerp = 1.0f - (time - mLastTime) / dt;
        offset = (mCurrentOrigin - mLastOrigin) * lerp;
    }
    if(result) *result = offset;
    return offset;
}

/*------------------------------------------------------------------------
    Misc. trivial helpers
  ----------------------------------------------------------------------*/
void rvBSE::SetDuration(float time)
{
    if (time > mDuration) {
        mDuration = time;
    }
}
//-----------------------------------------------------------------------
const char* rvBSE::GetDeclName()
{
    return mDeclEffect->GetName();
}

/*------------------------------------------------------------------------
    Core per-frame update fed by the owning renderEntity_t
  ----------------------------------------------------------------------*/
void rvBSE::UpdateFromOwner(renderEffect_s* parms,
    float           time,
    bool            init)
{
    //--------------------------------------------------------------------
    // 1) Book-keeping: previous state → last*
    //--------------------------------------------------------------------
    mLastTime = mCurrentTime;
    mLastOrigin = mCurrentOrigin;

    //--------------------------------------------------------------------
    // 2) Current absolute transform
    //--------------------------------------------------------------------
    mCurrentTime = time;
    mCurrentOrigin = parms->origin;
    mCurrentAxis = parms->axis;
    mCurrentAxisTransposed = mCurrentAxis.Transpose();

    //--------------------------------------------------------------------
    // 3) Velocity (ignore very small Δt to avoid div-by-0 & jitter)
    //--------------------------------------------------------------------
    const float dt = mCurrentTime - mLastTime;
    if (dt > 0.002f) {
        mCurrentVelocity = (mCurrentOrigin - mLastOrigin) * (1.0f / dt);
    }
    else {
        mCurrentVelocity.Zero();
    }

    //--------------------------------------------------------------------
    // 4) Gravity & normalised gravity direction
    //--------------------------------------------------------------------
    mGravity = parms->gravity;
    mGravityDir = mGravity;
    const float gLenSq = mGravityDir.LengthSqr();
    if (gLenSq != 0.0f) {
        mGravityDir *= idMath::InvSqrt(gLenSq);
    }

    //--------------------------------------------------------------------
    // 5) Build a world-space AABB that encloses:
    //    · current origin
    //    · (optionally) current end-origin
    //--------------------------------------------------------------------
    const float halfSize = mDeclEffect->mSize;
    idVec3 halfCorner(halfSize, halfSize, halfSize);
#if 0 //k??? TODO not be cleared in Q4D, but must re-calc full bounds with useEndOrigin if cleared
    idBounds newWorldBounds = mCurrentWorldBounds;
#else
    idBounds newWorldBounds;
    newWorldBounds.Clear();
#endif
    newWorldBounds.AddPoint(mCurrentOrigin +
                                    halfCorner);
    newWorldBounds.AddPoint(mCurrentOrigin -
                                    halfCorner);

    if ((mFlags & EF_USES_END_ORIGIN/* 0x0002 */) &&                       // effect uses end-origin
        (mDeclEffect->mFlags & DEF_USES_END_ORIGIN/* 0x0002 */))
    {
        if (init
            || mCurrentEndOrigin != parms->endOrigin
            || mCurrentOrigin != mLastOrigin)
        {
            mCurrentEndOrigin = parms->endOrigin;

            newWorldBounds.AddPoint(mCurrentEndOrigin +
                                            halfCorner);
            newWorldBounds.AddPoint(mCurrentEndOrigin -
                                            halfCorner);

            mFlags |= EF_MARK_BOUNDS_DIRTY; // 0x0004;                        // mark “world bounds dirty”
        }
#if 1 //k??? TODO not be cleared in Q4D, but must re-calc full bounds with useEndOrigin if cleared. Because cleared now, so re-calc it, but not setup EF_MARK_BOUNDS_DIRTY flag
        else
        {
            newWorldBounds.AddPoint(mCurrentEndOrigin +
                                    halfCorner);
            newWorldBounds.AddPoint(mCurrentEndOrigin -
                                    halfCorner);

            // mFlags |= EF_MARK_BOUNDS_DIRTY; // 0x0004;                        // mark “world bounds dirty”
        }
#endif
    }
    mCurrentWorldBounds = newWorldBounds;

    //--------------------------------------------------------------------
    // 6) Local-space bounds (world → object)
    //--------------------------------------------------------------------
    mCurrentLocalBounds.Clear();
#if 0
    idVec3 points[8];
    mCurrentWorldBounds.ToPoints(points);
    for(int i = 0; i < 8; i++)
    {
        idVec3 p = points[i] - mCurrentOrigin;
        p = mCurrentAxisTransposed * p;
        mCurrentLocalBounds.AddPoint(p);
    }
#else
    for (int ix = 0; ix < 2; ++ix)
        for (int iy = 0; iy < 2; ++iy)
            for (int iz = 0; iz < 2; ++iz)
            {
                idVec3 corner(newWorldBounds[ix].x,
                    newWorldBounds[iy].y,
                    newWorldBounds[iz].z);

                idVec3 local = mCurrentAxisTransposed * (corner - mCurrentOrigin);
                mCurrentLocalBounds.AddPoint(local);
            }
#endif

    //--------------------------------------------------------------------
    // 7) Copy shader parameters
    //--------------------------------------------------------------------
    mTint.Set(parms->shaderParms[0],
        parms->shaderParms[1],
        parms->shaderParms[2],
        parms->shaderParms[3]);
    mBrightness = parms->shaderParms[6];
    mSpriteSize.x = parms->shaderParms[8];
    mSpriteSize.y = parms->shaderParms[9];
    mAttenuation = parms->attenuation;

    //--------------------------------------------------------------------
    // 8) Optional debug visualisation
    //--------------------------------------------------------------------
    if (bse_debug.GetInteger() > 2)
    {
        session->rw->DebugLine(colorWhite, mCurrentOrigin, mLastOrigin);

        // Little green “up” marker (10 uu)
        session->rw->DebugLine(colorGreen,
            mCurrentOrigin,
            mCurrentOrigin + idVec3(0.f, 0.f, 10.f));
    }
}

//=======================================================================
//  rvBSE  –  remaining functions
//=======================================================================

/*------------------------------------------------------------------------
    1.  Camera–distance bookkeeping for attenuation
  ----------------------------------------------------------------------*/
void rvBSE::UpdateAttenuation()
{
    // Effect not flagged as distance–attenuated? → nothing to do
    if (!(mDeclEffect->mFlags & DEF_ATTENUATION/* 0x0004 */))
        return;

    // ------------------------------------------------------------------
    // a) Distance from effect *origin* to camera
    // ------------------------------------------------------------------
    idVec3  camOrg;
    idMat3  camAxis;                // ignored here, but retrieved for parity
    game->GetPlayerView(camOrg, camAxis);

    mOriginDistanceToCamera =
        idMath::ClampFloat(1.0f, 131072.0f,
            (mCurrentOrigin - camOrg).Length());

    // ------------------------------------------------------------------
    // b) Shortest distance from camera to the *local bounds* of the effect
    // ------------------------------------------------------------------
    //   • Convert camera position into local object space
    idVec3 localCam = (camOrg - mCurrentOrigin) * mCurrentAxis;

    //   • Query AABB distance, then clamp
    mShortestDistanceToCamera =
        idMath::ClampFloat(1.0f, 131072.0f,
            mCurrentLocalBounds.ShortestDistance(localCam));
}

/*------------------------------------------------------------------------
    2.  Looping helpers
  ----------------------------------------------------------------------*/
void rvBSE::LoopInstant(float /*time*/)
{
    if (mDuration != 0.0f)              // instant effects only
        return;

    // Shift start-time into the future and reset all segments
    mStartTime += mDeclEffect->mMaxDuration + 0.5f;

    for (int i = 0; i < mSegments.Num(); ++i)
        mSegments[i]->ResetTime(this, mStartTime);

    if (bse_debug.GetInteger() == 2)
        common->Printf("BSE: Looping duration: %g\n",
            mDeclEffect->mMaxDuration + 0.5f);

    ++mDeclEffect->mLoopCount;
}
//-----------------------------------------------------------------------
void rvBSE::LoopLooping(float /*time*/)
{
    if (mDuration == 0.0f)              // looping effects only
        return;

    // Push timeline forward by one full cycle
    mStartTime += mDuration;
    mDuration = 0.0f;                   // becomes “instant”

    for (int i = 0; i < mSegments.Num(); ++i)
        mSegments[i]->ResetTime(this, mStartTime);

    if (bse_debug.GetInteger() == 2)
        common->Printf("BSE: Looping duration: %g\n", mDuration);

    ++mDeclEffect->mLoopCount;
}

/*------------------------------------------------------------------------
    3.  Per-frame service – returns ‘finished?’
  ----------------------------------------------------------------------*/
bool rvBSE::Service(renderEffect_s* parms, float time)
{
    // ------------------------------------------------------------------
    // a) Owner-driven updates
    // ------------------------------------------------------------------
    UpdateFromOwner(parms, time, /*init=*/false);
    UpdateAttenuation();

	if(bseLocal.DebugHudActive()) //karin: add
    bseLocal.mPerfCounters[4]/* dword_1137DDB8 */ += mSegments.Num();

    // ------------------------------------------------------------------
    // b) Segment state / particle life-cycle
    // ------------------------------------------------------------------
    for (int i = 0; i < mSegments.Num(); ++i)
        mSegments[i]->Check(this, time);

    // If sound is *not* suppressed, the owner wants looping, and the
    // time line just ran past the end → handle formal “loop” transition
    if (!(mFlags & EF_SOUND/* 0x0008 */) && parms->loop &&
        (mDuration + mStartTime < time))
    {
        LoopLooping(time);
    }

    bool anyActive = false;
    for (int i = 0; i < mSegments.Num(); ++i)
        anyActive |= mSegments[i]->UpdateParticles(this, time);

    // ------------------------------------------------------------------
    // c) Clear dirty-bounds bit (segments may set it again this frame)
    // ------------------------------------------------------------------
    mFlags &= ~EF_MARK_BOUNDS_DIRTY; // 0x0004;

    // ------------------------------------------------------------------
    // d) Decide whether the whole effect is finished
    // ------------------------------------------------------------------
    if (mFlags & EF_SOUND/* 0x0008 */)                // sound suppressed ⇒ effect ends
        return !anyActive;

    if (parms->loop)                    // owner wants infinite looping
    {
        if (mDuration + mStartTime < time)
            LoopInstant(time);
        return false;                     // never finishes while looping
    }

    // Non-looping: done when past end-time
    return (mDuration + mStartTime) < time;
}

/*------------------------------------------------------------------------
    4.  Runtime cost evaluation
  ----------------------------------------------------------------------*/
float rvBSE::EvaluateCost(int segment /*-1 = whole effect*/)
{
    if (segment != -1)
        return mSegments[segment]->EvaluateCost();

    mCost = 0.f;
    for (int i = 0; i < mSegments.Num(); ++i)
        mCost += mSegments[i]->EvaluateCost();

    return mCost;
}

/*------------------------------------------------------------------------
    5.  Lightweight debug drawing
  ----------------------------------------------------------------------*/
void rvBSE::DisplayDebugInfo(const renderEffect_s* parms,
    const viewDef_s* view,
    idBounds& bounds)
{
    // Early-out if nothing requested
    if (!bse_debug.GetInteger() && !bse_showBounds.GetInteger())
        return;

    idRenderWorld* rw = (idRenderWorld*)view->renderWorld;

    // ------------------------------------------------------------------
    // a) Text & axis at effect origin
    // ------------------------------------------------------------------
    if (bse_debug.GetInteger())
    {
        idStr txt = va("(%g) (size %g) (bri %g)\n%s",
            mCost,
            mDeclEffect->mSize,
            parms->shaderParms[6],
            mDeclEffect->GetName());

        rw->DebugAxis(parms->origin, parms->axis);
        rw->DrawText(txt.c_str(),
            parms->origin,
            0.3f, // Q4BSE is 14.f,                 // 1050253722 pts
            colorCyan,
            view->renderView.viewaxis,
            1,                    // scale
            0);                  // lifetime
    }

    // ------------------------------------------------------------------
    // b) Bounding boxes
    // ------------------------------------------------------------------
    if (!bse_showBounds.GetInteger())
        return;

#if 1
    // World-space *effect* bounds (blue), differs once oriented
    idBox worldBox(bounds, parms->origin, parms->axis);
    rw->DebugBox(colorBlue, worldBox);

    // World-space model bounds (green)
    float modelMatrix[16];
	idVec3 points[8];
    idVec3 boundsPoints[8];
    idBounds globalBounds;
    idBounds modelBounds;

    globalBounds.Clear();
    modelBounds.Clear();
	mCurrentLocalBounds.ToPoints(points);
	bounds.ToPoints(boundsPoints);
    R_AxisToModelMatrix(parms->axis, parms->origin, modelMatrix);
	for(int i = 0; i < 8; i++)
	{
		idVec3 transformed;
		const idVec3 &v = points[i];
		R_LocalPointToGlobal(modelMatrix, v, transformed);
		globalBounds.AddPoint(transformed);
		const idVec3 &v2 = boundsPoints[i];
		R_LocalPointToGlobal(modelMatrix, v2, transformed);
		modelBounds.AddPoint(transformed);
		
	}
    rw->DebugBounds(colorGreen, globalBounds, vec3_origin);

    // If mesh bounds exceed world bounds draw them in red
    if (!BSE::ContainsBounds(globalBounds, modelBounds)) //k??? TODO BSE::ContainsBounds(modelBounds, globalBounds)
        rw->DebugBounds(colorRed, modelBounds, vec3_origin);
#else
    // World-space model bounds (green)
    rw->DebugBounds(colorGreen, bounds, vec3_origin);

    // World-space *effect* bounds (blue), differs once oriented
    idBox worldBox(mCurrentWorldBounds, parms->origin, parms->axis);
    rw->DebugBox(colorBlue, worldBox);

    // If mesh bounds exceed world bounds draw them in red
    if (!BSE::ContainsBounds(mCurrentWorldBounds, bounds))
        rw->DebugBounds(colorRed, bounds, vec3_origin);
#endif
}

/*------------------------------------------------------------------------
    6.  Build a single-frame rvRenderModelBSE
  ----------------------------------------------------------------------*/
rvRenderModelBSE* rvBSE::Render(const renderEffect_s* owner, const viewDef_s* view)
{
	BSE_LOGFI(rvBSE::Render, "%s", mDeclEffect->GetName())
    if (!bse_render.GetInteger())
        return NULL;

    rvRenderModelBSE* model = new rvRenderModelBSE;

    // Cache camera data so segments can billboard etc.
    mViewAxis = view->renderView.viewaxis;
    mViewOrg = view->renderView.vieworg;

    for (int i = 0; i < mSegments.Num(); ++i)
    {
        //karin: render single segment/particle type effect
        SINGLE_EFFECT(mDeclEffect, i);

        if (mSegments[i]->Active())
        {
            mSegments[i]->Render(this, owner, model, view->floatTime);
            mSegments[i]->RenderTrail(this, owner, model, view->floatTime);
        }
    }

    DisplayDebugInfo(owner, view, model->bounds);
    return model;
}

/*------------------------------------------------------------------------
    7.  Explicit shutdown
  ----------------------------------------------------------------------*/
void rvBSE::Destroy()
{
    // Stop & free sound emitter
    if (idSoundEmitter* emitter = soundSystem->EmitterForIndex(SOUNDWORLD_GAME/* 1 */, mReferenceSoundHandle))
    {
        emitter->StopSound(SCHANNEL_ANY/* 0 */);
        soundSystem->FreeSoundEmitter(SOUNDWORLD_GAME/* 1 */, emitter->Index(), false/* true */); //karin: TODO mark removed only, sound broken sometime
    }

    // Destroy all segments & particles, then free the array
    for (int i = 0; i < mSegments.Num(); ++i)
        delete mSegments[i];

    //Mem_Free(mSegments.Ptr());
    mSegments.Clear();
}

/*------------------------------------------------------------------------
    8.  (Re-)create and pre-sample every segment
  ----------------------------------------------------------------------*/
void rvBSE::UpdateSegments(float time)
{
    const int wanted = mDeclEffect->mSegmentTemplates.Num();

    // Resize container (constructors run automatically)
    mSegments.SetNum(wanted);
    for (int i = 0; i < wanted; ++i)
    {
        mSegments[i] = new rvSegment;
        mSegments[i]->Init(this, mDeclEffect, i, time);
    }

    // Pre-compute particle counts & allocate storage
    for (int i = 0; i < wanted; ++i)
        mSegments[i]->CalcCounts(this, time);
    for (int i = 0; i < wanted; ++i)
        mSegments[i]->InitParticles(this);

    // --------------------------------------------------------------
    // “while-true” ambient effects that may have started long ago
    // --------------------------------------------------------------
    if ((mFlags & EF_LOOP/* 0x0001 */) && (mFlags & EF_AMBIENT/* 0x0010 */) && mDuration != 0.0f)
    {
        // Advance the timeline so that ‘time’ sits inside the current loop
        while (time - (mDuration * 2.f) > mStartTime)
        {
            mStartTime += mDuration;

            for (int i = 0; i < wanted; ++i)
                mSegments[i]->Advance(this);
        }

        // Rewind internal clocks for the just-finished loop iteration
        const float rewind = mDuration * 2.f;
        mCurrentTime -= rewind;
        mStartTime -= rewind;

        for (int i = 0; i < wanted; ++i)
            mSegments[i]->Rewind(this);
    }
}

/*------------------------------------------------------------------------
    9.  First-time construction
  ----------------------------------------------------------------------*/
void rvBSE::Init(rvDeclEffect* decl,
    renderEffect_s* parms,
    float             time)
{
    BSE_LOGFI(PlayEffect -> rvBSE::Init, "%s: %f", decl->GetName(), time);
    // --------------------------------------------------------------
    // Core state
    // --------------------------------------------------------------
    mFlags = parms->loop ? EF_LOOP/* 0x0001 */ : 0;    // bit-0 = “explicit loop”
    if (parms->ambient)  mFlags |= EF_AMBIENT; // 0x0010;      // bit-4 = ambient
	else mFlags &= ~EF_AMBIENT;
											   //

    mDeclEffect = decl;
    mStartTime = parms->startTime;
    mLastTime = time;
    mCurrentTime = time;

    mDuration = 0.f;     // filled as segments load
    mAttenuation = 1.f;
    mCost = 0.f;

    mOriginalOrigin = parms->origin;
    mCurrentOrigin = parms->origin;
    mOriginalAxis = parms->axis;

    // Optional end-origin
    if (parms->hasEndOrigin)
    {
        mFlags |= EF_USES_END_ORIGIN; // 0x0002;      // “hasEndOrigin”
        mOriginalEndOrigin = parms->endOrigin;
        mCurrentEndOrigin = parms->endOrigin;
    }
	else
	{
		mOriginalEndOrigin.Zero();
		mCurrentEndOrigin.Zero();
	}

    // Local bounds start as a symmetric AABB of effect-size
    mCurrentLocalBounds.Zero(); //k??? TODO not Clear() in Q4D
    mCurrentLocalBounds.ExpandSelf(decl->mSize);
	mCurrentLocalBounds.TranslateSelf(mOriginalOrigin);

	mCurrentVelocity.Zero();
    // Build initial world bounds & everything that depends on it
    UpdateFromOwner(parms, time, /*init=*/true);

    // Remember sound emitter handle
    mReferenceSoundHandle = parms->referenceSoundHandle;

    // Create segments & pre-sample
    UpdateSegments(time);

    // Reset attenuation caches so first UpdateAttenuation() works
    mOriginDistanceToCamera = 0.f;
    mShortestDistanceToCamera = 0.f;
    mSpriteSize.Zero();
}

bool rvBSE::CanInterpolate() const
{
    return mCurrentTime - mLastTime > 0.0020000001f;
}

idSoundEmitter * rvBSE::GetReferenceSound(int worldId)
{
    return soundSystem->EmitterForIndex(worldId, mReferenceSoundHandle);
}

#include "BSE_Compat.cpp"
