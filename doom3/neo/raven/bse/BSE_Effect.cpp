// BSE_Effect.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop




#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"

#include "../../renderer/Model_local.h"
//#include "../renderer/RenderCommon.h"

#ifndef __thiscall
#define __thiscall
#endif

void rvBSE::Init(const rvDeclEffect* declEffect, renderEffect_s* parms, float time)
{
	int v6; // esi
	float* v7; // ecx
	float* v8; // eax
	double v9; // st3
	double v10; // st5
	float* v11; // esi
	float* v12; // eax
	int v13; // ecx
	double v14; // st5
	//float v15; // [esp+18h] [ebp-30h]
	//float v16; // [esp+1Ch] [ebp-2Ch]
	//float v17; // [esp+20h] [ebp-28h]
	//float v18; // [esp+24h] [ebp-24h]
	//float v19; // [esp+28h] [ebp-20h]
	//float v20; // [esp+2Ch] [ebp-1Ch]
	//float v21; // [esp+30h] [ebp-18h] BYREF
	//float v22[5]; // [esp+34h] [ebp-14h] BYREF
	float parmsa; // [esp+50h] [ebp+8h]

	this->mStartTime = time;
	this->mDeclEffect = declEffect;
	this->mLastTime = time;
	this->mFlags = 0;
	this->mDuration = 0.0;
	this->mAttenuation = 1.0;
	v6 = 2;
	this->mCost = 0.0;
	this->mFlags = parms->loop;
	this->mCurrentLocalBounds[1].z = 0.0;
	this->mCurrentLocalBounds[1].y = 0.0;
	v7 = &this->mLastRenderBounds[0].z;
	this->mCurrentLocalBounds[1].x = 0.0;
	this->mCurrentLocalBounds[0].z = 0.0;
	this->mCurrentLocalBounds[0].y = 0.0;
	this->mCurrentLocalBounds[0].x = 0.0;
	v8 = &this->mCurrentLocalBounds[0].z;
	v9 = declEffect->mSize;
	this->mCurrentLocalBounds[0].x = this->mCurrentLocalBounds[0].x - v9;
	this->mCurrentLocalBounds[0].y = this->mCurrentLocalBounds[0].y - v9;
	this->mCurrentLocalBounds[0].z = this->mCurrentLocalBounds[0].z - v9;
	this->mCurrentLocalBounds[1].x = this->mCurrentLocalBounds[1].x + v9;
	this->mCurrentLocalBounds[1].y = v9 + this->mCurrentLocalBounds[1].y;
	this->mCurrentLocalBounds[1].z = v9 + this->mCurrentLocalBounds[1].z;
	do
	{
		v10 = *(v8 - 2);
		v8 += 3;
		*(v7 - 2) = v10;
		v7 += 3;
		--v6;
		*(v7 - 4) = *(v8 - 4);
		*(v7 - 3) = *(v8 - 3);
	} while (v6);
	this->mGrownRenderBounds[0].z = 1.0e30;
	this->mGrownRenderBounds[0].y = 1.0e30;
	this->mGrownRenderBounds[0].x = 1.0e30;
	parmsa = -1.0e30;
	this->mGrownRenderBounds[1].z = parmsa;
	this->mGrownRenderBounds[1].y = parmsa;
	this->mGrownRenderBounds[1].x = parmsa;
	this->mForcePush = 0;
	this->mOriginalOrigin.x = parms->origin.x;
	this->mOriginalOrigin.y = parms->origin.y;
	this->mOriginalOrigin.z = parms->origin.z;
	this->mOriginalEndOrigin.z = 0.0;
	this->mOriginalEndOrigin.y = 0.0;
	this->mOriginalEndOrigin.x = 0.0;
	memcpy(&this->mOriginalAxis, &parms->axis, sizeof(this->mOriginalAxis));
	//v18 = this->mOriginalOrigin.x + this->mCurrentLocalBounds.b[1].x;
	//v19 = this->mCurrentLocalBounds[1].y + this->mOriginalOrigin.y;
	//v11 = &v21;
	//v20 = this->mCurrentLocalBounds[1].z + this->mOriginalOrigin.z;
	//v12 = &this->mCurrentWorldBounds[0].y;
	//v13 = 2;
	//v15 = this->mOriginalOrigin.x + this->mCurrentLocalBounds.b[0].x;
	//v16 = this->mCurrentLocalBounds[0].y + this->mOriginalOrigin.y;
	//v17 = this->mCurrentLocalBounds[0].z + this->mOriginalOrigin.z;
	//v21 = v15;
	//v22[0] = v16;
	//v22[1] = v17;
	//v22[2] = v18;
	//v22[3] = v19;
	//v22[4] = v20;
	//do
	//{
	//	v14 = *v11;
	//	v11 += 3;
	//	*(v12 - 1) = v14;
	//	v12 += 3;
	//	--v13;
	//	*(v12 - 3) = *(float*)((char*)v12 + (char*)&v21 - (char*)&this->mCurrentWorldBounds - 12);
	//	*(v12 - 2) = *(float*)((char*)v12 + (char*)v22 - (char*)&this->mCurrentWorldBounds - 12);
	//} while (v13);
	if (parms->hasEndOrigin)
	{
		this->mFlags |= 2u;
		this->mOriginalEndOrigin.x = parms->endOrigin.x;
		this->mOriginalEndOrigin.y = parms->endOrigin.y;
		this->mOriginalEndOrigin.z = parms->endOrigin.z;
		this->mCurrentEndOrigin.x = parms->endOrigin.x;
		this->mCurrentEndOrigin.y = parms->endOrigin.y;
		this->mCurrentEndOrigin.z = parms->endOrigin.z;
	}
	this->mCurrentTime = time;
	this->mCurrentOrigin.x = this->mOriginalOrigin.x;
	this->mCurrentOrigin.y = this->mOriginalOrigin.y;
	this->mCurrentOrigin.z = this->mOriginalOrigin.z;
	this->mCurrentVelocity.z = 0.0;
	this->mCurrentVelocity.y = 0.0;
	this->mCurrentVelocity.x = 0.0;
	UpdateFromOwner(parms, time, 1);
	this->mReferenceSound = 0;
// jmarshall
	//if ((declEffect->mFlags & 1) != 0)
	//	this->mReferenceSound = common->SW()->AllocSoundEmitter();
// jmarshall end
	UpdateSegments(time);
	this->mOriginDistanceToCamera = 0.0;
	this->mShortestDistanceToCamera = 0.0;
}

