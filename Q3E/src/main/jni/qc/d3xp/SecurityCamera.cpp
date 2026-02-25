/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

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
/*

  SecurityCamera.cpp

  Security camera that triggers targets when player is in view

*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"



#define CAMERAGHOST_LIFETIME			20 //how long ghost sits there.


/***********************************************************************

  idSecurityCamera
	
***********************************************************************/

const idEventDef EV_SecurityCam_ReverseSweep( "<reverseSweep>" );
const idEventDef EV_SecurityCam_ContinueSweep( "<continueSweep>" );
const idEventDef EV_SecurityCam_Pause( "<pause>" );
const idEventDef EV_SecurityCam_Alert( "<alert>" );
const idEventDef EV_SecurityCam_AddLight( "<addLight>" );

const idEventDef EV_SecurityCam_camOff( "camOff" );
const idEventDef EV_SecurityCam_camOn(  "camOn" );

const idEventDef EV_SecurityCam_KillGhost(  "<killghost>" );


const idEventDef EV_SecurityCam_reset(  "camreset" );

CLASS_DECLARATION( idEntity, idSecurityCamera )
	EVENT( EV_SecurityCam_ReverseSweep,		idSecurityCamera::Event_ReverseSweep )
	EVENT( EV_SecurityCam_ContinueSweep,	idSecurityCamera::Event_ContinueSweep )
	EVENT( EV_SecurityCam_Pause,			idSecurityCamera::Event_Pause )
	EVENT( EV_SecurityCam_Alert,			idSecurityCamera::Event_Alert )
	EVENT( EV_SecurityCam_AddLight,			idSecurityCamera::Event_AddLight )

	EVENT( EV_SecurityCam_camOff,			idSecurityCamera::Event_camOff)
	EVENT( EV_SecurityCam_camOn,			idSecurityCamera::Event_camOn)

	EVENT( EV_SecurityCam_KillGhost,		idSecurityCamera::Event_KillGhost)
	EVENT( EV_SecurityCam_reset,			idSecurityCamera::reset)
END_CLASS

/*
================
idSecurityCamera::Save
================
*/
void idSecurityCamera::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( angle );
	savefile->WriteFloat( sweepAngle );
	savefile->WriteInt( modelAxis );
	savefile->WriteBool( flipAxis );
	savefile->WriteFloat( scanDist );
	savefile->WriteFloat( scanFov );
							
	savefile->WriteFloat( sweepStart );
	savefile->WriteFloat( sweepEnd );
	savefile->WriteBool( negativeSweep );
	savefile->WriteBool( sweeping );
	savefile->WriteInt( alertMode );
	savefile->WriteFloat( stopSweeping );
	savefile->WriteFloat( scanFovCos );

	savefile->WriteVec3( viewOffset );
							
	savefile->WriteInt( pvsArea );
	savefile->WriteStaticObject( physicsObj );
	savefile->WriteTraceModel( trm );


	savefile->WriteBool(camActive);
	savefile->WriteFloat(softAlarmTimer);
	savefile->WriteBool(softAlarmActivated);
	savefile->WriteFloat(targetDist);
	savefile->WriteFloat(closeTolerance);
	lastSpottedEnt.Save(savefile);
	savefile->WriteObject(beam);
	savefile->WriteObject(beamTarget);
	savefile->WriteObject(ghostImage);

	int i;
	for (i = 0; i < VIEWCONE_SPOKES; i++)
	{
		savefile->WriteObject(boundBeam[i]);
		savefile->WriteObject(boundBeamTarget[i]);
	}
}

