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
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_meta.h"
#include "SecurityCamera.h"

/***********************************************************************

  idSecurityCamera

  Security camera that triggers targets when player is in view

***********************************************************************/



//BC securitycamera has largely been re-written to use a state machine.


const int SUSPICION_INTERVALTIME = 200;

const idVec3 CAMERA_DEFAULTCOLOR = idVec3(.9f, .9f, .9f);
const idVec3 CAMERA_FRIENDLYCOLOR = idVec3(0, .1f, 1);
const idVec3 HEADLIGHT_DEFAULTCOLOR = idVec3(0.4, 0, 0);
const idVec3 HEADLIGHT_FRIENDLYCOLOR = idVec3(0, 0, 0.4f);

const int ENEMYCHECKFREQUENCY = 500;

const int SPARKINTERVAL = 400;



const idEventDef EV_SecurityCam_AddLight( "<addLight>" );

CLASS_DECLARATION( idEntity, idSecurityCamera )
	EVENT( EV_SecurityCam_AddLight,			idSecurityCamera::Event_AddLight )
END_CLASS

int idSecurityCamera::cameraCount = 0;

idSecurityCamera::idSecurityCamera(void)
{
	cameraIndex = -1;
	aimAssistNode.SetOwner(this);
	aimAssistNode.AddToEnd(gameLocal.aimAssistEntities);

	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	damageSparkTimer = 0;

	cameraID = cameraCount;
	cameraCount++;

	securitycameraNode.SetOwner(this);
	securitycameraNode.AddToEnd(gameLocal.securitycameraEntities);

	memset( boundingBeam, 0 ,sizeof(idBeam*)*CAMERA_SPOKECOUNT );
	memset( boundingBeamTarget, 0 ,sizeof(idBeam*)*CAMERA_SPOKECOUNT );

	spotLight = nullptr;
}

idSecurityCamera::~idSecurityCamera(void)
{
	aimAssistNode.Remove();
	securitycameraNode.Remove();
	idPlayer* player = gameLocal.GetLocalPlayer();
	// SM: If we were the active camera splice target, have to change to any other target
	if ( player && player->IsCameraSpliceActive( this ) ) {
		player->StartCameraSplice( NULL );
	}

	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
		headlightHandle = -1;
	}

	cameraCount--;
}

/*
================
idSecurityCamera::Save
================
*/
void idSecurityCamera::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( IsSpliced ); // bool IsSpliced
	savefile->WriteInt( cameraIndex ); // int cameraIndex

	savefile->WriteInt( state ); // int state
	savefile->WriteInt( sweepAngle1 ); // int sweepAngle1
	savefile->WriteInt( sweepAngle2 ); // int sweepAngle2
	savefile->WriteInt( lerpStartAngle ); // int lerpStartAngle
	savefile->WriteInt( lerpEndAngle ); // int lerpEndAngle
	savefile->WriteInt( sweepTimerEnd ); // int sweepTimerEnd
	savefile->WriteInt( sweepTimerInterruptionTime ); // int sweepTimerInterruptionTime
	savefile->WriteInt( sweeptimeMax ); // int sweeptimeMax

	savefile->WriteFloat( sweepAngle ); // float sweepAngle
	savefile->WriteInt( modelAxis ); // int modelAxis
	savefile->WriteBool( flipAxis ); // bool flipAxis
	savefile->WriteFloat( scanDist ); // float scanDist
	savefile->WriteFloat( scanFov ); // float scanFov
	savefile->WriteFloat( frustumScale ); // float frustumScale

	savefile->WriteBool( negativeSweep ); // bool negativeSweep
	savefile->WriteFloat( scanFovCos ); // float scanFovCos

	savefile->WriteVec3( viewOffset ); // idVec3 viewOffset

	savefile->WriteInt( pvsArea ); // int pvsArea

	savefile->WriteStaticObject( idSecurityCamera::physicsObj ); // idPhysics_RigidBody physicsObj
	bool restorePhysics = &physicsObj == GetPhysics();
	savefile->WriteBool( restorePhysics );

	savefile->WriteTraceModel( trm ); // idTraceModel trm

	savefile->WriteInt( pauseTimer ); // int pauseTimer
	savefile->WriteInt( cameraCheckTimer ); // int cameraCheckTimer
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteInt( suspicionPips ); // int suspicionPips

	SaveFileWriteArray( boundingBeam, CAMERA_SPOKECOUNT, WriteObject ); // idBeam* boundingBeam[CAMERA_SPOKECOUNT]
	SaveFileWriteArray( boundingBeamTarget, CAMERA_SPOKECOUNT, WriteObject ); // idBeam* boundingBeamTarget[CAMERA_SPOKECOUNT]

	savefile->WriteObject( spotLight ); // idLight * spotLight

	savefile->WriteFloat( videoFOV ); // float videoFOV

	savefile->WriteInt( enemyCheckTimer ); // int enemyCheckTimer

	savefile->WriteFloat( enemycheckFovCos ); // float enemycheckFovCos

	savefile->WriteFloat( spotlightScale ); // float spotlightScale


	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteInt( damageSparkTimer ); // int damageSparkTimer
}

