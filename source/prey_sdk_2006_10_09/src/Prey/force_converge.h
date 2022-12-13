#ifndef __FORCE_CONVERGE_H__
#define __FORCE_CONVERGE_H__

//===============================================================================
//
//	Convergent force
//
//===============================================================================

class hhForce_Converge : public idForce {
	CLASS_PROTOTYPE( hhForce_Converge );

public:
						hhForce_Converge( void );
	virtual				~hhForce_Converge( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	void				SetTarget(idVec3 &newTarget);
	void				SetEntity(idEntity *entity, int id=0, const idVec3 &point=vec3_origin);
	idEntity *			GetEntity() const				{ return ent.GetEntity();				}
	void				SetRestoreTime( float time );
	void				SetRestoreFactor( float factor ){ restoreFactor = factor;	}
	void				SetRestoreSlack(float slack) { restoreForceSlack = slack; }

public: // common force interface
	virtual void		Evaluate( int time );
	virtual void		RemovePhysics( const idPhysics *phys );
	void				SetAxisEntity(idEntity *entity);
	void			    SetShuttle(bool shuttle) { bShuttle = shuttle; } 

private:
	idEntityPtr<idEntity>	ent;			// Entity to apply forces to
	idEntityPtr<idEntity>   axisEnt;
	idPhysics *			physics;			// physics object of entity during last evaluate
	idVec3				target;				// Point of convergence
	idVec3				offset;				// Entity local offset for force application
	int					bodyID;				// Body ID for force application
	float				restoreTime;		// Ideal convergence time
	float				restoreFactor;		// Spring constant for restorative force
	float				restoreForceSlack;	//rww - allow a linear buildup based on the distance of the target point
	bool				bShuttle;			//mdl:  Use an alternate spring equation for shuttle tractor beam
};

#endif
