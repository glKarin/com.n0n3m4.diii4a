
class idPanel : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idPanel );


	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );

	virtual void			Think( void );
	virtual void			Present( void );

	void					OnFrob( idEntity* activator );



private:

	enum					{ IDLE, CONNECTED };
	int						state;

	enum					{ IDX_SCREW0, IDX_SCREW1, IDX_BUNDLE, IDX_DOOR_OPENED, IDX_DOOR_CLOSED, IDX_CABLE };

	int						unscrewedCount;

	//int						connectTime; //what time the cable was connected.

	idEntity*				relay;

	idEntity*				frobcube0;
	idEntity*				frobcube1;

	idEntity*				frobcube_opened;
	idEntity*				frobcube_closed;

	idEntity*				bundle;
	idEntity*				gatorCable;

	idBeam*					beamStart;
	idBeam*					beamEnd;

	idEntity*				screw0;
	idEntity*				screw1;

	void					Event_Panel_Reset( void );
	void					Event_Panel_OnConnect( void );
	void					Event_Panel_Disconnect( void );
	void					Event_Panel_Open( void );
	void					Event_Panel_Close( void );

};
