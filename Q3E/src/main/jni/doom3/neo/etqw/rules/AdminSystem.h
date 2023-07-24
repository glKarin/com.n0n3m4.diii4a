// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __ADMINSYSTEM_H__
#define __ADMINSYSTEM_H__

#include "UserGroup.h"

class sdAdminSystemCommand {
public:
	virtual							~sdAdminSystemCommand( void ) { ; }

	virtual const char*				GetName( void ) const = 0;
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const = 0;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
	virtual bool					AllowedOnRankedServer( void ) const { return true; }

	void							Print( idPlayer* player, const char* locStr, const idWStrList& list = idWStrList() ) const;
};

class sdAdminSystemCommand_Kick : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "kick"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_KickAllBots : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "kickAllBots"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;

	static int						DoKick( void );
};

class sdAdminSystemCommand_Login : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "login"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
};

class sdAdminSystemCommand_Ban : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "ban"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_ListBans : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "listBans"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
};

class sdAdminSystemCommand_UnBan : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "unBan"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
};

class sdAdminSystemCommand_SetTeam : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "setTeam"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_SetCampaign : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "setCampaign"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_SetStopWatchMap : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "setStopWatchMap"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
	virtual bool					AllowedOnRankedServer( void ) const { return false; }
};

class sdAdminSystemCommand_SetObjectiveMap : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "setObjectiveMap"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
	virtual bool					AllowedOnRankedServer( void ) const { return false; }
};

class sdAdminSystemCommand_GlobalMute : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "globalMute"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
	virtual bool					AllowedOnRankedServer( void ) const { return false; }
};

class sdAdminSystemCommand_GlobalVOIPMute : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "globalVOIPMute"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_PlayerMute : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "playerMute"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_PlayerVOIPMute : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "playerVOIPMute"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_Warn : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "warn"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_RestartMap : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "restartMap"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_RestartCampaign : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "restartCampaign"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_ChangeUserGroup : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "changeUserGroup"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_StartMatch : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "startMatch"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
	virtual bool					AllowedOnRankedServer( void ) const { return false; }
};

class sdAdminSystemCommand_ExecConfig : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "execConfig"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_ShuffleTeams : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "shuffleTeams"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
};

class sdAdminSystemCommand_AddBot : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "addbot"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
	virtual bool					AllowedOnRankedServer( void ) const { return false; }
};

class sdAdminSystemCommand_AdjustBots : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "adjustBots"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
	virtual bool					AllowedOnRankedServer( void ) const { return false; }
};

class sdAdminSystemOnOffCommand : public sdAdminSystemCommand {
public:
	virtual void CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
		callback( va( "%s %s on", args.Argv( 0 ), args.Argv( 1 ) ) );
		callback( va( "%s %s off", args.Argv( 0 ), args.Argv( 1 ) ) );
	}

	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;

	virtual userPermissionFlags_t	GetRequiredPersmission( void ) const = 0;
	virtual const char*				GetPermissionFailedMessage( void ) const = 0;
	virtual void					OnCompleted( idPlayer* player, bool value ) const = 0;
};

class sdAdminSystemOnOffCommand_CVar : public sdAdminSystemOnOffCommand {
public:
	virtual void					OnCompleted( idPlayer* player, bool value ) const {
		GetCVar().SetBool( value );
	}

	virtual idCVar&					GetCVar( void ) const = 0;
};

class sdAdminSystemCommand_DisableProficiency : public sdAdminSystemOnOffCommand_CVar {
public:
	virtual const char*				GetName( void ) const { return "disableProficiency"; }
	virtual bool					AllowedOnRankedServer( void ) const { return false; }

	virtual const char*				GetPermissionFailedMessage( void ) const { return "guis/admin/system/nopermdisableproficiency"; }
	virtual userPermissionFlags_t	GetRequiredPersmission( void ) const { return PF_ADMIN_DISABLE_PROFICIENCY; }
	virtual idCVar&					GetCVar( void ) const { return si_noProficiency; }
};

