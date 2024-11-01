////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: TF Message Structure Definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __ET_MESSAGES_H__
#define __ET_MESSAGES_H__

#include "Base_Messages.h"

#pragma pack(push)
#pragma pack(4)

//////////////////////////////////////////////////////////////////////////

// struct: TF_PlayerPipeCount
//		m_NumPipes - Current number of player pipes.
//		m_MaxPipes - Max player pipes.
struct TF_PlayerPipeCount
{
	obint32		m_NumPipes;
	obint32		m_MaxPipes;
};

// struct: TF_TeamPipeInfo
//		m_NumTeamPipes - Current number of team pipes.
//		m_NumTeamPipers - Current number of team pipers(demo-men).
//		m_MaxPipesPerPiper - Max pipes per piper
struct TF_TeamPipeInfo
{
	obint32		m_NumTeamPipes;
	obint32		m_NumTeamPipers;
	obint32		m_MaxPipesPerPiper;
};

// struct: TF_DisguiseOptions
//		m_CheckTeam - team to check class disguises with.
//		m_Team - true/false if each team is available to be disguised as.
//		m_DisguiseClass - true/false if each class is available to be disguised as.
struct TF_DisguiseOptions
{
	int			m_CheckTeam;
	obBool		m_Team[TF_TEAM_MAX];
	obBool		m_Class[TF_CLASS_MAX];
};

// struct: TF_Disguise
//		m_DisguiseTeam - Team disguised as.
//		m_DisguiseClass - Class disguised as.
struct TF_Disguise
{
	obint32		m_DisguiseTeam;
	obint32		m_DisguiseClass;
};

// struct: TF_FeignDeath
//		m_SilentFeign - Silent feign or not.
struct TF_FeignDeath
{
	obBool		m_Silent;
};

// struct: TF_HudHint
//		m_TargetPlayer - Target player entity for the hint.
//		m_Id - Id for the hint.
//		m_Message[1024] - Hint message.
struct TF_HudHint
{
	GameEntity	m_TargetPlayer;
	obint32		m_Id;
	char		m_Message[1024];
};

// struct: TF_HudMenu
//		m_TargetPlayer - Target player entity for the hint.
//		m_MenuType - The type of menu.
//		m_Title[32] - Title of the menu.
//		m_Caption[32] - Caption of the menu.
//		m_Message[512] - Message of the menu.
//		m_Option[10][64] - Array of options, max 10.
//		m_Command[10][64] - Array of commands, max 10.
//		m_Level - Menu level.
//		m_TimeOut - Duration of the menu.
//		m_Color - Text color.
struct TF_HudMenu
{
	enum GuiType
	{
		GuiAlert,
		GuiMenu,
		GuiTextBox,
	};
	GameEntity	m_TargetPlayer;
	GuiType		m_MenuType;
	char		m_Title[32];
	char		m_Caption[32];
	char		m_Message[512];
	char		m_Option[10][64];
	char		m_Command[10][64];
	int			m_Level;
	float		m_TimeOut;
	obColor		m_Color;
};

// struct: TF_HudText
//		m_TargetPlayer - Target player entity for the message.
//		m_Message - Text to display.
struct TF_HudText
{
	enum MsgType
	{
		MsgConsole,
		MsgHudCenter,
	};
	GameEntity	m_TargetPlayer;
	MsgType		m_MessageType;
	char		m_Message[512];
};

// struct: TF_LockPosition
//		m_TargetPlayer - Target player entity for the hint.
//		m_Lock - Lock the player or not.
//		m_Succeeded - Status result.
struct TF_LockPosition
{
	GameEntity	m_TargetPlayer;
	obBool		m_Lock;
	obBool		m_Succeeded;
};

//////////////////////////////////////////////////////////////////////////

struct Event_RadioTagUpdate_TF
{
	GameEntity	m_Detected;
};

struct Event_RadarUpdate_TF
{
	GameEntity	m_Detected;
};

struct Event_CantDisguiseTeam_TF
{
	int			m_TeamId;
};

struct Event_CantDisguiseClass_TF
{
	int			m_ClassId;
};

struct Event_Disguise_TF
{
	int			m_TeamId;
	int			m_ClassId;
};

