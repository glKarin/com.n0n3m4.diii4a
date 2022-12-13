
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


const idEventDef EV_Hand_DoRemove( "DoHandRemove", NULL );
const idEventDef EV_Hand_DoAttach( "DoHandAttach", "e" );
const idEventDef EV_Hand_Remove( "RemoveHand", NULL );

const idEventDef EV_Hand_Ready( "<ready>" );
const idEventDef EV_Hand_Lowered( "<lowered>" );
const idEventDef EV_Hand_Raise( "<raise>" );

CLASS_DECLARATION( hhAnimatedEntity, hhHand )
	EVENT ( EV_Hand_DoRemove,			hhHand::Event_DoHandRemove )
	EVENT ( EV_Hand_DoAttach,			hhHand::Event_DoHandAttach )
	EVENT ( EV_Hand_Remove,				hhHand::Event_RemoveHand )
	EVENT ( EV_Hand_Ready,				hhHand::Event_Ready )
	EVENT ( EV_Hand_Lowered,			hhHand::Event_Lowered )
	EVENT ( EV_Hand_Raise,				hhHand::Event_Raise )
END_CLASS

/*
============
hhHand::~hhHand
============
*/
hhHand::~hhHand() {
	bool shouldWarn = (gameLocal.GameState() != GAMESTATE_SHUTDOWN && !gameLocal.isClient);
	//rww - don't warn about this on client, since snapshot entities are free to be removed
	//at any time.

	// Check if the hand deleted is still in the hands list
	if ( gameLocal.hands.Find( this ) ) {
		if ( shouldWarn ) {
			gameLocal.Warning( "The hand (%p/%s) was deleted before being removed",
								this, spawnArgs.GetString( "classname" ) );
		}
		gameLocal.hands.Remove( this );
	}

	if ( owner != NULL ) {
		if ( shouldWarn ) {
			gameLocal.Warning( "A hand (%p) was deleted while having an owner", this );
		}
	}

	StopSound( SND_CHANNEL_ANY );
}
                                                                      

/*
============
hhHand::Spawn
============
*/
void hhHand::Spawn( void ) {
	gameLocal.hands.Append( idEntityPtr<hhHand> (this) );

	owner = NULL;
	previousHand = NULL;

	renderEntity.weaponDepthHack = true;

	animEvent = NULL;

	priority = spawnArgs.GetInt("priority");

	status = HS_UNKNOWN;
	idealState = HS_READY;

	animBlendFrames	= 4;	// needs blending
	animDoneTime = 0;

	attached = false;
	attachTime = -1;

	spawnArgs.GetBool( "replace_previous", "0", replacePrevious );

	spawnArgs.GetBool( "lower_weapon", "1", lowerWeapon );
	spawnArgs.GetBool( "aside_weapon", "1", asideWeapon );

	handedness 	= spawnArgs.GetInt( "handedness", "2" );

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( NULL, 1.0f );
	physicsObj.SetOrigin( GetOrigin() );
	physicsObj.SetAxis( GetAxis() );
	SetPhysics( &physicsObj );

	// Lower overrides aside
	if ( lowerWeapon && asideWeapon ) {
		asideWeapon = false;
	}
	
	Hide();
}

void hhHand::Save(idSaveGame *savefile) const {
	//savefile->WriteObject( owner );
	owner.Save(savefile);
	savefile->WriteInt( priority );
	savefile->WriteObject( previousHand );
	savefile->WriteBool( lowerWeapon );
	savefile->WriteBool( asideWeapon );
	savefile->WriteBool( attached );
	savefile->WriteBool( replacePrevious );
	savefile->WriteInt( attachTime );
	savefile->WriteInt( handedness );
	savefile->WriteInt( animDoneTime );
	savefile->WriteInt( animBlendFrames );
	savefile->WriteInt( status );
	savefile->WriteInt( idealState );
	savefile->WriteEventDef(animEvent);
	savefile->WriteStaticObject( physicsObj );
}

