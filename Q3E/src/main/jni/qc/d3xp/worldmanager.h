
#ifndef __GAME_WORLDMANAGER_H__
#define __GAME_WORLDMANAGER_H__


typedef struct recordline_s
{
	idVec3			position1;
	idVec3			position2;
} recordline_t;

typedef struct recordlinecollection_s
{
	idList<recordline_t>	lines;	
} recordlinecollection_t;



typedef struct record_s
{
	int				timestamp;		
	idVec3			position;
	float			yaw;
	idStr			deckcommand;
	idStr			frobcommand;
	idStr			spawncommand;
	bool			done;
	int				entID;
	int				recordtype;
} record_t;

typedef struct personRecord_s
{
	idList<record_t>	events;
	idMover *			ghost;
	int					index;
	bool				playing;
	bool				completed;
} personRecord_t;


class idWorldManager : public idEntity
{
public:
							CLASS_PROTOTYPE( idWorldManager );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );

	void					RecordFrob( const char *entName );
	void					RecordDeck( const char *deckString );
	void					RecordSpawn( const char *defName, int id, const idVec3 &pos, float yaw );
	void					RecordUnspawn( int id );
	void					RecordLaunchAim( int id, const idVec3 &dir, float force );

	int						index;


private:

	
	
	int						state;
	enum					{ OFF, RECORDING };
	int						recordStarttime;
	int						nextRecordtime;
	idVec3					lastPosition;
	float					lastAngle;

	personRecord_t			records[3];
	recordlinecollection_t	recordlines[3];

	void					UpdatePlay( int idx );
	void					UpdateRecord( void );
	void					Event_recordstart(int idx, int continuation);	
	void					Event_recordplay(int idx);	
	void					Event_recordstop( void );
	void					Event_geteventcount( int idx );
	void					Event_getlasteventtimestamp( int idx );
	void					Event_setrecordstarttime();
	void					Event_moveplayertoghost(int idx);
	void					Event_recordclear(int idx);

	void					Event_getcomplete(int idx);
	void					Event_setcomplete(int idx, bool value);

	void					Event_reset( void );
};


#endif