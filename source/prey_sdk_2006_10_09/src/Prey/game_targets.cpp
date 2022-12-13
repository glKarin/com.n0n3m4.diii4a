/*
  game_targets.cpp

  These entities, when targeted, perform an action

*/


#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_Autosave( "<autosave>" );
const idEventDef EV_FinishedSave( "<finishedSave>" );

/***********************************************************************

  hhTarget_SetSkin

  Set all targets to given skin
***********************************************************************/

CLASS_DECLARATION( idTarget, hhTarget_SetSkin )
	EVENT( EV_Activate,		hhTarget_SetSkin::Event_Trigger )
END_CLASS

/*
================
hhTarget_SetSkin::Spawn
================
*/
void hhTarget_SetSkin::Spawn( void ) {
	spawnArgs.GetString( "skinName", "", skinName );
}


/*
================
 hhTarget_SetSkin::Event_Trigger
================
*/
void hhTarget_SetSkin::Event_Trigger( idEntity *activator ) {
	idEntity *ent = NULL;

	// Set all targets to the specified skin
	for( int t=0; t<targets.Num(); t++ ) {
		ent = targets[t].GetEntity();
		if( !ent ) {
			continue;
		}

		ent->SetSkinByName( (ent->GetSkin()) ? NULL : skinName.c_str() );
	}
}



/***********************************************************************

  hhTarget_Enable

  Enable all targets
***********************************************************************/

CLASS_DECLARATION( idTarget, hhTarget_Enable )
	EVENT( EV_Activate,		hhTarget_Enable::Event_Trigger )
END_CLASS

/*
================
hhTarget_Enable::Spawn
================
*/
void hhTarget_Enable::Spawn( void ) {
}


/*
================
 hhTarget_Enable::Event_Trigger
================
*/
void hhTarget_Enable::Event_Trigger( idEntity *activator ) {

	// Enable all targets
	for (int t=0; t<targets.Num(); t++) {
		idEntity *ent = targets[t].GetEntity();
		if (ent) {
			ent->PostEventMS( &EV_Enable, 0 );
		}
	}
}


/***********************************************************************

  hhTarget_Disable

  Disable all targets
***********************************************************************/

CLASS_DECLARATION( idTarget, hhTarget_Disable )
	EVENT( EV_Activate,		hhTarget_Disable::Event_Trigger )
END_CLASS

/*
================
hhTarget_Disable::Spawn
================
*/
void hhTarget_Disable::Spawn( void ) {
}


/*
================
 hhTarget_Disable::Event_Trigger
================
*/
void hhTarget_Disable::Event_Trigger( idEntity *activator ) {

	// Disable all targets
	for (int t=0; t<targets.Num(); t++) {
		idEntity *ent = targets[t].GetEntity();
		if (ent) {
			ent->PostEventMS( &EV_Disable, 0 );
		}
	}
}


/***********************************************************************

  hhTarget_Earthquake

***********************************************************************/
const idEventDef EV_TurnOff( "<TurnOff>", NULL );

CLASS_DECLARATION( idTarget, hhTarget_Earthquake )
	EVENT( EV_Activate,		hhTarget_Earthquake::Event_Trigger )
	EVENT( EV_TurnOff,		hhTarget_Earthquake::Event_TurnOff )
END_CLASS

/*
================
hhTarget_Earthquake::Spawn
================
*/
void hhTarget_Earthquake::Spawn( void ) {
	//PDMMERGE: Modify to use the sound for the visual shake exclusively
	// and forcefield for the world shake exclusively.
	//This duplicates functionality of func_earthquake.  Check their effect
	//out and see which we want to keep.  If keeping, this could then become func_earthquake.
	//Could alternatively use a sound player and a func_idforcefield

	shakeTime = SEC2MS(spawnArgs.GetFloat("shake_time"));
	shakeAmplitude = spawnArgs.GetFloat("shake_severity");

	forceField.RandomTorque( shakeAmplitude * 40);

	// Grab clip model to use for checking who's currently encroaching me
	cm = new idClipModel( GetPhysics()->GetClipModel() );
	if( !cm ) {
		gameLocal.Error( "hhTarget_Earthquake::Spawn:  Unable to spawn idClipModel\n" );
		return;		
	}
	forceField.SetClipModel( cm );
	// remove the collision model from the physics object
	GetPhysics()->SetClipModel( NULL, 1.0f );
}

