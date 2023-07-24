// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_GUIS_USERINTERFACEEDITHELPER_H__
#define __GAME_GUIS_USERINTERFACEEDITHELPER_H__


template< class StrClass >
class sdClipboardConverter {
public:
	static idWStr ToClipboard( const StrClass& toClipboard ) ;
	static StrClass FromClipboard( const idWStr& clipboardText );
};

template<>
class sdClipboardConverter< idWStr > {
public:
	static idWStr ToClipboard( const idWStr& toClipboard ) {
		return toClipboard;
	}

	static idWStr FromClipboard( const idWStr& clipboardText ) {
		return clipboardText;
	}
};

template<>
class sdClipboardConverter< idStr > {
public:
	static idWStr ToClipboard( const idStr& toClipboard ) {
		return va( L"%hs", toClipboard.c_str() );
	}

	static idStr FromClipboard( const idWStr& clipboardText ) {
		return va( "%ls", clipboardText.c_str() );
	}
};



template< class CharType >
class sdCaseConverter {
public:
	static CharType ToUpper( CharType c );
	static CharType ToLower( CharType c );
};

template<>
class sdCaseConverter< wchar_t > {
public:
	static wchar_t ToUpper( wchar_t c ) { return c; }
	static wchar_t ToLower( wchar_t c ) { return c; }
};

template<>
class sdCaseConverter< char > {
public:
	static char ToUpper( char c ) { return idStr::ToUpper( c ); }
	static char ToLower( char c ) { return idStr::ToLower( c ); }
};

/*
============
sdTextLine
============
*/
class sdTextLine : public sdPoolAllocator< sdTextLine, sdPoolAllocator_DefaultIdentifier, 32 > {	
public:
	typedef idLinkList< sdTextLine > node_t;
					sdTextLine( int start = 0, int end = 0, int cursor = -1 );

	int 			GetCursor() const { return cursor; }
	int 			GetStart() const { return start; }
	int 			GetEnd() const { return end; }

	int 			GetLength() const { return ( end - start ) + 1; }

	int				GetStringIndexForCursor() const;

	bool			HasCursor() const { return cursor >= 0 && cursor <= GetLength(); }

	void			SetCursorOnLine( int cursor );
	sdTextLine*		AdvanceCursor( int advance );

	sdTextLine*		MoveCursorUp();
	sdTextLine*		MoveCursorDown();

	node_t&			GetNode() { return node; }
	const node_t&	GetNode() const { return node; }

private:
	void			ClearCursor() { cursor = -1; }

private:
	// start and end are both inclusive
	// cursor is an offset in the string relative to start
	int 	start;
	int 	end;
	int 	cursor;

	node_t	node;
};


/*
============
sdTextLine::sdTextLine
============
*/
ID_INLINE sdTextLine::sdTextLine( int start, int end, int cursor ) :
	start( start ),
	end( end ),
	cursor( cursor ) {
	node.SetOwner( this );
}


/*
============
sdTextLine::SetCursorOnLine
============
*/
ID_INLINE void sdTextLine::SetCursorOnLine( int cursor ) {
	sdTextLine* line = GetNode().ListHead()->Next();
	while( line != NULL ) {
		line->ClearCursor();
		line = line->GetNode().Next();
	}
	this->cursor = idMath::ClampInt( 0, GetNode().Next() == NULL ? GetLength() - 1 : GetLength(), cursor );
}

/*
============
sdTextLine::AdvanceCursor
============
*/
ID_INLINE sdTextLine* sdTextLine::AdvanceCursor( int advance ) {
	assert( HasCursor() );

	sdTextLine* newLine = this;

	int newCursor = cursor + advance;
	int newTextCursor = start + newCursor;
	if( advance < 0 ) {
		// move to the previous line if necessary
		if( newTextCursor < start ) {
			if( node.Prev() != NULL ) {
				node.Prev()->SetCursorOnLine( node.Prev()->GetLength() );
				if( newTextCursor == start - 1 ) {
					newLine = node.Prev();
				} else {
					newLine = node.Prev()->AdvanceCursor( newTextCursor - start );
				}
			} else {
				SetCursorOnLine( 0 );
			}
		} else {
			SetCursorOnLine( newCursor );
		}
	} else if( advance > 0 ) {
		// move to the next line if necessary
		// catch the case where the move is exactly to the end of the line
		if( newTextCursor > end && newTextCursor != end + 1 ) {
			if( node.Next() != NULL ) {
				node.Next()->SetCursorOnLine( 0 );
				int advanceLocal = newTextCursor - node.Next()->start;
				newLine = node.Next()->AdvanceCursor( advanceLocal );
			} else {
				SetCursorOnLine( GetLength() - 1 );
			}
		} else {
			SetCursorOnLine( newCursor );
		}
	}
	return newLine;
}

/*
============
sdTextLine::MoveCursorUp
============
*/
ID_INLINE sdTextLine* sdTextLine::MoveCursorUp() {
	assert( HasCursor() );

	sdTextLine* prev = GetNode().Prev();
	if( prev == NULL ) {
		return this;
	}

	int cursorOffset = Min( prev->GetLength(), GetCursor() );
	prev->SetCursorOnLine( cursorOffset );

	return prev;	
}

/*
============
sdTextLine::MoveCursorDown
============
*/
ID_INLINE  sdTextLine*	sdTextLine::MoveCursorDown() {
	assert( HasCursor() );

	sdTextLine* next = GetNode().Next();
	if( next == NULL ) {
		return this;
	}

	int cursorOffset = Min( next->GetLength(), GetCursor() );
	next->SetCursorOnLine( cursorOffset );

	return next;
}

/*
============
sdTextLine::GetStringIndexForCursor
============
*/
ID_INLINE int sdTextLine::GetStringIndexForCursor() const {
	assert( HasCursor() );
	return start + cursor;
}

/*
============
sdUIEditHelper
============
*/
template < class UIEditClass, class StrClass, typename CharType >
class sdUIEditHelper {
public:
	class sdUIEditEvent {
	public:
		enum type_e {
			EE_DRAG_MOVE,
			EE_MOUSE,
			EE_SELECT_CANCEL,
			EE_SELECT_ALL,
			EE_MOVE_CURSOR,
			EE_BACKSPACE,
			EE_DELETE,
			EE_UNDO,
			EE_CLEAR,
			EE_COPY,
			EE_CUT,
			EE_PASTE,
			EE_CHAR,
			EE_NEWLINE,
			EE_INSERT_TEXT,
			EE_COMPOSITION_CURSOR_SET,
			EE_COMPOSITION_COMMIT
		};

		enum flags_e {
			EF_SHIFT_PRESSED	=  BITT< 0 >::VALUE,
			EF_CTRL_PRESSED		=  BITT< 1 >::VALUE
		};

										sdUIEditEvent() :
											dataSize( 0 ),
											data( NULL ) {
										}
										~sdUIEditEvent() {
											FreeData();
										}

		void							Init( type_e type, int value = 0, int value2 = 0, int value3 = 0, int value4 = 0, int flags = 0, size_t dataSize = 0, void* data = NULL ) {
											FreeData();

											this->type = type;

											this->value = value;
											this->value2 = value2;
											this->value3 = value3;
											this->value4 = value4;

											this->flags = flags;

											this->dataSize = dataSize;
											this->data = data;

											node.SetOwner( this );
										}

		void							FreeData() {
											Mem_Free( data );
											data = NULL;
											dataSize = 0;
										}

		type_e							GetType() const { return type; }

		idLinkList< sdUIEditEvent >&	GetNode() { return node; }

