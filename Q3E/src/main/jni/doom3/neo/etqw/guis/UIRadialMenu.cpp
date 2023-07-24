// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../misc/WorldToScreen.h"
#include "UIWindow.h"
#include "UIRadialMenu.h"
#include "UserInterfaceLocal.h"
#include "UserInterfaceManager.h"
#include "../Player.h"
#include "../script/Script_Helper.h"


#include "../../sys/sys_local.h"

// the angles on the circle
static const float DEFAULT_WEDGE_SIZE		= 45.0f;
static const float DEFAULT_WEDGE_START		= 180.0f - DEFAULT_WEDGE_SIZE * 0.5f;
static const float DEFAULT_WEDGE_END		= DEFAULT_WEDGE_START + DEFAULT_WEDGE_SIZE;
static const float LEFT_ARC_START			= DEFAULT_WEDGE_END;
static const float LEFT_ARC_END				= 360.0f - 45.0f;
static const float LEFT_ARC_RANGE			= LEFT_ARC_END - LEFT_ARC_START;

const char sdUITemplateFunctionInstance_IdentifierRadialMenu[] = "sdUIRadialFunction";

idBlockAlloc< sdUIRadialMenu::vec4EvaluatorList_t, 32 > sdUIRadialMenu::vec4EvaluatorListAllocator;


SD_UI_IMPLEMENT_CLASS( sdUIRadialMenu, sdUIWindow )

idHashMap< sdUITemplateFunction< sdUIRadialMenu >* > sdUIRadialMenu::radialMenuFunctions;
SD_UI_PUSH_CLASS_TAG( sdUIRadialMenu )
const char* sdUIRadialMenu::eventNames[ RME_NUM_EVENTS - WE_NUM_EVENTS ] = {
	SD_UI_EVENT_TAG( "onCommand",			"[Command Name]", "Called when the given input command occurs. The game script calls these commands to control navigation within the quick chat." ),

	SD_UI_EVENT_PARM_TAG( "onMeasureItem",	"", "Measure the item size before drawing." ),
		SD_UI_EVENT_PARM( float, "itemIndex", "Item index" )
		SD_UI_EVENT_PARM( float, "itemStyle", "Item style, position of the item if drawing a radial menu." )
		SD_UI_EVENT_PARM( vec2, "size", "Size of rectangle" )
		SD_UI_EVENT_RETURN_PARM( vec2, "New size of rectangle" )
	SD_UI_END_EVENT_TAG

	SD_UI_EVENT_TAG( "onDrawItem",			"", "Draw a quick chat item." ),
		SD_UI_EVENT_PARM( wstring, "key", "Shortcut key" )
		SD_UI_EVENT_PARM( float, "title", "Localized title (convert float to a handle before usage)." )
		SD_UI_EVENT_PARM( float, "chevron", "Material handle for chevron to draw." )
		SD_UI_EVENT_PARM( float, "enabled", "Player is able to execute the quick chat item." )
		SD_UI_EVENT_PARM( rect, "itemRecte", "Item rectangle." )
		SD_UI_EVENT_PARM( float, "itemIndex", "Item index." )
		SD_UI_EVENT_PARM( float, "itemStyle", "Item style, position of the item if drawing a radial menu." )
		SD_UI_EVENT_PARM( string, "drawCallback", "Optional callback when drawing. GUI posts an optional named event with this string." )
	SD_UI_END_EVENT_TAG

	SD_UI_EVENT_TAG( "onDrawContext",		"", "Draw the context button (centered in the radial menu)." ),
		SD_UI_EVENT_PARM( rect, "itemRect", "Context rectangle" )
	SD_UI_END_EVENT_TAG

	SD_UI_EVENT_TAG( "onDrawDeadZone",		"", "Not used." ),
	SD_UI_EVENT_TAG( "onPagePushed",		"", "Called when the player enters a new page." ),
	SD_UI_EVENT_TAG( "onPagePopped",		"", "Called when the player exits a page. currentPage will be -1 if there is no valid page." ),
};
SD_UI_POP_CLASS_TAG

idCVar gui_debugRadialMenus(			"gui_debugRadialMenus",					"0",	CVAR_GAME | CVAR_BOOL,							"Show radial menu debugging info" );
idCVar g_radialMenuStyle(				"g_radialMenuStyle",					"0",	CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE | CVAR_PROFILE,		"Sets the style of the quick chat menu: 0 = radial, 1 = vertical" );
idCVar g_radialMenuUseNumberShortcuts(	"g_radialMenuUseNumberShortcuts",		"1",	CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE | CVAR_PROFILE,			"Use numbers instead of alpha-numeric shortcuts" );
idCVar g_radialMenuMouseSensitivity(	"g_radialMenuMouseSensitivity",			"0.5",	CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE,			"Mouse input scale" );

/*
============
sdUIRadialMenu::radialItem_t::Update
============
*/
bool sdUIRadialMenu::radialItem_t::Update() {
	bool sizeChanged = false;
	if ( scriptUpdateCallback.Length() > 0 ) {
		idPlayer* localPlayer = gameLocal.GetLocalPlayer();
		if ( localPlayer != NULL ) {
			const sdProgram::sdFunction* callback = localPlayer->GetScriptFunction( scriptUpdateCallback.c_str() );
			if ( callback != NULL ) {
				bool wasEnabled = flags.enabled;

				sdScriptHelper h;
				h.Push( scriptUpdateParm.c_str() );
				localPlayer->CallNonBlockingScriptEvent( callback, h );

				const char* returnVal = h.GetReturnedString();
				idStrList list;
				idSplitStringIntoList( list, returnVal, "|" );
				if ( list.Num() >= 5 ) {
					flags.enabled = sdTypeFromString< bool >( list[ 4 ] );
				} else {
					flags.enabled = true;
				}

				if ( flags.enabled || wasEnabled ) {
					if ( list.Num() >= 1 ) {
						title = declHolder.FindLocStr( list[ 0 ] );
						sizeChanged = true;
						/* this can spam the console, only use it for debugging
						if( title->GetState() == DS_DEFAULTED ) {
							gameLocal.Warning( "Invalid title %s", title->GetName() );
						}
						*/
						
					} 
					if ( list.Num() >= 2 ) {
						if ( list[ 1 ].Length() == 0 ) {
							part.mi.Clear();
						} else {
							owner->GetScope().GetUI()->LookupMaterial( list[ 1 ].c_str(), part.mi, &part.width, &part.height );
						}
					}

					commandID.SetNum( 0 );
					commandData.SetNum( 0 );

					if ( list.Num() >= 4 ) {
						commandID.Alloc() = list[ 2 ];
						commandData.Alloc() = list[ 3 ];
					}
					if ( list.Num() >= 6 ) {
						// extra info
						commandData.Alloc() = list[ 5 ];
					}
				}
			}
		}
	}

	return sizeChanged;
}