struct Event_SentryBuilding_TF
{
	GameEntity	m_Sentry;
};

struct Event_SentryBuilt_TF
{
	GameEntity	m_Sentry;
};

struct Event_SentryAimed_TF
{
	GameEntity	m_Sentry;
	float		m_Direction[3];
};

struct Event_SentrySpotEnemy_TF
{
	GameEntity	m_SpottedEnemy;
};

struct Event_SentryTakeDamage_TF
{
	GameEntity	m_Inflictor;
};

struct Event_SentryUpgraded_TF
{
	int				m_Level;
};

struct Event_SentryStatus_TF
{
	GameEntity		m_Entity;
	float			m_Position[3];
	float			m_Facing[3];
	int				m_Level;
	int				m_Health;
	int				m_MaxHealth;
	int				m_Shells[2];
	int				m_Rockets[2];
	obBool			m_Sabotaged;
};

struct Event_DispenserStatus_TF
{
	GameEntity		m_Entity;
	float			m_Position[3];
	float			m_Facing[3];
	int				m_Health;
	int				m_MaxHealth;
	int				m_Cells;
	int				m_Nails;
	int				m_Rockets;
	int				m_Shells;
	int				m_Armor;
	obBool			m_Sabotaged;
};

struct Event_TeleporterStatus_TF
{
	GameEntity		m_EntityEntrance;
	GameEntity		m_EntityExit;
	int				m_NumTeleports;
	int				m_TimeToActivation;
	obBool			m_SabotagedEntry;
	obBool			m_SabotagedExit;
};

struct Event_TeleporterBuilding_TF
{
	GameEntity	m_Teleporter;
};

struct Event_TeleporterBuilt_TF
{
	GameEntity	m_Teleporter;
};

struct Event_DispenserBuilding_TF
{
	GameEntity	m_Dispenser;
};

struct Event_DispenserBuilt_TF
{
	GameEntity	m_Dispenser;
};

struct Event_DispenserEnemyUsed_TF
{
	GameEntity	m_Enemy;
};

struct Event_DispenserTakeDamage_TF
{
	GameEntity	m_Inflictor;
};

struct Event_DetpackBuilding_TF
{
	GameEntity	m_Detpack;
};

struct Event_DetpackBuilt_TF
{
	GameEntity	m_Detpack;
};

struct Event_BuildableDamaged_TF
{
	GameEntity	m_Buildable;
};

struct Event_BuildableDestroyed_TF
{
	GameEntity	m_WhoDoneIt;
	GameEntity	m_WhoAssistedIt;
};

struct Event_BuildableSabotaged_TF
{
	GameEntity	m_WhoDoneIt;
};

struct TF_BuildInfo
{
	GameEntity					m_Detpack;

	Event_SentryStatus_TF		m_SentryStats;
	Event_DispenserStatus_TF	m_DispenserStats;
	Event_TeleporterStatus_TF	m_TeleporterStats;
};

struct TF_HealTarget
{
	GameEntity					m_Target;
	float						m_UberLevel;
	bool						m_Healing;
};

struct Event_GotMedicHealth
{
	GameEntity	m_FromWho;
	int			m_BeforeValue;
	int			m_AfterValue;
};

struct Event_GaveMedicHealth
{
	GameEntity	m_ToWho;
	int			m_BeforeValue;
	int			m_AfterValue;
};

struct Event_GotEngyArmor
{
	GameEntity	m_FromWho;
	int			m_BeforeValue;
	int			m_AfterValue;
};

struct Event_GaveEngyArmor
{
	GameEntity	m_ToWho;
	int			m_BeforeValue;
	int			m_AfterValue;
};

struct Event_Infected
{
	GameEntity	m_FromWho;
};

struct Event_Cured
{
	GameEntity	m_ByWho;
};

struct Event_Burn
{
	GameEntity	m_ByWho;
	int			m_BurnLevel;
};

struct Event_MedicCall
{
	GameEntity	m_ByWho;
};

struct Event_EngineerCall
{
	GameEntity	m_ByWho;
};

struct Event_MedicUberCharged
{
	GameEntity	m_ByWho;
};

#pragma pack(pop)

#endif
