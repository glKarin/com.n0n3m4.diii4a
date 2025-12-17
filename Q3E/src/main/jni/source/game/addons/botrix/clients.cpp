#include "clients.h"
#include "config.h"
#include "console_commands.h"
#include "item.h"
#include "type2string.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//----------------------------------------------------------------------------------------------------------------
void CClient::Activated()
{
    CPlayer::Activated();

    BASSERT(m_pPlayerInfo, exit(1));
    m_sSteamId = m_pPlayerInfo->GetNetworkIDString();

    if ( m_sSteamId.size() )
    {
        TCommandAccessFlags iAccess = CConfiguration::ClientAccessLevel(m_sSteamId);
        if ( iAccess ) // Founded.
            iCommandAccessFlags = iAccess;
    }
    else
        iCommandAccessFlags = 0;

    BLOG_I( "User connected %s (steam id %s), access: %s.", GetName(), m_sSteamId.c_str(),
            CTypeToString::AccessFlagsToString(iCommandAccessFlags).c_str() );

    iWaypointDrawFlags = FWaypointDrawNone;
    iPathDrawFlags = FPathDrawNone;

    bAutoCreatePaths = FLAG_ALL_SET_OR_0(FCommandAccessWaypoint, iCommandAccessFlags);
    bAutoCreateWaypoints = false;

    iItemDrawFlags = FItemDrawAll;
    iItemTypeFlags = 0;

    iDestinationWaypoint = EWaypointIdInvalid;

#if defined(DEBUG) || defined(_DEBUG)
    bDebuggingEvents = FLAG_ALL_SET_OR_0(FCommandAccessConfig, iCommandAccessFlags);
#else
    bDebuggingEvents = false;
#endif
}

//----------------------------------------------------------------------------------------------------------------
void CClient::PreThink()
{
    //int iLastWaypoint = iCurrentWaypoint;
    CPlayer::PreThink();

    // Client don't have access to waypoint modification.
    if ( FLAG_CLEARED(FCommandAccessWaypoint, iCommandAccessFlags) )
        return;

    // Check if lost waypoint, in that case add new one.
    if ( bAutoCreateWaypoints && m_bAlive &&
         ( !CWaypoint::IsValid(iCurrentWaypoint) ||
           (GetHead().DistToSqr(CWaypoints::Get(iCurrentWaypoint).vOrigin) >= SQR(CWaypoint::iDefaultDistance)) ) )
    {
		// Execute command: botrix waypoint create
		const char* aCommands[] = { "waypoint", "create" };
		const int iSize = sizeof(aCommands) / sizeof(aCommands[0]);
		CBotrixCommand::instance->Execute(this, iSize, aCommands);

		// Old code auto adding waypoint directly
        //Vector vOrigin( GetHead() );

        //// Add new waypoint, but distance from previous one must not be bigger than iDefaultDistance.
        //if ( CWaypoint::IsValid(iLastWaypoint) )
        //{
        //    CWaypoint& wLast = CWaypoints::Get(iLastWaypoint);
        //    vOrigin -= wLast.vOrigin;
        //    vOrigin.NormalizeInPlace();
        //    vOrigin *= CWaypoint::iDefaultDistance;
        //    vOrigin += wLast.vOrigin;
        //}

        //// Add new waypoint.
        //iCurrentWaypoint = CWaypoints::Add(vOrigin);

        //// Add paths from previous to current.
        //if ( CWaypoint::IsValid(iLastWaypoint) )
        //{
        //    float fHeight = GetPlayerInfo()->GetPlayerMaxs().z - GetPlayerInfo()->GetPlayerMins().z + 1;
        //    bool bIsCrouched = (fHeight < CMod::GetVar( EModVarPlayerHeight ));

        //    CWaypoints::CreatePathsWithAutoFlags(iLastWaypoint, iCurrentWaypoint, bIsCrouched, true);
        //    iDestinationWaypoint = iLastWaypoint;
        //}
    }

    // Calculate destination waypoint according to angles. Path's should be drawn.
    if ( !bLockDestinationWaypoint && (iPathDrawFlags != FPathDrawNone) &&
         (CWaypoints::fNextDrawWaypointsTime >= CBotrixPlugin::fTime) )
    {
        QAngle ang;
        GetEyeAngles(ang);
        iDestinationWaypoint = CWaypoints::GetAimedWaypoint( GetHead(), ang );
    }

    // Draw waypoints.
    CWaypoints::Draw(this); // TODO: should not draw for admins without rights.

    // Draw entities.
    CItems::Draw(this);
}
