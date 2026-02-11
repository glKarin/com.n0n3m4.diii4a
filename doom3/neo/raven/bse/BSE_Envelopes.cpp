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

int rvEnvParms::GetType() const
{
    return (mTable && mTable->base) ? mTable->Index() : 0;
}

bool rvEnvParms::GetMinMax(float& lo, float& hi) const
{
    if (mTable) {
        lo = mTable->GetMinValue();
        hi = mTable->GetMaxValue();
        return true;
    }
    lo = hi = 0.f;
    return false;
}

void rvEnvParms::SetDefaultType()
{
    if (!mTable) {
        // *linear* is the canonical default in idTech effects code
        mTable = static_cast<const idDeclTable*>(
            declManager->FindType(DECL_TABLE, "linear", true));
        mIsCount = true;
    }
}

void rvEnvParms::Init()
{
    mTable = NULL;
    mIsCount = true;
    // subclasses will reset their own vectors/scalars
    mEnvOffset.Set(0.0f, 0.0f, 0.0f);
    mRate.Set(1.0f, 1.0f, 1.0f);
}

void rvEnvParms::CalcRate(float* outRate,
    float duration,
    int count,
    const float* srcRate) const
{
    for (int i = 0; i < count; ++i)
        outRate[i] = mIsCount ? srcRate[i] / duration : srcRate[i];
}

void rvEnvParms::Evaluate3(float time,
    const float* start,
    const float* rate,
    const float* end,
    float* dest) const
{
    if (mTable) {
        const float t0 = time * rate[0] + 0.f;          // envOffset.x==0 here
        const float t1 = time * rate[1] + 0.f;
        const float t2 = time * rate[2] + 0.f;
        dest[0] = (end[0] - start[0]) * TableSample(mTable, t0) + start[0];
        dest[1] = (end[1] - start[1]) * TableSample(mTable, t1) + start[1];
        dest[2] = (end[2] - start[2]) * TableSample(mTable, t2) + start[2];
    }
    else {
        dest[0] = start[0]; dest[1] = start[1]; dest[2] = start[2];
    }
}

bool rvEnvParms::Compare(const rvEnvParms& rhs) const
{
    return mTable == rhs.mTable && mIsCount == rhs.mIsCount;
}

// -----------------------------------------------------------------------------
//  rvEnvParms1
// -----------------------------------------------------------------------------
void rvEnvParms1::Init(const rvEnvParms& copy, float duration)
{
    mTable = copy.mTable;
    mEnvOffset = copy.mEnvOffset.x;
    if (copy.mIsCount)
        mRate = copy.mRate.x / duration;
    else
        mRate = copy.mRate.x;
}

void rvEnvParms1::Evaluate(float time, float* dest) const
{
    if (mTable) {
        float v = time * mRate + mEnvOffset;
        *dest = (mEnd - mStart) * TableSample(mTable, v) + mStart;
    }
    else {
        *dest = mStart;
    }
}

// -----------------------------------------------------------------------------
//  rvEnvParms2
// -----------------------------------------------------------------------------
void rvEnvParms2::Init(const rvEnvParms& src, float duration)
{
    mTable = src.mTable;
    mEnvOffset = idVec2(src.mEnvOffset.x, src.mEnvOffset.y);
    if (src.mIsCount) {
        const float invDur = 1.f / duration;
        mRate.x = src.mRate.x * invDur;
        mRate.y = src.mRate.y * invDur;
    }
    else {
        mRate = idVec2(src.mRate.x, src.mRate.y);
    }
    mFixedRateAndOffset = (mRate.x == mRate.y) &&
        (mEnvOffset.x == mEnvOffset.y);
}

void rvEnvParms2::Evaluate(float time, float* dest) const
{
    if (!mTable) { dest[0] = mStart.x; dest[1] = mStart.y; return; }

    const float tx = time * mRate.x + mEnvOffset.x;
    if (mFixedRateAndOffset) {
        const float s = TableSample(mTable, tx);
        dest[0] = (mEnd.x - mStart.x) * s + mStart.x;
        dest[1] = (mEnd.y - mStart.y) * s + mStart.y;
    }
    else {
        const float ty = time * mRate.y + mEnvOffset.y;
        dest[0] = (mEnd.x - mStart.x) * TableSample(mTable, tx) + mStart.x;
        dest[1] = (mEnd.y - mStart.y) * TableSample(mTable, ty) + mStart.y;
    }
}

// -----------------------------------------------------------------------------
//  rvEnvParms3 (adds to earlier snippet)
// -----------------------------------------------------------------------------
void rvEnvParms3::Init(const rvEnvParms& src, float duration)
{
    mTable = src.mTable;
    mEnvOffset = src.mEnvOffset;
    if (src.mIsCount) {
        const float invDur = 1.f / duration;
        mRate = src.mRate * invDur;
    }
    else {
        mRate = src.mRate;
    }
    mFixedRateAndOffset = (mRate.x == mRate.y) && (mRate.x == mRate.z) &&
        (mEnvOffset.x == mEnvOffset.y) &&
        (mEnvOffset.x == mEnvOffset.z);
}

void rvEnvParms3::Evaluate(float time, float* dest) const
{
    if (!mTable) {                         // constant
        dest[0] = mStart.x; dest[1] = mStart.y; dest[2] = mStart.z; return;
    }

    const float tx = time * mRate.x + mEnvOffset.x;
    if (mFixedRateAndOffset) {
        const float s = TableSample(mTable, tx);
        dest[0] = (mEnd.x - mStart.x) * s + mStart.x;
        dest[1] = (mEnd.y - mStart.y) * s + mStart.y;
        dest[2] = (mEnd.z - mStart.z) * s + mStart.z;
    }
    else {
        const float ty = time * mRate.y + mEnvOffset.y;
        const float tz = time * mRate.z + mEnvOffset.z;
        dest[0] = (mEnd.x - mStart.x) * TableSample(mTable, tx) + mStart.x;
        dest[1] = (mEnd.y - mStart.y) * TableSample(mTable, ty) + mStart.y;
        dest[2] = (mEnd.z - mStart.z) * TableSample(mTable, tz) + mStart.z;
    }
}

void rvEnvParms3::Scale(float k) { mStart *= k;  mEnd *= k; }
void rvEnvParms3::Transform(const idVec3& n)
{
    const idMat3 m = n.ToMat3();
    mStart = m * mStart;
    mEnd = m * mEnd;
}
void rvEnvParms3::Rotate(const idAngles& a)
{
    mStart += a.ToAngularVelocity();   // simple additive rotation (idTech idiom)
    mEnd += a.ToAngularVelocity();
}