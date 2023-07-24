// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../../framework/KeyInput.h"
#include "../../sys/sys_local.h"

#include "UserInterfaceManager.h"
#include "UserInterfaceLocal.h"
#include "UserInterfaceManagerLocal.h"
#include "UIWindow.h"
#include "UIList.h"
#include "UIEdit.h"
#include "UIEditW.h"
#include "UIBinder.h"
#include "UIRenderWorld.h"
#include "UISlider.h"
#include "UIWindow_Shaped.h"
#include "UIProgress.h"
#include "UIRadialMenu.h"
#include "UITimeline.h"
#include "UIMarquee.h"
#include "UILayout.h"
#include "UIIconNotification.h"
#include "UICrosshairInfo.h"
#include "UICinematic.h"
#include "UILayout.h"
#include "UICreditScroll.h"
#include "UINews.h"

#include "../demos/DemoManager.h"

// for render callbacks
#include "../structures/CommandMapPost.h"
#include "../Player.h"
#include "../Atmosphere.h"

sdUserInterfaceManagerLocal uiManagerLocal;
sdUserInterfaceManager* uiManager = &uiManagerLocal;

idCVar g_guiSpeeds( "g_guiSpeeds", "0", CVAR_GAME | CVAR_BOOL, "Show GUI speeds" );

/*
===============================================================================

	sdUserInterfaceManagerLocal

===============================================================================
*/

/*
================
sdUserInterfaceManagerLocal::sdUserInterfaceManagerLocal
================
*/
sdUserInterfaceManagerLocal::sdUserInterfaceManagerLocal( void ) :
	nextIndex( 1 ),
	lastNonGameGuiTime( 0 ) {
}

/*
================
sdUserInterfaceManagerLocal::~sdUserInterfaceManagerLocal
================
*/
sdUserInterfaceManagerLocal::~sdUserInterfaceManagerLocal( void ) {
	for ( int i = 0; i < instances.Num(); i++ ) {
		if ( !instances[ i ] ) {
			continue;
		}
		gameLocal.Warning( "sdUserInterfaceManagerLocal::~sdUserInterfaceManagerLocal: instance '%s' still exists at destruction", instances[ i ]->GetName() );
		// jrad - don't delete, just leak at this point
	}

	renderCallbacks.DeleteValues();
}

/*
================
sdUserInterfaceManagerLocal::GetHandle
================
*/
guiHandle_t sdUserInterfaceManagerLocal::GetHandle( int index ) {
	guiHandle_t handle;
	if ( index >= 0 && index < instances.Num() && instances[ index ] ) {
		handle = ( instances[ index ]->GetSpawnId() << GUINUM_BITS ) | index;
	}

	return handle;
}

/*
================
sdUserInterfaceManagerLocal::GetUserInterface
================
*/
sdUserInterfaceLocal* sdUserInterfaceManagerLocal::GetUserInterface( const guiHandle_t handle ) {
	int guiIndex = GetUserInterfaceIndex( handle );
	if ( guiIndex == -1 ) {
		return NULL;
	}
	return instances[ guiIndex ];
}

/*
================
sdUserInterfaceManagerLocal::GetUserInterface
================
*/
int sdUserInterfaceManagerLocal::GetUserInterfaceIndex( const guiHandle_t handle ) {
	if ( !handle.IsValid() ) {
		return -1;
	}

	int guiIndex = handle & ( ( 1 << GUINUM_BITS ) - 1 );
	if ( guiIndex >= instances.Num() ) {
		return -1;
	}

	if ( instances[ guiIndex ] == NULL ) {
		return -1;
	}

	if ( instances[ guiIndex ]->GetSpawnId() != ( handle >> GUINUM_BITS ) ) {
		return -1;
	}

	return guiIndex;
}

/*
================
sdUserInterfaceManagerLocal::FreeUserInterface
================
*/
void sdUserInterfaceManagerLocal::FreeUserInterface( guiHandle_t handle ) {
	int guiIndex = GetUserInterfaceIndex( handle );
	if ( guiIndex == -1 ) {
		return;
	}

	if( instances[ guiIndex ]->IsActive() ) {
		instances[ guiIndex ]->Deactivate( true );
	}

	delete instances[ guiIndex ];
	instances[ guiIndex ] = NULL;
}

