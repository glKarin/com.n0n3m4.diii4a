// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UIWindow.h"
#include "UserInterfaceLocal.h"
#include "UISlider.h"
#include "UserInterfaceManager.h"

#include "../../sys/sys_local.h"
#include "../../idlib/Sort.h"

SD_UI_IMPLEMENT_CLASS( sdUISlider, sdUIWindow )

idHashMap< sdUITemplateFunction< sdUISlider >* > sdUISlider::sliderFunctions;

sdUISlider::thumbButtonEvent_t sdUISlider::thumbButtonEvents[ MAX_BUTTONS ];
sdUISlider::thumbButtonEvent_t sdUISlider::thumbButtonClickEvents[ MAX_BUTTONS ];

const char sdUITemplateFunctionInstance_IdentifierSlider[] = "sdUISliderFunction";

SD_UI_PUSH_CLASS_TAG( sdUISlider )
const char* sdUISlider::eventNames[ SE_NUM_EVENTS - WE_NUM_EVENTS ] = {
	SD_UI_EVENT_TAG( "onMouseEnterThumb",			"", "Called when the mouse enters the thumb." ),
	SD_UI_EVENT_TAG( "onMouseExitThumb",			"", "Called when the mouse exits the thumb." ),
	SD_UI_EVENT_TAG( "onMouseEnterUpArrow",			"", "Called when the mouse enters the up arrow." ),
	SD_UI_EVENT_TAG( "onMouseExitUpArrow",			"", "Called when the mouse exits the up arrow." ),
	SD_UI_EVENT_TAG( "onMouseEnterDownArrow",		"", "Called when the mouse enters the down arrow." ),
	SD_UI_EVENT_TAG( "onMouseExitDownArrow",		"", "Called when the mouse exits the down arrow." ),
	SD_UI_EVENT_TAG( "onMouseEnterGutter",			"", "Called when the mouse enters the gutter." ),
	SD_UI_EVENT_TAG( "onMouseExitGutter",			"", "Called when the mouse exits the gutter." ),	
	SD_UI_EVENT_TAG( "onMouseDownUpArrow",			"", "Called when the left mouse button is pressed on the up arrow." ),
	SD_UI_EVENT_TAG( "onMouseUpUpArrow",			"", "Called when the left mouse button is depressed on the up arrow." ),
	SD_UI_EVENT_TAG( "onMouseDownDownArrow",		"", "Called when the left mouse button is pressed on the down arrow." ),
	SD_UI_EVENT_TAG( "onMouseUpDownArrow",			"", "Called when the left mouse button is depressed on the down arrow." ),
	SD_UI_EVENT_TAG( "onMouseDownThumb",			"", "Called when the left mouse button is pressed on the thumb." ),
	SD_UI_EVENT_TAG( "onMouseUpThumb",				"", "Called when the left mouse button is depressed on the thumb." ),
	SD_UI_EVENT_TAG( "onMouseDownGutter",			"", "Called when the left mouse button is pressed in the gutter." ),
	SD_UI_EVENT_TAG( "onMouseUpGutter",				"", "Called when the left mouse button is depressed in the gutter." ),
	SD_UI_EVENT_TAG( "onBeginScroll",				"", "Called when starting to drag the thumb." ),
	SD_UI_EVENT_TAG( "onEndScroll",					"", "Called when stopped dragging the thumb." ),
};
SD_UI_POP_CLASS_TAG

const char* sdUISlider::partNames[ SBP_MAX ] = {
	"t",
	"c",
	"b",
	"fill_t",
	"fill_c",
	"fill_b",
	"thumb",
	"thumb_overlay",
	"arrow_up",
	"arrow_down",
};