/*
================
idSecurityCamera::Restore
================
*/
void idSecurityCamera::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( angle );
	savefile->ReadFloat( sweepAngle );
	savefile->ReadInt( modelAxis );
	savefile->ReadBool( flipAxis );
	savefile->ReadFloat( scanDist );
	savefile->ReadFloat( scanFov );
							
	savefile->ReadFloat( sweepStart );
	savefile->ReadFloat( sweepEnd );
	savefile->ReadBool( negativeSweep );
	savefile->ReadBool( sweeping );
	savefile->ReadInt( alertMode );
	savefile->ReadFloat( stopSweeping );
	savefile->ReadFloat( scanFovCos );

	savefile->ReadVec3( viewOffset );
							
	savefile->ReadInt( pvsArea );
	savefile->ReadStaticObject( physicsObj );
	savefile->ReadTraceModel( trm );



	savefile->ReadBool(camActive);
	savefile->ReadFloat(softAlarmTimer);
	savefile->ReadBool(softAlarmActivated);
	savefile->ReadFloat(targetDist);
	savefile->ReadFloat(closeTolerance);
	lastSpottedEnt.Restore(savefile);
	savefile->ReadObject(reinterpret_cast<idClass *&>(beam));
	savefile->ReadObject(reinterpret_cast<idClass *&>(beamTarget));
	savefile->ReadObject(reinterpret_cast<idClass *&>(ghostImage));

	int i;
	for (i = 0; i < VIEWCONE_SPOKES; i++)
	{
		savefile->ReadObject(reinterpret_cast<idClass *&>(boundBeam[i]));
		savefile->ReadObject(reinterpret_cast<idClass *&>(boundBeamTarget[i]));
	}
}

/*
================
idSecurityCamera::Spawn
================
*/
void idSecurityCamera::Spawn( void ) {
	idStr	str;
	int	pitch;
	int i;

	sweepAngle	= spawnArgs.GetFloat( "sweepAngle", "90" );
	health		= spawnArgs.GetInt( "health", "100" );
	scanFov		= spawnArgs.GetFloat( "scanFov", "90" );
	scanDist	= spawnArgs.GetFloat( "scanDist", "200" );
	flipAxis	= spawnArgs.GetBool( "flipAxis" );

	modelAxis	= spawnArgs.GetInt( "modelAxis" );
	if ( modelAxis < 0 || modelAxis > 2 ) {
		modelAxis = 0;
	}

	//bc tilt the camera down.
	pitch = spawnArgs.GetInt("pitch", "35");
	idAngles curAng = GetPhysics()->GetAxis().ToAngles();
	curAng.pitch = pitch;
	SetAxis( curAng.ToMat3() );


	this->closeTolerance = spawnArgs.GetInt("closeTolerance", "130");

	this->camActive = true;


	spawnArgs.GetVector( "viewOffset", "0 0 0", viewOffset );

	if ( spawnArgs.GetBool( "spotLight" ) ) {
		PostEventMS( &EV_SecurityCam_AddLight, 0 );
	}

	negativeSweep = ( sweepAngle < 0 ) ? true : false;
	sweepAngle = fabsf( sweepAngle );

	scanFovCos = cos( scanFov * idMath::PI / 360.0f );

	angle = GetPhysics()->GetAxis().ToAngles().yaw;
	
	this->baseAngle = this->angle;
	if (this->baseAngle < 0)
		this->baseAngle += 360.0f;

	StartSweep();
	SetAlertMode( SCANNING );
	BecomeActive( TH_THINK );

	if ( health ) {
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


	//Beam stuff.
	idDict args;

	args.SetVector( "origin", vec3_origin );
	args.SetBool( "start_off", true );
	this->beamTarget = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );

	args.Clear();
	args.Set(		"target", beamTarget->name.c_str() );
	args.SetVector(	"origin", vec3_origin );
	args.SetBool(	"start_off", false );
	args.Set(		"width", "2" );
	args.Set(		"skin", "skins/beam_cameraline" );
	this->beam = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );

	for ( i = 0; i < VIEWCONE_SPOKES; i++ )
	{
		args.Clear();
		args.SetVector( "origin", vec3_origin );
		args.SetBool( "start_off", false );
		this->boundBeamTarget[ i ] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );		

		args.Clear();
		args.Set(		"target", this->boundBeamTarget[ i ]->name.c_str() );
		args.SetVector(	"origin", vec3_origin );
		args.SetBool(	"start_off", false );
		args.Set(		"width", "12" );
		args.Set(		"skin", "skins/beam_camera" );
		this->boundBeam[ i ] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_RED ] = 0;
		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_GREEN ] = 1.0f;
		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_BLUE ] = 0;

		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_TIMESCALE ] = 0.3f;
	}




	args.Clear();
	//args.SetInt( "solid", 0 );
	args.SetVector( "origin", vec3_origin );
	//args.Set( "model", "models/monster_npc/tris.md5mesh" );	
	//this->ghostImage = ( idAnimated * )gameLocal.SpawnEntityType( idAnimated::Type, &args );
	args.Set( "classname", "env_cameradummy" );
	gameLocal.SpawnEntityDef( args, &this->ghostImage );

	this->beam->Hide();
	this->ghostImage->Hide();

	renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;

	StartSound( "snd_hum", SND_CHANNEL_DEMONIC, 0, false, NULL );

}

