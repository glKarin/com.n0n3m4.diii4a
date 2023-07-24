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
#include "../../decllib/declTypeHolder.h"
#include "../../renderer/Image.h"

#include "../../sys/sys_local.h"

const char sdUITemplateFunctionInstance_IdentifierWindow[] = "sdUIWindowFunction";

using namespace sdProperties;

SD_UI_IMPLEMENT_CLASS( sdUIWindow, sdUIObject_Drawable )


/*
===============================================================================

	sdUIWindow

===============================================================================
*/

idHashMap< sdUITemplateFunction< sdUIWindow >* > sdUIWindow::windowFunctions;

SD_UI_PUSH_CLASS_TAG( sdUIWindow )
const char* sdUIWindow::eventNames[ WE_NUM_EVENTS - OE_NUM_EVENTS ] = {
	"<noop>",
	SD_UI_EVENT_PARM_TAG( "onPreDraw",		"",					"Called before the window is about to draw. The event is often used for doing custom text and material drawing instead of the default drawing." ),
		SD_UI_EVENT_RETURN_PARM( float,							"True if gamecode should continue drawing." )
	SD_UI_END_EVENT_TAG

	SD_UI_EVENT_TAG( "onPostDraw",			"", "Called after gamecode has finished drawing." ),
	SD_UI_EVENT_TAG( "onPostChildDraw",		"", "Called after all descendants have finished drawing." ),
	SD_UI_EVENT_TAG( "onMouseMove",			"",					"Called when the mouse moves." ),
	SD_UI_EVENT_TAG( "onMouseEnter",		"",					"Called when the mouse enters the window." ),
	SD_UI_EVENT_TAG( "onMouseExit",			"",					"Called when the mouse exits the window." ),
	SD_UI_EVENT_TAG( "onKeyUp",				"[Key ...]",		"Called when a key is depressed." ),
	SD_UI_EVENT_TAG( "onKeyDown",			"[Key ...]",		"Called when a key is pressed." ),
	SD_UI_EVENT_TAG( "onKeyUpBind",			"[Bind ...]",		"Called when keys using the bind is depressed." ),
	SD_UI_EVENT_TAG( "onKeyDownBind",		"[Bind ...]",		"Called when keys usng the bind is pressed" ),
	SD_UI_EVENT_TAG( "onShow",				"",					"When the visible property is changed to true." ),
	SD_UI_EVENT_TAG( "onHide",				"",					"When The hide property is changed to false." ),
	SD_UI_EVENT_TAG( "onGainFocus",			"",					"The window has gained focus." ),
	SD_UI_EVENT_TAG( "onLoseFocus",			"",					"The window has lost focus" ),
	SD_UI_EVENT_TAG( "onDoubleClick",		"[Mouse button ...]", "Mouse is double clicked within the window's rectangle." ),
	SD_UI_EVENT_TAG( "onActivate",			"",					"This happens when the GUI is activated." ),
	SD_UI_EVENT_TAG( "onQueryToolTip",		"",					"Used to set a tooltip which will be shown when the mouse os hovering over the window." ),
	SD_UI_EVENT_TAG( "onNavForward",		"",					"Called when any key bound to the _menuNavForward is pressed" ),
	SD_UI_EVENT_TAG( "onNavBackward",		"",					"Called when any key bound to the _menuNavBackward is pressed" ),
	SD_UI_EVENT_TAG( "onAccept",			"",					"Called when any key bound to the _menuAccept is pressed" ),
	SD_UI_EVENT_TAG( "onCancel",			"",					"Called when any key bound to the _menuCancel is pressed" ),
};
SD_UI_POP_CLASS_TAG

/*
================
sdUIWindow::sdUIWindow
================
*/
sdUIWindow::sdUIWindow( void ) {
	windowState.recalculateLayout		= true;
	windowState.lookupFont				= true;
	windowState.mouseFocused			= false;
	windowState.fullyClipped			= false;

	scriptState.				GetProperties().RegisterProperty( "rect",					clientRect );
	scriptState.				GetProperties().RegisterProperty( "backColor",				backColor );
	scriptState.				GetProperties().RegisterProperty( "foreColor",				foreColor );
	scriptState.				GetProperties().RegisterProperty( "borderColor",			borderColor );
	scriptState.				GetProperties().RegisterProperty( "colorMultiplier",		colorMultiplier );
	scriptState.				GetProperties().RegisterProperty( "text",					text );
	scriptState.				GetProperties().RegisterProperty( "localizedText",			localizedText );
	scriptState.				GetProperties().RegisterProperty( "textOffset",				textOffset );
	scriptState.				GetProperties().RegisterProperty( "fontSize",				fontSize );
	scriptState.				GetProperties().RegisterProperty( "visible",				visible );
	scriptState.				GetProperties().RegisterProperty( "textAlignment",			textAlignment );
	scriptState.				GetProperties().RegisterProperty( "material",				materialName );
	scriptState.				GetProperties().RegisterProperty( "borderWidth",			borderWidth );
	
	scriptState.				GetProperties().RegisterProperty( "font",					fontName );	
	scriptState.				GetProperties().RegisterProperty( "materialScale",			materialScale );
	scriptState.				GetProperties().RegisterProperty( "allowEvents",			allowEvents );
	scriptState.				GetProperties().RegisterProperty( "allowChildEvents",		allowChildEvents );	
	scriptState.				GetProperties().RegisterProperty( "materialShift",			materialShift );

	scriptState.				GetProperties().RegisterProperty( "rotation",				rotation );
	scriptState.				GetProperties().RegisterProperty( "toolTipText",			toolTipText );

	scriptState.				GetProperties().RegisterProperty( "absoluteRect",			absoluteRect );	
	
	// make sure this is looked up when we first set it up
	UI_ADD_STR_CALLBACK( materialName,		sdUIWindow, OnMaterialChanged )	
	UI_ADD_STR_CALLBACK( fontName,			sdUIWindow, OnFontChanged )

	backColor					= idVec4( 0.0f, 0.0f, 0.0f, 0.0f );	// transparent background	
	clientRect					= idVec4( 0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT );
	cachedClientRect			= clientRect;
	absoluteRect				= cachedClientRect;
	foreColor					= colorWhite;	
	colorMultiplier				= colorWhite;

	flags						= 0.0f;
	
	visible						= 1.0f;	

	localizedText				= -1;
	textAlignment				= idVec2( TA_CENTER, TA_VCENTER );
	fontSize					= 48.0f;
	textOffset					= vec2_zero;

	borderWidth					= 0.0f;
	borderColor					= colorWhite;	

	fontName					= "dinpro";

	materialShift				= vec2_zero;

#ifdef _DEBUG
	windowState.breakOnDraw		= false;
	windowState.breakOnLayout	= false;
#endif
	
	materialScale				= vec2_one;

	allowEvents					= 1.0f;
	allowChildEvents			= 1.0f;	

	cachedFontHandle			= -1;

	renderCallback[ UIRC_PRE ]	= NULL;
	renderCallback[ UIRC_POST ]	= NULL;
	inputHandler				= NULL;

	lastClickTime				= -1;

	rotation					= 0.0f;

	absoluteRect.SetReadOnly( true );

	// hook these up last to avoid inappropriate callbacks while we're not completely setup
	UI_ADD_FLOAT_CALLBACK( visible,			sdUIWindow, OnVisibleChanged );
	UI_ADD_FLOAT_CALLBACK( flags,			sdUIWindow, OnFlagsChanged )	
	UI_ADD_VEC2_CALLBACK( textAlignment,	sdUIWindow, OnAlignmentChanged )
	UI_ADD_VEC2_CALLBACK( textOffset,		sdUIWindow, OnTextOffsetChanged )
	UI_ADD_FLOAT_CALLBACK( fontSize,		sdUIWindow, OnFontSizeChanged )	
	UI_ADD_WSTR_CALLBACK( text,				sdUIWindow, OnTextChanged )
	UI_ADD_INT_CALLBACK( localizedText,		sdUIWindow, OnLocalizedTextChanged )
	UI_ADD_VEC4_CALLBACK( clientRect,		sdUIWindow, OnClientRectChanged )

}

