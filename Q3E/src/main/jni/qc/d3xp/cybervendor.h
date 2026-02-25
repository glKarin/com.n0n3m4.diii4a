
class idCybervendor : public idStaticEntity
{
public:
	CLASS_PROTOTYPE( idCybervendor );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual void			Think( void );
	void					OnFrob( );

private:

	int						state;
	int						nextUpdate;
	int						lerpStart;
	int						lerpEnd;

	idVec3					posStart;
	idVec3					posEnd;

	idVec3					angStart;
	idVec3					angEnd;

	int						count;
};
