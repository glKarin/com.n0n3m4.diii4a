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
#include "idlib/geometry/JointTransform.h"
#include "framework/Game.h"
#include "ui/DeviceContext.h"
#include "ui/Window.h"
#include "ui/UserInterfaceLocal.h"
#include "Game_local.h"

#include "ui/RenderWindow.h"

idRenderWindow::idRenderWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g) {
	dc = d;
	gui = g;
	CommonInit();
}

idRenderWindow::idRenderWindow(idUserInterfaceLocal *g) : idWindow(g) {
	gui = g;
	CommonInit();
}

idRenderWindow::~idRenderWindow() {
	renderSystem->FreeRenderWorld( world );
}

void idRenderWindow::WriteToSaveGame( idSaveGame *savefile ) const
{
	idWindow::WriteToSaveGame( savefile );

	savefile->WriteRenderView( refdef ); // renderView_t refdef
	// needsRender = true; // idRenderWorld * world// regened in PreRender()

	savefile->WriteRenderEntity( worldEntity ); // renderEntity_t worldEntity
	savefile->WriteRenderLight( rLight ); // renderLight_t rLight

	savefile->WriteBool( modelAnim != nullptr ); // updateAnimation // const idMD5Anim * modelAnim // regened in BuildAnimation()

	savefile->WriteInt( worldModelDef ); // int worldModelDef
	savefile->WriteInt( lightDef ); // int lightDef
	savefile->WriteInt( modelDef ); // int modelDef

	modelName.WriteToSaveGame( savefile ); // idWinStr modelName
	savefile->WriteString( lastModelName ); // idString lastModelName
	animName.WriteToSaveGame( savefile ); // idWinStr animName
	savefile->WriteString( animClass ); // idString animClass
	lightOrigin.WriteToSaveGame( savefile ); // idWinVec4 lightOrigin
	lightColor.WriteToSaveGame( savefile ); // idWinVec4 lightColor
	modelOrigin.WriteToSaveGame( savefile ); // idWinVec4 modelOrigin
	modelRotate.WriteToSaveGame( savefile ); // idWinVec4 modelRotate
	viewOffset.WriteToSaveGame( savefile ); // idWinVec4 viewOffset
	needsRender.WriteToSaveGame( savefile ); // idWinBool needsRender
	savefile->WriteInt( animLength ); // int animLength
	savefile->WriteInt( animEndTime ); // int animEndTime
	savefile->WriteBool( updateAnimation ); // bool updateAnimation
	skinName.WriteToSaveGame( savefile ); // idWinStr skinName
	modelFov.WriteToSaveGame( savefile ); // idWinFloat modelFov
}

void idRenderWindow::ReadFromSaveGame( idRestoreGame *savefile )
{
	idWindow::ReadFromSaveGame( savefile );

	savefile->ReadRenderView( refdef ); // renderView_t refdef
	world->InitFromMap( NULL ); // needsRender = true // idRenderWorld * world // already alloced

	savefile->ReadRenderEntity( worldEntity ); // renderEntity_t worldEntity
	savefile->ReadRenderLight( rLight ); // renderLight_t rLight

	bool forceUpdateAnimation;
	savefile->ReadBool( forceUpdateAnimation ); // const idMD5Anim * modelAnim // regened in BuildAnimation()

	savefile->ReadInt( worldModelDef ); // int worldModelDef // blendo eric: this isn't used (modelDef is)?

	savefile->ReadInt( lightDef ); // int lightDef
	if ( lightDef != - 1 ) {
		gameRenderWorld->UpdateLightDef( lightDef, &rLight );
	}

	savefile->ReadInt( modelDef ); // int modelDef
	if ( modelDef != - 1 ) {
		gameRenderWorld->UpdateEntityDef( modelDef, &worldEntity );
	}

	modelName.ReadFromSaveGame( savefile ); // idWinStr modelName
	savefile->ReadString( lastModelName ); // idString lastModelName
	animName.ReadFromSaveGame( savefile ); // idWinStr animName
	savefile->ReadString( animClass ); // idString animClass
	lightOrigin.ReadFromSaveGame( savefile ); // idWinVec4 lightOrigin
	lightColor.ReadFromSaveGame( savefile ); // idWinVec4 lightColor
	modelOrigin.ReadFromSaveGame( savefile ); // idWinVec4 modelOrigin
	modelRotate.ReadFromSaveGame( savefile ); // idWinVec4 modelRotate
	viewOffset.ReadFromSaveGame( savefile ); // idWinVec4 viewOffset
	needsRender.ReadFromSaveGame( savefile ); // idWinBool needsRender
	savefile->ReadInt( animLength ); // int animLength
	savefile->ReadInt( animEndTime ); // int animEndTime
	savefile->ReadBool( updateAnimation ); // bool updateAnimation
	skinName.ReadFromSaveGame( savefile ); // idWinStr skinName
	modelFov.ReadFromSaveGame( savefile ); // idWinFloat modelFov


	// BuildAnimation(gameLocal.hudTime); // crashes without world.hmodel, use updateanim
	updateAnimation = updateAnimation || forceUpdateAnimation;
}

