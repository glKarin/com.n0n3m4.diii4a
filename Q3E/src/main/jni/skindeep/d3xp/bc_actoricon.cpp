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
#include "renderer/ModelManager.h"

#include "gamesys/SysCvar.h"
#include "Player.h"
#include "Actor.h"

#include "bc_actoricon.h"

const float SPRITESIZE = 40;
const int SPRITE_VERTICAL_OFFSET = 36;

const int TIMER_DISPLAYTIME = 2000;
const int TIMER_SPAWNTIME = 300;
const int TIMER_DESPAWNTIME = 1000;




/*
===============
idActorIcon::idPlayerIcon
===============
*/
idActorIcon::idActorIcon() {
	iconHandle	= -1;
	iconType	= ACTORICON_NONE;
	timer = 0;
	iconState = ICONSTATE_NONE;
}

/*
===============
idActorIcon::~idPlayerIcon
===============
*/
idActorIcon::~idActorIcon() {
	FreeIcon();
}

/*
===============
idActorIcon::Draw
===============
*/
void idActorIcon::Draw( idActor *actor, jointHandle_t joint, int _aiState) {
	idVec3 origin;
	idMat3 axis;
	actorIconType_t _iconType;

	if ( joint == INVALID_JOINT ) {
		FreeIcon();
		return;
	}

	actor->GetJointWorldTransform( joint, gameLocal.time, origin, axis );
	origin.z += SPRITE_VERTICAL_OFFSET;


	if (actor->health <= 0)
	{
		_iconType = ACTORICON_NONE;
	}
	else if (_aiState == AISTATE_COMBAT)
	{
		_iconType = ACTORICON_ALERTED;
	}
	else if (_aiState == AISTATE_SEARCHING)
	{
		_iconType = ACTORICON_SEARCHING;
	}
	else if (_aiState == AISTATE_STUNNED)
	{
		_iconType = ACTORICON_STUNNED;
	}
	else
	{
		_iconType = ACTORICON_NONE;
	}

	Draw(actor, origin, _iconType);
}

/*
===============
idActorIcon::Draw
===============
*/
void idActorIcon::Draw( idActor *actor, const idVec3 &origin, actorIconType_t _icontype)
{
	idPlayer *localPlayer = gameLocal.GetLocalPlayer();

	if (!actor || !actor->GetRenderView())
	{
		FreeIcon();
		return;
	}

	if (!localPlayer || !localPlayer->GetRenderView()) {
		FreeIcon();
		return;
	}

	idMat3 axis = localPlayer->GetRenderView()->viewaxis;

	//idMat3 axis = actor->GetRenderView()->viewaxis;	

	if (_icontype != ACTORICON_NONE)
	{
		if (!CreateIcon(actor, _icontype, origin, axis))
			UpdateIcon(actor, origin, axis);
	}
	else
	{
		//Delete icon.
		FreeIcon();
	}
}

/*
===============
idActorIcon::FreeIcon
===============
*/
void idActorIcon::FreeIcon( void ) {
	if ( iconHandle != - 1 ) {
		gameRenderWorld->FreeEntityDef( iconHandle );
		iconHandle = -1;
	}
	iconType = ACTORICON_NONE;
}

/*
===============
idActorIcon::CreateIcon
===============
*/

//default call.

bool idActorIcon::CreateIcon(idActor *actor, actorIconType_t type, const idVec3 &origin, const idMat3 &axis ) {
	assert( type != ACTORICON_NONE );
	const char *mtr = actor->spawnArgs.GetString( iconKeys[ type ], "_default" );
	return CreateIcon(actor, type, mtr, origin, axis );
}

/*
===============
idActorIcon::CreateIcon
===============
*/

//Call this if you have custom material.

