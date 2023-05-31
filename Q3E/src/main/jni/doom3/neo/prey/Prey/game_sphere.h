
#ifndef __GAME_SPHERE_H__
#define __GAME_SPHERE_H__

class hhSphere : public hhMoveable {
public:
	CLASS_PROTOTYPE( hhSphere );

						hhSphere();
	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	virtual void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual bool		GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual void		Think( void );
	virtual void		ClientPredictionThink( void );

protected:
	void				RollThink( void );
	void				CreateLight();
	void				Event_Touch( idEntity *other, trace_t *trace );

private:
	float					radius;					// radius of sphere
	idVec3					lastOrigin;				// origin last frame
	idMat3					additionalAxis;			// transformation for visual model
	idEntityPtr<idLight>	light;					// light
};


#endif /* __GAME_SPHERE_H__ */