		// flags
		bool							ShiftPressed() const { return ( flags & EF_SHIFT_PRESSED ) != 0; }
		bool							CtrlPressed() const { return ( flags & EF_CTRL_PRESSED ) != 0; }

		// EE_DRAG_MOVE, EE_MOUSE
		float							GetCursorX() const { return *reinterpret_cast<const float *>( &value3 ); }
		float							GetCursorY() const { return *reinterpret_cast<const float *>( &value4 ); }

		// EE_MOUSE
		mouseButton_t					GetMouseButton() const { return static_cast< mouseButton_t >( value ); }
		bool							IsButtonDown() const { return ( value2 & 0x1 ) != 0; }

		// EE_MOVE_CURSOR
		enum cursoreMoveOp_e {
			MC_HOME,
			MC_END,
			MC_UP,
			MC_DOWN,
			MC_RIGHT,
			MC_LEFT,
			MC_PAGE_UP,
			MC_PAGE_DOWN
		};
		cursoreMoveOp_e					GetCursorMoveOp() const { return static_cast< cursoreMoveOp_e >( value ); }

		// EE_CHAR
		CharType						GetChar( void ) const { return static_cast< CharType >( value ); }

		// EE_INSERT_TEXT, EE_COMPOSITION_COMMIT
		const CharType*					GetText() const { return reinterpret_cast< const CharType* >( data ); }

	protected:
		type_e							type;
		int								value;
		int								value2;
		int								value3;
		int								value4;
		int								flags;

		size_t							dataSize;	// bytes of data pointed to by data
		void*							data;		// this must be manually freed if not NULL

		idLinkList< sdUIEditEvent >		node;
	};

public:
	static const int MAX_TEXT_LENGTH = 1024;		// keep users from causing an allocation failure by pasting an excessive number of characters into an uncapped dialog

							sdUIEditHelper();
							~sdUIEditHelper();

	void					Init( UIEditClass* parent );

	bool					PostEvent( bool retVal, const sdSysEvent* event );
	void					DrawLocal();
	void					DrawText( const idVec4& color );
	void					OnGainFocus();

	void					ApplyLayout();

	void					MakeTextDirty() { textDirty = true; }
	void					TextChanged();
	void					ClearText();

	void					InsertText( const CharType* text );
	void					SurroundSelection( const CharType* prefix, const CharType* suffix );
	bool					GetSelectionRange( int& start, int& end ) const;
	void					SelectAll();

	void					QueueEvent( typename sdUIEditEvent::type_e type, int value = 0, int value2 = 0, int value3 = 0, int value4 = 0, int flags = 0, size_t dataSize = 0, void* data = NULL );

private:
	int						EraseSelection();
	bool					SelectionActive() const { return currentLine->GetStringIndexForCursor() != selectionStart && selectionStart != -1; }
	void					CancelSelection() { selectionStart = -1; }
	void					SaveUndo() { undoText = parent->editText; undoCursorPosition = currentLine->GetStringIndexForCursor(); undoAvailable = true; }
	void					Undo() { if( undoAvailable ) { parent->editText = undoText; currentLine = lines.Next(); currentLine = currentLine->AdvanceCursor( undoCursorPosition ); CursorChanged(); undoCursorPosition = -1; undoAvailable = false; } }

	void					MoveLeft();
	void					MoveRight();

	void					UpdateIndexByColorCodes( int start, int& index );

	int						GetMaxTextLength() { return ( parent->maxTextLength <= 0.0f ) ? MAX_TEXT_LENGTH : idMath::Ftoi( parent->maxTextLength.GetValue() ); }

	void					GetVisibleLines( sdTextLine*& firstVisible, sdTextLine*& lastVisible ) const;
	sdTextLine*				GetLineForPosition( const sdBounds2D& rect, const idVec2& pos ) const;
	void					CursorChanged( bool adjustVScroll = true );
	bool					CaptureChar( const CharType ch );
	void					InsertChar( CharType ch, int& cursorMove );

	void					DoInsertText( const CharType* text );

	void					ClearLines();

	bool					IsFirstLine() const { return currentLine == lines.Next(); }
	bool					IsLastLine() const { return currentLine == lines.Prev(); }

	int						GetCurrentLineIndex() const;

	void					FreeEvent( const sdUIEditEvent* ev );
	const sdUIEditEvent*	GetEvent();
	void					ProcessEvent( const sdUIEditEvent& ev );
	void					RunEventLoop();

private:
	UIEditClass*	parent;

	// cursor
	idVec2			cursorDrawPosition;
	
	int				cursorDrawTime;
	bool			drawCursor;

	// composing
	int				compositionCursor;

	// selection/scrolling
	int				selectionStart;
	float			drawOffset;
	int				mouseDownCursorPosition;
	idVec2			mouseClick;

	// undo
	int				undoCursorPosition;
	StrClass		undoText;
	bool			undoAvailable;

	bool			lastEventWasCharacter;
	idWStr			localText;

	sdTextLine*		currentLine;

	int				cursorMove;

	sdTextDimensionHelper				tdh;

	idListGranularityOne< int >			lineBreaks;
	
	sdTextLine::node_t					lines;

	idLinkList< sdUIEditEvent >			eventQueue;
	idBlockAlloc< sdUIEditEvent, 16 >	eventAllocator;

	bool			textDirty;

	static const int CURSOR_FLASH_TIME = 400;
};

/*
============
sdUIEditHelper::sdUIEditHelper
============
*/
template < class UIEditClass, class StrClass, typename CharType >
sdUIEditHelper< UIEditClass, StrClass, CharType >::sdUIEditHelper() :
	compositionCursor( -1 ),
	selectionStart( -1 ),
	drawOffset( 0.0f ),
	cursorDrawPosition( vec2_zero ),
	undoAvailable( false ),
	undoCursorPosition( 0 ),
	mouseClick( -1.0f, -1.0f ),
	mouseDownCursorPosition( -1 ),
	lastEventWasCharacter( false ),
	cursorDrawTime( 0 ),
	drawCursor( true ),
	cursorMove( 0 ),
	currentLine( new sdTextLine( 0, 0, 0 ) ),
	textDirty( false ) {

	currentLine->GetNode().AddToEnd( lines );
}

/*
============
sdUIEditHelper::~sdUIEditHelper
============
*/
template < class UIEditClass, class StrClass, typename CharType >
sdUIEditHelper< UIEditClass, StrClass, CharType >::~sdUIEditHelper() {
	ClearLines();
}

