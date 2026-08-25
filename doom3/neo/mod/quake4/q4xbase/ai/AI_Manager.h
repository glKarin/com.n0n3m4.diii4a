#ifndef __AI_MANAGER_H__
#define __AI_MANAGER_H__

typedef enum {
	AITEAM_MARINE,
	AITEAM_STROGG,
	AITEAM_NUM
} aiTeam_t;

typedef enum {
	AITEAMTIMER_ANNOUNCE_TACTICAL,					// Tactical change
	AITEAMTIMER_ANNOUNCE_SUPPRESSING,
	AITEAMTIMER_ANNOUNCE_SUPPRESSED,				
	AITEAMTIMER_ANNOUNCE_FRIENDLYFIRE,				// Shot by a teammate
	AITEAMTIMER_ANNOUNCE_ENEMYSTEATH,	
	AITEAMTIMER_ANNOUNCE_NEWENEMY,					// New enemy was aquired
	AITEAMTIMER_ANNOUNCE_SNIPER,					// Sniper sighted
	AITEAMTIMER_ANNOUNCE_CANIHELPYOU,				// Player standing in front of a friendly too long
	AITEAMTIMER_ANNOUNCE_SIGHT,						// First time seeing an enemy
	AITEAMTIMER_ACTION_RELAX,						// Play relax animation
	AITEAMTIMER_ACTION_PEEK,						// Play peek animation
	AITEAMTIMER_ACTION_TALK,						// Able to talk to another person yet?
	AITEAMTIMER_MAX
} aiTeamTimer_t;

extern idVec4 aiTeamColor[AITEAM_NUM];

/*
=====================
blockedReach_t
=====================
*/
// cdr: Alternate Routes Bug
typedef struct aiBlocked_s {
	idAAS*							aas;
	idReachability*					reach;
	int								time;
	idList< idEntityPtr<idEntity> >	blockers;
	idList< idVec3 >				positions;
} aiBlocked_t;

/*
=====================
aiAvoid_t
=====================
*/
typedef struct aiAvoid_s {
	idVec3							origin;
	float							radius;
	int								team;
} aiAvoid_t;

/*
===============================================================================

rvAIHelper

===============================================================================
*/

class rvAIHelper : public idEntity {
public:
	CLASS_PROTOTYPE( rvAIHelper );

	rvAIHelper ( void );

	idLinkList<rvAIHelper>	helperNode;

	void			Spawn					( void );

	virtual bool	IsCombat				( void ) const;
	virtual bool	ValidateDestination		( const idAI* ent, const idVec3& dest ) const;
	
	idVec3			GetDirection			( const idAI* ent ) const;

protected:

	virtual void	OnActivate				( bool active );
	
private:

	void			Event_Activate			( idEntity *activator );
};

/*
===============================================================================

rvAIManager

===============================================================================
*/

class rvAIManager {
public:

	rvAIManager ( void );
	~rvAIManager ( void ) {}

	/*
	===============================================================================
									General
	===============================================================================
	*/

	void				RunFrame				( void );

	void				Save					( idSaveGame *savefile ) const;
	void				Restore					( idRestoreGame *savefile );

	void				Clear					( void );

	bool				IsActive				( void );
	bool				IsSimpleThink			( idAI* ai );

	/*
	===============================================================================
									Navigation
	===============================================================================
	*/

	void				UnMarkAllReachBlocked	( void );
	void				ReMarkAllReachBlocked	( void );
	void				MarkReachBlocked		( idAAS* aas, idReachability* reach, const idList<idEntity*>& blockers);
	bool				ValidateDestination		( idAI* ignore, const idVec3& dest, bool skipCurrent = false, idActor* skipActor = NULL ) const;
	void				AddAvoid				( const idVec3& origin, float range, int team );

	/*
	===============================================================================
									Helpers
	===============================================================================
	*/

	void				RegisterHelper			( rvAIHelper* helper );
	void				UnregisterHelper		( rvAIHelper* helper );
	rvAIHelper*			FindClosestHelper		( const idVec3& origin );

	/*
	===============================================================================
									Team Management
	===============================================================================
	*/

	void				AddTeammate				( idActor* ent );
	void				RemoveTeammate			( idActor* ent );

	idActor*			GetAllyTeam				( aiTeam_t team );
	idActor*			GetEnemyTeam			( aiTeam_t team );	

	idActor*			NearestTeammateToPoint	( idActor* from, idVec3 point, bool nonPlayer = false, float maxRange = 1000.0f, bool checkFOV = false, bool checkLOS = false );
	idEntity*			NearestTeammateEnemy	( idActor* from, float maxRange=1000.0f, bool checkFOV = false, bool checkLOS = false, idActor** ally = NULL );
	bool				LocalTeamHasEnemies		( idAI* self, float maxBuddyRange=640.0f, float maxEnemyRange=1024.0f, bool checkPVS=false );
	bool				ActorIsBehindActor		( idActor* ambusher, idActor* victim );

	/*
	===============================================================================
									Team Timers
	===============================================================================
	*/

	bool				CheckTeamTimer			( int team, aiTeamTimer_t timer );
	void				ClearTeamTimer			( int team, aiTeamTimer_t timer );
	void				SetTeamTimer			( int team, aiTeamTimer_t timer, int delay );

	/*
	===============================================================================
									Announcements
	===============================================================================
	*/

	void				AnnounceKill			( idAI* victim, idEntity* attacker, idEntity* inflictor );
	void				AnnounceDeath			( idAI* victim, idEntity* attacker );

	/*
	===============================================================================
									Reactions
	===============================================================================
	*/

	void				ReactToPlayerAttack		( idPlayer* player, const idVec3 &origOrigin, const idVec3 &origDir );

	/*
	===============================================================================
									Debugging
	===============================================================================
	*/

	idTimer				timerFindEnemy;
	idTimer				timerTactical;
	idTimer				timerMove;
	idTimer				timerThink;	
	
	int					thinkCount;
	int					simpleThinkCount;

protected:

	void				UpdateHelpers			( void );

	void				DebugDraw				( void );
	void				DebugDrawHelpers		( void );

	idList<aiBlocked_t>		blockedReaches;
	idLinkList<idAI>		simpleThink;	
	idLinkList<rvAIHelper>	helpers;

	idLinkList<idActor>		teams[AITEAM_NUM];
	int						teamTimers[AITEAM_NUM][AITEAMTIMER_MAX];
	
	idList<aiAvoid_t>		avoids;
};

ID_INLINE bool rvAIManager::CheckTeamTimer ( int team, aiTeamTimer_t timer ) {
	return gameLocal.time >= teamTimers[team][timer];
}

ID_INLINE void rvAIManager::ClearTeamTimer ( int team, aiTeamTimer_t timer ) {
	teamTimers[team][timer] = 0;
}

ID_INLINE void rvAIManager::SetTeamTimer ( int team, aiTeamTimer_t timer, int delay ) {
	teamTimers[team][timer] = gameLocal.time + delay;
}

ID_INLINE void rvAIManager::AddAvoid ( const idVec3& origin, float radius, int team ) {
	if ( !IsActive ( ) ) {
		return;
	}
	aiAvoid_t& a = avoids.Alloc ( );
	a.origin = origin;
	a.radius = radius;
	a.team   = team;
}

extern rvAIManager aiManager;

#endif // __AI_MANAGER_H__
