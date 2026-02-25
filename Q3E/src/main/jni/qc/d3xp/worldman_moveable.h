
typedef struct record_moveable_s
{
	int				timestamp;
	idVec3			position;
	idMat3			angle;	
	bool			done;	
} record_moveable_t;


class idWorldManager_Moveable : public idEntity
{
public:
							CLASS_PROTOTYPE( idWorldManager_Moveable );

	void					Spawn( void );
	virtual void			Think( void );

	void					Event_startplay( void );
	void					Event_startrecord( bool continuation );
	void					Event_stop( void );


private:

	idEntityPtr<idEntity>	targetEnt;
	idList<record_moveable_t>	events;
	
	int						state;
	enum					{ OFF, RECORDING, PLAYING };
	int						index;

	void					UpdateRecord( void );
	void					UpdatePlay( void );

	

	idVec3					lastPosition;
	int						lastRecordTime;
	int						lastTimestamp;

	int						startTime;
};


