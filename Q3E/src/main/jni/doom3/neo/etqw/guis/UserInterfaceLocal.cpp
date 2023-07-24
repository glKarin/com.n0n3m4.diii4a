// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserInterfaceLocal.h"
#include "UserInterfaceExpressions.h"
#include "UserInterfaceManagerLocal.h"
#include "UIWindow.h"

#include "../../sys/sys_local.h"

using namespace sdProperties;

idCVar sdUserInterfaceLocal::g_debugGUIEvents( "g_debugGUIEvents", "0", CVAR_GAME | CVAR_INTEGER, "Show the results of events" );
idCVar sdUserInterfaceLocal::g_debugGUI( "g_debugGUI", "0", CVAR_GAME | CVAR_INTEGER, "1 - Show GUI window outlines\n2 - Show GUI window names\n3 - Only show visible windows" );
idCVar sdUserInterfaceLocal::g_debugGUITextRect( "g_debugGUITextRect", "0", CVAR_GAME | CVAR_BOOL, "Show windows' text rectangle outlines" );
idCVar sdUserInterfaceLocal::g_debugGUITextScale( "g_debugGUITextScale", "24", CVAR_GAME | CVAR_FLOAT, "Size that the debug GUI info font is drawn in." );
idCVar sdUserInterfaceLocal::s_volumeMusic_dB( "s_volumeMusic_dB", "0", CVAR_GAME | CVAR_SOUND | CVAR_ARCHIVE | CVAR_FLOAT, "music volume in dB" );

#ifdef SD_PUBLIC_BETA_BUILD
idCVar sdUserInterfaceLocal::g_skipIntro( "g_skipIntro", "1", CVAR_GAME | CVAR_BOOL | CVAR_ROM, "skip the opening intro movie" );
#else
idCVar sdUserInterfaceLocal::g_skipIntro( "g_skipIntro", "0", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE, "skip the opening intro movie" );
#endif