/*
================
sdUserInterfaceManagerLocal::AllocUI
================
*/
guiHandle_t sdUserInterfaceManagerLocal::AllocUI( const char* name, bool requireUnique, bool permanent, const char* theme, sdHudModule* module ) {
	if ( !requireUnique ) {
		for ( int i = 0; i < instances.Num(); i++ ) {
			sdUserInterfaceLocal* ui = instances[ i ];

			if ( !ui || ui->IsUnique() ) {
				continue;
			}

			if ( idStr::Icmp( ui->GetName(), name ) ) {
				continue;
			}

			// make sure they're using the same theme
			if ( idStr::Icmp( ui->GetTheme()->GetName(), theme ) ) {
				continue;
			}

			// ensure the guiDecl gets touched
			gameLocal.declGUIType[ ui->GetName() ];

			return GetHandle( i );
		}
	}

	for ( int i = 0; i < instances.Num(); i++ ) {
		if ( !instances[ i ] ) {
			instances[ i ] = new sdUserInterfaceLocal( nextIndex++, requireUnique, permanent, module );
			instances[ i ]->SetTheme( theme	);
			instances[ i ]->ApplyLatchedTheme();
			instances[ i ]->Load( name );
			return GetHandle( i );
		}
	}

	if ( instances.Num() < MAX_UNIQUE_GUIS ) {
		sdUserInterfaceLocal* instance = new sdUserInterfaceLocal( nextIndex++, requireUnique, permanent, module );
		instance->SetTheme( theme );
		instance->ApplyLatchedTheme();
		instance->Load( name );
		return GetHandle( instances.Append( instance ) );
	}

	gameLocal.Error( "sdUserInterfaceManagerLocal::AllocUI Max GUIs hit" );
	return guiHandle_t();
}

/*
================
sdUserInterfaceManagerLocal::Init
================
*/
void sdUserInterfaceManagerLocal::Init( void ) {
	windowFactory.RegisterType( "window",			sdUIObjectFactory::Allocator< sdUIWindow > );
	windowFactory.RegisterType( "list",				sdUIObjectFactory::Allocator< sdUIList > );
	windowFactory.RegisterType( "edit",				sdUIObjectFactory::Allocator< sdUIEdit > );
	windowFactory.RegisterType( "editw",			sdUIObjectFactory::Allocator< sdUIEditW > );
	windowFactory.RegisterType( "renderWorld",		sdUIObjectFactory::Allocator< sdUIRenderWorld > );
	windowFactory.RegisterType( "binder",			sdUIObjectFactory::Allocator< sdUIBinder > );
//	windowFactory.RegisterType( "menu",				sdUIObjectFactory::Allocator< sdUIMenu > );
	windowFactory.RegisterType( "slider",			sdUIObjectFactory::Allocator< sdUISlider > );
//	windowFactory.RegisterType( "shaped",			sdUIObjectFactory::Allocator< sdUIWindow_Shaped > );
	windowFactory.RegisterType( "progress",			sdUIObjectFactory::Allocator< sdUIProgress > );
	windowFactory.RegisterType( "radialmenu",		sdUIObjectFactory::Allocator< sdUIRadialMenu > );
	windowFactory.RegisterType( "marquee",			sdUIObjectFactory::Allocator< sdUIMarquee > );
	windowFactory.RegisterType( "renderModel",		sdUIObjectFactory::Allocator< sdUIRenderModel > );
	windowFactory.RegisterType( "renderLight",		sdUIObjectFactory::Allocator< sdUIRenderLight > );
	windowFactory.RegisterType( "renderCamera",		sdUIObjectFactory::Allocator< sdUIRenderCamera > );
	windowFactory.RegisterType( "renderCameraAnim",	sdUIObjectFactory::Allocator< sdUIRenderCamera_Animated > );
	windowFactory.RegisterType( "crosshairInfo",	sdUIObjectFactory::Allocator< sdUICrosshairInfo > );
	windowFactory.RegisterType( "iconNotification",	sdUIObjectFactory::Allocator< sdUIIconNotification > );
	windowFactory.RegisterType( "cinematic",		sdUIObjectFactory::Allocator< sdUICinematic > );
	windowFactory.RegisterType( "layoutStatic",		sdUIObjectFactory::Allocator< sdUILayout_Static > );
	windowFactory.RegisterType( "layoutVertical",	sdUIObjectFactory::Allocator< sdUILayout_Vertical > );
	windowFactory.RegisterType( "layoutHorizontal",	sdUIObjectFactory::Allocator< sdUILayout_Horizontal > );
	windowFactory.RegisterType( "creditScroll",		sdUIObjectFactory::Allocator< sdUICreditScroll > );
	windowFactory.RegisterType( "news",				sdUIObjectFactory::Allocator< sdUINews > );

	sdUITypeInfo::Init();

	sdUIObject::InitFunctions();
	sdUIWindow::InitFunctions();
	sdUIWindow_Shaped::InitFunctions();
	sdUIList::InitFunctions();
	sdUIEdit::InitFunctions();
	sdUIEditW::InitFunctions();
	sdUIBinder::InitFunctions();
	sdUISlider::InitFunctions();
	sdUIIconNotification::InitFunctions();
	sdUIRadialMenu::InitFunctions();
	sdUITimeline::InitFunctions();
	sdUIMarquee::InitFunctions();
	sdUIRenderWorld::InitFunctions();
	sdUILayout_Vertical::InitFunctions();
	sdUICrosshairInfo::InitFunctions();
	sdUIProgress::InitFunctions();
	sdUICreditScroll::InitFunctions();
	sdUINews::InitFunctions();

	sdUserInterfaceLocal::InitFunctions();
	sdUserInterfaceLocal::InitEvaluators();

	// render callbacks
	RegisterRenderCallback( "commandMap",			sdCommandMapPost::DrawLocalPlayerCommandMapInfo,			UIRC_PRE );
	RegisterRenderCallback( "commandMapIcons",		sdCommandMapPost::DrawLocalPlayerCommandMapInfo_Icons,		UIRC_PRE );
	RegisterRenderCallback( "vehicleAttitude",		idPlayer::DrawLocalPlayerAttitude,							UIRC_PRE );
	RegisterRenderCallback( "postProcess",			sdAtmosphere::DrawPostProcess,								UIRC_POST );

	// input handlers
	RegisterInputHandler( "commandMap",				sdCommandMapPost::HandleLocalPlayerCommandMapInput );

	cmdSystem->AddCommand( "printGuiProperty", PrintGuiProperty, CMD_FL_CHEAT | CMD_FL_GAME, "Prints a property in the named gui" );
	cmdSystem->AddCommand( "setGuiProperty", SetGuiProperty, CMD_FL_CHEAT | CMD_FL_GAME, "Sets a property in the named gui" );
	cmdSystem->AddCommand( "printGuiStats", PrintGuiStats, CMD_FL_CHEAT | CMD_FL_GAME, "Prints statistics for a gui" );

	r_aspectRatio.RegisterCallback( &invalidateLayout );

#if defined( SD_DEMO_BUILD ) || defined( SD_DEMO_BUILD_CONSTRUCTION )
	sdDeclGUI::AddDefine( va( "SD_DEMO_BUILD %i", 1 ) );
#endif
}

