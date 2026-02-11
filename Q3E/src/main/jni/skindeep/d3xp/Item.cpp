/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "renderer/RenderSystem.h"
#include "renderer/tr_local.h"
#include "framework/DeclSkin.h"
#include "renderer/ModelManager.h"

#include "gamesys/SysCvar.h"
#include "Player.h"
#include "Fx.h"
#include "SmokeParticles.h"

#include "Misc.h"
#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"

#include "idlib/LangDict.h"
#include "bc_catcage.h"
#include "bc_lever.h"
#include "bc_meta.h"
#include "bc_skullsaver.h"
#include "bc_ftl.h"
#include "bc_tablet.h"
#include "bc_lostandfound.h"

#include "Item.h"


// SW: Number of milliseconds after dropping an item that we relinquish ownership over it.
const int MOVEABLE_PLAYEROWNERSHIP_LIFETIME = 500;

#define INTERESTPOINT_DISABLETIME		1500 //How long to temporarily disable interestpoint creation.


#define SPARK_TIME		5000 //how long does an item spark when it's bashed on something

/*
===============================================================================

  idItem

===============================================================================
*/

const idEventDef EV_DropToFloor( "<dropToFloor>" );
const idEventDef EV_RespawnItem( "respawn" );
const idEventDef EV_RespawnFx( "<respawnFx>" );
const idEventDef EV_GetPlayerPos( "<getplayerpos>" );
const idEventDef EV_HideObjective( "<hideobjective>", "e" );
const idEventDef EV_CamShot( "<camshot>" );

CLASS_DECLARATION( idEntity, idItem )
	EVENT( EV_DropToFloor,		idItem::Event_DropToFloor )
	EVENT( EV_Touch,			idItem::Event_Touch )
	EVENT( EV_Activate,			idItem::Event_Trigger )
	EVENT( EV_RespawnItem,		idItem::Event_Respawn )
	EVENT( EV_RespawnFx,		idItem::Event_RespawnFx )
	// SM
	EVENT( EV_PostPhysicsRest,	idItem::Event_PostPhysicsRest )
END_CLASS


/*
================
idItem::idItem
================
*/
idItem::idItem() {
	spin = false;
	inView = false;
	inViewTime = 0;
	lastCycle = 0;
	lastRenderViewTime = -1;
	itemShellHandle = -1;
	shellMaterial = NULL;
	orgOrigin.Zero();
	canPickUp = true;
	fl.networkSync = true;
}

/*
================
idItem::~idItem
================
*/
idItem::~idItem() {
	
	//bc 11-11-2024 commenting this out for now as it's happening very frequently (when carrying a weapon like shotgun, rifle, etc)
	if (gameLocal.GetLocalPlayer())
	{
		//assert(!gameLocal.GetLocalPlayer()->HasEntityInCarryableInventory(this));
	}

	// remove the highlight shell
	if ( itemShellHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( itemShellHandle );
	}
	if (rimLightShellHandle != -1) {
		gameRenderWorld->FreeEntityDef(rimLightShellHandle);
	}
}

/*
================
idItem::Save
================
*/
void idItem::Save( idSaveGame *savefile ) const {
	savefile->WriteObject( lastThrower ); // idEntityPtr<idEntity> lastThrower

	savefile->WriteBool( canPickUp ); //  bool canPickUp
	savefile->WriteBool( pulse ); //  bool pulse
	savefile->WriteBool( carryable ); //  bool carryable
	savefile->WriteInt( dropTimer ); //  int dropTimer
	savefile->WriteBool( justDropped ); //  bool justDropped

	savefile->WriteBool( dropInvincibleActive ); //  bool dropInvincibleActive
	savefile->WriteInt( dropInvincibleTimer ); //  int dropInvincibleTimer

	savefile->WriteVec3( orgOrigin ); //  idVec3 orgOrigin
	savefile->WriteBool( spin ); //  bool spin
	savefile->WriteBool( showRimLight ); //  bool showRimLight
	savefile->WriteInt( -1 ); // savefile->WriteInt( rimLightShellHandle ); //  int rimLightShellHandle

	// const  idMaterial* rimLightMaterial 	// setup in SetupRimlightMaterial();
	// idImage* rimLightImage 	// setup in SetupRimlightMaterial();

	savefile->WriteInt( itemShellHandle ); //  int itemShellHandle // blendo eric: regen in code
	savefile->WriteMaterial( shellMaterial ); // const  idMaterial * shellMaterial
	savefile->WriteBool( inView ); // mutable  bool inView
	savefile->WriteInt( inViewTime ); // mutable  int inViewTime
	savefile->WriteInt( lastCycle ); // mutable  int lastCycle
	savefile->WriteInt( lastRenderViewTime ); // mutable  int lastRenderViewTime
}

/*
================
idItem::Restore
================
*/
void idItem::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( lastThrower ); // idEntityPtr<idEntity> lastThrower

	savefile->ReadBool( canPickUp ); //  bool canPickUp
	savefile->ReadBool( pulse ); //  bool pulse
	savefile->ReadBool( carryable ); //  bool carryable
	savefile->ReadInt( dropTimer ); //  int dropTimer
	savefile->ReadBool( justDropped ); //  bool justDropped

	savefile->ReadBool( dropInvincibleActive ); //  bool dropInvincibleActive
	savefile->ReadInt( dropInvincibleTimer ); //  int dropInvincibleTimer

	savefile->ReadVec3( orgOrigin ); //  idVec3 orgOrigin
	savefile->ReadBool( spin ); //  bool spin
	savefile->ReadBool( showRimLight ); //  bool showRimLight
	savefile->ReadInt( rimLightShellHandle ); //  int rimLightShellHandle

	// const  idMaterial* rimLightMaterial 	// setup in SetupRimlightMaterial();
	//  idImage* rimLightImage 	// setup in SetupRimlightMaterial();
	SetupRimlightMaterial();

	savefile->ReadInt( itemShellHandle ); //  int itemShellHandle // blendo eric: regen in code
	savefile->ReadMaterial( shellMaterial ); // const  idMaterial * shellMaterial
	savefile->ReadBool( inView ); // mutable  bool inView
	savefile->ReadInt( inViewTime ); // mutable  int inViewTime
	savefile->ReadInt( lastCycle ); // mutable  int lastCycle
	savefile->ReadInt( lastRenderViewTime ); // mutable  int lastRenderViewTime
}

/*
================
idItem::UpdateRenderEntity
================
*/
bool idItem::UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) const {

	if ( lastRenderViewTime == renderView->time ) {
		return false;
	}

	lastRenderViewTime = renderView->time;
	if (itemShellHandle != -1 && renderEntity == gameRenderWorld->GetRenderEntity(itemShellHandle) )
	{
		// check for glow highlighting if near the center of the view
		idVec3 dir = renderEntity->origin - renderView->vieworg;
		dir.Normalize();
		float d = dir * renderView->viewaxis[0];

		// two second pulse cycle
		float cycle = (renderView->time - inViewTime) / 2000.0f;

		if (d > 0.94f) {
			if (!inView) {
				inView = true;
				if (cycle > lastCycle) {
					// restart at the beginning
					inViewTime = renderView->time;
					cycle = 0.0f;
				}
			}
		}
		else {
			if (inView) {
				inView = false;
				lastCycle = ceil(cycle);
			}
		}

		// fade down after the last pulse finishes
		if (!inView && cycle > lastCycle) {
			renderEntity->shaderParms[4] = 0.0f;
		}
		else {
			// pulse up in 1/4 second
			cycle -= (int)cycle;
			if (cycle < 0.1f) {
				renderEntity->shaderParms[4] = cycle * 10.0f;
			}
			else if (cycle < 0.2f) {
				renderEntity->shaderParms[4] = 1.0f;
			}
			else if (cycle < 0.3f) {
				renderEntity->shaderParms[4] = 1.0f - (cycle - 0.2f) * 10.0f;
			}
			else {
				// stay off between pulses
				renderEntity->shaderParms[4] = 0.0f;
			}
		}
	}
	else if (rimLightShellHandle != -1 && renderEntity == gameRenderWorld->GetRenderEntity(rimLightShellHandle) )
	{
		renderEntity->fragmentMapOverride = rimLightImage;
	}
	

	// update every single time this is in view
	return true;
}

/*
================
idItem::ModelCallback
================
*/
bool idItem::ModelCallback( renderEntity_t *renderEntity, const renderView_t *renderView ) {
	const idItem *ent;

	// this may be triggered by a model trace or other non-view related source
	if ( !renderView ) {
		return false;
	}

	ent = static_cast<idItem *>(gameLocal.entities[ renderEntity->entityNum ]);
	if ( !ent ) {
		gameLocal.Error( "idItem::ModelCallback: callback with NULL game entity" );
	}

	return ent->UpdateRenderEntity( renderEntity, renderView );
}

/*
================
idItem::Think
================
*/
void idItem::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		if ( spin ) {
			idAngles	ang;
			idVec3		org;

			ang.pitch = ang.roll = 0.0f;
			ang.yaw = ( gameLocal.time & 4095 ) * 360.0f / -4096.0f;
			SetAngles( ang );

			float scale = 0.005f + entityNumber * 0.00001f;

			org = orgOrigin;
			org.z += 4.0f + cos( ( gameLocal.time + 2000 ) * scale ) * 4.0f;
			SetOrigin( org );
		}
	}

	

	Present();
}

/*
================
idItem::Present
================
*/
void idItem::Present( void ) {
	idEntity::Present();

	if ( !fl.hidden )
	{
		if (health <= 1 && maxHealth > 1) //BC when item is critical low health, make it pulsate red.
		{
			// also add a highlight shell model
			renderEntity_t	shell;

			shell = renderEntity;

			// we will mess with shader parms when the item is in view
			// to give the "item pulse" effect
			shell.callback = idItem::ModelCallback;
			shell.entityNum = entityNumber;
			shell.customShader = shellMaterial;
			if (itemShellHandle == -1)
			{
				itemShellHandle = gameRenderWorld->AddEntityDef(&shell);
			}
			else
			{
				gameRenderWorld->UpdateEntityDef(itemShellHandle, &shell);
			}
		}

		if (showRimLight)
		{
			renderEntity_t	shell;

			shell = renderEntity;

			shell.callback = idItem::ModelCallback;
			shell.entityNum = entityNumber;
			shell.customShader = rimLightMaterial;
			if (rimLightShellHandle == -1)
			{
				rimLightShellHandle = gameRenderWorld->AddEntityDef(&shell);
			}
			else
			{
				gameRenderWorld->UpdateEntityDef(rimLightShellHandle, &shell);
			}
		}
	}
}

/*
================
idItem::Spawn
================
*/
void idItem::Spawn( void ) {
	idStr		giveTo;
	idEntity *	ent;
	float		tsize;

	if ( spawnArgs.GetBool( "dropToFloor" ) ) {
		PostEventMS( &EV_DropToFloor, 0 );
	}

	if ( spawnArgs.GetFloat( "triggersize", "0", tsize ) ) {
		GetPhysics()->GetClipModel()->LoadModel( idTraceModel( idBounds( vec3_origin ).Expand( tsize ) ) );
		GetPhysics()->GetClipModel()->Link( gameLocal.clip );
	}

	if ( spawnArgs.GetBool( "start_off" ) ) {
		GetPhysics()->SetContents( 0 );
		Hide();
	} else {
		GetPhysics()->SetContents( CONTENTS_TRIGGER );
	}

	giveTo = spawnArgs.GetString( "owner" );
	if ( giveTo.Length() ) {
		ent = gameLocal.FindEntity( giveTo );
		if ( !ent ) {
			gameLocal.Warning( "Item '%s' couldn't find owner '%s'", GetName(), giveTo.c_str() );
		} else {
			PostEventMS( &EV_Touch, 0, ent, 0 );
		}
	}

#ifdef CTF
	// idItemTeam does not rotate and bob
	if ( spawnArgs.GetBool( "spin" ) || (gameLocal.isMultiplayer && !this->IsType( idItemTeam::Type ) ) ) {
		spin = true;
		BecomeActive( TH_THINK );
	}
#else
	if ( spawnArgs.GetBool( "spin" ) || gameLocal.isMultiplayer ) {
		spin = true;
		BecomeActive( TH_THINK );
	}
#endif
	orgOrigin = GetPhysics()->GetOrigin();

	//canPickUp = !( spawnArgs.GetBool( "triggerFirst" ) || spawnArgs.GetBool( "no_touch" ) );
	canPickUp = spawnArgs.GetBool("canpickup", "0"); //BC can player touch it to pick it up.


	inViewTime = -1000;
	lastCycle = -1;
	itemShellHandle = -1;
	shellMaterial = declManager->FindMaterial( "itemHighlightShell" );
	
	showRimLight = spawnArgs.GetBool("rimLight", "0");
	rimLightShellHandle = -1;

	SetupRimlightMaterial();
	

	//BC
	isFrobbable = spawnArgs.GetBool("frobbable", "0");
	pulse = (isFrobbable && spawnArgs.GetBool("frobhighlight", "0"));
	lastThrower = NULL;
	dropTimer = gameLocal.time;
	justDropped = false;
	dropInvincibleActive = false;
	dropInvincibleTimer = 0;
}

void idItem::SetupRimlightMaterial() {
	rimLightMaterial = nullptr;
	rimLightImage = nullptr;

	if (showRimLight)
	{
		rimLightMaterial = declManager->FindMaterial("rimLightShader");

#if 0 // use skin image first
		idImage* rimLightImageFallback = NULL;

		// Try to find a diffuse map to use for our rim light shader
		idRenderModel* renderModel = this->renderEntity.hModel;

		for (int i = 0; i < renderModel->NumSurfaces(); i++)
		{
			const idMaterial* shader = renderModel->Surface(i)->shader;
			const idMaterial* remappedShader = gameRenderWorld->RemapShaderBySkin(shader, this->renderEntity.customSkin);
			for (int j = 0; j < remappedShader->GetNumStages(); j++)
			{
				if (remappedShader->GetStage(j)->lighting == SL_DIFFUSE)
				{
					idImage* curImage = remappedShader->GetStage(j)->texture.image;
					if (!rimLightImageFallback)
					{
						rimLightImageFallback = curImage;
					}
					if (curImage && curImage->imgName.Find("skin", false) >= 0)
					{
						rimLightImage = curImage;
						break;
					}
				}
			}
		}

		if (!rimLightImage)
		{
			rimLightImage = rimLightImageFallback;
		}

#else // default standard white or other texture

		// rimLightImage is redundant here, but keeping it just in case we want to go back to customSkin image version
		for (int j = 0; j < rimLightMaterial->GetNumStages(); j++)
		{
			rimLightImage = rimLightMaterial->GetStage(j)->texture.image;
			if (rimLightImage) {
				break;
			}
		}
#endif

		if (!rimLightImage)
		{

			gameLocal.Warning("idItem '%s' could not find a suitable diffuse map for use with rim light shell.", GetName());
			rimLightImage = NULL;
		}
	}
}

/*
================
idItem::GetAttributes
================
*/
void idItem::GetAttributes( idDict &attributes ) {
	int					i;
	const idKeyValue	*arg;

	for( i = 0; i < spawnArgs.GetNumKeyVals(); i++ ) {
		arg = spawnArgs.GetKeyVal( i );
		if ( arg->GetKey().Left( 4 ) == "inv_" ) {
			attributes.Set( arg->GetKey().Right( arg->GetKey().Length() - 4 ), arg->GetValue() );
		}
	}
}

/*
================
idItem::GiveToPlayer
================
*/
bool idItem::GiveToPlayer( idPlayer *player ) {
	if ( player == NULL ) {
		return false;
	}

	

	if (spawnArgs.GetInt("inv_carry") == 1)
	{
		return player->GiveInventoryItem(&spawnArgs);
	}	

	
	bool successfulGive = player->GiveItem( this );

	if (spawnArgs.GetInt("inv_carry") > 0 && successfulGive)
	{
		//BC
		gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_pickedup"), common->GetLanguageDict()->GetString(this->displayName.c_str())), this->GetPhysics()->GetOrigin(), false);

		gameLocal.GetLocalPlayer()->JustPickedupItem(this);
	}
	else if (spawnArgs.GetInt("inv_carry") > 0 && !successfulGive)
	{
		gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_carrynomore");
	}

	return successfulGive;
}

