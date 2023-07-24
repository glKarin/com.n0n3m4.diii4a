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
#include "UIEditW.h"


//===============================================================
//
//	sdUIEditW
//
//===============================================================

SD_UI_IMPLEMENT_CLASS( sdUIEditW, sdUIWindow )

SD_UI_PUSH_CLASS_TAG( sdUIEditW )
const char* sdUIEditW::eventNames[ EE_NUM_EVENTS - WE_NUM_EVENTS ] = {
	SD_UI_EVENT_TAG( "onInputFailed",		"", "Called if typing any characters that breaks the EF_ALLOW_DECIMAL/EF_INTEGERS_ONLY flags." ),
};
SD_UI_POP_CLASS_TAG

idHashMap< sdUITemplateFunction< sdUIEditW >* > sdUIEditW::editFunctions;
const char sdUITemplateFunctionInstance_IdentifierEditW[] = "sdUIEditWFunction";

/*
============
sdUIEditW::sdUIEditW
============
*/
sdUIEditW::sdUIEditW() {
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

	scriptState.GetProperties().RegisterProperty( "IMEFillColor", IMEFillColor );
	scriptState.GetProperties().RegisterProperty( "IMEBorderColor", IMEBorderColor );
	scriptState.GetProperties().RegisterProperty( "IMESelectionColor", IMESelectionColor );
	scriptState.GetProperties().RegisterProperty( "IMETextColor", IMETextColor );

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

	UI_ADD_WSTR_CALLBACK( editText, sdUIEditW, OnEditTextChanged )
	UI_ADD_FLOAT_CALLBACK( readOnly, sdUIEditW, OnReadOnlyChanged )
	UI_ADD_FLOAT_CALLBACK( flags, sdUIEditW, OnFlagsChanged )
	UI_ADD_STR_CALLBACK( cursorName, sdUIEditW, OnCursorNameChanged )
	UI_ADD_STR_CALLBACK( overwriteCursorName, sdUIEditW, OnOverwriteCursorNameChanged )

	SetWindowFlag( WF_ALLOW_FOCUS );
	SetWindowFlag( WF_CLIP_TO_RECT );

	drawIME = false;
	composing = false;
}

/*
============
sdUIEditW::InitProperties
============
*/
void sdUIEditW::InitProperties() {
	sdUIWindow::InitProperties();
	cursorName			= "editcursor";
	overwriteCursorName = "editcursor_overwrite";
}

/*
============
sdUIEditW::~sdUIEditW
============
*/
sdUIEditW::~sdUIEditW() {
	DisconnectGlobalCallbacks();
}

