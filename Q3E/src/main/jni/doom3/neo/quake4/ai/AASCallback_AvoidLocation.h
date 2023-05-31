// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __AASCALLBACK_AVOIDLOCATION_H__
#define __AASCALLBACK_AVOIDLOCATION_H__

/*
===============================================================================

	idAASCallback_AvoidLocation

===============================================================================
*/

struct idAASObstacle
{
	idBounds				absBounds;		// absolute bounds of obstacle
	mutable idBounds		expAbsBounds;	// expanded absolute bounds of obstacle
};

class idAASCallback_AvoidLocation : public idAASCallback
{
public:
	idAASCallback_AvoidLocation();
	~idAASCallback_AvoidLocation();

	void					SetAvoidLocation( const idVec3& start, const idVec3& avoidLocation );
	void					SetObstacles( const idAAS* aas, const idAASObstacle* obstacles, int numObstacles );

	virtual bool			PathValid( const idAAS* aas, const idVec3& start, const idVec3& end );
	virtual int				AdditionalTravelTimeForPath( const idAAS* aas, const idVec3& start, const idVec3& end );
	virtual bool			AreaIsGoal( const idAAS* aas, int areaNum ) = 0;

private:
	idVec3					avoidLocation;
	float					avoidDist;
	const idAASObstacle* 	obstacles;
	int						numObstacles;
};

#endif /* !__AASCALLBACK_AVOIDLOCATION_H__ */