/*
================
sdUserInterfaceManagerLocal::Shutdown
================
*/
void sdUserInterfaceManagerLocal::Shutdown( void ) {
	r_aspectRatio.UnRegisterCallback( &invalidateLayout );

	sdUserInterfaceLocal::Shutdown();

	sdUIObject::ShutdownFunctions();
	sdUIWindow::ShutdownFunctions();
	sdUIWindow_Shaped::ShutdownFunctions();
	sdUIList::ShutdownFunctions();
	sdUIEdit::ShutdownFunctions();
	sdUIEditW::ShutdownFunctions();
	sdUIBinder::ShutdownFunctions();
	sdUISlider::ShutdownFunctions();
	sdUIIconNotification::ShutdownFunctions();
	sdUIRadialMenu::ShutdownFunctions();
	sdUITimeline::ShutdownFunctions();
	sdUIMarquee::ShutdownFunctions();
	sdUIRenderWorld::ShutdownFunctions();
	sdUILayout_Vertical::ShutdownFunctions();
	sdUICrosshairInfo::ShutdownFunctions();
	sdUIProgress::ShutdownFunctions();
	sdUICreditScroll::ShutdownFunctions();
	sdUINews::ShutdownFunctions();

	renderCallbacks.DeleteValues();

	renderCallbacks.Clear();
	inputHandlers.Clear();
	listEnumCallbacks.Clear();
	iconEnumCallbacks.Clear();

	instances.DeleteContents( true );

	nextIndex = 1;
	lastNonGameGuiTime = 0;

	cmdSystem->RemoveCommand( "printGuiProperty" );
	cmdSystem->RemoveCommand( "setGuiProperty" );
	cmdSystem->RemoveCommand( "printGuiStats" );

	windowFactory.Shutdown();

	sdUITypeInfo::Shutdown();
	sdUIFunctionStack::allocator.Shutdown();
}

