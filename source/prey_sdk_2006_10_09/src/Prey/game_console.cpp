// Game_console.cpp
//

// Container class for all world objects that have GUIs on them.  (except monsters & weapons)
// Provides some keys to these guis not provided by default GUI code like "rand"

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#define TRANSLATION_TIME	1000		// Time over which translation takes place

const idEventDef EV_CallGuiEvent("guiEvent", "s");

//==========================================================================
//
//	hhConsole
//
//==========================================================================

CLASS_DECLARATION(idStaticEntity, hhConsole)
	EVENT( EV_Activate,				hhConsole::Event_Activate )
	EVENT( EV_TalonAction,			hhConsole::Event_TalonAction )
	EVENT( EV_CallGuiEvent,			hhConsole::Event_CallGuiEvent )
	EVENT( EV_BecomeNonSolid,		hhConsole::Event_BecomeNonSolid )
	EVENT( EV_PostSpawn,			hhConsole::Event_PostSpawn )
END_CLASS

void hhConsole::Spawn() {
//	BecomeActive(TH_MISC2);	// For ai

	bTimeEventsAlways = spawnArgs.GetBool("timeEventsAlways");
	bUsesRand = spawnArgs.GetBool("usesRand");
	if (bUsesRand || bTimeEventsAlways) {
		BecomeActive(TH_THINK);
	}

	bool bDamageable = !spawnArgs.GetBool("noDamage");
	if (bDamageable) {
		fl.takedamage = true;
	}

	// Stuff color into gui_parms 20,21,22
	idVec3 color;
	GetColor(color);
	SetOnAllGuis("gui_parm20", color[0]);
	SetOnAllGuis("gui_parm21", color[1]);
	SetOnAllGuis("gui_parm22", color[2]);

	// Translation variables
	translationAlpha.Init(gameLocal.time, 0, 0.0f, 0.0f);	// Start at 0 (untranslated)
	transState = TS_UNTRANSLATED;

	// Start idle sound
	StartSound("snd_idle", SND_CHANNEL_ANY, 0, true, NULL);

	UpdateVisuals();

	// Init AI data
	aiCanUse			= spawnArgs.GetBool("ai_can_use");
	aiUseCount			= 0;
	aiReuseWaitTime		= (int)(spawnArgs.GetFloat("ai_reuse_wait")*1000.0f);
	aiUseTime			= (int)(spawnArgs.GetFloat("ai_use_time")*1000.0f);
	aiTriggerWaitTime	= (int)(spawnArgs.GetFloat("ai_use_trigger_wait")*1000.0f);
	aiMaxUses			= spawnArgs.GetInt("ai_max_uses", "-1");
	aiRetriggerWait		= (int)(spawnArgs.GetFloat("ai_use_retrigger_wait")*1000.0f);
	
	// Clamp the trigger wait to be less than the total use time
	if(aiTriggerWaitTime > aiUseTime) {
		aiTriggerWaitTime = aiUseTime - 32;
		if(aiTriggerWaitTime < 0) {
			aiTriggerWaitTime = 0;
		}
	}
	
	aiCurrUsedStartTime	= gameLocal.GetTime();
	aiCurrUsedBy		= NULL;
	aiLastUsedBy		= NULL;	
	aiLastUsedTime		= -aiReuseWaitTime;
	aiWaitingToTrigger	= true;
	aiLastTriggerTime	= gameLocal.GetTime() - 1000;

	perchSpot = NULL;
	PostEventMS( &EV_PostSpawn, 0 );
}