/*
============
sdUISlider::sdUISlider
============
*/
sdUISlider::sdUISlider() :
	scrollDirection( 0.0f ),
	lastScrollTime( 0 ),
	pageStep( 1.0f ),
	currentScrollButton( NO_BUTTON ),
	currentClickedScrollButton( NO_BUTTON ) {

	flags.gutterHighlighted = false;
	flags.draggingThumb = false;

	scriptState.GetProperties().RegisterProperty( "range",						range );
	scriptState.GetProperties().RegisterProperty( "position",					position );
	scriptState.GetProperties().RegisterProperty( "orientation",				orientation );

	scriptState.GetProperties().RegisterProperty( "thumbColor",					thumbColor );
	scriptState.GetProperties().RegisterProperty( "upArrowColor",				upArrowColor );
	scriptState.GetProperties().RegisterProperty( "downArrowColor",				downArrowColor );
	scriptState.GetProperties().RegisterProperty( "thumbOverlayColor",			thumbOverlayColor );
	scriptState.GetProperties().RegisterProperty( "fillColor",					fillColor );
	scriptState.GetProperties().RegisterProperty( "pageStep",					pageStep );

	thumbColor			= colorWhite;
	thumbOverlayColor	= colorWhite;
	upArrowColor		= colorWhite;
	downArrowColor		= colorWhite;
	fillColor			= colorWhite;
	
	position = 0.0f;
	range = idVec2( 0.0f, 100.0f );
	orientation = SO_VERTICAL;

	SetWindowFlag( WF_ALLOW_FOCUS );
	
	scrollbarParts.SetNum( SBP_MAX );
}

/*
============
sdUISlider::InitProperties
============
*/
void sdUISlider::InitProperties() {
	sdUIWindow::InitProperties();
}

/*
============
sdUISlider::~sdUISlider
============
*/
sdUISlider::~sdUISlider() {
	DisconnectGlobalCallbacks();
}