/*
================
idSecurityCamera::Event_AddLight
================
*/
void idSecurityCamera::Event_AddLight( void ) {
	idDict	args;
	idVec3	right, up, target, temp;
	idVec3	dir;
	float	radius;
	idVec3	lightOffset;
	idLight	*spotLight;
	
	dir = GetAxis();
	dir.NormalVectors( right, up );
	target = GetPhysics()->GetOrigin() + dir * scanDist;
		
	radius = tan( scanFov * idMath::PI / 360.0f );
	up = dir + up * radius;
	up.Normalize();
	up = GetPhysics()->GetOrigin() + up * scanDist;
	up -= target;
	
	right = dir + right * radius;
	right.Normalize();
	right = GetPhysics()->GetOrigin() + right * scanDist;
	right -= target;

	spawnArgs.GetVector( "lightOffset", "0 0 0", lightOffset );
	
	args.Set( "origin", ( GetPhysics()->GetOrigin() + lightOffset ).ToString() );
	args.Set( "light_target", target.ToString() );
	args.Set( "light_right", right.ToString() );
	args.Set( "light_up", up.ToString() );
	args.SetFloat( "angle", GetPhysics()->GetAxis()[0].ToYaw() );
	args.SetInt("alarmlight", 0);

	spotLight = static_cast<idLight *>( gameLocal.SpawnEntityType( idLight::Type, &args ) );
	spotLight->Bind( this, true );
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

idVec3 idSecurityCamera::GetLensPos()
{
	idVec3	forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors( &forward, &right, &up );

	return GetPhysics()->GetOrigin() + (forward * viewOffset.x)	+ (right * viewOffset.y) + (up * viewOffset.z);
}

/*
================
idSecurityCamera::GetRenderView
================
*/
renderView_t *idSecurityCamera::GetRenderView()
{
	idVec3	forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors( &forward, &right, &up );

	renderView_t *rv = idEntity::GetRenderView();
	rv->fov_x = scanFov;
	rv->fov_y = scanFov;
	rv->viewaxis = GetAxis().ToAngles().ToMat3();
	rv->vieworg = GetLensPos();
	return rv;
}

/*
================
idSecurityCamera::CanSeePlayer
================
*/
bool idSecurityCamera::CanSeePlayer( void ) {
	int i;
	float dist;
	idPlayer *ent;
	trace_t tr;
	idVec3 dir;
	pvsHandle_t handle;
	
	idEntity *projector = NULL; //bc

	if (this->IsHidden())
		return false;

	if (g_skill.GetInteger() <= 0)
		return false;

	handle = gameLocal.pvs.SetupCurrentPVS( pvsArea );

	for ( i = 0; i < gameLocal.num_entities; i++ )
	{
		if ( !gameLocal.entities[ i ] )
			continue;

		//check if the classname matches.
		if (idStr::Icmp(  "env_projector", gameLocal.entities[ i ]->spawnArgs.GetString( "classname" )) == 0)
		{
			projector = gameLocal.entities[i];
			break;
		}
	}

	/*
	if (projector)
	{
		float leftLength = projector->spawnArgs.GetFloat("leftlength", "");
		float ceilingLength = projector->spawnArgs.GetFloat("ceilinglength", "");
		float rightLength = projector->spawnArgs.GetFloat("rightlength", "");
		idBounds bounds = idBounds( idVec3( -4, -rightLength, 0 ), idVec3( 4, leftLength, ceilingLength ) );


		bounds.RotateSelf(  projector->GetPhysics()->GetAxis().ToAngles().ToMat3() );
		
		gameRenderWorld->DebugBounds( colorMagenta, bounds, projector->GetPhysics()->GetOrigin(), gameLocal.msec );
	}
	*/


	for ( i = 0; i < gameLocal.numClients; i++ )
	{
		ent = static_cast<idPlayer*>(gameLocal.entities[ i ]);

		
		//if ( !ent || ( ent->fl.notarget ) )
		if ( !ent || ( ent->fl.notarget ) || ent->noclip)
		{
			continue;
		}

		// if there is no way we can see this player
		if ( !gameLocal.pvs.InCurrentPVS( handle, ent->GetPVSAreas(), ent->GetNumPVSAreas() ) ) {
			continue;
		}

		dir = ent->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
		dist = dir.Normalize();

		if ( dist > scanDist ) {
			continue;
		}

		if ( dir * GetAxis() < scanFovCos ) {
			continue;
		}

		idVec3 eye;

		eye = ent->EyeOffset();



		//gameRenderWorld->DebugLine( colorWhite, GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin() + eye + idVec3(0,0,-1), 1000 );
		//gameRenderWorld->DebugCircle( colorWhite, GetPhysics()->GetOrigin() + idVec3(0,0,-3.5f) /*close to where lens is*/, idVec3(0,0,1), 2, 5, 200);
		//gameRenderWorld->DebugCircle( colorWhite, GetPhysics()->GetOrigin() + (GetPhysics()->GetAxis().ToAngles().ToForward() * 6), idVec3(0,0,1), 2, 5, 200);

		gameLocal.clip.TracePoint( tr, GetLensPos()/*close to where lens is*/,
			ent->GetPhysics()->GetOrigin() + eye, /*MASK_OPAQUE*/MASK_SOLID, this );

		if (projector)
		{
			if (gameLocal.GetTraceEntity( tr ) == projector)
			{
				float projectorDot = DotProduct( this->GetPhysics()->GetAxis().ToAngles().ToForward(), projector->GetPhysics()->GetAxis().ToAngles().ToForward() );

				//if blocked by the projector screen...
				if (projectorDot > 0)
				{
					continue;
				}
				else
				{
					//projector is facing away from camera. Therefore we need the trace to now IGNORE the projector screen.
					gameLocal.clip.TracePoint( tr, GetPhysics()->GetOrigin() + idVec3(0,0,-3), ent->GetPhysics()->GetOrigin() + eye,
						MASK_SOLID, projector );
				}
			}
		}

		if ( tr.fraction >= 1.0 || ( gameLocal.GetTraceEntity( tr ) == ent ) )
		{
			gameLocal.pvs.FreeCurrentPVS( handle );
			this->targetDist = dist;
			this->lastSpottedEnt = ent;

			return true;
		}
	}

	gameLocal.pvs.FreeCurrentPVS( handle );

	this->lastSpottedEnt = NULL;
	return false;
}

/*
================
idSecurityCamera::SetAlertMode
================
*/
void idSecurityCamera::SetAlertMode( int alert ) {
	if (alert >= SCANNING && alert <= ACTIVATED) {
		alertMode = alert;
	}
	renderEntity.shaderParms[ SHADERPARM_MODE ] = alertMode;
	UpdateVisuals();
}

void idSecurityCamera::DrawBounds( void ) {
	int i;
	float radius, a, s, c, halfRadius;
	idVec3 right, up;
	idVec4 color(1, 1, 1, 0.1f), color2(1, 0.5f, 0, 1);
	idVec3 lastPoint, point, lastHalfPoint, halfPoint, center;

	idVec3 last3dPoint, first3dPoint;

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

	first3dPoint.Set( 0,0,0 );

	for ( i = 1; i < VIEWCONE_SPOKES; i++ )
	{
		trace_t tr;

		a = idMath::TWO_PI * i / (VIEWCONE_SPOKES * 1.0f);
		idMath::SinCos( a, s, c );
		point = dir + right * s * radius + up * c * radius;
		point.Normalize();
		point = GetPhysics()->GetOrigin() + point * scanDist;
		
		//gameRenderWorld->DebugLine( color, lastPoint, point ); //draw circle.	
		//gameRenderWorld->DebugLine( color, GetPhysics()->GetOrigin(), point ); //draw spokes.

		gameLocal.clip.TracePoint( tr, GetPhysics()->GetOrigin(), point, MASK_SOLID, this );

		if (i >= 2)
		{
			//gameRenderWorld->DebugLine( color, last3dPoint, tr.endpos );

			this->boundBeam[ i - 1 ]->SetOrigin( last3dPoint + dir * -4 );
			this->boundBeamTarget[ i - 1 ]->SetOrigin( tr.endpos + dir * -4 );
		}

		if ( first3dPoint.x == 0 && first3dPoint.y == 0 && first3dPoint.z == 0 )
		{
			first3dPoint.Set( tr.endpos.x, tr.endpos.y, tr.endpos.z );
		}

		last3dPoint = tr.endpos;


		lastPoint = point;

		halfPoint = dir + right * s * halfRadius + up * c * halfRadius;
		halfPoint.Normalize();
		halfPoint = GetPhysics()->GetOrigin() + halfPoint * scanDist;
		//gameRenderWorld->DebugLine( color2, point, halfPoint );
		//gameRenderWorld->DebugLine( color2, lastHalfPoint, halfPoint );
		lastHalfPoint = halfPoint;

		//gameRenderWorld->DebugLine( color2, halfPoint, center );

		
	}

	this->boundBeam[ VIEWCONE_SPOKES - 1 ]->SetOrigin( last3dPoint + dir * -4 );
	this->boundBeamTarget[ VIEWCONE_SPOKES - 1 ]->SetOrigin( first3dPoint + dir * -4 );
	//gameRenderWorld->DebugLine( color, first3dPoint, last3dPoint );
}

/*
================
idSecurityCamera::Think
================
*/
void idSecurityCamera::Think( void ) {
	float pct;
	float travel;

	if ( thinkFlags & TH_THINK ) {
		if ( g_showEntityInfo.GetBool() ) {
			DrawFov();
		}

		DrawBounds();

		if (health <= 0) {
			BecomeInactive( TH_THINK );
			return;
		}
	}

	// run physics
	RunPhysics();

	

	if ( thinkFlags & TH_THINK )	
	{
		if (!this->camActive)
		{
			//do nothing.
			if (gameLocal.time > this->softAlarmTimer && !this->softAlarmActivated)
			{
				uiAlarmTimer = gameLocal.time + UIALARMTIME;

				//trigger soft alarm.
				this->softAlarmActivated = true;

				
				gameLocal.GetLocalPlayer()->Event_hudMessage( va( common->GetLanguageDict()->GetString( "#str_01203" ), this->GetName() ) );

				idStr funcname = spawnArgs.GetString( "softalarmtarget", "" );
				if ( funcname.Length() )
				{
					idEntity	*softAlarmEnt;
					softAlarmEnt = gameLocal.FindEntity( funcname );
					if ( softAlarmEnt->RespondsTo( EV_Activate ) || softAlarmEnt->HasSignal( SIG_TRIGGER ) )
					{
						softAlarmEnt->Signal( SIG_TRIGGER );
						softAlarmEnt->ProcessEvent( &EV_Activate, this );
					}
				}
			}

			Present();
			return;
		}


		if (CanSeePlayer())
		{
			if (alertMode == SCANNING)
			{
				float	sightTime;		
				int i;

				SetAlertMode(ALERT);
				stopSweeping = gameLocal.time;
				
				if (sweeping)
				{
					CancelEvents( &EV_SecurityCam_Pause );
				}
				else
				{
					CancelEvents( &EV_SecurityCam_ReverseSweep );
				}

				sweeping = false;
				StopSound( SND_CHANNEL_ANY, false );
				StartSound( "snd_sight", SND_CHANNEL_BODY, 0, false, NULL );


				renderEntity.shaderParms[ SHADERPARM_RED ] = 1;
				renderEntity.shaderParms[ SHADERPARM_GREEN ] = .9f;
				renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;

				for ( i = 0; i < VIEWCONE_SPOKES; i++ )
				{
					//re-enable the boundbeam. Suspicious.

					// set skin.
					this->boundBeam[ i ]->SetSkin( declManager->FindSkin( "skins/beam_camera" ) );

					this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_RED ] = 1;
					this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_GREEN ] = 0.8f;
					this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_BLUE ] = 0;
				}

				//bc if player is very close, then player is spotted immediately.
				if (this->targetDist < this->closeTolerance)
				{
					sightTime = spawnArgs.GetFloat( "closeSightTime", "0.3" );					
				}
				else
				{
					sightTime = spawnArgs.GetFloat( "sightTime", "0.6" );
				}

				PostEventSec(&EV_SecurityCam_Alert, sightTime);
			}
		}
		else
		{
			if (alertMode == ALERT)
			{
				float	sightResume;

				SetAlertMode(LOSINGINTEREST);
				CancelEvents( &EV_SecurityCam_Alert );
				
				sightResume = spawnArgs.GetFloat( "sightResume", "3" );
				PostEventSec( &EV_SecurityCam_ContinueSweep, sightResume );
			}

			if ( sweeping && sweepAngle > 0)
			{
				idAngles a = GetPhysics()->GetAxis().ToAngles();

				pct = ( gameLocal.time - sweepStart ) / ( sweepEnd - sweepStart );
				travel = pct * sweepAngle;
				
				if ( negativeSweep )
				{
					a.yaw = angle + travel;
				}
				else
				{
					a.yaw = angle - travel;
				}

				/*
				//verify the camera isn't using some wacky angle.
				if (idMath::Fabs(a.yaw) > this->baseAngle + 2)
				{
					a.yaw = this->baseAngle + 2;
				}

				if (idMath::Fabs(a.yaw) < this->baseAngle - this->sweepAngle - 2)
				{
					a.yaw = this->baseAngle - this->sweepAngle - 2;
				}
				*/
				

				SetAngles( a );
			}
		}
	}
	Present();
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
float idSecurityCamera::SweepSpeed( void ) const {
	return spawnArgs.GetFloat( "sweepSpeed", "5" );
}

