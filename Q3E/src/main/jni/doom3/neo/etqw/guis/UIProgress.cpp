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
#include "UIProgress.h"
#include "UserInterfaceManager.h"

#include "../../renderer/DeviceContext.h"

#include "../../sys/sys_local.h"

SD_UI_IMPLEMENT_CLASS( sdUIProgress, sdUIWindow )

/*
============
sdUIProgress::sdUIProgress
============
*/
sdUIProgress::sdUIProgress() {
	scriptState.GetProperties().RegisterProperty( "fillMaterial",				fillMaterialName );

	scriptState.GetProperties().RegisterProperty( "range",						range );
	scriptState.GetProperties().RegisterProperty( "highlightRange",				highlightRange );
	scriptState.GetProperties().RegisterProperty( "highlightColor",				highlightColor );
	scriptState.GetProperties().RegisterProperty( "highlightFillMaterialName",	highlightFillMaterialName );
	scriptState.GetProperties().RegisterProperty( "position",					position );
	scriptState.GetProperties().RegisterProperty( "orientation",				orientation );

	scriptState.GetProperties().RegisterProperty( "segments",					numSegments );

	UI_ADD_STR_CALLBACK( fillMaterialName,	sdUIProgress, OnFillMaterialChanged );
	UI_ADD_STR_CALLBACK( fillMaterialName,	sdUIProgress, OnHighlightFillMaterialChanged );

	orientation			= SO_HORIZONTAL;
	numSegments			= 1.0f;
	range				= idVec2( 0.0f, 1.0f );
	position			= 0.0f;
	highlightRange		= vec2_zero;
	highlightColor		= colorWhite;

	sliderParts.SetNum( SP_MAX );
}

/*
============
sdUIProgress::InitProperties
============
*/
void sdUIProgress::InitProperties() {
	sdUIWindow::InitProperties();
}

/*
============
sdUIProgress::~sdUIProgress
============
*/
sdUIProgress::~sdUIProgress() {
	DisconnectGlobalCallbacks();
}


/*
============
sdUIProgress::InitFunctions
============
*/
SD_UI_PUSH_CLASS_TAG( sdUIProgress )
void sdUIProgress::InitFunctions( void ) {
	SD_UI_ENUM_TAG( PF_DRAW_FROM_LOWER_END, "Inverse progress drawing." )
	sdDeclGUI::AddDefine( va( "PF_DRAW_FROM_LOWER_END %i", PF_DRAW_FROM_LOWER_END ) );
}
SD_UI_POP_CLASS_TAG

/*
============
sdUIProgress::ShutdownFunctions
============
*/
void sdUIProgress::ShutdownFunctions( void ) {

}

/*
============
sdUIProgress::DrawSegment
============
*/
void sdUIProgress::DrawSegment( const idVec4& color, eSliderPart begin, eSliderPart center, eSliderPart end, int xDim, int yDim, int offset, float totalDim ) {
	sdBounds2D rect;

	if( TestFlag( PF_DRAW_FROM_LOWER_END ) ) {
		rect.GetMins()[ xDim ] = cachedClientRect[ xDim ] + ( numSegments - offset - 1 ) * totalDim;
		rect.GetMins()[ yDim ] = cachedClientRect[ yDim ];
		rect.GetMaxs()[ xDim ] = rect.GetMins()[ xDim ] + totalDim;
		rect.GetMaxs()[ yDim ] = rect.GetMins()[ yDim ] + cachedClientRect[ yDim + 2 ];
	} else {
		rect.GetMins()[ xDim ] = cachedClientRect[ xDim ] + offset * totalDim;
		rect.GetMins()[ yDim ] = cachedClientRect[ yDim ];
		rect.GetMaxs()[ xDim ] = rect.GetMins()[ xDim ] + totalDim;
		rect.GetMaxs()[ yDim ] = rect.GetMins()[ yDim ] + cachedClientRect[ yDim + 2 ];
	}

	idVec4 drawRect( rect.GetMins().x, rect.GetMins().y, rect.GetWidth(), rect.GetHeight() );
	if( orientation == SO_HORIZONTAL ) {
		DrawHorizontalProgress( drawRect, color, vec2_one, sliderParts[ SP_BEGIN ], sliderParts[ SP_CENTER ], sliderParts[ SP_END ] );
	} else {
		DrawVerticalProgress( drawRect, color, vec2_one, sliderParts[ SP_BEGIN ], sliderParts[ SP_CENTER ], sliderParts[ SP_END ] );
	}
	
}

