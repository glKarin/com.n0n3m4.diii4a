// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_MISC_PARACHUTE_H__
#define __GAME_MISC_PARACHUTE_H__

#include "../ScriptEntity.h"

class sdParachute : public sdScriptEntity {
	CLASS_PROTOTYPE( sdParachute );

	void					Spawn( void );
	virtual void			Think( void );

	virtual void			UpdateModelTransform( void );
	virtual void			Present( void );

	void					ApplyParachute( idEntity* owner, float canopyScale );

protected:
	void					Event_SetOwner( idEntity* _owner );
	void					Event_SetDeployStart( float time );
	void					Event_IsMovingTooSlow( void );

	int						deployStartTime;
	int						tooSlowTime;
	float					deployTime;

	float					maxSpeed;
	float					radius;
	float					forceHeight;
	float					height;
	float					Cd_up; // Gordon: FIXME: These variables are all against the naming standard
	float					Cd_side;
	float					Cl_side;
	float					maxSideDrag;
	float					rho;

	float					scale;

	idVec3					ownerOffset;

	idEntityPtr< idEntity >	owner;
};


#endif // __GAME_MISC_HEIGHTMAP_H__