void hhHand::Restore( idRestoreGame *savefile ) {
	//savefile->ReadObject( reinterpret_cast<idClass *&>(owner) );
	owner.Restore(savefile);
	savefile->ReadInt( priority );
	savefile->ReadObject( reinterpret_cast<idClass *&>(previousHand) );
	savefile->ReadBool( lowerWeapon );
	savefile->ReadBool( asideWeapon );
	savefile->ReadBool( attached );
	savefile->ReadBool( replacePrevious );
	savefile->ReadInt( attachTime );
	savefile->ReadInt( handedness );
	savefile->ReadInt( animDoneTime );
	savefile->ReadInt( animBlendFrames );
	savefile->ReadInt( status );
	savefile->ReadInt( idealState );
	savefile->ReadEventDef(animEvent);
	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );
}

//rww - network code
void hhHand::WriteToSnapshot( idBitMsgDelta &msg ) const {
	//physicsObj.WriteToSnapshot(msg);
	//rww - trying to keep this all local

	WriteBindToSnapshot(msg);

	msg.WriteBits(owner.GetSpawnId(), 32);

	if (previousHand) {
		msg.WriteBits(gameLocal.GetSpawnId(previousHand), 32);
	}
	else {
		msg.WriteBits(0, 32);
	}

	msg.WriteBits(attached, 1);
	msg.WriteBits(replacePrevious, 1);
	//msg.WriteBits(attachTime, 32);
	//msg.WriteBits(animDoneTime, 32);
	//msg.WriteBits(animBlendFrames, 32);

	assert(idealState < (1<<4));
	msg.WriteBits(idealState, 4);

	assert(status < (1<<4));
	msg.WriteBits(status, 4);

	msg.WriteBits(IsHidden(), 1);

	assert(gameLocal.hands.Num() < (1<<4));
	msg.WriteBits(gameLocal.hands.Num(), 4);
	int i = 0;
	while (i < gameLocal.hands.Num()) {
		hhHand *hand = gameLocal.hands[i].GetEntity();

		if (hand) {
			msg.WriteBits(gameLocal.GetSpawnId(hand), 32);
		}
		else {
			msg.WriteBits(0, 32);
		}

		i++;
	}

	msg.WriteBits(renderEntity.allowSurfaceInViewID, GENTITYNUM_BITS);

	idAnimatedEntity::WriteToSnapshot(msg);
}

void hhHand::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	//physicsObj.ReadFromSnapshot(msg);
	//rww - trying to keep this all local

	ReadBindFromSnapshot(msg);

	if (owner.SetSpawnId(msg.ReadBits(32))) {
		physicsObj.SetSelfOwner(owner.GetEntity());
		if (owner.IsValid() && owner.GetEntity()) {
			DoHandAttach(owner.GetEntity());
		} else {
			owner = NULL;
		}
	}

	//Show();

	idEntityPtr<hhHand> newPrevHand;
	int prevHandSpawnId = msg.ReadBits(32);
	if (newPrevHand.SetSpawnId(prevHandSpawnId)) {
		previousHand = newPrevHand.GetEntity();
	}
	else {
		previousHand = NULL;
	}

	attached = !!msg.ReadBits(1);
	replacePrevious = !!msg.ReadBits(1);
	//attachTime = msg.ReadBits(32);
	//animDoneTime = msg.ReadBits(32);
	//animBlendFrames = msg.ReadBits(32);

	idealState = msg.ReadBits(4);

	int nextStatus = msg.ReadBits(4);
	if (status != nextStatus) {
		if (nextStatus == HS_LOWERING || nextStatus == HS_LOWERED) {
			PutAway();
		}
		else if (nextStatus == HS_RAISING) {
			Raise();
		}
		else if (nextStatus == HS_READY) {
		 	//PlayAnim( -1, "idle" );
			Ready();
		}
		status = nextStatus;
	}

	bool hidden = !!msg.ReadBits(1);
	if (hidden != IsHidden()) {
		if (hidden) {
			Hide();
		} else {
			Show();
		}
	}

	int numHands = msg.ReadBits(4);
	gameLocal.hands.SetNum(numHands);
	int i = 0;
	while (i < numHands) {
		int handSpawnId = msg.ReadBits(32);
		gameLocal.hands[i].SetSpawnId( handSpawnId );

		i++;
	}

	renderEntity.allowSurfaceInViewID = msg.ReadBits(GENTITYNUM_BITS);

	idAnimatedEntity::ReadFromSnapshot(msg);

	/*
	if (msg.HasChanged()) {
		Present();
	}
	*/
}