bool idItem::DoFrob(int index, idEntity * frobber)
{
	return Pickup(gameLocal.GetLocalPlayer());
}

/*
================
idItem::Pickup
================
*/
bool idItem::Pickup( idPlayer *player )
{
	idVec3 fxPos;
	idMat3 fxMat;

	// SW 27th March 2025:
	// This code was previously used for stuffing a round into the chamber. It didn't work because the weapon didn't have a hotbar slot at this point,
	// and in any case, we don't want to just infinitely chamber a round every time the player picks up the weapon.
	// I'm repurposing it here for telling the inventory system whether it needs to load the weapon we picked up
	if (spawnArgs.GetBool("isweapon"))
	{
		const idKeyValue * keyval = this->spawnArgs.MatchPrefix("inv_weapon");
		if (keyval)
		{
			idStr value = keyval->GetValue();			
			int weaponIndex = gameLocal.GetLocalPlayer()->inventory.GetWeaponIndex(gameLocal.GetLocalPlayer(), value);

			if (weaponIndex >= 0)
			{
				if (gameLocal.GetLocalPlayer()->inventory.GetHotbarslotViaWeaponIndex(weaponIndex) < 0)
				{
					// SW 27th March 2025:
					// Player doesn't already have this weapon. So when we supply the new weapon to the inventory, we need the inventory to load it instead of just hoovering up the ammo
					// We pass on this instruction to the inventory via the keyvalue below
					this->spawnArgs.SetBool("loadnewweapon", true);
				}
			}
		}
	}

	if ( !GiveToPlayer( player ) ) {

		if (spawnArgs.GetBool("alwaysPickup", "0"))
		{
			//even if it can't give to player, make the thing go away. This is for the air bubble.
			GetPhysics()->SetContents(0);
			Hide();
			BecomeInactive(TH_THINK);
			PostEventMS(&EV_Remove, 0);
			StartSound("snd_alwayspickup", SND_CHANNEL_ITEM, 0, false, NULL);
		}

		return false;
	}

	this->fl.takedamage = false;

	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_PICKUP, NULL, false, -1 );
	}

	// play pickup sound
	StartSound( "snd_acquire", SND_CHANNEL_ITEM, 0, false, NULL );

	// trigger our targets
	ActivateTargets( player );

	// clear our contents so the object isn't picked up twice
	GetPhysics()->SetContents( 0 );

	// hide the model
	Hide();

	// remove the highlight shell
	if ( itemShellHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( itemShellHandle );
		itemShellHandle = -1;
	}

	float respawn = spawnArgs.GetFloat( "respawn" );
	bool dropped = spawnArgs.GetBool( "dropped" );
	bool no_respawn = spawnArgs.GetBool( "no_respawn" );

	if ( gameLocal.isMultiplayer && respawn == 0.0f ) {
		respawn = 20.0f;
	}

	if ( respawn && !dropped && !no_respawn )
	{
		const char *sfx = spawnArgs.GetString( "fxRespawn" );
		if ( sfx && *sfx ) {
			PostEventSec( &EV_RespawnFx, respawn - 0.5f );
		}
		PostEventSec( &EV_RespawnItem, respawn );
	}
	else if ( !spawnArgs.GetBool( "inv_objective" ) && !no_respawn )
	{
		// give some time for the pickup sound to play
		// FIXME: Play on the owner
		if ( !spawnArgs.GetBool( "inv_carry" ) )
		{
			PostEventMS( &EV_Remove, 5000 );
		}
	}

	fxPos = GetPhysics()->GetOrigin();
	fxMat = mat3_identity;

	idEntityFx::StartFx("fx/pickupitem", &fxPos, &fxMat, NULL, false); //BC

	BecomeInactive( TH_THINK );
	return true;
}

/*
================
idItem::ClientPredictionThink
================
*/
void idItem::ClientPredictionThink( void ) {
	// only think forward because the state is not synced through snapshots
	if ( !gameLocal.isNewFrame ) {
		return;
	}
	Think();
}

/*
================
idItem::WriteFromSnapshot
================
*/
void idItem::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( IsHidden(), 1 );
}

/*
================
idItem::ReadFromSnapshot
================
*/
void idItem::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	if ( msg.ReadBits( 1 ) ) {
		Hide();
	} else {
		Show();
	}
}

/*
================
idItem::ClientReceiveEvent
================
*/
bool idItem::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {

	switch( event ) {
		case EVENT_PICKUP: {

			// play pickup sound
			StartSound( "snd_acquire", SND_CHANNEL_ITEM, 0, false, NULL );

			// hide the model
			Hide();

			// remove the highlight shell
			if ( itemShellHandle != -1 ) {
				gameRenderWorld->FreeEntityDef( itemShellHandle );
				itemShellHandle = -1;
			}
			return true;
		}
		case EVENT_RESPAWN: {
			Event_Respawn();
			return true;
		}
		case EVENT_RESPAWNFX: {
			Event_RespawnFx();
			return true;
		}
		default:
			break;
	}

	return idEntity::ClientReceiveEvent( event, time, msg );
}

/*
================
idItem::Event_DropToFloor
================
*/
void idItem::Event_DropToFloor( void ) {
	trace_t trace;

	// don't drop the floor if bound to another entity
	if ( GetBindMaster() != NULL && GetBindMaster() != this ) {
		return;
	}

	gameLocal.clip.TraceBounds( trace, renderEntity.origin, renderEntity.origin - idVec3( 0, 0, 64 ), renderEntity.bounds, MASK_SOLID | CONTENTS_CORPSE, this );
	SetOrigin( trace.endpos );
}

/*
================
idItem::Event_Touch
================
*/
void idItem::Event_Touch( idEntity *other, trace_t *trace )
{
	if ( !other->IsType( idPlayer::Type ) ) {
		return;
	}

	if ( !canPickUp ) {
		return;
	}

	Pickup( static_cast<idPlayer *>(other) );
}

/*
================
idItem::Event_Trigger
================
*/
void idItem::Event_Trigger( idEntity *activator ) {

	if ( !canPickUp && spawnArgs.GetBool( "triggerFirst" ) ) {
		canPickUp = true;
		return;
	}

	if ( activator && activator->IsType( idPlayer::Type ) ) {
		Pickup( static_cast<idPlayer *>( activator ) );
	}
}

/*
================
idItem::Event_Respawn
================
*/
void idItem::Event_Respawn( void ) {
	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_RESPAWN, NULL, false, -1 );
	}
	BecomeActive( TH_THINK );
	Show();
	inViewTime = -1000;
	lastCycle = -1;
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	SetOrigin( orgOrigin );
	StartSound( "snd_respawn", SND_CHANNEL_ITEM, 0, false, NULL );
	CancelEvents( &EV_RespawnItem ); // don't double respawn
}

/*
================
idItem::Event_RespawnFx
================
*/
void idItem::Event_RespawnFx( void ) {
	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_RESPAWNFX, NULL, false, -1 );
	}
	const char *sfx = spawnArgs.GetString( "fxRespawn" );
	if ( sfx && *sfx ) {
		idEntityFx::StartFx( sfx, NULL, NULL, this, true );
	}
}

void idItem::Event_PostPhysicsRest() {
	// SW 6th May 2025: Don't clear our owner if we're currently being held. This covers the scenario where the player drops an item and quickly picks it back up again.
	if ( justDropped && (gameLocal.GetLocalPlayer()->GetCarryable() == NULL || gameLocal.GetLocalPlayer()->GetCarryable() != this)  )
	{
		GetPhysics()->GetClipModel()->SetOwner( nullptr );
	}

	justDropped = false;
}

void idItem::SetJustDropped(bool gentleDrop)
{
	//so that items don't take damage immediately after being dropped.
	

	// If we're airless just immediately clear the owner (SM: and if it's not the player)
	if ( gameLocal.GetAirlessAtPoint( GetPhysics()->GetOrigin() ) &&
		GetPhysics()->GetClipModel()->GetOwner() != gameLocal.GetLocalPlayer() )
	{
		GetPhysics()->GetClipModel()->SetOwner( nullptr );
	}
	else
	{
		GetPhysics()->GetClipModel()->SetOwner( gameLocal.GetLocalPlayer() ); //Disable collision with player.
		justDropped = true;
	}

	if (gentleDrop)
	{
		dropTimer = gameLocal.time; //This is to prevent dropped items from instantly creating interestpoints when they hit the ground.
	}
}

//When an item is dropped by an enemy, make it invincible for a short time.
//This because it feels weird and bad when explosions or chaos blow up the
//enemy and all their items in the same frame.
void idItem::SetDropInvincible()
{
	dropInvincibleActive = true;
	dropInvincibleTimer = gameLocal.time;
}




void idItem::Hide(void)
{
	idEntity::Hide();
	
	//The normal hide() command lets the rimlight stay. We manually delete the rimlight here.
	if (rimLightShellHandle != -1)
	{
		gameRenderWorld->FreeEntityDef(rimLightShellHandle);
		rimLightShellHandle = -1;
	}

	if (itemShellHandle != -1)
	{
		gameRenderWorld->FreeEntityDef(itemShellHandle);
		itemShellHandle = -1;
	}
}



/*
===============================================================================

  idItemPowerup

===============================================================================
*/

/*
===============
idItemPowerup
===============
*/

CLASS_DECLARATION( idItem, idItemPowerup )
END_CLASS

/*
================
idItemPowerup::idItemPowerup
================
*/
idItemPowerup::idItemPowerup() {
	time = 0;
	type = 0;
}

/*
================
idItemPowerup::Save
================
*/
void idItemPowerup::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( time ); // int time
	savefile->WriteInt( type ); // int type
}

/*
================
idItemPowerup::Restore
================
*/
void idItemPowerup::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( time ); // int time
	savefile->ReadInt( type ); // int type
}

/*
================
idItemPowerup::Spawn
================
*/
void idItemPowerup::Spawn( void ) {
	time = spawnArgs.GetInt( "time", "30" );
	type = spawnArgs.GetInt( "type", "0" );
}

/*
================
idItemPowerup::GiveToPlayer
================
*/
bool idItemPowerup::GiveToPlayer( idPlayer *player ) {
	if ( player->spectating ) {
		return false;
	}
	player->GivePowerUp( type, time * 1000 );
	return true;
}

#ifdef CTF


/*
===============================================================================

  idItemTeam

  Used for flags in Capture the Flag

===============================================================================
*/

// temporarely removed these events

const idEventDef EV_FlagReturn( "flagreturn", "e" );
const idEventDef EV_TakeFlag( "takeflag", "e" );
const idEventDef EV_DropFlag( "dropflag", "d" );
const idEventDef EV_FlagCapture( "flagcapture" );

CLASS_DECLARATION( idItem, idItemTeam )
	EVENT( EV_FlagReturn,  idItemTeam::Event_FlagReturn )
	EVENT( EV_TakeFlag,    idItemTeam::Event_TakeFlag )
	EVENT( EV_DropFlag,    idItemTeam::Event_DropFlag )
	EVENT( EV_FlagCapture, idItemTeam::Event_FlagCapture )
END_CLASS

/*
===============
idItemTeam::idItemTeam
===============
*/
idItemTeam::idItemTeam() {
	team		   = -1;
	carried		   = false;
	dropped		   = false;
	lastDrop	   = 0;

	itemGlowHandle = -1;

	skinDefault	= NULL;
	skinCarried	= NULL;

	scriptTaken		= NULL;
	scriptDropped	= NULL;
	scriptReturned	= NULL;
	scriptCaptured	= NULL;

	lastNuggetDrop	= 0;
	nuggetName		= 0;
}

/*
===============
idItemTeam::~idItemTeam
===============
*/
idItemTeam::~idItemTeam() {
	FreeLightDef();
}
/*
===============
idItemTeam::Spawn
===============
*/
void idItemTeam::Spawn( void ) {
	team					= spawnArgs.GetInt( "team" );
	returnOrigin			= GetPhysics()->GetOrigin() + idVec3( 0, 0, 20 );
	returnAxis				= GetPhysics()->GetAxis();

	BecomeActive( TH_THINK );

	const char * skinName;
	skinName = spawnArgs.GetString( "skin", ""  );
	if ( skinName[0] )
		skinDefault = declManager->FindSkin( skinName );

	skinName = spawnArgs.GetString( "skin_carried", ""  );
	if ( skinName[0] )
		skinCarried = declManager->FindSkin( skinName );

	nuggetName = spawnArgs.GetString( "nugget_name", "" );
	if ( !nuggetName[0] ) {
		nuggetName = NULL;
	}

	scriptTaken		= LoadScript( "script_taken" );
	scriptDropped	= LoadScript( "script_dropped"  );
	scriptReturned	= LoadScript( "script_returned" );
	scriptCaptured	= LoadScript( "script_captured" );

	/* Spawn attached dlight */
	/*
	idDict args;
	idVec3 lightOffset( 0.0f, 20.0f, 0.0f );

	// Set up the flag's dynamic light
	memset( &itemGlow, 0, sizeof( itemGlow ) );
	itemGlow.axis = mat3_identity;
	itemGlow.lightRadius.x = 128.0f;
	itemGlow.lightRadius.y = itemGlow.lightRadius.z = itemGlow.lightRadius.x;
	itemGlow.noShadows  = true;
	itemGlow.pointLight = true;
	itemGlow.shaderParms[ SHADERPARM_RED ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_GREEN ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_BLUE ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_ALPHA ] = 0.0f;

	// Select a shader based on the team
	if ( team == 0 )
		itemGlow.shader = declManager->FindMaterial( "lights/redflag" );
	else
		itemGlow.shader = declManager->FindMaterial( "lights/blueflag" );
	*/

	idMoveableItem::Spawn();

	physicsObj.SetContents( 0 );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );
	physicsObj.SetGravity( idVec3( 0, 0, spawnArgs.GetInt("gravity", "-30" ) ) );
}


/*
===============
idItemTeam::LoadScript
===============
*/
function_t * idItemTeam::LoadScript( const char * script ) {
	function_t * function = NULL;
	idStr funcname = spawnArgs.GetString( script, "" );
	if ( funcname.Length() ) {
		 function = gameLocal.program.FindFunction( funcname );
		 if ( function == NULL ) {
#ifdef _DEBUG
			gameLocal.Warning( "idItemTeam '%s' at (%s) calls unknown function '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), funcname.c_str() );
#endif
		 }
	}
	return function;
}


/*
===============
idItemTeam::Think
===============
*/
void idItemTeam::Think( void ) {
	idMoveableItem::Think();

	TouchTriggers();


	/*idVec3 offset( 0.0f, 0.0f, 20.0f );
	itemGlow.origin = GetPhysics()->GetOrigin() + offset;
	if ( itemGlowHandle == -1 ) {
		itemGlowHandle = gameRenderWorld->AddLightDef( &itemGlow );
	} else {
		gameRenderWorld->UpdateLightDef( itemGlowHandle, &itemGlow );
	}*/

#if 1
	// should only the server do this?
	if ( gameLocal.isServer && nuggetName && carried && ( !lastNuggetDrop || (gameLocal.time - lastNuggetDrop) >  spawnArgs.GetInt("nugget_frequency") ) ) {

		SpawnNugget( GetPhysics()->GetOrigin() );
		lastNuggetDrop = gameLocal.time;
	}
#endif

	// return dropped flag after si_flagDropTimeLimit seconds
	if ( dropped && !carried && lastDrop != 0 && (gameLocal.time - lastDrop) > ( si_flagDropTimeLimit.GetInteger()*1000 )  ) {

		Return();	// return flag after 30 seconds on ground
		return;
	}
}

/*
===============
idItemTeam::Pickup
===============
*/
bool idItemTeam::Pickup( idPlayer *player ) {
	if ( !gameLocal.mpGame.IsGametypeFlagBased() ) /* CTF */
		return false;

	if ( gameLocal.mpGame.GetGameState() == idMultiplayerGame::WARMUP ||
		 gameLocal.mpGame.GetGameState() == idMultiplayerGame::COUNTDOWN )
		return false;

	// wait 2 seconds after drop before beeing picked up again
	if ( lastDrop != 0 && (gameLocal.time - lastDrop) < spawnArgs.GetInt("pickupDelay", "500") )
		return false;

	if ( carried == false && player->team != this->team ) {

		PostEventMS( &EV_TakeFlag, 0, player );

		return true;
	} else if ( carried == false && dropped == true && player->team == this->team ) {

		gameLocal.mpGame.PlayerScoreCTF( player->entityNumber, 5 );

		// return flag
		PostEventMS( &EV_FlagReturn, 0, player );

		return false;
	}

	return false;
}

/*
===============
idItemTeam::ClientReceiveEvent
===============
*/
bool idItemTeam::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	gameLocal.DPrintf("ClientRecieveEvent: %i\n", event );

	switch ( event ) {
		case EVENT_TAKEFLAG: {
			idPlayer * player = static_cast<idPlayer *>(gameLocal.entities[ msg.ReadBits( GENTITYNUM_BITS ) ]);
			if ( player == NULL ) {
				gameLocal.Warning( "NULL player takes flag?\n" );
				return false;
			}

			Event_TakeFlag( player );
		}
		return true;

		case EVENT_DROPFLAG : {
			bool death = bool( msg.ReadBits( 1 ) == 1 );
			Event_DropFlag( death );
		}
		return true;

		case EVENT_FLAGRETURN : {
			Hide();

			FreeModelDef();
			FreeLightDef();

			Event_FlagReturn();
		}
		return true;

		case EVENT_FLAGCAPTURE : {
			Hide();

			FreeModelDef();
			FreeLightDef();

			Event_FlagCapture();
		}
		return true;
	};

	return false;
}

