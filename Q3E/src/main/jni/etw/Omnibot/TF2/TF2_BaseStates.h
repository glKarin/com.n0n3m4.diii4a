////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TF2BaseStates_H__
#define __TF2BaseStates_H__

#include "StateMachine.h"
#include "Path.h"
#include "ScriptManager.h"
#include "InternalFsm.h"
#include "TF_BaseStates.h"

class gmScriptGoal;

namespace AiState
{
	class MediGun : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		MediGun();
	private:
		GameEntity	m_HealEnt;

		struct Ent
		{
			GameEntity	m_Ent;
			float		m_Distance;
			obint32		m_Health;
			obint32		m_MaxHealth;
			Ent() : m_Distance(10000.f) {}

			static bool _ByDistance(const Ent &_1, const Ent &_2);
		};

		Ent NeedsHealing(GameEntity _ent);
		GameEntity FindHealerTarget();
	};
};

#endif
