
class idCameraPoint : public idEntity
{
public:
	CLASS_PROTOTYPE( idCameraPoint );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	//virtual void			Think( void );
	//virtual void			Present( void );

	bool					snapped;
	void					SetSnapped();
	void					SetHover(int value);

private:

	void					Event_setcamerapointvisibility( int value);
	idEntity *				arrowModel;
	
};
