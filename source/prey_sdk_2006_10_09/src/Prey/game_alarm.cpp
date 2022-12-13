#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


//==========================================================================
//
//	hhAlarmLight
//
//==========================================================================
CLASS_DECLARATION(idLight, hhAlarmLight)
	EVENT(EV_Activate,	   		hhAlarmLight::Event_Activate)
END_CLASS

void hhAlarmLight::Spawn() {
	bAlarmOn = false;
	fl.takedamage = false;	// Never take damage

	/* Shader parms set up like this (so the editor shows the light being on):
						parm6	parm7
		editor(on):		0		0
		spawn(off):		1		0
		trigger(on):	1		1
		broken(off):	1		0
	*/
	SetParmState(1.0f, 0.0f);

	// setup the clipModel
	GetPhysics()->SetContents( CONTENTS_SOLID );
}

void hhAlarmLight::Save(idSaveGame *savefile) const {
	savefile->WriteBool( bAlarmOn );
}

void hhAlarmLight::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( bAlarmOn );
}

void hhAlarmLight::TurnOn() {
	if (!bAlarmOn) {
		bAlarmOn = true;

		StartSound("snd_alarm", SND_CHANNEL_BODY2, 0, true, NULL);

		SetParmState(1.0f, 1.0f);
	}
}

void hhAlarmLight::TurnOff() {
	if (bAlarmOn) {
		bAlarmOn = false;
		StopSound(SND_CHANNEL_BODY2, true);

		SetParmState(0.0f, 1.0f);
	}
}

void hhAlarmLight::SetParmState(float value6, float value7) {
	SetShaderParm(6, value6);
	SetShaderParm(7, value7);
	SetLightParm(6, value6);
	SetLightParm(7, value7);
}

void hhAlarmLight::Event_Activate(idEntity *activator) {
	if (bAlarmOn) {
		if (activator && activator->IsType(hhPlayer::Type)) {
			TurnOff();
		}
	}
	else {
		TurnOn();
	}
}
