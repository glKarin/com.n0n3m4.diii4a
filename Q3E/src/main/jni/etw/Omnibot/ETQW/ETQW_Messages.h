////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: TF Message Structure Definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TF_MESSAGES_H__
#define __TF_MESSAGES_H__

#include "Base_Messages.h"

#pragma pack(push)
#pragma pack(4)

//////////////////////////////////////////////////////////////////////////

struct ETQW_WeaponOverheated
{
	ETQW_Weapon	m_Weapon;
	obBool		m_IsOverheated;
};

struct ETQW_WeaponHeatLevel
{
	GameEntity	m_Entity;
	int			m_Current;
	int			m_Max;
};

struct ETQW_ExplosiveState
{
	GameEntity		m_Explosive;
	ExplosiveState	m_State;
};

struct ETQW_ConstructionState
{
	GameEntity			m_Constructable;
	ConstructableState	m_State;
};

struct ETQW_Destroyable
{
	GameEntity			m_Entity;
	ConstructableState	m_State;
};

struct ETQW_HasFlag
{
	obBool		m_HasFlag;
};

struct ETQW_CanBeGrabbed
{
	GameEntity	m_Entity;
	obBool		m_CanBeGrabbed;
};

struct ETQW_TeamMines
{
	int			m_Current;
	int			m_Max;
};

struct ETQW_WaitingForMedic
{
	obBool		m_WaitingForMedic;
};

struct ETQW_SelectWeapon
{
	ETQW_Weapon	m_Selection;
	obBool		m_Good;
};

struct ETQW_ReinforceTime
{
	int			m_ReinforceTime;
};

struct ETQW_MedicNear
{
	obBool		m_MedicNear;
};

struct ETQW_GoLimbo
{
	obBool		m_GoLimbo;
};

struct ETQW_MG42MountedPlayer
{
	GameEntity	m_MG42Entity;
	GameEntity	m_MountedEntity;
};

struct ETQW_MG42MountedRepairable
{
	GameEntity	m_MG42Entity;
	obBool		m_Repairable;
};

struct ETQW_MG42Health
{
	GameEntity	m_MG42Entity;
	int			m_Health;
};

struct ETQW_CursorHint
{
	int			m_Type;
	int			m_Value;
};
struct ETQW_SpawnPoint
{
	int			m_SpawnPoint;
};

struct ETQW_MG42Info
{
	float		m_CenterFacing[3];
	float		m_MinHorizontalArc, m_MaxHorizontalArc;
	float		m_MinVerticalArc, m_MaxVerticalArc;
};

struct ETQW_CabinetData
{
	int			m_CurrentAmount;
	int			m_MaxAmount;
	int			m_Rate;
};

struct ETQW_PlayerSkills
{
	int		m_Skill[ETQW_SKILLS_NUM_SKILLS];
};

//////////////////////////////////////////////////////////////////////////

struct Event_MortarImpact_ETQW
{
	float	m_Position[3];
};

struct Event_TriggerMine_ETQW
{
	GameEntity	m_MineEntity;
};

#pragma pack(pop)

#endif