/*
================
idItemTeam::Drop
================
*/
void idItemTeam::Drop( bool death )
{
//	PostEventMS( &EV_DropFlag, 0, int(death == true) );
// had to remove the delayed drop because of drop flag on disconnect
	Event_DropFlag( death );
}

/*
================
idItemTeam::Return
================
*/
void idItemTeam::Return( idPlayer * player )
{
	if ( team != 0 && team != 1 )
		return;

//	PostEventMS( &EV_FlagReturn, 0 );
	Event_FlagReturn();
}

/*
================
idItemTeam::Capture
================
*/
void idItemTeam::Capture( void )
{
	if ( team != 0 && team != 1 )
		return;

	PostEventMS( &EV_FlagCapture, 0 );
}

/*
================
idItemTeam::PrivateReturn
================
*/
void idItemTeam::PrivateReturn( void )
{
	Unbind();

	if ( gameLocal.isServer && carried && !dropped ) {
		int playerIdx = gameLocal.mpGame.GetFlagCarrier( 1-team );
		if ( playerIdx != -1 ) {
			idPlayer * player = static_cast<idPlayer*>( gameLocal.entities[ playerIdx ] );
			player->carryingFlag = false;
		} else {
			gameLocal.Warning( "BUG: carried flag has no carrier before return" );
		}
	}

	dropped = false;
	carried = false;

	SetOrigin( returnOrigin );
	SetAxis( returnAxis );

	trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), mat3_identity );

	SetSkin( skinDefault );

	// Turn off the light
	/*itemGlow.shaderParms[ SHADERPARM_RED ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_GREEN ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_BLUE ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_ALPHA ] = 0.0f;

	if ( itemGlowHandle != -1 )
		gameRenderWorld->UpdateLightDef( itemGlowHandle, &itemGlow );*/

	GetPhysics()->SetLinearVelocity( idVec3(0, 0, 0) );
	GetPhysics()->SetAngularVelocity( idVec3(0, 0, 0) );
}

/*
================
idItemTeam::Event_TakeFlag
================
*/
void idItemTeam::Event_TakeFlag( idPlayer * player ) {
	gameLocal.DPrintf("Event_TakeFlag()!\n");

	if ( gameLocal.isServer ) {
		idBitMsg msg;
		byte msgBuf[MAX_EVENT_PARAM_SIZE];
		// Send the event
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteBits( player->entityNumber, GENTITYNUM_BITS );
		ServerSendEvent( EVENT_TAKEFLAG, &msg, false, -1 );

		gameLocal.mpGame.PlayTeamSound( player->team, SND_FLAG_TAKEN_THEIRS );
		gameLocal.mpGame.PlayTeamSound( team, SND_FLAG_TAKEN_YOURS );

		gameLocal.mpGame.PrintMessageEvent( -1, idMultiplayerGame::MSG_FLAGTAKEN, team, player->entityNumber );

		// dont drop a nugget RIGHT away
		lastNuggetDrop = gameLocal.time - gameLocal.random.RandomInt( 1000 );

	}

	BindToJoint( player, g_flagAttachJoint.GetString(), true );
	idVec3 origin( g_flagAttachOffsetX.GetFloat(), g_flagAttachOffsetY.GetFloat(), g_flagAttachOffsetZ.GetFloat() );
	idAngles angle( g_flagAttachAngleX.GetFloat(), g_flagAttachAngleY.GetFloat(), g_flagAttachAngleZ.GetFloat() );
	SetAngles( angle );
	SetOrigin( origin );

	// Turn the light on
	/*itemGlow.shaderParms[ SHADERPARM_RED ] = 1.0f;
	itemGlow.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
	itemGlow.shaderParms[ SHADERPARM_BLUE ] = 1.0f;
	itemGlow.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;

	if ( itemGlowHandle != -1 )
		gameRenderWorld->UpdateLightDef( itemGlowHandle, &itemGlow );*/

	if ( scriptTaken ) {
		idThread *thread = new idThread();
		thread->CallFunction( scriptTaken, false );
		thread->DelayedStart( 0 );
	}

	dropped = false;
	carried = true;
	player->carryingFlag = true;

	SetSkin( skinCarried );

	UpdateVisuals();
	UpdateGuis();

	if ( gameLocal.isServer ) {
		if ( team == 0 )
			gameLocal.mpGame.player_red_flag = player->entityNumber;
		else
			gameLocal.mpGame.player_blue_flag = player->entityNumber;
	}
}

/*
================
idItemTeam::Event_DropFlag
================
*/
void idItemTeam::Event_DropFlag( bool death ) {
	gameLocal.DPrintf("Event_DropFlag()!\n");

	if ( gameLocal.isServer ) {
		idBitMsg msg;
		byte msgBuf[MAX_EVENT_PARAM_SIZE];
		// Send the event
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteBits( death, 1 );
		ServerSendEvent( EVENT_DROPFLAG, &msg, false, -1 );

		if ( gameLocal.mpGame.IsFlagMsgOn() ) {
			gameLocal.mpGame.PlayTeamSound( 1-team,	SND_FLAG_DROPPED_THEIRS );
			gameLocal.mpGame.PlayTeamSound( team,	SND_FLAG_DROPPED_YOURS );

			gameLocal.mpGame.PrintMessageEvent( -1, idMultiplayerGame::MSG_FLAGDROP, team );
		}
	}

	lastDrop = gameLocal.time;

	BecomeActive( TH_THINK );
	Show();

	if ( death )
		GetPhysics()->SetLinearVelocity( idVec3(0, 0, 0) );
	else
		GetPhysics()->SetLinearVelocity( idVec3(0, 0, 20) );

	GetPhysics()->SetAngularVelocity( idVec3(0, 0, 0) );

//	GetPhysics()->SetLinearVelocity( ( GetPhysics()->GetLinearVelocity() * GetBindMaster()->GetPhysics()->GetAxis() ) + GetBindMaster()->GetPhysics()->GetLinearVelocity() );

	if ( GetBindMaster() ) {
		const idBounds bounds = GetPhysics()->GetBounds();
		idVec3 origin = GetBindMaster()->GetPhysics()->GetOrigin() + idVec3(0, 0, ( bounds[1].z-bounds[0].z )*0.6f );

		Unbind();

		SetOrigin( origin );
	}

	idAngles angle = GetPhysics()->GetAxis().ToAngles();
	angle.roll	= 0;
	angle.pitch = 0;
	SetAxis( angle.ToMat3() );

	dropped = true;
	carried = false;

	if ( scriptDropped ) {
		idThread *thread = new idThread();
		thread->CallFunction( scriptDropped, false );
		thread->DelayedStart( 0 );
	}

	SetSkin( skinDefault );
	UpdateVisuals();
	UpdateGuis();


	if ( gameLocal.isServer ) {
		if ( team == 0 )
			gameLocal.mpGame.player_red_flag = -1;
		else
			gameLocal.mpGame.player_blue_flag = -1;

	}
}

/*
================
idItemTeam::Event_FlagReturn
================
*/
void idItemTeam::Event_FlagReturn( idPlayer * player ) {
	gameLocal.DPrintf("Event_FlagReturn()!\n");

	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_FLAGRETURN, NULL, false, -1 );

		if ( gameLocal.mpGame.IsFlagMsgOn() ) {
			gameLocal.mpGame.PlayTeamSound( 1-team,	SND_FLAG_RETURN );
			gameLocal.mpGame.PlayTeamSound( team,	SND_FLAG_RETURN );

			int entitynum = 255;
			if ( player ) {
				entitynum = player->entityNumber;
			}

			gameLocal.mpGame.PrintMessageEvent( -1, idMultiplayerGame::MSG_FLAGRETURN, team, entitynum );
		}
	}

	BecomeActive( TH_THINK );
	Show();

	PrivateReturn();

	if ( scriptReturned ) {
		idThread *thread = new idThread();
		thread->CallFunction( scriptReturned, false );
		thread->DelayedStart( 0 );
	}

	UpdateVisuals();
	UpdateGuis();
//	Present();

	if ( gameLocal.isServer ) {
		if ( team == 0 )
			gameLocal.mpGame.player_red_flag = -1;
		else
			gameLocal.mpGame.player_blue_flag = -1;
	}
}

/*
================
idItemTeam::Event_FlagCapture
================
*/
void idItemTeam::Event_FlagCapture( void ) {
	gameLocal.DPrintf("Event_FlagCapture()!\n");

	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_FLAGCAPTURE, NULL, false, -1 );

		gameLocal.mpGame.PlayTeamSound( 1-team,	SND_FLAG_CAPTURED_THEIRS );
		gameLocal.mpGame.PlayTeamSound( team,	SND_FLAG_CAPTURED_YOURS );

		gameLocal.mpGame.TeamScoreCTF( 1-team, 1 );

		int playerIdx = gameLocal.mpGame.GetFlagCarrier( 1-team );
		if ( playerIdx != -1 ) {
			gameLocal.mpGame.PlayerScoreCTF( playerIdx, 10 );
		} else {
			playerIdx = 255;
		}

		gameLocal.mpGame.PrintMessageEvent( -1, idMultiplayerGame::MSG_FLAGCAPTURE, team, playerIdx );
	}

	BecomeActive( TH_THINK );
	Show();

	PrivateReturn();

	if ( scriptCaptured ) {
		idThread *thread = new idThread();
		thread->CallFunction( scriptCaptured, false );
		thread->DelayedStart( 0 );
	}

	UpdateVisuals();
	UpdateGuis();


	if ( gameLocal.isServer ) {
		if ( team == 0 )
			gameLocal.mpGame.player_red_flag = -1;
		else
			gameLocal.mpGame.player_blue_flag = -1;
	}

}

/*
================
idItemTeam::FreeLightDef
================
*/
void idItemTeam::FreeLightDef( void ) {
	if ( itemGlowHandle != -1 ) {
		gameRenderWorld->FreeLightDef( itemGlowHandle );
		itemGlowHandle = -1;
	}
}

/*
================
idItemTeam::SpawnNugget
================
*/
void idItemTeam::SpawnNugget( idVec3 pos ) {

	idAngles angle( gameLocal.random.RandomInt(spawnArgs.GetInt("nugget_pitch", "30")),	gameLocal.random.RandomInt(spawnArgs.GetInt("nugget_yaw", "360" )),	0 );
	float velocity = float(gameLocal.random.RandomInt( 40 )+15);

	velocity *= spawnArgs.GetFloat("nugget_velocity", "1" );

	idEntity * ent = idMoveableItem::DropItem( nuggetName, pos, GetPhysics()->GetAxis(), angle.ToMat3()*idVec3(velocity, velocity, velocity), 0, spawnArgs.GetInt("nugget_removedelay"), NULL, false );
	idPhysics_RigidBody * physics = static_cast<idPhysics_RigidBody *>( ent->GetPhysics() );

	if ( physics && physics->IsType( idPhysics_RigidBody::Type ) ) {
		physics->DisableImpact();
	}
}



/*
================
idItemTeam::Event_FlagCapture
================
*/
void idItemTeam::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( carried, 1 );
	msg.WriteBits( dropped, 1 );

	WriteBindToSnapshot( msg );

	idMoveableItem::WriteToSnapshot( msg );
}


/*
================
idItemTeam::ReadFromSnapshot
================
*/
void idItemTeam::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	carried = msg.ReadBits( 1 ) == 1;
	dropped = msg.ReadBits( 1 ) == 1;

	ReadBindFromSnapshot( msg );

	if ( msg.HasChanged() )
	{
		UpdateGuis();

		if ( carried == true )
			SetSkin( skinCarried );
		else
			SetSkin( skinDefault );
	}

	idMoveableItem::ReadFromSnapshot( msg );
}

/*
================
idItemTeam::UpdateGuis

Update all client's huds wrt the flag status.
================
*/
void idItemTeam::UpdateGuis( void ) {
	idPlayer *player;

	for ( int i = 0; i < gameLocal.numClients; i++ ) {
		player = static_cast<idPlayer *>( gameLocal.entities[ i ] );

		if ( player == NULL || player->hud == NULL )
			continue;

		player->hud->SetStateInt( "red_flagstatus", gameLocal.mpGame.GetFlagStatus( 0 ) );
		player->hud->SetStateInt( "blue_flagstatus", gameLocal.mpGame.GetFlagStatus( 1 ) );

		player->hud->SetStateInt( "red_team_score",  gameLocal.mpGame.GetFlagPoints( 0 ) );
		player->hud->SetStateInt( "blue_team_score", gameLocal.mpGame.GetFlagPoints( 1 ) );

	}

}

/*
================
idItemTeam::Present
================
*/
void idItemTeam::Present( void ) {
	// hide the flag for localplayer if in first person
	if ( carried && GetBindMaster() ) {
		idPlayer * player = static_cast<idPlayer *>( GetBindMaster() );
		if ( player == gameLocal.GetLocalPlayer() && !pm_thirdPerson.GetBool() ) {
			FreeModelDef();
			BecomeActive( TH_UPDATEVISUALS );
			return;
		}
	}

	idEntity::Present();
}

#endif

/*
===============================================================================

  idObjective

===============================================================================
*/

CLASS_DECLARATION( idItem, idObjective )
	EVENT( EV_Activate,			idObjective::Event_Trigger )
	EVENT( EV_HideObjective,	idObjective::Event_HideObjective )
	EVENT( EV_GetPlayerPos,		idObjective::Event_GetPlayerPos )
	EVENT( EV_CamShot,			idObjective::Event_CamShot )
END_CLASS

/*
================
idObjective::idObjective
================
*/
idObjective::idObjective() {
	playerPos.Zero();
}

/*
================
idObjective::Save
================
*/
void idObjective::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( playerPos ); // idVec3 playerPos
}

/*
================
idObjective::Restore
================
*/
void idObjective::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( playerPos ); // idVec3 playerPos
}

/*
================
idObjective::Spawn
================
*/
void idObjective::Spawn( void ) {
	Hide();
	if ( cvarSystem->GetCVarBool( "com_makingBuild") ) {
		PostEventMS( &EV_CamShot, 250 );
	}
}

