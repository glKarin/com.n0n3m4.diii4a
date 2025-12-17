#include <good/mutex.h>
#include <good/file.h>

#include "clients.h"
#include "console_commands.h"
#include "clients.h"
#include "server_plugin.h"
#include "source_engine.h"
#include "waypoint.h"

#include "cbase.h"
#include "IEffects.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//----------------------------------------------------------------------------------------------------------------
extern IVDebugOverlay* pVDebugOverlay;
extern IEffects* pEffects;

extern char* szMainBuffer;
extern int iMainBufferSize;

int CUtil::iTextTime = 20.0; // Time in seconds to show text in CUtil::GetReachableInfoFromTo().


//----------------------------------------------------------------------------------------------------------------
class CGenericTraceFilter: public CTraceFilterWithFlags
{
public:
    TVisibility iVisibility;

    CGenericTraceFilter( TVisibility iVisibility ): iVisibility( iVisibility )
    {
        switch ( iVisibility )
        {
            case EVisibilityWorld:
                // Everything that can be seen by player / bot.
                iTraceFlags = MASK_VISIBLE; 
                break;
            case EVisibilityWaypoints:
                // Everything that blocks player movement. Should include CONTENTS_PLAYERCLIP!!!
                iTraceFlags = MASK_PLAYERSOLID & ~CONTENTS_MOVEABLE;
                break;
            case EVisibilityBots:
                // Should't include CONTENTS_GRATE. TODO: exclude WINDOW?
                iTraceFlags = MASK_VISIBLE | MASK_SHOT;
                break;
            default:
                BASSERT(false);
        }
    }

    virtual TraceType_t	GetTraceType() const
    {
        switch ( iVisibility )
        {
            case EVisibilityWorld:
                return TRACE_WORLD_ONLY;
            case EVisibilityWaypoints:
                // Apparently, there is a bug in Source Engine when using TRACE_EVERYTHING_FILTER_PROPS, 
                // that doesn't pass in ShouldHitEntity() props that are CONTENTS_PLAYERCLIP. This will make to create
                // waypoints in invalid positions.
                return CWaypoints::IsAnalyzing() && CWaypoint::bAnalyzeTraceAll ? TRACE_EVERYTHING_FILTER_PROPS : TRACE_EVERYTHING;
            case EVisibilityBots:
                return TRACE_EVERYTHING_FILTER_PROPS;
            default:
                BASSERT( false );
        }
        return TRACE_WORLD_ONLY;
    }
    
    // Note that this function is not used for iVisibility == EVisibilityWorld. 
    virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int /*contentsMask*/ )
    {
        int index = pServerEntity->GetRefEHandle().GetEntryIndex();
        if ( index == 0 || index >= MAX_EDICTS ) // 0 is the world's entity.
            return true;

        // Should trace players only if the visibility is for shooting.
        if ( index < 1 + CPlayers::Size() )
            return false;

        if ( iVisibility == EVisibilityBots )
            return true; // Trace everything for shooting / analyzing with all trace.

        // Here we know for sure that we are analyzing the map: CWaypoints::IsAnalyzing() && CWaypoint::bAnalyzeTraceAll.
        edict_t *pEdict = CBotrixPlugin::instance->pEngineServer->PEntityOfEntIndex( index );
        if ( pEdict == NULL ) // Sometimes happens.
            return true;

        TItemIndex iItemIndex;
        TItemType iType = CItems::GetItemFromId( pEdict->m_EdictIndex, &iItemIndex );

        bool bShouldHit = ( iType == EItemTypeObject && FLAG_SOME_SET( FObjectHeavy, CItems::GetItems( iType )[ iItemIndex ].iFlags ) ) ||
                            iType == EItemTypeDoor || iType == EItemTypeOther || iType == EItemTypePlayerSpawn;

        // Trace only heavy objects / doors (elevators) / other objects.
        if ( !CWaypoints::IsAnalyzing() )// && iVisibility == EVisibilityWaypoints )
        {
            const char* szClassName = pEdict->GetClassName();
            BLOG_T( "Should hit %s %d (%s, id %d): %s.", CTypeToString::EntityTypeToString( iType ).c_str(), 
                    iItemIndex, szClassName, pEdict->m_EdictIndex, bShouldHit ? "yes" : "no" );
        }

        return bShouldHit;
    }
};

