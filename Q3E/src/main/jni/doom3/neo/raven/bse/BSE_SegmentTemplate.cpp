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

In addition, the QUAKE 4 BSE CODE RECREATION EFFORT is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the QUAKE 4 BSE CODE RECREATION EFFORT.  

If you have questions concerning this license or the applicable additional terms, you may contact in writing justinmarshall20@gmail.com 

===========================================================================
*/

#include "BSE.h"

#include "BSE_Compat.h"

/* --------------------------------------------------------------------- */
void rvSegmentTemplate::Init(rvDeclEffect* decl)
{
    mDeclEffect = decl;
    mFlags = STFLAG_ENABLED; // 1 //k??? TODO Q4BSE is STFLAG_INFINITE_DURATION;          // default “locked” bit off
    mSegType = SEG_NONE; // 0 //k??? TODO Q4BSE is SEG_INVALID;

    mLocalStartTime.Zero();
    mLocalDuration.Zero();
    mAttenuation.Zero();

    mParticleCap = 0.0f;
    mDetail = 0.0f;
    mScale = 1.0f;

    mCount.Set(1.0f, 1.0f);
    mDensity.Zero();

    mTrailSegmentIndex = -1;
    mNumEffects = 0;
    memset(mEffects, 0, sizeof(mEffects));

    mSoundShader = NULL;
    mSoundVolume.Zero();
    mFreqShift.Set(1.0f, 1.0f);

    mParticleTemplate.Init();
    mBSEEffect = NULL;

    mSegmentName = "";
}

/* --------------------------------------------------------------------- */
void rvSegmentTemplate::CreateParticleTemplate(rvDeclEffect* effect,
    idLexer* lexer,
    int           particleType)
{
    mParticleTemplate.Init();
    mParticleTemplate.mType = particleType;
    mParticleTemplate.SetParameterCounts();
    mParticleTemplate.Parse(effect, lexer);
}

