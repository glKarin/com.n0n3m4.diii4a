// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../../framework/KeyInput.h"
#include "../../sys/sys_local.h"

#include "UIWindow.h"
#include "UserInterfaceLocal.h"
#include "UIEdit.h"

//===============================================================
//
//	sdUIEdit
//
//===============================================================

SD_UI_IMPLEMENT_CLASS( sdUIEdit, sdUIWindow )

idHashMap< sdUITemplateFunction< sdUIEdit >* > sdUIEdit::editFunctions;
const char sdUITemplateFunctionInstance_IdentifierEdit[] = "sdUIEditFunction";

SD_UI_PUSH_CLASS_TAG( sdUIEdit )
const char* sdUIEdit::eventNames[ EE_NUM_EVENTS - WE_NUM_EVENTS ] = {
	SD_UI_EVENT_TAG( "onInputFailed",		"", "Called if typing any characters that breaks the EF_ALLOW_DECIMAL/EF_INTEGERS_ONLY flags." ),
};
SD_UI_POP_CLASS_TAG


/*
============
sdUIEdit::sdUIEdit
============
*/
sdUIEdit::sdUIEdit() {
	helper.Init( this );

	scriptState.GetProperties().RegisterProperty( "editText", editText );
	scriptState.GetProperties().RegisterProperty( "scrollAmount", scrollAmount );
	scriptState.GetProperties().RegisterProperty( "scrollAmountMax", scrollAmountMax );
	scriptState.GetProperties().RegisterProperty( "lineHeight", lineHeight );

	scriptState.GetProperties().RegisterProperty( "maxTextLength", maxTextLength );
	scriptState.GetProperties().RegisterProperty( "readOnly", readOnly );
	scriptState.GetProperties().RegisterProperty( "password", password );

	scriptState.GetProperties().RegisterProperty( "cursor", cursorName );
	scriptState.GetProperties().RegisterProperty( "overwriteCursor", overwriteCursorName );

	textAlignment.SetIndex( 0, TA_LEFT );
	maxTextLength = 0;
	readOnly = 0.0f;
	password = 0.0f;
	textOffset = idVec2( 2.0f, 0.0f );

	scrollAmount = vec2_zero;
	scrollAmountMax = 0.0f;
	lineHeight = 0.0f;

	scrollAmountMax.SetReadOnly( true );
	lineHeight.SetReadOnly( true );

	UI_ADD_STR_CALLBACK( editText, sdUIEdit, OnEditTextChanged )
	UI_ADD_FLOAT_CALLBACK( readOnly, sdUIEdit, OnReadOnlyChanged )
	UI_ADD_FLOAT_CALLBACK( flags, sdUIEdit, OnFlagsChanged )
	UI_ADD_STR_CALLBACK( cursorName, sdUIEdit, OnCursorNameChanged )
	UI_ADD_STR_CALLBACK( overwriteCursorName, sdUIEdit, OnOverwriteCursorNameChanged )

	SetWindowFlag( WF_ALLOW_FOCUS );
}

/*
============
sdUIEdit::InitProperties
============
*/
void sdUIEdit::InitProperties() {
	sdUIWindow::InitProperties();
	cursorName			= "editcursor";
	overwriteCursorName = "editcursor_overwrite";
}

/*
============
sdUIEdit::~sdUIEdit
============
*/
sdUIEdit::~sdUIEdit() {
	DisconnectGlobalCallbacks();
}

