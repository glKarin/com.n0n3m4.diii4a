////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: Q4 Message Structure Definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __Q4_MESSAGES_H__
#define __Q4_MESSAGES_H__

#include "Base_Messages.h"

#pragma pack(push)
#pragma pack(4)

//////////////////////////////////////////////////////////////////////////

struct Q4_Location
{
	float	m_Position[3];
	char	m_LocationName[64];
};

struct Q4_PlayerCash
{
	float	m_Cash;
};

struct Q4_IsBuyingAllowed
{
	obBool	m_BuyingAllowed;
};

struct Q4_ItemBuy
{
	int		m_Item;
	obBool	m_Success;
};

struct Q4_CanPickUp
{
	GameEntity	m_Entity;
	obBool		m_CanPickUp;
};
//////////////////////////////////////////////////////////////////////////

#pragma pack(pop)

#endif
