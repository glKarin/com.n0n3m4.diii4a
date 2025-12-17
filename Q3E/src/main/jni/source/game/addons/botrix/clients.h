#ifndef __BOTRIX_CLIENTS_H__
#define __BOTRIX_CLIENTS_H__


#include "players.h"
#include "waypoint.h"


//****************************************************************************************************************
/// Class that represents a client, connected to this server.
//****************************************************************************************************************
class CClient: public CPlayer
{

public: // Methods.

    /// Constructor.
    CClient( edict_t* pPlayer ): CPlayer(pPlayer, false),
        iCommandAccessFlags(0), iDestinationWaypoint(-1), iWaypointDrawFlags(0),
        iPathDrawFlags(0), iVisiblesDrawFlags(0), iItemTypeFlags(0), iItemDrawFlags(0), bAutoCreateWaypoints(false),
        bAutoCreatePaths(true), bDebuggingEvents(false), bLockDestinationWaypoint(false) { }


    /// Get Steam id of this player.
    const good::string& GetSteamID() { return m_sSteamId; }

    /// Returns true, if auto-creating waypoints.
    bool IsAutoCreatingWaypoints() { return bAutoCreateWaypoints; }

    /// Called when client finished connecting with server (becomes active).
    virtual void Activated();

    /// Called each frame. Will draw waypoints for this player.
    virtual void PreThink();


public: // Members and constants.
    TCommandAccessFlags iCommandAccessFlags;   ///< Access for console commands.

    TWaypointId iDestinationWaypoint;          ///< Path destination (path origin is iCurrentWaypoint).

    TWaypointDrawFlags iWaypointDrawFlags;     ///< Draw waypoint flags for this player.
    TPathDrawFlags iPathDrawFlags;             ///< Draw path flags for this player.
    TPathDrawFlags iVisiblesDrawFlags;         ///< Draw visible waypoints flags for this player.

    TItemTypeFlags iItemTypeFlags;             ///< Items to draw for this player.
    TItemDrawFlags iItemDrawFlags;             ///< Draw item flags for this player.

    bool bAutoCreateWaypoints;                 ///< Generate automatically new waypoints, if distance is too far.
    bool bAutoCreatePaths;                     ///< Generate automatically paths for new waypoint.

    bool bDebuggingEvents;                     ///< Print info when game event is fired.
    bool bLockDestinationWaypoint;             ///< Destination waypoint was set by console command.


protected:
    good::string m_sSteamId;                   // Steam id.

};


#endif // __BOTRIX_CLIENTS_H__
