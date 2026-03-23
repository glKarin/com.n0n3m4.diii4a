
class idBluebox : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idBluebox );



	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );

	void					OnFrob( void );

	void					OnDown( void );
	void					OnUp( void );
	void					OnNone( void );
	void					OnExecute( void );
	void					OnDrop( void );

	void					Reset( bool closeDoor );

private:


	void					WeevilOpen( void );
	void					GetOpenState( void );


	enum					{ READY, PLUGTRANSITION, BLUEBOXTRANSITION, BLUEBOXHELD };
	int						state;

	int						blueboxTransitionTime;
	idVec3					blueboxStartPos;

	void					OnUnplugDone( void );
	void					OnExecuteDone( void );
	void					PlugConnected( bool playEffects );
	void					DoPlug( void );
	void					HandleSelection( int delta );
	void					AttachBlueboxToCable();


	jointHandle_t			joint_W;
	jointHandle_t			joint_S;
	jointHandle_t			joint_execute;

	int						selectedIndex;
	int						maxIndex;


	bool					isOpen;
	bool					isPlugged;
	bool					canSelect;

	idMover *				plug;
	idEntity *				cable;
	idEntity *				bluebox;


	
};