void hhTarget_Earthquake::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( shakeTime );
	savefile->WriteFloat( shakeAmplitude );
	savefile->WriteStaticObject( forceField );
	savefile->WriteClipModel( cm );
}

void hhTarget_Earthquake::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( shakeTime );
	savefile->ReadFloat( shakeAmplitude );
	savefile->ReadStaticObject( forceField );
	savefile->ReadClipModel( cm );
}

/*
================
 hhTarget_Earthquake::Think
================
*/
void hhTarget_Earthquake::Think() {
	if (thinkFlags & TH_THINK) {
		forceField.Evaluate( gameLocal.time );
	}
}


/*
================
hhTarget_Earthquake::Event_TurnOff
================
*/
void hhTarget_Earthquake::Event_TurnOff( void ) {
	hhPlayer *player;
	BecomeInactive(TH_THINK);
	StopSound(SND_CHANNEL_BODY);

	// Turn camera interpolator back on
	for (int i = 0; i < gameLocal.numClients; i++ ) {
		player = static_cast<hhPlayer*>(gameLocal.entities[ i ]);
		if ( player ) {
			player->cameraInterpolator.SetInterpolationType( IT_VariableMidPointSinusoidal );
		}
	}
}

/*
================
 hhTarget_Earthquake::Event_Trigger
================
*/
void hhTarget_Earthquake::Event_Trigger( idEntity *activator ) {
	hhPlayer *player;

	if( !cm ) { // No collision model
		return;
	}

	StartSound("snd_quake", SND_CHANNEL_BODY);

	idBounds bounds;
	bounds.FromTransformedBounds( cm->GetBounds(), cm->GetOrigin(), cm->GetAxis() );
	//gameRenderWorld->DebugBounds(colorRed, bounds, vec3_origin, 5000);

	for (int i = 0; i < gameLocal.numClients; i++ ) {
		player = static_cast<hhPlayer*>(gameLocal.entities[ i ]);
		if ( !player || ( player->fl.notarget ) ) {
			continue;
		}

		if( bounds.IntersectsBounds(player->GetPhysics()->GetAbsBounds()) ) {
			player->cameraInterpolator.SetInterpolationType(IT_None);
		}
	}

	// Turn on physics
	BecomeActive( TH_THINK );
	PostEventSec( &EV_TurnOff, MS2SEC(shakeTime) );
}


/***********************************************************************

  hhTarget_SetLightParm

***********************************************************************/

CLASS_DECLARATION( idTarget, hhTarget_SetLightParm )
	EVENT( EV_Activate,	hhTarget_SetLightParm::Event_Activate )
END_CLASS

/*
================
hhTarget_SetLightParm::Event_Activate
================
*/
void hhTarget_SetLightParm::Event_Activate( idEntity *activator ) {
	int			i;
	idEntity *	ent;
	float		value;
	idVec3		color;
	int			parmnum;

	// set the color on the targets
	if ( spawnArgs.GetVector( "_color", "1 1 1", color ) ) {
		for( i = 0; i < targets.Num(); i++ ) {
			ent = targets[ i ].GetEntity();
			if ( ent ) {
				ent->SetColor( color[ 0 ], color[ 1 ], color[ 2 ] );
			}
		}
	}

	// set any shader parms on the targets
	for( parmnum = 0; parmnum < MAX_ENTITY_SHADER_PARMS; parmnum++ ) {
		if ( spawnArgs.GetFloat( va( "shaderParm%d", parmnum ), "0", value ) ) {
			for( i = 0; i < targets.Num(); i++ ) {
				ent = targets[ i ].GetEntity();
				if ( ent && ent->IsType(idLight::Type) ) {
					static_cast<idLight*>(ent)->SetLightParm( parmnum, value );
				}
			}
			if (spawnArgs.GetBool("toggle") && (value == 0 || value == 1)) {
				int val = value;
				val ^= 1;
				value = val;
				spawnArgs.SetFloat(va("shaderParm%d", parmnum), value);
			}
		}
	}
}

/***********************************************************************

  hhTarget_PlayWeaponAnim

***********************************************************************/

CLASS_DECLARATION( idTarget, hhTarget_PlayWeaponAnim )
	EVENT( EV_Activate,		hhTarget_PlayWeaponAnim::Event_Trigger )
END_CLASS

