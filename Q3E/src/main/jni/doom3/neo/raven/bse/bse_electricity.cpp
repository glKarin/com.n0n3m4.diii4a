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

static const idVec3 idVec3_zAxis(0, 0, 1);
static const idVec3 idVec3_xAxis(1, 0, 0);

static ID_INLINE void  rvEP_BuildQuad(idDrawVert* v,
    const idVec3& centre,
    const idVec3& offset,
    float         vCoord,
    unsigned int  colour)
{
    // upper edge
    v[0].xyz = centre + offset;
    v[0].st.x = vCoord;
    v[0].st.y = 0.0f;
    *reinterpret_cast<unsigned*>(v[0].color) = colour;

    // lower edge
    v[1].xyz = centre - offset;
    v[1].st.x = vCoord;
    v[1].st.y = 1.0f;
    *reinterpret_cast<unsigned*>(v[1].color) = colour;
	BSE_SETUP_VERT_NORMAL(v[0], centre);
	BSE_SETUP_VERT_NORMAL(v[1], centre);
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::GetBoltCount                                        *|
\*─────────────────────────────────────────────────────────────────────────────*/
int rvElectricityParticle::GetBoltCount(float length)
{
    const int bolts = static_cast<int>(ceil(length * 0.0625f));
    return idMath::ClampInt(3, BSE_ELEC_MAX_BOLTS/* 200 */, bolts);
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::Update                                              *|
\*─────────────────────────────────────────────────────────────────────────────*/
int rvElectricityParticle::Update(rvParticleTemplate* pt, float time)
{
    const float elapsed = time - mStartTime;

    idVec3 len;
    EvaluateLength(elapsed, &len);

    mNumBolts = GetBoltCount(len.Length()); //k??? TODO LengthFast
    return mNumBolts;
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::RenderLineSegment                                   *|
\*─────────────────────────────────────────────────────────────────────────────*/
void rvElectricityParticle::RenderLineSegment(const rvBSE* effect,SElecWork* work,const idVec3& start, float         startFraction)
{
    idVec3 left = work->length.Cross(work->viewPos);
#if 1 //k??? TODO same logical
	BSE::NormalizeSafely(left);
#else
    float  len2 = left.LengthSqr();
    if (len2 > 1e-6f) {
        left *= idMath::InvSqrt(len2); //k??? TODO
    }
#endif
    left *= work->size; //k??? TODO always scale by size in Q4D

    unsigned colour = rvParticle::HandleTint(effect, work->tint, work->alpha);

    idDrawVert* v = &work->tri->verts[work->tri->numVerts];
    rvEP_BuildQuad(v, start, left, startFraction * work->step + work->fraction, colour);

    work->tri->numVerts += 2;
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::ApplyShape                                          *|
\*─────────────────────────────────────────────────────────────────────────────*/
void rvElectricityParticle::ApplyShape(const rvBSE* effect,
    SElecWork* work,
    const idVec3& start,
    const idVec3& end,
    int           recurse,
    float         startFrac,
    float         endFrac)
{
    if (recurse <= 0) {
        RenderLineSegment(effect, work, start, startFrac);
        return;
    }

    /* -------- randomised split parameters (mimic original tables) -------- */
    const float randA = rvRandom::flrand(0.05f, 0.09f);
    const float randB = rvRandom::flrand(0.05f, 0.09f);
    const float shape = rvRandom::flrand(0.56f, 0.76f);
    
    const idVec3 dir = end - start;
    const float  length = dir.Length() * 0.70f; //k??? TODO LengthFast
    const idVec3 forward = BSE::Normalized(dir);
#if 1 //k??? TODO Q4D original source code
    float v16 = forward.y * forward.y + forward.x * forward.x;
    idVec3 nor(0.0f, 0.0f, 0.0f); // left
    float left, left_4;
    if(v16 == 0.0f)
    {
        left = 1.0f;
        left_4 = 0.0f;
    }
    else
    {
        float v17 = 1.0f / sqrt(v16);
        left = -(forward.y * v17);
        left_4 = v17 * forward.x;
    }
    nor.y = left;
    nor.z = left_4;

    idVec3       down = nor.Cross(forward);
    if (down.LengthSqr() < 1e-6f) {          // forward ≈ Z, choose arbitrary axis
        down = idVec3_xAxis;
    }
    down.Normalize();

    /* -------- generate two perturbed points along the segment -------- */
    const float len1 = rvRandom::flrand(-randA - 0.02f, 0.02f - randA) * length;
    const float len2 = rvRandom::flrand(-randB - 0.02f, 0.02f - randB) * length;

    const idVec3 point1 = down * len2 + nor * len1 + start * shape + end * (1.0f - shape);

    const float t2 = rvRandom::flrand(0.23f, 0.43f);
    const idVec3 point2 = down * len2 + nor * len1 + start * t2 + end * (1.0f - t2);
#else
    idVec3       left = forward.Cross(idVec3_zAxis);
    if (left.LengthSqr() < 1e-6f) {          // forward ≈ Z, choose arbitrary axis
        left = idVec3_xAxis;
    }
    left.Normalize();
    const idVec3 down = forward.Cross(left);

    /* -------- generate two perturbed points along the segment -------- */
    const float len1 = rvRandom::flrand(-randA - 0.02f, 0.02f - randA) * length;
    const float len2 = rvRandom::flrand(-randB - 0.02f, 0.02f - randB) * length;

    const idVec3 point1 = start * shape + end * (1.0f - shape) +
        left * len1 + down * rvRandom::flrand(0.23f, 0.43f) * length;

    const float t2 = rvRandom::flrand(0.23f, 0.43f);
    const idVec3 point2 = start * t2 + end * (1.0f - t2) +
        left * len2 + down * rvRandom::flrand(-0.02f, 0.02f) * length;
#endif

    const float mid0 = startFrac * 0.6666667f + endFrac * 0.3333333f;
    const float mid1 = startFrac * 0.3333333f + endFrac * 0.6666667f;

    ApplyShape(effect, work, start, point1, recurse - 1, startFrac, mid0);
    ApplyShape(effect, work, point1, point2, recurse - 1, mid0, mid1);
    ApplyShape(effect, work, point2, end, recurse - 1, mid1, endFrac);
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::RenderBranch                                        *|
\*─────────────────────────────────────────────────────────────────────────────*/
void rvElectricityParticle::RenderBranch(const rvBSE* effect,
    SElecWork* work,
    const idVec3& start,
    const idVec3& end)
{
#if 1 //k??? TODO Q4D original source code
    float left;
    float left_4;

	BSE::NormalizeSafely(work->forward);
    float v8 = work->forward.x * work->forward.x + work->forward.y * work->forward.y;
    idVec3 nor(0.0f, 0.0f, 0.0f); // left
    if ( v8 == 0.0f )
    {
        left = 1.0f;
        left_4 = 0.0f;
    }
    else
    {
        float v9 = 1.0f / sqrt(v8);
        left = -(v9 * work->forward.y);
        left_4 = v9 * work->forward.x;
    }
    nor.x = left;
    nor.y = left_4;
    idVec3 down = nor.Cross(work->forward);

    float fraction = work->step;
    idVec3 old = start;
    int eval = 1;
    int count = 0;
    idVec3 *coords = work->coords;
    float x = start.x;
    old.x = start.x;
    idVec3 len = end - start;
    idVec3 current;
    int numVerts = work->tri->numVerts;
    while(1)
    {
        ++count;
        coords->x = x;
        coords->y = old.y;
        idVec3 *v45 = coords + 1;
        if ( 1.0f - work->step * 0.5f <= fraction )
        {
            fraction = 1.0f;
            eval = 0;
        }

        const float noise = mJitterTable ? mJitterTable->TableLookup(fraction) : 0.0f;
        idVec3 jitter(
                rvRandom::flrand(-mJitterSize.x, mJitterSize.x),
                rvRandom::flrand(-mJitterSize.y, mJitterSize.y),
                rvRandom::flrand(-mJitterSize.z, mJitterSize.z));

        idVec3 v394041 = jitter.x * work->forward;
        idVec3 v4950 = jitter.y * nor;
        idVec3 v5556 = jitter.z * down;

        idVec3 offset = v394041 + v4950 + v5556;
        const idVec3 pos = start + len * fraction;
        current = offset * noise + pos;

        work->fraction = fraction - work->step;

        ApplyShape(effect, work, old, current, 2, 0.0f, 1.0f);

        old = current;
        fraction = fraction + work->step;

        if ( !eval )
            break;
        coords = v45;
    }

    srfTriangles_s *tri;
    if ( numVerts != work->tri->numVerts )
    {
        idVec3 *v17 = &work->coords[count];
        v17->y = current.y;
        v17->x = current.x;
        v17->z = current.z;
        RenderLineSegment(effect, work, current, 1.0);
        tri = work->tri;
        int numIndexes = tri->numIndexes;
        while ( numVerts < tri->numVerts - 2 )
        {
            work->tri->indexes[numIndexes] = numVerts;
            work->tri->indexes[numIndexes + 1] = numVerts + 1;
            work->tri->indexes[numIndexes + 2] = numVerts + 3;
            work->tri->indexes[numIndexes + 3] = numVerts;
            work->tri->indexes[numIndexes + 4] = numVerts + 3;
            work->tri->indexes[numIndexes + 5] = numVerts + 2;
            numVerts += 2;
            numIndexes += 6;
        }
        work->tri->numIndexes = numIndexes;
    }
#else
    idVec3 forward = end - start;
    const float len = BSE::NormalizeSafely(forward);
    if (len < 1e-6f) return;

    // Build an orthonormal basis (left, up) around the branch direction.
    idVec3 left = (fabs(forward.x) < 0.99f) ?
        Normalized(idVec3(0, 0, 1).Cross(forward)) :
        Normalized(idVec3(0, 1, 0).Cross(forward));
    idVec3 up = forward.Cross(left);

    /* Pre-allocate a temporary coordinate buffer on the stack.
       The caller (Render) guarantees enough space. */
    int   segVertStart = work->tri->numVerts;
    int   outCount = 0;
    float frac = 0.0f;
    idVec3 current = start;

    work->coords[outCount++] = current;

    while (frac < 1.0f - work->step * 0.5f) {
        frac += work->step;

        /* sample 1-D noise table to get smoothly varying offset weight */
        const float noise = mJitterTable ? mJitterTable->TableLookup(frac) : 0.0f;

        idVec3 jitter(
            rvRandom::flrand(-mJitterSize.x, mJitterSize.x),
            rvRandom::flrand(-mJitterSize.y, mJitterSize.y),
            rvRandom::flrand(-mJitterSize.z, mJitterSize.z));

        /* decompose jitter across the local basis */
        idVec3 offset = forward * jitter.x + left * jitter.y + up * jitter.z;

        current = start + forward * (len * frac) + offset * noise;
        work->coords[outCount++] = current;
    }

    work->coords[outCount++] = end;

    /* ------------------------------------------------------------------ */
    /* Build quads + indices                                              */
    /* ------------------------------------------------------------------ */
    float vCoord = 0.0f;
    for (int i = 0; i < outCount - 1; ++i) {
        RenderLineSegment(effect, work, work->coords[i], vCoord);
        vCoord += work->step;
    }

    // Turn the generated vertex strip into a solid ribbon.
    for (int base = segVertStart;
        base < work->tri->numVerts - 2;
        base += 2)
    {
        const int idx = work->tri->numIndexes;
        work->tri->indexes[idx + 0] = base;
        work->tri->indexes[idx + 1] = base + 1;
        work->tri->indexes[idx + 2] = base + 2;
        work->tri->indexes[idx + 3] = base;
        work->tri->indexes[idx + 4] = base + 2;
        work->tri->indexes[idx + 5] = base + 3;
        work->tri->numIndexes += 6;
    }
#endif
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::SetupElectricity                                    *|
\*─────────────────────────────────────────────────────────────────────────────*/
void rvElectricityParticle::SetupElectricity(const rvParticleTemplate* pt)
{
    mNumForks = pt->mNumForks;
    mSeed = rvRandom::Init();
    mForkSizeMins = pt->mForkSizeMins;
    mForkSizeMaxs = pt->mForkSizeMaxs;
    mJitterSize = pt->mJitterSize;
    mLastJitter = 0.0f;
    mJitterRate = pt->mJitterRate;
    mJitterTable = pt->mJitterTable;
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::Render                                              *|
\*─────────────────────────────────────────────────────────────────────────────*/
bool rvElectricityParticle::Render(const rvBSE* effect,
    const rvParticleTemplate* pt,
    const idMat3& view,
    srfTriangles_s* tri,
    float           time,
    float           overrideAlpha)
{
    /* ───── lifetime / env-parm evaluation ───── */
    float evalTime;
    if (!rvParticle::GetEvaluationTime(time, evalTime, false))
        return false;

    SElecWork work;
    memset(&work, 0, sizeof(work));
    mTintEnv.Evaluate(evalTime, &work.tint.x);
    mFadeEnv.Evaluate(evalTime, &work.tint.w);

    (this->*mEvalSizePtr)(evalTime, &work.size);
    (this->*mEvalLengthPtr)(evalTime, &work.length);

    /* ───── transform to effect-space if requested ───── */
    if (!(mFlags & PTFLAG_LOCKED/* 2 */)) {
        work.length = effect->mCurrentAxis * (mInitAxis * work.length);
    }

    /* ───── velocity-driven length (optional) ───── */
    if (mFlags & PTFLAG_GENERATED_LINE/* 0x10000 */) {
        idVec3 vel;
        rvParticle::EvaluateVelocity(effect,
            vel, time - mMotionStartTime);
		BSE::NormalizeSafely(vel);
        const float len = work.length.Length(); //k??? TODO LengthFast
        work.length = vel * len;
    }

    const float mainLength = work.length.Length(); //k??? TODO LengthFast
    if (mainLength < 0.1f)
        return false;

    /* ───── jitter reseeding ───── */
    unsigned int seed = rvRandom::irand(0, 0x7FFF);
    if (mLastJitter + mJitterRate <= time) {
        mLastJitter = time;
        //k??? TODO always setup mSeed = rvRandom::Init();
    }
	mSeed = seed;
    unsigned int restoreSeed = rvRandom::Seed();
    rvRandom::Init(mSeed);

    /* ───── per-frame scratch initialisation ───── */
    work.viewPos.x = view[0].x;
    work.viewPos.y = view[0].y;
    work.viewPos.z = view[0].z;

    work.tri = tri;
    work.alpha = overrideAlpha;
    work.forward = work.length;
    work.step = mTextureScale / static_cast<float>(mNumBolts);

    /* Co-ordinates buffer for this branch (main + forks share the same stack) */
    /*//k??? TODO unused idVec3 tmpCoords[256];
    work.coords = tmpCoords;*/

    /* ───── evaluate particle origin in world-space ───── */
    idVec3 position;
    rvParticle::EvaluatePosition(effect,
        position, time - mMotionStartTime);

    const idVec3 endPos = position + work.length;

    /* ───── main bolt ───── */
    RenderBranch(effect, &work, position, endPos);

    /* ───── build list of fork start points ───── */
    idVec3 forkBases[BSE_MAX_FORKS/* 16 */];
    for (int i = 0; i < mNumForks; ++i) {
        const int idx = rvRandom::irand(1, mNumBolts - 1);
        forkBases[i] = work.coords[idx];
    }

    /* ───── render forks ───── */
    for (int i = 0; i < mNumForks; ++i)
    {
        /* choose a point roughly halfway between fork-base and main end */
        const idVec3 mid = (forkBases[i] + endPos) * 0.5f;

        const idVec3 forkEnd = mid + idVec3(
            rvRandom::flrand(mForkSizeMins.x, mForkSizeMaxs.x),
            rvRandom::flrand(mForkSizeMins.y, mForkSizeMaxs.y),
            rvRandom::flrand(mForkSizeMins.z, mForkSizeMaxs.z));

        idVec3 dir = forkEnd - forkBases[i];
#if 1 //k??? TODO Q4D original source code
        work.forward = dir;
        const float forkLen = dir.Length(); //k??? TODO LengthFast
        if (forkLen > 1.0f && forkLen < mainLength)
        {
            const int forkBolts = GetBoltCount(forkLen);

            work.step = 1.0f / static_cast<float>(forkBolts);
            RenderBranch(effect, &work, forkBases[i], forkEnd);
        }
#else
        const float forkLen = dir.LengthFast();
        if (forkLen < 1.0f || forkLen >= mainLength)
            continue;

        const int forkBolts = GetBoltCount(forkLen);

        work.forward = dir;
        work.step = 1.0f / static_cast<float>(forkBolts);
        RenderBranch(effect, &work, forkBases[i], forkEnd);
#endif
    }

    /* ───── restore global RNG state ───── */
    rvRandom::Init(restoreSeed);
    return true;
}
