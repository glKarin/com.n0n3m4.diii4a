// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_ROLES_OBJECTIVEMANAGER_H__
#define __GAME_ROLES_OBJECTIVEMANAGER_H__

#include "../../game/botai/BotAI_Actions.h" //mal: gonna need this for the bot actions

class idScriptObject;
class sdDeclLocStr;

class sdObjectiveObject {
public:
	typedef struct teamInfo_s {
		idStr											iconName;
		const sdDeclLocStr*								description;
		int												state;
	} teamInfo_t;

public:
														sdObjectiveObject( void );

	void												Init( int numTeams );
	void												SetState( int teamIndex, int state );
	void												SetIcon( int teamIndex, const char* icon );
	int													GetState( int teamIndex ) const;
	void												SetDescription( int teamIndex, const sdDeclLocStr* description );
	int													GetDescription( int teamIndex ) const;

	const teamInfo_t&									GetInfo( int teamIndex ) const { return teamInfo[ teamIndex ]; }

private:
	idList< teamInfo_t >								teamInfo;
};

class sdObjectiveManagerLocal : public idClass {
public:
	static const int									MAX_OBJECTIVES = 8;

public:
	CLASS_PROTOTYPE( sdObjectiveManagerLocal );

														sdObjectiveManagerLocal( void );

	void												Init( void );
	void												Shutdown( void );

	static void											GetRealTime( idStr& text );
	void												LogPlayerStats( void );

	void												OnScriptChange( void );
	void												OnNewScriptLoad( void );
	void												OnMapStart( void );
	void												OnMapShutdown( void );
	void												OnGameStateChange( int newState );
	void												OnLocalMapRestart( void );

	void												OnLocalViewPlayerTeamChanged( void );
	void												OnLocalViewPlayerChanged( void );

	void												Think( void );

	virtual idScriptObject*								GetScriptObject( void ) const { return scriptObject; }

	const sdObjectiveObject*							GetObjective( int index ) const { return &objectives[ index ]; }
	int													GetNumObjectives( void ) const { return objectives.Num(); }
	int													GetNextObjective( int teamIndex ) const { return nextObjective[ teamIndex ]; }

