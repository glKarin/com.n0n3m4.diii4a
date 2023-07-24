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
#include "UIMarquee.h"

#include "../../sys/sys_local.h"

SD_UI_IMPLEMENT_CLASS( sdUIMarquee, sdUIWindow )

idHashMap< sdUITemplateFunction< sdUIMarquee >* >	sdUIMarquee::marqueeFunctions;

/*
============
sdUIMarquee::sdUIMarquee
============
*/
sdUIMarquee::sdUIMarquee( void ) :
	scrollStartTime( 0 ),
	scrollTargetTime( 0 ),
	textWidth( 0 ),
	textHeight( 0 ) {
	scriptState.GetProperties().RegisterProperty( "speed",			speed );
	scriptState.GetProperties().RegisterProperty( "orientation",	orientation );

	speed				= 5.0f;
	orientation			= SO_HORIZONTAL;

	SetWindowFlag( WF_CLIP_TO_RECT );
}

/*
============
sdUIMarquee::DrawText
============
*/
void sdUIMarquee::DrawText( const wchar_t* text, const idVec4& color ) {
	if ( *text != L'\0' ) {
		sdBounds2D rect( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w );
		rect.TranslateSelf( scrollOffset );

		sdUIWindow::DrawText( text, color, fontSize, rect, cachedFontHandle, GetDrawTextFlags() | DTF_NO_SNAP_TO_PIXELS );
	}
}

/*
============
sdUIMarquee::CalcOffsets
============
*/
void sdUIMarquee::CalcOffsets() {
	if ( textWidth == 0 || textHeight == 0 ) {
		return;
	}

	float textSize;
	float rectSize;
	int index;

	switch( static_cast< eScrollOrientation >( idMath::FtoiFast( orientation ) ) ) {
		case SO_HORIZONTAL:
			textSize = textWidth;
			rectSize = cachedClientRect.z;
			scrollOffset[ 1 ] = 0.0f;
			index = 0;
			break;		
		case SO_VERTICAL:
			textSize = textHeight;
			rectSize = cachedClientRect.w;
			scrollOffset[ 0 ] = 0.0f;
			index = 1;
			break;
		default:
			gameLocal.Warning( "sdUIMarquee::CalcOffsets: unknown orientation '%i'", idMath::Ftoi( orientation ) );
			scrollStartTime = 0;
			scrollTargetTime = 0;
			return;
	}

	const int now = GetUI()->GetCurrentTime();

	float ratio = textSize / rectSize;
	if ( ratio < 1.0f ) {
		ratio = 1.0f;
	}

	if ( now >= scrollTargetTime || scrollStartTime == scrollTargetTime || now < scrollStartTime ) {
		scrollStartTime = now;
	}
	scrollTargetTime = scrollStartTime + SEC2MS( speed * ratio );

	float totalTime = static_cast< float >( scrollTargetTime - scrollStartTime );
	float percent = static_cast< float >( now - scrollStartTime ) / totalTime;	

	scrollOffset[ index ] = rectSize - percent * ( rectSize + textSize );
}

/*
============
sdUIMarquee::GetFunction
============
*/
sdUIFunctionInstance* sdUIMarquee::GetFunction( const char* name ) {
	const sdUITemplateFunction< sdUIMarquee >* function = sdUIMarquee::FindFunction( name );
	if( !function ) {		
		return sdUIWindow::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUIMarquee, sdUITemplateFunctionInstance_IdentifierTimeline >( this, function );
}

/*
============
sdUIMarquee::FindFunction
============
*/
const sdUITemplateFunction< sdUIMarquee >* sdUIMarquee::FindFunction( const char* name ) {
	sdUITemplateFunction< sdUIMarquee >** ptr;
	return marqueeFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUIMarquee::InitFunctions
============
*/
SD_UI_PUSH_CLASS_TAG( sdUIMarquee )
void sdUIMarquee::InitFunctions() {
	SD_UI_FUNC_TAG( resetScroll, "Reset scroll amount." )
	SD_UI_END_FUNC_TAG
	marqueeFunctions.Set( "resetScroll",	new sdUITemplateFunction< sdUIMarquee >( 'v', "",	&sdUIMarquee::Script_ResetScroll ) );	
}
SD_UI_POP_CLASS_TAG

/*
============
sdUIMarquee::Script_ResetScroll
============
*/
void sdUIMarquee::Script_ResetScroll( sdUIFunctionStack& stack ) {
	scrollStartTime = 0;
	scrollTargetTime = 0;
}

/*
============
sdUIMarquee::ApplyLayout
============
*/
void sdUIMarquee::ApplyLayout() {
	sdUIWindow::ApplyLayout();

	if ( localizedText.GetValue() >= 0 || text.GetValue().Length() > 0 ) {
		const wchar_t* localText = localizedText.GetValue() >= 0 ? declHolder.FindLocStrByIndex( localizedText.GetValue() )->GetText() : text.GetValue().c_str();

		sdBounds2D rect( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w );
		deviceContext->GetTextDimensions( localText, rect, GetDrawTextFlags(), cachedFontHandle, fontSize, textWidth, textHeight );
	} else {
		textWidth = textHeight = 0;
	}

	//scrollStartTime = scrollTargetTime = 0;
	CalcOffsets();
}
