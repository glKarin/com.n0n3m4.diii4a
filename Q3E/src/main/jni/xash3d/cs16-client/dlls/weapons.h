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
#ifndef WEAPONS_H
#define WEAPONS_H

#include "effects.h"

class CBasePlayer;
extern int gmsgWeapPickup;
extern int gmsgReloadSound;

class CGrenade : public CBaseMonster
{
public:
	void Spawn(void);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	virtual void BounceSound(void);
	int ObjectCaps(void) { return m_bIsC4 != false ? FCAP_CONTINUOUS_USE : 0; }
	int BloodColor(void) { return DONT_BLEED; }
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Killed(entvars_t *pevAttacker, int iGib);

public:
	typedef enum { SATCHEL_DETONATE = 0, SATCHEL_RELEASE } SATCHELCODE;

public:
#ifndef CLIENT_DLL
	static CGrenade *ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time);
	static CGrenade *ShootTimed2(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, int iTeam, unsigned short usEvent);
	static CGrenade *ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
	static CGrenade *ShootSmokeGrenade(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, unsigned short usEvent);
	static CGrenade *ShootSatchelCharge(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
#else
	static CGrenade *ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time) { return NULL; }
	static CGrenade *ShootTimed2(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, int iTeam, unsigned short usEvent) { return NULL; }
	static CGrenade *ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity) { return NULL; }
	static CGrenade *ShootSmokeGrenade(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, unsigned short usEvent) { return NULL; }
	static CGrenade *ShootSatchelCharge(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity) { return NULL; }
#endif
	static void UseSatchelCharges(entvars_t *pevOwner, SATCHELCODE code);

public:
	void Explode(Vector vecSrc, Vector vecAim);
	void Explode(TraceResult *pTrace, int bitsDamageType);
	void Explode2(TraceResult *pTrace, int bitsDamageType);
	void Explode3(TraceResult *pTrace, int bitsDamageType);
	void EXPORT Smoke(void);
	void EXPORT SG_Smoke(void);
	void EXPORT Smoke2(void);
	void EXPORT Smoke3_A(void);
	void EXPORT Smoke3_B(void);
	void EXPORT Smoke3_C(void);
	void EXPORT BounceTouch(CBaseEntity *pOther);
	void EXPORT SlideTouch(CBaseEntity *pOther);
	void EXPORT ExplodeTouch(CBaseEntity *pOther);
	void EXPORT DangerSoundThink(void);
	void EXPORT PreDetonate(void);
	void EXPORT Detonate(void);
	void EXPORT SG_Detonate(void);
	void EXPORT Detonate2(void);
	void EXPORT Detonate3(void);
	void EXPORT DetonateUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT TumbleThink(void);
	void EXPORT SG_TumbleThink(void);
	void EXPORT C4Think(void);
	void EXPORT C4Touch(CBaseEntity *pOther) {}

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	bool m_bStartDefuse;
	bool m_bIsC4;
	EHANDLE m_pBombDefuser;
	float m_flDefuseCountDown;
	float m_flC4Blow;
	float m_flNextFreqInterval;
	float m_flNextBeep;
	float m_flNextFreq;
	char *m_sBeepName;
	float m_fAttenu;
	float m_flNextBlink;
	float m_fNextDefuse;
	bool m_bJustBlew;
	int m_iTeam;
	int m_iCurWave;
	edict_t *m_pentCurBombTarget;
	int m_SGSmoke;
	int m_angle;
	unsigned short m_usEvent;
	bool m_bLightSmoke;
	bool m_bDetonated;
	Vector m_vSmokeDetonate;
	int m_iBounceCount;
	BOOL m_fRegisteredSound;
};

#define ITEM_HEALTHKIT 1
#define ITEM_ANTIDOTE 2
#define ITEM_SECURITY 3
#define ITEM_BATTERY 4

#define WEAPON_ALLWEAPONS (~(1 << WEAPON_SUIT))
#define WEAPON_SUIT 31
#define MAX_WEAPONS 32

#define MAX_NORMAL_BATTERY 100

