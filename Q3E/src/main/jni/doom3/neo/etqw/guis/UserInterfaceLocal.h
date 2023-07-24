// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACELOCAL_H__
#define __GAME_GUIS_USERINTERFACELOCAL_H__

#include "../../idlib/PropertiesImpl.h"
#include "UserInterfaceTypes.h"
#include "UITimeline.h"
#include "UserInterfaceScript.h"
#include "../../renderer/DeviceContext.h"
#include "../gamesys/DeviceContextHelper.h"

class sdFloatParmExpression;
class sdSingleParmExpression;
class sdUserInterfaceLocal;
class sdUIExpression;
class sdDeclGUITheme;
class sdHudModule;
class sdDeclInvItem;
class idEntity;
class sdUIObject;
class sdBindContext;
class sdDeclItemPackageNode;

/*
============
sdUserInterfaceState
============
*/
class idEntity;
class sdUserInterfaceState : public sdUserInterfaceScope {
public:
										sdUserInterfaceState( void ) : ui( NULL ) { ; }
										~sdUserInterfaceState( void );

	void								Init( sdUserInterfaceLocal* _ui ) { ui = _ui; }

	virtual bool						IsReadOnly() const { return false; }
	virtual int							NumSubScopes() const { return 0; }
	virtual const char*					GetSubScopeNameForIndex( int index ) { assert( 0 ); return ""; }
	virtual sdUserInterfaceScope*		GetSubScopeForIndex( int index ) { assert( 0 ); return NULL; }

	virtual sdProperties::sdProperty*	GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdProperties::sdProperty*	GetProperty( const char* name );
	virtual sdUIFunctionInstance*		GetFunction( const char* name );
	virtual sdUIEvaluatorTypeBase*		GetEvaluator( const char* name );
	virtual bool						RunNamedFunction( const char* name, sdUIFunctionStack& stack );
	virtual const char*					FindPropertyNameByKey( int propertyKey, sdUserInterfaceScope*& scope ) { return FindPropertyName( boundProperties.PropertyForKey( propertyKey ), scope ); }
	virtual const char*					FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope );

	sdProperties::sdPropertyHandler&	GetProperties() { return properties; }

	virtual int							IndexForProperty( sdProperties::sdProperty*	property );
	virtual void						SetPropertyExpression( int propertyKey, int propertyIndex, sdUIExpression* expression );
	virtual void						ClearPropertyExpression( int propertyKey, int propertyIndex );
	virtual void						RunFunction( int expressionIndex );
	virtual int							AddExpression( sdUIExpression* expression ) { return expressions.Append( expression ); }
	virtual sdUIExpression*				GetExpression( int index ) { return expressions[ index ]; }

	sdProperties::sdPropertyHandler&	GetPropertyHandler( void ) { return properties; }

	virtual sdUIEventHandle				GetEvent( const sdUIEventInfo& info ) const;
	virtual void						AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle );

	void								Clear();
	void								ClearExpressions();
	void								Update( void );

	
	void								OnSnapshotHitch( int delta );

	virtual sdUserInterfaceLocal*		GetUI() { return ui; }
	virtual const char*					GetName() const;

	void								AddTransition( sdUIExpression* expression );
	void								RemoveTransition( sdUIExpression* expression );

	virtual sdUserInterfaceScope*		GetSubScope( const char* name );

	virtual const char*					GetScopeClassName() const { return "sdUserInterfaceLocal"; }

private:
	sdUserInterfaceLocal*				ui;
	sdProperties::sdPropertyHandler		properties;

	idList< sdUIExpression* >			transitionExpressions;
	idList< sdUIExpression* >			expressions;

	sdPropertyBinder					boundProperties;
};


/*
============
sdUserInterfaceLocal
============
*/
SD_UI_PUSH_CLASS_TAG( sdUserInterfaceLocal )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"This is the GUI itself, all window defs must be inside it. You may set properties, " \
	"flags, cache materials, cache sounds, set atmospheres and add events directly on the GUI. " \
	"GUI script functions may be called from any event or timeline by using a " \
	"\"gui.\" prefix to the function name."
/* ============ */
)
SD_UI_CLASS_EXAMPLE_TAG(
/* ============ */
	" gui guis/vehicles/edf_titan {\n" \
		" \t// Properties for the GUI.\n" \
		" \tproperties {\n" \
			" \t\tfloat flags = immediate( flags ) | GUI_FULLSCREEN;\n" \
		" \t}\n" \
		" \t_class_icons\n" \
		" \n" \
		" \t// Cache the titan icon material.\n" \
		" \tmaterials {\n" \
			" \t\t\"icon\"		\"guis/assets/hud/gdf/vehicles/titan\"\n" \
		" \t}\n" \
		" \n" \
		" \t_base_icon // Template: setup background material, player list, etc.\n" \
		" \t_hud_materials // Template: Cache all materials used in the HUD.\n" \
		" \t_position( 0, 2, 10 ) // Template: Setup icon for player position 0\n" \
		" \t_position( 1, 2, 24 ) // Template: Setup icon for player position 1\n" \
		" \n" \
		" \t// Template: Draw a generic GDF missile sight.\n" \
		" \t_gdf_generic_missile_sight( \"dontshowlines\", \"0,0,0,1\", \"0,0,0,1\" )\n" \
	" }"
/* ============ */
)
SD_UI_POP_CLASS_TAG

