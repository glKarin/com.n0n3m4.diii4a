// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_TARGET_H__
#define __GAME_TARGET_H__

/*
===============================================================================

idTarget

===============================================================================
*/

class idTarget : public idEntity {
public:
	CLASS_PROTOTYPE( idTarget );
};

/*
===============================================================================

idTarget_Null

===============================================================================
*/

class idTarget_Null : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_Null );

	void				Spawn();
};

#endif /* !__GAME_TARGET_H__ */
