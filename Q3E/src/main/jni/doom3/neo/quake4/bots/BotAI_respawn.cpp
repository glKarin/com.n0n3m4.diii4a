// BotAI_respawn.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

/*
=====================
rvmBot::state_Respawn
=====================
*/
stateResult_t rvmBot::state_Respawn(const stateParms_t& parms)
{
	if (parms.stage == 0)
	{
		bs.botinput.respawn = true;
		((stateParms_t &)parms).stage = 1;
		return SRESULT_WAIT;
	}

	if (parms.stage == 1)
	{
		if (spectating)
		{
			return SRESULT_WAIT; // Wait until we have moved from spectator back into the game. 
		}
	}

	bs.botinput.respawn = false;
	stateThread.SetState("state_SeekLTG");
	return SRESULT_DONE;
}