class sdUIObject;
class sdUIObject_Drawable;
class sdUIEvaluatorTypeBase;
class sdUIEvaluator;
extern const char sdUITemplateFunctionInstance_Identifier[];

typedef sdPair< const sdDeclInvItem*, int > itemBankPair_t;

SD_UI_PROPERTY_TAG(
		"1. Behavior" = "Expand";
		"2. Drawing" = "Expand";
		"3. Cursor" = "Collapse";
)
class sdUserInterfaceLocal : public sdUserInterface {
public:
	typedef sdUITemplateFunction< sdUserInterfaceLocal > uiFunction_t;
	typedef sdUITemplateFunctionInstance< sdUserInterfaceLocal, sdUITemplateFunctionInstance_Identifier > uiFunctionInstance_t;

private:
	enum guiEvent_e {
		GE_CONSTRUCTOR,
		GE_ACTIVATE,
		GE_DEACTIVATE,
		GE_CREATE,
		GE_NAMED,
		GE_PROPCHANGED,
		GE_CVARCHANGED,
		GE_CANCEL,
		GE_TOOLTIPEVENT,
		GE_NUM_EVENTS
	};

	enum soundTestCommand_e {
		STC_MIN = -1,
		STC_START,
		STC_STATUS_RECORDING,
		STC_STATUS_PLAYBACK,
		STC_STATUS_PERCENT,
		STC_MAX
	};

public:
	enum voipCommand_e {
		VOIPC_MIN = -1,
		VOIPC_GLOBAL,
		VOIPC_TEAM,
		VOIPC_FIRETEAM,
		VOIPC_DISABLE,
		VOIPC_MAX
	};

	enum guiFlags_e {
		GUI_FRONTEND					= BITT< 0 >::VALUE,		// receive time updates from the system, rather than the game
		GUI_FULLSCREEN					= BITT< 1 >::VALUE,		// allow for aspect ratio corrections
		GUI_SHOWCURSOR					= BITT< 2 >::VALUE,		// show mouse cursor
		GUI_TOOLTIPS					= BITT< 3 >::VALUE,		// post onQueryTooltip events, display them in windowDef "tooltip"
		GUI_INTERACTIVE					= BITT< 4 >::VALUE,		// allow player interaction
		GUI_SCREENSAVER					= BITT< 5 >::VALUE,		// (in-world) - replaces normal GUI drawing with a simple image when the player is far away
		GUI_CATCH_ALL_EVENTS			= BITT< 6 >::VALUE,		// capture all events, used to prevent keys from making it back to the game
		GUI_NON_FOCUSED_MOUSE_EVENTS	= BITT< 7 >::VALUE,		// send mouse button events to windows that aren't focused
		GUI_USE_MOUSE_PITCH				= BITT< 8 >::VALUE,		// negative m_pitch will cause mouse movement to invert as well
		GUI_INHIBIT_GAME_WORLD			= BITT< 9 >::VALUE,		// the game view should not be drawn if this is set
	};

										sdUserInterfaceLocal( int _spawnId, bool _isUnique, bool _isPermanent, sdHudModule* _module );
	virtual								~sdUserInterfaceLocal( void );

	virtual void						Draw( void );
	virtual void						OnInputInit( void );
	virtual void						OnInputShutdown( void );
	virtual void						OnLanguageInit( void );
	virtual void						OnLanguageShutdown( void );

	void								SetRenderCallback( const char* objectName, uiRenderCallback_t callback, uiRenderCallbackType_t type );

	bool								IsActive( void ) const { return flags.isActive; }
	bool								IsUnique( void ) const { return flags.isUnique; }
	bool								IsPermanent( void ) const { return flags.isPermanent; }

	int									GetSpawnId( void ) const { return spawnId; }
	const char*							GetName( void ) const { return guiDecl ? guiDecl->GetName(): "<INVALID>"; }
	const sdDeclGUI*					GetDecl() const { return guiDecl; }
	virtual sdUIObject*					GetWindow( const char* name );
	virtual const sdUIObject*			GetWindow( const char* name ) const;

	sdUIWindow*							GetDesktop() { return desktop; }
	const sdUIWindow*					GetDesktop() const { return desktop; }

	virtual int							GetNumWindows() { return windows.Num(); }
	virtual sdUIObject*					GetWindow( int index ) { return windows.FindIndex( index )->second; }

	void								OnSnapshotHitch( int delta );
	void								OnToolTipEvent( const char* arg );

	virtual bool						Load( const char* name );

	void								EndLevelLoad();