#define WEAPON_NOCLIP -1

#define _9MM_MAX_CARRY MAX_AMMO_9MM
#define BUCKSHOT_MAX_CARRY MAX_AMMO_BUCKSHOT
#define _556NATO_MAX_CARRY MAX_AMMO_556NATO
#define _556NATOBOX_MAX_CARRY MAX_AMMO_556NATOBOX
#define _762NATO_MAX_CARRY MAX_AMMO_762NATO
#define _45ACP_MAX_CARRY MAX_AMMO_45ACP
#define _50AE_MAX_CARRY MAX_AMMO_50AE
#define _338MAGNUM_MAX_CARRY MAX_AMMO_338MAGNUM
#define _57MM_MAX_CARRY MAX_AMMO_57MM
#define _357SIG_MAX_CARRY MAX_AMMO_357SIG

#define HEGRENADE_MAX_CARRY 1
#define FLASHBANG_MAX_CARRY 2
#define SMOKEGRENADE_MAX_CARRY 1
#define C4_MAX_CARRY 1


#define ITEM_FLAG_SELECTONEMPTY 1
#define ITEM_FLAG_NOAUTORELOAD 2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY 4
#define ITEM_FLAG_LIMITINWORLD 8
#define ITEM_FLAG_EXHAUSTIBLE 16

#define WPNSLOT_PRIMARY 1
#define WPNSLOT_SECONDARY 2
#define WPNSLOT_KNIFE 3
#define WPNSLOT_GRENADE 4
#define WPNSLOT_C4 5

#define WEAPON_IS_ONTARGET 0x40

typedef struct
{
	int iSlot;
	int iPosition;
	const char *pszAmmo1;
	int iMaxAmmo1;
	const char *pszAmmo2;
	int iMaxAmmo2;
	const char *pszName;
	int iMaxClip;
	int iId;
	int iFlags;
	int iWeight;
}
ItemInfo;

typedef struct
{
	const char *pszName;
	int iId;
}
AmmoInfo;

class CBasePlayerItem : public CBaseAnimating
{
public:
	virtual int Save(CSave &save) { return 1; }
	virtual int Restore(CRestore &restore) { return 1; }
	virtual void SetObjectCollisionBox(void) { }
	virtual int AddToPlayer(CBasePlayer *pPlayer) { return false; }
	virtual int AddDuplicate(CBasePlayerItem *pItem) { return FALSE; }
	virtual int GetItemInfo(ItemInfo *p) { return 0; }
	virtual BOOL CanDeploy(void) { return TRUE; }
	virtual BOOL CanDrop(void) { return TRUE; }
	virtual BOOL Deploy(void) { return TRUE; }
	virtual BOOL IsWeapon(void) { return FALSE; }
	virtual BOOL CanHolster(void) { return TRUE; }
	virtual void Holster(int skiplocal = 0) {}
	virtual void UpdateItemInfo(void) {}
	virtual void ItemPreFrame(void) {}
	virtual void ItemPostFrame(void) {}
	virtual void Drop(void) {}
	virtual void Kill(void) {}
	virtual void AttachToPlayer(CBasePlayer *pPlayer) {}
	virtual int PrimaryAmmoIndex(void) { return -1; }
	virtual int SecondaryAmmoIndex(void) { return -1; }
	virtual int UpdateClientData(CBasePlayer *pPlayer) { return 0; }
	virtual CBasePlayerItem *GetWeaponPtr(void) { return NULL; }
	virtual float GetMaxSpeed(void) { return 260; }
	virtual int iItemSlot(void) { return 0; }

public:
	void EXPORT DestroyItem(void) {}
	void EXPORT DefaultTouch(CBaseEntity *pOther) {}
	void EXPORT FallThink(void) {}
	void EXPORT Materialize(void) {}
	void EXPORT AttemptToMaterialize(void) {}
	CBaseEntity *Respawn(void) { return this; }
	void FallInit(void) { }
	void CheckRespawn(void) {}

public:
	static TYPEDESCRIPTION m_SaveData[];
	static ItemInfo ItemInfoArray[MAX_WEAPONS];
	static AmmoInfo AmmoInfoArray[MAX_AMMO_SLOTS];

public:
	CBasePlayer *m_pPlayer;
	CBasePlayerItem *m_pNext;
	int m_iId;

public:
	int iItemPosition(void) { return ItemInfoArray[m_iId].iPosition; }
	const char *pszAmmo1(void) { return ItemInfoArray[m_iId].pszAmmo1; }
	int iMaxAmmo1(void) { return ItemInfoArray[m_iId].iMaxAmmo1; }
	const char *pszAmmo2(void) { return ItemInfoArray[m_iId].pszAmmo2; }
	int iMaxAmmo2(void) { return ItemInfoArray[m_iId].iMaxAmmo2; }
	const char *pszName(void) { return ItemInfoArray[m_iId].pszName; }
	int iMaxClip(void) { return ItemInfoArray[m_iId].iMaxClip; }
	int iWeight(void) { return ItemInfoArray[m_iId].iWeight; }
	int iFlags(void) { return ItemInfoArray[m_iId].iFlags; }
};