/* --------------------------------------------------------------------- */
int rvSegmentTemplate::GetTexelCount() const
{
    if (mParticleTemplate.mMaterial) {
        return BSE::GetTexelCount(mParticleTemplate.mMaterial); // mParticleTemplate.mMaterial->GetTexelCount(); //k??? TODO not implement in idMaterial and idImage
    }
    return 0;
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::GetSmoker() const
{
    return mParticleTemplate.mTrailType == TRAIL_PARTICLE; // 3;     // ‘smoker’ trail
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::GetSoundLooping() const
{
    return mSoundShader && (mSoundShader->GetParms()->soundShaderFlags & SSF_LOOPING);
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::DetailCull() const
{
    return (mDetail > 0.0f) &&
        (bse_scale.GetFloat() < mDetail);    // cvar comparison
}

float rvSegmentTemplate::CalculateBounds() const
{
    const rvParticleTemplate* p_mParticleTemplate; // esi
    int mType; // eax
    float result; // st7
    float maxLength; // [esp+0h] [ebp-Ch]
    float maxDist; // [esp+4h] [ebp-8h]
    float maxSize; // [esp+8h] [ebp-4h]

    switch (mSegType)
    {
	case SEG_EMITTER: // 2:
	case SEG_SPAWNER: // 3:
	case SEG_LIGHT: // 7:
        p_mParticleTemplate = &mParticleTemplate;
        maxSize = mParticleTemplate.GetMaxParmValue(
            &mParticleTemplate.mSpawnSize,
            &mParticleTemplate.mDeathSize,
            &mParticleTemplate.mSizeEnvelope);
        maxDist = p_mParticleTemplate->GetFurthestDistance();
        mType = p_mParticleTemplate->mType;
        if (mType == PTYPE_LINE/* 2 */ || mType == PTYPE_ELECTRIC/* 7 */)
            maxLength = p_mParticleTemplate->GetMaxParmValue(
                &p_mParticleTemplate->mSpawnLength,
                &p_mParticleTemplate->mDeathLength,
                &p_mParticleTemplate->mLengthEnvelope);
        else
            maxLength = 0.0;
        result = p_mParticleTemplate->GetMaxParmValue(
            &p_mParticleTemplate->mSpawnOffset,
            &p_mParticleTemplate->mDeathOffset,
            &p_mParticleTemplate->mOffsetEnvelope)
            + maxLength
            + maxDist
            + maxSize;
        break;
	case SEG_DECAL: // 6:
        result = mParticleTemplate.GetMaxParmValue(
            &mParticleTemplate.mSpawnSize,
            &mParticleTemplate.mDeathSize,
            &mParticleTemplate.mSizeEnvelope);
        break;
    default:
        result = 8.0;
        break;
    }
    return result;
}

/* --------------------------------------------------------------------- */
void rvSegmentTemplate::SetMaxDuration(rvDeclEffect* effect)
{
    if (!(mFlags & STFLAG_COMPLEX)) {
        effect->SetMaxDuration(mLocalStartTime.x + mLocalDuration.x);

        if (mParticleTemplate.mType != 0) {
            effect->SetMaxDuration(mLocalStartTime.x + mLocalDuration.x + mParticleTemplate.mDuration.y);
        }
    }
}

/* --------------------------------------------------------------------- */
void rvSegmentTemplate::SetMinDuration(rvDeclEffect* effect)
{
    if ((mFlags & STFLAG_COMPLEX) != 0)
        return;

    if (!mSoundShader || !(mSoundShader->GetParms()->soundShaderFlags & SSF_LOOPING)) {
        effect->SetMinDuration(mLocalStartTime.x + mLocalDuration.x);
    }
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::Compare(const rvSegmentTemplate& a) const
{
    /* cheap reject ------------------------------------------------------ */
    if (mSegmentName.Icmp(a.mSegmentName) != 0)
        return false;

    if (((mFlags ^ a.mFlags) & ~STFLAG_LOCKED) != 0)
        return false;

    if (mSegType != a.mSegType)
        return false;

    /* timeline ---------------------------------------------------------- */
    if (mSegType != SEG_TRAIL/* 4 */) {   // sound segs ignore timings //k??? TODO Q4D is 4, Q4BSE is SEG_SOUND
        if (mLocalStartTime != a.mLocalStartTime ||
            mLocalDuration != a.mLocalDuration)
            return false;
    }

    /* scale/detail/attenuation ----------------------------------------- */
    if (mScale != a.mScale ||
        mDetail != a.mDetail ||
        mAttenuation != a.mAttenuation)
        return false;

    /* count vs density variant ----------------------------------------- */
    if (mDensity.y == 0.0f) {
        if (mCount != a.mCount)
            return false;
    }
    else {
        if (mDensity != a.mDensity ||
            mParticleCap != a.mParticleCap)
            return false;
    }

    /* trail / effect list ---------------------------------------------- */
    if (mTrailSegmentIndex != a.mTrailSegmentIndex)
        return false;

    if (mNumEffects != a.mNumEffects)
        return false;

    for (int i = 0; i < mNumEffects; ++i) {
        if (mEffects[i] != a.mEffects[i])
            return false;
    }

    /* sound / freq ------------------------------------------------------ */
    if (mSoundShader != a.mSoundShader ||
        mSoundVolume != a.mSoundVolume ||
        mFreqShift != a.mFreqShift)
        return false;

    /* particle template deep compare ----------------------------------- */
    return mParticleTemplate.Compare(a.mParticleTemplate);
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::Finish(rvDeclEffect* effect)
{
    // 1) Ensure each “.x/.y” range has min ≤ max (manual swap, no std::swap)
    if (mLocalStartTime.x > mLocalStartTime.y) {
        float tmp = mLocalStartTime.x;
        mLocalStartTime.x = mLocalStartTime.y;
        mLocalStartTime.y = tmp;
    }
    if (mLocalDuration.x > mLocalDuration.y) {
        float tmp = mLocalDuration.x;
        mLocalDuration.x = mLocalDuration.y;
        mLocalDuration.y = tmp;
    }
    if (mCount.x > mCount.y) {
        float tmp = mCount.x;
        mCount.x = mCount.y;
        mCount.y = tmp;
    }
    if (mDensity.x > mDensity.y) {
        float tmp = mDensity.x;
        mDensity.x = mDensity.y;
        mDensity.y = tmp;
    }
    if (mAttenuation.x > mAttenuation.y) {
        float tmp = mAttenuation.x;
        mAttenuation.x = mAttenuation.y;
        mAttenuation.y = tmp;
    }

    // 2) Finish nested particle template, if any
    if (mParticleTemplate.mType) {
        mParticleTemplate.Finish();
        // clear the “intermediate-trail” bit (third byte)
        mParticleTemplate.mFlags &= ~(0x08 << 16); // 0x080000
    }

    // 3) Segment‐type‐specific setup
    switch (mSegType) {
	case SEG_EMITTER: // 2: // single‐shot
        mFlags |= STFLAG_HASPARTICLES; // 0x04;
        if (!mParticleTemplate.mType) return false;
        if (mFlags & STFLAG_INFINITE_DURATION/* 0x20 */) return false;
        break;

	case SEG_SPAWNER: // 3: // conditional
        mFlags |= STFLAG_HASPARTICLES; // 0x04;
        if (!mParticleTemplate.mType) return false;
        break;

	case SEG_TRAIL: // 4: // burst
        mFlags |= STFLAG_HASPARTICLES; // 0x04;
        // reset any local time/duration
        mLocalStartTime.x = mLocalStartTime.y = 0.0f;
        mLocalDuration.x = mLocalDuration.y = 0.0f;
        if (!mParticleTemplate.mType) return false;
        // set the “final-trail” bit
        mParticleTemplate.mFlags |= (0x08 << 16); // 0x080000
        break;

	case SEG_SOUND: // 5: // continuous
        mFlags |= STFLAG_IGNORE_DURATION; // 0x10;
        break;

	case SEG_DECAL: // 6: // delayed
        mFlags &= ~STFLAG_HASPARTICLES; // ~0x04;    // clear active
        mFlags |= STFLAG_TEMPORARY; // 0x0100;  // set byte1 bit0
        break;

	case SEG_DV: // 9:  // attenuation-only
	case SEG_SHAKE: // 10:
	case SEG_TUNNEL: // 11:
        if (mAttenuation.y > 0.0f) {
            mFlags |= STFLAG_ATTENUATE_EMITTER; // 0x40;
        }
        // fall through to default
    default:
        mFlags &= ~STFLAG_HASPARTICLES; // ~0x04;
        break;
    }

    // 4) Common post-setup logic
    if (mParticleTemplate.mType == PTYPE_DEBRIS/* 9 */) {
        mFlags &= ~STFLAG_HASPARTICLES; // ~0x04;
        mFlags |= STFLAG_TEMPORARY; // 0x0100;
    }

    if ((mFlags & STFLAG_INFINITE_DURATION/* 0x20 */) ||
        mParticleTemplate.mTrailType == TRAIL_PARTICLE/* 3 */ ||
        (mParticleTemplate.mFlags & 0x0200 ) ||
        mParticleTemplate.mNumTimeoutEffects) {
        mFlags |= 0x0200;  // set byte1 bit1
    }

    if (mParticleTemplate.mType == PTYPE_LIGHT/* 6 */ || mParticleTemplate.mType == PTYPE_ELECTRIC/* 7 */) {
        mFlags |= 0x0200; // BYTE1
    }

    return true;
}

/* --------------------------------------------------------------------- */
void rvSegmentTemplate::EvaluateTrailSegment(rvDeclEffect* et)
{
#if 0 //k??? TODO no entityFilter
    if (mParticleTemplate.mTrailTypeName.Icmp(entityFilter) != 0 &&
        mParticleTemplate.mTrailType != 0)
    {
        mTrailSegmentIndex = et->GetTrailSegmentIndex(mParticleTemplate.mTrailTypeName);
    }
#else
    if (mParticleTemplate.mTrailType != 0)
    {
        mTrailSegmentIndex = et->GetTrailSegmentIndex(mParticleTemplate.mTrailTypeName);
    }
#endif
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::Parse(rvDeclEffect* effect,
    int            segmentType,
    idLexer* lexer)
{
    idToken token;
    mSegType = segmentType;

    /* -------- optional explicit segment name -------------------------- */
    if (!lexer->ReadToken(&token))
        return false;

    if (token.Icmp("{") != 0) {
        mSegmentName = token.c_str();
    }
    else {
        mSegmentName = va("unnamed%d", effect->mSegmentTemplates.Num());
        lexer->UnreadToken(&token);                // put the “{” back
    }

    /* -------- open brace ------------------------------------------------ */
    if (!lexer->ExpectTokenString("{"))
        return false;

    /* -------- parameter loop ------------------------------------------- */
    while (lexer->ReadToken(&token))
    {
        if (token == "}")
            break;

        /* --- numeric pairs -------------------------------------------- */
#       define READ_VEC2( dst )                 \
            do {                                \
                (dst).x = lexer->ParseFloat();  \
                lexer->ExpectTokenString( "," );\
                (dst).y = lexer->ParseFloat();  \
            } while(0)

        /* --- dispatch table ------------------------------------------- */
        if (token.Icmp("count") == 0 ||
            token.Icmp("rate") == 0) {
            READ_VEC2(mCount);
        }
        else if (token.Icmp("density") == 0) { READ_VEC2(mDensity); }
        else if (token.Icmp("particleCap") == 0) { mParticleCap = lexer->ParseFloat(); }
        else if (token.Icmp("start") == 0) { READ_VEC2(mLocalStartTime); }
        else if (token.Icmp("duration") == 0) { READ_VEC2(mLocalDuration); }
        else if (token.Icmp("detail") == 0) { mDetail = lexer->ParseFloat(); }
        else if (token.Icmp("scale") == 0) { mScale = lexer->ParseFloat(); }
        else if (token.Icmp("attenuation") == 0) { READ_VEC2(mAttenuation); }
        else if (token.Icmp("attenuateEmitter") == 0) { mFlags |= STFLAG_ATTENUATE_EMITTER; }
        else if (token.Icmp("inverseAttenuateEmitter") == 0) { mFlags |= STFLAG_INVERSE_ATTENUATE; }
        else if (token.Icmp("locked") == 0) { mFlags |= STFLAG_LOCKED; }
        else if (token.Icmp("constant") == 0) { mFlags |= STFLAG_INFINITE_DURATION; }
        /* --- sound ---------------------------------------------------- */
        else if (token.Icmp("soundShader") == 0)
        {
            lexer->ReadToken(&token);
            mSoundShader = declManager->FindSound(token, true);
            const float len = mSoundShader->GetTimeLength();
            mLocalDuration.x = mLocalDuration.y = len;
            mFlags |= STFLAG_CALCULATE_DURATION;
        }
        else if (token.Icmp("volume") == 0) { READ_VEC2(mSoundVolume); }
        else if (token.Icmp("freqShift") == 0) { READ_VEC2(mFreqShift); }
        /* --- nested effect refs --------------------------------------- */
        else if (token.Icmp("effect") == 0)
        {
            lexer->ReadToken(&token);
            if (mNumEffects >= BSE_NUM_SPAWNABLE/* 4 */) {
                common->Warning("^4BSE:^1 Too many sub-effects in segment '%s'",
                    mSegmentName.c_str());
            }
            else {
                mEffects[mNumEffects++] = declManager->FindEffect(token, true);
            }
        }
        /* --- particle primitive keywords ------------------------------ */
        else if (token.Icmp("sprite") == 0) CreateParticleTemplate(effect, lexer, PTYPE_SPRITE); // 1
        else if (token.Icmp("line") == 0) CreateParticleTemplate(effect, lexer, PTYPE_LINE); // 2
        else if (token.Icmp("oriented") == 0) CreateParticleTemplate(effect, lexer, PTYPE_ORIENTED); // 3
        else if (token.Icmp("decal") == 0) CreateParticleTemplate(effect, lexer, PTYPE_DECAL); // 4
        else if (token.Icmp("model") == 0) CreateParticleTemplate(effect, lexer, PTYPE_MODEL); // 5
        else if (token.Icmp("light") == 0) CreateParticleTemplate(effect, lexer, PTYPE_LIGHT); // 6
        else if (token.Icmp("electricity") == 0) CreateParticleTemplate(effect, lexer, PTYPE_ELECTRIC); // 7
        else if (token.Icmp("linked") == 0) CreateParticleTemplate(effect, lexer, PTYPE_LINKED); // 8
        else if (token.Icmp("debris") == 0) CreateParticleTemplate(effect, lexer, PTYPE_DEBRIS); // 9
        /* --- channel-only keyword ------------------------------------- */
        else if (token.Icmp("channel") == 0) { /* nothing – marker */ }
        /* --- unknown token -------------------------------------------- */
        else
        {
            common->Warning("^4BSE:^1 Invalid segment parameter '%s' (file: %s, line: %d)",
                token.c_str(), lexer->GetFileName(), lexer->GetLineNum());
        }
#       undef READ_VEC2
    }

    return true;
}

float rvSegmentTemplate::GetSoundVolume() const
{
    return rvRandom::flrand(mSoundVolume.x, mSoundVolume.y);
}

float rvSegmentTemplate::GetFreqShift() const
{
    return rvRandom::flrand(mFreqShift.x, mFreqShift.y);
}