/*
================
idSecurityCamera::Restore
================
*/
void idSecurityCamera::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( IsSpliced ); // bool IsSpliced
	savefile->ReadInt( cameraIndex ); // int cameraIndex

	savefile->ReadInt( state ); // int state
	savefile->ReadInt( sweepAngle1 ); // int sweepAngle1
	savefile->ReadInt( sweepAngle2 ); // int sweepAngle2
	savefile->ReadInt( lerpStartAngle ); // int lerpStartAngle
	savefile->ReadInt( lerpEndAngle ); // int lerpEndAngle
	savefile->ReadInt( sweepTimerEnd ); // int sweepTimerEnd
	savefile->ReadInt( sweepTimerInterruptionTime ); // int sweepTimerInterruptionTime
	savefile->ReadInt( sweeptimeMax ); // int sweeptimeMax

	savefile->ReadFloat( sweepAngle ); // float sweepAngle
	savefile->ReadInt( modelAxis ); // int modelAxis
	savefile->ReadBool( flipAxis ); // bool flipAxis
	savefile->ReadFloat( scanDist ); // float scanDist
	savefile->ReadFloat( scanFov ); // float scanFov
	savefile->ReadFloat( frustumScale ); // float frustumScale

	savefile->ReadBool( negativeSweep ); // bool negativeSweep
	savefile->ReadFloat( scanFovCos ); // float scanFovCos

	savefile->ReadVec3( viewOffset ); // idVec3 viewOffset

	savefile->ReadInt( pvsArea ); // int pvsArea

	savefile->ReadStaticObject( physicsObj ); // idPhysics_RigidBody physicsObj
	bool restorePhys;
	savefile->ReadBool( restorePhys );
	if (restorePhys)
	{
		RestorePhysics( &physicsObj );
	}

	savefile->ReadTraceModel( trm ); // idTraceModel trm

	savefile->ReadInt( pauseTimer ); // int pauseTimer
	savefile->ReadInt( cameraCheckTimer ); // int cameraCheckTimer
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadInt( suspicionPips ); // int suspicionPips

	SaveFileReadArrayCast( boundingBeam, ReadObject, idClass*& ); // idBeam* boundingBeam[CAMERA_SPOKECOUNT]
	SaveFileReadArrayCast( boundingBeamTarget, ReadObject, idClass*& ); // idBeam* boundingBeamTarget[CAMERA_SPOKECOUNT]

	savefile->ReadObject( CastClassPtrRef(spotLight) ); // idLight * spotLight

	savefile->ReadFloat( videoFOV ); // float videoFOV

	savefile->ReadInt( enemyCheckTimer ); // int enemyCheckTimer

	savefile->ReadFloat( enemycheckFovCos ); // float enemycheckFovCos

	savefile->ReadFloat( spotlightScale ); // float spotlightScale


	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadInt( damageSparkTimer ); // int damageSparkTimer
}

/*
================
idSecurityCamera::Spawn
================
*/
void idSecurityCamera::Spawn( void ) {
	idStr	str;

	
	spotlightScale = spawnArgs.GetFloat("spotlightscale", ".9");
	HotReload();
	
	BecomeActive( TH_THINK );

	if ( health )
	{
		fl.takedamage = true;
	}

	pvsArea = gameLocal.pvs.GetPVSArea( GetPhysics()->GetOrigin() );
	// if no target specified use ourself
	str = spawnArgs.GetString( "cameraTarget" );
	if ( str.Length() == 0 ) {
		spawnArgs.Set( "cameraTarget", spawnArgs.GetString( "name" ) );
	}

	// check if a clip model is set
	spawnArgs.GetString( "clipmodel", "", str );
	if ( !str[0] ) {
		str = spawnArgs.GetString( "model" );		// use the visual model
	}

	if ( !collisionModelManager->TrmFromModel( str, trm ) ) {
		gameLocal.Error( "idSecurityCamera '%s': cannot load collision model %s", name.c_str(), str.c_str() );
		return;
	}

	GetPhysics()->SetContents( CONTENTS_SOLID );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
	// setup the physics
	UpdateChangeableSpawnArgs( NULL );

	

	//Spawn the bounding beams.
	for (int i = 0; i < CAMERA_SPOKECOUNT; i++)
	{
		idDict args;
		args.Clear();
		args.SetVector("origin", vec3_origin);
		args.SetBool("start_off", false);
		this->boundingBeamTarget[i] = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);

		args.Clear();
		args.Set("target", this->boundingBeamTarget[i]->name.c_str());
		args.SetVector("origin", vec3_origin);
		args.SetBool("start_off", false);
		args.Set("width", "4");
		args.Set("skin", "skins/beam_colorable");
		this->boundingBeam[i] = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
		this->boundingBeam[i]->GetRenderEntity()->shaderParms[SHADERPARM_RED] = 0;
		this->boundingBeam[i]->GetRenderEntity()->shaderParms[SHADERPARM_GREEN] = 0;
		this->boundingBeam[i]->GetRenderEntity()->shaderParms[SHADERPARM_BLUE] = 0;
	}


	if (1)
	{
		// The local ambient glow.
		#define GLOW_RADIUS 24
		idVec3 modelForward, modelUp;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&modelForward, NULL, &modelUp);

		headlight.shader = declManager->FindMaterial("lights/flicker_securitycamera", false);
		headlight.pointLight = true;
		headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = GLOW_RADIUS;
		headlight.shaderParms[0] = 1;
		headlight.shaderParms[1] = 0;
		headlight.shaderParms[2] = 0;
		headlight.shaderParms[3] = 1.0f;
		headlight.noShadows = true;
		headlight.isAmbient = false;
		headlight.axis = mat3_identity;
		headlightHandle = gameRenderWorld->AddLightDef(&headlight);

		headlight.origin = GetPhysics()->GetOrigin() + (modelForward * 6) + (modelUp * -6);
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}

	team = spawnArgs.GetInt("team", "1");
	SetTeam(team);


	IsSpliced = true; //BC all cameras are now instantly unlocked at level start.

	enemyCheckTimer = 0;

	
	
}

