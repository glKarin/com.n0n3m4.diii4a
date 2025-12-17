#ifndef __BOTRIX_TYPE_TO_STRING_H__
#define __BOTRIX_TYPE_TO_STRING_H__


#include "types.h"


enum {
	BoolStringTrueFalse = 0,
	BoolStringYesNo,
	BoolStringOnOff,
	BoolStringEnableDisable,
};
typedef int TBoolString;


//****************************************************************************************************************
/// Useful class to get type from string and viceversa.
//****************************************************************************************************************
class CTypeToString
{
public:

    //------------------------------------------------------------------------------------------------------------
    /// Get string from array of strings.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& StringArrayToString( const good::string aStrings[], int iSize );


    //------------------------------------------------------------------------------------------------------------
    /// Get bool from string (on/off). -1 = invalid string.
    //------------------------------------------------------------------------------------------------------------
    static int BoolFromString( const good::string& sBool );

    /// Get string from bool.
	static const good::string& BoolToString( bool b, int which = BoolStringTrueFalse );


    //------------------------------------------------------------------------------------------------------------
    /// Get mod id from string.
    //------------------------------------------------------------------------------------------------------------
    static TModId ModFromString( const good::string& sMod );

    /// Get string from mod id.
    static const good::string& ModToString( TModId iMod );


    //------------------------------------------------------------------------------------------------------------
    /// Get mod var from string.
    //------------------------------------------------------------------------------------------------------------
    static TModVar ModVarFromString( const good::string& sVar );

    /// Get string from mod var.
    static const good::string& ModVarToString( TModVar iVar );


    //------------------------------------------------------------------------------------------------------------
    /// Get access flags from string.
    //------------------------------------------------------------------------------------------------------------
    static int AccessFlagsFromString( const good::string& sFlags );

    /// Get string from access flags.
    static const good::string& AccessFlagsToString( TCommandAccessFlags iFlags, bool bUseNone = true );


    //------------------------------------------------------------------------------------------------------------
    /// Get waypoint flags from string.
    //------------------------------------------------------------------------------------------------------------
    static int WaypointFlagsFromString( const good::string& sFlags );

    /// Get string from waypoint flags.
    static const good::string& WaypointFlagsToString( TWaypointFlags iFlags, bool bUseNone = true );


    //------------------------------------------------------------------------------------------------------------
    /// Get path flags from string.
    //------------------------------------------------------------------------------------------------------------
    static int PathFlagsFromString( const good::string& sFlags );

    /// Get string from path flags.
    static const good::string& PathFlagsToString( TPathFlags iFlags, bool bUseNone = true );


    //------------------------------------------------------------------------------------------------------------
    /// Get waypoint draw types from string.
    //------------------------------------------------------------------------------------------------------------
    static int WaypointDrawFlagsFromString( const good::string& sFlags );

    /// Get string from waypoint draw types.
    static const good::string& WaypointDrawFlagsToString( TWaypointDrawFlags iFlags, bool bUseNone = true );


    //------------------------------------------------------------------------------------------------------------
    /// Get path draw types from string.
    //------------------------------------------------------------------------------------------------------------
    static int PathDrawFlagsFromString( const good::string& sFlags );

    /// Get string from waypoint draw flags.
    static const good::string& PathDrawFlagsToString( TPathDrawFlags iFlags, bool bUseNone = true );


    //------------------------------------------------------------------------------------------------------------
    /// Get item type from string.
    //------------------------------------------------------------------------------------------------------------
    static int EntityTypeFromString( const good::string& sType );

    /// Get string from item type.
    static const good::string& EntityTypeToString( TItemType iType );


    //------------------------------------------------------------------------------------------------------------
    /// Get item type flag from string. Used to draw entities on map.
    //------------------------------------------------------------------------------------------------------------
    static int EntityTypeFlagsFromString( const good::string& sFlag );

    /// Get string from item type flags.
    static const good::string& EntityTypeFlagsToString( TItemTypeFlags iItemTypeFlags, bool bUseNone = true );


    //------------------------------------------------------------------------------------------------------------
    /// Get item flags from string. Used to draw entities on map.
    //------------------------------------------------------------------------------------------------------------
    static int EntityClassFlagsFromString( const good::string& sFlags );

    /// Get string from item flags.
    static const good::string& EntityClassFlagsToString( TItemFlags iItemFlags, bool bUseNone = true );


    //------------------------------------------------------------------------------------------------------------
    /// Get item draw flags from string.
    //------------------------------------------------------------------------------------------------------------
    static int ItemDrawFlagsFromString( const good::string& sFlags );

