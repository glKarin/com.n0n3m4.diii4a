
#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UIWindow.h"
#include "UserInterfaceLocal.h"
#include "UINews.h"

#include "../../sys/sys_local.h"

SD_UI_IMPLEMENT_CLASS( sdUINews, sdUIWindow )

idHashMap< sdUITemplateFunction< sdUINews >* >	sdUINews::newsFunctions;

const char* sdUINews::eventNames[ NE_NUM_EVENTS - WE_NUM_EVENTS ] = {
	"<noop>",
	"onDrawButton",
	"onMouseEnterPrev",
	"onMouseExitPrev",
	"onMouseEnterNext",
	"onMouseExitNext",
	"onMouseButtonPrevDown",
	"onMouseButtonPrevUp",
	"onMouseButtonNextDown",
	"onMouseButtonNextUp",
	"onMouseUpText",
	"onMouseDownText",
};

/*
============
sdUINews::sdUINews
============
*/
sdUINews::sdUINews( void ) :
	itemState( IS_INVALID ),
	scrollStartTime( 0 ),
	scrollTargetTime( 0 ),
	currentItem( 0 ),
	textWidth( 0 ),
	textHeight( 0 ),
	displayStartTime( 0 ),
	alphaMul( 1.0f ),
	currentButton( BR_NONE ),
	currentClickedButton( BR_NONE ) {
	scriptState.GetProperties().RegisterProperty( "speed",						speed );
	scriptState.GetProperties().RegisterProperty( "displayTime",				displayTime );
	scriptState.GetProperties().RegisterProperty( "fadeType",					fadeType );
	scriptState.GetProperties().RegisterProperty( "colorLink",					colorLink );
	scriptState.GetProperties().RegisterProperty( "linkState",					linkState );
	scriptState.GetProperties().RegisterProperty( "newsText",					newsText );
	scriptState.GetProperties().RegisterProperty( "pauseNews",					pauseNews );
	scriptState.GetProperties().RegisterProperty( "itemState",					itemState );
	scriptState.GetProperties().RegisterProperty( "url",						url );

	speed				= 2.0f;
	displayTime			= 5;
	fadeType			= FT_VSCROLL;
	colorLink			= colorWhite;
	linkState			= US_NONE;
	pauseNews			= 0.0f;

	newsText.SetReadOnly( true );
	url.SetReadOnly( true );

	SetWindowFlag( WF_CLIP_TO_RECT );
}

/*
============
sdUINews::~sdUINews
============
*/
sdUINews::~sdUINews( void ) {

}