float rvBSE::GetAttenuation(rvSegmentTemplate* st) const
{
	double result; // st7
	float sta; // [esp+4h] [ebp+4h]
	float stb; // [esp+4h] [ebp+4h]
	float stc; // [esp+4h] [ebp+4h]
	float std; // [esp+4h] [ebp+4h]

	result = 0.0;
	if (st->mAttenuation.x <= 0.0 && st->mAttenuation.y <= 0.0)
		return this->mAttenuation;
	sta = st->mAttenuation.x + 1.0;
	if (sta > (double)this->mShortestDistanceToCamera)
		return this->mAttenuation;
	stb = st->mAttenuation.y - 1.0;
	if (stb >= (double)this->mShortestDistanceToCamera)
	{
		stc = (this->mShortestDistanceToCamera - st->mAttenuation.x) / (st->mAttenuation.y - st->mAttenuation.x);
		std = (1.0 - stc) * this->mAttenuation;
		result = std;
	}
	return result;
}

void rvBSE::UpdateSoundEmitter(rvSegmentTemplate* st, rvSegment* seg)
{
	idSoundEmitter* v4; // ecx
	soundShaderParms_t parms; // [esp+0h] [ebp-28h] BYREF
	int v6; // [esp+24h] [ebp-4h]

	parms.maxDistance = 0.0;
	//parms.farDistance = 0.0;
	parms.volume = 0.0;
	parms.shakes = 0.0;
	parms.soundShaderFlags = 0;
	//parms.pitchShift = 0.0;
	parms.soundClass = 0;
	//parms.soundArea = 0;
	v6 = 0;
	if ((this->mFlags & 8) != 0)
	{
		if (st->GetSoundLooping() && (seg->mFlags & 2) != 0)
			this->mReferenceSound->StopSound(seg->mSegmentTemplateHandle + 1);
	}
	else
	{
		v4 = this->mReferenceSound;
		parms.shakes = seg->mSoundVolume;
		*(float*)&parms.soundClass = seg->mFreqShift;
		v4->UpdateEmitter(mCurrentOrigin, 0, (soundShaderParms_t*)&parms.maxDistance);
	}
}
const idVec3 rvBSE::GetInterpolatedOffset(float time) const
{
	idVec3 result; // eax
	float v5; // [esp+0h] [ebp-18h]
	float v6; // [esp+4h] [ebp-14h]
	float v7; // [esp+8h] [ebp-10h]
	float v8; // [esp+Ch] [ebp-Ch]
	float v9; // [esp+10h] [ebp-8h]
	float v10; // [esp+14h] [ebp-4h]
	float deltaTime; // [esp+1Ch] [ebp+4h]
	float deltaTimea; // [esp+1Ch] [ebp+4h]

	result.z = 0.0;
	result.y = 0.0;
	result.x = 0.0;
	deltaTime = this->mCurrentTime - this->mLastTime;
	if (deltaTime >= 0.0020000001)
	{
		deltaTimea = 1.0 - (time - this->mLastTime) / deltaTime;
		v5 = this->mCurrentOrigin.x - this->mLastOrigin.x;
		v6 = this->mCurrentOrigin.y - this->mLastOrigin.y;
		v7 = this->mCurrentOrigin.z - this->mLastOrigin.z;
		v8 = v5 * deltaTimea;
		v9 = v6 * deltaTimea;
		v10 = deltaTimea * v7;
		result.x = v8;
		result.y = v9;
		result.z = v10;
	}
	return result;
}

void rvBSE::SetDuration(float time)
{
	double v2; // st7

	v2 = time;
	if (time < 0.0)
	{
		v2 = this->mDeclEffect->mMinDuration;
	LABEL_3:
		this->mDuration = v2;
		return;
	}
	if (this->mDuration < v2)
		goto LABEL_3;
}

const char* rvBSE::GetDeclName()
{
	return mDeclEffect->base->GetName();
}