class CBasePlayerWeapon : public CBasePlayerItem
{
public:
	virtual int Save(CSave &save) { return 1; }
	virtual int Restore(CRestore &restore) { return 1; }
	virtual int AddToPlayer(CBasePlayer *pPlayer) { return 0; }
	virtual int AddDuplicate(CBasePlayerItem *pItem) { return 0; }
	virtual int ExtractAmmo(CBasePlayerWeapon *pWeapon) { return 0; }
	virtual int ExtractClipAmmo(CBasePlayerWeapon *pWeapon) { return 0; }
	virtual int AddWeapon(void) { ExtractAmmo(this); return TRUE; }
	virtual void UpdateItemInfo(void) {}
	virtual BOOL PlayEmptySound(void);
	virtual void ResetEmptySound(void);
	virtual void SendWeaponAnim(int iAnim, int skiplocal = 0);
	virtual BOOL CanDeploy(void);
	virtual BOOL IsWeapon(void) { return TRUE; }
	virtual BOOL IsUseable(void) { return true; }
	virtual void ItemPostFrame(void);
	virtual void PrimaryAttack(void) {}
	virtual void SecondaryAttack(void) {}
	virtual void Reload(void) {}
	virtual void WeaponIdle(void) {}
	virtual int UpdateClientData(CBasePlayer *pPlayer) { return 0; }
	virtual void RetireWeapon(void);
	virtual BOOL ShouldWeaponIdle(void) { return FALSE; }
	virtual void Holster(int skiplocal = 0);
	virtual BOOL UseDecrement(void) { return FALSE; }
	virtual CBasePlayerItem *GetWeaponPtr(void) { return (CBasePlayerItem *)this; }

public:
	BOOL DefaultDeploy(const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt, int skiplocal = 0);
	int DefaultReload(int iClipSize, int iAnim, float fDelay, int body = 0);
	void ReloadSound(void);
	BOOL AddPrimaryAmmo(int iCount, char *szName, int iMaxClip, int iMaxCarry) { return true; }
	BOOL AddSecondaryAmmo(int iCount, char *szName, int iMaxCarry) { return true; }
	int PrimaryAmmoIndex(void) { return -1; }
	int SecondaryAmmoIndex(void) { return -1; }
	void EjectBrassLate(void);
	void KickBack(float up_base, float lateral_base, float up_modifier, float lateral_modifier, float up_max, float lateral_max, int direction_change);
	void FireRemaining(int &shotsFired, float &shootTime, BOOL isGlock18);
	void SetPlayerShieldAnim(void);
	void ResetPlayerShieldAnim(void);
	bool ShieldSecondaryFire(int up_anim, int down_anim);
	bool HasSecondaryAttack(void);
	float GetNextAttackDelay(float delay);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	int m_iPlayEmptySound;
	int m_fFireOnEmpty;
	float m_flNextPrimaryAttack;
	float m_flNextSecondaryAttack;
	float m_flTimeWeaponIdle;
	int m_iPrimaryAmmoType;
	int m_iSecondaryAmmoType;
	int m_iClip;
	int m_iClientClip;
	int m_iClientWeaponState;
	int m_fInReload;
	int m_fInSpecialReload;
	int m_iDefaultAmmo;
	int m_iShellId;
	int m_fMaxSpeed;
	bool m_bDelayFire;
	int m_iDirection;
	bool m_bSecondarySilencerOn;
	float m_flAccuracy;
	float m_flLastFire;
	int m_iShotsFired;
	Vector m_vVecAiming;
	string_t model_name;
	float m_flGlock18Shoot;
	int m_iGlock18ShotsFired;
	float m_flFamasShoot;
	int m_iFamasShotsFired;
	float m_fBurstSpread;
	int m_iWeaponState;
	float m_flNextReload;
	float m_flDecreaseShotsFired;
	unsigned short m_usFireGlock18;
	unsigned short m_usFireFamas;
};

