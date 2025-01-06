// DnItemShotgun.cpp
//

#include "../Gamelib/Game_local.h"

CLASS_DECLARATION(idEntity, DnItemShotgun)
	EVENT(EV_Touch, DnItem::Event_Touch)
END_CLASS

/*
=================
DnItemShotgun::TouchEvent
=================
*/
void DnItemShotgun::TouchEvent(DukePlayer* player, trace_t* trace) {
	if (!player->WeaponAvailable("weapon_shotgun"))
	{
		player->Give("weapon", "weapon_shotgun");

		player->Event_DukeTalk("duke_useshotgun");
	}

	DnItem::TouchEvent(player, trace);
}