/*
================
sdUIWindow::~sdUIWindow
================
*/
sdUIWindow::~sdUIWindow( void ) {
	DisconnectGlobalCallbacks();

	if ( cachedFontHandle != -1 ) {
		deviceContext->FreeFont( cachedFontHandle );
		cachedFontHandle = -1;
	}
}

/*
============
sdUIWindow::CreateProperties
============
*/
void sdUIWindow::CreateProperties( sdUserInterfaceLocal* _ui, const sdDeclGUIWindow* windowParms, const idTokenCache& tokenCache ) {
	sdUIObject::CreateProperties( _ui, windowParms, tokenCache );

#ifdef _DEBUG
	windowState.breakOnDraw = windowParms->GetBreakOnDraw();
	windowState.breakOnLayout = windowParms->GetBreakOnLayout();
#endif
}

/*
================
sdUIWindow::Draw
================
*/
void sdUIWindow::Draw() {
#ifdef _DEBUG
	if ( windowState.breakOnDraw ) {
		assert( !"BREAK_ON_DRAW" );
	}
#endif // _DEBUG

	ActivateFont();

	if ( !visible ) {
		return;
	}

	bool clipToRect = TestFlag( WF_CLIP_TO_RECT );

	if( clipToRect ) {
		deviceContext->PushClipRect( sdBounds2D( cachedClientRect ) );
	}

	if( TestFlag( WF_INHERIT_PARENT_COLORS ) ) {
		const idVec4& parent = GetUI()->TopColor();
		idVec4 c(	colorMultiplier.GetValue().x * parent.x, 
					colorMultiplier.GetValue().y * parent.y,
					colorMultiplier.GetValue().z * parent.z,
					colorMultiplier.GetValue().w * parent.w );
		GetUI()->PushColor( c );
	} else {
		GetUI()->PushColor( colorMultiplier );
	}
	

	// pre-render callback
	if( renderCallback[ UIRC_PRE ] ) {
		renderCallback[ UIRC_PRE ]( GetUI(), cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w );
	}
	
	DrawLocal();

	sdUIObject* child = GetNode().GetChild();
	while( child != NULL ) {
		if( sdUIObject_Drawable* window = child->Cast< sdUIObject_Drawable >() ) {
			window->Draw();
		}
		child = child->GetNode().GetSibling();
	}

	if( postDrawChildHandle.IsValid() ) {
		RunEventHandle( postDrawChildHandle );
	}	

	// post-render callback
	if( renderCallback[ UIRC_POST ] ) {
		renderCallback[ UIRC_POST ]( GetUI(), cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w );
	}	

	GetUI()->PopColor();

	if( sdUserInterfaceLocal::g_debugGUI.GetInteger() > 0 ) {
		if( sdUserInterfaceLocal::g_debugGUI.GetInteger() != 3 || ( sdUserInterfaceLocal::g_debugGUI.GetInteger() == 3 && IsVisible() ) ) {
			deviceContext->DrawClippedBox( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w, 1, colorWhite );
			if( sdUserInterfaceLocal::g_debugGUI.GetInteger() > 1 ) {
				sdBounds2D bounds( GetWorldRect() );
				deviceContext->SetFontSize( 12 );
				deviceContext->SetColor( 1.0f, 1.0f, 0.0f, 1.0f );
				deviceContext->DrawText( va( L"%hs", name.GetValue().c_str() ), bounds, DTF_SINGLELINE | DTF_CENTER | DTF_VCENTER );
			}
		}
	}

	if ( clipToRect ) {
		deviceContext->PopClipRect();
	}
}

/*
================
sdUIWindow::FinalDraw
================
*/
void sdUIWindow::FinalDraw() {
	sdUIObject* child = GetNode().GetChild();
	while ( child != NULL ) {
		if ( sdUIObject_Drawable* window = child->Cast< sdUIObject_Drawable >() ) {
			window->FinalDraw();
		}
		child = child->GetNode().GetSibling();
	}
}

/*
============
sdUIWindow::GetDrawTextFlags
============
*/
unsigned int sdUIWindow::GetDrawTextFlags() const {
	unsigned int flags = 0;
	
	if ( TestFlag( WF_WRAP_TEXT ) ) {
		flags |= DTF_WORDWRAP;
	} else if ( !TestFlag( WF_MULTILINE_TEXT ) ) {
		flags |= DTF_SINGLELINE;
	}

	if ( TestFlag( WF_COLOR_ESCAPES ) ) {
		flags |= DTF_INCLUDECOLORESCAPES;
	}

	if ( TestFlag( WF_TRUNCATE_TEXT ) ) {
		flags |= DTF_TRUNCATE;
	}

	if ( TestFlag( WF_DROPSHADOW ) ) {
		flags |= DTF_DROPSHADOW;
	}

	switch( idMath::Ftoi( textAlignment.GetValue().x ) ) {
		case TA_LEFT:		flags |= DTF_LEFT;		break;
		case TA_CENTER:		flags |= DTF_CENTER;	break;
		case TA_RIGHT:		flags |= DTF_RIGHT;		break;
		default:			assert( false );		break;
	}
	switch( idMath::Ftoi( textAlignment.GetValue().y ) ) {
		case TA_TOP:		flags |= DTF_TOP;		break;
		case TA_VCENTER:	flags |= DTF_VCENTER;	break;
		case TA_BOTTOM:		flags |= DTF_BOTTOM;	break;
		default:			assert( false );		break;
	}

	return flags;
}

/*
============
sdUIWindow::DrawText
============
*/
void sdUIWindow::DrawText( const wchar_t* text, const idVec4& color ) {
	if ( *text != L'\0' ) {
		sdBounds2D rect( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w );
		rect.TranslateSelf( textOffset );

		DrawText( text, color, fontSize, rect, cachedFontHandle, GetDrawTextFlags() );
	}
}

/*
============
sdUIWindow::DrawText
============
*/
void sdUIWindow::DrawText( const wchar_t* text, const idVec4& color, const int pointSize, const sdBounds2D& rect, qhandle_t font, const unsigned int flags ) {
	if( idMath::Fabs( color.w ) < idMath::FLT_EPSILON ) {
		return;
	}
	deviceContext->SetFontSize( pointSize );

	sdWStringBuilder_Heap  builder;
	const wchar_t* drawText = text;
	if( ( flags & DTF_TRUNCATE ) && ( flags & DTF_SINGLELINE ) ) {		

		const wchar_t* truncationText = L"...";
		int truncationWidth;
		int truncationHeight;
		deviceContext->GetTextDimensions( truncationText, sdBounds2D( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT ), DTF_SINGLELINE | DTF_VCENTER | DTF_CENTER, font, pointSize, truncationWidth, truncationHeight );

		ShortenText( text, sdBounds2D( rect ), font, flags, pointSize, truncationText, truncationWidth, builder );
		drawText = builder.c_str();
	}

	deviceContext->SetColor( color );
	deviceContext->DrawText( drawText, rect, flags );
}

