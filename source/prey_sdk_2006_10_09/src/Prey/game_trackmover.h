#ifndef __GAME_TRACKMOVER_H__
#define __GAME_TRACKMOVER_H__


class hhTrackMover : public idMover {
	CLASS_PROTOTYPE( hhTrackMover );
public:
					hhTrackMover();
	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
	idEntity *		GetNextDestination( void );
	void			DoneMoving( void );

protected:
	enum States {
		StateGlobal = 0,
		StateIdle,
		StateDead
	} state;

	int				timeBetweenMoves;
	idEntity *		currentNode;		// Current node of track

	void			Event_Activate( idEntity *activator );
	void			Event_Deactivate();
	void			Event_PostSpawn( void );
	void			Event_StartNextMove( void );
	void			Event_Kill();
};


#endif /* __GAME_TRACKMOVER_H__ */