void hhConsole::Event_PostSpawn() {
	// Automatically cause all consoles to spawn Talon perch spots
	if(!spawnArgs.GetBool("noTalonTarget")) {
		idDict	args;
		idVec3	offset;
		bool	bGuiInteractive = false;

		// cjr - Iterate through guis and determine if any are interactive.  If all are not, then Talon should not squawk
		for ( int ix = 0; ix < MAX_RENDERENTITY_GUI; ix++ ) {
			if ( renderEntity.gui[ix] && renderEntity.gui[ix]->IsInteractive() ) {
				bGuiInteractive = true;
			}
		}

		const char *perchDef;
		if ( spawnArgs.GetBool( "noTalonSquawk" ) || !bGuiInteractive) { // CJR:  Check if talon should squawk at this spot
			perchDef = spawnArgs.GetString("def_perch");
		} else {
			perchDef = spawnArgs.GetString("def_perchSquawk"); // Talon should squawk at this spot
		}

		if (!gameLocal.isClient) {
			if (perchDef && perchDef[0]) {
				// Set the offset for the perch location and priority
				offset = GetPhysics()->GetAxis() * spawnArgs.GetVector("offset_perch", "0 0 24");
				float offsetYaw = spawnArgs.GetFloat("offset_perchyaw");
				args.SetVector("origin", GetPhysics()->GetOrigin() + offset);

				idAngles angles = GetAxis().ToAngles();
				angles.yaw += offsetYaw;
				args.SetMatrix("rotation", angles.ToMat3());
				args.Set("target", this->name.c_str());
				perchSpot = (hhTalonTarget *)gameLocal.SpawnObject( perchDef, &args ); // Consoles are automatically high priority spots
				if (perchSpot && ( spawnArgs.GetBool("bindPerchSpot") || IsBound() ) ) {
					perchSpot->Bind(this, true);
				}
			}
			else {
				gameLocal.Warning("Need def_perch key on %s", name.c_str());
				perchSpot = NULL;
			}
		}
	}
}

void hhConsole::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( translationAlpha.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( translationAlpha.GetDuration() );
	savefile->WriteFloat( translationAlpha.GetStartValue() );
	savefile->WriteFloat( translationAlpha.GetEndValue() );

	savefile->Write( &transState, sizeof(transState) );
	savefile->WriteBool( bTimeEventsAlways );
	savefile->WriteBool( bUsesRand );
	savefile->WriteObject( perchSpot );
	savefile->WriteBool( aiCanUse );
	savefile->WriteInt( aiUseCount );
	savefile->WriteInt( aiMaxUses );
	savefile->WriteInt( aiReuseWaitTime );
	savefile->WriteInt( aiUseTime );
	savefile->WriteInt( aiTriggerWaitTime );
	savefile->WriteBool( aiWaitingToTrigger );
	savefile->WriteInt( aiLastTriggerTime );
	savefile->WriteInt( aiRetriggerWait );
	aiCurrUsedBy.Save(savefile);
	savefile->WriteInt( aiCurrUsedStartTime );
	aiLastUsedBy.Save(savefile);
	savefile->WriteInt( aiLastUsedTime );
}

void hhConsole::Restore( idRestoreGame *savefile ) {
	float set;

	savefile->ReadFloat( set );			// idInterpolate<float>
	translationAlpha.SetStartTime( set );
	savefile->ReadFloat( set );
	translationAlpha.SetDuration( set );
	savefile->ReadFloat( set );
	translationAlpha.SetStartValue(set);
	savefile->ReadFloat( set );
	translationAlpha.SetEndValue( set );

	savefile->Read( &transState, sizeof(transState) );
	savefile->ReadBool( bTimeEventsAlways );
	savefile->ReadBool( bUsesRand );
	savefile->ReadObject( reinterpret_cast<idClass *&>(perchSpot) );
	savefile->ReadBool( aiCanUse );
	savefile->ReadInt( aiUseCount );
	savefile->ReadInt( aiMaxUses );
	savefile->ReadInt( aiReuseWaitTime );
	savefile->ReadInt( aiUseTime );
	savefile->ReadInt( aiTriggerWaitTime );
	savefile->ReadBool( aiWaitingToTrigger );
	savefile->ReadInt( aiLastTriggerTime );
	savefile->ReadInt( aiRetriggerWait );

	aiCurrUsedBy.Restore(savefile);
	savefile->ReadInt( aiCurrUsedStartTime );
	aiLastUsedBy.Restore(savefile);
	savefile->ReadInt( aiLastUsedTime );
}

void hhConsole::Event_CallGuiEvent(const char *eventName) {
	CallNamedEvent(eventName);
}

void hhConsole::Event_BecomeNonSolid( void ) {
	GetPhysics()->GetClipModel()->SetContents( 0 );
	GetPhysics()->SetClipMask( 0 );
}

