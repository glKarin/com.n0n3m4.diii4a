#ifndef __HH_HEALTH_BASIN_H
#define __HH_HEALTH_BASIN_H

/*
TODO:
	+ effects do not delete when using hhUtils::RemoveContents
	+ Need to have player face basin to allow use
*/

class hhHealthBasin : public hhAnimatedEntity {
	CLASS_PROTOTYPE( hhHealthBasin );
	
public:
					hhHealthBasin();
	virtual			~hhHealthBasin();

	void			Spawn();
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

protected:
	void			SpawnTrigger();
	void			SpawnFx();

	bool			ActivatorVerified( const idEntityPtr<idActor>& Activator );

	int				PlayAnim( const char* pName, int iBlendTime = 0 );
	void			PlayCycle( const char* pName, int iBlendTime = 0 );
	void			ClearAnims( int iBlendTime = 0 );

protected:
	void			Event_AppendFxToIdleList( hhEntityFx* fx );
	void			Event_Activate( idEntity *pActivator );
	void			Event_IdleMode();
	void			Event_FinishedPuking();
	void			Event_Expended();
	void			Event_GiveHealth();

protected:
	enum EBasinMode {
		BASIN_Idle,
		BASIN_Puking,
		BASIN_Expended,
	} BasinMode;

	idEntityPtr<idActor>	activator;
	float			currentHealth;

	idBounds		verificationAbsBounds;

	idList< idEntityPtr<idEntityFx> >	idleFxList;
};

#endif
