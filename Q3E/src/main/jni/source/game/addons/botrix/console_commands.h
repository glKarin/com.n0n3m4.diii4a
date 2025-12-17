#ifndef __BOTRIX_CONSOLE_COMMANDS_H__
#define __BOTRIX_CONSOLE_COMMANDS_H__

#include <good/memory.h>

#include "types.h"

#include "clients.h"
#include "item.h"
#include "type2string.h"

#include "public/tier1/convar.h"
#include "public/edict.h"


//****************************************************************************************************************
/// Console command.
//****************************************************************************************************************
class CConsoleCommand
{
public:

    CConsoleCommand( char *szCommand, TCommandAccessFlags iCommandAccessFlags = FCommandAccessNone ):
        m_sCommand(szCommand), m_iAccessLevel(iCommandAccessFlags) {}

    virtual ~CConsoleCommand() {}

    bool IsCommand( const char* szCommand ) { return m_sCommand == szCommand; }

    bool HasAccess( CClient* pClient )
    {
        TCommandAccessFlags access = pClient ? pClient->iCommandAccessFlags : FCommandAccessAll;
        return FLAG_ALL_SET(m_iAccessLevel, access);
    }

	virtual TCommandResult Execute( CClient* pClient, int argc, const char** argv );

    virtual void PrintCommand( edict_t* pPrintTo, int indent = 0);

#if defined(BOTRIX_NO_COMMAND_COMPLETION)
#elif defined(BOTRIX_OLD_COMMAND_COMPLETION)
    virtual int AutoComplete( const char* partial, int partialLength,
                              char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ],
                              int strIndex, int charIndex );
#else
    virtual int AutoComplete( good::string& partial, CUtlVector<CUtlString>& cCommands, int charIndex );
#endif

protected:
    CConsoleCommand() : m_iAccessLevel(FCommandAccessNone) {}

    good::string m_sCommand;
    int m_iAccessLevel;

    good::string m_sHelp;
    good::string m_sDescription;

	good::vector<TConsoleAutoCompleteArg> m_cAutoCompleteArguments;
	good::vector<StringVector> m_cAutoCompleteValues;
};


//****************************************************************************************************************
/// Container of commands.
//****************************************************************************************************************
class CConsoleCommandContainer: public CConsoleCommand
{
public:
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );

    void Add( CConsoleCommand* newCommand ) { m_aCommands.push_back( good::unique_ptr<CConsoleCommand>(newCommand) ); }
    virtual void PrintCommand( edict_t* pPrintTo, int indent = 0);

#if defined(BOTRIX_NO_COMMAND_COMPLETION)
#elif defined(BOTRIX_OLD_COMMAND_COMPLETION)
    virtual int AutoComplete( const char* partial, int partialLength,
                              char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ],
                              int strIndex, int charIndex );
#else
    virtual int AutoComplete( good::string& partial, CUtlVector< CUtlString > &commands, int charIndex );
#endif

protected:
    good::vector< good::unique_ptr<CConsoleCommand> > m_aCommands;
};



