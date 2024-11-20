////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __JABaseStates_H__
#define __JABaseStates_H__

#include "StateMachine.h"
#include "Path.h"
#include "ScriptManager.h"

class gmScriptGoal;

namespace AiState
{
	//////////////////////////////////////////////////////////////////////////
	class ForceJump : public StateChild
	{
	public:
		
		void RenderDebug();

		obReal GetPriority();
		StateStatus Update(float fDt);

		void Enter();

		ForceJump();
	private:
		Path::PathPoint m_NextPt;
		float			m_DistanceLeft;
		float			m_DistanceTotal;
		bool			m_IsJumping : 1;
		bool			m_IsOnGround : 1;
	};
	//////////////////////////////////////////////////////////////////////////

};

#endif