/*
================
idSecurityCamera::StartSweep
================
*/
void idSecurityCamera::StartSweep( void ) {
	int speed;

	sweeping = true;
	sweepStart = gameLocal.time;
	speed = SEC2MS( SweepSpeed() );
	sweepEnd = sweepStart + speed;
   	PostEventMS( &EV_SecurityCam_Pause, speed );

	if ( this->sweepAngle != 0 )
		StartSound( "snd_moving", SND_CHANNEL_BODY, 0, false, NULL );


}

void idSecurityCamera::Event_KillGhost( void )
{
	this->beam->Hide();
	this->ghostImage->Hide();
}

/*
================
idSecurityCamera::Event_ContinueSweep
================
*/
void idSecurityCamera::Event_ContinueSweep( void ) {
	int i;
	float pct = (stopSweeping - sweepStart) / (sweepEnd - sweepStart);

	float f = gameLocal.time - (sweepEnd - sweepStart) * pct;
	int speed;

	sweepStart = f;
	//speed = MS2SEC( SweepSpeed() ); 
	speed = SEC2MS( SweepSpeed() );	 //bc fixed bug.
	sweepEnd = sweepStart + speed;   	
	
	PostEventMS( &EV_SecurityCam_Pause, speed * (1.0 - pct));
	

	if ( this->sweepAngle != 0 )
		StartSound( "snd_moving", SND_CHANNEL_BODY, 0, false, NULL );

	SetAlertMode(SCANNING);
	sweeping = true;

	renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;

	for ( i = 0; i < VIEWCONE_SPOKES; i++ )
	{
		//re-enable the boundbeam.
		
		// set skin.
		this->boundBeam[ i ]->SetSkin( declManager->FindSkin( "skins/beam_camera" ) );

		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_RED ] = 0;
		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_GREEN ] = 1.0f;
		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_BLUE ] = 0;
	}

	StartSound( "snd_hum", SND_CHANNEL_DEMONIC, 0, false, NULL );
}

