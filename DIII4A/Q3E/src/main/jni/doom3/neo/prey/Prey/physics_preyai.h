#ifndef __HH_PHYSICS_AI
#define __HH_PHYSICS_AI

class hhPhysics_AI : public idPhysics_Monster {
	CLASS_PROTOTYPE( hhPhysics_AI );

	public:
										hhPhysics_AI();

		bool							Evaluate( int timeStepMSec, int endTimeMSec );
		const idVec3&					GetLinearVelocity( int id = 0 ) const;
		virtual void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );
		void							AddForce( const int id, const idVec3 &point, const idVec3 &force );
		void							ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );
		idVec3							GetLocalOrigin( void ) { return current.localOrigin; }

		idEntity*						GetLastMoveTouch(void)	{return lastMoveTouch.GetEntity();}
		
		void							EnableGravity(bool tf = true)	{useGravity = tf;}
		bool							IsGravityEnabled(void)	const	{return useGravity;}
		void							SetMaster( idEntity *master, const bool orientated );
		void							SetGravity( const idVec3 &newGravity );
		void							SetGravClipModelAxis( bool enable ) { bGravClipModelAxis = enable; }

		void							Save( idSaveGame *savefile ) const;
		void							Restore( idRestoreGame *savefile );

	protected:
		idVec3							ApplyFriction( const idVec3& vel, const float deltaTime );
		virtual monsterMoveResult_t		FlyMove( idVec3 &start, idVec3 &velocity, const idVec3 &delta );
		virtual monsterMoveResult_t		SlideMove( idVec3 &start, idVec3 &velocity, const idVec3 &delta, idList<int> *touched = NULL );

	// HUMANHEAD nla - Constants 
	public:
		static const int				NO_FLY_DIRECTION;
		idMat3							localAxis;

	protected:
		int								flyStepDirection;	// Direction we were last stepping in
		idEntityPtr<idEntity>			lastMoveTouch;		// Entity we touched on last slidemove
		bool							useGravity;
		bool							bGravClipModelAxis;	// Set clipmodel axis for gravity changes
};

#endif