void hhConsole::SetOnAllGuis(const char *key, float value) {
	for (int ix=0; ix<MAX_RENDERENTITY_GUI; ix++) {
		if (renderEntity.gui[ix]) {
			renderEntity.gui[ix]->SetStateFloat(key, value);
			renderEntity.gui[ix]->StateChanged(gameLocal.time);
		}
	}
}

void hhConsole::SetOnAllGuis(const char *key, int value) {
	for (int ix=0; ix<MAX_RENDERENTITY_GUI; ix++) {
		if (renderEntity.gui[ix]) {
			renderEntity.gui[ix]->SetStateInt(key, value);
			renderEntity.gui[ix]->StateChanged(gameLocal.time);
		}
	}
}

void hhConsole::SetOnAllGuis(const char *key, bool value) {
	for (int ix=0; ix<MAX_RENDERENTITY_GUI; ix++) {
		if (renderEntity.gui[ix]) {
			renderEntity.gui[ix]->SetStateBool(key, value);
			renderEntity.gui[ix]->StateChanged(gameLocal.time);
		}
	}
}

void hhConsole::SetOnAllGuis(const char *key, const char *value) {
	for (int ix=0; ix<MAX_RENDERENTITY_GUI; ix++) {
		if (renderEntity.gui[ix]) {
			renderEntity.gui[ix]->SetStateString(key, value);
			renderEntity.gui[ix]->StateChanged(gameLocal.time);
		}
	}
}

void hhConsole::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location) {

	fl.takedamage = false;
	
	// JRM - disable msgs on kill?
	if(spawnArgs.GetBool("ai_damage_disable_msgs","0")) {
		fl.refreshReactions = false;
	}

	// Inform the gui that we are damaged (not used, since gui materials can't see entity parms)
//	SetShaderParm(SHADERPARM_DIVERSITY, gameLocal.random.RandomFloat());
	SetOnAllGuis("damaged", true);

	// Tell the gui that we died, so the gui can be in sync
	CallNamedEvent("Death");
}

bool hhConsole::HandleSingleGuiCommand(idEntity *entityGui, idLexer *src) {
	idToken token;
	if (!src->ReadToken(&token) || token == ";") {
		return false;
	}
	if (token.Icmp("setfont") == 0) {
		if (src->ReadToken(&token)) {
			idStr fontname = "fonts\\";
			fontname += token;
			if (renderEntity.gui[0]) {
				renderEntity.gui[0]->Translate(fontname.c_str());
			}
		}
		return true;
	}

	src->UnreadToken( &token );
	return false;
}

