//----------------------------------------------------------------
// Instance.h
//
// Copyright 2002-2005 Raven Software
//----------------------------------------------------------------

#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include "Game_local.h"

class rvInstance {
public:
	rvInstance( int id, bool deferPopulate = false );
	~rvInstance();

	void Populate( int serverChecksum = 0 );
	void PopulateFromMessage( const idBitMsg& msg );
	void Restart( void );
	
	void JoinInstance( idPlayer* player );
	int GetInstanceID( void );
	
	void SetSpawnInstanceID( int newInstance );

	void PrintMapNumbers( void );
	int	GetNumMapEntities( void ) { return numMapEntities; }
	unsigned short GetMapEntityNumber( int i ) { return mapEntityNumbers[ i ]; }

private:
	void					BuildInstanceMessage( void );

	int						instanceID;
	int						spawnInstanceID;
	unsigned short*			mapEntityNumbers;
	int						numMapEntities;
	int						initialSpawnCount;
	
	idBitMsg				mapEntityMsg;
	byte					mapEntityMsgBuf[ MAX_GAME_MESSAGE_SIZE ];
};

ID_INLINE int rvInstance::GetInstanceID( void ) {
	return instanceID;
}

#endif