/*
============
sdUIRadialMenu::InitFunctions
============
*/
#pragma inline_depth( 0 )
#pragma optimize( "", off )
SD_UI_PUSH_CLASS_TAG( sdUIRadialMenu )
void sdUIRadialMenu::InitFunctions() {
	SD_UI_FUNC_TAG( insertItem, "Insert an item into a page." )
		SD_UI_FUNC_PARM( float, "page", "Page number." )
		SD_UI_FUNC_PARM( string, "text", "Item text. \"title||id,id2,id3||data,data1,data2||key\"." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of items." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "insertItem",			new sdUITemplateFunction< sdUIRadialMenu >( 'f', "fs",	&sdUIRadialMenu::Script_InsertItem ) );

	SD_UI_FUNC_TAG( insertPage, "Insert a new page." )
		SD_UI_FUNC_PARM( string, "title", "Page title." )
		SD_UI_FUNC_RETURN_PARM( float, "Page number for current page." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "insertPage",			new sdUITemplateFunction< sdUIRadialMenu >( 'f', "s",	&sdUIRadialMenu::Script_InsertPage ) );

	SD_UI_FUNC_TAG( getItemData, "Get item data." )
		SD_UI_FUNC_PARM( float, "itemPage", "Item page. If -1 then use the current page." )
		SD_UI_FUNC_PARM( float, "itemNumber", "Item number. If -1 then use the current page" )
		SD_UI_FUNC_PARM( float, "dataIndex", "Command data index." )
		SD_UI_FUNC_RETURN_PARM( string, "Commnand data." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "getItemData", 		new sdUITemplateFunction< sdUIRadialMenu >( 's', "fff", &sdUIRadialMenu::Script_GetItemData ) );

	SD_UI_FUNC_TAG( clear, "Clear all pages." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "clear",				new sdUITemplateFunction< sdUIRadialMenu >( 'v', "",	&sdUIRadialMenu::Script_Clear ) );

	SD_UI_FUNC_TAG( clearPage, "Clear all page items." )
		SD_UI_FUNC_PARM( float, "page", "Page index." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "clearPage",			new sdUITemplateFunction< sdUIRadialMenu >( 'v', "f",	&sdUIRadialMenu::Script_ClearPage ) );

	SD_UI_FUNC_TAG( pushPage, "Push page on stack, make it the current page and reset mouse position. Calls onPagePushed event." )
		SD_UI_FUNC_PARM( float, "page", "Page index." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "pushPage",			new sdUITemplateFunction< sdUIRadialMenu >( 'v', "f",	&sdUIRadialMenu::Script_PushPage ) );

	SD_UI_FUNC_TAG( popPage, "Pop the stack of pages. Current page is the page on the top after popping." )
		SD_UI_FUNC_RETURN_PARM( string, "The page popped." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "popPage",				new sdUITemplateFunction< sdUIRadialMenu >( 'f', "",	&sdUIRadialMenu::Script_PopPage ) );

	SD_UI_FUNC_TAG( clearPageStack, "Clear the page stack." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "clearPageStack",		new sdUITemplateFunction< sdUIRadialMenu >( 'v', "",	&sdUIRadialMenu::Script_ClearPageStack ) );

	SD_UI_FUNC_TAG( postCommand, "Post a command with the command data for the item." )
		SD_UI_FUNC_PARM( float, "page", "Page number. Current page if -1." )
		SD_UI_FUNC_PARM( float, "item", "Item number. Current item if -1." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "postCommand", 		new sdUITemplateFunction< sdUIRadialMenu >( 'v', "ff",	&sdUIRadialMenu::Script_PostCommand ) );

	SD_UI_FUNC_TAG( loadFromDef, "Load a radial menu from def files." )
		SD_UI_FUNC_PARM( string, "defName", "Def name to load." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "loadFromDef", 		new sdUITemplateFunction< sdUIRadialMenu >( 'v', "s",	&sdUIRadialMenu::Script_LoadFromDef ) );

	SD_UI_FUNC_TAG( appendFromDef, "Load a radial menu page from def files." )
		SD_UI_FUNC_PARM( string, "defName", "Def name to load." )
		SD_UI_FUNC_PARM( float, "page", "Page index." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "appendFromDef",		new sdUITemplateFunction< sdUIRadialMenu >( 'v', "sf",	&sdUIRadialMenu::Script_AppendFromDef ) );

	SD_UI_FUNC_TAG( fillFromEnumerator, "Fill menu from an enumerator." )
		SD_UI_FUNC_PARM( string, "enumerator", "Name of enumerator." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "fillFromEnumerator",	new sdUITemplateFunction< sdUIRadialMenu >( 'v', "s",	&sdUIRadialMenu::Script_FillFromEnumerator ) );
 
	SD_UI_FUNC_TAG( transitionItemVec4, "Transition for an item." )
		SD_UI_FUNC_PARM( float, "propertyType", "Property type. Either RTP_FORECOLOR/RTP_BACKCOLOR/RTP_PROPERTY_0/RTP_PROPERTY_1/RTP_PROPERTY_2/RTP_PROPERTY_3." )
		SD_UI_FUNC_PARM( vec4, "from", "Transition from." )
		SD_UI_FUNC_PARM( vec4, "to", "Transition to." )
		SD_UI_FUNC_PARM( float, "time", "Time for transition." )
		SD_UI_FUNC_PARM( string, "acceleration", "Non linear transition." )
		SD_UI_FUNC_PARM( float, "item", "Item index." )
		SD_UI_FUNC_PARM( float, "page", "Page index." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "transitionItemVec4",			new sdUITemplateFunction< sdUIRadialMenu >( 'v', "f44fsff",	&sdUIRadialMenu::Script_TransitionItemVec4 ) );

	SD_UI_FUNC_TAG( getItemTransitionVec4Result, "Get the result from a transition given a property type." )
		SD_UI_FUNC_PARM( float, "propertyType", "Where property type is one of:\n* RTP_FORECOLOR\n* RTP_BACKCOLOR\n* RTP_PROPERTY_0\n* RTP_PROPERTY_1\n* RTP_PROPERTY_2\n* RTP_PROPERTY_3." )
		SD_UI_FUNC_PARM( vec4, "default", "Default value is returned if item is not found or propertyType is invalid." )
		SD_UI_FUNC_PARM( float, "item", "Item index." )
		SD_UI_FUNC_PARM( float, "page", "Page index." )
		SD_UI_FUNC_RETURN_PARM( vec4, "Transition value." )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "getItemTransitionVec4Result",	new sdUITemplateFunction< sdUIRadialMenu >( '4', "f4ff",	&sdUIRadialMenu::Script_GetItemTransitionVec4Result ) );

	SD_UI_FUNC_TAG( clearTransitions, "Clear transitions." )
		SD_UI_FUNC_PARM( float, "item", "Item index. All items if less than 0" )
		SD_UI_FUNC_PARM( float, "page", "Page index. All pages if less than 0" )
	SD_UI_END_FUNC_TAG
	radialMenuFunctions.Set( "clearTransitions",			new sdUITemplateFunction< sdUIRadialMenu >( 'v', "ff",		&sdUIRadialMenu::Script_ClearTransitions ) );

	SD_UI_ENUM_TAG( RMF_USE_NUMBER_SHORTCUTS, "Use number shortcuts." )
	sdDeclGUI::AddDefine( va( "RMF_USE_NUMBER_SHORTCUTS %i", RMF_USE_NUMBER_SHORTCUTS ) );

	SD_UI_PUSH_GROUP_TAG( "Radial Transition Property Flags" )

	SD_UI_ENUM_TAG( RTP_FORECOLOR, "Forecolor transition property." )
	sdDeclGUI::AddDefine( va( "RTP_FORECOLOR %i", 		RTP_FORECOLOR ) );
	SD_UI_ENUM_TAG( RTP_BACKCOLOR, "Backcolor transition property." )
	sdDeclGUI::AddDefine( va( "RTP_BACKCOLOR %i", 		RTP_BACKCOLOR ) );
	SD_UI_ENUM_TAG( RTP_PROPERTY_0, "Transition property 0." )
	sdDeclGUI::AddDefine( va( "RTP_PROPERTY_0 %i", 		RTP_PROPERTY_0 ) );
	SD_UI_ENUM_TAG( RTP_PROPERTY_1, "Transition property 1." )
	sdDeclGUI::AddDefine( va( "RTP_PROPERTY_1 %i", 		RTP_PROPERTY_1 ) );
	SD_UI_ENUM_TAG( RTP_PROPERTY_2, "Transition property 2." )
	sdDeclGUI::AddDefine( va( "RTP_PROPERTY_2 %i", 		RTP_PROPERTY_2 ) );
	SD_UI_ENUM_TAG( RTP_PROPERTY_3, "Transition property 3." )
	sdDeclGUI::AddDefine( va( "RTP_PROPERTY_3 %i", 		RTP_PROPERTY_3 ) );

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Draw Style Flags" )

	SD_UI_ENUM_TAG( DS_ARC, "ARC draw style." )
	sdDeclGUI::AddDefine( va( "DS_ARC %i",				DS_ARC ) );
	SD_UI_ENUM_TAG( DS_VERTICAL, "Vertical draw style." )
	sdDeclGUI::AddDefine( va( "DS_VERTICAL %i",			DS_VERTICAL ) );
	SD_UI_ENUM_TAG( DS_INVALID, "Invalid draw style." )
	sdDeclGUI::AddDefine( va( "DS_INVALID %i",			DS_INVALID ) );

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Radial Item Style Flags" )

	SD_UI_ENUM_TAG( RIS_LEFT, "Item style left." )
	sdDeclGUI::AddDefine( va( "RIS_LEFT %i",			RIS_LEFT ) );
	SD_UI_ENUM_TAG( RIS_RIGHT, "Item style right." )
	sdDeclGUI::AddDefine( va( "RIS_RIGHT %i",			RIS_RIGHT ) );
	SD_UI_ENUM_TAG( RIS_CENTER, "Item style center." )
	sdDeclGUI::AddDefine( va( "RIS_CENTER %i",			RIS_CENTER ) );
	SD_UI_ENUM_TAG( RIS_TOP, "Item style top." )
	sdDeclGUI::AddDefine( va( "RIS_TOP %i",				RIS_TOP ) );
	SD_UI_ENUM_TAG( RIS_BOTTOM, "Item style bottom." )
	sdDeclGUI::AddDefine( va( "RIS_BOTTOM %i",			RIS_BOTTOM ) );

	SD_UI_POP_GROUP_TAG
}
SD_UI_POP_CLASS_TAG
#pragma optimize( "", on )
#pragma inline_depth()
/*
============
sdUIRadialMenu::sdUIRadialMenu
============
*/
sdUIRadialMenu::sdUIRadialMenu() :
	currentPage( -1.0f ),
	currentItem( -1.0f ),
	radius( 64.0f ),
	verticalPadding( 0.0f ),
	lastActiveEventFrame ( -1 ) {

	scriptState.GetProperties().RegisterProperty( "currentPage", currentPage );
	scriptState.GetProperties().RegisterProperty( "radius", radius );
	scriptState.GetProperties().RegisterProperty( "currentItem", currentItem );

	scriptState.GetProperties().RegisterProperty( "verticalPadding", verticalPadding );
	scriptState.GetProperties().RegisterProperty( "drawStyle", drawStyle );

	lastArcMoveInfo.Set( 0.0f, 0.0f );
	imaginaryCursorPos.Set( 0.0f, 0.0f );
	drawStyle = DS_ARC;

	UI_ADD_FLOAT_CALLBACK( currentPage, sdUIRadialMenu, OnCurrentPageChanged );
	UI_ADD_FLOAT_CALLBACK( currentPage, sdUIRadialMenu, OnDrawStyleChanged );
	UI_ADD_FLOAT_VALIDATOR( currentItem, sdUIRadialMenu, OnValidateCurrentItem );
	UI_ADD_FLOAT_VALIDATOR( drawStyle, sdUIRadialMenu, OnValidateDrawStyle );
}