	bool								IsValidMaterial( uiMaterialCache_t::Iterator iter ) { return iter != materialCache.End(); }

	sdUserInterfaceState&				GetState( void ) { return scriptState; }
	int									AddExpression( sdUIExpression* expression ) { return scriptState.AddExpression( expression ); }

	bool								Translate( const idKey& key, sdKeyCommand** cmd );

	bool								TestGUIFlag( const int flag ) const { return ( idMath::Ftoi( scriptFlags ) & flag ) != 0; }
	void								SetGUIFlag( const int flag ) { scriptFlags = idMath::Ftoi( scriptFlags ) | flag; }
	void								ClearGUIFlag( const int flag ) { scriptFlags = idMath::Ftoi( scriptFlags ) & ~flag; }
	bool								FlagActivated( const float oldValue, const float newValue, const int flag ) const	{ return	( !( idMath::Ftoi( oldValue ) & flag ) && ( idMath::Ftoi( newValue ) & flag ) ); }
	bool								FlagDeactivated( const float oldValue, const float newValue, const int flag ) const	{ return	( ( idMath::Ftoi( oldValue ) & flag ) && !( idMath::Ftoi( newValue ) & flag ) ); }
	bool								FlagChanged( const float oldValue, const float newValue, const int flag ) const		{ return	FlagActivated( oldValue, newValue, flag ) || FlagDeactivated( oldValue, newValue, flag ); }


	sdIntProperty*						AllocIntProperty( void ) { sdIntProperty* p = new sdIntProperty( -1 ); externalProperties.Alloc() = p; return p; }
	sdFloatProperty*					AllocFloatProperty( void ) { sdFloatProperty* p = new sdFloatProperty( 0.f ); externalProperties.Alloc() = p; return p; }
	sdStringProperty*					AllocStringProperty( void ) { sdStringProperty* p = new sdStringProperty(); externalProperties.Alloc() = p; return p; }
	sdWStringProperty*					AllocWStringProperty( void ) { sdWStringProperty* p = new sdWStringProperty(); externalProperties.Alloc() = p; return p; }
	sdVec2Property*						AllocVec2Property( void ) { sdVec2Property* p = new sdVec2Property( idVec2( 0.f, 0.f ) ); externalProperties.Alloc() = p; return p; }
	sdVec3Property*						AllocVec3Property( void ) { sdVec3Property* p = new sdVec3Property( idVec3( 0.f, 0.f, 0.f ) ); externalProperties.Alloc() = p; return p; }
	sdVec4Property*						AllocVec4Property( void ) { sdVec4Property* p = new sdVec4Property( idVec4( 0.f, 0.f, 0.f, 0.f ) ); externalProperties.Alloc() = p; return p; }

	sdUIScript&							GetScript( void ) { return script; }

	virtual const sdDeclGUITheme*		GetTheme() const { return theme; }
	virtual void						SetTheme( const char* theme );

	void								MakeLayoutDirty();


	void								Init();
	virtual void						Clear();

	virtual void						Activate( void );
	virtual void						Deactivate( bool forceDeactivate = false );

										// in-game guis don't need to accumulate mouse movement deltas, since the player will constantly set the mouse position
	void								SetIgnoreLocalCursorUpdates( bool ignore ) {flags.ignoreLocalCursorUpdates = ignore; }
	void								SetCursor( const int x, const int y );

	virtual void						Update( void );

	bool								RunEvent( const sdUIEventInfo& info );
	virtual bool						PostEvent( const sdSysEvent* event );
	bool								PostNamedEvent( const char* event, bool allowMissing = false );

	void								RegisterTimelineWindow( sdUIObject* window );

	bool								NonGameGui( void ) { return TestGUIFlag( GUI_FRONTEND ); }

	static void							InitFunctions( void );
	static void							InitEvaluators( void );
	static uiFunction_t*				FindFunction( const char* name );
	static void							Shutdown( void );

