/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

// Big thanks to Chicken Fortress developers
// for this code.
#include <assert.h>
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

#include "event_api.h"
#include "pm_defs.h"

#include "com_weapons.h"

extern CGameStudioModelRenderer g_StudioRenderer;
extern engine_studio_api_t IEngineStudio;
typedef struct pmtrace_s pmtrace_t;

void CStudioModelRenderer::Init(void)
{
	m_pCvarHiModels = IEngineStudio.GetCvar("cl_himodels");
	m_pCvarDeveloper = IEngineStudio.GetCvar("developer");
	m_pCvarDrawEntities = IEngineStudio.GetCvar("r_drawentities");
	m_pCvarShadows = CVAR_CREATE("cl_shadows", "1", FCVAR_ARCHIVE );
#ifndef NDEBUG
	m_pCvarDebug = CVAR_CREATE( "r_smr_debug", "0", 0 );
#endif
	m_pChromeSprite = IEngineStudio.GetChromeSprite();

	IEngineStudio.GetModelCounters(&m_pStudioModelCount, &m_pModelsDrawn);

	m_pbonetransform = (float (*)[MAXSTUDIOBONES][3][4])IEngineStudio.StudioGetBoneTransform();
	m_plighttransform = (float (*)[MAXSTUDIOBONES][3][4])IEngineStudio.StudioGetLightTransform();
	m_paliastransform = (float (*)[3][4])IEngineStudio.StudioGetAliasTransform();
	m_protationmatrix = (float (*)[3][4])IEngineStudio.StudioGetRotationMatrix();
}

CStudioModelRenderer::CStudioModelRenderer(void)
{
	m_fDoInterp = 1;
	m_fGaitEstimation = 1;
	m_pCurrentEntity = NULL;
	m_pCvarHiModels = NULL;
	m_pCvarDeveloper = NULL;
	m_pCvarDrawEntities = NULL;
	m_pChromeSprite = NULL;
	m_pStudioModelCount = NULL;
	m_pModelsDrawn = NULL;
	m_protationmatrix = NULL;
	m_paliastransform = NULL;
	m_pbonetransform = NULL;
	m_plighttransform = NULL;
	m_pStudioHeader = NULL;
	m_pBodyPart = NULL;
	m_pSubModel = NULL;
	m_pPlayerInfo = NULL;
	m_pRenderModel = NULL;
	m_iShadowSprite = 0;
}

CStudioModelRenderer::~CStudioModelRenderer(void)
{
}

void CStudioModelRenderer::StudioCalcBoneAdj(float dadt, float *adj, const byte *pcontroller1, const byte *pcontroller2, byte mouthopen)
{
	int i, j;
	float value;
	mstudiobonecontroller_t *pbonecontroller;

	pbonecontroller = (mstudiobonecontroller_t *)((byte *)m_pStudioHeader + m_pStudioHeader->bonecontrollerindex);

	for (j = 0; j < m_pStudioHeader->numbonecontrollers; j++)
	{
		i = pbonecontroller[j].index;

		if (i <= 3)
		{
			if (pbonecontroller[j].type & STUDIO_RLOOP)
			{
				if (abs(pcontroller1[i] - pcontroller2[i]) > 128)
				{
					int a, b;
					a = (pcontroller1[j] + 128) % 256;
					b = (pcontroller2[j] + 128) % 256;
					value = ((a * dadt) + (b * (1 - dadt)) - 128) * (360.0 / 256.0) + pbonecontroller[j].start;
				}
				else
				{
					value = ((pcontroller1[i] * dadt + (pcontroller2[i]) * (1.0 - dadt))) * (360.0 / 256.0) + pbonecontroller[j].start;
				}
			}
			else
			{
				value = (pcontroller1[i] * dadt + pcontroller2[i] * (1.0 - dadt)) / 255.0;

				if (value < 0)
					value = 0;

				if (value > 1.0)
					value = 1.0;

				value = (1.0 - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			}
		}
		else
		{
			value = mouthopen / 64.0;

			if (value > 1.0)
				value = 1.0;

			value = (1.0 - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
		}

		switch (pbonecontroller[j].type & STUDIO_TYPES)
		{
			case STUDIO_XR:
			case STUDIO_YR:
			case STUDIO_ZR:
			{
				adj[j] = value * (M_PI / 180.0);
				break;
			}
			case STUDIO_X:
			case STUDIO_Y:
			case STUDIO_Z:
			{
				adj[j] = value;
				break;
			}
		}
	}
}

void CStudioModelRenderer::StudioCalcBoneQuaterion(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *q)
{
	int j, k;
	vec4_t q1, q2;
	vec3_t angle1, angle2;
	mstudioanimvalue_t *panimvalue;

	for (j = 0; j < 3; j++)
	{
		if (panim->offset[j + 3] == 0)
		{
			angle2[j] = angle1[j] = pbone->value[j + 3];
		}
		else
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j + 3]);
			k = frame;

			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;

			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;

				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}

			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k + 1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k + 2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;

				if (panimvalue->num.total > k + 1)
					angle2[j] = angle1[j];
				else
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
			}

			angle1[j] = pbone->value[j + 3] + angle1[j] * pbone->scale[j + 3];
			angle2[j] = pbone->value[j + 3] + angle2[j] * pbone->scale[j + 3];
		}

		if (pbone->bonecontroller[j + 3] != -1)
		{
			angle1[j] += adj[pbone->bonecontroller[j + 3]];
			angle2[j] += adj[pbone->bonecontroller[j + 3]];
		}
	}

	if (!VectorCompare(angle1, angle2))
	{
		AngleQuaternion(angle1, q1);
		AngleQuaternion(angle2, q2);
		QuaternionSlerp(q1, q2, s, q);
	}
	else
	{
		AngleQuaternion(angle1, q);
	}
}

