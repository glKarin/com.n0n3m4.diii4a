////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: JA Message Structure Definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __JA_MESSAGES_H__
#define __JA_MESSAGES_H__

#include "Base_Messages.h"

#pragma pack(push)
#pragma pack(4)

//////////////////////////////////////////////////////////////////////////

struct JA_HasFlag
{
	obBool		m_HasFlag;
};

struct JA_CanBeGrabbed
{
	GameEntity	m_Entity;
	obBool		m_CanBeGrabbed;
};

struct JA_TeamMines
{
	int			m_Current;
	int			m_Max;
};

struct JA_TeamDetpacks
{
	int			m_Current;
	int			m_Max;
};

struct JA_SelectWeapon
{
	JA_Weapon	m_Selection;
	obBool		m_Good;
};

/*struct JA_ReinforceTime
{
	int			m_ReinforceTime;
};*/

typedef struct 
{
	int			m_Force;
	int			m_KnownBits;
	int			m_ActiveBits;
	int			m_Levels[JA_MAX_FORCE_POWERS];
} JA_ForceData;

struct JA_Mindtricked
{
	GameEntity	m_Entity;
	obBool		m_IsMindtricked;
};

//////////////////////////////////////////////////////////////////////////
// Events

struct Event_SystemGametype
{
	int		m_Gametype;
};

struct Event_ForceInflicted
{
	GameEntity	m_Inflictor;
	int			m_Type;
};

#pragma pack(pop)

#endif