void hhHand::ClientPredictionThink( void ) {
	BecomeActive(TH_THINK|TH_ANIMATE);
	UpdateVisuals();
	idAnimatedEntity::ClientPredictionThink();
}

bool hhHand::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	switch (event) {
		case EVENT_REMOVEHAND: {
			RemoveHand();
			return true;
		}
		default: {
			return hhAnimatedEntity::ClientReceiveEvent(event, time, msg);
		}
	}
}

/*
============
hhHand::Event_Ready
============
*/
void hhHand::Event_Ready() {
	Show();
	Ready();
}


/*
===========
hhHand::Event_Lowered
===========
*/
void hhHand::Event_Lowered() {
	status = HS_LOWERED;
	Hide();
}


/*
============
hhHand::Event_Raise
============
*/
void hhHand::Event_Raise() {
	Raise();
}


/*
============
hhHand::Ready
============
*/
void hhHand::Ready() {

	switch ( idealState ) {

	  case HS_READY:
		CycleAnim(-1, "idle", 500);
	 	status = HS_READY;
		break;
		
	  case HS_LOWERED:
	 	PutAway();
	 	break;

	  default:
		gameLocal.Warning( "hhHand::Ready: Unsupported ideal state: %d", idealState );
		break;
			
	}
}


/*
============
hhHand::Raise
============
*/
void hhHand::Raise() {

	Show();

	if ( IsRaising() ) {	// If already raising, return
		return;
	}

	status = HS_RAISING;
	PlayAnim( -1, "raise", &EV_Hand_Ready );
}


/*
============
hhHand::PutAway
============
*/
void hhHand::PutAway() {

	if ( IsLowering() ) {
		// already being put away, so don't play more than one put away animations
		return;
	}
	
	status = HS_LOWERING;
	PlayAnim( -1, "putaway", &EV_Hand_Lowered ); 
}


/*
===============
hhHand::CycleAnim
===============
*/
void hhHand::CycleAnim( int channel, const char *animname, int blendTime ) {
	if ( channel < 0 ) {
		channel = GetChannelForAnim( animname );
	}

	int anim = GetAnimator()->GetAnim( animname );
	if ( anim ) {
		GetAnimator()->CycleAnim( channel, anim, gameLocal.time, blendTime );
	}
}

/*
===============
hhHand::PlayAnim
===============
*/
void hhHand::PlayAnim( int channel, const char *animname, const idEventDef *event ) {
	int anim;

	// HUMANHEAD nla
	if ( animEvent ) {
		CancelEvents( animEvent );
	}
	animEvent = event;	

	if ( channel < 0 ) {
		channel = GetChannelForAnim( animname );
	}
	// HUMANHEAD END

	anim = GetAnimator()->GetAnim( animname );
	if ( anim ) {
		GetAnimator()->PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = GetAnimator()->CurrentAnim( channel )->GetEndTime();
	} else {
		// This is a valid case, some hands (mounted gun) don't animate
		GetAnimator()->Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	}
	animBlendFrames = 4;

	// HUMANHEAD nla
	if ( animEvent ) {
		PostEventMS( animEvent, animDoneTime - gameLocal.time );
	}
	// HUMANHEAD END
}


/*
============
hhHand::AttachHand
Returns false if the hand can not be attached
============
*/
bool hhHand::AttachHand( hhPlayer *player, bool attachNow ) {
	assert(player);

	if ( !CheckHandAttach( player ) ) {
		return( false );
	}

	int loweringFinished = LowerHandOrWeapon( player );
	SetOwner( player );
	Bind( player, true );

	if ( loweringFinished >= 0 ) {
		player->handNext = this;
		// If the attachment should wait, and we don't want to force it to happen now
		if ( loweringFinished - gameLocal.time > 0 && !attachNow ) {
			PostEventMS( &EV_Hand_DoAttach, loweringFinished - gameLocal.time, player );
		}
		else {
			Event_DoHandAttach( player );
		}
		attachTime = loweringFinished;
	}

	return( true );
}

