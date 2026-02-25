
class idTurret : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idTurret );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );

	virtual void			Think( void );
	virtual void			Present( void );


private:

	void					UpdateStates( void );
	enum					{ OFF, ON, CLOSING };
	int						state;


	enum					{ IDLE, WARMUP, VOLLEYING };
	int						attackState;
	int						nextStateTime;
	int						volleyCount;
	int						nextIdleSound;

	
	void					Event_activate( int value );
	void					Event_isactive();


	void					MuzzleflashOff();

	idMat3					bodyAxis;
	idMat3					turretAxis;

	idBeam*					beamStart;	
	idBeam*					beamEnd;

	idEntity*				laserdot;


	idEntityPtr<idEntity>	targetEnt;		// entity being dragged


	bool					CheckTargetLOS(idEntity *ent, idVec3 offset);
	bool					CheckTargetValidity(idEntity *ent);
	void					GotoWarmupState( void );
};
