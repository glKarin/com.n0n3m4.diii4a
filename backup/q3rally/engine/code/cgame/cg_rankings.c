// cg_rankings.c -- client global rankings system

#include "cg_local.h"

/*
================
CG_RankRunFrame
================
*/

void CG_RankRunFrame(void)
{
	grank_status_t	status;

	if (! cgs.localServer)
		trap_CL_RankPoll();
	
	status = trap_CL_RankUserStatus();
	
	if( cgs.client_status != status )
	{
		// GRank status changed
		
		// inform UI of current status
		trap_Cvar_Set("client_status", va("%i",(int)(status)));
	
		// show rankings status dialog if error
		trap_CL_RankShowStatus((int)status);
		
		cgs.client_status = status;
	}
	return;
}
