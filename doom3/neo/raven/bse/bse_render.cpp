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

//--------------------------------------------------------------
//  rvParticle – utility helpers
//--------------------------------------------------------------

bool rvParticle::GetEvaluationTime(float time, float& outEvalTime, bool  infinite) const
{
    outEvalTime = time - mStartTime;

    // Clamp to end of life minus a small epsilon, matching original logic
    if (time >= mEndTime - BSE_TIME_EPSILON/* 0.0020000001f */)
    {
        outEvalTime = (mEndTime - mStartTime) - BSE_TIME_EPSILON/* 0.0020000001f */;
    }

    if (infinite)
    {
        return true;
    }

    // Valid while strictly inside [mStartTime − ε, mEndTime)
    return (time > mStartTime - BSE_TIME_EPSILON/* 0.0020000001f */) &&
        (time < mEndTime);
}

//----------------------------------------------------------------------
int rvParticle::HandleTint(const rvBSE* effect, const idVec4& colour, float alpha) const
{
#if 1 //k??? TODO using full RGB
    // Base tint comes from the effect definition (x channel only)
    idVec4 tint = effect->mTint;

    // Optional premultiplied tint path (matches flag 0x8000 in original code)
    if (mFlags & PTFLAG_ADDITIVE/* 0x8000 */)
    {
		float f = colour.w * alpha;
		tint.x *= colour.x * f;
		tint.y *= colour.y * f;
		tint.z *= colour.z * f;
		tint.w *= colour.w * f;
    }
    else
    {
		tint.x *= colour.x;
		tint.y *= colour.y;
		tint.z *= colour.z;
		tint.w *= colour.w;
    }

    tint *= effect->mBrightness;                // global brightness scale

    // Convert [0‑1] float into packed 0‑255 channel the renderer expects
    return BSE::PackColor(tint.x, tint.y, tint.z, tint.w);
#else
    // Base tint comes from the effect definition (x channel only)
    float tint = effect->mTint.x;

    // Optional premultiplied tint path (matches flag 0x8000 in original code)
    if (mFlags & 0x8000)
    {
        tint *= colour.w * colour.x * alpha;    // premultiplied by existing colour + alpha
    }
    else
    {
        tint *= colour.x;                       // simple multiply
    }

    tint *= effect->mBrightness;                // global brightness scale

    // Convert [0‑1] float into packed 0‑255 channel the renderer expects
	//k??? TODO r000 -> rrrr
	unsigned int c = (byte)(tint * 255.0f);
	c = c << 24 | c << 16 | c << 8 | c;
    return *(int *)&c;
#endif
}

//--------------------------------------------------------------------
void rvParticle::RenderQuadTrail(const rvBSE* effect,srfTriangles_s* tri, const idVec3& offset, float fraction, const idVec4& colour, const idVec3& pos,bool firstSegment)
{
    // Compute colour tint once for this segment
    const int packedTint = HandleTint(effect, colour, /*alpha*/1.0f);

    // Convenience references
    const int baseVert = tri->numVerts;
    idDrawVert* v = &tri->verts[baseVert];

    // First vertex (offset + position)
    v[0].xyz = pos + offset;
    v[0].st = idVec2(0.0f, fraction);
    *reinterpret_cast<uint32_t*>(v[0].color) = packedTint;

    // Second vertex (‑offset + position)
    v[1].xyz = pos - offset;
    v[1].st = idVec2(1.0f, fraction);
    *reinterpret_cast<uint32_t*>(v[1].color) = packedTint;
	BSE_SETUP_VERT_NORMAL(v[0], pos);
	BSE_SETUP_VERT_NORMAL(v[1], pos);

    // Connect to previous quad if not the first segment
    if (!firstSegment)
    {
        glIndex_t* idx = &tri->indexes[tri->numIndexes];
        idx[0] = baseVert - 2; idx[1] = baseVert - 1; idx[2] = baseVert;
        idx[3] = baseVert - 1; idx[4] = baseVert;     idx[5] = baseVert + 1;
        tri->numIndexes += 6;
    }

    tri->numVerts += 2;
}

