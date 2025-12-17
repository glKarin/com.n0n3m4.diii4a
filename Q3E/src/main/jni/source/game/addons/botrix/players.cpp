#include <good/string_buffer.h>
#include <good/string_utils.h>

#include "chat.h"
#include "clients.h"
#include "mod.h"
#include "players.h"
#include "server_plugin.h"
#include "type2string.h"
#include "waypoint.h"
#include "mods/borzh/bot_borzh.h"
#include "mods/hl2dm/bot_hl2dm.h"
#include "mods/tf2/bot_tf2.h"

extern char* szMainBuffer;
extern int iMainBufferSize;


//----------------------------------------------------------------------------------------------------------------
bool CPlayers::bAddingBot = false;
int CPlayers::iBotsPlayersCount = 0;
float CPlayers::fPlayerBotRatio = 0.0f;

good::vector<CPlayerPtr> CPlayers::m_aPlayers(16);
CClient* CPlayers::m_pListenServerClient = NULL;

int CPlayers::m_iClientsCount = 0;
int CPlayers::m_iBotsCount = 0;
bool CPlayers::m_bCheckBotCountFinished = true;

good::string CPlayers::m_sLastError;

#if defined(DEBUG) || defined(_DEBUG)
    bool CPlayers::m_bClientDebuggingEvents = true;
#else
    bool CPlayers::m_bClientDebuggingEvents = false;
#endif


//----------------------------------------------------------------------------------------------------------------
CPlayer::CPlayer(edict_t* pEdict, bool bIsBot):
    iCurrentWaypoint(EWaypointIdInvalid), iNextWaypoint(EWaypointIdInvalid), iPrevWaypoint(EWaypointIdInvalid),
    iChatMate(-1), m_pEdict(pEdict), m_iIndex(-1),
    m_pPlayerInfo(CBotrixPlugin::pPlayerInfoManager->GetPlayerInfo(m_pEdict)),
	m_fEndProtectionTime(0), m_bBot(bIsBot), m_bAlive(false), m_bProtected(false) {}

//----------------------------------------------------------------------------------------------------------------
void CPlayer::Activated()
{
    BLOG_T( "Player %s activated.", GetName() );

    m_iIndex = CPlayers::GetIndex(m_pEdict);

    m_sName.assign(GetName(), good::string::npos, true);
    good::lower_case(m_sName);
}

//----------------------------------------------------------------------------------------------------------------
void CPlayer::Respawned()
{
    BLOG_T( "Player %s respawned.", GetName() );

#if DRAW_PLAYER_HULL
    m_fNextDrawHullTime = 0.0f;
#endif
    CUtil::EntityHead( m_pEdict, m_vHead );
    iCurrentWaypoint = CWaypoints::GetNearestWaypoint( m_vHead );
	m_bAlive = m_bProtected = true;

	// A value less than 0 means that player is forever protected.
	if (m_fEndProtectionTime >= 0)
	{
		float defaultEndProtectionTime = CBotrixPlugin::fTime + CMod::fSpawnProtectionTime;
		// Player can be protected for longer time by console command.
		m_fEndProtectionTime = MAX(m_fEndProtectionTime, defaultEndProtectionTime);
	}
}

