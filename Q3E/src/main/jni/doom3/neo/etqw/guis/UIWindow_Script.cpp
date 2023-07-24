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

using namespace sdProperties;

#define INIT_SCRIPT_FUNCTION( SCRIPTNAME, RETURN, PARMS, FUNCTION ) windowFunctions.Set( SCRIPTNAME, new sdUITemplateFunction< sdUIWindow >( RETURN, PARMS, &sdUIWindow::FUNCTION ) );

/*
===============================================================================

	sdUIWindow

===============================================================================
*/

/*
============
sdUIWindow::InitFunctions
============
*/
#pragma inline_depth( 0 )
#pragma optimize( "", off )
SD_UI_PUSH_CLASS_TAG( sdUIWindow )
void sdUIWindow::InitFunctions( void ) {
	SD_UI_FUNC_TAG( attachRenderCallback, "Attach a render callback to the drawing of the window." )
		SD_UI_FUNC_PARM( string, "callbackName", "Name of the render callback." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "attachRenderCallback", 'v', "s", Script_AttachRenderCallback );

	SD_UI_FUNC_TAG( attachInputHandler, "Attach an input handler for the window." )
		SD_UI_FUNC_PARM( string, "inputHandlerName", "Name of the input handler callback." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "attachInputHandler", 'v', "s", Script_AttachInputHandler );

	SD_UI_FUNC_TAG( drawMaterial, "Draw a material." )
		SD_UI_FUNC_PARM( string, "materialName", "Material name." )
		SD_UI_FUNC_PARM( rect, "rectangle", "The rectangle where we draw the material." )
		SD_UI_FUNC_PARM( color, "materialColor", "The color to use." )
		SD_UI_FUNC_PARM( vec2, "offset", "Texture offset." )
		SD_UI_FUNC_PARM( vec2, "scale", "Texture scale." )
		SD_UI_FUNC_PARM( float, "angle", "Angle to draw." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawMaterial", 'v', "s4422f", Script_DrawMaterial );

	SD_UI_FUNC_TAG( drawRenderCallback, "Call the draw render callback for the given render callback handle." )
		SD_UI_FUNC_PARM( handle, "renderCallback", "Render callback handle." )
		SD_UI_FUNC_PARM( rect, "drawRect", "Draw rectangle." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawRenderCallback", 'v', "i4", Script_DrawRenderCallback );

	SD_UI_FUNC_TAG( drawCachedMaterial, "Draw cached material." )
		SD_UI_FUNC_PARM( handle, "materialHandle", "Material handle." )
		SD_UI_FUNC_PARM( rect, "rectangle", "Rectangle to draw in." )
		SD_UI_FUNC_PARM( color, "materialColor", "Color of the material." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawCachedMaterial", 'v', "i44", Script_DrawCachedMaterial );

	SD_UI_FUNC_TAG( getCachedMaterialDimensions, "Get the width and height of the material." )
		SD_UI_FUNC_PARM( handle, "materialHandle", "Material handle." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getCachedMaterialDimensions", '2', "i", Script_GetCachedMaterialDimensions );

	SD_UI_FUNC_TAG( drawMaterialInfo, "Draw material. Get data from the material info." )
		SD_UI_FUNC_PARM( handle, "material", "Material name." )
		SD_UI_FUNC_PARM( rect, "rectangle", "Rectangle to draw in." )
		SD_UI_FUNC_PARM( color, "materialColor", "Color of the material." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawMaterialInfo", 'v', "s44", Script_DrawMaterialInfo );

	SD_UI_FUNC_TAG( drawMaterialArc, "Draw an arc." )
		SD_UI_FUNC_PARM( string, "material", "Material name." )
		SD_UI_FUNC_PARM( vec2, "origin", "Origin where arc is drawn from." )
		SD_UI_FUNC_PARM( color, "materialColor", "Color of the material." )
		SD_UI_FUNC_PARM( float, "radius", "Radius of arc." )
		SD_UI_FUNC_PARM( float, "start", "Starting angle." )
		SD_UI_FUNC_PARM( float, "percent", "Percent of coverage." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawMaterialArc", 'v', "s24fff", Script_DrawMaterialArc );

	SD_UI_FUNC_TAG( drawTimer, "Draw a timer, used by the grenade indicator and progress indicators by the crosshair." )
		SD_UI_FUNC_PARM( handle, "materialHandle", "Material handle." )
		SD_UI_FUNC_PARM( rect, "rectangle", "Position and size of timer." )
		SD_UI_FUNC_PARM( color, "materialColor", "Color of the material." )
		SD_UI_FUNC_PARM( float, "percent", "Percent complete, between 0 and 1." )
		SD_UI_FUNC_PARM( float, "invert", "Invert the direction of the timer." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawTimer", 'v', "i44ff", Script_DrawTimer );

	SD_UI_FUNC_TAG( drawText, "Draw text." )
		SD_UI_FUNC_PARM( wstring, "text", "Text to be drawn." )
		SD_UI_FUNC_PARM( rect, "rectangle", "rectangle to draw text in." )
		SD_UI_FUNC_PARM( color, "materialColor", "Color of the material." )
		SD_UI_FUNC_PARM( float, "scale", "Text scale." )
		SD_UI_FUNC_PARM( float, "flags", "Draw text flags." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawText", 'v', "w44ff", Script_DrawText );

	SD_UI_FUNC_TAG( measureText, "Measure the text." )
		SD_UI_FUNC_PARM( wstring, "text", "Text to measure." )
		SD_UI_FUNC_PARM( rect, "rectangle", "rectangle to draw text in." )
		SD_UI_FUNC_PARM( float, "scale", "Text scale." )
		SD_UI_FUNC_PARM( float, "flags", "Draw text flags." )
		SD_UI_FUNC_RETURN_PARM( vec2, "The width and height of the text measured." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "measureText", '2', "w4ff", Script_MeasureText );

	SD_UI_FUNC_TAG( measureLocalizedText, "Measure the text." )
		SD_UI_FUNC_PARM( handle, "textHandle", "Handle to text to measure." )
		SD_UI_FUNC_PARM( rect, "rectangle", "rectangle to draw text in." )
		SD_UI_FUNC_PARM( float, "scale", "Text scale." )
		SD_UI_FUNC_PARM( float, "flags", "Draw text flags." )
		SD_UI_FUNC_RETURN_PARM( vec2, "The width and height of the text measured." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "measureLocalizedText", '2', "i4ff", Script_MeasureLocalizedText );

	SD_UI_FUNC_TAG( drawTiledMaterial, "Draw a tiled material." )
		SD_UI_FUNC_PARM( handle, "textHandle", "Handle to text to measure." )
		SD_UI_FUNC_PARM( rect, "rectangle", "rectangle to draw text in." )
		SD_UI_FUNC_PARM( color, "materialColor", "Color of the material." )
		SD_UI_FUNC_PARM( vec2, "repeats", "Number of X/Y repeats." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawTiledMaterial", 'v', "i442", Script_DrawTiledMaterial );

	SD_UI_FUNC_TAG( drawLocalizedText, "Text to be drawn." )
		SD_UI_FUNC_PARM( handle, "textHandle", "Handle to text to be drawn." )
		SD_UI_FUNC_PARM( rect, "rectangle", "rectangle to draw text in." )
		SD_UI_FUNC_PARM( color, "materialColor", "Color of the material." )
		SD_UI_FUNC_PARM( float, "scale", "Text scale." )
		SD_UI_FUNC_PARM( float, "flags", "Draw text flags." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawLocalizedText", 'v', "i44ff", Script_DrawLocalizedText );

	SD_UI_FUNC_TAG( drawLine, "Draw a line." )
		SD_UI_FUNC_PARM( vec2, "start", "Line start." )
		SD_UI_FUNC_PARM( vec2, "end", "Line end." )
		SD_UI_FUNC_PARM( color, "lineColor", "Line color." )
		SD_UI_FUNC_PARM( float, "width", "Line width." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawLine", 'v', "224f", Script_DrawLine );

	SD_UI_FUNC_TAG( requestLayout, "Request the window layout to be calculated again." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "requestLayout", 'v', "", Script_RequestLayout );

	SD_UI_FUNC_TAG( drawRect, "Draw a rectangle." )
		SD_UI_FUNC_PARM( rect, "rectangle", "Rectangle to draw." )
		SD_UI_FUNC_PARM( color, "rectangleColor", "Rectangle color." )
		SD_UI_FUNC_PARM( float, "borderWidth", "Width of the border." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawRect", 'v', "44f", Script_DrawRect );

	SD_UI_FUNC_TAG( nextTabStop, "Get the next tab stop for the specified window." )
		SD_UI_FUNC_PARM( string, "window", "Window name." )
		SD_UI_FUNC_RETURN_PARM( string, "The name of the next tab stop window or self.name if there are no other tab stops." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "nextTabStop", 's', "s", Script_NextTabStop );

	SD_UI_FUNC_TAG( prevTabStop, "Get the previous tab stop for the specified window." )
		SD_UI_FUNC_PARM( string, "window", "Window name." )
		SD_UI_FUNC_RETURN_PARM( string, "The name of the previous tab stop window or self.name if there are no other tab stops." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "prevTabStop", 's', "s", Script_PrevTabStop );

	SD_UI_FUNC_TAG( setTabStop, "Focuses the nth tab stop." )
		SD_UI_FUNC_PARM( float, "tabIndex", "Tab stop to focus." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setTabStop", 'v', "f", Script_SetTabStop );

	SD_UI_FUNC_TAG( containsPoint, "Checks if the (world-based) point is in the client rectangle." )
		SD_UI_FUNC_PARM( rect, "rectangle", "Client rectangle." )
		SD_UI_FUNC_PARM( float, "screenX", "X position." )
		SD_UI_FUNC_PARM( float, "screenY", "Y position." )
		SD_UI_FUNC_RETURN_PARM( float, "Return 1 if screenX,screenY is within the client rectangle." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "containsPoint", 'f', "4ff", Script_ContainsPoint );

	SD_UI_FUNC_TAG( cacheRenderCallback, "Return a reference to a render callback." )
		SD_UI_FUNC_PARM( string, "callbackName", "Name of the render callback." )
		SD_UI_FUNC_RETURN_PARM( string, "Reference to the render callback." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "cacheRenderCallback", 'i', "s", Script_CacheRenderCallback );

	SD_UI_FUNC_TAG( clipToRect, "Clip subsequent drawing to the clipping rectangle. Must be ended with unclipRect after clipped drawing is finished." )
		SD_UI_FUNC_PARM( rect, "clipRectangle", "Clipping rectangle." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "clipToRect", 'v', "4", Script_ClipToRect );

	SD_UI_FUNC_TAG( unclipRect, "Remove clipping to rectangle specified in clipToRect." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "unclipRect", 'v', "", Script_UnclipRect );

	SD_UI_FUNC_TAG( pushColor, "Push a color on the GUI's color stack." )
		SD_UI_FUNC_PARM( color, "pushColor", "Color to push." )
		SD_UI_FUNC_PARM( float, "inherit", "If not 0, multiply the color with the color on the top of the color stack before pushing." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "pushColor", 'v', "4f", Script_PushColor );

	SD_UI_FUNC_TAG( pushColorComponents, "Push a color on the GUI's color stack." )
		SD_UI_FUNC_PARM( float, "R", "Red." )
		SD_UI_FUNC_PARM( float, "G", "Green." )
		SD_UI_FUNC_PARM( float, "B", "Blue." )
		SD_UI_FUNC_PARM( float, "A", "Alpha." )
		SD_UI_FUNC_PARM( float, "inherit", "If not 0, multiply the color with the color on the top of the color stack before pushing." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "pushColorComponents", 'v', "fffff", Script_PushColor );				// r,g,b,a, inherit previous alpha	(not a C&P error, just different ways of getting the same inputs)
	
	SD_UI_FUNC_TAG( popColor, "Pop a color from the GUI's color stack." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "popColor", 'v', "", Script_PopColor );

	SD_UI_FUNC_TAG( drawShaderParm, "Set a shader parm value." )
		SD_UI_FUNC_PARM( float, "index", "Shader parameter index." )
		SD_UI_FUNC_PARM( float, "value", "Shader parameter value." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawShaderParm", 'v', "ff", Script_SetShaderParm );

	SD_UI_FUNC_TAG( isVisible, "Check if self is visible." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns false if self or any parents are hidden." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "isVisible", 'f', "", Script_IsVisible );

	SD_UI_PUSH_GROUP_TAG( "Text Align Flags" )

	SD_UI_ENUM_TAG( TA_LEFT, "Left align text." )
	sdDeclGUI::AddDefine( va( "TA_LEFT %i", TA_LEFT ) );
	SD_UI_ENUM_TAG( TA_CENTER, "Center align text." )
	sdDeclGUI::AddDefine( va( "TA_CENTER %i", TA_CENTER ) );
	SD_UI_ENUM_TAG( TA_RIGHT, "Right align text." )
	sdDeclGUI::AddDefine( va( "TA_RIGHT %i", TA_RIGHT ) );

	SD_UI_ENUM_TAG( TA_TOP, "Align to the top." )
	sdDeclGUI::AddDefine( va( "TA_TOP %i", TA_TOP ) );
	SD_UI_ENUM_TAG( TA_VCENTER, "Align to the vertical center." )
	sdDeclGUI::AddDefine( va( "TA_VCENTER %i", TA_VCENTER ) );
	SD_UI_ENUM_TAG( TA_BOTTOM, "Align to the bottom." )
	sdDeclGUI::AddDefine( va( "TA_BOTTOM %i", TA_BOTTOM ) );

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Window Flags" )

	SD_UI_ENUM_TAG( WF_CLIP_TO_RECT, "Clip all drawing to the window rectangle." )
	sdDeclGUI::AddDefine( va( "WF_CLIP_TO_RECT %i", WF_CLIP_TO_RECT ) );
	SD_UI_ENUM_TAG( WF_COLOR_ESCAPES, "Parse color escapes when drawing text." )
	sdDeclGUI::AddDefine( va( "WF_COLOR_ESCAPES %i", WF_COLOR_ESCAPES ) );
	SD_UI_ENUM_TAG( WF_CAPTURE_KEYS, "Capture all key events even if the window does not have focus." )
	sdDeclGUI::AddDefine( va( "WF_CAPTURE_KEYS %i", WF_CAPTURE_KEYS ) );
	SD_UI_ENUM_TAG( WF_CAPTURE_MOUSE, "Capture mouse click events even if mouse is not inside the window. Capture mouse movement even when window does not have focus." )
	sdDeclGUI::AddDefine( va( "WF_CAPTURE_MOUSE %i", WF_CAPTURE_MOUSE ) );
	SD_UI_ENUM_TAG( WF_WRAP_TEXT, "Wrap text if wider than rectangle width." )
	sdDeclGUI::AddDefine( va( "WF_WRAP_TEXT %i", WF_WRAP_TEXT ) );
	SD_UI_ENUM_TAG( WF_MULTILINE_TEXT, "Support line breaks." )
	sdDeclGUI::AddDefine( va( "WF_MULTILINE_TEXT %i", WF_MULTILINE_TEXT ) );
	SD_UI_ENUM_TAG( WF_AUTO_SIZE_WIDTH, "Resize the width to fit all the text inside the rectangle. Recalculated when the window layout is dirty." )
	sdDeclGUI::AddDefine( va( "WF_AUTO_SIZE_WIDTH %i", WF_AUTO_SIZE_WIDTH ) );
	SD_UI_ENUM_TAG( WF_AUTO_SIZE_HEIGHT, "Resize the height to fit all the text inside the rectangle. Recalculated when the window layout is dirty." )
	sdDeclGUI::AddDefine( va( "WF_AUTO_SIZE_HEIGHT %i", WF_AUTO_SIZE_HEIGHT ) );
	SD_UI_ENUM_TAG( WF_ALLOW_FOCUS, "Allow window to have focus." )
	sdDeclGUI::AddDefine( va( "WF_ALLOW_FOCUS %i", WF_ALLOW_FOCUS ) );
	SD_UI_ENUM_TAG( WF_TAB_STOP, "Should stop at window when using tab to cycle focus window." )
	sdDeclGUI::AddDefine( va( "WF_TAB_STOP %i", WF_TAB_STOP ) );
	SD_UI_ENUM_TAG( WF_TRUNCATE_TEXT, "Truncate with ... if text goes outside the rectangle. WF_MULTILINE_TEXT and WF_WRAP_TEXT must not be specified for this flag to have an effect." )
	sdDeclGUI::AddDefine( va( "WF_TRUNCATE_TEXT %i",	WF_TRUNCATE_TEXT ) );
	SD_UI_ENUM_TAG( WF_INHERIT_PARENT_COLORS, "Multiply colorMultiplier with the parent's color before pushing on the color stack." )
	sdDeclGUI::AddDefine( va( "WF_INHERIT_PARENT_COLORS %i",	WF_INHERIT_PARENT_COLORS ) );
	SD_UI_ENUM_TAG( WF_DROPSHADOW, "Add a drop shadow to the text." )
	sdDeclGUI::AddDefine( va( "WF_DROPSHADOW %i",	WF_DROPSHADOW ) );

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Draw Text Flags" )

	SD_UI_ENUM_TAG( DTF_TOP, "Align to the top." )
	sdDeclGUI::AddDefine( va( "DTF_TOP %i",		DTF_TOP ) );
	SD_UI_ENUM_TAG( DTF_VCENTER, "Align to the vertical center." )
	sdDeclGUI::AddDefine( va( "DTF_VCENTER %i", DTF_VCENTER ) );
	SD_UI_ENUM_TAG( DTF_BOTTOM, "Align to the bottom." )
	sdDeclGUI::AddDefine( va( "DTF_BOTTOM %i",	DTF_BOTTOM ) );

	SD_UI_ENUM_TAG( DTF_LEFT, "Align text to the left." )
	sdDeclGUI::AddDefine( va( "DTF_LEFT %i",	DTF_LEFT ) );
	SD_UI_ENUM_TAG( DTF_CENTER, "Center align text." )
	sdDeclGUI::AddDefine( va( "DTF_CENTER %i",	DTF_CENTER ) );
	SD_UI_ENUM_TAG( DTF_RIGHT, "Align text to the right." )
	sdDeclGUI::AddDefine( va( "DTF_RIGHT %i",	DTF_RIGHT ) );

	SD_UI_ENUM_TAG( DTF_SINGLELINE, "Single line text only." )
	sdDeclGUI::AddDefine( va( "DTF_SINGLELINE %i",	DTF_SINGLELINE ) );
	SD_UI_ENUM_TAG( DTF_WORDWRAP, "Wrap the text to the next line if longer than the rectangle width." )
	sdDeclGUI::AddDefine( va( "DTF_WORDWRAP %i",	DTF_WORDWRAP ) );
	SD_UI_ENUM_TAG( DTF_DROPSHADOW, "Add a drop shadow to the text." )
	sdDeclGUI::AddDefine( va( "DTF_DROPSHADOW %i",	DTF_DROPSHADOW ) );
	SD_UI_ENUM_TAG( DTF_TRUNCATE, "Truncate with ... if text goes outside the rectangle. DTF_SINGLELINE must also be specified for this flag to have an effect." )
	sdDeclGUI::AddDefine( va( "DTF_TRUNCATE %i",	DTF_TRUNCATE ) );

	SD_UI_POP_GROUP_TAG
}
SD_UI_POP_CLASS_TAG
#pragma optimize( "", on )
#pragma inline_depth()

/*
============
sdUIWindow::Script_AttachRenderCallback
============
*/
void sdUIWindow::Script_AttachRenderCallback( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );
	
	uiManager->SetRenderCallback( this, name );
}

/*
============
sdUIWindow::Script_AttachInputHandler
============
*/
void sdUIWindow::Script_AttachInputHandler( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );
	
	uiManager->SetInputHandler( this, name );
}

/*
============
sdUIWindow::Script_DrawMaterial
============
*/
void sdUIWindow::Script_DrawMaterial( sdUIFunctionStack& stack ) {
	idStr	material;
	idVec4	rect;
	idVec4	color;
	idVec2	shift;
	idVec2	scale;
	float	angle;

	stack.Pop( material );
	stack.Pop( rect );
	stack.Pop( color );
	stack.Pop( shift );
	stack.Pop( scale );
	stack.Pop( angle );

	uiMaterialInfo_t mi;
	GetUI()->LookupMaterial( material, mi );	
	deviceContext->DrawMaterial( rect, mi.material, color, scale, shift, angle );
}

/*
============
sdUIWindow::Script_DrawMaterialInfo
============
*/
void sdUIWindow::Script_DrawMaterialInfo( sdUIFunctionStack& stack ) {
	idStr	material;
	idVec4	rect;
	idVec4	color;

	stack.Pop( material );
	stack.Pop( rect );
	stack.Pop( color );

	uiMaterialInfo_t mi;
	idStr outMaterial;

	GetUI()->LookupMaterial( material, mi );
	DrawMaterial( mi, rect.x, rect.y, rect.z, rect.w, color );
}

/*
============
sdUIWindow::Script_DrawRect
============
*/
void sdUIWindow::Script_DrawRect( sdUIFunctionStack& stack ) {
	idVec4	rect;
	idVec4	color;
	float	size;

	stack.Pop( rect );
	stack.Pop( color );
	stack.Pop( size );

	if ( size <= 0.0f ) {
		deviceContext->DrawClippedRect( rect.x, rect.y, rect.z, rect.w, color );
	} else {
		deviceContext->DrawClippedBox( rect.x, rect.y, rect.z, rect.w, size, color );
	}
}

/*
============
sdUIWindow::Script_DrawText
============
*/
void sdUIWindow::Script_DrawText( sdUIFunctionStack& stack ) {
	idWStr	text;
	idVec4	rect;
	idVec4	color;
	float	scale;
	float	flags;

	stack.Pop( text );
	stack.Pop( rect );
	stack.Pop( color );
	stack.Pop( scale );
	stack.Pop( flags );

	if( text.IsEmpty() ) {
		return;
	}

	ActivateFont( false );
	
	int iFlags = idMath::Ftoi( flags );

	sdWStringBuilder_Heap  builder;
	const wchar_t* drawText = text.c_str();
	if( ( iFlags & DTF_TRUNCATE ) && ( iFlags & DTF_SINGLELINE ) ) {		

		const wchar_t* truncationText = L"...";
		int truncationWidth;
		int truncationHeight;
		deviceContext->GetTextDimensions( truncationText, sdBounds2D( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT ), DTF_SINGLELINE | DTF_VCENTER | DTF_CENTER, cachedFontHandle, scale, truncationWidth, truncationHeight );

		ShortenText( text.c_str(), sdBounds2D( rect ), cachedFontHandle, iFlags, scale, truncationText, truncationWidth, builder );
		drawText = builder.c_str();
	}
	deviceContext->SetColor( color );
	deviceContext->SetFontSize( scale );
	deviceContext->DrawText( drawText, sdBounds2D( rect ), iFlags );
}

/*
============
sdUIWindow::Script_MeasureText
============
*/
void sdUIWindow::Script_MeasureText( sdUIFunctionStack& stack ) {
	idWStr	text;
	idVec4	rect;
	float	scale;
	float	flags;

	stack.Pop( text );
	stack.Pop( rect );
	stack.Pop( scale );
	stack.Pop( flags );

	if ( text.Length() == 0 ) {
		stack.Push( idVec2( 0.0f, 0.0f ) );
	} else {
		ActivateFont( false );

		int w, h;
		deviceContext->GetTextDimensions( text.c_str(), sdBounds2D( rect ), idMath::Ftoi( flags ), cachedFontHandle, idMath::Ftoi( scale ), w, h );
		stack.Push( idVec2( w, h ) );
	}
}

/*
============
sdUIWindow::Script_MeasureLocalizedText
============
*/
void sdUIWindow::Script_MeasureLocalizedText( sdUIFunctionStack& stack ) {
	int	text;
	idVec4	rect;
	float	scale;
	float	flags;

	stack.Pop( text );
	stack.Pop( rect );
	stack.Pop( scale );
	stack.Pop( flags );

	int w = 0;
	int h = 0;

	ActivateFont( false );

	const sdDeclLocStr* str = declHolder.declLocStrType.LocalFindByIndex( text, false );
	if ( str != NULL && str->GetText()[ 0 ] != L'\0' ) {
		deviceContext->GetTextDimensions( str->GetText(), sdBounds2D( rect ), idMath::Ftoi( flags ), cachedFontHandle, idMath::Ftoi( scale ), w, h );
	}
	
	stack.Push( idVec2( w, h ) );
}

/*
============
sdUIWindow::Script_DrawLocalizedText
============
*/
void sdUIWindow::Script_DrawLocalizedText( sdUIFunctionStack& stack ) {
	int		text;
	idVec4	rect;
	idVec4	color;
	float	scale;
	float	flags;

	stack.Pop( text );
	stack.Pop( rect );
	stack.Pop( color );
	stack.Pop( scale );
	stack.Pop( flags );

	if( text >= 0 && text < declHolder.declLocStrType.Num() ) {
		const wchar_t* textStr = declHolder.FindLocStrByIndex( text )->GetText();

		if( textStr[ 0 ] == L'\0' ) {
			return;
		}

		ActivateFont( false );

		int iFlags = idMath::Ftoi( flags );
		const wchar_t* drawText = textStr;
		sdWStringBuilder_Heap  builder;
		if( ( iFlags & DTF_TRUNCATE ) && ( iFlags & DTF_SINGLELINE ) ) {
			const wchar_t* truncationText = L"...";
			int truncationWidth;
			int truncationHeight;
			deviceContext->GetTextDimensions( truncationText, sdBounds2D( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT ), DTF_SINGLELINE | DTF_VCENTER | DTF_CENTER, cachedFontHandle, scale, truncationWidth, truncationHeight );

			ShortenText(textStr, sdBounds2D( rect ), cachedFontHandle, iFlags, scale, truncationText, truncationWidth, builder );
			drawText = builder.c_str();
		}
		
		deviceContext->SetColor( color );
		deviceContext->SetFontSize( scale );
		deviceContext->DrawText( drawText, sdBounds2D( rect ), iFlags );
	}	
}


/*
============
sdUIWindow::Script_DrawLine
============
*/
void sdUIWindow::Script_DrawLine( sdUIFunctionStack& stack ) {
	idVec2	start;
	idVec2	end;
	idVec4	color;
	float	width;

	stack.Pop( start );
	stack.Pop( end );
	stack.Pop( color );
	stack.Pop( width );

	deviceContext->DrawLine( start, end, width, color );
}

/*
============
sdUIWindow::Script_DrawMaterialArc
============
*/
void sdUIWindow::Script_DrawMaterialArc( sdUIFunctionStack& stack ) {
	idStr material;
	idVec2 origin;
	float radius;
	float percent;
	float start;
	idVec4 color;

	stack.Pop( material );
	stack.Pop( origin );
	stack.Pop( color );
	stack.Pop( radius );
	stack.Pop( start );
	stack.Pop( percent );

	uiMaterialInfo_t mi;
	GetUI()->LookupMaterial( material, mi );
	deviceContext->DrawFilledArc( origin.x, origin.y, radius, 32, percent, color, start, mi.material );
}

/*
============
sdUIWindow::Script_RequestLayout
============
*/
void sdUIWindow::Script_RequestLayout( sdUIFunctionStack& stack ) {
	MakeLayoutDirty();
}

/*
============
sdUIWindow::Script_NextTabStop
============
*/
void sdUIWindow::Script_NextTabStop( sdUIFunctionStack& stack ) {
	idStr currentItem;
	stack.Pop( currentItem );

	const sdUIObject* targetChild = GetUI()->GetWindow( currentItem.c_str() );
	if( targetChild == NULL ) {
		stack.Push( "" );
		return;
	}

	int numTabStops = NumTabStops_r( this );
	if( numTabStops == 0 ) {
		stack.Push( "" );
		return;
	}

	int index = 0;
	const sdUIObject** tabStops = static_cast< const sdUIObject** >( _alloca( numTabStops * sizeof( const sdUIObject* )) );
	ListTabStops_r( this, tabStops, index );

	int i;
	for( i = 0; i < numTabStops; i++ ) {
		if( tabStops[ i ] == targetChild ) {
			break;
		}
	}
	if( i < numTabStops - 1 ) {
		i++;
	} else {
		i = 0;
	}
	const sdUIObject* tabStop = tabStops[ i ];

	assert( tabStop != NULL );
	stack.Push( tabStop->name.GetValue().c_str() );

	if( sdUserInterfaceLocal::g_debugGUIEvents.GetInteger() ) {
		gameLocal.Printf( "NextTabStop: Found '%s'\n", tabStop->name.GetValue().c_str() );
	}
}

/*
============
sdUIWindow::Script_PrevTabStop
============
*/
void sdUIWindow::Script_PrevTabStop( sdUIFunctionStack& stack ) {
	idStr currentItem;
	stack.Pop( currentItem );

	const sdUIObject* targetChild = GetUI()->GetWindow( currentItem.c_str() );
	if( targetChild == NULL ) {
		stack.Push( "" );
		return;
	}

	int numTabStops = NumTabStops_r( this );
	if( numTabStops == 0 ) {
		stack.Push( "" );
		return;
	}

	int index = 0;
	const sdUIObject** tabStops = static_cast< const sdUIObject** >( _alloca( numTabStops * sizeof( const sdUIObject* )) );
	ListTabStops_r( this, tabStops, index );

	int i;
	for( i = numTabStops - 1; i >= 0; i-- ) {
		if( tabStops[ i ] == targetChild ) {
			break;
		}
	}
	if( i > 0 ) {
		i--;
	} else {
		i = numTabStops - 1;
	}
	const sdUIObject* tabStop = tabStops[ i ];

	stack.Push( tabStop->name.GetValue().c_str() );

	if( sdUserInterfaceLocal::g_debugGUIEvents.GetInteger() ) {
		gameLocal.Printf( "PrevTabStop: Found '%s'\n", tabStop->name.GetValue().c_str() );
	}
}

/*
============
sdUIWindow::Script_SetTabStop
============
*/
void sdUIWindow::Script_SetTabStop( sdUIFunctionStack& stack ) {
	int targetIndex;
	stack.Pop( targetIndex );

	if( targetIndex < 0 ) {
		gameLocal.Warning( "Script_SetTabStop: '%s' '%i' out of range", name.GetValue().c_str(), targetIndex );
		return;
	}

	int numTabStops = NumTabStops_r( this );
	if( numTabStops == 0 ) {
		return;
	}

	int index = 0;
	const sdUIObject** tabStops = static_cast< const sdUIObject** >( _alloca( numTabStops * sizeof( const sdUIObject* )) );
	ListTabStops_r( this, tabStops, index );
	if( targetIndex < numTabStops ) {
		GetUI()->focusedWindowName = tabStops[ targetIndex ]->name;
		return;
	}

	gameLocal.Warning( "Script_SetTabStop: '%s' '%i' out of range", name.GetValue().c_str(), index );
}

/*
============
sdUIWindow::Script_ContainsPoint
============
*/
void sdUIWindow::Script_ContainsPoint( sdUIFunctionStack& stack ) {
	idVec4 rect;
	stack.Pop( rect );

	idVec2 point;
	stack.Pop( point );	
	
	sdBounds2D bounds( rect );
	bool contained = bounds.ContainsPoint( point );
	stack.Push( contained ? 1.0f : 0.0f );
}

/*
============
sdUIWindow::Script_DrawCachedMaterial
============
*/
void sdUIWindow::Script_DrawCachedMaterial( sdUIFunctionStack& stack ) {
	int		handle;
	idVec4	rect;
	idVec4	color;

	stack.Pop( handle );
	stack.Pop( rect );
	stack.Pop( color );
		
	uiMaterialCache_t::Iterator iter = GetUI()->FindCachedMaterialForHandle( handle );
	if( !GetUI()->IsValidMaterial( iter ) ) {
		//assert( 0 );
		return;
	}

	uiCachedMaterial_t& cached = *( iter->second );

	switch( cached.drawMode ) {
		case BDM_USE_ST:
		case BDM_SINGLE_MATERIAL:
			DrawMaterial( cached.material, rect.x, rect.y, rect.z, rect.w, color );
			break;
		case BDM_FRAME:
		case BDM_TRI_PART_H:	// FALL THROUGH
		case BDM_TRI_PART_V:	// FALL THROUGH
		case BDM_FIVE_PART_H:	// FALL THROUGH
			DrawFrame( rect, iter, color );
			break;
	}	
}

/*
============
sdUIWindow::Script_DrawTiledMaterial
============
*/
void sdUIWindow::Script_DrawTiledMaterial( sdUIFunctionStack& stack ) {
	int		handle;
	idVec4	rect;
	idVec4	color;
	idVec2	repeats;

	stack.Pop( handle );
	stack.Pop( rect );
	stack.Pop( color );
	stack.Pop( repeats );

	uiMaterialCache_t::Iterator iter = GetUI()->FindCachedMaterialForHandle( handle );
	if( !GetUI()->IsValidMaterial( iter ) ) {
		//assert( 0 );
		return;
	}

	uiCachedMaterial_t& cached = *( iter->second );

	int xRepeat = idMath::Ftoi( repeats.x );
	int yRepeat = idMath::Ftoi( repeats.y );

	idVec4 drawRect = rect;
	for( int x = 0; x < xRepeat; x++ ) {
		drawRect.x = rect.x + ( x * rect.z );
		for( int y = 0; y < yRepeat; y++ ) {			
			drawRect.y = rect.y + ( y * rect.w );
			switch( cached.drawMode ) {
			case BDM_USE_ST:
			case BDM_SINGLE_MATERIAL:
				DrawMaterial( cached.material, drawRect.x, drawRect.y, drawRect.z, drawRect.w, color );
				break;
			case BDM_FRAME:
			case BDM_TRI_PART_H:	// FALL THROUGH
			case BDM_TRI_PART_V:	// FALL THROUGH
			case BDM_FIVE_PART_H:	// FALL THROUGH
				DrawFrame( drawRect, iter, color );
				break;
			}
		}
	}
}

/*
============
sdUIWindow::Script_DrawRenderCallback
============
*/
void sdUIWindow::Script_DrawRenderCallback ( sdUIFunctionStack& stack ) {
	int callbackHandle;
	stack.Pop( callbackHandle );

	idVec4 rect;
	stack.Pop( rect );
	 
	uiRenderCallback_t callback = uiManager->GetRenderCallback( callbackHandle );
	if( callback != NULL ) {
		callback( GetUI(), rect.x, rect.y, rect.z, rect.w );
	}
}

/*
============
sdUIWindow::Script_CacheRenderCallback
============
*/
void sdUIWindow::Script_CacheRenderCallback( sdUIFunctionStack& stack ) {
	idStr callbackName;
	stack.Pop( callbackName );

	sdUserInterfaceManager::renderCallbackHandle_t handle = uiManager->GetRenderCallbackHandle( callbackName.c_str() );
	stack.Push( handle );
}

/*
============
sdUIWindow::Script_DrawTimer
============
*/
void sdUIWindow::Script_DrawTimer( sdUIFunctionStack& stack ) {
	int handle;
	stack.Pop( handle );

	idVec4 rect;
	stack.Pop( rect );

	idVec4 color;
	stack.Pop( color );
	
	float percent;
	stack.Pop( percent );

	float invert;
	stack.Pop( invert );

	uiMaterialCache_t::Iterator iter = GetUI()->FindCachedMaterialForHandle( handle );
	if( !GetUI()->IsValidMaterial( iter ) ) {
		return;
	}
	uiCachedMaterial_t& cached = *iter->second;
	if( cached.material.material == NULL ) {
		return;
	}
	float halfW = rect.z * 0.5f;
	float halfH = rect.w * 0.5f;
	deviceContext->DrawTimer( rect.x + halfW, rect.y + halfH, halfW, halfH, percent, color, cached.material.material, invert != 0.0f, cached.material.st0, cached.material.st1 );
}

/*
============
sdUIWindow::Script_ClipToRect
============
*/
void sdUIWindow::Script_ClipToRect( sdUIFunctionStack& stack ) {
	idVec4 rect;
	stack.Pop( rect );
	deviceContext->PushClipRect( sdBounds2D( rect ) );
}

/*
============
sdUIWindow::Script_UnclipRect
============
*/
void sdUIWindow::Script_UnclipRect( sdUIFunctionStack& stack ) {
	deviceContext->PopClipRect();
}

/*
============
sdUIWindow::Script_GetCachedMaterialDimensions
============
*/
void sdUIWindow::Script_GetCachedMaterialDimensions( sdUIFunctionStack& stack ) {
	int		handle;
	stack.Pop( handle );

	uiMaterialCache_t::Iterator iter = GetUI()->FindCachedMaterialForHandle( handle );
	if( !GetUI()->IsValidMaterial( iter ) ) {
		stack.Push( idVec2( cachedClientRect.z, cachedClientRect.w ) );
		return;
	}

	uiCachedMaterial_t& cached = *( iter->second );
	switch( cached.drawMode ) {
		case BDM_USE_ST:
		case BDM_SINGLE_MATERIAL:
			if( const idImage* image = cached.material.material->GetEditorImage() ) {
				stack.Push( idVec2( image->sourceWidth, image->sourceHeight ) );
				return;
			}
			break;
		case BDM_FRAME:
		case BDM_TRI_PART_H:	// FALL THROUGH
		case BDM_TRI_PART_V:	// FALL THROUGH
		case BDM_FIVE_PART_H:	// FALL THROUGH			
			break;
	}
	stack.Push( idVec2( cachedClientRect.z, cachedClientRect.w ) );
}

/*
============
sdUIWindow::Script_IsVisible
============
*/
void sdUIWindow::Script_IsVisible( sdUIFunctionStack& stack ) {
	stack.Push( IsVisible() ? 1.0f : 0.0f );
}

/*
============
sdUIWindow::Script_PushColor
============
*/
void sdUIWindow::Script_PushColor( sdUIFunctionStack& stack ) {
	idVec4 color;
	stack.Pop( color );
	bool inherit;
	stack.Pop( inherit );
	
	if( inherit ) {
		const idVec4& parent = GetUI()->TopColor();
		color.x *= parent.x;
		color.y *= parent.y;
		color.z *= parent.z;
		color.w *= parent.w;
	}
	GetUI()->PushColor( color );
}

/*
============
sdUIWindow::Script_PopColor
============
*/
void sdUIWindow::Script_PopColor( sdUIFunctionStack& stack ) {
	GetUI()->PopColor();
}

/*
============
sdUIWindow::Script_SetShaderParm
============
*/
void sdUIWindow::Script_SetShaderParm( sdUIFunctionStack& stack ) {
	int parm;
	stack.Pop( parm );
	
	float value;
	stack.Pop( value );

	if( parm < 0 || parm >= MAX_ENTITY_SHADER_PARMS ) {
		return;
	}

	deviceContext->SetRegister( parm, value );
}
