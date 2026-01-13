
#ifndef __GAME_FIXEDPOD_H__
#define __GAME_FIXEDPOD_H__

extern const idEventDef EV_ExplodedBy;

class hhFixedPod : public idEntity {
public:
	CLASS_PROTOTYPE( hhFixedPod );

					hhFixedPod();
	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
	virtual void	Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );	

	void			Explode( idEntity *attacker );
	void			SpawnDebris();

protected:
	void			Event_Trigger( idEntity *activator );
	void			Event_ExplodeDamage( idEntity *attacker );
	void			Event_ExplodedBy( idEntity *activator );
	void			Event_SpawnDebris();
	void			Event_AssignFx( hhEntityFx* fx );

	idEntityPtr<hhEntityFx>	fx;
};


#endif /* __GAME_FIXEDPOD_H__ */
