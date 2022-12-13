#ifndef __PREY_DOOR_H__
#define __PREY_DOOR_H__

class hhDoor : public idDoor {
	CLASS_PROTOTYPE( hhDoor );

public:
	void			Spawn();
	virtual			~hhDoor();
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual void	Use_BinaryMover( idEntity *activator );
	virtual void	GotoPosition1();
	virtual void	GotoPosition2();

	//HUMANHEAD: aob
	virtual void	GotoPosition1( float wait );
	virtual void	GotoPosition2( float wait );
	//HUMANHEAD END

	ID_INLINE const char* GetAirLockTeamName() const { return airlockTeamName.c_str(); }
	ID_INLINE hhDoor* GetAirLockMaster() const { return airlockMaster; }
	void			Open();
	void			Close();
	bool			CanOpen() const;
	bool			CanClose() const;
	bool			IsClosed() const;

	bool			OnMyMoveTeam( hhDoor* doorPiece ) const;
	void			CopyTeamInfoToMoveMaster( hhDoor* master );

	void			CancelReturnToPos1();

	void			SetBuddiesShaderParm( int parm, float value );
	void			ToggleBuddiesShaderParm( int parm, float firstValue, float secondValue, float toggleDelay );
	
	bool			ForcedOpen() const { return( forcedOpen ); };
	void 			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

protected:
	idEntity*		DetermineTeamMaster( const char* teamName );
	void			JoinAirLockTeam( hhDoor *master );
	void			VerifyAirlockTeamStatus();

	void			ForceAirLockTeamClosed();
	void			ForceAirLockTeamOpen();

protected:
	void			Event_ReturnToPos1( void );			// overridden to stop it moving when closed
	void			Event_SetBuddiesShaderParm( int parm, float value );
	void			Event_Reached_BinaryMover( void );
	virtual void	Event_Touch( idEntity *other, trace_t *trace );

public:
	idLinkList<hhDoor> airlockTeam;

protected://More airLock stuff
	idStr			airlockTeamName;
	hhDoor*			airlockMaster;
	
	bool			openWhenDead;	// Do we open when dead?
	bool			forcedOpen;		// Is the door forced open
	float			airLockSndWait;
	float			nextAirLockSnd;
	bool			bShuttleDoors;
};

#endif