//====================================================================
//  rvParticle::RenderMotion – legacy quad‑trail motion blur renderer
//====================================================================

void rvParticle::RenderMotion(rvBSE* effect,srfTriangles_s* tri,const renderEffect_s* owner,float time)
{
    if (mTrailCount == 0)
        return;

    // Compute delta time window this trail covers
    float startTime = Max(time - mTrailTime, mStartTime);
    float delta = time - startTime;
    if (delta <= kEpsilon)
        return; // nothing to draw

    //--------------------------------------------
    // Evaluate dynamic parameters at current time
    const float evalTime = time - mStartTime;

    idVec4 colour;
    mTintEnv.Evaluate(evalTime, &colour.x); // rgb
    mFadeEnv.Evaluate(evalTime, &colour.w); // alpha

    idVec3 size;          EvaluateSize(evalTime, size.ToFloatPtr());
    idVec3 position;      EvaluatePosition(effect, position, time - mMotionStartTime);

    //--------------------------------------------
    // Build orientation vectors to face camera
    idVec3 toView = effect->mViewOrg - owner->origin;
    toView = toView * owner->axis; // bring into local space

    // Axis perpendicular to motion vector (via cross product)
    idVec3 motionDir = BSE::Normalized((position - mInitPos).Cross(mVelocity));
    if (motionDir.LengthSqr() == 0.0f)
    {
        // fallback if velocity parallel – use world up
        motionDir = BSE::Normalized(idVec3(0, 0, 1).Cross(position - mInitPos));
    }

    const idVec3 halfWidth = motionDir * (size.x * kHalf);

    //--------------------------------------------
    // Emit segments along the trail (oldest → newest)
    for (int segment = 0; segment < mTrailCount; ++segment)
    {
        const float t = static_cast<float>(segment) / static_cast<float>(mTrailCount);
        colour.w = (1.f - t) * colour.w; // fade tail

        RenderQuadTrail(effect,
            tri,
            halfWidth,
            t,
            colour,
            position,
            /*firstSegment*/ segment == 0);

        // step backwards in time along the motion path
        const float sampleTime = time - t * delta - mMotionStartTime;
        EvaluatePosition(effect, position, sampleTime);
    }

    // final degenerate segment to fade to zero alpha
    if (mTrailCount)
    {
        colour.w = 0.0f;
        RenderQuadTrail(effect, tri, halfWidth, 1.0f, colour, position, /*first*/false);
    }
}

