#ifndef __BOTRIX_MOD_H__
#define __BOTRIX_MOD_H__


#include <stdlib.h> // rand()
#include <good/ini_file.h>

#include "event.h"
#include "item.h"

//****************************************************************************************************************
/**
 * @brief Mod interface.
 *
 * Used to get strings, colors from types or flags. TODO:
 */
//****************************************************************************************************************
abstract class IMod
{
public: // Methods.

    // Virtual destructor.
    virtual ~IMod() {}

    /// Return last error.
    const good::string& GetLastError() { return m_sLastError; }

    /// Process configuration file.
    virtual bool ProcessConfig( const good::ini_file& cIni ) = 0;


    /// Called when map is loaded, after waypoints and items has been loaded.
    virtual void MapLoaded() = 0;

    /// Called when map is unloaded.
    virtual void MapFinished() = 0;

    /// Add bot with given name, intelligence, class and other optional parameters.
    virtual CPlayer* AddBot( const char* szName, TBotIntelligence iIntelligence, TTeam iTeam,
                             TClass iClass, int iParamsCount, const char **aParams ) = 0;


    /// Get waypoint type count.
    virtual int GetWaypointTypeCount() = 0;

    /// Get waypoint type names.
    virtual const good::string* GetWaypointTypeNames() = 0;

    /// Get waypoint type colors.
    virtual const int* GetWaypointTypeColors() = 0;


    /// Get waypoint path count.
    virtual int GetWaypointPathCount() = 0;

    /// Get waypoints path names.
    virtual const good::string* GetWaypointPathNames() = 0;

    /// Get waypoints path colors.
    virtual const int* GetWaypointPathColors() = 0;

    /// Mod think function.
    virtual void Think() = 0;

protected:
    good::string m_sLastError;

};


//****************************************************************************************************************
/// Class for Half Life 2 mod types. By default loads all needed stuff for Half-life 2 deathmatch mod.
//****************************************************************************************************************
class CMod
{

public: // Methods.

    static IMod* pCurrentMod;

    /// Get mod id for this game mod.
    static TModId GetModId() { return m_iModId; }

    /// Load all needed staff for mod.
    static bool LoadDefaults( TModId iModId );

    /// Prepare for use, called after all needed vars are set.
    static void Prepare();

    // Add event and start listening to it.
    static void AddEvent( CEvent* pEvent ) { m_aEvents.push_back( CEventPtr(pEvent) ); }

    /// Unload mod.
    static void UnLoad()
    {
        m_aEvents.clear();
        m_aFrameEvents.clear();

        m_iModId = EModId_Invalid;
        sModName = "";
        aTeamsNames.clear();
        aBotNames.clear();
        delete pCurrentMod;
        pCurrentMod = NULL;
    }

    /// Called when map finished loading items and waypoints.
    static void MapLoaded();

    /// Mod's think function.
    static void Think();

    /// Add frame event.
    static void AddFrameEvent( TPlayerIndex iPlayer, TFrameEvent iEvent )
    {
		good::pair<TFrameEvent, TPlayerIndex> pair(iEvent, iPlayer);
		if ( good::find(m_aFrameEvents, pair) == m_aFrameEvents.end() ) // Avoid firing events twice.
			m_aFrameEvents.push_back( pair );
    }

    /// Return true if map has items or waypoint's of given type.
    static bool HasMapItems( TItemType iEntityType ) { return m_bMapHas[iEntityType]; }

    /// Get team index from team name.
    static int GetTeamIndex( const good::string& sTeam )
    {
        for ( good::string::size_type i=0; i < aTeamsNames.size(); ++i )
            if ( sTeam == aTeamsNames[i] )
                return i;
        return -1;
    }

    /// Get random bot name from [General] section, key bot_names.
    static const good::string& GetRandomBotName( TBotIntelligence iIntelligence );