class sdAdminSystemCommand_SetTimeLimit : public sdAdminSystemCommand {
public:
	virtual const char*				GetName( void ) const { return "setTimeLimit"; }
	virtual bool					PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const;
	virtual void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const;
	virtual bool					AllowedOnRankedServer( void ) const { return false; }
};

class sdAdminSystemCommand_SetTeamDamage : public sdAdminSystemOnOffCommand_CVar {
public:
	virtual const char*				GetName( void ) const { return "setTeamDamage"; }
	virtual bool					AllowedOnRankedServer( void ) const { return false; }

	virtual const char*				GetPermissionFailedMessage( void ) const { return "guis/admin/system/nopermsetteamdamage"; }
	virtual userPermissionFlags_t	GetRequiredPersmission( void ) const { return PF_ADMIN_SET_TEAMDAMAGE; }
	virtual idCVar&					GetCVar( void ) const { return si_teamDamage; }
};

class sdAdminSystemCommand_SetTeamBalance : public sdAdminSystemOnOffCommand_CVar {
public:
	virtual const char*				GetName( void ) const { return "setTeamBalance"; }
	virtual bool					AllowedOnRankedServer( void ) const { return false; }

	virtual const char*				GetPermissionFailedMessage( void ) const { return "guis/admin/system/nopermsetteambalance"; }
	virtual userPermissionFlags_t	GetRequiredPersmission( void ) const { return PF_ADMIN_SET_TEAMBALANCE; }
	virtual idCVar&					GetCVar( void ) const { return si_teamForceBalance; }
};

class sdAdminSystemLocal;

SD_UI_PUSH_CLASS_TAG( sdAdminProperties )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"Admin system properties may be accessed with \"admin.<propname>\". The properties are used in the admin tab in the limbo menu " \
	"to control which commands, buttons and lists should be enabled or greyed out. " \
	"All player properties are read only."
/* ============ */
)
SD_UI_POP_CLASS_TAG
SD_UI_PROPERTY_TAG(
alias = "admin";
)
class sdAdminProperties : public sdUIPropertyHolder {
public:
	virtual sdProperties::sdProperty*			GetProperty( const char* name );
	virtual sdProperties::sdProperty*			GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdProperties::sdPropertyHandler&	GetProperties() { return properties; }
	virtual const char*							GetName() const { return "adminProperties"; }
	virtual const char*							FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) { scope = this; return properties.NameForProperty( property ); }

	void										Update( void );
	void										RegisterProperties( void );