/*
================
idSecurityCamera::Event_Alert
================
*/
void idSecurityCamera::Event_Alert( void ) {
	float	wait;
	int i;

	//the camera has confirmed the suspicious thingamajig. Go red.
	if ( this->lastSpottedEnt.IsValid() )
	{
		CancelEvents( &EV_SecurityCam_KillGhost );

		this->beam->Show();
		this->ghostImage->Show();

		//draw the beam to the entity.
		this->beam->SetOrigin( GetLensPos() );
		this->beamTarget->SetOrigin( this->lastSpottedEnt.GetEntity()->GetPhysics()->GetAbsBounds().GetCenter() );
		this->ghostImage->SetOrigin( this->lastSpottedEnt.GetEntity()->GetPhysics()->GetOrigin() );

		//set the ghost angle.
		idAngles playerAng;
		playerAng.yaw = static_cast<idPlayer *>( this->lastSpottedEnt.GetEntity() )->viewAngles.yaw;
		playerAng.pitch = 0;
		playerAng.roll = 0;
		this->ghostImage->SetAxis( playerAng.ToMat3() );

		PostEventSec( &EV_SecurityCam_KillGhost, CAMERAGHOST_LIFETIME );

		//common->Printf("Event: CAUGHT BY CAMERA: %s\n", this->GetName() );
		gameLocal.GetLocalPlayer()->Event_hudMessage( va( common->GetLanguageDict()->GetString( "#str_01202" ), this->GetName() ) );
	}

	renderEntity.shaderParms[ SHADERPARM_RED ] = 1;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 0;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;

	for ( i = 0; i < VIEWCONE_SPOKES; i++ )
	{
		//re-enable the boundbeam. Caught player.

		// set skin.
		this->boundBeam[ i ]->SetSkin( declManager->FindSkin( "skins/beam_camera" ) );

		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_RED ] = 1;
		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_GREEN ] = 0;
		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_BLUE ] = 0;
	}

	SetAlertMode(ACTIVATED);
	StopSound( SND_CHANNEL_ANY, false );
	StartSound( "snd_activate", SND_CHANNEL_BODY, 0, false, NULL ); //"You've been spotted!" sound.
	ActivateTargets(this);
	CancelEvents( &EV_SecurityCam_ContinueSweep );

	wait = spawnArgs.GetFloat( "wait", "2" );
	PostEventSec( &EV_SecurityCam_ContinueSweep, wait );
}