/*
============
sdUIEditHelper::ClearLines
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::ClearLines() {
	while( !lines.IsListEmpty() ) {
		delete lines.Next();
	}
}

/*
============
sdUIEditHelper::Init
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::Init( UIEditClass* parent ) {
	this->parent = parent;
	TextChanged();
}

/*
============
sdUIEditHelper::PostEvent
============
*/
template < class UIEditClass, class StrClass, typename CharType >
bool sdUIEditHelper< UIEditClass, StrClass, CharType >::PostEvent( bool retVal, const sdSysEvent* event ) {
	bool shiftPressed = keyInputManager->IsDown( K_SHIFT ) || keyInputManager->IsDown( K_RIGHT_SHIFT );
	bool ctrlPressed = keyInputManager->IsDown( K_CTRL ) || keyInputManager->IsDown( K_RIGHT_CTRL );
	int flags = ( shiftPressed ? sdUIEditEvent::EF_SHIFT_PRESSED : 0 ) | ( ctrlPressed ? sdUIEditEvent::EF_CTRL_PRESSED : 0 );

	if ( event->IsMouseEvent() ) {
		QueueEvent( sdUIEditEvent::EE_DRAG_MOVE, 0, 0, *reinterpret_cast<const int *>( &parent->GetUI()->cursorPos.GetValue().x ), *reinterpret_cast<const int *>( &parent->GetUI()->cursorPos.GetValue().y ) );
	} else if ( event->IsMouseButtonEvent() && parent->GetUI()->IsFocused( parent ) ) {
		QueueEvent( sdUIEditEvent::EE_MOUSE, event->GetMouseButton(), event->IsButtonDown(), *reinterpret_cast<const int *>( &parent->GetUI()->cursorPos.GetValue().x ), *reinterpret_cast<const int *>( &parent->GetUI()->cursorPos.GetValue().y ), flags );

		mouseButton_t mb = event->GetMouseButton();

		switch( mb ) {
			case M_MOUSE1:	// FALL THROUGH
			case M_MOUSE2:	// FALL THROUGH
			case M_MOUSE3:	// FALL THROUGH
			case M_MOUSE4:	// FALL THROUGH
			case M_MOUSE5:	// FALL THROUGH
			case M_MOUSE6:	// FALL THROUGH
			case M_MOUSE7:	// FALL THROUGH
			case M_MOUSE8:
			case M_MOUSE9:
			case M_MOUSE10:
			case M_MOUSE11:
			case M_MOUSE12:
				retVal = true;
				break;
			case M_MWHEELUP: 
				if ( parent->TestFlag( UIEditClass::EF_MULTILINE ) ) {
					retVal = true;
				}
				if ( parent->TestFlag( sdUIWindow::WF_CAPTURE_KEYS ) ) {
					retVal = true;
				}
				break;
			case M_MWHEELDOWN: 
				if ( parent->TestFlag( UIEditClass::EF_MULTILINE ) ) {
					retVal = true;
				}
				if ( parent->TestFlag( sdUIWindow::WF_CAPTURE_KEYS ) ) {
					retVal = true;
				}					
				break;
		}
	} else if ( event->IsKeyEvent() && parent->GetUI()->IsFocused( parent ) ) {

		keyNum_t key = event->GetKey();

		if ( event->IsKeyDown() ) {
			switch( key ) {
			case K_ESCAPE:
				QueueEvent( sdUIEditEvent::EE_SELECT_CANCEL );
				retVal = true;
				break;
			case K_HOME:
				QueueEvent( sdUIEditEvent::EE_MOVE_CURSOR, sdUIEditEvent::MC_HOME, 0, 0, 0, flags );
				retVal = true;
				break;
			case K_END:
				QueueEvent( sdUIEditEvent::EE_MOVE_CURSOR, sdUIEditEvent::MC_END, 0, 0, 0, flags );
				retVal = true;
				break;
			case K_UPARROW:
				QueueEvent( sdUIEditEvent::EE_MOVE_CURSOR, sdUIEditEvent::MC_UP, 0, 0, 0, flags );
				retVal = true;
				break;
			case K_DOWNARROW:
				QueueEvent( sdUIEditEvent::EE_MOVE_CURSOR, sdUIEditEvent::MC_DOWN, 0, 0, 0, flags );
				retVal = true;
				break;
			case K_RIGHTARROW:
				QueueEvent( sdUIEditEvent::EE_MOVE_CURSOR, sdUIEditEvent::MC_RIGHT, 0, 0, 0, flags );
				retVal = true;
				break;
			case K_LEFTARROW:
				QueueEvent( sdUIEditEvent::EE_MOVE_CURSOR, sdUIEditEvent::MC_LEFT, 0, 0, 0, flags );
				retVal = true;
				break;
			case K_PGUP:
				if ( parent->TestFlag( UIEditClass::EF_MULTILINE ) ) {
					QueueEvent( sdUIEditEvent::EE_MOVE_CURSOR, sdUIEditEvent::MC_PAGE_UP );
				}
				break;
			case K_PGDN:
				if ( parent->TestFlag( UIEditClass::EF_MULTILINE ) ) {
					QueueEvent( sdUIEditEvent::EE_MOVE_CURSOR, sdUIEditEvent::MC_PAGE_DOWN );
				}
				break;
			case K_DEL:
				if ( parent->readOnly == 0.0f ) {
					QueueEvent( sdUIEditEvent::EE_DELETE );
				}

				retVal = true;
				break;
			default:
				break;
			}
		}
	} else if( !parent->readOnly ) {
		if ( ( event->IsCharEvent() ) && parent->GetUI()->IsFocused( parent ) ) {
// EE_CHAR + ch + ctrlPressed
			CharType ch = event->GetChar();
			if ( ch == '\r' ) {
				ch = '\n';
			}

#define TO_CTRL( character ) ( ( ( character ) - 'a' ) + 1 )
			switch ( ch ) {
				// ctrl-A through ctrl-Z map to ASCII characters 1 through 26
				case TO_CTRL( 'z' ):
					// Ctrl+Z = Undo
					QueueEvent( sdUIEditEvent::EE_UNDO );
					retVal = true;
					break;
				case TO_CTRL( 'l' ):
					// Ctrl+L = Clear
					QueueEvent( sdUIEditEvent::EE_CLEAR );
					retVal = true;
					break;
				case TO_CTRL( 'a' ):
					// Ctrl+A = Select All
					QueueEvent( sdUIEditEvent::EE_SELECT_ALL );
					retVal = true;
					break;
				case TO_CTRL( 'c' ):
				case TO_CTRL( 'x' ):
					// Ctrl+C = Copy, Ctrl+X = Cut
					QueueEvent( sdUIEditEvent::EE_COPY );
					if ( TO_CTRL( 'x' ) == ch ) {
						QueueEvent( sdUIEditEvent::EE_CUT );
					}
					retVal = true;
					break;
				case TO_CTRL( 'v' ):
					// Ctrl+V = Paste
					QueueEvent( sdUIEditEvent::EE_PASTE );
					retVal = true;
					break;
				case TO_CTRL( 'h' ):
					// Ctrl+H = Backspace
					if ( parent->readOnly == 0.0f ) {
						QueueEvent( sdUIEditEvent::EE_BACKSPACE );
					}
					retVal = true;
					break;
				default:
					if ( CaptureChar( ch ) ) {
						QueueEvent( sdUIEditEvent::EE_CHAR, ch );

						retVal = true;
					}
					break;
#undef TO_CTRL
			}
		}
	} else if ( event->IsGuiEvent() ) {
		if ( event->GetGuiAction() == ULI_MENU_NEWLINE ) {
// EE_NEWLINE
			if ( CaptureChar( '\n' ) ) {
				QueueEvent( sdUIEditEvent::EE_NEWLINE );
				retVal = true;
			}
		}
		if ( parent->TestFlag( sdUIWindow::WF_CAPTURE_KEYS ) ) {
			retVal = true;
		}
	}

	if ( !event->IsMouseEvent() && !event->IsMouseButtonEvent() ) {
		if ( parent->TestFlag( sdUIWindow::WF_CAPTURE_KEYS ) && ( ( event->GetKey() < K_F1 || event->GetKey() > K_F15 ) || event->IsGuiEvent() ) ) {
			retVal = true;
		}
	}

	return retVal;
}