/*
============
sdUIRadialMenu::~sdUIRadialMenu
============
*/
sdUIRadialMenu::~sdUIRadialMenu() {
	DisconnectGlobalCallbacks();
}

/*
============
sdUIRadialMenu::EnumerateEvents
============
*/
void sdUIRadialMenu::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	if( !idStr::Icmp( name, "onCommand" )) {
		int id = -1;		
		for( int i = 0; i < flags.Num(); i++ ) {
			const idToken& name = tokenCache[ flags[ i ] ];
			id = NamedEventHandleForString( name.c_str() );
			events.Append( sdUIEventInfo( RME_COMMAND, id ) );
		}
		return;
	}
	
	if( !idStr::Icmp( name, "onMeasureItem" )) {
		events.Append( sdUIEventInfo( RME_MEASUREITEM, 0 ) );
		return;
	}

	if( !idStr::Icmp( name, "onDrawItem" )) {
		events.Append( sdUIEventInfo( RME_DRAWITEM, 0 ) );
		return;
	}

	if( !idStr::Icmp( name, "onDrawContext" )) {
		events.Append( sdUIEventInfo( RME_DRAWCONTEXT, 0 ) );
		return;
	}

	if( !idStr::Icmp( name, "onPagePushed" )) {
		events.Append( sdUIEventInfo( RME_PAGE_PUSHED, 0 ) );
		return;
	}

	if( !idStr::Icmp( name, "onPagePopped" )) {
		events.Append( sdUIEventInfo( RME_PAGE_POPPED, 0 ) );
		return;
	}

	sdUIWindow::EnumerateEvents( name, flags, events, tokenCache );
}