/*
============
sdUIWindow::DrawText
============
*/
void sdUIWindow::DrawText() {
	if ( localizedText.GetValue() >= 0 ) {
		DrawText( declHolder.FindLocStrByIndex( localizedText.GetValue() )->GetText(), foreColor );
	} else if ( !text.GetValue().IsEmpty() ) {
		DrawText( text.GetValue().c_str(), foreColor );
	}
}

/*
============
sdUIWindow::DrawLocal
============
*/
void sdUIWindow::DrawLocal() {
	if( PreDraw() ) {
		DrawBackground( cachedClientRect );

		DrawText();
		
		// border
		if ( borderWidth > 0.0f ) {
			deviceContext->DrawClippedBox( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w, borderWidth, borderColor );
		}
	}

	PostDraw();
}

/*
============
sdUIWindow::DrawBackground
============
*/
void sdUIWindow::DrawBackground( const idVec4& rect ) {	
	if( backColor.GetValue().w <= idMath::FLT_EPSILON && borderColor.GetValue().w <= idMath::FLT_EPSILON ) {
		return;
	}

	int handle;
	uiMaterialCache_t::Iterator iter = GetUI()->FindCachedMaterial( va( "%p_mat", this ), handle );
	if( !GetUI()->IsValidMaterial( iter ) ) {
		iter = GetUI()->SetCachedMaterial( va( "%p_mat", this ), "::guis/assets/white", handle );
	}
	
	uiCachedMaterial_t& cached = *iter->second;
	if ( cached.drawMode != BDM_USE_ST ) {
		deviceContext->DrawMaterial( rect, cached.material.material, backColor, materialScale, materialShift, rotation );
	}
	
	switch( cached.drawMode ) {
		case BDM_USE_ST:
			DrawMaterial( cached.material, rect.x, rect.y, rect.z, rect.w, backColor );
			break;
		case BDM_SINGLE_MATERIAL:			
			break;
		case BDM_FRAME:
		case BDM_TRI_PART_H:	// FALL THROUGH
		case BDM_TRI_PART_V:	// FALL THROUGH
		case BDM_FIVE_PART_H:	// FALL THROUGH
			DrawFrame( rect, iter, borderColor );
			break;
	}
}

/*
============
sdUIWindow::DrawThreeHorizontalParts
left and right are kept contant, center is stretched as necessary
============
*/
void sdUIWindow::DrawThreeHorizontalParts( const idVec4& rect, const idVec4& color, const idVec2& scale, const uiDrawPart_t& left, const uiDrawPart_t& center, const uiDrawPart_t& right ) {
	float centerWidth = Max( 0.0f, rect.z - ( left.width + right.width ) * scale.x );
	idVec2 offset( vec2_zero );

	DrawMaterial( left.mi, rect.x, rect.y, left.width * scale.x, rect.w * scale.y, color );
	DrawMaterial( center.mi, rect.x + left.width * scale.x, rect.y, centerWidth, rect.w * scale.y, color );
	DrawMaterial( right.mi, rect.x + left.width * scale.x + centerWidth, rect.y, right.width * scale.x, rect.w * scale.y, color );
}

/*
============
sdUIWindow::DrawFiveHorizontalParts
left, center, right are kept constant, stretchLeft and stretchRight are equally stretched between left/right and center
============
*/
void sdUIWindow::DrawFiveHorizontalParts( const idVec4& rect, const idVec4& color, const idVec2& scale, const uiDrawPart_t& left, const uiDrawPart_t& stretchLeft, const uiDrawPart_t& center, const uiDrawPart_t& stretchRight, const uiDrawPart_t& right ) {
	float stretchWidth = Max( 0.0f, rect.z - ( left.width + center.width + right.width ) * scale.x );
	stretchWidth *= 0.5f;

	float height = rect.w * scale.y;

	float leftStretchStart = rect.x + ( left.width * scale.x );
	float centerStart = leftStretchStart + stretchWidth;
	float rightStretchStart = centerStart + center.width * scale.x;
	float rightStart = rightStretchStart + stretchWidth;

	DrawMaterial( left.mi, rect.x, rect.y, left.width * scale.x, height, color );
	DrawMaterial( stretchLeft.mi, leftStretchStart, rect.y, stretchWidth, height, color );
	DrawMaterial( center.mi, centerStart, rect.y, center.width, height, color );
	DrawMaterial( stretchRight.mi, rightStretchStart, rect.y, stretchWidth, height, color );
	DrawMaterial( right.mi, rightStart, rect.y, right.width * scale.x, height, color );
}

/*
============
sdUIWindow::DrawThreeVerticalParts
============
*/
void sdUIWindow::DrawThreeVerticalParts( const idVec4& rect, const idVec4& color, const idVec2& scale, const uiDrawPart_t& top, const uiDrawPart_t& center, const uiDrawPart_t& bottom ) {
	float centerHeight = Max( 0.0f, rect.w - ( top.height + bottom.height ) * scale.y );

	DrawMaterial( top.mi, rect.x, rect.y, top.width * scale.x, top.height * scale.y, color );
	DrawMaterial( center.mi, rect.x, rect.y + top.height * scale.y, center.width * scale.x, centerHeight, color );
	DrawMaterial( bottom.mi, rect.x, rect.y + top.height * scale.y + centerHeight, bottom.width * scale.x, bottom.height * scale.y, color );
}

/*
============
sdUIWindow::DrawHorizontalProgress
============
*/
void sdUIWindow::DrawHorizontalProgress( const idVec4& rect, const idVec4& color, const idVec2& scale, const uiDrawPart_t& left, const uiDrawPart_t& center, const uiDrawPart_t& right ) {
	float centerWidth = rect.z - ( left.width + right.width ) * scale.x;

	DrawMaterial( left.mi, rect.x, rect.y, left.width * scale.x, rect.w * scale.y, color );
	DrawMaterial( center.mi, rect.x + left.width * scale.x, rect.y, centerWidth, rect.w * scale.y, color );
	DrawMaterial( right.mi, rect.x + left.width * scale.x + centerWidth, rect.y, right.width * scale.x, rect.w * scale.y, color );		
}


/*
============
sdUIWindow::DrawVerticalProgress
============
*/
void sdUIWindow::DrawVerticalProgress( const idVec4& rect, const idVec4& color, const idVec2& scale, const uiDrawPart_t& top, const uiDrawPart_t& center, const uiDrawPart_t& bottom ) {
	float centerHeight = rect.w - ( top.height + bottom.height ) * scale.y;

	DrawMaterial( top.mi, rect.x, rect.y, top.width * scale.x, top.height * scale.y, color );
	DrawMaterial( center.mi, rect.x, rect.y + top.height * scale.y, center.width * scale.x, centerHeight, color );
	DrawMaterial( bottom.mi, rect.x, rect.y + top.height * scale.y + centerHeight, bottom.width * scale.x, bottom.height * scale.y, color );
}

