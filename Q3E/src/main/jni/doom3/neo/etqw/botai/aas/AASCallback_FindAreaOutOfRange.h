// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __AASCALLBACK_FINDAREAOUTOFRANGE_H__
#define __AASCALLBACK_FINDAREAOUTOFRANGE_H__

#include "AASCallback_AvoidLocation.h"

/*
===============================================================================

	idAASCallback_FindAreaOutOfRange

===============================================================================
*/

class idAASCallback_FindAreaOutOfRange : public idAASCallback_AvoidLocation {
public:
							idAASCallback_FindAreaOutOfRange( const idVec3 &targetPos, float maxDist );

	virtual bool			AreaIsGoal( const idAAS *aas, int areaNum );

private:
	idVec3					targetPos;
	float					maxDistSqr;
};

#endif /* !__AASCALLBACK_FINDAREAOUTOFRANGE_H__ */
