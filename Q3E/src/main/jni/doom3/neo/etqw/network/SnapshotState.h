// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __NETWORK_SNAPSHOTSTATE_H__
#define __NETWORK_SNAPSHOTSTATE_H__

// #define SAFE_STATE_CONVERSION

enum networkStateMode_t {
	NSM_VISIBLE = 0,
	NSM_BROADCAST,
	NSM_NUM_MODES,
};

enum extNetworkStateMode_t {
	ENSM_TEAM = 0,
	ENSM_RULES,
	ENSM_NUM_MODES,
};

#define NET_GET_STATES( STATE_TYPE )								\
	const STATE_TYPE& baseData = baseState.GetData< STATE_TYPE >();	\
	STATE_TYPE& newData = newState.GetData< STATE_TYPE >()			\

#define NET_ON_BROADCAST											\
	if ( mode == NSM_BROADCAST )

#define NET_ON_VISIBLE												\
	if ( mode == NSM_VISIBLE )

#define NET_GET_BASE( STATE_TYPE )									\
	const STATE_TYPE& baseData = baseState.GetData< STATE_TYPE >()

#define NET_GET_NEW( STATE_TYPE )									\
	const STATE_TYPE& newData = newState.GetData< STATE_TYPE >()

#define NET_CHECK_FIELD( FIELD_NAME, OBJECT_FIELD )					\
	if ( baseData.FIELD_NAME != OBJECT_FIELD ) {					\
		return true;												\
	}

#define NET_READ_STATE_PHYSICS																		\
	if ( baseData.physicsData ) {																	\
		GetPhysics()->ReadNetworkState( mode, *baseData.physicsData, *newData.physicsData, msg );	\
	}

#define NET_WRITE_STATE_PHYSICS																		\
	if ( baseData.physicsData ) {																	\
		GetPhysics()->WriteNetworkState( mode, *baseData.physicsData, *newData.physicsData, msg );	\
	}

#define NET_APPLY_STATE_PHYSICS																		\
	if ( newData.physicsData ) {																	\
		GetPhysics()->ApplyNetworkState( mode, *newData.physicsData );								\
	}

#define NET_CHECK_STATE_PHYSICS															\
	if ( baseData.physicsData ) {														\
		if ( GetPhysics()->CheckNetworkStateChanges( mode, *baseData.physicsData ) ) {	\
			return true;																\
		}																				\
	}

#define NET_APPLY_STATE_SCRIPT																	\
		scriptObject->ApplyNetworkState( mode, newData.scriptData );

#define NET_READ_STATE_SCRIPT																	\
		scriptObject->ReadNetworkState( mode, baseData.scriptData, newData.scriptData, msg );

#define NET_WRITE_STATE_SCRIPT																	\
		scriptObject->WriteNetworkState( mode, baseData.scriptData, newData.scriptData, msg );

#define NET_CHECK_STATE_SCRIPT																	\
		if ( scriptObject->CheckNetworkStateChanges( mode, baseData.scriptData ) ) {			\
			return true;																		\
		}




class sdEntityStateNetworkData {
public:
	virtual						~sdEntityStateNetworkData( void ) { }

	virtual void				MakeDefault( void ) = 0;
	virtual void				Write( idFile* file ) const = 0;
	virtual void				Read( idFile* file ) = 0;

#ifdef SAFE_STATE_CONVERSION
	template< typename T >
	T&							GetData( void ) { return dynamic_cast< T& >( *this ); }

	template< typename T >
	const T&					GetData( void ) const { return dynamic_cast< const T& >( *this ); }
#else
	template< typename T >
	T&							GetData( void ) { return static_cast< T& >( *this ); }

	template< typename T >
	const T&					GetData( void ) const { return static_cast< const T& >( *this ); }
#endif // SAFE_STATE_CONVERSION
};

class sdEntityState {
public:
								sdEntityState( idEntity* entity, networkStateMode_t mode );
								~sdEntityState( void );


	int							GetSpawnId( void ) const { return spawnId; }

	int							spawnId;
	sdEntityState*				next;
	sdEntityStateNetworkData*	data;
};

class sdNetworkStateObject;

class sdGameState {
public:
								sdGameState( const sdNetworkStateObject& object );
								~sdGameState( void );


	sdGameState*				next;
	sdEntityStateNetworkData*	data;
};

class snapshot_t {
public:
								snapshot_t( void );

	void						Init( int _sequence, int _time );

	int							sequence;			// snapshot number
	int							time;
	
	idList< int >				visibleEntities;
	sdBitField< MAX_CLIENTS >	clientUserCommands;	// which clients have user commands going along with this snapshot
	sdEntityState*				firstEntityState[ NSM_NUM_MODES ];
	sdGameState*				gameStates[ ENSM_NUM_MODES ];
	
	snapshot_t*					next;
};

class clientNetworkInfo_t {
public:
								clientNetworkInfo_t( void );

	void						Reset( void );

	int							lastMarker[ MAX_GENTITIES ];			// when each entity was last sent to a client
	int							lastUserCommand[ MAX_CLIENTS ];			// when user commands were last sent to a client
	int							lastUserCommandDelay[ MAX_CLIENTS ];	// time between two previous user commands

	snapshot_t*					snapshots;

	sdEntityState*				states[ MAX_GENTITIES ][ NSM_NUM_MODES ];
	sdGameState*				gameStates[ ENSM_NUM_MODES ];
};

class sdNetworkStateObject {
public:
										sdNetworkStateObject( void ) { freeStates = NULL; }
										~sdNetworkStateObject( void );

	void								Clear( void );

	virtual void						ApplyNetworkState( const sdEntityStateNetworkData& newState ) = 0;
	virtual void						ReadNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const = 0;
	virtual void						WriteNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const = 0;
	virtual bool						CheckNetworkStateChanges( const sdEntityStateNetworkData& baseState ) const = 0;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( void ) const = 0;

public:
	mutable sdGameState*				freeStates;
};

template< typename T,
			void ( T::*APPLYFUNC )( const sdEntityStateNetworkData& newState ),
			void ( T::*READFUNC )( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const,
			void ( T::*WRITEFUNC )( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const,
			bool ( T::*CHECKFUNC )( const sdEntityStateNetworkData& baseState ) const,
			sdEntityStateNetworkData* ( T::*CREATEFUNC )( void ) const >
class sdNetworkStateObject_Generic : public sdNetworkStateObject {
public:
										sdNetworkStateObject_Generic( void ) { owner = NULL; }

	void								Init( T* _owner ) { owner = _owner; }

	virtual void						ApplyNetworkState( const sdEntityStateNetworkData& newState ) { CALL_MEMBER_FN_PTR( owner, APPLYFUNC )( newState ); }
	virtual void						ReadNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const { CALL_MEMBER_FN_PTR( owner, READFUNC )( baseState, newState, msg ); }
	virtual void						WriteNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const { CALL_MEMBER_FN_PTR( owner, WRITEFUNC )( baseState, newState, msg ); }
	virtual bool						CheckNetworkStateChanges( const sdEntityStateNetworkData& baseState ) const { return CALL_MEMBER_FN_PTR( owner, CHECKFUNC )( baseState ); }
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( void ) const { return CALL_MEMBER_FN_PTR( owner, CREATEFUNC )(); }

private:
	T*									owner;
};

#endif // __NETWORK_SNAPSHOTSTATE_H__
