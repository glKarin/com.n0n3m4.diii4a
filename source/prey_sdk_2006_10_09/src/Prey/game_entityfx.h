#ifndef __HH_ENTITY_FX_H
#define __HH_ENTITY_FX_H

class hhEntityFx : public idEntityFx {
	CLASS_PROTOTYPE( hhEntityFx );

public:
						hhEntityFx();
	virtual				~hhEntityFx();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	//HUMANHEAD rww
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );
	//HUMANHEAD END

	virtual void		Run( int time );

	// HUMANHEAD nla
	void				SetUseAxis( fxAxis theAxis ) { };
	void				SetFxInfo( const hhFxInfo &i ) { fxInfo = i; setFxInfo = true; removeWhenDone = fxInfo.RemoveWhenDone(); GetRenderEntity()->weaponDepthHack = fxInfo.UseWeaponDepthHack(); }
	bool				RemoveWhenDone() { return( removeWhenDone ); }
	void				RemoveWhenDone( bool remove ) { removeWhenDone = remove; }
	void				Toggle() { Nozzle( !IsActive(TH_THINK) ); }
	void				Nozzle( bool bOn );
	idMat3				DetermineAxis( const idFXSingleAction& fxaction );
	void				CreateFx( idEntity *activator );

	virtual void		Hide();

	static hhEntityFx	*StartFx( const char *fx, const idVec3 *useOrigin, const idMat3 *useAxis, idEntity *ent, bool bind );
	void				SetParticleShaderParm( int parmnum, float value );
	// HUMANHEAD END

protected:
	virtual void		CleanUpSingleAction( const idFXSingleAction& fxaction, idFXLocalAction& laction );
	virtual void		DormantBegin();
	virtual void		DormantEnd();

protected:
	void				Event_Trigger( idEntity *activator );
	void				Event_ClearFx( void );

protected:
	// HUMANHEAD nla
	enum fxAxis { AXIS_CURRENT, AXIS_NORMAL, AXIS_BOUNCE, AXIS_INCOMING, AXIS_CUSTOMLOCAL };
	// HUMANHEAD END

	// HUMANHEAD 
	hhFxInfo				fxInfo;
	bool					setFxInfo;
	bool					removeWhenDone;
	// HUMANHEAD END

	// HUMANHEAD bg
	bool					restartActive;
	// HUMANHEAD END
};

#endif