#ifndef __RTCW_MESSAGES_H__
#define __RTCW_MESSAGES_H__

#include "Base_Messages.h"

#pragma pack(push)
#pragma pack(4)

//////////////////////////////////////////////////////////////////////////

struct RTCW_GetPlayerClass
{
	GameEntity	m_Entity;
	int			m_PlayerClass;
};

struct RTCW_WeaponOverheated
{
	RTCW_Weapon	m_Weapon;
	obBool		m_IsOverheated;
};

struct RTCW_WeaponHeatLevel
{
	GameEntity	m_Entity;
	int			m_Current;
	int			m_Max;
};

struct RTCW_ExplosiveState
{
	GameEntity		m_Explosive;
	ExplosiveState	m_State;
};

struct RTCW_Destroyable
{
	GameEntity			m_Entity;
	ConstructableState	m_State;
};

struct RTCW_HasFlag
{
	obBool		m_HasFlag;
};

struct RTCW_CanBeGrabbed
{
	GameEntity	m_Entity;
	obBool		m_CanBeGrabbed;
};

struct RTCW_WaitingForMedic
{
	obBool		m_WaitingForMedic;
};

struct RTCW_SelectWeapon
{
	RTCW_Weapon	m_Selection;
	obBool		m_Good;
};

struct RTCW_ReinforceTime
{
	int			m_ReinforceTime;
};

struct RTCW_MedicNear
{
	obBool		m_MedicNear;
};

struct RTCW_GoLimbo
{
	obBool		m_GoLimbo;
};

struct RTCW_MG42MountedPlayer
{
	GameEntity	m_MG42Entity;
	GameEntity	m_MountedEntity;
};

struct RTCW_MG42MountedRepairable
{
	GameEntity	m_MG42Entity;
	obBool		m_Repairable;
};

struct RTCW_MG42Health
{
	GameEntity	m_MG42Entity;
	int			m_Health;
};

struct RTCW_CursorHint
{
	int			m_Type;
	int			m_Value;
};

struct RTCW_SpawnPoint
{
	int			m_SpawnPoint;
};

struct RTCW_GetSpawnPoint
{
	int			m_SpawnPoint;
};

struct RTCW_MG42Info
{
	float		m_CenterFacing[3];
	float		m_MinHorizontalArc, m_MaxHorizontalArc;
	float		m_MinVerticalArc, m_MaxVerticalArc;
};

struct RTCW_SendPM
{
	char *		m_TargetName;
	char *		m_Message;
};

struct RTCW_GameType
{
	int			m_GameType;
};

struct RTCW_CvarSet
{
	char *		m_Cvar;
	char *		m_Value;
};

struct RTCW_CvarGet
{
	const char *m_Cvar;
	int			m_Value;
};

struct RTCW_SetSuicide
{
	int			m_Suicide;
	int			m_Persist;
};

struct RTCW_DisableBotPush
{
	int			m_Push;
};

struct Event_Ammo
{
	GameEntity	m_WhoDoneIt;
};

#pragma pack(pop)

#endif
