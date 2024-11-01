////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompOF.h"
#include "OF_Client.h"
#include "TF_NavigationFlags.h"
#include "OF_Messages.h"
#include "TF_BaseStates.h"

#include "IGame.h"
#include "IGameManager.h"

//////////////////////////////////////////////////////////////////////////

OF_Client::OF_Client()
{
	m_StepHeight = 8.0f; // subtract a small value as a buffer to jump
}

OF_Client::~OF_Client()
{
}

void OF_Client::Init(int _gameid)
{
	//NOTENOTE: PURPOSELY BYPASSING TF_Client::Init
	Client::Init(_gameid);

	// We want to use a custom targeting filter.
	FilterPtr filter(new FilterClosestOF(this, AiState::SensoryMemory::EntEnemy));
	filter->AddCategory(ENT_CAT_SHOOTABLE);
	GetTargetingSystem()->SetDefaultTargetingFilter(filter);

	/*using namespace AiState;
	FINDSTATE(ws,WeaponSystem,GetStateRoot());
	if(ws)
	{
	ws->AddWeaponToInventory(TF_WP_GRENADE1);
	ws->AddWeaponToInventory(TF_WP_GRENADE2);
	}*/
}

float OF_Client::NavCallback(const NavFlags &_flag, Waypoint *from, Waypoint *to) 
{
	using namespace AiState;

	WeaponSystem *wsys = GetWeaponSystem();
	if(_flag & F_TF_NAV_ROCKETJUMP)
	{		
		if(wsys->HasWeapon(TF_Options::ROCKETJUMP_WPN) && wsys->HasAmmo(TF_Options::ROCKETJUMP_WPN))
			return 1.f;
	}

	if(_flag & F_TF_NAV_CONCJUMP)
	{
		int iCls = GetClass(), iGren2s = 0, iGren2sMax = 0;
		g_EngineFuncs->GetCurrentAmmo(GetGameEntity(), TF_WP_GRENADE2, Primary, iGren2s, iGren2sMax);
		if((iCls == TF_CLASS_SCOUT || iCls == TF_CLASS_MEDIC) && iGren2s)
			return 1.f;
	}

	return 0.f;
}