/*
================
 hhTarget_PlayWeaponAnim::Event_Trigger
================
*/
void hhTarget_PlayWeaponAnim::Event_Trigger( idEntity *activator ) {
	hhPlayer* pPlayer = NULL;

	if( activator && activator->IsType(hhPlayer::Type) ) {
		pPlayer = static_cast<hhPlayer*>( activator );
		if( pPlayer ) {
			pPlayer->ProcessEvent( &EV_PlayWeaponAnim, spawnArgs.GetString("anim", "initialPickup"), spawnArgs.GetInt("numTries") );
		}
	}
}


//==========================================================================
//
//	hhTarget_ControlVehicle
//
//==========================================================================

CLASS_DECLARATION(idTarget, hhTarget_ControlVehicle)
	EVENT( EV_Activate,		hhTarget_ControlVehicle::Event_Activate )
END_CLASS


void hhTarget_ControlVehicle::Spawn() {
}

void hhTarget_ControlVehicle::Event_Activate( idEntity *activator ) {
	hhVehicle *vehicle = NULL;

	if (activator->IsType(hhPlayer::Type)) {
		hhPlayer *player = static_cast<hhPlayer*>(activator);

		// Search target list to find vehicle
		int numTargets = targets.Num();
		for (int ix=0; ix<numTargets; ix++) {
			if (targets[ix].IsValid()) {
				if (targets[ix].GetEntity()->IsType(hhVehicle::Type)) {
					vehicle = static_cast<hhVehicle*>(targets[ix].GetEntity());
				}
			}
		}

		if( vehicle ) {
			player->EnterVehicle( vehicle );
		}
	}
}


//==========================================================================
//
//	hhTarget_AttachToRail
//
//	OBSOLETE, just target the hhRailRide directly
//==========================================================================

CLASS_DECLARATION(idTarget, hhTarget_AttachToRail)
	EVENT( EV_Activate,		hhTarget_AttachToRail::Event_Activate )
END_CLASS


void hhTarget_AttachToRail::Spawn() {
}

// Bind player to given bone of a targeted hhAnimator
void hhTarget_AttachToRail::Event_Activate( idEntity *activator ) {
	hhRailRide *rail = NULL;

	if (activator && activator->IsType(hhPlayer::Type)) {
		hhPlayer *player = static_cast<hhPlayer*>(activator);

		// Search target list to find hhRailController
		int numTargets = targets.Num();
		for (int ix=0; ix<numTargets; ix++) {
			if (targets[ix].IsValid()) {
				if (targets[ix].GetEntity()->IsType(hhRailRide::Type)) {
					rail = static_cast<hhRailRide*>(targets[ix].GetEntity());
				}
			}
		}

		if (rail) {
			rail->Attach(player, false);
		}
	}
}

//==========================================================================
//
//	hhTarget_EnableReactions
//
//==========================================================================
CLASS_DECLARATION( idTarget, hhTarget_EnableReactions )
	EVENT( EV_Activate,		hhTarget_EnableReactions::Event_Activate )
END_CLASS

void hhTarget_EnableReactions::Spawn() {
}

void hhTarget_EnableReactions::Event_Activate(idEntity *activator) {
	// Enable reactions on all targets
	for (int t=0; t<targets.Num(); t++) {
		idEntity *ent = targets[t].GetEntity();
		if (ent) {
			ent->fl.refreshReactions = true;
		}
	}
}

//==========================================================================
//
//	hhTarget_DisableReactions
//
//==========================================================================
CLASS_DECLARATION( idTarget, hhTarget_DisableReactions )
	EVENT( EV_Activate,		hhTarget_DisableReactions::Event_Activate )
END_CLASS

void hhTarget_DisableReactions::Spawn() {
}

void hhTarget_DisableReactions::Event_Activate(idEntity *activator) {
	// Disable reactions on all targets
	for (int t=0; t<targets.Num(); t++) {
		idEntity *ent = targets[t].GetEntity();
		if (ent) {
			ent->fl.refreshReactions = false;
		}
	}
}

//==========================================================================
//
//	hhTarget_EnablePassageway
//
//==========================================================================
CLASS_DECLARATION( idTarget, hhTarget_EnablePassageway )
	EVENT( EV_Activate,		hhTarget_EnablePassageway::Event_Activate )
END_CLASS

void hhTarget_EnablePassageway::Spawn() {
}

