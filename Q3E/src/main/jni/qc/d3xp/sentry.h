
class idSentry : public idMoveableItem
{
public:
	CLASS_PROTOTYPE( idSentry );

	void					Spawn( void );

	virtual void			Think( void );


	void					OnGet( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );


private:

	enum					{ OFF, DEPLOYING, ON, CLOSING };
	int						state;

	float					yaw;
	float					pitch;
	
	void					Event_sentrykill( void );
	void					Event_activate( int value );

	void					Event_issentryactive();

	void					Event_sentryturn( float yawValue );
	void					Event_sentrypitch( float pitchValue );
	void					Event_sentryface( float x, float y, float z );
	void					Event_sentryfire( void );
	void					Event_sentrystand( void );
	void					Event_sentrygetlaser( void );


	void					fireShot( void );

	idBeam*					beamStart;	
	idBeam*					beamEnd;

	idAnimatedEntity *		model;

	idEntity*				laserdot;
	idEntity *				nosecam;
	int						laserActivateTime;


	float					yawStart;
	float					yawEnd;
	int						yawTimeStart;
	int						yawTimeEnd;
	bool					yawActive;

	float					pitchStart;
	float					pitchEnd;
	int						pitchTimeStart;
	int						pitchTimeEnd;
	bool					pitchActive;


	int						queuedShots;
	int						nextShotTime;
};
