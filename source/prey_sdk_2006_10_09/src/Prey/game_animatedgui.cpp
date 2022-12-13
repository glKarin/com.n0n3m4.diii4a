//**************************************************************************
//**
//** hhAnimatedGui
//**
//**************************************************************************

//TODO: Could impliment a queue for open/close events, so we can wait until
// fully open before closing
// -or-
// could make animations blend out of current and into the new
// requires dynamic control of animation weights


// HEADER FILES ------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const float AG_SMALL_SCALE = 0.01f;
const float AG_LARGE_SCALE = 1.0f;

CLASS_DECLARATION( hhAnimatedEntity, hhAnimatedGui )
	EVENT(EV_PlayIdle,		hhAnimatedGui::Event_PlayIdle)
	EVENT(EV_Activate,	   	hhAnimatedGui::Event_Trigger)
END_CLASS


//==========================================================================
//
// hhAnimatedGui::Spawn
//
//==========================================================================

void hhAnimatedGui::Spawn(void) {
	idDict args;

	GetPhysics()->SetContents( CONTENTS_BODY );

	idleOpenAnim		= GetAnimator()->GetAnim("idleopen");
	idleCloseAnim		= GetAnimator()->GetAnim("idleclose");
	openAnim			= GetAnimator()->GetAnim("open");
	closeAnim			= GetAnimator()->GetAnim("close");

	bOpen = false;
	guiScale.Init(gameLocal.time, 0, AG_SMALL_SCALE, AG_SMALL_SCALE);

	// Spawn and bind the console on
	const char *consoleName = spawnArgs.GetString("def_gui");
	if (consoleName && *consoleName) {
		args.Clear();
		args.Set("gui", spawnArgs.GetString("gui_topass"));
		args.Set("origin", GetOrigin().ToString());	// need the joint position
		args.Set("rotation", GetAxis().ToString());
		attachedConsole = gameLocal.SpawnObject(consoleName, &args);
		assert(attachedConsole);
		attachedConsole->SetOrigin(GetOrigin() + GetAxis()[0]*10);
		attachedConsole->Bind(this, true);
		attachedConsole->Hide();
	}

	// Spawn the trigger
	const char *triggerName = spawnArgs.GetString("def_trigger");
	if (triggerName && *triggerName) {
		args.Clear();
		args.Set( "target", name.c_str() );
		args.Set( "mins", spawnArgs.GetString("triggerMins") );
		args.Set( "maxs", spawnArgs.GetString("triggerMaxs") );
		args.Set( "bind", name.c_str() );
		args.SetVector( "origin", GetOrigin() );
		args.SetMatrix( "rotation", GetAxis() );
		idEntity *trigger = gameLocal.SpawnObject( triggerName, &args );
	}

	if (idleOpenAnim && idleCloseAnim) {
		PostEventMS(&EV_PlayIdle, 0);
	}
}

hhAnimatedGui::hhAnimatedGui() {
	attachedConsole = NULL;
}

hhAnimatedGui::~hhAnimatedGui() {
}

void hhAnimatedGui::Save(idSaveGame *savefile) const {
	savefile->WriteBool( bOpen );
	savefile->WriteInt( idleOpenAnim );
	savefile->WriteInt( idleCloseAnim );
	savefile->WriteInt( openAnim );
	savefile->WriteInt( closeAnim );

	savefile->WriteFloat( guiScale.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( guiScale.GetDuration() );
	savefile->WriteFloat( guiScale.GetStartValue() );
	savefile->WriteFloat( guiScale.GetEndValue() );

	savefile->WriteObject( attachedConsole );
}

void hhAnimatedGui::Restore( idRestoreGame *savefile ) {
	float set;

	savefile->ReadBool( bOpen );
	savefile->ReadInt( idleOpenAnim );
	savefile->ReadInt( idleCloseAnim );
	savefile->ReadInt( openAnim );
	savefile->ReadInt( closeAnim );

	savefile->ReadFloat( set );			// idInterpolate<float>
	guiScale.SetStartTime( set );
	savefile->ReadFloat( set );
	guiScale.SetDuration( set );
	savefile->ReadFloat( set );
	guiScale.SetStartValue(set);
	savefile->ReadFloat( set );
	guiScale.SetEndValue( set );

	savefile->ReadObject( reinterpret_cast<idClass *&>( attachedConsole ) );
}

//==========================================================================
//
// hhAnimatedGui::Think
//
//==========================================================================
void hhAnimatedGui::Think( void ) {
	hhAnimatedEntity::Think();

	if (thinkFlags & TH_THINK) {
		float curScale = guiScale.GetCurrentValue(gameLocal.time);
		attachedConsole->SetDeformation(DEFORMTYPE_SCALE, curScale);
		if (guiScale.IsDone(gameLocal.time)) {
			if (curScale == AG_SMALL_SCALE) {	// Just finished scaling down
				attachedConsole->Hide();
			}
			BecomeInactive(TH_THINK);
		}
	}
}

//==========================================================================
//
// hhAnimatedGui::Event_PlayIdle
//
//==========================================================================
void hhAnimatedGui::Event_PlayIdle() {

	GetAnimator()->ClearAllAnims(gameLocal.time, 0);

	if (bOpen) {
		GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, idleOpenAnim, gameLocal.time, 0);

		FadeInGui();
	}
	else {
		GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, idleCloseAnim, gameLocal.time, 0);
	}
}


//==========================================================================
//
// hhAnimatedGui::Event_Trigger
//
//==========================================================================

void hhAnimatedGui::Event_Trigger( idEntity *activator ) {

	if (!openAnim || !closeAnim) {
		return;
	}

	GetAnimator()->ClearAllAnims(gameLocal.time, 0);

	int ms;
	if(bOpen) {
		GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, closeAnim, gameLocal.time, 0);
		ms = GetAnimator()->GetAnim( closeAnim )->Length();
		bOpen = false;

		FadeOutGui();
	}
	else {
		GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, openAnim, gameLocal.time, 0);
		ms = GetAnimator()->GetAnim( openAnim )->Length();
		bOpen = true;
	}

	PostEventMS(&EV_PlayIdle, ms);
}

void hhAnimatedGui::FadeInGui() {
	float curScale = guiScale.GetCurrentValue(gameLocal.time);
	guiScale.Init(gameLocal.time, 500, curScale, AG_LARGE_SCALE);
	BecomeActive(TH_THINK);
	attachedConsole->SetDeformation(DEFORMTYPE_SCALE, curScale);
	attachedConsole->Show();
}

void hhAnimatedGui::FadeOutGui() {
	float curScale = guiScale.GetCurrentValue(gameLocal.time);
	guiScale.Init(gameLocal.time, 100, curScale, AG_SMALL_SCALE);
	BecomeActive(TH_THINK);
	attachedConsole->Show();
}
