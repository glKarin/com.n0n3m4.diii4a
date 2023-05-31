#ifndef __PREY_GAME_ANIMDRIVEN_H__
#define __PREY_GAME_ANIMDRIVEN_H__

// Class name by Jimmy! :)
class hhAnimDriven : public hhAnimatedEntity {
 public:
	CLASS_PROTOTYPE( hhAnimDriven );


	void				Spawn();
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Think();

	void				SetPassenger( idEntity *newPassenger, bool orientFromPassenger = true );


	// Events
	void				Event_PlayCycle( int channel, const char *animName );


	bool				Collide( const trace_t &collision, const idVec3 &velocity );
	int					PlayAnim( int channel, const char *animName );
	bool				PlayCycle( int channel, const char *animName );
	void				ClearAllAnims();
	void				UpdateAnimation();
	bool				AllowCollision( const trace_t& collision );

 protected:
	hhPhysics_Delta		physicsAnim;

	idEntityPtr<idEntity>	passenger;

	int					spawnTime;
	bool				hadPassenger;
	idVec3				deltaScale;			//scale anim delta movement by this
};

#endif /* __PREY_GAME_ANIMDRIVEN_H__ */
