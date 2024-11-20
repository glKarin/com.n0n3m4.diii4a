////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: MC Message Structure Definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MC_MESSAGES_H__
#define __MC_MESSAGES_H__

#include "Base_Messages.h"

#pragma pack(push)
#pragma pack(4)

//////////////////////////////////////////////////////////////////////////

// struct: Event_PlayerSpree
struct Event_PlayerSpree
{
	static const int EventId = MC_EVENT_PLAYER_SPREE;

	GameEntity	m_Player;
	int			m_Kills;
};

// struct: Event_PlayerSpreeEnd
struct Event_PlayerSpreeEnd
{
	static const int EventId = MC_EVENT_PLAYER_SPREE_END;

	GameEntity	m_Player;
	GameEntity	m_ByWho;
	int			m_Kills;
};

// struct: Event_SpreeWarStart
struct Event_SpreeWarStart
{
	static const int EventId = MC_EVENT_SPREEWAR_START;

	GameEntity	m_Victim;
};

struct Event_SpreeWarEnd
{
	static const int EventId = MC_EVENT_SPREEWAR_END;
};

struct Event_LevelUp
{
	static const int EventId = MC_EVENT_LEVEL_UP;
	int	m_Level;
};

//////////////////////////////////////////////////////////////////////////

// struct: MC_PlayerStats
struct MC_PlayerStats
{
	int			m_ExperienceTotal;
	int			m_ExperienceGame;
	int			m_ModulePoints;
	int			m_WeaponPoints;
	int			m_Credits;
	int			m_Level;
	int			m_Minions;
	int			m_MinionsMax;
	float		m_AuxPower;
	float		m_AuxPowerMax;
	float		m_AuxRegenRate;
};

// struct: MC_ModuleStats
struct MC_ModuleStats
{
	struct ModuleInfo
	{
		int		m_Lvl;
		int		m_MaxLvl;
		int		m_UpgradeCost;
		float	m_AuxDrain;
		float	m_Cooldown;
		ModuleInfo() : m_Lvl(0), m_MaxLvl(0), m_UpgradeCost(0), m_AuxDrain(0.f), m_Cooldown(0.f) {}
	};
	ModuleInfo	m_Module[MC_MODULE_MAX];
};

// struct: MC_UpgradeModule
struct MC_UpgradeModule
{
	int			m_ModuleId;
};

// struct: MC_WeaponUpgradeStats
struct MC_WeaponUpgradeStats
{
	struct WeaponUpgradeInfo
	{
		int		m_Lvl;
		int		m_MaxLvl;
		int		m_UpgradeCost;
		WeaponUpgradeInfo() : m_Lvl(0), m_MaxLvl(0), m_UpgradeCost(0) {}
	};
	WeaponUpgradeInfo	m_Upgrade[MC_UPGRADE_MAX];
};

// struct: MC_UpgradeWeapon
struct MC_UpgradeWeapon
{
	int			m_WeaponId;
};

// struct: MC_CanPhysPickup
struct MC_CanPhysPickup
{
	GameEntity	m_Entity;
	obBool		m_CanPickUp;
};

// struct: MC_PhysGunInfo
struct MC_PhysGunInfo
{
	GameEntity	m_HeldEntity;
	float		m_LaunchSpeed;
};

struct MC_ChargerStatus
{
	float	m_CurrentCharge;
	float	m_MaxCharge;
};

#pragma pack(pop)

#endif