bool idActorIcon::CreateIcon( idActor *actor, actorIconType_t type, const char *mtr, const idVec3 &origin, const idMat3 &axis )
{
	assert( type != ACTORICON_NONE );

	if ( type == iconType ) {
		return false;
	}

	FreeIcon();

	memset( &renderEnt, 0, sizeof( renderEnt ) );
	renderEnt.origin	= origin;
	renderEnt.axis		= axis;
	renderEnt.shaderParms[ SHADERPARM_RED ]				= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_GREEN ]			= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_BLUE ]			= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_ALPHA ]			= 0;
	renderEnt.shaderParms[ SHADERPARM_SPRITE_WIDTH ]	= SPRITESIZE;
	renderEnt.shaderParms[ SHADERPARM_SPRITE_HEIGHT ]	= SPRITESIZE;
	renderEnt.hModel = renderModelManager->FindModel( "_sprite" );
	renderEnt.callback = NULL;
	renderEnt.numJoints = 0;
	renderEnt.joints = NULL;
	renderEnt.customSkin = 0;
	renderEnt.noShadow = true;
	renderEnt.noSelfShadow = true;
	renderEnt.customShader = declManager->FindMaterial( mtr );
	renderEnt.referenceShader = 0;
	renderEnt.bounds = renderEnt.hModel->Bounds( &renderEnt );

	
	renderEnt.weaponDepthHack = true; 
	

	iconHandle = gameRenderWorld->AddEntityDef( &renderEnt );
	iconType = type;

	//BC Start the timer.
	timer = gameLocal.time + TIMER_SPAWNTIME;
	iconState = ICONSTATE_SPAWNING;

	return true;
}

/*
===============
idActorIcon::UpdateIcon
===============
*/
void idActorIcon::UpdateIcon( idActor *actor, const idVec3 &origin, const idMat3 &axis ) {

	if (iconState == ICONSTATE_DONE)
		return;

	assert( iconHandle >= 0 );

	renderEnt.origin = origin;
	renderEnt.axis	= axis;
	//renderEnt.forceUpdate = true;

	if (iconState == ICONSTATE_SPAWNING)
	{
		float lerp = (timer - gameLocal.time) / (float)TIMER_SPAWNTIME;
		float popLerp;

		if (lerp < 0)
			lerp = 0;

		lerp = 1.0f - lerp;
		popLerp = idMath::PopLerp(-24, 16, 0, lerp);		

		renderEnt.shaderParms[SHADERPARM_ALPHA] = lerp;		
		renderEnt.origin.z += popLerp;		

		if (gameLocal.time >= timer)
		{
			iconState = ICONSTATE_IDLE;

			if (iconType == ACTORICON_STUNNED)
			{
				//Displaytime varies depending on how long stun state is.
				timer = gameLocal.time + actor->stunTime;
			}
			else
			{
				//Normal display time.
				timer = gameLocal.time + TIMER_DISPLAYTIME;
			}
		}
	}
	else if (iconState == ICONSTATE_IDLE)
	{
		if (iconType == ACTORICON_STUNNED)
		{
			int delta = timer - gameLocal.time;
			float lerp = delta / (float)actor->stunTime;
			lerp = idMath::ClampFloat(0, 1, lerp);			
			lerp = 1.0f - lerp;

			renderEnt.shaderParms[7] = lerp;
		}

		if (gameLocal.time > timer)
		{
			iconState = ICONSTATE_DESPAWNING;
			timer = gameLocal.time + TIMER_DESPAWNTIME;			
		}
	}
	else if (iconState == ICONSTATE_DESPAWNING)
	{
		float lerp = (timer - gameLocal.time) / (float)TIMER_DESPAWNTIME;

		if (lerp < 0)
			lerp = 0;

		renderEnt.shaderParms[SHADERPARM_ALPHA] = lerp;

		if (gameLocal.time > timer)
		{
			iconState = ICONSTATE_DONE;

			//Free the icon.
			if (iconHandle != -1)
			{
				gameRenderWorld->FreeEntityDef(iconHandle);
				iconHandle = -1;
			}
		}
	}


	if (iconHandle > -1)
	{
		gameRenderWorld->UpdateEntityDef(iconHandle, &renderEnt);

	}

	
	
}
