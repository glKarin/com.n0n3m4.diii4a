////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompETQW.h"
#include "ETQW_FilterClosest.h"

#include "ScriptManager.h"

ETQW_FilterClosest::ETQW_FilterClosest(Client *_client, AiState::SensoryMemory::Type _type) :
	FilterClosest				(_client, _type)
{
}

bool ETQW_FilterClosest::CheckEx(const MemoryRecord &_record)
{
	// Special consideration for some entity types.
	switch(_record.m_TargetInfo.m_EntityClass) 
	{
	case ETQW_CLASSEX_VEHICLE_HVY:
		{
			break;
		}
	case ETQW_CLASSEX_MG42MOUNT:
		{
			GameEntity mounted = InterfaceFuncs::GetMountedPlayerOnMG42(m_Client, _record.GetEntity());
			if(!mounted.IsValid() || m_Client->IsAllied(mounted))
				return false;
			break;
		}
	case ETQW_CLASSEX_BREAKABLE:
		{
			float fBreakableDist = static_cast<ETQW_Client*>(m_Client)->GetBreakableTargetDist();
			float fDistance = (m_Client->GetPosition() - _record.GetLastSensedPosition()).SquaredLength();
			if(fDistance > (fBreakableDist * fBreakableDist))
				return false;
			break;
		}
	}

	// TODO: Handle disguised covertops
	if (_record.m_TargetInfo.m_EntityFlags.CheckFlag(ETQW_ENT_FLAG_DISGUISED) && Mathf::UnitRandom() > 0.33f)
		return false;

	return true;
}

