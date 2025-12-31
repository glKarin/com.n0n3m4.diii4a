#ifndef __WL_DEBUG_H__
#define __WL_DEBUG_H__

#include "zstring.h"

/*
=============================================================================

								WL_DEBUG

=============================================================================
*/

enum EDebugCmd
{
	DEBUG_Give,
	DEBUG_GiveItems,
	DEBUG_GiveKey,
	DEBUG_GodMode,
	DEBUG_HurtSelf,
	DEBUG_MLI,
	DEBUG_NextLevel,
	DEBUG_NoClip,
	DEBUG_Summon,
	DEBUG_Warp,
};

struct DebugCmd
{
	EDebugCmd Type;
	FString ArgS;
	uint32_t ArgI;
};

void CheckDebugKeys();
void DoDebugKey(int player, const DebugCmd &cmd);

#endif
