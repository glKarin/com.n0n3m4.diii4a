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
#include "UIWindow_Shaped.h"
#include "UserInterfaceLocal.h"

#include "../../sys/sys_local.h"

using namespace sdProperties;

const char sdUITemplateFunctionInstance_IdentifierShaped[]	= "sdUIShapedFunction";
idHashMap< sdUIWindow_Shaped::ShapedTemplateFunction* >	sdUIWindow_Shaped::shapedFunctions;

/*
================
sdUIWindow_Shaped::sdUIWindow_Shaped
================
*/
sdUIWindow_Shaped::sdUIWindow_Shaped( void ) {

}

/*
================
sdUIWindow_Shaped::~sdUIWindow_Shaped
================
*/
sdUIWindow_Shaped::~sdUIWindow_Shaped( void ) {
	DisconnectGlobalCallbacks();
}


/*
============
sdUIWindow_Shaped::DrawLocal
============
*/
void sdUIWindow_Shaped::DrawLocal() {
	sdUIWindow::DrawLocal();
}


/*
============
sdUIWindow_Shaped::FindFunction
============
*/
const sdUIWindow_Shaped::ShapedTemplateFunction* sdUIWindow_Shaped::FindFunction( const char* name ) {
	sdUIWindow_Shaped::ShapedTemplateFunction** ptr;
	return shapedFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUIWindow_Shaped::GetFunction
============
*/
sdUIFunctionInstance* sdUIWindow_Shaped::GetFunction( const char* name ) {
	const ShapedTemplateFunction* function = sdUIWindow_Shaped::FindFunction( name );
	if ( !function ) {		
		return sdUIWindow::GetFunction( name );
	}
	return new sdUITemplateFunctionInstance< sdUIWindow_Shaped, sdUITemplateFunctionInstance_IdentifierShaped >( this, function );
}

/*
============
sdUIWindow_Shaped::RunNamedMethod
============
*/
bool sdUIWindow_Shaped::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const ShapedTemplateFunction* func = sdUIWindow_Shaped::FindFunction( name );
	if ( !func ) {
		return sdUIWindow::RunNamedMethod( name, stack );
	}

	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}


/*
============
sdUIWindow_Shaped::InitFunctions
============
*/
void sdUIWindow_Shaped::InitFunctions() {
	shapedFunctions.Set( "pushShapeVertex",				new ShapedTemplateFunction( 'v', "4",		&sdUIWindow_Shaped::Script_PushShapeVertex ) );		// vertex info ( x, y, s, t )
	shapedFunctions.Set( "drawMaterialShape",			new ShapedTemplateFunction( 'v', "s4",		&sdUIWindow_Shaped::Script_DrawMaterialShape ) );			// material, color
}


/*
============
sdUIWindow_Shaped::Script_PushShapeVertex
============
*/
void sdUIWindow_Shaped::Script_PushShapeVertex( sdUIFunctionStack& stack ) {
	idVec4 point;
	stack.Pop( point );

	drawWinding.AddPoint( point.x, point.y, point.z, point.w );
}

/*
============
sdUIWindow_Shaped::Script_DrawMaterialShape
============
*/
void sdUIWindow_Shaped::Script_DrawMaterialShape( sdUIFunctionStack& stack ) {
	if( drawWinding.GetNumPoints() < 3 ) {
		return;
	}

	idStr material;
	idVec4 color;
	stack.Pop( material );
	stack.Pop( color );

	uiMaterialInfo_t mi;
	GetUI()->LookupMaterial( material, mi );
	deviceContext->DrawWindingMaterial( drawWinding, mi.material, color );

	drawWinding.Clear();
}
