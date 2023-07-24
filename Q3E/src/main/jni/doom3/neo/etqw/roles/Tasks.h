// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_ROLES_TASKS_H__
#define __GAME_ROLES_TASKS_H__

#include "../misc/RenderEntityBundle.h"
#include "../decls/DeclPlayerTask.h"

class sdFireTeam;
class sdDeclPlayerTask;
class sdWayPoint;
class sdRenderEntityBundle;

typedef sdHandle< int, -1 >											taskHandle_t;

class sdPlayerTask : public idClass {
public:
	CLASS_PROTOTYPE( sdPlayerTask );

	static const int					TASK_BITS				= 8;
	static const int					MAX_TASKS				= ( 1 << TASK_BITS );
	static const int					MAX_WAYPOINTS			= 8;
	static const int					MAX_CHOOSABLE_TASKS		= 5;

	enum taskFlags_t {
		TF_COMPLETE			= BITT< 0 >::VALUE,
		TF_USERCREATED		= BITT< 1 >::VALUE,
	};
	static const int					NUM_TASKFLAGS	= 2;

	enum taskMessageType_t {
		TM_CREATE,
		TM_REMOVE,
		TM_COMPLETE,
		TM_FULLSTATE,
		TM_SETLOCATIION,
		TM_USER_CREATED,
		TM_VISSTATE,
		TM_NUM_MESSAGES,
	};

	struct wayPointInfo_t {
		bool				fixed;
		bool				enabled;
		sdWayPoint*			wayPoint;
		idVec3				offset;
	};

	class sdTaskMessage : public sdReliableServerMessage {
	public:
		sdTaskMessage( const sdPlayerTask* task, taskMessageType_t type ) : sdReliableServerMessage( GAME_RELIABLE_SMESSAGE_TASK ) {
			WriteBits( type, idMath::BitsForInteger( TM_NUM_MESSAGES ) );
			WriteBits( task->GetHandle(), sdPlayerTask::TASK_BITS );
		}
	};

	typedef idLinkList< sdPlayerTask > nodeType_t;

										sdPlayerTask( void );
										~sdPlayerTask( void );

	void								Create( int taskIndex, const sdDeclPlayerTask* taskInfo );

	taskHandle_t						GetHandle( void ) const;
	int									GetIndex( void ) const { return _taskIndex; }

	void								SetComplete( void );
	void								SetTimeout( int duration );

	void								SetEntity( int spawnId );

	void								UpdateObjectiveWayPoint( void );

	void								Think( void );

	static void							Read( idFile* file );

	idEntity*							GetEntity( void ) const { return _entity; }
	bool								IsAvailable( idPlayer* player ) const;
	bool								IsPlayerEligible( idPlayer* player ) const;
	nodeType_t&							GetNode( void ) { return _node; }
	nodeType_t&							GetWorldNode( void ) { return _worldNode; }
	nodeType_t&							GetObjectiveNode( void ) { return _objectiveNode; }
	const sdDeclPlayerTask*				GetInfo( void ) const { return _taskInfo; }
	int									GetEndTime( void ) const { return _endTime; }
	int									GetStartTime( void ) const { return _creationTime; }
	int									GetPriority( void ) const { return _taskInfo->GetPriority(); }
	float								GetCurrentRange( void ) const { return _currentRange; }
	bool								IsComplete( void ) const { return ( _flags & TF_COMPLETE ) != 0; }
	bool								HasEligibleWayPoint( void ) const { return _taskInfo->HasEligibleWayPoint() || IsUserCreated(); }

	void								OnPlayerJoined( idPlayer* player );
	void								OnPlayerLeft( idPlayer* player );

	void								SelectWayPoints( int time );

	void								WriteInitialState( const sdReliableMessageClientInfoBase& target ) const;
	void								ReadInitialState( const idBitMsg& msg );
	void								ReadInitialState( idFile* file );

	void								Write( idFile* file ) const;

	void								SetUserCreated( void );
	bool								IsMission( void ) const { return _taskInfo->IsMission() && !IsUserCreated(); }
	bool								IsUserCreated( void ) const { return ( _flags & TF_USERCREATED ) != 0; }
	bool								IsObjective( void ) const { return _taskInfo->IsObjective(); }

	const char*							GetTimeLimit( void ) const;
	const wchar_t*						GetTitle( idPlayer* player = NULL ) const;
	const wchar_t*						GetCompletedTitle( idPlayer* player = NULL ) const;
	float								GetXP( void ) const;

	static void							HandleMessage( const idBitMsg& msg );
	void								HandleMessage( taskMessageType_t type, const idBitMsg& msg );

	void								ShowWayPoint( void );
	void								ShowWayPoint( int i );