/*
============
sdUIEditHelper::DrawLocal
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::ProcessEvent( const sdUIEditEvent& ev ) {
	switch ( ev.GetType() ) {
		case sdUIEditEvent::EE_DRAG_MOVE: {
			if ( mouseDownCursorPosition != -1 ) {
				if ( !SelectionActive() ) {
					selectionStart = currentLine->GetStringIndexForCursor();
				}
				mouseClick.x = ev.GetCursorX();
				mouseClick.y = ev.GetCursorY();
				CursorChanged();
			}
			break;
		}
		case sdUIEditEvent::EE_MOUSE: {
			mouseButton_t mb = ev.GetMouseButton();

			if ( !ev.IsButtonDown() ) {
				switch ( mb ) {
					case M_MOUSE1:	// FALL THROUGH
					case M_MOUSE2:	// FALL THROUGH
					case M_MOUSE3:	// FALL THROUGH
					case M_MOUSE4:	// FALL THROUGH
					case M_MOUSE5:	// FALL THROUGH
					case M_MOUSE6:	// FALL THROUGH
					case M_MOUSE7:	// FALL THROUGH
					case M_MOUSE8:
					case M_MOUSE9:
					case M_MOUSE10:
					case M_MOUSE11:
					case M_MOUSE12:
						mouseDownCursorPosition = -1;
						break;
				}
			} else {
				switch ( mb ) {
					case M_MOUSE1:	// FALL THROUGH
					case M_MOUSE2:	// FALL THROUGH
					case M_MOUSE3:	// FALL THROUGH
					case M_MOUSE4:	// FALL THROUGH
					case M_MOUSE5:	// FALL THROUGH
					case M_MOUSE6:	// FALL THROUGH
					case M_MOUSE7:	// FALL THROUGH
					case M_MOUSE8:
					case M_MOUSE9:
					case M_MOUSE10:
					case M_MOUSE11:
					case M_MOUSE12:
						if ( !ev.ShiftPressed() && SelectionActive() ) {
							CancelSelection();
						}
						mouseDownCursorPosition = currentLine->GetStringIndexForCursor();
						mouseClick.x = ev.GetCursorX();
						mouseClick.y = ev.GetCursorY();
						CursorChanged();
						break;
					case M_MWHEELUP: {
						sdTextLine* firstVisible = NULL;
						sdTextLine* lastVisible = NULL;
						GetVisibleLines( firstVisible, lastVisible );
						if ( parent->scrollAmount.GetValue().y < 0.0f ) {
							float amount = idMath::ClampInt( -parent->scrollAmountMax, 0.0f, parent->scrollAmount.GetValue().y + tdh.GetLineHeight() );
							parent->scrollAmount.SetIndex( 1, amount );
							CursorChanged( false );
						}
						break;
					}
					case M_MWHEELDOWN: {
						sdTextLine* firstVisible = NULL;
						sdTextLine* lastVisible = NULL;
						GetVisibleLines( firstVisible, lastVisible );

						if ( firstVisible != lastVisible ) {
							float amount = idMath::ClampFloat( -parent->scrollAmountMax, 0.0f, parent->scrollAmount.GetValue().y - tdh.GetLineHeight() );
							parent->scrollAmount.SetIndex( 1, amount );
							CursorChanged( false );
						}
						break;
					}
				}
			}
			break;
		}
		case sdUIEditEvent::EE_SELECT_CANCEL: {
			CancelSelection();
			break;
		}
		case sdUIEditEvent::EE_MOVE_CURSOR: {
			switch ( ev.GetCursorMoveOp() ) {
				case sdUIEditEvent::MC_HOME:
					if ( !ev.ShiftPressed() ) {
						CancelSelection();
					} else if ( !SelectionActive() ) {
						selectionStart = currentLine->GetStringIndexForCursor();
					}

					if ( ev.CtrlPressed() || lineBreaks.Num() == 0 ) {
						currentLine = lines.Next();
						parent->scrollAmount = vec2_zero;
					}

					currentLine->SetCursorOnLine( 0 );
					CursorChanged();
					break;
				case sdUIEditEvent::MC_END:
					if ( !ev.ShiftPressed() ) {
						CancelSelection();
					} else if ( !SelectionActive() ) {
						selectionStart = currentLine->GetStringIndexForCursor();
					}

					if ( ev.CtrlPressed() || lineBreaks.Num() == 0 ) {
						currentLine = lines.Next();
						currentLine->SetCursorOnLine( 0 );
						currentLine = currentLine->AdvanceCursor( parent->editText.GetValue().Length() );
					} else {
						currentLine->SetCursorOnLine( currentLine->GetLength() );
					}

					CursorChanged();
					break;
				case sdUIEditEvent::MC_UP:
					if ( !ev.ShiftPressed() ) {
						if ( parent->TestFlag( UIEditClass::EF_MULTILINE ) == true ) {
							CancelSelection();
						}
					} else if ( !SelectionActive() ) {
						selectionStart = currentLine->GetStringIndexForCursor();
					}
					currentLine = currentLine->MoveCursorUp();
					CursorChanged();
					break;
				case sdUIEditEvent::MC_DOWN:
					if ( !ev.ShiftPressed() ) {
						if( parent->TestFlag( UIEditClass::EF_MULTILINE ) == true ) {
							CancelSelection();
						}
					} else if ( !SelectionActive() ) {
						selectionStart = currentLine->GetStringIndexForCursor();
					}
					currentLine = currentLine->MoveCursorDown();
					CursorChanged();
					break;
				case sdUIEditEvent::MC_RIGHT:
					if ( !ev.ShiftPressed() ) {
						CancelSelection();
					} else if ( !SelectionActive() ) {
						selectionStart = currentLine->GetStringIndexForCursor();
					}
					MoveRight();
					break;
				case sdUIEditEvent::MC_LEFT:
					if ( !ev.ShiftPressed() ) {
						CancelSelection();
					} else if ( !SelectionActive() ) {
						selectionStart = currentLine->GetStringIndexForCursor();
					}
					MoveLeft();
					break;
				case sdUIEditEvent::MC_PAGE_UP: {
					sdBounds2D drawRect = parent->GetDrawRect();
					float pageSize = drawRect.GetHeight();
					float amount = idMath::ClampFloat( -parent->scrollAmountMax, 0.0f, parent->scrollAmount.GetValue().y + pageSize );
					parent->scrollAmount.SetIndex( 1, amount );
					CursorChanged( false );
					break;
				}
				case sdUIEditEvent::MC_PAGE_DOWN: {
					sdBounds2D drawRect = parent->GetDrawRect();
					float pageSize = drawRect.GetHeight();
					float amount = idMath::ClampFloat( -parent->scrollAmountMax, 0.0f, parent->scrollAmount.GetValue().y - pageSize );
					parent->scrollAmount.SetIndex( 1, amount );
					CursorChanged( false );
					break;
				}
			}
			break;
		}
		case sdUIEditEvent::EE_BACKSPACE: {
			SaveUndo();

			int index = currentLine->GetStringIndexForCursor();
			if ( !SelectionActive() ) {
				// the cursor is at the very end
				if( index >= parent->editText.GetValue().Length() ) {
					StrClass temp = parent->editText.GetValue().Mid( 0, parent->editText.GetValue().Length() - 1 );							
					parent->editText = temp;
				} else {
					cursorMove += Max( 0, index - 1 ) - index;
					StrClass temp = parent->editText.GetValue().Mid( 0, index + cursorMove );
					temp += parent->editText.GetValue().Mid( index, parent->editText.GetValue().Length() - index );
					parent->editText = temp;
				}
			} else {
				EraseSelection();
				if ( selectionStart < index ) {
					cursorMove += selectionStart - index;
				}
			}

			lastEventWasCharacter = false;
			CancelSelection();	// collapse the selection after the deletion

			break;
		}
		case sdUIEditEvent::EE_DELETE: {
			SaveUndo();

			int index = currentLine->GetStringIndexForCursor();
			if ( !SelectionActive() ) {
				StrClass temp = parent->editText.GetValue().Mid( 0, index );
				temp += parent->editText.GetValue().Mid( index + 1, parent->editText.GetValue().Length() - index - 1 );
				parent->editText = temp;
			} else {
				EraseSelection();
				if ( selectionStart < index ) {
					cursorMove += selectionStart - index;
				}
			}

			lastEventWasCharacter = false;
			CancelSelection();	// collapse the selection after the deletion

			break;
		}
		case sdUIEditEvent::EE_UNDO: {
			Undo();
			break;
		}
		case sdUIEditEvent::EE_CLEAR: {
			parent->ClearText();
			cursorMove += -currentLine->GetStringIndexForCursor();
			break;
		}
		case sdUIEditEvent::EE_SELECT_ALL: {
			SelectAll();
			break;
		}
		case sdUIEditEvent::EE_COPY: {
			if ( SelectionActive() ) {
				int selStart;
				int selEnd;
				GetSelectionRange( selStart, selEnd );

				StrClass selectionText = parent->editText.GetValue().Mid( selStart, selEnd - selStart );
				idWStr clipboardText;
				if ( parent->password ) {
					clipboardText.Fill( L'*', selectionText.Length() );
				} else {
					clipboardText = sdClipboardConverter< StrClass >::ToClipboard( selectionText );
				}					

				sys->SetClipboardData( clipboardText.c_str() );
			}
			break;
		}
		case sdUIEditEvent::EE_CUT: {
			SaveUndo();
			EraseSelection();
			if ( selectionStart < currentLine->GetStringIndexForCursor() ) {
				cursorMove += selectionStart - currentLine->GetStringIndexForCursor();
			}
			CancelSelection();
			break;
		}
		case sdUIEditEvent::EE_PASTE: {
			SaveUndo();

			idWStr pasteText = sys->GetClipboardData();
			pasteText.Replace( L"\b", L"" );
			pasteText.Replace( L"\t", L"" );
			pasteText.Replace( L"\r", L"" );

			if ( pasteText.Length() ) {
				int numErased = 0;
				if ( SelectionActive() ) {
					numErased = EraseSelection();
				}

				int selStart;
				int selEnd;
				GetSelectionRange( selStart, selEnd );
				StrClass clipboardTemp( sdClipboardConverter< StrClass >::FromClipboard( pasteText ) );
				StrClass temp = parent->editText;
				temp.Insert( clipboardTemp.c_str(), selEnd - numErased );

				if ( parent->maxTextLength == 0.0f || temp.LengthWithoutColors() <= parent->maxTextLength ) {
					if ( parent->TestFlag( UIEditClass::EF_UPPERCASE ) ) {
						for ( int i = 0; i < temp.Length(); i++ ) {
							temp[ i ] = sdCaseConverter< CharType >::ToUpper( temp[ i ] );
						}
					}
					parent->editText = temp;

					CancelSelection();
					cursorMove += pasteText.Length();
				}
			}
			break;
		}
		case sdUIEditEvent::EE_CHAR: {
			CharType ch = ev.GetChar();
			StrClass temp = parent->editText;

			int selStart;
			int selEnd;

			GetSelectionRange( selStart, selEnd );
			if ( selStart < selEnd ) {
				temp.EraseRange( selStart, selEnd - selStart );
			}

			temp.Insert( ch, currentLine->GetStringIndexForCursor() - ( selEnd - selStart ) );
			if ( parent->maxTextLength == 0.0f || temp.LengthWithoutColors() <= parent->maxTextLength ) {
				InsertChar( ch, cursorMove );
			}

			break;
		}
		case sdUIEditEvent::EE_NEWLINE: {
			InsertChar( '\n', cursorMove );
			break;
		}
		default:
			break;
	}

	switch ( ev.GetType() ) {
		case sdUIEditEvent::EE_INSERT_TEXT: {
			DoInsertText( ev.GetText() );
			break;
		}
		case sdUIEditEvent::EE_COMPOSITION_CURSOR_SET: {
			compositionCursor = currentLine->GetStringIndexForCursor();
			break;
		}
		case sdUIEditEvent::EE_COMPOSITION_COMMIT: {
			if ( compositionCursor != -1 ) {
				// commit composition text
				currentLine = lines.Next();
				currentLine->SetCursorOnLine( 0 );
				currentLine = currentLine->AdvanceCursor( compositionCursor );
				compositionCursor += StrClass::Length( ev.GetText() );
				DoInsertText( ev.GetText() );
			}
			break;
		}
		default: {
			if ( cursorMove != 0 && !ev.ShiftPressed() && SelectionActive() ) {
				CancelSelection();
			}
			break;
		 }
	}

	if ( textDirty ) {
		TextChanged();
	}
}

/*
============
sdUIEditHelper::DrawLocal
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::DrawLocal() {
	sdBounds2D rect = parent->GetDrawRect();
	rect.GetMins() += parent->scrollAmount;

	int textLength = localText.Length();	

	int now = parent->GetUI()->GetCurrentTime();

	// Draw cursor
	if ( parent->GetUI()->IsFocused( parent ) ) {		
		if( now > cursorDrawTime ) {
			drawCursor = !drawCursor;
			cursorDrawTime = now + CURSOR_FLASH_TIME;
		}
		if ( drawCursor ) {
			float x = cursorDrawPosition.x;
			float y = cursorDrawPosition.y;
			float w = parent->cursor.width;
			float h = tdh.GetLineHeight() ? tdh.GetLineHeight() : parent->cursor.height;

			// highlight whole cursor text if present
			const wchar_t* cursorText = parent->GetCursorText();
			if ( *cursorText != L'\0' ) {
				int currentCursor = currentLine->GetStringIndexForCursor() + cursorMove;	

				w = tdh.GetWidth( currentCursor, currentCursor );

				idVec4 color( parent->foreColor.GetValue().x, parent->foreColor.GetValue().y, parent->foreColor.GetValue().z, 0.25f );

				deviceContext->DrawClippedRect( x, y, w, h, color );
				deviceContext->DrawClippedBox( x, y, w, h, 1.0f, parent->foreColor );
			} else {
				parent->DrawMaterial( parent->cursor.mi, x, y, w, h, parent->foreColor );	
			}
		}
	}

	// selection highlight
	if ( SelectionActive() ) {
		int selectionStart = this->selectionStart;
		int selectionEnd = currentLine->GetStringIndexForCursor();
		if( selectionEnd < selectionStart ) {
			Swap( selectionEnd, selectionStart );
		}

		sdTextLine* line = lines.Next();

		float height = tdh.GetLineHeight();
		idVec4 color( parent->foreColor.GetValue().x, parent->foreColor.GetValue().y, parent->foreColor.GetValue().z, 0.25f );

		int i = 0;
		sdBounds2D offsetRect;
		while( line != NULL ) {		
			int begin	= Max( selectionStart, line->GetStart() );
			int end		= Min( selectionEnd - 1, line->GetEnd() );

			float width = tdh.GetWidth( begin, end );
			float xOffset = tdh.GetWidth( line->GetStart(), line->GetStart() + ( begin - line->GetStart() - 1 ) );

			offsetRect = rect;
			offsetRect.TranslateSelf( xOffset, parent->lineHeight * i );

			deviceContext->DrawClippedRect( offsetRect.GetMins().x, offsetRect.GetMins().y, width, height, color );

			line = line->GetNode().Next();
			i++;
		}
	}
}

/*
============
sdUIEditHelper::DrawText
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::DrawText( const idVec4& color ) {
	// edit text
	if ( localText.Length() > 0 ) {
		sdBounds2D rect = parent->GetDrawRect();
		rect.GetMins() += parent->scrollAmount;

		sdWStringBuilder_Heap builder;

		sdTextLine* line = lines.Next();
		int i = 0;
		sdBounds2D offsetRect;
		while( line != NULL ) {		
			int len = line->GetLength();
			if( len > 0 ) {
				builder = L"";
				builder.Append( &localText[ line->GetStart() ], len );

				offsetRect = rect;
				offsetRect.TranslateSelf( 0.0f, parent->lineHeight * i );
				parent->sdUIWindow::DrawText( builder.c_str(), color, parent->fontSize, offsetRect, parent->cachedFontHandle, parent->GetDrawTextFlags() );
			}
			line = line->GetNode().Next();
			i++;
		}
	}
}

/*
============
sdUIEditHelper::ClearText
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::ClearText() {
	SaveUndo();
	CancelSelection();
	CursorChanged();
	ClearLines();
	currentLine = new sdTextLine( 0, 0, 0 );
	currentLine->GetNode().AddToEnd( lines );
}

/*
============
sdUIEditHelper::GetSelectionRange
============
*/
template < class UIEditClass, class StrClass, typename CharType >
bool sdUIEditHelper< UIEditClass, StrClass, CharType >::GetSelectionRange( int& selStart, int& selEnd ) const {
	int stringIndex = currentLine->GetStringIndexForCursor() + cursorMove;

	if ( !SelectionActive() ) {
		selStart = stringIndex;
		selEnd = stringIndex;
		return false;
	}

	selStart = selectionStart == -1 ? stringIndex : selectionStart;
	selEnd = selectionStart == -1 ? stringIndex + 1 : stringIndex;
	if ( selEnd < selStart ) {
		Swap( selEnd, selStart );
	}

	int len = parent->editText.GetValue().Length();
	selStart = idMath::ClampInt( 0, len - 1, selStart );
	selEnd = idMath::ClampInt( selStart, len, selEnd );
	return true;
}

