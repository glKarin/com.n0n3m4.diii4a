
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/*
TERMS:
    Transition: How to transition up and down
	  0 - Abruptly.  No up/down anims.  Just disappears
	  1 - With Anims.  Play the up/down anims.
*/

CLASS_DECLARATION( idClass, hhWeaponHandState )
END_CLASS

/*
============
hhWeaponHandState::hhWeaponHandState
============
*/
hhWeaponHandState::hhWeaponHandState( hhPlayer *aPlayer,
									  bool aTrackWeapon, int weaponTransition,
									  bool aTrackHand, int handTransition ){

	player = aPlayer;

	oldHand = NULL;
	oldWeaponNum = 0;

	oldHandUp = handTransition;
	oldWeaponUp = weaponTransition;

	trackHand = aTrackHand;
	trackWeapon = aTrackWeapon;
}

void hhWeaponHandState::Save(idSaveGame *savefile) const {
	player.Save(savefile);
	oldHand.Save(savefile);
	savefile->WriteInt(oldWeaponNum);
	savefile->WriteInt(oldHandUp);
	savefile->WriteInt(oldWeaponUp);
	savefile->WriteBool(trackHand);
	savefile->WriteBool(trackWeapon);
}

void hhWeaponHandState::Restore( idRestoreGame *savefile ) {
	player.Restore(savefile);
	oldHand.Restore(savefile);
	savefile->ReadInt(oldWeaponNum);
	savefile->ReadInt(oldHandUp);
	savefile->ReadInt(oldWeaponUp);
	savefile->ReadBool(trackHand);
	savefile->ReadBool(trackWeapon);
}

void hhWeaponHandState::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits(player.GetSpawnId(), 32);
	msg.WriteBits(oldHand.GetSpawnId(), 32);

	msg.WriteBits(oldWeaponNum, 32);
	msg.WriteBits(oldHandUp, 32);
	msg.WriteBits(oldWeaponUp, 32);
	msg.WriteBits(trackHand, 1);
	msg.WriteBits(trackWeapon, 1);
}

void hhWeaponHandState::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	player.SetSpawnId(msg.ReadBits(32));
	oldHand.SetSpawnId(msg.ReadBits(32));

	oldWeaponNum = msg.ReadBits(32);
	oldHandUp = msg.ReadBits(32);
	oldWeaponUp = msg.ReadBits(32);
	trackHand = !!msg.ReadBits(1);
	trackWeapon = !!msg.ReadBits(1);
}


/*
============
hhWeaponHandState::SetPlayer
============
*/
void hhWeaponHandState::SetPlayer( hhPlayer *aPlayer ) {
	player = aPlayer;
}

/*
============
hhWeaponHandState::Save
============
*/
int hhWeaponHandState::Archive( const char *newWeaponDef, int weaponTransition,
							 const char *newHandDef, int handTransition ) {
	
	if ( !player.IsValid() ) {
		gameLocal.Warning( "Error in hhWeaponHandState::Archive, no player set" );
	}

	// Do we care about the old hand?
	if ( trackHand ) {
		// Save the hand, break if no hand.  Must save for the restore
		oldHand = player->hand;

		if ( oldHand.IsValid() ) { 
			// If we are going to raise it later put down
			if ( oldHandUp ) {
				oldHand->PutAway();
			}
			
			// Make it disapear
			oldHand->Hide();

			// Make sure it doesn't come back (Ready makes it visible again)
			oldHand->CancelEvents(&EV_Hand_Ready);
		}

		// Clear the hand out
		player->hand = NULL;
	}

	// Do we care about the old weapon?
	if ( trackWeapon ) {
		// Save the weapon, break if none.  Must save for the restore
		oldWeaponNum = player->GetCurrentWeapon();
		if (oldWeaponNum == -1)
		{
			oldWeaponNum = player->GetIdealWeapon();
			assert(oldWeaponNum != -1);
		}
		//Calling destructor directly otherwise it doesn't get called until next frame
		if( player->weapon.IsValid() ) { 
			player->weapon->DeconstructScriptObject();
			player->weapon->Hide();
		}
		SAFE_REMOVE( player->weapon );
	}

	// See if we have a new weapon to put up
	if ( trackWeapon && newWeaponDef && newWeaponDef[0] ) {
		player->ForceWeapon( player->GetWeaponNum(newWeaponDef) );

		// Weapon_Combat() normally takes care of the lowering/raising logic, so force the raise here
		player->weapon->Raise();
		player->SetState( "RaiseWeapon" );

		// Now that we have the weapon, how do we get it there?
		if ( weaponTransition ) {
			player->weapon->Raise();
		}
		else {
			player->weapon->ProcessEvent( &EV_Weapon_State, "Idle", 0 );
		}
	}

	// Should we put up a new hand?
	if ( trackHand && newHandDef && !player->IsDeathWalking() ) {
		hhHand *newHand;		//! Debug,remove later

		// Create the new hand
		newHand = hhHand::AddHand( player.GetEntity(), newHandDef, true );

		if ( !newHand ) {
			gameLocal.Warning( "Error creating new hand for state change" );
			return( 0 );
		}
	
		// Make sure the weapon is pop'd into place
		newHand->HandleWeapon( player.GetEntity(), newHand, 0, true );
		// Now that the hand is up, how do we get it there?
		if ( handTransition ) {
			player->hand->Raise();
		}
		else {
			player->hand->Ready();
		}
	}
	
	return( 0 );
}


