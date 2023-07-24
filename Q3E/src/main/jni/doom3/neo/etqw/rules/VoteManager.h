// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __VOTE_MANAGER_H__
#define __VOTE_MANAGER_H__

class idPlayer;
class sdUserGroup;
class sdPlayerVote;

// for scripts
enum voteMode_t {
	VM_GLOBAL,
	VM_TEAM,
	VM_PRIVATE,
};

enum voteId_t {
	VI_NONE,
	VI_FIRETEAM_CREATE,
	VI_FIRETEAM_JOIN,
	VI_FIRETEAM_REQUEST,
	VI_FIRETEAM_PROPOSE,
	VI_FIRETEAM_INVITE,
	VI_DRIVER_BACKUP,
	VI_SWITCH_TEAM,
};

class sdCallVote {
public:
	virtual										~sdCallVote( void ) { ; }

	virtual const char*							GetCommandName( void ) const = 0;
	virtual void								GetText( idWStr& out ) const = 0;
	virtual void								EnumerateOptions( sdUIList* list ) const = 0;
	virtual void								Execute( int dataKey ) const = 0;
	virtual sdPlayerVote*						Execute( const idCmdArgs& args, idPlayer* player ) const = 0;
	virtual void								PreCache( void ) const = 0;
	virtual bool								AllowedOnRankedServer( void ) const { return true; }

	void										CreateCmdArgs( idCmdArgs& args ) const;

	static void									AddVoteOption( sdUIList* list, const wchar_t* text );
	static void									AddVoteOption( sdUIList* list, const wchar_t* text, int dataKey );
};

class sdCallVoteAdminCommand : public sdCallVote {
public:
	virtual void EnumerateOptions( sdUIList* list ) const;
	virtual void Execute( int dataKey ) const;
	virtual sdPlayerVote* Execute( const idCmdArgs& args, idPlayer* player ) const;

	virtual const char* GetAdminCommand( void ) const = 0;
	virtual const char* GetAdminCommandSuffix( void ) const { return NULL; }
	virtual const char* GetVoteTextKey( void ) const = 0;


	virtual void		PreCache( void ) const { 
		gameLocal.declToolTipType[ GetVoteTextKey() ];
	}
};

class sdCallVoteMapReset : public sdCallVoteAdminCommand {
public:
	virtual const char* GetCommandName( void ) const { return "maprestart"; }
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/restartMap" ); }
	virtual const char* GetAdminCommand( void ) const { return "restartMap"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/restartMap/text"; }
};

class sdCallVoteCampaignReset : public sdCallVoteAdminCommand {
public:
	virtual const char* GetCommandName( void ) const { return "campaignreset"; }
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/campaignreset" ); }
	virtual const char* GetAdminCommand( void ) const { return "restartCampaign"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/campaignreset/text"; }
};

class sdVoteFinalizer {
public:
	virtual										~sdVoteFinalizer( void ) { ; }
	virtual void								OnVoteCompleted( bool passed ) const = 0;
};


class sdVoteMode {
public:
	virtual										~sdVoteMode( void ) { ; }
	virtual bool								CanVote( idPlayer* player ) const = 0;
};

class sdVoteModePrivate : public sdVoteMode {
public:
												sdVoteModePrivate( idPlayer* other );
	virtual										~sdVoteModePrivate( void ) { ; }
	virtual bool								CanVote( idPlayer* player ) const;

private:
	idEntityPtr< idPlayer >						client;
};

class sdVoteModeTeam : public sdVoteMode {
public:
												sdVoteModeTeam( sdTeamInfo* _team );
	virtual										~sdVoteModeTeam( void ) { ; }
	virtual bool								CanVote( idPlayer* player ) const;

private:
	sdTeamInfo*									team;
};

class sdVoteModeGlobal : public sdVoteMode {
public:
												sdVoteModeGlobal( void ) { ; }
	virtual										~sdVoteModeGlobal( void ) { ; }
	virtual bool								CanVote( idPlayer* player ) const;

private:
};

class sdVoteFinalizer_Script : public sdVoteFinalizer {
public:
												sdVoteFinalizer_Script( idScriptObject* object, const char* functionName );
	virtual										~sdVoteFinalizer_Script( void ) { ; }
	virtual void								OnVoteCompleted( bool passed ) const;

private:
	qhandle_t									scriptObjectHandle;
	const sdProgram::sdFunction*				function;
};

class sdPlayerVote {
public:
	enum voteState_t {
		VS_NOT_VOTED,
		VS_CANNOT_VOTE,
		VS_YES,
		VS_NO,
	};
												sdPlayerVote( void );
												~sdPlayerVote( void );

