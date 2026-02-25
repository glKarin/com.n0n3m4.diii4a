
#define VO_BOOLSIZE		18

class idCountdowntimer : public idEntity
{
public:
	CLASS_PROTOTYPE( idCountdowntimer );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	
	virtual void			Think( void );

private:

	int						time;
	void					Event_reset( void );
	void					Event_start( void );
	void					Event_hasTripped( void );

	int						maxTime;


	enum					{ VO_5MIN, VO_4MIN, VO_3MIN, VO_2MIN, VO_1MIN, VO_45SEC, VO_30SEC, VO_10SEC, VO_9SEC, VO_8SEC, VO_7SEC, VO_6SEC, VO_5SEC, VO_4SEC, VO_3SEC, VO_2SEC, VO_1SEC };
	bool					voBool[VO_BOOLSIZE];

	bool					active;
	bool					DoVO(int timeLeft, int count, int voEnum, const char *voName);
	bool					hasTripped; //determines has this ever been run

	void					Event_Activate( idEntity *activator );
};
