#include "PrecompCommon.h"
#include "gmTimer.h"

GMBIND_INIT_TYPE( gmTimer, "Timer" );
GMBIND_FUNCTION_MAP_BEGIN( gmTimer )
GMBIND_FUNCTION( "Reset", gmfResetTimer )
GMBIND_FUNCTION( "GetElapsedTime", gmfGetElapsedTime )
GMBIND_FUNCTION_MAP_END()

//////////////////////////////////////////////////////////////////////////

int gmTimer::gmfResetTimer(gmThread *a_thread)
{
	Timer *pNative = gmTimer::GetThisObject( a_thread );
	if(!pNative)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal"); 
		return GM_EXCEPTION;
	}
	pNative->Reset();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

int gmTimer::gmfGetElapsedTime(gmThread *a_thread)
{
	Timer *pNative = gmTimer::GetThisObject( a_thread );
	if(!pNative)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal"); 
		return GM_EXCEPTION;
	}
	a_thread->PushFloat((float)pNative->GetElapsedSeconds());
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