/*
============
sdUINews::GetFunction
============
*/
sdUIFunctionInstance* sdUINews::GetFunction( const char* name ) {
	const sdUITemplateFunction< sdUINews >* function = sdUINews::FindFunction( name );
	if( !function ) {		
		return sdUIWindow::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUINews, sdUITemplateFunctionInstance_IdentifierTimeline >( this, function );
}

/*
============
sdUINews::FindFunction
============
*/
const sdUITemplateFunction< sdUINews >* sdUINews::FindFunction( const char* name ) {
	sdUITemplateFunction< sdUINews >** ptr;
	return newsFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUINews::InitFunctions
============
*/
void sdUINews::InitFunctions() {
	newsFunctions.Set( "prevNewsItem", new sdUITemplateFunction< sdUINews >( 'v', "", &sdUINews::Script_PrevNewsItem ) );
	newsFunctions.Set( "nextNewsItem", new sdUITemplateFunction< sdUINews >( 'v', "", &sdUINews::Script_NextNewsItem ) );
	newsFunctions.Set( "gotoURL", new sdUITemplateFunction< sdUINews >( 'v', "f", &sdUINews::Script_GotoURL ) );

	sdDeclGUI::AddDefine( va( "FT_VSCROLL %i", FT_VSCROLL ) );
	sdDeclGUI::AddDefine( va( "FT_FADE %i", FT_FADE ) );
	sdDeclGUI::AddDefine( va( "BR_NONE %i", BR_NONE ) );
	sdDeclGUI::AddDefine( va( "BR_PREV %i", BR_PREV ) );
	sdDeclGUI::AddDefine( va( "BR_NEXT %i", BR_NEXT ) );
	sdDeclGUI::AddDefine( va( "BR_TEXT %i", BR_TEXT ) );
	sdDeclGUI::AddDefine( va( "US_NONE %i", US_NONE ) );
	sdDeclGUI::AddDefine( va( "US_LINK %i", US_LINK ) );
	sdDeclGUI::AddDefine( va( "US_HIGHLIGHTED_LINK %i", US_HIGHLIGHTED_LINK ) );
	sdDeclGUI::AddDefine( va( "IS_INVALID %i", IS_INVALID ) );
	sdDeclGUI::AddDefine( va( "IS_SCROLLIN %i", IS_SCROLLIN ) );
	sdDeclGUI::AddDefine( va( "IS_FIXED %i", IS_FIXED ) );
	sdDeclGUI::AddDefine( va( "IS_SCROLLOUT %i", IS_SCROLLOUT ) );
}

/*
============
sdUINews::CalcOffsets
============
*/
void sdUINews::CalcOffsets() {
	if ( textWidth == 0 || textHeight == 0 ) {
		return;
	}

	alphaMul = 1.0f;
	scrollOffset = idVec2( 0.0f, 0.0f );

	int type = static_cast< eFadeType >( idMath::FtoiFast( fadeType ) );
	if ( type != FT_VSCROLL && type != FT_FADE ) {
		gameLocal.Warning( "sdUINews::CalcOffsets: unknown fade type '%i'", type );
		return;
	}

	if ( static_cast< eItemState >( (int)itemState.GetValue() ) == IS_SCROLLIN ) {
		const int now = GetUI()->GetCurrentTime();
		if ( scrollStartTime <= 0 ) {
			scrollStartTime = now;
		}

		float dist = cachedClientRect.w / 2.0f + textHeight / 2.0f;

		scrollTargetTime = scrollStartTime + SEC2MS( speed );
		float totalTime = static_cast< float >( scrollTargetTime - scrollStartTime );
		float percent = static_cast< float >( now - scrollStartTime ) / totalTime;	

		if ( percent >= 1.0f ) {
			percent = 1.0f;
			itemState = IS_FIXED;
			displayStartTime = GetUI()->GetCurrentTime();
		}

		alphaMul = percent;
		if ( type == FT_VSCROLL ) {
			scrollOffset.y = cachedClientRect.w - percent * dist;
		}

	} else if ( static_cast< eItemState >( (int)itemState.GetValue() ) == IS_FIXED ) {
		if ( type == FT_VSCROLL ) {
			scrollOffset.y = ( cachedClientRect.w / 2.0f ) - ( textHeight / 2.0f );
		}

		if ( currentButton != BR_TEXT && GetUI()->GetCurrentTime() - displayStartTime >= SEC2MS( displayTime ) ) {
			itemState = IS_SCROLLOUT;
			scrollStartTime = 0;
		}
	} else if ( static_cast< eItemState >( (int)itemState.GetValue() ) == IS_SCROLLOUT ) {
		const int now = GetUI()->GetCurrentTime();
		if ( scrollStartTime <= 0 ) {
			scrollStartTime = now;
		}

		float dist = ( cachedClientRect.w / 2.0f ) + ( textHeight / 2.0f );

		scrollTargetTime = scrollStartTime + SEC2MS( speed );
		float totalTime = static_cast< float >( scrollTargetTime - scrollStartTime );
		float percent = static_cast< float >( now - scrollStartTime ) / totalTime;	

		if ( percent >= 1.0f ) {
			percent = 1.0f;

			// go to next news item
			SetNewsItem( ( currentItem + 1 ) % items.Num() );
		}

		alphaMul = 1.0f - percent;
		if ( type == FT_VSCROLL ) {
			scrollOffset.y = ( cachedClientRect.w / 2.0f ) - ( textHeight / 2.0f ) - ( percent * dist );
		}
	} else {
		gameLocal.Warning( "sdUINews::CalcVScrollOffsets: Unknown itemState %i", itemState.GetValue() );
	}
}

/*
============
sdUINews::ApplyLayout
============
*/
void sdUINews::ApplyLayout() {
	sdUIWindow::ApplyLayout();

	if ( itemState == IS_INVALID ) {
		FillNewsList();
		SetNewsItem( 0 );
	}

	if ( HasNewsItem() ) {
		const wchar_t* localText = items[ currentItem ].first.c_str();

		sdBounds2D rect( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w );
		deviceContext->GetTextDimensions( localText, rect, GetDrawTextFlags(), cachedFontHandle, fontSize, textWidth, textHeight );
	} else {
		textWidth = textHeight = 0;
	}

	CalcOffsets();

	if ( pauseNews == 0.0f ) {
		if ( items[ currentItem ].second.Length() > 0 ) {
			if ( currentButton == BR_TEXT && itemState == IS_FIXED  ) {
				linkState = US_HIGHLIGHTED_LINK;
				// pause scrolling
				displayStartTime += gameLocal.msec;
			} else {
				linkState = US_LINK;
			}
		} else {
			linkState = US_NONE;
		}
	} else {
		// pause scrolling
		displayStartTime += gameLocal.msec;
	}
}

/*
============
sdUINews::DrawLocal
============
*/
void sdUINews::DrawLocal() {
	if( !PreDraw() ) {
		return;
	}

	DrawBackground( cachedClientRect );

	DrawButtons();

	if ( HasNewsItem() ) {
		idWStr& newsItem = items[ currentItem ].first;
		sdBounds2D rect( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w );
		rect.TranslateSelf( scrollOffset );

		idVec4 color;
		color = colorLink;
		color.w *= alphaMul;

		sdUIWindow::DrawText( newsItem.c_str(), color, fontSize, rect, cachedFontHandle, GetDrawTextFlags() | DTF_NO_SNAP_TO_PIXELS );

	}

	DrawText();

	// border
	if ( borderWidth > 0.0f ) {
		deviceContext->DrawClippedBox( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w, borderWidth, borderColor );
	}

	PostDraw();
}

/*
============
sdUINews::CacheEvents
============
*/
void sdUINews::CacheEvents() {
	sdUIWindow::CacheEvents();
	drawButtonHandle = events.GetEvent( sdUIEventInfo( NE_DRAW_BUTTON, 0 ) );
}

/*
============
sdUINews::SetNewsItem
============
*/
void sdUINews::SetNewsItem( int index ) {
	assert( index >= 0 && index < items.Num() );
	itemState = IS_SCROLLIN;
	currentItem = index;
	scrollStartTime = scrollTargetTime = 0;

	newsText.SetReadOnly( false );
	newsText =  items[ currentItem ].first.c_str();
	newsText.SetReadOnly( true );

	url.SetReadOnly( false );
	url = va( L"%hs", items[ currentItem ].second.c_str() );
	url.SetReadOnly( true );
}

/*
============
sdUINews::MakeDefaultMotD
============
*/
void sdUINews::MakeDefaultMotD() {
	items.AddUnique( motdItem_t( idWStr( L"Welcome to Enemy Territory: QUAKE Wars!" ), idStr( "http://www.enemyterritory.com" ) ) );
	items.AddUnique( motdItem_t( idWStr( L"Game news and updates will be shown here!" ), idStr( "" ) ) );
	items.AddUnique( motdItem_t( idWStr( L"Click here to go to the community site." ), idStr( "http://community.enemyterritory.com" ) ) );
}

/*
============
sdUINews::FillNewsList
============
*/
void sdUINews::FillNewsList() {
	items.Clear();

	// fill items with motd list
	const sdNetService::motdList_t& motd = networkService->GetMotD();
	sdNetService::motdList_t::ConstIterator it = motd.Begin();
	while ( it != motd.End() ) {
		motdItem_t& item = items.Alloc();
		item.first = it->text;
		item.second = it->url;
		it++;
	}

	if ( !HasNewsItem() ) {
		MakeDefaultMotD();
	}
}

/*
============
sdUINews::DrawButtons
============
*/
void sdUINews::DrawButtons() {
	if( drawButtonHandle.IsValid() == false ) {
		return;
	}

	GetUI()->PushScriptVar( BR_PREV );
	if( RunEventHandle( drawButtonHandle ) ){
		GetUI()->PopScriptVar( buttonRect[ BR_PREV ] );
	}

	GetUI()->PushScriptVar( BR_NEXT );
	if( RunEventHandle( drawButtonHandle ) ){
		GetUI()->PopScriptVar( buttonRect[ BR_NEXT ] );
	}
}

/*
================
sdUINews::EnumerateEvents
================
*/
void sdUINews::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	
	if( !idStr::Icmp( name, "onDrawButton" ) ) {
		events.Append( sdUIEventInfo( NE_DRAW_BUTTON, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onMouseEnterPrev" ) ) {
		events.Append( sdUIEventInfo( NE_MOUSE_ENTER_PREV, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onMouseExitPrev" ) ) {
		events.Append( sdUIEventInfo( NE_MOUSE_EXIT_PREV, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onMouseEnterNext" ) ) {
		events.Append( sdUIEventInfo( NE_MOUSE_ENTER_NEXT, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onMouseExitNext" ) ) {
		events.Append( sdUIEventInfo( NE_MOUSE_EXIT_NEXT, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onMouseButtonPrevDown" ) ) {
		events.Append( sdUIEventInfo( NE_MOUSE_BUTTON_DOWN_PREV, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onMouseButtonPrevUp" ) ) {
		events.Append( sdUIEventInfo( NE_MOUSE_BUTTON_UP_PREV, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onMouseButtonNextDown" ) ) {
		events.Append( sdUIEventInfo( NE_MOUSE_BUTTON_DOWN_NEXT, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onMouseButtonNextUp" ) ) {
		events.Append( sdUIEventInfo( NE_MOUSE_BUTTON_UP_NEXT, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onMouseDownText" ) ) {
		events.Append( sdUIEventInfo( NE_MOUSE_DOWN_TEXT, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onMouseUpText" ) ) {
		events.Append( sdUIEventInfo( NE_MOUSE_UP_TEXT, 0 ));
		return;
	}

	sdUIWindow::EnumerateEvents( name, flags, events, tokenCache );
}

/*
============
sdUINews::PostEvent
============
*/
bool sdUINews::PostEvent( const sdSysEvent* event ) {
	if( windowState.fullyClipped || !IsVisible() || !ParentsAllowEventPosting() ) {
		return false;
	}

	idVec2 point( ui->cursorPos );

	if ( CheckButtonClick( event, point ) ) {
		return true;
	}

	bool retVal = sdUIWindow::PostEvent( event );

	if(	IsMouseClick( event ) && !cachedClippedRect.ContainsPoint( point ) ) {
		return false;
	}

	if( event->IsMouseEvent() ) {
		CheckButtonMouseOver( event, point );
	}

	return retVal;
}

/*
============
sdUINews::CheckButtonMouseOver
============
*/
bool sdUINews::CheckButtonMouseOver( const sdSysEvent* event, const idVec2& point ) {

	eButtonRect button = GetButtonForPoint( point );

	if ( button == BR_TEXT ) {
		currentButton = BR_TEXT;
		return true;
	}

	// exit button
	if( button != currentButton && currentButton != BR_NONE ) {
		if ( currentButton == BR_PREV ) {
			RunEvent( sdUIEventInfo( NE_MOUSE_EXIT_PREV, 0 ) );
		} else if ( currentButton == BR_NEXT ) {
			RunEvent( sdUIEventInfo( NE_MOUSE_EXIT_NEXT, 0 ) );
		}
		
		currentButton = BR_NONE;
		currentClickedButton = BR_NONE; // cancel any button up events
		return true;
	} 
	
	// enter button
	if( button != BR_NONE && currentButton == BR_NONE ) {
		currentButton = button;

		if ( currentButton == BR_PREV ) {
			RunEvent( sdUIEventInfo( NE_MOUSE_ENTER_PREV, 0 ) );
		} else if ( currentButton == BR_NEXT ) {
			RunEvent( sdUIEventInfo( NE_MOUSE_ENTER_NEXT, 0 ) );
		}

		return true;
	}

	return false;
}

/*
============
sdUINews::CheckButtonClick
============
*/
bool sdUINews::CheckButtonClick( const sdSysEvent* event, const idVec2& point ) {
	if( !IsMouseClick( event )  ) {
		return false;
	}

	if ( pauseNews != 0.0f ) {
		return false;
	}

	bool retVal = false;

	// mouse up
	if( !event->IsKeyDown() && currentClickedButton != BR_NONE ) {
		if ( currentClickedButton == BR_PREV ) {
			RunEvent( sdUIEventInfo( NE_MOUSE_BUTTON_UP_PREV, 0 ) );
		} else if ( currentClickedButton == BR_NEXT ) {
			RunEvent( sdUIEventInfo( NE_MOUSE_BUTTON_UP_NEXT, 0 ) );
		} else if ( currentClickedButton == BR_TEXT ) {
			RunEvent( sdUIEventInfo( NE_MOUSE_UP_TEXT, 0 ) );
		}

		currentClickedButton = BR_NONE;
		retVal = true;
	} else if( event->IsKeyDown() ){
		// mouse down
		currentClickedButton = GetButtonForPoint( point );

		if ( currentClickedButton == BR_PREV ) {
			RunEvent( sdUIEventInfo( NE_MOUSE_BUTTON_DOWN_PREV, 0 ) );
		} else if ( currentClickedButton == BR_NEXT ) {
			RunEvent( sdUIEventInfo( NE_MOUSE_BUTTON_DOWN_NEXT, 0 ) );
		} else if ( currentClickedButton == BR_TEXT ) {
			RunEvent( sdUIEventInfo( NE_MOUSE_DOWN_TEXT, 0 ) );
		}

		retVal = true;
	}

	return retVal;
}

/*
============
sdUINews::GetButtonForPoint
============
*/
sdUINews::eButtonRect sdUINews::GetButtonForPoint( const idVec2& point ) const {
	if ( pauseNews != 0.0f ) {
		return currentButton;
	}

	if ( buttonRect[ BR_PREV ].ContainsPoint( point ) ) {
		return BR_PREV;
	} else if ( buttonRect[ BR_NEXT ].ContainsPoint( point ) ) {
		return BR_NEXT;
	} else {
		idVec4 rect = cachedClientRect;
		rect.x += rect.z / 2.0f - textWidth / 2.0f;
		rect.z = textWidth;
		rect.w = textHeight;
		if ( rect.ContainsPoint( point ) ) {
			return BR_TEXT;
		}
	}

	return BR_NONE;
}

/*
============
sdUINews::Script_PrevNewsItem
============
*/
void sdUINews::Script_PrevNewsItem( sdUIFunctionStack& stack ) {
	int newItem = currentItem - 1;
	if ( newItem < 0 ) {
		newItem = items.Num() - 1;
	}

	SetNewsItem( newItem );
	itemState = IS_FIXED;
	displayStartTime = GetUI()->GetCurrentTime();
}

/*
============
sdUINews::Script_NextNewsItem
============
*/
void sdUINews::Script_NextNewsItem( sdUIFunctionStack& stack ) {
	SetNewsItem( ( currentItem + 1 ) % items.Num() );
	itemState = IS_FIXED;
	displayStartTime = GetUI()->GetCurrentTime();
}

/*
============
sdUINews::Script_GotoURL
============
*/
void sdUINews::Script_GotoURL( sdUIFunctionStack& stack ) {
	bool quit;
	stack.Pop( quit );

	if ( static_cast< int >(itemState) != IS_FIXED ) {
		gameLocal.Warning( "Unexpected item state %i", static_cast< int >(itemState) );
		return;
	}

	sys->OpenURL( items[ currentItem ].second.c_str(), quit );
}