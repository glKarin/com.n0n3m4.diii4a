#ifndef __HH_PHYSICS_PLAYER_H
#define __HH_PHYSICS_PLAYER_H

/**********************************************************************

hhPhysics_Player

**********************************************************************/
const int c_iNumWallwalkTraces = 5;

class hhPhysics_Player: public idPhysics_Player
{
	CLASS_PROTOTYPE( hhPhysics_Player );

	public:
							hhPhysics_Player( void );

		virtual bool		Evaluate( int timeStepMSec, int endTimeMSec );

		virtual void		SetSelf( idEntity *e );

		virtual void		SaveState( void );
		virtual	void		RestoreState( void );

		void				WriteToSnapshot( idBitMsgDelta &msg, bool inVehicle ) const;
		void				ReadFromSnapshot( const idBitMsgDelta &msg, bool inVehicle );

		bool				IsWallWalking() const { return isWallWalking; }
		bool				WasWallWalking() const { return wasWallWalking; }
		void				IsWallWalking( bool wallwalking ) { isWallWalking = wallwalking; }
		void				WasWallWalking( bool wallwalking ) { wasWallWalking = wallwalking; }
		bool				AlignedWithGravity() const;
		bool				ShouldSlopeCheck() const	{ return bDoSlopeCheck && !isWallWalking; }
		void				SetSlopeCheck( bool on )		{ bDoSlopeCheck = on; }
		void				SetInwardGravity(int inwardGravity); //rww
		bool				IsInwardGravity() const	{ return iInwardGravity > 0; } //rww

		virtual void		ShouldRemainAlignedToAxial( const bool remainAligned );
		virtual bool		ShouldRemainAlignedToAxial() const;
		virtual void		OrientToGravity( bool orientToGravity );
		virtual bool		OrientToGravity() const;


		virtual void		Translate( const idVec3 &translation, int id = 0 );
		virtual	void		Rotate( const idRotation& rotation, int id = 0 );
		virtual const idVec3&	GetOrigin( int id = 0 ) const;
		virtual const idMat3&	GetAxis( int id = 0 ) const;
		virtual void		SetOrigin( const idVec3 &newOrigin, int id = 0 );
		virtual void		SetAxis( const idMat3 &newAxis, int id = 0 );
		virtual void		ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const;
		virtual void		ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const;
		virtual bool		IsGroundEntity( int entityNum ) const;
		virtual bool		IsGroundClipModel( int entityNum, int id = 0 ) const;
		virtual bool		HasGroundContacts( void ) const;
		virtual bool		ExtraGroundCheck(idClipModel *customClipModel = NULL); //HUMANHEAD rww
		virtual void		AddGroundContacts( const idClipModel *clipModel );
		virtual	bool		EvaluateContacts( void );
		virtual void		SetKnockBack( const int knockBackTime );

		virtual	void		SetPushed( int deltaTime );

		virtual void		ForceMovedFrame(bool forceMoved) { bMoveNextFrame = forceMoved; } //HUMANHEAD rww

		virtual void		ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );

		virtual bool		WallWalkIsAllowed() const;
		void				CheckWallWalk( bool bForce = false );
		virtual void		CheckGround();

		virtual const idMaterial*	GetGroundMaterial() const { return groundMaterial; }

		playerPState_t&		GetPlayerState() { return current; }

		virtual void		ForceCrouching();

		void				Save( idSaveGame *savefile ) const;
		void				Restore( idRestoreGame *savefile );

	protected:
		idVec3				ClipModelRotationOrigin;
		bool				shouldRemainAlignedToAxial;
		bool				orientToGravity;
		bool				isWallWalking;
		bool				wasWallWalking;
		idVec3				oldOrigin;
		idMat3				oldAxis;
		bool				bDoSlopeCheck;
		int					iInwardGravity; //rww - number of inward gravity zones player is in

		//HUMANHEAD PCF rww 05/11/06 - hack fix for stuck bugs
		int					stuckCount;
		idVec3				stuckOrigin;
		//HUMANHEAD END

		hhCameraInterpolator*	camera;
		hhPlayer*			castSelf;
		idEntity*			castSelfGeneric; //rww

		idVec3				wallwalkTraceOriginTable[ c_iNumWallwalkTraces ];
		bool				bMoveNextFrame; //HUMANHEAD rww

	protected:
		void				EvaluateOwnerCamera( const int timeStep );
		void				SetOwnerCameraTarget( const idVec3& Origin, const idMat3& Axis, int iInterpFlags );
        void				LinkClip( const idVec3& Origin, const idMat3& Axis );
		virtual idVec3		DetermineJumpVelocity();
		void				BuildWallwalkTraceOriginTable( const idMat3& Axis );
		void				PerformGroundTrace( trace_t& TraceInfo, const idVec3& Start );
	
		bool				EvaluateGroundTrace( const trace_t& TraceInfo );

		virtual bool		IterativeRotateMove( const idVec3& UpVector, const idVec3& IdealUpVector, const idVec3& RotationOrigin, const idVec3& RotationCheckOrigin, int iNumIterations );
		bool				RotateMove( const idVec3& UpVector, const idVec3& IdealUpVector, const idVec3& RotationOrigin, const idVec3& RotationCheckOrigin );
		void				TranslateMove( const idVec3& TranslationVector, const float fDist );
		
		virtual	void		WalkMove();
		virtual void		AirMove();

		float				GetContactEpsilon(void) const;
};

#endif