class CBasePlayerAmmo : public CBaseEntity
{
public:
	virtual void Spawn(void){}
	virtual BOOL AddAmmo(CBaseEntity *pOther) { return TRUE; }

public:
	void EXPORT Materialize(void) { }
	void EXPORT DefaultTouch(CBaseEntity *pOther) { }
	CBaseEntity *Respawn(void) { return this; }
};

extern DLL_GLOBAL short g_sModelIndexLaser;
extern DLL_GLOBAL const char *g_pModelNameLaser;
extern DLL_GLOBAL short g_sModelIndexLaserDot;
extern DLL_GLOBAL short g_sModelIndexFireball;
extern DLL_GLOBAL short g_sModelIndexFireball2;
extern DLL_GLOBAL short g_sModelIndexFireball3;
extern DLL_GLOBAL short g_sModelIndexFireball4;
extern DLL_GLOBAL short g_sModelIndexSmoke;
extern DLL_GLOBAL short g_sModelIndexSmokePuff;
extern DLL_GLOBAL short g_sModelIndexWExplosion;
extern DLL_GLOBAL short g_sModelIndexBubbles;
extern DLL_GLOBAL short g_sModelIndexBloodDrop;
extern DLL_GLOBAL short g_sModelIndexBloodSpray;
extern DLL_GLOBAL short g_sModelIndexRadio;
extern DLL_GLOBAL short g_sModelIndexCTGhost;
extern DLL_GLOBAL short g_sModelIndexTGhost;
extern DLL_GLOBAL short g_sModelIndexC4Glow;

#ifndef CLIENT_WEAPONS
extern void ClearMultiDamage(void);
extern void ApplyMultiDamage(entvars_t *pevInflictor, entvars_t *pevAttacker);
extern void AddMultiDamage(entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType);
extern void DecalGunshot(TraceResult *pTrace, int iBulletType, bool ClientOnly, entvars_t *pShooter, bool bHitMetal);
extern void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
extern int DamageDecal(CBaseEntity *pEntity, int bitsDamageType);
extern void RadiusFlash(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage);
extern void RadiusDamage(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType);
#else
inline void ClearMultiDamage(void) { }
inline void ApplyMultiDamage(entvars_t *pevInflictor, entvars_t *pevAttacker) { }
inline void DecalGunshot(TraceResult *pTrace, int iBulletType, bool ClientOnly, entvars_t *pShooter, bool bHitMetal) { }
#endif
typedef struct
{
	CBaseEntity *pEntity;
	float amount;
	int type;
}
MULTIDAMAGE;

extern MULTIDAMAGE gMultiDamage;

#define LOUD_GUN_VOLUME 1000
#define NORMAL_GUN_VOLUME 600
#define QUIET_GUN_VOLUME 200

#define BRIGHT_GUN_FLASH 512
#define NORMAL_GUN_FLASH 256
#define DIM_GUN_FLASH 128

#define BIG_EXPLOSION_VOLUME 2048
#define NORMAL_EXPLOSION_VOLUME 1024
#define SMALL_EXPLOSION_VOLUME 512