private:
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/UserGroup";
	desc				= "Name of the local players user group.";
	datatype			= "wstring";
	)
	sdWStringProperty							userGroup;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CommandState";
	desc				= "If command succeded or failed (CS_SUCCESS/CS_FAILED).";
	datatype			= "float";
	)
	sdFloatProperty								commandState;

	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CommandStateTime";
	desc				= "Last time command state was set.";
	datatype			= "float";
	)
	sdFloatProperty								commandStateTime;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanAddBot";
	desc				= "Local player is allowed to add bots.";
	datatype			= "float";
	)
	sdFloatProperty								canAddBot;

	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanAdjustBots";
	desc				= "Local player is allowed to adjust the number of bots.";
	datatype			= "float";
	)
	sdFloatProperty								canAdjustBots;

	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanKick";
	desc				= "Local player is allowed to kick players.";
	datatype			= "float";
	)
	sdFloatProperty								canKick;

	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanBan";
	desc				= "Local player is allowed to ban players.";
	datatype			= "float";
	)
	sdFloatProperty								canBan;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanSetTeam";
	desc				= "Local player is allowed to change other players team.";
	datatype			= "float";
	)
	sdFloatProperty								canSetTeam;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanChangeCampaign";
	desc				= "Local player is allowed to change the campaign.";
	datatype			= "float";
	)
	sdFloatProperty								canChangeCampaign;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanChangeMap";
	desc				= "Local player is allowed to change the map.";
	datatype			= "float";
	)
	sdFloatProperty								canChangeMap;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanGlobalMute";
	desc				= "Local player is allowed mute chat globally.";
	datatype			= "float";
	)
	sdFloatProperty								canGlobalMute;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanGlobalMuteVoip";
	desc				= "Local player is allowed to mute VOIP globally.";
	datatype			= "float";
	)
	sdFloatProperty								canGlobalMuteVoip;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanPlayerMute";
	desc				= "Local player is allowed to mute a player.";
	datatype			= "float";
	)
	sdFloatProperty								canPlayerMute;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanPlayerMuteVoip";
	desc				= "Local player is allowed to mute a player from VOIP.";
	datatype			= "float";
	)
	sdFloatProperty								canPlayerMuteVoip;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanWarn";
	desc				= "Local player is allowed to warn a player. If player recieves g_maxPlayerWarnings warnings the player will be kicked.";
	datatype			= "float";
	)
	sdFloatProperty								canWarn;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanRestartMap";
	desc				= "Local player is allowed to restart the map.";
	datatype			= "float";
	)
	sdFloatProperty								canRestartMap;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanRestartCampaign";
	desc				= "Local player is allowed to restart the campaign.";
	datatype			= "float";
	)
	sdFloatProperty								canRestartCampaign;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanStartMatch";
	desc				= "Local player is allowed to start the match (valid while in warmup).";
	datatype			= "float";
	)
	sdFloatProperty								canStartMatch;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanExecConfig";
	desc				= "Local player is allowed to exec a server config.";
	datatype			= "float";
	)
	sdFloatProperty								canExecConfig;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanShuffleTeams";
	desc				= "Local player is allowed to shuffle the teams.";
	datatype			= "float";
	)
	sdFloatProperty								canShuffleTeams;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/IsSuperUser";
	desc				= "User typing commands in the console is super user.";
	datatype			= "float";
	)
	sdFloatProperty								isSuperUser;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanLogin";
	desc				= "True if any of the user groups requires a password to login to.";
	datatype			= "float";
	)
	sdFloatProperty								canLogin;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanDisableProficiency";
	desc				= "Local player is allowed to disable XP gaining.";
	datatype			= "float";
	)
	sdFloatProperty								canDisableProficiency;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanSetTimelimit";
	desc				= "Local player is allowed to set the timelimit.";
	datatype			= "float";
	)
	sdFloatProperty								canSetTimelimit;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanSetTeamDamage";
	desc				= "Local player is allowed to toggle team damage.";
	datatype			= "float";
	)
	sdFloatProperty								canSetTeamDamage;
	
	SD_UI_PROPERTY_TAG(
	title				= "AdminProperties/CanSetTeamBalance";
	desc				= "Local player is allowed to toggle team balancing, disallowing players to join the team with less players.";
	datatype			= "float";
	)
	sdFloatProperty								canSetTeamBalance;

	sdProperties::sdPropertyHandler				properties;

	const sdDeclLocStr*							superUserString;
};

class sdAdminSystemLocal {
public:
									sdAdminSystemLocal( void );
									~sdAdminSystemLocal( void );

	enum commandState_e { CS_NOCOMMAND, CS_SUCCESS, CS_FAILED };

	void							Init( void );
	void							UpdateProperties( void ) { properties.Update(); }

	void							SetCommandState( commandState_e state );
	commandState_e					GetCommandState() const { return commandState; }
	int								GetCommandStateTime() const { return commandStateTime; }

	void							PerformCommand( const idCmdArgs& cmd, idPlayer* player );
	static void						CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback );
	sdUIPropertyHolder&				GetProperties( void ) { return properties; }

	void							CreatePlayerAdminList( sdUIList* list );

private:
	idList< sdAdminSystemCommand* >	commands;
	sdAdminProperties				properties;
	commandState_e					commandState;
	int								commandStateTime;
};

typedef sdSingleton< sdAdminSystemLocal > sdAdminSystem;

#endif // __ADMINSYSTEM_H__
