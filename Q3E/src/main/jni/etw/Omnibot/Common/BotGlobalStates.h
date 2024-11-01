#ifndef __BotGlobalStates_H__
#define __BotGlobalStates_H__

#include "StateMachine.h"
#include "ScriptManager.h"

class gmScriptGoal;

namespace AiState
{
	class RegionTriggers : public StateChild
	{
	public:

		RegionTriggers();
	private:
	};

	class GlobalRoot : public StateFirstAvailable
	{
	public:
		GlobalRoot();
	private:
	};
}

#endif