    /// Get var value for needed class.
    static float GetVar( TModVar iVar, TClass iClass = 0 )
    {
        GoodAssert( 0 <= iVar && iVar < EModVarTotal );
        int iIndex = iClass < m_aVars[ iVar ].size() ? iClass : 0;
        return m_aVars[ iVar ][ iIndex ];
    }

    static void SetVars( good::vector<float> *aVars )
    {
        for ( int iVar = 0; iVar < EModVarTotal; ++iVar )
            if ( aVars[ iVar ].size() > 0 )
                m_aVars[ iVar ] = aVars[ iVar ];
    }

public: // Static members.
    static good::string sModName;                   ///< Mod name.
    static StringVector aTeamsNames;                ///< Name of teams.
    static int iUnassignedTeam;                     ///< Index of unassigned (deathmatch) team.
    static int iSpectatorTeam;                      ///< Index of spectator team.

	static good::vector<TWeaponId> aDefaultWeapons; ///< Default respawn weapons. Can be set by a console command.
	static bool bRemoveWeapons;                     ///< If true, will remove all weapons from the bot on respawn. Can be set by a console command.

    static StringVector aBotNames;                  ///< Available bot names.
    static StringVector aClassNames;                ///< Name of player's classes.

    static bool bIntelligenceInBotName;             ///< Use bot's intelligence as part of his name.
	static bool bHeadShotDoesMoreDamage;            ///< HL2DM, CSS have that (true by default).
	static bool bUseModels;                         ///< HL2DM, CSS have that (true by default).

	static float fSpawnProtectionTime;              ///< Spawn protection time, 0 by default. Can be set by a console command.
	static int iSpawnProtectionHealth;              ///< Spawn protection health, 0 by default. Can be set by a console command.

//    static TDeathmatchFlags iDeathmatchFlags;       ///< Flags for deathmatch mode.

    static Vector vPlayerCollisionHull;             ///< Maxs of player collision box with origin in (0, 0, 0).
    static Vector vPlayerCollisionHullCrouched;     ///< Maxs of crouched player collision box with origin in (0, 0, 0).

    static Vector vPlayerCollisionHullMins;         // Those are for centered hull in (0, 0, 0).
    static Vector vPlayerCollisionHullMaxs;

    static Vector vPlayerCollisionHullCrouchedMins;
    static Vector vPlayerCollisionHullCrouchedMaxs;

    static Vector vPlayerCollisionHullMaxsGround;

    static float fMinNonStuckSpeed;                 ///< Minimum velocity to consider that bot is moving and non stucked.


    static int iPlayerRadius;                       ///< Player's radius (used to check if bot is stucked).
    static int iNearItemMaxDistanceSqr;             ///< Max distance to consider item to be near to player.
    static int iItemPickUpDistance;                 ///< Additional distance from player to item to consider it taken.
                                                    // Item is picked, if distance-to-player < player's-radius + item's-radius + this-distance.

    static int iPointTouchSquaredXY;                ///< Squared distance to consider that we are touching waypoint.
    static int iPointTouchSquaredZ;                 ///< Z distance to consider that we are touching waypoint. Should be no more than player can jump.
    static int iPointTouchLadderSquaredZ;           ///< Z distance to consider that we are touching waypoint while on ladder.


protected: // Methods.
    friend class CConfiguration; // Give access to next protected methods.

    // Returns true there is a player/bot with name cName.
    static bool IsNameTaken(const good::string& cName, TBotIntelligence iIntelligence);


protected: // Members.

    static TModId m_iModId;                         // Mod id.
    
    // Mod vars.
    static good::vector<float> m_aVars[ EModVarTotal ];

    static good::vector<CEventPtr> m_aEvents;       // Events this mod handles.
    static bool m_bMapHas[EItemTypeCanPickTotal];   // To check if map has items or waypoints of types: health, armor, weapon, ammo.

    // Events that happend on this frame.
    static good::vector< good::pair<TFrameEvent, TPlayerIndex> > m_aFrameEvents;
};


#endif // __BOTRIX_MOD_H__
