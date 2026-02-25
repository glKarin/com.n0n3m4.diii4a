
class idKeypad : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idKeypad );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );


	virtual void			Think( void );	

	void					OnFrob( idEntity* activator );

private:

	enum					{ OFF, ACTIVE, CONFIRM_SUCCESS, CONFIRM_FAIL, READY_TO_CLOSE };
	int						state;

	int						counter;
	int						nextStateTime;

	idEntity*				frobcubeMain;
	idEntity*				frobcubes[9];
	const idDeclSkin *		skin_glow[9];
	int						transitions[9];

	void					GenerateKey( void );
	void					GenerateDictionary( void );
	idStrList				keys;

	int						keycode[4];
	int						input[4];
	int						keyIndex;

	idEntity*				bluebox;

	idStr					GetJointViaIndex( int index );

	void					UpdateStates( void );
	void					Event_keypadopen( int value );

};