/*
============
sdUIEditHelper::EraseSelection
============
*/
template < class UIEditClass, class StrClass, typename CharType >
int sdUIEditHelper< UIEditClass, StrClass, CharType >::EraseSelection() {
	int selStart;
	int selEnd;
	GetSelectionRange( selStart, selEnd );
	if ( selStart == selEnd ) {
		return 0;
	}

	StrClass temp = parent->editText;
	temp.EraseRange( selStart, selEnd - selStart );
	parent->editText = temp;

	return ( selEnd - selStart );
}

/*
============
sdUIEditHelper::MoveLeft
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::MoveLeft() {

	int cursorMove = -1;
	// move to the previous word	
	const CharType* buffer = parent->editText.GetValue().c_str();
	int len = StrClass::Length( buffer );

	if ( keyInputManager->IsDown( K_CTRL ) || keyInputManager->IsDown( K_RIGHT_CTRL ) ) {						
		if( UIEditClass::CharIsPrintable( buffer[ currentLine->GetStringIndexForCursor() + cursorMove - 1 ] ) ) {
			while ( ( ( currentLine->GetStringIndexForCursor() + cursorMove ) > 0 ) && UIEditClass::CharIsPrintable( buffer[ currentLine->GetStringIndexForCursor() + cursorMove - 1 ] ) ) {
				cursorMove--;
			}
		} else {
			// skip out of whitespace to a word
			while ( ( ( currentLine->GetStringIndexForCursor() + cursorMove ) > 0 ) && !UIEditClass::CharIsPrintable( buffer[ currentLine->GetStringIndexForCursor() + cursorMove - 1 ] ) ) {
				cursorMove--;
			}

			// now skip the word
			while ( ( ( currentLine->GetStringIndexForCursor() + cursorMove ) > 0 ) && UIEditClass::CharIsPrintable( buffer[ currentLine->GetStringIndexForCursor() + cursorMove - 1 ] ) ) {
				cursorMove--;
			}
		}

		
	}

	currentLine = currentLine->AdvanceCursor( cursorMove );
	CursorChanged();
}


/*
============
sdUIEditHelper::MoveRight
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::MoveRight() {
	int cursorMove = 1;
	// move to the next word
	const CharType* buffer = parent->editText.GetValue().c_str();
	int len = StrClass::Length( buffer );

	if ( keyInputManager->IsDown( K_CTRL ) || keyInputManager->IsDown( K_RIGHT_CTRL ) ) {
		// skip out of the current word
		while ( ( ( currentLine->GetStringIndexForCursor() + cursorMove ) < len ) && UIEditClass::CharIsPrintable( buffer[ currentLine->GetStringIndexForCursor() + cursorMove ] ) ) {
			cursorMove++;
		}

		// skip whitespace
		while ( ( ( currentLine->GetStringIndexForCursor() + cursorMove ) < len ) && !UIEditClass::CharIsPrintable( buffer[ currentLine->GetStringIndexForCursor() + cursorMove ] ) ) {
			cursorMove++;
		}
	}

	currentLine = currentLine->AdvanceCursor( cursorMove );
	CursorChanged();
}


/*
============
sdUIEditHelper::UpdateIndexByColorCodes
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::UpdateIndexByColorCodes( int start, int& index ) {
}

/*
============
sdUIEditHelper::GetVisibleLines
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::GetVisibleLines( sdTextLine*& firstVisible, sdTextLine*& lastVisible ) const {
	firstVisible = lastVisible = NULL;

	sdBounds2D rect = parent->GetDrawRect();
	
	firstVisible = lines.Prev();
	
	int index = lines.Num();
	while( firstVisible != NULL ) {
		if( ( index * tdh.GetLineHeight() ) + parent->scrollAmount.GetValue().y <= 0.0f ) {
			break;
		}
		index--;
		firstVisible = firstVisible->GetNode().Prev();
	}
	if( firstVisible == NULL ) {
		firstVisible = lines.Next();
		index = 0;
	}

	lastVisible = firstVisible;

	while( lastVisible != NULL ) {
		if( ( ( index + 1 ) * tdh.GetLineHeight() ) + parent->scrollAmount.GetValue().y >= rect.GetHeight() ) {
			break;
		}
		index++;
		lastVisible = lastVisible->GetNode().Next();
	}
	if( lastVisible == NULL ) {
		lastVisible = lines.Prev();
	}
}

/*
============
sdUIEditHelper::GetLineForPosition
============
*/
template < class UIEditClass, class StrClass, typename CharType >
sdTextLine* sdUIEditHelper< UIEditClass, StrClass, CharType >::GetLineForPosition( const sdBounds2D& rect, const idVec2& pos ) const {
	if( pos.x < rect.GetMins().x ) {
		return currentLine;
	}

	float rowHeight = tdh.GetLineHeight();
	int index = 0;
	sdTextLine* line = lines.Next();

	while( line != NULL ) {
		float rowBase = rect.GetMins().y + index * rowHeight;	
		if( rowBase < pos.y && pos.y < rowBase + rowHeight ) {
			return line;
		}
		index++;
		line = line->GetNode().Next();
	}
	return currentLine;
}

