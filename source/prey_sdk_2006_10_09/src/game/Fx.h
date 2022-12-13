// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __GAME_FX_H__
#define __GAME_FX_H__

/*
===============================================================================

  Special effects.

===============================================================================
*/

typedef struct {
	renderLight_t			renderLight;			// light presented to the renderer
	qhandle_t				lightDefHandle;			// handle to renderer light def
	renderEntity_t			renderEntity;			// used to present a model to the renderer
	int						modelDefHandle;			// handle to static renderer model
	float					delay;
	//HUMANHEAD: aob - made particleSystem an idDeclParticle
	const idDeclParticle*	particleSystem;
	int						particleStartTime;
	//HUMANHEAD END
	int						start;
	bool					soundStarted;
	bool					shakeStarted;
	bool					decalDropped;
	bool					launched;
} idFXLocalAction;

// HUMANHEAD PDM
extern const idEventDef EV_Fx_KillFx;
extern const idEventDef EV_Fx_Action;
// HUMANHEAD END

class idEntityFx : public idEntity {
public:
	CLASS_PROTOTYPE( idEntityFx );

							idEntityFx();
	virtual					~idEntityFx();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think();
	void					Setup( const char *fx );
	virtual //HUMANHEAD: aob - made virtual
	void					Run( int time );
	void					Start( int time );
	void					Stop( void );
	const int				Duration( void );
	const char *			EffectName( void );
	const char *			Joint( void );
	const bool				Done();

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );

	static idEntityFx *		StartFx( const char *fx, const idVec3 *useOrigin, const idMat3 *useAxis, idEntity *ent, bool bind );

	// HUMANHEAD: aob - all definitions are in hhEntityFx
	virtual void			SetUseAxis( fxAxis theAxis ) {}
	virtual void			SetFxInfo( const hhFxInfo &i ) {}
	virtual bool			RemoveWhenDone() { return false; }
	virtual void			RemoveWhenDone( bool remove ) {}
	virtual void			Toggle() {}
	virtual void			Nozzle( bool bOn ) {}
	// HUMANHEAD END

	//HUMANHEAD rww
	bool					persistentNetTime;

	//for functionality regarding client entity effects.
	idEntityPtr<idEntity>	snapshotOwner;
	//HUMANHEAD END
protected:
	void					Event_Trigger( idEntity *activator );
	void					Event_ClearFx( void );

	void					CleanUp( void );
	virtual //HUMANHEAD: aob - made virtual
	void					CleanUpSingleAction( const idFXSingleAction& fxaction, idFXLocalAction& laction );
	void					ApplyFade( const idFXSingleAction& fxaction, idFXLocalAction& laction, const int time, const int actualStart );

	int						started;
	int						nextTriggerTime;
	const idDeclFX *		fxEffect;				// GetFX() should be called before using fxEffect as a pointer
	idList<idFXLocalAction>	actions;
	idStr					systemName;
};

class idTeleporter : public idEntityFx {
public:
	CLASS_PROTOTYPE( idTeleporter );

private:
	// teleporters to this location
	void					Event_DoAction( idEntity *activator );
};

#endif /* !__GAME_FX_H__ */
