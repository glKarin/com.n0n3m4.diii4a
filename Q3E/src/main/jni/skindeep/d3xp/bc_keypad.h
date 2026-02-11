
class idKeypad : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idKeypad );

							idKeypad(void);
	virtual					~idKeypad(void);

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );


	virtual void			Think( void );	

	//void					OnFrob( idEntity* activator );
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			DoHack(); //for the hackgrenade.

	void					DetachFromWall();

	virtual void			Event_PostSpawn(void);

	void					SetCode(idStr code);

private:

	enum					{ OFF, ACTIVE, CONFIRM_SUCCESS, CONFIRM_FAIL, READY_TO_CLOSE, FULLY_CLOSED };
	int						state;

	int						counter;
	int						nextStateTime;

	//idEntity*				frobcubeMain;
	idEntity*				frobcubes[9] = {};
	const idDeclSkin *		skin_glow[9] = {};
	int						transitions[9];

	void					GenerateKeyFromKeyvalue(idStr newcode);
	void					GenerateKeyFromDictionary( void );
	idStrList				keys;

	int						keycode[4];
	int						input[4];
	int						keyIndex;

	

	idStr					GetJointViaIndex( int index );

	void					UpdateStates( void );
	void					Event_keypadopen( int value );


	

	void					ParseKeycodeValue();

	void					DoCodeSuccess();

	idVec3					GetAntennaPosition();
	idFuncEmitter			*zapParticle = nullptr;

};
