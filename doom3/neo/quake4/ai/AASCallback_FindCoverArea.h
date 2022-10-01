// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __AASCALLBACK_FINDCOVERAREA_H__
#define __AASCALLBACK_FINDCOVERAREA_H__

#include "AASCallback_AvoidLocation.h"

/*
===============================================================================

	idAASCallback_FindCoverArea

===============================================================================
*/

class idAASCallback_FindCoverArea : public idAASCallback_AvoidLocation
{
public:
	idAASCallback_FindCoverArea( const idVec3& hideFromPos );
	~idAASCallback_FindCoverArea();

	virtual bool			AreaIsGoal( const idAAS* aas, int areaNum );

private:
	static const int		MAX_PVS_AREAS = 4;

	pvsHandle_t				hidePVS;
	int						PVSAreas[ MAX_PVS_AREAS ];
};

#endif /* !__AASCALLBACK_FINDCOVERAREA_H__ */
