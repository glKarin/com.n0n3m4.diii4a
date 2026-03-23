
class idClock : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idClock );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );	

private:

	bool					digital;

	int						startTime;
	void					Event_reset( void );
	void					Event_setTime( int time );
	void					SetHandAngle( const char *jointName, float angle );
	int						offsetTime;




	int						lastMinuteCount;
	float					startAngle;
	float					endAngle;
	int						minuteStartTime;
	int						minuteEndTime;
	

	int						nextSecondsInterval;
	int						secondsLerpStart;
	int						secondsLerpEnd;
	int						secondsState;
	int						secondsAngleStart;
	int						secondsAngleEnd;
};