//When 'reloadmap' is used.
void idSecurityCamera::HotReload()
{
	sweepAngle = spawnArgs.GetFloat("sweepAngle", "90");
	health = spawnArgs.GetInt("health", "100");
	scanFov = spawnArgs.GetFloat("scanFov", "40");
	scanDist = spawnArgs.GetFloat("scanDist", "512");
	flipAxis = spawnArgs.GetBool("flipAxis");
	frustumScale = spawnArgs.GetFloat("frustumScale", "1.15f");
	videoFOV = spawnArgs.GetInt("videofov", "110");
	enemycheckFovCos = cos(spawnArgs.GetInt("enemycheckFov", "90") * idMath::PI / 360.0f);

	modelAxis = spawnArgs.GetInt("modelAxis");
	if (modelAxis < 0 || modelAxis > 2) {
		modelAxis = 0;
	}

	spawnArgs.GetVector("viewOffset", "0 0 0", viewOffset);

	negativeSweep = (sweepAngle < 0) ? true : false;
	sweepAngle = idMath::Fabs(sweepAngle);
	scanFovCos = cos(scanFov * idMath::PI / 360.0f);

	if (sweepAngle > 0)
	{
		//Set up the info for the sweep arc angles.
		sweepAngle1 = GetPhysics()->GetAxis().ToAngles().yaw - (sweepAngle / 2);
		sweepAngle2 = GetPhysics()->GetAxis().ToAngles().yaw + (sweepAngle / 2);



		//If it rotates, then snap angle to the extreme end of the arc. We do this so that don't need special logic for having the camera start in the middle of the arc.
		idAngles startAngle = GetPhysics()->GetAxis().ToAngles();
		startAngle.yaw = sweepAngle1;
		SetAngles(startAngle);
	}
	else
	{
		sweepAngle1 = 0;
		sweepAngle2 = 0;
	}

	if (spawnArgs.GetBool("spotLight"))
	{
		//PostEventMS(&EV_SecurityCam_AddLight, 0);
		Event_AddLight();
	}

	state = PAUSED; //Start at the end of the arc.
	pauseTimer = 0;
	lerpStartAngle = 0;
	lerpEndAngle = 0;
	cameraCheckTimer = 0;
	stateTimer = 0;
	sweepTimerEnd = 0;
	sweepTimerInterruptionTime = 0;
	sweeptimeMax = 0;
	suspicionPips = 0;

	SetAlertModeVis(state);
}


/*
================
idSecurityCamera::Event_AddLight
================
*/
void idSecurityCamera::Event_AddLight( void ) {
	idDict	args;
	idVec3	right, up, target;
	idVec3	dir;
	float	radius;
	idVec3	lightOffset;
	
	
	// SM: Rewrote this to just calculate frustum vectors based on the forward axis of the model
	// (Which from the description of light_target/etc, seems to be the correct approach)
	switch (modelAxis)
	{
	case 0:
	default:
		dir = idVec3( 1.0f, 0.0f, 0.0f );
		break;
	case 1:
		dir = idVec3( 0.0f, 1.0f, 0.0f );
		break;
	case 2:
		dir = idVec3( 0.0f, 0.0f, 1.0f );
		break;
	}

	if (flipAxis)
	{
		dir *= -1.0f;
	}
	
	dir.NormalVectors( right, up );
	target = dir * scanDist;

	// SW: There's a (low, but nonzero) chance that this fix might mess with existing camera setups,
	// so we're putting it behind a cvar for doing side-by-side comparisons.
	if (g_newSecurityCamLights.GetBool())
	{
		radius = scanDist * tan(scanFov * idMath::PI / 360.0f);
		up = up * radius * frustumScale;
		right = right * radius * frustumScale;
	}
	else
	{
		radius = tan(scanFov * idMath::PI / 360.0f);
		up = dir + up * radius;
		up.Normalize();
		up = up * scanDist * frustumScale;
		up -= target;

		right = dir + right * radius;
		right.Normalize();
		right = right * scanDist * frustumScale;
		right -= target;
	}

	//BC move light forward a bit so that its shadow casting doesnt get messed up by the model. This will affect the accuracy of the spotlight a bit, but it should be minor.
	idVec3 modelForward;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&modelForward, NULL, NULL);

	spawnArgs.GetVector( "lightOffset", "0 0 0", lightOffset );
	args.Set( "origin", ( GetPhysics()->GetOrigin() + lightOffset + (modelForward * 7.7f) ).ToString() );
	args.Set( "light_target", target.ToString() );
	args.Set( "light_right", right.ToString() );
	args.Set( "light_up", up.ToString() );
	args.Set( "texture", "lights/securitycamera_spotlight" );
	args.SetInt("noshadows", 0);
	

	spotLight = static_cast<idLight *>( gameLocal.SpawnEntityType( idLight::Type, &args ) );
	spotLight->Bind( this, true );
	// SM: This makes it so the spotlight always aligns with the facing of this camera entity
	spotLight->SetAngles( idAngles( 0.0f, 0.0f, 0.0f ) );
	spotLight->UpdateVisuals();
}

