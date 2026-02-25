
#define BEAMCOUNT		5

class idLaserWire : public idEntity
{
public:
	CLASS_PROTOTYPE( idLaserWire );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );
	virtual void			Present( void );


private:

	enum					{ IDLE, DIMMING, OFF, TURNON, DISABLED };

	int						nextThinkTime;

	int						state;

	idBeam*					boundBeam[ BEAMCOUNT ];	
	idBeam*					boundBeamTarget[ BEAMCOUNT ];

	void					Event_off( float delay );
	void					Event_on( void );
	void					Event_laserenable( int value );

	int						dimEndTime;
	int						dimStartTime;

	int						turnonTime;

	int						nextAlarmTime;

	int						softAlarmTime;
	bool					softAlarmReady;

	idEntity*				ghostImage;
	void					Event_KillGhost( void );

	int						softAlarmTimeMax;
};