/*
============
sdUIEditW::FindFunction
============
*/
const sdUITemplateFunction< sdUIEditW >* sdUIEditW::FindFunction( const char* name ) {
	sdUITemplateFunction< sdUIEditW >** ptr;
	return editFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUIEditW::GetFunction
============
*/
sdUIFunctionInstance* sdUIEditW::GetFunction( const char* name ) {
	const sdUITemplateFunction< sdUIEditW >* function = sdUIEditW::FindFunction( name );
	if ( !function ) {		
		return sdUIWindow::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUIEditW, sdUITemplateFunctionInstance_IdentifierEditW >( this, function );
}

/*
============
sdUIEditW::RunNamedMethod
============
*/
bool sdUIEditW::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const sdUITemplateFunction< sdUIEditW >* func = sdUIEditW::FindFunction( name );
	if ( !func ) {
		return sdUIWindow::RunNamedMethod( name, stack );
	}

	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}

/*
============
sdUIEditW::DrawLocal
============
*/
void sdUIEditW::DrawLocal() {
	if ( PreDraw() ) {
		DrawBackground( cachedClientRect );

		deviceContext->PushClipRect( GetDrawRect() );

		helper.DrawText( foreColor );
		helper.DrawLocal();

		deviceContext->PopClipRect();

		// border
		if ( borderWidth > 0.0f ) {
			deviceContext->DrawClippedBox( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w, borderWidth, borderColor );
		}

		// Can draw the IME elements
		drawIME = true;
	}

	PostDraw();	
}

/*
============
sdUIEditW::DrawLocal
============
*/
void sdUIEditW::FinalDraw() {
	if ( drawIME ) {
		drawIME = false;

		// Render the IME elements
		if ( sys->IME().LangSupportsIME() && GetUI()->IsFocused( this ) ) {
			// Draw the input locale indicator
			DrawIndicator();

			// Display the composition string
			//DrawComposition();

			if ( sys->IME().IsReadingWindowActive() ) {
				// Reading window
				DrawCandidateReadingWindow( true );
			} else if ( sys->IME().IsCandidateListActive() ) {
				DrawCandidateReadingWindow( false );
			}
		}
	}

	sdUIWindow::FinalDraw();
}

/*
============
sdUIEditW::PostEvent
============
*/
bool sdUIEditW::PostEvent( const sdSysEvent* event ) {
	if ( !IsVisible() || !ParentsAllowEventPosting() ) {
		return false;
	}

	bool retVal = sdUIWindow::PostEvent( event );

	if ( event->IsIMEEvent() ) {
		switch ( event->GetIMEEvent() ) {
			case IMEV_COMPOSITION_START:
				if ( !readOnly ) {
					// enable composition mode
					composing = true;
					helper.QueueEvent( sdUIEditHelper< sdUIEditW, idWStr, wchar_t >::sdUIEditEvent::EE_COMPOSITION_CURSOR_SET );
					helper.InsertText( L"" );	// makes sure that the current selection gets cleared out
					compositionString.Clear();
					return true;
				}
				break;
			case IMEV_COMPOSITION_END:
				composing = false;
				compositionString.Clear();
				return true;
			case IMEV_COMPOSITION_UPDATE:
				if ( composing ) {
					// schedule a recalculation of text wrapping etc
					compositionString = event->GetCompositionString();
					MakeLayoutDirty();
					return true;
				}
				break;
			case IMEV_COMPOSITION_COMMIT:
				if ( !readOnly ) {
					int len = idWStr::Length( event->GetCompositionString() );

					size_t dataSize = ( len + 1 ) * sizeof( wchar_t );
					void* data = Mem_Alloc( dataSize );
					::memcpy( data, event->GetCompositionString(), dataSize );
					helper.QueueEvent( sdUIEditHelper< sdUIEditW, idWStr, wchar_t >::sdUIEditEvent::EE_COMPOSITION_COMMIT, 0, 0, 0, 0, 0, dataSize, data );
					return true;
				}
				break;
		}
	}

	return helper.PostEvent( retVal, event );
}

/*
============
sdUIEditW::Script_ClearText
============
*/
void sdUIEditW::Script_ClearText( sdUIFunctionStack& stack ) {
	ClearText();
}

/*
============
sdUIEditW::Script_ClearText
============
*/
const wchar_t* sdUIEditW::GetCursorText() const {
	return ( compositionString.c_str() );
}

/*
============
sdUIEditW::InitFunctions
============
*/
#pragma inline_depth( 0 )
#pragma optimize( "", off )
SD_UI_PUSH_CLASS_TAG( sdUIEditW )
void sdUIEditW::InitFunctions() {
	SD_UI_FUNC_TAG( clearText, "Clear all window text." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "clearText", new sdUITemplateFunction< sdUIEditW >( 'v', "", &sdUIEditW::Script_ClearText ) );

	SD_UI_FUNC_TAG( isWhitespace, "Check whitespaces, ignores all color codes." )
		SD_UI_FUNC_RETURN_PARM( float, "True if it is empty or contain only whitespaces." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "isWhitespace", new sdUITemplateFunction< sdUIEditW >( 'f', "", &sdUIEditW::Script_IsWhitespace ) );

	SD_UI_FUNC_TAG( insertText, "Insert text into the edit box." )
		SD_UI_FUNC_PARM( wstring, "text", "Text to insert." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "insertText", new sdUITemplateFunction< sdUIEditW >( 'v', "w", &sdUIEditW::Script_InsertText ) );

	SD_UI_FUNC_TAG( surroundSelection, "Select certain substring." )
		SD_UI_FUNC_PARM( wstring, "prefix", "Prefix to selection text." )
		SD_UI_FUNC_PARM( wstring, "suffix", "Suffix to selection text." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "surroundSelection", new sdUITemplateFunction< sdUIEditW >( 'v', "ww", &sdUIEditW::Script_SurroundSelection ) );

	SD_UI_FUNC_TAG( anySelected, "Check if any of the edit text is selected." )
		SD_UI_FUNC_RETURN_PARM( float, "True if any text has been selected." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "anySelected", new sdUITemplateFunction< sdUIEditW >( 'f', "", &sdUIEditW::Script_AnySelected ) );

	SD_UI_FUNC_TAG( selectAll, "Select all text in the edit box." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "selectAll", new sdUITemplateFunction< sdUIEditW >( 'v', "", &sdUIEditW::Script_SelectAll ) );

	SD_UI_PUSH_GROUP_TAG( "Edit Flags" )

	SD_UI_ENUM_TAG( EF_INTEGERS_ONLY, "Accept only integer numbers in edit box." )
	sdDeclGUI::AddDefine( va( "EF_INTEGERS_ONLY %d", EF_INTEGERS_ONLY ) );
	SD_UI_ENUM_TAG( EF_ALLOW_DECIMAL, "Allow decimal numbers to be typed." )
	sdDeclGUI::AddDefine( va( "EF_ALLOW_DECIMAL %d", EF_ALLOW_DECIMAL ) );

	SD_UI_POP_GROUP_TAG
}
SD_UI_POP_CLASS_TAG
#pragma optimize( "", on )
#pragma inline_depth()

/*
============
sdUIEditW::ApplyLayout
============
*/
void sdUIEditW::ApplyLayout() {
	bool needLayout = windowState.recalculateLayout;
	sdUIWindow::ApplyLayout();
	helper.ApplyLayout();
	if ( needLayout ) {
		helper.TextChanged();
	}
}

/*
============
sdUIEditW::ClearText
============
*/
void sdUIEditW::ClearText() {
	helper.ClearText();
	editText = L"";
}

/*
============
sdUIEditW::OnEditTextChanged
============
*/
void sdUIEditW::OnEditTextChanged( const idWStr& oldValue, const idWStr& newValue ) {
	MakeLayoutDirty();
	helper.MakeTextDirty();
}

/*
============
sdUIEditW::OnCursorNameChanged
============
*/
void sdUIEditW::OnCursorNameChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->LookupMaterial( newValue, cursor.mi, &cursor.width, &cursor.height );
}