void CStudioModelRenderer::StudioCalcBonePosition(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *pos)
{
	int j, k;
	mstudioanimvalue_t *panimvalue;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j];

		if (panim->offset[j] != 0)
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);
			k = frame;

			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;

			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;

				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}

			if (panimvalue->num.valid > k)
			{
				if (panimvalue->num.valid > k + 1)
					pos[j] += (panimvalue[k + 1].value * (1.0 - s) + s * panimvalue[k + 2].value) * pbone->scale[j];
				else
					pos[j] += panimvalue[k + 1].value * pbone->scale[j];
			}
			else
			{
				if (panimvalue->num.total <= k + 1)
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0 - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				else
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
			}
		}

		if (pbone->bonecontroller[j] != -1 && adj)
			pos[j] += adj[pbone->bonecontroller[j]];
	}
}

void CStudioModelRenderer::StudioSlerpBones(vec4_t q1[], float pos1[][3], vec4_t q2[], float pos2[][3], float s)
{
	int i;
	vec4_t q3;
	float s1;

	if (s < 0)
		s = 0;
	else if (s > 1.0)
		s = 1.0;

	s1 = 1.0 - s;

	for (i = 0; i < m_pStudioHeader->numbones; i++)
	{
		QuaternionSlerp(q1[i], q2[i], s, q3);

		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}

mstudioanim_t *CStudioModelRenderer::StudioGetAnim(model_t *pSubModel, mstudioseqdesc_t *pseqdesc)
{
	mstudioseqgroup_t *pseqgroup;
	cache_user_t *paSequences;

	pseqgroup = (mstudioseqgroup_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqgroupindex) + pseqdesc->seqgroup;

	if (pseqdesc->seqgroup == 0)
		return (mstudioanim_t *)((byte *)m_pStudioHeader + pseqdesc->animindex);

	paSequences = (cache_user_t *)pSubModel->submodels;

	if (paSequences == NULL)
	{
		paSequences = (cache_user_t *)IEngineStudio.Mem_Calloc(16, sizeof(cache_user_t));
		pSubModel->submodels = (dmodel_t *)paSequences;
	}

	if (!IEngineStudio.Cache_Check((struct cache_user_s *)&(paSequences[pseqdesc->seqgroup])))
	{
		gEngfuncs.Con_DPrintf("loading %s\n", pseqgroup->name);
		IEngineStudio.LoadCacheFile(pseqgroup->name, (struct cache_user_s *)&paSequences[pseqdesc->seqgroup]);
	}

	return (mstudioanim_t *)((byte *)paSequences[pseqdesc->seqgroup].data + pseqdesc->animindex);
}

void CStudioModelRenderer::StudioPlayerBlend(mstudioseqdesc_t *pseqdesc, int *pBlend, float *pPitch)
{
	*pBlend = (*pPitch * 3);

	if (*pBlend < pseqdesc->blendstart[0])
	{
		*pPitch -= pseqdesc->blendstart[0] / 3.0;
		*pBlend = 0;
	}
	else if (*pBlend > pseqdesc->blendend[0])
	{
		*pPitch -= pseqdesc->blendend[0] / 3.0;
		*pBlend = 255;
	}
	else
	{
		if (pseqdesc->blendend[0] - pseqdesc->blendstart[0] < 0.1)
			*pBlend = 127;
		else
			*pBlend = 255 * (*pBlend - pseqdesc->blendstart[0]) / (pseqdesc->blendend[0] - pseqdesc->blendstart[0]);

		*pPitch = 0;
	}
}

void CStudioModelRenderer::StudioSetUpTransform(int trivial_accept)
{
	int i;
	vec3_t angles;
	vec3_t modelpos;

	VectorCopy(m_pCurrentEntity->origin, modelpos);

	angles[ROLL] = m_pCurrentEntity->curstate.angles[ROLL];
	angles[PITCH] = m_pCurrentEntity->curstate.angles[PITCH];
	angles[YAW] = m_pCurrentEntity->curstate.angles[YAW];

	if (m_pCurrentEntity->curstate.movetype != MOVETYPE_NONE)
	{
		VectorCopy(m_pCurrentEntity->angles, angles);
	}

	angles[PITCH] = -angles[PITCH];
	AngleMatrix(angles, (*m_protationmatrix));

	if (!IEngineStudio.IsHardware())
	{
		static float viewmatrix[3][4];

		VectorCopy(m_vRight, viewmatrix[0]);
		VectorCopy(m_vUp, viewmatrix[1]);
		VectorInverse(viewmatrix[1]);
		VectorCopy(m_vNormal, viewmatrix[2]);

		(*m_protationmatrix)[0][3] = modelpos[0] - m_vRenderOrigin[0];
		(*m_protationmatrix)[1][3] = modelpos[1] - m_vRenderOrigin[1];
		(*m_protationmatrix)[2][3] = modelpos[2] - m_vRenderOrigin[2];

		ConcatTransforms(viewmatrix, (*m_protationmatrix), (*m_paliastransform));

		if (trivial_accept)
		{
			for (i = 0; i < 4; i++)
			{
				(*m_paliastransform)[0][i] *= m_fSoftwareXScale * (1.0 / (ZISCALE * 0x10000));
				(*m_paliastransform)[1][i] *= m_fSoftwareYScale * (1.0 / (ZISCALE * 0x10000));
				(*m_paliastransform)[2][i] *= 1.0 / (ZISCALE * 0x10000);
			}
		}
	}

	(*m_protationmatrix)[0][3] = modelpos[0];
	(*m_protationmatrix)[1][3] = modelpos[1];
	(*m_protationmatrix)[2][3] = modelpos[2];
}

float CStudioModelRenderer::StudioEstimateInterpolant(void)
{
	float dadt = 1.0;

	if (m_fDoInterp && (m_pCurrentEntity->curstate.animtime >= m_pCurrentEntity->latched.prevanimtime + 0.01))
	{
		dadt = (m_clTime - m_pCurrentEntity->curstate.animtime) / 0.1;

		if (dadt > 2.0)
			dadt = 2.0;
	}

	return dadt;
}

void CStudioModelRenderer::StudioCalcRotations(float pos[][3], vec4_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f)
{
	int i;
	int frame;
	mstudiobone_t *pbone;

	float s;
	float adj[MAXSTUDIOCONTROLLERS];
	float dadt;

	if (f > pseqdesc->numframes - 1)
		f = 0;
	else if (f < -0.01)
		f = -0.01;

	frame = (int)f;
	dadt = StudioEstimateInterpolant();
	s = (f - frame);

	pbone = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	StudioCalcBoneAdj(dadt, adj, m_pCurrentEntity->curstate.controller, m_pCurrentEntity->latched.prevcontroller, m_pCurrentEntity->mouth.mouthopen);

	for (i = 0; i < m_pStudioHeader->numbones; i++, pbone++, panim++)
	{
		StudioCalcBoneQuaterion(frame, s, pbone, panim, adj, q[i]);
		StudioCalcBonePosition(frame, s, pbone, panim, adj, pos[i]);
	}

	if (pseqdesc->motiontype & STUDIO_X)
		pos[pseqdesc->motionbone][0] = 0.0;

	if (pseqdesc->motiontype & STUDIO_Y)
		pos[pseqdesc->motionbone][1] = 0.0;

	if (pseqdesc->motiontype & STUDIO_Z)
		pos[pseqdesc->motionbone][2] = 0.0;

	s = 0 * ((1.0 - (f - (int)(f))) / (pseqdesc->numframes)) * m_pCurrentEntity->curstate.framerate;

	if (pseqdesc->motiontype & STUDIO_LX)
		pos[pseqdesc->motionbone][0] += s * pseqdesc->linearmovement[0];

	if (pseqdesc->motiontype & STUDIO_LY)
		pos[pseqdesc->motionbone][1] += s * pseqdesc->linearmovement[1];

	if (pseqdesc->motiontype & STUDIO_LZ)
		pos[pseqdesc->motionbone][2] += s * pseqdesc->linearmovement[2];
}

void CStudioModelRenderer::StudioFxTransform(cl_entity_t *ent, float transform[3][4])
{
	switch (ent->curstate.renderfx)
	{
		case kRenderFxDistort:
		case kRenderFxHologram:
		{
			if (Com_RandomLong(0, 49) == 0)
			{
				int axis = Com_RandomLong(0, 1);

				if (axis == 1)
					axis = 2;

				VectorScale( transform[axis], gEngfuncs.pfnRandomFloat(1,1.484), transform[axis] );
			}
			else if (Com_RandomLong(0, 49) == 0)
			{
				float offset;
				int axis = Com_RandomLong(0, 1);

				if (axis == 1)
					axis = 2;

				offset = gEngfuncs.pfnRandomFloat(-10, 10);
				transform[Com_RandomLong(0, 2)][3] += offset;
			}

			break;
		}

		case kRenderFxExplode:
		{
			float scale;

			scale = 1.0 + (m_clTime - ent->curstate.animtime) * 10.0;

			if (scale > 2)
				scale = 2;

			transform[0][1] *= scale;
			transform[1][1] *= scale;
			transform[2][1] *= scale;
			break;
		}
	}
}

float CStudioModelRenderer::StudioEstimateFrame(mstudioseqdesc_t *pseqdesc)
{
	double dfdt, f;

	if (m_fDoInterp)
	{
		if (m_clTime < m_pCurrentEntity->curstate.animtime)
			dfdt = 0;
		else
			dfdt = (m_clTime - m_pCurrentEntity->curstate.animtime) * m_pCurrentEntity->curstate.framerate * pseqdesc->fps;
	}
	else
		dfdt = 0;

	if (pseqdesc->numframes <= 1)
		f = 0;
	else
		f = (m_pCurrentEntity->curstate.frame * (pseqdesc->numframes - 1)) / 256.0;

	f += dfdt;

	if (pseqdesc->flags & STUDIO_LOOPING)
	{
		if (pseqdesc->numframes > 1)
			f -= (int)(f / (pseqdesc->numframes - 1)) * (pseqdesc->numframes - 1);

		if (f < 0)
			f += (pseqdesc->numframes - 1);
	}
	else
	{
		if (f >= pseqdesc->numframes - 1.001)
			f = pseqdesc->numframes - 1.001;

		if (f < 0.0)
			f = 0.0;
	}

	return f;
}

void CStudioModelRenderer::StudioSetupBones(void)
{
	int i;
	double f;

	mstudiobone_t *pbones;
	mstudioseqdesc_t *pseqdesc;
	mstudioanim_t *panim;

	static float pos[MAXSTUDIOBONES][3];
	static vec4_t q[MAXSTUDIOBONES];
	float bonematrix[3][4];

	static float pos2[MAXSTUDIOBONES][3];
	static vec4_t q2[MAXSTUDIOBONES];
	static float pos3[MAXSTUDIOBONES][3];
	static vec4_t q3[MAXSTUDIOBONES];
	static float pos4[MAXSTUDIOBONES][3];
	static vec4_t q4[MAXSTUDIOBONES];

	if( m_pCurrentEntity->curstate.sequence >= m_pStudioHeader->numseq
		|| m_pCurrentEntity->curstate.sequence < 0 )
		m_pCurrentEntity->curstate.sequence = 0;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;

	f = StudioEstimateFrame(pseqdesc);
	panim = StudioGetAnim(m_pRenderModel, pseqdesc);

	StudioCalcRotations(pos, q, pseqdesc, panim, f);

	if (pseqdesc->numblends > 1)
	{
		float s;
		float dadt;

		panim += m_pStudioHeader->numbones;
		StudioCalcRotations(pos2, q2, pseqdesc, panim, f);

		dadt = StudioEstimateInterpolant();
		s = (m_pCurrentEntity->curstate.blending[0] * dadt + m_pCurrentEntity->latched.prevblending[0] * (1.0 - dadt)) / 255.0;

		StudioSlerpBones(q, pos, q2, pos2, s);

		if (pseqdesc->numblends == 4)
		{
			panim += m_pStudioHeader->numbones;
			StudioCalcRotations(pos3, q3, pseqdesc, panim, f);

			panim += m_pStudioHeader->numbones;
			StudioCalcRotations(pos4, q4, pseqdesc, panim, f);

			s = (m_pCurrentEntity->curstate.blending[0] * dadt + m_pCurrentEntity->latched.prevblending[0] * (1.0 - dadt)) / 255.0;
			StudioSlerpBones(q3, pos3, q4, pos4, s);

			s = (m_pCurrentEntity->curstate.blending[1] * dadt + m_pCurrentEntity->latched.prevblending[1] * (1.0 - dadt)) / 255.0;
			StudioSlerpBones(q, pos, q3, pos3, s);
		}
	}

	if (m_fDoInterp && m_pCurrentEntity->latched.sequencetime && (m_pCurrentEntity->latched.sequencetime + 0.2 > m_clTime) && (m_pCurrentEntity->latched.prevsequence < m_pStudioHeader->numseq))
	{
		static float pos1b[MAXSTUDIOBONES][3];
		static vec4_t q1b[MAXSTUDIOBONES];
		float s;

		pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->latched.prevsequence;
		panim = StudioGetAnim(m_pRenderModel, pseqdesc);

		StudioCalcRotations(pos1b, q1b, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);

		if (pseqdesc->numblends > 1)
		{
			panim += m_pStudioHeader->numbones;
			StudioCalcRotations(pos2, q2, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);

			s = (m_pCurrentEntity->latched.prevseqblending[0]) / 255.0;
			StudioSlerpBones(q1b, pos1b, q2, pos2, s);

			if (pseqdesc->numblends == 4)
			{
				panim += m_pStudioHeader->numbones;
				StudioCalcRotations(pos3, q3, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);

				panim += m_pStudioHeader->numbones;
				StudioCalcRotations(pos4, q4, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);

				s = (m_pCurrentEntity->latched.prevseqblending[0]) / 255.0;
				StudioSlerpBones(q3, pos3, q4, pos4, s);

				s = (m_pCurrentEntity->latched.prevseqblending[1]) / 255.0;
				StudioSlerpBones(q1b, pos1b, q3, pos3, s);
			}
		}

		s = 1.0 - (m_clTime - m_pCurrentEntity->latched.sequencetime) / 0.2;
		StudioSlerpBones(q, pos, q1b, pos1b, s);
	}
	else
	{
		m_pCurrentEntity->latched.prevframe = f;
	}

	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	if (m_pPlayerInfo && m_pPlayerInfo->gaitsequence != 0)
	{
		pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pPlayerInfo->gaitsequence;

		panim = StudioGetAnim(m_pRenderModel, pseqdesc);
		StudioCalcRotations(pos2, q2, pseqdesc, panim, m_pPlayerInfo->gaitframe);

		for (i = 0; i < m_pStudioHeader->numbones; i++)
		{
			if (strcmp(pbones[i].name, "Bip01 Spine") == 0)
				break;

			memcpy(pos[i], pos2[i], sizeof( pos[i]));
			memcpy(q[i], q2[i], sizeof( q[i]));
		}
	}

	bool bIsViewModel = gEngfuncs.GetViewModel() == m_pCurrentEntity;

	for (i = 0; i < m_pStudioHeader->numbones; i++)
	{
		QuaternionMatrix(q[i], bonematrix);

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1)
		{
			if (IEngineStudio.IsHardware())
			{
				// i know this looks HORRIBLE but I'm too lazy to simplify this right now
				if( gHUD.cl_righthand && gHUD.cl_righthand->value > 0.0f && bIsViewModel && !g_bHoldingShield || gHUD.GetGameType() == GAME_CZERO && bIsViewModel && g_bHoldingShield )
				{
					bonematrix[1][0] = -bonematrix[1][0];
					bonematrix[1][1] = -bonematrix[1][1];
					bonematrix[1][2] = -bonematrix[1][2];
					bonematrix[1][3] = -bonematrix[1][3];
				}

				ConcatTransforms((*m_protationmatrix), bonematrix, (*m_pbonetransform)[i]);
				MatrixCopy((*m_pbonetransform)[i], (*m_plighttransform)[i]);
			}
			else
			{
				ConcatTransforms((*m_paliastransform), bonematrix, (*m_pbonetransform)[i]);
				ConcatTransforms((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
			}

			StudioFxTransform(m_pCurrentEntity, (*m_pbonetransform)[i]);
		}
		else
		{
			ConcatTransforms((*m_pbonetransform)[pbones[i].parent], bonematrix, (*m_pbonetransform)[i]);
			ConcatTransforms((*m_plighttransform)[pbones[i].parent], bonematrix, (*m_plighttransform)[i]);
		}
	}
}

void CStudioModelRenderer::StudioSaveBones(void)
{
	int i;

	mstudiobone_t *pbones;
	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	m_nCachedBones = m_pStudioHeader->numbones;

	for (i = 0; i < m_pStudioHeader->numbones; i++)
	{
		strncpy(m_nCachedBoneNames[i], pbones[i].name, 32);
		MatrixCopy((*m_pbonetransform)[i], m_rgCachedBoneTransform[i]);
		MatrixCopy((*m_plighttransform)[i], m_rgCachedLightTransform[i]);
	}

}

void CStudioModelRenderer::StudioMergeBones(model_t *pSubModel)
{
	int i, j;
	double f;

	mstudiobone_t *pbones;
	mstudioseqdesc_t *pseqdesc;
	mstudioanim_t *panim;

	static float pos[MAXSTUDIOBONES][3];
	float bonematrix[3][4];
	static vec4_t q[MAXSTUDIOBONES];

	if( !m_pStudioHeader || !m_pCurrentEntity )
		return;

	if (m_pCurrentEntity->curstate.sequence >= m_pStudioHeader->numseq)
		m_pCurrentEntity->curstate.sequence = 0;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;

	f = StudioEstimateFrame(pseqdesc);

	/*if (m_pCurrentEntity->latched.prevframe > f)
	{
	}*/

	panim = StudioGetAnim(pSubModel, pseqdesc);
	StudioCalcRotations(pos, q, pseqdesc, panim, f);

	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	for (i = 0; i < m_pStudioHeader->numbones; i++)
	{
		for (j = 0; j < m_nCachedBones; j++)
		{
			if (stricmp(pbones[i].name, m_nCachedBoneNames[j]) == 0)
			{
				MatrixCopy(m_rgCachedBoneTransform[j], (*m_pbonetransform)[i]);
				MatrixCopy(m_rgCachedLightTransform[j], (*m_plighttransform)[i]);
				break;
			}
		}

		if (j >= m_nCachedBones)
		{
			QuaternionMatrix(q[i], bonematrix);

			bonematrix[0][3] = pos[i][0];
			bonematrix[1][3] = pos[i][1];
			bonematrix[2][3] = pos[i][2];

			if (pbones[i].parent == -1)
			{
				if (IEngineStudio.IsHardware())
				{
					ConcatTransforms((*m_protationmatrix), bonematrix, (*m_pbonetransform)[i]);
					MatrixCopy((*m_pbonetransform)[i], (*m_plighttransform)[i]);
				}
				else
				{
					ConcatTransforms((*m_paliastransform), bonematrix, (*m_pbonetransform)[i]);
					ConcatTransforms((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
				}

				StudioFxTransform(m_pCurrentEntity, (*m_pbonetransform)[i]);
			}
			else
			{
				ConcatTransforms((*m_pbonetransform)[pbones[i].parent], bonematrix, (*m_pbonetransform)[i]);
				ConcatTransforms((*m_plighttransform)[pbones[i].parent], bonematrix, (*m_plighttransform)[i]);
			}
		}
	}
}

int CStudioModelRenderer::StudioDrawModel(int flags)
{
	bool bChangedRightHand = false;
	int iRightHandValue;

	m_pCurrentEntity = IEngineStudio.GetCurrentEntity();

	if( g_bHoldingKnife && m_pCurrentEntity == gEngfuncs.GetViewModel() && (flags & STUDIO_RENDER) )
	{
		bChangedRightHand = true;

		iRightHandValue = gHUD.cl_righthand->value;

		gHUD.cl_righthand->value = !gHUD.cl_righthand->value;
	}

	IEngineStudio.GetTimes(&m_nFrameCount, &m_clTime, &m_clOldTime);
	IEngineStudio.GetViewInfo(m_vRenderOrigin, m_vUp, m_vRight, m_vNormal);
	IEngineStudio.GetAliasScale(&m_fSoftwareXScale, &m_fSoftwareYScale);

	if (m_pCurrentEntity->curstate.renderfx == kRenderFxDeadPlayer)
	{
		entity_state_t deadplayer;

		int result;
		int save_interp;

		if (m_pCurrentEntity->curstate.renderamt <= 0 || m_pCurrentEntity->curstate.renderamt > gEngfuncs.GetMaxClients())
			return 0;

		deadplayer = *(IEngineStudio.GetPlayerState(m_pCurrentEntity->curstate.renderamt - 1));

		deadplayer.number = m_pCurrentEntity->curstate.renderamt;
		deadplayer.weaponmodel = 0;
		deadplayer.gaitsequence = 0;

		deadplayer.movetype = MOVETYPE_NONE;
		VectorCopy(m_pCurrentEntity->curstate.angles, deadplayer.angles);
		VectorCopy(m_pCurrentEntity->curstate.origin, deadplayer.origin);

		save_interp = m_fDoInterp;
		m_fDoInterp = 0;

		result = StudioDrawPlayer(flags, &deadplayer);

		m_fDoInterp = save_interp;
		return result;
	}

	m_pRenderModel = m_pCurrentEntity->model;
	m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(m_pRenderModel);

	IEngineStudio.StudioSetHeader(m_pStudioHeader);
	IEngineStudio.SetRenderModel(m_pRenderModel);

	StudioSetUpTransform(0);

	if (flags & STUDIO_RENDER)
	{
		if (!IEngineStudio.StudioCheckBBox())
			return 0;

		(*m_pModelsDrawn)++;
		(*m_pStudioModelCount)++;

		if (m_pStudioHeader->numbodyparts == 0)
			return 1;
	}

	if (m_pCurrentEntity->curstate.movetype == MOVETYPE_FOLLOW)
		StudioMergeBones(m_pRenderModel);
	else
		StudioSetupBones();

	StudioSaveBones();

	if (flags & STUDIO_EVENTS)
	{
		StudioCalcAttachments();
		IEngineStudio.StudioClientEvents();

		if (m_pCurrentEntity->index > 0)
		{
			cl_entity_t *ent = gEngfuncs.GetEntityByIndex(m_pCurrentEntity->index);
			memcpy(ent->attachment, m_pCurrentEntity->attachment, sizeof(vec3_t) * 4);
		}
	}

	if (flags & STUDIO_RENDER)
	{
		alight_t lighting;
		vec3_t dir;
		lighting.plightvec = dir;

		IEngineStudio.StudioDynamicLight(m_pCurrentEntity, &lighting);

		IEngineStudio.StudioEntityLight(&lighting);
		IEngineStudio.StudioSetupLighting(&lighting);

		m_nTopColor = m_pCurrentEntity->curstate.colormap & 0xFF;
		m_nBottomColor = (m_pCurrentEntity->curstate.colormap & 0xFF00) >> 8;

		IEngineStudio.StudioSetRemapColors(m_nTopColor, m_nBottomColor);

		StudioRenderModel(dir);
	}

	if( bChangedRightHand )
	{
		gHUD.cl_righthand->value = iRightHandValue;
	}

	return 1;
}

void CStudioModelRenderer::StudioEstimateGait(entity_state_t *pplayer)
{
	float dt;
	vec3_t est_velocity;

	dt = (m_clTime - m_clOldTime);

	if (dt < 0)
		dt = 0;
	else if (dt > 1.0)
		dt = 1;

	if (dt == 0 || m_pPlayerInfo->renderframe == m_nFrameCount)
	{
		m_flGaitMovement = 0;
		return;
	}

	if (m_fGaitEstimation)
	{
		VectorSubtract(m_pCurrentEntity->origin, m_pPlayerInfo->prevgaitorigin, est_velocity);
		VectorCopy(m_pCurrentEntity->origin, m_pPlayerInfo->prevgaitorigin);
		m_flGaitMovement = est_velocity.Length();

		if (dt <= 0 || m_flGaitMovement / dt < 5)
		{
			m_flGaitMovement = 0;
			est_velocity[0] = 0;
			est_velocity[1] = 0;
		}
	}
	else
	{
		VectorCopy(pplayer->velocity, est_velocity);
		m_flGaitMovement = est_velocity.Length() * dt;
	}

	if (est_velocity[1] == 0 && est_velocity[0] == 0)
	{
		float flYawDiff = m_pCurrentEntity->angles[YAW] - m_pPlayerInfo->gaityaw;
		flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;

		if (flYawDiff > 180)
			flYawDiff -= 360;
		if (flYawDiff < -180)
			flYawDiff += 360;

		if (dt < 0.25)
			flYawDiff *= dt * 4;
		else
			flYawDiff *= dt;

		m_pPlayerInfo->gaityaw += flYawDiff;
		m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw - (int)(m_pPlayerInfo->gaityaw / 360) * 360;
		m_flGaitMovement = 0;
	}
	else
	{
		m_pPlayerInfo->gaityaw = (atan2(est_velocity[1], est_velocity[0]) * 180 / M_PI);

		if (m_pPlayerInfo->gaityaw > 180)
			m_pPlayerInfo->gaityaw = 180;

		if (m_pPlayerInfo->gaityaw < -180)
			m_pPlayerInfo->gaityaw = -180;
	}
}

void CStudioModelRenderer::StudioProcessGait(entity_state_t *pplayer)
{
	mstudioseqdesc_t *pseqdesc;
	float dt;
	int iBlend;
	float flYaw;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;

	StudioPlayerBlend(pseqdesc, &iBlend, &m_pCurrentEntity->angles[PITCH]);

	m_pCurrentEntity->latched.prevangles[PITCH] = m_pCurrentEntity->angles[PITCH];
	m_pCurrentEntity->curstate.blending[0] = iBlend;
	m_pCurrentEntity->latched.prevblending[0] = m_pCurrentEntity->curstate.blending[0];
	m_pCurrentEntity->latched.prevseqblending[0] = m_pCurrentEntity->curstate.blending[0];

	dt = (m_clTime - m_clOldTime);

	if (dt < 0)
		dt = 0;
	else if (dt > 1.0)
		dt = 1;

	StudioEstimateGait(pplayer);

	flYaw = m_pCurrentEntity->angles[YAW] - m_pPlayerInfo->gaityaw;
	flYaw = flYaw - (int)(flYaw / 360) * 360;

	if (flYaw < -180)
		flYaw = flYaw + 360;

	if (flYaw > 180)
		flYaw = flYaw - 360;

	if (flYaw > 120)
	{
		m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw - 180;
		m_flGaitMovement = -m_flGaitMovement;
		flYaw = flYaw - 180;
	}
	else if (flYaw < -120)
	{
		m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw + 180;
		m_flGaitMovement = -m_flGaitMovement;
		flYaw = flYaw + 180;
	}

	m_pCurrentEntity->curstate.controller[0] = ((flYaw / 4.0) + 30) / (60.0 / 255.0);
	m_pCurrentEntity->curstate.controller[1] = ((flYaw / 4.0) + 30) / (60.0 / 255.0);
	m_pCurrentEntity->curstate.controller[2] = ((flYaw / 4.0) + 30) / (60.0 / 255.0);
	m_pCurrentEntity->curstate.controller[3] = ((flYaw / 4.0) + 30) / (60.0 / 255.0);
	m_pCurrentEntity->latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
	m_pCurrentEntity->latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
	m_pCurrentEntity->latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];
	m_pCurrentEntity->latched.prevcontroller[3] = m_pCurrentEntity->curstate.controller[3];

	m_pCurrentEntity->angles[YAW] = m_pPlayerInfo->gaityaw;

	if (m_pCurrentEntity->angles[YAW] < -0)
		m_pCurrentEntity->angles[YAW] += 360;

	m_pCurrentEntity->latched.prevangles[YAW] = m_pCurrentEntity->angles[YAW];

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + pplayer->gaitsequence;

	if (pseqdesc->linearmovement[0] > 0)
		m_pPlayerInfo->gaitframe += (m_flGaitMovement / pseqdesc->linearmovement[0]) * pseqdesc->numframes;
	else
		m_pPlayerInfo->gaitframe += pseqdesc->fps * dt;

	m_pPlayerInfo->gaitframe = m_pPlayerInfo->gaitframe - (int)(m_pPlayerInfo->gaitframe / pseqdesc->numframes) * pseqdesc->numframes;

	if (m_pPlayerInfo->gaitframe < 0)
		m_pPlayerInfo->gaitframe += pseqdesc->numframes;
}

int CStudioModelRenderer::StudioDrawPlayer(int flags, entity_state_t *pplayer)
{
	m_pCurrentEntity = IEngineStudio.GetCurrentEntity();

	IEngineStudio.GetTimes(&m_nFrameCount, &m_clTime, &m_clOldTime);
	IEngineStudio.GetViewInfo(m_vRenderOrigin, m_vUp, m_vRight, m_vNormal);
	IEngineStudio.GetAliasScale(&m_fSoftwareXScale, &m_fSoftwareYScale);

	m_nPlayerIndex = pplayer->number - 1;

	if (m_nPlayerIndex < 0 || m_nPlayerIndex >= gEngfuncs.GetMaxClients())
		return 0;

	m_pRenderModel = IEngineStudio.SetupPlayerModel(m_nPlayerIndex);

	if (m_pRenderModel == NULL)
		return 0;

	m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(m_pRenderModel);

	IEngineStudio.StudioSetHeader(m_pStudioHeader);
	IEngineStudio.SetRenderModel(m_pRenderModel);

	if (pplayer->gaitsequence)
	{
		vec3_t orig_angles;
		m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);

		VectorCopy(m_pCurrentEntity->angles, orig_angles);

		StudioProcessGait(pplayer);

		m_pPlayerInfo->gaitsequence = pplayer->gaitsequence;
		m_pPlayerInfo = NULL;

		StudioSetUpTransform(0);
		VectorCopy(orig_angles, m_pCurrentEntity->angles);
	}
	else
	{
		m_pCurrentEntity->curstate.controller[0] = 127;
		m_pCurrentEntity->curstate.controller[1] = 127;
		m_pCurrentEntity->curstate.controller[2] = 127;
		m_pCurrentEntity->curstate.controller[3] = 127;
		m_pCurrentEntity->latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
		m_pCurrentEntity->latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
		m_pCurrentEntity->latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];
		m_pCurrentEntity->latched.prevcontroller[3] = m_pCurrentEntity->curstate.controller[3];

		m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);
		m_pPlayerInfo->gaitsequence = 0;

		StudioSetUpTransform(0);
	}

	if (flags & STUDIO_RENDER)
	{
		if (!IEngineStudio.StudioCheckBBox())
			return 0;

		(*m_pModelsDrawn)++;
		(*m_pStudioModelCount)++;

		if (m_pStudioHeader->numbodyparts == 0)
			return 1;
	}

	m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);

	StudioSetupBones();
	StudioSaveBones();

	m_pPlayerInfo->renderframe = m_nFrameCount;
	m_pPlayerInfo = NULL;

	if (flags & STUDIO_EVENTS)
	{
		StudioCalcAttachments();
		IEngineStudio.StudioClientEvents();

		if (m_pCurrentEntity->index > 0)
		{
			cl_entity_t *ent = gEngfuncs.GetEntityByIndex(m_pCurrentEntity->index);
			memcpy(ent->attachment, m_pCurrentEntity->attachment, sizeof(vec3_t) * 4);
		}
	}

	if (flags & STUDIO_RENDER)
	{
		if (m_pCvarHiModels->value && m_pRenderModel != m_pCurrentEntity->model)
			m_pCurrentEntity->curstate.body = 255;

		if (!(m_pCvarDeveloper->value == 0 && gEngfuncs.GetMaxClients() == 1) && (m_pRenderModel == m_pCurrentEntity->model))
			m_pCurrentEntity->curstate.body = 1;

		alight_t lighting;
		vec3_t dir;
		lighting.plightvec = dir;

		IEngineStudio.StudioDynamicLight(m_pCurrentEntity, &lighting);
		IEngineStudio.StudioEntityLight(&lighting);
		IEngineStudio.StudioSetupLighting(&lighting);

		m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);

		m_nTopColor = m_pPlayerInfo->topcolor;

		if (m_nTopColor < 0)
			m_nTopColor = 0;

		if (m_nTopColor > 360)
			m_nTopColor = 360;

		m_nBottomColor = m_pPlayerInfo->bottomcolor;

		if (m_nBottomColor < 0)
			m_nBottomColor = 0;

		if (m_nBottomColor > 360)
			m_nBottomColor = 360;

		IEngineStudio.StudioSetRemapColors(m_nTopColor, m_nBottomColor);

		StudioRenderModel(dir);
		m_pPlayerInfo = NULL;

		if (pplayer->weaponmodel)
		{
			cl_entity_t saveent = *m_pCurrentEntity;
			model_t *pweaponmodel = IEngineStudio.GetModelByIndex(pplayer->weaponmodel);

			m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(pweaponmodel);
			IEngineStudio.StudioSetHeader(m_pStudioHeader);

			StudioMergeBones(pweaponmodel);

			IEngineStudio.StudioSetupLighting(&lighting);

			StudioRenderModel(dir);

			StudioCalcAttachments();

			*m_pCurrentEntity = saveent;
		}
	}

	return 1;
}

