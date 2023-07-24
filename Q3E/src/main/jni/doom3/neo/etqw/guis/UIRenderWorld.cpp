// Copyright (C) 2007 Id Software, Inc.
//



#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserInterfaceManager.h"
#include "UIWindow.h"
#include "UserInterfaceLocal.h"
#include "UIRenderWorld.h"

#include "../../sys/sys_local.h"
#include "../../decllib/declTypeHolder.h"

idCVar g_debugGUIRenderWorld( "g_debugGUIRenderWorld", "0", CVAR_GAME | CVAR_BOOL, "Output information for GUI-based renderWorlds" );

SD_UI_IMPLEMENT_ABSTRACT_CLASS( sdUIRenderWorld_Child, sdUIObject )

SD_UI_IMPLEMENT_CLASS( sdUIRenderWorld, sdUIWindow )
SD_UI_IMPLEMENT_CLASS( sdUIRenderModel, sdUIRenderWorld_Child )
SD_UI_IMPLEMENT_CLASS( sdUIRenderLight, sdUIRenderWorld_Child )
SD_UI_IMPLEMENT_CLASS( sdUIRenderCamera, sdUIRenderWorld_Child )
SD_UI_IMPLEMENT_CLASS( sdUIRenderCamera_Animated, sdUIRenderWorld_Child )

const char sdUITemplateFunctionInstance_IdentifierRenderWorld[]	= "sdUIRenderWorldFunction";
idHashMap< sdUIRenderWorld::RenderWorldTemplateFunction* >	sdUIRenderWorld::renderWorldFunctions;

/*
============
sdUIRenderWorld::sdUIRenderWorld
============
*/
sdUIRenderWorld::sdUIRenderWorld() {
	world = renderSystem->AllocRenderWorld();
	world->InitFromMap( NULL ); // Initialize a single empty area for ambient lighting
	
	scriptState.GetProperties().RegisterProperty( "atmosphere", atmosphere );

	UI_ADD_STR_CALLBACK( atmosphere, sdUIRenderWorld, OnAtmosphereChanged )

	atmosphere = "";
}


/*
============
sdUIRenderWorld::~sdUIRenderWorld
============
*/
sdUIRenderWorld::~sdUIRenderWorld() {
	renderSystem->FreeRenderWorld( world ); 
	DisconnectGlobalCallbacks();
}

/*
============
sdUIRenderWorld::OnAtmosphereChanged
============
*/
void sdUIRenderWorld::OnAtmosphereChanged( const idStr& oldValue, const idStr& newValue ) {
	if( world != NULL ) {
		if( !newValue.IsEmpty() ) {
			const sdDeclAtmosphere* atmosphere = declHolder.FindAtmosphere( newValue.c_str() );
			world->SetAtmosphere( atmosphere );
			world->SetAreaAmbientCubeMap( 0, atmosphere->GetAmbientCubeMap() );
		} else {
			world->SetAtmosphere( NULL );
			world->SetAreaAmbientCubeMap( 0, NULL );
		}
	}
}

/*
============
sdUIRenderWorld::DrawLocal

	This should only be called from fullscreen guis
============
*/
void sdUIRenderWorld::DrawLocal() {
	sdUIWindow::DrawLocal();

	deviceContext->End();

	memset( &viewDef, 0, sizeof( viewDef ) );
	viewDef.flags.forceDefsVisible = true;

	viewDef.shaderParms[0] = 1.0f;
	viewDef.shaderParms[1] = 1.0f;
	viewDef.shaderParms[2] = 1.0f;
	viewDef.shaderParms[3] = 1.0f;	

	const float correction = deviceContext->GetAspectRatioCorrection();
	viewDef.x = cachedClientRect.x * correction;
	viewDef.y = cachedClientRect.y;
	viewDef.width = cachedClientRect.z * correction;
	viewDef.height = cachedClientRect.w;
	viewDef.time = ui->GetCurrentTime();
	
	Update_r( GetNode().GetChild() );

	if( viewDef.fov_x <= 0.0f ) {
		viewDef.fov_x = 90.0f;
	}

	if( viewDef.fov_y <= 0.0f ) {
		viewDef.fov_y = 90.0f;
	}	

	world->RenderScene( &viewDef );
	
	world->DebugClearLines( ui->GetCurrentTime() );
	world->DebugClearPolygons( ui->GetCurrentTime() );	

	deviceContext->BeginEmitFullScreen();
}

