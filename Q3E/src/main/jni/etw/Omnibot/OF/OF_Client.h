////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __OF_CLIENT_H__
#define __OF_CLIENT_H__

#include "TF_Client.h"
#include "OF_Config.h"

// class: OF_Client
//		OF Bot Class
class OF_Client : public TF_Client
{
public:
	void Init(int _gameid);

	float NavCallback(const NavFlags &_flag, Waypoint *from, Waypoint *to);

	OF_Client();
	virtual ~OF_Client();
};

#endif