//----------------------------------------------------------------------------------------------------------------
void CPlayer::PreThink()
{
    if ( !m_bAlive && m_bBot ) // Don't update current waypoint for dead bots.
        return;

    if ( m_pPlayerInfo->IsDead() ) // CBasePlayer::IsDead() returns true only when player became dead,  but when
        m_bAlive = false;          // player is respawnable (but still dead) it returns false.

    if ( m_bAlive )
    {
        if ( m_bProtected && m_fEndProtectionTime >= 0 && CBotrixPlugin::fTime >= m_fEndProtectionTime )
            m_bProtected = false;
    }

#if DRAW_PLAYER_HULL
    static const float fDrawTime = 0.1f;
    if ( CBotrixPlugin::fTime >= m_fNextDrawHullTime )
    {
        m_fNextDrawHullTime = CBotrixPlugin::fTime + fDrawTime;
        Vector vPos = m_pPlayerInfo->GetAbsOrigin();
        Vector vMins = m_pPlayerInfo->GetPlayerMins();
        Vector vMaxs = m_pPlayerInfo->GetPlayerMaxs();
        CUtil::DrawBox(vPos, vMins, vMaxs, fDrawTime, 0xFF, 0xFF, 0xFF);
    }
#endif

    m_vPrevHead = m_vHead;
    CUtil::EntityHead( m_pEdict, m_vHead );

    // If waypoint is not valid or we are too far, recalculate current waypoint.
    float fDist;
    if ( !CWaypoint::IsValid(iCurrentWaypoint) ||
       ( (fDist = m_vHead.DistToSqr(CWaypoints::Get(iCurrentWaypoint).vOrigin)) > SQR(CWaypoint::MAX_RANGE) ) )
    {
        iPrevWaypoint = iCurrentWaypoint = CWaypoints::GetNearestWaypoint( m_vHead );
        return;
    }

    // Check if we are moved too far away from current waypoint as to be near to one of its neighbour.
    TWaypointId iNewWaypoint = EWaypointIdInvalid;

    CWaypoints::WaypointNode& w = CWaypoints::GetNode(iCurrentWaypoint);
    if ( CWaypoint::IsValid(iNextWaypoint) ) // There is next waypoint in path, check if we are closer to it.
    {
        CWaypoint& n = CWaypoints::Get(iNextWaypoint);
        float fNewDist = m_vHead.DistToSqr(n.vOrigin);
        if ( fNewDist < fDist )
            iNewWaypoint = iNextWaypoint;
    }
    else
    {
        for (int i = 0; i < w.neighbours.size(); ++i)
        {
            TWaypointId id = w.neighbours[i].target;
            CWaypoint& n = CWaypoints::Get(id);
            float fNewDist = m_vHead.DistToSqr(n.vOrigin);
            if ( fNewDist < fDist )
            {
                iNewWaypoint = id;
                fDist = fNewDist;
            }
        }
    }

    if ( CWaypoint::IsValid(iNewWaypoint) )
    {
        iPrevWaypoint = iCurrentWaypoint;
        iCurrentWaypoint = iNewWaypoint;
    }
}








//********************************************************************************************************************
void CPlayers::GetNames( StringVector& aNames, bool bGetBots, bool bGetUsers )
{
	for ( good::vector<CPlayerPtr>::const_iterator it = m_aPlayers.begin(); it != m_aPlayers.end(); ++it )
		if ( *it && ( ( bGetBots && ( *it )->IsBot() ) || ( bGetUsers && !( *it )->IsBot() ) ) )
			aNames.push_back( ( *it )->GetName() );
}

void CPlayers::CheckBotsCount()
{
    if ( !CBotrixPlugin::instance->bMapRunning ||
         ( (fPlayerBotRatio == 0.0f) && (iBotsPlayersCount == 0) ) )
        return;

    int iNeededCount = fPlayerBotRatio ? GetClientsCount()*fPlayerBotRatio : iBotsPlayersCount - GetClientsCount();
    if ( iNeededCount < 0 )
        iNeededCount = 0;
    else if ( iNeededCount + GetClientsCount() > Size()-1 )
        iNeededCount = Size()-1 - GetClientsCount(); // Save one space for player.

    if ( iNeededCount == GetBotsCount() )
        m_bCheckBotCountFinished = true;
    else
    {
        m_bCheckBotCountFinished = false;

        if ( iNeededCount > GetBotsCount() )
            AddBot(NULL, CBot::iDefaultTeam, CBot::iDefaultClass, -1);
        else
            KickRandomBot();
    }
}