/*
================
idSecurityCamera::Event_ReverseSweep
================
*/
void idSecurityCamera::Event_ReverseSweep( void ) {

	if (spawnArgs.GetBool("hidebeam"))
	{
		int i;
		for ( i = 0; i < VIEWCONE_SPOKES; i++ )
		{
			this->boundBeam[i]->Hide();
		}
	}

	angle = GetPhysics()->GetAxis().ToAngles().yaw;

	negativeSweep = !negativeSweep;
	StartSweep();
}

/*
================
idSecurityCamera::Event_Pause
================
*/
void idSecurityCamera::Event_Pause( void ) {
	float	sweepWait;

	sweepWait = spawnArgs.GetFloat( "sweepWait", "0.5" );
	sweeping = false;
	StopSound( SND_CHANNEL_BODY, false );
	StartSound( "snd_stop", SND_CHANNEL_BODY, 0, false, NULL );
   	PostEventSec( &EV_SecurityCam_ReverseSweep, sweepWait );
}



void idSecurityCamera::Event_camOff( void )
{
	int i;
	idDict args;

	if (!this->camActive)
		return;

	this->camActive = false;
	StopSound( SND_CHANNEL_ANY, false );
	CancelEvents( &EV_SecurityCam_ContinueSweep );
	CancelEvents( &EV_SecurityCam_Pause );
	CancelEvents( &EV_SecurityCam_ReverseSweep );
	CancelEvents( &EV_SecurityCam_Alert );

	this->stopSweeping = gameLocal.time;

	this->softAlarmTimer = gameLocal.time + 100 /*leeway time*/ + (spawnArgs.GetFloat( "softalarmtime", "3" ) * 1000.0f);


	for ( i = 0; i < VIEWCONE_SPOKES; i++ )
	{
		//this->boundBeamTarget[ i ]->Hide();
		//this->boundBeam[ i ]->Hide();

		//cam has been disabled.
		//this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_RED ] = 0.1f;
		//this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_GREEN ] = 0.1f;
		//this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_BLUE ] = 0.1f;		

		// set skin.
		this->boundBeam[ i ]->SetSkin( declManager->FindSkin( "skins/beam_camera_noscroll" ) );
	}

	//StopSound( SND_CHANNEL_ANY, false );
	StartSound( "snd_off", SND_CHANNEL_BODY2, 0, false, NULL );

	
	args.Clear();
	args.SetVector( "origin",		this->GetPhysics()->GetOrigin() +idVec3(0,0,-4) );
	args.Set(		"model",		"sparkburst3.prt" );
	args.SetBool(	"start_off",	false );
	gameLocal.SpawnEntityType( idExplodable::Type, &args );


	renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 0;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;
	UpdateVisuals();

	uiHackTimer = gameLocal.time + UIHACKTIME;
}