/*
============
sdUIEditHelper::CursorChanged
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::CursorChanged( bool adjustVScroll ) {
	int textLength = localText.Length();
	
	idVec2 newCursorPos;
	idVec2 tdhOffset;

	sdBounds2D rect( parent->GetWorldRect() );
	//rect.TranslateSelf( -rect.GetMins() );
	sdBounds2D offsetRect( rect );
	offsetRect.TranslateSelf( parent->scrollAmount );

	if( mouseClick.x >= 0.0f && mouseClick.y >= 0.0f ) {
		idVec2 offsetClick = mouseClick;
		offsetClick -= parent->GetWorldRect().ToVec2();

		sdTextLine* clickedRow = GetLineForPosition( offsetRect, mouseClick );
		currentLine = clickedRow;

		// see if the click is past the end of the line
		float width = tdh.GetWidth( clickedRow->GetStart(), clickedRow->GetEnd() );
		if ( offsetClick.x >= width ) {
			clickedRow->SetCursorOnLine( clickedRow->GetLength() );
		} else {
			int i;
			for ( i = clickedRow->GetStart(); i <= clickedRow->GetEnd(); i++ ) {
				width = tdh.GetWidth( clickedRow->GetStart(), i - 1 );
				float charWidth = tdh.ToVirtualScreenSizeFloat( tdh.GetAdvance( i ) );
				float halfWidth = ( charWidth * 0.5f );
				if ( ( offsetClick.x - halfWidth ) < ( width + parent->scrollAmount.GetValue().x + halfWidth ) ) {
					clickedRow->SetCursorOnLine( i - clickedRow->GetStart() );
					break;
				}
			}
			if( i > clickedRow->GetEnd() ) {
				clickedRow->SetCursorOnLine( clickedRow->GetLength() );
			}
		}

		mouseClick.Set( -1.0f, -1.0f );
	}

	tdhOffset.Zero();

	sdTextLine* line = lines.Next();
	while( line != NULL ) {
		if( line == currentLine ) {
			tdhOffset.x = tdh.GetWidth( line->GetStart(), currentLine->GetStringIndexForCursor() - 1 );
			break;
		}		
		tdhOffset.y += tdh.GetLineHeight();		
		line = line->GetNode().Next();
	}

	sdBounds2D drawRect = parent->GetDrawRect();

	tdhOffset += drawRect.GetMins() - rect.GetMins();

	newCursorPos = rect.GetMins() + tdhOffset;
	if( parent->GetUI() != NULL ) {
		cursorDrawTime = parent->GetUI()->GetCurrentTime() + CURSOR_FLASH_TIME;
	}
	drawCursor = true;

	idVec2 offset = parent->scrollAmount;	
	if( newCursorPos.x >= drawRect.GetMaxs().x ) {
		// moved past the right end
		if( ( newCursorPos.x + offset.x ) >= cursorDrawPosition.x ) {
			offset.x = ( drawRect.GetMaxs().x - newCursorPos.x ) - parent->cursor.width;
		}
	} else if( ( newCursorPos.x + offset.x ) <= drawRect.GetMins().x ) {
		// moving past the left side
		offset.x = ( newCursorPos.x + offset.x ) - drawRect.GetMins().x;
	}
	
	if( parent->TestFlag( UIEditClass::EF_MULTILINE ) ) {
		if( adjustVScroll ) {
			if( newCursorPos.y + offset.y + parent->cursor.height > rect.GetMaxs().y ) {
				// moved past the bottom
				offset.y -= tdh.GetLineHeight() + ( offset.y + ( newCursorPos.y + parent->cursor.height ) - rect.GetMaxs().y );
			} else if( newCursorPos.y + offset.y <= rect.GetMins().y ) {
				// moving past the top
				offset.y = -( GetCurrentLineIndex() * tdh.GetLineHeight() );
			}
		}
		offset.x = 0.0f;
	} else {
		offset.y = 0.0f;
	}

	parent->scrollAmount = offset;

	cursorDrawPosition = newCursorPos + offset;
}

/*
============
sdUIEditHelper::TextChanged
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::TextChanged() {
	textDirty = false;

	int currentCursor = currentLine->GetStringIndexForCursor() + cursorMove;	

	localText = sdClipboardConverter< StrClass >::ToClipboard( parent->editText.GetValue() );

	// insert cursor text if required
	const wchar_t* cursorText = parent->GetCursorText();

	if ( *cursorText != L'\0' ) {
		localText.Insert( cursorText, currentCursor );
	}

	// update text
	int textLength = localText.Length();

	if ( parent->password != 0.0f ) {
		localText.Fill( L'*', textLength ); 
	}

	ClearLines();

	sdBounds2D rect = parent->GetDrawRect();

	parent->ActivateFont( false );	

	tdh.Init( localText.c_str(), textLength, rect, parent->GetDrawTextFlags(), parent->cachedFontHandle, parent->fontSize, &lineBreaks );

	int currentPos = 0;

	for( int i = 0; i < lineBreaks.Num(); i++ ) {
		int index = lineBreaks[ i ] - 1;
		sdTextLine* line = new sdTextLine( currentPos, index );
		line->GetNode().AddToEnd( lines );
		currentPos = lineBreaks[ i ];
		if( localText[ currentPos ] == L'\n' ) {
			currentPos++;
		}
	}

	sdTextLine* line = new sdTextLine( currentPos, textLength );
	line->GetNode().AddToEnd( lines );

	currentLine = lines.Next();
	currentLine->SetCursorOnLine( 0 );

	parent->lineHeight.SetReadOnly( false );
	parent->lineHeight = tdh.GetLineHeight();
	parent->lineHeight.SetReadOnly( true );

	if( parent->TestFlag( UIEditClass::EF_MULTILINE ) ) {
		parent->scrollAmountMax.SetReadOnly( false );
		float amount = ( ( lines.Num() ) * parent->lineHeight ) - parent->cachedClientRect.w;
		if( amount > 0.0f ) {
			parent->scrollAmountMax = amount;
		} else {
			parent->scrollAmountMax = 0.0f;
		}

		parent->scrollAmountMax.SetReadOnly( true );
	}

	if( localText.IsEmpty() ) {
		parent->scrollAmount = vec2_zero;
	}
	currentLine = currentLine->AdvanceCursor( currentCursor );
	cursorMove = 0;
	CursorChanged();
}

template < class UIEditClass, class StrClass, typename CharType >
bool sdUIEditHelper< UIEditClass, StrClass, CharType >::CaptureChar( const CharType ch  ) {	
	bool isNewline = ch == '\n';
	if ( isNewline && !parent->TestFlag( UIEditClass::EF_MULTILINE ) ) {
		return false;
	}

	bool retVal = false;
	unsigned int flags = parent->GetDrawTextFlags();

	if ( UIEditClass::CharIsPrintable( ch ) || ( isNewline && ( ( flags & DTF_WORDWRAP ) || !( flags & DTF_SINGLELINE ) ) ) ) {
		bool isNumeric = idStr::CharIsNumeric( ch );
		bool isDecimal = ch == '.';
		bool allowInput = true;

		bool allowDecimal = parent->TestFlag( UIEditClass::EF_ALLOW_DECIMAL );
		bool acceptOnlyNumbers = parent->TestFlag( UIEditClass::EF_INTEGERS_ONLY );

		if ( acceptOnlyNumbers ) {
			if ( !isDecimal && !isNumeric ) {
				allowInput = false;
			} else if ( isDecimal && !allowDecimal ) {
				allowInput = false;
			}
		}

		// Can't check for this here as it depends on editText, which is deferred
		/*if ( allowDecimal && isDecimal ) {
			if ( parent->editText.GetValue().Find( static_cast< CharType >( ch ) ) != -1 ) {
				allowInput = false;
			}
		}*/

		if ( allowInput ) {
			retVal = true;
		}
	} else {
		parent->RunEvent( sdUIEventInfo( UIEditClass::EE_INPUT_FAILED, 0 ) );
	}

	return retVal;
}