//****************************************************************************************************************
// Waypoint commands.
//****************************************************************************************************************
class CWaypointDrawFlagCommand: public CConsoleCommand
{
public:
    CWaypointDrawFlagCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointResetCommand: public CConsoleCommand
{
public:
    CWaypointResetCommand()
    {
        m_sCommand = "reset";
        m_sHelp = "reset current waypoint to nearest";
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointCreateCommand: public CConsoleCommand
{
public:
    CWaypointCreateCommand()
    {
        m_sCommand = "create";
        m_sHelp = "create new waypoint at current player's position";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointRemoveCommand: public CConsoleCommand
{
public:
	CWaypointRemoveCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointMoveCommand: public CConsoleCommand
{
public:
    CWaypointMoveCommand()
    {
        m_sCommand = "move";
        m_sHelp = "moves current or given waypoint to player's position";
		m_sDescription = "Parameter: (waypoint), current waypoint is used if omitted";
        m_iAccessLevel = FCommandAccessWaypoint;

		m_cAutoCompleteArguments.push_back(EConsoleAutoCompleteArgWaypoint);
		m_cAutoCompleteValues.push_back(StringVector());
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAutoCreateCommand: public CConsoleCommand
{
public:
    CWaypointAutoCreateCommand()
    {
        m_sCommand = "autocreate";
        m_sHelp = "automatically create new waypoints ('off' - disable, 'on' - enable)";
        m_sDescription = "Waypoint will be added when player goes too far from current one.";
        m_iAccessLevel = FCommandAccessWaypoint;
		
		m_cAutoCompleteArguments.push_back(EConsoleAutoCompleteArgBool);
		m_cAutoCompleteValues.push_back(StringVector());
	}

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointInfoCommand: public CConsoleCommand
{
public:
    CWaypointInfoCommand()
    {
        m_sCommand = "info";
        m_sHelp = "display information for the needed waypoint";

		m_cAutoCompleteArguments.push_back(EConsoleAutoCompleteArgWaypointForever);
		m_cAutoCompleteValues.push_back(StringVector());
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointDestinationCommand: public CConsoleCommand
{
public:
    CWaypointDestinationCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointSaveCommand: public CConsoleCommand
{
public:
    CWaypointSaveCommand()
    {
        m_sCommand = "save";
        m_sHelp = "save waypoints";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointLoadCommand: public CConsoleCommand
{
public:
    CWaypointLoadCommand()
    {
        m_sCommand = "load";
        m_sHelp = "load waypoints";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointClearCommand: public CConsoleCommand
{
public:
    CWaypointClearCommand()
    {
        m_sCommand = "clear";
        m_sHelp = "delete all waypoints";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAddTypeCommand: public CConsoleCommand
{
public:
    CWaypointAddTypeCommand()
    {
        m_sCommand = "addtype";
        m_sHelp = "add type to waypoint";
        m_sDescription = good::string("Can be mix of: ") + CTypeToString::WaypointFlagsToString(FWaypointAll);
        m_iAccessLevel = FCommandAccessWaypoint;

		StringVector args;
        for ( int i=0; i < EWaypointFlagTotal; ++i )
			args.push_back( CTypeToString::WaypointFlagsToString(1 << i).duplicate() );

		m_cAutoCompleteArguments.push_back(EConsoleAutoCompleteArgValuesForever);
		m_cAutoCompleteValues.push_back(args);
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAnalyzeToggleCommand: public CConsoleCommand
{
public:
    CWaypointAnalyzeToggleCommand()
    {
        m_sCommand = "toggle";
        m_sHelp = "start / stop analyzing waypoints for current map";
        m_sDescription = "This is a time consuming operation, so be patient.";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAnalyzeCreateCommand: public CConsoleCommand
{
public:
    CWaypointAnalyzeCreateCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAnalyzeDebugCommand: public CConsoleCommand
{
public:
    CWaypointAnalyzeDebugCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAnalyzeOmitCommand: public CConsoleCommand
{
public:
    CWaypointAnalyzeOmitCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAnalyzeTraceCommand: public CConsoleCommand
{
public:
    CWaypointAnalyzeTraceCommand()
    {
        m_sCommand = "trace";
        m_sHelp = "ray trace moveable entities (such as objects) during map analyze";
        m_sDescription = "Parameter: (on / off). When 'off', ray tracing won't hit any moveable entities (such as objects), "
            "so waypoints will be placed 'inside' those entities. But when it is 'on', there may be troubles with analyze.";
        m_iAccessLevel = FCommandAccessWaypoint;

        m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBool );
        m_cAutoCompleteValues.push_back( StringVector() );
    }
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAnalyzeCommand: public CConsoleCommandContainer
{
public:
    CWaypointAnalyzeCommand()
    {
        m_sCommand = "analyze";
        Add( new CWaypointAnalyzeToggleCommand );
        Add( new CWaypointAnalyzeCreateCommand );
        Add( new CWaypointAnalyzeDebugCommand );
        Add( new CWaypointAnalyzeOmitCommand );
        Add( new CWaypointAnalyzeTraceCommand );
    }
};

class CWaypointRemoveTypeCommand: public CConsoleCommand
{
public:
    CWaypointRemoveTypeCommand()
    {
        m_sCommand = "removetype";
        m_sHelp = "remove all types from current or given waypoint";
        m_iAccessLevel = FCommandAccessWaypoint;

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgWaypoint );
		m_cAutoCompleteValues.push_back( StringVector() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointArgumentCommand: public CConsoleCommand
{
public:
    CWaypointArgumentCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointVisibilityCommand: public CConsoleCommand
{
public:
    CWaypointVisibilityCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


//****************************************************************************************************************
// Area waypoint commands.
//****************************************************************************************************************
class CWaypointAreaRemoveCommand: public CConsoleCommand
{
public:
    CWaypointAreaRemoveCommand()
    {
        m_sCommand = "remove";
        m_sHelp = "delete waypoint area";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAreaRenameCommand: public CConsoleCommand
{
public:
    CWaypointAreaRenameCommand()
    {
        m_sCommand = "rename";
        m_sHelp = "rename waypoint area";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAreaSetCommand: public CConsoleCommand
{
public:
    CWaypointAreaSetCommand()
    {
        m_sCommand = "set";
        m_sHelp = "set waypoint area";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CWaypointAreaShowCommand: public CConsoleCommand
{
public:
    CWaypointAreaShowCommand()
    {
        m_sCommand = "show";
        m_sHelp = "print all waypoint areas";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

//****************************************************************************************************************
// Path waypoint commands.
//****************************************************************************************************************
class CPathDebugCommand: public CConsoleCommand
{
public:
    CPathDebugCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathDistanceCommand: public CConsoleCommand
{
public:
    CPathDistanceCommand()
    {
        m_sCommand = "distance";
        m_sHelp = "set distance to add default paths & auto add waypoints";
        m_iAccessLevel = FCommandAccessWaypoint;
    }
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathDrawCommand: public CConsoleCommand
{
public:
    CPathDrawCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathCreateCommand: public CConsoleCommand
{
public:
    CPathCreateCommand()
    {
        m_sCommand = "create";
        m_sHelp = "create path (from 'current' waypoint to 'destination')";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathRemoveCommand: public CConsoleCommand
{
public:
    CPathRemoveCommand()
    {
        m_sCommand = "remove";
        m_sHelp = "remove given path (or from 'current' waypoint to 'destination')";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathAutoCreateCommand: public CConsoleCommand
{
public:
    CPathAutoCreateCommand()
    {
        m_sCommand = "autocreate";
        m_sHelp = "enable auto path creation for new waypoints ('off' - disable, 'on' - enable)";
        m_sDescription = "If disabled, only path from 'destination' to new waypoint will be added";
        m_iAccessLevel = FCommandAccessWaypoint;

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBool );
		m_cAutoCompleteValues.push_back( StringVector() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

/*
class CPathSwapCommand: public CConsoleCommand
{
public:
    CPathSwapCommand()
    {
        m_sCommand = "swap";
        m_sHelp = "set current waypoint as 'destination' and teleport to old 'destination'";
        m_sDescription = "If argument is provided, then teleport there";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};
*/

class CPathAddTypeCommand: public CConsoleCommand
{
public:
    CPathAddTypeCommand()
    {
        m_sCommand = "addtype";
        m_sHelp = "add path type (from 'current' waypoint to 'destination').";
        m_sDescription = good::string("Can be mix of: ") + CTypeToString::PathFlagsToString(FPathAll);
        m_iAccessLevel = FCommandAccessWaypoint;

		StringVector args;
        for ( int i=0; i < EPathFlagUserTotal; ++i )
            args.push_back( CTypeToString::PathFlagsToString(1<<i).duplicate() );

		m_cAutoCompleteArguments.push_back(EConsoleAutoCompleteArgValuesForever);
		m_cAutoCompleteValues.push_back(args);
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathRemoveTypeCommand: public CConsoleCommand
{
public:
    CPathRemoveTypeCommand()
    {
        m_sCommand = "removetype";
        m_sHelp = "remove path type (from 'current' waypoint to 'destination')";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathArgumentCommand: public CConsoleCommand
{
public:
	CPathArgumentCommand();
	TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CPathInfoCommand: public CConsoleCommand
{
public:
    CPathInfoCommand()
    {
        m_sCommand = "info";
        m_sHelp = "display path info on console (from 'current' waypoint to 'destination')";
        m_iAccessLevel = FCommandAccessWaypoint;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


//****************************************************************************************************************
// Weapon commands.
//****************************************************************************************************************
class CBotWeaponCommand: public CConsoleCommand
{
public:
	CBotWeaponCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotWeaponAllowCommand: public CConsoleCommand
{
public:
	CConfigBotWeaponAllowCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotWeaponDefaultCommand: public CConsoleCommand
{
public:
	CConfigBotWeaponDefaultCommand();
	TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotWeaponRemoveCommand: public CConsoleCommand
{
public:
	CConfigBotWeaponRemoveCommand();
	TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotWeaponRemoveCommand: public CConsoleCommand
{
public:
    CBotWeaponRemoveCommand()
    {
        m_sCommand = "remove";
        m_sHelp = "remove all weapons from bot";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotWeaponUnknownCommand: public CConsoleCommand
{
public:
	CConfigBotWeaponUnknownCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


//****************************************************************************************************************
// Bot commands.
//****************************************************************************************************************
class CBotAddCommand: public CConsoleCommand
{
public:
	CBotAddCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotCommandCommand: public CConsoleCommand
{
public:
    CBotCommandCommand()
    {
        m_sCommand = "command";
        m_sHelp = "execute console command by bot";
        m_sDescription = "Parameters: <command> <bot-name(s)>. Example: 'botrix bot command \"jointeam 2\" all'.";
        m_iAccessLevel = FCommandAccessBot;

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgIgnore );
		m_cAutoCompleteValues.push_back( StringVector() );

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBotsForever );
		m_cAutoCompleteValues.push_back( StringVector() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotKickCommand: public CConsoleCommand
{
public:
    CBotKickCommand()
    {
        m_sCommand = "kick";
        m_sHelp = "kick bot";
        m_sDescription = "Parameters: (bot-name) will kick random / given bot(s).";
        m_iAccessLevel = FCommandAccessBot;

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBotsForever );
		m_cAutoCompleteValues.push_back( StringVector() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotDebugCommand: public CConsoleCommand
{
public:
    CBotDebugCommand()
    {
        m_sCommand = "debug";
        m_sHelp = "show bot debug messages on server";
        m_sDescription = "Parameters: <on/off> <bot-name(s)>.";
        m_iAccessLevel = FCommandAccessBot;

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBool );
		m_cAutoCompleteValues.push_back( StringVector() );

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBotsForever );
		m_cAutoCompleteValues.push_back( StringVector() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotQuotaCommand: public CConsoleCommand
{
public:
    CConfigBotQuotaCommand()
    {
        m_sCommand = "quota";
        m_sHelp = "set bots+players quota.";
        m_sDescription = "You can use 'n-m' to have m bots per n players. Set to 0 to disable quota.";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotIntelligenceCommand: public CConsoleCommand
{
public:
    CConfigBotIntelligenceCommand()
    {
        m_sCommand = "intelligence";
		m_sHelp = "set min/max bot intelligence";
		m_sDescription = "Parameters: <min> (max). Can be one of: random fool stupied normal smart pro";// TODO: intelligence flags to string.
        m_iAccessLevel = FCommandAccessBot;
        
		StringVector args;
		for ( int i = 0; i < EBotIntelligenceTotal; ++i )
			args.push_back( CTypeToString::IntelligenceToString(i).duplicate() );

		m_cAutoCompleteArguments.push_back(EConsoleAutoCompleteArgValuesForever);
		m_cAutoCompleteValues.push_back(args);
	}

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotTeamCommand: public CConsoleCommand
{
public:
    CConfigBotTeamCommand()
    {
        m_sCommand = "team";
        m_sHelp = "set default bot team";
        m_sDescription = good::string("Can be one of: ") + CTypeToString::TeamFlagsToString(-1);
        m_iAccessLevel = FCommandAccessBot;

		StringVector args;
		args.push_back("random");
		for ( int i = 0; i < CMod::aTeamsNames.size(); ++i )
			args.push_back( CTypeToString::TeamToString(i).duplicate() );

		m_cAutoCompleteArguments.push_back(EConsoleAutoCompleteArgValues);
		m_cAutoCompleteValues.push_back(args);
	}

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


class CConfigBotProtectionHealthCommand: public CConsoleCommand
{
public:
	CConfigBotProtectionHealthCommand();
	TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


class CConfigBotProtectionSpawnTimeCommand: public CConsoleCommand
{
public:
	CConfigBotProtectionSpawnTimeCommand();
	TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotProtectionCommand: public CConsoleCommandContainer
{
public:
	CConfigBotProtectionCommand()
	{
		m_sCommand = "protection";
		Add( new CConfigBotProtectionHealthCommand );
		Add( new CConfigBotProtectionSpawnTimeCommand );
	}
};

class CConfigBotClassCommand: public CConsoleCommand
{
public:
    CConfigBotClassCommand()
    {
        m_sCommand = "class";
        m_sHelp = "set default bot class";
        m_sDescription = good::string("Can be one of: random ") + CTypeToString::ClassFlagsToString(-1);
        m_iAccessLevel = FCommandAccessBot;

		StringVector args;
		args.push_back("random");
		for ( int i = 0; i < CMod::aClassNames.size(); ++i )
			args.push_back( CTypeToString::ClassToString(i).duplicate() );

		m_cAutoCompleteArguments.push_back(EConsoleAutoCompleteArgValues);
		m_cAutoCompleteValues.push_back(args);
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotChangeClassCommand: public CConsoleCommand
{
public:
    CConfigBotChangeClassCommand()
    {
        m_sCommand = "change-class";
        m_sHelp = "change bot class to another random class after x rounds.";
        m_sDescription = "Set to 0 to disable.";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotSuicideCommand: public CConsoleCommand
{
public:
    CConfigBotSuicideCommand()
    {
        m_sCommand = "suicide";
        m_sHelp = "when staying far from waypoints for this time (in seconds), suicide";
        m_sDescription = "Set to 0 to disable.";
        m_iAccessLevel = FCommandAccessBot;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotStrategyFlagsCommand: public CConsoleCommand
{
public:
    CConfigBotStrategyFlagsCommand()
    {
        m_sCommand = "flags";
        m_sHelp = "set bot fight strategy flags";
        m_sDescription = good::string("Can be mix of: ") + CTypeToString::StrategyFlagsToString(FFightStrategyAll);
        m_iAccessLevel = FCommandAccessBot;

		StringVector args;
		for ( int i = 0; i < EFightStrategyFlagTotal; ++i )
			args.push_back( CTypeToString::StrategyFlagsToString(1 << i).duplicate() );

		m_cAutoCompleteArguments.push_back(EConsoleAutoCompleteArgValuesForever);
		m_cAutoCompleteValues.push_back(args);
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotStrategySetCommand: public CConsoleCommand
{
public:
    CConfigBotStrategySetCommand()
    {
        m_sCommand = "set";
        m_sHelp = "set bot fight strategy argument";
        m_sDescription = "Parameters: <near-distance/far-distance> <distance>.";
        m_iAccessLevel = FCommandAccessBot;

		StringVector args;
		for ( int i = 0; i < EFightStrategyArgTotal; ++i )
			args.push_back( CTypeToString::StrategyArgToString(i).duplicate() );

		m_cAutoCompleteArguments.push_back(EConsoleAutoCompleteArgValues);
		m_cAutoCompleteValues.push_back(args);	
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigBotStrategyCommand: public CConsoleCommandContainer
{
public:
    CConfigBotStrategyCommand()
    {
        m_sCommand = "strategy";
        Add(new CConfigBotStrategyFlagsCommand);
        Add(new CConfigBotStrategySetCommand);
    }
};

class CConfigBotWeaponCommand: public CConsoleCommandContainer
{
public:
	CConfigBotWeaponCommand()
	{
		m_sCommand = "weapon";
		Add( new CConfigBotWeaponAllowCommand );
		Add( new CConfigBotWeaponDefaultCommand );
		Add( new CConfigBotWeaponRemoveCommand );
		Add( new CConfigBotWeaponUnknownCommand );
	}
};

class CConfigBotCommand: public CConsoleCommandContainer
{
public:
    CConfigBotCommand()
    {
        m_sCommand = "bot";
		if ( CMod::aClassNames.size() )
		{
			Add( new CConfigBotClassCommand );
			Add( new CConfigBotChangeClassCommand );
		}
		Add( new CConfigBotIntelligenceCommand );
		Add( new CConfigBotProtectionCommand );
		Add( new CConfigBotQuotaCommand );
		Add( new CConfigBotStrategyCommand );
		Add( new CConfigBotSuicideCommand );
		Add( new CConfigBotTeamCommand );
		Add( new CConfigBotWeaponCommand );
	}
};

class CBotAllyCommand: public CConsoleCommand
{
public:
    CBotAllyCommand()
    {
        m_sCommand = "ally";
        m_sHelp = "given bot won't attack another given player";
        m_sDescription = "Parameters: <player-name> <on/off> <bot-name(s)>.";
        m_iAccessLevel = FCommandAccessBot;

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgPlayers );
		m_cAutoCompleteValues.push_back( StringVector() );

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBool );
		m_cAutoCompleteValues.push_back( StringVector() );

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBotsForever );
		m_cAutoCompleteValues.push_back( StringVector() );
	}

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotAttackCommand: public CConsoleCommand
{
public:
    CBotAttackCommand()
    {
        m_sCommand = "attack";
        m_sHelp = "forces bot to start/stop attacking";
		m_sDescription = "Parameters: <on/off> <bot-name(s)>.";
		m_iAccessLevel = FCommandAccessBot;

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBool );
		m_cAutoCompleteValues.push_back( StringVector() );

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBotsForever );
		m_cAutoCompleteValues.push_back( StringVector() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotMoveCommand: public CConsoleCommand
{
public:
    CBotMoveCommand()
    {
        m_sCommand = "move";
        m_sHelp = "forces bot to start/stop moving";
		m_sDescription = "Parameters: <on/off> <bot-name(s)>.";
        m_iAccessLevel = FCommandAccessBot;

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBool );
		m_cAutoCompleteValues.push_back( StringVector() );

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBotsForever );
		m_cAutoCompleteValues.push_back( StringVector() );
	}

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotPauseCommand: public CConsoleCommand
{
public:
    CBotPauseCommand()
    {
        m_sCommand = "pause";
        m_sHelp = "pause/resume given bots";
		m_sDescription = "Parameters: <on/off> <bot-name(s)>.";
        m_iAccessLevel = FCommandAccessBot;
	
		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBool );
		m_cAutoCompleteValues.push_back( StringVector() );

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgBotsForever );
		m_cAutoCompleteValues.push_back( StringVector() );
	}

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


class CBotProtectCommand : public CConsoleCommand
{
public:
	CBotProtectCommand();
	TCommandResult Execute(CClient* pClient, int argc, const char** argv);
};

class CBotDrawPathCommand: public CConsoleCommand
{
public:
	CBotDrawPathCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CBotTestPathCommand: public CConsoleCommand
{
public:
    CBotTestPathCommand()
    {
        m_sCommand = "test";
        m_sHelp = "create bot to test a path";
		m_sDescription = "Parameters: (waypoint-from) (waypoint-to). Default waypoint-from is 'current', waypoint-to is 'destination'";
        m_iAccessLevel = FCommandAccessBot;

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgWaypoint );
		m_cAutoCompleteValues.push_back( StringVector() );

		m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgWaypoint );
		m_cAutoCompleteValues.push_back( StringVector() );
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


//****************************************************************************************************************
// Item commands.
//****************************************************************************************************************
class CItemDrawCommand: public CConsoleCommand
{
public:
	CItemDrawCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


class CItemDrawTypeCommand: public CConsoleCommand
{
public:
	CItemDrawTypeCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CItemMarkCommand: public CConsoleCommand
{
public:
    CItemMarkCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CItemReloadCommand: public CConsoleCommand
{
public:
    CItemReloadCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};


//****************************************************************************************************************
// Config commands.
//****************************************************************************************************************
class CConfigAdminsSetAccessCommand: public CConsoleCommand
{
public:
	CConfigAdminsSetAccessCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigAdminsShowCommand: public CConsoleCommand
{
public:
    CConfigAdminsShowCommand()
    {
        m_sCommand = "show";
        m_sHelp = "show admins currently on server";
        m_iAccessLevel = FCommandAccessConfig;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );

};

class CConfigEventsCommand: public CConsoleCommand
{
public:
    CConfigEventsCommand()
    {
        m_sCommand = "event";
        m_sHelp = "display events on console ('off' - disable, 'on' - enable)";
        m_iAccessLevel = FCommandAccessConfig;

		m_cAutoCompleteArguments.push_back(EConsoleAutoCompleteArgBool);
		m_cAutoCompleteValues.push_back(StringVector());
	}

    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigLogCommand: public CConsoleCommand
{
public:
	CConfigLogCommand();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigWaypointAnalyzeAmount: public CConsoleCommand
{
public:
    CConfigWaypointAnalyzeAmount()
    {
        m_sCommand = "amount";
        m_sHelp = "amount of waypoints to analyze per frame";
        m_sDescription = "Parameter: number of waypoints to analyze per frame. Can be fractional.";
        m_iAccessLevel = FCommandAccessConfig;
    }
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigWaypointAnalyzeMapChange: public CConsoleCommand
{
public:
    CConfigWaypointAnalyzeMapChange()
    {
        m_sCommand = "map-change";
        m_sHelp = "force analyze waypoints on map change";
        m_sDescription = "Parameter: maximum number of waypoints to start analyze on map change. 'off' or -1 to disable.";
        m_iAccessLevel = FCommandAccessConfig;

        StringVector args;
        args.push_back( "off" );
        m_cAutoCompleteArguments.push_back( EConsoleAutoCompleteArgValues );
        m_cAutoCompleteValues.push_back( args );
    }
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigWaypointAnalyzeDistance: public CConsoleCommand
{
public:
    CConfigWaypointAnalyzeDistance()
    {
        m_sCommand = "distance";
        m_sHelp = "default distance between waypoints when analyzing the map";
        m_iAccessLevel = FCommandAccessConfig;
    }
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigWaypointAnalyze: public CConsoleCommandContainer
{
public:
    CConfigWaypointAnalyze() {
		m_sCommand = "analyze";
        m_aCommands.push_back( new CConfigWaypointAnalyzeAmount );
        m_aCommands.push_back( new CConfigWaypointAnalyzeDistance );
        m_aCommands.push_back( new CConfigWaypointAnalyzeMapChange );
    }
};

class CConfigWaypointSave: public CConsoleCommand
{
public:
    CConfigWaypointSave();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigWaypointUnreachable: public CConsoleCommand
{
public:
    CConfigWaypointUnreachable();
    TCommandResult Execute( CClient* pClient, int argc, const char** argv );
};

class CConfigWaypoint: public CConsoleCommandContainer
{
public:
    CConfigWaypoint()
    {
        m_sCommand = "waypoint";
        m_aCommands.push_back( new CConfigWaypointAnalyze );
        m_aCommands.push_back( new CConfigWaypointSave );
        m_aCommands.push_back( new CConfigWaypointUnreachable );
    }
};
//****************************************************************************************************************
// Admins: show admins and set admin flags.
//****************************************************************************************************************
class CConfigAdminsCommand: public CConsoleCommandContainer
{
public:
    CConfigAdminsCommand()
    {
        m_sCommand = "admins";
        Add(new CConfigAdminsSetAccessCommand);
        Add(new CConfigAdminsShowCommand);
    }
};


//****************************************************************************************************************
// Waypoint areas: show/set/rename waypoint area.
//****************************************************************************************************************
class CWaypointAreaCommand: public CConsoleCommandContainer
{
public:
    CWaypointAreaCommand()
    {
        m_sCommand = "area";
        Add(new CWaypointAreaRemoveCommand);
        Add(new CWaypointAreaRenameCommand);
        Add(new CWaypointAreaSetCommand);
        Add(new CWaypointAreaShowCommand);
    }
};


//****************************************************************************************************************
// Container of all commands starting with "waypoint".
//****************************************************************************************************************
class CWaypointCommand: public CConsoleCommandContainer
{
public:
    CWaypointCommand()
    {
        m_sCommand = "waypoint";
		Add( new CWaypointAddTypeCommand );
		Add( new CWaypointAnalyzeCommand );
		Add(new CWaypointAreaCommand);
        Add(new CWaypointArgumentCommand);
        Add(new CWaypointAutoCreateCommand);
        Add(new CWaypointClearCommand);
        Add(new CWaypointCreateCommand);
        Add(new CWaypointDestinationCommand);
        Add(new CWaypointDrawFlagCommand);
        Add(new CWaypointInfoCommand);
        Add(new CWaypointLoadCommand);
        Add(new CWaypointMoveCommand);
        Add(new CWaypointRemoveCommand);
        Add(new CWaypointRemoveTypeCommand);
        Add(new CWaypointResetCommand);
        Add(new CWaypointSaveCommand);
        Add(new CWaypointVisibilityCommand);
    }
};


//****************************************************************************************************************
// Container of all commands starting with "pathwaypoint".
//****************************************************************************************************************
class CPathCommand: public CConsoleCommandContainer
{
public:
    CPathCommand()
    {
        m_sCommand = "path";
        Add( new CPathAutoCreateCommand );
        Add( new CPathAddTypeCommand );
        Add( new CPathArgumentCommand );
        Add( new CPathCreateCommand );
        Add( new CPathDebugCommand );
        Add( new CPathDistanceCommand );
        Add( new CPathDrawCommand );
        Add( new CPathInfoCommand );
        //Add(new CPathSwapCommand);
        Add( new CPathRemoveCommand );
        Add( new CPathRemoveTypeCommand );
    }
};


//****************************************************************************************************************
// Container of all commands starting with "item".
//****************************************************************************************************************
class CItemCommand: public CConsoleCommandContainer
{
public:
    CItemCommand()
    {
        m_sCommand = "item";
        Add( new CItemDrawCommand );
        Add( new CItemDrawTypeCommand );
        Add( new CItemMarkCommand );
        Add( new CItemReloadCommand );
    }
};


//****************************************************************************************************************
// Container of all commands starting with "bot".
//****************************************************************************************************************
class CBotCommand: public CConsoleCommandContainer
{
public:
    CBotCommand()
    {
        m_sCommand = "bot";
        Add(new CBotAddCommand);
        Add(new CBotAllyCommand);
        Add(new CBotAttackCommand);
        Add(new CBotCommandCommand);
        Add(new CBotDebugCommand);
        Add(new CBotDrawPathCommand);
        Add(new CBotKickCommand);
        Add(new CBotMoveCommand);
		Add(new CBotPauseCommand);
		Add(new CBotProtectCommand);
		if (CMod::GetModId() != EModId_TF2) // TF2 bots can't be spawned after round has started.
            Add(new CBotTestPathCommand);
		Add( new CBotWeaponCommand );
		//Add(new CBotWeaponRemoveCommand);
	}
};


//****************************************************************************************************************
// Container of all commands starting with "config".
//****************************************************************************************************************
class CConfigCommand: public CConsoleCommandContainer
{
public:
    CConfigCommand()
    {
        m_sCommand = "config";
        Add(new CConfigAdminsCommand);
		Add(new CConfigBotCommand);
        Add(new CConfigEventsCommand);
        Add(new CConfigLogCommand);
		Add(new CConfigWaypoint);
	}
};


//****************************************************************************************************************
// Command "enable".
//****************************************************************************************************************
class CEnableCommand: public CConsoleCommand
{
public:
    CEnableCommand()
    {
        m_sCommand = "enable";
        m_sHelp = "enable plugin";
        m_iAccessLevel = FCommandAccessConfig;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv )
    {
		if ( CConsoleCommand::Execute( pClient, argc, argv ) == ECommandPerformed )
			return ECommandPerformed;

		edict_t* pEdict = pClient ? pClient->GetEdict() : NULL;
        if ( argc )
        {
            BULOG_W( pEdict, "Error, invalid parameters count." );
            return ECommandError;
        }
        else
        {
            CBotrixPlugin::instance->Enable(true);
            BULOG_I( pEdict, "Plugin enabled." );
            return ECommandPerformed;
        }
    }
};


// Command "disable".
class CDisableCommand: public CConsoleCommand
{
public:
    CDisableCommand()
    {
        m_sCommand = "disable";
        m_sHelp = "disable plugin";
        m_iAccessLevel = FCommandAccessConfig;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv )
    {
		if ( CConsoleCommand::Execute( pClient, argc, argv ) == ECommandPerformed )
			return ECommandPerformed;

		edict_t* pEdict = pClient ? pClient->GetEdict() : NULL;
        if ( argc )
        {
            BULOG_W( pEdict, "Error, invalid parameters count." );
            return ECommandError;
        }
        else
        {
            CBotrixPlugin::instance->Enable(false);
            BULOG_I( pEdict, "Plugin disabled." );
            return ECommandPerformed;
        }
    }
};


//****************************************************************************************************************
// Command "version".
//****************************************************************************************************************
class CVersionCommand: public CConsoleCommand
{
public:
	CVersionCommand(): CConsoleCommand()
    {
        m_sCommand = "version";
        m_sHelp = "display plugin version";
        m_iAccessLevel = FCommandAccessConfig;
    }

    TCommandResult Execute( CClient* pClient, int argc, const char** argv )
    {
		if ( CConsoleCommand::Execute( pClient, argc, argv ) == ECommandPerformed )
			return ECommandPerformed;

		edict_t* pEdict = pClient ? pClient->GetEdict() : NULL;
        BULOG_I( pEdict, "Version " PLUGIN_VERSION );
        return ECommandPerformed;
    }
};


//****************************************************************************************************************
/// Container of all commands starting with "botrix".
//****************************************************************************************************************
class CBotrixCommand: public CConsoleCommandContainer
#if !defined(BOTRIX_NO_COMMAND_COMPLETION) && !defined(BOTRIX_OLD_COMMAND_COMPLETION)
    , public ICommandCompletionCallback, public ICommandCallback
#endif
{
public:
    /// Contructor.
    CBotrixCommand();

    /// Destructor.
    ~CBotrixCommand();

#if !defined(BOTRIX_NO_COMMAND_COMPLETION) && !defined(BOTRIX_OLD_COMMAND_COMPLETION)
    /// Execute "botrix" command on server (not as client).
    virtual void CommandCallback( const CCommand &command );

    /// Autocomplete "botrix" command on server (not as client).
    virtual int CommandCompletionCallback( const char *pPartial, CUtlVector< CUtlString > &commands )
    {
        good::string sPartial(pPartial, true, true);
        return AutoComplete(sPartial, commands, 0);
    }
#endif

    static good::unique_ptr<CBotrixCommand> instance; ///< Singleton of this class.

protected:
	static good::unique_ptr<ConCommand> m_pServerCommand;
};


#endif // __BOTRIX_CONSOLE_COMMANDS_H__