/*
================
idObjective::Event_Screenshot
================
*/
void idObjective::Event_CamShot( ) {
	const char *camName;
	idStr shotName = gameLocal.GetMapName();
	shotName.StripFileExtension();
	shotName += "/";
	shotName += spawnArgs.GetString( "screenshot" );
	shotName.SetFileExtension( ".tga" );
	if ( spawnArgs.GetString( "camShot", "", &camName ) ) {
		idEntity *ent = gameLocal.FindEntity( camName );
		if ( ent && ent->cameraTarget ) {
			const renderView_t *view = ent->cameraTarget->GetRenderView();
			renderView_t fullView = *view;
			fullView.width = SCREEN_WIDTH;
			fullView.height = SCREEN_HEIGHT;

#ifdef _D3XP
			// HACK : always draw sky-portal view if there is one in the map, this isn't real-time
			if ( gameLocal.portalSkyEnt.GetEntity() && g_enablePortalSky.GetBool() ) {
				renderView_t	portalView = fullView;
				portalView.vieworg = gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetOrigin();

				// setup global fixup projection vars
				if ( 1 ) {
					int vidWidth, vidHeight;
					idVec2 shiftScale;

					renderSystem->GetGLSettings( vidWidth, vidHeight );

					float pot;
					int temp;

					int	 w = vidWidth;
					for (temp = 1 ; temp < w ; temp<<=1) {
					}
					pot = (float)temp;
					shiftScale.x = (float)w / pot;

					int	 h = vidHeight;
					for (temp = 1 ; temp < h ; temp<<=1) {
					}
					pot = (float)temp;
					shiftScale.y = (float)h / pot;

					fullView.shaderParms[4] = shiftScale.x;
					fullView.shaderParms[5] = shiftScale.y;
				}

				gameRenderWorld->RenderScene( &portalView );
				renderSystem->CaptureRenderToImage( "_currentRender" );
			}
#endif

			// draw a view to a texture
			renderSystem->CropRenderSize( 256, 256, true );
			gameRenderWorld->RenderScene( &fullView );
			renderSystem->CaptureRenderToFile( shotName );
			renderSystem->UnCrop();
		}
	}
}

/*
================
idObjective::Event_Trigger
================
*/
void idObjective::Event_Trigger( idEntity *activator ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {

		//Pickup( player );

		if ( spawnArgs.GetString( "inv_objective", NULL ) ) {
			if ( player && player->hud ) {
				idStr shotName = gameLocal.GetMapName();
				shotName.StripFileExtension();
				shotName += "/";
				shotName += spawnArgs.GetString( "screenshot" );
				shotName.SetFileExtension( ".tga" );
				player->hud->SetStateString( "screenshot", shotName );
				player->hud->SetStateString( "objective", "1" );
				player->hud->SetStateString( "objectivetext", spawnArgs.GetString( "objectivetext" ) );
				player->hud->SetStateString( "objectivetitle", spawnArgs.GetString( "objectivetitle" ) );
				player->GiveObjective( spawnArgs.GetString( "objectivetitle" ), spawnArgs.GetString( "objectivetext" ), shotName );

				// a tad slow but keeps from having to update all objectives in all maps with a name ptr
				for( int i = 0; i < gameLocal.num_entities; i++ ) {
					if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idObjectiveComplete::Type ) ) {
						if ( idStr::Icmp( spawnArgs.GetString( "objectivetitle" ), gameLocal.entities[ i ]->spawnArgs.GetString( "objectivetitle" ) ) == 0 ){
							gameLocal.entities[ i ]->spawnArgs.SetBool( "objEnabled", true );
							break;
						}
					}
				}

				PostEventMS( &EV_GetPlayerPos, 2000 );
			}
		}
	}
}

/*
================
idObjective::Event_GetPlayerPos
================
*/
void idObjective::Event_GetPlayerPos() {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		playerPos = player->GetPhysics()->GetOrigin();
		PostEventMS( &EV_HideObjective, 100, player );
	}
}

/*
================
idObjective::Event_HideObjective
================
*/
void idObjective::Event_HideObjective(idEntity *e) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		idVec3 v = player->GetPhysics()->GetOrigin() - playerPos;
		if ( v.Length() > 64.0f ) {
			player->HideObjective();
			PostEventMS( &EV_Remove, 0 );
		} else {
			PostEventMS( &EV_HideObjective, 100, player );
		}
	}
}

/*
===============================================================================

  idVideoCDItem

===============================================================================
*/

CLASS_DECLARATION( idItem, idVideoCDItem )
END_CLASS

/*
================
idVideoCDItem::Spawn
================
*/
void idVideoCDItem::Spawn( void ) {
}

/*
================
idVideoCDItem::GiveToPlayer
================
*/
bool idVideoCDItem::GiveToPlayer( idPlayer *player ) {
	idStr str = spawnArgs.GetString( "video" );
	if ( player && str.Length() ) {
		player->GiveVideo( str, &spawnArgs );
	}
	return true;
}

/*
===============================================================================

  idPDAItem

===============================================================================
*/

CLASS_DECLARATION( idItem, idPDAItem )
END_CLASS

/*
================
idPDAItem::GiveToPlayer
================
*/
bool idPDAItem::GiveToPlayer(idPlayer *player) {
	const char *str = spawnArgs.GetString( "pda_name" );
	if ( player ) {
		player->GivePDA( str, &spawnArgs );
	}
	return true;
}

/*
===============================================================================

  idMoveableItem

===============================================================================
*/

const int FIRE_BURNTIME = 8000;

const idEventDef EV_ItemEnableDamage("itemEnableDamage", "f");

CLASS_DECLARATION( idItem, idMoveableItem )
	EVENT( EV_DropToFloor,	idMoveableItem::Event_DropToFloor )
	EVENT( EV_Gib,			idMoveableItem::Event_Gib )
	EVENT(EV_Touch,			idMoveableItem::Event_Touch)
	EVENT(EV_ItemEnableDamage,  idMoveableItem::Event_EnableDamage)
END_CLASS

/*
================
idMoveableItem::idMoveableItem
================
*/
idMoveableItem::idMoveableItem() {
	// blendo eric: script gen
	canDealEntDamage = false;

	trigger = nullptr;
	smoke = nullptr;
	smokeTime = 0;
	smokeParticleAngleLock = false;

	nextSoundTime = 0;
	repeatSmoke = false;

	fireTrigger = nullptr;
	fireTimer = 0;
	fireDamageIntervalTimer = 0;

	maxSmackCount = 0;
	smackCount = 0;

	interestSingleBounceDone = false;

	collideFrobTimer = 0;

	showItemLine = false;

	moveToPlayerTimer = 0;

	sparkTimer = 0;
	isSparking = false;
	sparkEmitter = nullptr;

	outerspaceUpdateTimer = 0;
	outerspaceDeleteCounter = 0;

	canBeLostInSpace = false;

	nextSmackTime = 0;

	//BC
	isOnFire = false;    
	nextDischargeTime = 0;
	nextParticleBounceTime = 0;

	// SW: We need to track this so that we can remove ownership of the item once it's been in the world for X milliseconds
	spawnTime = gameLocal.time;

	itemLineHandle = -1;
	itemLineColor = idVec3(1, 1, 1);
}

/*
================
idMoveableItem::~idMoveableItem
================
*/
idMoveableItem::~idMoveableItem() {
	if ( trigger ) {
		delete trigger;
	}
	if (fireTrigger) {
		delete fireTrigger;
	}

	if (itemLineHandle != -1) {
		gameRenderWorld->FreeEntityDef(itemLineHandle);
	}
}


/*
================
idMoveableItem::Save
================
*/
void idMoveableItem::Save( idSaveGame *savefile ) const {

	savefile->WriteBool( isOnFire ); //  bool isOnFire

	savefile->WriteBool( canDealEntDamage ); //  bool canDealEntDamage

	savefile->WriteStaticObject( idMoveableItem::physicsObj ); //  idPhysics_RigidBody physicsObj
	bool restorePhysics = &physicsObj == GetPhysics();
	savefile->WriteBool( restorePhysics );

	savefile->WriteClipModel( trigger ); //  idClipModel * trigger
	savefile->WriteParticle( smoke ); // const  idDeclParticle * smoke
	savefile->WriteInt( smokeTime ); //  int smokeTime
	savefile->WriteBool( smokeParticleAngleLock ); //  bool smokeParticleAngleLock

	savefile->WriteInt( nextSoundTime ); //  int nextSoundTime

	savefile->WriteBool( repeatSmoke ); //  bool repeatSmoke


	//savefile->WriteClipModel( fireTrigger ); //  idClipModel * fireTrigger // regened

	savefile->WriteInt( fireTimer ); //  int fireTimer
	savefile->WriteInt( fireDamageIntervalTimer ); //  int fireDamageIntervalTimer
	fxFire.Save( savefile ); //  idEntityPtr<idEntityFx> fxFire

	savefile->WriteInt( maxSmackCount ); //  int maxSmackCount
	savefile->WriteInt( smackCount ); //  int smackCount

	savefile->WriteInt( nextDischargeTime ); //  int nextDischargeTime

	savefile->WriteInt( nextParticleBounceTime ); //  int nextParticleBounceTime

	savefile->WriteBool( interestSingleBounceDone ); //  bool interestSingleBounceDone

	savefile->WriteInt( collideFrobTimer ); //  int collideFrobTimer


	savefile->WriteInt( spawnTime ); //  int spawnTime
	savefile->WriteBool( showItemLine ); //  bool showItemLine
	savefile->WriteInt( itemLineHandle ); //  int itemLineHandle
	savefile->WriteVec3( itemLineColor ); //  idVec3 itemLineColor

	savefile->WriteInt( moveToPlayerTimer ); //  int moveToPlayerTimer

	savefile->WriteInt( sparkTimer ); //  int sparkTimer
	savefile->WriteBool( isSparking ); //  bool isSparking
	savefile->WriteObject( sparkEmitter ); // idFuncEmitter* sparkEmitter

	savefile->WriteInt( outerspaceUpdateTimer ); //  int outerspaceUpdateTimer
	savefile->WriteInt( outerspaceDeleteCounter ); //  int outerspaceDeleteCounter

	savefile->WriteBool( canBeLostInSpace ); //  bool canBeLostInSpace

	savefile->WriteInt( nextSmackTime ); //  int nextSmackTime

	savefile->WriteString( smackMaterial ); //  idString smackMaterial

}

/*
================
idMoveableItem::Restore
================
*/
void idMoveableItem::Restore( idRestoreGame *savefile ) {

	savefile->ReadBool( isOnFire ); //  bool isOnFire

	savefile->ReadBool( canDealEntDamage ); //  bool canDealEntDamage

	savefile->ReadStaticObject( physicsObj ); //  idPhysics_RigidBody physicsObj
	bool restorePhys;
	savefile->ReadBool( restorePhys );
	if (restorePhys)
	{
		RestorePhysics( &physicsObj );
	}

	savefile->ReadClipModel( trigger ); //  idClipModel * trigger
	savefile->ReadParticle( smoke ); // const  idDeclParticle * smoke
	savefile->ReadInt( smokeTime ); //  int smokeTime
	savefile->ReadBool( smokeParticleAngleLock ); //  bool smokeParticleAngleLock

	savefile->ReadInt( nextSoundTime ); //  int nextSoundTime

	savefile->ReadBool( repeatSmoke ); //  bool repeatSmoke


	//savefile->ReadClipModel( fireTrigger ); //  idClipModel * fireTrigger
	fireTrigger = new idClipModel(idTraceModel(idBounds(vec3_origin).Expand(24)));
	fireTrigger->Link(gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis());
	fireTrigger->SetContents(CONTENTS_TRIGGER);
	fireTrigger->SetOwner(this);

	savefile->ReadInt( fireTimer ); //  int fireTimer
	savefile->ReadInt( fireDamageIntervalTimer ); //  int fireDamageIntervalTimer
	fxFire.Restore( savefile ); //  idEntityPtr<idEntityFx> fxFire

	savefile->ReadInt( maxSmackCount ); //  int maxSmackCount
	savefile->ReadInt( smackCount ); //  int smackCount

	savefile->ReadInt( nextDischargeTime ); //  int nextDischargeTime

	savefile->ReadInt( nextParticleBounceTime ); //  int nextParticleBounceTime

	savefile->ReadBool( interestSingleBounceDone ); //  bool interestSingleBounceDone

	savefile->ReadInt( collideFrobTimer ); //  int collideFrobTimer


#if !defined(_WIN32) //karin: write int, but read long here. long is 4 bytes in win64 only, others 64-bits is 8bytes
	int i_spawnTime;
	savefile->ReadInt( i_spawnTime ); //  int spawnTime
	spawnTime = i_spawnTime;
#else
	savefile->ReadLong( spawnTime ); //  int spawnTime
#endif
	savefile->ReadBool( showItemLine ); //  bool showItemLine
	savefile->ReadInt( itemLineHandle ); //  int itemLineHandle
	savefile->ReadVec3( itemLineColor ); //  idVec3 itemLineColor

	savefile->ReadInt( moveToPlayerTimer ); //  int moveToPlayerTimer

	savefile->ReadInt( sparkTimer ); //  int sparkTimer
	savefile->ReadBool( isSparking ); //  bool isSparking
	savefile->ReadObject( reinterpret_cast<idClass*&>(sparkEmitter) ); // idFuncEmitter* sparkEmitter

	savefile->ReadInt( outerspaceUpdateTimer ); //  int outerspaceUpdateTimer
	savefile->ReadInt( outerspaceDeleteCounter ); //  int outerspaceDeleteCounter

	savefile->ReadBool( canBeLostInSpace ); //  bool canBeLostInSpace

	savefile->ReadInt( nextSmackTime ); //  int nextSmackTime

	savefile->ReadString( smackMaterial ); //  idString smackMaterial
}

/*
================
idMoveableItem::Spawn
================
*/
void idMoveableItem::Spawn( void ) {
	idTraceModel trm;
	float density, linearFriction, contactFriction, bouncyness, angularFriction, mass;
	idStr clipModelName;
	idBounds bounds;
#ifdef _D3XP
	SetTimeState ts( timeGroup );
#endif

	// SW: Trigger a gravity check // blendo eric: moved this from init, hopefully still fine here
	PostEventMS(&EV_PostSpawn, 0);

	// create a trigger for item pickup
	/*
	spawnArgs.GetFloat( "triggersize", "16.0", tsize );
	trigger = new idClipModel( idTraceModel( idBounds( vec3_origin ).Expand( tsize ) ) );
	trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	
	//BC remove the pickup trigger volume.
	//trigger->SetContents( CONTENTS_TRIGGER );
	trigger->SetContents(0);
	*/


	fireTrigger = new idClipModel(idTraceModel(idBounds(vec3_origin).Expand(24)));
	fireTrigger->Link(gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis());
	fireTrigger->SetContents(CONTENTS_TRIGGER);
	fireTrigger->SetOwner(this);




	// check if a clip model is set
	spawnArgs.GetString( "clipmodel", "", clipModelName );
	if ( !clipModelName[0] ) {
		clipModelName = spawnArgs.GetString( "model" );		// use the visual model
	}

	// load the trace model
	if ( !collisionModelManager->TrmFromModel( clipModelName, trm ) ) {
		gameLocal.Error( "idMoveableItem '%s': cannot load collision model %s", name.c_str(), clipModelName.c_str() );
		return;
	}

	// if the model should be shrinked
	if ( spawnArgs.GetBool( "clipshrink" ) ) {
		trm.Shrink( CM_CLIP_EPSILON );
	}

	// get rigid body properties
	spawnArgs.GetFloat( "density", "0.5", density );
	density = idMath::ClampFloat(0.001f, 1000.0f, density);

	// blendo eric: alternative named parameters
	spawnArgs.GetFloat("linear_friction", "0.05", linearFriction);
	linearFriction = idMath::ClampFloat(0.0f, 1.0f, linearFriction);
	if (spawnArgs.FindKey("friction"))
	{
		spawnArgs.GetFloat("friction", "0.5", contactFriction);
	}
	else
	{
		spawnArgs.GetFloat("contact_friction", "0.5", contactFriction);
	}
	contactFriction = idMath::ClampFloat( 0.0f, 1.0f, contactFriction );

	if (spawnArgs.FindKey("bouncyness"))
	{
		spawnArgs.GetFloat("bouncyness", "0.2", bouncyness);
	}
	else
	{
		spawnArgs.GetFloat("bounce", "0.2", bouncyness);
	}
	bouncyness = idMath::ClampFloat( 0.0f, 1.0f, bouncyness );

	//BC
	spawnArgs.GetFloat("angular_friction", "0.1", angularFriction);
	angularFriction = idMath::ClampFloat(0.0, 1.1f, angularFriction);

	spawnArgs.GetFloat("mass", "1", mass);

	// setup the physics
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( trm ), density );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetBouncyness( bouncyness );
	physicsObj.SetFriction(linearFriction, angularFriction, contactFriction );

	int customGravity = spawnArgs.GetInt("gravity");
	if (customGravity == 0)
	{
		physicsObj.SetGravity(gameLocal.GetGravity()); //use standard gravity
	}
	else
	{
		idVec3 newGravity = vec3_zero;
		newGravity.z = customGravity;
		physicsObj.SetGravity(newGravity);
	}

	physicsObj.SetMass(mass);
	
	////physicsObj.SetContents( CONTENTS_RENDERMODEL );
	//physicsObj.SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

	//physicsObj.SetContents(CONTENTS_SOLID);
	//physicsObj.SetClipMask(MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP);

	//BC make objects physical.
	physicsObj.SetContents( CONTENTS_RENDERMODEL );
	//physicsObj.SetContents(CONTENTS_SOLID);
	//physicsObj.SetClipMask(MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);
	//physicsObj.SetClipMask(CONTENTS_PROJECTILE);

	


	SetPhysics( &physicsObj );

	smoke = NULL;
	smokeTime = 0;