	void								CreateEvents( const sdDeclGUI* guiDecl, idTokenCache& tokenCache );
	void								EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache );

	sdUITimelineManager*				GetTimelineManager() { return timelines.Get(); }

	// Events
	virtual int							GetNumEvents( const sdUIEventHandle type ) { return events.Num( type ); }
	virtual sdUIEventHandle				GetEvent( const sdUIEventInfo& info ) const;
	void								AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle );

	virtual int							GetFirstEventType() { return 0; }
	virtual int							GetNextEventType( int event ) { return ++event; }
	virtual int							GetMaxEventTypes( void ) { return GE_NUM_EVENTS; }

	virtual const char*					GetNameForEventId( int event ) const { return eventNames[ event ]; }

	sdUIFunctionInstance*				GetFunction( const char* name );
	sdUIEvaluatorTypeBase*				GetEvaluator( const char* name );

	uiMaterialCache_t::Iterator			SetCachedMaterial( const char* alias, const char* newMaterial, int& handle );
	uiMaterialCache_t::Iterator			FindCachedMaterial( const char* alias, int& handle );
	uiMaterialCache_t::Iterator			FindCachedMaterialForHandle( int handle );

	static void							SetupMaterialInfo( uiMaterialInfo_t& mi, int* baseWidth = NULL, int* baseHeight = NULL );
	static void							LookupPartSizes( uiDrawPart_t* parts, int num );
	static int							ParseMaterial( const char* mat, idStr& outMaterial, bool& globalLookup, bool& literal, uiDrawMode_e& mode );
	void								LookupMaterial( const char* materialName, uiMaterialInfo_t& mi, int* baseWidth = NULL, int* baseHeight = NULL );
	void								InitPartsForBaseMaterial( const char* material, uiCachedMaterial_t& cached );
	void								SetupPart( uiDrawPart_t& part, const char* partName, const char* material );

	void								EnumerateItemNodeForBank( idList< itemBankPair_t >& items, int slot, const sdDeclItemPackageNode& node, int packageIndex );
	void								EnumerateItemsForBank( idList< itemBankPair_t >& items, const char* className, int slot, int package );
	void								PopScriptVar( idStr& str )	{ scriptStack.Pop( str ); }
	void								PopScriptVar( float& f )	{ scriptStack.Pop( f ); }
	void								PopScriptVar( idVec4& v )	{ scriptStack.Pop( v ); }
	void								PopScriptVar( idVec3& v )	{ scriptStack.Pop( v ); }
	void								PopScriptVar( idVec2& v )	{ scriptStack.Pop( v ); }
	void								PopScriptVar( int& i )		{ scriptStack.Pop( i ); }
	void								PopScriptVar( bool& b )		{ scriptStack.Pop( b ); }
	void								ClearScriptStack()			{ scriptStack.Clear(); }
	bool								IsScriptStackEmpty() const	{ return scriptStack.Empty(); }

	void								PushScriptVar( const idWStr& str )	{ scriptStack.Push( str.c_str() ); }
	void								PushScriptVar( const idStr& str )	{ scriptStack.Push( str ); }
	void								PushScriptVar( float f )			{ scriptStack.Push( f ); }
	void								PushScriptVar( const idVec2& v )	{ scriptStack.Push( v ); }
	void								PushScriptVar( const idVec4& v )	{ scriptStack.Push( v ); }
	void								PushScriptVar( int i )				{ scriptStack.Push( i ); }

										// up to 16 strings can be pushed into each stack (see the definition of stringStack_t)
	void								GetGeneralScriptVar( const char* stackName, idStr& str );
	void								PopGeneralScriptVar( const char* stackName, idStr& str );
	void								PushGeneralScriptVar( const char* stackName, const char* str );
	void								ClearGeneralStrings( const char* stackName );

	void								Script_ConsoleCommand( sdUIFunctionStack& stack );
	void								Script_ConsoleCommandImmediate( sdUIFunctionStack& stack );
	void								Script_PostNamedEvent( sdUIFunctionStack& stack );
	void								Script_PostNamedEventOn( sdUIFunctionStack& stack );
	void								Script_PlaySound( sdUIFunctionStack& stack );
	void								Script_PlayGameSound( sdUIFunctionStack& stack );
	void								Script_PlayGameSoundDirectly( sdUIFunctionStack& stack );
	void								Script_PlayMusic( sdUIFunctionStack& stack );
	void								Script_StopMusic( sdUIFunctionStack& stack );
	void								Script_PlayVoice( sdUIFunctionStack& stack );
	void								Script_StopVoice( sdUIFunctionStack& stack );
	void								Script_QuerySpeakers( sdUIFunctionStack& stack );
	void								Script_FadeSoundClass( sdUIFunctionStack& stack );
	void								Script_Activate( sdUIFunctionStack& stack );
	void								Script_Deactivate( sdUIFunctionStack& stack );
	void								Script_SendNetworkCommand( sdUIFunctionStack& stack );
	void								Script_SendCommand( sdUIFunctionStack& stack );
	void								Script_ScriptCall( sdUIFunctionStack& stack );