    /// Get string from item draw flags.
    static const good::string& ItemDrawFlagsToString( TItemDrawFlags iFlags, bool bUseNone = true );


    //------------------------------------------------------------------------------------------------------------
    /// Get weapon type from string.
    //------------------------------------------------------------------------------------------------------------
    static int WeaponTypeFromString( const good::string& sType );

    /// Get string from weapon type.
    static const good::string& WeaponTypeToString( TWeaponType iType );


    //------------------------------------------------------------------------------------------------------------
    /// Get weapon flag from string.
    //------------------------------------------------------------------------------------------------------------
    static TWeaponFlags WeaponFlagsFromString( const good::string& sWeaponFlag );

    /// Get string from weapon flags.
    static const good::string& WeaponFlagsToString( TWeaponFlags iWeaponFlags, bool bUseNone = true );

    //------------------------------------------------------------------------------------------------------------
    /// Get weapon aim from string.
    //------------------------------------------------------------------------------------------------------------
    static TWeaponAim WeaponAimFromString( const good::string& sWeaponAim );

    /// Get string from weapon aim.
    static const good::string& WeaponAimToString( TWeaponAim iWeaponAim );

    //------------------------------------------------------------------------------------------------------------
    /// Get bot preference from string.
    //------------------------------------------------------------------------------------------------------------
    static int PreferenceFromString( const good::string& sPreference );

    /// Get string from bot preference .
    static const good::string& PreferenceToString( TBotIntelligence iPreference );


    //------------------------------------------------------------------------------------------------------------
    /// Get string from bot intelligence.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& IntelligenceToString( int iIntelligence );

    /// Get bot intelligence from string.
    static int IntelligenceFromString( const good::string& sIntelligence );


    //------------------------------------------------------------------------------------------------------------
    /// Get string from bot team.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& TeamToString( int iTeam );

    /// Team flags to string.
    static const good::string& TeamFlagsToString( int iTeams, bool bUseNone = true );

    /// Get team from string.
    static int TeamFromString( const good::string& sTeam );


    //------------------------------------------------------------------------------------------------------------
    /// Get string from bot class.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& ClassToString( int iClass );

    /// Get bot class from string.
    static const good::string& ClassFlagsToString( int iClasses, bool bUseNone = true );

    /// Get bot class from string.
    static int ClassFromString( const good::string& sClass );


    //------------------------------------------------------------------------------------------------------------
    /// Get bot task name.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& BotTaskToString( TBotTask iBotTask );

	//------------------------------------------------------------------------------------------------------------
	/// Get bot command from string.
	//------------------------------------------------------------------------------------------------------------
	static int BotCommandFromString( const good::string& sCommand );

	/// Get string from bot command.
	static const good::string& BotCommandToString( TBotChat iCommand );


	//------------------------------------------------------------------------------------------------------------
	/// Console command result to string.
	//------------------------------------------------------------------------------------------------------------
	static const good::string& ConsoleCommandResultToString( TCommandResult iCmdResult );


    //------------------------------------------------------------------------------------------------------------
    /// Get log level from string.
    //------------------------------------------------------------------------------------------------------------
    static int LogLevelFromString( const good::string& sLevel );

    /// Get string from log level.
    static const good::string& LogLevelToString( int iLevel );


    //------------------------------------------------------------------------------------------------------------
    /// Get strategy flags from string.
    //------------------------------------------------------------------------------------------------------------
    static int StrategyFlagsFromString( const good::string& sFlags );

    /// Get string from strategy flags.
    static const good::string& StrategyFlagsToString( int iFlags, bool bUseNone = true );

    //------------------------------------------------------------------------------------------------------------
    /// Get strategy argument from string.
    //------------------------------------------------------------------------------------------------------------
    static int StrategyArgFromString( const good::string& sArg );

    /// Get string from strategy argument.
    static const good::string& StrategyArgToString( int iArg );

    /// Get all strategy args.
    static const good::string& StrategyArgs();


#ifdef BOTRIX_BORZH
    //------------------------------------------------------------------------------------------------------------
    /// Get bot action from string.
    //------------------------------------------------------------------------------------------------------------
    static int BotActionFromString( const good::string& sAction );

    /// Get string from bot action.
    static const good::string& BotActionToString( int iAction );

    //------------------------------------------------------------------------------------------------------------
    /// Get string from borzh's task.
    //------------------------------------------------------------------------------------------------------------
    static const good::string& BorzhTaskToString( int iTask );
#endif

};

#endif // __BOTRIX_TYPE_TO_STRING_H__