void idSecurityCamera::Event_camOn( void )
{
	int i;
	if (this->camActive)
		return;

	StopSound( SND_CHANNEL_ANY, false );
	


	renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;

	for ( i = 0; i < VIEWCONE_SPOKES; i++ )
	{
		//re-enable the boundbeam. Normal idle.

		// set skin.
		this->boundBeam[ i ]->SetSkin( declManager->FindSkin( "skins/beam_camera" ) );

		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_RED ] = 0;
		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_GREEN ] = 1.0f;
		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_BLUE ] = 0;
	}


	this->camActive = true;
	this->softAlarmActivated = false;
	Event_ContinueSweep();

	StartSound( "snd_on", SND_CHANNEL_BODY2, 0, false, NULL );

	StartSound( "snd_hum", SND_CHANNEL_DEMONIC, 0, false, NULL );
}










/*
============
idSecurityCamera::Killed
============
*/
void idSecurityCamera::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	sweeping = false;
	StopSound( SND_CHANNEL_ANY, false );
	const char *fx = spawnArgs.GetString( "fx_destroyed" );
	if ( fx[0] != '\0' ) {
		idEntityFx::StartFx( fx, NULL, NULL, this, true );
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
	physicsObj.DropToFloor();
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



void idSecurityCamera::reset( void )
{
	int i;

	CancelEvents( &EV_SecurityCam_Alert );

	this->beam->Hide();
	this->ghostImage->Hide();

	this->camActive = false;
	StopSound( SND_CHANNEL_ANY, false );

	renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;

	for ( i = 0; i < VIEWCONE_SPOKES; i++ )
	{
		//re-enable the boundbeam. Normal idle.

		// set skin.
		this->boundBeam[ i ]->SetSkin( declManager->FindSkin( "skins/beam_camera" ) );

		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_RED ] = 0;
		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_GREEN ] = 1.0f;
		this->boundBeam[ i ]->GetRenderEntity()->shaderParms[ SHADERPARM_BLUE ] = 0;
	}

	this->camActive = true;
	this->softAlarmActivated = false;

	alertMode = SCANNING;

	StartSound( "snd_hum", SND_CHANNEL_DEMONIC, 0, false, NULL );
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