void CStudioModelRenderer::StudioCalcAttachments(void)
{
	int i;
	mstudioattachment_t *pattachment;

	if (m_pStudioHeader->numattachments > 4)
		gEngfuncs.Con_DPrintf("Too many attachments on %s\n", m_pCurrentEntity->model->name);

	pattachment = (mstudioattachment_t *)((byte *)m_pStudioHeader + m_pStudioHeader->attachmentindex);

	for (i = 0; i < m_pStudioHeader->numattachments; i++)
		VectorTransform(pattachment[i].org, (*m_plighttransform)[pattachment[i].bone], m_pCurrentEntity->attachment[i]);
}

void CStudioModelRenderer::StudioRenderModel(float *lightdir)
{
	IEngineStudio.SetChromeOrigin();

	int iSaveRenderMode =  m_pCurrentEntity->curstate.rendermode;
	int iSaveRenderFx = m_pCurrentEntity->curstate.renderfx;
	int iSaveRenderAmt = m_pCurrentEntity->curstate.renderamt;


	IEngineStudio.SetForceFaceFlags(0);
	StudioRenderFinal();

	if (iSaveRenderFx == kRenderFxGlowShell)
	{
		m_pCurrentEntity->curstate.renderfx = kRenderFxGlowShell;

		gEngfuncs.pTriAPI->SpriteTexture(m_pChromeSprite, 0);

		IEngineStudio.SetForceFaceFlags(STUDIO_NF_CHROME);
		StudioRenderFinal();
	}


	m_pCurrentEntity->curstate.rendermode = iSaveRenderMode;
	m_pCurrentEntity->curstate.renderfx = iSaveRenderFx;
	m_pCurrentEntity->curstate.renderamt = iSaveRenderAmt;
}

