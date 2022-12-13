#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhWeaponCrawlerGrenade
	
***********************************************************************/
const idEventDef EV_SpawnBloodSpray( "spawnBloodSpray" );

CLASS_DECLARATION( hhWeapon, hhWeaponCrawlerGrenade )
	EVENT( EV_SpawnBloodSpray,				hhWeaponCrawlerGrenade::Event_SpawnBloodSpray )
END_CLASS

/*
=================
hhWeaponCrawlerGrenade::WriteToSnapshot
=================
*/
void hhWeaponCrawlerGrenade::WriteToSnapshot( idBitMsgDelta &msg ) const {
	hhWeapon::WriteToSnapshot(msg);
	msg.WriteBits(WEAPON_ALTMODE, 1);
}

/*
=================
hhWeaponCrawlerGrenade::ReadFromSnapshot
=================
*/
void hhWeaponCrawlerGrenade::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hhWeapon::ReadFromSnapshot(msg);
	WEAPON_ALTMODE = !!msg.ReadBits(1);
}

/*
=================
hhWeaponCrawlerGrenade::Event_SpawnBloodSpray
=================
*/
void hhWeaponCrawlerGrenade::Event_SpawnBloodSpray() {
	hhFxInfo fxInfo;
	fxInfo.UseWeaponDepthHack( true );
	if (WEAPON_ALTMODE) {
		BroadcastFxInfoAlongBonePrefix( dict, "fx_blood", "joint_AltBloodFx", &fxInfo );
	}
	else {
		BroadcastFxInfoAlongBonePrefix( dict, "fx_blood", "joint_bloodFx", &fxInfo );
	}
}