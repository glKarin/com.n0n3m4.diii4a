
class idButtonSwitcher : public idAnimated
{
public:
	CLASS_PROTOTYPE( idButtonSwitcher );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	//virtual void			Think( void );
	void					OnFrob( idEntity* activator );

private:

	int						state;

	bool					active;
	


	void					Event_buttonswitcheractivate(int value);
	void					CallScript(const char* name);
};
