#ifndef __HH_LIGHT_FIXTURE_H
#define __HH_LIGHT_FIXTURE_H

class hhLightFixture : public hhAFEntity {
	CLASS_PROTOTYPE( hhLightFixture );

public:
					~hhLightFixture();

	void			Spawn();
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
	
	virtual	void	Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void	Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void	Present( void );

protected:
	void			GetBoundLight();
	idLight*		SearchForBoundLight();
	bool			StillBound( const idLight* light );
	void			RemoveLight();

	void			PresentModelDefChange( void );

protected:
	void			Event_PostSpawn();
	void			Event_Hide();
	void			Event_Show();

protected:
	jointHandle_t			collisionBone;
	
	idEntityPtr<idLight>	boundLight;
};

#endif