/*
============
sdUserInterfaceManagerLocal::Update
============
*/
void sdUserInterfaceManagerLocal::Update( bool outOfSequence ) {
	int i;
	idTimer timer;
	if( g_guiSpeeds.GetBool() ) {
		timer.Start();
	}

	int numUpdated = 0;
	for ( i = 0; i < instances.Num(); i++ ) {
		sdUserInterfaceLocal* ui = instances[ i ];
		if ( !ui || !ui->GetShouldUpdate() || !ui->IsActive() ) {
			continue;
		}

		if ( ui->NonGameGui() ) {
			if ( !outOfSequence ) {
				lastNonGameGuiTime = sys->Milliseconds();
			}
			ui->SetCurrentTime( lastNonGameGuiTime );
		} else {
			ui->SetCurrentTime( gameLocal.time + gameLocal.timeOffset );
		}

		ui->Update();
		numUpdated++;
	}

	// jrad - applying themes causes the UI to be destroyed and recreated
	// we want to avoid doing this in the middle of other evaluations
	// so it's deferred to here
	for ( i = 0; i < instances.Num(); i++ ) {
		sdUserInterfaceLocal* ui = instances[ i ];
		if ( !ui || !ui->GetShouldUpdate() ) {
			continue;
		}

		ui->ApplyLatchedTheme();
	}

	if( g_guiSpeeds.GetBool() ) {
		timer.Stop();
		gameLocal.Printf( "numguis: %i  updated: %i  msec: %1.4f\n", instances.Num(), numUpdated, timer.Milliseconds() );
	}
}

/*
================
sdUserInterfaceManagerLocal::Clear
================
*/
void sdUserInterfaceManagerLocal::Clear( bool force ) {
	int i;
	for ( i = 0; i < instances.Num(); i++ ) {
		if ( !instances[ i ] ) {
			continue;
		}

		if ( !instances[ i ]->IsPermanent() || force ) {
			delete instances[ i ];
			instances[ i ] = NULL;
		}
	}	
}

/*
============
sdUserInterfaceManagerLocal::ReloadGUIs
============
*/
void sdUserInterfaceManagerLocal::OnReloadGUI( idDecl* gui ) {

	for( int i = 0; i < uiManagerLocal.instances.Num(); i++ ) {
		sdUserInterfaceLocal* ui = uiManagerLocal.instances[ i ];
		if ( !ui ) {
			continue;
		}

		if( idStr::Icmp( ui->GetName(), gui->GetName() ) != 0 ) {
			continue;
		}

		idStr declName = ui->GetName();
		bool active = ui->IsActive();
		if ( active ) {
			ui->Deactivate( true );
		}

		ui->Clear();
		ui->Load( declName );

		if ( active ) {
			ui->Activate();
		}
		
		guiHandle_t handle = uiManagerLocal.GetHandle( i );
		for( int i = 0; i < uiManagerLocal.reloadCallbacks.Num(); i++ ) {
			uiManagerLocal.reloadCallbacks[ i ]( handle );
		}
	}
}

/*
============
sdUserInterfaceManagerLocal::RegisterRenderCallback
============
*/
void sdUserInterfaceManagerLocal::RegisterRenderCallback( const char* name, uiRenderCallback_t callback, uiRenderCallbackType_t type ) {
	if( callback == NULL ) {
		gameLocal.Warning( "sdUserInterfaceManager::RegisterRenderCallback: NULL callback" );
		return;
	}

	renderCallbackHash_t::Iterator iter = renderCallbacks.Find( name ) ;

	if( iter != renderCallbacks.End() ) {
		gameLocal.Warning( "sdUserInterfaceManager::RegisterRenderCallback: '%s' is already registered", name );
		return;
	}

	uiRenderCallbackEntry_t* entry = new uiRenderCallbackEntry_t;
	entry->func = callback;
	entry->type = type;
	renderCallbacks.Set( name, entry );
}

/*
============
sdUserInterfaceManagerLocal::SetRenderCallback
============
*/
void sdUserInterfaceManagerLocal::SetRenderCallback( sdUIWindow* object, const char* name ) {
	if( !object ) {
		gameLocal.Warning( "sdUserInterfaceManager::SetRenderCallback: NULL object" );
		return;
	}

	renderCallbackHash_t::Iterator iter = renderCallbacks.Find( name );
	if( iter == renderCallbacks.End() ) {
		gameLocal.Warning( "sdUserInterfaceManager::SetRenderCallback: '%s' not found", name );
		return;
	}

	object->SetRenderCallback( iter->second->func, iter->second->type );
}

/*
============
sdUserInterfaceManagerLocal::UnregisterRenderCallback
============
*/
void sdUserInterfaceManagerLocal::UnregisterRenderCallback( const char* name ) {
	renderCallbackHash_t::Iterator iter = renderCallbacks.Find( name );
	if( iter == renderCallbacks.End() ) {
		gameLocal.Warning( "sdUserInterfaceManager::UnregisterRenderCallback: '%s' not found", name );
		return;
	}
	delete iter->second;
	renderCallbacks.Remove( iter );	
}

