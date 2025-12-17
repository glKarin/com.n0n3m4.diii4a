#ifdef BOTRIX_MOD_CSS

#ifndef __BOTRIX_EVENT_CSS_H__
#define __BOTRIX_EVENT_CSS_H__


#include "event.h"


//================================================================================================================
/// This event is fired when new round starts.
//================================================================================================================
class CRoundStartEvent: public CEvent
{
public:
    CRoundStartEvent(): CEvent("round_start") {}

    void Execute( IEventInterface* pEvent );
};


//================================================================================================================
/// This event is fired when round ends.
//================================================================================================================
class CRoundEndEvent: public CEvent
{
public:
    CRoundEndEvent(): CEvent("round_end") {}

    void Execute( IEventInterface* pEvent );
};


//================================================================================================================
/// This event is fired when bomb is picked up.
//================================================================================================================
class CBombPickupEvent: public CEvent
{
public:
    CBombPickupEvent(): CEvent("bomb_pickup") {}

    void Execute( IEventInterface* pEvent );
};


//================================================================================================================
/// This event is fired when player can hear other player's footstep.
//================================================================================================================
class CPlayerFootstepEvent: public CEvent
{
public:
    CPlayerFootstepEvent(): CEvent("player_footstep") {}

    void Execute( IEventInterface* pEvent );
};


//================================================================================================================
/// This event is fired every time player shoots weapon.
//================================================================================================================
class CPlayerShootEvent: public CEvent
{
public:
    CPlayerShootEvent(): CEvent("player_shoot") {}

    void Execute( IEventInterface* pEvent );
};



//================================================================================================================
/// This event is fired when bomb is dropped.
//================================================================================================================
class CBombDroppedEvent: public CEvent
{
public:
    CBombDroppedEvent(): CEvent("bomb_dropped") {}

    void Execute( IEventInterface* pEvent );
};


//================================================================================================================
/// This event is fired when player can hear other player's gunshot.
//================================================================================================================
class CWeaponFireEvent: public CEvent
{
public:
    CWeaponFireEvent(): CEvent("weapon_fire") {}

    void Execute( IEventInterface* pEvent );
};


//================================================================================================================
/// This event is fired when player can hear bullet's impact.
//================================================================================================================
class CBulletImpactEvent: public CEvent
{
public:
    CBulletImpactEvent(): CEvent("bullet_impact") {}

    void Execute( IEventInterface* pEvent );
};


#endif // __BOTRIX_EVENT_CSS_H__

#endif // BOTRIX_MOD_CSS
