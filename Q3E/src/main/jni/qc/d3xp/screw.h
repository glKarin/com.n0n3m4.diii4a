
class idScrew : public idMoveableItem
{
public:
							CLASS_PROTOTYPE( idScrew );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Frob( void );
	void					Spawn( void );

	void					Reset( void );
	bool					unscrewed;

private:

	void					ScrewOut( void );
	idMover *				moverItem;
};
