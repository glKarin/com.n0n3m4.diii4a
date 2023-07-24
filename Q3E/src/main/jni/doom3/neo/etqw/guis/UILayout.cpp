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
#include "UILayout.h"
#include "UserInterfaceLocal.h"

using namespace sdProperties;

SD_UI_IMPLEMENT_CLASS( sdUILayout_Static, sdUIObject_Drawable )

idCVar gui_debugLayout( "gui_debugLayout", "0", CVAR_GAME | CVAR_BOOL, "Debug UI layout classes" );

/*
============
sdUILayout_Static::sdUILayout_Static
============
*/
sdUILayout_Static::sdUILayout_Static() {
	GetScope().GetProperties().RegisterProperty( "rect",			rect );
	GetScope().GetProperties().RegisterProperty( "absoluteRect",	absoluteRect );
	GetScope().GetProperties().RegisterProperty( "visible",			visible );

	rect = idVec4( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT );
	
	absoluteRect = rect;
	absoluteRect.SetReadOnly( true );
	visible = 1.0f;

	recalculateLayout = true;

	UI_ADD_VEC4_CALLBACK( rect, sdUILayout_StaticBase, OnRectChanged )
}

/*
============
sdUILayout_Static::~sdUILayout_Static
============
*/
sdUILayout_Static::~sdUILayout_Static() {
}

/*
============
sdUILayout_Static::ApplyLayout
============
*/
void sdUILayout_Static::ApplyLayout() {
	if( recalculateLayout ) {
		sdUIObject* object = GetNode().GetParent();
		idVec4 cachedOffset( rect );

		while( object != NULL ) {
			if( sdProperty* rect = object->GetScope().GetProperty( "rect", sdProperties::PT_VEC4 ) ) {
				cachedOffset.ToVec2() += rect->value.vec4Value->GetValue().ToVec2();
			}
			object = object->GetNode().GetParent();
		}

		absoluteRect.SetReadOnly( false );
		absoluteRect = cachedOffset;
		absoluteRect.SetReadOnly( true );		

		DoApplyLayout();
		recalculateLayout = false;
	}	
	sdUIObject::ApplyLayout();	
}


