
#ifndef __GAME_POD_H__
#define __GAME_POD_H__

class hhPod : public hhMine {
public:
	CLASS_PROTOTYPE( hhPod );

					hhPod();
	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
	virtual bool	Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void			Release();	
	virtual bool	GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual void	Think( void );
	virtual void	ClientPredictionThink( void );

	virtual bool	Collide( const trace_t &collision, const idVec3 &velocity );
	virtual void	Event_HoverTo( const idVec3 &position );
	virtual void	Event_Unhover();
protected:
	void			RollThink( void );

private:
	float			radius;					// radius of barrel
	idVec3			lastOrigin;				// origin of the barrel the last frame
	idMat3			additionalAxis;			// transformation for visual model
	bool			bMoverThink;
};


#endif /* __GAME_POD_H__ */