/*
============
sdUserInterfaceManagerLocal::RegisterInputHandler
============
*/
void sdUserInterfaceManagerLocal::RegisterInputHandler( const char* name, uiInputHandler_t handler ) {
	if ( !handler ) {
		gameLocal.Warning( "sdUserInterfaceManagerLocal::RegisterInputHandler: NULL callback" );
		return;
	}

	if ( inputHandlers.Get( name ) ) {
		gameLocal.Warning( "sdUserInterfaceManagerLocal::RegisterInputHandler: '%s' is already registered", name );
		return;
	}

	inputHandlers.Set( name, handler );
}

/*
============
sdUserInterfaceManagerLocal::UnregisterInputHandler
============
*/
void sdUserInterfaceManagerLocal::UnregisterInputHandler( const char* name ) {
	if( !inputHandlers.Remove( name )) {
		gameLocal.Warning( "sdUserInterfaceManagerLocal::UnregisterInputHandler: '%s' not found", name );
		return;
	}
}

/*
============
sdUserInterfaceManagerLocal::SetInputHandler
============
*/
void sdUserInterfaceManagerLocal::SetInputHandler( sdUIWindow* object, const char* handler ) {
	if( !object ) {
		gameLocal.Warning( "sdUserInterfaceManagerLocal::SetInputHandler: NULL object" );
		return;
	}

	uiInputHandler_t* entry;
	if( !inputHandlers.Get( handler, &entry )) {
		gameLocal.Warning( "sdUserInterfaceManagerLocal::SetInputHandler: '%s' not found", handler );
		return;
	}

	object->SetInputHandler( *entry );
}

/*
============
sdUserInterfaceManagerLocal::RegisterReloadCallback
============
*/
void sdUserInterfaceManagerLocal::RegisterReloadCallback( reloadGUICallback_t callback ) {
	reloadCallbacks.Append( callback );
}

/*
============
sdUserInterfaceManagerLocal::UnregisterReloadCallback
============
*/
void sdUserInterfaceManagerLocal::UnregisterReloadCallback( reloadGUICallback_t callback ) {
	reloadCallbacks.Remove( callback );
}

/*
============
sdUserInterfaceManagerLocal::RegisterCallback
============
*/
template< class T >	
void sdUserInterfaceManagerLocal::RegisterCallback( idHashMap< T >& list, const char* name, T callback ) {
	if( !callback ) {
		gameLocal.Warning( "sdUserInterfaceManager::RegisterCallback: NULL callback" );
		return;
	}

	if( list.Get( name ) ) {
		gameLocal.Warning( "sdUserInterfaceManager::RegisterCallback: '%s' is already registered", name );
		return;
	}

	list.Set( name, callback );
}

/*
============
sdUserInterfaceManagerLocal::UnregisterCallback
============
*/
template< class T >
void sdUserInterfaceManagerLocal::UnregisterCallback( idHashMap< T >& list, const char* name ) {
	if( !list.Remove( name )) {
		gameLocal.Warning( "sdUserInterfaceManager::UnregisterCallback: '%s' not found", name );
	}
}

/*
============
sdUserInterfaceManagerLocal::GetCallback
============
*/
template< class T >
T sdUserInterfaceManagerLocal::GetCallback( idHashMap< T >& list, const char* name ) {
	T* callback;
	if( !list.Get( name, &callback ) ) {
		//gameLocal.Warning( "sdUserInterfaceManager::GetCallback: '%s' not found", name );
		return NULL;
	}

	return *callback;
}

/*
============
sdUserInterfaceManagerLocal::RegisterRadialMenuEnumerationCallback
============
*/
void sdUserInterfaceManagerLocal::RegisterRadialMenuEnumerationCallback( const char* name, uiRadialMenuEnumerationCallback_t callback ) {
	RegisterCallback( radialMenuEnumCallbacks, name, callback );
}


/*
============
sdUserInterfaceManagerLocal::UnregisterRadialMenuEnumerationCallback
============
*/
void sdUserInterfaceManagerLocal::UnregisterRadialMenuEnumerationCallback( const char* name ) {
	UnregisterCallback( radialMenuEnumCallbacks, name );
}

/*
============
sdUserInterfaceManagerLocal::GetRadialMenuEnumerationCallback
============
*/
uiRadialMenuEnumerationCallback_t sdUserInterfaceManagerLocal::GetRadialMenuEnumerationCallback( const char* name ) {
	return GetCallback( radialMenuEnumCallbacks, name );
}

/*
============
sdUserInterfaceManagerLocal::RegisterListEnumerationCallback
============
*/
void sdUserInterfaceManagerLocal::RegisterListEnumerationCallback( const char* name, uiListEnumerationCallback_t callback ) {
	RegisterCallback( listEnumCallbacks, name, callback );
}


/*
============
sdUserInterfaceManagerLocal::UnregisterListEnumerationCallback
============
*/
void sdUserInterfaceManagerLocal::UnregisterListEnumerationCallback( const char* name ) {
	UnregisterCallback( listEnumCallbacks, name );
}