#ifdef _D3XP
	nextSoundTime = 0;
#endif
	const char *smokeName = spawnArgs.GetString( "smoke_trail" );
	if ( *smokeName != '\0' )
	{
		smoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeTime = gameLocal.time;
		BecomeActive( TH_UPDATEPARTICLES );
	}

	smokeParticleAngleLock = spawnArgs.GetBool("smokeanglelock", "1");

#ifdef CTF
	repeatSmoke = spawnArgs.GetBool( "repeatSmoke", "0" );
#endif

	
	
	//BC
	BecomeActive(TH_THINK);
	fl.takedamage = spawnArgs.GetBool("takedamage", "1");
	health = spawnArgs.GetInt("health", "100"); //default health.
	isOnFire = false;
	fireDamageIntervalTimer = 0;
	fxFire = NULL;

	smackCount = 0;
	maxSmackCount = spawnArgs.GetInt("maxsmackcount", "0");

	interestSingleBounceDone = false;
	collideFrobTimer = 0;

    nextDischargeTime = gameLocal.time + spawnArgs.GetInt("nextdischargetime", "1000");


	carryable = spawnArgs.GetBool( "carryable", "0" );
	if ( carryable ) {
		isFrobbable = carryable;
	}

	//Spawn the item line.
	if (pulse)
	{
		showItemLine = true;
	}


    //BC Ok, for weapons we want to jam loose ammo into the magazine. We don't want the magazine to just be empty.
	// SW 27th March 2025: note that this will be immediately overridden if the weapon in question is dropped or thrown by the player,
	// so it's effectively just "initial spawn" behaviour
    if (this->spawnArgs.GetBool("isweapon", "0"))
    {
        const idKeyValue * keyval = this->spawnArgs.MatchPrefix("inv_ammo_");
		
        if (keyval)
        {
            idStr keyName = keyval->GetKey();
            idStr inclipKey = keyval->GetKey();
			idStr inChamberKey = keyval->GetKey();
            inclipKey.Insert("inclip_", 4);
			inChamberKey.Insert("inchamber_", 4);

			// SW 31st March 2025: Shuffling some logic around so that weapons without a clip will still get their initial round in the chamber
			if (this->spawnArgs.GetInt("clipSize", "0") > 0)
			{
				int amount = atoi(keyval->GetValue());

				//BC
				if (spawnArgs.GetBool("mag_randomize"))
				{
					int randomMin = spawnArgs.GetInt("mag_randmin");
					int randomMax = spawnArgs.GetInt("mag_randmax");
					amount = gameLocal.random.RandomInt(randomMin, randomMax);
				}

				if (amount)
				{
					//Stuff ammo into mag.
					this->spawnArgs.SetInt(inclipKey, amount);

					//and also REMOVE the loose ammo amount.
					this->spawnArgs.SetInt(keyName, 0);
				}
				else
				{
					// SW 1st May 2025: even if there are no rounds, we want to make sure we have the keyvalue in there for other code to look at
					this->spawnArgs.SetInt(inclipKey, 0);
				}
			}

			// SW 27th March 2025: add a round to the chamber for initial spawn
			this->spawnArgs.SetInt(inChamberKey, 1);
        }
    }

	itemLineColor = spawnArgs.GetVector("itemlinecolor", "1 1 1");

	moveToPlayerTimer = 0;

	//spark logic.
	isSparking = false;
	sparkTimer = 0;
	if (spawnArgs.GetBool("bashspark"))
	{
		idVec3 forward, right, up;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

		idVec3 sparkOffset = spawnArgs.GetVector("bashspark_offset", "0 0 0");
		idVec3 sparkPos = GetPhysics()->GetOrigin() + (forward * sparkOffset.x) + (right * sparkOffset.y) + (up * sparkOffset.z);

		idDict args;
		args.Clear();
		args.Set("model", "machine_sparkstream04.prt");
		args.Set("start_off", "1");
		args.SetVector("origin", sparkPos);
		sparkEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));		
		sparkEmitter->Bind(this, false);
	}
	else
	{
		sparkEmitter = NULL;
	}

	outerspaceUpdateTimer = 0;
	outerspaceDeleteCounter = 0;
	canBeLostInSpace = spawnArgs.GetBool("can_lostinspace", "1");

	smackMaterial = spawnArgs.GetString("mtr_smack");

	if (!g_bloodEffects.GetBool() && spawnArgs.FindKey("mtr_smack_noblood") != NULL)
	{
		smackMaterial = spawnArgs.GetString("mtr_smack_noblood");
	}

	idStr soundName = spawnArgs.GetString("snd_ambient");
	if (soundName.Length() > 1)
	{
		StartSound("snd_ambient", SND_CHANNEL_BODY);
	}
}

// SW 18th Feb 2025
// Adding bespoke show/hide functions to idMoveableItem so that spark trails are correctly hidden
// if the item is stowed away in the inventory
void idMoveableItem::Show(void)
{
	idItem::Show();

	if (sparkEmitter != NULL)
	{
		sparkEmitter->Show();

		// Unmute sparks if they're sparking
		// SND_CHANNEL_WEAPON is used for the spark sound here
		if (refSound.referenceSound != NULL)
		{
			refSound.referenceSound->FadeSound(SND_CHANNEL_WEAPON, 0, 0);
		}
	}
}

void idMoveableItem::Hide(void)
{
	idItem::Hide();

	if (sparkEmitter != NULL)
	{
		sparkEmitter->Hide();

		// Mute so the hidden item doesn't continue to emit the sparking sound
		// SND_CHANNEL_WEAPON is used for the spark sound here
		if (refSound.referenceSound != NULL)
		{
			refSound.referenceSound->FadeSound(SND_CHANNEL_WEAPON, -60, 0);
		}
	}
}

/*
================
idMoveableItem::Think
================
*/
void idMoveableItem::Think( void ) {

	if (this->IsHidden())
		return;

	RunPhysics();
	/*
	if ( thinkFlags & TH_PHYSICS ) {
		// update trigger position
		trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), mat3_identity );
	}*/

	// Fire triggers serve double duty as they are also a trigger for picking up objects
	// Either way, if the object is not at rest the fire trigger position needs to update
	if (isOnFire || !IsAtRest())
	{
		fireTrigger->Link(gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), mat3_identity);
	}

	if ( thinkFlags & TH_UPDATEPARTICLES )
	{
		
		//BC todo: orient particle angle based on item's velocity.
		//idMat3 particleAngle;
		//particleAngle = GetPhysics()->GetAxis();
		//idVec3 forwardVel = GetPhysics()->GetLinearVelocity();
		//forwardVel.NormalizeFast();
		//particleAngle = forwardVel.ToMat3();

		if ( !gameLocal.smokeParticles->EmitSmoke( smoke, smokeTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), smokeParticleAngleLock ? GetPhysics()->GetAxis() : mat3_identity, timeGroup /*_D3XP*/ ) ) {
#ifdef CTF
			if ( !repeatSmoke ) {
				smokeTime = 0;
				BecomeInactive( TH_UPDATEPARTICLES );
			} else {
				smokeTime = gameLocal.time;
			}
#else
			smokeTime = 0;
			BecomeInactive( TH_UPDATEPARTICLES );
#endif
		}
	}

	//BC
	TouchTriggers();

	if (gameLocal.time >= fireTimer && isOnFire)
	{
		isOnFire = false;

		
		if (fxFire.GetEntity())
		{
			//Stop fire fx.
			static_cast<idEntityFx *>(fxFire.GetEntity())->FadeOut();
		}
		
	}

	if (isOnFire)
	{
		if (gameLocal.time > fireDamageIntervalTimer)
		{
			//Do fire damage to self.
			const char	*damageDefName;
			fireDamageIntervalTimer = gameLocal.time + 200;

			damageDefName = spawnArgs.GetString("def_firedamage_self", "damage_fire_dot");

			if (damageDefName[0] != '\0')
			{
				this->Damage(this, NULL, vec3_zero, damageDefName, 1.0f, -1);
			}
		}
	}

	//gameRenderWorld->DrawTextA(va("%d", (int)health), this->GetPhysics()->GetOrigin() + idVec3(0, 0, 32), .15f, idVec4(1, 1, 1, 1), gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
	
	// SW: Dropped items like magazines and tele-pucks will initially have their owner set to the player, to avoid collision issues.
	// However, leaving them in this state causes havoc with frob traces, so we need to remove the owner once we've been in the world for a bit
	idClipModel* cm = this->GetPhysics()->GetClipModel();
	if (cm->GetOwner() && cm->GetOwner() == gameLocal.GetLocalPlayer() && gameLocal.time - spawnTime > MOVEABLE_PLAYEROWNERSHIP_LIFETIME && gameLocal.GetLocalPlayer()->GetCarryable() != this && !justDropped)
		cm->SetOwner(NULL);
	
	UpdateMoveToPlayer();

	UpdateSparkTimer();

	UpdateSpacePush();	

	if (gameLocal.time > dropInvincibleTimer + ITEM_DROPINVINCIBLE_TIME && dropInvincibleActive)
	{
		dropInvincibleActive = false;
	}

	Present();
}

void idMoveableItem::UpdateSpacePush()
{
	//if object is in outer space, then make it drift toward the 'back' of the ship, to simulate the ship moving through space.
	if (gameLocal.time > outerspaceUpdateTimer)
	{
		#define SPACEPUSH_UPDATEINTERVAL 200
		outerspaceUpdateTimer = gameLocal.time + SPACEPUSH_UPDATEINTERVAL; //how often to update the space check timer.

		if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
		{
			if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
			{
				return; //skip if I'm being held by player.
			}
		}

		if (this->GetBindMaster() != NULL)
			return; //skip if I'm bound to something

		//Only do the push if FTL is active.
		idMeta *meta = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity());
		if (meta)
		{
			idFTL *ftl = static_cast<idFTL *>(meta->GetFTLDrive.GetEntity());
			if (ftl)
			{
				if (!ftl->IsJumpActive(false, false))
				{
					return;
				}
			}
		}


		if (isInOuterSpace() && canBeLostInSpace)
		{
			if (gameLocal.world->doSpacePush)
			{
				#define SPACEPUSH_POWER 8
				idVec3 spacePushPower = idVec3(0, -1, 0); //move toward back of ship.
				spacePushPower = spacePushPower * SPACEPUSH_POWER * this->GetPhysics()->GetMass();
				GetPhysics()->ApplyImpulse(0, GetPhysics()->GetAbsBounds().GetCenter(), spacePushPower);
			}

			
			//Update when I blink out of existence.
			//We delete the item when the player hasn't seen it for a while. This is
			//intended to simulate the item floating away when the player isn't looking.

			if (spawnArgs.GetBool("lostinspace_aggressive", "0"))
			{
				outerspaceDeleteCounter++;
			}
			else
			{
				//Normal behavior. If player is looking at the object, don't delete it.
				if (!gameLocal.InPlayerConnectedArea(this))
				{
					//If I'm in a pvs that's totally disconnected from the item...
					outerspaceDeleteCounter++;
				}
				else
				{
					//do LOS check.
					trace_t tr;
					//gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, GetPhysics()->GetOrigin(), MASK_SOLID, NULL);

					//Allow trace to go through windows.
					const int MASK_AIMASSIST = CONTENTS_RENDERMODEL | CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_BODY;
					const int MASK_AIMASSIST_IGNORE = CONTENTS_TRANSLUCENT;
					gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, GetPhysics()->GetOrigin(), MASK_AIMASSIST, this, MASK_AIMASSIST_IGNORE);

					if (tr.fraction < 1)
					{
						outerspaceDeleteCounter++;
					}
					else
					{
						outerspaceDeleteCounter = 0; //reset the delete counter.
					}
				}
			}

			#define	DELETECOUNTER_THRESHOLD 4
			if (outerspaceDeleteCounter > DELETECOUNTER_THRESHOLD)
			{
				SetLostInSpace();
			}			
		}
		else
		{
			outerspaceDeleteCounter = 0; //reset the delete counter.
		}
	}
}

void idMoveableItem::SetLostInSpace()
{
	//player hasn't seen the object for a while.
	//object has been consumed by FTL space......
	bool shouldRemove = true;

	if (IsType(idSkullsaver::Type))
	{
		//BC 2-13-2025: play VO if skull is lost in space & player has LOS to it happening.
		if (DoesPlayerHaveLOStoMe_Simple())
		{
			gameLocal.GetLocalPlayer()->SayVO_WithIntervalDelay("snd_vo_throw_skull_space"); //BC 3-3-2025: fixed bug where skull vo was being called without cooldown.
		}


		static_cast<idSkullsaver*>(this)->StoreSkull();
	}
	else if (IsType(idTablet::Type))
	{
		//5-28-2025: workaround fix for tablets, to fix bug where their text gets wipe out when they go to lostandfound. Instead,
		//teleport their position to the lost and found.
		idLostAndFound* lostAndFound = static_cast<idLostAndFound*>(static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->FindLostAndFoundMachine());
		if (lostAndFound)
		{
			idVec3 newPosition = lostAndFound->FindValidSpawnPosition(GetPhysics()->GetBounds());
			GetPhysics()->SetOrigin(newPosition);
			UpdateGravity();
			shouldRemove = false;

			//BC 5-28-2025: print message that tablet is sent to lost and found.
			gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_lostfound_sent"), displayName.c_str()), GetPhysics()->GetOrigin());

			dropTimer = gameLocal.time; //This is to prevent dropped items from instantly creating interestpoints when they hit the ground.
		}
	}
	else
	{
		gameLocal.GetLocalPlayer()->AddLostInSpace(entityDefNumber);
	}

	bool showInfoFeed = (gameLocal.time > 2000) ? true : false; //dont show lost in space message if within first couple seconds of level

	gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_lostinspace"), displayName.c_str()), GetPhysics()->GetOrigin(), showInfoFeed);
	
	if (shouldRemove) {
		gameLocal.DoParticle(spawnArgs.GetString("model_lostinspace", "lost_despawn.prt"), GetPhysics()->GetOrigin());
		Hide();
		PostEventMS(&EV_Remove, 0);
	}
}

void idMoveableItem::SetNextSoundTime(int value)
{
	nextSoundTime = value;
}

void idMoveableItem::UpdateSparkTimer()
{
	if (isSparking && gameLocal.time > sparkTimer)
	{
		//stop sparking.
		isSparking = false;
		StopSound(SND_CHANNEL_WEAPON); //stop the sparking sound.

		if (sparkEmitter)
		{
			sparkEmitter->SetActive(false);
		}
	}
}

//Make some items wiggle themselves toward the player's location.
void idMoveableItem::UpdateMoveToPlayer()
{
	if (!gameLocal.InPlayerConnectedArea(this))
		return;

	if (!spawnArgs.GetBool("movetoplayer", "0"))
		return;

	if (gameLocal.time < moveToPlayerTimer)
		return;

	#define WIGGLE_TIMER 200
	moveToPlayerTimer = gameLocal.time + WIGGLE_TIMER;

	//Distance check.
	float distance;
	distance = (GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast();
	#define WIGGLE_DISTANCE 160
	#define WIGGLE_DISTANCE_MIN 12
	if (distance > WIGGLE_DISTANCE || distance < WIGGLE_DISTANCE_MIN)
		return;

	//Check LOS.
	trace_t tr;
	gameLocal.clip.TracePoint(tr, GetPhysics()->GetOrigin() + idVec3(0, 0, 1), gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0, 0, 1), MASK_SOLID, NULL);
	if (tr.fraction < 1)
		return;

	//make it wiggle.
	#define WIGGLE_SPEED 48
	idVec3 directionToPlayer = (gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0,0,8)) - GetPhysics()->GetOrigin();
	directionToPlayer.Normalize();
	GetPhysics()->ApplyImpulse(0, GetPhysics()->GetOrigin(), directionToPlayer * WIGGLE_SPEED * GetPhysics()->GetMass());
}

