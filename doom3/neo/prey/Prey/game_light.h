#ifndef __HH_GAME_LIGHT_H
#define __HH_GAME_LIGHT_H

/*
===============================================================================

hhLight

===============================================================================
*/

class hhLight : public idLight {
	CLASS_PROTOTYPE( hhLight );
	
public:
	void			SetLightCenter( idVec3 center );

	void			StartAltSound();

protected:
	void			Event_SetTargetHandles( void );
	void			Event_StartAltMode();
};

#endif