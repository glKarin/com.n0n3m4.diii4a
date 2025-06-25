
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
#if !defined (GAMESTUDIOMODELRENDERER_H)
#define GAMESTUDIOMODELRENDERER_H

#include "studio.h"
#include "StudioModelRenderer.h"

enum BoneIndex
{
	BONE_HEAD,
	BONE_PELVIS,
	BONE_SPINE1,
	BONE_SPINE2,
	BONE_SPINE3,
	BONE_MAX,
};

struct client_anim_state_t
{
	vec3_t origin;
	vec3_t angles;

	vec3_t realangles;

	float animtime;
	float frame;
	int sequence;
	int gaitsequence;
	float framerate;

	int m_fSequenceLoops;
	int m_fSequenceFinished;

	byte controller[4];
	byte blending[2];

	latchedvars_t lv;
};

class CGameStudioModelRenderer : public CStudioModelRenderer
{
public:
	CGameStudioModelRenderer(void);

public:
	virtual void StudioSetupBones(void);
	virtual void StudioEstimateGait(entity_state_t *pplayer);
	virtual void StudioProcessGait(entity_state_t *pplayer);
	virtual int StudioDrawPlayer(int flags, entity_state_t *pplayer);
	virtual int _StudioDrawPlayer(int flags, entity_state_t *pplayer);
	virtual void StudioFxTransform(cl_entity_t *ent, float transform[3][4]);
	virtual void StudioPlayerBlend(mstudioseqdesc_t *pseqdesc, int *pBlend, float *pPitch);
	virtual void CalculateYawBlend(entity_state_t *pplayer);
	virtual void CalculatePitchBlend(entity_state_t *pplayer);

private:
	void SavePlayerState(entity_state_t *pplayer);
	void SetupClientAnimation(entity_state_t *pplayer);
	void RestorePlayerState(entity_state_t *pplayer);
	mstudioanim_t* LookupAnimation(mstudioseqdesc_t *pseqdesc, int index);

private:
	int m_nPlayerGaitSequences[MAX_CLIENTS];
	bool m_bLocal;
};

extern CGameStudioModelRenderer g_StudioRenderer;
extern int g_rseq;
extern int g_gaitseq;
extern Vector g_clorg;
extern Vector g_clang;

#endif