/*
============
sdUIRenderWorld::Update_r
============
*/
void sdUIRenderWorld::Update_r( sdUIObject* object ) {
	if( object == NULL ) {
		return;
	}

	if( sdUIRenderWorld_Child* window = object->Cast< sdUIRenderWorld_Child >() ) {
		window->SetRenderWorld( world );
		window->Update( viewDef );
	}

	sdUIObject* child = object->GetNode().GetChild();
	Update_r( child );
	Update_r( object->GetNode().GetSibling() );
}

/*
============
sdUIRenderWorld::FindFunction
============
*/
const sdUIRenderWorld::RenderWorldTemplateFunction* sdUIRenderWorld::FindFunction( const char* name ) {
	sdUIRenderWorld::RenderWorldTemplateFunction** ptr;
	return renderWorldFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUIRenderWorld::GetFunction
============
*/
sdUIFunctionInstance* sdUIRenderWorld::GetFunction( const char* name ) {
	const RenderWorldTemplateFunction* function = sdUIRenderWorld::FindFunction( name );
	if ( !function ) {		
		return sdUIWindow::GetFunction( name );
	}
	return new sdUITemplateFunctionInstance< sdUIRenderWorld, sdUITemplateFunctionInstance_IdentifierRenderWorld >( this, function );
}

/*
============
sdUIRenderWorld::RunNamedMethod
============
*/
bool sdUIRenderWorld::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const sdUIRenderWorld::RenderWorldTemplateFunction* func = sdUIRenderWorld::FindFunction( name );
	if ( !func ) {
		return sdUIWindow::RunNamedMethod( name, stack );
	}

	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}


/*
============
sdUIRenderWorld::InitFunctions
============
*/
void sdUIRenderWorld::InitFunctions() {
}







/*
============
sdUIRenderWorld_Child::sdUIRenderWorld_Child
============
*/
sdUIRenderWorld_Child::sdUIRenderWorld_Child() :
	renderWorld( NULL ), visible(1.f) {
	GetScope().GetProperties().RegisterProperty( "visible", visible );
	GetScope().GetProperties().RegisterProperty( "bindJoint", bindJoint );
}


/*
============
sdUIRenderModel::sdUIRenderModel
============
*/
sdUIRenderModel::sdUIRenderModel() {
	worldModel = renderModelManager->AllocModel();
	memset( &worldEntity, 0, sizeof( worldEntity ));

	modelOrigin		= vec3_origin;
	modelRotation	= vec3_origin;

	modelAnim		= NULL;
	animate			= 1.0f;

	modelDef		= -1;

	scriptState.GetProperties().RegisterProperty( "modelName", modelName );
	scriptState.GetProperties().RegisterProperty( "skinName", skinName );
	scriptState.GetProperties().RegisterProperty( "animName", animName );
	scriptState.GetProperties().RegisterProperty( "animClass", animClass );
	scriptState.GetProperties().RegisterProperty( "modelOrigin", modelOrigin );
	scriptState.GetProperties().RegisterProperty( "modelRotation", modelRotation );
	scriptState.GetProperties().RegisterProperty( "animate", animate );

	UI_ADD_STR_CALLBACK( modelName, sdUIRenderModel, OnModelInfoChanged )
	UI_ADD_STR_CALLBACK( skinName, sdUIRenderModel, OnModelInfoChanged )
	UI_ADD_STR_CALLBACK( animName, sdUIRenderModel, OnModelInfoChanged )
	UI_ADD_STR_CALLBACK( animClass, sdUIRenderModel, OnModelInfoChanged )
	UI_ADD_FLOAT_CALLBACK( visible, sdUIRenderModel, OnVisibleChanged )
}


/*
============
sdUIRenderModel::~sdUIRenderModel
============
*/
sdUIRenderModel::~sdUIRenderModel() {
	gameEdit->DestroyRenderEntity( worldEntity );
	renderModelManager->FreeModel( worldModel );
	DisconnectGlobalCallbacks();
}