/*
============
hhHand::LowerHandOrWeapon
  Used when the hand adds itself.
  returns the time when the lowering will be done
  		will be -1 if the hand has already been attached.  
============
*/
int hhHand::LowerHandOrWeapon( hhPlayer *player ) {
	int animDone = gameLocal.time;
	int animDone2;

	// If there is a 'next' hand, we can just replace it, as it is already playing the down anim for the hand
	if ( player->handNext.IsValid() ) {
		animDone = player->handNext->GetAttachTime();		

		gameLocal.hands.Remove( player->handNext.GetEntity() );

		// Clear the old hand of any pending events, and then remove it
		player->handNext->SetOwner( NULL );
		player->handNext->CancelEvents( NULL );
		player->handNext->ProcessEvent( &EV_Remove );
		
		player->handNext = NULL;
	}
	// Else, If there is just a hand, play the down anim on it or replace it
	else if ( player->hand.IsValid() ) {
		//! Work out this logic more fully
		if ( replacePrevious ) {
			ReplaceHand( player );
			animDone = -1;
		}
		// If it isn't on it's way down, move it down
		if ( !player->hand->IsLowered() ) {
			player->hand->PutAway();
			animDone = player->hand->GetAnimDoneTime();
		}
	}
	
	animDone2 = HandleWeapon( player, this, animDone - gameLocal.time );
	if ( ( animDone >= 0 ) && ( animDone2 > animDone ) ) {
		animDone = animDone2;
	}
	// If there is a weapon play the down anim on it
	/*
	if ( player->hhweapon ) {		
		if ( lowerWeapon && !player->hhweapon->IsLowered() ) {
			player->hhweapon->PutAway();
			animDone2 = player->hhweapon->GetAnimDoneTime();
			if ( animDone2 > animDone ) {
				animDone = animDone2;
			}
 		}
 		else if ( asideWeapon && !player->hhweapon->IsAside() ) {
 			player->hhweapon->PutAside();
 			// When aside, we want to have it play right away, so dont' change animDone
 		}
 		// gameLocal.Printf("Scheduled down for %.2f\n", animDone);
	}
	*/

	return( animDone );	
}

/*
==============
RaiseHandOrWeapon

  Assumes the hand is all the way down
==============
*/	
void hhHand::RaiseHandOrWeapon( hhPlayer *player ) {
	hhHand *theHand = NULL;
	int	raiseDelay = 0;

	// Get the next hand
	theHand = this->GetPreviousHand( player );
	
	raiseDelay = HandleWeapon( player, theHand );
	
	if ( theHand && ( !theHand->IsRaising() || !theHand->IsReady() ) ) {
		theHand->PostEventMS( &EV_Hand_Raise, raiseDelay - gameLocal.time );
	}
}


/*
================
hhHand::Reraise
  Raises the hand again.
================
*/
void hhHand::Reraise( ) {
	hhPlayer *thePlayer = NULL;
	int raiseDelay = 0;
	
	CancelEvents( &EV_Remove );
	CancelEvents( &EV_Hand_Raise );
	
	if ( owner.IsValid() && owner.GetEntity() && owner->IsType( hhPlayer::Type ) ) {
		thePlayer = ( hhPlayer * ) owner.GetEntity();
	}
	else {
		gameLocal.Printf( "hhHand::Reraise: Warning tried to reraise when not on an hhPlayer (%p)\n", owner );
		return;
	}

	raiseDelay = HandleWeapon( thePlayer, this );
	
	if ( !IsRaising() || !IsReady() ) {
		PostEventMS( &EV_Hand_Raise, raiseDelay - gameLocal.time );	
	}
}


