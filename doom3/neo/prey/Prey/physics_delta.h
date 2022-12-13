#ifndef __PREY_PHYSICS_DELTA_H__
#define __PREY_PHYSICS_DELTA_H__



class hhPhysics_Delta : public idPhysics_Actor {

 public:
	CLASS_PROTOTYPE( hhPhysics_Delta );

						hhPhysics_Delta();

	void				SetOrigin( const idVec3 &newOrigin, int id = -1 );
	void				SetAxis( const idMat3 &newAxis, int id = -1 );


	bool				Evaluate( int timeStepMSec, int endTimeMSec );
	

	void				SetDelta( const idVec3 &d );

	void				Activate();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

 protected:
	void				Rest();

	idVec3				delta;

};

#endif /* __PREY_PHYSICS_DELTA_H__ */