/*
============
sdUIRenderModel::SpawnDefs
============
*/
void sdUIRenderModel::SpawnDefs() {
	if( renderWorld == NULL ) {
		return;
	}

	idDict spawnArgs;
	FreeModel();

	spawnArgs.Clear();

	memset( &worldEntity, 0, sizeof( worldEntity ));
	spawnArgs.Set( "classname", "func_static" );
	spawnArgs.Set( "model", modelName.GetValue() );
	spawnArgs.Set( "origin", modelOrigin.GetValue().ToString() );
	spawnArgs.Set( "anim", animName.GetValue() );
	spawnArgs.Set( "skin", skinName.GetValue() );

	if( animClass.GetValue().Length() > 0 ) {
		if( const idDeclEntityDef* def = gameLocal.declEntityDefType.LocalFind( animClass.GetValue(), false ) ) {
			spawnArgs.SetDefaults( &def->dict );
		}
	}

	gameEdit->ParseSpawnArgsToRenderEntity( spawnArgs, worldEntity );

	if ( worldEntity.hModel != NULL ) {
		worldEntity.axis = modelRotation.GetValue().ToMat3();
		worldEntity.shaderParms[ 0 ] = 1.0f;
		worldEntity.shaderParms[ 1 ] = 1.0f;
		worldEntity.shaderParms[ 2 ] = 1.0f;
		worldEntity.shaderParms[ 3 ] = 1.0f;
		worldEntity.bounds = worldEntity.hModel->Bounds();
		modelDef = renderWorld->AddEntityDef( &worldEntity );

		worldEntity.numJoints = worldEntity.hModel->NumJoints();
		worldEntity.joints = ( idJointMat * )Mem_AllocAligned( worldEntity.numJoints * sizeof( *worldEntity.joints ), ALIGN_16 );
		modelAnim = gameEdit->ANIM_GetAnimFromEntityDef( &spawnArgs );

	} else if( g_debugGUIRenderWorld.GetBool() ) {
		gameLocal.Warning( "sdUIRenderModel::SpawnDefs: '%s' couldn't find model '%s'", name.GetValue().c_str(), modelName.GetValue().c_str() );
	}	

	if( modelAnim != NULL ) {
		animLength = gameEdit->ANIM_GetLength( modelAnim );
		animEndTime = ui->GetCurrentTime() + animLength;
	} else if( g_debugGUIRenderWorld.GetBool() ) {
		gameLocal.Warning( "sdUIRenderModel::SpawnDefs: '%s' couldn't find anim '%s' in def '%s'", name.GetValue().c_str(), animName.GetValue().c_str(), animClass.GetValue().c_str() );
	}
}


/*
============
sdUIRenderModel::OnModelInfoChanged
============
*/
void sdUIRenderModel::OnModelInfoChanged( const idStr& oldValue, const idStr& newValue ) {
	if( SpawnInfoValid() ) {
		SpawnDefs();
	} else {
		FreeModel();
	}
}

/*
============
sdUIRenderModel::OnVisibleChanged
============
*/
void sdUIRenderModel::OnVisibleChanged( const float oldValue, const float newValue ) {
	if( newValue != 0.0f ) {
	} else {
		FreeModel();
	}
}

/*
============
sdUIRenderModel::SpawnInfoValid
============
*/
bool sdUIRenderModel::SpawnInfoValid() const {
	return ( modelName.GetValue().Length() != 0 && animName.GetValue().Length() != 0  );
}

/*
============
sdUIRenderModel::FreeModel
============
*/
void sdUIRenderModel::FreeModel() {
	if ( modelDef != -1 ) {
		renderWorld->FreeEntityDef( modelDef );
		modelDef = -1;
	}
}

