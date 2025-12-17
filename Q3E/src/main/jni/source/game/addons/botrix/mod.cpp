#include <good/string_utils.h>

#include "bot.h"
#include "mod.h"
#include "players.h"
#include "server_plugin.h"
#include "type2string.h"
#include "waypoint.h"

#include "mods/css/event_css.h"
#include "mods/borzh/mod_borzh.h"
#include "mods/hl2dm/mod_hl2dm.h"
#include "mods/tf2/mod_tf2.h"

//----------------------------------------------------------------------------------------------------------------
TModId CMod::m_iModId;
good::string CMod::sModName;
IMod* CMod::pCurrentMod = NULL;

StringVector CMod::aBotNames;
good::vector<CEventPtr> CMod::m_aEvents;
good::vector< good::pair<TFrameEvent, TPlayerIndex> > CMod::m_aFrameEvents;

bool CMod::m_bMapHas[ EItemTypeCanPickTotal ]; // Health, armor, weapon, ammo.

float CMod::fMinNonStuckSpeed = 30;

StringVector CMod::aTeamsNames;
int CMod::iUnassignedTeam = 0;
int CMod::iSpectatorTeam = 1;

good::vector<TWeaponId> CMod::aDefaultWeapons;

// Next are console commands
bool CMod::bRemoveWeapons = false;

StringVector CMod::aClassNames;

bool CMod::bIntelligenceInBotName = true;
bool CMod::bHeadShotDoesMoreDamage = true;
bool CMod::bUseModels = true;

float CMod::fSpawnProtectionTime = 0;
int CMod::iSpawnProtectionHealth = 0;

//TDeathmatchFlags CMod::iDeathmatchFlags = -1;
good::vector<float> CMod::m_aVars[EModVarTotal];

Vector CMod::vPlayerCollisionHull;
Vector CMod::vPlayerCollisionHullCrouched;

Vector CMod::vPlayerCollisionHullMins;
Vector CMod::vPlayerCollisionHullMaxs;

Vector CMod::vPlayerCollisionHullCrouchedMins;
Vector CMod::vPlayerCollisionHullCrouchedMaxs;

Vector CMod::vPlayerCollisionHullMaxsGround;

int CMod::iPlayerRadius;

int CMod::iNearItemMaxDistanceSqr = SQR(312);
int CMod::iItemPickUpDistance = 100;

int CMod::iPointTouchSquaredXY;
int CMod::iPointTouchSquaredZ;
int CMod::iPointTouchLadderSquaredZ;


//----------------------------------------------------------------------------------------------------------------
bool CMod::LoadDefaults( TModId iModId )
{
    m_iModId = iModId;
    m_aEvents.clear();
    m_aEvents.reserve(16);

    m_aFrameEvents.clear();
    m_aFrameEvents.reserve(8);

    float defaults[ EModVarTotal ] = {
        /*max_health = */100,
        /*max_armor = */100,

        /*player_width = */36,
        /*player_height = */72,
        /*player_height_crouched = */36,
        /*player_eye = */64,
        /*player_eye_crouched = */36,

        /*velocity_crouch = */63.33,
        /*velocity_walk = */150,
        /*velocity_run = */190,
        /*velocity_sprint = */327.5,

        /*obstacle_height_no_jump = */18,
        /*jump_height = */20,
        /*jump_height_crouched = */56,

        /*max_height_no_fall_damage = */185,
        /*max_slope_gradient = */45,
    };
    for ( TModVar iVar = 0; iVar < EModVarTotal; ++iVar )
        m_aVars[ iVar ].resize( 1, defaults[ iVar ] );

    bool bResult = true;
    switch ( iModId )
    {
#ifdef BOTRIX_TF2
        case EModId_TF2:
        {
            bHeadShotDoesMoreDamage = false;
            AddEvent( new CPlayerHurtEvent );
            AddEvent( new CPlayerSpawnEvent );
            AddEvent( new CPlayerDeathEvent );
            AddEvent( new CPlayerActivateEvent );
            AddEvent( new CPlayerTeamEvent );

            //AddEvent(new CRoundStartEvent);
            // AddEvent(new CPlayerChatEvent);

            // TODO: events https://wiki.alliedmods.net/Team_Fortress_2_Events
            // player_changeclass ctf_flag_captured
            // teamplay_point_startcapture achievement_earned
            // player_calledformedic

            pCurrentMod = new CModTF2();
            break;
        }
#endif

#ifdef BOTRIX_HL2DM
        case EModId_HL2DM:
        {
            AddEvent( new CPlayerActivateEvent() );
            AddEvent( new CPlayerTeamEvent() );
            AddEvent( new CPlayerSpawnEvent() );
            AddEvent( new CPlayerHurtEvent() );
            AddEvent( new CPlayerDeathEvent() );
            //AddEvent(new CPlayerChatEvent());

            pCurrentMod = new CModHL2DM();
            break;
        }
#endif

#ifdef BOTRIX_BORZH
        case EModId_Borzh:
            pCurrentMod = new CModBorzh();
            break;
#endif

#ifdef BOTRIX_MOD_CSS
        case EModId_CSS:
            AddEvent( new CRoundStartEvent() );
            AddEvent( new CWeaponFireEvent() );
            AddEvent( new CBulletImpactEvent() );
            AddEvent( new CPlayerFootstepEvent() );
            AddEvent( new CBombDroppedEvent() );
            AddEvent( new CBombPickupEvent() );
            pCurrentMod = new CModCss();
            break;
#endif

        default:
            BLOG_E( "Error: unknown mod." );
            BreakDebugger();
    }

    fMinNonStuckSpeed = 30.0f;

    return bResult;
}