void CStudioModelRenderer::StudioRenderFinal_Software(void)
{
	int i;

	IEngineStudio.SetupRenderer(0);

	/*if (m_pCvarDrawEntities->value == 2)
	{
		IEngineStudio.StudioDrawBones();
	}
	else if (m_pCvarDrawEntities->value == 3)
	{
		IEngineStudio.StudioDrawHulls();
	}
	else*/
	{
		for (i = 0; i < m_pStudioHeader->numbodyparts; i++)
		{
			IEngineStudio.StudioSetupModel(i, (void **)&m_pBodyPart, (void **)&m_pSubModel);
			IEngineStudio.StudioDrawPoints();
		}
	}

	/*if (m_pCvarDrawEntities->value == 4)
	{
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
		IEngineStudio.StudioDrawHulls();
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	}

	if (m_pCvarDrawEntities->value == 5)
		IEngineStudio.StudioDrawAbsBBox();*/

	IEngineStudio.RestoreRenderer();
}

int twice;

void CStudioModelRenderer::StudioRenderFinal_Hardware(void)
{
	int i;
	int rendermode;

	rendermode = IEngineStudio.GetForceFaceFlags() ? kRenderTransAdd : m_pCurrentEntity->curstate.rendermode;
	IEngineStudio.SetupRenderer(rendermode);

	/*if (m_pCvarDrawEntities->value == 2)
	{
		IEngineStudio.StudioDrawBones();
	}
	else if (m_pCvarDrawEntities->value == 3)
	{
		IEngineStudio.StudioDrawHulls();
	}
	else*/
	{
		for (i = 0; i < m_pStudioHeader->numbodyparts; i++)
		{
			IEngineStudio.StudioSetupModel(i, (void **)&m_pBodyPart, (void **)&m_pSubModel);

			if (m_fDoInterp)
				m_pCurrentEntity->trivial_accept = 0;

			IEngineStudio.GL_SetRenderMode(rendermode);
			IEngineStudio.StudioSetRenderamt(m_pCurrentEntity->curstate.renderamt);
			IEngineStudio.StudioDrawPoints();
		}
	}

	/*if (m_pCvarDrawEntities->value == 4)
	{
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
		IEngineStudio.StudioDrawHulls();
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	}*/

	//if (m_pCvarDrawEntities->value == 5)
	//	IEngineStudio.StudioDrawAbsBBox();

	IEngineStudio.RestoreRenderer();
}