/*
================
idSecurityCamera::DrawFov
================
*/
void idSecurityCamera::DrawFov( void ) {
	int i;
	float radius, a, s, c, halfRadius;
	idVec3 right, up;
	idVec4 color(1, 0, 0, 1), color2(0, 0, 1, 1);
	idVec3 lastPoint, point, lastHalfPoint, halfPoint, center;

	idVec3 dir = GetAxis();
	dir.NormalVectors( right, up );

	radius = tan( scanFov * idMath::PI / 360.0f );
	halfRadius = radius * 0.5f;
	lastPoint = dir + up * radius;
	lastPoint.Normalize();
	lastPoint = GetPhysics()->GetOrigin() + lastPoint * scanDist;
	lastHalfPoint = dir + up * halfRadius;
	lastHalfPoint.Normalize();
	lastHalfPoint = GetPhysics()->GetOrigin() + lastHalfPoint * scanDist;
	center = GetPhysics()->GetOrigin() + dir * scanDist;
	for ( i = 1; i < 12; i++ ) {
		a = idMath::TWO_PI * i / 12.0f;
		idMath::SinCos( a, s, c );
		point = dir + right * s * radius + up * c * radius;
		point.Normalize();
		point = GetPhysics()->GetOrigin() + point * scanDist;
		gameRenderWorld->DebugLine( color, lastPoint, point );
		gameRenderWorld->DebugLine( color, GetPhysics()->GetOrigin(), point );
		lastPoint = point;

		halfPoint = dir + right * s * halfRadius + up * c * halfRadius;
		halfPoint.Normalize();
		halfPoint = GetPhysics()->GetOrigin() + halfPoint * scanDist;
		gameRenderWorld->DebugLine( color2, point, halfPoint );
		gameRenderWorld->DebugLine( color2, lastHalfPoint, halfPoint );
		lastHalfPoint = halfPoint;

		gameRenderWorld->DebugLine( color2, halfPoint, center );
	}
}


void idSecurityCamera::DrawBounds(void) {
	int i;
	float radius, a, s, c, halfRadius;
	idVec3 right, up;
	idVec4 color(1, 0, 0, 1), color2(0, 0, 1, 1);
	idVec3 lastPoint, point, lastHalfPoint, halfPoint, center;

	idVec3 dir = GetAxis();
	dir.NormalVectors(right, up);

	radius = tan(scanFov * idMath::PI / 360.0f);
	halfRadius = radius * 0.5f;
	lastPoint = dir + up * radius;
	lastPoint.Normalize();
	lastPoint = GetPhysics()->GetOrigin() + lastPoint * scanDist;
	lastHalfPoint = dir + up * halfRadius;
	lastHalfPoint.Normalize();
	lastHalfPoint = GetPhysics()->GetOrigin() + lastHalfPoint * scanDist;
	center = GetPhysics()->GetOrigin() + dir * scanDist;

	idVec3 firstSpokePosition = vec3_zero;
	idVec3 lastSpokePosition = vec3_zero;

	for (i = 1; i < CAMERA_SPOKECOUNT; i++)
	{
		a = idMath::TWO_PI * i / (float)CAMERA_SPOKECOUNT;
		idMath::SinCos(a, s, c);
		point = dir + right * s * radius + up * c * radius;
		point.Normalize();
		point = GetPhysics()->GetOrigin() + point * scanDist;

		trace_t tr;
		gameLocal.clip.TracePoint(tr, GetPhysics()->GetOrigin(), point, MASK_SOLID, this);		
		
		if (i >= 2)
		{
			this->boundingBeam[i - 2]->SetOrigin(lastSpokePosition + dir * -1);
			this->boundingBeamTarget[i - 2]->SetOrigin(tr.endpos + dir * -1);
		}
		else if (i <= 1)
		{
			firstSpokePosition = tr.endpos;
		}

		lastSpokePosition = tr.endpos;
	}

	//Final spoke.
	this->boundingBeam[CAMERA_SPOKECOUNT - 1]->SetOrigin(lastSpokePosition + dir * -1);
	this->boundingBeamTarget[CAMERA_SPOKECOUNT - 1]->SetOrigin(firstSpokePosition + dir * -1);
}


/*
================
idSecurityCamera::GetRenderView
================
*/
renderView_t *idSecurityCamera::GetRenderView() {
	renderView_t *rv = idEntity::GetRenderView();
	rv->fov_x = videoFOV; //was scanFov
	rv->fov_y = videoFOV; //was scanFov
	rv->viewaxis = GetAxis().ToAngles().ToMat3();
	rv->vieworg = GetPhysics()->GetOrigin() + viewOffset;
	rv->viewID = entityNumber + 1;
	return rv;
}