void idMoveableItem::Event_Touch(idEntity *other, trace_t *trace)
{
	if (other->IsType(idMoveableItem::Type) && !isOnFire)
	{
		if (fireTrigger && trace->c.id == fireTrigger->GetId() && static_cast<idMoveableItem *>(other)->isOnFire)
		{
			Ignite();
		}
	}
	else if (other->IsType(idActor::Type) && isOnFire)
	{
		//Damage 
		const char	*damageDefName = spawnArgs.GetString("def_firedamage", "damage_fire"); //Damage done to others. By default, spread the fire.

		if (damageDefName[0] != '\0')
		{
			other->Damage(this, NULL, vec3_zero, damageDefName, 1.0f, CLIPMODEL_ID_TO_JOINT_HANDLE(trace->c.id));
		}
	}

	if (spawnArgs.GetBool("anytouch", "0") && GetBindMaster() == NULL)
	{
		if (other->IsType(idPlayer::Type))
		{
			//Pickup(static_cast<idPlayer *>(other));

			DoFrob(0, other);
		}
	}
}

bool idMoveableItem::DoFrob(int index, idEntity * frobber)
{
	if (frobber == gameLocal.GetLocalPlayer())
	{
		if (canPickUp)
		{
			//Pickuppable item.
			//bc This is for things like picking up an item and putting it into inventory. i.e., ammo, weapons, security cards, etc.

			idEntity* owner = GetBindMaster();
			if (owner && owner->IsType(idActor::Type) && frobber != NULL)
			{
				if (frobber == gameLocal.GetLocalPlayer())
				{
					static_cast<idActor*>(owner)->PlayerTookCarryable(this);
				}
			}

			bool ret = idItem::Pickup(gameLocal.GetLocalPlayer());
			if (ret)
			{
				//item was picked up.
				showItemLine = false;
				if (itemLineHandle != -1)
				{
					gameRenderWorld->FreeEntityDef(itemLineHandle);
					itemLineHandle = -1;
				}

				if (spawnArgs.GetBool("activateTargetsOnPickup"))
					ActivateTargets(gameLocal.GetLocalPlayer());

				StopSound(SND_CHANNEL_BODY);
			}
			return ret;
		}
		else if (carryable)
		{
			//This is for picking things up and carrying them around, NOT in inventory, but just carrying them in hand. i.e. teapot
			//gameLocal.GetLocalPlayer()->SetCarryable(this, true);
			//return true;

			bool hasGiven = gameLocal.GetLocalPlayer()->PlayerFrobbedCarryable(this);

			if (hasGiven)
			{
                //hide item line.
                showItemLine = false;
                if (itemLineHandle != -1)
                {
                    gameRenderWorld->FreeEntityDef(itemLineHandle);
                    itemLineHandle = -1;
                }

				if (spawnArgs.GetBool("activateTargetsOnPickup"))
					ActivateTargets(gameLocal.GetLocalPlayer());

				StopSound(SND_CHANNEL_BODY);

				gameLocal.GetLocalPlayer()->StartSound("snd_grab", SND_CHANNEL_ANY);
				idEntityFx::StartFx("fx/pickupitem", &this->GetPhysics()->GetOrigin(), &mat3_default, NULL, false); //BC
			}

			return hasGiven;
		}
	}
	
	return false;
}

// SW 6th Feb 2025: wake up physics on physics objects the item is near
// so we don't end up with 'hovering' stacks of items
// (Doing this with actual physics contacts is unreliable, so we use a slightly expanded bounding box
void idMoveableItem::WakeNearbyMoveablePhysics(void)
{
	idEntity* entityList[32];
	idEntity* touchingEntity = NULL;
	int numEntities = gameLocal.EntitiesWithinAbsBoundingbox(this->GetPhysics()->GetAbsBounds().Expand(1), entityList, 32);
	for (int i = 0; i < numEntities; i++)
	{
		touchingEntity = entityList[i];
		// For each entity, if it has valid physics and it's asleep, wake it up
		if (touchingEntity != NULL && 
			touchingEntity != this && 
			touchingEntity->IsType(idMoveableItem::Type) && 
			touchingEntity->GetPhysics() != NULL && 
			touchingEntity->GetPhysics()->IsAtRest())
		{
			touchingEntity->ActivatePhysics(this);
			static_cast<idMoveableItem*>(touchingEntity)->WakeNearbyMoveablePhysics(); // recurse for stacks of items
		}
	}
}

// SW: Because thrown objects slowly drift to a halt in a vacuum, 
// it's possible to get a case where they never progress beyond the 'idle' state, unable to be interacted with.
// We break out of this special case by destroying the object and creating the pickup again.
// Each throwable object is responsible for determining *when* to call this method,
// as they all have their own individual states and logic.
void idMoveableItem::TryRevertToPickup(void)
{
	idVec3 origin = this->GetPhysics()->GetOrigin();

	// This additional check for zero-G is unfortunately necessary,
	// because otherwise this code will trigger at the apex if we throw the throwable straight up into the air (!!)
	if (gameLocal.GetAirlessAtPoint(origin))
	{
		this->GetPhysics()->SetClipMask(0); // Disable clipping so we don't collide with the pickup spawning on top of us
		PostEventMS(&EV_Remove, 0);

		// Try to find the classname of the pickup associated with this thrown object
		// (If we fail to find one, or it doesn't exist, just destroy the object)
		const idKeyValue* pickupKV = spawnArgs.FindKey("def_dropItem");
		if (pickupKV && !pickupKV->GetValue().IsEmpty())
		{
			idMat3 axis = this->GetPhysics()->GetAxis();
			
			idEntity* pickupEnt;
			const idDict* pickupDict = gameLocal.FindEntityDefDict(pickupKV->GetValue());
			gameLocal.SpawnEntityDef(*pickupDict, &pickupEnt);
			pickupEnt->SetOrigin(origin);
			pickupEnt->SetAxis(axis);
		}
	}
}

void idMoveableItem::Ignite(void)
{
	//Is taking FIRE damage. Make it burst into flames.
	const char	*fxName;

	if (spawnArgs.GetFloat("flammability", "0") < 1.0f) //Check if item is flammable.
		return;

	if (isOnFire)
		return;

	fxName = spawnArgs.GetString("fx_fire", "fx/small_fire_loop");

	if (fxName[0] != '\0')
	{
		fxFire = idEntityFx::StartFx(fxName, NULL, NULL, this, true);
	}
	else
	{
		common->Warning("%s is missing fx_fire setting.\n", this->GetName());
	}

	isOnFire = true;
	fireTimer = gameLocal.time + FIRE_BURNTIME;
}

//Gets called when a thrown moveableitem hits a frobbable ent. Frobs the ent.
void idMoveableItem::ThrownFrob(idEntity * ent)
{
	if (ent == NULL)
		return;

	if (!spawnArgs.GetBool("canfrob", "1"))
		return;

	#define JUST_SPAWNED_GRACEPERIOD 600
	if ((gameLocal.time - dropTimer) < JUST_SPAWNED_GRACEPERIOD) //Prevent it from frobbing something if it JUST spawned into the world. Because it's too weird/unreadable if a newly-spawned object immediately frobs something.
		return;
	

	if (ent->DoFrob(0, this))
	{
		//BC 2-27-2025: fix bug where the thing being interacted with sometimes has no frob name (such as frobcube).
		//Try to find a suitable name.
		idStr interacteeName = ent->displayName;
		if (interacteeName.Length() <= 0)
		{
			//name is empty.
			//see if it has an owner or is bound to something.
			if (ent->GetBindMaster() != nullptr)
			{
				//get the bindmaster's name.
				interacteeName = ent->GetBindMaster()->displayName;
			}
		}
		
		//Try again, but with owner.
		if (interacteeName.Length() <= 0)
		{
			if (ent->GetPhysics()->GetClipModel()->GetOwner() != nullptr)
			{
				interacteeName = ent->GetPhysics()->GetClipModel()->GetOwner()->displayName;
			}
		}

		//Still fail, so just exit out here.
		if (interacteeName.Length() <= 0)
		{
			gameLocal.Warning("Failed to find interactee name for str_def_gameplay_frobbed: %s interact with %s\n",
				common->GetLanguageDict()->GetString(displayName.c_str()),
				ent->GetName());
		}
		else
		{
			gameLocal.AddEventLog(idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_frobbed"),
				common->GetLanguageDict()->GetString(displayName.c_str()),
				common->GetLanguageDict()->GetString(interacteeName.c_str())),
				ent->GetPhysics()->GetOrigin());
		}

		idEntityFx::StartFx("fx/frob_lines", &ent->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
	}

	collideFrobTimer = gameLocal.time + 200; //Limit frob to every xx milliseconds, so that it doesn't trigger multiple frames in a row.	
}