/*
============
sdUIRadialMenu::FindFunction
============
*/
const sdUIRadialMenu::uiTemplateFunction_t* sdUIRadialMenu::FindFunction( const char* name ) {
	sdUITemplateFunction< sdUIRadialMenu >** ptr;
	return radialMenuFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUIRadialMenu::GetFunction
============
*/
sdUIFunctionInstance* sdUIRadialMenu::GetFunction( const char* name ) {
	const sdUITemplateFunction< sdUIRadialMenu >* function = sdUIRadialMenu::FindFunction( name );
	if ( !function ) {		
		return sdUIWindow::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUIRadialMenu, sdUITemplateFunctionInstance_IdentifierRadialMenu >( this, function );
}

/*
============
sdUIRadialMenu::RunNamedMethod
============
*/
bool sdUIRadialMenu::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const sdUITemplateFunction< sdUIRadialMenu >* func = sdUIRadialMenu::FindFunction( name );
	if ( !func ) {
		return sdUIWindow::RunNamedMethod( name, stack );
	}

	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}

/*
============
sdUIRadialMenu::DrawLocal
============
*/
void sdUIRadialMenu::DrawLocal() {
	if( !PreDraw() ) {
		return;
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( player != NULL ) {
		idEntity* entity = gameLocal.localPlayerProperties.GetContextEntity();
		if( entity != NULL ) {
			idVec3 org = entity->GetLastPushedOrigin();
			idMat3 axes = entity->GetLastPushedAxis();
			idBounds bounds = entity->GetPhysics()->GetBounds();

			sdWorldToScreenConverter converter( gameLocal.playerView.GetCurrentView() );

			sdBounds2D screenBounds;
			converter.Transform( bounds, axes, org, screenBounds );

			GetUI()->PushScriptVar( screenBounds.ToVec4() );
			RunEvent( sdUIEventInfo( RME_DRAWCONTEXT, 0 ) );
			GetUI()->ClearScriptStack();
		}
	}

	if( drawStyle == DS_INVALID ) {
		return;
	}

	int currentPageIndex = idMath::Ftoi( currentPage );
	if( currentPageIndex >= 0 && currentPageIndex < pages.Num() ) {
		radialPage_t& page			= pages[ currentPageIndex ];
		int currentItem				= idMath::Ftoi( this->currentItem );

		idVec2 origin;
		
		if( drawStyle == DS_ARC ) {
			origin.Set( cachedClientRect.x + cachedClientRect.z * 0.5f, cachedClientRect.y + cachedClientRect.w * 0.5f );
		} else if( drawStyle == DS_VERTICAL ) {
			origin.Set( cachedClientRect.x + cachedClientRect.z * 0.5f, cachedClientRect.y );
		}
		
		for ( int i = 0; i < page.items.Num(); i++ ) {
			page.items[ i ].Update();
		}

		DrawTitle( page, origin );
		if( drawStyle == DS_ARC ) {
			DrawItemCircle( page, origin );
		} else if( drawStyle == DS_VERTICAL ) {			
			DrawItemLine( page, origin );
		}
	}
}

/*
============
sdUIRadialMenu::DrawItemCircle
============
*/
void sdUIRadialMenu::DrawItemCircle( radialPage_t& page, const idVec2& center ) {
	if ( page.items.Num() < 1 ) {
		return;
	}

	// calculate an approximate angle interval to place the items at
	int numDivisions = page.items.Num();
	float angleInterval = 360.0f / ( numDivisions - 2 );
	
	// calculate the radius the circle will need to be
	float minHeight = page.maxSize.y + verticalPadding;
	float deltaD = 2.0f * radius * idMath::Sin( DEG2RAD( angleInterval * 0.5f ) );
	float deltaW = radius * idMath::Sin( DEG2RAD( angleInterval ) );
	float deltaH = idMath::Sqrt( deltaD * deltaD - deltaW * deltaW );

	float newRadius = radius;
	if ( deltaH > idMath::FLT_EPSILON ) {
		newRadius = ( minHeight / deltaH ) * radius;
		if ( newRadius < radius + page.maxSize.y * 0.5f ) {
			newRadius = radius + page.maxSize.y;
		}
	}

	if ( gui_debugRadialMenus.GetBool() ) {
		deviceContext->DrawCircle( center.x, center.y, idVec2( newRadius, newRadius ), 1.0f, 32, colorGreen );
		deviceContext->DrawClippedRect( center.x - 2.0f, center.y - 2.0f, 4.0f, 4.0f, colorYellow );
	}
	
	// calculate where the first position down can be
	int numRows = ( page.items.Num() - 1 ) / 2;

	// draw the odd item out at the bottom, centered to keep the list from looking lopsided
	bool drawBottomItem = ( ( page.items.Num() - 2 ) % 2 ) != 0 ;
	if( drawBottomItem ) {
		numRows--;
	}

	float currentY = ( numRows * ( page.maxSize.y ) ) * 0.5f;
	float xOffset = cachedClientRect.z * 0.5f - page.maxSize.y;
	
	// draw the first item right in the center - this is the default
	DrawItem( page, page.items[ 0 ], 0, center, center, RIS_CENTER );

	int itemUpto = 2;
	if ( itemUpto >= page.items.Num() ) {
		return;
	}

	DrawItem( page, page.items[ 1 ], 1, center, center - idVec2( 0.0f, currentY + page.maxSize.y + verticalPadding ), RIS_CENTER | RIS_TOP );

	for ( int i = 0; i < numRows; i++, currentY -= page.maxSize.y + verticalPadding ) {
		float currentX = -idMath::Sqrt( idMath::Fabs( newRadius * newRadius - currentY * currentY ) );

		radialItem_t& item = page.items[ itemUpto ];
		idVec2 offset( currentX - xOffset, -currentY );
		int flags = 0;
		if( itemUpto == 1 || itemUpto == 2 ) {
			flags = RIS_TOP;
		} else {
			if( drawBottomItem ) {
				if( itemUpto == page.items.Num() - 3 || itemUpto == page.items.Num() - 2 ) {
					flags = RIS_BOTTOM;
				}
			} else {
				if( itemUpto == page.items.Num() - 2 || itemUpto == page.items.Num() - 1 ) {
					flags = RIS_BOTTOM;
				}
			}
		}

		DrawItem( page, item, itemUpto, center, center + offset, RIS_LEFT | flags);
		if ( gui_debugRadialMenus.GetBool() ) {
			deviceContext->DrawLine( center, center + offset, 1.0f, colorRed );
		}
		itemUpto++;

		if ( itemUpto < page.items.Num() ) {
			radialItem_t& item = page.items[ itemUpto ];
			offset.x = -offset.x;
			DrawItem( page, item, itemUpto, center, center + offset, RIS_RIGHT | flags );
			if ( gui_debugRadialMenus.GetBool() ) {
				deviceContext->DrawLine( center, center + offset, 1.0f, colorRed );
			}
			itemUpto++;
		}
	}

	if( drawBottomItem ) {
		DrawItem( page, page.items[ page.items.Num() - 1 ], page.items.Num() - 1, center, center + idVec2( 0.0f, -currentY + verticalPadding ), RIS_CENTER | RIS_BOTTOM );
	}
}

/*
============
sdUIRadialMenu::DrawItemLine
============
*/
void sdUIRadialMenu::DrawItemLine( radialPage_t& page, const idVec2& center ) {

	float currentX = 0.0f;
	float currentY = 0.0f;
	for ( int i = 0; i < page.items.Num(); i++, currentY += page.maxSize.y + verticalPadding ) {

		radialItem_t& item = page.items[ i ];
		idVec2 offset( currentX, currentY );

		int flags = RIS_CENTER;
		if( i == 0 ) {
			flags |= RIS_TOP;
		} else if( i == page.items.Num() - 1 ) {
			flags |= RIS_BOTTOM;
		}

		DrawItem( page, item, i, center, center + offset, flags );
		if ( gui_debugRadialMenus.GetBool() ) {
			deviceContext->DrawLine( center, center + offset, 1.0f, colorRed );
		}
	}
}

/*
============
ToPolar
============
*/
static void ToPolar( const idVec2& valueExpr, float& angleOut, float& distanceOut ) {
	if( idMath::Fabs( valueExpr.x ) < idMath::FLT_EPSILON && idMath::Fabs( valueExpr.y ) < idMath::FLT_EPSILON ) {
		angleOut = 0.0f;
		distanceOut = 0.0f;
		return;
	}
	idVec2 conversionValue = valueExpr;
	if ( idMath::Fabs( conversionValue.x ) > idMath::FLT_EPSILON ) {
		angleOut = RAD2DEG( idMath::ATan( -conversionValue.y, conversionValue.x ) );
		distanceOut = conversionValue.Length();
	} else {
		if ( conversionValue.y > 0.0f ) {
			distanceOut = conversionValue.y;
			angleOut = -90.0f;
		} else {
			distanceOut = -conversionValue.y;
			angleOut = 90.0f;
		}
	}
	angleOut = idMath::AngleNormalize360( angleOut + 90.0f );
}

/*
============
ToCartesian
============
*/
static idVec2 ToCartesian( const float distanceIn, const float angleIn ) {
	return idVec2( idMath::Cos( DEG2RAD( angleIn - 90.0f ) ) * distanceIn, -idMath::Sin( DEG2RAD( angleIn - 90.0f ) ) * distanceIn );
}

/*
============
sdUIRadialMenu::DrawItem
============
*/
void sdUIRadialMenu::DrawItem( radialPage_t& page, radialItem_t& item, int index, const idVec2& center, const idVec2& offset, int radialItemStyle ) {
	sdBounds2D	drawRect;	

	idVec2 size( page.maxSize.x, item.size.y );

	GetUI()->PushScriptVar( size );
	GetUI()->PushScriptVar( static_cast< float >( radialItemStyle ) );
	GetUI()->PushScriptVar( static_cast< float >( index ) );

	if( RunEvent( sdUIEventInfo( RME_MEASUREITEM, 0 ) ) ) {
		GetUI()->PopScriptVar( size );
	}
	
	GetUI()->ClearScriptStack();

	drawRect.FromRectangle( 0.0f, 0.0f, size.x, size.y );
	drawRect.TranslateSelf( offset );
	drawRect.TranslateSelf( 0.5f * ( offset.x - drawRect.GetMins().x ), 0.0f );
	if( drawStyle == DS_ARC ) {
		drawRect.TranslateSelf( 0.0f, -size.y * 0.5f );
	}
	
	drawRect.TranslateSelf( -0.5f * drawRect.GetWidth(), 0.0f );

	idVec2 delta = drawRect.GetCenter() - center;
	float theta;
	float distance;
	ToPolar( delta, theta, distance );

	item.lastDrawAngle = theta;	
		
	if ( item.title != NULL ) {				
		bool useIndex = TestFlag( RMF_USE_NUMBER_SHORTCUTS );
		const wchar_t* shortcutKey = L"";

		if( !item.commandKey.IsEmpty() || useIndex ) {
			if( useIndex ) {
				if ( !item.commandNumberKey.IsEmpty() ) {
					shortcutKey = va( L"%hs", item.commandNumberKey.c_str() );
				} else {
					if( index == 9 ) {
						shortcutKey = L"0";
					} else {
						shortcutKey = va( L"%i", index + 1 );
					}
				}
			} else {
				shortcutKey = va( L"%hs", item.commandKey.c_str() );
			}			
		}

		GetUI()->PushScriptVar( item.drawCallback.c_str() );
		GetUI()->PushScriptVar( radialItemStyle );
		GetUI()->PushScriptVar( index );
		GetUI()->PushScriptVar( drawRect.ToVec4() );
		GetUI()->PushScriptVar( item.flags.enabled ? 1.0f : 0.0f );
		GetUI()->PushScriptVar( item.flags.drawChevron ? 1.0f : 0.0f );
		GetUI()->PushScriptVar( item.title->Index() );
		GetUI()->PushScriptVar( shortcutKey );

		RunEvent( sdUIEventInfo( RME_DRAWITEM, 0 ) );

		GetUI()->ClearScriptStack();
	}

	if ( gui_debugRadialMenus.GetBool() ) {
//		deviceContext->DrawClippedRect( offset.x - 2.0f, offset.y - 2.0f, 4.0f, 4.0f, idVec4( 1.0f, 0.0f, 1.0f, 0.5f ) );
//		deviceContext->DrawClippedRect( drawRect.GetMins().x, drawRect.GetMins().y, drawRect.GetWidth(), drawRect.GetHeight(), idVec4( 1.0f, 0.0f, 0.0f, 0.5f ) );
	}
}

/*
============
sdUIRadialMenu::DrawTitle
============
*/
void sdUIRadialMenu::DrawTitle( radialPage_t& page, const idVec2& center ) {
	if ( gui_debugRadialMenus.GetBool() ) {
		idVec2 centerPos = GetUI()->screenCenter;
		idVec2 cursorPos = GetUI()->cursorPos;

		deviceContext->DrawClippedRect( cursorPos.x - 3, cursorPos.y - 3, 6, 6, colorYellow );
		deviceContext->DrawLine( centerPos, cursorPos, 1, colorYellow );

		deviceContext->DrawClippedRect( imaginaryCursorPos.x - 3 + centerPos.x, imaginaryCursorPos.y - 3 + centerPos.y, 6, 6, colorGreen );
		deviceContext->DrawLine( centerPos, imaginaryCursorPos + centerPos, 1, colorGreen );
	}

	if( drawStyle == DS_ARC ) {
		if( gui_debugRadialMenus.GetBool() ) {
			idVec4 yellow( colorYellow );
			yellow.w = 0.5f;

			const idVec2& cursor = GetUI()->cursorPos;
			deviceContext->DrawLine( center, cursor, 1.0f, yellow );
		}
	} else if( drawStyle == DS_VERTICAL	) {

	}
}

/*
============
sdUIRadialMenu::HandleArcMouseMove
============
*/
void sdUIRadialMenu::HandleArcMouseMove( const sdSysEvent* event, const idVec2 delta, radialPage_t& page, int& currentItem ) {
	idVec2 newPosition = delta;

	float angle, distance;
	ToPolar( newPosition, angle, distance );

	// clamp the distance to the radius
	if ( idMath::Fabs( distance ) > radius ) {
		distance = idMath::ClampFloat( -radius, radius, distance );
		newPosition = ToCartesian( distance, angle );
		GetUI()->cursorPos = newPosition + GetUI()->screenCenter;
	}

	// find the delta from the last move
	idVec2 lastCursorPos = ToCartesian( lastArcMoveInfo.x, lastArcMoveInfo.y );
	idVec2 moveDelta = ( newPosition - lastCursorPos );

	// if it hasn't moved a few units from the last position that was processed then don't process it
	float moveDist = moveDelta.Length();	

	// record the processing
	lastArcMoveInfo.Set( distance, angle );

	// update where the imaginary cursor is
	imaginaryCursorPos = lastCursorPos + moveDelta;

	int iCurrentItem = idMath::Ftoi( currentItem );

	bool dontSnap = false;
	bool useDelta = false;

	if ( event->IsControllerMouseEvent() ) {
	
	} else {
		// check whether the player has made a fast movement (to be handled gesturally)
		bool isExtremeVerticalItem =	( iCurrentItem != -1 && 
										( ( idMath::Fabs( page.items[ iCurrentItem ].lastDrawAngle - 180.0f ) < 4.0f ) ||
										idMath::Fabs( page.items[ iCurrentItem ].lastDrawAngle ) < 4.0f ) );
		bool isVerticalMove = idMath::Fabs( angle - 180.0f ) < 4.0f || idMath::Fabs( angle ) < 4.0f;

		
		if ( moveDist > 5.0f || ( isVerticalMove && moveDist >= 2.0f ) ) {
			if ( idMath::Fabs( moveDelta.x ) < 2.0f ) {
				// not as sensitive to movements if they're largely up & down
				if ( moveDist > 10.0f || isExtremeVerticalItem ) {
					dontSnap = true;
					useDelta = true;
				}
			} else {
				dontSnap = true;
				useDelta = true;
			}
		}


	//		gameLocal.Printf( "%.1f %.1f\n", moveDelta.x, moveDelta.y );

		if ( useDelta ) {
			// do a super-fast snappy move
			
			// find the closest point of the flick line relative to the center
			idVec2 dir = moveDelta;
			dir.Normalize();
			idVec2 diff = -lastCursorPos;
			float t = diff * dir;
			idVec2 closest = lastCursorPos + t * dir;

			// find out the angle and length of the vector
			float alpha, dist;
			ToPolar( closest - lastCursorPos, alpha, dist );
			alpha = idMath::AngleNormalize360( alpha - 90.0f );

			float closestDistance = closest.Length();
			float distToPoint = idMath::Sqrt( Square( radius ) - Square( closestDistance ) );

			idVec2 point = closest + distToPoint * dir;
			ToPolar( point, angle, distance );
		} else if ( distance < radius * 0.25f ) {
			// move to the dead zone
			if( idMath::Ftoi( drawStyle ) == DS_ARC ) {
				currentItem = 0.0f;
			}
			return;
		}
	}

	// how many items on each side
	int numItems = page.items.Num();
	if ( numItems == 0 ) {
		return;
	}
	int numOnLeft = numItems / 2;

	if( ( ( numItems - 2 ) % 2 ) != 0 ) {
		numOnLeft--;
	}

	// figure out the angle divisions for items
	float angleInterval = LEFT_ARC_RANGE / numOnLeft;
	float midAngle = angle;
	float bestAngle = idMath::INFINITY;

	int	newItem = iCurrentItem;
	int checkedItems = 0;
	bool found = false;
	
	int i = ( iCurrentItem + 1 ) % numItems;
	while( checkedItems < page.items.Num() ) {
		float min = page.items[ i ].lastDrawAngle - angleInterval * 0.5f;
		
		float max = min + angleInterval;
 		float angleDifference = idMath::AngleNormalize360( idMath::Fabs( angle - page.items[ i ].lastDrawAngle ) );

		if ( ( ( angle > min && angle < max ) || ( angle > ( min + 360.0f ) && angle < ( max + 360.0f ) ) ) && angleDifference <= bestAngle ) {
			if( i != 0 || found == false ) {
				newItem = i;
				midAngle = ( min + max ) * 0.5f;
				distance = radius;
				bestAngle = angleDifference;
				found = true;
			}
		}
		i = ( i + 1 ) % numItems;
		checkedItems++;
	}

	// jrad - allow selection of disabled items to improve usability (muscle-memory)
//	if ( page.items[ newItem ].flags.enabled ) {
		currentItem = newItem;
//	}

	if ( !event->IsControllerMouseEvent() ) {
		//
		// OUTPUT
		// calculate the cursor position
		idVec2 newCursorPos = ToCartesian( distance, angle ) + GetUI()->screenCenter;
		if ( !dontSnap ) {
			imaginaryCursorPos = newCursorPos - GetUI()->screenCenter;
		}

		//
		// Debugging!
		newCursorPos = ToCartesian( radius, midAngle ) + GetUI()->screenCenter;

		if ( useDelta ) {
			GetUI()->cursorPos = newCursorPos;
			lastArcMoveInfo.Set( radius, midAngle );
			imaginaryCursorPos = newCursorPos - GetUI()->screenCenter;
		}
	}
}

/*
============
sdUIRadialMenu::PostEvent
============
*/
bool sdUIRadialMenu::PostEvent( const sdSysEvent* event ) {
	int currentPage = idMath::Ftoi( this->currentPage );
	int currentItem = idMath::Ftoi( this->currentItem );

	if( currentPage < 0 || currentPage >= pages.Num() ) {
		return false;
	}

	radialPage_t& page = pages[ currentPage ];

	bool retVal = false;

	if ( event->IsMouseEvent() ) {
		if( ( currentItem < 0 && idMath::Ftoi( drawStyle ) != DS_ARC ) || currentItem > page.items.Num() ) {
			return false;
		}

		idVec2 delta = GetUI()->cursorPos.GetValue() - GetUI()->screenCenter.GetValue();

		if( drawStyle == DS_ARC ) {
			HandleArcMouseMove( event, delta, page, currentItem );
			this->currentItem = idMath::ClampInt( -1, page.items.Num() - 1, currentItem );
		} else if( drawStyle == DS_VERTICAL ) {
			float moveThreshhold = Max< float >( radius, 32.0f );
			if( idMath::Fabs( delta.y ) > moveThreshhold ) {
				GetUI()->cursorPos = GetUI()->screenCenter;

				if( delta.y < 0.0f && currentItem > 0 ) {
					currentItem--;
				} else if( delta.y > 0.0f && currentItem < page.items.Num() ) {
					currentItem++;
				}
			}
			this->currentItem = idMath::ClampInt( 0, page.items.Num() - 1, currentItem );
		}
	
	} else if ( event->IsMouseButtonEvent() ) {
		if ( event->IsButtonDown() ) {
			// handle mouse wheel
			if ( drawStyle == DS_VERTICAL ) {
				mouseButton_t mb = event->GetMouseButton();
				if ( mb == M_MWHEELUP ) {
					currentItem--;
					retVal = true;
				} else if ( mb == M_MWHEELDOWN ) {
					currentItem++;
					retVal = true;
				}
				this->currentItem = idMath::ClampInt( 0, page.items.Num() - 1, currentItem );
			}
		}
	} else if ( event->IsKeyEvent() ) {
		if ( event->IsKeyDown() && lastActiveEventFrame != gameLocal.framenum ) {
			bool useIndex = TestFlag( RMF_USE_NUMBER_SHORTCUTS );
			keyNum_t keyNum = K_INVALID;

			for ( int i = 0; i < page.items.Num(); i++ ) {
				const radialItem_t& item = page.items[ i ];
				if ( !item.flags.enabled ) {
					continue;
				}

				if ( useIndex ) {
					if ( !item.commandNumberKey.IsEmpty() ) {
						keyNum = sys->Keyboard().ConvertCharToKey( item.commandNumberKey.c_str()[0] );	// FIXME: change storage to just a char
					} else {
						if ( i == 9 ) {
							keyNum = K_0;
						} else {
							keyNum = (keyNum_t)( K_1 + i );
						}						
					}
				} else {
					keyNum = sys->Keyboard().ConvertCharToKey( item.commandKey.c_str()[0] );	// FIXME: change storage to just a char
				}

				if ( keyNum == K_INVALID ) {
					continue;
				}

				if ( event->GetKey() == keyNum ) {
					this->currentItem = i;
					for ( int cmdIndex = 0; cmdIndex < item.commandID.Num(); cmdIndex++ ) {
						const char* id = item.commandID[ cmdIndex ];
						int eventID = namedEvents.FindIndex( id );
						if ( eventID == -1 || !RunEvent( sdUIEventInfo( RME_COMMAND, eventID ) ) ) {
							gameLocal.Warning( "sdUIRadialMenu::PostEvent: no command handler provided for '%s' (item '%s')", id, item.title == NULL ? "NULL" : item.title->GetName() );
						} else {
							lastActiveEventFrame = gameLocal.framenum;
						}
					}
					retVal = true;
					break;
				}
			}
		}
	}

	if ( !retVal ) {
		if ( event->IsMouseButtonEvent() || event->IsKeyEvent() ) {
			windowEvent_t eventID = event->IsKeyDown() ? WE_KEYDOWN : WE_KEYUP;

			retVal |= HandleBoundKeyInput( event );
			if ( !retVal && lastActiveEventFrame != gameLocal.framenum ) {
				bool down;
				idKey* key = keyInputManager->GetKeyForEvent( *event, down );
				int keyId = key == NULL ? -1 : key->GetId();

				retVal |= RunEvent( sdUIEventInfo( eventID, keyId ) );

				if ( retVal == true && event->GetMouseButton() == M_MOUSE1 ) {
					lastActiveEventFrame = gameLocal.framenum;
				}
			}
		}
	}

	return retVal;
}

/*
============
sdUIRadialMenu::Script_InsertItem
============
*/
void sdUIRadialMenu::Script_InsertItem( sdUIFunctionStack& stack ) {	
	int pageNum;
	stack.Pop( pageNum );
	if( pageNum < 0 ) {
		pageNum = idMath::Ftoi( currentPage );		
	}

	if( pageNum < 0 || pageNum > pages.Num() ) {
		stack.Push( -1.0f );
		return;
	}

	radialPage_t& page = pages[ pageNum ];
	radialItem_t& item = page.items.Alloc();

	idStr title;
	stack.Pop( title );

	idStrList list;
	idSplitStringIntoList( list, title.c_str(), "||" );
	if( list.Num() >= 1 ) {
		item.title = declHolder.FindLocStr( list[ 0 ].c_str() );
		if( item.title->GetState() == DS_DEFAULTED ) {
			gameLocal.Warning( "Invalid title %s", item.title->GetName() );
		}
	}

	if( list.Num() >= 2 ) {
		idSplitStringIntoList( item.commandID, list[ 1 ].c_str(), "," );
	}

	if( list.Num() >= 3 ) {
		idSplitStringIntoList( item.commandData, list[ 2 ].c_str(), "," );
	}

	if( list.Num() >= 4 ) {
		item.commandKey = list[ 3 ];
	}

	item.commandNumberKey = "";

	stack.Push( page.items.Num() - 1 );

	MakeLayoutDirty();
}

/*
============
sdUIRadialMenu::Script_InsertPage
============
*/
void sdUIRadialMenu::Script_InsertPage( sdUIFunctionStack& stack ) {	
	radialPage_t& page = pages.Alloc();

	idStr title;
	stack.Pop( title );
	page.title = declHolder.FindLocStr( title );
	if( page.title->GetState() == DS_DEFAULTED ) {
		gameLocal.Warning( "Invalid title %s", page.title->GetName() );
	}

	currentPage = pages.Num() - 1;
	stack.Push( currentPage );
}

/*
============
sdUIRadialMenu::Script_PushPage
============
*/
void sdUIRadialMenu::Script_PushPage( sdUIFunctionStack& stack ) {	
	int index;
	stack.Pop( index );
	pageStack.Push( index );
	currentPage = index;

	GetUI()->cursorPos = GetUI()->screenCenter;
	lastArcMoveInfo.x = 0.0f;
	lastArcMoveInfo.y = 0.0f;

	imaginaryCursorPos = GetUI()->screenCenter;

	RunEvent( sdUIEventInfo( RME_PAGE_PUSHED, 0 ) );
}

/*
============
sdUIRadialMenu::Script_PopPage
============
*/
void sdUIRadialMenu::Script_PopPage( sdUIFunctionStack& stack ) {	
	int index = -1;
	
	if( !pageStack.Empty() ) {
		pageStack.Pop();		
	}
	if( !pageStack.Empty() ) {
		index = pageStack.Top();
	}
	currentPage = index;
	stack.Push( index );
	RunEvent( sdUIEventInfo( RME_PAGE_POPPED, 0 ) );
}

/*
============
sdUIRadialMenu::OnCurrentPageChanged
============
*/
void sdUIRadialMenu::OnCurrentPageChanged( const float oldValue, const float newValue ) {
	radialPage_t* oldPage = GetSafePage( idMath::Ftoi( oldValue ) );
	radialPage_t* newPage = GetSafePage( idMath::Ftoi( newValue ) );
	if( oldPage != NULL ) {
		oldPage->currentItem = idMath::Ftoi( currentItem );
	}
	if( newPage != NULL ) {
		if( idMath::Ftoi( drawStyle ) != DS_ARC ) {
			MoveToFirstEnabledItem( newPage->currentItem );
		} else {
			currentItem = -1.0f;
		}
			
		newPage->maxSize.Zero();
		MakeLayoutDirty();
	} else {
		currentItem = 0.0f;
	}
}

/*
============
sdUIRadialMenu::Script_GetItemData
============
*/
void sdUIRadialMenu::Script_GetItemData( sdUIFunctionStack& stack ) {
	int pageNum;
	int itemNum;
	int dataIndex;
	stack.Pop( pageNum );
	stack.Pop( itemNum );
	stack.Pop( dataIndex );

	if( dataIndex < 0 ) {
		dataIndex = 0;
	}

	if( pageNum < 0 ) {
		pageNum = idMath::Ftoi( currentPage );
	}
	
	if( itemNum < 0 ) {
		itemNum = idMath::Ftoi( currentItem );		
	}

	radialItem_t* item = GetSafeItem( pageNum, itemNum );
	if( !item || dataIndex >= item->commandData.Num() ) {
		stack.Push( "" );
		return;
	}
	
	stack.Push( item->commandData[ dataIndex ] );
}


/*
============
sdUIRadialMenu::Script_ClearPage
============
*/
void sdUIRadialMenu::Script_ClearPage( sdUIFunctionStack& stack ) {
	int pageNum;
	stack.Pop( pageNum );
	if( pageNum < 0 ) {
		pageNum = idMath::Ftoi( currentPage );
	}

	if( radialPage_t* page = GetSafePage( pageNum )) {
		page->items.Clear();
		if( pageNum == currentPage ) {
			page->currentItem = -1;
		}
	}
	MakeLayoutDirty();
}

/*
============
sdUIRadialMenu::Script_ClearPageStack
============
*/
void sdUIRadialMenu::Script_ClearPageStack( sdUIFunctionStack& stack ) {
	pageStack.Clear();
}


/*
============
sdUIRadialMenu::Script_PostCommand
============
*/
void sdUIRadialMenu::Script_PostCommand( sdUIFunctionStack& stack ) {
	int pageNum;
	stack.Pop( pageNum );
	if( pageNum < 0.0f ) {
		pageNum = idMath::Ftoi( currentPage );
	}

	int itemNum;
	stack.Pop( itemNum );
	if( itemNum < 0.0f ) {
		itemNum = idMath::Ftoi( currentItem );
	}


	if( radialItem_t* item = GetSafeItem( pageNum, itemNum )) {
		if ( item->flags.enabled ) {
			for( int cmdIndex = 0; cmdIndex < item->commandID.Num(); cmdIndex++ ) {
				const char* id = item->commandID[ cmdIndex ];
				int eventID = namedEvents.FindIndex( id );
				if( eventID == -1 || !RunEvent( sdUIEventInfo( RME_COMMAND, eventID ) ) ) {
					gameLocal.Warning( "sdUIRadialMenu::PostEvent: no command handler provided for '%s' (item '%s')", id, item->title == NULL ? "NULL" : item->title->GetName() );
				}
			}
		}
	}
}

/*
============
sdUIRadialMenu::Script_LoadFromDef
============
*/
void sdUIRadialMenu::Script_LoadFromDef( sdUIFunctionStack& stack ) {
	idStr defName;
	stack.Pop( defName );

	Clear( this );

	if( defName.IsEmpty() ) {
		gameLocal.Warning( "sdUIRadialMenu::Script_LoadFromDef: gui '%s': window '%s', tried to load an empty radialMenuDef", GetUI()->GetName(), name.GetValue().c_str() );
		return;
	}

	const sdDeclRadialMenu* def = gameLocal.declRadialMenuType.LocalFind( defName, true );
	declManager->AddDependency( GetUI()->GetDecl(), def );

	for( int i = 0; i < def->GetNumPages(); i++ ) {
		declManager->AddDependency( GetUI()->GetDecl(), &def->GetPage( i ) );

		radialPage_t& page = pages.Alloc();
		page.title = def->GetPage( i ).GetTitle();
		if( page.title->GetState() == DS_DEFAULTED ) {
			gameLocal.Warning( "sdUIRadialMenu::Script_LoadFromDef: gui '%s': window '%s', defaulted title for '%s'", GetUI()->GetName(), name.GetValue().c_str(), page.title->GetName() );
		}
		page.popFactor = def->GetPage( i ).GetKeys().GetFloat( "popFactor", "1.5" );
		LoadFromDef( def->GetPage( i ), page );
	}
	
	if( pages.Num() ) {
		currentPage = 0.0f;
		if( pages.Front().items.Num() ) {
			currentItem = 0.0f;
		}
	}
	MakeLayoutDirty();
}


/*
============
LoadPrefixFromDict
============
*/
void LoadPrefixFromDict( const char* prefix, const idDict& dict, idStrList& list ) {
	const idKeyValue* kv = dict.FindKey( prefix );
	if( kv ) {
		list.Append( kv->GetValue() );
	}

	int index = 1;
	kv = dict.FindKey( va( "%s%i", prefix, index ) );
	while( kv ) {
		list.Append( kv->GetValue() );
		index++;
		kv = dict.FindKey( va( "%s%i", prefix, index ) );
	}
}

/*
============
sdUIRadialMenu::LoadFromDef
============
*/
void sdUIRadialMenu::LoadFromDef( const sdDeclRadialMenu& def, radialPage_t& page ) {
	const char* tempStr = NULL;
	for( int i = 0; i < def.GetNumItems(); i++ ) {
		if( i >= MAX_ITEMS_PER_PAGE ) {
			const char* pageTitle = va( "%ls", page.title->GetText() );
			gameLocal.Warning( "More than %i items on page '%s'", MAX_ITEMS_PER_PAGE, pageTitle );
			return;
		}

		radialItem_t& item = page.items.Alloc();
		item.owner = this;

		const idDict& keys = def.GetItemKeys( i );
		LoadPrefixFromDict( "data",		keys, item.commandData );
		LoadPrefixFromDict( "command",	keys, item.commandID );
		
		item.commandKey				= keys.GetString( "key" );
		item.commandNumberKey		= keys.GetString( "numberKey" );
		
		item.title					= def.GetItemTitle( i );
		item.flags.drawChevron		= keys.GetBool( "drawChevron", "0" );
		item.flags.enabled			= keys.GetBool( "enabled", "1" );
		item.drawCallback			= keys.GetString( "draw", "" );

		if( item.title->GetState() == DS_DEFAULTED ) {
			gameLocal.Warning( "sdUIRadialMenu::Script_LoadFromDef: gui '%s': window '%s', defaulted title for '%s' on page '%s'", GetUI()->GetName(), name.GetValue().c_str(), item.title->GetName(), page.title->GetName() );
		}

		item.scriptUpdateCallback	= keys.GetString( "scriptUpdate" );
		item.scriptUpdateParm		= keys.GetString( "scriptUpdateParm" );

		tempStr = keys.GetString( "mtr_icon" );
		if( tempStr[ 0 ] != '\0' ) {
			GetUI()->LookupMaterial( tempStr, item.part.mi, &item.part.width, &item.part.height );
		}
	}
}

/*
============
sdUIRadialMenu::Script_AppendFromDef
============
*/
void sdUIRadialMenu::Script_AppendFromDef( sdUIFunctionStack& stack ) {
	idStr defName;
	stack.Pop( defName );
	int pageNum;
	stack.Pop( pageNum );

	if( defName.IsEmpty() ) {
		gameLocal.Warning( "sdUIRadialMenu::Script_LoadFromDef: gui '%s': window '%s', tried to load an empty radialMenuDef", GetUI()->GetName(), name.GetValue().c_str() );
		return;
	}

	const sdDeclRadialMenu* def = gameLocal.declRadialMenuType.LocalFind( defName, true );
	declManager->AddDependency( GetUI()->GetDecl(), def );

	if( pageNum < 0.0f ) {
		pageNum = idMath::Ftoi( currentPage );
	}

	if( radialPage_t* page = GetSafePage( pageNum ) ) {
		LoadFromDef( *def, *page );
	}
	MakeLayoutDirty();
}

/*
============
sdUIRadialMenu::ClearPage
============
*/
void sdUIRadialMenu::Clear( sdUIRadialMenu* menu ) {
	sdUIFunctionStack stack;

	menu->RunNamedMethod( "clear", stack );
}

/*
============
sdUIRadialMenu::InsertPage
============
*/
int sdUIRadialMenu::InsertPage( sdUIRadialMenu* menu, int page, const char* text ) {
	sdUIFunctionStack stack;
	stack.Push( page );
	stack.Push( text );

	if( !menu->RunNamedMethod( "insertPage", stack ) ) {
		return -1;
	}

	int retVal;
	stack.Pop( retVal );
	return retVal;
}

/*
============
sdUIRadialMenu::InsertItem
============
*/
int sdUIRadialMenu::InsertItem( sdUIRadialMenu* menu, int page, const char* text ) {
	sdUIFunctionStack stack;
	stack.Push( page );
	stack.Push( text );

	if( !menu->RunNamedMethod( "insertItem", stack ) ) {
		return -1;
	}

	int retVal;
	stack.Pop( retVal );
	return retVal;
}

/*
============
sdUIRadialMenu::Script_FillFromEnumerator
============
*/
void sdUIRadialMenu::Script_FillFromEnumerator( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );


	uiRadialMenuEnumerationCallback_t enumerator = uiManager->GetRadialMenuEnumerationCallback( name );
	if ( enumerator != NULL ) {
		enumerator( this );
	} else {
		gameLocal.Warning( "sdUIRadialMenu::Script_FillFromEnumerator: '%s' Unknown enumerator '%s'", this->name.GetValue().c_str(), name.c_str() );
	}
}

/*
============
sdUIRadialMenu::Script_Clear
============
*/
void sdUIRadialMenu::Script_Clear( sdUIFunctionStack& stack ) {
	pages.Clear();
	pageStack.Clear();
	currentItem = -1.0f;
	currentPage = -1.0f;
	MakeLayoutDirty();
	lastActiveEventFrame = -1;
}

/*
============
sdUIRadialMenu::OnValidateCurrentItem
============
*/
bool sdUIRadialMenu::OnValidateCurrentItem( const float newValue ) {
	radialPage_t* page = GetSafePage( currentPage );
	if( page == NULL ) {
		return true;	// allow resetting the current selection when we're empty
	}
	int index = idMath::Ftoi( newValue );

	// arc style has a dead zone in the center that can be accessed
	if( ( newValue < 0 && idMath::Ftoi( drawStyle ) != DS_ARC ) || newValue >= page->items.Num() ) {
		return false;
	}
// jrad - allow selection of disabled items to improve usability (muscle-memory)
	/*
	if( page->items[ index ].flags.enabled == false ) {
		return false;
	}
	*/
	return true;
}

/*
============
sdUIRadialMenu::Script_TransitionItemVec4
============
*/
void sdUIRadialMenu::Script_TransitionItemVec4( sdUIFunctionStack& stack ) {
	float property;
	float page;
	float item;
	idVec4 from;
	idVec4 to;
	float time;
	idStr accel;

	stack.Pop( property );
	stack.Pop( from );
	stack.Pop( to );
	stack.Pop( time );
	stack.Pop( accel );
	stack.Pop( item );	
	stack.Pop( page );

	if( time < 0.0f ) {
		gameLocal.Error( "TransitionItemVec4: '%s' duration '%i' out of bounds", name.GetValue().c_str(), idMath::Ftoi( time ) );
		return;
	}

	// handle all pages
	if( page < 0.0f ) {
		for( int i = 0; i < pages.Num(); i++ ) {			
			InitVec4Transition( idMath::Ftoi( property ), from, to, idMath::Ftoi( time ), accel, item, i );
		}
	} else {
		InitVec4Transition( idMath::Ftoi( property ), from, to, idMath::Ftoi( time ), accel, item, page );
	}
}


/*
============
sdUIRadialMenu::InitVec4Transition
============
*/
void sdUIRadialMenu::InitVec4Transition( const int property, const idVec4& from, const idVec4& to, const int time, const idStr& accel, int item, int page ) {
	radialItem_t* radialItem = GetSafeItem( page, item );
	if( radialItem == NULL ) {
		return;
	}

	vec4Evaluator_t* evaluator = radialItem->transition.GetEvaluator( property, true );
	assert( evaluator != NULL );

	evaluator->InitLerp( GetUI()->GetCurrentTime(), GetUI()->GetCurrentTime() + idMath::Ftoi( time ), from, to );

	if( !accel.IsEmpty() ) {
		idLexer src( accel.c_str(), accel.Length(), "TransitionItemVec4", LEXFL_NOERRORS );
		idToken token;

		if( !src.ReadToken( &token ) ) {
			gameLocal.Error( "TransitionItemVec4: '%s' invalid acceleration", name.GetValue().c_str() );
			return;
		}
		bool isTable = idStr::Icmpn( token, "table://", 8 ) == 0;
		if( !isTable ) {
			idVec2 accelTimes;
			accelTimes.x = src.ParseFloat();
			src.ExpectTokenString( "," );
			accelTimes.y = src.ParseFloat();

			evaluator->InitAccelDecelEvaluation( accelTimes.x, accelTimes.y );
		} else if( token.Length() > 8 ) {
			evaluator->InitTableEvaluation( token.c_str() + 8 );
		}
	}
}

/*
============
sdUIRadialMenu::Script_GetItemTransitionVec4Result
============
*/
void sdUIRadialMenu::Script_GetItemTransitionVec4Result( sdUIFunctionStack& stack ) {
	float	property;
	idVec4	defaultValue;
	float	page;
	float	item;

	stack.Pop( property );
	stack.Pop( defaultValue );
	stack.Pop( item );	
	stack.Pop( page );

	if( property < RTP_FORECOLOR || property >= RTP_PROPERTY_MAX ) {
		gameLocal.Error( "GetItemTransitionVec4Result: '%s' property index '%i' out of bounds", name.GetValue().c_str(), idMath::Ftoi( property ) );
		stack.Push( defaultValue );
		return;
	}

	radialItem_t* radialItem = GetSafeItem(  page, item );
	if( radialItem == NULL ) {
		stack.Push( defaultValue );
		return;
	}

	vec4Evaluator_t* evaluator = radialItem->transition.GetEvaluator( idMath::Ftoi( property ), false );

	if( evaluator == NULL || !evaluator->IsInitialized() ) {
		stack.Push( defaultValue );
		return;
	}
	idVec4 result = evaluator->Evaluate( GetUI()->GetCurrentTime() );
	stack.Push( result );
}


/*
============
sdUIRadialMenu::Script_ClearTransitions
============
*/
void sdUIRadialMenu::Script_ClearTransitions( sdUIFunctionStack& stack ) {
	float item;
	float page;

	stack.Pop( item );
	stack.Pop( page );

	if( page >= pages.Num() ) {
		gameLocal.Warning( "ClearTransitions: '%s' page '%i' out of bounds", name.GetValue().c_str(), idMath::Ftoi( page ) );
		return;
	}

	int iItem = idMath::Ftoi( item );

	if( page < 0.0f ) {
		// clear all columns
		for( int i = 0; i < pages.Num(); i++ ) {
			radialPage_t& page = pages[ i ];

			if( iItem < 0 ) {
				for( int trans = 0; trans < page.items.Num(); trans++ ) {
					ClearTransition( page.items[ trans ].transition );
				}				
			} else {
				ClearTransition( page.items[ iItem ].transition );
			}
		}
	} else {
		radialPage_t& radialPage = pages[ page ];
		if( iItem < 0 ) {
			for( int trans = 0; trans < radialPage.items.Num(); trans++ ) {
				ClearTransition( radialPage.items[ trans ].transition );				
			}			
		} else if( iItem < radialPage.items.Num() ) {
			ClearTransition( radialPage.items[ iItem ].transition );
		}
	}
}


/*
============
sdUIRadialMenu::ClearTransition
============
*/
void sdUIRadialMenu::ClearTransition( sdTransition& transition ) {
	static const idVec4 colorInvisible( 0.0f, 0.0f, 0.0f, 0.0f );
	transition.backColor.InitConstant( colorInvisible );
	transition.foreColor.InitConstant( colorWhite );
	transition.FreeEvaluators();
}
/*
============
sdUIRadialMenu::GetEvaluator
============
*/
sdUIRadialMenu::vec4Evaluator_t* sdUIRadialMenu::sdTransition::GetEvaluator( int index, bool allowCreate ) {
	switch( index ) {
		case RTP_FORECOLOR:
			return &foreColor;
		case RTP_BACKCOLOR:
			return &backColor;
		default:
			if( evaluators == NULL ) {
				if( !allowCreate ) {
					return NULL;
				}
				evaluators = vec4EvaluatorListAllocator.Alloc();
				evaluators->SetNum( MAX_VEC4_EVALUATORS );
			}
			return &(*evaluators)[ index ];
	}
}

/*
============
sdUIRadialMenu::MoveToFirstEnabledItem
============
*/
void sdUIRadialMenu::MoveToFirstEnabledItem( int index ) {
	if( currentPage < 0 || currentPage >= pages.Num() ) {
		currentItem = -1.0f;
		return;
	}	
	currentItem = index;
	/* jrad - allow selection of disabled items to improve usability (muscle-memory)
	int item = idMath::Ftoi( currentItem );	
	radialPage_t& page = pages[ idMath::Ftoi( currentPage ) ];

	if( page.items.Num() == 0 ) {
		return;
	}

	if( item < 0 || item >= page.items.Num() ) {
		item = 0;
	}
	
	if( item >= page.items.Num() ) {
		return;
	}

	// see if the current item is available
	page.items[ item ].Update();
	if( page.items[ item ].flags.enabled ) {
		currentItem = item;
		return;
	}

	// find the next item that's available
	for ( int i = 0; i < page.items.Num(); i++ ) {
		page.items[ i ].Update();
		if( page.items[ i ].flags.enabled ) {
			currentItem = i;
			break;
		}
	}
	currentItem = -1.0f;
	*/
}


/*
============
sdUIRadialMenu::OnValidateDrawStyle
============
*/
bool sdUIRadialMenu::OnValidateDrawStyle( const float newValue ) {
	int iValue = idMath::Ftoi( newValue );
	return ( iValue >= DS_INVALID && iValue < DS_MAX );
}


/*
============
sdUIRadialMenu::ApplyLayout
============
*/
void sdUIRadialMenu::ApplyLayout() {	
	if( windowState.recalculateLayout ) {
		int textWidth;
		int textHeight;
		radialPage_t* newPage = GetSafePage( idMath::Ftoi( currentPage ) );
		if( newPage == NULL ) {
			assert( 0 );
			return;
		}
		
		ActivateFont( true );
		sdBounds2D screenBounds( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT );

		for( int i = 0; i < newPage->items.Num(); i++ ) {
			radialItem_t& item = newPage->items[ i ];
			deviceContext->GetTextDimensions( item.title->GetText(), screenBounds, GetDrawTextFlags(), cachedFontHandle, fontSize, textWidth, textHeight );
			item.size.Set( textWidth, textHeight );
			newPage->maxSize.x = Max( idMath::Ftoi( newPage->maxSize.x ), textWidth );
			newPage->maxSize.y = Max( idMath::Ftoi( newPage->maxSize.y ), textHeight );
		}
	}
	sdUIWindow::ApplyLayout();
}

/*
============
sdUIRadialMenu::OnDrawStyleChanged
============
*/
void sdUIRadialMenu::OnDrawStyleChanged( const float oldValue, const float newValue ) {
	MakeLayoutDirty();
}