void CStudioModelRenderer::StudioRenderFinal(void)
{
	if (IEngineStudio.IsHardware())
		StudioRenderFinal_Hardware();
	else
		StudioRenderFinal_Software();
}

void CStudioModelRenderer::StudioSetShadowSprite(int idx)
{
	m_iShadowSprite = idx;
}

void CStudioModelRenderer::StudioDrawShadow( Vector origin, float scale )
{
	Vector endPoint = origin;
	Vector p1, p2, p3, p4;
	pmtrace_t pmtrace;

	endPoint.z -= 150.0f;

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates( );
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( -1 );
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( origin, endPoint, PM_STUDIO_IGNORE | PM_GLASS_IGNORE, -1, &pmtrace );
	gEngfuncs.pEventAPI->EV_PopPMStates( );

	// don't allow shadow if player in solid area
	if( pmtrace.startsolid )
		return;

	// don't allow shadow if doesn't hit anything
	if( pmtrace.fraction >= 1.0f )
		return;

	pmtrace.plane.normal = pmtrace.plane.normal.Normalize( );

	// don't allow shadow on too lean planes
	if( pmtrace.plane.normal.z <= 0.7 )
		return;

	pmtrace.plane.normal = pmtrace.plane.normal * scale * ( 1.0 - pmtrace.fraction );


	// add 2.0f to Z, for avoid Z-fighting
	p1.x = pmtrace.endpos.x - pmtrace.plane.normal.z;
	p1.y = pmtrace.endpos.y + pmtrace.plane.normal.z;
	p1.z = pmtrace.endpos.z + 2.0f + pmtrace.plane.normal.x - pmtrace.plane.normal.y;

	p2.x = pmtrace.endpos.x + pmtrace.plane.normal.z;
	p2.y = pmtrace.endpos.y + pmtrace.plane.normal.z;
	p2.z = pmtrace.endpos.z + 2.0f - pmtrace.plane.normal.x - pmtrace.plane.normal.y;

	p3.x = pmtrace.endpos.x + pmtrace.plane.normal.z;
	p3.y = pmtrace.endpos.y - pmtrace.plane.normal.z;
	p3.z = pmtrace.endpos.z + 2.0f - pmtrace.plane.normal.x + pmtrace.plane.normal.y;

	p4.x = pmtrace.endpos.x - pmtrace.plane.normal.z;
	p4.y = pmtrace.endpos.y - pmtrace.plane.normal.z;
	p4.z = pmtrace.endpos.z + 2.0f + pmtrace.plane.normal.x + pmtrace.plane.normal.y;

	IEngineStudio.StudioRenderShadow( m_iShadowSprite, p1, p2, p3, p4 );
}
