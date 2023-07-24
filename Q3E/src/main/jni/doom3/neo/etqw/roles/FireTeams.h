// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_ROLES_FIRETEAMS_H__
#define __GAME_ROLES_FIRETEAMS_H__

typedef sdPair< idPlayer*, int >	recentlyRejected_t;

class sdFireTeam {
public:
	typedef enum fireteamMessageType_e {
		FM_FULLSTATE,
		FM_ADDMEMBER,
		FM_REMOVEMEMBER,
		FM_PROMOTEMEMBER,
		FM_SETPRIVATE,
		FM_SETNAME,
		FM_NUM_MESSAGES,
	} fireteamMessageType_t;

	class sdFireTeamMessage : public sdReliableServerMessage {
	public:
		sdFireTeamMessage( const sdFireTeam* fireTeam, fireteamMessageType_t type ) : sdReliableServerMessage( GAME_RELIABLE_SMESSAGE_FIRETEAM ) {
			WriteBits( type, idMath::BitsForInteger( FM_NUM_MESSAGES ) );
			WriteBits( fireTeam->GetHandle(), _fireTeamBits );
		}
	};

public:
										sdFireTeam( void );
										~sdFireTeam( void );

	void								Init( int index );
	void								Clear( void );
	void								Think( void );

	qhandle_t							GetHandle( void ) const { return _index; }
	const char*							GetName( void ) const { return _name; }

	void								AddMember( int clientNum );
	void								RemoveMember( int clientNum );
	void								PromoteMember( int clientNum );
	void								SetName( const char* name );
	void								SetDefaultName( const char* name );
	void								SetGameTeam( sdTeamInfo* team ) { _team = team; }

	void								Invite( idPlayer* other );
	void								Propose( idPlayer* other, idPlayer* member );
	void								Request( idPlayer* other );
	void								Disband( void );
	void								SendDisbandMessage( void );

	void								SetPrivate( bool isPrivate );
	bool								IsPrivate() const { return _private; }

	int									GetNumMembers( void ) const { return _members.Num(); }
	idPlayer*							GetMember( int index ) const { return gameLocal.GetClient( _members[ index ] ); }

	void								WriteInitialState( const sdReliableMessageClientInfoBase& target ) const;
	void								ReadInitialState( const idBitMsg& msg );
	void								HandleMessage( fireteamMessageType_t type, const idBitMsg& msg );

	void								Write( idFile* file ) const;
	void								Read( idFile* file );

	static void							HandleMessage( const idBitMsg& msg );

	static void							SetFireTeamBits( int bits ) { _fireTeamBits = bits; }

	static int							MaxMembers( void );

	idPlayer*							GetCommander( void ) const { return GetNumMembers() > 0 ? GetMember( 0 ) : NULL; }
	bool								IsCommander( idPlayer* other ) const { return other == GetCommander(); }
	bool								IsMember( idPlayer* other ) const;

	void								SetLastInvite( int clientNum, int time ) { assert( clientNum >= 0 && clientNum < MAX_CLIENTS ); _lastInvite[ clientNum ] = time; }
	int									GetLastInvite( int clientNum ) const { assert( clientNum >= 0 && clientNum < MAX_CLIENTS ); return _lastInvite[ clientNum ]; }

private:
	static int							_fireTeamBits;
	int									_index;
	bool								_private;
	idStr								_name;
	idStr								_defaultName;
	sdTeamInfo*							_team;
	idList< int >						_members;
	int									_lastInvite[ MAX_CLIENTS ];
};

class sdFireTeamSystemCommand {
public:
	virtual								~sdFireTeamSystemCommand( void ) { ; }

	virtual const char*					GetName( void ) const = 0;
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const = 0;
	virtual void						CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdFireTeamSystemCommand_Join : public sdFireTeamSystemCommand {
public:
	virtual const char*					GetName( void ) const { return "join"; }
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const;
	virtual void						CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdFireTeamSystemCommand_Leave : public sdFireTeamSystemCommand {
public:
	virtual const char*					GetName( void ) const { return "leave"; }
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const;
};

class sdFireTeamSystemCommand_Create : public sdFireTeamSystemCommand {
public:
	virtual const char*					GetName( void ) const { return "create"; }
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const;
};

class sdFireTeamSystemCommand_Invite : public sdFireTeamSystemCommand {
public:
	virtual const char*					GetName( void ) const { return "invite"; }
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const;
	virtual void						CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdFireTeamSystemCommand_Propose : public sdFireTeamSystemCommand {
public:
	virtual const char*					GetName( void ) const { return "propose"; }
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const;
	virtual void						CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdFireTeamSystemCommand_Kick : public sdFireTeamSystemCommand {
public:
	virtual const char*					GetName( void ) const { return "kick"; }
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const;
	virtual void						CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdFireTeamSystemCommand_Disband : public sdFireTeamSystemCommand {
public:
	virtual const char*					GetName( void ) const { return "disband"; }
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const;
};

class sdFireTeamSystemCommand_Promote : public sdFireTeamSystemCommand {
public:
	virtual const char*					GetName( void ) const { return "promote"; }
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const;
	virtual void						CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdFireTeamSystemCommand_Name : public sdFireTeamSystemCommand {
public:
	virtual const char*					GetName( void ) const { return "name"; }
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const;
};

class sdFireTeamSystemCommand_Private : public sdFireTeamSystemCommand {
public:
	virtual const char*					GetName( void ) const { return "private"; }
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const;
};

class sdFireTeamSystemCommand_Public : public sdFireTeamSystemCommand {
public:
	virtual const char*					GetName( void ) const { return "public"; }
	virtual void						PerformCommand( const idCmdArgs& cmd, idPlayer* player ) const;
};

class sdFireTeamManagerLocal {
public:
										sdFireTeamManagerLocal( void );
										~sdFireTeamManagerLocal( void );

	void								Init( void );
	void								Clear( void );
	
	void								Think( void );

	sdFireTeam*							FireTeamForHandle( qhandle_t handle );

	int									NumFireTeams() const { return _fireTeams.Num(); }
	sdFireTeam*							FireTeamForIndex( int index ) { return _fireTeams[ index ]; }
	const sdFireTeam*					FireTeamForIndex( int index ) const { return _fireTeams[ index ]; }
	
	void								WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) const;

	void								Write( idFile* file ) const;
	void								Read( idFile* file );

	void								PerformCommand( const idCmdArgs& cmd, idPlayer* player );

	static void							CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback );

private:
	idList< sdFireTeam* >				_fireTeams;
	idList< sdFireTeamSystemCommand* >	_commands;
};

typedef sdSingleton< sdFireTeamManagerLocal > sdFireTeamManager;

#endif // __GAME_ROLES_FIRETEAMS_H__