//----------------------------------------------------------------------------------------------------------------
void CMod::Prepare()
{
    float fWidth = m_aVars[ EModVarPlayerWidth ][ 0 ], fHalfWidth = fWidth / 2.0f;
    float fHeight = m_aVars[ EModVarPlayerHeight ][ 0 ];
    float fHeightCrouched = m_aVars[ EModVarPlayerHeightCrouched ][ 0 ];
    float fJumpCrouched = m_aVars[ EModVarPlayerJumpHeightCrouched ][ 0 ];

    iPlayerRadius = (int)FastSqrt( 2 * SQR( fHalfWidth ) ); // Pithagoras.
    iPointTouchSquaredXY = SQR( fWidth / 4 );
    iPointTouchSquaredZ = SQR( fJumpCrouched );
    iPointTouchLadderSquaredZ = SQR( 5 );

    CWaypoint::iAnalyzeDistance = fWidth * 2;
    CWaypoint::iDefaultDistance = fWidth * 4;
    
    // Get max collision hull, so bot doesn't stack (when auto-create waypoints).
    //fWidth *= FastSqrt( 2.0f );
    //fHalfWidth = fWidth / 2.0f;

    vPlayerCollisionHull = Vector( fWidth, fWidth, fHeight );
    vPlayerCollisionHullMins = Vector( -fHalfWidth, -fHalfWidth, 0 );
    vPlayerCollisionHullMaxs = Vector( fHalfWidth, fHalfWidth, fHeight );
    vPlayerCollisionHullMaxsGround = Vector( fHalfWidth, fHalfWidth, 1.0f );

    vPlayerCollisionHullCrouched = Vector( fWidth, fWidth, fHeightCrouched );
    vPlayerCollisionHullCrouchedMins = Vector( -fHalfWidth, -fHalfWidth, 0 );
    vPlayerCollisionHullCrouchedMaxs = Vector( fHalfWidth, fHalfWidth, fHeightCrouched );
}

//----------------------------------------------------------------------------------------------------------------
void CMod::MapLoaded()
{
    // TODO: move this to items.
    for ( TItemType iType=0; iType < EItemTypeCanPickTotal; ++iType )
    {
        m_bMapHas[iType] = false;

        if ( CItems::GetItems(iType).size() > 0 ) // Check if has items.
            m_bMapHas[iType] = true;
        else
        {
            // Check if map has waypoints of given type.
            TWaypointFlags iFlags = CWaypoint::GetFlagsFor(iType);

            for ( TWaypointId id=0; id < CWaypoints::Size(); ++id )
                if ( FLAG_SOME_SET(CWaypoints::Get(id).iFlags, iFlags) )
                {
                    m_bMapHas[iType] = true;
                    break;
                }
        }
    }

    if ( pCurrentMod )
        pCurrentMod->MapLoaded();
}

//----------------------------------------------------------------------------------------------------------------
const good::string& CMod::GetRandomBotName( TBotIntelligence iIntelligence )
{
    int iIdx = rand() % aBotNames.size();
    for ( int i=iIdx; i<aBotNames.size(); ++i )
        if ( !IsNameTaken(aBotNames[i], iIntelligence) )
            return aBotNames[i];
    for ( int i=iIdx-1; i>=0; --i )
        if ( !IsNameTaken(aBotNames[i], iIntelligence) )
            return aBotNames[i];
    if ( iIdx < 0 ) // All names taken.
        iIdx = rand() % aBotNames.size();
    return aBotNames[iIdx];
}


//----------------------------------------------------------------------------------------------------------------
bool CMod::IsNameTaken( const good::string& cName, TBotIntelligence iIntelligence )
{
    for ( TPlayerIndex iPlayer=0; iPlayer<CPlayers::Size(); ++iPlayer )
    {
        const CPlayer* pPlayer = CPlayers::Get(iPlayer);
        if ( pPlayer && good::starts_with( good::string(pPlayer->GetName()), cName ) &&
             pPlayer->IsBot() && ( iIntelligence == ((CBot*)pPlayer)->GetIntelligence()) )
            return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------
void CMod::Think()
{
    if ( pCurrentMod )
        pCurrentMod->Think();

    for ( int i = 0; i < m_aFrameEvents.size(); ++i )
    {
        CPlayer* pPlayer = CPlayers::Get( m_aFrameEvents[i].second );
        if ( pPlayer == NULL )
            BLOG_E( "Player with index %d is not present to receive event %d.", m_aFrameEvents[i].second, m_aFrameEvents[i].first );
        else
        {
            switch ( m_aFrameEvents[i].first )
            {
            case EFrameEventActivated:
                pPlayer->Activated();
                break;
            case EFrameEventRespawned:
                pPlayer->Respawned();
                break;
            default:
                GoodAssert(false);
            }
        }
    }

    m_aFrameEvents.clear();
}