/*
============
hhWeaponHandState::RestoreFromArchive
============
*/
int hhWeaponHandState::RestoreFromArchive() {
	bool setWeapon = false;

	if ( !player.IsValid() ) {
		gameLocal.Warning( "Error in hhWeaponHandState::RestoreFromArchive, no player set" );
		return 0;
	}

	if ( trackWeapon ) {
		//Calling destructor directly otherwise it doesn't get called until next frame
		if( player->weapon.IsValid() ) { 
			player->weapon->DeconstructScriptObject();
			player->weapon->Hide();
		}
		SAFE_REMOVE( player->weapon );
		
		// Put up the old weapon if there
		if ( oldWeaponNum ) {
			player->ForceWeapon( oldWeaponNum );

			// Weapon_Combat() normally takes care of the lowering/raising logic, so force the raise here
			player->SetState( "RaiseWeapon" );

			if ( oldWeaponUp && player->health <= 0 ) {		//HUMANHEAD bjk PCF (4-29-06) - fix weapon after shuttle death
				// Make sure it is lowered
				player->weapon->ProcessEvent( &EV_Weapon_State, "Raise", 0 );
				player->weapon->UpdateScript();

				player->weapon->Raise();
			}
			else {
				player->weapon->ProcessEvent( &EV_Weapon_State, "Idle", 0 );
			}
		} else {
			player->ForceWeapon( -1 );
		}
		setWeapon = true;
	}
	

	if ( trackHand ) {
		// Remove the old hand - Need to set owner to NULL so it won't complain
		if ( player->hand.IsValid() ) {
			player->hand->ForceRemove();
		}
		SAFE_REMOVE( player->hand );

		hhHand *debugHand = oldHand.GetEntity();

		// Find the first hand that should be there.  Note: The 'if' could be made into a 'while' to do multiple levels
		if ( oldHand.IsValid() && !oldHand->IsValidFor( player.GetEntity() ) ) {
			idEntityPtr<hhHand> delHand;
			
			//gameLocal.Printf( "%d Removing weapon state hand\n", gameLocal.time );

			delHand = oldHand;		// Save the old hand for deletion
			
			// Move to the next hand
			oldHand = oldHand->GetPreviousHand();
			debugHand = oldHand.GetEntity();

			// delete the old old hand
			delHand->ForceRemove();
			SAFE_REMOVE( delHand );
		}

		player->hand = oldHand;

		// If valid, put it up/have it handle the weapon
		if ( oldHand.IsValid() ) {
			player->hand->Show();

			// Make sure the weapon is in the proper state
			player->hand->HandleWeapon( player.GetEntity(),
										player->hand.GetEntity(), 0, true );

			// If they want it raised, raise it
			if ( oldHandUp ) {
				player->hand->Raise();
			}
			// Else just pop it in there!
			else {
				player->hand->Ready();
			}
		}
		// If no hands in line, then set the weapon straight if we haven't set it already
		else if ( !setWeapon ) {
			player->weapon->ProcessEvent( &EV_Weapon_State, "Idle", 0 );
		}		//. Got a real hand?
	}	//. We care about the hand

	return( 0 );
}
