#ifndef __BOTRIX_EVENT_H__
#define __BOTRIX_EVENT_H__


#include <good/memory.h>

#include "types.h"
#include "server_plugin.h"


//****************************************************************************************************************
/// Game event (like player hurt, game start, etc).
//****************************************************************************************************************
class CEvent: public IGameEventListener2
{
public:
    /// Constructor.
    CEvent( const char* szType ): m_sType(szType)
    {
        CBotrixPlugin::pGameEventManager->AddListener(this, szType, true);
    }

    /// Destructor.
    virtual ~CEvent()
    {
        CBotrixPlugin::pGameEventManager->RemoveListener(this);
    }

    /// Return name of this event.
    const good::string& GetName() const { return m_sType; }

    /// Called directly by EventManager if event just occured.
    virtual void FireGameEvent( IGameEvent* pEvent ) = 0;

protected:
    good::string m_sType;
};

typedef good::shared_ptr<CEvent> CEventPtr; ///< Typedef for unique_ptr of CEvent.


//================================================================================================================
/// This event is fired when player finished connecting to server.
//================================================================================================================
class CPlayerActivateEvent: public CEvent
{
public:
    CPlayerActivateEvent(): CEvent("player_activate") {}

    virtual void FireGameEvent( IGameEvent* pEvent );
};


//================================================================================================================
/// This event is fired every time player changes team.
//================================================================================================================
class CPlayerTeamEvent: public CEvent
{
public:
    CPlayerTeamEvent(): CEvent("player_team") {}

    virtual void FireGameEvent( IGameEvent* pEvent );
};


//================================================================================================================
/// This event is fired every time player spawns on server.
//================================================================================================================
class CPlayerSpawnEvent: public CEvent
{
public:
    CPlayerSpawnEvent(): CEvent("player_spawn") {}

    virtual void FireGameEvent( IGameEvent* pEvent );
};


//================================================================================================================
/// This event is fired when player chats.
//================================================================================================================
class CPlayerChatEvent: public CEvent
{
public:
    CPlayerChatEvent(): CEvent("player_say") {}

    virtual void FireGameEvent( IGameEvent* pEvent );
};


//================================================================================================================
/// This event is fired when player is hurted by another player.
//================================================================================================================
class CPlayerHurtEvent: public CEvent
{
public:
    CPlayerHurtEvent(): CEvent("player_hurt") {}

    virtual void FireGameEvent( IGameEvent* pEvent );
};


//================================================================================================================
/// This event is fired when player is dead.
//================================================================================================================
class CPlayerDeathEvent: public CEvent
{
public:
    CPlayerDeathEvent(): CEvent("player_death") {}

    virtual void FireGameEvent( IGameEvent* pEvent );
};


//TODO: MOVE TO TF2 EVENTS.
/*class CRoundStartEvent: public CEvent
{
public:
    CRoundStartEvent(): CEvent("teamplay_round_start")
    {
        CBotrixPlugin::pGameEventManager->AddListener(this, "teamplay_round_selected", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "teamplay_round_active", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "teamplay_waiting_begins", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "teamplay_waiting_ends", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "teamplay_waiting_abouttoend", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "teamplay_restart_round", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "teamplay_ready_restart", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "teamplay_round_restart_seconds", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "teamplay_team_ready", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "teamplay_round_win", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "arena_player_notification", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "arena_match_maxstreak", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "arena_round_start", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "arena_win_panel", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "player_spawn", true);
        CBotrixPlugin::pGameEventManager->AddListener(this, "teamplay_pre_round_time_left", true);
//        CBotrixPlugin::pGameEventManager->AddListener(this, "", true);
//        CBotrixPlugin::pGameEventManager->AddListener(this, "", true);
//        CBotrixPlugin::pGameEventManager->AddListener(this, "", true);
//        CBotrixPlugin::pGameEventManager->AddListener(this, "", true);
//        CBotrixPlugin::pGameEventManager->AddListener(this, "", true);
//        CBotrixPlugin::pGameEventManager->AddListener(this, "", true);
    }

    virtual void FireGameEvent( IGameEvent* pEvent );
};
*/

#endif // __BOTRIX_EVENT_H__
