/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "RenderWindow.h"

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

void idRenderWindow::CommonInit() {
	world = renderSystem->AllocRenderWorld();
	needsRender = true;
	lightOrigin = idVec4(-128.0f, 0.0f, 0.0f, 1.0f);
	lightColor = idVec4(1.0f, 1.0f, 1.0f, 1.0f);
	modelOrigin.Zero();
	viewOffset = idVec4(-128.0f, 0.0f, 0.0f, 1.0f);
	modelAnim = NULL;
	animLength = 0;
	animEndTime = -1;
	modelDef = -1;
	updateAnimation = true;
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
	if (needsRender) {
		world->InitFromMap( NULL );
		idDict spawnArgs;
		spawnArgs.Set("classname", "light");
		spawnArgs.Set("name", "light_1");
		spawnArgs.Set("origin", lightOrigin.ToVec3().ToString());
		spawnArgs.Set( "_color", lightColor.ToVec3().ToString() );
		for ( auto var : definedVars )
			if ( !strcmp( var->GetName(), "noshadows" ) )
				spawnArgs.Set( "noshadows", "1" );
		gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &rLight );
		lightDef = world->AddLightDef( &rLight );
		if ( !modelName[0] ) {
			common->Warning( "Window '%s' in gui '%s': no model set", GetName(), GetGui()->GetSourceFile() );
		}
		memset( &worldEntity, 0, sizeof( worldEntity ) );
		spawnArgs.Clear();
		spawnArgs.Set("classname", "func_static");
		spawnArgs.Set("model", modelName);
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




void idRenderWindow::Draw(int time, float x, float y) {
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

	refdef.x = drawRect.x;
	refdef.y = drawRect.y;
	refdef.width = drawRect.w;
	refdef.height = drawRect.h;
	refdef.fov_x = 90;
	refdef.fov_y = 2 * atan((float)drawRect.h / drawRect.w) * idMath::M_RAD2DEG;
	// stgatilov: don't clear color buffer, render this on top of previous contents
	// note that right now only compass is rendered this way
	refdef.isOverlay = true;

	refdef.time = time;
	world->RenderScene(refdef);
}

void idRenderWindow::PostParse() {
	idWindow::PostParse();
}

// 
//  
idWinVar *idRenderWindow::GetThisWinVarByName(const char *varname) {
// 
	if (idStr::Icmp(varname, "model") == 0) {
		return &modelName;
	}
	if (idStr::Icmp(varname, "anim") == 0) {
		return &animName;
	}
	if (idStr::Icmp(varname, "lightOrigin") == 0) {
		return &lightOrigin;
	}
	if (idStr::Icmp(varname, "lightColor") == 0) {
		return &lightColor;
	}
	if (idStr::Icmp(varname, "modelOrigin") == 0) {
		return &modelOrigin;
	}
	if (idStr::Icmp(varname, "modelRotate") == 0) {
		return &modelRotate;
	}
	if (idStr::Icmp(varname, "viewOffset") == 0) {
		return &viewOffset;
	}
	if (idStr::Icmp(varname, "needsRender") == 0) {
		return &needsRender;
	}

// 
//  
	return idWindow::GetThisWinVarByName(varname);
// 
}

bool idRenderWindow::ParseInternalVar(const char *_name, idParser *src) {
	if (idStr::Icmp(_name, "animClass") == 0) {
		ParseString(src, animClass);
		return true;
	}
	return idWindow::ParseInternalVar(_name, src);
}