/*
================
idSecurityCamera::CanSeePlayer
================
*/
bool idSecurityCamera::CanSeePlayer( void ) {
	idPlayer *ent;	
	pvsHandle_t handle;

	handle = gameLocal.pvs.SetupCurrentPVS( pvsArea );

	for ( int i = 0; i < gameLocal.numClients; i++ )
	{
		ent = static_cast<idPlayer*>(gameLocal.entities[ i ]);

		if ( !ent ||  ent->fl.notarget  || ent->noclip  ) {
			continue;
		}

		// if there is no way we can see this player
		if ( !gameLocal.pvs.InCurrentPVS( handle, ent->GetPVSAreas(), ent->GetNumPVSAreas() ) ) {
			continue;
		}			

		//Check LOS to feet.
		if (GetLOSCheck(ent, vec3_zero, scanFovCos))
		{
			gameLocal.pvs.FreeCurrentPVS(handle);
			return true;
		}

		//Check LOS to eyeballs.
		idVec3 eyeOffset;
		eyeOffset = ent->EyeOffset();
		if (GetLOSCheck(ent, eyeOffset, scanFovCos))
		{
			gameLocal.pvs.FreeCurrentPVS(handle);
			return true;
		}		
	}

	gameLocal.pvs.FreeCurrentPVS( handle );

	return false;
}

//Checks if camera can see enemy. If so, tell game to draw the enemy's healthbar. This is used when the player has hacked (gained control of) the camera.
void idSecurityCamera::CanSeeEnemy()
{
	pvsHandle_t handle;

	handle = gameLocal.pvs.SetupCurrentPVS(pvsArea);

	for (idEntity* ent = gameLocal.aimAssistEntities.Next(); ent != NULL; ent = ent->aimAssistNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idAI::Type) || ent->health <= 0 || static_cast<idAI*>(ent)->team != TEAM_ENEMY)
			continue;

		if (!gameLocal.pvs.InCurrentPVS(handle, ent->GetPVSAreas(), ent->GetNumPVSAreas()))
			continue;

		if (GetLOSCheck(ent, idVec3(0,0,1), enemycheckFovCos) || GetLOSCheck(ent, idVec3(0, 0, 68), enemycheckFovCos)) //Check LOS to feet and/or eyes.
		{
			//Can see this enemy.
			static_cast<idAI *>(ent)->ShowHealthbar();
			static_cast<idAI *>(ent)->ShowCameraicon();
			continue;
		}
	}

	gameLocal.pvs.FreeCurrentPVS(handle);
	return;
}

//We have this function so we can check LOS on the feet AND eyeballs.
//Offset = this is to handle checking LOS to feet and eyeball.
//fovAmount = when camera is scanning for enemies, we tweak it so the fov is wider (to make the camera more useful)
bool idSecurityCamera::GetLOSCheck(idEntity *ent, idVec3 offset, float fovAmount)
{
	idVec3 dir;
	float dist;
	trace_t tr;

	dir = (ent->GetPhysics()->GetOrigin() + offset) - GetPhysics()->GetOrigin();
	dist = dir.Normalize();

	if (dist > scanDist)
	{
		return false;
	}

	if (dir * GetAxis() < fovAmount)
	{
		return false;
	}

	gameLocal.clip.TracePoint(tr, GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin() + offset, MASK_SOLID, this);
	if (tr.fraction >= 1.0f || (gameLocal.GetTraceEntity(tr) == ent))
	{		
		return true;
	}

	return false;
}



/*
================
idSecurityCamera::SetAlertMode
================
*/
void idSecurityCamera::SetAlertModeVis( int _newState )
{
	idVec3 newColor = CAMERA_DEFAULTCOLOR;

	switch (_newState)
	{
		case ALERTED:
			newColor = idVec3(1, 0, 0);
			break;
		case SUSPICIOUS:
			newColor = idVec3(1, .6f, 0);
			break;
		case ELECTRICAL_DISABLED:
			newColor = idVec3(0, 0, 0);
			break;
		default:
			break;
	}

	// SW 18th March 2025: friendly cameras don't get a free pass -- they're also disabled by power outages
	if(team == TEAM_FRIENDLY && _newState != ELECTRICAL_DISABLED)
	{
		newColor = CAMERA_FRIENDLYCOLOR;
	}

	SetShaderParm(5, team);

	if (headlightHandle != -1)
	{
		idVec3 headLightColor = team == TEAM_FRIENDLY ? HEADLIGHT_FRIENDLYCOLOR : HEADLIGHT_DEFAULTCOLOR;
		//update local light.
		headlight.shaderParms[0] = headLightColor.x;
		headlight.shaderParms[1] = headLightColor.y;
		headlight.shaderParms[2] = headLightColor.z;
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}

	SetColor(newColor); // this calls updatevisuals()
	newColor *= spotlightScale;
	spotLight->SetColor(newColor); // this calls updatevisuals()

	

	//for (int i = 0; i < CAMERA_SPOKECOUNT; i++)
	//{
	//	this->boundingBeam[i]->GetRenderEntity()->shaderParms[SHADERPARM_RED] = newColor.x;
	//	this->boundingBeam[i]->GetRenderEntity()->shaderParms[SHADERPARM_GREEN] = newColor.y;
	//	this->boundingBeam[i]->GetRenderEntity()->shaderParms[SHADERPARM_BLUE] = newColor.z;
	//	this->boundingBeam[i]->UpdateVisuals();
	//}
}

