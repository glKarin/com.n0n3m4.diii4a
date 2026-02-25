
class idLever : public idAnimated
{
public:
	CLASS_PROTOTYPE( idLever );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );
	virtual void			Think( void );
	void					OnFrob( idEntity* activator );
	void					Reset();

private:

	bool					toggleable;
	int						toggleState;

	enum					{ IDLE, PRESSED };
	int						state;

	int						nextStateTime;

	void					UpdateStates( void );


	void					OnFrobScript( void );
};