	void												WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) const;

	void												SetNextObjective( sdTeamInfo* team, int objectiveIndex );

	void												Event_SetObjectiveState( idScriptObject* object, int objectiveIndex, int state );
	void												Event_SetObjectiveIcon( idScriptObject* object, int objectiveIndex, const char* icon );
	void												Event_GetObjectiveState( idScriptObject* object, int objectiveIndex );
	void												Event_SetNextObjective( idScriptObject* object, int objectiveIndex );
	void												Event_GetNextObjective( idScriptObject* object );
	void												Event_SetShortDescription( idScriptObject* object, int objectiveIndex, int description );
	void												Event_GetShortDescription( idScriptObject* object, int objectiveIndex );
	void												Event_CreateMapScript( void );
	void												Event_LogObjectiveCompletion( int objectiveIndex );

	void												Event_DeactivateBotActionGroup( int actionGroupNum );
	void												Event_ActivateBotActionGroup( int actionGroupNum );
	void												Event_SetBotActionStateForEvent( const botActionStates_t state, idEntity* triggerEntity );
	void												Event_DeactivateBotAction( const char *actionName );
	void												Event_ActivateBotAction( const char *actionName );
	void												Event_SetBotCriticalClass( const playerTeamTypes_t playerTeam, const playerClassTypes_t criticalClass );
	void												Event_NotifyBotOfEvent( const playerTeamTypes_t playerTeam, const playerClassTypes_t playerClass, const botActionStates_t eventType );
	void												Event_SetAttackingTeam( const playerTeamTypes_t playerTeam );
	void												Event_SetPrimaryAction( const playerTeamTypes_t playerTeam, const char *actionName );
	void												Event_SetSecondaryAction( const playerTeamTypes_t playerTeam, const char *actionName );
	void												Event_SetTeamNeededClass( const playerTeamTypes_t playerTeam, const playerClassTypes_t neededClass, const playerClassTypes_t sampleClass, int minNeeded, bool priority, bool storeRequest );
	void												Event_SetBotSightDist( float sightDist );
	void												Event_EnableRouteGroup( int routeGroupNum );
	void												Event_DisableRouteGroup( int routeGroupNum );
	void												Event_DisableRoute( const char* routeName );
	void												Event_EnableRoute( const char* routeName );
	void												Event_SetMapHasMCPGoal( bool hasMCPGoal );
	void												Event_SetSpawnActionOwner( const playerTeamTypes_t playerTeam, idEntity* triggerEntity );
	void												Event_SetActionObjState( const botActionStates_t state, const playerTeamTypes_t ownerTeam, idEntity* carryableObjective, idEntity* carrier );
	void												Event_DisableNodes( const char* nodeName );
	void												Event_EnableNodes( const char* nodeName );
	void												Event_TeamSuicideIfNotNearAction( const char* actionName, float requiredDist, const playerTeamTypes_t playerTeam );
	void												Event_IsActionGroupActive( int actionGroupNum );
	void												Event_IsActionActive( const char* actionName );
	void												Event_SwitchTeamWeapons( const playerTeamTypes_t playerTeam, const playerClassTypes_t playerClass, const playerWeaponTypes_t curWeapType, const playerWeaponTypes_t desiredWeapType, bool randomly );
	void												Event_KillBotActionGroup( int actionGroupNum );
	void												Event_KillBotAction( const char *actionName );
	void												Event_SetTeamUseRearSpawn( const playerTeamTypes_t playerTeam, bool useSpawn );
	void												Event_GetNumBotsOnTeam( const playerTeamTypes_t playerTeam );
	void												Event_SetActionPriority( const char *actionName, bool isPriority );
	void												Event_SetTeamAttacksDeployables( const playerTeamTypes_t playerTeam, bool attackDeployables );
	void												Event_SetActionHumanGoal( const char* actionName, const botActionGoals_t humanGoal );
	void												Event_SetActionStroggGoal( const char* actionName, const botActionGoals_t stroggGoal );
	void												Event_ClearBotBoundEntities( const playerTeamTypes_t playerTeam );
	void												Event_SetBotTeamRetreatTime( const playerTeamTypes_t playerTeam, int retreatTime );
	void												Event_SetTeamUseRearSpawnPercentage( const playerTeamTypes_t playerTeam, int percentageUsed );
	void												Event_SetNodeTeam( const char* nodeName, const playerTeamTypes_t playerTeam );
	void												Event_SetTeamMinePlantIsPriority( const playerTeamTypes_t playerTeam, bool isPriority );
	void												Event_SetBotActionVehicleType( const char* actionName, int actionVehicleFlags );
	void												Event_SetBotActionGroupVehicleType( int actionGroupNum, int actionVehicleFlags );
	void												Event_DisableAASAreaInLocation( int aasType, const idVec3& location );
	void												Event_GameIsOnFinalObjective();
	void												Event_MapIsTrainingMap();
	void												Event_SetPlayerIsOnFinalMission();
	void												Event_SetActorPrimaryAction( const char* actionName, bool isDeployableMission, int goalPauseTime );
	void												Event_SetBriefingPauseTime( int pauseTime );
	void												Event_GetBotCriticalClass( const playerTeamTypes_t playerTeam );
	void												Event_GetNumClassPlayers( const playerTeamTypes_t playerTeam, const playerClassTypes_t playerClass );
	void												PlayerDeployableDeployed();
	void												Event_SetTrainingBotsCanGrabForwardSpawns();

	idEntity*											GetSpectateEntity( void );

	void												Event_SendNetworkEvent( int clientIndex, bool isRepeater, const char* message );
	void												OnNetworkEvent( const char* message );

	void												SetCriticalClass( const playerTeamTypes_t playerTeam, const playerClassTypes_t criticalClass );

private:
	const sdDeclPlayerClass*							GetPlayerClass( const playerTeamTypes_t playerTeam, const playerClassTypes_t playerClass );


	idScriptObject*										scriptObject;

	idStaticList< sdObjectiveObject, MAX_OBJECTIVES >	objectives;
	idList< int >										nextObjective;
	playerClassTypes_t									gdfCriticalClass;
	playerClassTypes_t									stroggCriticalClass;

	const sdProgram::sdFunction*						onCarryableItemStolenFunc;
	const sdProgram::sdFunction*						onCarryableItemReturnedFunc;
	const sdProgram::sdFunction*						onSpawnCapturedFunc;
	const sdProgram::sdFunction*						onSpawnLiberatedFunc;
	const sdProgram::sdFunction*						getSpectateEntityFunc;
	const sdProgram::sdFunction*						onDeployableDeployedFunc;
};

typedef sdSingleton< sdObjectiveManagerLocal > sdObjectiveManager;

#endif // __GAME_ROLES_OBJECTIVEMANAGER_H__
