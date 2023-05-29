
#ifndef __HH_GAME_RAIL_H__
#define __HH_GAME_RAIL_H__

class hhBindController;

/*
===================================================================================

	hhRailRide

	An animating object that controls player position and orientation.
	Player can look around within some restricted cone of vision.

===================================================================================
*/

class hhRailRide : public hhAnimated {

public:
	CLASS_PROTOTYPE( hhRailRide );

	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual bool			Pain(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void					Attach(idEntity *rider, bool loose);
	void					Detach();
	bool					ValidRider(idEntity *entity);

protected:
	void					Event_Activate( idEntity *activator );
	void					Event_Attach(idEntity *rider, bool loose);
	void					Event_Detach();

	hhBindController *		bindController;
};


#endif 