#define WEAPON_ACTIVITY_VOLUME 64

#define VECTOR_CONE_1DEGREES Vector(0.00873, 0.00873, 0.00873)
#define VECTOR_CONE_2DEGREES Vector(0.01745, 0.01745, 0.01745)
#define VECTOR_CONE_3DEGREES Vector(0.02618, 0.02618, 0.02618)
#define VECTOR_CONE_4DEGREES Vector(0.03490, 0.03490, 0.03490)
#define VECTOR_CONE_5DEGREES Vector(0.04362, 0.04362, 0.04362)
#define VECTOR_CONE_6DEGREES Vector(0.05234, 0.05234, 0.05234)
#define VECTOR_CONE_7DEGREES Vector(0.06105, 0.06105, 0.06105)
#define VECTOR_CONE_8DEGREES Vector(0.06976, 0.06976, 0.06976)
#define VECTOR_CONE_9DEGREES Vector(0.07846, 0.07846, 0.07846)
#define VECTOR_CONE_10DEGREES Vector(0.08716, 0.08716, 0.08716)
#define VECTOR_CONE_15DEGREES Vector(0.13053, 0.13053, 0.13053)
#define VECTOR_CONE_20DEGREES Vector(0.17365, 0.17365, 0.17365)

class CWeaponBox : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	void Touch(CBaseEntity *pOther);
	void SetObjectCollisionBox(void);

public:
	BOOL IsEmpty(void);
	int GiveAmmo(int iCount, char *szName, int iMax, int *pIndex = NULL); // -V762
	void EXPORT Kill(void);
	BOOL HasWeapon(CBasePlayerItem *pCheckItem);
	BOOL PackWeapon(CBasePlayerItem *pWeapon);
	BOOL PackAmmo(int iszName, int iCount);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	CBasePlayerItem *m_rgpPlayerItems[MAX_ITEM_TYPES];
	int m_rgiszAmmo[MAX_AMMO_SLOTS];
	int m_rgAmmo[MAX_AMMO_SLOTS];
	int m_cAmmoTypes;
};

class CWShield : public CBaseEntity
{
public:
	void Spawn(void);
	void EXPORT Touch(CBaseEntity *pOther);
	void SetCantBePickedUpByUser(CBaseEntity *pEntity, float time);

public:
	EHANDLE m_hEntToIgnoreTouchesFrom;
	float m_flTimeToIgnoreTouches;
};

#ifdef PLAYER_H

class CAK47 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 221; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void AK47Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireAK47;
};

class CAUG : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 240; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	void AUGFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireAug;
};

class CAWP : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void);
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void AWPFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireAWP;
};

// for usermsg BombDrop
#define BOMB_FLAG_DROPPED	0 // if the bomb was dropped due to voluntary dropping or death/disconnect
#define BOMB_FLAG_PLANTED	1 // if the bomb has been planted will also trigger the round timer to hide will also show where the dropped bomb on the Terrorist team's radar.

class CC4 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	void Holster(int skiplocal);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_C4; }
	void PrimaryAttack(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	bool m_bStartedArming;
	bool m_bBombPlacedAnimation;
	float m_fArmedTime;
	bool m_bHasShield;
};

class CDEAGLE : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void DEAGLEFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireDeagle;
};

class CELITE : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void ELITEFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireELITE_LEFT;
	unsigned short m_usFireELITE_RIGHT;
};

class CFamas : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 240; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void FamasFire(float flSpread, float flCycleTime, BOOL fUseAutoAim, BOOL bFireBurst);

private:
	int m_iShell;
	int iShellOn;
};

class CFiveSeven : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void FiveSevenFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireFiveSeven;
};

class CFlashbang : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL CanDeploy(void);
	BOOL CanDrop(void) { return FALSE; }
	BOOL Deploy(void);
	void Holster(int skiplocal);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_GRENADE; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void WeaponIdle(void);
	void SetPlayerShieldAnim(void);
	void ResetPlayerShieldAnim(void);
	bool ShieldSecondaryFire(int up_anim, int down_anim);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
};

