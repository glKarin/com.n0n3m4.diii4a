/*
================

AI_Util.h

================
*/

#ifndef __AI_UTIL__
#define __AI_UTIL__

const float AI_TETHER_MINRANGE			= 8.0f;

/*
===============================================================================
								rvAITrigger
===============================================================================
*/

class rvAITrigger : public idEntity {
public:
	CLASS_PROTOTYPE ( rvAITrigger );
	
	rvAITrigger ( void );

	void			Spawn					( void );
	void			Save					( idSaveGame *savefile ) const;
	void			Restore					( idRestoreGame *savefile );
	virtual void	Think					( void );

	virtual void	FindTargets				( void );

protected:

	idList< idEntityPtr<idAI> >			testAI;
	idList< idEntityPtr<rvSpawner> >	testSpawner;
	
	bool								conditionDead;
	bool								conditionTether;
	bool								conditionStop;

	int									wait;
	int									nextTriggerTime;
	
	float								percent;

private:

	void			Event_Activate			( idEntity* activator );
	void			Event_PostRestore		( void );

	void			Event_AppendFromSpawner	( rvSpawner* spawner, idEntity* spawned );
};

/*
===============================================================================
								rvAITether
===============================================================================
*/

class rvAITether : public idEntity {
public:
	CLASS_PROTOTYPE ( rvAITether );
	
	rvAITether ( void );
	
	void			Spawn						( void );
	void			Save						( idSaveGame *savefile ) const;
	void			Restore						( idRestoreGame *savefile );
	void			InitNonPersistentSpawnArgs	( void );	
	
	virtual bool	ValidateAAS				( idAI* ai );	
	virtual bool	ValidateDestination		( idAI* ai, const idVec3& dest );
	virtual bool	ValidateBounds			( const idBounds& bounds );

	virtual bool	FindGoal				( idAI* ai, aasGoal_t& goal );
	virtual float	GetOriginReachedRange	( void ) {return AI_TETHER_MINRANGE;}
	
	virtual void	DebugDraw				( void );

	bool			CanBreak				( void ) const;
	bool			IsAutoBreak				( void ) const;

	idList<int>		areaNum;

	bool			IsWalkForced			( void ) const;
	bool			IsRunForced				( void ) const;

protected:

	idEntityPtr<idLocationEntity>	location;

	struct tetherFlags_s {
		bool		canBreak			:1;			// Temporarily break when enemy is within tether
		bool		autoBreak			:1;			// Break when the ai gets within the tether
		bool		forceRun			:1;			// Alwasy run when heading towards tether
 		bool		forceWalk			:1;			// Alwasy walk when heading towards tether
		bool		becomeAggressive	:1;			// 
		bool		becomePassive		:1;
	} tfl;
	
private:

	void			Event_Activate				( idEntity* activator );
	void			Event_TetherSetupLocation	( void );
	void			Event_TetherGetLocation		( void );
};

ID_INLINE bool rvAITether::CanBreak ( void ) const {
	return tfl.canBreak;
}

ID_INLINE bool rvAITether::IsWalkForced ( void ) const {
	return tfl.forceWalk;
}

ID_INLINE bool rvAITether::IsRunForced ( void ) const {
	return tfl.forceRun;
}

ID_INLINE bool rvAITether::IsAutoBreak ( void ) const {
	return tfl.autoBreak;
}

/*
===============================================================================
								rvAITetherBehind
===============================================================================
*/

class rvAITetherBehind : public rvAITether {
public:
	CLASS_PROTOTYPE ( rvAITetherBehind );
	
					rvAITetherBehind( void ) { range = 0.0f; }

	void			Spawn						( void );
	void			Save						( idSaveGame *savefile ) const { }
	void			Restore						( idRestoreGame *savefile );
	void			InitNonPersistentSpawnArgs	( void );	

	virtual bool	ValidateDestination			( idAI* ai, const idVec3& dest );				
	virtual bool	ValidateBounds				( const idBounds& bounds );
	virtual void	DebugDraw					( void );

protected:

	float	range;
};

/*
===============================================================================
								rvAITetherRadius
===============================================================================
*/

class rvAITetherRadius : public rvAITether {
public:
	CLASS_PROTOTYPE ( rvAITetherRadius );

					rvAITetherRadius( void ) { radiusSqr = 0.0f; }

	void			Spawn						( void );
	void			Save						( idSaveGame *savefile ) const { }
	void			Restore						( idRestoreGame *savefile );
	void			InitNonPersistentSpawnArgs	( void );	

	virtual bool	ValidateDestination			( idAI* ai, const idVec3& dest );	
	virtual bool	ValidateBounds				( const idBounds& bounds );
	virtual void	DebugDraw					( void );

	/*
	virtual float	GetOriginReachedRange		( void ) 
	{
		float rad = sqrt(radiusSqr);
		float halfRad = rad/2.0f;
		if ( rad < AI_TETHER_MINRANGE )
		{
			return rad;
		}
		return (halfRad<AI_TETHER_MINRANGE)?AI_TETHER_MINRANGE:halfRad;
	}
	*/
	
protected:

	float	radiusSqr;
};

/*
===============================================================================
								rvAITetherRadius
===============================================================================
*/

class rvAITetherClear : public rvAITether {
public:
	CLASS_PROTOTYPE ( rvAITetherClear );
};

/*
===============================================================================
								rvAIBecomePassive
===============================================================================
*/

class rvAIBecomePassive : public idEntity {
public:
	CLASS_PROTOTYPE ( rvAIBecomePassive );

private:

	void			Event_Activate			( idEntity* activator );
};

/*
===============================================================================
								rvAIBecomeAggressive
===============================================================================
*/

class rvAIBecomeAggressive : public idEntity {
public:
	CLASS_PROTOTYPE ( rvAIBecomeAggressive );

private:

	void			Event_Activate			( idEntity* activator );
};

#endif // __AI_UTIL__