//====================================================================
//  rvSpriteParticle::Render – billboard sprite quad
//====================================================================
bool rvSpriteParticle::Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float alphaOverride)
{
    float eval = time - mStartTime;
    if (time >= mEndTime - kEpsilon) eval = (mEndTime - mStartTime) - kEpsilon;
    if (time < mStartTime - kEpsilon || time >= mEndTime) return false;

    // --- dynamic parameters ---
    idVec4 tint; mTintEnv.Evaluate(eval, &tint.x);
    mFadeEnv.Evaluate(eval, &tint.w);

    idVec3 size;  EvaluateSize(eval, size.ToFloatPtr());
    idVec3 angleV;  EvaluateRotation(eval, angleV.ToFloatPtr());
    float angle = angleV.x;

    idVec3 pos;   EvaluatePosition(effect, pos, time - mMotionStartTime);

    // optional per‑effect clamp
    if (effect->mSpriteSize.x > 0 || effect->mSpriteSize.y > 0)
        size = idVec3(effect->mSpriteSize.x, effect->mSpriteSize.y, 1.0f);

    // --- build oriented quad ---
    const float c = cos(angle), s = sin(angle);
    
#if 1 //k??? TODO Q4D original source code
	idVec3 left = -view[2] * s + view[1] * c;
	idVec3 up = view[2] * c + view[1] * s;

    idVec3 halfRight = left * (size.x);
    idVec3 halfUp = up * (size.y);

    const int packed = HandleTint(effect, tint, alphaOverride);
    const int base = tri->numVerts;
    idDrawVert* v = &tri->verts[base];

    // TL
    v[0].xyz = pos - halfRight; v[0].st = idVec2(0, 0);
    *reinterpret_cast<uint32_t*>(v[0].color) = packed;
    // TR
    v[1].xyz = pos - halfUp; v[1].st = idVec2(1, 0);
    *reinterpret_cast<uint32_t*>(v[1].color) = packed;
    // BR
    v[2].xyz = pos + halfRight; v[2].st = idVec2(1, 1);
    *reinterpret_cast<uint32_t*>(v[2].color) = packed;
    // BL
    v[3].xyz = pos + halfUp; v[3].st = idVec2(0, 1);
    *reinterpret_cast<uint32_t*>(v[3].color) = packed;
#else
	idVec3 right = view[2] * s - view[1] * c;
	idVec3 up = view[2] * c + view[1] * s;

    idVec3 halfRight = right * (size.x * kHalf);
    idVec3 halfUp = up * (size.y * kHalf);

    const int packed = HandleTint(effect, tint, alphaOverride);
    const int base = tri->numVerts;
    idDrawVert* v = &tri->verts[base];

    // TL
    v[0].xyz = pos - halfRight - halfUp; v[0].st = idVec2(0, 0);
    *reinterpret_cast<uint32_t*>(v[0].color) = packed;
    // TR
    v[1].xyz = pos + halfRight - halfUp; v[1].st = idVec2(1, 0);
    *reinterpret_cast<uint32_t*>(v[1].color) = packed;
    // BR
    v[2].xyz = pos + halfRight + halfUp; v[2].st = idVec2(1, 1);
    *reinterpret_cast<uint32_t*>(v[2].color) = packed;
    // BL
    v[3].xyz = pos - halfRight + halfUp; v[3].st = idVec2(0, 1);
    *reinterpret_cast<uint32_t*>(v[3].color) = packed;
#endif
	BSE_SETUP_VERT_NORMAL(v[0], pos);
	BSE_SETUP_VERT_NORMAL(v[1], pos);
	BSE_SETUP_VERT_NORMAL(v[2], pos);
	BSE_SETUP_VERT_NORMAL(v[3], pos);

    glIndex_t* idx = &tri->indexes[tri->numIndexes];
    idx[0] = base; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base; idx[4] = base + 2; idx[5] = base + 3;

    tri->numVerts += 4;
    tri->numIndexes += 6;
    return true;
}

