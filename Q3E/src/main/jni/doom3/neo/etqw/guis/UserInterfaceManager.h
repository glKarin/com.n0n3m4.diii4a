// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACEMANAGER_H__
#define __GAME_GUIS_USERINTERFACEMANAGER_H__

#ifdef _WIN32
#undef CreateWindow
#endif // _WIN32

#include "UserInterfaceTypes.h"
#include "UIObject.h"

typedef sdFactory< sdUIObject > sdUIObjectFactory;

class sdUserInterfaceLocal;
class sdDeclGUITheme;
class sdHudModule;

class sdUserInterfaceManager {
public:
	typedef void (*reloadGUICallback_t)( guiHandle_t );
	typedef sdHandle< int, -1 > renderCallbackHandle_t;

	virtual ~sdUserInterfaceManager( void ) {}

	virtual sdUserInterface*					GetUserInterface( const guiHandle_t handle ) = 0;
	virtual void								FreeUserInterface( guiHandle_t handle ) = 0;
	virtual guiHandle_t							AllocUI( const char* name, bool requireUnique, bool permanent, const char* theme = "default", sdHudModule* module = NULL ) = 0;
	virtual	guiHandle_t							GetHandle( int index ) = 0;
	virtual int									NumInstances() const = 0;
	virtual void								Init( void ) = 0;
	virtual void								Shutdown( void ) = 0;
	virtual void								Clear( bool force = false ) = 0;	
	virtual void								Update( bool outOfSequence ) = 0;
	virtual void								OnInputInit( void ) = 0;
	virtual void								OnInputShutdown( void ) = 0;
	virtual void								OnLanguageInit( void ) = 0;
	virtual void								OnLanguageShutdown( void ) = 0;

	virtual void								BeginLevelLoad( void ) = 0;
	virtual void								EndLevelLoad( void ) = 0;

	virtual void								RegisterRenderCallback( const char* name, uiRenderCallback_t callback, uiRenderCallbackType_t type ) = 0;
	virtual	void								UnregisterRenderCallback( const char* name ) = 0;
	virtual	void								SetRenderCallback( sdUIWindow* object, const char* callback ) = 0;

	virtual renderCallbackHandle_t				GetRenderCallbackHandle( const char* name ) const = 0;
	virtual uiRenderCallback_t					GetRenderCallback( const renderCallbackHandle_t& handle ) const = 0;

	virtual void								RegisterInputHandler( const char* name, uiInputHandler_t handler ) = 0;
	virtual void								UnregisterInputHandler( const char* name ) = 0;
	virtual void								SetInputHandler( sdUIWindow* object, const char* handler ) = 0;

	virtual void								RegisterListEnumerationCallback( const char* name, uiListEnumerationCallback_t callback ) = 0;
	virtual void								UnregisterListEnumerationCallback( const char* name ) = 0;
	virtual	uiListEnumerationCallback_t			GetListEnumerationCallback( const char* callback ) = 0;

	virtual void								RegisterIconEnumerationCallback( const char* name, uiIconEnumerationCallback_t callback ) = 0;
	virtual void								UnregisterIconEnumerationCallback( const char* name ) = 0;
	virtual	uiIconEnumerationCallback_t			GetIconEnumerationCallback( const char* callback ) = 0;

	virtual void								RegisterRadialMenuEnumerationCallback( const char* name, uiRadialMenuEnumerationCallback_t callback ) = 0;
	virtual void								UnregisterRadialMenuEnumerationCallback( const char* name ) = 0;
	virtual	uiRadialMenuEnumerationCallback_t	GetRadialMenuEnumerationCallback( const char* callback ) = 0;

	virtual void								RegisterReloadCallback( reloadGUICallback_t callback ) = 0;
	virtual void								UnregisterReloadCallback( reloadGUICallback_t callback ) = 0;

	virtual void								ReloadLanguage() = 0;

	virtual sdUIObject*							CreateWindow( const char* typeName ) = 0;

	virtual void								ListGUIs( const idCmdArgs& args ) = 0;

	virtual int									GetLastNonGameTime() const = 0;

	virtual void								OnSnapshotHitch( int delta ) const = 0;
	virtual void								OnToolTipEvent( const char* arg ) const = 0;
	virtual void								InvalidateLayout() = 0;
};

extern sdUserInterfaceManager* uiManager;

#endif // __GAME_GUIS_USERINTERFACEMANAGER_H__