	void										Init( int _index ) { index = _index; }
	void										MakeTeamVote( sdTeamInfo* team );
	void										MakePrivateVote( idPlayer* client );
	void										MakeGlobalVote( void );
	void										ClearMode( void );
	void										ClearFinalizer( void );
	void										SetVoteFlags( void );
	void										SetFinalizer( sdVoteFinalizer* _finalizer ) { ClearFinalizer(); finalizer = _finalizer; }
	void										SetText( const sdDeclToolTip* _text ) { text = _text; }
	void										AddTextParm( const sdDeclLocStr* text );
	void										AddTextParm( const wchar_t* text );
	void										Start( idPlayer* player = NULL );
	int											GetEndTime( void ) const { return endTime; }
	sdPlayerVote*								GetNextActiveVote( void ) { return activeNode.Next(); }
	bool										GetPlayerCanVote( idPlayer* player ) const;
	void										Complete( void );
	int											GetIndex( void ) const { return index; }
	const wchar_t*								GetText( void ) const { return finalText.c_str(); }
	void										Vote( idPlayer* client, bool result );
	void										Vote( bool result );
	void										ClientVote( bool result );
	void										SendMessage( sdReliableServerMessage& msg );
	int											GetYesCount( void ) const { return yesCount; }
	int											GetNoCount( void ) const { return noCount; }
	float										GetPassPercentage( void ) const;
	void										DisableFinishMessage( void ) { displayFinishedMessage = false; }
	void										SetEndTime( int time ) { endTime = time; }
	voteState_t									GetVoteState( idPlayer* player ) { if ( player != NULL ) { return votes[ player->entityNumber ]; } else { return VS_CANNOT_VOTE; } }

	void										Tag( voteId_t id, idEntity* object );
	bool										Match( voteId_t id, idEntity* object );

	sdVoteFinalizer*							GetFinalizer( void ) const { return finalizer; }

	idPlayer*									GetVoteCaller( void ) const { return voteCaller; }
	voteId_t									GetVoteId( void ) const { return voteId; }

private:
	sdVoteMode*									mode;
	sdVoteFinalizer*							finalizer;

	const sdDeclToolTip*						text;
	struct textParm_t {
		const sdDeclLocStr* locStr;
		idWStr				text;
	};
	idList< textParm_t >						textParms;

	idWStr										finalText;

	int											endTime;
	voteState_t									votes[ MAX_CLIENTS ];
	idLinkList< sdPlayerVote >					activeNode;
	int											index;
	int											yesCount;
	int											noCount;
	int											totalCount;
	bool										displayFinishedMessage;

	voteId_t									voteId;
	idEntityPtr< idEntity >						voteObject;

	idEntityPtr< idPlayer >						voteCaller;
};

class sdVoteManagerLocal {
public:
	enum voteMessage_t {
		VM_CREATE,
		VM_FREE,
		VM_VOTE,
		VM_NUM,
	};

	static const int							MAX_VOTES = 1024;

												sdVoteManagerLocal( void );
												~sdVoteManagerLocal( void );

	void										MakeActive( idLinkList< sdPlayerVote >& node );
	void										Think( void );
	void										Init( void );

	sdPlayerVote*								AllocVote( int index = -1 );
	void										FreeVote( int index );
	void										FreeVotes( void );
	sdPlayerVote*								GetActiveVote( idPlayer* p );

	void										CancelVote( sdPlayerVote* vote );
	void										CancelVote( voteId_t id, idEntity* object );
	void										CancelFireTeamVotesForPlayer( idPlayer *player );
	sdPlayerVote*								FindVote( voteId_t id, idEntity* object );
	sdPlayerVote*								FindVote( voteId_t id );

	static void									CreateCallVoteList( sdUIList* list);
	static void									CreateCallVoteOptionList( sdUIList* list );
	static void									Callvote_f( const idCmdArgs &args );

	void										ListVotes( void );

	void										ClientReadNetworkMessage( const idBitMsg& msg );
	void										ServerReadNetworkMessage( idPlayer* player, const idBitMsg& msg );

	void										PerformCommand( const idCmdArgs& cmd, idPlayer* player );

	void										EnumerateCallVotes( sdUIList* list );
	void										ExecVote( sdUserInterfaceLocal* ui );
	bool										CanUseVote( sdCallVote* vote, idPlayer* player );

	int											NumActiveVotes( voteId_t id, idEntity* player = NULL ) const;
private:
	void										PushVoteItem( sdUIList* list, sdCallVote* vote, int voteIndex );
	sdCallVote*									CallVoteForIndex( int index );

private:
	sdPlayerVote*								votes[ MAX_VOTES ];
	idLinkList< sdPlayerVote >					activeVotes;
	idList< sdCallVote* >						globalCallVotes;
};

typedef sdSingleton< sdVoteManagerLocal > sdVoteManager;

#endif // __VOTE_MANAGER_H__