//====================================================================
//  rvLineParticle::Render – camera‑facing ribbon / beam
//====================================================================
bool rvLineParticle::Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float alphaOverride)
{
    float eval = time - mStartTime;
    if (time >= mEndTime - kEpsilon) eval = (mEndTime - mStartTime) - kEpsilon;
    if (time < mStartTime - kEpsilon || time >= mEndTime) return false;

    // parameters
    idVec4 tint; mTintEnv.Evaluate(eval, &tint.x);
    mFadeEnv.Evaluate(eval, &tint.w);

    idVec3 halfWidthV; EvaluateSize(eval, halfWidthV.ToFloatPtr()); // size.x stores width
    float halfWidth = halfWidthV.x; // size.x stores width
    idVec3 len;      EvaluateLength(eval, &len);     // will be rotated below

    idVec3 pos;      EvaluatePosition(effect, pos, time - mMotionStartTime);

#if 1 //karin: don't transform if useEndOrigin
    if( ! (
            (pt && pt->mSpawnLength.mFlags & PPFLAG_USEENDORIGIN)
			|| (!pt && effect->mDeclEffect->mFlags & ETFLAG_USES_ENDORIGIN)
    ) ) //karin: useEndOrigin
#endif
    {
        // rotate len by current axis unless fixed flag set
        if (!(mFlags & PTFLAG_LOCKED/* 2 */)) len = mInitAxis * effect->mCurrentAxisTransposed/*//k??? TODO mCurrentAxis */ * len; // simplified – assumes operator*
    }

    // if velocity‑aligned flag, project onto velocity dir
    if (mFlags & PTFLAG_GENERATED_LINE/* 0x10000 */)
    {
        idVec3 vel; EvaluateVelocity(effect, vel, time - mMotionStartTime);
        float speedSq = vel.LengthSqr();
        if (speedSq != 0.0) vel /= sqrt(speedSq);
        float length = len.Length();
        len = vel * length;
    }

    // perpendicular vector to camera for ribbon thickness
    idVec3 viewRight = view[0];
#if 1 //k??? TODO Q4D
	idVec3 newPos = pos + len;
	idVec3 offset = viewRight - (len * 0.5f) + pos;

	idVec3 velocity = len.Cross(offset);
	offset = BSE::NormalizedSafely(velocity) * halfWidth;

    const int packed = HandleTint(effect, tint, alphaOverride);
    const int base = tri->numVerts;
    idDrawVert* v = &tri->verts[base];

	v[0].xyz = pos + offset; v[0].st = idVec2(0, 0);
	v[1].xyz = pos - offset; v[1].st = idVec2(0, 1);
	v[2].xyz = newPos - offset; v[2].st = idVec2(mTextureScale, 1);
	v[3].xyz = newPos + offset; v[3].st = idVec2(mTextureScale, 0);
#else
    idVec3 halfOffset = BSE::Normalized(viewRight.Cross(len)) * halfWidth;

    const int packed = HandleTint(effect, tint, alphaOverride);
    const int base = tri->numVerts;
    idDrawVert* v = &tri->verts[base];

    // four verts along ribbon ends
    v[0].xyz = pos + halfOffset; v[0].st = idVec2(0, 0);
    v[1].xyz = pos - halfOffset; v[1].st = idVec2(0, 1);
    v[2].xyz = pos + len - halfOffset; v[2].st = idVec2(mTextureScale, 1);
    v[3].xyz = pos + len + halfOffset; v[3].st = idVec2(mTextureScale, 0);
#endif
    for (int i = 0; i < 4; ++i) *reinterpret_cast<uint32_t*>(v[i].color) = packed;
	BSE_SETUP_VERT_NORMAL(v[0], pos);
	BSE_SETUP_VERT_NORMAL(v[1], pos);
	BSE_SETUP_VERT_NORMAL(v[2], pos);
	BSE_SETUP_VERT_NORMAL(v[3], pos);

    glIndex_t* idx = &tri->indexes[tri->numIndexes];
    idx[0] = base; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base; idx[4] = base + 2; idx[5] = base + 3;

    tri->numVerts += 4;
    tri->numIndexes += 6;
    return true;
}

//====================================================================
//  rvOrientedParticle::Render – quad oriented by per‑particle matrix
//====================================================================
bool rvOrientedParticle::Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float alphaOverride)
{
    float eval = time - mStartTime;
    if (time >= mEndTime - kEpsilon) eval = (mEndTime - mStartTime) - kEpsilon;
    if (time < mStartTime - kEpsilon || time >= mEndTime) return false;

    idVec4 tint; mTintEnv.Evaluate(eval, &tint.x);
    mFadeEnv.Evaluate(eval, &tint.w);

    idVec3 sizeV;   EvaluateSize(eval, sizeV.ToFloatPtr());
    float size = sizeV.x;
    idVec3 rot;   EvaluateRotation(eval, rot.ToFloatPtr());
    idVec3 pos;   EvaluatePosition(effect, pos, time - mMotionStartTime);

    idMat3 orient; rvAngles(rot).ToMat3(orient);
#if 1 //k??? TODO Q4D original source code, and it's correct
    idVec3 right = size * -orient[1];
    idVec3 up = size * orient[2];
#else
    idVec3 right = orient[0] * size * 0.5f;
    idVec3 up = orient[1] * size * 0.5f;
#endif

    const int packed = HandleTint(effect, tint, alphaOverride);
    const int base = tri->numVerts;
    idDrawVert* v = &tri->verts[base];

#if 1 //k??? TODO Q4D original source code, and it's correct
    v[0].xyz = pos - right; v[0].st = idVec2(0, 0);
    v[1].xyz = pos - up; v[1].st = idVec2(1, 0);
    v[2].xyz = pos + right; v[2].st = idVec2(1, 1);
    v[3].xyz = pos + up; v[3].st = idVec2(0, 1);
#else
    v[0].xyz = pos - right - up; v[0].st = idVec2(0, 0);
    v[1].xyz = pos + right - up; v[1].st = idVec2(1, 0);
    v[2].xyz = pos + right + up; v[2].st = idVec2(1, 1);
    v[3].xyz = pos - right + up; v[3].st = idVec2(0, 1);
#endif
    for (int i = 0; i < 4; ++i) *reinterpret_cast<uint32_t*>(v[i].color) = packed;
	BSE_SETUP_VERT_NORMAL(v[0], pos);
	BSE_SETUP_VERT_NORMAL(v[1], pos);
	BSE_SETUP_VERT_NORMAL(v[2], pos);
	BSE_SETUP_VERT_NORMAL(v[3], pos);

    glIndex_t* idx = &tri->indexes[tri->numIndexes];
    idx[0] = base; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base; idx[4] = base + 2; idx[5] = base + 3;

    tri->numVerts += 4;
    tri->numIndexes += 6;
    return true;
}