/*
============
sdUIWindow::DrawFrame
============
*/
void sdUIWindow::DrawFrame( const idVec4& rect, uiMaterialCache_t::Iterator& cacheEntry, const idVec4& color ) {
	uiCachedMaterial_t& entry = *( cacheEntry->second );
	const uiDrawPartList_t& parts = entry.parts;
	if( parts.Empty() ) {
		return;
	}

	if( entry.drawMode == BDM_FIVE_PART_H ) {
		const uiDrawPart_t& left			= parts[ FP_TOPLEFT ];
		const uiDrawPart_t& leftStretch		= parts[ FP_LEFT ];
		const uiDrawPart_t& center			= parts[ FP_CENTER ];
		const uiDrawPart_t& rightStretch	= parts[ FP_RIGHT ];
		const uiDrawPart_t& right			= parts[ FP_TOPRIGHT ];		

		DrawFiveHorizontalParts( rect, color, materialScale, left, leftStretch, center, rightStretch, right );
		return;
	}

	if( entry.drawMode == BDM_TRI_PART_H ) {
		const uiDrawPart_t& left		= parts[ FP_LEFT ];
		const uiDrawPart_t& right		= parts[ FP_RIGHT ];
		const uiDrawPart_t& center		= parts[ FP_CENTER ];

		DrawThreeHorizontalParts( rect, color, materialScale, left, center, right );
		return;
	}

	if( entry.drawMode == BDM_TRI_PART_V ) {
		const uiDrawPart_t& top		= parts[ FP_TOP ];
		const uiDrawPart_t& bottom	= parts[ FP_BOTTOM ];
		const uiDrawPart_t& center	= parts[ FP_CENTER ];
		
		DrawThreeVerticalParts( rect, color, materialScale, top, center, bottom );

		return;
	}

	const idVec2& scale = materialScale;

	const uiDrawPart_t& topLeft		= parts[ FP_TOPLEFT ];
	const uiDrawPart_t& topRight	= parts[ FP_TOPRIGHT ];
	const uiDrawPart_t& top			= parts[ FP_TOP ];
	const uiDrawPart_t& right		= parts[ FP_RIGHT ];
	const uiDrawPart_t& left		= parts[ FP_LEFT ];
	const uiDrawPart_t& bottomLeft	= parts[ FP_BOTTOMLEFT ];
	const uiDrawPart_t& bottomRight	= parts[ FP_BOTTOMRIGHT ];
	const uiDrawPart_t& bottom		= parts[ FP_BOTTOM ];
	const uiDrawPart_t& center		= parts[ FP_CENTER ];	

	float topWidth		= rect.z - ( topLeft.width + topRight.width ) * scale.x;
	float bottomWidth	= rect.z - ( bottomLeft.width + bottomRight.width ) * scale.x;
	float rightHeight	= rect.w - ( topRight.height + bottomRight.height ) * scale.y;
	float leftHeight	= rect.w - ( topLeft.height + bottomLeft.height ) * scale.y;

	if( topLeft.width < idMath::FLT_EPSILON && topRight.width < idMath::FLT_EPSILON ) {
		topWidth = 0.0f;
	}

	if( bottomLeft.width < idMath::FLT_EPSILON && bottomRight.width < idMath::FLT_EPSILON ) {
		bottomWidth = 0.0f;
	}

	leftHeight = Max( 0.0f, leftHeight );
	rightHeight = Max( 0.0f, rightHeight );

	// Top left
	DrawMaterial( topLeft.mi, rect.x, rect.y, ( topLeft.width * scale.x ), ( topLeft.height * scale.y ), color );

	// Top
	if( topWidth > 0.0f ) {
		DrawMaterial( top.mi, rect.x + ( topLeft.width * scale.x ), rect.y, topWidth, ( top.height * scale.y ), color );
	}

	// Top right
	DrawMaterial( topRight.mi, rect.x + ( topLeft.width * scale.x ) + topWidth, rect.y, ( topRight.width * scale.x ), ( topRight.height * scale.y ), color );

	// Left
	if( leftHeight > 0.0f ) {
		DrawMaterial( left.mi, rect.x, rect.y + ( topLeft.height * scale.y ), ( left.width * scale.x ), leftHeight, color );
	}

	// Right
	if( rightHeight > 0.0f ) {
		DrawMaterial( right.mi, rect.x + ( topLeft.width * scale.x ) + topWidth, rect.y + ( topLeft.height * scale.y ), ( right.width * scale.x ), rightHeight, color );
	}

	// Bottom left
	DrawMaterial( bottomLeft.mi, rect.x, rect.y + ( topLeft.height * scale.y ) + leftHeight, ( bottomLeft.width * scale.x ), ( bottomLeft.height * scale.y ), color );

	// Bottom
	if( bottomWidth > 0.0f ) {
		DrawMaterial( bottom.mi, rect.x + ( bottomLeft.width * scale.x ), rect.y + ( topLeft.height * scale.y ) + leftHeight, bottomWidth, ( bottom.height * scale.y ), color );
	}

	// Bottom right
	DrawMaterial( bottomRight.mi, rect.x + ( bottomLeft.width * scale.x ) + bottomWidth, rect.y + ( topRight.height * scale.y ) + rightHeight, ( bottomRight.width * scale.x ),  bottomRight.height * scale.y, color );

	// Center
	float centerWidth = Max( topWidth, bottomWidth );
	float centerHeight = Max( leftHeight, rightHeight );

	if( centerWidth > 0.0f && centerHeight > 0.0f ) {
		DrawMaterial( center.mi, rect.x + ( topLeft.width * scale.x ), rect.y + ( topLeft.height * scale.y ), centerWidth, centerHeight, color );
	}
}

/*
============
sdUIWindow::InitProperties
============
*/
void sdUIWindow::InitProperties() {
}

/*
================
sdUIWindow::OnActivate
================
*/
void sdUIWindow::OnActivate( void ) {
	if ( RunEvent( sdUIEventInfo( WE_ACTIVATE, 0 ) ) ) {
		if( sdUserInterfaceLocal::g_debugGUIEvents.GetBool() ) {
			gameLocal.Printf( "%s: OnActivate", name.GetValue().c_str() );
		}
	}

	if( timelines.Get() != NULL ) {
		timelines->PushAllTimelines();
	}	
}


/*
============
sdUIWindow::ParseKeys
============
*/
bool sdUIWindow::ParseKeys( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache, const char* identifier, int type ) {
	if ( flags.Num() == 0 ) {
		gameLocal.Error( "sdUIWindow::ParseKeys: no keys specified for event '%s'", identifier );
		return false;
	}

	for ( int i = 0; i < flags.Num(); i++ ) {
		int keyParameter = -1;
		const idToken& name = tokenCache[ flags[ i ] ];
		int typeLocal = type;

		if ( idStr::Icmpn( "bind", name.c_str(), 4 ) == 0 ) {
			if ( typeLocal == WE_KEYUP ) {
				typeLocal = WE_KEYUP_BIND;
			} else if ( type == WE_KEYDOWN ) {
				typeLocal = WE_KEYDOWN_BIND;
			} else {
				gameLocal.Error( "sdUIWindow::ParseKeys: cannot use 'bind' on '%s'", identifier );
			}
			keyParameter = NamedEventHandleForString( name.c_str() + 4 );
		} else {
			idKey* key = keyInputManager->GetKey( name.c_str() );
			if ( key == NULL ) {
				gameLocal.Error( "sdUIWindow::ParseKeys: key '%s' for '%s'", name.c_str(), identifier );
				continue;
			}
			keyParameter = key->GetId();
		}

		events.Append( sdUIEventInfo( typeLocal, keyParameter ) );
	}
	return true;
}

