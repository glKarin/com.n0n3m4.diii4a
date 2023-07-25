
class idTarget_EndLevel : public idEntity {
public:
	CLASS_PROTOTYPE( idTarget_EndLevel );

	void	Spawn( void );
			~idTarget_EndLevel();

	void	Draw();
	// the endLevel will be responsible for drawing the entire screen
	// when it is active

	void	PlayerCommand( int buttons );
	// when an endlevel is active, plauer buttons get sent here instead
	// of doing anything to the player, which will allow moving to
	// the next level

	const char *ExitCommand();
	// the game will check this each frame, and return it to the
	// session when there is something to give

private:
	idStr	exitCommand;

	idVec3	initialViewOrg;
	idVec3	initialViewAngles;
	// set when the player triggers the exit

	idUserInterface	*gui;

	bool	buttonsReleased;
	// don't skip out until buttons are released, then pressed

	bool	readyToExit;
	bool	noGui;

	void	Event_Trigger( idEntity *activator );
};