void idRenderWindow::CommonInit() {
	memset(&refdef, 0, sizeof(refdef));
	memset(&worldEntity, 0, sizeof(worldEntity));
	memset(&rLight, 0, sizeof(rLight));

	world = renderSystem->AllocRenderWorld();
	needsRender = true;
	lightDef = -1;
	lightOrigin = idVec4(-128.0f, 0.0f, 0.0f, 1.0f);
	lightColor = idVec4(1.0f, 1.0f, 1.0f, 1.0f);
	modelOrigin.Zero();
	viewOffset = idVec4(-128.0f, 0.0f, 0.0f, 1.0f);
	modelAnim = NULL;
	animLength = 0;
	animEndTime = -1;
	modelDef = -1;
	worldModelDef = -1;
	updateAnimation = true;

	//BC
	modelFov = 90;
}


void idRenderWindow::BuildAnimation(int time) {

	if (!updateAnimation) {
		return;
	}

	if (animName.Length() && animClass.Length()) {
		worldEntity.numJoints = worldEntity.hModel->NumJoints();
		worldEntity.joints = ( idJointMat * )Mem_Alloc16( worldEntity.numJoints * sizeof( *worldEntity.joints ) );
		modelAnim = gameEdit->ANIM_GetAnimFromEntityDef(animClass, animName);
		if (modelAnim) {
			animLength = gameEdit->ANIM_GetLength(modelAnim);
			animEndTime = time + animLength;
		}
	}
	updateAnimation = false;

}

void idRenderWindow::PreRender() {
	if (needsRender || idStr::Cmp(lastModelName.c_str(), modelName) != 0) {
		world->InitFromMap( NULL );
		idDict spawnArgs;
		spawnArgs.Set("classname", "light");
		spawnArgs.Set("name", "light_1");
		spawnArgs.Set("origin", lightOrigin.ToVec3().ToString());
		spawnArgs.Set("_color", lightColor.ToVec3().ToString());
		gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &rLight );
		lightDef = world->AddLightDef( &rLight );
		if ( !modelName[0] ) {
			common->Warning( "Window '%s' in gui '%s': no model set", GetName(), GetGui()->GetSourceFile() );
		}
		memset( &worldEntity, 0, sizeof( worldEntity ) );
		spawnArgs.Clear();
		spawnArgs.Set("classname", "func_static");
		spawnArgs.Set("model", modelName);
		lastModelName = modelName.c_str();
		spawnArgs.Set("skin", skinName.c_str()); //BC allow gui models to have skin
		spawnArgs.Set("origin", modelOrigin.c_str());
		gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &worldEntity );
		if ( worldEntity.hModel ) {
			idVec3 v = modelRotate.ToVec3();
			worldEntity.axis = v.ToMat3();
			worldEntity.shaderParms[0] = 1;
			worldEntity.shaderParms[1] = 1;
			worldEntity.shaderParms[2] = 1;
			worldEntity.shaderParms[3] = 1;
			modelDef = world->AddEntityDef( &worldEntity );
		}
		needsRender = false;
		updateAnimation = true;
	}
}