/*
============
sdUserInterfaceManagerLocal::GetListEnumerationCallback
============
*/
uiListEnumerationCallback_t sdUserInterfaceManagerLocal::GetListEnumerationCallback( const char* name ) {
	return GetCallback( listEnumCallbacks, name );
}


/*
============
sdUserInterfaceManagerLocal::RegisterIconEnumerationCallback
============
*/
void sdUserInterfaceManagerLocal::RegisterIconEnumerationCallback( const char* name, uiIconEnumerationCallback_t callback ) {
	RegisterCallback( iconEnumCallbacks, name, callback );
}


/*
============
sdUserInterfaceManagerLocal::UnregisterIconEnumerationCallback
============
*/
void sdUserInterfaceManagerLocal::UnregisterIconEnumerationCallback( const char* name ) {
	UnregisterCallback( iconEnumCallbacks, name );
}

/*
============
sdUserInterfaceManagerLocal::GetIconEnumerationCallback
============
*/
uiIconEnumerationCallback_t sdUserInterfaceManagerLocal::GetIconEnumerationCallback( const char* name ) {
	return GetCallback( iconEnumCallbacks, name );
}


/*
============
sdUserInterfaceManagerLocal::ListGUIs
============
*/
void sdUserInterfaceManagerLocal::ListGUIs( const idCmdArgs& args ) {
	int numGuis = 0;
	int numActive = 0;
	int numPermanent = 0;
	int numInteractive = 0;
	int numUnique = 0;

	for ( int i = 0; i < instances.Num(); i++ ) {
		sdUserInterfaceLocal* ui = instances[ i ];
		if ( !ui ) {
			continue;
		}		
		bool isActive = ui->IsActive();
		bool isPermanent = ui->IsPermanent();
		bool isInteractive = ui->IsInteractive();
		bool isUnique = ui->IsUnique();
		gameLocal.Printf( "%-32s: %s %s %s %s\n", ui->GetDecl()->GetName(),	isActive		? "A"	: " ", 
																			isPermanent		? "P"	: " ", 
																			isInteractive	? "I"	: " ",
																			isUnique		? "U"	: " " );
		if( isActive ) {
			numActive++;
		}
		if( isPermanent ) {
			numPermanent++;
		}
		if( isInteractive ) {
			numInteractive++;
		}
		if( isUnique ) {
			numUnique++;
		}
		numGuis++;
	}
	gameLocal.Printf( "\n%i Total GUIs allocated.\n  %i Active\n  %i Permanent\n  %i Interactive\n  %i Unique\n", numGuis, numActive, numPermanent, numInteractive, numUnique );
}

/*
============
sdUserInterfaceManagerLocal::PrintGuiProperty
============
*/
void sdUserInterfaceManagerLocal::PrintGuiProperty( const idCmdArgs& args ) {
	if( args.Argc() < 3 ) {
		gameLocal.Printf( "usage: printGuiProperty guiName propertyName" );
		return;
	}

	bool found = false;
	for ( int i = 0; i < uiManagerLocal.instances.Num(); i++ ) {
		if ( !uiManagerLocal.instances[ i ] ) {
			continue;
		}
		if( idStr::Icmp( uiManagerLocal.instances[ i ]->GetName(), args.Argv( 1 ) ) == 0 ) {
			sdProperties::sdProperty* property = gameLocal.GetUserInterfaceProperty_r( uiManagerLocal.GetHandle( i ), args.Argv( 2 ), sdProperties::PT_INVALID );

			if( property ) {
				gameLocal.Printf( "'%s' (%i): '%s' == '%s'\n", args.Argv( 1 ), i, args.Argv( 2 ), sdProperties::sdToString( *property ).c_str() );
			} else {
				gameLocal.Printf( "Property '%s' not found in gui '%s'\n", args.Argv( 2 ), args.Argv( 1 ) );
			}
			found = true;
		}
	}	

	if( !found ) {
		gameLocal.Printf( "GUI  '%s' is not active\n", args.Argv( 1 ));
	}
}

