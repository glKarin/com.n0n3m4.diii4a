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
#pragma once
#ifndef STUDIOMODELRENDERER_H
#define STUDIOMODELRENDERER_H

#include "studio.h"
#include "com_model.h"

class CStudioModelRenderer
{
public:
	CStudioModelRenderer(void);
	virtual ~CStudioModelRenderer(void);

public:
	virtual void Init(void);
	virtual int StudioDrawModel(int flags);
	virtual int StudioDrawPlayer(int flags, struct entity_state_s *pplayer);

public:
	virtual mstudioanim_t *StudioGetAnim(model_t *pSubModel, mstudioseqdesc_t *pseqdesc);
	virtual void StudioSetUpTransform(int trivial_accept);
	virtual void StudioSetupBones(void);
	virtual void StudioCalcAttachments(void);
	virtual void StudioSaveBones(void);
	virtual void StudioMergeBones(model_t *pSubModel);
	virtual float StudioEstimateInterpolant(void);
	virtual float StudioEstimateFrame(mstudioseqdesc_t *pseqdesc);
	virtual void StudioFxTransform(cl_entity_t *ent, float transform[3][4]);
	virtual void StudioSlerpBones(vec4_t q1[], float pos1[][3], vec4_t q2[], float pos2[][3], float s);
	virtual void StudioCalcBoneAdj(float dadt, float *adj, const byte *pcontroller1, const byte *pcontroller2, byte mouthopen);
	virtual void StudioCalcBoneQuaterion(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *q);
	virtual void StudioCalcBonePosition(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *pos);
	virtual void StudioCalcRotations(float pos[][3], vec4_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f);
	virtual void StudioRenderModel(float *lightdir);
	virtual void StudioRenderFinal(void);
	virtual void StudioRenderFinal_Software(void);
	virtual void StudioRenderFinal_Hardware(void);
	virtual void StudioPlayerBlend(mstudioseqdesc_t *pseqdesc, int *pBlend, float *pPitch);
	virtual void StudioEstimateGait(entity_state_t *pplayer);
	virtual void StudioProcessGait(entity_state_t *pplayer);
	virtual void StudioSetShadowSprite(int idx);
	void StudioDrawShadow(Vector origin, float scale);


public:
	double m_clTime;
	double m_clOldTime;
	int m_fDoInterp;
	int m_iShadowSprite;
	int m_fGaitEstimation;
	int m_nFrameCount;
	cvar_t *m_pCvarHiModels;
	cvar_t *m_pCvarDeveloper;
	cvar_t *m_pCvarDrawEntities;
	cvar_t *m_pCvarShadows;
	cvar_t *m_pCvarDebug;
	cl_entity_t *m_pCurrentEntity;
	model_t *m_pRenderModel;
	player_info_t *m_pPlayerInfo;
	int m_nPlayerIndex;
	float m_flGaitMovement;
	studiohdr_t *m_pStudioHeader;
	mstudiobodyparts_t *m_pBodyPart;
	mstudiomodel_t *m_pSubModel;
	int m_nTopColor;
	int m_nBottomColor;
	model_t *m_pChromeSprite;
	int m_nCachedBones;
	char m_nCachedBoneNames[MAXSTUDIOBONES][32];
	float m_rgCachedBoneTransform[MAXSTUDIOBONES][3][4];
	float m_rgCachedLightTransform[MAXSTUDIOBONES][3][4];
	float m_fSoftwareXScale, m_fSoftwareYScale;
	float m_vUp[3];
	float m_vRight[3];
	float m_vNormal[3];
	float m_vRenderOrigin[3];
	int *m_pStudioModelCount;
	int *m_pModelsDrawn;
	float (*m_protationmatrix)[3][4];
	float (*m_paliastransform)[3][4];
	float (*m_pbonetransform)[MAXSTUDIOBONES][3][4];
	float (*m_plighttransform)[MAXSTUDIOBONES][3][4];
	entity_state_t *m_pplayer;
};

#endif