/*
============
sdUIEditW::OnOverwriteCursorNameChanged
============
*/
void sdUIEditW::OnOverwriteCursorNameChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->LookupMaterial( newValue, overwriteCursor.mi, &overwriteCursor.width, &overwriteCursor.height );
}

/*
============
sdUIEditW::EndLevelLoad
============
*/
void sdUIEditW::EndLevelLoad() {
	sdUserInterfaceLocal::SetupMaterialInfo( cursor.mi, &cursor.width, &cursor.height );
	sdUserInterfaceLocal::SetupMaterialInfo( overwriteCursor.mi, &overwriteCursor.width, &overwriteCursor.height );
}

/*
============
sdUIEditW::EnumerateEvents
============
*/
void sdUIEditW::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	if ( !idStr::Icmp( name, "onInputFailed" ) ) {
		events.Append( sdUIEventInfo( EE_INPUT_FAILED, 0 ) );
		return;
	}
	sdUIWindow::EnumerateEvents( name, flags, events, tokenCache );
}

/*
============
sdUIEditW::OnReadOnlyChanged
============
*/
void sdUIEditW::OnReadOnlyChanged( const float oldValue, const float newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIEditW::OnFlagsChanged
============
*/
void sdUIEditW::OnFlagsChanged( const float oldValue, const float newValue ) {
	if ( FlagChanged( oldValue, newValue, WF_COLOR_ESCAPES ) ) {
		MakeLayoutDirty();
	}
}

/*
============
sdUIEditW::Script_IsWhitespace
============
*/
void sdUIEditW::Script_IsWhitespace( sdUIFunctionStack& stack ) {
	for( int i = 0; i < editText.GetValue().Length(); i++ ) {
		const wchar_t& c = editText.GetValue()[ i ];
		if( idWStr::IsColor( &c ) ) {
			i++;
			continue;
		}
		
		if( c != L'\n' && c != L' ' ) {
			stack.Push( false );
			return;
		}
	}
	stack.Push( true );
}

/*
============
sdUIEditW::OnInputInit
============
*/
void sdUIEditW::OnLanguageInit() {
	if ( GetUI()->IsFocused( this ) ) {
		if ( sys->IME().LangSupportsIME() ) {
			sys->IME().Enable( true );
		}
	}

	sdUIWindow::OnLanguageInit();
}

/*
============
sdUIEditW::OnInputShutdown
============
*/
void sdUIEditW::OnLanguageShutdown() {
	if ( GetUI()->IsFocused( this ) ) {
		if ( sys->IME().LangSupportsIME() ) {
			sys->IME().FinalizeString();
			sys->IME().Enable( false );
		}
	}

	sdUIWindow::OnLanguageShutdown();
}


/*
============
sdUIEditW::OnGainFocus
============
*/
void sdUIEditW::OnGainFocus( void ) {
	sdUIWindow::OnGainFocus();
	helper.OnGainFocus();
	if ( sys->IME().LangSupportsIME() ) {
		sys->IME().Enable( true );
	}
}

/*
============
sdUIEditW::OnLoseFocus
============
*/
void sdUIEditW::OnLoseFocus( void ) {
	sdUIWindow::OnLoseFocus();
	if ( sys->IME().LangSupportsIME() ) {
		sys->IME().FinalizeString();
		sys->IME().Enable( false );
	}
}

/*
============
sdUIEditW::DrawIndicator
============
*/
void sdUIEditW::DrawIndicator() {
	// If IME system is off, draw English indicator
	const wchar_t* indicator = sys->IME().GetState() == sdIME::IME_STATE_ON ? sys->IME().GetIndicator() : L"A";

	ActivateFont();

	int fontHeight = deviceContext->GetFontHeight( cachedFontHandle, fontSize );

#if 1
	int heightRequired;

	if ( sys->IME().VerticalCandidateLine() ) {
		heightRequired = ( fontHeight * sdIME::MAX_CANDLIST ) + 2;
	} else {
		heightRequired = fontHeight + 2;
	}

	sdBounds2D rect( cachedClientRect.x,
						cachedClientRect.y + cachedClientRect.w,
						fontHeight + 2,
						fontHeight + 2 );

	sdUIWindow* desktop = GetUI()->GetDesktop();
	if ( desktop != NULL ) {
		if ( rect.GetMins().y + heightRequired > desktop->GetWorldRect().w ) {
			float height = rect.GetHeight();
			rect.GetMins().y = cachedClientRect.y - height;
			rect.GetMaxs().y = rect.GetMins().y + height;
		}
	} else {
		if ( rect.GetMins().y + heightRequired > SCREEN_HEIGHT ) {
			float height = rect.GetHeight();
			rect.GetMins().y = cachedClientRect.y - height;
			rect.GetMaxs().y = rect.GetMins().y + height;
		}
	}

	sdBounds2D textRect( rect.GetMins().x + 1,
						rect.GetMins().y + 1,
						fontHeight,
						fontHeight );
#else
	sdBounds2D textRect( cachedClientRect.x + cachedClientRect.z + 1,
						cachedClientRect.y + cachedClientRect.w,
						fontHeight,
						fontHeight );
	sdBounds2D rect( cachedClientRect.x + cachedClientRect.z,
						cachedClientRect.y + cachedClientRect.w,
						fontHeight + 2,
						fontHeight + 2 );
#endif

	deviceContext->DrawRect( rect.GetMins().x, rect.GetMins().y, rect.GetWidth(), rect.GetHeight(), IMEFillColor );
	deviceContext->DrawBox( rect.GetMins().x, rect.GetMins().y, rect.GetWidth(), rect.GetHeight(), 1.0f, IMEBorderColor );

	deviceContext->SetFontSize( fontSize );

	deviceContext->SetColor( IMETextColor );
	deviceContext->DrawText( indicator, textRect, DTF_CENTER | DTF_SINGLELINE | DTF_BOTTOM | DTF_NOCLIPPING );								
}

#if 0
/*
============
sdUIEditW::DrawComposition
============
*/
void sdUIEditW::DrawComposition() {
	const wchar_t* compStr = sys->IME().GetCompositionString();

	ActivateFont();
	deviceContext->SetFontSize( fontSize );

	int textWidth, textHeight;
	sdBounds2D screenBounds( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT );

	deviceContext->GetTextDimensions( compStr, screenBounds, DTF_LEFT | DTF_SINGLELINE | DTF_BOTTOM | DTF_NOCLIPPING, cachedFontHandle, fontSize, textWidth, textHeight );

	// Draw the window and string
	sdBounds2D rect( cachedClientRect.x,
						cachedClientRect.y + cachedClientRect.w,
						textWidth,
						textHeight );//2 * cachedClientRect.w );

	if ( false /*sys->IME().GetPrimaryLanguage() == sdIME::IME_LANG_KOREAN*/ ) {

	} else {
		deviceContext->SetColor( idVec4( 1.0f, 1.0f, 1.0f, 0.3f ) );
		deviceContext->DrawRect( rect.GetMins().x, rect.GetMins().y, rect.GetWidth(), rect.GetHeight(), 0.0f, 0.0f, 1.0f, 1.0f, declHolder.FindMaterial( "_whiteVertexColor" ) );
	}

	deviceContext->SetColor( colorWhite );
	deviceContext->DrawText( compStr, rect, DTF_LEFT | DTF_SINGLELINE | DTF_BOTTOM | DTF_NOCLIPPING );		
}
#endif

/*
============
sdUIEditW::DrawCandidateReadingWindow
============
*/
void sdUIEditW::DrawCandidateReadingWindow( bool reading ) {
	int numEntries = reading ? 4 : sdIME::MAX_CANDLIST;

	ActivateFont();
	deviceContext->SetFontSize( fontSize );

	int textWidth, textHeight;
	sdBounds2D screenBounds( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT );

	int widthRequired = 0;
	int lineHeight = 0;
	int heightRequired = 0;

	wchar_t candidateString[ sdIME::MAX_CANDIDATE_LENGTH + 4 ];
	candidateString[ 0 ] = L'\0';
	int firstSelected = 0;
	int horizontalSelectedLen = 0;

	int fontHeight = deviceContext->GetFontHeight( cachedFontHandle, fontSize );

	if ( ( sys->IME().VerticalCandidateLine() && !reading ) ||
		( !sys->IME().IsHorizontalReading() && reading ) ) {
		// vertical window
		for ( int i = 0; i < numEntries; i++ ) {
			if ( *( sys->IME().GetCandidate( i ) ) == L'\0' ) {
				numEntries = i;
				break;
			}

			deviceContext->GetTextDimensions( sys->IME().GetCandidate( i ), screenBounds, DTF_LEFT | DTF_SINGLELINE | DTF_BOTTOM | DTF_NOCLIPPING, cachedFontHandle, fontSize, textWidth, textHeight );

			widthRequired = Max( widthRequired, textWidth );
			lineHeight = fontHeight;//Max( lineHeight, textHeight );
		}
#if 0
		heightRequired = lineHeight * numEntries;
#else
		heightRequired = lineHeight * sdIME::MAX_CANDLIST;
#endif
	} else {
		// horizontal window

		if ( reading ) {
			//???
		} else {
			idWStr::Append( candidateString, sdIME::MAX_CANDIDATE_LENGTH + 2, L"\x00ab" );

			for ( int i = 0; i < sdIME::MAX_CANDLIST; i++ ) {
				if ( *( sys->IME().GetCandidate( i ) ) == L'\0' ) {
					break;
				}

				wchar_t* entry = va( L"%ls", sys->IME().GetCandidate( i ) );

				// If this is the selected entry, mark its character position
				if ( sys->IME().GetCandidateSelection() == i ) {
					firstSelected = idWStr::Length( candidateString );
					horizontalSelectedLen = idWStr::Length( entry );
				}

				idWStr::Append( candidateString, sdIME::MAX_CANDIDATE_LENGTH + 4, entry );
			}

			idWStr::Append( candidateString, sdIME::MAX_CANDIDATE_LENGTH + 2, L"\x00bb" );

			deviceContext->GetTextDimensions( candidateString, screenBounds, DTF_LEFT | DTF_SINGLELINE | DTF_BOTTOM | DTF_NOCLIPPING, cachedFontHandle, fontSize, textWidth, textHeight );
		}

		widthRequired = textWidth;
		lineHeight = heightRequired = fontHeight;//textHeight;
	}

	// Draw the elements

#if 0
	sdUIWindow* desktop = GetUI()->GetDesktop();
	float textX, textY, rectX, rectY;

	if ( desktop != NULL ) {
		textX = ( desktop->GetWorldRect().z - ( widthRequired + 2 ) ) + 1;
		textY = ( desktop->GetWorldRect().w - ( heightRequired + 2 ) ) + 1;
		rectX = desktop->GetWorldRect().z - ( widthRequired + 2 );
		rectY = desktop->GetWorldRect().w - ( heightRequired + 2 );
	} else {
		textX = ( SCREEN_WIDTH - ( widthRequired + 2 ) ) + 1;
		textY = ( SCREEN_HEIGHT - ( heightRequired + 2 ) ) + 1;
		rectX = SCREEN_WIDTH - ( widthRequired + 2 );
		rectY = SCREEN_HEIGHT - ( heightRequired + 2 );
	}

	sdBounds2D textRect( textX,
						textY,
						widthRequired,
						heightRequired );
	sdBounds2D rect( rectX,
						rectY,
						widthRequired + 2,
						heightRequired + 2 );
#else
	sdBounds2D rect( cachedClientRect.x + fontHeight + 2,
						cachedClientRect.y + cachedClientRect.w,
						widthRequired + 2,
						heightRequired + 2 );

	sdUIWindow* desktop = GetUI()->GetDesktop();
	if ( desktop != NULL ) {
		if ( rect.GetMins().y + rect.GetHeight() > desktop->GetWorldRect().w ) {
			float height = rect.GetHeight();
			rect.GetMins().y = cachedClientRect.y - height;
			rect.GetMaxs().y = rect.GetMins().y + height;
		}
	} else {
		if ( rect.GetMins().y + rect.GetHeight() > SCREEN_HEIGHT ) {
			float height = rect.GetHeight();
			rect.GetMins().y = cachedClientRect.y - height;
			rect.GetMaxs().y = rect.GetMins().y + height;
		}
	}
#endif

	if ( ( sys->IME().VerticalCandidateLine() && !reading ) ||
		( !sys->IME().IsHorizontalReading() && reading ) ) {
		// Vertical candidate window
		deviceContext->DrawRect( rect.GetMins().x, rect.GetMins().y, rect.GetWidth(), rect.GetHeight(), IMEFillColor );
		deviceContext->DrawBox( rect.GetMins().x, rect.GetMins().y, rect.GetWidth(), rect.GetHeight(), 1.0f, IMEBorderColor );

		for ( int i = 0; i < numEntries; i++ ) {
			sdBounds2D textRect( rect.GetMins().x + 1,
								rect.GetMins().y + i * lineHeight + 1,
								widthRequired,
								lineHeight );

			if ( sys->IME().GetCandidateSelection() == i ) {
				deviceContext->DrawRect( textRect.GetMins().x, textRect.GetMins().y, textRect.GetWidth(), textRect.GetHeight(), IMESelectionColor );
			}

			deviceContext->SetColor( IMETextColor );
			deviceContext->DrawText( sys->IME().GetCandidate( i ), textRect, DTF_LEFT | DTF_SINGLELINE | DTF_BOTTOM | DTF_NOCLIPPING );
		}
	} else {
		// Horizontal candidate window
		sdBounds2D textRect( rect.GetMins().x + 1,
							rect.GetMins().y + 1,
							widthRequired,
							heightRequired );

		deviceContext->DrawRect( rect.GetMins().x, rect.GetMins().y, rect.GetWidth(), rect.GetHeight(), IMEFillColor );
		deviceContext->DrawBox( rect.GetMins().x, rect.GetMins().y, rect.GetWidth(), rect.GetHeight(), 1.0f, IMEBorderColor );

		if ( reading ) {

		} else {
			if ( horizontalSelectedLen > 0 ) {
				// Draw selection
				sdTextDimensionHelper tdh;
				tdh.Init( candidateString, idWStr::Length( candidateString ), textRect, DTF_LEFT | DTF_SINGLELINE | DTF_BOTTOM | DTF_NOCLIPPING, cachedFontHandle, fontSize );

				idVec2 drawBegin;
				drawBegin.x = textRect.GetMins().x + tdh.GetWidth( 0, firstSelected - 1 );
				drawBegin.y = textRect.GetMins().y;

				idVec2 drawEnd;
				drawEnd.x = drawBegin.x + tdh.GetWidth( firstSelected, firstSelected + horizontalSelectedLen - 1 );
				drawEnd.y = textRect.GetMins().y;
				
				deviceContext->DrawRect( drawBegin.x, drawBegin.y, drawEnd.x - drawBegin.x, textRect.GetHeight(), IMESelectionColor );
			}

			// Draw text
			deviceContext->SetColor( IMETextColor );
			deviceContext->DrawText( candidateString, textRect, DTF_LEFT | DTF_SINGLELINE | DTF_BOTTOM | DTF_NOCLIPPING );
		}
	}
}

/*
============
sdUIEditW::Script_InsertText
============
*/
void sdUIEditW::Script_InsertText( sdUIFunctionStack& stack ) {
	idWStr text;
	stack.Pop( text );
	helper.InsertText( text.c_str() );
}

/*
============
sdUIEditW::Script_SurroundSelection
============
*/
void sdUIEditW::Script_SurroundSelection( sdUIFunctionStack& stack ) {
	idWStr prefix;
	stack.Pop( prefix );

	idWStr suffix;
	stack.Pop( suffix );

	helper.SurroundSelection( prefix.c_str(), suffix.c_str() );
}

/*
============
sdUIEditW::Script_AnySelected
============
*/
void sdUIEditW::Script_AnySelected( sdUIFunctionStack& stack ) {
	int selStart;
	int selEnd;
	helper.GetSelectionRange( selStart, selEnd );
	stack.Push( selStart != selEnd );
}


/*
============
sdUIEditW::Script_SelectAll
============
*/
void sdUIEditW::Script_SelectAll( sdUIFunctionStack& stack ) {
	helper.SelectAll();
}