void rvBSE::UpdateAttenuation()
{
	double v2; // st7
	double v3; // st7
	float fovx; // [esp+Ch] [ebp-50h]
	float fovxb; // [esp+Ch] [ebp-50h]
	float fovxa; // [esp+Ch] [ebp-50h]
	float fovxc; // [esp+Ch] [ebp-50h]
	float v8; // [esp+10h] [ebp-4Ch] BYREF
	float v9; // [esp+14h] [ebp-48h]
	float v10; // [esp+18h] [ebp-44h]
	idVec3 origin; // [esp+1Ch] [ebp-40h] BYREF
	idVec3 localOrigin; // [esp+28h] [ebp-34h] BYREF
	idMat3 axis; // [esp+34h] [ebp-28h] BYREF

	if ((this->mDeclEffect->mFlags & 4) != 0)
	{
		game->GetPlayerView(origin, axis);
		fovx = (mCurrentOrigin - origin).LengthFast();  //idVec3::Dist(&this->mCurrentOrigin, (idVec3*)&origin.y);
		v2 = 1.0;
		if (fovx >= 1.0)
		{
			v2 = fovx;
			if (fovx > 131072.0)
				v2 = 131072.0;
		}
		fovxb = v2;
		this->mOriginDistanceToCamera = fovxb;
		v9 = origin.y - this->mCurrentOrigin.x;
		v10 = origin.z - this->mCurrentOrigin.y;
		origin.x = localOrigin.x - this->mCurrentOrigin.z;
		localOrigin.y = this->mCurrentAxis[2].x * origin.x
			+ this->mCurrentAxis[0].x * v9
			+ this->mCurrentAxis[1].x * v10;
		localOrigin.z = this->mCurrentAxis[1].y * v10
			+ this->mCurrentAxis[0].y * v9
			+ this->mCurrentAxis[2].y * origin.x;
		axis[0].x = v9 * this->mCurrentAxis[0].z
			+ v10 * this->mCurrentAxis[1].z
			+ origin.x * this->mCurrentAxis[2].z;
		fovxa = mCurrentLocalBounds.ShortestDistance(localOrigin);
		v3 = 1.0;
		if (fovxa < 1.0 || (v3 = fovxa, fovxa <= 131072.0))
		{
			fovxc = v3;
			this->mShortestDistanceToCamera = fovxc;
		}
		else
		{
			this->mShortestDistanceToCamera = 131072.0;
		}
	}
}

void rvBSE::LoopInstant(float time)
{
	rvBSE* v2; // esi
	int v3; // edi
	bool v4; // zf
	bool v5; // sf
	int v6; // ebx
	uint64_t v7; // st7 //k unsigned __int64

	v2 = this;
	if (0.0 == this->mDuration)
	{
		v3 = 0;
		v4 = this->mSegments.Num() == 0;
		v5 = this->mSegments.Num() < 0;
		this->mStartTime = this->mDeclEffect->mMaxDuration + 0.5 + this->mStartTime;
		if (!v5 && !v4)
		{
			v6 = 0;
			do
			{
				v2->mSegments[v6].ResetTime(v2, v2->mStartTime);
				++v3;
				++v6;
			} while (v3 < v2->mSegments.Num());
		}
		if (bse_debug.GetInteger() == 2)
		{
			v7 = v2->mDeclEffect->mMaxDuration + 0.5;
			common->Printf("BSE: Looping duration %g\n", v7);
		}
		++v2->mDeclEffect->mLoopCount;
	}
}

void rvBSE::LoopLooping(float time)
{
	int v3; // edi
	bool v4; // cc
	int v5; // ebx

	if (0.0 != this->mDuration)
	{
		v3 = 0;
		v4 = this->mSegments.Num() <= 0;
		this->mStartTime = this->mStartTime + this->mDuration;
		this->mDuration = 0.0;
		if (!v4)
		{
			v5 = 0;
			do
			{
				mSegments[v5].ResetTime(this, this->mStartTime);
				++v3;
				++v5;
			} while (v3 < this->mSegments.Num());
		}
		if (bse_debug.GetInteger() == 2) {
			common->Printf("BSE: Looping duration: %g", this->mDuration);
		}

		++this->mDeclEffect->mLoopCount;
	}
}
bool rvBSE::Service(renderEffect_t* parms, float time, bool spawn, bool& forcePush)
{
	renderEffect_s* v5; // ebp
	int v7; // edi
	int v8; // ebx
	int v9; // edi
	char v10; // bl
	int v11; // ebp
	bool v12; // zf
	float spawna; // [esp+24h] [ebp+Ch]
	float spawnb; // [esp+24h] [ebp+Ch]
	float spawnc; // [esp+24h] [ebp+Ch]
	float spawnd; // [esp+24h] [ebp+Ch]

	v5 = parms;
	UpdateFromOwner(parms, time, 0);
	UpdateAttenuation();
	if (spawn)
	{
		v7 = 0;
		if (this->mSegments.Num() > 0)
		{
			v8 = 0;
			do
			{
				spawna = (double)(this->mSegments.Num() - v7) * -10.0;
				mSegments[v8].Check(this, time, spawna);
				++v7;
				++v8;
			} while (v7 < this->mSegments.Num());
		}
	}
	if ((this->mFlags & 8) == 0 && parms->loop)
	{
		spawnb = this->mDuration + this->mStartTime;
		if (spawnb < (double)time)
			LoopLooping(time);
	}
	v9 = 0;
	v10 = 0;
	if (this->mSegments.Num() > 0)
	{
		v11 = 0;
		do
		{
			if (mSegments[v11].UpdateParticles(this, time))
				v10 = 1;
			++v9;
			++v11;
		} while (v9 < this->mSegments.Num());
		v5 = parms;
	}
	this->mFlags &= 0xFFFFFFFB;
	forcePush = this->mForcePush;
	v12 = (this->mFlags & 8) == 0;
	this->mForcePush = 0;
	if (!v12)
		return v10 == 0;
	if (v5->loop)
	{
		spawnc = this->mDuration + this->mStartTime;
		if (spawnc < (double)time)
			LoopInstant(time);
		if (v5->loop)
			return 0;
	}
	spawnd = this->mDuration + this->mStartTime;
	return spawnd < (double)time;
}

