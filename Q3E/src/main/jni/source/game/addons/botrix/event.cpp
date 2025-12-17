#include "bot.h"
#include "clients.h"
#include "event.h"
#include "server_plugin.h"
#include "source_engine.h"

#ifdef BOTRIX_TF2
    #include "mods/tf2/bot_tf2.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//----------------------------------------------------------------------------------------------------------------
void CPlayerActivateEvent::FireGameEvent( IGameEvent* pEvent )
{
    int iUserId = pEvent->GetInt("userid");
    edict_t* pActivator = CUtil::GetEntityByUserId( iUserId );

    int iIdx = CPlayers::GetIndex(pActivator);
    GoodAssert( iIdx >= 0 );

    CMod::AddFrameEvent(iIdx, EFrameEventActivated);
}


//----------------------------------------------------------------------------------------------------------------
void CPlayerTeamEvent::FireGameEvent( IGameEvent* pEvent )
{
    edict_t* pActivator = CUtil::GetEntityByUserId( pEvent->GetInt("userid") );
    TTeam iTeam = pEvent->GetInt("team");

    int iIdx = CPlayers::GetIndex(pActivator);
    GoodAssert( iIdx >= 0 );

    CPlayer* pPlayer = CPlayers::Get(iIdx);
    if ( pPlayer && pPlayer->IsBot() )
        ((CBot*)pPlayer)->ChangeTeam( iTeam );
}


//----------------------------------------------------------------------------------------------------------------
void CPlayerSpawnEvent::FireGameEvent( IGameEvent* pEvent )
{
    edict_t* pActivator = CUtil::GetEntityByUserId( pEvent->GetInt("userid") );

    int iIdx = CPlayers::GetIndex(pActivator);
    GoodAssert( iIdx >= 0 );

    CMod::AddFrameEvent(iIdx, EFrameEventRespawned);
}


//----------------------------------------------------------------------------------------------------------------
void CPlayerChatEvent::FireGameEvent( IGameEvent* pEvent )
{
    edict_t* pActivator = CUtil::GetEntityByUserId( pEvent->GetInt("userid") );
    const char* szText = pEvent->GetString("text");
    bool bTeamOnly = pEvent->GetBool("teamonly");

    CPlayers::DeliverChat(pActivator, bTeamOnly, szText);
}


//----------------------------------------------------------------------------------------------------------------
void CPlayerHurtEvent::FireGameEvent( IGameEvent* pEvent )
{
    edict_t* pActivator = CUtil::GetEntityByUserId( pEvent->GetInt("userid") );
    int iActivator = CPlayers::GetIndex(pActivator);
    BASSERT( iActivator >= 0, return );

    edict_t* pAttacker = CUtil::GetEntityByUserId( pEvent->GetInt("attacker") );
    int iAttacker = pAttacker ? CPlayers::GetIndex(pAttacker) : iActivator; // May hurt himself.

    CPlayer *pPlayer = CPlayers::Get(iActivator);
    CPlayer *pPlayerAttacker = iAttacker >= 0 ? CPlayers::Get(iAttacker) : NULL;
	if (pPlayer && pPlayerAttacker && pPlayer->IsBot())
        ((CBot*)pPlayer)->HurtBy( iAttacker, pPlayerAttacker, pEvent->GetInt("health") );
}


//----------------------------------------------------------------------------------------------------------------
void CPlayerDeathEvent::FireGameEvent( IGameEvent* pEvent )
{
    edict_t* pActivator = CUtil::GetEntityByUserId( pEvent->GetInt("userid") );
    edict_t* pAttacker = CUtil::GetEntityByUserId( pEvent->GetInt("attacker") );

    int iActivator = CPlayers::GetIndex(pActivator);
    BASSERT( iActivator >= 0, return );

    CPlayer* pPlayerActivator = CPlayers::Get(iActivator);
    if ( pPlayerActivator )
        pPlayerActivator->Dead();

    if ( pAttacker )
    {
        int iAttacker = CPlayers::GetIndex(pAttacker);
        BASSERT( iAttacker >= 0, return );

        CPlayer* pPlayerAttacker = CPlayers::Get(iAttacker);
        if ( pPlayerAttacker && pPlayerAttacker->IsBot() )
            ((CBot*)pPlayerAttacker)->KilledEnemy(iActivator, pPlayerActivator);
    }
}


//----------------------------------------------------------------------------------------------------------------
/*void CRoundStartEvent::FireGameEvent( IGameEvent* pEvent )
{
    BLOG_I( pEvent->GetName() );
    //CBot_TF2::bCanJoinTeams = true;
}*/