/*
============
sdUIEdit::FindFunction
============
*/
const sdUITemplateFunction< sdUIEdit >* sdUIEdit::FindFunction( const char* name ) {
	sdUITemplateFunction< sdUIEdit >** ptr;
	return editFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUIEdit::GetFunction
============
*/
sdUIFunctionInstance* sdUIEdit::GetFunction( const char* name ) {
	const sdUITemplateFunction< sdUIEdit >* function = sdUIEdit::FindFunction( name );
	if ( !function ) {		
		return sdUIWindow::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUIEdit, sdUITemplateFunctionInstance_IdentifierEdit >( this, function );
}

/*
============
sdUIEdit::RunNamedMethod
============
*/
bool sdUIEdit::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const sdUITemplateFunction< sdUIEdit >* func = sdUIEdit::FindFunction( name );
	if ( !func ) {
		return sdUIWindow::RunNamedMethod( name, stack );
	}

	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}

/*
============
sdUIEdit::DrawLocal
============
*/
void sdUIEdit::DrawLocal() {
	if( PreDraw() ) {
		DrawBackground( cachedClientRect );
		
		deviceContext->PushClipRect( GetDrawRect() );

		helper.DrawText( foreColor );
		helper.DrawLocal();
		
		deviceContext->PopClipRect();

		// border
		if ( borderWidth > 0.0f ) {
			deviceContext->DrawClippedBox( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w, borderWidth, borderColor );
		}
	}

	PostDraw();		
}

/*
============
sdUIEdit::PostEvent
============
*/
bool sdUIEdit::PostEvent( const sdSysEvent* event ) {
	if ( !IsVisible() || !ParentsAllowEventPosting() ) {
		return false;
	}

	bool retVal = sdUIWindow::PostEvent( event );

	return helper.PostEvent( retVal, event );
}

/*
============
sdUIEdit::Script_ClearText
============
*/
void sdUIEdit::Script_ClearText( sdUIFunctionStack& stack ) {
	ClearText();	
}

/*
============
sdUIEdit::InitFunctions
============
*/
#pragma inline_depth( 0 )
#pragma optimize( "", off )
SD_UI_PUSH_CLASS_TAG( sdUIEdit )
void sdUIEdit::InitFunctions() {
	SD_UI_FUNC_TAG( clearText, "Clear all window text." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "clearText", new sdUITemplateFunction< sdUIEdit >( 'v', "", &sdUIEdit::Script_ClearText ) );

	SD_UI_FUNC_TAG( isWhitespace, "Check whitespaces, ignores all color codes." )
		SD_UI_FUNC_RETURN_PARM( float, "True if it is empty or contain only whitespaces." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "isWhitespace", new sdUITemplateFunction< sdUIEdit >( 'f', "", &sdUIEdit::Script_IsWhitespace ) );

	SD_UI_FUNC_TAG( insertText, "Insert text into the edit box." )
		SD_UI_FUNC_PARM( string, "text", "Text to insert." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "insertText", new sdUITemplateFunction< sdUIEdit >( 'v', "s", &sdUIEdit::Script_InsertText ) );

	SD_UI_FUNC_TAG( surroundSelection, "Select certain substring." )
		SD_UI_FUNC_PARM( string, "prefix", "Prefix to selection text." )
		SD_UI_FUNC_PARM( string, "suffix", "Suffix to selection text." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "surroundSelection", new sdUITemplateFunction< sdUIEdit >( 'v', "ss", &sdUIEdit::Script_SurroundSelection ) );

	SD_UI_FUNC_TAG( anySelected, "Check if any of the edit text is selected." )
		SD_UI_FUNC_RETURN_PARM( float, "True if any text has been selected." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "anySelected", new sdUITemplateFunction< sdUIEdit >( 'f', "", &sdUIEdit::Script_AnySelected ) );

	SD_UI_FUNC_TAG( selectAll, "Select all text in the edit box." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "selectAll", new sdUITemplateFunction< sdUIEdit >( 'v', "", &sdUIEdit::Script_SelectAll ) );


	SD_UI_PUSH_GROUP_TAG( "Edit Flags" )

	SD_UI_ENUM_TAG( EF_INTEGERS_ONLY, "Accept only integer numbers in edit box." )
	sdDeclGUI::AddDefine( va( "EF_INTEGERS_ONLY %d", EF_INTEGERS_ONLY ) );
	SD_UI_ENUM_TAG( EF_ALLOW_DECIMAL, "Allow decimal numbers to be typed." )
	sdDeclGUI::AddDefine( va( "EF_ALLOW_DECIMAL %d", EF_ALLOW_DECIMAL ) );
	SD_UI_ENUM_TAG( EF_UPPERCASE, "Convert all characters to upper case." )
	sdDeclGUI::AddDefine( va( "EF_UPPERCASE %d", EF_UPPERCASE ) );
	SD_UI_ENUM_TAG( EF_MULTILINE, "Allow a multi line edit box." )
	sdDeclGUI::AddDefine( va( "EF_MULTILINE %d", EF_MULTILINE ) );

	SD_UI_POP_GROUP_TAG
}
SD_UI_POP_CLASS_TAG
#pragma optimize( "", on )
#pragma inline_depth()

/*
============
sdUIEdit::ApplyLayout
============
*/
void sdUIEdit::ApplyLayout() {
	bool needLayout = windowState.recalculateLayout;
	sdUIWindow::ApplyLayout();
	helper.ApplyLayout();
	if ( needLayout ) {
		helper.TextChanged();
	}
}

/*
============
sdUIEdit::ClearText
============
*/
void sdUIEdit::ClearText() {
	helper.ClearText();
	editText = "";
}

/*
============
sdUIEdit::OnEditTextChanged
============
*/
void sdUIEdit::OnEditTextChanged( const idStr& oldValue, const idStr& newValue ) {
	MakeLayoutDirty();
	helper.MakeTextDirty();
}

/*
============
sdUIEdit::OnCursorNameChanged
============
*/
void sdUIEdit::OnCursorNameChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->LookupMaterial( newValue, cursor.mi, &cursor.width, &cursor.height );
}

