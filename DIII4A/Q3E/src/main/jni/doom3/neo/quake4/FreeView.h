#ifndef __FREEVIEW_H__
#define __FREEVIEW_H__

class idPhysics_Player;

class idFreeView {
public:

	idFreeView() { physics = NULL; snapAngle = false; }

	~idFreeView();

	// start free flying from this client's current position
	void SetFreeView( int clientNum );

	// pick a random spawn in the map
	void PickRandomSpawn( void );
	
	// update view and position
	void Fly( const usercmd_t &ucmd );

	void Draw( void );

	bool Initialized( void ) const { return physics != NULL; }

	void Shutdown( void );

	const idVec3 & GetOrigin( void );

private:

	void Setup( void );

	renderView_t		view;
	idPhysics_Player	*physics;
	idAngles			viewAngles;

	bool				snapAngle;
	idAngles			viewAngleOffset;
};

#endif