/*
============
sdUserInterfaceManagerLocal::SetGuiProperty
============
*/
void sdUserInterfaceManagerLocal::SetGuiProperty( const idCmdArgs& args ) {
	if( args.Argc() < 4 ) {
		gameLocal.Printf( "usage: setGuiProperty guiName propertyName value" );
		return;
	}

	bool found = false;
	for ( int i = 0; i < uiManagerLocal.instances.Num(); i++ ) {
		if ( !uiManagerLocal.instances[ i ] ) {
			continue;
		}
		if( idStr::Icmp( uiManagerLocal.instances[ i ]->GetName(), args.Argv( 1 ) ) == 0 ) {
			sdProperties::sdProperty* property = gameLocal.GetUserInterfaceProperty_r( uiManagerLocal.GetHandle( i ), args.Argv( 2 ), sdProperties::PT_INVALID );

			if( property != NULL ) {
				sdProperties::sdFromString( *property, args.Argv( 3 ) );
				gameLocal.Printf( "SET '%s' (%i): '%s' == '%s'\n", args.Argv( 1 ), i, args.Argv( 2 ), args.Argv( 3 ) );
			} else {
				gameLocal.Printf( "Property '%s' not found in gui '%s'\n", args.Argv( 2 ), args.Argv( 1 ) );
			}
			found = true;
		}
	}	

	if( !found ) {
		gameLocal.Printf( "GUI  '%s' is not active\n", args.Argv( 1 ));
	}
}


/*
============
sdUserInterfaceManagerLocal::PrintGuiStats
============
*/
void sdUserInterfaceManagerLocal::PrintGuiStats( const idCmdArgs& args ) {
	if( args.Argc() < 2 ) {
		gameLocal.Printf( "usage: printGuiStats guiName <showProps>" );
		return;
	}

	using namespace sdProperties;

	bool found = false;
	for( int i = 0; i < uiManagerLocal.instances.Num(); i++ ) {
		if( uiManagerLocal.instances[ i ] == NULL ) {
			continue;
		}
		if( idStr::Icmp( uiManagerLocal.instances[ i ]->GetName(), args.Argv( 1 ) ) == 0 ) {
			sdUserInterfaceLocal* ui = uiManagerLocal.instances[ i ];
			gameLocal.Printf( "  %i windowDefs\n", ui->GetNumWindows() );
			gameLocal.Printf( "  %i bytes of source text\n", ui->GetDecl()->GetTextLength() );

			idToken token;
			idParser src;
			src.SetFlags( sdDeclGUI::LEXER_FLAGS );

			byte*					declBuffer;			// binary buffer retrieved from the decl
			int						declBufferLength;	// binary buffer length retrieved from the decl

			idTokenCache& cache = declManager->GetGlobalTokenCache();
			ui->GetDecl()->GetBinarySource( declBuffer, declBufferLength );

			if( declBufferLength > 0 ) {
				src.LoadMemoryBinary( declBuffer, declBufferLength, va( "%s: %s", ui->GetDecl()->GetFileName(), ui->GetDecl()->GetName() ), &cache );

				int count = 0;
				while( src.ReadToken( &token ) ) {
					count++;
				}
				gameLocal.Printf( "  %i tokens\n", count );

			}
			ui->GetDecl()->FreeSourceBuffer( declBuffer );
			declBuffer = NULL;

			if( args.Argc() >=3 && idStr::Icmp( "showProps", args.Argv( 2 ) ) == 0 ) {

				typedef sdPair< int, int > count_t;
				sdHashMapGeneric< idStr, count_t > attachedProperties;

				for( int i = 0; i < ui->GetNumWindows(); i++ ) {
					int onValidate = 0;
					int onChange = 0;

					sdUIObject* object = ui->GetWindow( i );
					for( int propIndex = 0; propIndex < object->GetScope().GetProperties().Num(); propIndex++ ) {
						const sdPropertyHandler::propertyPair_t& prop = object->GetScope().GetProperties().GetProperty( propIndex );
						count_t& counts = attachedProperties[ prop.first ];
						prop.second->NumAttached( onChange, onValidate );
						counts.first = Max( counts.first, onChange );
						counts.second = Max( counts.second, onValidate );
					}
				}

				sdHashMapGeneric< idStr, count_t >::ConstIterator iter = attachedProperties.Begin();
				while( iter != attachedProperties.End() ) {
					if(  iter->second.first > 0 ||  iter->second.second > 0 ) {
						gameLocal.Printf( "%s: %i %i\n", iter->first.c_str(), iter->second.first, iter->second.second );
					}
					++iter;
				}
			}
			found = true;
		}
	}	

	if( !found ) {
		gameLocal.Printf( "GUI  '%s' not found\n", args.Argv( 1 ));
	}
}

/*
============
sdUserInterfaceManagerLocal::BeginLevelLoad
============
*/
void sdUserInterfaceManagerLocal::BeginLevelLoad() {

}


/*
============
sdUserInterfaceManagerLocal::EndLevelLoad
============
*/
void sdUserInterfaceManagerLocal::EndLevelLoad() {
	for( int i = 0; i < instances.Num(); i++ ) {
		if( instances[ i ] == NULL ) {
			continue;
		}
		instances[ i ]->EndLevelLoad();
	}
}

/*
============
sdUserInterfaceManagerLocal::NumInstances
============
*/
int	sdUserInterfaceManagerLocal::NumInstances() const {
	return instances.Num();
}