float rvBSE::EvaluateCost(int segment)
{
	double result; // st7
	int v4; // edi
	int v5; // ebx
	double v6; // st7

	if (segment < 0)
	{
		v4 = 0;
		this->mCost = 0.0;
		if (this->mSegments.Num() > 0)
		{
			v5 = 0;
			do
			{
				v6 = mDeclEffect->EvaluateCost(
					this->mSegments[v5].mActiveCount,
					segment);
				++v4;
				++v5;
				this->mCost = v6 + this->mCost;
			} while (v4 < this->mSegments.Num());
		}
		result = this->mCost;
	}
	else
	{
		this->mCost = mDeclEffect->EvaluateCost(
			this->mSegments[segment].mActiveCount,
			segment);
		result = this->mCost;
	}
	return result;
}

void rvBSE::InitModel(idRenderModel* model)
{
	int v3; // edi
	int v4; // ebx

	v3 = 0;
	if (this->mSegments.Num() > 0)
	{
		v4 = 0;
		do
		{
			mSegments[v4].AllocateSurface(this, model);
			++v3;
			++v4;
		} while (v3 < this->mSegments.Num());
	}
}

// jmarshall
#if 0
idRenderModel* rvBSE::Render(idRenderModel* model, const struct renderEffect_s* owner, const viewDef_t* view)
{
	rvRenderModelBSE* renderModel; // ebp
	idBounds v23; // [esp+2Ch] [ebp-30h] BYREF
	float time; // [esp+6Ch] [ebp+10h]

	if (!bse_render.GetInteger())
		return 0;

	renderModel = (rvRenderModelBSE*)model;

	if (model == NULL)
	{
		renderModel = renderModelManager->AllocBSEModel();
		InitModel(renderModel);
	}

	renderModel->FreeVertexCache();

	v23 = renderModel->Bounds(NULL);

	memcpy(&this->mViewAxis, &view->renderView.viewaxis, sizeof(this->mViewAxis));

	this->mViewOrg.x = view->renderView.vieworg.x;
	this->mViewOrg.y = view->renderView.vieworg.y;
	//v11 = view->renderView.vieworg.z;
	this->mViewOrg.z = view->renderView.vieworg.z;
	if (bse->IsTimeLocked())
		time = bse->GetLockedTime();
	else
		time = view->renderView.time[1] * 0.001f; //time = view->floatTime;
	for (int i = 0; i < this->mSegments.Num(); ++i)
	{
		mSegments[i].ClearSurface(this, renderModel);
		if (mSegments[i].Active())
		{
			mSegments[i].Render(this, owner, renderModel, time);
			mSegments[i].RenderTrail(this, owner, renderModel, time);
		}
	}
	renderModel->FinishSurfaces(false);
	mLastRenderBounds = renderModel->Bounds(NULL);

	v23[1].x = mLastRenderBounds[0].x + 20.0;
	v23[1].y = mLastRenderBounds[0].y + 20.0;
	v23[1].z = mLastRenderBounds[0].z + 20.0;
	v23[0].x = mLastRenderBounds[1].x - 20.0;
	v23[0].y = mLastRenderBounds[1].y - 20.0;
	v23[0].z = mLastRenderBounds[1].z - 20.0;
	this->mGrownRenderBounds = v23;
	this->mForcePush = 1;
LABEL_18:
	//this->DisplayDebugInfo(this, a9, view, (idBounds*)&v24);
	return renderModel;
}
#endif
// jmarshall end

void rvBSE::Destroy()
{
	mSegments.Clear();
	if (mReferenceSound)
		mReferenceSound->Free(false);
}


void rvBSE::UpdateSegments(float time)
{
	int v3; // ebx
	rvSegment* v4; // eax
	rvParticle** v5; // esi
	bool v6; // cc
	int v7; // ecx
	int* v8; // eax
	rvSegment* v9; // esi
	rvSegment* v10; // ecx
	int v11; // edx
	int v12; // eax
	rvParticle** v13; // esi
	int v14; // esi
	int v15; // edi
	int v16; // esi
	int v17; // edi
	int v18; // esi
	int v19; // edi
	rvSegment* ptr; // [esp+14h] [ebp-14h]

	v3 = this->mDeclEffect->mSegmentTemplates.Num();
	if (v3 > 0)
	{
		if (v3 != this->mSegments.Size())
		{
			mSegments.SetNum(v3);
		}
	}
	else
	{
		mSegments.Clear();
	}
	v14 = 0;
	//this->mSegments.num = v3; // Not sure why this gets set twice? compiler optimization gone wrong?
	if (v3 > 0)
	{
		v15 = 0;
		do
			mSegments[v15++].Init(this, this->mDeclEffect, v14++, time);
		while (v14 < this->mSegments.Num());
	}
	v16 = 0;
	if (this->mSegments.Num() > 0)
	{
		v17 = 0;
		do
		{
			mSegments[v17].CalcCounts(this, time);
			++v16;
			++v17;
		} while (v16 < this->mSegments.Num());
	}
	v18 = 0;
	if (this->mSegments.Num() > 0)
	{
		v19 = 0;
		do
		{

			mSegments[v19].InitParticles(this);
			++v18;
			++v19;
		} while (v18 < this->mSegments.Num());
	}
}


void rvBSE::DisplayDebugInfo(const struct renderEffect_s* parms, const struct viewDef_s* view, idBounds& bounds) {
	// TODO
}