/*
===========
hhHand::GetPreviousHand
===========
*/
hhHand *hhHand::GetPreviousHand( hhPlayer *player ) {
	hhHand *prevHand = NULL;

	// If there is no player, just give them our previous hand
	if ( player == NULL ) {
		return( previousHand );
	}

	// If we are the next hand, up the current hand
	if ( player->handNext == this ) {
		prevHand = player->hand.GetEntity();
	}	
	// Else If we are the current hand
	else if ( player->hand == this ) {
	 	// If we have a previous hand, raise it if it isn't already up or being raised
		if ( previousHand ) {
			prevHand = previousHand;
		}	 	
	}
	// Else we are neither, throw a warning.  How are we here?
	else {
		gameLocal.Warning( "ERROR GetPreviousHand: ERROR: Tried to get a previous hand when we (%p) are not on the player (%p/%p)",
							this, player->handNext, player->hand );
	}

	return( prevHand );
}


/*
============
hhHand::HandleWeapon
  Set the weapon in the proper place for the hand
    Assumes the hand in question will be raised.
    Returns time (MS) to wait until the hand can be raised.  Only in the
    case of the weapon being lowered does it really matter/ > 0 
============
*/
int hhHand::HandleWeapon( hhPlayer *player, hhHand *hand, int weaponDelay, bool doNow ) {
	int 	raiseHandDelay = gameLocal.time;
	bool	weaponReady = ( hand == NULL ) && !player->ChangingWeapons();

	// If we don't have a weapon, then just return
	if ( !player->weapon.IsValid() || player->GetIdealWeapon() == 0 ) {
		return( 0 );
	}

	// If we have a hand to compare to
	if ( hand ) {
		// If the hand wants the weapon lowered
		if ( hand->lowerWeapon || ( hand->handedness & player->weapon->GetHandedness() ) ) {		
			player->weapon->PutAway();
			if ( doNow ) {
				player->weapon->SetState( "Down", 0 );
			}
			else {
				// Foce the gun to go, so we can know when it'll be done playing the anim.
				player->weapon->UpdateScript();
				raiseHandDelay = player->weapon->GetAnimDoneTime();
			}
			/*
			// If it isn't lowered, lower it
			if ( !player->weapon->IsLowered() ) {
				player->weapon->PutAway();
				raiseHandDelay = player->weapon->GetAnimDoneTime();
			}
			*/
		}	//. lower the weapon
		// If the hand wants the weapon to the side
		else if ( hand->asideWeapon ) {		
			player->weapon->PutAside();
			if ( doNow ) {
				player->weapon->SetState( "Aside", 0 );
			}
			// We only want a delay if the weapon should pop up?
			else if ( player->weapon->IsLowered() ) {
				// Foce the gun to go, so we can know when it'll be done playing the anim.
				player->weapon->UpdateScript();
				raiseHandDelay = player->weapon->GetAnimDoneTime();
				player->weapon->SetState( "PutAside", 4 );
			}
			/*
			// If it is lowered, 
			if ( player->weapon->IsLowered() ) {
				// First raise it, then set it aside
				int done = weaponDelay;				
				idAnim *anim = player->weapon->GetAnimator()->GetAnim( "raise" );	// UGLY Hack, as hardcoded to the name

				player->weapon->PostEventMS( &EV_Weapon_WeaponRising, weaponDelay );
				if ( anim ) { done = anim->Length(); }
				player->weapon->PostEventMS( &EV_Weapon_Aside, done );
			}
			// Else if it is ready/upright and not aside
			else if ( !player->weapon->IsAside() ) {
				// Just set it aside	
				player->weapon->PutAside();
			}
			*/
		}
		// Weapon should be in ready mode
		else {
			weaponReady = true;		
		}
	}	// Valid hand

	// If we don't have a hand, so just make sure the weapon is raised
	if ( weaponReady ) {
		player->weapon->Raise();
		if ( doNow ) {
			player->weapon->SetState( "Idle", 0 );
		}
		
		/*

		// Clear any Aside events that may be posted. Happens when it was lowered, and the previous hand wanted aside		
		player->weapon->CancelEvents( &EV_Weapon_Aside );
		
		// If the weapon is aside, put it upright
		if ( player->weapon->IsAside() ) {
			player->weapon->PutUpright();
		}
		// Else if it is lowering, raise it
		else if ( !player->weapon->IsRising() && player->weapon->IsLowered() ) {
			player->weapon->Raise();
		}
		// Else if we aren't ready, how can we not be??!?
		else if ( player->weapon->IsLowered() || player->weapon->IsLowered() ) {
			gameLocal.Warning( "hhHand::HandleWeapon ERROR: Have a weapon that is lowered/lowering when shouldn't be!" );
		}
		*/
	}
	
	return( raiseHandDelay );
}


