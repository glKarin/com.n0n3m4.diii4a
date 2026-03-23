
class idPowerScrewGeneric : public idMover
{
public:
							CLASS_PROTOTYPE( idPowerScrewGeneric );


	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					OnActivate( void );
	void					Spawn( void );
	void					Reset( void );

private:

	//idMover *				mover;

	int						countMax;
	int						count;
};
