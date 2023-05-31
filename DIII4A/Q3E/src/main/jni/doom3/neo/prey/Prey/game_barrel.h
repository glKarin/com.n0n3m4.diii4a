#ifndef __HH_BARREL_H
#define __HH_BARREL_H

class hhBarrel : public hhMoveable {

public:
	CLASS_PROTOTYPE( hhBarrel );
							hhBarrel();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					BarrelThink( void );
	virtual void			Think( void );
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual void			ClientPredictionThink( void );

private:
	float					radius;					// radius of barrel
	int						barrelAxis;				// one of the coordinate axes the barrel cylinder is parallel to
	idVec3					lastOrigin;				// origin of the barrel the last think frame
	idMat3					lastAxis;				// axis of the barrel the last think frame
	float					additionalRotation;		// additional rotation of the barrel about it's axis
	idMat3					additionalAxis;			// additional rotation axis
};


#endif