//----------------------------------------------------------------------------------------------------------------
void CPlayers::InvalidatePlayersWaypoints()
{
    // Invalidate current / destination waypoints for all players.
    for ( int iPlayer = 0; iPlayer < Size(); ++iPlayer )
    {
        CPlayer *pPlayer = Get( iPlayer );
        if ( pPlayer )
        {
            if ( !pPlayer->IsBot() ) // TODO: virtual function for players to invalidate waypoints.
                ( (CClient *)pPlayer )->iDestinationWaypoint = EWaypointIdInvalid;

            // Force move failure, because path can contain removed waypoint.
            pPlayer->iCurrentWaypoint = pPlayer->iNextWaypoint = pPlayer->iPrevWaypoint = EWaypointIdInvalid;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
void CPlayers::Init( int iMaxPlayers )
{
    m_aPlayers.clear();
    m_aPlayers.resize( iMaxPlayers );
    m_bCheckBotCountFinished = false;
}

//----------------------------------------------------------------------------------------------------------------
void CPlayers::Clear()
{
    for ( int i = 0; i < CPlayers::Size(); ++i )
    {
        CPlayer* pPlayer = m_aPlayers[i].get();
        if ( pPlayer )
        {
            if ( pPlayer->IsBot() )
                KickBot(pPlayer);
            m_aPlayers[i].reset();
        }
    }
    m_pListenServerClient = NULL;
    m_iClientsCount = m_iBotsCount = 0;
    m_bClientDebuggingEvents = false;
}


//----------------------------------------------------------------------------------------------------------------
CPlayer* CPlayers::AddBot( const char* szName, TTeam iTeam, TClass iClass,
                           TBotIntelligence iIntelligence, int argc, const char** argv )
{
    if ( !CBotrixPlugin::instance->bMapRunning )
    {
        m_sLastError = "Error: no map is loaded.";
        return NULL;
    }

    if ( iIntelligence == -1 )
        iIntelligence = ( rand() % (CBot::iMaxIntelligence - CBot::iMinIntelligence + 1) ) + CBot::iMinIntelligence;

    if ( !szName || !szName[0] )
    {
        const good::string& sName = CMod::GetRandomBotName(iIntelligence);
        if ( CMod::bIntelligenceInBotName )
        {
            good::string_buffer sbNameWithIntelligence(szMainBuffer, iMainBufferSize, false); // Don't deallocate.
            sbNameWithIntelligence = sName;
            sbNameWithIntelligence.append(' ');
            sbNameWithIntelligence.append('(');
            sbNameWithIntelligence.append(CTypeToString::IntelligenceToString(iIntelligence));
            sbNameWithIntelligence.append(')');
            szName = sbNameWithIntelligence.c_str();
        }
        else
            szName = sName.c_str();
    }

    if ( (iClass == -1) && CMod::aClassNames.size() )
         iClass = rand() % CMod::aClassNames.size();

    bAddingBot = true;
    CPlayer* pBot = CMod::pCurrentMod->AddBot( szName, iIntelligence, iTeam, iClass, argc, argv );
    bAddingBot = false;

    if ( pBot )
    {
        TPlayerIndex iIndex = CBotrixPlugin::pEngineServer->IndexOfEdict( pBot->GetEdict() ) - 1;
        GoodAssert( iIndex >= 0 );
        m_aPlayers[iIndex] = CPlayerPtr(pBot);
    }
    else
        m_sLastError = CMod::pCurrentMod->GetLastError();
    return pBot;
}

//----------------------------------------------------------------------------------------------------------------
void CPlayers::KickBot( CPlayer* pPlayer )
{
    BASSERT( pPlayer && pPlayer->IsBot() );
    char szCommand[16];
    sprintf( szCommand,"kickid %d\n", pPlayer->GetPlayerInfo()->GetUserID() ); // \n ???
    CBotrixPlugin::pEngineServer->ServerCommand(szCommand);
}

//----------------------------------------------------------------------------------------------------------------
bool CPlayers::KickRandomBot()
{
    return KickRandomBotOnTeam(-1);
}

//----------------------------------------------------------------------------------------------------------------
bool CPlayers::KickRandomBotOnTeam( int iTeam )
{
    if ( m_iBotsCount == 0 )
        return false;

    int index = rand() % m_aPlayers.size();

    // Search for used bot from index to left.
    CBot* toKick = NULL;
    int i;

    // TODO: not use 2 for.
    for (i = index; i >= 0; --i)
    {
        CPlayer* pPlayer = m_aPlayers[i].get();
        if ( pPlayer && pPlayer->IsBot() &&
             ( (iTeam == -1) || (pPlayer->GetTeam() == iTeam) ) )
        {
            toKick = (CBot*)pPlayer;
            break;
        }
    }

    // Search for used bot from index to right.
    if (toKick == NULL)
        for (i = index+1; i < (int)m_aPlayers.size(); ++i)
        {
            CPlayer* pPlayer = m_aPlayers[i].get();
            if ( pPlayer && pPlayer->IsBot() &&
                 ( (iTeam == -1) || (pPlayer->GetTeam() == iTeam) ) )
            {
                toKick = (CBot*)pPlayer;
                break;
            }
        }

    if ( toKick )
        KickBot(toKick);

    return ( toKick != NULL );
}

//----------------------------------------------------------------------------------------------------------------
void CPlayers::PlayerConnected( edict_t* pEdict )
{
    //IPlayerInfo* pPlayerInfo = CBotrixPlugin::pPlayerInfoManager->GetPlayerInfo(pEdict);
    //if ( pPlayerInfo && pPlayerInfo->IsConnected() ) // IsPlayer() IsFakeClient() not working
    if ( bAddingBot )
    {
        m_iBotsCount++;
    }
    else
    {
        TPlayerIndex iIdx = CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict)-1;
        GoodAssert( iIdx >= 0 ); // Valve should not allow this assert.

        BASSERT( m_aPlayers[iIdx].get() == NULL, return ); // Should not happend.

        CPlayer* pPlayer = new CClient(pEdict);

        if ( !CBotrixPlugin::pEngineServer->IsDedicatedServer() && (m_pListenServerClient == NULL) )
        {
            // Give listenserver client all access to bot commands.
            m_pListenServerClient = (CClient*)pPlayer;
            ((CClient*)pPlayer)->iCommandAccessFlags = FCommandAccessAll;
        }

        m_aPlayers[iIdx] = CPlayerPtr(pPlayer);
        m_iClientsCount++;

#ifdef BOTRIX_CHAT
        if ( CChat::iPlayerVar != EChatVariableInvalid )
        {
            good::string sName(pPlayer->GetName(), true, true); // Copy and deallocate.
            sName.lower_case();
            CChat::SetVariableValue( CChat::iPlayerVar, iIdx, sName );
        }
#endif

        CPlayers::CheckBotsCount();
    }
}


//----------------------------------------------------------------------------------------------------------------
void CPlayers::PlayerDisconnected( edict_t* pEdict )
{
    int iIdx = CBotrixPlugin::pEngineServer->IndexOfEdict(pEdict)-1;
    GoodAssert( iIdx >= 0 ); // Valve should not allow this assert.

    if ( !m_aPlayers[iIdx].get() )
        return; // Happens when starting new map and pressing cancel button. Valve issue.

    CPlayer* pPlayer = m_aPlayers[iIdx].get();
    BASSERT( pPlayer, return );

#ifdef BOTRIX_CHAT
    if ( CChat::iPlayerVar != EChatVariableInvalid )
        CChat::SetVariableValue( CChat::iPlayerVar, iIdx, "" );
#endif

    // Notify bots that player is disconnected.
    for ( int i=0; i < Size(); ++i)
    {
        CPlayer* pBot = m_aPlayers[i].get();
        if ( pBot && pBot->IsBot() && pBot->IsAlive() )
            ((CBot*)pBot)->PlayerDisconnect(iIdx, pPlayer);
    }

    bool bIsBot = pPlayer->IsBot();
    m_aPlayers[iIdx].reset();

    if ( bIsBot )
        m_iBotsCount--;
    else
    {
        m_iClientsCount--;
        if ( !CBotrixPlugin::pEngineServer->IsDedicatedServer() && (m_pListenServerClient == pPlayer) )
            m_pListenServerClient = NULL;

        CheckForDebugging();
        CPlayers::CheckBotsCount();
    }

    GoodAssert( (m_iBotsCount >= 0) && (m_iClientsCount >= 0) );
}


//----------------------------------------------------------------------------------------------------------------
void CPlayers::PreThink()
{
    if ( !m_bCheckBotCountFinished )
        CheckBotsCount();

    for (good::vector<CPlayerPtr>::iterator it = m_aPlayers.begin(); it != m_aPlayers.end(); ++it)
        if ( it->get() )
            it->get()->PreThink();
}


//----------------------------------------------------------------------------------------------------------------
void CPlayers::DebugEvent( const char *szFormat, ... )
{
    static char buffer[128];
    va_list args;
    va_start(args, szFormat);
    vsprintf(buffer, szFormat, args);
    va_end(args);

    for ( good::vector<CPlayerPtr>::const_iterator it = m_aPlayers.begin(); it != m_aPlayers.end(); ++it )
    {
        const CPlayer* pPlayer = it->get();
        if ( pPlayer  &&  !pPlayer->IsBot()  &&  ((CClient*)pPlayer)->bDebuggingEvents )
            BULOG_I(pPlayer->GetEdict(), buffer);
    }
}


//----------------------------------------------------------------------------------------------------------------
void CPlayers::CheckForDebugging()
{
    m_bClientDebuggingEvents = false;
    for (good::vector<CPlayerPtr>::iterator it = m_aPlayers.begin(); it != m_aPlayers.end(); ++it)
    {
        const CPlayer* pPlayer = it->get();
        if ( pPlayer  &&  !pPlayer->IsBot()  &&  ((CClient*)pPlayer)->bDebuggingEvents )
        {
            m_bClientDebuggingEvents = true;
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
#ifdef BOTRIX_CHAT
void CPlayers::DeliverChat( edict_t* pFrom, bool bTeamOnly, const char* szText )
{
    int iTeam = 0;

    int iIdx = CPlayers::Get(pFrom);
    BASSERT( iIdx >= 0, return );
    CPlayer* pSpeaker = CPlayers::Get(iIdx);

    if ( bTeamOnly )
        iTeam = pSpeaker->GetTeam();

    // Get command from text.
    if ( GetBotsCount() > 0 )
    {
        static CBotChat cChat;
        cChat.iBotChat = EBotChatUnknown;
        cChat.iDirectedTo = EPlayerIndexInvalid;
        cChat.iSpeaker = iIdx;

        float fPercentage = CChat::ChatFromText( szText, cChat );
        bool bIsRequest = (fPercentage >= 6.0f);

/*
        if ( cChat.iDirectedTo == -1 )
            cChat.iDirectedTo = pSpeaker->iChatMate;
        else
            pSpeaker->iChatMate = cChat.iDirectedTo; // TODO: SetChatMate();
*/
        int iFrom = 0, iTo = CPlayers::Size();
        if ( bIsRequest && (cChat.iDirectedTo != -1) )
        {
            BASSERT( cChat.iDirectedTo != iIdx ); // TODO:???
            iFrom = cChat.iDirectedTo;
            iTo = iFrom + 1;
        }
        //else
        //	BLOG_E("Can't detect player, chat is directed to.");


        // Deliver chat for all bots.
        for ( TPlayerIndex iPlayer = iFrom; iPlayer < iTo; ++iPlayer )
        {
            CPlayer* pReceiver = CPlayers::Get(iPlayer);
            if ( pReceiver && pReceiver->IsBot() && (iIdx != iPlayer) )
            {
                CBot* pBot = (CBot*)pReceiver;
                if ( !bTeamOnly || (pBot->GetTeam() == iTeam) ) // Should be on same iTeam if chat is for iTeam only.
                {
                    if ( bIsRequest )
                        pBot->ReceiveChatRequest(cChat);
                    else
                        pBot->ReceiveChat( iIdx, pSpeaker, bTeamOnly, szText );
                }
            }
        }
    }
}
#else
void CPlayers::DeliverChat( edict_t*, bool, const char* ) {}
#endif // BOTRIX_CHAT