/*
============
sdUILayout_Static::UpdateToolTip
============
*/
sdUIObject* sdUILayout_Static::UpdateToolTip( const idVec2& cursor ) {
	if( !visible ) {
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
	return NULL;
}

/*
============
sdUILayout_Static::OnRectChanged
============
*/
void sdUILayout_Static::OnRectChanged( const idVec4& oldValue, const idVec4& newValue ) {
	MakeLayoutDirty(); 
}

/*
============
sdUILayout_StaticBase::OnChildRectChanged
============
*/
void sdUILayout_StaticBase::OnChildRectChanged( const idVec4& oldValue, const idVec4& newValue ) {
	for( int i = 0; i < childRects.Num(); i++ ) {
		rectInfo_t& info = childRects[ i ];
		info.rect.z = info.property->value.vec4Value->GetValue().z;
		info.rect.w = info.property->value.vec4Value->GetValue().w;
	}

	MakeLayoutDirty(); 
}

/*
============
sdUILayout_Static::Draw
============
*/
void sdUILayout_Static::Draw() {
	if( !visible ) {
		return;
	}

	sdUIObject* child = GetNode().GetChild();
	while( child != NULL ) {
		if( sdUIObject_Drawable* window = child->Cast< sdUIObject_Drawable >() ) {
			window->Draw();
		}
		child = child->GetNode().GetSibling();
	}
	if( gui_debugLayout.GetBool() ) {
		deviceContext->DrawClippedBox( absoluteRect.GetValue().x, absoluteRect.GetValue().y, absoluteRect.GetValue().z, absoluteRect.GetValue().w, 1.0f, colorGreen );
	}
}

/*
============
sdUILayout_Static::FinalDraw
============
*/
void sdUILayout_Static::FinalDraw() {
	if( !visible ) {
		return;
	}

	sdUIObject* child = GetNode().GetChild();
	while( child != NULL ) {
		if( sdUIObject_Drawable* window = child->Cast< sdUIObject_Drawable >() ) {
			window->FinalDraw();
		}
		child = child->GetNode().GetSibling();
	}
}

/*
============
sdUILayout_Static::PostEvent
============
*/
bool sdUILayout_Static::PostEvent( const sdSysEvent* event ) {
	if( !visible ) {
		return false;
	}

	sdUIObject* child = GetNode().GetChild();
	while( child != NULL ) {
		if( child->PostEvent( event ) ) {
			return true;
		}
		child = child->GetNode().GetSibling();
	}
	return false;
}

/*
============
sdUILayout_Static::HandleFocus
============
*/
bool sdUILayout_Static::HandleFocus( const sdSysEvent* event ) {
	if( !visible ) {
		return false;
	}

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
	return false;
}








SD_UI_IMPLEMENT_CLASS( sdUILayout_StaticBase, sdUILayout_Static )

extern const char sdUITemplateFunctionInstance_IdentifierLayout_Table[] = "Layout_Table";
idHashMap< sdUILayout_StaticBase::sdUILayoutFunction* >	sdUILayout_StaticBase::layoutFunctions;


/*
============
sdUILayout_StaticBase::sdUILayout_StaticBase
============
*/
sdUILayout_StaticBase::sdUILayout_StaticBase() {
	GetScope().GetProperties().RegisterProperty( "margins", margins );
	GetScope().GetProperties().RegisterProperty( "spacing", spacing );

	margins = idVec4( 4.0f, 4.0f, 4.0f, 4.0f );
	rect	= idVec4( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT );
	spacing = idVec2( 2.0f, 2.0f );	

	UI_ADD_VEC4_CALLBACK( margins, sdUILayout_StaticBase, OnMarginsChanged )
	UI_ADD_VEC2_CALLBACK( spacing, sdUILayout_StaticBase, OnSpacingChanged )
}

/*
============
sdUILayout_StaticBase::~sdUILayout_StaticBase
============
*/
sdUILayout_StaticBase::~sdUILayout_StaticBase() {

}

/*
============
sdUILayout_StaticBase::GetFunction
============
*/
sdUIFunctionInstance* sdUILayout_StaticBase::GetFunction( const char* name ) {
	const sdUILayoutFunction* function = sdUILayout_StaticBase::FindFunction( name );
	if( !function ) {		
		return sdUIObject::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUILayout_StaticBase, sdUITemplateFunctionInstance_IdentifierLayout_Table >( this, function );
}
/*
============
sdUILayout_StaticBase::FindFunction
============
*/
const sdUILayout_StaticBase::sdUILayoutFunction* sdUILayout_StaticBase::FindFunction( const char* name ) {
	sdUILayoutFunction** ptr;
	return layoutFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUILayout_StaticBase::InitFunctions
============
*/
SD_UI_PUSH_CLASS_TAG( sdUILayout_StaticBase )
void sdUILayout_StaticBase::InitFunctions( void ) {
	SD_UI_ENUM_TAG( VLF_DRAW_REVERSED, "Draw children in reverse order." )
	sdDeclGUI::AddDefine( va( "VLF_DRAW_REVERSED %i", VLF_DRAW_REVERSED ) );
	SD_UI_ENUM_TAG( VLF_NOSIZE, "Do not extend the child window widths to the right edge of the layout edge." )
	sdDeclGUI::AddDefine( va( "VLF_NOSIZE %i", VLF_NOSIZE ) );
	sdDeclGUI::AddDefine( va( "VLF_DYNAMIC_CHILDREN %i", VLF_DYNAMIC_CHILDREN ) );
}
SD_UI_POP_CLASS_TAG

/*
============
sdUILayout_StaticBase::OnMarginsChanged
============
*/
void sdUILayout_StaticBase::OnMarginsChanged( const idVec4& oldValue, const idVec4& newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUILayout_StaticBase::OnSpacingChanged
============
*/
void sdUILayout_StaticBase::OnSpacingChanged( const idVec2& oldValue, const idVec2& newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUILayout_StaticBase::OnCreate
============
*/
void sdUILayout_StaticBase::OnCreate() {
	sdUIObject* child = GetNode().GetChild();
	while( child != NULL ) {
		sdProperty* property = child->GetScope().GetProperty( "rect", sdProperties::PT_VEC4 );
		if( property != NULL ) {
			rectInfo_t info;
			info.property = property;
			info.rect = *property->value.vec4Value;
			info.source = child;
			childRects.Append( info );
			if( TestFlag( VLF_DYNAMIC_CHILDREN ) ) {
				UI_ADD_VEC4_CALLBACK( ( *property->value.vec4Value ), sdUILayout_StaticBase, OnChildRectChanged )
			}
		}
		child = child->GetNode().GetSibling();
	}
}


/*
============
sdUILayout_StaticBase::Draw
============
*/
void sdUILayout_StaticBase::Draw() {
	if( !visible ) {
		return;
	}

	if( !TestFlag( VLF_DRAW_REVERSED ) ) {
		sdUILayout_Static::Draw();
		return;
	}

	sdUIObject* prev = NULL;
	sdUIObject* child = GetNode().GetChild();
	while( child != NULL ) {
		prev = child;
		child = child->GetNode().GetSibling();
	}

	child = prev;

	while( child != NULL ) {
		if( sdUIObject_Drawable* window = child->Cast< sdUIObject_Drawable >() ) {
			window->Draw();
		}
		child = child->GetNode().GetPriorSibling();
	}
	if( gui_debugLayout.GetBool() ) {
		deviceContext->DrawClippedBox( absoluteRect.GetValue().x, absoluteRect.GetValue().y, absoluteRect.GetValue().z, absoluteRect.GetValue().w, 1.0f, colorGreen );
	}
}

/*
============
sdUILayout_StaticBase::FinalDraw
============
*/
void sdUILayout_StaticBase::FinalDraw() {
	if( !TestFlag( VLF_DRAW_REVERSED ) ) {
		sdUILayout_Static::FinalDraw();
		return;
	}

	sdUIObject* prev = NULL;
	sdUIObject* child = GetNode().GetChild();
	while( child != NULL ) {
		prev = child;
		child = child->GetNode().GetSibling();
	}

	child = prev;

	while( child != NULL ) {
		if( sdUIObject_Drawable* window = child->Cast< sdUIObject_Drawable >() ) {
			window->FinalDraw();
		}
		child = child->GetNode().GetPriorSibling();
	}
}

/*
============
sdUILayout_StaticBase::PostEvent
============
*/
bool sdUILayout_StaticBase::PostEvent( const sdSysEvent* event ) {
	if( !visible ) {
		return false;
	}

	if( !TestFlag( VLF_DRAW_REVERSED ) ) {
		return sdUILayout_Static::PostEvent( event );		
	}

	sdUIObject* prev = NULL;
	sdUIObject* child = GetNode().GetChild();
	while( child != NULL ) {
		prev = child;
		child = child->GetNode().GetSibling();
	}

	child = prev;

	while( child != NULL ) {
		if( child->PostEvent( event ) ) {
			return true;
		}
		child = child->GetNode().GetPriorSibling();
	}
	return false;
}


/*
============
sdUILayout_StaticBase::HandleFocus
============
*/
bool sdUILayout_StaticBase::HandleFocus( const sdSysEvent* event ) {
	if( !visible ) {
		return false;
	}

	if( !TestFlag( VLF_DRAW_REVERSED ) ) {
		return sdUILayout_Static::HandleFocus( event );		
	}

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
	return false;
}

/*
============
sdUILayout_Static::MakeLayoutDirty
============
*/
void sdUILayout_Static::MakeLayoutDirty() {
	recalculateLayout = true;
	MakeLayoutDirty_r( this );
}

SD_UI_IMPLEMENT_CLASS( sdUILayout_Vertical, sdUILayout_StaticBase )


/*
============
sdUILayout_Vertical::sdUILayout_Vertical
============
*/
sdUILayout_Vertical::sdUILayout_Vertical() {
	GetScope().GetProperties().RegisterProperty( "verticalSize",			verticalSize );
	verticalSize = 0.0f;
	verticalSize.SetReadOnly( true );
}
/*
============
sdUILayout_Vertical::DoApplyLayout
============
*/
void sdUILayout_Vertical::DoApplyLayout() {
	idVec2 offset( margins.GetValue().x, margins.GetValue().y );

	for( int i = 0; i < childRects.Num(); i++ ) {
		rectInfo_t& rectInfo = childRects[ i ];

		idVec4 childRect = rectInfo.rect;
		if( TestFlag( VLF_NOSIZE ) == false && rectInfo.source->TestFlag( OF_FIXED_LAYOUT ) == false ) {
			childRect.z = rect.GetValue().z - ( margins.GetValue().x + margins.GetValue().z ) - childRect.x;
		}

		childRect.x = rectInfo.rect.x + offset.x;
		childRect.y = rectInfo.rect.y + offset.y;

		*rectInfo.property->value.vec4Value = childRect;
		offset.y += rectInfo.rect.y + childRect.w + spacing.GetValue().y;
	}
	verticalSize.SetReadOnly( false );
	verticalSize = offset.y;
	verticalSize.SetReadOnly( true );
}

SD_UI_IMPLEMENT_CLASS( sdUILayout_Horizontal, sdUILayout_StaticBase )

/*
============
sdUILayout_Horizontal::DoApplyLayout
============
*/
void sdUILayout_Horizontal::DoApplyLayout() {
	idVec2 offset( margins.GetValue().x, margins.GetValue().y );

	for( int i = 0; i < childRects.Num(); i++ ) {
		rectInfo_t& rectInfo = childRects[ i ];

		idVec4 childRect = rectInfo.rect;
		if( TestFlag( VLF_NOSIZE ) == false && rectInfo.source->TestFlag( OF_FIXED_LAYOUT ) == false ) {
			childRect.w = rect.GetValue().w - ( margins.GetValue().y + margins.GetValue().w ) - childRect.y;
		}

		childRect.x += offset.x;
		childRect.y += offset.y;

		*rectInfo.property->value.vec4Value = childRect;
		offset.x += rectInfo.rect.x + childRect.z + spacing.GetValue().x;
	}
}