template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::InsertChar( CharType ch, int& cursorMove ) {	
	bool isNewline = ch == '\n';
	if ( isNewline && !parent->TestFlag( UIEditClass::EF_MULTILINE ) ) {
		return;
	}

	unsigned int flags = parent->GetDrawTextFlags();

	if ( UIEditClass::CharIsPrintable( ch ) || ( isNewline && ( ( flags & DTF_WORDWRAP ) || !( flags & DTF_SINGLELINE ) ) ) ) {
		bool isNumeric = idStr::CharIsNumeric( ch );
		bool isDecimal = ch == '.';
		bool allowInput = true;

		bool allowDecimal = parent->TestFlag( UIEditClass::EF_ALLOW_DECIMAL );
		bool acceptOnlyNumbers = parent->TestFlag( UIEditClass::EF_INTEGERS_ONLY );

		if ( acceptOnlyNumbers ) {
			if ( !isDecimal && !isNumeric ) {
				allowInput = false;
			} else if ( isDecimal && !allowDecimal ) {
				allowInput = false;
			}
		}

		if ( allowDecimal && isDecimal ) {
			if ( parent->editText.GetValue().Find( static_cast< CharType >( ch ) ) != -1 ) {
				allowInput = false;
			}
		}

		if ( allowInput ) {
			// FIXME: this is wrong for non english-ish languages
			// don't make a separate undo for an unbroken string of typing
			if ( !lastEventWasCharacter ) {
				SaveUndo();
			}

			lastEventWasCharacter = true;
			int numErased = 0;
			int selStart;
			int selEnd;

			GetSelectionRange( selStart, selEnd );

			if ( SelectionActive() ) {
				numErased = EraseSelection();
				if( selectionStart < currentLine->GetStringIndexForCursor() ) {
					cursorMove += selectionStart - currentLine->GetStringIndexForCursor() + 1;
				} else {
					cursorMove += 1;
				}
			} else {
				cursorMove += 1;
			}

			UpdateIndexByColorCodes( selStart, selEnd );

			if ( parent->TestFlag( UIEditClass::EF_UPPERCASE ) ) {
				ch = sdCaseConverter< CharType >::ToUpper( ch );
			}

			StrClass temp = parent->editText;				
			temp.Insert( static_cast< CharType >( ch ), selEnd - numErased );
			parent->editText = temp;
			CancelSelection();	// collapse the selection after the replacement
		}
	}
}