#ifdef _D3XP
/*
=================
idMoveableItem::Collide
=================
*/
bool idMoveableItem::Collide( const trace_t &collision, const idVec3 &velocity ) {
	float		v, f;
	idEntity	*ent;

	ent = gameLocal.entities[collision.c.entityNum];

	// Check if we're hitting a skullsaver here because checking in the skullsaver collide sometimes doesn't fire
	if ( ent && ent->IsType( idSkullsaver::Type ) )
	{
		idSkullsaver* skullsaver = static_cast< idSkullsaver* >( ent );
		// SW 27th Feb 2025: Check for respawn state too
		if ( skullsaver->IsConveying() || skullsaver->IsRespawning() )
		{
			skullsaver->ResetConveyTime();
		}
	}

	v = -( velocity * collision.c.normal );

	if (v > 20 && gameLocal.time > nextParticleBounceTime)
	{
		const char *fxBounce;
		fxBounce = spawnArgs.GetString("fx_bounce");
		if (!g_bloodEffects.GetBool() && spawnArgs.FindKey("fx_bounce_noblood"))
		{
			fxBounce = spawnArgs.GetString("fx_bounce_noblood");
		}
		if (*fxBounce != '\0')
		{
			idAngles particleAngle = collision.c.normal.ToAngles();
			particleAngle.pitch += 90;
			idEntityFx::StartFx(fxBounce, collision.endpos, particleAngle.ToMat3());
		}

		nextParticleBounceTime = gameLocal.time + 300;


		//also do spark logic here.
		if (spawnArgs.GetBool("impact_spark"))
		{
			if (gameLocal.IsCollisionSparkable(collision))
			{
				gameLocal.CreateSparkObject(GetPhysics()->GetOrigin(), collision.c.normal);
			}
		}
	}
	
	if (v > 30 && gameLocal.time > nextSmackTime && smackMaterial.Length() > 0)
	{
		nextSmackTime = gameLocal.time + 100;

		if ((maxSmackCount > 0 && smackCount < maxSmackCount) || maxSmackCount <= 0)
		{
			gameLocal.ProjectDecal(collision.c.point, -collision.c.normal, 0.5f, true, spawnArgs.GetInt("smacksize", "16"), smackMaterial.c_str());
			smackCount++;
		}
	}

	if ( v > 30 && gameLocal.time > nextSoundTime )
	{
		

		const char *sound, *typeName;
		surfTypes_t materialType;
		bool useSpecificSound;

		//const char *interestMultibounceDef;

		f = v > 200 ? 1.0f : idMath::Sqrt( v - 80 ) * 0.091f;

		//BC add material-specific sound interactions.
		if (collision.c.material != NULL)
		{
			materialType = collision.c.material->GetSurfaceType();
		}
		else
		{
			materialType = SURFTYPE_METAL;
		}

		typeName = gameLocal.sufaceTypeNames[materialType];		

		sound = spawnArgs.GetString(va("snd_bounce_%s", typeName));

		if (*sound == '\0')
		{
			useSpecificSound = false;
		}
		else
		{
			useSpecificSound = true;
		}

		if (gameLocal.time > 1000) //don't play these bounce sounds at game start.
		{
			if (StartSound(useSpecificSound ? va("snd_bounce_%s", typeName) : "snd_bounce", SND_CHANNEL_ANY, 0, false, NULL))
			{
				// don't set the volume unless there is a bounce sound as it overrides the entire channel
				// which causes footsteps on ai's to not honor their shader parms
				SetSoundVolume(f);
			}
		}

		




		if (!interestSingleBounceDone && gameLocal.time > dropTimer + INTERESTPOINT_DISABLETIME && v  > 80)
		{
			const char *singlebounceDef;
			if (spawnArgs.GetString("interest_singlebounce", "", &singlebounceDef))
			{
				gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), singlebounceDef);
			}
			interestSingleBounceDone = true;
		}

		if (gameLocal.time > dropTimer + INTERESTPOINT_DISABLETIME && v > 80) //If player drops the item, then ignore interestpoint generation for a short bit of time.
		{
			//ability to spawn multiple interestpoints, if you want both audio and visual.
			//interest_multibounce, interest_multibounce2, interest_multibounce3, etc etc
			const idKeyValue *kv;
			kv = this->spawnArgs.MatchPrefix("interest_multibounce", NULL);
			while (kv)
			{
				const char *interestName = kv->GetValue();				
				if (interestName[0] != '\0')
				{
					gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), interestName);
				}

				kv = this->spawnArgs.MatchPrefix("interest_multibounce", kv); //Iterate to next entry.
			}


			//The old method where it only spawned one interestpoint
			//interestMultibounceDef = spawnArgs.GetString("interest_multibounce", "");
			//if (interestMultibounceDef)
			//{
			//	gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), interestMultibounceDef);
			//}
		}

		

        //================================= WEAPON MISFIRE =================================
		//if this is a weapon, then it fires when it hits the ground.
		//if (spawnArgs.GetBool("isweapon") && v > 80)

		if (v > 70)
		{
			bool hitFrobbable = false;

			//Frob things that it touches.
			if (collision.c.entityNum > 0 && collision.c.entityNum < MAX_GENTITIES - 2 && gameLocal.time > collideFrobTimer)
			{
				if (gameLocal.entities[collision.c.entityNum]->isFrobbable)
				{
					ThrownFrob(gameLocal.entities[collision.c.entityNum]);					
					hitFrobbable = true;
				}
			}

			if (!hitFrobbable)
			{
				//did NOT hit a frobbable.
				#define LENIENCY_FROB_DISTANCE 24

				//Do a leniency check to see if we hit NEAR a frobbable.
				idEntity	*entityList[MAX_GENTITIES];
				int			listedEntities, i;

				listedEntities = gameLocal.EntitiesWithinRadius(collision.endpos, LENIENCY_FROB_DISTANCE, entityList, MAX_GENTITIES);
				if (listedEntities > 0)
				{
					for (i = 0; i < listedEntities; i++)
					{
						idEntity *ent = entityList[i];

						if (!ent)
							continue;

						if (ent == this)
							continue;

						if (ent->IsHidden())
							continue;

						if (ent->IsType(idLever::Type) || ent->IsType(idCatcage::Type))						
						{
							//trace_t losTr;
							//gameLocal.clip.TracePoint(losTr, collision.endpos, ent->GetPhysics()->GetOrigin(), MASK_SHOT_RENDERMODEL, this, MASK_MONSTERSOLID);
							//if (losTr.c.entityNum == ent->entityNumber)
							{
								//ok we are NEAR a frobbable AND we have LOS to it. Press it.
								ThrownFrob(ent);
							}
						}
					}
				}
			}



			if (collision.c.normal.z > .5f && gameLocal.time > nextDischargeTime)
			{
				//it hit the ground.

				//Check ammo.
				// SW 1st April 2025: use chamber to determine if it should misfire, not clip
				// (so weapons without a round chambered won't misfire)
				const idKeyValue * inchamber = spawnArgs.MatchPrefix("inv_inchamber_");
				const idKeyValue * inclip = spawnArgs.MatchPrefix("inv_inclip_");
				if (inchamber && inclip && spawnArgs.GetBool("canMisfire", "0")) //BC 4-3-2025 make this default to canMisfare 0, to prevent crash with non-misfireable entities
				{
					// There should only ever be 1 or 0 in the chamber, but we express it as an integer, oops
					int roundsInChamber = atoi(inchamber->GetValue());
					int roundsInClip = atoi(inclip->GetValue());

					if (roundsInChamber > 0)
					{
						//Still have at least one round remaining. Fire the weapon.

						//Find the projectile definition.
						const char *projectileName;
						projectileName = spawnArgs.GetString("def_projectile", "");
						if (*projectileName != '\0')
						{
							const idDict *	projectileDef;

							projectileDef = gameLocal.FindEntityDefDict(projectileName, false);

							if (projectileDef)
							{
								int i;
								int shellcount = spawnArgs.GetInt("shellcount", "1");

								for (i = 0; i < shellcount; i++)
								{
									idEntity *		projectileEnt;
									gameLocal.SpawnEntityDef(*projectileDef, &projectileEnt, false);

									if (projectileEnt && projectileEnt->IsType(idProjectile::Type))
									{
										idVec3 projectileDir;
										idAngles propelAngles;
										float propelSpeed;
										idProjectile *	projectile;

										projectileDir = this->GetPhysics()->GetAxis().ToAngles().ToForward();

										//Do some fudging here. If gunfire is aimed toward ground, re-aim it upward toward sky. TODO: does this work????
										if (projectileDir.z < 0)
										{
											projectileDir.z = -projectileDir.z;
										}
										
										if (shellcount > 1)
										{											
											//for multi-shell shots, i.e. shotgun. We need to handle bullet spread.
											float ang, spin;
											float spreadRad = DEG2RAD(spawnArgs.GetFloat("spread", "1"));
											idMat3 shellMat;

											//Copy the weapon spread code.
											ang = idMath::Sin(spreadRad * gameLocal.random.RandomFloat());
											spin = (float)DEG2RAD(360.0f) * gameLocal.random.RandomFloat();

											shellMat = projectileDir.ToMat3();
											projectileDir = shellMat[0] + shellMat[2] * (ang * idMath::Sin(spin)) - shellMat[1] * (ang * idMath::Cos(spin));
											projectileDir.Normalize();
										}
										
										//Launch the projectile.
										projectile = (idProjectile *)projectileEnt;
										projectile->Create(this, this->GetPhysics()->GetOrigin(), projectileDir);
										projectile->Launch(this->GetPhysics()->GetOrigin(), projectileDir, vec3_origin);

										StartSound("snd_fire", SND_CHANNEL_BODY, 0, false, NULL);

										//Muzzle flash fx.
										idEntityFx::StartFx("fx/muzzlefire", &this->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);

										//Propel the weapon.
										//Tweak the propel direction to encourage weapon sliding across the ground.
										propelAngles = projectileDir.ToAngles();
										propelAngles.pitch = idMath::ClampFloat(5, 20, propelAngles.pitch); //clamp propel direction to a low angle. -90 = straight up, 90 = straight down.	 


										propelSpeed = spawnArgs.GetFloat("propelspeed", "384");
										this->ApplyImpulse(NULL, 0, this->GetPhysics()->GetOrigin(), (propelAngles.ToForward() * -propelSpeed) * this->GetPhysics()->GetMass());

										
									}
								}

								//BC 4-3-2025: Moved this out of the for loop so that it didn't create a bajillion misfire events.
								if (!spawnArgs.GetBool("silent_fire"))
								{
									gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), "interest_weaponfire");

									//add text to infofeed. "hit ground and misfired"
									gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_misfire"), displayName.c_str()), GetPhysics()->GetOrigin());
								}

								//Expend one round. If there are no rounds left in the clip/magazine, empty the chamber
								if (roundsInClip > 0)
								{
									// subtract one from the clip and move it to the chamber
									spawnArgs.SetInt(inchamber->GetKey(), 1);
									spawnArgs.SetInt(inclip->GetKey(), roundsInClip - 1);
								}
								else
								{
									// empty out the chamber (this was the last round
									spawnArgs.SetInt(inchamber->GetKey(), 0);
									spawnArgs.SetInt(inclip->GetKey(), 0);
									StartSound("snd_lastshot", SND_CHANNEL_BODY, 0, false, NULL);
								}
								
							}							
						}
					}
					else
					{
						//No more ammo in mag, so do a dryfire click when it hits the ground.
						StartSound("snd_dryfire", SND_CHANNEL_BODY, 0, false, NULL);
					}
				}

				nextDischargeTime = gameLocal.time + 1000; //can only discharge once every X time interval.
			}
		}

		nextSoundTime = gameLocal.time + 500;
	}

	if (ent && canDealEntDamage && health > 0) //Hit another ent. Note: we do a health check here largely for the 'canbreak 0' value, i.e. for skullsavers that have been depleted.
	{
		//TODO: Detect if this is from a player kick. If so, then override this speed requirement.
		//if (v >= 150 && ent->IsType(idActor::Type))
		if (v >= ent->itemDamageVelocityMin && ent->fl.takedamage)
		{
			//BC item is flying at high speeds and hit an actor. Inflict damage to actor. TODO: do a special knock-down damage.
			const char	*damageDefName;
			idVec3 damageDir = velocity;

			damageDir.Normalize();			

			damageDefName = spawnArgs.GetString("def_damage", "damage_throwable");
			
			if (damageDefName[0] != '\0')
			{
				idEntity *attackerEnt;

				if (this->lastThrower.IsValid())
					attackerEnt = lastThrower.GetEntity();
				//else if (this->GetPhysics()->GetClipModel()->GetOwner() != NULL) //crash.
				//	attackerEnt = this->GetPhysics()->GetClipModel()->GetOwner();
				else
					attackerEnt = NULL;

				//Thrown item has injured something.
				int startingHealth = ent->health;
				ent->Damage(this, attackerEnt, damageDir, damageDefName, 1.0f, CLIPMODEL_ID_TO_JOINT_HANDLE(collision.c.id));
				int endingHealth = ent->health;

				if (endingHealth < startingHealth)
				{
					ent->lastDamageTime = gameLocal.time;
				}


				

				if (ent->spawnArgs.GetBool("bleed"))
				{
					const idDict *damageDef = gameLocal.FindEntityDefDict(damageDefName);
					if (damageDef)
					{
						if (damageDef->GetInt("damage") > 0 /*&& startingHealth > endingHealth*/) //BC only apply damage decal if damage is inflicted.
						{
							if (damageDef->GetBool("ignore_player") && ent == gameLocal.GetLocalPlayer())
							{
							}
							else
							{
								ent->AddDamageEffect(collision, velocity, damageDefName);
							}
						}
					}
				}

				#define	COLLISION_DESTROY_THRESHOLD 128
#if _DEBUG
				//if (v < COLLISION_DESTROY_THRESHOLD)
					//common->Printf("ITEM '%s' COLLIDE WITH ACTOR '%s', insufficent velocity '%f'. .\n", this->GetName(), ent->GetName(), v);
#endif


				bool objectAppliedDamage = false;
				
				if (1)
				{
					const idDict *damageDef = gameLocal.FindEntityDefDict(damageDefName);
					if (damageDef)
					{
						if (damageDef->GetInt("damage") > 0 && (startingHealth > endingHealth))
						{
							objectAppliedDamage = true;

							//do durability damage to self.
							if (this->health > 0)
							{
								this->Damage(NULL, NULL, vec3_zero, "damage_durabilitybash", 1.0f, 0); //apply damage to the carryable.
							}
						}
					}
				}

				//Only destroy object if it's moving fast.
				//Thrown items are immediately destroyed.
				if (v >= COLLISION_DESTROY_THRESHOLD && ent->IsType(idActor::Type) && this->spawnArgs.GetBool("remove_on_actor", "1") && ent != gameLocal.GetLocalPlayer() && objectAppliedDamage)
				{
					//I hit an actor.
					//Make me explode. Immediately.
					//common->Printf("ITEM '%s' COLLIDE WITH ACTOR '%s'. Vel '%f'. Removing item.\n", this->GetName(), ent->GetName(), v);					
				
					idEntityFx::StartFx(spawnArgs.GetString("fx_death", "fx/explosion_item"), &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
					
                    if (spawnArgs.GetBool("death_gibs", "1"))
                    {
                        //TODO: allow custom gibs.
                        gameLocal.SetDebrisBurst("moveable_metalgibtiny1", this->GetPhysics()->GetOrigin(), 4, 24, 64 + gameLocal.random.RandomInt(64), vec3_zero);
                        gameLocal.SetDebrisBurst("moveable_metalgibtiny2", this->GetPhysics()->GetOrigin(), 4, 24, 64 + gameLocal.random.RandomInt(64), vec3_zero);
                        gameLocal.SetDebrisBurst("moveable_metalgibtiny3", this->GetPhysics()->GetOrigin(), 4, 24, 64 + gameLocal.random.RandomInt(64), vec3_zero);
                    }
				
                    //We want the damage to destroy this object.
                    //this->Damage(attackerEnt, attackerEnt, vec3_zero, "damage_generic", this->health + 1, 0);
					if (fl.takedamage && !dropInvincibleActive)
					{
						fl.takedamage = false;
						this->Killed(attackerEnt, attackerEnt, 0, vec3_zero, 0);
					}



					//If player is the thrower and player can see the item, then do slow mo.
					if (lastThrower.IsValid())
					{
						if (lastThrower.GetEntity() == gameLocal.GetLocalPlayer() && DoesPlayerHaveLOStoMe())
						{
							gameLocal.GetLocalPlayer()->SetImpactSlowmo(true);
						}
					}
				}
				// SW 24th March 2025: changing type comparison here because otherwise it produces odd results when colliding with animated non-actors
				else if (v >= COLLISION_DESTROY_THRESHOLD && ent->IsType(idActor::Type) && this->spawnArgs.GetBool("remove_on_actor", "1") && ent != gameLocal.GetLocalPlayer() && !objectAppliedDamage)
				{
					//so -- item hit an actor, but the item didn't apply any damage. So, we give
					//a little impulse to push the object away from the actor. This is to prevent
					//the object from getting tangled up in the actor model.

					idVec3 velocityNormal = velocity;
					velocityNormal.Normalize();

					// SW 24th March 2025: Do some trace safety checks here to make sure the item doesn't just teleport into the void
					trace_t results;
					idVec3 start = GetPhysics()->GetOrigin();
					idVec3 end = start + velocityNormal * 32;
					gameLocal.clip.TraceBounds(results, start, end, GetPhysics()->GetBounds(), CONTENTS_SOLID, ent);
					GetPhysics()->SetOrigin(results.endpos);
						
					idVec3 pushawayImpulse = velocityNormal * 64 * GetPhysics()->GetMass();
					GetPhysics()->ApplyImpulse(0, GetPhysics()->GetOrigin(), pushawayImpulse);
				}
			}
		}
	}
	else if (health <= 0 && ent && canDealEntDamage)
	{
		if (ent->IsType(idActor::Type) && ent->health > 0)
		{
			//I have no health and hit an actor. Spawn an interestpoint to get their attention.
			gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin(), "interest_gentlebonk");
			gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_toodamaged"), displayName.c_str()), GetPhysics()->GetOrigin());
		}
	}

	// Whatever we hit, the first thing we hit we can't deal ent damage anymore
	canDealEntDamage = false;

	return false;
}
#endif

/*
================
idMoveableItem::Pickup
================
*/
bool idMoveableItem::Pickup( idPlayer *player ) {
	bool ret = idItem::Pickup( player );
	/*if ( ret )
	{
		trigger->SetContents( 0 );
	}*/
	return ret;
}

void idMoveableItem::Present()
{
	idItem::Present();
	if (showItemLine && GetBindMaster() == NULL)
	{
		renderEntity_t itemLine;
		memset(&itemLine, 0, sizeof(itemLine));
		itemLine.origin = this->GetPhysics()->GetOrigin();
		itemLine.axis = mat3_identity;
		itemLine.hModel = renderModelManager->FindModel("models/objects/ui_itemline/itemline.ase");
		itemLine.callback = idItem::ModelCallback;
		itemLine.entityNum = entityNumber;
		itemLine.shaderParms[0] = itemLineColor.x;
		itemLine.shaderParms[1] = itemLineColor.y;
		itemLine.shaderParms[2] = itemLineColor.z;
		if (itemLineHandle == -1)
		{
			itemLineHandle = gameRenderWorld->AddEntityDef(&itemLine);
		}
		else
		{
			gameRenderWorld->UpdateEntityDef(itemLineHandle, &itemLine);
		}

	}
}


/*
================
idMoveableItem::DropItem
================
*/
idEntity *idMoveableItem::DropItem( const char *classname, const idVec3 &origin, const idMat3 &axis, const idVec3 &velocity, int activateDelay, int removeDelay, idEntity *dropper, bool combatThrow)
{
	idDict args;
	idEntity *item;

	args.Set( "classname", classname );
	args.Set( "dropped", "1" );

	// we sometimes drop idMoveables here, so set 'nodrop' to 1 so that it doesn't get put on the floor
	args.Set( "nodrop", "1" );
	args.SetVector("origin", origin);

	if ( activateDelay ) {
		args.SetBool( "triggerFirst", true );
	}

    if (combatThrow)
        args.SetInt("nextdischargetime", 0);

	gameLocal.SpawnEntityDef( args, &item );

	if ( item )
	{
		// set item position
		//item->GetPhysics()->SetOrigin( origin );
		item->GetPhysics()->SetAxis( axis );
		item->GetPhysics()->SetLinearVelocity( velocity );

		//BC spin the dropped item.
		item->GetPhysics()->SetAngularVelocity(gameLocal.GetLocalPlayer()->GetThrowAngularVelocity());

		item->UpdateVisuals();

		if ( activateDelay )
		{
			item->PostEventMS( &EV_Activate, activateDelay, item );
		}

		//BC this is so that thrown things don't clip into the thrower.
        if (dropper != NULL)
        {
            item->GetPhysics()->GetClipModel()->SetOwner(dropper);

			//Reactivate the collision after a while. For non-player, we increase the value a bit so that the item doesn't collide into the thrower immediately.
            item->PostEventMS(&EV_SetOwner, (dropper->IsType(idPlayer::Type)) ? 300 : 2000, 0); //BC after a short time, let the item be pick-uppable to player again.
        }


		if (removeDelay >= 0)
		{
			/*if (!removeDelay)
			{
				removeDelay = 5 * 60 * 1000;
			}*/
			// always remove a dropped item after 5 minutes in case it dropped to an unreachable location
			item->PostEventMS(&EV_Remove, removeDelay);
		}

		if (item->IsType(idItem::Type) && dropper != NULL)
		{
			static_cast<idItem *>(item)->lastThrower = dropper;
		}
	}
	return item;
}



/*
================
idMoveableItem::DropItems

  The entity should have the following key/value pairs set:
	"def_drop<type>Item"			"item def"
	"drop<type>ItemJoint"			"joint name"
	"drop<type>ItemRotation"		"pitch yaw roll"
	"drop<type>ItemOffset"			"x y z"
	"skin_drop<type>"				"skin name"
  To drop multiple items the following key/value pairs can be used:
	"def_drop<type>Item<X>"			"item def"
	"drop<type>Item<X>Joint"		"joint name"
	"drop<type>Item<X>Rotation"		"pitch yaw roll"
	"drop<type>Item<X>Offset"		"x y z"
  where <X> is an aribtrary string.
================
*/