//----------------------------------------------------------------------------------------------------------------
class COnlyOneEntityTraceFilter: public ITraceFilter
{
public:
    COnlyOneEntityTraceFilter( edict_t* pEntity )
    {
        pHandle = pEntity->GetIServerEntity()->GetNetworkable()->GetEntityHandle();
    }

    virtual TraceType_t	GetTraceType() const { return TRACE_ENTITIES_ONLY; }
    virtual bool ShouldHitEntity( IHandleEntity *pEntity, int /*contentsMask*/ ) { return pHandle == pEntity; }

protected:
    const IHandleEntity* pHandle;
};


CGenericTraceFilter cWorldTraceFilter( EVisibilityWorld );
CGenericTraceFilter cWaypointTraceFilter( EVisibilityWaypoints );
CGenericTraceFilter cBotsTraceFilter(EVisibilityBots);

CTraceFilterWithFlags& CUtil::GetTraceFilter( TVisibility iVisibility )
{
    switch ( iVisibility )
    {
        case EVisibilityWorld:
            return cWorldTraceFilter;
        case EVisibilityWaypoints:
            return cWaypointTraceFilter;
        case EVisibilityBots:
            return cBotsTraceFilter;
        default:
            BASSERT(false);
    }
    return cWorldTraceFilter;
}

//----------------------------------------------------------------------------------------------------------------
// If slope is less that 45 degrees (for HL2) then player can move forward (returns EReachReachable).
// If not and vSrc is higher, then player can take damage trying to get to lower ground (returns EReachFallDamage).
// If vSrc is lower in Z, and slope is more than 45 degrees then player can't reach destination (returns
// EReachNotReachable).
//----------------------------------------------------------------------------------------------------------------
TReach CanClimbSlope( const Vector& vSrc, const Vector& vDest )
{
    Vector vDiff = vDest - vSrc;
    QAngle ang;
    VectorAngles( vDiff, ang ); // Get pitch to know if gradient is too big.

    if ( !CWaypoints::IsAnalyzing() )
        BLOG_T( "Slope angle %.2f", ang.x );

    float fSlope = CMod::GetVar( EModVarSlopeGradientToSlideOff );
    return CUtil::CanPassSlope( ang.x, fSlope ) ? EReachReachable : EReachNotReachable;
}

//----------------------------------------------------------------------------------------------------------------
// Returns true if can move forward when standing at vGround performing a jump (normal, with crouch o maybe just
// walking). At return vHit contains new coord which is where player can get after jump.
//----------------------------------------------------------------------------------------------------------------
TReach CanPassOrJump( Vector& vGround, Vector& vDirectionInc, const Vector& vMins, const Vector& vMaxs )
{
    // Try to walk unit.
    Vector vHit = vGround + vDirectionInc;

    CUtil::TraceHull( vGround, vHit, vMins, vMaxs, cWaypointTraceFilter.iTraceFlags, &cWaypointTraceFilter );

    if ( !CUtil::IsTraceHitSomething() )
    {
        vGround = CUtil::GetHullGroundVec( vHit );
        return EReachReachable;
    }

    if ( !CUtil::EqualVectors( vGround, CUtil::TraceResult().endpos, 0.01f ) )
    {
        vGround = CUtil::TraceResult().endpos;
        return EReachReachable; // Can walk from vGround to vHit.
    }

    // Try to walk over (one stair step).
    float fMaxWalkHeight = CMod::GetVar( EModVarPlayerObstacleToJump );
    Vector vStair = vGround; vStair.z += fMaxWalkHeight;
    vHit.z += fMaxWalkHeight;
   
    CUtil::TraceHull( vStair, vHit, vMins, vMaxs, cWaypointTraceFilter.iTraceFlags, &cWaypointTraceFilter );
    if ( !CUtil::IsTraceHitSomething() )
    {
        vStair = vGround;
        vGround = CUtil::GetHullGroundVec( vHit );
        if ( CanClimbSlope( vStair, vGround ) )
        {
            // Check if can climb up without making the stair step.
            CUtil::TraceHull( vStair, vGround, vMins, vMaxs, cWaypointTraceFilter.iTraceFlags, &cWaypointTraceFilter );
            if ( !CUtil::IsTraceHitSomething() )
                return EReachReachable;
        }
        return EReachStairs;
    }

    // Try to jump.
    float fJumpCrouched = CMod::GetVar( EModVarPlayerJumpHeightCrouched );
    vGround.z += fJumpCrouched;
    vHit.z += fJumpCrouched - fMaxWalkHeight; // We added previously fMaxWalkHeight.

    CUtil::TraceHull( vGround, vHit, vMins, vMaxs, cWaypointTraceFilter.iTraceFlags, &cWaypointTraceFilter ); // TODO: use vJumpMaxs instead of vMaxs here.
    
    if ( !CUtil::IsTraceHitSomething() ) // We can stand on vHit after jump.
    {
        vGround = CUtil::GetHullGroundVec( vHit );
        return EReachNeedJump;
    }

    vGround = CUtil::TraceResult().endpos;
    //vGround = CUtil::GetHullGroundVec( CUtil::TraceResult().endpos );
    return EReachNotReachable; // Can't jump over.
}