/*
============
sdUIEditHelper::OnGainFocus
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::OnGainFocus() {
	if( parent->GetUI() != NULL ) {
		cursorDrawTime = parent->GetUI()->GetCurrentTime() + CURSOR_FLASH_TIME;
	}
}

/*
============
sdUIEditHelper::InsertText
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::InsertText( const CharType* text ) {
	if ( parent->readOnly ) {
		return;
	}

	int len = StrClass::Length( text );

	size_t dataSize = ( len + 1 ) * sizeof( CharType );
	void* data = Mem_Alloc( dataSize );
	::memcpy( data, text, dataSize );
	QueueEvent( sdUIEditEvent::EE_INSERT_TEXT, 0, 0, 0, 0, 0, dataSize, data );
}

/*
============
sdUIEditHelper::DoInsertText
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::DoInsertText( const CharType* text ) {
	int numErased = 0;
	if ( SelectionActive() ) {
		numErased = EraseSelection();
	}

	int selStart;
	int selEnd;
	GetSelectionRange( selStart, selEnd );
	StrClass temp = parent->editText;
	temp.Insert( text, selEnd - numErased );

	if ( parent->maxTextLength == 0.0f || temp.LengthWithoutColors() <= parent->maxTextLength ) {
		if ( parent->TestFlag( UIEditClass::EF_UPPERCASE ) ) {
			for ( int i = 0; i < temp.Length(); i++ ) {
				temp[ i ] = sdCaseConverter< CharType >::ToUpper( temp[ i ] );
			}
		}
		SaveUndo();
		parent->editText = temp;

		CancelSelection();
		cursorMove += StrClass::Length( text );
	}
}

/*
============
sdUIEditHelper::SurroundSelection
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::SurroundSelection( const CharType* prefix, const CharType* suffix ) {
	if ( parent->readOnly ) {
		return;
	}
	int selStart;
	int selEnd;
	GetSelectionRange( selStart, selEnd );
	StrClass temp = parent->editText;
	if ( selStart < 0 ) {
		selStart = 0;
	}
	if ( selEnd < 0 ) {
		selEnd = temp.Length();
	}
	temp.Insert( suffix, selEnd );
	temp.Insert( prefix, selStart );	

	if ( parent->maxTextLength == 0.0f || temp.LengthWithoutColors() <= parent->maxTextLength ) {
		if ( parent->TestFlag( UIEditClass::EF_UPPERCASE ) ) {
			for ( int i = 0; i < temp.Length(); i++ ) {
				temp[ i ] = sdCaseConverter< CharType >::ToUpper( temp[ i ] );
			}
		}
		SaveUndo();
		parent->editText = temp;
		
		int len = StrClass::Length( prefix );
		cursorMove += len;

		if ( selectionStart != -1 ) {
			selectionStart += len;
		}
	}
}

/*
============
sdUIEditHelper::SelectAll
============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::SelectAll() {
	selectionStart = 0;
	currentLine = currentLine->AdvanceCursor( parent->editText.GetValue().Length() - currentLine->GetStringIndexForCursor() );
	CursorChanged();
}

/*
============
sdUIEditHelper::GetCurrentLineIndex
============
*/
template < class UIEditClass, class StrClass, typename CharType >
int sdUIEditHelper< UIEditClass, StrClass, CharType >::GetCurrentLineIndex() const {
	sdTextLine* line = lines.Next();
	int index = 0;
	while( line != currentLine ) {
		index++;
		line = line->GetNode().Next();		
	}
	return index;
}

/*
=================
sdUIEditHelper::FreeEvent
=================
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::FreeEvent( const sdUIEditEvent* ev ) {
	sdUIEditEvent* event = const_cast< sdUIEditEvent* >( ev );
	event->GetNode().Remove();
	eventAllocator.Free( event );
}

/*
=================
sdUIEditHelper::GenerateBlankEvent
=================
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::QueueEvent( typename sdUIEditEvent::type_e type, int value, int value2, int value3, int value4, int flags, size_t dataSize, void* data ) {
	sdUIEditEvent* ev = eventAllocator.Alloc();
	ev->Init( type, value, value2, value3, value4, flags, dataSize, data );
	ev->GetNode().AddToEnd( eventQueue );
}

/*
=================
sdUIEditHelper::GetEvent
=================
*/
template < class UIEditClass, class StrClass, typename CharType >
const typename sdUIEditHelper< UIEditClass, StrClass, CharType >::sdUIEditEvent* sdUIEditHelper< UIEditClass, StrClass, CharType >::GetEvent() {
	return eventQueue.Next();
}

/*
===============
sdUIEditHelper::RunEventLoop
===============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::RunEventLoop() {
	while ( true ) {
		const sdUIEditEvent* ev = GetEvent();

		// if no more events are available
		if ( ev == NULL ) {
			return;
		}

		ProcessEvent( *ev );
		FreeEvent( ev );
	}
}

/*
===============
sdUIEditHelper::RunEventLoop
===============
*/
template < class UIEditClass, class StrClass, typename CharType >
void sdUIEditHelper< UIEditClass, StrClass, CharType >::ApplyLayout() {
	RunEventLoop();
}

#endif // ! __GAME_GUIS_USERINTERFACEEDITHELPER_H__
