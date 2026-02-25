
class idAirlock : public idStaticEntity
{
public:
	CLASS_PROTOTYPE( idAirlock );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );
	virtual void			Present( void );

	void					OnFrob( idEntity* activator );

private:

	bool					initialized;

	int						nextFlingTime;
	int						lastOpenTime;
	bool					lastOpenState;

	//idStr					functionName;
	//idStr					masterName;

	idMover *				carousel;
//	void					Event_onfrobPanel_Reset( void );

	idAnimated *			leverCarousel;
	idEntity *				frobcubeCarousel;
	idLight *				light;

	idDoor *				door_round;
	idDoor *				door_inner;

	idEntity *				bin;

	idVacuumSeparatorEntity * vacuumSeparator1;
	idVacuumSeparatorEntity * vacuumSeparator2;
};