	void								HideWayPoint( void );
	void								HideWayPoint( int i );

	bool								SetTargetPos( int index, const idVec3& target );
	bool								SetWayPointState( int index, bool state );

	virtual idScriptObject*				GetScriptObject( void ) const { return _scriptObject; }

	void								SetCurrentRange( float value ) { _currentRange = value; }

	void								FlashIcon( int time );

	void								Event_Complete( void );
	void								Event_SetTimeout( float duration );
	void								Event_Free( void );

	void								Event_GetTaskEntity( void );
	void								Event_SetTargetPos( int index, const idVec3& position );
	void								Event_SetWayPointState( int index, bool state );

	void								Event_GetKey( const char *key );
	void								Event_GetIntKey( const char *key );
	void								Event_GetFloatKey( const char *key );
	void								Event_GetVectorKey( const char *key );

	void								Event_GetKeyWithDefault( const char *key, const char* defaultvalue );
	void								Event_GetIntKeyWithDefault( const char *key, int defaultvalue );
	void								Event_GetFloatKeyWithDefault( const char *key, float defaultvalue );
	void								Event_GetVectorKeyWithDefault( const char *key, const idVec3& defaultvalue );
	
	void								Event_GiveObjectiveProficiency( float count, const char* reason );

	void								Event_SetUserCreated( void );
	void								Event_IsUserCreated( void );

	void								Event_FlashIcon( int time );

private:
	idWStr&								ProcessTitle( const wchar_t* text, idWStr& output ) const;

	int									_taskIndex;
	const sdDeclPlayerTask*				_taskInfo;
	idEntityPtr< idEntity >				_entity;
	int									_flags;
	nodeType_t							_node;
	nodeType_t							_worldNode;
	nodeType_t							_objectiveNode;
	int									_endTime;
	int									_creationTime;

	idStaticList< wayPointInfo_t, MAX_WAYPOINTS >		_wayPointInfo;

	mutable idWStr						_titleBuffer;
	idScriptObject*						_scriptObject;
	float								_xp;
	bool								_wayPointEnabled;

	float								_currentRange;

	idStaticList< idEntityPtr< idPlayer >, MAX_CLIENTS > _players;
};

typedef idStaticList< sdPlayerTask*, sdPlayerTask::MAX_TASKS >		playerTaskList_t;

class sdTaskManagerLocal : public idClass {
public:
	typedef sdPlayerTask*				taskPtr_t;

public:
	CLASS_PROTOTYPE( sdTaskManagerLocal );

										sdTaskManagerLocal( void );
										~sdTaskManagerLocal( void );

	sdPlayerTask*						TaskForHandle( taskHandle_t handle ) const { return handle.IsValid() ? _taskList[ handle ] : NULL; }

	void								Init( void );
	void								Shutdown( void );
	void								CheckForLeaks( void );

	void								OnNewTask( sdPlayerTask* task );
	sdPlayerTask*						AllocTask( const sdDeclPlayerTask* taskInfo, idEntity* object );
	void								AllocTask( taskHandle_t handle, const sdDeclPlayerTask* taskInfo, int spawnId );
	void								FreeTask( taskHandle_t handle );
	const sdPlayerTask::nodeType_t&		GetTasks( void ) { return _activeTasks; }
	const sdPlayerTask::nodeType_t&		GetObjectiveTasks( const sdTeamInfo* team );
	void								Think( void );

	bool								IsTaskValid( idPlayer* player, taskHandle_t taskHandle, bool onlyCheckInitialFT );
	void								SelectTask( idPlayer* player, taskHandle_t taskHandle );
	
	void								WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) const;

	void								Write( idFile* file ) const;
	void								Read( idFile* file );

	void								AddTaskEntity( idLinkList< idEntity >& entity );

	static int							SortTasksByPriority( const taskPtr_t* a, const taskPtr_t* b );

	void								OnNewScriptLoad( void );
	void								OnMapStart( void );
	void								OnMapShutdown( void );
	void								OnScriptChange( void );
	void								OnLocalTeamChanged( void );

	void								BuildTaskList( idPlayer* player, playerTaskList_t& list );

	void								Event_AllocEntityTask( int taskIndex, idEntity* object );

private:
	playerTaskList_t								_taskList;

	idStaticList< int, sdPlayerTask::MAX_TASKS >	_freeTasks;
	sdPlayerTask::nodeType_t						_activeTasks;
	idList< sdPlayerTask::nodeType_t >				_objectiveTasks;

	idLinkList< idEntity >							_taskEntities;

	idScriptObject*									_scriptObject;
};

typedef sdSingleton< sdTaskManagerLocal > sdTaskManager;

#endif // __GAME_ROLES_TASKS_H__
