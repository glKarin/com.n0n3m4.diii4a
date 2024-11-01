////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompQ4.h"
#include "Q4_InterfaceFuncs.h"

namespace InterfaceFuncs
{
	const char *GetLocation(const Vector3f &_pos)
	{
		Q4_Location data;
		data.m_Position[0] = _pos[0];
		data.m_Position[1] = _pos[1];
		data.m_Position[2] = _pos[2];
		data.m_LocationName[0] = 0;
		MessageHelper msg(Q4_MSG_GETLOCATION, &data, sizeof(data));
		InterfaceMsg(msg);
		return data.m_LocationName ? data.m_LocationName : "";
	}

	float GetPlayerCash(const GameEntity _player)
	{
		Q4_PlayerCash data = { 0.0f };
		MessageHelper msg(Q4_MSG_GETPLAYERCASH, &data, sizeof(data));
		g_EngineFuncs->InterfaceSendMessage(msg, _player);
		return data.m_Cash;
	}

	bool IsBuyingAllowed()
	{
		Q4_IsBuyingAllowed data = { False };
		MessageHelper msg(Q4_MSG_ISBUYINGALLOWED, &data, sizeof(data));
		InterfaceMsg(msg);
		return (data.m_BuyingAllowed == True);
	}

	bool BuySomething(const GameEntity _player, int _item)
	{
		Q4_ItemBuy data = { _item };
		MessageHelper msg(Q4_MSG_BUYSOMETHING, &data, sizeof(data));
		if(!SUCCESS(InterfaceMsg(msg, _player)))
		{
			LOGERR("Invalid Item specified: "<<_item);
		}
		return data.m_Success == True;
	}
};