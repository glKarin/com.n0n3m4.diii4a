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
#ifndef MONSTERS_H
#include "skill.h"
#define MONSTERS_H

typedef enum
{
	HITGROUP_GENERIC,
	HITGROUP_HEAD,
	HITGROUP_CHEST,
	HITGROUP_STOMACH,
	HITGROUP_LEFTARM,
	HITGROUP_RIGHTARM,
	HITGROUP_LEFTLEG,
	HITGROUP_RIGHTLEG,
	HITGROUP_SHIELD,
	NUM_HITGROUPS
}
HitBoxGroup;

#define SF_MONSTER_WAIT_TILL_SEEN 1
#define SF_MONSTER_GAG 2
#define SF_MONSTER_HITMONSTERCLIP 4
#define SF_MONSTER_PRISONER 16
#define SF_MONSTER_WAIT_FOR_SCRIPT 128
#define SF_MONSTER_PREDISASTER 256
#define SF_MONSTER_FADECORPSE 512
#define SF_MONSTER_FALL_TO_GROUND 0x80000000
#define SF_MONSTER_TURRET_AUTOACTIVATE 32
#define SF_MONSTER_TURRET_STARTINACTIVE 64

extern void UTIL_MoveToOrigin(edict_t *pent, const Vector &vecGoal, float flDist, int iMoveType);
Vector VecCheckToss(entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flGravityAdj = 1);
Vector VecCheckThrow(entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj = 1);

extern DLL_GLOBAL Vector g_vecAttackDir;
extern DLL_GLOBAL CONSTANT float g_flMeleeRange;
extern DLL_GLOBAL CONSTANT float g_flMediumRange;
extern DLL_GLOBAL CONSTANT float g_flLongRange;

extern void EjectBrass(const Vector &vecOrigin, const Vector &vecLeft, const Vector &vecVelocity, float rotation, int model, int soundtype, int entityIndex);
extern void EjectBrass2(const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype, entvars_t *pev);
extern void ExplodeModel(const Vector &vecOrigin, float speed, int model, int count);

BOOL FBoxVisible(entvars_t *pevLooker, entvars_t *pevTarget);
BOOL FBoxVisible(entvars_t *pevLooker, entvars_t *pevTarget, Vector &vecTargetOrigin, float flSize = 0);

#define R_AL -2
#define R_FR -1
#define R_NO 0
#define R_DL 1
#define R_HT 2
#define R_NM 3

#define bits_MEMORY_KILLED (1<<7)

class CGib : public CBaseEntity
{
public:
	void Spawn(const char *szGibModel); // -V762
	int ObjectCaps(void) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }

public:
	void EXPORT BounceGibTouch(CBaseEntity *pOther);
	void EXPORT StickyGibTouch(CBaseEntity *pOther);
	void EXPORT WaitTillLand(void);
	void LimitVelocity(void);

public:
	static void SpawnHeadGib(entvars_t *pevVictim);
	static void SpawnRandomGibs(entvars_t *pevVictim, int cGibs, int human);
	static void SpawnStickyGibs(entvars_t *pevVictim, Vector vecOrigin, int cGibs);

public:
	int m_bloodColor;
	int m_cBloodDecals;
	int m_material;
	float m_lifeTime;
};

#endif