/*
============
sdUIRenderModel::Update
============
*/
void sdUIRenderModel::Update( renderView_t& viewDef ) {	
	
	if( visible == 0.0f ) {
		return;
	}

	if( modelDef == -1 || worldEntity.hModel == NULL /*|| worldEntity.hModel->IsDefaultModel()*/ ) {
		return;
	}

	assert( renderWorld != NULL );

	int time = ui->GetCurrentTime();
	if( modelAnim != NULL ) {
		if( time > animEndTime ) {
			animEndTime = time + animLength;
		}

		gameEdit->ANIM_CreateAnimFrame( worldEntity.hModel, modelAnim, worldEntity.numJoints, worldEntity.joints, animLength - ( animEndTime - time ), vec3_origin, false );		
	}

	worldEntity.origin = modelOrigin;
	worldEntity.axis = idAngles( modelRotation.GetValue().x, modelRotation.GetValue().y, modelRotation.GetValue().z ).ToMat3();

	if( !bindJoint.GetValue().IsEmpty() ) {
		if( sdUIRenderModel* parent = GetNode().GetParent()->Cast< sdUIRenderModel >() ) {
			if( parent->worldEntity.hModel != NULL ) {
				jointHandle_t handle = parent->worldEntity.hModel->GetJointHandle( bindJoint.GetValue() );
				if( handle != INVALID_JOINT ) {
					worldEntity.origin = parent->worldEntity.origin + ( parent->worldEntity.joints[ handle ].ToVec3() * parent->worldEntity.axis );
					worldEntity.axis = parent->worldEntity.joints[ handle ].ToMat3() * parent->worldEntity.axis;

					//worldEntity.origin *= ;
					//worldEntity.axis *= ;
				}
			}
		}
	} 
	renderWorld->UpdateEntityDef( modelDef, &worldEntity );
}

/*
============
sdUIRenderModel::SetRenderWorld
============
*/
void sdUIRenderModel::SetRenderWorld( idRenderWorld* world ) {
	if( world == renderWorld ) {
		return;
	}

	if( renderWorld != world && world != NULL ) {
		FreeModel();
	}

	sdUIRenderWorld_Child::SetRenderWorld( world );

	if( renderWorld != NULL ) {
		SpawnDefs();
	}
}




/*
============
sdUIRenderLight::sdUIRenderLight
============
*/
sdUIRenderLight::sdUIRenderLight() {
	lightDef		= -1;	
	lightOrigin		= idVec3( -128.0f, 0.0f, 0.0f );
	lightColor		= idVec4( 1.0f, 1.0f, 1.0f, 1.0f );

	memset( &rLight, 0, sizeof( rLight ) );

	scriptState.GetProperties().RegisterProperty( "lightColor", lightColor );
	scriptState.GetProperties().RegisterProperty( "lightOrigin", lightOrigin );	

	UI_ADD_FLOAT_CALLBACK( visible, sdUIRenderLight, OnVisibleChanged )
}

/*
============
sdUIRenderLight::~sdUIRenderLight
============
*/
sdUIRenderLight::~sdUIRenderLight() {
	DisconnectGlobalCallbacks();
}

/*
============
sdUIRenderLight::Update
============
*/
void sdUIRenderLight::Update( renderView_t& viewDef ) {
	if( visible == 0.0f ) {
		return;
	}

	assert( renderWorld != NULL );

	if( lightDef == -1 ) {
		return;
	}
	rLight.origin = lightOrigin;
	rLight.shaderParms[ SHADERPARM_RED ]	= lightColor.GetValue().x;
	rLight.shaderParms[ SHADERPARM_GREEN ]	= lightColor.GetValue().y;
	rLight.shaderParms[ SHADERPARM_BLUE ]	= lightColor.GetValue().z;

	renderWorld->UpdateLightDef( lightDef, &rLight );		
}
/*
============
sdUIRenderLight::SpawnDefs
============
*/
void sdUIRenderLight::SpawnDefs() {
	assert( renderWorld != NULL );

	FreeLight();

	idDict spawnArgs;

	spawnArgs.Set( "classname", "light");
	spawnArgs.Set( "name", "light_1");
	spawnArgs.Set( "origin", lightOrigin.GetValue().ToString() );
	spawnArgs.Set( "_color", lightColor.GetValue().ToString() );

	memset( &rLight, 0, sizeof( rLight ) );
	gameEdit->ParseSpawnArgsToRenderLight( spawnArgs, rLight );

	lightDef = renderWorld->AddLightDef( &rLight );	
}

/*
============
sdUIRenderLight::FreeLight
============
*/
void sdUIRenderLight::FreeLight() {
	assert( renderWorld != NULL );
	if ( lightDef != -1 ) {
		renderWorld->FreeLightDef( lightDef );
		lightDef = -1;
	}
}

/*
============
sdUIRenderLight::OnVisibleChanged
============
*/
void sdUIRenderLight::OnVisibleChanged( const float oldValue, const float newValue ) {
	if( newValue != 0.0f ) {
	} else {
		FreeLight();
	}
}

