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
#ifndef BASEMONSTER_H
#define BASEMONSTER_H

class CBaseMonster : public CBaseToggle
{
public:
	virtual void KeyValue(KeyValueData *pkvd) { }
	virtual float ChangeYaw(int speed) { return 0; }
	virtual BOOL HasHumanGibs(void) { return FALSE; }
	virtual BOOL HasAlienGibs(void) { return FALSE; }
	virtual void FadeMonster(void) { }
	virtual void GibMonster(void) { }
	virtual Activity GetDeathActivity(void) { return ACT_DIE_HEADSHOT; }
	virtual void BecomeDead(void) { }
	virtual BOOL ShouldFadeOnDeath(void) { return FALSE; }
	virtual int IRelationship(CBaseEntity *pTarget) { return 0; }
	virtual int TakeHealth(float flHealth, int bitsDamageType) { return 0; }
	virtual int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType) { return 0; }
	virtual void Killed(entvars_t *pevAttacker, int iGib) { }
	virtual void PainSound(void) { return; }
	virtual void ResetMaxSpeed(void) {}
	virtual void ReportAIState(void) { }
	virtual void MonsterInitDead(void) { }
	virtual void Look(int iDistance) { }
	virtual CBaseEntity *BestVisibleEnemy(void) { return NULL; }
	virtual BOOL FInViewCone(CBaseEntity *pEntity) { return FALSE; }
	virtual BOOL FInViewCone(Vector *pOrigin) { return FALSE; }
	virtual int BloodColor(void) { return m_bloodColor; }
	virtual BOOL IsAlive(void) { return (pev->deadflag != DEAD_DEAD); }

public:
	void MakeIdealYaw(Vector vecTarget);
	Activity GetSmallFlinchActivity(void);
	BOOL ShouldGibMonster(int iGib);
	void CallGibMonster(void);
	BOOL FCheckAITrigger(void);
	int DeadTakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	float DamageForce(float damage);
	void RadiusDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType);
	void RadiusDamage(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType);
	void EXPORT CorpseFallThink(void);
	CBaseEntity *CheckTraceHullAttack(float flDist, int iDamage, int iDmgType);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType) {}
	void MakeDamageBloodDecal(int cCount, float flNoise, TraceResult *ptr, const Vector &vecDir);
	void BloodSplat(const Vector &vecPos, const Vector &vecDir, int hitgroup, int iDamage);

public:
	inline void SetConditions(int iConditions) { m_afConditions |= iConditions; }
	inline void ClearConditions(int iConditions) { m_afConditions &= ~iConditions; }
	inline BOOL HasConditions(int iConditions) { if (m_afConditions & iConditions) return TRUE; return FALSE; }
	inline BOOL HasAllConditions(int iConditions) { if ((m_afConditions & iConditions) == iConditions) return TRUE; return FALSE; }
	inline void Remember(int iMemory) { m_afMemory |= iMemory; }
	inline void Forget(int iMemory) { m_afMemory &= ~iMemory; }
	inline BOOL HasMemory(int iMemory) { if (m_afMemory & iMemory) return TRUE; return FALSE; }
	inline BOOL HasAllMemories(int iMemory) { if ((m_afMemory & iMemory) == iMemory) return TRUE; return FALSE; }
	inline void StopAnimation(void) { pev->framerate = 0; }

public:
	Activity m_Activity;
	Activity m_IdealActivity;
	int m_LastHitGroup;
	int m_bitsDamageType;
	BYTE m_rgbTimeBasedDamage[CDMG_TIMEBASED];
	MONSTERSTATE m_MonsterState;
	MONSTERSTATE m_IdealMonsterState;
	int m_afConditions;
	int m_afMemory;
	float m_flNextAttack;
	EHANDLE m_hEnemy;
	EHANDLE m_hTargetEnt;
	float m_flFieldOfView;
	int m_bloodColor;
	Vector m_HackedGunPos;
	Vector m_vecEnemyLKP;
};
#endif