/*
============
sdUIProgress::DrawLocal
============
*/
void sdUIProgress::DrawLocal() {	
	if( !PreDraw() ) {
		return;
	}
	sdBounds2D rect( cachedClientRect );

	// background
	DrawBackground( cachedClientRect );

	int xDim = 0;
	int yDim = 1;
	float totalDim = 0.0f;	

	switch( idMath::Ftoi( orientation ) ) {
		case SO_HORIZONTAL:
			xDim = 0;
			yDim = 1;
			totalDim = rect.GetWidth() / numSegments;
			break;
		case SO_VERTICAL:
			xDim = 1;
			yDim = 0;
			totalDim = rect.GetHeight() / numSegments;
			break;
		default:
			gameLocal.Warning( "sdUIProgress::DrawLocal: '%s' unknown orientation '%i'", name.GetValue().c_str(), idMath::Ftoi( orientation ));
			totalDim = rect.GetWidth() / numSegments;
			break;
	}
	

	sdBounds2D clipRect( cachedClientRect );
	float percent = GetPercent();

	if( TestFlag( PF_DRAW_FROM_LOWER_END ) ) {
		clipRect.GetMins()[ xDim ] = clipRect.GetMaxs()[ xDim ] - cachedClientRect[ xDim + 2 ] * percent;
	} else {
		clipRect.GetMaxs()[ xDim ] = clipRect.GetMins()[ xDim ] + cachedClientRect[ xDim + 2 ] * percent;
	}
	
	{
		deviceContext->PushClipRect( clipRect );
		// draw normal segments
		float segmentCount = numSegments;
		int i = 0;
		while ( segmentCount > 0.f ) {
			DrawSegment( foreColor, SP_BEGIN, SP_CENTER, SP_END, xDim, yDim, i, totalDim );
			i++;
			segmentCount--;
		}
		deviceContext->PopClipRect();

		// now draw highlighted region
		if( idMath::Fabs( highlightRange.GetValue().x - highlightRange.GetValue().y ) > idMath::FLT_EPSILON && highlightColor.GetValue().w > idMath::FLT_EPSILON ) {
			float lowerPercent = ( highlightRange.GetValue().x - range.GetValue().x ) / ( range.GetValue().y - range.GetValue().x );
			float upperPercent = ( highlightRange.GetValue().y - range.GetValue().x ) / ( range.GetValue().y - range.GetValue().x );

			clipRect.FromRectangle( cachedClientRect );
			if( TestFlag( PF_DRAW_FROM_LOWER_END ) ) {
				clipRect.GetMins()[ xDim ] = cachedClientRect[ xDim ] + cachedClientRect[ xDim + 2 ] - ( cachedClientRect[ xDim + 2 ] * upperPercent );
				clipRect.GetMaxs()[ xDim ] = cachedClientRect[ xDim ] + cachedClientRect[ xDim + 2 ] - ( cachedClientRect[ xDim + 2 ] * lowerPercent );
			} else {
				clipRect.GetMins()[ xDim ] = cachedClientRect[ xDim ] + cachedClientRect[ xDim + 2 ] * lowerPercent;
				clipRect.GetMaxs()[ xDim ] = cachedClientRect[ xDim ] + cachedClientRect[ xDim + 2 ] * upperPercent;
			}

			deviceContext->PushClipRect( clipRect );
			{
				segmentCount = numSegments;
				i = 0;
				while ( segmentCount > 0.f ) {
					DrawSegment( highlightColor, SP_HIGHLIGHT_BEGIN, SP_HIGHLIGHT_CENTER, SP_HIGHLIGHT_END, xDim, yDim, i, totalDim );
					i++;
					segmentCount--;
				}
			}
			deviceContext->PopClipRect();
		}
	}	
	
	// text
	DrawText();

	// border
	if ( borderWidth > 0.0f ) {
		deviceContext->DrawClippedBox( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w, borderWidth, borderColor );
	}
	PostDraw();
}

/*
============
sdUIProgress::OnFillMaterialChanged
============
*/
void sdUIProgress::OnFillMaterialChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->SetupPart( sliderParts[ SP_BEGIN ],	"b", newValue );
	GetUI()->SetupPart( sliderParts[ SP_CENTER ],	"c", newValue );
	GetUI()->SetupPart( sliderParts[ SP_END ],		"e", newValue );
}

/*
============
sdUIProgress::OnHighlightFillMaterialChanged
============
*/
void sdUIProgress::OnHighlightFillMaterialChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->SetupPart( sliderParts[ SP_HIGHLIGHT_BEGIN ],		"b", newValue );
	GetUI()->SetupPart( sliderParts[ SP_HIGHLIGHT_CENTER ],		"c", newValue );
	GetUI()->SetupPart( sliderParts[ SP_HIGHLIGHT_END ],		"e", newValue );
}

/*
============
sdUIProgress::EndLevelLoad
============
*/
void sdUIProgress::EndLevelLoad() {
	sdUIWindow::EndLevelLoad();
	sdUserInterfaceLocal::LookupPartSizes( sliderParts.Begin(), sliderParts.Num() );
}
