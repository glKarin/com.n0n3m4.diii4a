#ifndef __BOTRIX_PLAYERS_H__
#define __BOTRIX_PLAYERS_H__


#include <good/memory.h> // unique_ptr

#include "source_engine.h"
#include "types.h"

#include "edict.h"
#include "iplayerinfo.h"


#if defined(DEBUG) || defined(_DEBUG)
    #define DRAW_PLAYER_HULL 0
#endif


class CClient; // Forward declaration.


//****************************************************************************************************************
/// Class that defines a player (bot or client).
//****************************************************************************************************************
class CPlayer
{
public:
    /// Constructor.
    CPlayer(edict_t* pEdict, bool bIsBot);

    /// Destructor.
    virtual ~CPlayer() {}

    /// Return true if player is a bot.
    bool IsBot() const { return m_bBot; }

	/// Return true if player is alive.
	bool IsAlive() const { return m_bAlive; }

	/// Return true if player is protected.
	bool IsProtected() const { return m_bProtected; }


    /// Get entity index of player.
    TPlayerIndex GetIndex() const { return m_iIndex; }

    /// Get edict of player.
    edict_t* GetEdict() const { return m_pEdict; }

    /// Return player's team.
    int GetTeam() const { return m_pPlayerInfo->GetTeamIndex(); }

    /// Get player's info.
    IPlayerInfo* GetPlayerInfo() const { return m_pPlayerInfo; }

    /// Get name of this player.
    const char* GetName() const { return m_pPlayerInfo ? m_pPlayerInfo->GetName() : NULL; }

    /// Get lowercase name of this player.
    const good::string& GetLowerName() const { return m_sName; }

    /// Get head position of player.
    const Vector& GetHead() const { return m_vHead; }

    /// Get previous head position of player.
    const Vector& GetPreviousHead() const { return m_vPrevHead; }

    /// Get center position of player.
    void GetCenter( Vector& v ) const { CUtil::EntityCenter(m_pEdict, v); }

	/// Get foot position of player.
	void GetFoot( Vector& v ) const { v = m_pPlayerInfo->GetAbsOrigin(); }

	/// Get health of player.
	int GetHealth() const { return m_pPlayerInfo->GetHealth(); }

	/// Get player's view angles.
    void GetEyeAngles( QAngle& a ) const
    {
        // TODO: remove comment if works.
        CBotCmd cCmd = m_pPlayerInfo->GetLastUserCommand();
        a = cCmd.viewangles;
//        a = m_pPlayerInfo->GetAbsAngles();
    }

	/// Protect player for certain amount of time. 0 means don't protect, -1 means forever.
	void Protect( float time ) {
		m_bProtected = time != 0.0f;
		m_fEndProtectionTime = time >= 0 ? CBotrixPlugin::fTime + time : -1;
	}


    //------------------------------------------------------------------------------------------------------------
    // Virtual functions, client or bot should override these.
    //------------------------------------------------------------------------------------------------------------
    /// Called when client finished connecting with server (becomes active).
    virtual const char* GetStatus() { return GetName(); }

    /// Called when client finished connecting with server (becomes active).
    virtual void Activated();

    /// Player is respawned. Set up current waypoint and head position.
    virtual void Respawned();

    /// Called when player becomes dead.
    virtual void Dead() { m_bAlive = false; }

    /// Called each frame. Update current waypoint and head position.
    virtual void PreThink();


public:
    TWaypointId iCurrentWaypoint;    ///< Nearest waypoint to player's position.
    TWaypointId iNextWaypoint;       ///< Next waypoint in path, used by bot to check if touching next waypoint (to perform actions).
    TWaypointId iPrevWaypoint;       ///< Previous waypoint in path, used by bot to rewind action if action fails.

    TPlayerIndex iChatMate;          ///< Current chat mate.


protected:
    edict_t* m_pEdict;               // Player's edict.
    TPlayerIndex m_iIndex;           // Player's index.
    IPlayerInfo* m_pPlayerInfo;      // Player's info.
#if DRAW_PLAYER_HULL
    float m_fNextDrawHullTime;       // Next time to draw player's hull.
#endif

	float m_fEndProtectionTime;      // Time when player's protection ends.

    good::string m_sName;            // Lowercased name of player.
    Vector m_vHead;                  // Head position of player.
    Vector m_vPrevHead;              // Previous position of player.

    bool m_bBot:1;                   // This member will be set to true by bot.
	bool m_bAlive : 1;               // IPlayerInfo::IsDead() returns true only when player is dead, not when becomes respawnable.
	bool m_bProtected : 1;           // If this player / bot is protected, then bots shouldn't attack him.

};

