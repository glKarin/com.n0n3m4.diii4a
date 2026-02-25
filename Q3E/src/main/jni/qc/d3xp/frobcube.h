
class idFrobCube : public idStaticEntity
{
public:
	CLASS_PROTOTYPE( idFrobCube );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					OnFrob( idEntity* activator );

private:

	idStr					functionName;
	idStr					masterName;

//	void					Event_onfrobPanel_Reset( void );
};
