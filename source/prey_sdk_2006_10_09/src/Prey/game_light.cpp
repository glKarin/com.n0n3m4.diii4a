#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhLight
	
***********************************************************************/

//HUMANHEAD: aob
const idEventDef EV_Light_StartAltMode( "startAltMode" );
//HUMANHEAD END

CLASS_DECLARATION( idLight, hhLight )
	EVENT( EV_PostSpawn,			hhLight::Event_SetTargetHandles )
	EVENT( EV_ResetTargetHandles,	hhLight::Event_SetTargetHandles )
	EVENT( EV_Light_StartAltMode,	hhLight::Event_StartAltMode )
END_CLASS

/*
================
hhLight::SetLightCenter

HUMANHEAD cjr
================
*/
void hhLight::SetLightCenter( idVec3 center ) {
	renderLight.lightCenter = center;
	PresentLightDefChange();
}

/*
================
hhLight::StartAltSound
================
*/
void hhLight::StartAltSound() {
	if ( refSound.shader ) {
		StopSound( SND_CHANNEL_ANY );
		const idSoundShader *alternate = refSound.shader->GetAltSound();
		if ( alternate ) {
			StartSoundShader( alternate, SND_CHANNEL_ANY );
		}
	}
}

/*
================
hhLight::Event_SetTargetHandles

  set the same sound def handle on all targeted entities

HUMANHEAD: aob
================
*/
void hhLight::Event_SetTargetHandles( void ) {
	int i;
	idEntity *targetEnt = NULL;

	if ( !refSound.referenceSound ) {
		return;
	}

	for( i = 0; i < targets.Num(); i++ ) {
		targetEnt = targets[ i ].GetEntity();
		if ( targetEnt ) {
			if( targetEnt->IsType(idLight::Type) ) {
				static_cast<idLight*>(targetEnt)->SetLightParent( this );
			}

			targetEnt->FreeSoundEmitter( true );

			// manually set the refSound to this light's refSound
			targetEnt->GetRenderEntity()->referenceSound = renderEntity.referenceSound;

			// update the renderEntity to the renderer
			targetEnt->UpdateVisuals();
		}
	}
}

/*
================
hhLight::Event_StartAltMode
================
*/
void hhLight::Event_StartAltMode() {
	//Copied fron idLight::BecomeBroken

	// offset the start time of the shader to sync it to the game time
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	// set the state parm
	renderEntity.shaderParms[ SHADERPARM_MODE ] = 1;
	renderLight.shaderParms[ SHADERPARM_MODE ] = 1;

	StartAltSound();
	
	UpdateVisuals();
}