/*
============
hhHand::RemoveHand
  Plays the animations and schedules pointer changes
============
*/
bool hhHand::RemoveHand( void ) {
	hhPlayer	*player;
	int 		animDone = gameLocal.time;

	if ( owner.IsValid() && owner.GetEntity() ) {
		if ( owner->IsType( hhPlayer::Type ) ) {
			player = static_cast<hhPlayer *>( owner.GetEntity() );
		}
		else {
			gameLocal.Warning( "ERROR: RemoveHand: Tried to remove from a non player" );
			return( false );
		}
	}
	else {	
		gameLocal.Warning( "ERROR: RemoveHand: Tried to remove with no owner!!" );
		return( false );			
	}

	if ( !CheckHandRemove( player ) ) {
		return( false );
	}

	// Unaside is asap if we asided it, and the prev hand isn't gonna want it asided
	// NLA - If we want to upright the GUI
	hhHand *prevHand = GetPreviousHand( player );
	if ( ( !prevHand || !prevHand->asideWeapon ) && 
		 ( asideWeapon && player->weapon.IsValid() && player->weapon->IsAside() ) ){
		player->weapon->PutUpright();			
	}

	CancelEvents( &EV_Hand_DoRemove );

	// Play down anim
	if( !IsLowering() ) {
		PutAway();
		animDone = GetAnimDoneTime();
		PostEventMS( &EV_Hand_DoRemove, animDone - gameLocal.time );
	}
	else if ( IsLowered() ) { 		// If already down, just remove it now
		Event_DoHandRemove();
	}
	
	return( true );
}


/*
============
hhHand::SetOwner
============
*/
void hhHand::SetOwner( idActor *owner ) {

	this->owner = owner;

	if (owner) {
		if( GetPhysics() && GetPhysics()->IsType(hhPhysics_StaticWeapon::Type) ) {
			static_cast<hhPhysics_StaticWeapon*>(GetPhysics())->SetSelfOwner( owner );
		}

		// only show the surface in player view
		renderEntity.allowSurfaceInViewID = owner->entityNumber+1;
	}
}


/*
============
hhHand::ReplaceHand
============
*/
void hhHand::ReplaceHand( hhPlayer *player) {
	//int animDone;
	hhHand *hand = NULL;
	
	//! What to do if hand is null?
	if( !player || !player->hand.IsValid() ) {
		return;
	}
	hand = player->hand.GetEntity();

	// Copy over any key info needed
	this->previousHand = hand->previousHand;
	
	HandleWeapon( player, this );

	player->hand = this;
	hand->attached = false;
	hand->owner = NULL;
	hand->Hide();
	hand->PostEventMS( &EV_Remove, 0 );
	
	gameLocal.hands.Remove( hand );
	
	attached = true;
	Show();		
	Raise();
}


