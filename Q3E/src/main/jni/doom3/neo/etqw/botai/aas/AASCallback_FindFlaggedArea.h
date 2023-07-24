// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __AASCALLBACK_FINDFLAGGEDAREA_H__
#define __AASCALLBACK_FINDFLAGGEDAREA_H__

#include "AASCallback_AvoidLocation.h"

/*
===============================================================================

	idAASCallback_FindFlaggedArea

===============================================================================
*/

class idAASCallback_FindFlaggedArea : public idAASCallback_AvoidLocation {
public:
							idAASCallback_FindFlaggedArea( const int areaFlag, bool set );
							~idAASCallback_FindFlaggedArea();

	virtual bool			AreaIsGoal( const idAAS *aas, int areaNum );

private:
	int						areaFlag;
	int						test;
};

#endif /* !__AASCALLBACK_FINDFLAGGEDAREA_H__ */