//	void								Script_Resupply( sdUIFunctionStack& stack );
	void								Script_GetKeyBind( sdUIFunctionStack& stack );
	void								Script_GetParentName( sdUIFunctionStack& stack );
	void								Script_CacheMaterial( sdUIFunctionStack& stack );
	void								Script_CollapseColors( sdUIFunctionStack& stack );

	void								Script_PushString( sdUIFunctionStack& stack );
	void								Script_PushFloat( sdUIFunctionStack& stack );
	void								Script_PushVec2( sdUIFunctionStack& stack );
	void								Script_PushVec3( sdUIFunctionStack& stack );
	void								Script_PushVec4( sdUIFunctionStack& stack );

	void								Script_SetCookieString( sdUIFunctionStack& stack );
	void								Script_GetCookieString( sdUIFunctionStack& stack );

	void								Script_SetCookieInt( sdUIFunctionStack& stack );
	void								Script_GetCookieInt( sdUIFunctionStack& stack );

	void								Script_GetFloatResult( sdUIFunctionStack& stack );
	void								Script_GetVec2Result( sdUIFunctionStack& stack );
	void								Script_GetVec4Result( sdUIFunctionStack& stack );
	void								Script_GetStringResult( sdUIFunctionStack& stack );
	void								Script_GetWStringResult( sdUIFunctionStack& stack );

	void								Script_PushGeneralString( sdUIFunctionStack& stack );
	void								Script_PopGeneralString( sdUIFunctionStack& stack );
	void								Script_GetGeneralString( sdUIFunctionStack& stack );
	void								Script_GeneralStringAvailable( sdUIFunctionStack& stack );
	void								Script_ClearGeneralStrings( sdUIFunctionStack& stack );

	void								Script_Print( sdUIFunctionStack& stack );
	void								Script_ChatCommand( sdUIFunctionStack& stack );
	void								Script_GetStringForProperty( sdUIFunctionStack& stack );
	void								Script_SetPropertyFromString( sdUIFunctionStack& stack );
										// only broadcast to immediate children
	void								Script_BroadcastEventToChildren( sdUIFunctionStack& stack );
										// broadcast recursively to all children
	void								Script_BroadcastEventToDescendants( sdUIFunctionStack& stack );
	void								Script_BroadcastEvent( sdUIFunctionStack& stack );

	void								Script_GetCVar( sdUIFunctionStack& stack );
	void								Script_SetCVar( sdUIFunctionStack& stack );
	void								Script_ResetCVar( sdUIFunctionStack& stack );
	void								Script_GetCVarInt( sdUIFunctionStack& stack );
	void								Script_SetCVarInt( sdUIFunctionStack& stack );
	void								Script_GetCVarFloat( sdUIFunctionStack& stack );
	void								Script_SetCVarFloat( sdUIFunctionStack& stack );
	void								Script_GetCVarBool( sdUIFunctionStack& stack );
	void								Script_SetCVarBool( sdUIFunctionStack& stack );
	void								Script_GetCVarColor( sdUIFunctionStack& stack );
	void								Script_IsCVarLocked( sdUIFunctionStack& stack );

	void								Script_SetShaderParm( sdUIFunctionStack& stack );
	void								Script_GetWeaponBankForName( sdUIFunctionStack& stack );
	void								Script_GetWeaponData( sdUIFunctionStack& stack );
	void								Script_GetNumWeaponPackages( sdUIFunctionStack& stack );
	void								Script_GetRoleCountForTeam( sdUIFunctionStack& stack );
	void								Script_GetEquivalentClass( sdUIFunctionStack& stack );
	void								Script_GetStringMapValue( sdUIFunctionStack& stack );
	void								Script_GetPersistentRankInfo( sdUIFunctionStack& stack );
	void								Script_SetSpawnPoint( sdUIFunctionStack& stack );
	void								Script_HighlightSpawnPoint( sdUIFunctionStack& stack );

	void								Script_GetClassSkin( sdUIFunctionStack& stack );

	void								Script_MutePlayer( sdUIFunctionStack& stack );
	void								Script_MutePlayerQuickChat( sdUIFunctionStack& stack );

	void								Script_GetTeamPlayerCount( sdUIFunctionStack& stack );

	void								Script_UpdateLimboProficiency( sdUIFunctionStack& stack );
	void								Script_GetSpecatorList( sdUIFunctionStack& stack );
	void								Script_CopyHandle( sdUIFunctionStack& stack );

	void								Script_ActivateMenuSoundWorld( sdUIFunctionStack& stack );
	void								Script_ToggleReady( sdUIFunctionStack& stack );

	void								Script_ExecVote( sdUIFunctionStack& stack );
	void								Script_GetCommandMapTitle( sdUIFunctionStack& stack );
	void								Script_IsBackgroundLoadComplete( sdUIFunctionStack& stack );

	void								Script_CopyText( sdUIFunctionStack& stack );
	void								Script_PasteText( sdUIFunctionStack& stack );

	void								Script_UploadLevelShot( sdUIFunctionStack& stack );
	void								Script_GetGameTag( sdUIFunctionStack& stack );

	void								Script_SpectateClient( sdUIFunctionStack& stack );
	void								Script_GetLoadTip( sdUIFunctionStack& stack );
	void								Script_CancelToolTip( sdUIFunctionStack& stack );
	void								Script_CheckCVarsAgainstCFG( sdUIFunctionStack& stack );
	void								Script_OpenURL( sdUIFunctionStack& stack );

	void								Script_SoundTest( sdUIFunctionStack& stack );
	void								Script_DeleteFile( sdUIFunctionStack& stack );

	void								Script_VoiceChat( sdUIFunctionStack& stack );
	void								Script_RefreshSoundDevices( sdUIFunctionStack& stack );

	void								SetFocus( sdUIWindow* focus );
	bool								IsFocused( const sdUIObject* focus ) const { return focusedWindow == focus; }

	void								SetEntity( idEntity* entity )		{ this->entity = entity; }
	idEntity*							GetEntity() const					{ return entity; }

	float								GetShaderParms( int idx )			{ return shaderParms[idx-4]; }

	bool								GetUseLocalTime() const				{ return TestGUIFlag( GUI_FRONTEND ); }
	void								SetUseLocalTime( bool localTime )	{	if( localTime ) {
																					SetGUIFlag( GUI_FRONTEND );
																				} else {
																					ClearGUIFlag( GUI_FRONTEND );
																				}
																			}

	bool								GetShouldUpdate() const				{ return flags.shouldUpdate; }
	bool								SetShouldUpdate( bool update )		{ return flags.shouldUpdate = update; }

	virtual void						SetCurrentTime( int now )			{ currentTime = now; }
	virtual int							GetCurrentTime() const				{ return currentTime; }

	const char*							GetMaterial( const char* key ) const;
	const char*							GetSound( const char* key ) const;
	idVec4								GetColor( const char* key ) const;

	void								ApplyLatchedTheme();

	bool								IsInteractive() { return TestGUIFlag( GUI_INTERACTIVE ); }

	void								PushColor( const idVec4& color );
	const idVec4&						TopColor() const;
	idVec4								PopColor();

	static float						Eval_Compare( const sdUIEvaluator* evaluator );
	static float						Eval_Icompare( const sdUIEvaluator* evaluator );
	static float						Eval_Wcompare( const sdUIEvaluator* evaluator );
	static float						Eval_Iwcompare( const sdUIEvaluator* evaluator );
	static float						Eval_Hcompare( const sdUIEvaluator* evaluator );
	static idStr						Eval_ToString( const sdUIEvaluator* evaluator );
	static idWStr						Eval_ToWString( const sdUIEvaluator* evaluator );
	static float						Eval_ToFloat( const sdUIEvaluator* evaluator );
	static idWStr						Eval_LocalizeArgs( const sdUIEvaluator* evaluator );
	static idWStr						Eval_FormatWString( const sdUIEvaluator* evaluator );
	static idWStr						Eval_LocalizeHandle( const sdUIEvaluator* evaluator );
	static int							Eval_Localize( const sdUIEvaluator* evaluator );
	static float						Eval_FAbs( const sdUIEvaluator* evaluator );
	static float						Eval_Square( const sdUIEvaluator* evaluator );
	static float						Eval_Vec2Length( const sdUIEvaluator* evaluator );
	static idVec2						Eval_Vec2Normalize( const sdUIEvaluator* evaluator );
	static float						Eval_Clamp( const sdUIEvaluator* evaluator );
	static float						Eval_Min( const sdUIEvaluator* evaluator );
	static float						Eval_Max( const sdUIEvaluator* evaluator );
	static idStr						Eval_MSToHMS( const sdUIEvaluator* evaluator );
	static idStr						Eval_SToHMS( const sdUIEvaluator* evaluator );
	static float						Eval_Toggle( const sdUIEvaluator* evaluator );
	static float						Eval_Floor( const sdUIEvaluator* evaluator );
	static float						Eval_Ceil( const sdUIEvaluator* evaluator );
	static idStr						Eval_ToUpper( const sdUIEvaluator* evaluator );
	static idStr						Eval_ToLower( const sdUIEvaluator* evaluator );
	static idWStr						Eval_ToWStr( const sdUIEvaluator* evaluator );
	static idStr						Eval_ToStr( const sdUIEvaluator* evaluator );
	static float						Eval_IsValidHandle( const sdUIEvaluator* evaluator );
	static int							Eval_FloatToHandle( const sdUIEvaluator* evaluator );
	static idVec4						Eval_Color( const sdUIEvaluator* evaluator );
	static float						Eval_StrLen( const sdUIEvaluator* evaluator );
	static float						Eval_WStrLen( const sdUIEvaluator* evaluator );
	static float						Eval_Sin( const sdUIEvaluator* evaluator );
	static float						Eval_Cos( const sdUIEvaluator* evaluator );
	static idVec4						Eval_ColorForIndex( const sdUIEvaluator* evaluator );
	static idVec4						Eval_StringToVec4( const sdUIEvaluator* evaluator );

	sdHudModule*						GetModule( void ) { return module; }

	static idCVar 						g_debugGUIEvents;
	static idCVar 						g_debugGUI;
	static idCVar 						g_debugGUITextRect;
	static idCVar 						g_debugGUITextScale;
	static idCVar						gui_tooltipDelay;
	static idCVar 						gui_doubleClickTime;
	static idCVar						s_volumeMusic_dB;
	static idCVar						g_skipIntro;

	static idCVar						gui_invertMenuPitch;

	static idCVar 						gui_crosshairDef;
	static idCVar 						gui_crosshairKey;
	static idCVar 						gui_crosshairAlpha;
	static idCVar 						gui_crosshairSpreadAlpha;
	static idCVar 						gui_crosshairStatsAlpha;
	static idCVar						gui_crosshairGrenadeAlpha;
	static idCVar						gui_crosshairSpreadScale;
	static idCVar						gui_crosshairColor;

	static idCVar						gui_chatAlpha;
	static idCVar						gui_fireTeamAlpha;
	static idCVar						gui_commandMapAlpha;
	static idCVar						gui_objectiveListAlpha;
	static idCVar						gui_personalBestsAlpha;
	static idCVar						gui_objectiveStatusAlpha;
	static idCVar						gui_obitAlpha;
	static idCVar						gui_voteAlpha;
	static idCVar						gui_tooltipAlpha;
	static idCVar						gui_vehicleAlpha;
	static idCVar						gui_vehicleDirectionAlpha;
	static idCVar						gui_showRespawnText;

	static idStrList					parseStack;
	static void							PushTrace( const char* info );
	static void							PrintStackTrace();
	static void							PopTrace();