/*
============
hhHand::CheckHandAttach
============
*/
bool hhHand::CheckHandAttach( hhPlayer *owner ) {
	bool debug = 0;

	if ( attached ) {
		gameLocal.Warning( "Tried to attach an already attached hand." );
		return( false );
	}

	if( !owner ) {
		if ( debug ) { gameLocal.Printf( "Out because of owner\n" ); }
		return( false );
	}
	
	// More important hand there.  Abort
	if ( owner->hand.IsValid() && ( GetPriority() < owner->hand->GetPriority() ) ) {
		if ( debug ) { gameLocal.Printf( "Out because of hand priority\n" ); }
		return( false );
	}

	// More important hand about to be there.  Abort
	if ( owner->handNext.IsValid() && ( GetPriority() < owner->handNext->GetPriority() ) ) {
		if ( debug ) { gameLocal.Printf( "Out because of next hand priority\n" ); }
		return( false );
	}

	//? What do to if both equal priority?
	if ( owner->hand.IsValid() && ( GetPriority() == owner->hand->GetPriority() ) ) {
		// Same hand, abort!
		if ( ( owner->hand->spawnArgs.GetString( "classname" ) == spawnArgs.GetString( "classname" ) ) && ( owner->hand != this ) ) {
			if ( debug ) { gameLocal.Printf( "Out because of same hand\n" ); }
			return( false );
		}
	}

	//? What do to if both equal priority?
	if ( owner->handNext.IsValid() && ( GetPriority() == owner->handNext->GetPriority() ) ) {
		// Same hand, abort!
		hhHand* currentHand = owner->handNext.GetEntity();
		if ( ( owner->handNext->spawnArgs.GetString( "classname" ) == spawnArgs.GetString( "classname" ) ) && ( owner->handNext != this ) ) {
			if ( debug ) { gameLocal.Printf( "Out because of same next hand\n" ); }
			return( false );
		}
	}

	return( true );
}


/*
============
hhHand::CheckHandRemove
============
*/
bool hhHand::CheckHandRemove( hhPlayer *player ) {

	// Check have a valid player
	if ( player == NULL ) {
		gameLocal.Warning( "ERROR: DoHandRemove: We are not attached to a player!");
		return( false );
	}

	// We aren't here!
	if ( IsAttached() && ( player->hand != this ) ) {
		gameLocal.Warning( "ERROR: DoHandRemove: Tried to detach from an player we are not assigned to %d %p %p",
							(int) IsAttached(), player->hand, this );
		return( false );
	}

	return( true );
}

/*
============
hhHand::SetModel
============
*/
void hhHand::SetModel( const char *modelname ) {
	hhAnimatedEntity::SetModel( modelname );
}

/*
============
hhHand::Present
============
*/
void hhHand::Present() {
	hhAnimatedEntity::Present();
}

/*
============
hhHand::Event_DoHandAttach
============
*/
void hhHand::Event_DoHandAttach( idEntity *owner ) {
	DoHandAttach( owner );
}

/*
============
hhHand::DoHandAttach
============
*/
bool hhHand::DoHandAttach( idEntity *owner ) {
	hhPlayer *player;

	if ( owner && owner->IsType( hhPlayer::Type ) ) {
		player = static_cast<hhPlayer *>( owner );
	}
	else {
		gameLocal.Warning("ERROR: Tried to attach to a non-player");
		return( false );
	}

	if ( !CheckHandAttach( player ) ) {
		PostEventMS( &EV_Remove, 0 );
		return( false );
	}

	// Sanity check, we should be the top dogh
	if ( player->handNext != this && !gameLocal.isClient ) { //rww - don't care on client, because we cannot rely on handNext
		gameLocal.Warning( "ERROR: We (%p) should be the next hand (%p) but aren't", 
							this, player->handNext );
	}

	// Make sure nothing has changed
	HandleWeapon( player, this );

	// We are top dog, replace!  :)
	if ( !replacePrevious ) {
		previousHand = player->hand.GetEntity();
	}
	else {
		previousHand = NULL;
	}
	player->hand = this;
	player->handNext = NULL;

	attached = true;
	attachTime = -1;

	Raise();
	Show();

	return( true );
}

/*
============
hhHand::Event_DoHandRemove
============
*/
void hhHand::Event_DoHandRemove( void ) {
	DoHandRemove( );
}


/*
============
hhHand::ForceRemove
  - Should only be called when you don't need other hands/weaspons adjusted
============
*/
void hhHand::ForceRemove( void ) {

	gameLocal.hands.Remove( this );

	owner = NULL;

	Hide();
}

