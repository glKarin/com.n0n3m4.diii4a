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

#include "stdafx.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS(weapon_hegrenade, CHEGrenade)

void CHEGrenade::Spawn(void)
{
	Precache();

	m_iId = WEAPON_HEGRENADE;
	SET_MODEL(edict(), "models/w_hegrenade.mdl");

	pev->dmg = 4;

	m_iDefaultAmmo = HEGRENADE_DEFAULT_GIVE;
	m_flStartThrow = 0;
	m_flReleaseThrow = -1.0f;
	m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;

	// get ready to fall down.
	FallInit();
}

void CHEGrenade::Precache(void)
{
	PRECACHE_MODEL("models/v_hegrenade.mdl");
	PRECACHE_MODEL("models/shield/v_shield_hegrenade.mdl");

	PRECACHE_SOUND("weapons/hegrenade-1.wav");
	PRECACHE_SOUND("weapons/hegrenade-2.wav");
	PRECACHE_SOUND("weapons/he_bounce-1.wav");
	PRECACHE_SOUND("weapons/pinpull.wav");

	m_usCreateExplosion = PRECACHE_EVENT(1, "events/createexplo.sc");
}

int CHEGrenade::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "HEGrenade";

	p->iMaxAmmo1 = MAX_AMMO_HEGRENADE;
	p->iMaxClip = WEAPON_NOCLIP;

	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_HEGRENADE;
	p->iWeight = HEGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;}

BOOL CHEGrenade::Deploy(void)
{
	m_flReleaseThrow = -1.0f;
	m_fMaxSpeed = HEGRENADE_MAX_SPEED;
	m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;

	m_pPlayer->m_bShieldDrawn = false;

	if (m_pPlayer->HasShield())
		return DefaultDeploy("models/shield/v_shield_hegrenade.mdl", "models/shield/p_shield_hegrenade.mdl", HEGRENADE_DRAW, "shieldgren", UseDecrement() != FALSE);
	else
		return DefaultDeploy("models/v_hegrenade.mdl", "models/p_hegrenade.mdl", HEGRENADE_DRAW, "grenade", UseDecrement() != FALSE);
}

BOOL CHEGrenade::CanHolster(void)
{
	return m_flStartThrow == 0;
}

void CHEGrenade::Holster(int skiplocal)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		m_pPlayer->pev->weapons &= ~(1 << WEAPON_HEGRENADE);
		DestroyItem();
	}

	m_flStartThrow = 0;
	m_flReleaseThrow = -1.0f;
}

void CHEGrenade::PrimaryAttack(void)
{
	if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
	{
		return;
	}

	if (!m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		m_flReleaseThrow = 0;
		m_flStartThrow = gpGlobals->time;

		SendWeaponAnim(HEGRENADE_PULLPIN, UseDecrement() != FALSE);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
	}
}

void CHEGrenade::SetPlayerShieldAnim(void)
{
	if (!m_pPlayer->HasShield())
		return;

	if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
		strcpy(m_pPlayer->m_szAnimExtention, "shield");
	else
		strcpy(m_pPlayer->m_szAnimExtention, "shieldgren");
}

void CHEGrenade::ResetPlayerShieldAnim(void)
{
	if (!m_pPlayer->HasShield())
		return;

	if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
	{
		strcpy(m_pPlayer->m_szAnimExtention, "shieldgren");
	}
}

bool CHEGrenade::ShieldSecondaryFire(int iUpAnim, int iDownAnim)
{
	if (!m_pPlayer->HasShield() || m_flStartThrow > 0)
	{
		return false;
	}

	if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
	{
		m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;
		SendWeaponAnim(iDownAnim, UseDecrement() != FALSE);
		strcpy(m_pPlayer->m_szAnimExtention, "shieldgren");

		m_fMaxSpeed = HEGRENADE_MAX_SPEED;
		m_pPlayer->m_bShieldDrawn = false;
	}
	else
	{
		m_iWeaponState |= WPNSTATE_SHIELD_DRAWN;
		SendWeaponAnim(iUpAnim, UseDecrement() != FALSE);
		strcpy(m_pPlayer->m_szAnimExtention, "shielded");

		m_fMaxSpeed = HEGRENADE_MAX_SPEED_SHIELD;
		m_pPlayer->m_bShieldDrawn = true;
	}

#ifndef CLIENT_DLL
	m_pPlayer->UpdateShieldCrosshair((m_iWeaponState & WPNSTATE_SHIELD_DRAWN) != WPNSTATE_SHIELD_DRAWN);
#endif
	m_pPlayer->ResetMaxSpeed();

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.4f;
	m_flNextPrimaryAttack = GetNextAttackDelay(0.4);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6f;

	return true;
}

void CHEGrenade::SecondaryAttack(void)
{
	ShieldSecondaryFire(SHIELDGUN_DRAW, SHIELDGUN_DRAWN_IDLE);
}

void CHEGrenade::WeaponIdle(void)
{
	if (m_flReleaseThrow == 0 && m_flStartThrow != 0.0f)
		m_flReleaseThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_flStartThrow)
	{
		m_pPlayer->Radio("%!MRAD_FIREINHOLE", "#Fire_in_the_hole");

		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if (angThrow.x < 0)
			angThrow.x = -10 + angThrow.x * ((90 - 10) / 90.0);
		else
			angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);

		float flVel = (90.0f - angThrow.x) * 6.0f;

		if (flVel > 750.0f)
			flVel = 750.0f;

		UTIL_MakeVectors(angThrow);

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;
		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		CGrenade::ShootTimed2(m_pPlayer->pev, vecSrc, vecThrow, 1.5, m_pPlayer->m_iTeam, m_usCreateExplosion);

		SendWeaponAnim(HEGRENADE_THROW, UseDecrement() != FALSE);
		SetPlayerShieldAnim();

#ifndef CLIENT_DLL
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#endif
		m_flStartThrow = 0;
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75f;

		if (--m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			// ensure that the animation can finish playing
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		}

		ResetPlayerShieldAnim();
	}
	else if (m_flReleaseThrow > 0)
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			SendWeaponAnim(HEGRENADE_DRAW, UseDecrement() != FALSE);
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10, 15);
		m_flReleaseThrow = -1.0f;
	}
	else if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		if (m_pPlayer->HasShield())
		{
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20.0f;

			if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
			{
				SendWeaponAnim(SHIELDREN_IDLE, UseDecrement() != FALSE);
			}
		}
		else
		{
			SendWeaponAnim(HEGRENADE_IDLE, UseDecrement() != FALSE);

			// how long till we do this again.
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10, 15);
		}
	}
}

BOOL CHEGrenade::CanDeploy(void)
{
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0;
}