//****************************************************************************************************************
good::TLogLevel CUtil::iLogLevel = good::ELogLevelInfo;

const Vector CUtil::vZero(0, 0, 0);
const QAngle CUtil::angZero(0, 0, 0);

trace_t CUtil::m_TraceResult;



//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsRayHitsEntity( edict_t* pEntity, const Vector& vSrc, const Vector& vDest )
{
    COnlyOneEntityTraceFilter filter(pEntity);
    TraceLine(vSrc, vDest, MASK_OPAQUE, &filter);
    return m_TraceResult.fraction >= 0.95f;
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsVisible( const Vector& vSrc, const Vector& vDest, TVisibility iVisibility, bool bUsePVS )
{
    if ( bUsePVS )
    {
        CUtil::SetPVSForVector( vSrc );
        if ( !CUtil::IsVisiblePVS( vDest ) )
            return false;
    }

    CTraceFilterWithFlags* pTraceFilter = &GetTraceFilter( iVisibility );
    TraceLine(vSrc, vDest, pTraceFilter->iTraceFlags, pTraceFilter );
    return m_TraceResult.fraction >= 0.95f;
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsVisible( const Vector& vSrc, edict_t* pDest )
{
    Vector v;
    EntityHead(pDest, v);
    
    return IsVisible(vSrc, v, EVisibilityBots);
}

//----------------------------------------------------------------------------------------------------------------
TReach CUtil::GetReachableInfoFromTo( const Vector& vSrc, Vector& vDest, bool& bCrouch, float fDistanceSqr, float fMaxDistanceSqr, bool bShowHelp )
{
    static int iRandom = 0; // This function may be called 2 times with (v1, v2) and (v2,v1)
    iRandom = (iRandom + 1) % 4;  // so iRandom is to draw text higher / other color that previous time.

    static int colors[4] = { 0xFFFF00, 0xFFFFFF, 0xFF0000, 0x00FF00 };
    int color = colors[ iRandom ];
    unsigned char r = GET_3RD_BYTE( color ), g = GET_2ND_BYTE( color ), b = GET_1ST_BYTE( color );
    
    Vector vOffset( iRandom / 4.0, iRandom / 4.0, 0 );

    if ( fDistanceSqr <= 0.0f )
        fDistanceSqr = vSrc.AsVector2D().DistToSqr(vDest.AsVector2D());

    if ( fDistanceSqr > fMaxDistanceSqr )
        return EReachNotReachable;

    if ( !CUtil::IsVisible(vSrc, vDest, EVisibilityWaypoints ) )
        return EReachNotReachable;

    // Check if can swim there first.
    int iSrcContent = CBotrixPlugin::pEngineTrace->GetPointContents( vSrc );
    int iDestContent = CBotrixPlugin::pEngineTrace->GetPointContents( vDest );
    if ( iSrcContent == CONTENTS_WATER && iDestContent == CONTENTS_WATER)
        return EReachReachable;

    // Get all needed vars ready.
    float fPlayerEye = CMod::GetVar( EModVarPlayerEye );
    float fPlayerEyeCrouched = CMod::GetVar( EModVarPlayerEyeCrouched );

    Vector vMinZ( 0, 0, -iHalfMaxMapSize );

    Vector vMins = CMod::vPlayerCollisionHullMins;
    Vector vMaxs = CMod::vPlayerCollisionHullMaxs;
    Vector vGroundMaxs = CMod::vPlayerCollisionHullMaxsGround;
    vGroundMaxs.z = 1.0f;

    // Get ground positions.
    Vector vSrcGround = GetHullGroundVec( vSrc );
    if ( vSrcGround.z <= vMinZ.z ) 
        return EReachNotReachable;

    Vector vDestGround = GetHullGroundVec( vDest );
    if ( vDestGround.z == vMinZ.z )
        return EReachNotReachable;

    // Check if take damage at fall.
    if ( vSrcGround.z - vDestGround.z >= CMod::GetVar( EModVarHeightForFallDamage ) )
        return EReachNotReachable;

    // Try to get up if needed.
    float zDiff = vDest.z - vDestGround.z;
    if ( zDiff == 0 )
        return EReachNotReachable; // Can happens when the vDestGround is inside some solid.

    if ( zDiff != fPlayerEye && zDiff != fPlayerEyeCrouched )
    {
        if ( !bCrouch )
        {
            // Try to stand up.
            vDest.z = vDestGround.z + fPlayerEye;
            TraceHull( vDest, vDestGround, vMins, vGroundMaxs, cWaypointTraceFilter.iTraceFlags, &cWaypointTraceFilter );
            bCrouch = IsTraceHitSomething();
        }
        
        if ( bCrouch )
        {
            // Try to stand up crouching.
            vDest.z = vDestGround.z + fPlayerEyeCrouched;
            TraceHull( vDest, vDestGround, vMins, vGroundMaxs, cWaypointTraceFilter.iTraceFlags, &cWaypointTraceFilter );
            if ( IsTraceHitSomething() )
                return EReachNotReachable;
        }
    }

    // Draw waypoints until ground.
    if ( bShowHelp )
    {
        // TODO: draw from the ground position.
        DrawLine( vSrc + vOffset, vSrcGround + vOffset, iTextTime, r, g, b );
        DrawLine( vDest + vOffset, vDestGround + vOffset, iTextTime, r, g, b );
    }

    TReach iResult = EReachReachable;

    // Need to trace several times to know if can jump up all obstacles.
    Vector vHit = vSrcGround;
    Vector vDirection = vDestGround - vSrcGround;
    vDirection.z = 0.0f; // We need only X-Y direction.
    vDirection.NormalizeInPlace();
    vDirection *= 1.0;

    Vector vLastStair;
    bool bNeedJump = false, bHasStair = false;
    int i = 0;
    for ( ; i < iMaxTraceRaysForReachable && !EqualVectors(vHit, vDestGround); ++i )
    {
        Vector vStart = vHit;

        // Trace from hit point to the floor.
        TReach iReach = CanPassOrJump( vHit, vDirection, vMins, vMaxs );

        if ( bShowHelp )
            DrawLine( vStart + vOffset, vHit + vOffset, iTextTime, r, g, b );

        switch ( iReach )
        {
            case EReachNotReachable:
                if ( bShowHelp )
                    DrawText( vHit, 0, iTextTime, 0xFF, 0xFF, 0xFF, "High jump" );
                return EReachNotReachable;

            case EReachNeedJump:
                if ( bNeedJump )
                {
                    if ( bShowHelp )
                        CUtil::DrawText( vHit, 0, iTextTime, 0xFF, 0xFF, 0xFF, "2 jumps" );
                    return EReachNotReachable;
                }
                bNeedJump = true;
                break;

            case EReachStairs:
            {
                float fDistSqr = vLastStair.AsVector2D().DistToSqr( vHit.AsVector2D() );
                if ( bHasStair && fDistSqr <= 9 ) // 3 units at least
                {
                    if ( bShowHelp )
                        CUtil::DrawText( vHit, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Slope" );
                    return EReachNotReachable;
                }
                bHasStair = true;
                vLastStair = vStart;
                break;
            }

            case EReachReachable:
                break;

            default:
                GoodAssert( false );
                break;
        }
    }
    
    // Set text position.
    Vector vText = (vSrcGround + vDestGround) / 2;
    vText.z += iRandom * 10;

    if ( i == iMaxTraceRaysForReachable )
        iResult = EReachNotReachable;
    
    switch (iResult)
    {
    case EReachReachable:
        if ( bNeedJump )
        {
            iResult = EReachNeedJump;
            if ( bShowHelp )
                CUtil::DrawText( vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Jump" );
        }
        else if ( bShowHelp )
            CUtil::DrawText(vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Walk");
        break;

    case EReachFallDamage:
        if ( bShowHelp )
            CUtil::DrawText(vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "Fall");
        break;

    case EReachNotReachable:
        if ( bShowHelp )
            CUtil::DrawText( vText, 0, iTextTime, 0xFF, 0xFF, 0xFF, "High" );
        break;

    default:
        GoodAssert( false );
    }

    return iResult;
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::TraceLine(const Vector& vSrc, const Vector& vDest, int mask, ITraceFilter *pFilter)
{
    Ray_t ray;
    memset(&m_TraceResult, 0, sizeof(trace_t));
    ray.Init( vSrc, vDest );
    CBotrixPlugin::pEngineTrace->TraceRay( ray, mask, pFilter, &m_TraceResult );
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::TraceHull( const Vector& vSrc, const Vector& vDest, const Vector& vMins, const Vector& vMaxs, int mask, ITraceFilter *pFilter )
{
    Ray_t ray;
    memset( &m_TraceResult, 0, sizeof( trace_t ) );
    ray.Init( vSrc, vDest, vMins, vMaxs );
    CBotrixPlugin::pEngineTrace->TraceRay( ray, mask, pFilter, &m_TraceResult );
}

//----------------------------------------------------------------------------------------------------------------
Vector& CUtil::GetGroundVec( const Vector& vSrc )
{
    Vector vDest = vSrc;
    vDest.z = -iHalfMaxMapSize;

    TraceLine( vSrc, vDest, cWaypointTraceFilter.iTraceFlags, &cWaypointTraceFilter );
    return m_TraceResult.endpos;
}

//----------------------------------------------------------------------------------------------------------------
Vector& CUtil::GetHullGroundVec( const Vector& vSrc )
{
    Vector vDest = vSrc;
    vDest.z = -iHalfMaxMapSize;

    TraceHull( vSrc, vDest, CMod::vPlayerCollisionHullMins, CMod::vPlayerCollisionHullMaxsGround, cWaypointTraceFilter.iTraceFlags, &cWaypointTraceFilter );

    //float fHullPos = m_TraceResult.endpos.z;

    //TraceLine( vSrc, vDest, cWaypointTraceFilter.iTraceFlags, &cWaypointTraceFilter );

    //if ( fHullPos >= m_TraceResult.endpos.z )
    //    m_TraceResult.endpos.z = fHullPos + 2;
    //else
    //    m_TraceResult.endpos.z += 1;
    return m_TraceResult.endpos;
}

//----------------------------------------------------------------------------------------------------------------
edict_t* CUtil::GetEntityByUserId( int iUserId )
{
    for ( int i = 1; i <= CPlayers::Size(); i ++ )
    {
        edict_t* pEdict = CBotrixPlugin::pEngineServer->PEntityOfEntIndex(i);

        if ( pEdict && (CBotrixPlugin::pEngineServer->GetPlayerUserId(pEdict) == iUserId) )
            return pEdict;
    }
    return NULL;
}

//----------------------------------------------------------------------------------------------------------------
unsigned char pvs[MAX_MAP_CLUSTERS/8];

void CUtil::SetPVSForVector( const Vector& v )
{
    // Get visible clusters from player's position.
    int iClusterIndex = CBotrixPlugin::pEngineServer->GetClusterForOrigin( v );
    CBotrixPlugin::pEngineServer->GetPVSForCluster( iClusterIndex, sizeof(pvs), pvs );
}

bool CUtil::IsVisiblePVS( const Vector& v )
{
    return CBotrixPlugin::pEngineServer->CheckOriginInPVS( v, pvs, sizeof(pvs) );
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsNetworkable( edict_t* pEntity )
{
    IServerEntity* pServerEnt = pEntity->GetIServerEntity();
    return ( pServerEnt && (pServerEnt->GetNetworkable() != NULL) );
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::EntityCenter( edict_t* pEntity, Vector& v )
{
    static float* fOrigin;
    BASSERT( IsNetworkable(pEntity), return );
    fOrigin = pEntity->GetIServerEntity()->GetNetworkable()->GetPVSInfo()->m_vCenter;

    v.x = fOrigin[0]; v.y = fOrigin[1]; v.z = fOrigin[2];
}


//================================================================================================================
bool CUtil::IsTouchBoundingBox2d( const Vector2D &a1, const Vector2D &a2, const Vector2D &bmins, const Vector2D &bmaxs )
{
    Vector2D amins = Vector2D(MIN2(a1.x,a2.x),MIN2(a1.y,a2.y));
    Vector2D amaxs = Vector2D(MAX2(a1.x,a2.x),MAX2(a1.y,a2.y));

    return (((bmins.x >= amins.x) && (bmins.y >= amins.y) && (bmins.x <= amaxs.x) && (bmins.y <= amaxs.y)) ||
            ((bmaxs.x >= amins.x) && (bmaxs.y >= amins.y) && (bmaxs.x <= amaxs.x) && (bmaxs.y <= amaxs.y)));
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsTouchBoundingBox3d( const Vector& a1, const Vector& a2, const Vector& bmins, const Vector& bmaxs )
{
    Vector amins = Vector(MIN2(a1.x,a2.x),MIN2(a1.y,a2.y),MIN2(a1.z,a2.z));
    Vector amaxs = Vector(MAX2(a1.x,a2.x),MAX2(a1.y,a2.y),MAX2(a1.z,a2.z));

    return (((bmins.x >= amins.x) && (bmins.y >= amins.y) && (bmins.z >= amins.z) && (bmins.x <= amaxs.x) && (bmins.y <= amaxs.y) && (bmins.z <= amaxs.z)) ||
            ((bmaxs.x >= amins.x) && (bmaxs.y >= amins.y) && (bmaxs.z >= amins.z) && (bmaxs.x <= amaxs.x) && (bmaxs.y <= amaxs.y) && (bmaxs.z <= amaxs.z)));
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsOnOppositeSides2d( const Vector2D &amins, const Vector2D &amaxs, const Vector2D &bmins, const Vector2D &bmaxs )
{
  float g = (amaxs.x - amins.x) * (bmins.y - amins.y) -
            (amaxs.y - amins.y) * (bmins.x - amins.x);

  float h = (amaxs.x - amins.x) * (bmaxs.y - amins.y) -
           ( amaxs.y - amins.y) * (bmaxs.x - amins.x);

  return (g * h) <= 0.0f;
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsOnOppositeSides3d( const Vector& amins, const Vector& amaxs, const Vector& bmins, const Vector& bmaxs )
{
    amins.Cross(bmins);
    amaxs.Cross(bmaxs);

  float g =(amaxs.x - amins.x) * (bmins.y - amins.y) * (bmins.z - amins.z) -
           (amaxs.z - amins.z) * (amaxs.y - amins.y) * (bmins.x - amins.x);

  float h =(amaxs.x - amins.x) * (bmaxs.y - amins.y) * (bmaxs.z - amins.z) -
           (amaxs.z - amins.z) * (amaxs.y - amins.y) * (bmaxs.x - amins.x);

  return (g * h) <= 0.0f;
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsLineTouch2d( const Vector2D &amins, const Vector2D &amaxs, const Vector2D &bmins, const Vector2D &bmaxs )
{
    return IsOnOppositeSides2d(amins,amaxs,bmins,bmaxs) && IsTouchBoundingBox2d(amins,amaxs,bmins,bmaxs);
}

//----------------------------------------------------------------------------------------------------------------
bool CUtil::IsLineTouch3d(const Vector& amins, const Vector& amaxs, const Vector& bmins, const Vector& bmaxs )
{
    return IsOnOppositeSides3d(amins,amaxs,bmins,bmaxs) && IsTouchBoundingBox3d(amins,amaxs,bmins,bmaxs);
}

//================================================================================================================
void CUtil::Message( good::TLogLevel iLevel, edict_t* pEntity, const char* szMsg )
{
    if ( pEntity && ( CBotrixPlugin::pEngineServer->IsDedicatedServer() ||
                      CPlayers::Get(pEntity) != CPlayers::GetListenServerClient() ) )
        CBotrixPlugin::pEngineServer->ClientPrintf(pEntity, szMsg);

    if ( iLevel >= iLogLevel )
    {
        switch ( iLevel )
        {
        case good::ELogLevelTrace:
        case good::ELogLevelDebug:
        case good::ELogLevelInfo:
            Msg(szMsg);
//            fprintf(stdout, "%s", szMsg);
            break;
        case good::ELogLevelWarning:
        case good::ELogLevelError:
            Warning(szMsg);
//            fprintf(stdout, "%s", szMsg);
            break;
        }
/*#ifdef GOOD_LOG_FLUSH
        fflush(stdout);
        fflush(stderr);
#endif*/
    }
}

//----------------------------------------------------------------------------------------------------------------
good::mutex cMessagesMutex;
int iQueueMessageStringSize = 0;
char szQueueMessageString[64*1024];

void CUtil::PutMessageInQueue( const char* fmt, ... )
{
    va_list argptr;

    va_start(argptr, fmt);
    cMessagesMutex.lock();

    int iSize = vsprintf( &szQueueMessageString[iQueueMessageStringSize], fmt, argptr );
    BASSERT( iSize >= 0, szQueueMessageString[iQueueMessageStringSize] = 0; return );
    iQueueMessageStringSize += iSize;

    cMessagesMutex.unlock();
    va_end(argptr);
}


//----------------------------------------------------------------------------------------------------------------
void CUtil::PrintMessagesInQueue()
{
    if ( (iQueueMessageStringSize > 0) && cMessagesMutex.try_lock() )
    {
        Message(good::ELogLevelInfo, NULL, szQueueMessageString);
        iQueueMessageStringSize = 0;
        cMessagesMutex.unlock();
    }
}

//----------------------------------------------------------------------------------------------------------------
FILE *CUtil::OpenFile( const good::string& szFile, const char *szMode )
{
    FILE *fp = fopen(szFile.c_str(), szMode);

    if ( fp == NULL )
    {
        good::file::make_folders( szFile.c_str() );
        fp = fopen(szFile.c_str(), szMode);
    }

    return fp;
}

//----------------------------------------------------------------------------------------------------------------
const good::string& CUtil::BuildFileName( const good::string& sFolder, const good::string& sFile, const good::string& sExtension )
{
    static good::string_buffer sbResult(szMainBuffer, iMainBufferSize, false);
    sbResult.erase();

    sbResult << CBotrixPlugin::instance->sBotrixPath;
    sbResult << PATH_SEPARATOR
             << sFolder << PATH_SEPARATOR << sFile;

    if ( sExtension.length() )
        sbResult << "." << sExtension;

    return sbResult;
}


//----------------------------------------------------------------------------------------------------------------
// Draw functions.
//----------------------------------------------------------------------------------------------------------------
void CUtil::DrawBeam( const Vector& v1, const Vector& v2, unsigned char iWidth, float fDrawTime, unsigned char r, unsigned char g, unsigned char b )
{
    pEffects->Beam( v1, v2, CWaypoint::iWaypointTexture,
        0, 0, 1, fDrawTime,  // haloIndex, frameStart, frameRate, LifeTime
        iWidth, iWidth, 255, // width, endWidth, fadeLength
        1, r, g, b, 200, 10  // noise, R, G, B, brightness, speed
    );
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::DrawLine( const Vector& v1, const Vector& v2, float fDrawTime, unsigned char r, unsigned char g, unsigned char b )
{
    if (pVDebugOverlay)
        pVDebugOverlay->AddLineOverlay(v1, v2, r, g, b, false, fDrawTime);
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::DrawBox( const Vector& vOrigin, const Vector& vMins, const Vector& vMaxs, float fDrawTime, unsigned char r, unsigned char g, unsigned char b, const QAngle& ang )
{
    if (pVDebugOverlay)
        pVDebugOverlay->AddBoxOverlay(vOrigin, vMins, vMaxs, ang, r, g, b, 0, fDrawTime);
}

//----------------------------------------------------------------------------------------------------------------
void CUtil::DrawText( const Vector& vOrigin, int iLine, float fDrawTime, unsigned char, unsigned char, unsigned char, const char* szText )
{
    if (pVDebugOverlay)
        pVDebugOverlay->AddTextOverlay(vOrigin, iLine, fDrawTime, szText );
}