/*
================
sdUIWindow::EnumerateEvents
================
*/
void sdUIWindow::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	
	if( !idStr::Icmp( name, "onKeyUp" ) ) {
		if( ParseKeys( name, flags, events, tokenCache, "onKeyUp", WE_KEYUP ) ) {
			SetWindowFlag( WF_ALLOW_FOCUS );
		}
		return;
	}

	if( !idStr::Icmp( name, "onKeyDown" ) ) {
		if( ParseKeys( name, flags, events, tokenCache, "onKeyDown", WE_KEYDOWN ) ) {
			SetWindowFlag( WF_ALLOW_FOCUS );
		}
		return;
	}

	if( !idStr::Icmp( name, "onDoubleClick" ) ) {
		if( ParseKeys( name, flags, events, tokenCache, "onDoubleClick", WE_DOUBLE_CLICK ) ) {
			SetWindowFlag( WF_ALLOW_FOCUS );
		}
		return;
	}

	if( !idStr::Icmp( name, "onNavForward" ) ) {
		events.Append( sdUIEventInfo( WE_NAV_FORWARD, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onNavBackward" ) ) {
		events.Append( sdUIEventInfo( WE_NAV_BACKWARD, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onCancel" ) ) {
		events.Append( sdUIEventInfo( WE_CANCEL, 0 ));
		SetWindowFlag( WF_ALLOW_FOCUS );	
		return;
	}

	if( !idStr::Icmp( name, "onAccept" ) ) {
		events.Append( sdUIEventInfo( WE_ACCEPT, 0 ));
		SetWindowFlag( WF_ALLOW_FOCUS );	
		return;
	}

	if( !idStr::Icmp( name, "onActivate" ) ) {
		events.Append( sdUIEventInfo( WE_ACTIVATE, 0 ) );
		return;
	}

	if( !idStr::Icmp( name, "onMouseMove" ) ) {
		events.Append( sdUIEventInfo( WE_MOUSEMOVE, 0 ) );
		return;
	}

	if( !idStr::Icmp( name, "onGainFocus" ) ) {
		events.Append( sdUIEventInfo( WE_GAINFOCUS, 0 ) );
		SetWindowFlag( WF_ALLOW_FOCUS );
		return;
	}

	if( !idStr::Icmp( name, "onLoseFocus" ) ) {
		events.Append( sdUIEventInfo( WE_LOSEFOCUS, 0 ) );
		SetWindowFlag( WF_ALLOW_FOCUS );
		return;
	}

	if( !idStr::Icmp( name, "onMouseEnter" ) ) {
		events.Append( sdUIEventInfo( WE_MOUSEENTER, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onMouseExit" ) ) {
		events.Append( sdUIEventInfo( WE_MOUSEEXIT, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onShow" ) ) {
		events.Append( sdUIEventInfo( WE_SHOW, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onHide" ) ) {
		events.Append( sdUIEventInfo( WE_HIDE, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onPreDraw" ) ) {
		events.Append( sdUIEventInfo( WE_PREDRAW, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onPostDraw" ) ) {
		events.Append( sdUIEventInfo( WE_POSTDRAW, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onPostChildDraw" ) ) {
		events.Append( sdUIEventInfo( WE_POSTCHILDDRAW, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onQueryToolTip" ) ) {
		events.Append( sdUIEventInfo( WE_QUERY_TOOLTIP, 0 ));
		return;
	}	

	sdUIObject::EnumerateEvents( name, flags, events, tokenCache );
}

/*
============
sdUIWindow::LookupFont
============
*/
void sdUIWindow::LookupFont() {
	if ( windowState.lookupFont ) {
		if ( cachedFontHandle != -1 ) {
			deviceContext->FreeFont( cachedFontHandle );
		}
		cachedFontHandle = deviceContext->FindFont( fontName.GetValue() );		
		windowState.lookupFont = false;
	}
}

/*
============
sdUIWindow::ActivateFont
============
*/
void sdUIWindow::ActivateFont( bool set ) {
	LookupFont();

	if ( set ) {
		deviceContext->SetFont( cachedFontHandle );
	}
}

/*
============
sdUIWindow::CalcWorldOffset
============
*/
idVec2 sdUIWindow::CalcWorldOffset() const {
	sdUIObject* object = GetNode().GetParent();
	idVec2 cachedOffset( vec2_zero );

	while( object != NULL ) {
		if( sdUIWindow* window = object->Cast< sdUIWindow >() ) {
			cachedOffset += window->clientRect.GetValue().ToVec2();
		} else if( sdProperty* rect = object->GetScope().GetProperty( "rect", sdProperties::PT_VEC4 ) ) {
			cachedOffset += rect->value.vec4Value->GetValue().ToVec2();
		}
		object = object->GetNode().GetParent();
	}
	return cachedOffset;
}

/*
============
sdUIWindow::BeginLayout
============
*/
void sdUIWindow::BeginLayout() {
	LookupFont();
	windowState.recalculateLayout = false;
	MakeLayoutDirty_r( this );
}

/*
============
sdUIWindow::MakeLayoutDirty
============
*/
void sdUIWindow::MakeLayoutDirty() {
#ifdef _DEBUG
	if( windowState.breakOnLayout ) {
		assert( !"BREAK_ON_LAYOUT" );
	}
#endif // _DEBUG
	windowState.recalculateLayout = true;

	MakeLayoutDirty_r( this );
}

/*
============
sdUIWindow::EndLayout
============
*/
void sdUIWindow::EndLayout() {
	absoluteRect.SetReadOnly( false );
	absoluteRect = cachedClientRect;
	absoluteRect.SetReadOnly( true );

	cachedClippedRect.FromRectangle( cachedClientRect );
	windowState.fullyClipped = ClipBounds( cachedClippedRect );
	
	// always allow exit events to play
	// enter events should only happen for visible items
	bool contained = cachedClippedRect.ContainsPoint( ui->cursorPos ) || ui->TestGUIFlag( sdUserInterfaceLocal::GUI_NON_FOCUSED_MOUSE_EVENTS );
	if( !contained && windowState.mouseFocused ) {
		windowState.mouseFocused = false;
		RunEvent( sdUIEventInfo( WE_MOUSEEXIT, 0 ) );			
	} else if( contained && !windowState.mouseFocused && allowEvents && IsVisible() ) {
		windowState.mouseFocused = true;
		RunEvent( sdUIEventInfo( WE_MOUSEENTER, 0 ) );
	}
}

/*
============
sdUIWindow::ApplyLayout
============
*/
void sdUIWindow::ApplyLayout() {	
	if( windowState.recalculateLayout ) {
		bool updateRect = false;
		BeginLayout();
		
		float maxHeight = 0.0f;

		if ( localizedText.GetValue() >= 0 || text.GetValue().Length() > 0 ) {
			idVec4 temp = clientRect;

			bool calculateWidth = TestFlag( WF_AUTO_SIZE_WIDTH );
			bool calculateHeightFast = TestFlag( WF_AUTO_SIZE_HEIGHT );
			bool calculateHeight = calculateHeightFast | TestFlag( WF_WRAP_TEXT ) | TestFlag( WF_MULTILINE_TEXT );

			if ( calculateWidth || calculateHeight ) {
				const wchar_t* localText = localizedText.GetValue() >= 0 ? declHolder.FindLocStrByIndex( localizedText.GetValue() )->GetText() : text.GetValue().c_str();

				int width = idMath::Ftoi( clientRect.GetValue().z );
				int height = idMath::Ftoi( clientRect.GetValue().w );
				sdBounds2D rect( clientRect.GetValue() );
				deviceContext->GetTextDimensions( localText, rect, GetDrawTextFlags(), cachedFontHandle, fontSize, width, height );

				if ( calculateWidth ) {
					temp.z = width;
				}

				if ( calculateHeight ) {
					temp.w = height;
				}

				updateRect = true;
			} else if ( calculateHeightFast ) {
				temp.w = deviceContext->GetFontHeight( cachedFontHandle, fontSize );
				updateRect = true;
			}

			if ( updateRect ) {
				clientRect = temp;
			}
		}

		cachedClientRect = clientRect;	
		cachedClientRect.ToVec2() += CalcWorldOffset();	
		EndLayout();
	}
	sdUIObject::ApplyLayout();
}

/*
============
sdUIWindow::OnVisibleChanged
============
*/
void sdUIWindow::OnVisibleChanged( const float oldValue, const float newValue ) {
	Show_r( newValue != 0.0f );
}

/*
============
sdUIWindow::Show_r
============
*/
void sdUIWindow::Show_r( bool show ) {
	if( show ) {
		if( timelines.Get() != NULL ) {
			timelines->ResetAllTimelines();
		}		
		RunEvent( sdUIEventInfo( WE_SHOW, 0 ) );

		sdBounds2D bounds( cachedClientRect );
		bool contained = bounds.ContainsPoint( ui->cursorPos ) || ui->TestGUIFlag( sdUserInterfaceLocal::GUI_NON_FOCUSED_MOUSE_EVENTS );
		if( contained && ParentsAllowEventPosting() && !windowState.recalculateLayout ) {
			windowState.mouseFocused = true;
			RunEvent( sdUIEventInfo( WE_MOUSEENTER, 0 ) );			
		}
	} else {		
		if ( GetUI()->IsFocused( this ) ) {
			GetUI()->SetFocus( NULL );
		}
		if( timelines.Get() != NULL ) {
			timelines->ClearAllTimelines();
		}
		RunEvent( sdUIEventInfo( WE_HIDE, 0 ) );

		if( windowState.mouseFocused && ParentsAllowEventPosting() ) {
			windowState.mouseFocused = false;
			RunEvent( sdUIEventInfo( WE_MOUSEEXIT, 0 ) );			
		}		
	}

	// broadcast to all children
	sdUIObject* child = GetNode().GetChild();
	while( child != NULL ) {
		if( sdUIWindow* window = child->Cast< sdUIWindow >() ) {
			window->Show_r( show );
		}
		child = child->GetNode().GetSibling();
	}
}

/*
============
sdUIWindow::OnAlignmentChanged
============
*/
void sdUIWindow::OnAlignmentChanged( const idVec2& oldValue, const idVec2& newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIWindow::OnTextOffsetChanged
============
*/
void sdUIWindow::OnTextOffsetChanged( const idVec2& oldValue, const idVec2& newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIWindow::OnTextChanged
============
*/
void sdUIWindow::OnTextChanged( const idWStr& oldValue, const idWStr& newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIWindow::OnLocalizedTextChanged
============
*/
void sdUIWindow::OnLocalizedTextChanged( const int& oldValue, const int& newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIWindow::OnFontSizeChanged
============
*/
void sdUIWindow::OnFontSizeChanged( const float oldValue, const float newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIWindow::OnClientRectChanged
============
*/
void sdUIWindow::OnClientRectChanged( const idVec4& oldValue, const idVec4& newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIWindow::FindFunction
============
*/
const sdUITemplateFunction< sdUIWindow >* sdUIWindow::FindFunction( const char* name ) {
	sdUITemplateFunction< sdUIWindow >** ptr;
	return windowFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUIWindow::OnMaterialChanged
============
*/
void sdUIWindow::OnMaterialChanged( const idStr& oldValue, const idStr& newValue ) {
	int handle;
	uiMaterialCache_t::Iterator entry = GetUI()->SetCachedMaterial( va( "%p_mat", this ), newValue.c_str(), handle );
	uiCachedMaterial_t& cached = *(entry->second);

	if( cached.drawMode != BDM_SINGLE_MATERIAL && cached.drawMode != BDM_USE_ST ) {
		InitPartsForBaseMaterial( newValue.c_str(), cached );
	}
}

/*
============
sdUIWindow::OnFontChanged
============
*/
void sdUIWindow::OnFontChanged( const idStr& oldValue, const idStr& newValue ) {
	windowState.lookupFont = true;
}

/*
============
sdUIWindow::HandleFocus
============
*/
bool sdUIWindow::HandleFocus( const sdSysEvent* event ) {	
	if( !IsVisible() ) {
		return false;
	}

	if( allowChildEvents ) {
		sdUIObject* prev = NULL;
		sdUIObject* child = GetNode().GetChild();
		while( child != NULL ) {
			prev = child;
			child = child->GetNode().GetSibling();
		}

		child = prev;
		while( child != NULL ) {
			if( child->HandleFocus( event ) ) {
				return true;
			}
			child = child->GetNode().GetPriorSibling();
		}
	}

	if( !ParentsAllowEventPosting() ) {
		return false;
	}

	sdUIObject* parent = GetNode().GetParent();
	while( parent != NULL ) {
		if( sdUIWindow* window = parent->Cast< sdUIWindow >() ) {
			if( !window->allowChildEvents ) {
				return false;
			}
		}
		parent = parent->GetNode().GetParent();
	}

	if( TestFlag( WF_ALLOW_FOCUS ) ) {
		sdBounds2D bounds( cachedClientRect );

		if( ClipBounds( bounds ) ) {
			return false;
		}

		if( bounds.ContainsPoint( idVec2( ui->cursorPos ))) {
			if( sdUserInterfaceLocal::g_debugGUIEvents.GetInteger()  && name.GetValue().Icmp( ui->focusedWindowName.GetValue().c_str() ) != 0 ) {
				gameLocal.Printf( "Set focus to %s\n", name.GetValue().c_str() );
			}
			ui->focusedWindowName = name;
			return true;
		}
	}

	return false;
}


/*
============
sdUIWindow::ClipBounds
============
*/
bool sdUIWindow::ClipBounds( sdBounds2D& bounds ) {
	sdUIObject* parent = GetNode().GetParent();
	while( parent != NULL ) {
		if( sdUIWindow* windowParent = parent->Cast< sdUIWindow >() ) {
			if( windowParent->TestFlag( WF_CLIP_TO_RECT ) ) {
				sdBounds2D clipped;
				sdBounds2D parentRect( windowParent->cachedClientRect );
				bounds.IntersectBounds( parentRect, clipped );
				bounds = clipped;
			}
		}
		parent = parent->GetNode().GetParent();
	}

	// completely clipped
	if( bounds.GetWidth() <= 0.0f || bounds.GetHeight() <= 0.0f ) {
		return true;
	}
	return false;
}

/*
============
sdUIWindow::PostEvent
============
*/
bool sdUIWindow::PostEvent( const sdSysEvent* event ) {	
	if ( !IsVisible() ) {
		return false;
	}

	bool parentAllowsEvents = ParentsAllowEventPosting();
	if ( parentAllowsEvents && inputHandler ) {
		if( inputHandler( this, event ) ) {
			return true;
		}
	}

	if ( allowChildEvents ) {
		sdUIObject* child = GetNode().GetChild();
		while( child != NULL ) {
			if( child->PostEvent( event ) ) {
				return true;
			}
			child = child->GetNode().GetSibling();
		}
	}

	// ensure that an ancestor hasn't disabled events
	if( !parentAllowsEvents ) {
		return false;
	}

	bool retVal = false;
	windowEvent_t eventID = WE_NOOP;

	bool contained = cachedClippedRect.ContainsPoint( ui->cursorPos ) || ui->TestGUIFlag( sdUserInterfaceLocal::GUI_NON_FOCUSED_MOUSE_EVENTS );
	bool allowKeyEvents = GetUI()->IsFocused( this ) || TestFlag( WF_CAPTURE_KEYS ) || TestFlag( WF_CAPTURE_MOUSE );

	if ( event->IsKeyEvent() || event->IsMouseButtonEvent() ) {	
		if ( allowKeyEvents ) {			
			bool isMouseClick = IsMouseClick( event );
			if ( ( isMouseClick && ( contained || TestFlag( WF_CAPTURE_MOUSE ) ) ) || ( !isMouseClick ) ) {
				eventID = event->IsKeyDown() ? WE_KEYDOWN : WE_KEYUP;

				GetUI()->ClearScriptStack();
				retVal |= HandleBoundKeyInput( event );

				if ( isMouseClick ) {
					int timeDiff = ui->GetCurrentTime() - lastClickTime;
					int doubleClickSpeed = SEC2MS( sdUserInterfaceLocal::gui_doubleClickTime.GetFloat() );

					// FIXME in 1.3: the timeDiff < 0 test shouldn't be necessary;
					// the proper fix is to recreate the scoreboard like the rest of the in-game UI
					if ( lastClickTime < 0 && !event->IsKeyDown() ) {
						lastClickTime = ui->GetCurrentTime();
					} else if ( lastClickTime >= 0 && timeDiff >= 0 && timeDiff <= doubleClickSpeed && event->IsKeyDown() ) {
						retVal |= RunEvent( sdUIEventInfo( WE_DOUBLE_CLICK, event->GetMouseButton() ) );
						eventID = WE_DOUBLE_CLICK;
						lastClickTime = -1;
					} else if ( event->IsKeyDown() || timeDiff < 0 ) {
						lastClickTime = -1;
					}
				}

				if ( !retVal ) {
					bool down;
					idKey* key = keyInputManager->GetKeyForEvent( *event, down );
					int keyId = key == NULL ? -1 : key->GetId();

					retVal |= RunEvent( sdUIEventInfo( eventID, keyId ) );
				}				
			}
		}
	} else if ( event->IsMouseEvent() ) {

		// check for mouse entering/exiting
		if( ParentsAllowEventPosting() ) {
			if ( contained ) {				
				if ( windowState.mouseFocused == false ) {
					windowState.mouseFocused = true;
					retVal |= RunEvent( sdUIEventInfo( WE_MOUSEENTER, 0 ) );
					eventID = WE_MOUSEENTER;
				}
			} else if ( windowState.mouseFocused == true ) {
				windowState.mouseFocused = false;
				retVal |= RunEvent( sdUIEventInfo( WE_MOUSEEXIT, 0 ) );
				eventID = WE_MOUSEEXIT;
			}
			if( sdUserInterfaceLocal::g_debugGUIEvents.GetInteger() < 2 ) {
				eventID = WE_NOOP;
			}
		}

		if ( ( TestFlag( WF_CAPTURE_MOUSE ) || windowState.mouseFocused == true ) && ( event->GetXCoord() != 0 || event->GetYCoord() != 0 ) ) {
			// post the move event, possibly in addition to an enter event
			retVal |= RunEvent( sdUIEventInfo( WE_MOUSEMOVE, 0 ) );
			// jrad - don't keep track of mouse movement when debugging
		}
	} else if( event->IsGuiEvent() ) {
		if( ParentsAllowEventPosting() ) {
			if( (	event->GetGuiAction() == ULI_MENU_EVENT_NAV_BACKWARD ||
					event->GetGuiAction() == ULI_MENU_EVENT_NAV_FORWARD || 
					event->GetGuiAction() == ULI_MENU_EVENT_ACCEPT ||
					event->GetGuiAction() == ULI_MENU_EVENT_CANCEL ) && allowKeyEvents ) {
				
				if( event->GetGuiAction() == ULI_MENU_EVENT_NAV_BACKWARD ) {
					eventID = WE_NAV_BACKWARD;
				} else if( event->GetGuiAction() == ULI_MENU_EVENT_NAV_FORWARD ) {
					eventID = WE_NAV_FORWARD;
				} else if( event->GetGuiAction() == ULI_MENU_EVENT_ACCEPT ) {
					eventID = WE_ACCEPT;
				} else if( event->GetGuiAction() == ULI_MENU_EVENT_CANCEL ) {
					eventID = WE_CANCEL;
				}

				if( eventID != WE_NOOP ) {
					retVal |= RunEvent( sdUIEventInfo( eventID, 0 ) );
				}
			}
		}
	}

	if ( retVal && sdUserInterfaceLocal::g_debugGUIEvents.GetInteger() && eventID != WE_NOOP ) {
		const char* eventName = GetEventNameForType( eventID );
		if ( eventName ) {
			gameLocal.Printf( "GUI '%s', window '%s', event '%s'\n", ui->GetName(), name.GetValue().c_str(), eventName );
		} else {
			gameLocal.Printf( "GUI '%s', window '%s', event %i\n", ui->GetName(), name.GetValue().c_str(), eventID );
		}
	}

	return retVal;
}

/*
============
sdUIWindow::CacheEvents
============
*/
void sdUIWindow::CacheEvents() {
	preDrawHandle = events.GetEvent( sdUIEventInfo( WE_PREDRAW, 0 ) );
	postDrawHandle = events.GetEvent( sdUIEventInfo( WE_POSTDRAW, 0 ) );
	postDrawChildHandle = events.GetEvent( sdUIEventInfo( WE_POSTCHILDDRAW, 0 ) );
}

/*
============
sdUIWindow::HandleBoundKeyInput
============
*/
bool sdUIWindow::HandleBoundKeyInput( const sdSysEvent* event ) {
	windowEvent_t eventID = event->IsKeyDown() ? WE_KEYDOWN_BIND : WE_KEYUP_BIND;
	idList< idKey* > keys;

	bool retVal = false;
	for ( int i = 0; !retVal && i < events.Num( eventID ); i++ ) {
		sdUIEventHandle handle = events.GetEvent( sdUIEventInfo( eventID, i ) );
		if ( !handle.IsValid() ) {
			continue;
		}

		int numKeys = 0;
		keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), namedEvents[ i ], numKeys, NULL );

		if( numKeys > 0 ) {
			idKey** keys = static_cast< idKey** >( _alloca( numKeys * sizeof( idKey* ) ) );
			keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), namedEvents[ i ], numKeys, keys );

			for ( int j = 0; j < numKeys; j++ ) {
				bool down;
				idKey* key = keyInputManager->GetKeyForEvent( *event, down );
				if ( key == keys[j] ) {
					retVal |= RunEvent( sdUIEventInfo( eventID, i ) );
					break;
				}			
			}
		}
	}

	return retVal;
}


/*
============
sdUIWindow::GetFunction
============
*/
sdUIFunctionInstance* sdUIWindow::GetFunction( const char* name ) {
	const sdUITemplateFunction< sdUIWindow >* function = sdUIWindow::FindFunction( name );
	if( !function ) {		
		return sdUIObject::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUIWindow, sdUITemplateFunctionInstance_IdentifierWindow >( this, function );
}


/*
============
sdUIWindow::SetRenderCallback
============
*/
void sdUIWindow::SetRenderCallback( uiRenderCallback_t callback, uiRenderCallbackType_t type ) {
	renderCallback[ type ] = callback;	
}

/*
============
sdUIWindow::SetInputHandler
============
*/
void sdUIWindow::SetInputHandler( uiInputHandler_t handler ) {
	inputHandler = handler;
}

/*
============
sdUIWindow::OnGainFocus
============
*/
void sdUIWindow::OnGainFocus( void ) {
	RunEvent( sdUIEventInfo( WE_GAINFOCUS, 0 ) );
}

/*
============
sdUIWindow::OnLoseFocus
============
*/
void sdUIWindow::OnLoseFocus( void ) {
	RunEvent( sdUIEventInfo( WE_LOSEFOCUS, 0 ) );
}

/*
============
sdUIWindow::IsMouseClick
============
*/
bool sdUIWindow::IsMouseClick( const sdSysEvent* event ) {
	if ( event->IsMouseButtonEvent() ) {
		mouseButton_t mb = event->GetMouseButton();
		if ( mb >= M_MOUSE1 && mb <= M_MOUSE12 ) {
			return true;
		}
	}
	return false;
}


/*
============
sdUIWindow::UpdateToolTip
============
*/
sdUIObject* sdUIWindow::UpdateToolTip( const idVec2& cursor ) {
	if( !IsVisible() ) {
		return NULL;
	}

	sdUIObject* prev = NULL;
	sdUIObject* child = GetNode().GetChild();
	while( child != NULL ) {
		prev = child;
		child = child->GetNode().GetSibling();
	}

	child = prev;
	while( child != NULL ) {
		if( sdUIObject* result = child->UpdateToolTip( cursor ) ) {
			return result;
		}
		child = child->GetNode().GetPriorSibling();
	}


	sdBounds2D bounds( GetWorldRect() );
	if( !bounds.ContainsPoint( cursor ) ) {
		return NULL;
	}

	RunEvent( sdUIEventInfo( WE_QUERY_TOOLTIP, 0 ) );
	if ( toolTipText.GetValue().IsEmpty() ) {
		return NULL;
	}
	return this;
}


/*
============
sdUIWindow::ClearCapture_r
============
*/
void sdUIWindow::ClearCapture_r( sdUIWindow* window ) {
	if( window == NULL ) {
		return;
	}
	if( window != this ) {
		window->ClearWindowFlag( WF_CAPTURE_MOUSE );
	}

	sdUIObject* child = window->GetNode().GetChild();
	while( child != NULL ) {
		if( sdUIWindow* window = child->Cast< sdUIWindow >() ) {
			ClearCapture_r( window );
		}
		child = child->GetNode().GetSibling();
	}
}

/*
============
sdUIWindow::OnFlagsChanged
============
*/
void sdUIWindow::OnFlagsChanged( const float oldValue, const float newValue ) {
	// if we've gained mouse capture, remove focus from all other windows in the GUI
	if ( FlagActivated( oldValue, newValue, WF_CAPTURE_MOUSE ) ) {
		if ( sdUIWindow* window = GetUI()->GetDesktop()->Cast< sdUIWindow >() ) {
			ClearCapture_r( window );
		}
	}
	if ( FlagChanged( oldValue, newValue, WF_AUTO_SIZE_WIDTH | WF_AUTO_SIZE_HEIGHT | WF_WRAP_TEXT | WF_MULTILINE_TEXT ) ) {
		MakeLayoutDirty();
	}
}

/*
============
sdUIWindow::EndLevelLoad
============
*/
void sdUIWindow::EndLevelLoad() {
	sdUIObject::EndLevelLoad();
}

/*
============
sdUIWindow::NumTabStops_r
============
*/
int sdUIWindow::NumTabStops_r( const sdUIObject* object ) const {
	if( object == NULL ) {
		return 0;
	}
	int num = 0;
	if( const sdUIWindow* window = object->Cast< sdUIWindow >() ) {
		if( window->TestFlag( WF_TAB_STOP ) && window->ParentsAllowEventPosting() && window->IsVisible() ) {
			num++;
		}
	}
	const sdUIObject* child = object->GetNode().GetChild();
	while( child != NULL ) {
		num += NumTabStops_r( child );
		child = child->GetNode().GetSibling();
	}
	return num;
}

/*
============
sdUIWindow::ListTabStops_r
============
*/
void sdUIWindow::ListTabStops_r( const sdUIObject* object, const sdUIObject** objects, int& index ) const {
	if( object == NULL ) {
		return;
	}
	if( const sdUIWindow* window = object->Cast< sdUIWindow >() ) {
		if( window->TestFlag( WF_TAB_STOP ) && window->ParentsAllowEventPosting() && window->IsVisible() ) {
			objects[ index++ ] = object;
		}
	}
	const sdUIObject* child = object->GetNode().GetChild();
	while( child != NULL ) {
		ListTabStops_r( child, objects, index );
		child = child->GetNode().GetSibling();
	}
}

/*
============
sdUIWindow::IsTabStop
============
*/
const sdUIObject* sdUIWindow::IsTabStop( const sdUIObject* object ) const {
	if( object == NULL ) {
		return NULL;
	}

	if( const sdUIWindow* window = object->Cast< sdUIWindow >() ) {
		if( window->TestFlag( WF_TAB_STOP ) && window->ParentsAllowEventPosting() && window->IsVisible() ) {
			return window;
		}
	}

	const sdUIObject* child = object->GetNode().GetChild();
	while( child != NULL ) {
		if( const sdUIObject* tabStop = IsTabStop( child ) ) {
			return tabStop;
		}
		child = child->GetNode().GetSibling();
	}
	return NULL;
}

/*
============
sdUIWindow::PreDraw
============
*/
bool sdUIWindow::PreDraw() {
	if( preDrawHandle.IsValid() == false ) {
		return true;
	}

	float continueDrawing = 1.0f;
	if( RunEventHandle( preDrawHandle ) ){
		GetUI()->PopScriptVar( continueDrawing );		
	}

	return continueDrawing != 0.0f;
}

/*
============
sdUIWindow::PostDraw
============
*/
void sdUIWindow::PostDraw() {
	if( postDrawHandle.IsValid() ) {
		RunEventHandle( postDrawHandle );
	}
}

/*
============
sdUIWindow::ShortenText
============
*/
void sdUIWindow::ShortenText( const wchar_t* src, const sdBounds2D& rect, qhandle_t font, int drawFlags, int fontSize, const wchar_t* truncation, int truncationWidth, sdWStringBuilder_Heap& builder ) {
	if( !( drawFlags & DTF_SINGLELINE ) ) {
		builder = src;
		return;
	}
	builder.Clear();

	int len = idWStr::Length( src );
	if( len == 0 ) {
		return;
	}

	sdTextDimensionHelper tdh;
	
	tdh.Init( src, len, rect, drawFlags, font, fontSize );

	if( tdh.GetTextWidth() < rect.GetWidth() ) {
		builder = src;
		return;
	}

	float target = rect.GetWidth() - truncationWidth;
	float current = tdh.GetTextWidth();

	int i = len - 1;
	while( i >= 0 && current >= target ) {
		int advance = tdh.GetAdvance( i );
		current -= tdh.ToVirtualScreenSizeFloat( advance );
		i--;
	}
	if( i > 0 ) {
		builder.Append( src, i );
	}
	
	builder += truncation;
}

/*
============
sdUIWindow::InitPartsForBaseMaterial
============
*/
void sdUIWindow::InitPartsForBaseMaterial( const char* material, uiCachedMaterial_t& cached ) {
}

/*
============
sdUIWindow::IsVisible
============
*/
bool sdUIWindow::IsVisible() const {
	bool isVisible = visible != 0.0f;

	sdUIObject* parent = GetNode().GetParent();
	while( isVisible && parent != NULL ) {
		if( sdUIWindow* windowParent = parent->Cast< sdUIWindow >() ) {
			isVisible &= windowParent->visible != 0.0f;
		}
		parent = parent->GetNode().GetParent();
	}
	return isVisible;
}

/*
============
sdUIWindow::ParentsAllowEventPosting
============
*/
bool sdUIWindow::ParentsAllowEventPosting() const {
	bool allow = allowEvents != 0.0f;

	sdUIObject* parent = GetNode().GetParent();
	while( allow && parent != NULL ) {
		if( sdUIWindow* windowParent = parent->Cast< sdUIWindow >() ) {
			allow &= windowParent->allowChildEvents != 0.0f;
		}
		parent = parent->GetNode().GetParent();
	}
	return allow;
}
