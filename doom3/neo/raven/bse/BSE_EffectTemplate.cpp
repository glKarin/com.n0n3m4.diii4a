// BSE_EffectTemplate.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop



#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"

void rvDeclEffect::Init()
{
	mMinDuration = 0.0;
	mMaxDuration = 0.0;
	mSize = 512.0;
	mFlags = 0;
	mPlayCount = 0;
	mLoopCount = 0;
	mCutOffDistance = 0.0;
	mSegmentTemplates.Clear();
}

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

int rvDeclEffect::GetTrailSegmentIndex(const idStr& name)
{
	rvDeclEffect* v2; // esi
	int v3; // edi
	int v4; // ebx
	rvSegmentTemplate* v5; // eax
	int result; // eax

	v2 = this;
	v3 = 0;
	if (mSegmentTemplates.Num() <= 0)
	{
	LABEL_6:
		common->Warning("^4BSE:^1 Unable to find segment '%s'\n", name.c_str());
		result = -1;
	}
	else
	{
		v4 = 0;
		while (1)
		{
			v5 = &this->mSegmentTemplates[v4];
			if (v5)
			{
				if (name == v5->GetSegmentName())
					break;
			}
			++v3;
			++v4;
			if (v3 >= this->mSegmentTemplates.Num())
				goto LABEL_6;
		}
		result = v3;
	}
	return result;
}

float rvDeclEffect::EvaluateCost(int activeParticles, int segment) const
{
	int v5; // edi
	int v6; // ebx
	double v7; // st7
	float cost; // [esp+Ch] [ebp+8h]

	if (segment != -1)
		return mSegmentTemplates[segment].EvaluateCost(activeParticles);
	v5 = 0;
	cost = 0.0;
	if (this->mSegmentTemplates.Num() > 0)
	{
		v6 = 0;
		do
		{
			v7 = mSegmentTemplates[v6].EvaluateCost(activeParticles);
			++v5;
			++v6;
			cost = v7 + cost;
		} while (v5 < this->mSegmentTemplates.Num());
	}
	return cost;
}

void rvDeclEffect::FreeData()
{
	int v2; // ebx
	int v3; // edi
	rvSegmentTemplate* v4; // eax
	int* v5; // edi

	v2 = 0;
	if (this->mSegmentTemplates.Num() > 0)
	{
		v3 = 0;
		do
		{
			mSegmentTemplates[v3].GetParticleTemplate()->Purge();
			mSegmentTemplates[v3].GetParticleTemplate()->PurgeTraceModel();
			++v2;
			++v3;
		} while (v2 < this->mSegmentTemplates.Num());
	}
	mSegmentTemplates.Clear();
}

const char* rvDeclEffect::DefaultDefinition() const
{
	return "{\n}\n";
}

void rvDeclEffect::SetMinDuration(float duration)
{
	if (this->mMinDuration < duration)
		this->mMinDuration = duration;
}

void rvDeclEffect::SetMaxDuration(float duration)
{
	if (this->mMaxDuration < duration)
		this->mMaxDuration = duration;
}

void rvDeclEffect::Finish() {
	rvSegmentTemplate* segment;
	for (int j = 0; j < mSegmentTemplates.Num(); j++)
	{
		segment = &mSegmentTemplates[j];
		if (segment)
		{
			segment->Finish(this);

			if (segment->GetType() == SEG_SOUND)
				mFlags |= ETFLAG_HAS_SOUND;

			if (segment->GetParticleTemplate()->UsesEndOrigin())
				mFlags |= ETFLAG_USES_ENDORIGIN;

			if ((mFlags & 0x200) != 0)
				mFlags |= ETFLAG_HAS_PHYSICS;
			if ((mFlags & 0x40) != 0)
				mFlags |= ETFLAG_ATTENUATES;

			segment->EvaluateTrailSegment(this);
			segment->SetMinDuration(this);
			segment->SetMaxDuration(this);
		}
	}
}

bool rvDeclEffect::Parse(const char* text, const int textLength) {
#if 1
	return true;
#else
	idParser src;
	idToken	token, token2;
	rvSegmentTemplate segment;

	src.LoadMemory(text, textLength, GetFileName());
	src.SetFlags(DECL_LEXER_FLAGS);
	src.SkipUntilString("{");

	if (src.ReadToken(&token))
	{
		while (token != "}")
		{
			segment.Init(this);
			if (token == "tunnel")
			{
				segment.Parse(this, SEG_TUNNEL, &src);
				if (segment.Finish(this)) {
					mSegmentTemplates.Append(segment);
				}
			}
			else if (token == "shake")
			{
				segment.Parse(this, SEG_SHAKE, &src);
				if (segment.Finish(this)) {
					mSegmentTemplates.Append(segment);
				}
			}
			else if (token == "delay")
			{
				segment.Parse(this, SEG_DELAY, &src);
				if (segment.Finish(this)) {
					mSegmentTemplates.Append(segment);
				}
			}
			else if (token == "light")
			{
				segment.Parse(this, SEG_LIGHT, &src);
				if (segment.Finish(this)) {
					mSegmentTemplates.Append(segment);
				}
			}
			else if (token == "decal")
			{
				segment.Parse(this, SEG_DECAL, &src);
				if (segment.Finish(this)) {
					mSegmentTemplates.Append(segment);
				}
			}
			else if (token == "sound")
			{
				segment.Parse(this, SEG_SOUND, &src);
				if (segment.Finish(this)) {
					mSegmentTemplates.Append(segment);
				}
			}
			else if (token == "trail")
			{
				segment.Parse(this, SEG_TRAIL, &src);
				if (segment.Finish(this)) {
					mSegmentTemplates.Append(segment);
				}
			}
			else if (token == "spawner")
			{
				segment.Parse(this, SEG_SPAWNER, &src);

				if (segment.Finish(this)) {
					mSegmentTemplates.Append(segment);
				}
			}
			else if (token == "emitter")
			{
				segment.Parse(this, SEG_EMITTER, &src);

				if (segment.Finish(this)) {
					mSegmentTemplates.Append(segment);
				}
			}
			else if (token == "effect")
			{
				segment.Parse(this, SEG_EFFECT, &src);

				if (segment.Finish(this)) {
					mSegmentTemplates.Append(segment);
				}
			}
			else if (token == "cutOffDistance") {
				mCutOffDistance = src.ParseFloat();
			}
			else if (token == "size")
			{
				mSize = src.ParseFloat();
			}
			else
			{
				src.Error("^4BSE:^1 Invalid segment type '%s' (file: %s, line: %d)\n", token, GetFileName(), src.GetLineNum());
			}

			src.ReadToken(&token);
		}
	}

	Finish();

	return true;
#endif
}
