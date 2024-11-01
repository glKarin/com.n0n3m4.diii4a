////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompD3.h"
#include "D3_InterfaceFuncs.h"

namespace InterfaceFuncs
{
	const char *GetLocation(const Vector3f &_pos)
	{
		D3_Location data;
		data.m_Position[0] = _pos[0];
		data.m_Position[1] = _pos[1];
		data.m_Position[2] = _pos[2];
		data.m_LocationName[0] = 0;
		MessageHelper msg(D3_MSG_GETLOCATION, &data, sizeof(data));
		InterfaceMsg(msg);
		return data.m_LocationName ? data.m_LocationName : "";
	}
};