void idMoveableItem::DropItemsBurst(idEntity *ent, const char *type, idVec3 spawnOffset, float speed)
{
	bool airless;
	const idKeyValue *kv;
	idVec3 forward, up, right;
	idVec3 baseOrigin;
	ent->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	baseOrigin = ent->GetPhysics()->GetOrigin() + (forward * spawnOffset.x) + (right * spawnOffset.y) + (up * spawnOffset.z);

	airless = gameLocal.GetAirlessAtPoint(baseOrigin);

	kv = ent->spawnArgs.MatchPrefix(va("def_drop%sItem", type), NULL);
	while (kv)
	{
		idVec3 origin;
		idVec3 spawnOrigin = baseOrigin;		
		idAngles gibAng;
		idEntity *item;

		spawnOrigin.x += -6 + gameLocal.random.RandomInt(12);
		spawnOrigin.y += -6 + gameLocal.random.RandomInt(12);
		spawnOrigin.z += -6 + gameLocal.random.RandomInt(12);

		//TODO: Check if spawning inside a wall.

		gibAng = idAngles(gameLocal.random.RandomInt(360), gameLocal.random.RandomInt(360), 0);

		item = DropItem(kv->GetValue(), spawnOrigin, gibAng.ToMat3(), vec3_zero, 0, 60000, ent, false);
		if (item)
		{
			idVec3 gibVel;
			idVec3 adjustedBaseOrigin = baseOrigin; //BC gibs never are thrown downward. Only throw them upward.

			//gibVel = idVec3(-16 + gameLocal.random.RandomInt(32), -16 + gameLocal.random.RandomInt(32), speed + gameLocal.random.RandomInt(64));

			if (adjustedBaseOrigin.z < spawnOrigin.z)
				adjustedBaseOrigin.z = spawnOrigin.z;

			gibVel = spawnOrigin - adjustedBaseOrigin;
			gibVel.Normalize();
			gibVel *= (speed + gameLocal.random.RandomInt(32));
			
			if (item->IsType(idDebris::Type))
			{
				static_cast<idDebris *>(item)->Launch();
			}

			if (!airless)
			{
				item->GetPhysics()->SetGravity(gameLocal.GetGravity());
			}

			item->GetPhysics()->SetLinearVelocity(gibVel);			
		}

		kv = ent->spawnArgs.MatchPrefix(va("def_drop%sItem", type), kv);
	}
}

void idMoveableItem::DropItems( idAnimatedEntity  *ent, const char *type, idList<idEntity *> *list ) {
	const idKeyValue *kv;
	const char *skinName, *c, *jointName;
	idStr key, key2;
	idVec3 origin;
	idMat3 axis;
	idAngles angles;
	const idDeclSkin *skin;
	jointHandle_t joint;
	idEntity *item;

	// drop all items
	kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sItem", type ), NULL );
	while ( kv ) {

		c = kv->GetKey().c_str() + kv->GetKey().Length();
		if ( idStr::Icmp( c - 5, "Joint" ) != 0 && idStr::Icmp( c - 8, "Rotation" ) != 0 ) {

			key = kv->GetKey().c_str() + 4;
			key2 = key;
			key += "Joint";
			key2 += "Offset";
			jointName = ent->spawnArgs.GetString( key );
			joint = ent->GetAnimator()->GetJointHandle( jointName );
			if ( !ent->GetJointWorldTransform( joint, gameLocal.time, origin, axis ) )
			{
				//gameLocal.Warning( "%s refers to invalid joint '%s' on entity '%s'\n", key.c_str(), jointName, ent->name.c_str() );
				origin = ent->GetPhysics()->GetOrigin();
				axis = ent->GetPhysics()->GetAxis();
			}
			if ( g_dropItemRotation.GetString()[0] ) {
				angles.Zero();
				sscanf( g_dropItemRotation.GetString(), "%f %f %f", &angles.pitch, &angles.yaw, &angles.roll );
			} else {
				key = kv->GetKey().c_str() + 4;
				key += "Rotation";
				ent->spawnArgs.GetAngles( key, "0 0 0", angles );
			}
			axis = angles.ToMat3() * axis;

			origin += ent->spawnArgs.GetVector( key2, "0 0 0" );

			//Item drop when monster dies....
			idVec3 entityCenter = ent->GetPhysics()->GetOrigin() + idVec3(0,0,-16);
			idVec3 itemVelocity = origin - entityCenter;
			itemVelocity.NormalizeFast();
			itemVelocity *= 64 + gameLocal.random.RandomInt(64);
			itemVelocity.z += 128 + gameLocal.random.RandomInt(128); //upward boost.



            if (gameLocal.GetAirlessAtPoint(origin))
            {
                //in zero g. do less item velocity.
                itemVelocity = idVec3(-32 + gameLocal.random.RandomInt(64), -32 + gameLocal.random.RandomInt(64), -8 + gameLocal.random.RandomInt(16));
            }

			item = DropItem( kv->GetValue(), origin, axis, itemVelocity, 0, -1, ent, false );  //BC velocity was originally vec3_origin
			if ( list && item ) {
				list->Append( item );
			}
		}

		kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sItem", type ), kv );
	}

	// change the skin to hide all items
	skinName = ent->spawnArgs.GetString( va( "skin_drop%s", type ) );
	if ( skinName[0] ) {
		skin = declManager->FindSkin( skinName );
		ent->SetSkin( skin );
	}
}

/*
======================
idMoveableItem::WriteToSnapshot
======================
*/
void idMoveableItem::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
}

/*
======================
idMoveableItem::ReadFromSnapshot
======================
*/
void idMoveableItem::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	physicsObj.ReadFromSnapshot( msg );
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}

/*
============
idMoveableItem::Gib
============
*/
void idMoveableItem::Gib( const idVec3 &dir, const char *damageDefName ) {
	// spawn smoke puff
	const char *smokeName = spawnArgs.GetString( "smoke_gib" );
	if ( *smokeName != '\0' ) {
		const idDeclParticle *smoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		gameLocal.smokeParticles->EmitSmoke( smoke, gameLocal.time, gameLocal.random.CRandomFloat(), renderEntity.origin, renderEntity.axis, timeGroup /*_D3XP*/ );
	}
	// remove the entity
	PostEventMS( &EV_Remove, 0 );
}

/*
================
idMoveableItem::Event_DropToFloor
================
*/
void idMoveableItem::Event_DropToFloor( void ) {
	// the physics will drop the moveable to the floor
}

/*
============
idMoveableItem::Event_Gib
============
*/
void idMoveableItem::Event_Gib( const char *damageDefName ) {
	Gib( idVec3( 0, 0, 1 ), damageDefName );
}

/*
============
idMoveableItem::Event_EnableDamage
============
*/
void idMoveableItem::Event_EnableDamage(float enable) {
	fl.takedamage = (enable != 0.0f);
}


void idMoveableItem::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	int damage;
	const idDeclEntityDef *damageDef = gameLocal.FindEntityDef(damageDefName, false);	

	//common->Printf("Damage: name=%s inflictor=%s attacker=%s damagedef=%s %d\n", GetName(), inflictor == nullptr ? "null" : inflictor->GetName(), attacker == nullptr ? "null" : attacker->GetName(), damageDefName, gameLocal.time);

	if (!fl.takedamage)
		return;

	if (dropInvincibleActive)
		return;

	if (!damageDef)
	{
		common->Warning("%s unable to find damagedef %s\n", this->GetName(), damageDefName);
		return;
	}

	//BC check if player is holding onto it.
	if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
	{
		if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
		{
			if (!damageDef->dict.GetBool("durabilitybash"))
			{
				return; //Player is holding onto this item. Don't damage it.
			}
		}
	}

	//if bound to actor, don't take damage.
	//if (GetBindMaster() != NULL && GetBindMaster() != this)
	//{
	//	if (GetBindMaster()->IsType(idActor::Type))
	//	{	//		
	//		return;
	//	}
	//}

	if (damageDef->dict.GetBool("isfire") && !isOnFire)
	{
		Ignite();

		if (dir != vec3_zero)
		{
			//If damage trigger has a Direction, then push the object.
			GetPhysics()->SetLinearVelocity(dir * 256);
		}
	}

	damage = damageDef->dict.GetInt("damage", "0");

	damage = (int)(damage * (float)damageScale);

	if (damage > 0)
	{
		//Inflict damage.
		health -= damage;
	}

	if (health <= 0)
	{
		//Death.
		const char	*fxFireExplodeName;
		const char	*defFireExplode;

		fl.takedamage = false; //Is dead. Stop taking any further damage.

		//Stop fire fx.
		if (fxFire.GetEntity() && isOnFire)
		{
			static_cast<idEntityFx *>(fxFire.GetEntity())->FadeOut();
		}

		//Explosion particles.
		fxFireExplodeName = spawnArgs.GetString("fx_fireexplode"); //I.e. for exploding bullets.

		if (fxFireExplodeName[0] != '\0')
		{
			idEntityFx::StartFx(fxFireExplodeName, &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		}

		//Explosion damage logic.
		defFireExplode = spawnArgs.GetString("def_fireexplode"); //I.e. for exploding bullets.

		DropItemsBurst(this, "gib", idVec3(8, 0, 0), spawnArgs.GetFloat("gibspeed", "0"));

		//Hide();
		physicsObj.SetContents(0);

		if (defFireExplode[0] != '\0')
		{
			gameLocal.RadiusDamage( GetPhysics()->GetOrigin() , this, this, this, this, defFireExplode);
		}

		//Do a default death fx for all moveableitem deaths.

		bool doDeathFX = true;
		if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
		{
			if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
			{
				doDeathFX = false;

				//held item took DURABILITY DAMAGE when player was bashing the item on something.
				//item has been DESTROYED.
			}
		}
		
		if (doDeathFX)
		{
			idEntityFx::StartFx(spawnArgs.GetString("fx_death", "fx/explosion_item"), &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		}


		if (spawnArgs.GetBool("burnaway", "0") && (damageDef->dict.GetBool("isfiredot") || damageDef->dict.GetBool("isfire")))
		{
			const char	*ashName;

			//Burn out of existence.
			renderEntity.noShadow = true;
			renderEntity.shaderParms[SHADERPARM_TIME_OF_DEATH] = gameLocal.time * 0.001f;
			UpdateVisuals();

			ashName = spawnArgs.GetString("fx_ashes", "fx/ashes32"); //I.e. for exploding bullets.

			if (ashName[0] != '\0')
			{
				idEntityFx::StartFx(ashName, NULL, NULL, this, true);
			}

			//Death. Remove from world.
			PostEventMS(&EV_Remove, 2100); //Have a removal delay so that the fxFire effect has enough time to gracefully fade out. If item is removed too quickly, then the fxFire just blips out of existence and looks kinda bad.
		}
		else if (spawnArgs.GetBool("canbreak", "1"))
		{
			//Death. Remove from world.
			Hide();
			PostEventMS(&EV_Remove, 100);
		}

		

		//TODO: make burned objects fade away their particles better.
		//TODO: Make bullets explode and ricochet away.


		Killed(inflictor, attacker, damage, dir, location);
	}
}

void idMoveableItem::SetItemlineActive(bool value)
{
	if (value)
		showItemLine = true;
	else
		showItemLine = false;
}

void idMoveableItem::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
    idEntity::Killed(inflictor, attacker, damage, dir, location);
	
	if (spawnArgs.GetBool("event_deathmsg", "1"))
	{
		gameLocal.AddEventlogDeath(this, damage, inflictor, attacker, "", EL_DESTROYED);
	}

    Hide();
    PostEventMS(&EV_Remove, 0);
}

void idMoveableItem::JustThrown()
{
	// SW 14th April 2025: When throwing a weapon, the weapon moveable exists in a "just spawned" state, which means its dropTimer is at the current game time.
	// While this sometimes helps as an anti-chaos measure, it prevents the weapon from frobbing things or generating interestpoints the way you'd expect it to when thrown.
	// So, to accommodate for this, we cheat the dropTimer somewhat for thrown weapons
	if (spawnArgs.GetBool("isweapon") && dropTimer == gameLocal.time)
	{
		dropTimer = gameLocal.time - max(JUST_SPAWNED_GRACEPERIOD, INTERESTPOINT_DISABLETIME);
	}
}

void idMoveableItem::JustPickedUp()
{
}

bool idMoveableItem::JustBashed(trace_t tr)
{
	return true;
}




bool idMoveableItem::IsOnFire()
{
	return (spawnArgs.GetBool("isfire") || isSparking);
}

void idMoveableItem::SetSparking()
{
	isSparking = true;
	sparkTimer = gameLocal.time + SPARK_TIME;
	if (sparkEmitter != nullptr)
	{
		sparkEmitter->SetActive(true);
		StartSound("snd_bashspark", SND_CHANNEL_WEAPON);

		gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_sparking"), displayName.c_str()), GetPhysics()->GetOrigin());
	}	
}

bool idMoveableItem::GetSparking()
{
	return isSparking;
}



//BC end idmoveableitem


/*
===============================================================================

  idMoveablePDAItem

===============================================================================
*/

CLASS_DECLARATION( idMoveableItem, idMoveablePDAItem )
END_CLASS

/*
================
idMoveablePDAItem::GiveToPlayer
================
*/
bool idMoveablePDAItem::GiveToPlayer(idPlayer *player) {
	const char *str = spawnArgs.GetString( "pda_name" );
	if ( player ) {
		player->GivePDA( str, &spawnArgs );
	}
	return true;
}

/*
===============================================================================

  idItemRemover

===============================================================================
*/

CLASS_DECLARATION( idEntity, idItemRemover )
	EVENT( EV_Activate,		idItemRemover::Event_Trigger )
END_CLASS

/*
================
idItemRemover::Spawn
================
*/
void idItemRemover::Spawn( void ) {
}

/*
================
idItemRemover::RemoveItem
================
*/
void idItemRemover::RemoveItem( idPlayer *player ) {
	const char *remove;

	remove = spawnArgs.GetString( "remove" );
	player->RemoveInventoryItem( remove );
}

/*
================
idItemRemover::Event_Trigger
================
*/
void idItemRemover::Event_Trigger( idEntity *activator ) {
	if ( activator->IsType( idPlayer::Type ) ) {
		RemoveItem( static_cast<idPlayer *>(activator) );
	}
}

/*
===============================================================================

  idObjectiveComplete

===============================================================================
*/

CLASS_DECLARATION( idItemRemover, idObjectiveComplete )
	EVENT( EV_Activate,			idObjectiveComplete::Event_Trigger )
	EVENT( EV_HideObjective,	idObjectiveComplete::Event_HideObjective )
	EVENT( EV_GetPlayerPos,		idObjectiveComplete::Event_GetPlayerPos )
END_CLASS

/*
================
idObjectiveComplete::idObjectiveComplete
================
*/
idObjectiveComplete::idObjectiveComplete() {
	playerPos.Zero();
}

/*
================
idObjectiveComplete::Save
================
*/
void idObjectiveComplete::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( playerPos ); // idVec3 playerPos
}

/*
================
idObjectiveComplete::Restore
================
*/
void idObjectiveComplete::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( playerPos ); // idVec3 playerPos
}

/*
================
idObjectiveComplete::Spawn
================
*/
void idObjectiveComplete::Spawn( void ) {
	spawnArgs.SetBool( "objEnabled", false );
	Hide();
}

/*
================
idObjectiveComplete::Event_Trigger
================
*/
void idObjectiveComplete::Event_Trigger( idEntity *activator ) {
	if ( !spawnArgs.GetBool( "objEnabled" ) ) {
		return;
	}
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		RemoveItem( player );

		if ( spawnArgs.GetString( "inv_objective", NULL ) ) {
			if ( player->hud ) {
				player->hud->SetStateString( "objective", "2");

				player->hud->SetStateString( "objectivetext", spawnArgs.GetString( "objectivetext" ) );
#ifdef _D3XP
				player->hud->SetStateString( "objectivecompletetitle", spawnArgs.GetString( "objectivetitle" ) );
#else
				player->hud->SetStateString( "objectivetitle", spawnArgs.GetString( "objectivetitle" ) );
#endif
				player->CompleteObjective( spawnArgs.GetString( "objectivetitle" ) );
				PostEventMS( &EV_GetPlayerPos, 2000 );
			}
		}
	}
}

/*
================
idObjectiveComplete::Event_GetPlayerPos
================
*/
void idObjectiveComplete::Event_GetPlayerPos() {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		playerPos = player->GetPhysics()->GetOrigin();
		PostEventMS( &EV_HideObjective, 100, player );
	}
}

/*
================
idObjectiveComplete::Event_HideObjective
================
*/
void idObjectiveComplete::Event_HideObjective( idEntity *e ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		idVec3 v = player->GetPhysics()->GetOrigin();
		v -= playerPos;
		if ( v.Length() > 64.0f ) {
			player->hud->HandleNamedEvent( "closeObjective" );
			PostEventMS( &EV_Remove, 0 );
		} else {
			PostEventMS( &EV_HideObjective, 100, player );
		}
	}
}