/*
============
sdUISlider::FindFunction
============
*/
const sdUISlider::SliderTemplateFunction* sdUISlider::FindFunction( const char* name ) {
	sdUISlider::SliderTemplateFunction** ptr;
	return sliderFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUISlider::GetFunction
============
*/
sdUIFunctionInstance* sdUISlider::GetFunction( const char* name ) {
	const SliderTemplateFunction* function = sdUISlider::FindFunction( name );
	if ( !function ) {		
		return sdUIWindow::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUISlider, sdUITemplateFunctionInstance_IdentifierSlider >( this, function );
}

/*
============
sdUISlider::RunNamedMethod
============
*/
bool sdUISlider::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const sdUISlider::SliderTemplateFunction* func = sdUISlider::FindFunction( name );
	if ( !func ) {
		return sdUIWindow::RunNamedMethod( name, stack );
	}

	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}

/*
============
sdUISlider::DrawLocal
============
*/
void sdUISlider::DrawLocal() {
	if( !PreDraw() ) {
		return;
	}

	idVec4 thumbRect;
	GetScrollbarButtonRect( THUMB_BUTTON, thumbRect );

	if( scrollDirection != 0.0f && ( ui->GetCurrentTime() - lastScrollTime ) >= 5 ) {
		// stop scrolling we've reached the cursor
		float dir = SignForPoint( ui->cursorPos, thumbRect );
		if( dir == 0.0f || ( dir > 0.0f && scrollDirection < 0.0f  ) || ( dir < 0.0f && scrollDirection > 0.0f  ) ) {
			scrollDirection = 0.0f;
		} else {
			position = idMath::ClampFloat( range.GetValue().x, range.GetValue().y, position + scrollDirection );
			lastScrollTime = ui->GetCurrentTime();
		}
	}	

	// thumb
	const uiDrawPart_t& top			= scrollbarParts[ SBP_BACK_TOP ];
	const uiDrawPart_t& bottom		= scrollbarParts[ SBP_BACK_BOTTOM ];
	const uiDrawPart_t& center		= scrollbarParts[ SBP_BACK_CENTER ];

	const uiDrawPart_t& fillTop		= scrollbarParts[ SBP_FILL_TOP ];
	const uiDrawPart_t& fillBottom	= scrollbarParts[ SBP_FILL_BOTTOM ];
	const uiDrawPart_t& fillCenter	= scrollbarParts[ SBP_FILL_CENTER ];

	idVec4 rect;
	GetScrollbarButtonRect( GUTTER_AREA, rect );
	
	sdBounds2D bounds( rect );
	if( orientation == SO_HORIZONTAL ) {
		bounds.GetMaxs().x = bounds.GetMins().x + ( thumbRect.x - rect.x );
		
		deviceContext->PushClipRect( bounds );
		DrawThreeHorizontalParts( rect, fillColor, materialScale, fillTop, fillCenter, fillBottom );
		deviceContext->PopClipRect();

		DrawThreeHorizontalParts( rect, borderColor, materialScale, top, center, bottom );
	} else {
		bounds.GetMaxs().y = bounds.GetMins().y + ( thumbRect.y - rect.y );

		deviceContext->PushClipRect( bounds );
		DrawThreeVerticalParts( rect, fillColor, materialScale, fillTop, fillCenter, fillBottom );
		deviceContext->PopClipRect();

		DrawThreeVerticalParts( rect, borderColor, materialScale, top, center, bottom );
	}
	
	DrawMaterial( scrollbarParts[ SBP_THUMB ].mi, thumbRect.x, thumbRect.y, thumbRect.z, thumbRect.w, thumbColor );
	DrawMaterial( scrollbarParts[ SBP_THUMB_OVERLAY ].mi, thumbRect.x, thumbRect.y, thumbRect.z, thumbRect.w, thumbOverlayColor );

	GetScrollbarButtonRect( UP_BUTTON, rect );
	DrawMaterial( scrollbarParts[ SBP_ARROW_UP ].mi, rect.x, rect.y, rect.z, rect.w, upArrowColor );

	GetScrollbarButtonRect( DOWN_BUTTON, rect );
	DrawMaterial( scrollbarParts[ SBP_ARROW_DOWN ].mi, rect.x, rect.y, rect.z, rect.w, downArrowColor );
	
	DrawText();

	// border
	if( borderWidth > 0.0f ) {
		deviceContext->DrawClippedBox( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w, borderWidth, borderColor );
	}

	PostDraw();
}

/*
============
sdUISlider::InitFunctions
============
*/
SD_UI_PUSH_CLASS_TAG( sdUISlider )
void sdUISlider::InitFunctions() {
	thumbButtonEvents[ UP_BUTTON ]			= thumbButtonEvent_t( SE_MOUSE_ENTER_UP_ARROW, SE_MOUSE_EXIT_UP_ARROW );
	thumbButtonEvents[ DOWN_BUTTON ]		= thumbButtonEvent_t( SE_MOUSE_ENTER_DOWN_ARROW, SE_MOUSE_EXIT_DOWN_ARROW );
	thumbButtonEvents[ THUMB_BUTTON ]		= thumbButtonEvent_t( SE_MOUSE_ENTER_THUMB, SE_MOUSE_EXIT_THUMB );
	thumbButtonEvents[ GUTTER_AREA ]		= thumbButtonEvent_t( SE_MOUSE_ENTER_GUTTER, SE_MOUSE_EXIT_GUTTER );

	thumbButtonClickEvents[ UP_BUTTON ]		= thumbButtonEvent_t( SE_MOUSE_DOWN_UP_ARROW, SE_MOUSE_UP_UP_ARROW );
	thumbButtonClickEvents[ DOWN_BUTTON ]	= thumbButtonEvent_t( SE_MOUSE_DOWN_DOWN_ARROW, SE_MOUSE_UP_DOWN_ARROW );
	thumbButtonClickEvents[ THUMB_BUTTON ]	= thumbButtonEvent_t( SE_MOUSE_DOWN_THUMB, SE_MOUSE_UP_THUMB );
	thumbButtonClickEvents[ GUTTER_AREA ]	= thumbButtonEvent_t( SE_MOUSE_DOWN_GUTTER, SE_MOUSE_UP_GUTTER );

	SD_UI_ENUM_TAG( SF_INTEGER_SNAP, "Snap slider value to an integer value." )
	sdDeclGUI::AddDefine( va( "SF_INTEGER_SNAP %i", SF_INTEGER_SNAP ) );
	SD_UI_ENUM_TAG( SO_HORIZONTAL, "Horizontal slider." )
	sdDeclGUI::AddDefine( va( "SO_HORIZONTAL %i", SO_HORIZONTAL ) );
	SD_UI_ENUM_TAG( SO_VERTICAL, "Vertical slider." )
	sdDeclGUI::AddDefine( va( "SO_VERTICAL %i", SO_VERTICAL ) );
}
SD_UI_POP_CLASS_TAG

/*
============
sdUISlider::CheckScrollbarButtonMouseOver
============
*/
bool sdUISlider::CheckScrollbarButtonMouseOver( const sdSysEvent* event, const idVec2& point ) {

	eScrollButtonType button = GetButtonForPoint( point );
	if( ( button != currentScrollButton ) && currentScrollButton != NO_BUTTON ) {
		RunEvent( sdUIEventInfo( thumbButtonEvents[ currentScrollButton ].second, 0 ) );	// exit
		currentScrollButton = NO_BUTTON;
	} 
	
	if( button != NO_BUTTON && button != GUTTER_AREA && currentScrollButton == NO_BUTTON ) {
		currentScrollButton = button;
		RunEvent( sdUIEventInfo( thumbButtonEvents[ currentScrollButton ].first, 0 ) );		// enter				
	}

	bool lastGutterState = flags.gutterHighlighted;
	flags.gutterHighlighted = button == GUTTER_AREA || button == THUMB_BUTTON;

	if( lastGutterState && !flags.gutterHighlighted ) {
		RunEvent( sdUIEventInfo( thumbButtonEvents[ GUTTER_AREA ].second, 0 ) );			// exit
	} else if( !lastGutterState && flags.gutterHighlighted ) {
		RunEvent( sdUIEventInfo( thumbButtonEvents[ GUTTER_AREA ].first, 0 ) );				// end
	}

	return true;
}

/*
============
sdUISlider::CheckScrollbarButtonClick
============
*/
bool sdUISlider::CheckScrollbarButtonClick( const sdSysEvent* event, const idVec2& point ) {
	if( !IsMouseClick( event )  ) {
		return false;
	}

	bool retVal = false;

	if( !event->IsKeyDown() && currentClickedScrollButton != NO_BUTTON ) {
		RunEvent( sdUIEventInfo( thumbButtonClickEvents[ currentClickedScrollButton ].second, 0 ) );	// mouse up
		currentClickedScrollButton = NO_BUTTON;
		scrollDirection = 0.0f;
		flags.draggingThumb = false;
		retVal = true;
	} else if( event->IsKeyDown() ){
		currentClickedScrollButton = GetButtonForPoint( point );

		idVec4 thumbRect;
		GetScrollbarButtonRect( THUMB_BUTTON, thumbRect );

		switch( currentClickedScrollButton ) {
			case NO_BUTTON:
				break;
			case THUMB_BUTTON:
				RunEvent( sdUIEventInfo( SE_BEGIN_SCROLL, 0 ) );
				flags.draggingThumb = true;
			case UP_BUTTON:		// FALL THROUGH
			case DOWN_BUTTON:	// FALL THROUGH
			case GUTTER_AREA:	// FALL THROUGH
				RunEvent( sdUIEventInfo( thumbButtonClickEvents[ currentClickedScrollButton ].first, 0 ) );	// mouse down
				scrollDirection = SignForPoint( point, thumbRect );
				retVal = true;
				break;
		}
		
		if( currentClickedScrollButton == GUTTER_AREA ) {
			// scroll by a full page
			scrollDirection *= pageStep;

			// scroll once for the gutter before auto-scrolling
			float value = idMath::ClampFloat( range.GetValue().x, range.GetValue().y, position + scrollDirection );
			if( TestFlag( SF_INTEGER_SNAP ) ) {
				value = idMath::Floor( value );
			}
			position = value;
			
			// delay auto-scrolling to allow for a single click
			lastScrollTime = GetUI()->GetCurrentTime() + 500;
		}
	}

	return retVal;
}

/*
============
sdUISlider::UpdateScrollbarDrag
============
*/
bool sdUISlider::UpdateScrollbarDrag( const sdSysEvent* event, const idVec2& point ) {
	// dragging the scroll thumb
	if( event->IsMouseEvent() && flags.draggingThumb ) {		

		idVec4 rect;
		GetScrollbarButtonRect( GUTTER_AREA, rect );
		if( orientation == SO_VERTICAL ) {
 			float percent = ( point.y - rect.y ) / ( rect.w );

			ClampPosition( percent );
		} else if( orientation == SO_HORIZONTAL ) {
			float percent = ( point.x - rect.x ) / ( rect.z );

			ClampPosition( percent );
		}

		// cursor clamp
		idVec2 cursorPos = GetUI()->cursorPos;
		if( cursorPos.x < rect.x ) {
			cursorPos.x = rect.x;
		} else if( cursorPos.x > rect.x + rect.z ) {
			cursorPos.x = rect.x + rect.z;
		}

		if( cursorPos.y < rect.y ) {
			cursorPos.y = rect.y;
		} else if( cursorPos.y > rect.y + rect.w ) {
			cursorPos.y = rect.y + rect.w;
		}

		GetUI()->cursorPos = cursorPos;

		return true;
	}

	// see if we need to release the scroll
	if ( flags.draggingThumb ) {
		if ( !IsMouseClick( event ) ) {
			return false;
		}

		if ( event->IsButtonDown() ) {
			return false;
		}

		flags.draggingThumb = false;
		RunEvent( sdUIEventInfo( SE_END_SCROLL, 0 ) );

		return true;
	}
	return false;
}

/*
============
sdUISlider::PostEvent
============
*/
bool sdUISlider::PostEvent( const sdSysEvent* event ) {
	if( windowState.fullyClipped || !IsVisible() || !ParentsAllowEventPosting() ) {
		return false;
	}

	idVec2 point( ui->cursorPos );
	
	if( UpdateScrollbarDrag( event, point ) || 
		CheckScrollbarButtonClick( event, point ) ) {
		
		return true;
	}

	bool retVal = sdUIWindow::PostEvent( event );
	bool hitItem = false;	


	if(	IsMouseClick( event ) && !cachedClippedRect.ContainsPoint( point ) ) {
		return false;
	}

	if( event->IsMouseEvent() ) {
		CheckScrollbarButtonMouseOver( event, point );
	} else if ( event->IsKeyEvent()  ) {
		if ( event->IsKeyDown() ) {
			keyNum_t key = event->GetKey();

			if( GetUI()->IsFocused( this ) && event->IsKeyDown() ) {
				switch( key ) {
					case K_UPARROW:			/* FALL THROUGH */
					case K_LEFTARROW: {
						float tempPos = position - GetScrollPercent();
						tempPos = idMath::ClampFloat( range.GetValue().x, range.GetValue().y, tempPos );
						if( TestFlag( SF_INTEGER_SNAP ) ) {
							tempPos = idMath::Floor( tempPos );
						}
						position = tempPos;
						retVal |= true;
						}
						break;
					case K_DOWNARROW:		/* FALL THROUGH */
					case K_RIGHTARROW: {
						float tempPos = position + GetScrollPercent();
						tempPos = idMath::ClampFloat( range.GetValue().x, range.GetValue().y, tempPos );
						if( TestFlag( SF_INTEGER_SNAP ) ) {
							tempPos = idMath::Floor( tempPos );
						}
						position = tempPos;
						retVal |= true;
						}
						break;
					case K_HOME:
						position = range.GetValue().x;
						break;
					case K_END:
						position = range.GetValue().y;
						break;
				}
			}
			flags.draggingThumb = false;
		} else {
			flags.draggingThumb = false;
			scrollDirection = 0.0f;
		}
	}	
	return retVal;
}

/*
============
sdUISlider::EnumerateEvents
============
*/
void sdUISlider::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	if ( !idStr::Icmp( name, "onMouseEnterThumb" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_ENTER_THUMB, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseExitThumb" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_EXIT_THUMB, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseEnterUpArrow" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_ENTER_UP_ARROW, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseExitUpArrow" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_EXIT_UP_ARROW, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseEnterDownArrow" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_ENTER_DOWN_ARROW, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseExitDownArrow" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_EXIT_DOWN_ARROW, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseEnterGutter" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_ENTER_GUTTER, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseExitGutter" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_EXIT_GUTTER, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseDownDownArrow" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_DOWN_DOWN_ARROW, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseUpDownArrow" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_UP_DOWN_ARROW, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseDownUpArrow" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_DOWN_UP_ARROW, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseUpUpArrow" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_UP_UP_ARROW, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseDownThumb" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_DOWN_THUMB, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseUpThumb" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_UP_THUMB, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseDownGutter" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_DOWN_GUTTER, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onMouseUpGutter" ) ) {
		events.Append( sdUIEventInfo( SE_MOUSE_UP_GUTTER, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onBeginScroll" ) ) {
		events.Append( sdUIEventInfo( SE_BEGIN_SCROLL, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onEndScroll" ) ) {
		events.Append( sdUIEventInfo( SE_END_SCROLL, 0 ) );
		return;
	}	

	sdUIWindow::EnumerateEvents( name, flags, events, tokenCache );
}

/*
============
sdUISlider::GetScrollbarButtonRect
============
*/
void sdUISlider::GetScrollbarButtonRect( eScrollButtonType button, idVec4& rect ) const {
	rect = cachedClientRect;
	int value = idMath::FtoiFast( orientation );

	float rangeLocal = idMath::Fabs( range.GetValue().y - range.GetValue().x );
	float positionClamped = idMath::ClampFloat( range.GetValue().x, range.GetValue().y, position );
	float percent = ( rangeLocal <= idMath::FLT_EPSILON ) ? 0.0f : idMath::Fabs( ( positionClamped - range.GetValue().x ) / rangeLocal );

	idVec2 thumbSize(	Max( scrollbarParts[ SBP_THUMB ].width, scrollbarParts[ SBP_THUMB_OVERLAY ].width ) * materialScale.GetValue().x, 
						Max( scrollbarParts[ SBP_THUMB ].height, scrollbarParts[ SBP_THUMB_OVERLAY ].height ) * materialScale.GetValue().y );
	idVec2 arrowSize( scrollbarParts[ SBP_ARROW_UP ].width * materialScale.GetValue().x, scrollbarParts[ SBP_ARROW_UP ].height * materialScale.GetValue().y );

	switch( static_cast< eScrollOrientation >( value )) {
		case SO_HORIZONTAL:
			switch( button ) {
				case THUMB_BUTTON:
					rect.x += arrowSize.x;
					rect.x += ( ( rect.z - thumbSize.x - ( 2.0f * arrowSize.x ) ) * percent );
					rect.z = thumbSize.x;
					break;
				case UP_BUTTON:
					rect.z = arrowSize.x;
					break;
				case DOWN_BUTTON:
					rect.x = rect.x + rect.z - arrowSize.x;
					rect.z = arrowSize.x;
					break;
				case GUTTER_AREA:
					rect.x += arrowSize.x;
					rect.z -= 2.0f * arrowSize.x;
					break;
			}
			break;
		case SO_VERTICAL:
			switch( button ) {
				case THUMB_BUTTON:
					rect.y += arrowSize.y;
					rect.y += ( ( rect.w - thumbSize.y - ( 2.0f * arrowSize.y ) ) * percent );
					rect.w = thumbSize.y;
					break;
				case UP_BUTTON:
					rect.w = arrowSize.y;
					break;
				case DOWN_BUTTON:
					rect.y = rect.y + rect.w - arrowSize.y;
					rect.w = arrowSize.y;
					break;
				case GUTTER_AREA:
					rect.y += arrowSize.y;
					rect.w -= 2.0f * arrowSize.y;
					break;
			}
			break;
	}
}

/*
============
sdUISlider::InitPartsForBaseMaterial
============
*/
void sdUISlider::InitPartsForBaseMaterial( const char* material, uiCachedMaterial_t& cached ) {
	sdUIWindow::InitPartsForBaseMaterial( material, cached );
	idStr out;
	for( int i = 0; i < SBP_MAX; i++ ) {
		scrollbarParts[ i ].mi.material = NULL;
		scrollbarParts[ i ].width = 0;
		scrollbarParts[ i ].height = 0;
		
		bool globalLookup;
		bool literal;
		uiDrawMode_e mode;

		GetUI()->ParseMaterial( material, out, globalLookup, literal, mode );
		GetUI()->SetupPart( scrollbarParts[ i ], sdUISlider::partNames[ i ], out.c_str() );
	}
}

/*
============
sdUISlider::EndLevelLoad
============
*/
void sdUISlider::EndLevelLoad() {
	sdUIWindow::EndLevelLoad();

	sdUserInterfaceLocal::LookupPartSizes( scrollbarParts.Begin(), scrollbarParts.Num() );
}