private:
	void								DisconnectGlobalCallbacks() {
											for( int i = 0; i < toDisconnect.Num(); i++ ) {
												toDisconnect[ i ].first->RemoveOnChangeHandler( toDisconnect[ i ].second );
											}
											toDisconnect.Clear();
											cvarCallbacks.DeleteContents( true );
										}

										sdUserInterfaceLocal( const sdUserInterfaceLocal& );
	sdUserInterfaceLocal&				operator=( const sdUserInterfaceLocal& );

	void								UpdateToolTip();
	void								CancelToolTip();

	int									NamedEventHandleForString( const char* name );
	void								OnCursorMaterialNameChanged( const idStr& oldValue, const idStr& newValue );
	void								OnPostProcessMaterialNameChanged( const idStr& oldValue, const idStr& newValue );
	void								OnScreenSaverMaterialNameChanged( const idStr& oldValue, const idStr& newValue );
	void								OnFocusedWindowNameChanged( const idStr& oldValue, const idStr& newValue );
	void								OnThemeNameChanged( const idStr& oldValue, const idStr& newValue );
	void								OnBindContextChanged( const idStr& oldValue, const idStr& newValue );
	void								OnScreenDimensionChanged( const idVec2& oldValue, const idVec2& newValue );

	void								OnStringPropertyChanged( int event, const idStr& oldValue, const idStr& newValue );
	void								OnWStringPropertyChanged( int event, const idWStr& oldValue, const idWStr& newValue );
	void								OnIntPropertyChanged( int event, const int oldValue, const int newValue );
	void								OnFloatPropertyChanged( int event, const float oldValue, const float newValue );
	void								OnVec4PropertyChanged( int event, const idVec4& oldValue, const idVec4& newValue );
	void								OnVec3PropertyChanged( int event, const idVec3& oldValue, const idVec3& newValue );
	void								OnVec2PropertyChanged( int event, const idVec2& oldValue, const idVec2& newValue );

	void								BroadcastEventToDescendants_r( sdUIObject* parent, const char* eventName );
	void								OnCVarChanged( idCVar& cvar, int id );