void __thiscall rvBSE::UpdateFromOwner(renderEffect_s* parms, float time, bool init)
{
	// jmarshall - todo
	/*
		renderEffect_s* v4; // edx
		rvBSE* v5; // ebx
		float* v6; // ebp
		double v7; // st7
		double v8; // st7
		renderEffect_s* v9; // ecx
		float* v10; // eax
		double v11; // st7
		double v12; // st7
		double v13; // st6
		double v14; // st5
		float* v15; // esi
		float v16; // edi
		float v17; // xmm1_4
		float v18; // xmm3_4
		float v19; // xmm5_4
		double v20; // st4
		float* v21; // esi
		float v22; // xmm1_4
		float v23; // xmm3_4
		float v24; // xmm5_4
		float* v25; // eax
		double v26; // st7
		float* v27; // esi
		float v28; // edi
		float v29; // xmm1_4
		float v30; // xmm3_4
		float v31; // xmm5_4
		double v32; // st4
		float* v33; // esi
		float v34; // xmm1_4
		float v35; // xmm3_4
		float v36; // xmm5_4
		float* v37; // ecx
		int v38; // eax
		signed int v39; // edx
		double v40; // st7
		idRenderWorldVtbl* v41; // edx
		double v42; // st7
		double v43; // st7
		float* v44; // esi
		float v45; // xmm1_4
		float v46; // xmm3_4
		float v47; // xmm5_4
		double v48; // st4
		float* v49; // esi
		float v50; // xmm1_4
		float v51; // xmm3_4
		float v52; // xmm5_4
		float* v53; // eax
		signed int v54; // ecx
		double v55; // st7
		float* v56; // esi
		float v57; // edi
		float v58; // xmm1_4
		float v59; // xmm3_4
		float v60; // xmm5_4
		double v61; // st7
		idMat3* v62; // eax
		double v63; // st7
		double v64; // st7
		idVec4* v65; // [esp+4h] [ebp-B4h]
		float* v66; // [esp+8h] [ebp-B0h]
		float* v67; // [esp+Ch] [ebp-ACh]
		int v68; // [esp+10h] [ebp-A8h]
		int v69; // [esp+14h] [ebp-A4h]
		float v70; // [esp+28h] [ebp-90h]
		float v71; // [esp+2Ch] [ebp-8Ch]
		float v72; // [esp+30h] [ebp-88h]
		float v73; // [esp+34h] [ebp-84h]
		float length; // [esp+38h] [ebp-80h]
		idVec3 size; // [esp+3Ch] [ebp-7Ch]
		idVec3 corner; // [esp+48h] [ebp-70h]
		idVec3 dir; // [esp+54h] [ebp-64h]
		float v78; // [esp+60h] [ebp-58h]
		float v79; // [esp+64h] [ebp-54h]
		float v80; // [esp+68h] [ebp-50h]
		float v81; // [esp+6Ch] [ebp-4Ch]
		float v82; // [esp+70h] [ebp-48h]
		float v83; // [esp+74h] [ebp-44h]
		float v84; // [esp+78h] [ebp-40h]
		float v85; // [esp+7Ch] [ebp-3Ch]
		float v86; // [esp+80h] [ebp-38h]
		float v87; // [esp+84h] [ebp-34h]
		float v88; // [esp+88h] [ebp-30h]
		float v89; // [esp+8Ch] [ebp-2Ch]
		float v90; // [esp+90h] [ebp-28h]
		idMat3 result; // [esp+94h] [ebp-24h]

		v4 = parms;
		v5 = this;
		this->mLastTime = this->mCurrentTime;
		v6 = &this->mCurrentOrigin.x;
		this->mLastOrigin.x = this->mCurrentOrigin.x;
		this->mLastOrigin.y = this->mCurrentOrigin.y;
		this->mLastOrigin.z = this->mCurrentOrigin.z;
		this->mCurrentTime = time;
		*v6 = parms->origin.x;
		v6[1] = parms->origin.y;
		v6[2] = parms->origin.z;
		memcpy(&this->mCurrentAxis, &parms->axis, sizeof(this->mCurrentAxis));
		v70 = this->mCurrentAxis[2].z;
		v71 = this->mCurrentAxis[1].z;
		v85 = this->mCurrentAxis[0].z;
		v86 = this->mCurrentAxis[2].y;
		v88 = this->mCurrentAxis[1].y;
		v90 = this->mCurrentAxis[0].y;
		v89 = this->mCurrentAxis[2].x;
		v87 = this->mCurrentAxis[1].x;
		size.x = this->mCurrentAxis[0].x;
		dir.y = size.x;
		dir.z = v87;
		v78 = v89;
		v79 = v90;
		v80 = v88;
		v81 = v86;
		v82 = v85;
		v83 = v71;
		v84 = v70;
		qmemcpy(&this->mCurrentAxisTransposed, &dir.y, sizeof(this->mCurrentAxisTransposed));
		size.x = this->mCurrentAxis.mat[2].z;
		v90 = this->mCurrentAxis.mat[1].z;
		v85 = this->mCurrentAxis.mat[0].z;
		v89 = this->mCurrentAxis.mat[2].y;
		v88 = this->mCurrentAxis.mat[1].y;
		v71 = this->mCurrentAxis.mat[0].y;
		v87 = this->mCurrentAxis.mat[2].x;
		v86 = this->mCurrentAxis.mat[1].x;
		v70 = this->mCurrentAxis.mat[0].x;
		size.y = v4->windVector.x * v70 + v4->windVector.y * v71 + v4->windVector.z * v85;
		size.z = v4->windVector.x * v86 + v4->windVector.y * v88 + v4->windVector.z * v90;
		corner.x = v4->windVector.y * v89 + v4->windVector.x * v87 + v4->windVector.z * size.x;
		this->mCurrentWindVector.x = size.y;
		this->mCurrentWindVector.y = size.z;
		this->mCurrentWindVector.z = corner.x;
		v7 = v5->mCurrentTime - v5->mLastTime;
		if (v7 > 0.002000000094994903)
		{
			size.y = *v6 - this->mLastOrigin.x;
			size.z = this->mCurrentOrigin.y - this->mLastOrigin.y;
			corner.x = this->mCurrentOrigin.z - this->mLastOrigin.z;
			v70 = v7;
			v70 = 1.0 / v70;
			dir.y = v70 * size.y;
			dir.z = size.z * v70;
			v78 = v70 * corner.x;
			this->mCurrentVelocity.x = dir.y;
			this->mCurrentVelocity.y = dir.z;
			this->mCurrentVelocity.z = v78;
		}
		this->mGravity.x = parms->gravity.x;
		this->mGravity.y = parms->gravity.y;
		this->mGravity.z = parms->gravity.z;
		this->mGravityDir.x = this->mGravity.x;
		this->mGravityDir.y = this->mGravity.y;
		this->mGravityDir.z = this->mGravity.z;
		v70 = this->mGravityDir.y * this->mGravityDir.y
			+ this->mGravityDir.x * this->mGravityDir.x
			+ this->mGravityDir.z * this->mGravityDir.z;
		v70 = sqrt(v70);
		if (v70 >= 0.00000011920929)
		{
			v70 = 1.0 / v70;
			v8 = v70;
			this->mGravityDir.x = this->mGravityDir.x * v70;
			this->mGravityDir.y = this->mGravityDir.y * v8;
			this->mGravityDir.z = v8 * this->mGravityDir.z;
		}
		v9 = parms;
		if (parms->useRenderBounds || parms->isStatic)
		{
			if (v5->mGrownRenderBounds.b[1].x >= (double)v5->mGrownRenderBounds.b[0].x)
			{
				v37 = &dir.y;
				size.y = v5->mGrownRenderBounds.b[1].x + 10.0;
				v38 = (int)&v5->mCurrentWorldBounds.b[0].y;
				v39 = 2;
				size.z = v5->mGrownRenderBounds.b[1].y + 10.0;
				corner.x = v5->mGrownRenderBounds.b[1].z + 10.0;
				v72 = v5->mGrownRenderBounds.b[0].x - 10.0;
				v73 = v5->mGrownRenderBounds.b[0].y - 10.0;
				length = v5->mGrownRenderBounds.b[0].z - 10.0;
				dir.y = v72;
				dir.z = v73;
				v78 = length;
				v79 = size.y;
				v80 = size.z;
				v81 = corner.x;
				do
				{
					v40 = *v37;
					v37 += 3;
					*(float*)(v38 - 4) = v40;
					v38 += 12;
					--v39;
					*(float*)(v38 - 12) = *(float*)((char*)&dir.y - (char*)&v5->mCurrentWorldBounds + v38 - 12);
					*(float*)(v38 - 8) = *(float*)((char*)&dir.z - (char*)&v5->mCurrentWorldBounds + v38 - 12);
				} while (v39);
				idBounds::FromTransformedBounds(
					&v5->mCurrentWorldBounds,
					&v5->mCurrentWorldBounds,
					&vec3_origin,
					&v5->mCurrentAxis);
				v9 = parms;
				v5->mCurrentWorldBounds.b[0].x = v5->mCurrentWorldBounds.b[0].x + *v6;
				v5->mCurrentWorldBounds.b[0].y = v6[1] + v5->mCurrentWorldBounds.b[0].y;
				v5->mCurrentWorldBounds.b[0].z = v6[2] + v5->mCurrentWorldBounds.b[0].z;
				v5->mCurrentWorldBounds.b[1].x = *v6 + v5->mCurrentWorldBounds.b[1].x;
				v5->mCurrentWorldBounds.b[1].y = v6[1] + v5->mCurrentWorldBounds.b[1].y;
				v5->mCurrentWorldBounds.b[1].z = v5->mCurrentWorldBounds.b[1].z + v6[2];
				v13 = size.z;
				v14 = corner.x;
				v12 = size.y;
			}
			else
			{
				size.y = v5->mDeclEffect->mSize;
				v70 = COERCE_FLOAT(&v72);
				size.z = size.y;
				v25 = (float*)&v5->mCurrentWorldBounds;
				corner.x = size.y;
				v71 = *(float*)&v25;
				v25[2] = 1.0e30;
				v25[1] = 1.0e30;
				*v25 = 1.0e30;
				size.x = -1.0e30;
				v26 = size.x;
				v25[5] = size.x;
				v25[4] = v26;
				v25[3] = v26;
				v12 = size.y;
				v72 = size.y + *v6;
				v13 = size.z;
				v73 = v6[1] + size.z;
				v14 = corner.x;
				length = v6[2] + corner.x;
				v27 = (float*)LODWORD(v71);
				v28 = v70;
				v29 = *(float*)LODWORD(v70);
				*(float*)LODWORD(v71) = fminf(*(float*)LODWORD(v71), *(float*)LODWORD(v70));
				v30 = *(float*)(LODWORD(v28) + 4);
				v27[1] = fminf(v27[1], v30);
				v31 = *(float*)(LODWORD(v28) + 8);
				v27[2] = fminf(v27[2], v31);
				v27[3] = fmaxf(v29, v27[3]);
				v27[4] = fmaxf(v30, v27[4]);
				v27[5] = fmaxf(v31, v27[5]);
				v32 = *v6 - v12;
				v70 = COERCE_FLOAT(&v72);
				LODWORD(v71) = (char*)v5 + 396;
				v72 = v32;
				v73 = v6[1] - v13;
				length = v6[2] - v14;
				v33 = (float*)&v5->mCurrentWorldBounds;
				v34 = v72;
				*v33 = fminf(v5->mCurrentWorldBounds.b[0].x, v72);
				v35 = v73;
				v33[1] = fminf(v5->mCurrentWorldBounds.b[0].y, v73);
				v36 = length;
				v33[2] = fminf(v5->mCurrentWorldBounds.b[0].z, length);
				v33[3] = fmaxf(v34, v5->mCurrentWorldBounds.b[1].x);
				v33[4] = fmaxf(v35, v5->mCurrentWorldBounds.b[1].y);
				v33[5] = fmaxf(v36, v5->mCurrentWorldBounds.b[1].z);
			}
		}
		else
		{
			size.y = v5->mDeclEffect->mSize;
			v70 = COERCE_FLOAT(&v72);
			size.z = size.y;
			v10 = (float*)&v5->mCurrentWorldBounds;
			corner.x = size.y;
			v71 = *(float*)&v10;
			v10[2] = 1.0e30;
			v10[1] = 1.0e30;
			*v10 = 1.0e30;
			size.x = -1.0e30;
			v11 = size.x;
			v10[5] = size.x;
			v10[4] = v11;
			v10[3] = v11;
			v12 = size.y;
			v72 = *v6 + size.y;
			v13 = size.z;
			v73 = v6[1] + size.z;
			v14 = corner.x;
			length = v6[2] + corner.x;
			v15 = (float*)LODWORD(v71);
			v16 = v70;
			v17 = *(float*)LODWORD(v70);
			*(float*)LODWORD(v71) = fminf(*(float*)LODWORD(v71), *(float*)LODWORD(v70));
			v18 = *(float*)(LODWORD(v16) + 4);
			v15[1] = fminf(v15[1], v18);
			v19 = *(float*)(LODWORD(v16) + 8);
			v15[2] = fminf(v15[2], v19);
			v15[3] = fmaxf(v17, v15[3]);
			v15[4] = fmaxf(v18, v15[4]);
			v15[5] = fmaxf(v19, v15[5]);
			v20 = *v6 - v12;
			v70 = COERCE_FLOAT(&v72);
			LODWORD(v71) = (char*)v5 + 396;
			v72 = v20;
			v73 = v6[1] - v13;
			length = v6[2] - v14;
			v21 = (float*)&v5->mCurrentWorldBounds;
			v22 = v72;
			*v21 = fminf(v5->mCurrentWorldBounds.b[0].x, v72);
			v23 = v73;
			v21[1] = fminf(v5->mCurrentWorldBounds.b[0].y, v73);
			v24 = length;
			v21[2] = fminf(v5->mCurrentWorldBounds.b[0].z, length);
			v21[3] = fmaxf(v22, v5->mCurrentWorldBounds.b[1].x);
			v21[4] = fmaxf(v23, v5->mCurrentWorldBounds.b[1].y);
			v21[5] = fmaxf(v24, v5->mCurrentWorldBounds.b[1].z);
			v5->mForcePush = 0;
		}
		if (bse_debug.internalVar->integerValue > 2)
		{
			v41 = session->rw->vfptr;
			v69 = 0;
			v68 = 10000;
			v67 = &v5->mLastOrigin.x;
			((void(__stdcall*)(idVec4*, float*, idVec3*, signed int, _DWORD))v41->DebugLine)(
				&colorWhite,
				v6,
				&v5->mLastOrigin,
				10000,
				0);
			v72 = *v6;
			v42 = v5->mCurrentOrigin.y;
			v69 = 0;
			v73 = v42;
			v68 = 10000;
			v43 = v5->mCurrentOrigin.z + 10.0;
			v67 = &v72;
			v66 = v6;
			v65 = &colorGreen;
			length = v43;
			((void(__stdcall*)(idVec4*, float*, float*, signed int, _DWORD))session->rw->vfptr->DebugLine)(
				&colorGreen,
				v6,
				&v72,
				10000,
				0);
			v13 = size.z;
			v9 = parms;
			v14 = corner.x;
			v12 = size.y;
		}
		if (((unsigned int)v5->mFlags >> 1) & 1
			&& ((unsigned int)v5->mDeclEffect->mFlags >> 1) & 1
			&& (init
				|| v9->endOrigin.x != v5->mCurrentEndOrigin.x
				|| v9->endOrigin.y != v5->mCurrentEndOrigin.y
				|| v9->endOrigin.z != v5->mCurrentEndOrigin.z
				|| v5->mLastOrigin.x != *v6
				|| v5->mLastOrigin.y != v6[1]
				|| v5->mLastOrigin.z != v6[2]))
		{
			v5->mCurrentEndOrigin.x = v9->endOrigin.x;
			LODWORD(v71) = (char*)v5 + 396;
			v5->mCurrentEndOrigin.y = v9->endOrigin.y;
			v5->mCurrentEndOrigin.z = v9->endOrigin.z;
			v70 = COERCE_FLOAT(&v72);
			v72 = v12 + v5->mCurrentEndOrigin.x;
			v73 = v5->mCurrentEndOrigin.y + v13;
			length = v5->mCurrentEndOrigin.z + v14;
			v44 = (float*)LODWORD(v71);
			v45 = v72;
			*(float*)LODWORD(v71) = fminf(*(float*)LODWORD(v71), v72);
			v46 = v73;
			v44[1] = fminf(v44[1], v73);
			v47 = length;
			v44[2] = fminf(v44[2], length);
			v44[3] = fmaxf(v45, v44[3]);
			v44[4] = fmaxf(v46, v44[4]);
			v44[5] = fmaxf(v47, v44[5]);
			v48 = v5->mCurrentEndOrigin.x;
			v70 = COERCE_FLOAT(&v72);
			LODWORD(v71) = (char*)v5 + 396;
			v72 = v48 - v12;
			v73 = v5->mCurrentEndOrigin.y - v13;
			length = v5->mCurrentEndOrigin.z - v14;
			v49 = (float*)&v5->mCurrentWorldBounds;
			v50 = v72;
			*v49 = fminf(v5->mCurrentWorldBounds.b[0].x, v72);
			v51 = v73;
			v49[1] = fminf(v5->mCurrentWorldBounds.b[0].y, v73);
			v52 = length;
			v49[2] = fminf(v5->mCurrentWorldBounds.b[0].z, length);
			v49[3] = fmaxf(v50, v5->mCurrentWorldBounds.b[1].x);
			v49[4] = fmaxf(v51, v5->mCurrentWorldBounds.b[1].y);
			v49[5] = fmaxf(v52, v5->mCurrentWorldBounds.b[1].z);
			v5->mFlags |= 4u;
		}
		v53 = (float*)&v5->mCurrentLocalBounds;
		v53[2] = 1.0e30;
		v54 = 0;
		v53[1] = 1.0e30;
		v70 = COERCE_FLOAT((idVec3*)((char*)&corner + 4));
		LODWORD(v71) = (char*)v5 + 372;
		*v53 = 1.0e30;
		size.x = -1.0e30;
		v55 = size.x;
		v53[5] = size.x;
		v53[4] = v55;
		v53[3] = v55;
		do
		{
			corner.y = *((float*)&v5->vfptr + 3 * ((v54 & 1) + 33));
			corner.z = v5->mCurrentWorldBounds.b[(v54 >> 1) & 1].y;
			dir.x = v5->mCurrentWorldBounds.b[(v54 >> 2) & 1].z;
			corner.y = corner.y - *v6;
			corner.z = corner.z - v6[1];
			dir.x = dir.x - v6[2];
			v72 = v5->mCurrentAxisTransposed.mat[2].x * dir.x
				+ v5->mCurrentAxisTransposed.mat[0].x * corner.y
				+ corner.z * v5->mCurrentAxisTransposed.mat[1].x;
			v73 = v5->mCurrentAxisTransposed.mat[0].y * corner.y
				+ v5->mCurrentAxisTransposed.mat[1].y * corner.z
				+ dir.x * v5->mCurrentAxisTransposed.mat[2].y;
			length = dir.x * v5->mCurrentAxisTransposed.mat[2].z
				+ corner.y * v5->mCurrentAxisTransposed.mat[0].z
				+ corner.z * v5->mCurrentAxisTransposed.mat[1].z;
			corner.y = v72;
			corner.z = v73;
			dir.x = length;
			v56 = (float*)LODWORD(v71);
			v57 = v70;
			v58 = *(float*)LODWORD(v70);
			*(float*)LODWORD(v71) = fminf(*(float*)LODWORD(v71), *(float*)LODWORD(v70));
			v59 = *(float*)(LODWORD(v57) + 4);
			v56[1] = fminf(v56[1], v59);
			v60 = *(float*)(LODWORD(v57) + 8);
			v56[2] = fminf(v56[2], v60);
			v56[3] = fmaxf(v58, v56[3]);
			v56[4] = fmaxf(v59, v56[4]);
			v56[5] = fmaxf(v60, v56[5]);
			++v54;
		} while (v54 < 8);
		dir.y = v5->mCurrentEndOrigin.x - *v6;
		dir.z = v5->mCurrentEndOrigin.y - v6[1];
		v78 = v5->mCurrentEndOrigin.z - v6[2];
		v70 = dir.y * dir.y + dir.z * dir.z + v78 * v78;
		v70 = sqrt(v70);
		v61 = v70;
		if (v70 >= 0.00000011920929)
		{
			v70 = 1.0 / v61;
			dir.y = v70 * dir.y;
			dir.z = dir.z * v70;
			v78 = v70 * v78;
		}
		else
		{
			v61 = 0.0;
		}
		size.x = v61;
		v62 = idVec3::ToMat3((idVec3*)((char*)&dir + 4), &result);
		v63 = size.x / 100.0;
		qmemcpy(&v5->mLightningAxis, v62, sizeof(v5->mLightningAxis));
		v70 = v63;
		v64 = v70;
		v5->mLightningAxis.mat[0].x = v5->mLightningAxis.mat[0].x * v70;
		v5->mLightningAxis.mat[0].y = v64 * v5->mLightningAxis.mat[0].y;
		v5->mLightningAxis.mat[0].z = v64 * v5->mLightningAxis.mat[0].z;
		v5->mTint.x = parms->shaderParms[0];
		v5->mTint.y = parms->shaderParms[1];
		v5->mTint.z = parms->shaderParms[2];
		v5->mTint.w = parms->shaderParms[3];
		v5->mBrightness = parms->shaderParms[6];
		v5->mSuppressLightsInViewID = parms->suppressLightsInViewID;
		v5->mAttenuation = parms->attenuation;
		v5->mMaterialColor.x = parms->materialColor.x;
		v5->mMaterialColor.y = parms->materialColor.y;
		v5->mMaterialColor.z = parms->materialColor.z;
	*/
}