void hhTarget_EnablePassageway::Event_Activate(idEntity *activator) {
	// Enable reactions on all targets
	for (int t=0; t<targets.Num(); t++) {
		idEntity *ent = targets[t].GetEntity();
		if (ent && ent->IsType(hhAIPassageway::Type) ) {
			static_cast<hhAIPassageway*>(ent)->SetEnablePassageway(TRUE);
		}
	}
}

//==========================================================================
//
//	hhTarget_DisablePassageway
//
//==========================================================================
CLASS_DECLARATION( idTarget, hhTarget_DisablePassageway )
	EVENT( EV_Activate,		hhTarget_DisablePassageway::Event_Activate )
END_CLASS

void hhTarget_DisablePassageway::Spawn() {
}

void hhTarget_DisablePassageway::Event_Activate(idEntity *activator) {
	// Enable reactions on all targets
	for (int t=0; t<targets.Num(); t++) {
		idEntity *ent = targets[t].GetEntity();
		if (ent && ent->IsType(hhAIPassageway::Type)) {
			static_cast<hhAIPassageway*>(ent)->SetEnablePassageway(FALSE);
		}
	}
}


const idEventDef EV_TriggerTargets( "<triggertargets>" );

//==========================================================================
//
//	hhTarget_PatternRelay
//
//	When triggered, relays trigger messages in a pattern
//==========================================================================
CLASS_DECLARATION( idTarget, hhTarget_PatternRelay )
	EVENT( EV_Activate,			hhTarget_PatternRelay::Event_Activate )
	EVENT( EV_TriggerTargets,	hhTarget_PatternRelay::Event_TriggerTargets )
END_CLASS

void hhTarget_PatternRelay::Spawn() {
	timeGranularity = spawnArgs.GetFloat("timestep", "1");
}

void hhTarget_PatternRelay::Event_Activate(idEntity *activator) {
	idStr pattern = spawnArgs.GetString("pattern", "xoxoxo");

	//TODO: Clear out any from last time around?

	float issueTime = 0.0f;
	int length = pattern.Length();
	for (int ix=0; ix<length; ix++) {
		if (pattern[ix] == 'x') {
			PostEventSec(&EV_TriggerTargets, issueTime);
		}
		else if (pattern[ix] == 'o') {
		}
		else {
			gameLocal.Warning("unrecognized pattern character: %c", pattern[ix]);
			break;
		}
		issueTime += timeGranularity;
	}
}

void hhTarget_PatternRelay::Event_TriggerTargets() {
	ActivateTargets(this);
}


//==========================================================================
//
//	hhTarget_Subtitle
//
//	When triggered, displays subtitle text on the HUD
//==========================================================================
CLASS_DECLARATION( idTarget, hhTarget_Subtitle )
	EVENT( EV_Activate,			hhTarget_Subtitle::Event_Activate )
	EVENT( EV_TurnOff,			hhTarget_Subtitle::Event_FadeOutText )
END_CLASS

void hhTarget_Subtitle::Spawn() {
}

void hhTarget_Subtitle::Event_Activate(idEntity *activator) {
	int x = spawnArgs.GetInt("xpos");
	int y = spawnArgs.GetInt("ypos");
	bool bCentered = spawnArgs.GetBool("centered");
	const char *text = spawnArgs.GetString("text_subtitle");
	float duration = spawnArgs.GetFloat("duration");

	idPlayer *player = gameLocal.GetLocalPlayer();
	if (!player) { //rww
		return;
	}
	player->hud->SetStateInt("subtitlex", bCentered ? 0 : x);
	player->hud->SetStateInt("subtitley", y);
	player->hud->SetStateInt("subtitlecentered", bCentered);
	player->hud->SetStateString("subtitletext", text);
	player->hud->StateChanged(gameLocal.time);
	player->hud->HandleNamedEvent("DisplaySubtitle");

	CancelEvents(&EV_TurnOff);
	PostEventSec(&EV_TurnOff, duration);
}

void hhTarget_Subtitle::Event_FadeOutText() {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if (player && player->hud) {
		player->hud->HandleNamedEvent("RemoveSubtitle");
	}
}


//==========================================================================
//
//	hhTarget_EndLevel
//
//	When triggered, does a level transition
//==========================================================================
CLASS_DECLARATION( idTarget_EndLevel, hhTarget_EndLevel )
	EVENT( EV_Activate,			hhTarget_EndLevel::Event_Activate )