class CG3SG1 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void);
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void G3SG1Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireG3SG1;
};

class CGalil : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 240; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void GalilFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireGalil;
};

class CGLOCK18 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void GLOCK18Fire(float flSpread, float flCycleTime, BOOL fUseBurstMode);

private:
	int m_iShell;
	bool m_bBurstFire;
};

class CHEGrenade : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL CanDeploy(void);
	BOOL CanDrop(void) { return FALSE; }
	BOOL Deploy(void);
	BOOL CanHolster(void);
	void Holster(int skiplocal);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_GRENADE; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void WeaponIdle(void);
	void SetPlayerShieldAnim(void);
	void ResetPlayerShieldAnim(void);
	bool ShieldSecondaryFire(int up_anim, int down_anim);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	unsigned short m_usCreateExplosion;
};

class CKnife : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL CanDrop(void) { return FALSE; }
	BOOL Deploy(void);
	void Holster(int skiplocal);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_KNIFE; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void SetPlayerShieldAnim(void);
	void ResetPlayerShieldAnim(void);
	bool ShieldSecondaryFire(int up_anim, int down_anim);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	void WeaponAnimation(int iAnimation);
	void EXPORT SwingAgain(void);
	void EXPORT Smack(void);
	int Swing(int fFirst);
	int Stab(int fFirst);

private:
	//int m_iSwing; //Already exists in CBaseDelay
	TraceResult m_trHit;
	unsigned short m_usKnife;
};

class CM249 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 220; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void M249Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireM249;
};

class CM3 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 230; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	int m_iShell;
	float m_flPumpTime;
	unsigned short m_usFireM3;
};

class CM4A1 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 230; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void M4A1Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireM4A1;
};

class CMAC10 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void MAC10Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireMAC10;
};

class CMP5N : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void MP5NFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireMP5N;
};

class CP228 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void P228Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireP228;
};

class CP90 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 245; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void P90Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireP90;
};

class CSCOUT : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void);
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void SCOUTFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireScout;
};

class CSG550 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void);
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void SG550Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireSG550;
};

class CSG552 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void);
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	void SG552Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireSG552;
};

class CSmokeGrenade : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL CanDeploy(void);
	BOOL CanDrop(void) { return FALSE; }
	BOOL Deploy(void);
	void Holster(int skiplocal);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_GRENADE; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void WeaponIdle(void);
	void SetPlayerShieldAnim(void);
	void ResetPlayerShieldAnim(void);
	bool ShieldSecondaryFire(int up_anim, int down_anim);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	unsigned short m_usCreateSmoke;
};

class CTMP : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void TMPFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireTMP;
};

class CUMP45 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void UMP45Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireUMP45;
};

class CUSP : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void USPFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireUSP;
};

class CXM1014 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 240; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	int m_iShell;
	float m_flPumpTime;
	unsigned short m_usFireXM1014;
};

#endif

#define ARMOURY_MP5NAVY 0
#define ARMOURY_TMP 1
#define ARMOURY_P90 2
#define ARMOURY_MAC10 3
#define ARMOURY_AK47 4
#define ARMOURY_SG552 5
#define ARMOURY_M4A1 6
#define ARMOURY_AUG 7
#define ARMOURY_SCOUT 8
#define ARMOURY_G3SG1 9
#define ARMOURY_AWP 10
#define ARMOURY_M3 11
#define ARMOURY_XM1014 12
#define ARMOURY_M249 13
#define ARMOURY_FLASHBANG 14
#define ARMOURY_HEGRENADE 15
#define ARMOURY_KEVLAR 16
#define ARMOURY_ASSAULT 17
#define ARMOURY_SMOKEGRENADE 18

class CArmoury : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);
	void Restart(void);
	void KeyValue(KeyValueData *pkvd);

public:
	void EXPORT ArmouryTouch(CBaseEntity *pOther);

public:
	int m_iItem;
	int m_iCount;
	int m_iInitialCount;
	bool m_bAlreadyCounted;
};

#include "wpn_shared.h"
#include "weapontype.h"

#endif