/*
================
idSecurityCamera::Think
================
*/
void idSecurityCamera::Think(void) {

	// space out checks among all cameras (some # per frame, as defined below)
	int camerasPerFrame = 4;
	int cameraBatches = ((cameraCount - 1) / camerasPerFrame) + 1; 
	int cameraBatch = (cameraID / camerasPerFrame) + 1;
	bool canCheckSuspicion = (gameLocal.framenum % cameraBatches) + 1 == cameraBatch;

	if (thinkFlags & TH_THINK)
	{
		if (health <= 0)
		{
			BecomeInactive(TH_THINK);
		}
	}

	// SW 18th March 2025:
	// Friendly cameras in a power outage can't spot enemies either
	if (canCheckSuspicion && (team == TEAM_FRIENDLY) && (gameLocal.time >= enemyCheckTimer) && state != ELECTRICAL_DISABLED)
	{
		//If on friendly team, then do checks to see if camera can see enemies.
		CanSeeEnemy();
		enemyCheckTimer = gameLocal.time + ENEMYCHECKFREQUENCY;
	}

	if (state == SWEEPING)
	{
		if (canCheckSuspicion && (team == TEAM_ENEMY) && CanSeePlayer())
		{
			BecomeSuspicious();
			return;
		}

		if (sweepAngle != 0)
		{
			float lerp = 1.0f - (sweepTimerEnd - gameLocal.time) / (float)(sweeptimeMax);
			lerp = idMath::ClampFloat(0, 1, lerp);

			idAngles newAngle = GetPhysics()->GetAxis().ToAngles();
			newAngle.yaw = idMath::Lerp(lerpStartAngle, lerpEndAngle, lerp);
			SetAngles(newAngle);
		}
		else
		{
			//camera is STATIC. does NOT move.
		}

		if (gameLocal.time >= sweepTimerEnd)
		{
			StopSound(SND_CHANNEL_BODY, false);
			state = PAUSED;
			pauseTimer = gameLocal.time + SEC2MS(spawnArgs.GetFloat("sweepWait", "2"));

			StartSound("snd_idlebeep", SND_CHANNEL_BODY, 0, false, NULL);
		}
	}
	else if (state == PAUSED)
	{
		if (canCheckSuspicion && (team == TEAM_ENEMY) && CanSeePlayer())
		{
			BecomeSuspicious();
			return;
		}

		if (gameLocal.time >= pauseTimer)
		{
			//start rotating again.
			negativeSweep = !negativeSweep;
			if (negativeSweep)
			{
				lerpStartAngle = GetPhysics()->GetAxis().ToAngles().yaw;
				lerpEndAngle = sweepAngle2;
			}
			else
			{
				lerpStartAngle = GetPhysics()->GetAxis().ToAngles().yaw;
				lerpEndAngle = sweepAngle1;
			}
			
			AdjustStartEndLerpAngles();

			sweeptimeMax = SEC2MS(GetSweepTime()); //Reset to normal full sweep time.

			if (sweepAngle != 0)
				StartSound("snd_moving", SND_CHANNEL_BODY, 0, false, NULL);

			sweepTimerEnd = gameLocal.time + sweeptimeMax;
			state = SWEEPING;
		}
	}
	else if (state == SUSPICIOUS)
	{
		if (team == TEAM_FRIENDLY)
		{
			ResumeSweep();
			return;
		}

		if (canCheckSuspicion && (gameLocal.time > cameraCheckTimer)) //We don't need to check every frame. Space things out a bit.
		{
			cameraCheckTimer = gameLocal.time + SUSPICION_INTERVALTIME;

			if (CanSeePlayer())
			{
				//Reset the suspicion timer. Make it stay suspicious longer.
				stateTimer = gameLocal.time + GetSightTimeMS() + SEC2MS(spawnArgs.GetFloat("sightResume", "2"));
				suspicionPips++;

				int maxPips = GetSightTimeMS() / (float)SUSPICION_INTERVALTIME;

				if (suspicionPips >= maxPips)
				{
					//Camera has sighted player.
					gameLocal.AddEventLog("#str_def_gameplay_camera_seeyou", GetPhysics()->GetOrigin(), true, EL_INTERESTPOINT);
					BecomeAlerted(gameLocal.GetLocalPlayer());
					return;
				}
				else
				{
					StartSound("snd_visible", SND_CHANNEL_BODY2, 0, false, NULL);
				}
			}

			//Display suspicion pip amount.
			float totalPipCount = GetSightTimeMS() / (float)SUSPICION_INTERVALTIME;
			float lerp = suspicionPips / (float)totalPipCount;
			lerp = idMath::ClampFloat(0, 1, lerp);
			lerp *= .6f;

			renderEntity.shaderParms[5] = 1;
			renderEntity.shaderParms[SHADERPARM_MODE] = lerp; //do the red bar fx.
			UpdateVisuals();
		}


		if (gameLocal.time > stateTimer)
		{
			//We haven't seen anything suspicious for a while. Resume the sweep.
			ResumeSweep();
		}
	}
	else if (state == ALERTED)
	{
		if (team == TEAM_FRIENDLY)
		{
			ResumeSweep();
			return;
		}

		if (gameLocal.time > stateTimer)
		{
			//Cool down a bit, to give a nice cooldown buffer between the alarm state and the rotating state.
			StopSound(SND_CHANNEL_BODY, false);
			state = COOLDOWN;
			stateTimer = gameLocal.time + SEC2MS(spawnArgs.GetFloat("sightResume", "2"));
			SetAlertModeVis(state);
		}
	}
	else if (state == COOLDOWN)
	{
		if (team == TEAM_FRIENDLY)
		{
			ResumeSweep();
			return;
		}

		if (gameLocal.time > stateTimer)
		{
			//Return to normal state.
			ResumeSweep();
		}
	}
	else if (state == DAMAGEWARNING)
	{
		if (team == TEAM_FRIENDLY)
		{
			ResumeSweep();
			return;
		}

		if (gameLocal.time > damageSparkTimer)
		{
			damageSparkTimer = gameLocal.time + SPARKINTERVAL;

			//spawn a spark.
			idAngles randomAng = idAngles(-15, gameLocal.random.RandomInt(359), 0);
			gameLocal.CreateSparkObject(GetPhysics()->GetOrigin()  + idVec3(0,0,-6), randomAng.ToForward());
		}

		//Get a lerp from 0.0 - 0.5 for the red bar effect.
		int totalDamageWarnTime = GetSightDamageWarnTimeMS();
		float lerp = (stateTimer - gameLocal.time) / (float)totalDamageWarnTime;
		lerp = 1.0f - lerp;
		lerp = idMath::ClampFloat(0, 1, lerp);
		lerp *= .5f;

		renderEntity.shaderParms[5] = 0;
		renderEntity.shaderParms[SHADERPARM_MODE] = lerp; //do the red bar fx.
		UpdateVisuals();		

		if (gameLocal.time > stateTimer)
		{
			//Go to alert mode.
			BecomeAlerted(NULL);

			//Camera took damage and is now activating the alarm.
			gameLocal.AddEventLog("#str_def_gameplay_camera_damagealert", GetPhysics()->GetOrigin());
		}
	}

	//if (state != DEAD)
	//{
	//	DrawBounds();
	//}

	RunPhysics();
	Present();
}

