
class idPicker : public idEntity {
public:
	CLASS_PROTOTYPE( idPicker );

							idPicker( void );
							~idPicker( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	//void					Initialize( void );
	void					SetDragDistance( float dist );
	int						Update( idPlayer *player, bool hide, idEntity *newEnt, int newEntID, bool forcePick );
	int						UpdateHeld( idPlayer *player );

	idEntityPtr<idEntity>	dragEnt;			// entity being dragged
	void					SetHoldDistance( int delta );
	bool					attackHeld;

	void					StopDrag( bool dropOnly, bool isRightClick );

private:


	int						attacktimerStart;
	int						pickuptimerStart;

	int						holdstate;
	
	

	int						holdDistance;	//how close/far to hold the object.	
	idForce_Grab			drag;
	idVec3					saveGravity;
	idEntityPtr<idPlayer>	owner;
	int						oldUcmdFlags;
	int						id;					// id of body being dragged
	idVec3					localPlayerPoint;	// dragged point in player space
	int						dragFailTime;
	int						startDragTime;
	bool					holdingAF;
	bool					pickerHeld;
	
	float					dragTraceDist;

	int						rotationIndex;
	
	
	void					StartDrag( idEntity *grabEnt, int id );


	
	int						rotationAngleStart;
	int						rotationAngleEnd;
	int						rotationTimeStart;
	int						rotationTimeEnd;
};