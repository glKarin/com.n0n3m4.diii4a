#ifndef __HH_MOVEABLE_H
#define __HH_MOVEABLE_H


class hhMoveable: public idMoveable {
	CLASS_PROTOTYPE( hhMoveable );

public:
						hhMoveable();
						~hhMoveable();
	void				Spawn();
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		SquishedByDoor(idEntity *door);
	virtual bool		Collide( const trace_t &collision, const idVec3 &velocity );
	virtual void		Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void				ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse );
	void				AllowImpact( bool allow );

	virtual void		Event_HoverTo( const idVec3& position );
	virtual void		Event_HoverMove( );
	virtual void		Event_Unhover( );
	void				Event_Touch( idEntity *other, trace_t *trace );
	void				Event_SpawnFxFlyLocal( const char* defName );

	// HUMANHEAD mdl:  For the keeper
	ID_INLINE void		SetDamageDef( const char *damageDef ) { damage = damageDef; }

protected:
	virtual idEntity*	ValidateEntity( const int collisionEntityNum );
	virtual float		DetermineCollisionSpeed( const idEntity* entity, const idVec3& point1, const idVec3& velocity1, const idVec3& point2, const idVec3& velocity2 );
	virtual s_channelType	DetermineNextChannel();

	virtual void		AttemptToPlayBounceSound( const trace_t &collision, const idVec3 &velocity );
	virtual void		Ticker( void );
	void				Event_StartFadingOut(float fadetime);

protected:
	bool				removeOnCollision;
	bool				notPushableAI;

	float				collisionSpeed_min;
	int					currentChannel;
	int					nextDamageTime;			// next time movable is allowed to cause collision damage

	hhBindController *	hoverController;
	idVec3				hoverPosition;	
	idVec3				hoverAngle;	

	idEntityPtr<idEntityFx> fxFly;
    idInterpolate<float>	fadeAlpha;
};


#endif