END_CLASS

void hhTarget_EndLevel::Event_Activate(idEntity *activator) {
	idUserInterface *guiLoading = NULL;
	idStr nextMap;
	if ( spawnArgs.GetString( "nextMap", "", nextMap ) ) {

		// Session code hasn't yet loaded the gui for next level here, so we preload it based on the same
		// logic as it uses in idSessionLocal::LoadLoadingGui().
		idStr stripped = nextMap;
		stripped.StripFileExtension();
		stripped.StripPath();
		idStr guiMap = va( "guis/map/%s.gui", stripped.c_str() );
		if (uiManager->CheckGui( guiMap ) ) {
			guiLoading = uiManager->FindGui( guiMap, true, false, true );
		} else {
			guiLoading = uiManager->FindGui( "guis/map/loading.gui", true, false, true );
		}
		guiLoading->SetStateFloat("map_loading", 0.0f);
		guiLoading->SetStateBool("showddainfo", true);

		const idDecl *mapDecl = declManager->FindType(DECL_MAPDEF, stripped.c_str(), false );
		if ( mapDecl ) {
			const idDeclEntityDef *mapInfo = static_cast<const idDeclEntityDef *>(mapDecl);
			const char *friendlyName = mapInfo->dict.GetString("name");
			guiLoading->SetStateString("friendlyname", friendlyName);
		}

		guiLoading->StateChanged(gameLocal.time);

		// HUMANHEAD CJR:  If the player hits this and they are spiritwalking, stop the spiritwalk before loading the next level
		if ( activator->IsType( hhPlayer::Type ) ) {
			hhPlayer *player = static_cast<hhPlayer *>( activator );
			if ( player ) {

				// Player managed kill themselves at the end of level?  Clear deathwalk flags
				if (player->IsDeathWalking()) {
					player->bDeathWalk = false;
					player->SetHealth( 50.0f ); // Don't say I never got you anything
					player->SetSpiritPower( 0.0f ); // Clear spirit power
				}

				if ( player->IsSpiritWalking() ) {
					player->PutawayEtherealWeapon(); // Don't do a full spiritstop, as that teleports the player, too.  Just fix the player's weapon
				}
			}
		}
		// END CJR
	}

	idTarget_EndLevel::Event_Activate(activator);
}


//==========================================================================
//
//	hhTarget_ConsolidatePlayers
//
//	When triggered, consolidates all players into a single view (coop)
//==========================================================================
CLASS_DECLARATION( idTarget, hhTarget_ConsolidatePlayers )
	EVENT( EV_Activate,			hhTarget_ConsolidatePlayers::Event_Activate )
END_CLASS

void hhTarget_ConsolidatePlayers::Event_Activate(idEntity *activator) {
	if (gameLocal.IsCooperative()) {
		//TODO: Implement
	}
}


//==========================================================================
//
//	hhTarget_WarpPlayers
//
//	When triggered, teleports all players to targeted start spots (coop)
//==========================================================================
CLASS_DECLARATION( idTarget, hhTarget_WarpPlayers )
	EVENT( EV_Activate,			hhTarget_WarpPlayers::Event_Activate )
END_CLASS

void hhTarget_WarpPlayers::Event_Activate(idEntity *activator) {
	if (gameLocal.IsCooperative()) {
		//TODO: Implement
	}
}

//==========================================================================
//
//	hhTarget_FollowPath
//
//	When triggered, make an ai follow a path
//==========================================================================
CLASS_DECLARATION( idTarget, hhTarget_FollowPath )
	EVENT( EV_Activate,			hhTarget_FollowPath::Event_Activate )
END_CLASS