void idSecurityCamera::BecomeAlerted(idEntity *targetEnt)
{
	StopSound(SND_CHANNEL_ITEM, false);
	state = ALERTED;
	StopSound(SND_CHANNEL_BODY, false);
	StartSound("snd_alert", SND_CHANNEL_BODY, 0, false, NULL);
	stateTimer = gameLocal.time + SEC2MS(spawnArgs.GetFloat("alertwait", "20"));
	gameLocal.DoParticle("camera_alert.prt", this->GetPhysics()->GetOrigin());
	
	if (targetEnt)
	{
		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetLKPPositionByEntity(targetEnt);
	}

	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateMetaLKP(false);
	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->AlertAIFriends(this); //Summon baddies. Go to combat state.

	//Spawn a security bot.
	gameLocal.AddEventLog("#str_def_gameplay_camera_summon", GetPhysics()->GetOrigin());
	if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SpawnSecurityBot(GetPhysics()->GetOrigin()))
	{
		gameLocal.DoParticle(spawnArgs.GetString("model_soundwave"), GetPhysics()->GetOrigin());
		StartSound("snd_vo_deploysecurity", SND_CHANNEL_VOICE);
	}

	renderEntity.shaderParms[SHADERPARM_MODE] = 0; //hide red bar fx.
	SetAlertModeVis(state);
}

void idSecurityCamera::ResumeSweep()
{
	lerpStartAngle = GetPhysics()->GetAxis().ToAngles().yaw; //We only change the lerpStartAngle. We don't change the lerpEndAngle.
	AdjustStartEndLerpAngles();

	renderEntity.shaderParms[SHADERPARM_MODE] = 0; //hide red bar fx.
	UpdateVisuals();

	if (sweepTimerInterruptionTime >= sweepTimerEnd)
	{
		//The camera was done with its movement, was in the pause state.
		state = PAUSED;
		pauseTimer = 0;
		SetAlertModeVis(state);
		return;
	}

	//Do some math to find out how much remaining percentage the camera has to travel. This will determine the amount of time to move the camera.
	state = SWEEPING;
	float remainingPercentage = (sweepTimerEnd - sweepTimerInterruptionTime) / (float)(SEC2MS(GetSweepTime()));
	sweepTimerEnd = gameLocal.time + (remainingPercentage * (float)(SEC2MS(GetSweepTime())));
	sweeptimeMax = remainingPercentage * (float)(SEC2MS(GetSweepTime()));

	if (sweepAngle != 0)
		StartSound("snd_moving", SND_CHANNEL_BODY, 0, false, NULL);

	SetAlertModeVis(state);
}

void idSecurityCamera::BecomeSuspicious()
{
	cameraCheckTimer = gameLocal.time + SUSPICION_INTERVALTIME;
	state = SUSPICIOUS;
	StopSound(SND_CHANNEL_BODY, false);
	StartSound("snd_sight", SND_CHANNEL_BODY, 0, false, NULL);
	SetAlertModeVis(state);

	stateTimer = gameLocal.time + GetSightTimeMS() + SEC2MS(spawnArgs.GetFloat("sightResume", "2"));
	sweepTimerInterruptionTime = gameLocal.time;
	suspicionPips = 0;

	gameLocal.AddEventLog("#str_def_gameplay_camera_suspicious", GetPhysics()->GetOrigin());
}


/*
================
idSecurityCamera::GetAxis
================
*/
const idVec3 idSecurityCamera::GetAxis( void ) const {
	return (flipAxis) ? -GetPhysics()->GetAxis()[modelAxis] : GetPhysics()->GetAxis()[modelAxis];
};

/*
================
idSecurityCamera::SweepSpeed
================
*/
float idSecurityCamera::GetSweepTime( void ) const {
	return spawnArgs.GetFloat( "sweepTime", "7" );
}

