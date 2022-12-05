//----------------------------------------------------------------
// VehicleDriver.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

extern const idEventDef VD_ChoosePathTarget;

class rvVehicleDriver : public idActor {
	friend class rvVehicleAI;
	friend class rvVehicleMonster;
public:

	CLASS_PROTOTYPE( rvVehicleDriver );

							rvVehicleDriver				( void );

	void					Think						( void );
	void					Spawn						( void );

	static bool				IsValidTarget				( const idEntity* ent );			// valid entity for looking or facing
	static bool				IsValidPathNode				( const idEntity* ent );			// valid entity for pathing
	static bool				IsValidLeader				( const idEntity* ent );			// valid entity to set as a leader

	static int				SortValid					( const void* a, const void* b );

	virtual void			Present						( void ) { }

protected:
	void					SimulateKeys				( usercmd_t& cmd, float dotForward, float dotRight, float speed, float distance = 0 );
	void					SimulateButtons				( usercmd_t& cmd );
	void					SimulateMouseMove			( usercmd_t& cmd );

	bool					GetTargetInfo				( const idEntity* target, idVec3* targetOrigin, idVec3* dirToTarget, 
														  float* dotForward, float* dotRight, float* distance ) const;
	idEntity*				ChooseNextNode				( idEntity* target );

	bool					IsMoving					( void ) const { return isMoving && IsDriving();		}
	bool					IsDriving					( void ) const { return vehicleController.IsDriving();	}

	bool					SetLeader					( idEntity* newLeader );
	const idEntity* 		GetLeader					( void ) const	{ return leader;		}
	void					SetLeaderHint				( int flags )	{ leaderFlags = flags;	}
	int						GetLeaderHint				( void ) const	{ return leaderFlags;	}

	int						SortTargetList				( idEntity* ent ) const;
	idEntity*				RandomValidTarget			( idEntity* ent ) const;
	int						NumValidTargets				( idEntity* ent ) const;
	int						NumTargets					( const idEntity* ent ) const;
	idEntity*				GetTarget					( const idEntity* ent, int index ) const;

	void					UpdateAutoCorrection		( void );

	void					Save						( idSaveGame *savefile ) const;
	void					Restore						( idRestoreGame *savefile );

	enum VD_PathingMode {
		VDPM_Random		= 0,
		VDPM_MoveTo		= 1,
		VDPM_MoveAway	= 2,
		VDPM_Custom		= 4,
	};


	void					SetPathingMode				( VD_PathingMode mode, const idVec3 & origin );
//	CustomPathingFunc		SetPathingMode				( VD_PathingMode mode, CustomPathingFunc cb );
	idEntity *				SetPathingMode				( VD_PathingMode mode, idEntity * );
	VD_PathingMode 			GetPathingMode				( void ) const;
	const idVec3 & 			GetPathingOrigin			( void ) const;
//	CustomPathingFunc		GetPathingCustomCallback	( void ) const;
	idEntity *				GetPathingEntity			( void ) const;

protected:
	void					Event_PostSpawn				( void );
	void					Event_EnterVehicle			( idEntity * vehicle );
	void					Event_ExitVehicle			( bool force );
	void					Event_ScriptedMove			( idEntity *target, float minDist, bool exitVehicle );
	void					Event_ScriptedDone			( void );
	void					Event_ScriptedStop			( void );
	void					Event_Trigger				( idEntity *activator );
	void					Event_SetSpeed				( float speed );
	void					Event_FireWeapon			( float weapon_index, float time );
	void					Event_FaceEntity			( const idEntity* entity );
	void					Event_LookAt				( const idEntity* entity );
	void					Event_SetLeader				( idEntity* newLeader );

//twhitaker: remove - begin
	void					Event_SetFollowOffset		( const idVec3 &offset );
//twhitaker: remove - end

	// Leader hints : a driver will send the following driving hints to it's leader
	enum VD_LeaderHints {
		VDLH_SlowDown	= 1,							// Set when distance is too great
		VDLH_Wait		= 2,							// Set when there is no line of site
		VDLH_Continue	= 0,							// Default following behavior
	};

	idStr & LeaderHintsString( int hints, idStr & out ) const {
		switch ( hints ) {
			case VDLH_SlowDown:		out = "SlowDown";	break;
			case VDLH_Wait:			out = "Wait";		break;
			default:				out = "Continue";	break;
		}
		return out;
	}

	// Stores information about a path target
	struct PathTargetInfo {
		idEntityPtr<idEntity>		node;				// the target entity
		float						initialDistance;	// the distance to this entity when we first decide to move there
		float						minDistance;		// the range that specifies when this entity has been reached
		float						throttle;			// 0 -> 1 speed setting
		bool						exitVehicle;		// set to true to exit a vehicle when we reach this target
	};

	struct PathNavigationData {
		float						dotForward;
		float						dotRight;
	};

	// standard movement
	PathTargetInfo					pathTargetInfo;
	PathTargetInfo					lastPathTargetInfo;
	float							currentThrottle;

	// facing
	idEntityPtr<const idEntity>		faceTarget;
	idEntityPtr<const idEntity>		lookTarget;

	// following
	idEntityPtr<rvVehicleDriver>	leader;				//TODO: twhitaker: see if I can make this an rvVehicle (so entities can follow player)
	int								leaderFlags;
	float							decelDistance;
	float							minDistance;
	bool							avoidingLeader;

	// firing
	float							fireEndTime;

	// event callbacks
	rvScriptFuncUtility				func;

	// state
	bool							isMoving;

	VD_PathingMode					pathingMode;
//	idEntity *						(* pathingCallback)( idEntity * );
	idEntityPtr<idEntity>			pathingEntity;
	idVec3							pathingOrigin;

	float							forwardThrustScale;
	float							rightThrustScale;
};


ID_INLINE void rvVehicleDriver::SetPathingMode( VD_PathingMode mode, const idVec3 & origin ) {
	if ( mode != VDPM_Custom ) {
		pathingMode		= mode;
		pathingOrigin	= origin;
	}
}

ID_INLINE idEntity * rvVehicleDriver::SetPathingMode( VD_PathingMode mode, idEntity * entity ) {
	if ( mode != VDPM_Custom ) {
		return NULL;
	}

	pathingMode			= mode;
	idEntity * prev		= pathingEntity;
	pathingEntity		= entity;
	return prev;
}

ID_INLINE rvVehicleDriver::VD_PathingMode rvVehicleDriver::GetPathingMode ( void ) const {
	return pathingMode;
}

ID_INLINE const idVec3 & rvVehicleDriver::GetPathingOrigin ( void ) const {
	return pathingOrigin;
}

ID_INLINE idEntity * rvVehicleDriver::GetPathingEntity ( void ) const {
	return pathingEntity;
}

