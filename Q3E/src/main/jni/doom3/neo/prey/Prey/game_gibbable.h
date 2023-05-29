#ifndef __PREY_GIBBABLE_H
#define __PREY_GIBBABLE_H

class hhGibbable : public hhAnimatedEntity {
public:
	CLASS_PROTOTYPE( hhGibbable );

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
	void			Explode(idEntity *activator);
	virtual bool	Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void	Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void	SetModel( const char *modelname );

protected:
	void			Event_Activate( idEntity *activator );
	void			Event_PlayIdle( void );
	void			Event_Respawn( void );
	int				DetermineThinnestAxis();

protected:
	int				idleAnim;
	int				painAnim;
	
	int				idleChannel;
	int				painChannel;
	bool			bVertexColorFade;
};

#endif