// Crosshair
idCVar sdUserInterfaceLocal::gui_crosshairDef( "gui_crosshairDef", "crosshairs", CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "name of def containing crosshair" );
idCVar sdUserInterfaceLocal::gui_crosshairKey( "gui_crosshairKey", "pin_01", CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "name of crosshair key in def specified by gui_crosshairDef" );
idCVar sdUserInterfaceLocal::gui_crosshairAlpha( "gui_crosshairAlpha", "0.5", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of crosshair" );
idCVar sdUserInterfaceLocal::gui_crosshairSpreadAlpha( "gui_crosshairSpreadAlpha", "0.5", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of spread components" );
idCVar sdUserInterfaceLocal::gui_crosshairStatsAlpha( "gui_crosshairStatsAlpha", "0", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of health/ammo/reload components" );
idCVar sdUserInterfaceLocal::gui_crosshairGrenadeAlpha( "gui_crosshairGrenadeAlpha", "0.5", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of grenade timer components" );
idCVar sdUserInterfaceLocal::gui_crosshairSpreadScale( "gui_crosshairSpreadScale", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "amount to scale the spread indicator movement" );
idCVar sdUserInterfaceLocal::gui_crosshairColor( "gui_crosshairColor", "1 1 1 1", CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "RGB color tint for crosshair elements" );

// HUD
idCVar sdUserInterfaceLocal::gui_chatAlpha( "gui_chatAlpha", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of chat text" );
idCVar sdUserInterfaceLocal::gui_fireTeamAlpha( "gui_fireTeamAlpha", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of fireteam list" );
idCVar sdUserInterfaceLocal::gui_commandMapAlpha( "gui_commandMapAlpha", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of command map" );
idCVar sdUserInterfaceLocal::gui_objectiveListAlpha( "gui_objectiveListAlpha", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of objective list" );
idCVar sdUserInterfaceLocal::gui_personalBestsAlpha( "gui_personalBestsAlpha", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of personal bests display list" );
idCVar sdUserInterfaceLocal::gui_objectiveStatusAlpha( "gui_objectiveStatusAlpha", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of objective status" );
idCVar sdUserInterfaceLocal::gui_obitAlpha( "gui_obitAlpha", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of obituaries" );
idCVar sdUserInterfaceLocal::gui_voteAlpha( "gui_voteAlpha", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of vote" );
idCVar sdUserInterfaceLocal::gui_tooltipAlpha( "gui_tooltipAlpha", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of tooltips" );
idCVar sdUserInterfaceLocal::gui_vehicleAlpha( "gui_vehicleAlpha", "1", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of vehicle information" );
idCVar sdUserInterfaceLocal::gui_vehicleDirectionAlpha( "gui_vehicleDirectionAlpha", "0.5", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "alpha of vehicle direction indicators" );
idCVar sdUserInterfaceLocal::gui_showRespawnText( "gui_showRespawnText", "1", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE, "show text about respawning when in limbo or dead" );

idCVar sdUserInterfaceLocal::gui_tooltipDelay( "gui_tooltipDelay", "0.7", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "Delay in seconds before tooltips pop up." );
idCVar sdUserInterfaceLocal::gui_doubleClickTime( "gui_doubleClickTime", "0.2", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "Delay in seconds between considering two mouse clicks a double-click" );

idStrList sdUserInterfaceLocal::parseStack;
const int sdUserInterfaceLocal::TOOLTIP_MOVE_TOLERANCE = 2;

idBlockAlloc< uiCachedMaterial_t, 64 > sdUserInterfaceLocal::materialCacheAllocator;

const char* sdUserInterfaceLocal::partNames[ FP_MAX ] = {
	"tl",
	"t",
	"tr",
	"l",
	"r",
	"bl",
	"b",
	"br",
	"c",
};


/*
===============================================================================

	sdPropertyBinder

===============================================================================
*/

/*
================
sdPropertyBinder::ClearPropertyExpression
================
*/
void sdPropertyBinder::ClearPropertyExpression( int propertyKey, int propertyIndex ) {
	boundProperty_t& bp = indexedProperties[ propertyKey ];
	int index = bp.second + propertyIndex;	

	if ( !propertyExpressions[ index ] ) {
		return;
	}

	int count = sdProperties::CountForPropertyType( propertyExpressions[ index ]->GetType() );
	int i;
	for ( i = 0; i < count; i++ ) {
		if ( !propertyExpressions[ index + i ] ) {
			continue;
		}

		propertyExpressions[ index + i ]->Detach();
		propertyExpressions[ index + i ] = NULL;
	}
}

/*
================
sdPropertyBinder::Clear
================
*/
void sdPropertyBinder::Clear( void ) {
	indexedProperties.Clear();
	propertyExpressions.Clear();
}

/*
================
sdPropertyBinder::IndexForProperty
================
*/
int sdPropertyBinder::IndexForProperty( sdProperties::sdProperty* property ) {
	int i;
	for ( i = 0; i < indexedProperties.Num(); i++ ) {
		if ( indexedProperties[ i ].first == property ) {
			return i;
		}
	}

	boundProperty_t& bp = indexedProperties.Alloc();
	bp.first = property;
	bp.second = propertyExpressions.Num();

	int count = CountForPropertyType( property->GetValueType() );
	if ( count == -1 ) {
		gameLocal.Error( "sdPropertyBinder::IndexForProperty Property has Invalid Field Count" );
	}
	for ( i = 0; i < count; i++ ) {
		propertyExpressions.Append( NULL );
	}
		
	return indexedProperties.Num() - 1;
}

/*
================
sdPropertyBinder::SetPropertyExpression
================
*/
void sdPropertyBinder::SetPropertyExpression( int propertyKey, int propertyIndex, sdUIExpression* expression, sdUserInterfaceScope* scope ) {
	boundProperty_t& bp = indexedProperties[ propertyKey ];
	int index = bp.second + propertyIndex;

	int count = sdProperties::CountForPropertyType( expression->GetType() );
	int i;
	for ( i = 0; i < count; i++ ) {
		if ( propertyExpressions[ index + i ] ) {
			propertyExpressions[ index + i ]->Detach();
			propertyExpressions[ index + i ] = NULL;
		}
	}

	propertyExpressions[ index ] = expression;
	propertyExpressions[ index ]->SetProperty( bp.first, propertyIndex, propertyKey, scope );
}

/*
===============================================================================

	sdUserInterfaceState

===============================================================================
*/

/*
================
sdUserInterfaceState::~sdUserInterfaceState
================
*/
sdUserInterfaceState::~sdUserInterfaceState( void ) {
	for ( int i = 0; i < expressions.Num(); i++ ) {
		expressions[ i ]->Free();
	}
	expressions.Clear();
}

/*
============
sdUserInterfaceState::GetName
============
*/
const char* sdUserInterfaceState::GetName() const {
	return ui->GetName();
}

/*
============
sdUserInterfaceState::GetSubScope
============
*/
sdUserInterfaceScope* sdUserInterfaceState::GetSubScope( const char* name ) {
	if ( !idStr::Icmp( name, "module" ) ) {
		return ( ui->GetModule() != NULL ) ? ui->GetModule()->GetScope() : NULL;
	}

	if( !idStr::Icmp( name, "gui" ) ) {
		return this;
	}

	if( !idStr::Icmp( name, "timeline" ) ) {
		return ui->GetTimelineManager();
	}

	sdUIObject* window = ui->GetWindow( name );
	if ( window ) {
		return &window->GetScope();
	}

	return NULL;
}

/*
================
sdUserInterfaceState::GetProperty
================
*/
sdProperties::sdProperty* sdUserInterfaceState::GetProperty( const char* name ) {
	return properties.GetProperty( name, PT_INVALID, false );
}

/*
============
sdUserInterfaceState::GetProperty
============
*/
sdProperties::sdProperty* sdUserInterfaceState::GetProperty( const char* name, sdProperties::ePropertyType type ) {
	sdProperties::sdProperty* prop = properties.GetProperty( name, PT_INVALID, false );
	if ( prop && prop->GetValueType() != type && type != PT_INVALID ) {
		gameLocal.Error( "sdUserInterfaceState::GetProperty: type mismatch for property '%s'", name );
	}
	return prop;
}

/*
================
sdUserInterfaceState::SetPropertyExpression
================
*/
void sdUserInterfaceState::SetPropertyExpression( int propertyKey, int propertyIndex, sdUIExpression* expression ) {
	boundProperties.SetPropertyExpression( propertyKey, propertyIndex, expression, this );
}

/*
================
sdUserInterfaceState::ClearPropertyExpression
================
*/
void sdUserInterfaceState::ClearPropertyExpression( int propertyKey, int propertyIndex ) {
	boundProperties.ClearPropertyExpression( propertyKey, propertyIndex );
}

/*
================
sdUserInterfaceState::RunFunction
================
*/
void sdUserInterfaceState::RunFunction( int expressionIndex ) {
	expressions[ expressionIndex ]->Evaluate();
}

/*
================
sdUserInterfaceState::IndexForProperty
================
*/
int sdUserInterfaceState::IndexForProperty( sdProperties::sdProperty* property ) {
	return boundProperties.IndexForProperty( property );
}


/*
============
sdUserInterfaceState::GetEvent
============
*/
sdUIEventHandle sdUserInterfaceState::GetEvent( const sdUIEventInfo& info ) const {
	return ui->GetEvent( info );
}

/*
============
sdUserInterfaceState::AddEvent
============
*/
void sdUserInterfaceState::AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle ) {
	ui->AddEvent( info, scriptHandle );
}

/*
============
sdUserInterfaceState::ClearExpressions
============
*/
void sdUserInterfaceState::ClearExpressions() {
	for ( int i = 0; i < expressions.Num(); i++ ) {
		expressions[ i ]->Free();
	}
	expressions.Clear();
}

/*
============
sdUserInterfaceState::Clear
============
*/
void sdUserInterfaceState::Clear() {
	properties.Clear();
	transitionExpressions.Clear();
	boundProperties.Clear();
}

/*
============
sdUserInterfaceState::GetFunction
============
*/
sdUIFunctionInstance* sdUserInterfaceState::GetFunction( const char* name ) {
	return ui->GetFunction( name );
}

/*
============
sdUserInterfaceState::RunNamedFunction
============
*/
bool sdUserInterfaceState::RunNamedFunction( const char* name, sdUIFunctionStack& stack ) {
	const sdUserInterfaceLocal::uiFunction_t* func = sdUserInterfaceLocal::FindFunction( name );
	if ( !func ) {
		return false;
	}

	CALL_MEMBER_FN_PTR( ui, func->GetFunction() )( stack );
	return true;
}

/*
============
sdUserInterfaceState::FindPropertyName
============
*/
const char* sdUserInterfaceState::FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) {
	scope = this;

	const char* name = properties.NameForProperty( property );
	if ( name != NULL ) {
		return name;
	}

	for ( int i = 0 ; i < ui->GetNumWindows(); i++ ) {
		sdUIObject* obj = ui->GetWindow( i );
		name = obj->GetScope().FindPropertyName( property, scope );
		if ( name != NULL ) {
			return name;
		}
	}

	return NULL;
}

/*
============
sdUserInterfaceState::GetEvaluator
============
*/
sdUIEvaluatorTypeBase* sdUserInterfaceState::GetEvaluator( const char* name ) {
	return GetUI()->GetEvaluator( name );
}

/*
============
sdUserInterfaceState::Update
============
*/
void sdUserInterfaceState::Update( void ) {
	int i;
	for ( i = 0; i < transitionExpressions.Num(); ) {
		sdUIExpression* expression = transitionExpressions[ i ];
		
		if ( !expression->UpdateValue() ) {
			transitionExpressions.RemoveIndex( i );
		} else {
			i++;
		}
	}
}

/*
================
sdUserInterfaceState::AddTransition
================
*/
void sdUserInterfaceState::AddTransition( sdUIExpression* expression ) {
	transitionExpressions.AddUnique( expression );
}

/*
================
sdUserInterfaceState::RemoveTransition
================
*/
void sdUserInterfaceState::RemoveTransition( sdUIExpression* expression ) {
	transitionExpressions.Remove( expression );
}


/*
============
sdUserInterfaceState::OnSnapshotHitch
============
*/
void sdUserInterfaceState::OnSnapshotHitch( int delta ) {
	for( int i = 0; i < transitionExpressions.Num(); i++ ) {
		transitionExpressions[ i ]->OnSnapshotHitch( delta );
	}
}

/*
===============================================================================

	sdUserInterfaceLocal

===============================================================================
*/

idHashMap< sdUserInterfaceLocal::uiFunction_t* >	sdUserInterfaceLocal::uiFunctions;
idList< sdUIEvaluatorTypeBase* >					sdUserInterfaceLocal::uiEvaluators;

SD_UI_PUSH_CLASS_TAG( sdUserInterfaceLocal )
const char* sdUserInterfaceLocal::eventNames[ GE_NUM_EVENTS ] = {
	SD_UI_EVENT_TAG( "onCreate",			"",					"Called on window creation" ),
	SD_UI_EVENT_TAG( "onActivate",			"",					"This happens when the GUI is activated." ),
	SD_UI_EVENT_TAG( "onDeactivate",		"",					"This happens when the GUI is activated." ),
	SD_UI_EVENT_TAG( "onNamedEvent",		"[Event ...]",		"Called when one of the events specified occurs" ),
	SD_UI_EVENT_TAG( "onPropertyChanged",	"[Property ...]",	"Called when one of the properties specified occurs" ),
	SD_UI_EVENT_TAG( "onCVarChanged",		"[CVar ...]",		"Called when one of the CVars' value changes" ),
	SD_UI_EVENT_TAG( "onCancel",			"",					"Called when any key bound to the _menuCancel is pressed" ),
	SD_UI_EVENT_TAG( "onCancel",			"",					"Called when any key bound to the _menuCancel is pressed" ),
	SD_UI_EVENT_TAG( "onToolTipEvent",		"",					"Called when a tooltip event occurs" ),
};
SD_UI_POP_CLASS_TAG

/*
================
sdUserInterfaceLocal::sdUserInterfaceLocal
================
*/
sdUserInterfaceLocal::sdUserInterfaceLocal( int _spawnId, bool _isUnique, bool _isPermanent, sdHudModule* _module ) :
	spawnId( _spawnId ),
	desktop( NULL ),
	focusedWindow( NULL ),
	entity( NULL ),
	guiTime( 0.0f ){

	guiDecl				= NULL;
	theme				= NULL;
	module				= _module;
	bindContext			= NULL;

	shaderParms.SetNum( MAX_ENTITY_SHADER_PARMS - 4 );
	for( int i = 0 ; i < shaderParms.Num(); i++ ) {
		shaderParms[ i ] = 0.0f;
	}

	flags.isActive		= false;
	flags.isUnique		= _isUnique;
	flags.isPermanent	= _isPermanent;		
	flags.shouldUpdate	= true;

	currentTime = 0;

	scriptState.Init( this );	

	UI_ADD_STR_CALLBACK( cursorMaterialName, sdUserInterfaceLocal, OnCursorMaterialNameChanged )
	UI_ADD_STR_CALLBACK( postProcessMaterialName,sdUserInterfaceLocal, OnPostProcessMaterialNameChanged )
	UI_ADD_STR_CALLBACK( focusedWindowName, sdUserInterfaceLocal, OnFocusedWindowNameChanged )
	UI_ADD_STR_CALLBACK( screenSaverName, sdUserInterfaceLocal, OnScreenSaverMaterialNameChanged )	
	UI_ADD_STR_CALLBACK( themeName, sdUserInterfaceLocal, OnThemeNameChanged )	
	UI_ADD_STR_CALLBACK( bindContextName, sdUserInterfaceLocal, OnBindContextChanged )
	UI_ADD_VEC2_CALLBACK( screenDimensions, sdUserInterfaceLocal, OnScreenDimensionChanged )
	
	postProcessMaterial = NULL;
	screenSaverMaterial = NULL;

	lastMouseMoveTime = 0;
	nextAllowToolTipTime = 0;

	generalStacks.SetGranularity( 1 );
}

/*
============
sdUserInterfaceLocal::Init
============
*/
void sdUserInterfaceLocal::Init() {
	scriptState.GetPropertyHandler().RegisterProperty( "cursorMaterial", cursorMaterialName );
	scriptState.GetPropertyHandler().RegisterProperty( "cursorSize", cursorSize );
	scriptState.GetPropertyHandler().RegisterProperty( "cursorColor", cursorColor );
	scriptState.GetPropertyHandler().RegisterProperty( "cursorPos", cursorPos );
	scriptState.GetPropertyHandler().RegisterProperty( "postProcessMaterial", postProcessMaterialName );
	scriptState.GetPropertyHandler().RegisterProperty( "focusedWindow", focusedWindowName );
	scriptState.GetPropertyHandler().RegisterProperty( "screenDimensions", screenDimensions );
	scriptState.GetPropertyHandler().RegisterProperty( "screenCenter", screenCenter );
	scriptState.GetPropertyHandler().RegisterProperty( "screenSaverName", screenSaverName );
	scriptState.GetPropertyHandler().RegisterProperty( "time", guiTime );
	scriptState.GetPropertyHandler().RegisterProperty( "theme", themeName );
	scriptState.GetPropertyHandler().RegisterProperty( "bindContext", bindContextName );
	scriptState.GetPropertyHandler().RegisterProperty( "flags", scriptFlags );
	scriptState.GetPropertyHandler().RegisterProperty( "inputScale", inputScale );
	scriptState.GetPropertyHandler().RegisterProperty( "blankWStr", blankWStr );

	inputScale						= 1.0f;
	cursorSize						= idVec2( 32.0f, 32.0f );
	cursorColor						= GetColor( "system/cursor" );

	screenSaverName					= "system/screensaver";
	cursorMaterialName				= "system/cursor";

	scriptFlags						= GUI_INTERACTIVE | GUI_SCREENSAVER;



	screenDimensions				= idVec2( SCREEN_WIDTH, SCREEN_HEIGHT );
	screenDimensions				.SetReadOnly( true );

	screenCenter					= idVec2( SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f );
	screenCenter					.SetReadOnly( true );

	guiTime.SetReadOnly( true );
	blankWStr.SetReadOnly( true );

	cursorPos						= screenCenter;

	focusedWindow					= NULL;
	focusedWindowName				= "";

	flags.ignoreLocalCursorUpdates	= false;
	flags.shouldUpdate				= true;

	toolTipWindow					= NULL;
	toolTipSource					= NULL;
	tooltipAnchor					= vec2_origin;
	postProcessMaterialName 		= "";
	themeName						= "default";

	generalStacks.Clear();
	scriptStack.Clear();
	
	colorStack.Clear();
	colorStack.SetGranularity( 1 );
	currentColor = colorWhite;
}

/*
================
sdUserInterfaceLocal::~sdUserInterfaceLocal
================
*/
sdUserInterfaceLocal::~sdUserInterfaceLocal( void ) {
	parseStack.Clear();
	Clear();
}

/*
============
sdUserInterfaceLocal::PushTrace
============
*/
void sdUserInterfaceLocal::PushTrace( const char* info ) {
	parseStack.Append( info );
}

/*
============
sdUserInterfaceLocal::PopTrace
============
*/
void sdUserInterfaceLocal::PopTrace() {
	if( parseStack.Num() == 0 ) {
		gameLocal.Warning( "sdUserInterfaceLocal::PopTrace: Stack underflow" );
		return;
	}
	parseStack.RemoveIndex( parseStack.Num() - 1 );
}

/*
============
sdUserInterfaceLocal::PrintStackTrace
============
*/
void sdUserInterfaceLocal::PrintStackTrace() {	
	if( parseStack.Num() == 0 ) {
		return;
	}

	gameLocal.Printf( "^3===============================================\n" );
	
	for( int i = parseStack.Num() - 1; i >= 0; i-- ) {
		gameLocal.Printf( "%s\n", parseStack[ i ].c_str() );
	}
	
	gameLocal.Printf( "^3===============================================\n" );
	parseStack.Clear();
}

/*
================
sdUserInterfaceLocal::Load
================
*/
bool sdUserInterfaceLocal::Load( const char* name ) {
	if ( guiDecl ) {
		assert( false );
		return false;
	}

	guiDecl = gameLocal.declGUIType[ name ];
	if ( guiDecl == NULL ) {
		gameLocal.Warning( "sdUserInterfaceLocal::Load Invalid GUI '%s'", name );
		return false;
	}

	Init();

	PushTrace( va( "Loading %s", GetName() ) );
	scriptStack.SetID( GetName() );

	const sdDeclGUITheme* theme = gameLocal.declGUIThemeType.LocalFind( "default" );
	declManager->AddDependency( GetDecl(), theme );

	idTokenCache& tokenCache = declManager->GetGlobalTokenCache();

	try {
		sdUIWindow::SetupProperties( scriptState.GetPropertyHandler(), guiDecl->GetProperties(), this, tokenCache );

		int i;
		for ( i = 0; i < guiDecl->GetNumWindows(); i++ ) {
			const sdDeclGUIWindow* windowDecl = guiDecl->GetWindow( i );
			PushTrace( windowDecl->GetName() );

			sdUIObject* object = uiManager->CreateWindow( windowDecl->GetTypeName() );
			if ( !object ) {			
				gameLocal.Error( "sdUserInterfaceLocal::Load Invalid Window Type '%s'", windowDecl->GetTypeName() );
			}

			object->CreateProperties( this, windowDecl, tokenCache );
			object->CreateTimelines( this, windowDecl, tokenCache );

			if( windows.Find( object->GetName() ) != windows.End() ) {
				gameLocal.Error( "Window named '%s' already exists", object->GetName() );
			}
			windows.Set( object->GetName(), object );

			PopTrace();
		}

		CreateEvents( guiDecl, tokenCache );

		assert( windows.Num() == guiDecl->GetNumWindows() );

		for ( i = 0; i < guiDecl->GetNumWindows(); i++ ) {
			GetWindow( i )->InitEvents();
		}

		for ( i = 0; i < guiDecl->GetNumWindows(); i++ ) {
			GetWindow( i )->CreateEvents( this, guiDecl->GetWindow( i ), tokenCache );
		}
		for ( i = 0; i < guiDecl->GetNumWindows(); i++ ) {
			GetWindow( i )->CacheEvents();
		}
		

		desktop = GetWindow( "desktop" )->Cast< sdUIWindow >();
		if( desktop == NULL ) {
			gameLocal.Warning( "sdUserInterfaceLocal::Load: could not find 'desktop' in '%s'", name );
		}

		toolTipWindow = GetWindow( "toolTip" )->Cast< sdUIWindow >();

		// parent all the nested windows
		for ( i = 0; i < guiDecl->GetNumWindows(); i++ ) {
			const sdDeclGUIWindow* windowDecl = guiDecl->GetWindow( i );
			const idStrList& children = windowDecl->GetChildren();

			PushTrace( windowDecl->GetName() );

			windowHash_t::Iterator parentIter = windows.Find( windowDecl->GetName() );
			if( parentIter == windows.End() ) {
				gameLocal.Error( "sdUserInterfaceLocal::Could not find window '%s'", windowDecl->GetName() );
			}

			for( int childIndex = 0; childIndex < children.Num(); childIndex++ ) {
				windowHash_t::Iterator iter = windows.Find( children[ childIndex ] );
				if( iter == windows.End() ) {
					gameLocal.Error( "sdUserInterfaceLocal::Could not find window '%s'", children[ childIndex ].c_str() );
				}
				iter->second->SetParent( parentIter->second );
			}
			PopTrace();
		}

		// run constructors now that everything is parented
		for ( i = 0; i < guiDecl->GetNumWindows(); i++ ) {			
			GetWindow( i )->RunEvent( sdUIEventInfo( sdUIObject::OE_CREATE, 0 ) );
			GetWindow( i )->OnCreate();
		}
	} catch ( idException& exception ) {
		PrintStackTrace();
		throw exception;
	}

	PopTrace();

	return true;
}

/*
================
sdUserInterfaceLocal::Draw
================
*/
void sdUserInterfaceLocal::Draw() {
#ifdef _DEBUG
	if( guiDecl && guiDecl->GetBreakOnDraw() ) {
		assert( !"BREAK_ON_DRAW" );
	}
#endif // _DEBUG
	if ( !desktop ) {
		return;
	}
	
	deviceContext->SetRegisters( shaderParms.Begin() );
	bool allowScreenSaver = TestGUIFlag( GUI_SCREENSAVER ) && !TestGUIFlag( GUI_FULLSCREEN );

	if ( IsActive() || !allowScreenSaver ) {
		desktop->ApplyLayout();
		desktop->Draw();
		desktop->FinalDraw();

		if ( TestGUIFlag( GUI_SHOWCURSOR ) ) {
			deviceContext->DrawMaterial( cursorPos.GetValue().x, cursorPos.GetValue().y, cursorSize.GetValue().x, cursorSize.GetValue().y, cursorMaterial, cursorColor );
		}
	} else if ( screenSaverMaterial != NULL && allowScreenSaver ) {
		deviceContext->DrawMaterial( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, screenSaverMaterial, colorWhite );
	}
	
	if( g_debugGUI.GetBool() ) {
		sdBounds2D rect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );

		deviceContext->SetColor( colorWhite );
		deviceContext->SetFontSize( g_debugGUITextScale.GetFloat() );
		deviceContext->DrawText( va( L"%hs", guiDecl->GetName() ), rect, DTF_CENTER | DTF_VCENTER | DTF_SINGLELINE );
	}

	UpdateToolTip();
	
	if ( postProcessMaterial != NULL ) {
		deviceContext->DrawMaterial( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, postProcessMaterial, colorWhite );
	}
	assert( colorStack.Num() == 0 );
}


/*
============
sdUserInterfaceLocal::UpdateToolTip
============
*/
void sdUserInterfaceLocal::UpdateToolTip() {
	if( TestGUIFlag( GUI_TOOLTIPS ) ) {
		if( toolTipWindow == NULL ) {
			gameLocal.Warning( "%s: could not find windowDef 'tooltip' for updating", GetName() );
			ClearGUIFlag( GUI_TOOLTIPS );
			return;
		}
		if( toolTipSource == NULL && GetCurrentTime() >= nextAllowToolTipTime ) {
			if( !keyInputManager->AnyKeysDown() ) {
				// try to spawn a new tool-tip
				if( toolTipSource = desktop->UpdateToolTip( cursorPos ) ) {
					tooltipAnchor = cursorPos;
				} else {
					nextAllowToolTipTime = GetCurrentTime() + SEC2MS( gui_tooltipDelay.GetFloat() );
				}
			}
		} else if( toolTipSource != NULL ) {
			bool keepWindow = false;
			if( sdUIWindow* window = toolTipSource->Cast< sdUIWindow >() ) {
				// see if the cursor has moved outside of the tool-tip window
				sdBounds2D bounds( window->GetWorldRect() );
				keepWindow = bounds.ContainsPoint( cursorPos );
			}
			if( !keepWindow ) {
				toolTipSource = NULL;
				nextAllowToolTipTime = GetCurrentTime() + SEC2MS( gui_tooltipDelay.GetFloat() );
			}
		}

		if( !toolTipSource ) {
			CancelToolTip();
			return;
		}
		if( idMath::Fabs( cursorPos.GetValue().x - tooltipAnchor.x ) >= TOOLTIP_MOVE_TOLERANCE || 
			idMath::Fabs( cursorPos.GetValue().y - tooltipAnchor.y ) >= TOOLTIP_MOVE_TOLERANCE ) {
			CancelToolTip();
			nextAllowToolTipTime = GetCurrentTime() + SEC2MS( gui_tooltipDelay.GetFloat() );
			return;
		}

		sdProperties::sdProperty* toolText	= toolTipSource->GetScope().GetProperty( "toolTipText", PT_WSTRING );

		sdProperties::sdProperty* active	= toolTipWindow->GetScope().GetProperty( "active", PT_FLOAT );
		sdProperties::sdProperty* tipText	= toolTipWindow->GetScope().GetProperty( "tipText", PT_WSTRING );
		sdProperties::sdProperty* rect		= toolTipWindow->GetScope().GetProperty( "rect", PT_VEC4 );

		if( toolText && tipText && active ) {
			*tipText->value.wstringValue = *toolText->value.wstringValue;
			*active->value.floatValue = 1.0f;				
		}

		if( rect != NULL ) {
			idVec4 temp = *rect->value.vec4Value;
			temp.x = cursorPos.GetValue().x;
			temp.y = cursorPos.GetValue().y + cursorSize.GetValue().y * 0.5f;
			
			if( temp.x + temp.z >= screenDimensions.GetValue().x ) {
				temp.x -= temp.z;
			}

			if( temp.y + temp.w >= screenDimensions.GetValue().y ) {
				temp.y -= temp.w + cursorSize.GetValue().y * 0.5f;;
			}
			*rect->value.vec4Value = temp;
		}
		tooltipAnchor = cursorPos;
		nextAllowToolTipTime = 0;
	}
}


/*
============
sdUserInterfaceLocal::CancelToolTip
============
*/
void sdUserInterfaceLocal::CancelToolTip() {
	if( !toolTipWindow ) {
		return;
	}

	sdProperties::sdProperty* active = toolTipWindow->GetScope().GetProperty( "active" );
	if( active ) {
		*active->value.floatValue = 0.0f;
	}
	toolTipSource = NULL;
}

/*
================
sdUserInterfaceLocal::GetWindow
================
*/
sdUIObject* sdUserInterfaceLocal::GetWindow( const char* name ) {
	windowHash_t::Iterator iter = windows.Find( name );
	if( iter == windows.End() ) {
		return NULL;
	}
	return iter->second;
}

/*
================
sdUserInterfaceLocal::GetWindow
================
*/
const sdUIObject* sdUserInterfaceLocal::GetWindow( const char* name ) const {
	windowHash_t::ConstIterator iter = windows.Find( name );
	if( iter == windows.End() ) {
		return NULL;
	}
	return iter->second;
}

/*
============
sdUserInterfaceLocal::Clear
============
*/
void sdUserInterfaceLocal::Clear() {
	scriptState.ClearExpressions();	

	// we must do this before we destroy any windows, since a window could be watching other windows' properties
	DisconnectGlobalCallbacks();

	windowHash_t::Iterator iter = windows.Begin();
	while( iter != windows.End() ) {
		iter->second->DisconnectGlobalCallbacks();
		++iter;
	}

	for( int i = 0; i < materialCache.Num(); i++ ) {
		uiMaterialCache_t::Iterator entry = materialCache.FindIndex( i );
		entry->second->material.Clear();
		materialCacheAllocator.Free( entry->second );
	}
	materialCache.Clear();

	timelineWindows.Clear();
	windows.DeleteValues();
	windows.Clear();	

	externalProperties.DeleteContents( true );

	scriptState.Clear();

	script.Clear();
	scriptStack.Clear();

	if( timelines.Get() != NULL ) {
		timelines->Clear();
	}

	focusedWindow	= NULL;
	desktop			= NULL;
	guiDecl			= NULL;
	toolTipSource	= NULL;
	toolTipWindow	= NULL;

	themeName		= "";
	focusedWindowName = "";
	cursorMaterialName = "";
	cursorSize = vec2_zero;
	cursorColor = vec4_zero;	
	screenSaverName = "";
	postProcessMaterialName = "";
}

/*
============
sdUserInterfaceLocal::RegisterTimelineWindow
============
*/
void sdUserInterfaceLocal::RegisterTimelineWindow( sdUIObject* window ) {
	timelineWindows.Alloc() = window;
}

idCVar sdUserInterfaceLocal::gui_invertMenuPitch( "gui_invertMenuPitch", "0", CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE, "invert mouse movement in in-game menus" );
/*
============
sdUserInterfaceLocal::PostEvent
============
*/
bool sdUserInterfaceLocal::PostEvent( const sdSysEvent* event ) {
	if ( !desktop || !IsInteractive() ) {
		return false;
	}
	
	if ( event->IsControllerButtonEvent() || event->IsKeyEvent() ) {
		if ( bindContext != NULL ) {
			bool down;
			sdKeyCommand* cmd = keyInputManager->GetCommand( bindContext, *keyInputManager->GetKeyForEvent( *event, down ) );
			if ( cmd != NULL ) {
				keyInputManager->ProcessUserCmdEvent( *event );
				return true;
			}
		}
	}

	// save these off for the events
	if ( !flags.ignoreLocalCursorUpdates && event->IsMouseEvent() ) {
		idVec2 pos = cursorPos;
		idVec2 scaledDelta( event->GetXCoord(), event->GetYCoord() );
		scaledDelta *= inputScale;

		if ( TestGUIFlag( GUI_FULLSCREEN ) ) {
			scaledDelta.x *= ( 1.0f / deviceContext->GetAspectRatioCorrection() );
		}		

		pos.x += scaledDelta.x;
		if ( TestGUIFlag( GUI_USE_MOUSE_PITCH ) && gui_invertMenuPitch.GetBool() ) {
			pos.y -= scaledDelta.y;
		} else {
			pos.y += scaledDelta.y;
		}

		pos.x = idMath::ClampFloat( 0.0f, screenDimensions.GetValue().x, pos.x );
		pos.y = idMath::ClampFloat( 0.0f, screenDimensions.GetValue().y, pos.y );
		cursorPos = pos;
	}

	bool retVal = false;
	
	if ( !TestGUIFlag( GUI_SHOWCURSOR ) &&
		( event->IsMouseEvent() || ( event->IsMouseButtonEvent() && event->GetMouseButton() >= M_MOUSE1 && event->GetMouseButton() <= M_MOUSE12 ) && !TestGUIFlag( GUI_NON_FOCUSED_MOUSE_EVENTS ) )) {
		retVal = false;
	} else {
		if ( event->IsMouseButtonEvent() && event->IsButtonDown() ) {
			if( focusedWindow ) {
				retVal |= focusedWindow->HandleFocus( event );
			}
			if( !retVal ) {
				retVal |= desktop->HandleFocus( event );
			}
			nextAllowToolTipTime = GetCurrentTime() + SEC2MS( gui_tooltipDelay.GetFloat() );
		}

		if( ( ( event->IsMouseButtonEvent() || event->IsKeyEvent() ) && event->IsButtonDown() ) || event->IsGuiEvent() )  {
			CancelToolTip();
			nextAllowToolTipTime = GetCurrentTime() + SEC2MS( gui_tooltipDelay.GetFloat() );
		}
		if ( focusedWindow == NULL && focusedWindowName.GetValue().Length() ) {
			SetFocus( GetWindow( focusedWindowName.GetValue().c_str() )->Cast< sdUIWindow >() );
		}
		if ( focusedWindow ) {
			bool focusedRetVal = focusedWindow->PostEvent( event );
			retVal |= focusedRetVal;
			if( !focusedRetVal ) {
				// give immediate parents that capture key events a crack
				if( !retVal && event->IsKeyEvent() || event->IsGuiEvent() ) {
					sdUIObject* parent = focusedWindow->GetNode().GetParent();
					while( parent != NULL && retVal == false ) {
						if( sdUIWindow* window = parent->Cast< sdUIWindow >() ) {
							if( window->TestFlag( sdUIWindow::WF_CAPTURE_KEYS ) ) {
								retVal |= parent->PostEvent( event );
							}
						}
						parent = parent->GetNode().GetParent();
					}
				}
			}
		} 
		
		if( !retVal ) {
			retVal |= desktop->PostEvent( event );
		}
	}

	// eat everything but the F-Keys
	if ( TestGUIFlag( GUI_CATCH_ALL_EVENTS ) ) {
		keyNum_t keyNum;
		
		if ( event->IsKeyEvent() ) {
			keyNum = event->GetKey();
		} else {
			keyNum = K_INVALID;
		}

		if ( ( keyNum != K_INVALID && ( keyNum < K_F1 || keyNum > K_F15 ) ) || ( event->IsControllerButtonEvent() ) ) {
			retVal = true;
		}
	}

	if( TestGUIFlag( GUI_TOOLTIPS ) && event->IsMouseEvent() ) {
		lastMouseMoveTime = GetCurrentTime();
	}

	return retVal;
}

/*
============
sdUserInterfaceLocal::Shutdown
============
*/
void sdUserInterfaceLocal::Shutdown( void ) {
	uiFunctions.DeleteContents();
	uiEvaluators.DeleteContents( true );
}

/*
============
sdUserInterfaceLocal::FindFunction
============
*/
sdUserInterfaceLocal::uiFunction_t* sdUserInterfaceLocal::FindFunction( const char* name ) {
	sdUserInterfaceLocal::uiFunction_t** ptr;
	return uiFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUserInterfaceLocal::GetFunction
============
*/
sdUIFunctionInstance* sdUserInterfaceLocal::GetFunction( const char* name ) {
	uiFunction_t* function = FindFunction( name );
	if ( function == NULL ) {
		return NULL;
	}

	return new sdUITemplateFunctionInstance< sdUserInterfaceLocal, sdUITemplateFunctionInstance_Identifier >( this, function );
}

/*
============
sdUserInterfaceLocal::GetEvaluator
============
*/
sdUIEvaluatorTypeBase* sdUserInterfaceLocal::GetEvaluator( const char* name ) {
	int i;
	for ( i = 0; i < uiEvaluators.Num(); i++ ) {
		if ( !idStr::Cmp( uiEvaluators[ i ]->GetName(), name ) ) {
			return uiEvaluators[ i ];
		}
	}

	return NULL;
}

/*
================
sdUserInterfaceLocal::CreateEvents
================
*/
void sdUserInterfaceLocal::CreateEvents( const sdDeclGUI* guiDecl, idTokenCache& tokenCache ) {
	parseStack.Clear();
	const idList< sdDeclGUIProperty* >& guiProperties = guiDecl->GetProperties();

	events.Clear();
	events.SetNumEvents( GE_NUM_EVENTS );
	namedEvents.Clear();
	
	sdUserInterfaceLocal::PushTrace( va( "sdUserInterfaceLocal::CreateEvents for gui '%s'", guiDecl->GetName() ));

	idList<unsigned short> constructorTokens;
	bool hasValues = sdDeclGUI::CreateConstructor( guiDecl->GetProperties(), constructorTokens, tokenCache );

	if( hasValues ) {
		sdUserInterfaceLocal::PushTrace( "<constructor>" );
		idLexer parser( sdDeclGUI::LEXER_FLAGS );
		parser.LoadTokenStream( constructorTokens, tokenCache, "sdUserInterfaceLocal::CreateEvents" );

		sdUIEventInfo constructionEvent( GE_CONSTRUCTOR, 0 );
		GetScript().ParseEvent( &parser, constructionEvent, &scriptState );
		RunEvent( constructionEvent );

		sdUserInterfaceLocal::PopTrace();
	}
	

	if( guiDecl->GetTimelines().GetNumTimelines() > 0 ) {
		timelines.Reset( new sdUITimelineManager( *this, scriptState, script ));
		timelines->CreateTimelines( guiDecl->GetTimelines(), guiDecl );	
		timelines->CreateProperties( guiDecl->GetTimelines(), guiDecl, tokenCache );
	}

	const idList< sdDeclGUIEvent* >& guiEvents = guiDecl->GetEvents();

	idList< sdUIEventInfo > eventList;
	for ( int i = 0; i < guiEvents.Num(); i++ ) {
		const sdDeclGUIEvent* eventInfo = guiEvents[ i ];
		sdUserInterfaceLocal::PushTrace( tokenCache[ eventInfo->GetName() ] );

		eventList.Clear();
		EnumerateEvents( tokenCache[ eventInfo->GetName() ], eventInfo->GetFlags(), eventList, tokenCache );

		for ( int j = 0; j < eventList.Num(); j++ ) {
			idLexer parser( sdDeclGUI::LEXER_FLAGS );
			parser.LoadTokenStream( eventInfo->GetTokenIndices(), tokenCache, tokenCache[ eventInfo->GetName() ] );
			
			GetScript().ParseEvent( &parser, eventList[ j ], &scriptState );
		}
		sdUserInterfaceLocal::PopTrace();
	}

	if( timelines.Get() != NULL ) {
		timelines->CreateEvents( guiDecl->GetTimelines(), guiDecl, tokenCache );
	}

	RunEvent( sdUIEventInfo( GE_CREATE, 0 ) );
}

/*
================
sdUserInterfaceLocal::EnumerateEvents
================
*/
void sdUserInterfaceLocal::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	if ( !idStr::Icmp( name, "onActivate" ) ) {
		events.Append( sdUIEventInfo( GE_ACTIVATE, 0 ) );
		return;
	}

	if ( !idStr::Icmp( name, "onDeactivate" ) ) {
		events.Append( sdUIEventInfo( GE_DEACTIVATE, 0 ) );
		return;
	}

	if ( !idStr::Icmp( name, "onCancel" ) ) {
		events.Append( sdUIEventInfo( GE_CANCEL, 0 ) );
		return;
	}

	if ( !idStr::Icmp( name, "onNamedEvent" ) ) {
		int i;
		for ( i = 0; i < flags.Num(); i++ ) {
			events.Append( sdUIEventInfo( GE_NAMED, NamedEventHandleForString( tokenCache[ flags[ i ] ] ) ) );
		}
		return;
	}

	if( !idStr::Icmp( name, "onCVarChanged" ) ) {
		int i;
		for( i = 0; i < flags.Num(); i++ ) {
			const idToken& name = tokenCache[ flags[ i ] ];
			idCVar* cvar = cvarSystem->Find( name.c_str() );
			if( cvar == NULL ) {
				gameLocal.Error( "Event 'onCVarChanged' could not find cvar '%s'", name.c_str() );
				return;
			}

			int eventHandle = NamedEventHandleForString( name.c_str() );
			cvarCallback_t* callback = new cvarCallback_t( *this, *cvar, eventHandle );
			cvarCallbacks.Append( callback );

			events.Append( sdUIEventInfo( GE_CVARCHANGED, eventHandle ) );
		}
		return;
	}

	if ( !idStr::Icmp( name, "onToolTipEvent" ) ) {
		events.Append( sdUIEventInfo( GE_TOOLTIPEVENT, 0 ) );
		return;
	}

	if ( !idStr::Icmp( name, "onPropertyChanged" ) ) {
		int i;
		for ( i = 0; i < flags.Num(); i++ ) {
			const idToken& name = tokenCache[ flags[ i ] ];

			// do a proper lookup, so windows can watch guis and vice-versa
			idLexer p( sdDeclGUI::LEXER_FLAGS );
			p.LoadMemory( name, name.Length(), "onPropertyChanged event handler" );			

			sdUserInterfaceScope* propertyScope = gameLocal.GetUserInterfaceScope( GetState(), &p );

			idToken token;
			p.ReadToken( &token );

			sdProperty* prop = propertyScope->GetProperty( token );

			if( !prop ) {
				gameLocal.Error( "sdUserInterfaceLocal::EnumerateEvents: event 'onPropertyChanged' could not find property '%s'", name.c_str() );
				return;
			}

			int eventHandle = NamedEventHandleForString( name.c_str() );
			int cbHandle = -1;
			switch( prop->GetValueType() ) {
				case PT_VEC4: 
					cbHandle = prop->value.vec4Value->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const idVec4&, const idVec4& >( &sdUserInterfaceLocal::OnVec4PropertyChanged, this , eventHandle ) );
					break;
				case PT_VEC3: 
					cbHandle = prop->value.vec3Value->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const idVec3&, const idVec3& >( &sdUserInterfaceLocal::OnVec3PropertyChanged, this , eventHandle ) );
					break;
				case PT_VEC2: 
					cbHandle = prop->value.vec2Value->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const idVec2&, const idVec2& >( &sdUserInterfaceLocal::OnVec2PropertyChanged, this , eventHandle ) );
					break;
				case PT_INT: 
					cbHandle = prop->value.intValue->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const int, const int >( &sdUserInterfaceLocal::OnIntPropertyChanged, this , eventHandle ) );
					break;
				case PT_FLOAT: 
					cbHandle = prop->value.floatValue->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const float, const float >( &sdUserInterfaceLocal::OnFloatPropertyChanged, this , eventHandle ) );
					break;
				case PT_STRING: 
					cbHandle = prop->value.stringValue->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const idStr&, const idStr& >( &sdUserInterfaceLocal::OnStringPropertyChanged, this , eventHandle ) );
					break;
				case PT_WSTRING: 
					cbHandle = prop->value.wstringValue->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const idWStr&, const idWStr& >( &sdUserInterfaceLocal::OnWStringPropertyChanged, this , eventHandle ) );
					break;
			}
			toDisconnect.Append( callbackHandler_t( prop, cbHandle ));

			events.Append( sdUIEventInfo( GE_PROPCHANGED, eventHandle ) );
		}
		return;
	}
	gameLocal.Error( "sdUserInterfaceLocal::EnumerateEvents: unknown event '%s'", name );
}

/*
================
sdUserInterfaceLocal::Activate
================
*/
void sdUserInterfaceLocal::Activate( void ) {
	if( !guiDecl ) {
		return;
	}

	if ( flags.isActive ) {
		return;
	}

	flags.isActive = true;
	CancelToolTip();

	if ( NonGameGui() ) {
		SetCurrentTime( sys->Milliseconds() );
	} else {
		SetCurrentTime( gameLocal.time + gameLocal.timeOffset );
	}
	Update();

	if( timelines.Get() != NULL ) {
		timelines->ResetAllTimelines();
	}

	int i;
	for ( i = 0; i < windows.Num(); i++ ) {
		sdUIObject* object = windows.FindIndex( i )->second;
		if( sdUIWindow* window = object->Cast< sdUIWindow >() ) {
			window->OnActivate();
		}		
	}

	RunEvent( sdUIEventInfo( GE_ACTIVATE, 0 ) );
}

/*
================
sdUserInterfaceLocal::Deactivate
================
*/
void sdUserInterfaceLocal::Deactivate( bool forceDeactivate ) {
	if( !guiDecl ) {
		return;
	}

	if ( !flags.isActive || ( !forceDeactivate && !TestGUIFlag( GUI_SCREENSAVER ) )) {
		return;
	}

	flags.isActive = false;

	if( timelines.Get() != NULL ) {
		timelines->ClearAllTimelines();
	}

	RunEvent( sdUIEventInfo( GE_DEACTIVATE, 0 ) );
}

/*
================
sdUserInterfaceLocal::Update
================
*/
void sdUserInterfaceLocal::Update( void ) {
	if ( !IsActive() ) {
		return;
	}

	guiTime.SetReadOnly( false );
	guiTime = currentTime;
	guiTime.SetReadOnly( true );
	
	screenDimensions.SetReadOnly( false );
	screenCenter.SetReadOnly( false );
	if ( TestGUIFlag( GUI_FULLSCREEN ) ) {
		float adjustedWidth = idMath::Ceil( SCREEN_WIDTH * ( 1.0f / deviceContext->GetAspectRatioCorrection() ) );
		screenDimensions.SetIndex( 0, adjustedWidth );
	} else {
		screenDimensions.SetIndex( 0, SCREEN_WIDTH );
	}

	screenCenter.SetIndex( 0 , screenDimensions.GetValue().x / 2.0f );
	screenCenter.SetReadOnly( true );
	screenDimensions.SetReadOnly( true );

	if ( timelines.Get() != NULL ) {
		timelines->Run( currentTime );
	}

	for ( int i = 0; i < timelineWindows.Num(); i++ ) {
		sdUITimelineManager* manager = timelineWindows[ i ]->GetTimelineManager();
		manager->Run( currentTime );
	}

	scriptState.Update();
}

/*
================
sdUserInterfaceLocal::AddEvent
================
*/
void sdUserInterfaceLocal::AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle ) {
	events.AddEvent( info, scriptHandle );
}

/*
================
sdUserInterfaceLocal::GetEvent
================
*/
sdUIEventHandle sdUserInterfaceLocal::GetEvent( const sdUIEventInfo& info ) const {
	return events.GetEvent( info );
}

/*
============
sdUserInterfaceLocal::RunEvent
============
*/
bool sdUserInterfaceLocal::RunEvent( const sdUIEventInfo& info ) {
	return GetScript().RunEventHandle( GetEvent( info ), &scriptState );
}

/*
============
sdUserInterfaceLocal::SetFocus
============
*/
void sdUserInterfaceLocal::SetFocus( sdUIWindow* focus ) {
	if( focusedWindow == focus ) {
		return;
	}
	if( focusedWindow ) {
		focusedWindow->OnLoseFocus(); 
	}

	focusedWindow = focus;

	if( focusedWindow ) {
		focusedWindow->OnGainFocus();
	}	
}

/*
============
sdUserInterfaceLocal::OnCursorMaterialNameChanged
============
*/
void sdUserInterfaceLocal::OnCursorMaterialNameChanged( const idStr& oldValue, const idStr& newValue ) {
	if( newValue.Length() ) {
		cursorMaterial = gameLocal.declMaterialType.LocalFind( GetMaterial( newValue ));
		if( cursorMaterial && cursorMaterial->GetSort() < SS_POST_PROCESS ) {
			if ( cursorMaterial->GetSort() != SS_GUI && cursorMaterial->GetSort() != SS_NEAREST ) {
				gameLocal.Warning( "sdUserInterfaceLocal::OnCursorMaterialNameChanged: material %s used in gui '%s' without proper sort", cursorMaterial->GetName(), GetName() );
			}
		}
	} else {
		cursorMaterial = NULL;
	}
}

/*
============
sdUserInterfaceLocal::OnPostProcessMaterialNameChanged
============
*/
void sdUserInterfaceLocal::OnPostProcessMaterialNameChanged( const idStr& oldValue, const idStr& newValue ) {
	if( newValue.Length() ) {
		postProcessMaterial = gameLocal.declMaterialType.LocalFind( GetMaterial( newValue ));
	} else {
		postProcessMaterial = NULL;
	}
}

/*
============
sdUserInterfaceLocal::OnFocusedWindowNameChanged
============
*/
void sdUserInterfaceLocal::OnFocusedWindowNameChanged( const idStr& oldValue, const idStr& newValue ) {	
	SetFocus( GetWindow( newValue )->Cast< sdUIWindow >() );
	if( newValue.Length() && !focusedWindow ) {
		gameLocal.Warning( "sdUserInterfaceLocal::OnFocusedWindowNameChanged: '%s' could not find windowDef '%s' for focus", GetName(), newValue.c_str() );
	}
}

/*
============
sdUserInterfaceLocal::OnOnScreenSaverMaterialNameChanged
============
*/
void sdUserInterfaceLocal::OnScreenSaverMaterialNameChanged( const idStr& oldValue, const idStr& newValue ) {
	if( newValue.Length() ) {
		screenSaverMaterial = gameLocal.declMaterialType.LocalFind( GetMaterial( newValue ));
	} else {
		screenSaverMaterial = NULL;
	}
}

/*
============
sdUserInterfaceLocal::SetRenderCallback
============
*/
void sdUserInterfaceLocal::SetRenderCallback( const char* objectName, uiRenderCallback_t callback, uiRenderCallbackType_t type ) {
	sdUIWindow* object = GetWindow( objectName )->Cast< sdUIWindow >();
	if( object == NULL ) {
		gameLocal.Error( "sdUserInterfaceLocal::SetRenderCallback: could not find window '%s'", objectName );
	}
	object->SetRenderCallback( callback, type );
}


/*
============
sdUserInterfaceLocal::PostNamedEvent
============
*/
bool sdUserInterfaceLocal::PostNamedEvent( const char* event, bool allowMissing ) {
	assert( event );
	int index = namedEvents.FindIndex( event );
	if( index == -1 ) {
		if( !allowMissing ) {
			gameLocal.Error( "sdUserInterfaceLocal::PostNamedEvent: could not find event '%s' in '%s'", event, guiDecl != NULL ? guiDecl->GetName() : "unknown GUI" );
		}		
		return false;
	}

	if( g_debugGUIEvents.GetBool() ) {
		gameLocal.Printf( "GUI '%s': named event '%s'\n", GetName(), event );
	}
	return RunEvent( sdUIEventInfo( GE_NAMED, index ) );
}

/*
============
sdUserInterfaceLocal::NamedEventHandleForString
============
*/
int sdUserInterfaceLocal::NamedEventHandleForString( const char* name ) {
	int index = namedEvents.FindIndex( name );
	if( index == -1 ) {
		index = namedEvents.Append( name ) ;
	}
	return index;
}

/*
============
sdUserInterfaceLocal::OnStringPropertyChanged
============
*/
void sdUserInterfaceLocal::OnStringPropertyChanged( int event, const idStr& oldValue, const idStr& newValue ) {
	RunEvent( sdUIEventInfo( GE_PROPCHANGED, event ) );
}

/*
============
sdUserInterfaceLocal::OnWStringPropertyChanged
============
*/
void sdUserInterfaceLocal::OnWStringPropertyChanged( int event, const idWStr& oldValue, const idWStr& newValue ) {
	RunEvent( sdUIEventInfo( GE_PROPCHANGED, event ) );
}

/*
============
sdUserInterfaceLocal::OnIntPropertyChanged
============
*/
void sdUserInterfaceLocal::OnIntPropertyChanged( int event, const int oldValue, const int newValue ) {
	RunEvent( sdUIEventInfo( GE_PROPCHANGED, event ) );
}

/*
============
sdUserInterfaceLocal::OnFloatPropertyChanged
============
*/
void sdUserInterfaceLocal::OnFloatPropertyChanged( int event, const float oldValue, const float newValue ) {
	RunEvent( sdUIEventInfo( GE_PROPCHANGED, event ) );
}

/*
============
sdUserInterfaceLocal::OnVec4PropertyChanged
============
*/
void sdUserInterfaceLocal::OnVec4PropertyChanged( int event, const idVec4& oldValue, const idVec4& newValue ) {
	RunEvent( sdUIEventInfo( GE_PROPCHANGED, event ) );
}

/*
============
sdUserInterfaceLocal::OnVec3PropertyChanged
============
*/
void sdUserInterfaceLocal::OnVec3PropertyChanged( int event, const idVec3& oldValue, const idVec3& newValue ) {
	RunEvent( sdUIEventInfo( GE_PROPCHANGED, event ) );
}

/*
============
sdUserInterfaceLocal::OnVec2PropertyChanged
============
*/
void sdUserInterfaceLocal::OnVec2PropertyChanged( int event, const idVec2& oldValue, const idVec2& newValue ) {
	RunEvent( sdUIEventInfo( GE_PROPCHANGED, event ) );
}

/*
============
sdUserInterfaceLocal::SetCursor
============
*/
void sdUserInterfaceLocal::SetCursor( const int x, const int y ) {
	idVec2 pos( idMath::ClampInt( 0, SCREEN_WIDTH, x ), idMath::ClampInt( 0, SCREEN_HEIGHT, y ) );
	cursorPos = pos;
}

/*
============
sdUserInterfaceLocal::SetTheme
============
*/
void sdUserInterfaceLocal::SetTheme( const char* theme ) {
	const sdDeclGUITheme* newTheme = gameLocal.declGUIThemeType.LocalFind( theme, false );

	if( !newTheme ) {
		newTheme = gameLocal.declGUIThemeType.LocalFind( "default", true );
	}

	if( this->theme != newTheme ) {

		bool isActive = IsActive();
		if ( isActive ) {
			Deactivate( true );
		}

		this->theme = newTheme;
		if ( guiDecl != NULL ) {
			declManager->AddDependency( GetDecl(), newTheme );

			// regenerate
			idStr name = guiDecl->GetName();
			guiDecl = NULL;
			Clear();
			Load( name );	
		}

		if ( isActive ) {
			Activate();
		}
	}
}

/*
============
sdUserInterfaceLocal::GetMaterial
============
*/
const char* sdUserInterfaceLocal::GetMaterial( const char* key ) const {
	if( key[ 0 ] == '\0' ) {
		return "";
	}

	const char* out = GetDecl() ? GetDecl()->GetMaterials().GetString( key, "" ) : "";
	if( out[ 0 ] == '\0' && GetTheme() ) {
		out = GetTheme()->GetMaterial( key );
	}

	return out;
}

/*
============
sdUserInterfaceLocal::GetSound
============
*/
const char* sdUserInterfaceLocal::GetSound( const char* key ) const {
	if( key[ 0 ] == '\0' ) {
		return "";
	}
	
	if( idStr::Icmpn( key, "::", 2 ) == 0 ) {
		return key + 2;
	}

	const char* out = GetDecl() ? GetDecl()->GetSounds().GetString( key, "" ) : "";
	if( out[ 0 ] == '\0' && GetTheme() ) {
		out = GetTheme()->GetSound( key );
	}

	return out;
}

/*
============
sdUserInterfaceLocal::GetColor
============
*/
idVec4 sdUserInterfaceLocal::GetColor( const char* key ) const {
	if( key[ 0 ] == '\0' ) {
		return colorWhite;
	}

	idVec4 out;

	if( !GetDecl() || !GetDecl()->GetColors().GetVec4( key, "1 1 1 1", out ) && GetTheme() ) {
		out = GetTheme()->GetColor( key );
	}

	return out;
}


/*
============
sdUserInterfaceLocal::ApplyLatchedTheme
============
*/
void sdUserInterfaceLocal::ApplyLatchedTheme() {
	if ( latchedTheme.Length() > 0 ) {
		SetTheme( latchedTheme );
		latchedTheme.Clear();	
	}
}

/*
============
sdUserInterfaceLocal::OnThemeNameChanged
============
*/
void sdUserInterfaceLocal::OnThemeNameChanged( const idStr& oldValue, const idStr& newValue ) {
	if( newValue.IsEmpty() ) {
		latchedTheme = "default";
	} else {
		latchedTheme = newValue;
	}	
}

/*
============
sdUserInterfaceLocal::OnBindContextChanged
============
*/
void sdUserInterfaceLocal::OnBindContextChanged( const idStr& oldValue, const idStr& newValue ) {
	if ( newValue.IsEmpty() ) {
		bindContext = NULL;
	} else {
		bindContext = keyInputManager->AllocBindContext( newValue.c_str() );
	}	
}

/*
============
sdUserInterfaceLocal::OnScreenDimensionChanged
============
*/
void sdUserInterfaceLocal::OnScreenDimensionChanged( const idVec2& oldValue, const idVec2& newValue ) {
	windowHash_t::Iterator iter = windows.Begin();
	while( iter != windows.End() ) {
		if( sdUIWindow* window = iter->second->Cast< sdUIWindow >() ) {
			window->MakeLayoutDirty();
		}		
		++iter;
	}
}

/*
============
sdUserInterfaceLocal::Translate
============
*/
bool sdUserInterfaceLocal::Translate( const idKey& key, sdKeyCommand** cmd ) {
	if ( bindContext == NULL ) {
		return false;
	}
	*cmd = keyInputManager->GetCommand( bindContext, key );
	return *cmd != NULL;
}


/*
============
sdUserInterfaceLocal::EndLevelLoad
============
*/
void sdUserInterfaceLocal::EndLevelLoad() {
	uiMaterialCache_t::Iterator cacheIter= materialCache.Begin();
	while( cacheIter != materialCache.End() ) {
		uiCachedMaterial_t& cached = *( cacheIter->second );
		LookupPartSizes( cached.parts.Begin(), cached.parts.Num() );
		SetupMaterialInfo( cached.material );
		++cacheIter;
	}

	windowHash_t::Iterator iter = windows.Begin();
	while( iter != windows.End() ) {
		iter->second->EndLevelLoad();
		++iter;
	}
}

/*
============
sdUserInterfaceLocal::PopGeneralScriptVar
============
*/
void sdUserInterfaceLocal::PopGeneralScriptVar( const char* stackName, idStr& str ) {
	if( stackName[ 0 ] == '\0' ) {
		gameLocal.Error( "sdUserInterfaceLocal::PopGeneralScriptVar: empty stack name" );
	}
	stringStack_t& stack = generalStacks[ stackName ];
	if( stack.Num() == 0 ) {
		gameLocal.Error( "sdUserInterfaceLocal::PopGeneralScriptVar: stack underflow for '%s'", stackName );
	}

	int index = stack.Num() - 1;
	str = stack[ index ];
	stack.SetNum( index );
}

/*
============
sdUserInterfaceLocal::PushGeneralScriptVar
============
*/
void sdUserInterfaceLocal::PushGeneralScriptVar( const char* stackName, const char* str ) {
	if( stackName[ 0 ] == '\0' ) {
		gameLocal.Error( "sdUserInterfaceLocal::PushGeneralScriptVar: empty stack name" );
	}
	generalStacks[ stackName ].Append( str );
}

/*
============
sdUserInterfaceLocal::GetGeneralScriptVar
============
*/
void sdUserInterfaceLocal::GetGeneralScriptVar( const char* stackName, idStr& str ) {
	if( stackName[ 0 ] == '\0' ) {
		gameLocal.Error( "sdUserInterfaceLocal::GetGeneralScriptVar: empty stack name" );
	}

	stringStack_t& stack = generalStacks[ stackName ];
	if( stack.Num() == 0 ) {
		gameLocal.Error( "sdUserInterfaceLocal::GetGeneralScriptVar: stack underflow for '%s'", stackName );
	}
	
	int index = stack.Num() - 1;
	str = stack[ index ];
}

/*
============
sdUserInterfaceLocal::ClearGeneralStrings
============
*/
void sdUserInterfaceLocal::ClearGeneralStrings( const char* stackName ) {
	if( stackName[ 0 ] == '\0' ) {
		gameLocal.Error( "sdUserInterfaceLocal::ClearGeneralStrings: empty stack name" );
	}

	stringStack_t& stack = generalStacks[ stackName ];
	stack.Clear();
}

/*
============
sdUserInterfaceLocal::OnCVarChanged
============
*/
void sdUserInterfaceLocal::OnCVarChanged( idCVar& cvar, int id ) {
	bool result = RunEvent( sdUIEventInfo( GE_CVARCHANGED, id ) );
	if( result && sdUserInterfaceLocal::g_debugGUIEvents.GetInteger() ) {
		gameLocal.Printf( "%s: OnCVarChanged\n", GetName() );
	}
}

/*
============
sdUserInterfaceLocal::OnInputInit
============
*/
void sdUserInterfaceLocal::OnInputInit( void ) {	
	if ( bindContextName.GetValue().IsEmpty() ) {
		bindContext = NULL;
	} else {
		bindContext = keyInputManager->AllocBindContext( bindContextName.GetValue().c_str() );
	}
}

/*
============
sdUserInterfaceLocal::OnInputShutdown
============
*/
void sdUserInterfaceLocal::OnInputShutdown( void ) {
	bindContext = NULL;
}

/*
============
sdUserInterfaceLocal::OnLanguageInit
============
*/
void sdUserInterfaceLocal::OnLanguageInit( void ) {
	if ( desktop != NULL ) {
		desktop->OnLanguageInit();
		sdUIObject::OnLanguageInit_r( desktop );
	}
}

/*
============
sdUserInterfaceLocal::OnLanguageShutdown
============
*/
void sdUserInterfaceLocal::OnLanguageShutdown( void ) {
	if ( desktop != NULL ) {
		desktop->OnLanguageShutdown();
		sdUIObject::OnLanguageShutdown_r( desktop );
	}
}

/*
============
sdUserInterfaceLocal::MakeLayoutDirty
============
*/
void sdUserInterfaceLocal::MakeLayoutDirty() {
	if ( desktop != NULL ) {
		desktop->MakeLayoutDirty();
		sdUIObject::MakeLayoutDirty_r( desktop );
	}
}


/*
============
sdUserInterfaceLocal::SetCachedMaterial
============
*/
uiMaterialCache_t::Iterator sdUserInterfaceLocal::SetCachedMaterial( const char* alias, const char* newMaterial, int& handle ) {
	uiMaterialCache_t::Iterator findResult = FindCachedMaterial( alias, handle );
	if( findResult == materialCache.End() ) {
		uiMaterialCache_t::InsertResult result = materialCache.Set( alias, materialCacheAllocator.Alloc() );
		findResult = result.first;		
	}

	handle = findResult - materialCache.Begin();

	uiCachedMaterial_t& cached = *( findResult->second );

	idStr material;
	bool globalLookup = false;
	bool literal = false;

	int offset = ParseMaterial( newMaterial, material, globalLookup, literal, cached.drawMode );

	if( cached.drawMode != BDM_SINGLE_MATERIAL && cached.drawMode != BDM_USE_ST ) {
		InitPartsForBaseMaterial( material, cached );
		return findResult;
	}

	if( globalLookup ) {
		material = "::" + material;
	}

	if( literal ) {
		LookupMaterial( va( "literal: %hs", newMaterial + offset ), cached.material );
	} else if( ( material.Length() && !globalLookup ) || ( material.Length() > 2 && globalLookup ) ) {
		LookupMaterial( material, cached.material );
	}
	return findResult;
}

/*
============
sdUserInterfaceLocal::FindCachedMaterial
============
*/
uiMaterialCache_t::Iterator sdUserInterfaceLocal::FindCachedMaterial( const char* alias, int& handle ) {
	uiMaterialCache_t::Iterator iter = materialCache.Find( alias );
	if( iter == materialCache.End() ) {
		handle = -1;
	} else {
		handle = iter - materialCache.Begin();
	}

	return iter;
}



/*
============
sdUserInterfaceLocal::FindCachedMaterialForHandle
============
*/
uiMaterialCache_t::Iterator sdUserInterfaceLocal::FindCachedMaterialForHandle( int handle ) {
	if( handle < 0 || handle >= materialCache.Num() ) {
		return materialCache.End();
	}
	
	return materialCache.Begin() + handle;
}

/*
============
sdUserInterfaceLocal::LookupPartSizes
============
*/
void sdUserInterfaceLocal::LookupPartSizes( uiDrawPart_t* parts, int num ) {
	for ( int i= 0; i < num; i++ ) {
		uiDrawPart_t& part = parts[ i ];
		if ( part.mi.material == NULL || ( part.width != 0 && part.height != 0 ) ) {
			continue;
		}

		if ( part.mi.material->GetNumStages() == 0 ) {
			part.mi.material = NULL;
			continue;
		}

		SetupMaterialInfo( part.mi, &part.width, &part.height );

		if ( part.width == 0 || part.height == 0 ) {
			assert( 0 );
			part.mi.material = NULL;
		}		
	}
}

/*
============
sdUserInterfaceLocal::SetupMaterialInfo
============
*/
void sdUserInterfaceLocal::SetupMaterialInfo( uiMaterialInfo_t& mi, int* baseWidth, int* baseHeight ) {
	if( mi.material == NULL ) {		
		return;
	}

	if( const idImage* image = mi.material->GetEditorImage() ) {
		if( image->sourceWidth == 0 ) {	// the image hasn't been loaded yet, so defer texture coordinate calculation
			mi.flags.lookupST = true;
			return;
		}
	}

	if( !mi.flags.lookupST ) {
		return;
	}

	if( const idImage* image = mi.material->GetEditorImage() ) {
		// if they're zeroed assume 0-1 range
		if( mi.st0.Compare( vec2_zero, idMath::FLT_EPSILON ) && mi.st1.Compare( vec2_zero, idMath::FLT_EPSILON ) ) {
			mi.st0.Set( 0.0f, 0.0f );
			mi.st1.Set( 1.0f, 1.0f );

			if( baseWidth != NULL ) {
				*baseWidth = image->sourceWidth;
			}
			if( baseHeight != NULL ) {
				*baseHeight = image->sourceHeight;
			}
		} else {
			if( baseWidth != NULL ) {
				*baseWidth = idMath::Ftoi( mi.st1.x );
			}
			if( baseHeight != NULL ) {
				*baseHeight = idMath::Ftoi( mi.st1.y );
			}

			mi.st0.x = mi.st0.x / static_cast< float >( image->sourceWidth );
			mi.st0.y = mi.st0.y / static_cast< float >( image->sourceHeight );

			mi.st1.x = mi.st0.x + ( mi.st1.x / static_cast< float >( image->sourceWidth ) );
			mi.st1.y = mi.st0.y + ( mi.st1.y / static_cast< float >( image->sourceHeight ) );
		}		
	}
	if( mi.flags.flipX ) {
		idSwap( mi.st0.x, mi.st1.x );
	}
	if( mi.flags.flipY ) {
		idSwap( mi.st0.y, mi.st1.y );
	}

	mi.flags.lookupST = false;
}

/*
============
sdUserInterfaceLocal::ParseMaterial
============
*/
int sdUserInterfaceLocal::ParseMaterial( const char* mat, idStr& outMaterial, bool& globalLookup, bool& literal, uiDrawMode_e& mode ) {
	idLexer src( mat, idStr::Length( mat ), "ParseMaterial", LEXFL_ALLOWPATHNAMES );
	idToken token;

	outMaterial.Empty();
	globalLookup = false;
	literal = false;

	mode = BDM_SINGLE_MATERIAL;

	int materialStart = 0;

	while( !src.HadError() ) {
		materialStart = src.GetFileOffset();
		if( !src.ReadToken( &token )) {
			break;
		}

		if( token.Icmp( "literal:" ) == 0 ) {
			literal = true;
			continue;
		}

		if( token.Icmp( "_frame" ) == 0 ) {
			mode = BDM_FRAME;
			continue;
		}

		if( token.Icmp( "_st" ) == 0 ) {
			mode = BDM_USE_ST;
			continue;
		}

		if( token.Icmp( "_3v" ) == 0 ) {
			mode = BDM_TRI_PART_V;
			continue;
		}

		if( token.Icmp( "_3h" ) == 0 ) {
			mode = BDM_TRI_PART_H;
			continue;
		}

		if( token.Icmp( "_5h" ) == 0 ) {
			mode = BDM_FIVE_PART_H;
			continue;
		}

		if( token == "::" ) {
			globalLookup = true;
			continue;
		}	
		outMaterial = token;
		break;
	}
	return materialStart;
}


/*
============
sdUserInterfaceLocal::InitPartsForBaseMaterial
============
*/
void sdUserInterfaceLocal::InitPartsForBaseMaterial( const char* material, uiCachedMaterial_t& cached ) {
	cached.parts.SetNum( FP_MAX );

	if( cached.drawMode == BDM_FIVE_PART_H ) {
		SetupPart( cached.parts[ FP_TOPLEFT ],	partNames[ FP_TOPLEFT ], material );
		SetupPart( cached.parts[ FP_LEFT ],		partNames[ FP_LEFT ], material );		// stretched
		SetupPart( cached.parts[ FP_CENTER ],	partNames[ FP_CENTER ], material );
		SetupPart( cached.parts[ FP_RIGHT ],	partNames[ FP_RIGHT ], material );		// stretched
		SetupPart( cached.parts[ FP_TOPRIGHT ], partNames[ FP_TOPRIGHT ], material );		
		return;
	}

	if( cached.drawMode == BDM_TRI_PART_H ) {
		SetupPart( cached.parts[ FP_LEFT ], partNames[ FP_LEFT ], material );
		SetupPart( cached.parts[ FP_RIGHT ], partNames[ FP_RIGHT ], material );
		SetupPart( cached.parts[ FP_CENTER ], partNames[ FP_CENTER ], material );
		return;
	}

	if( cached.drawMode == BDM_TRI_PART_V ) {
		SetupPart( cached.parts[ FP_TOP ], partNames[ FP_TOP ], material );
		SetupPart( cached.parts[ FP_BOTTOM ], partNames[ FP_BOTTOM ], material );
		SetupPart( cached.parts[ FP_CENTER ], partNames[ FP_CENTER ], material );
		return;
	}

	for( int i = 0; i < FP_MAX; i++ ) {
		SetupPart( cached.parts[ i ], partNames[ i ], material );
	}
}


/*
============
sdUserInterfaceLocal::SetupPart
============
*/
void sdUserInterfaceLocal::SetupPart( uiDrawPart_t& part, const char* partName, const char* material ) {
	if( idStr::Length( material ) == 0 ) {
		part.mi.material = NULL;
		part.width = 0;
		part.height = 0;
		return;
	}

	LookupMaterial( va( "%s_%s", material, partName ), part.mi, &part.width, &part.height );

	if( part.mi.material->GetNumStages() == 0 ) {
		part.mi.material = NULL;
		part.width = 0;
		part.height = 0;
		return;
	}
}

/*
============
sdUserInterfaceLocal::LookupMaterial
============
*/
void sdUserInterfaceLocal::LookupMaterial( const char* materialName, uiMaterialInfo_t& mi, int* baseWidth, int* baseHeight ) {
	mi.Clear();

	bool globalLookup = false;
	static const char* LITERAL_ID = "literal:";
	static const int LITERAL_ID_LENGTH = idStr::Length( LITERAL_ID );
	bool literal = !idStr::Icmpn( LITERAL_ID, materialName, LITERAL_ID_LENGTH );

	if( !literal ) {
		globalLookup = !idStr::Icmpn( "::", materialName, 2 );
		if( globalLookup ) {
			materialName += 2;
		}
	}

	if( globalLookup ) {
		if(idStr::Length( materialName ) == 0 ) {
			mi.material = declHolder.FindMaterial( "_default" );
		} else {
			mi.material = declHolder.FindMaterial( materialName );
		}
	} else {
		const char* materialInfo = materialName;
		if( literal ) {
			materialInfo += LITERAL_ID_LENGTH;
		} else {
			materialInfo = GetMaterial( materialName );
		}
		idToken token;
		char buffer[128];
		token.SetStaticBuffer( buffer, sizeof(buffer) );
		idLexer src( materialInfo, idStr::Length( materialInfo ), "LookupMaterial", LEXFL_ALLOWPATHNAMES );

		src.ReadToken( &token );	// material name

		if( token.Length() == 0 ) {
			mi.material = declHolder.FindMaterial( "_default" );
		} else {
			mi.material = declHolder.FindMaterial( token );
		}

		while( src.ReadToken( &token )) {
			if( token == "," ) {
				continue;
			}
			if( token.Icmp( "flipX" ) == 0 ) {
				mi.flags.flipX = true;
				continue;
			}
			if( token.Icmp( "flipY" ) == 0 ) {
				mi.flags.flipY = true;
				continue;
			}
			static idVec4 vec;
			if( token.Icmp( "rect" ) == 0 ) {
				src.Parse1DMatrix( 4, vec.ToFloatPtr(), true );
				mi.st0.x = vec.x;
				mi.st0.y = vec.y;
				mi.st1.x = vec.z;
				mi.st1.y = vec.w;
				continue;
			}
			src.Error( "Unknown token '%s'", token.c_str() );
			break;
		}
	}


	if ( mi.material->GetSort() < SS_POST_PROCESS ) {
		if ( mi.material->GetSort() != SS_GUI && mi.material->GetSort() != SS_NEAREST ) {
			gameLocal.Warning( "LookupMaterial: '%s' material '%s' (alias '%s') used without proper sort", GetName(), mi.material->GetName(), materialName );
		}
	}

	mi.flags.lookupST = true;
	SetupMaterialInfo( mi, baseWidth, baseHeight );
}

/*
============
sdUserInterfaceLocal::PushColor
============
*/
void sdUserInterfaceLocal::PushColor( const idVec4& color ) {
	currentColor = color;
	colorStack.Push( deviceContext->SetColorMultiplier( color ) );
}

/*
============
sdUserInterfaceLocal::PopColor
============
*/
idVec4 sdUserInterfaceLocal::PopColor() {
	idVec4 c = colorStack.Top();
	deviceContext->SetColorMultiplier( c );
	colorStack.Pop();
	currentColor = c;
	return c;
}

/*
============
sdUserInterfaceLocal::TopColor
============
*/
const idVec4& sdUserInterfaceLocal::TopColor() const{
	return currentColor;
}


/*
============
sdUserInterfaceLocal::OnSnapshotHitch
============
*/
void sdUserInterfaceLocal::OnSnapshotHitch( int delta ) {
	scriptState.OnSnapshotHitch( delta );
	if( timelines.IsValid() ) {
		timelines->OnSnapshotHitch( delta );
	}
}

/*
============
sdUserInterfaceLocal::OnToolTipEvent
============
*/
void sdUserInterfaceLocal::OnToolTipEvent( const char* arg ) {
	sdUIEventInfo event( GE_TOOLTIPEVENT, 0 );

	if ( event.eventType.IsValid() ) {
		PushScriptVar( arg );
		RunEvent( sdUIEventInfo( GE_TOOLTIPEVENT, 0 ) );		
		ClearScriptStack();
	}
}