/*
============
sdUIRenderLight::SetRenderWorld
============
*/
void sdUIRenderLight::SetRenderWorld( idRenderWorld* world ) {
	if( renderWorld != world && renderWorld != NULL ) {
		FreeLight();
	}

	sdUIRenderWorld_Child::SetRenderWorld( world );

	if( renderWorld != NULL ) {
		SpawnDefs();
	}
}

/*
============
sdUIRenderCamera::sdUIRenderCamera
============
*/
sdUIRenderCamera::sdUIRenderCamera() {
	scriptState.GetProperties().RegisterProperty( "cameraOrigin", cameraOrigin );
	scriptState.GetProperties().RegisterProperty( "cameraRotation", cameraRotation );
	scriptState.GetProperties().RegisterProperty( "active", active );
	scriptState.GetProperties().RegisterProperty( "fov", fov );
	scriptState.GetProperties().RegisterProperty( "aspectRatio", aspectRatio );

	cameraOrigin	= idVec3( -128.0f, 0.0f, 0.0f );
	cameraRotation	= vec3_zero;
	fov				= 90.0f;
	aspectRatio		= 4.0f / 3.0f;
	active			= 1.0f;

}

/*
============
sdUIRenderCamera::~sdUIRenderCamera
============
*/
sdUIRenderCamera::~sdUIRenderCamera() {

}

/*
============
sdUIRenderCamera::Update
============
*/
void sdUIRenderCamera::Update( renderView_t& viewDef ) {
	if( !active ) {
		return;
	}
	viewDef.vieworg	= cameraOrigin;
	viewDef.viewaxis.Identity();

	CalcFov( viewDef );
}

/*
============
sdUIRenderCamera::CalcFov
============
*/
void sdUIRenderCamera::CalcFov( renderView_t& viewDef ) const {
	const float correction = deviceContext->GetAspectRatioCorrection();
	gameLocal.CalcFov( fov, viewDef.fov_x, viewDef.fov_y, viewDef.width, viewDef.height, correction );
}

/*
============
sdUIRenderCamera_Animated::sdUIRenderCamera_Animated
============
*/
sdUIRenderCamera_Animated::sdUIRenderCamera_Animated() {
	scriptState.GetProperties().RegisterProperty( "active", active );
	scriptState.GetProperties().RegisterProperty( "cameraAnim", cameraAnim );
	scriptState.GetProperties().RegisterProperty( "cameraOrigin", cameraOrigin );

	UI_ADD_STR_CALLBACK( cameraAnim, sdUIRenderCamera_Animated, OnCameraAnimChanged )
	UI_ADD_VEC3_CALLBACK( cameraOrigin, sdUIRenderCamera_Animated, OnCameraOriginChanged )
	UI_ADD_FLOAT_CALLBACK( cycle, sdUIRenderCamera_Animated, OnCycleChanged )
}

/*
============
sdUIRenderCamera_Animated::~sdUIRenderCamera_Animated
============
*/
sdUIRenderCamera_Animated::~sdUIRenderCamera_Animated() {
}

/*
============
sdUIRenderCamera_Animated::Update
============
*/
void sdUIRenderCamera_Animated::Update( renderView_t& viewDef ) {
	if( !active ) {
		return;
	}	
	camera.Evaluate( viewDef.vieworg, viewDef.viewaxis, viewDef.fov_x, GetUI()->GetCurrentTime() );
	viewDef.fov_y = viewDef.fov_x;
}


/*
============
sdUIRenderCamera_Animated::OnCameraAnimChanged
============
*/
void sdUIRenderCamera_Animated::OnCameraAnimChanged( const idStr& oldValue, const idStr& newValue ) {
	camera.LoadAnim( newValue );
	camera.SetStartTime( ui->GetCurrentTime() );
}

/*
============
sdUIRenderCamera_Animated::OnCameraOriginChanged
============
*/
void sdUIRenderCamera_Animated::OnCameraOriginChanged( const idVec3& oldValue, const idVec3& newValue ) {
	camera.SetOffset( cameraOrigin );
}

/*
============
sdUIRenderCamera_Animated::OnCycleChanged
============
*/
void sdUIRenderCamera_Animated::OnCycleChanged( const float oldValue, const float newValue ) {
	camera.SetCycle( idMath::Ftoi( newValue ) );
}
