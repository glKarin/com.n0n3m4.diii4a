#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


CLASS_DECLARATION( hhMonsterAI, hhCrawler )
	EVENT( EV_Touch,				hhCrawler::Event_Touch )
END_CLASS


void hhCrawler::Spawn() {
	if( spawnArgs.FindKey("def_pickup") ) {
		if ( gameLocal.isMultiplayer ) {
			GetPhysics()->SetContents( CONTENTS_TRIGGER | CONTENTS_MONSTERCLIP );
		} else {
			GetPhysics()->SetContents( CONTENTS_TRIGGER );
		}
	}	
}

void hhCrawler::Event_Touch( idEntity *other, trace_t *trace ) {
	hhPlayer *player;

	if( !spawnArgs.FindKey("def_pickup") || !other->IsType(idPlayer::Type) ) {
		return;
	}

	player = static_cast<hhPlayer *>(other);

	Pickup( player );
}

void hhCrawler::Pickup( hhPlayer *player ) {

	if( !GiveToPlayer( player ) ) {
		return;
	}

	// play pickup sound
	StartSoundShader( refSound.shader, SND_CHANNEL_ITEM, false );			// play what's defined in the entity

	// trigger our targets
	ActivateTargets( player );

	// clear our contents so the object isn't picked up twice
	GetPhysics()->SetContents( 0 );

	// hide the model
	Hide();

	if (player->hud) {
		player->hud->SetStateInt("item", 1);
		player->hud->SetStateString("itemtext", spawnArgs.GetString("inv_name"));
		player->hud->SetStateString("itemicon", spawnArgs.GetString("inv_icon"));
	}

	idStr str;
	spawnArgs.GetString("inv_name", "Item", str);

	if (player == gameLocal.GetClientByNum(gameLocal.localClientNum)) {
		gameLocal.Printf("Picked up a %s\n", str.c_str());
	}


	PostEventMS( &EV_Remove, 2000 );
}

bool hhCrawler::GiveToPlayer( hhPlayer* player ) {
	const char *pickupName = spawnArgs.GetString("def_pickup", NULL);
	bool pickedUp = false;

	if ( player && pickupName ) {
		idEntity *ent = gameLocal.SpawnObject(pickupName, NULL);
		if (ent->IsType(hhItem::Type)) {
			pickedUp = player->GiveItem( static_cast<hhItem*>(ent) );
		}
		ent->Hide();
		ent->PostEventMS(&EV_Remove, 2000);
	}

	return pickedUp;
}

void hhCrawler::Think( void ) {
	PROFILE_SCOPE("AI", PROFMASK_NORMAL|PROFMASK_AI);
	if (ai_skipThink.GetBool()) { //HUMANHEAD rww
		return;
	}

	if ( thinkFlags & TH_THINK ) {
	    current_yaw += deltaViewAngles.yaw;
	    ideal_yaw = idMath::AngleNormalize180( ideal_yaw + deltaViewAngles.yaw );
	    deltaViewAngles.Zero();
	    viewAxis = idAngles( 0, current_yaw, 0 ).ToMat3();

	    // HUMANHEAD NLA
	    physicsObj.ResetNumTouchEnt(0);
	    // HUMANHEAD END
		// animation based movement
		UpdateAIScript();
		AnimMove();
	} else if ( thinkFlags & TH_PHYSICS ) {
		RunPhysics();
	}

	UpdateAnimation();
	Present();
	LinkCombat();
}

bool hhCrawler::UpdateAnimationControllers() {
	//do nothing
	return false;
}