void hhTarget_FollowPath::Event_Activate(idEntity *activator) {
	int					i;
	idEntity			*ent;
	const function_t	*func;
	const char			*funcName;
	idThread			*thread;
	hhMonsterAI			*entAI;
	
	funcName = spawnArgs.GetString( "call" );
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent && ent->scriptObject.HasObject() ) {
			func = ent->scriptObject.GetFunction( funcName );
			if ( !func ) {
				gameLocal.Error( "Function '%s' not found on entity '%s' for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}
			if ( func->type->NumParameters() != 1 ) {
				gameLocal.Error( "Function '%s' on entity '%s' has the wrong number of parameters for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}
			if ( !ent->scriptObject.GetTypeDef()->Inherits( func->type->GetParmType( 0 ) ) ) {
				gameLocal.Error( "Function '%s' on entity '%s' is the wrong type for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}
			entAI = static_cast<hhMonsterAI*>(ent);
			if ( entAI && entAI->AI_FOLLOWING_PATH ) {
				gameLocal.Warning( "hhTarget_FollowPath %s already following path", ent->name.c_str() );
				continue;
			}
			ent->spawnArgs.Set( "alt_path", spawnArgs.GetString( "alt_path" ) );
			// create a thread and call the function
			thread = new idThread();
			thread->CallFunction( ent, func, true );
			thread->Start();
		}
	}
}

//===============================================================================
//
// hhTarget_LockDoor
//
//===============================================================================

CLASS_DECLARATION( idTarget, hhTarget_LockDoor )
	EVENT( EV_Activate,	hhTarget_LockDoor::Event_Activate )
END_CLASS

void hhTarget_LockDoor::Event_Activate( idEntity *activator ) {
	int i;
	idEntity *ent;
	int lock;

	lock = spawnArgs.GetInt( "locked", "1" );
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent && ent->IsType( idDoor::Type ) ) {
			if ( static_cast<idDoor *>( ent )->IsLocked() ) {
				static_cast<idDoor *>( ent )->Lock( 0 );
			} else {
				static_cast<idDoor *>( ent )->Lock( lock );
			}
		}
		if ( ent && ent->IsType( hhModelDoor::Type ) ) {
			if ( static_cast<hhModelDoor *>( ent )->IsLocked() ) {
				static_cast<hhModelDoor *>( ent )->Lock( 0 );
			} else {
				static_cast<hhModelDoor *>( ent )->Lock( lock );
			}
		}
		if ( ent && ent->IsType( hhProxDoor::Type ) ) {
			if ( static_cast<hhProxDoor *>( ent )->IsLocked() ) {
				static_cast<hhProxDoor *>( ent )->Lock( 0 );
			} else {
				static_cast<hhProxDoor *>( ent )->Lock( lock );
			}
		}
	}
}

//==========================================================================
//
//	hhTarget_DisplayGui
//
//	When triggered, displays full screen gui
//==========================================================================
CLASS_DECLARATION( idTarget, hhTarget_DisplayGui )
	EVENT( EV_Activate,			hhTarget_DisplayGui::Event_Activate )
END_CLASS

void hhTarget_DisplayGui::Event_Activate(idEntity *activator) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if (player && player->IsType(hhPlayer::Type)) {
		static_cast<hhPlayer*>(player)->SetOverlayGui( spawnArgs.GetString("gui_overlay") );
	}
}


//==========================================================================
//
//	hhTarget_Autosave
//
//	When triggered, autosaves
//==========================================================================
CLASS_DECLARATION( idTarget, hhTarget_Autosave )
	EVENT( EV_Activate,			hhTarget_Autosave::Event_Activate )
END_CLASS

void hhTarget_Autosave::Event_Activate(idEntity *activator) {
	const char *desc = spawnArgs.GetString( "text_savename" );
	if ( !desc || desc[0] == 0 ) {
		gameLocal.Warning( "hhTarget_Autosave has no text_savename key" );
		desc = "";
	}

	idStr saveName = va( common->GetLanguageDict()->GetString( "#str_00838" ), desc );
	//HUMANHEAD PCF mdl 05/05/06 - Changed ' to " to avoid problems when saves contained '
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "savegame \"%s\"", saveName.c_str() ) );
}


//==========================================================================
//
//	hhTarget_Show
//
//==========================================================================

CLASS_DECLARATION( idTarget, hhTarget_Show )
	EVENT( EV_Activate, hhTarget_Show::Event_Activate )
END_CLASS

void hhTarget_Show::Event_Activate( idEntity *activator ) {
	int			i;
	idEntity	*ent;

	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent ) {
			ent->Show();
		}
	}
}

//==========================================================================
//
//	hhTarget_Hide
//
//==========================================================================

CLASS_DECLARATION( idTarget, hhTarget_Hide )
	EVENT( EV_Activate, hhTarget_Hide::Event_Activate )
END_CLASS

void hhTarget_Hide::Event_Activate( idEntity *activator ) {
	int			i;
	idEntity	*ent;

	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent ) {
			ent->Hide();
		}
	}
}