public:

	SD_UI_PROPERTY_TAG(
	title				= "1. Behavior/Cursor Position";
	desc				= "Cursor position in GUI.";
	editor				= "edit";
	datatype			= "vec2";
	)
	sdVec2Property		cursorPos;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Behavior/Screen Dimensions";
	desc				= "GUI screen dimensions";
	editor				= "edit";
	datatype			= "ved2";
	readOnly			= "true";
	)
	sdVec2Property		screenDimensions;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Behavior/Screen Center";
	desc				= "GUI screen center";
	editor				= "edit";
	datatype			= "vec2";
	readOnly			= "true";
	)
	sdVec2Property		screenCenter;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Behavior/Focused Window Name";
	desc				= "Window name of focused window";
	editor				= "edit";
	datatype			= "string";
	alias				= "focusedWindow";
	)
	sdStringProperty	focusedWindowName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
		title				= "3. Cursor/Cursor Material";
		desc				= "Name of the cursor material.";
		editor				= "edit";
		datatype			= "string";
		alias				= "cursorMaterial";
	)
	sdStringProperty	cursorMaterialName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
		title				= "3. Cursor/Cursor Size";
		desc				= "Width and height of the cursor (only used if Show Cursor is true).";
		editor				= "edit";
		datatype			= "vec2";
	)
	sdVec2Property		cursorSize;
	// ===========================================

	SD_UI_PROPERTY_TAG(
		title				= "3. Cursor/Cursor Color";
		desc				= "Color of the cursor.";
		editor				= "edit";
		option1				= "{editorComponents} {r,g,b,a}";
		option2				= "{editorSeparator} {,}";
		datatype			= "vec4";
		aliasdatatype		= "color";
	)
	sdVec4Property		cursorColor;
	// ===========================================

	SD_UI_PROPERTY_TAG(
		title				= "2. Drawing/Screensaver Material";
		desc				= "Name of the screensaver material.";
		editor				= "edit";
		datatype			= "string";
		option1				= "{browseTable} {materials}";
	)
	sdStringProperty	screenSaverName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
		title				= "2. Drawing/Postprocess Material";
		desc				= "Name of the material to draw over the UI. Most-often used to disable bloom on in-game GUIs.";
		editor				= "edit";
		datatype			= "string";
		option1				= "{browseTable} {materials}";
		alias				= "postProcessMaterial";
	)
	sdStringProperty	postProcessMaterialName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
		title				= "2. Drawing/Theme";
		desc				= "Name of the current theme";
		editor				= "edit";
		datatype			= "string";
		alias				= "theme";
	)
	sdStringProperty	themeName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
		title				= "1. Behavior/Flags";
		desc				= "Script behavior flags. See GUI_* flags.";
		editor				= "edit";
		datatype			= "float";
		alias				= "flags";
	)
	sdFloatProperty		scriptFlags;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Behavior/Input scale";
	desc				= "Factor to scale mouse input";
	editor				= "edit";
	datatype			= "float";
	alias				= "flags";
	)
	sdFloatProperty		inputScale;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Behavior/Bind Context Name";
	desc				= "Bind context name. Use a specific bind context for the GUI instead of the default bind context.";
	editor				= "edit";
	datatype			= "string";
	alias				= "bindContext";
	)
	sdStringProperty	bindContextName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Behavior/GUI Time";
	desc				= "Time in milliseconds.";
	editor				= "edit";
	datatype			= "float";
	alias				= "time";
	readOnly			= "true";
	)
	sdFloatProperty		guiTime;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Behavior/Blank Wide String";
	desc				= "A blank wide string.";
	editor				= "edit";
	datatype			= "wstring";
	readOnly			= "true";
	)
	sdWStringProperty	blankWStr;
	// ===========================================