/*
============
sdUserInterfaceManagerLocal::GetRenderCallback
============
*/
sdUserInterfaceManagerLocal::renderCallbackHandle_t sdUserInterfaceManagerLocal::GetRenderCallbackHandle( const char* name ) const {
	renderCallbackHash_t::ConstIterator iter = renderCallbacks.Find( name );

	if( iter == renderCallbacks.End() ) {
		gameLocal.Warning( "sdUserInterfaceManager::GetRenderCallbackHandle: '%s' not found", name );
		return renderCallbackHandle_t();
	}
	return iter - renderCallbacks.Begin();
}

/*
============
sdUserInterfaceManagerLocal::GetRenderCallback
============
*/
uiRenderCallback_t sdUserInterfaceManagerLocal::GetRenderCallback( const renderCallbackHandle_t& handle ) const {
	if( !handle.IsValid() || handle >= renderCallbacks.Num() ) {
		return NULL;
	}
	return renderCallbacks.FindIndex( handle )->second->func;
}

/*
============
sdUserInterfaceManagerLocal::OnInputInit
============
*/
void sdUserInterfaceManagerLocal::OnInputInit( void ) {
	for ( int i = 0; i < instances.Num(); i++ ) {
		if ( instances[ i ] == NULL ) {
			continue;
		}
		instances[ i ]->OnInputInit();
	}
}

/*
============
sdUserInterfaceManagerLocal::OnInputShutdown
============
*/
void sdUserInterfaceManagerLocal::OnInputShutdown( void ) {
	for ( int i = 0; i < instances.Num(); i++ ) {
		if ( instances[ i ] == NULL ) {
			continue;
		}
		instances[ i ]->OnInputShutdown();
	}
}

/*
============
sdUserInterfaceManagerLocal::OnLanguageShutdown
============
*/
void sdUserInterfaceManagerLocal::OnLanguageInit( void ) {
	for ( int i = 0; i < instances.Num(); i++ ) {
		if ( instances[ i ] == NULL ) {
			continue;
		}
		instances[ i ]->OnLanguageInit();
	}
}

/*
============
sdUserInterfaceManagerLocal::OnLanguageShutdown
============
*/
void sdUserInterfaceManagerLocal::OnLanguageShutdown( void ) {
	for ( int i = 0; i < instances.Num(); i++ ) {
		if ( instances[ i ] == NULL ) {
			continue;
		}
		instances[ i ]->OnLanguageShutdown();
	}
}

/*
============
sdSys_LangCallback::ReloadLanguage
============
*/
void sdUserInterfaceManagerLocal::ReloadLanguage() {
	InvalidateLayout();
}

idCVar gui_allowSnapshotHitchCorrection( "gui_allowSnapshotHitchCorrection", "1", CVAR_BOOL | CVAR_NOCHEAT, "Correct timelines when notified of large time hitches" );
/*
============
sdUserInterfaceManagerLocal::OnSnapshotHitch
============
*/
void sdUserInterfaceManagerLocal::OnSnapshotHitch( int delta ) const {
	if( gui_allowSnapshotHitchCorrection.GetBool() == false ) {
		return;
	}

	for( int i = 0; i < instances.Num(); i++ ) {
		if( instances[ i ] == NULL ) {
			continue;
		}

		sdUserInterfaceLocal* ui = instances[ i ];
		if( ui->TestGUIFlag( sdUserInterfaceLocal::GUI_FRONTEND ) ) {
			continue;
		}

		ui->OnSnapshotHitch( delta );
	}
//	gameLocal.DPrintf( "sdUserInterfaceManagerLocal::OnSnapshotHitch: corrected %d milliseconds\n", delta );
}


/*
============
sdUserInterfaceManagerLocal::InvalidateLayout
============
*/
void sdUserInterfaceManagerLocal::InvalidateLayout() {
	for ( int i = 0; i < uiManagerLocal.instances.Num(); i++ ) {
		if ( uiManagerLocal.instances[ i ] == NULL ) {
			continue;
		}
		uiManagerLocal.instances[ i ]->MakeLayoutDirty();
	}
}

/*
============
sdUserInterfaceManagerLocal::OnToolTipEvent
============
*/
void sdUserInterfaceManagerLocal::OnToolTipEvent( const char* arg ) const {
	for( int i = 0; i < instances.Num(); i++ ) {
		if( instances[ i ] == NULL ) {
			continue;
		}

		sdUserInterfaceLocal* ui = instances[ i ];
		if( ui->TestGUIFlag( sdUserInterfaceLocal::GUI_FRONTEND ) ) {
			continue;
		}

		ui->OnToolTipEvent( arg );
	}
}