/*
============
sdUIEdit::OnOverwriteCursorNameChanged
============
*/
void sdUIEdit::OnOverwriteCursorNameChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->LookupMaterial( newValue, overwriteCursor.mi, &overwriteCursor.width, &overwriteCursor.height );
}

/*
============
sdUIEdit::EndLevelLoad
============
*/
void sdUIEdit::EndLevelLoad() {
	sdUserInterfaceLocal::SetupMaterialInfo( cursor.mi, &cursor.width, &cursor.height );
	sdUserInterfaceLocal::SetupMaterialInfo( overwriteCursor.mi, &overwriteCursor.width, &overwriteCursor.height );
}

/*
============
sdUIEdit::EnumerateEvents
============
*/
void sdUIEdit::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	if ( !idStr::Icmp( name, "onInputFailed" ) ) {
		events.Append( sdUIEventInfo( EE_INPUT_FAILED, 0 ) );
		return;
	}
	sdUIWindow::EnumerateEvents( name, flags, events, tokenCache );
}

/*
============
sdUIEdit::OnReadOnlyChanged
============
*/
void sdUIEdit::OnReadOnlyChanged( const float oldValue, const float newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIEdit::OnFlagsChanged
============
*/
void sdUIEdit::OnFlagsChanged( const float oldValue, const float newValue ) {
	if( FlagChanged( oldValue, newValue, WF_COLOR_ESCAPES ) ) {
		MakeLayoutDirty();
	}
}

/*
============
sdUIEdit::Script_IsWhitespace
============
*/
void sdUIEdit::Script_IsWhitespace( sdUIFunctionStack& stack ) {
	for( int i = 0; i < editText.GetValue().Length(); i++ ) {
		const char& c = editText.GetValue()[ i ];
		if( idStr::IsColor( &c ) ) {
			i++;
			continue;
		}
		
		if( c != '\n' && c != ' ' ) {
			stack.Push( false );
			return;
		}
	}
	stack.Push( true );
}

/*
============
sdUIEdit::OnGainFocus
============
*/
void sdUIEdit::OnGainFocus( void ) {
	sdUIWindow::OnGainFocus();
	helper.OnGainFocus();
}

/*
============
sdUIEdit::Script_InsertText
============
*/
void sdUIEdit::Script_InsertText( sdUIFunctionStack& stack ) {
	idStr text;
	stack.Pop( text );
	helper.InsertText( text.c_str() );
}

/*
============
sdUIEdit::Script_SurroundSelection
============
*/
void sdUIEdit::Script_SurroundSelection( sdUIFunctionStack& stack ) {
	idStr prefix;
	stack.Pop( prefix );
	
	idStr suffix;
	stack.Pop( suffix );

	helper.SurroundSelection( prefix.c_str(), suffix.c_str() );
}

/*
============
sdUIEdit::Script_AnySelected
============
*/
void sdUIEdit::Script_AnySelected( sdUIFunctionStack& stack ) {
	int selStart;
	int selEnd;
	helper.GetSelectionRange( selStart, selEnd );
	stack.Push( selStart != selEnd );
}


/*
============
sdUIEdit::Script_SelectAll
============
*/
void sdUIEdit::Script_SelectAll( sdUIFunctionStack& stack ) {
	helper.SelectAll();
}