/*
============
hhHand::DoHandRemove
============
*/
bool hhHand::DoHandRemove( void ) {
	hhPlayer	*player;

	gameLocal.hands.Remove( this );

	if ( owner.IsValid() && owner.GetEntity() && owner->IsType( hhPlayer::Type ) ) {
		player = static_cast<hhPlayer *>( owner.GetEntity() );
	}
	else {
		gameLocal.Warning( "ERROR: DoHandRemove: Tried to remove from a non player" );
		return( false );
	}

	// Play up anim
	RaiseHandOrWeapon( player );

	if ( !CheckHandRemove( player ) ) {
		return( false );
	}

	// Do the actual removal logic
	Hide();
	PostEventMS( &EV_Remove, 0 );

	if ( IsAttached() ) {
		player->hand = previousHand;
		previousHand = NULL;

		attached = false;
	}
	
	// If we were the next hand, clear it
	if ( player->handNext == this ) {
		player->handNext = NULL;
	}

	owner = NULL;

	return( true );
}

/*
============
hhHand::Event_RemoveHand
  Plays the animations and schedules pointer changes
============
*/
void hhHand::Event_RemoveHand( void ) {
	RemoveHand();
}

/*
===========
hhHand::AddHand
===========
*/
hhHand *hhHand::AddHand( hhPlayer *player, const char *handclass, bool attachNow ) {
	idDict args;
	hhHand *hand = NULL;

	args.SetVector( "origin", player->GetEyePosition() );
	args.SetMatrix( "rotation", player->viewAngles.ToMat3() );

	hand = static_cast< hhHand * >( gameLocal.SpawnObject( handclass, &args) );
	if ( hand ) {
		hand->SetOwner( player );
		// We have a prob if you can't attach.  Should return NULL
		if ( !hand->AttachHand( player, attachNow ) ) {
			hand->SetOwner( NULL );

			gameLocal.hands.Remove( hand );

			hand->PostEventMS( &EV_Remove, 0 );
			
			return( NULL );
		}
	}

	return( hand );
}


/*
============
hhHand::PrintHandInfo
============
*/
void hhHand::PrintHandInfo( idPlayer *player ) {
	hhPlayer *hhplayer = NULL;
	hhHand *hand = NULL;
	int count = 0;
	idList< idEntityPtr< hhHand > > orphaned;
	
	if ( player->IsType( hhPlayer::Type ) ) {
		hhplayer = (hhPlayer *) player;
	}	
	else {
		return;
	}

	orphaned = gameLocal.hands;

  	gameLocal.Printf( "Weapon: %p Class: %s  State: %d\n", player->weapon,
  			(const char *) player->weapon->GetDict()->GetString("classname"), 
  			(int) player->weapon->GetStatus() );

	hand = hhplayer->hand.GetEntity();
	while ( hand != NULL ) {
		count++;
	  	gameLocal.Printf( "Hand %d: %p Class: %s  State: %d\n", count, hand,
  				hand->spawnArgs.GetString("classname"), (int) hand->GetStatus() );
  		orphaned.Remove( hand );
  		hand = hand->previousHand;	
	}  
  
  	count = 0;
  	for ( int i = 0; i < orphaned.Num(); ++i ){ 
		count++;
	 	hand = orphaned[ i ].GetEntity();
    	gameLocal.Printf( "Orphaned Hand %d: %p Class: %s  State: %d\n", count, hand,
  				hand->spawnArgs.GetString("classname"), (int) hand->GetStatus() );
  	}
}


/*
================
hhHand::GetMasterDefaultPosition
================
*/
void hhHand::GetMasterDefaultPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const {
	idActor*	actor = NULL;
	idEntity*	master = GetBindMaster();

	if( master ) {
		if( master->IsType(idActor::Type) ) {
			actor = static_cast<idActor*>( master );
			actor->DetermineOwnerPosition( masterOrigin, masterAxis );

			masterOrigin = actor->ApplyLandDeflect( masterOrigin, 1.1f );
		} else {
			hhAnimatedEntity::GetMasterDefaultPosition( masterOrigin, masterAxis );
		}
	}
}
