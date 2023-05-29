
#ifndef __PREY_SLUDGE_H__
#define __PREY_SLUDGE_H__

class hhLiquid : public hhRenderEntity {
public:
	CLASS_PROTOTYPE( hhLiquid );

	void			Spawn(void);
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
	virtual void	Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );

	void			Disturb( const idVec3 &point, const idBounds &bounds, const float magnitude );

protected:
	virtual void	Event_Touch(idEntity *other, trace_t *trace);
	void			Event_Disturb(const idVec3 &position, float size, float magnitude);

protected:
	float			factor_movement;
	float			factor_collide;
};


#endif /* __PREY_SLUDGE_H__ */
