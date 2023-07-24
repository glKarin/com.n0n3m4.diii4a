// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACEMANAGERLOCAL_H__
#define __GAME_GUIS_USERINTERFACEMANAGERLOCAL_H__

#undef CreateWindow

#include "UserInterfaceManager.h"
#include "UserInterfaceTypes.h"

class sdUserInterfaceLocal;

class sdUserInterfaceManagerLocal :
	public sdUserInterfaceManager {
public:

										sdUserInterfaceManagerLocal( void );
	virtual								~sdUserInterfaceManagerLocal( void );

	virtual sdUserInterfaceLocal*		GetUserInterface( const guiHandle_t handle );
	virtual void						FreeUserInterface( guiHandle_t handle );
	virtual guiHandle_t					AllocUI( const char* name, bool requireUnique, bool permanent, const char* theme = "default", sdHudModule* module = NULL );
	virtual guiHandle_t					GetHandle( int index );
	virtual int							NumInstances() const;
	virtual void						Init( void );
	virtual void						Shutdown( void );
	virtual void						Clear( bool force = false );	
	virtual void						Update( bool outOfSequence );
	virtual void						OnInputInit( void );
	virtual void						OnInputShutdown( void );
	virtual void						OnLanguageInit( void ) ;
	virtual void						OnLanguageShutdown( void ) ;

	virtual void						BeginLevelLoad( void );
	virtual void						EndLevelLoad( void );
	
	virtual renderCallbackHandle_t		GetRenderCallbackHandle( const char* name ) const;
	virtual uiRenderCallback_t			GetRenderCallback( const renderCallbackHandle_t& handle ) const;

	virtual void						RegisterRenderCallback( const char* name, uiRenderCallback_t callback, uiRenderCallbackType_t type );
	virtual void						UnregisterRenderCallback( const char* name );
	virtual void						SetRenderCallback( sdUIWindow* object, const char* callback );

	virtual void						RegisterInputHandler( const char* name, uiInputHandler_t handler );
	virtual void						UnregisterInputHandler( const char* name );
	virtual void						SetInputHandler( sdUIWindow* object, const char* handler );

	virtual void						RegisterListEnumerationCallback( const char* name, uiListEnumerationCallback_t callback );
	virtual void						UnregisterListEnumerationCallback( const char* name );
	virtual	uiListEnumerationCallback_t	GetListEnumerationCallback( const char* callback );
	
	virtual void								RegisterIconEnumerationCallback( const char* name, uiIconEnumerationCallback_t callback );
	virtual void								UnregisterIconEnumerationCallback( const char* name );
	virtual	uiIconEnumerationCallback_t			GetIconEnumerationCallback( const char* callback );

	virtual void								RegisterRadialMenuEnumerationCallback( const char* name, uiRadialMenuEnumerationCallback_t callback );
	virtual void								UnregisterRadialMenuEnumerationCallback( const char* name );
	virtual	uiRadialMenuEnumerationCallback_t	GetRadialMenuEnumerationCallback( const char* callback );

	virtual sdUIObject*					CreateWindow( const char* typeName ) { return windowFactory.CreateType( typeName ); }

	virtual void						RegisterReloadCallback( reloadGUICallback_t callback );
	virtual void						UnregisterReloadCallback( reloadGUICallback_t callback );

	virtual void						ReloadLanguage();

	virtual void						ListGUIs( const idCmdArgs& args );

	virtual int							GetLastNonGameTime() const { return lastNonGameGuiTime; }
	virtual void						OnSnapshotHitch( int delta ) const;
	virtual void						OnToolTipEvent( const char* arg ) const;

	virtual void						InvalidateLayout();

	static void							OnReloadGUI( idDecl* gui );
	static void							SetGuiProperty( const idCmdArgs& args );
	static void							PrintGuiProperty( const idCmdArgs& args );
	static void							PrintGuiStats( const idCmdArgs& args );

private:
	template< class T >	
	void								RegisterCallback( idHashMap< T >& list, const char* name, T callback );
	template< class T >					
	void								UnregisterCallback( idHashMap< T >& list, const char* name );
	template< class T >
	T									GetCallback( idHashMap< T >& list, const char* name );
	
	int									GetUserInterfaceIndex( const guiHandle_t handle );

private:
	class sdUIInvalidateLayout :
		public idCVarCallback {
	public:
		virtual ~sdUIInvalidateLayout() {}

		virtual void OnChanged() {
			uiManager->InvalidateLayout();
		}
	}									invalidateLayout;

	static const int					GUINUM_BITS			= 9;
	static const int					MAX_UNIQUE_GUIS		= 1 << GUINUM_BITS;

	int									nextIndex;
	idList< sdUserInterfaceLocal* >		instances;
	sdUIObjectFactory					windowFactory;

	int									lastNonGameGuiTime;

	idList< reloadGUICallback_t >		reloadCallbacks;

	struct uiRenderCallbackEntry_t {
		uiRenderCallback_t		func;
		uiRenderCallbackType_t	type;
	};

	typedef sdHashMapGeneric< idStr, uiRenderCallbackEntry_t*, sdHashCompareStrIcmp > renderCallbackHash_t;

	renderCallbackHash_t							renderCallbacks;
	idHashMap< uiInputHandler_t >					inputHandlers;
	idHashMap< uiListEnumerationCallback_t >		listEnumCallbacks;
	idHashMap< uiIconEnumerationCallback_t >		iconEnumCallbacks;
	idHashMap< uiRadialMenuEnumerationCallback_t >	radialMenuEnumCallbacks;

};

#endif // __GAME_GUIS_USERINTERFACEMANAGERLOCAL_H__
