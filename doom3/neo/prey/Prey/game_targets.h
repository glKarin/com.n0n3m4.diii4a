
#ifndef __GAME_TARGETS_H__
#define __GAME_TARGETS_H__

class hhTarget_SetSkin : public idTarget {
public:
	CLASS_PROTOTYPE(hhTarget_SetSkin);

	void		Spawn(void);
	void		Save( idSaveGame *savefile ) const	{ savefile->WriteString(skinName); }
	void		Restore( idRestoreGame *savefile )	{ savefile->ReadString(skinName); }

protected:
	void		Event_Trigger( idEntity *activator );

	idStr		skinName;
};

class hhTarget_Enable : public idTarget {
public:
	CLASS_PROTOTYPE(hhTarget_Enable);

	void		Spawn(void);

protected:
	void		Event_Trigger( idEntity *activator );
};

class hhTarget_Disable : public idTarget {
public:
	CLASS_PROTOTYPE(hhTarget_Disable);

	void		Spawn(void);

protected:
	void		Event_Trigger( idEntity *activator );
};

class hhTarget_Earthquake : public idTarget {
public:
	CLASS_PROTOTYPE(hhTarget_Earthquake);

	void			Spawn(void);
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
	virtual void	Present(void) { }
	virtual void	Think(void);

protected:
	void		Event_Trigger( idEntity *activator );
	void		Event_TurnOff( void );

private:
	float			shakeTime;
	float			shakeAmplitude;
	idForce_Field	forceField;
	idClipModel		*cm;
};

class hhTarget_SetLightParm : public idTarget {
public:
	CLASS_PROTOTYPE( hhTarget_SetLightParm );

protected:
	void				Event_Activate( idEntity *activator );
};

class hhTarget_PlayWeaponAnim : public idTarget {
public:
	CLASS_PROTOTYPE(hhTarget_PlayWeaponAnim);

protected:
	void		Event_Trigger( idEntity *activator );
};

class hhTarget_ControlVehicle : public idTarget {
	CLASS_PROTOTYPE( hhTarget_ControlVehicle );

public:
	void					Spawn();

protected:
	void					Event_Activate( idEntity *activator );
};

class hhTarget_AttachToRail : public idTarget {
	CLASS_PROTOTYPE( hhTarget_AttachToRail );

public:
	void					Spawn();

protected:
	void					Event_Activate( idEntity *activator );
};

class hhTarget_EnableReactions : public idTarget {
	CLASS_PROTOTYPE( hhTarget_EnableReactions );

public:
	void					Spawn();

protected:
	void					Event_Activate( idEntity *activator );
};

class hhTarget_DisableReactions : public idTarget {
	CLASS_PROTOTYPE( hhTarget_DisableReactions );

public:
	void					Spawn();

protected:
	void					Event_Activate( idEntity *activator );
};

class hhTarget_EnablePassageway : public idTarget {
	CLASS_PROTOTYPE( hhTarget_EnablePassageway );

public:
	void					Spawn();

protected:
	void					Event_Activate( idEntity *activator );
};

class hhTarget_DisablePassageway : public idTarget {
	CLASS_PROTOTYPE( hhTarget_DisablePassageway );

public:
	void					Spawn();

protected:
	void					Event_Activate( idEntity *activator );
};

class hhTarget_PatternRelay : public idTarget {
	CLASS_PROTOTYPE( hhTarget_PatternRelay );

public:
	void					Spawn();
	void					Save( idSaveGame *savefile ) const { savefile->WriteFloat(timeGranularity); }
	void					Restore( idRestoreGame *savefile ) { savefile->ReadFloat(timeGranularity); }

protected:
	void					Event_Activate( idEntity *activator );
	void					Event_TriggerTargets();

	float					timeGranularity;
};

class hhTarget_Subtitle : public idTarget {
	CLASS_PROTOTYPE( hhTarget_Subtitle );

public:
	void					Spawn();

protected:
	void					Event_Activate( idEntity *activator );
	void					Event_FadeOutText();
};


class hhTarget_EndLevel : public idTarget_EndLevel {
	CLASS_PROTOTYPE( hhTarget_EndLevel );

protected:
	virtual void			Event_Activate(idEntity *activator);
};


class hhTarget_ConsolidatePlayers : public idTarget {
	CLASS_PROTOTYPE( hhTarget_ConsolidatePlayers );

protected:
	void					Event_Activate( idEntity *activator );
};


class hhTarget_WarpPlayers : public idTarget {
	CLASS_PROTOTYPE( hhTarget_WarpPlayers );

protected:
	void					Event_Activate( idEntity *activator );
};

class hhTarget_FollowPath : public idTarget {
	CLASS_PROTOTYPE( hhTarget_FollowPath );

protected:
	void					Event_Activate( idEntity *activator );
};

class hhTarget_LockDoor: public idTarget {
public:
	CLASS_PROTOTYPE( hhTarget_LockDoor );

private:
	void				Event_Activate( idEntity *activator );
};

class hhTarget_DisplayGui : public idTarget {
public:
	CLASS_PROTOTYPE(hhTarget_DisplayGui);

protected:
	void		Event_Activate( idEntity *activator );
};

class hhTarget_Autosave : public idTarget {
public:
	CLASS_PROTOTYPE(hhTarget_Autosave);

protected:
	void		Event_Activate( idEntity *activator );
	void		Event_Autosave( void );
	void		Event_FinishedSave( void );
};

class hhTarget_Show : public idTarget {
public:
	CLASS_PROTOTYPE(hhTarget_Show);

protected:
	void		Event_Activate( idEntity *activator );
};

class hhTarget_Hide : public idTarget {
public:
	CLASS_PROTOTYPE(hhTarget_Hide);

protected:
	void		Event_Activate( idEntity *activator );
};

#endif /* __GAME_TARGETS_H__ */