// This is overridden so we can update camera targets reguardless of PVS
void hhConsole::Present( void ) {
	PROFILE_SCOPE("Present", PROFMASK_NORMAL);

	if ( !gameLocal.isNewFrame ) {
		return;
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	// camera target for remote render views
	if ( cameraTarget ) {	// && gameLocal.InPlayerPVS( this ) ) {		// HUMANHEAD pdm: removed PVS check
		renderEntity.remoteRenderView = cameraTarget->GetRenderView();
	}

	// if set to invisible, skip
	if ( !renderEntity.hModel || IsHidden() ) {
		return;
	}

	// add to refresh list
	if ( modelDefHandle == -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}
}

void hhConsole::Think() {

	if (thinkFlags & TH_THINK) {
		if (bUsesRand) {
			float rand = gameLocal.random.RandomFloat();
			SetOnAllGuis("rand", rand);
		}

		if (bTimeEventsAlways) {
			// Call event handler so time events get run
			const char	*cmd;
			sysEvent_t	ev;
			memset( &ev, 0, sizeof( ev ) );
			ev.evType = SE_NONE;

			// This should work after the merge is done, since in the new code, HandleEvent() calls RunTimeEvents()
			for (int ix=0; ix<MAX_RENDERENTITY_GUI; ix++) {
				if (renderEntity.gui[ix]) {
					cmd = renderEntity.gui[ix]->HandleEvent(&ev, gameLocal.time);
					HandleGuiCommands(this, cmd);
				}
			}
		}
	}

	if (thinkFlags & TH_MISC1) {
		// Handle any translation
		if (transState == TS_TRANSLATING || transState == TS_UNTRANSLATING) {
			float alpha = translationAlpha.GetCurrentValue(gameLocal.time);
			if (renderEntity.gui[0] || renderEntity.gui[1] || renderEntity.gui[2]) {

				SetOnAllGuis("translationAlpha", alpha);

				// Determine if the transition is done
				if (translationAlpha.IsDone(gameLocal.time)) {
					if (transState == TS_TRANSLATING) {
						transState = TS_TRANSLATED;
						BecomeInactive(TH_MISC1);
					}
					else if (transState == TS_UNTRANSLATING) {
						transState = TS_UNTRANSLATED;
						BecomeInactive(TH_MISC1);
					}
				}
			}
		}
	}

	// JRM - ai using update
	if (thinkFlags & TH_MISC2) {
		UpdateUse();
	}

	idStaticEntity::Think();
}

// Toggles the translation state of the gui
void hhConsole::Translate(bool bLanded) {

	if (!spawnArgs.GetBool("translate")) {
		return;
	}

	float curalpha = translationAlpha.GetCurrentValue(gameLocal.time);
	switch(transState) {
		case TS_UNTRANSLATED:
		case TS_UNTRANSLATING:
			translationAlpha.Init(gameLocal.time, TRANSLATION_TIME, curalpha, 1.0f);
			transState = TS_TRANSLATING;
			BecomeActive(TH_MISC1);
			break;
		case TS_TRANSLATED:
		case TS_TRANSLATING:
			translationAlpha.Init(gameLocal.time, TRANSLATION_TIME, curalpha, 0.0f);
			transState = TS_UNTRANSLATING;
			BecomeActive(TH_MISC1);
			break;
	}

	if (bLanded) {
		StartSound( "snd_translate", SND_CHANNEL_ANY, 0, true, NULL);
	}
}

void hhConsole::ConsoleActivated() {
	// Called when someone "uses" the gui (gui calls activate cmd)
	// JRM - turn off msgs after the player uses this
	if(!aiCurrUsedBy.IsValid() && spawnArgs.GetBool("ai_player_use_disable_msgs", "0")) {
		fl.refreshReactions = false;
	}
}

void hhConsole::ClearTalonTargetType() { // CJR - Called when any command is sent to a gui.  This disables talon's squawking
	if ( perchSpot ) {	
		perchSpot->SetSquawk( false ); // Set the console to stop Talon from squawking
	}
}

void hhConsole::Event_Activate( idEntity *activator ) {
	// Don't toggle on/off like in idStaticEntity
	// any GUIs still get the onTrigger() event	
}

void hhConsole::Event_TalonAction(idEntity *talon, bool landed) {
	Translate(landed);
}

void hhConsole::Use(idAI *ai) {
	HH_ASSERT(ai);

	// Just started using console?
	if(!aiCurrUsedBy.IsValid()) {
		aiCurrUsedBy				= ai;
		aiCurrUsedStartTime			= gameLocal.GetTime();
		BecomeActive(TH_MISC2);
	}
}

void hhConsole::UpdateUse(void) {

	// Time to deactivate?
	if(aiCurrUsedBy.IsValid() && gameLocal.GetTime() > aiCurrUsedStartTime + aiUseTime) {
		aiLastUsedBy			= aiCurrUsedBy;
		aiLastUsedTime			= gameLocal.GetTime();
		aiCurrUsedBy			= NULL;
		aiWaitingToTrigger		= true;
		aiUseCount++;
		if(aiMaxUses > 0 && aiUseCount >= aiMaxUses) {
			fl.refreshReactions = false;
		}
		BecomeInactive(TH_MISC2);
	}

	// Time to fire triggers?
	if (aiCurrUsedBy.IsValid()) {
		
		// Waiting to trigger...
		if(aiWaitingToTrigger && gameLocal.GetTime() > aiCurrUsedStartTime + aiTriggerWaitTime) {
			aiWaitingToTrigger = false;
			
			OnTriggeredByAI(aiCurrUsedBy.GetEntity());
		}
		// Time to re-trigger? 
		else if (!aiWaitingToTrigger && aiRetriggerWait > 0 && gameLocal.GetTime() - aiLastTriggerTime > aiRetriggerWait) {
			OnTriggeredByAI(aiCurrUsedBy.GetEntity());
		}
	}
}

void hhConsole::OnTriggeredByAI(idAI *ai) {
			
	aiLastTriggerTime = gameLocal.GetTime();

	// Tell the gui that we were used, so the gui can be in sync
	CallNamedEvent("AIUse");
}

bool hhConsole::CanUse(idAI *ai) {
	HH_ASSERT(ai);	

	if (!aiCanUse) {
		return false;
	}

	if (aiMaxUses > 0 && aiUseCount >= aiMaxUses) {
		return false;
	}

	// Can't use it if someone else is using it
	if (aiCurrUsedBy.IsValid() && aiCurrUsedBy.GetEntity() != ai) {
		return false;
	}

	// Have to wait before we can use it again?
	if (gameLocal.GetTime() < aiLastUsedTime + aiReuseWaitTime) {
		return false;
	}

	return ai->spawnArgs.GetBool("CanUseConsole", "0");
}

//==========================================================================
//
//	hhConsoleCountdown
//
//==========================================================================

const idEventDef EV_CountdownStart("startcountdown", NULL);
const idEventDef EV_CountdownStop("stopcountdown", NULL);
const idEventDef EV_CountdownSet("setcountdown", "d");

CLASS_DECLARATION(hhConsole, hhConsoleCountdown)
	EVENT( EV_Activate,				hhConsoleCountdown::Event_Activate )
	EVENT( EV_CountdownStart,		hhConsoleCountdown::Event_StartCountdown )
	EVENT( EV_CountdownStop,		hhConsoleCountdown::Event_StopCountdown )
	EVENT( EV_CountdownSet,			hhConsoleCountdown::Event_SetCountdown )
END_CLASS

void hhConsoleCountdown::Spawn() {
	countingDown = false;
	countStart = spawnArgs.GetFloat("countStart");
	countEnd = spawnArgs.GetFloat("countEnd");
	countdown.Init(gameLocal.time, 0, countStart, countStart);
	SetGuiOctal((int)countStart);

	if (spawnArgs.GetBool("enabled")) {
		StartCountdown();
	}
}

void hhConsoleCountdown::Save(idSaveGame *savefile) const {
	savefile->WriteBool(countingDown);
	savefile->WriteFloat(countStart);
	savefile->WriteFloat(countEnd);

	savefile->WriteFloat( countdown.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( countdown.GetDuration() );
	savefile->WriteFloat( countdown.GetStartValue() );
	savefile->WriteFloat( countdown.GetEndValue() );
}

void hhConsoleCountdown::Restore( idRestoreGame *savefile ) {
	float set;

	savefile->ReadBool(countingDown);
	savefile->ReadFloat(countStart);
	savefile->ReadFloat(countEnd);

	savefile->ReadFloat( set );			// idInterpolate<float>
	countdown.SetStartTime( set );
	savefile->ReadFloat( set );
	countdown.SetDuration( set );
	savefile->ReadFloat( set );
	countdown.SetStartValue(set);
	savefile->ReadFloat( set );
	countdown.SetEndValue( set );
}

void hhConsoleCountdown::SetGuiOctal(int value) {
	// Convert to base-8 numbers and set on gui
	idStr octalValue;
	sprintf(octalValue, "%o", value);
	SetOnAllGuis("countdown", octalValue.c_str());
}

void hhConsoleCountdown::UpdateGUI(float curValue) {
	SetGuiOctal((int)curValue);
	SetOnAllGuis("fraction", curValue / countStart);
	SetOnAllGuis("counting", countingDown);
}

void hhConsoleCountdown::Think() {
	hhConsole::Think();

	if (thinkFlags & TH_MISC3) {
		if (countingDown) {
			UpdateGUI(countdown.GetCurrentValue(gameLocal.time));
			if (countdown.IsDone(gameLocal.time)) {
				BecomeInactive(TH_MISC3);
				ActivateTargets(this);
				StopCountdown();
			}
		}
	}
}

void hhConsoleCountdown::SetCountdown(int count) {
	if (countingDown) {
		gameLocal.Warning("You must stop countdown before set");
		return;
	}
	countStart = count;
	countdown.Init(gameLocal.time, 0, countStart, countStart);
	UpdateGUI(count);
}

void hhConsoleCountdown::StartCountdown() {
	if (!countingDown) {
		// Start counting down from current value
		float curValue = countdown.GetCurrentValue(gameLocal.time);
		// resume from previously stopped count
		countdown.Init(gameLocal.time, SEC2MS(countStart), curValue, countEnd);
		BecomeActive(TH_MISC3);
		countingDown = true;
	}
}

void hhConsoleCountdown::StopCountdown() {
	if (countingDown) {
		// Stop countdown at current value
		float curValue = countdown.GetCurrentValue(gameLocal.time);
		countdown.Init(gameLocal.time, 0, curValue, curValue);
		countingDown = false;
		UpdateGUI(countdown.GetCurrentValue(gameLocal.time));
	}
}

void hhConsoleCountdown::Reset() {
	// Reset to initial start time (or set start time)
	countdown.Init(gameLocal.time, 0, countStart, countStart);
	UpdateGUI(countdown.GetCurrentValue(gameLocal.time));
}

void hhConsoleCountdown::Event_Activate(idEntity *activator) {
	StartCountdown();
}
void hhConsoleCountdown::Event_SetCountdown(int count) {
	SetCountdown(count);
}
void hhConsoleCountdown::Event_StartCountdown() {
	StartCountdown();
}
void hhConsoleCountdown::Event_StopCountdown() {
	StopCountdown();
}


//==========================================================================
//
//	hhConsoleKeypad
//
// Generic console for entering text on a keypad and displaying it
//==========================================================================

CLASS_DECLARATION(hhConsole, hhConsoleKeypad)
END_CLASS

void hhConsoleKeypad::Spawn() {
	maxLength = spawnArgs.GetInt("maxlength");

	SetOnAllGuis("atmax", false);
}

void hhConsoleKeypad::Save(idSaveGame *savefile) const {
	savefile->WriteString(keyBuffer);
	savefile->WriteInt(maxLength);
}

void hhConsoleKeypad::Restore( idRestoreGame *savefile ) {
	savefile->ReadString(keyBuffer);
	savefile->ReadInt(maxLength);
}

void hhConsoleKeypad::UpdateGui() {
	int length = keyBuffer.Length();

	// Enforce maxlength
	if (length > maxLength) {
		keyBuffer = keyBuffer.Left(maxLength);
	}

	SetOnAllGuis("atmax", (length >= maxLength));
	SetOnAllGuis("keypad", keyBuffer.c_str());
}

bool hhConsoleKeypad::HandleSingleGuiCommand(idEntity *entityGui, idLexer *src) {
	idToken token;
	if (!src->ReadToken(&token) || token == ";") {
		return false;
	}
	if (token.Icmp("backspace") == 0) {
		if (keyBuffer.Length() > 0) {
			keyBuffer = keyBuffer.Left(keyBuffer.Length()-1);
		}
	}
	else if (token.Icmp("clear") == 0) {
		keyBuffer.Empty();
	}
	else if (token.Icmp("enter") == 0) {
		keyBuffer += '\n';
	}
	else if (token.IcmpPrefix("keypad_") == 0) {		// KEYPAD_X
		token.ToLower();
		token.Strip("keypad_");
		keyBuffer += token.c_str();
	}
	else {
		return false;
	}

	UpdateGui();
	return true;
}


//==========================================================================
//
//	hhConsoleAlarm
//
// alarm console
//==========================================================================

const idEventDef EV_SpawnMonster("<spawnMonster>");

CLASS_DECLARATION(hhConsole, hhConsoleAlarm)
	EVENT( EV_SpawnMonster,		hhConsoleAlarm::Event_SpawnMonster )
END_CLASS

void hhConsoleAlarm::Spawn() {
	bAlarmActive = false;
	numMonsters = 0;
	bSpawning = false;
	maxMonsters = spawnArgs.GetInt( "max_monsters", "0" );
	BecomeActive(TH_TICKER);
}

void hhConsoleAlarm::Save(idSaveGame *savefile) const {
	savefile->WriteBool(bAlarmActive);
	savefile->WriteInt( numMonsters );
	savefile->WriteInt( maxMonsters );
	currentMonster.Save( savefile );
	savefile->WriteBool( bSpawning );
}

void hhConsoleAlarm::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool(bAlarmActive);
	savefile->ReadInt( numMonsters );
	savefile->ReadInt( maxMonsters );
	currentMonster.Restore( savefile );
	savefile->ReadBool( bSpawning );
}

void hhConsoleAlarm::ConsoleActivated() {
	bAlarmActive = !bAlarmActive;
	if ( !bAlarmActive ) {
		CancelEvents( &EV_SpawnMonster );
		for ( int i=0;i<targets.Num();i++ ) {
			if ( targets[i].IsValid() ) {
				if ( targets[i]->IsType( hhMountedGun::Type ) ) {
					targets[i]->PostEventMS( &EV_Deactivate, 0 );
				}
				targets[i]->DeactivateTargetsType( hhMountedGun::Type );
			}
		}
	}
}

void hhConsoleAlarm::Event_SpawnMonster() {
	idDict args;
	// Copy keys for monster
	idStr tmpStr, realKeyName;
	const idKeyValue *kv = spawnArgs.MatchPrefix("ent_", NULL);
	while(kv) {
		tmpStr = kv->GetKey();
		int usIndex = tmpStr.FindChar("ent_", '_');
		realKeyName = tmpStr.Mid(usIndex+1, strlen(kv->GetKey())-usIndex-1);
		args.Set(realKeyName, kv->GetValue());
		kv = spawnArgs.MatchPrefix("ent_", kv);
	}
	idEntity *ent = gameLocal.FindEntity( spawnArgs.GetString( "spawn_location" ) );
	if ( ent ) {
		args.SetVector( "origin", ent->GetOrigin() );
	} else {
		gameLocal.Warning("No spawn_location specified for %s\n", GetName());
		return;
	} 

	// entity collision checks for seeing if we are going to collide with another entity on spawn
	const idDict *entDef = gameLocal.FindEntityDefDict( spawnArgs.GetString( "def_monster" ), false );
	if ( entDef ) {
		bool clipped = false;
		idVec3 entSize;
		idClipModel *cm;
		idClipModel *clipModels[ MAX_GENTITIES ];
		entDef->GetVector( "size", "0", entSize );
		idBounds bounds = idBounds( ent->GetOrigin() ).Expand( max( entSize.x, entSize.y ) );
		int num = gameLocal.clip.ClipModelsTouchingBounds( bounds, MASK_MONSTERSOLID, clipModels, MAX_GENTITIES );
		if ( ai_debugBrain.GetBool() ) {
			gameRenderWorld->DebugBounds( colorRed, bounds, vec3_origin, 5000 );
		}
		for ( int i=0;i<num;i++ ) {
			cm = clipModels[ i ];
			// don't check render entities
			if ( cm->IsRenderModel() ) {
				continue;
			}
			idEntity *hit = cm->GetEntity();
			if ( ( hit == this ) || !hit->fl.takedamage || !hit->IsType( idActor::Type ) ) {
				continue;
			}
			clipped = true;
			break;
		}
		if ( !clipped ) {
			currentMonster = gameLocal.SpawnObject(spawnArgs.GetString( "def_monster" ), &args);
			if ( currentMonster.IsValid() ) {
				currentMonster->PostEventMS( &EV_Activate, 0, this );
				bSpawning = false;
				numMonsters++;
			} else {
				gameLocal.Warning( "%s could not spawn monster\n", GetName() );
			}
		}
	}
}

void hhConsoleAlarm::Ticker() {

	if (bAlarmActive) {
		if ( currentMonster.IsValid() && currentMonster->GetHealth() <= 0 ) {
			currentMonster.Clear();
		}

		//check on our monster
		if ( !bSpawning && !currentMonster.IsValid() && spawnArgs.GetString( "def_monster" )[0] ) {
			if ( !maxMonsters || numMonsters < maxMonsters ) {
				float min = spawnArgs.GetFloat( "mindelay", "2.0" );
				float max = spawnArgs.GetFloat( "maxdelay", "2.0" );
				if ( max < min ) {
					max = min;
				}
				bSpawning = true;
				PostEventSec( &EV_SpawnMonster, min + (max-min)*gameLocal.random.RandomFloat() );
			}
		}
	}

	hhConsole::Ticker();
}