//====================================================================
//  rvLinkedParticle::Render – single segment of linked ribbon
//====================================================================
bool rvLinkedParticle::Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float alphaOverride)
{
    float eval = time - mStartTime;
    if (time >= mEndTime - kEpsilon) eval = (mEndTime - mStartTime) - kEpsilon;
    if (time < mStartTime - kEpsilon || time >= mEndTime) return false;

    idVec4 tint; mTintEnv.Evaluate(eval, &tint.x);
    mFadeEnv.Evaluate(eval, &tint.w);

    idVec3 halfWidthV; EvaluateSize(eval, halfWidthV.ToFloatPtr());
    float halfWidth = halfWidthV.x;
    idVec3 pos;      EvaluatePosition(effect, pos, time - mMotionStartTime);

    idVec3 up = view[1] * halfWidth;
    const int packed = HandleTint(effect, tint, alphaOverride);
    const int base = tri->numVerts;
    idDrawVert* v = &tri->verts[base];

    v[0].xyz = pos + up; v[0].st = idVec2(mFraction * mTextureScale, 0);
    v[1].xyz = pos - up; v[1].st = idVec2(mFraction * mTextureScale, 1);
	BSE_SETUP_VERT_NORMAL(v[0], pos);
	BSE_SETUP_VERT_NORMAL(v[1], pos);
    for (int i = 0; i < 2; ++i) *reinterpret_cast<uint32_t*>(v[i].color) = packed;

    if (base > 0)
    {
        glIndex_t* idx = &tri->indexes[tri->numIndexes];
        idx[0] = base - 2; idx[1] = base - 1; idx[2] = base;
        idx[3] = base - 1; idx[4] = base + 1;   idx[5] = base; //k??? TODO Q4D is -1, +1 0, Q4BSE is -1, 0, +1
        tri->numIndexes += 6;
    }
    tri->numVerts += 2;
    return true;
}

//====================================================================
//  rvModelParticle::Render – instanced static mesh (simplified)
//====================================================================
bool rvModelParticle::Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float alphaOverride)
{
    if (!mModel) return false;
    float eval = time - mStartTime;
    if (time >= mEndTime - kEpsilon) eval = (mEndTime - mStartTime) - kEpsilon;
    if (time < mStartTime - kEpsilon || time >= mEndTime) return false;

    idVec4 tint; mTintEnv.Evaluate(eval, &tint.x);
    mFadeEnv.Evaluate(eval, &tint.w);

    idVec3 scale; EvaluateSize(eval, scale.ToFloatPtr());
    idVec3 pos;   EvaluatePosition(effect, pos, time - mMotionStartTime);
    idVec3 rot;   EvaluateRotation(eval, rot.ToFloatPtr());

    idMat3 orient; rvAngles(rot).ToMat3(orient);

    // Build 4×4 matrix
    idMat4 m = BSE::FromMat3(orient * BSE::Scale(scale), pos);

    const uint32_t packed = HandleTint(effect, tint, alphaOverride);

    // We rely on helper to append model geometry transformed into tri
	BSE::AppendToSurface(mModel, tri, m, packed);
    return true;
}