void idRenderWindow::Render( int time ) {
	rLight.origin = lightOrigin.ToVec3();
	rLight.shaderParms[SHADERPARM_RED] = lightColor.x();
	rLight.shaderParms[SHADERPARM_GREEN] = lightColor.y();
	rLight.shaderParms[SHADERPARM_BLUE] = lightColor.z();
	world->UpdateLightDef(lightDef, &rLight);
	if ( worldEntity.hModel ) {
		if (updateAnimation) {
			BuildAnimation(time);
		}
		if (modelAnim) {
			if (time > animEndTime) {
				animEndTime = time + animLength;
			}
			gameEdit->ANIM_CreateAnimFrame(worldEntity.hModel, modelAnim, worldEntity.numJoints, worldEntity.joints, animLength - (animEndTime - time), vec3_origin, false );
		}
		worldEntity.axis = idAngles(modelRotate.x(), modelRotate.y(), modelRotate.z()).ToMat3();
		world->UpdateEntityDef(modelDef, &worldEntity);
	}
}


void idRenderWindow::Draw(int time, float x_, float y_) {
	PreRender();
	Render(time);

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.vieworg = viewOffset.ToVec3();;
	//refdef.vieworg.Set(-128, 0, 0);

	refdef.viewaxis.Identity();
	refdef.shaderParms[0] = 1;
	refdef.shaderParms[1] = 1;
	refdef.shaderParms[2] = 1;
	refdef.shaderParms[3] = 1;

	// DG: for scaling menus to 4:3 (like that spinning mars globe in the main menu)
 	float x = drawRect.x;
 	float y = drawRect.y;
 	float w = drawRect.w;
 	float h = drawRect.h;
 	if(dc->IsMenuScaleFixActive()) {
 		dc->AdjustCoords(&x, &y, &w, &h);
 	}
 
 	refdef.x = x;
 	refdef.y = y;
 	refdef.width = w;
 	refdef.height = h;
 	// DG end
	//refdef.fov_x = modelFov;
	//refdef.fov_y = 2 * atan((float)drawRect.h / drawRect.w) * idMath::M_RAD2DEG;
	refdef.fov_x = modelFov;
	refdef.fov_y = modelFov;

	refdef.time = time;
	world->RenderScene(&refdef);
}

void idRenderWindow::PostParse() {
	updateAnimation = true;
	idWindow::PostParse();
}

//
//
idWinVar *idRenderWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t** owner ) {
//
	if (idStr::Icmp(_name, "model") == 0) {
		return &modelName;
	}
	if (idStr::Icmp(_name, "anim") == 0) {
		return &animName;
	}
	if (idStr::Icmp(_name, "lightOrigin") == 0) {
		return &lightOrigin;
	}
	if (idStr::Icmp(_name, "lightColor") == 0) {
		return &lightColor;
	}
	if (idStr::Icmp(_name, "modelOrigin") == 0) {
		return &modelOrigin;
	}
	if (idStr::Icmp(_name, "modelRotate") == 0) {
		return &modelRotate;
	}
	if (idStr::Icmp(_name, "viewOffset") == 0) {
		return &viewOffset;
	}
	if (idStr::Icmp(_name, "needsRender") == 0) {
		return &needsRender;
	}

	//BC allow gui models to use skins
	if (idStr::Icmp(_name, "modelskin") == 0)
	{
		return &skinName;
	}

	if (idStr::Icmp(_name, "modelFov") == 0)
	{
		return &modelFov;
	}


//
//
	return idWindow::GetWinVarByName(_name, fixup, owner);
//
}

bool idRenderWindow::ParseInternalVar(const char *_name, idParser *src) {
	if (idStr::Icmp(_name, "animClass") == 0) {
		updateAnimation = true;
		ParseString(src, animClass);
		return true;
	}
	return idWindow::ParseInternalVar(_name, src);
}