int idSecurityCamera::GetSightTimeMS() const
{
	return SEC2MS(spawnArgs.GetFloat("sightTime", "1.5"));
}

int idSecurityCamera::GetSightDamageWarnTimeMS() const
{
	return SEC2MS(spawnArgs.GetFloat("damagetime", "6"));
}


/*
============
idSecurityCamera::Killed
============
*/
void idSecurityCamera::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {

	state = DEAD;
	StopSound( SND_CHANNEL_ANY, false );
	const char *fx = spawnArgs.GetString( "fx_destroyed" );
	if ( fx[0] != '\0' ) {
		idEntityFx::StartFx( fx, NULL, NULL, this, true );
	}

	//BC cleanup.
	for (int i = 0; i < CAMERA_SPOKECOUNT; i++)
	{
		this->boundingBeam[i]->Hide();
	}

	spotLight->SetColor(0, 0, 0); //hide the light.
	SetColor(0, 0, 0);
	SetShaderParm(5, TEAM_NEUTRAL);
	UpdateVisuals();


	//Kill any lights it is targeting.
	if (targets.Num() > 0)
	{
		for (int i = 0; i < targets.Num(); i++)
		{
			if (!targets[i].IsValid())
				continue;

			if (targets[i].GetEntity()->IsType(idLight::Type))
			{
				targets[i].GetEntity()->PostEventMS(&EV_Remove, 0);
			}
		}
	}

	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
		headlightHandle = -1;
	}

	

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( trm ), 0.02f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetBouncyness( 0.2f );
	physicsObj.SetFriction( 0.6f, 0.6f, 0.2f );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetContents( CONTENTS_SOLID );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
	SetPhysics( &physicsObj );
}


/*
============
idSecurityCamera::Pain
============
*/
bool idSecurityCamera::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	const char *fx = spawnArgs.GetString( "fx_damage" );
	if ( fx[0] != '\0' ) {
		idEntityFx::StartFx( fx, NULL, NULL, this, true );
	}
	return true;
}


/*
================
idSecurityCamera::Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/
void idSecurityCamera::Present( void ) {
	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	// camera target for remote render views
	if ( cameraTarget ) {
		renderEntity.remoteRenderView = cameraTarget->GetRenderView();
		renderEntity.suppressSurfaceInViewID = entityNumber + 1;
		renderEntity.suppressShadowInViewID = entityNumber + 1;
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

void idSecurityCamera::AdjustStartEndLerpAngles()
{
	//Make sure it's the shortest distance.
	lerpStartAngle = idMath::AngleNormalize360(lerpStartAngle);
	lerpEndAngle = idMath::AngleNormalize360(lerpEndAngle);
	float delta_yaw = lerpStartAngle - lerpEndAngle;
	if (idMath::Fabs(delta_yaw) > 180.f)
	{
		if (lerpStartAngle > lerpEndAngle)
			lerpEndAngle += 360;
		else
			lerpStartAngle += 360;
	}
}

void idSecurityCamera::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	if (gameLocal.time > lastHealthTimer) //if take multiple damage quickly, batch them all up together here to make it easier to read for the player.
		lastHealth = health;	

	lastHealthTimer = gameLocal.time + LASTHEALTH_MAXTIME;

	idEntity::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);

	if (team == TEAM_FRIENDLY)
	{
		return;
	}

	if (state != DAMAGEWARNING && state != ALERTED && health > 0)
	{
		state = DAMAGEWARNING;
		StopSound(SND_CHANNEL_BODY, false);
		StartSound("snd_damagebeep", SND_CHANNEL_ITEM);
		stateTimer = gameLocal.time + GetSightDamageWarnTimeMS();
		sweepTimerInterruptionTime = gameLocal.time;

		renderEntity.shaderParms[SHADERPARM_MODE] = .01f; //do the red bar fx.
		UpdateVisuals();

		gameLocal.AddEventLog("#str_def_gameplay_camera_sparking", GetPhysics()->GetOrigin());
	}

	if (health <= 0)
	{
		renderEntity.shaderParms[SHADERPARM_MODE] = 0; //hide red bar fx.
		UpdateVisuals();

		StopSound(SND_CHANNEL_ANY, false);
	}
}

bool idSecurityCamera::IsAlerted()
{
	return (state == ALERTED && health > 0);
}

void idSecurityCamera::SetTeam(int value)
{
	team = value;

	stateTimer = gameLocal.time + 1;
	sweepTimerInterruptionTime = gameLocal.time;

	SetAlertModeVis(state);
}

void idSecurityCamera::SetElectricalActive(bool value)
{
	if (value)
	{
		//turn ON.
		if (health <= 0 || state == DEAD)
		{
			return;
		}

		ResumeSweep();
	}
	else
	{
		//turn OFF.
		state = ELECTRICAL_DISABLED;
		StopSound(SND_CHANNEL_BODY, false);
		// SW 18th March 2025: turn off shader parms so a partially-suspicious camera doesn't keep showing pips
		suspicionPips = 0; 
		renderEntity.shaderParms[SHADERPARM_MODE] = 0;
		SetAlertModeVis(state);
		sweepTimerInterruptionTime = gameLocal.time;
	}
}

void idSecurityCamera::DoHack()
{
	gameLocal.AddEventLog("#str_def_gameplay_camera_hacked", GetPhysics()->GetOrigin());
	SetTeam(TEAM_FRIENDLY);
}