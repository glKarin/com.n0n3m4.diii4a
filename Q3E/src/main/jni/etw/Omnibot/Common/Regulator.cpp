#include "PrecompCommon.h"
#include "Regulator.h"
#include "IGame.h"



bool Regulator::IsReady()
{
	int iCurrentTime = IGame::GetTime();
	if(iCurrentTime >= m_NextUpdateTime)
	{
		m_NextUpdateTime = iCurrentTime + m_UpdateInterval;
		return true;
	}
	return false;
}

