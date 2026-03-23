
class idPowerSawGeneric : public idMover
{
public:
							CLASS_PROTOTYPE( idPowerSawGeneric );

							
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Think( void );
	void					OnSplineEnd( void );
	void					Frob( void );
	void					Spawn( void );

private:

	enum					{ IDLE, SAWING, DONE };
	int						state;

	int						nextSparktime;

	void					Spawn2( void );
	idStr					callName;

	idMover *				mover;
	idEntity *				splineEnt;
	idEntity *				sparks;

	idLight *				light;

	void					reset( void );
};
