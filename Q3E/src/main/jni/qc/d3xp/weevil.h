
class idWeevil : public idMoveableItem
{
public:
	CLASS_PROTOTYPE( idWeevil );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );
	virtual void			Present( void );

	void					OnGet( void );


	bool					isJumping( void );

private:


	bool					isGravity;
	enum					{ IDLE, WALKING, TURNING, JUMPSTART, JUMPING };
	int						state;
	
	int						walkDirection;
	int						turnCurrent;
	int						turnTarget;
	int						turnDirection;

	int						offroadTimer;
	int						jumpstartTimer;

	idVec3					lastPosition;


	void					Event_doPlug( int active, idEntity * plugEnt );
	void					Event_PlayAnim( const char* animname, bool cycle );
	void					Event_stop( void );
	void					Event_forward( int distance );
	void					Event_turn( int degrees );
	void					Event_stand( void );
	void					Event_jump( void );
	void					Event_light( int value );
	void					Event_gravity( int value );
	void					Event_getgravity( );
	void					Event_plugdone(idVec3 const &position );
	void					Event_isweevilplugconnected( void );







	void					UpdateStates( void );
	bool					CanMove( void );
	bool					HasObstacle( void );

	idAnimatedEntity *		model;
	idEntity *				nosecam;
	idLight *				light;

	idMover *				plugModel;
	idBeam*					beamStart;
	idBeam*					beamEnd;


	void					Event_weevilEyeMove(float pitch, float yaw, bool reset);
	jointHandle_t			headJoint;
	idAngles				headAngle;
};
