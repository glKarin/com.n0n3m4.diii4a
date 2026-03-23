
class idTrembler : public idAnimatedEntity
{
public:
							CLASS_PROTOTYPE( idTrembler );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	void					Event_Touch( idEntity *other, trace_t *trace );
	void					Think( void );

	void					Reset( void );

private:
	

	int						ticksoundTime;
	int						state;	
	void					Arm( void );
	idVec3					itemOriginalPosition;
	//idEntity*				itemEnt;
	idEntityPtr<idEntity>	itemEnt;
};
