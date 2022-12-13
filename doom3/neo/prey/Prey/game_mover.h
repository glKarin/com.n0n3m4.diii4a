#ifndef __PREY_MOVER_H__
#define __PREY_MOVER_H__


class hhMover : public idMover {
	CLASS_PROTOTYPE( hhMover );

public:
	void					Spawn();
	void					BecomeNonSolid();
	void					Save( idSaveGame *savefile ) const { }
	void					Restore( idRestoreGame *savefile ) { Spawn(); }
};


class hhMoverWallwalk : public hhMover {
	CLASS_PROTOTYPE(hhMoverWallwalk);

public:
	void						Spawn( void );
	void						Save( idSaveGame *savefile ) const;
	void						Restore( idRestoreGame *savefile );
	void						SetWallWalkable(bool on);
	void						Think();

protected:
	void						Event_Activate(idEntity *activator);

protected:
	hhHermiteInterpolate<float>	alphaOn;		// Degree to which wallwalk is on
	bool						wallwalkOn;
	bool						flicker;
	const idDeclSkin			*onSkin;
	const idDeclSkin			*offSkin;
};


//=============================================================================

class hhExplodeMover : public hhMover {
	CLASS_PROTOTYPE( hhExplodeMover );

public:
	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	void					DoneMoving( void );

	float					GetMoveDelay( void ) { return moveDelay; }

protected:
	void					Event_PostSpawn( void );
	void					Event_Trigger( idEntity *activator );

protected:
	int						oldContents; // contents before moving (contents are none while moving)
	int						oldClipMask;
	float					moveDelay;
};

class hhExplodeMoverOrigin : public idEntity {
	CLASS_PROTOTYPE( hhExplodeMoverOrigin );

public:
	void					Spawn();
protected:
	void					Event_PostSpawn( void );
	void					Event_Trigger( idEntity *activator );
};

//=============================================================================

#endif	// __PREY_MOVER_H__