typedef good::shared_ptr<CPlayer> CPlayerPtr; ///< Typedef for unique_ptr of CPlayer.



//****************************************************************************************************************
/// Class that holds both clients and bots.
//****************************************************************************************************************
class CPlayers
{

public:
    static bool bAddingBot;                   ///< True if currently adding bot.
    static int iBotsPlayersCount;             ///< Count of bots + players.
    static float fPlayerBotRatio;             ///< Player-Bot ratio. For example 3 means 3bots per 1player.

    /// Get count of players on this server.
    static inline int Size() { return m_aPlayers.size(); };

    /// Get bots count.
    static inline int GetBotsCount() { return m_iBotsCount; }

    /// Get clients count.
    static inline int GetClientsCount() { return m_iClientsCount; }

    /// Get players count.
    static inline int GetPlayersCount() { return m_iClientsCount + m_iBotsCount; }

    /// Get players count in given team.
    static int GetTeamCount( TTeam iTeam )
    {
        int iCount = 0;
        for ( int i = 0; i < Size(); ++i )
        {
            CPlayer* pPlayer = m_aPlayers[i].get();
            if ( pPlayer && (pPlayer->GetTeam() == iTeam) )
                iCount++;
        }
        return iCount;
    }

	/// Get players names on this server.
	static void GetNames( StringVector& aNames, bool bGetBots = true, bool bGetUsers = true );

    /// Check amount of bots on server.
    static void CheckBotsCount();

    /// Get player from index.
    static inline CPlayer* Get( TPlayerIndex iIndex ) { return m_aPlayers[iIndex].get(); }

    /// Get player from index.
    static inline CPlayer* Get( edict_t* pEdict ) { return m_aPlayers[ GetIndex(pEdict) ].get(); }

    /// Get player index from edict.
    static inline int GetIndex( edict_t* pPlayer )
    {
        return CBotrixPlugin::pEngineServer->IndexOfEdict(pPlayer)-1;
    }

    /// Get client that created listen server (not dedicated server).
    static inline CClient* GetListenServerClient() { return m_pListenServerClient; }

    /// Invalidate waypoints for all players.
    static void InvalidatePlayersWaypoints();

    /// Start new map.
    static void Init( int iMaxPlayers );

    /// End map.
    static void Clear();


    //------------------------------------------------------------------------------------------------------------
    // Bot handling.
    //------------------------------------------------------------------------------------------------------------
    /// Add bot.
    static CPlayer* AddBot( const char* sName = NULL, TTeam iTeam = 0, TClass iClass = -1,
                            TBotIntelligence iIntelligence = -1, int argc = 0, const char** argv = NULL );

    /// Kick given bot.
    static void KickBot( CPlayer* pPlayer );

    /// Kick random bot on random team.
    static bool KickRandomBot();

    /// Kick random bot on given team.
    static bool KickRandomBotOnTeam( int team );

    /// Deliver chat / voice command to bots.
    static void DeliverChat( edict_t* pFrom, bool bTeamOnly, const char* szText );

    /// Deliver message to all clients.
    static void Message( const char* szFormat, ... );


    /// Called when player connects this server.
    static void PlayerConnected( edict_t* pEdict );

    /// Called when player disconnects from this server.
    static void PlayerDisconnected( edict_t* pEdict );


    /// Called each frame. Will make players and bots 'think'.
    static void PreThink();


    //------------------------------------------------------------------------------------------------------------
    // Debugging events.
    //------------------------------------------------------------------------------------------------------------
    /// Returns true if some client is debugging events.
    static inline bool IsDebuggingEvents() { return m_bClientDebuggingEvents; }

    /// Check if some client is debugging events. If so, DebugEvent() with event info will be called.
    static void CheckForDebugging();

    /// Display info message on client that is debugging events.
    static void DebugEvent( const char *szFormat, ... );

    /// Get last error.
    static inline const good::string& GetLastError() { return m_sLastError; }

protected:
    static good::vector<CPlayerPtr> m_aPlayers;
    static CClient* m_pListenServerClient;                  // Client that created listen server (not dedicated server).
    static bool m_bClientDebuggingEvents;                   // True if some client is debugging event messages.
    static bool m_bCheckBotCountFinished;                   // True if finished adding/kicking bots.

    static int m_iClientsCount;                             // Total amount of clients on this server.
    static int m_iBotsCount;                                // Total amount of bots on this server.

    static good::string m_sLastError;                       // Last error.

    static good::vector< good::vector<bool> > m_iChatPairs; // 2D array representing who chats with whom.
};

#endif // __BOTRIX_PLAYERS_H__