private:
	typedef sdHashMapGeneric< idStr, sdUIObject*, sdHashCompareStrIcmp, sdHashGeneratorIHash > windowHash_t;
	typedef sdPair< sdProperties::sdProperty*, sdProperties::CallbackHandle > callbackHandler_t;

	typedef idStaticList< idStr, 16 > stringStack_t;
	typedef sdHashMapGeneric< idStr, stringStack_t, sdHashCompareStrIcmp, sdHashGeneratorIHash > stackHash_t;

	idList< callbackHandler_t >				toDisconnect;

	stackHash_t								generalStacks;		// general purpose stacks used by the UI script
	sdUIFunctionStack						scriptStack;		// used for passing parameters between code, UI script, and game scripts
	sdUIWindow*								desktop;
	sdUIObject*								focusedWindow;

	// Cursor management
	const idMaterial*						cursorMaterial;
	const idMaterial*						screenSaverMaterial;
	const idMaterial*						postProcessMaterial;

	struct uiFlags_t {
		bool								isUnique					: 1;
		bool								isActive					: 1;
		bool								isPermanent					: 1;
		bool								ignoreLocalCursorUpdates	: 1;
		bool								shouldUpdate				: 1;
	};

	int										spawnId;
	uiFlags_t								flags;
	const sdDeclGUI*						guiDecl;

	sdUserInterfaceState					scriptState;


	windowHash_t							windows;
	idListGranularityOne< sdUIObject* >		timelineWindows;
	sdUIEventTable							events;

	idList< sdProperties::sdPropertyValueBase* > externalProperties;

	sdUIScript								script;

	sdAutoPtr< sdUITimelineManager >		timelines;

	static idHashMap< uiFunction_t* >		uiFunctions;
	static idList< sdUIEvaluatorTypeBase* >	uiEvaluators;

	sdBindContext*							bindContext;

	idStrList								namedEvents;
	idStaticList< float, MAX_ENTITY_SHADER_PARMS - 4 >	shaderParms;

	idEntityPtr< idEntity >					entity;
	sdHudModule*							module;

	int										currentTime;

	const sdDeclGUITheme*					theme;
	idStr									latchedTheme;

	// tooltips
	sdUIWindow*								toolTipWindow;
	sdUIObject*								toolTipSource;
	int										lastMouseMoveTime;
	int										nextAllowToolTipTime;
	idVec2									tooltipAnchor;

	typedef sdUICVarCallback< sdUserInterfaceLocal >	cvarCallback_t;
	friend class sdUICVarCallback< sdUserInterfaceLocal >;

	idList< cvarCallback_t* >				cvarCallbacks;

	uiMaterialCache_t						materialCache;

	sdStack< idVec4 >						colorStack;
	idVec4									currentColor;

	static idBlockAlloc< uiCachedMaterial_t, 64 > materialCacheAllocator;

	static const char*			partNames[ FP_MAX ];

private:
	static	const char* eventNames[ GE_NUM_EVENTS ];
	static const int TOOLTIP_DELAY_MS;
	static const int TOOLTIP_MOVE_TOLERANCE;
};

#endif // __GAME_GUIS_USERINTERFACELOCAL_H__
