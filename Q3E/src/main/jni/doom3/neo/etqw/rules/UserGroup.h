// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __USERGROUP_H__
#define __USERGROUP_H__

enum userPermissionFlags_t {
	PF_ADMIN_KICK,
	PF_ADMIN_BAN,
	PF_ADMIN_SETTEAM,
	PF_ADMIN_CHANGE_CAMPAIGN,
	PF_ADMIN_CHANGE_MAP,
	PF_ADMIN_GLOBAL_MUTE,
	PF_ADMIN_GLOBAL_MUTE_VOIP,
	PF_ADMIN_PLAYER_MUTE,
	PF_ADMIN_PLAYER_MUTE_VOIP,
	PF_ADMIN_WARN,
	PF_ADMIN_RESTART_MAP,
	PF_ADMIN_RESTART_CAMPAIGN,
	PF_ADMIN_START_MATCH,
	PF_ADMIN_EXEC_CONFIG,
	PF_ADMIN_SHUFFLE_TEAMS,
	PF_NO_BAN,
	PF_NO_KICK,
	PF_NO_MUTE,
	PF_NO_MUTE_VOIP,
	PF_NO_WARN,
	PF_QUIET_LOGIN,
	PF_ADMIN_ADD_BOT,
	PF_ADMIN_ADJUST_BOTS,
	PF_ADMIN_DISABLE_PROFICIENCY,
	PF_ADMIN_SET_TIMELIMIT,
	PF_ADMIN_SET_TEAMDAMAGE,
	PF_ADMIN_SET_TEAMBALANCE,
	PF_NUM_FLAGS,
};

class sdUserGroup {
public:
									sdUserGroup( void );

	bool							HasPermission( userPermissionFlags_t flag ) const { return superUser || ( flags.Get( flag ) != 0 ); }
	const char*						GetName( void ) const { return name; }
	bool							CheckPassword( const char* pass ) const { return password.Length() > 0 && idStr::Icmp( password, pass ) == 0; }

	void							GivePermission( userPermissionFlags_t flag ) { flags.Set( flag ); }
	void							SetName( const char* _name ) { name = _name; }
	void							SetPassword( const char* pass ) { password = pass; needsLogin = idStr::Length( password ) > 0; }

	bool							ParseControl( idLexer& src );
	bool							Parse( idLexer& src );
	void							PostParseFixup( void );

	bool							CanControl( const sdUserGroup& group ) const;

	void							MakeSuperUser( void ) { superUser = true; }
	void							Write( idBitMsg& msg ) const;
	void							Read( const idBitMsg& msg );
	void 							Write( idFile* file ) const;
	void 							Read( idFile* file );

	bool							CanCallVote( int level ) const { return superUser || voteLevel >= level; }

	bool							IsSuperUser() const { return superUser;}
	bool							HasPassword() const { return needsLogin; }

private:
	sdBitField< PF_NUM_FLAGS >		flags;
	bool							superUser;
	bool							needsLogin;
	int								voteLevel;
	idStr							name;
	idStr							password;
	idList< sdPair< idStr, int > >	controlGroups;
};

class sdUserGroupManagerLocal {
public:
									sdUserGroupManagerLocal( void );
									~sdUserGroupManagerLocal( void );

	const sdUserGroup&				GetConsoleGroup( void ) { return consoleGroup; }
	int								FindGroup( const char* name );
	const sdUserGroup&				GetGroup( int index ) { if ( index < 0 || index >= userGroups.Num() ) { return userGroups[ defaultGroup ]; } else { return userGroups[ index ]; } }
	bool							Login( idPlayer* player, const char* password );
	void							ReadNetworkData( const idBitMsg& msg );
	void							WriteNetworkData( const sdReliableMessageClientInfoBase& target );
	int								NumGroups( void ) const { return userGroups.Num(); }

	bool							CanLogin() const;

	void							Init( void );
	void							CreateUserGroupList( sdUIList* list );
	void							CreateServerConfigList( sdUIList* list );	

	int								GetVoteLevel( const char* voteName );

	const idDict&					GetConfigs( void ) const { return configs; }

	void							Write( idFile* file ) const;
	void							Read( idFile* file );

private:
	sdUserGroup						consoleGroup;

	idDict							configs;
	idDict							votes;

	int								nextLoginTime[ MAX_CLIENTS ];

	idList< sdUserGroup >			userGroups;
	int								defaultGroup;
};

typedef sdSingleton< sdUserGroupManagerLocal > sdUserGroupManager;

#endif // __USERGROUP_H__
