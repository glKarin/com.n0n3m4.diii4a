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
#include "UIList.h"
#include "UserInterfaceManager.h"

#include "../../sys/sys_local.h"
#include "../../idlib/Sort.h"
#include "../../decllib/declTypeHolder.h"

SD_UI_IMPLEMENT_CLASS( sdUIList, sdUIWindow )

idHashMap< sdUITemplateFunction< sdUIList >* > sdUIList::listFunctions;

const char sdUITemplateFunctionInstance_IdentifierList[]	= "sdUIListFunction";
const char sdUIListItem_Identifier[]						= "sdUIListItem";
const char sdUIListColumn_Identifier[]						= "sdUIListColumn";

idBlockAlloc< sdUIList::vec4EvaluatorList_t, 32 > sdUIList::vec4EvaluatorListAllocator;

SD_UI_PUSH_CLASS_TAG( sdUIList )
const char* sdUIList::eventNames[ LE_NUM_EVENTS - WE_NUM_EVENTS ] = {
	SD_UI_EVENT_TAG( "onSelectItem",					"",				"Called when a list item is selected." ),
	SD_UI_EVENT_TAG( "onItemAdded",						"",				"Called when an item is inserted into the list." ),
	SD_UI_EVENT_TAG( "onItemRemoved",					"",				"Called when an item is deleted from the list." ),

	SD_UI_EVENT_PARM_TAG( "onDrawSelectedBackground",	"",				"If this event is valid in the GUI it will be called instead of onDrawItemBackGround for the currently selected item." ),
		SD_UI_EVENT_PARM( float, "index", "Row index." )
		SD_UI_EVENT_PARM( rect, "rectangle", "Item rectangle." )
	SD_UI_END_EVENT_TAG

	SD_UI_EVENT_PARM_TAG( "onDrawItemBackGround",		"",				"Called when drawing an item background." ),
		SD_UI_EVENT_PARM( float, "index", "Row index." )
		SD_UI_EVENT_PARM( color, "itemColor", "Item color." )
		SD_UI_EVENT_PARM( rect, "rectangle", "Item rectangle." )
	SD_UI_END_EVENT_TAG

	SD_UI_EVENT_PARM_TAG( "onDrawItem",					"",				"Called when drawing an item. \"customDraw\" flag must be specified for the item for this event to be called for the item." ),
		SD_UI_EVENT_PARM( float, "itemRow", "Row index." )
		SD_UI_EVENT_PARM( float, "itemColumn", "Item column." )
		SD_UI_EVENT_RETURN_PARM( float, "Return false if the window should not do drawing itself." )
	SD_UI_END_EVENT_TAG

	SD_UI_EVENT_PARM_TAG( "onDrawColumn",					"",			"Called when drawing a column. \"customDraw\" flag must be specified for the column for event to be called." ),
		SD_UI_EVENT_PARM( rect, "rectangle", "Column rectangle." )
		SD_UI_EVENT_PARM( float, "column", "Column to draw." )
		SD_UI_EVENT_RETURN_PARM( float, "Return false if the window should not do drawing itself." )
	SD_UI_END_EVENT_TAG

	SD_UI_EVENT_PARM_TAG( "onEnterColumnHeader",				"",		"Called when the mouse enters a column header." ),
		SD_UI_EVENT_PARM( float, "column", "Column index." )
	SD_UI_END_EVENT_TAG

	SD_UI_EVENT_PARM_TAG( "onExitColumnHeader",				"",			"Called when the mouse exits a column header." ),
		SD_UI_EVENT_PARM( float, "column", "Column index." )
	SD_UI_END_EVENT_TAG

	SD_UI_EVENT_TAG( "onClickColumnHeader",				"",				"Called when there's a mouse click on the column header." ),

	SD_UI_EVENT_PARM_TAG( "onEnterItem",						"",		"Called when the mouse enters the list item." ),
	SD_UI_EVENT_PARM( float, "row", "The row index." )
		SD_UI_EVENT_PARM( float, "column", "Column index." )
	SD_UI_END_EVENT_TAG

	SD_UI_EVENT_PARM_TAG( "onExitItem",						"",				"Called when the mouse exits the list item." ),
		SD_UI_EVENT_PARM( float, "row", "Row index." )
		SD_UI_EVENT_PARM( float, "column", "Column index." )
	SD_UI_END_EVENT_TAG
};
SD_UI_POP_CLASS_TAG

/*
============
sdUIList::sdUIList
============
*/
sdUIList::sdUIList() :
	scrollAmount( 0.0f ),
	scrollAmountMax( 0.0f ),	
	columnHeight( 0.0f ),
	activeColumn( -1.0f ),
	activeColumnSort( 1.0f ),
	pageSize( 0.0f ),
	storedScrollAmount( -1.0f ),	
	numItems( 0.0f ) {
	
	inBatchOperation = false;
	sizeLastColumnDelta = 0.0f;
	scrollToItem = -1;

	scriptState.GetProperties().RegisterProperty( "selectedItemForeColor",			selectedItemForeColor );

	scriptState.GetProperties().RegisterProperty( "columnFontSize",					columnFontSize );
	scriptState.GetProperties().RegisterProperty( "columnBorder",					columnBorder );
	
	scriptState.GetProperties().RegisterProperty( "rowSpacing",						rowSpacing );
	scriptState.GetProperties().RegisterProperty( "fixedRowHeight",					fixedRowHeight );

	scriptState.GetProperties().RegisterProperty( "currentSelection",				currentSelection );
	scriptState.GetProperties().RegisterProperty( "activeColumn",					activeColumn );
	scriptState.GetProperties().RegisterProperty( "activeColumnSort",				activeColumnSort );
	scriptState.GetProperties().RegisterProperty( "columnHeight",					columnHeight );
	scriptState.GetProperties().RegisterProperty( "scrollAmount",					scrollAmount );
	scriptState.GetProperties().RegisterProperty( "scrollAmountMax",				scrollAmountMax );
	scriptState.GetProperties().RegisterProperty( "pageSize",						pageSize );
	scriptState.GetProperties().RegisterProperty( "internalBorderWidth",			internalBorderWidth );
	scriptState.GetProperties().RegisterProperty( "numItems",						numItems );

	scrollAmountMax.SetReadOnly( true );
	pageSize.SetReadOnly( true );
	numItems.SetReadOnly( true );
	columnHeight.SetReadOnly( true );
	columnFontSize				= 48.0f;
	columnBorder				= 0.0f;
	internalBorderWidth			= 0.0f;

	fixedRowHeight				= 0.0f;
	rowSpacing					= 0.0f;
	textAlignment				= idVec2( TA_LEFT, TA_VCENTER );
	
	selectedItemForeColor		= colorWhite;

	currentSelection			= -1.0f;
	dropShadowOffsetImage		= 0.0f;

	UI_ADD_FLOAT_CALLBACK( fixedRowHeight,		sdUIList, OnFixedRowHeightChanged )
	UI_ADD_FLOAT_CALLBACK( currentSelection,	sdUIList, OnCurrentItemChanged )
	UI_ADD_FLOAT_CALLBACK( activeColumn,		sdUIList, OnActiveColumnChanged )
	UI_ADD_FLOAT_CALLBACK( internalBorderWidth,	sdUIList, OnInternalBorderChanged )

	SetWindowFlag( WF_ALLOW_FOCUS );
	SetWindowFlag( WF_CLIP_TO_RECT );
	SetWindowFlag( LF_AUTO_SCROLL_TO_SELECTION );
}

/*
============
sdUIList::InitProperties
============
*/
void sdUIList::InitProperties() {
	sdUIWindow::InitProperties();
}

/*
============
sdUIList::~sdUIList
============
*/
sdUIList::~sdUIList() {
	columns.DeleteContents( true );
	DisconnectGlobalCallbacks();
}

/*
============
sdUIList::FindFunction
============
*/
const sdUIList::ListTemplateFunction* sdUIList::FindFunction( const char* name ) {
	sdUIList::ListTemplateFunction** ptr;
	return listFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUIList::GetFunction
============
*/
sdUIFunctionInstance* sdUIList::GetFunction( const char* name ) {
	const ListTemplateFunction* function = sdUIList::FindFunction( name );
	if ( !function ) {		
		return sdUIWindow::GetFunction( name );
	}
	return new sdUITemplateFunctionInstance< sdUIList, sdUITemplateFunctionInstance_IdentifierList >( this, function );
}

/*
============
sdUIList::RunNamedMethod
============
*/
bool sdUIList::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const sdUIList::ListTemplateFunction* func = sdUIList::FindFunction( name );
	if ( !func ) {
		return sdUIWindow::RunNamedMethod( name, stack );
	}

	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}

/*
============
sdUIList::CacheEvents
============
*/
void sdUIList::CacheEvents() {
	sdUIWindow::CacheEvents();
	drawItemHandle				= events.GetEvent( sdUIEventInfo( LE_DRAW_ITEM, 0 ) );
	drawItemBackHandle			= events.GetEvent( sdUIEventInfo( LE_DRAW_ITEM_BACKGROUND, 0 ) );
	drawColumnHandle			= events.GetEvent( sdUIEventInfo( LE_DRAW_COLUMN, 0 ) );
	drawSelectedItemBackHandle	= events.GetEvent( sdUIEventInfo( LE_DRAW_SELECTED_BACKGROUND, 0 ) );
}

/*
============
sdUIList::DrawLocal
============
*/
void sdUIList::DrawLocal() {
	if( !PreDraw() ) {
		return;
	}

	const float border = ( borderWidth + internalBorderWidth );
	const float headerHeight = GetHeaderHeight();

	const int selectedItem = idMath::Ftoi( currentSelection );

	float xCoord = cachedClientRect.x + border;

	idVec4 borderRect( cachedClientRect.x, cachedClientRect.y + headerHeight, cachedClientRect.z, cachedClientRect.w - headerHeight );
	DrawBackground( borderRect );
	
	bool selected = false;
	int active = idMath::Ftoi( activeColumn );

	int now = GetUI()->GetCurrentTime();

	deviceContext->SetFont( cachedFontHandle );

	bool showHeadings		= TestFlag( LF_SHOW_HEADINGS );
	bool clip				= TestFlag( WF_CLIP_TO_RECT );
	bool truncateText		= TestFlag( WF_TRUNCATE_TEXT );
	bool truncateColumns	= TestFlag( LF_TRUNCATE_COLUMNS );
	int extraTextFlags		= TestFlag( LF_VARIABLE_HEIGHT_ROWS ) ? DTF_WORDWRAP : 0;
	int removeFlags			= extraTextFlags & DTF_WORDWRAP ? DTF_SINGLELINE : 0;

	if( TestFlag( WF_DROPSHADOW ) ) {
		extraTextFlags |= DTF_DROPSHADOW;
	}

	const wchar_t* truncationText = L"...";
	int truncationWidth;
	int truncationHeight;
	if( truncateText || truncateColumns ) {
		deviceContext->GetTextDimensions( truncationText, sdBounds2D( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT ), DTF_SINGLELINE | DTF_VCENTER | DTF_CENTER, cachedFontHandle, fontSize, truncationWidth, truncationHeight );
	}

	sdWStringBuilder_Heap builder;

	bool firstColumn = true;

	// draw each column's row elements
	sdBounds2D clipRect( cachedClientRect.x, cachedClientRect.y + border + headerHeight, cachedClientRect.z, cachedClientRect.w - ( border + internalBorderWidth + headerHeight ) );

	float* columnWidths = static_cast< float* >( _alloca( columns.Num() * sizeof( float ) ) );

	CalcColumnWidths( columnWidths );

	for ( int i = 0; i < columns.Num(); i++ ) {
		if ( columnWidths[ i ] <= 0.0f ) {
			continue;
		}

		sdUIListColumn* column = columns[ i ];

		// column headings
		if ( showHeadings ) {
			sdBounds2D headerRect( xCoord, cachedClientRect.y, columnWidths[ i ], headerHeight );
			bool continueDrawing = true;
			if( column->flags.customDraw ) {
				GetUI()->PushScriptVar( i );
				GetUI()->PushScriptVar( headerRect.ToVec4() );
				if( drawColumnHandle.IsValid() && RunEventHandle( drawColumnHandle ) ) {
					GetUI()->PopScriptVar( continueDrawing );
				}
				GetUI()->ClearScriptStack();
			}
			if( continueDrawing ) {
				const wchar_t* title = column->text.c_str();
				if( title[ 0 ] == L'\0' && column->textHandle != -1 ) {
					const sdDeclLocStr* locStr = declHolder.declLocStrType.LocalFindByIndex( column->textHandle );
					title = locStr->GetText();
				}

				if( title[ 0 ] != '\0' ) {
					if( truncateText || truncateColumns ) {
						ShortenText( title, headerRect, cachedFontHandle, column->textFlags, columnFontSize, truncationText, truncationWidth, builder );
						title = builder.c_str();
					}

					int flags = column->textFlags;
					if( TestFlag( WF_DROPSHADOW ) ) {
						flags |= DTF_DROPSHADOW;
					}
					DrawText( title, column->transition.foreColor.Evaluate( now ), columnFontSize, headerRect, cachedFontHandle, flags );
				}
			}
		}

		int firstVisibleItem = GetFirstVisibleItem();
		if( clip ) {
			deviceContext->PushClipRect( clipRect );
		}

		if ( firstVisibleItem >= 0 ) {
			int lastVisibleItem = GetLastVisibleItem( firstVisibleItem );			

			for( int j = firstVisibleItem; j <= lastVisibleItem; j++ ) {
				sdUIListItem* item			= column->items[ indices[ j ] ];
				sdTransition& transition	= column->itemTransitions[ indices[ j ] ];

				selected = ( j == selectedItem );				

				idVec4 itemRect;
				GetItemRect( j, i, itemRect, columnWidths );

				if( i >= 1 ) {
					itemRect.x += columnBorder;
					itemRect.z -= columnBorder;
				}

				// draw selection highlight background
				if ( firstColumn ) {
					idVec4 rect = itemRect;

					for( int col = i + 1; col < columns.Num(); col++ ) {
						rect.z += columnWidths[ col ];
					}					
					
					GetUI()->PushScriptVar( rect );
					if( selected && drawSelectedItemBackHandle.IsValid() ) {
						GetUI()->PushScriptVar( j );
						RunEventHandle( drawSelectedItemBackHandle );
					} else if( drawItemBackHandle.IsValid() ) {						
						GetUI()->PushScriptVar( transition.backColor.Evaluate( now ) );
						GetUI()->PushScriptVar( j );
						RunEventHandle( drawItemBackHandle );
					}
					GetUI()->ClearScriptStack();
				}

				// script-controlled drawing
				if( item->flags.customDraw ) {										
					GetUI()->PushScriptVar( i );
					GetUI()->PushScriptVar( j );
					
					if( RunEventHandle( drawItemHandle ) ) {
						bool continueDrawing;
						GetUI()->PopScriptVar( continueDrawing );
						if( !continueDrawing ) {
							continue;
						}
					}
				}

				// draw icon
				if( item->part.mi.material != NULL ) {
					DrawItemMaterial( j, i, itemRect, transition.foreColor.Evaluate( now ), false );
					continue;
				}

				const wchar_t* text = item->text.c_str();
				if( text[ 0 ] == L'\0' && item->textHandle != -1 ) {
					const sdDeclLocStr* locStr = declHolder.declLocStrType.LocalFindByIndex( item->textHandle );
					text = locStr->GetText();
				}

				if ( text[ 0 ] != L'\0' ) {
					sdBounds2D itemBounds( itemRect.x, itemRect.y, itemRect.z, itemRect.w );
					if( truncateText ) {
						ShortenText( text, itemBounds, cachedFontHandle, item->textFlags, fontSize, truncationText, truncationWidth, builder );
						text = builder.c_str();
					}

					DrawText( text, selected ? selectedItemForeColor.GetValue() : transition.foreColor.Evaluate( now ), fontSize, itemBounds, cachedFontHandle, ( item->textFlags | extraTextFlags ) & ~removeFlags );
				}
			}			
			firstColumn = false;
		}

		if( clip ) {
			deviceContext->PopClipRect();
		}

		xCoord += columnWidths[ i ];
	}

	PostDraw();

	if ( borderWidth > 0.0f ) {
		deviceContext->DrawClippedBox( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w, borderWidth, borderColor );
	}
}

/*
============
sdUIList::ApplyLayout
============
*/
void sdUIList::ApplyLayout() {
	if( windowState.recalculateLayout ) {
		BeginLayout();

		if( cachedFontHandle == -1 ) {
			gameLocal.Warning( "%s: '%s' has no font set", GetUI()->GetName(), name.GetValue().c_str() );
		}

		columnHeight.SetReadOnly( false );
		columnHeight = 0.0f;

		int numItems = 0;
		for( int i = 0; i < columns.Num(); i++ ) {
			sdUIListColumn* column = columns[ i ];

			numItems = Max( numItems, column->items.Num() );
		}

		if ( TestFlag( LF_SHOW_HEADINGS ) ) {
			columnHeight = deviceContext->GetFontHeight( cachedFontHandle, columnFontSize ) + 2.0f;
		}

		columnHeight.SetReadOnly( true );

		cachedClientRect = clientRect;	
		cachedClientRect.ToVec2() += CalcWorldOffset();	

		if( TestFlag( LF_VARIABLE_HEIGHT_ROWS ) )  {
			float* columnWidths = static_cast< float* >( _alloca( columns.Num() * sizeof( float ) ) );
			CalcColumnWidths( columnWidths );
			
			idVec4 rect;
			int height;
			if( fixedRowHeight > 0.0f ) {
				height = fixedRowHeight;
			} else {
				height = deviceContext->GetFontHeight( cachedFontHandle, fontSize );
			}

			for( int i = 0; i < columns.Num(); i++ ) {
				sdUIListColumn* c = columns[ i ];
				for( int j = 0; j < c->items.Num(); j++ ) {
					const sdUIListItem& item = *c->items[ j ];
					
					// reset to default height first
					if( i == 0 ) {
						rowHeights[ j ] = height;
					}

					GetItemRect( j, i, rect, columnWidths );
					rect.w = height;

					int w, h;
					const wchar_t* text = item.textHandle != -1 ? declHolder.declLocStrType.LocalFindByIndex( item.textHandle )->GetText() : item.text.c_str();
					if( text[ 0 ] != L'\0' ) {
						deviceContext->GetTextDimensions( text, sdBounds2D( rect ), DTF_LEFT | DTF_WORDWRAP | DTF_VCENTER, cachedFontHandle, fontSize, w, h );
						
						// keep the element as a multiple of the fixed row height
						if( fixedRowHeight > 0.0f ) {
							float num = idMath::Floor( ( h / fixedRowHeight ) + 0.5f );
							h = idMath::Ftoi( num * fixedRowHeight );
						}
					} else {
						h = height;
					}
					
					rowHeights[ j ] = Max( rowHeights[ j ], h );
				}
			}	
		} else {
			if( fixedRowHeight > 0.0f ) {
				for( int i = 0; i < rowHeights.Num(); i++ ) {
					rowHeights[ i ] = fixedRowHeight;
				}		
			} else {
				int fontHeight = Max( 2, deviceContext->GetFontHeight( cachedFontHandle, fontSize ) );
				for( int i = 0; i < rowHeights.Num(); i++ ) {
					rowHeights[ i ] = fontHeight;
				}
			}
		}	

		
		bool updateChildren = false;
		if ( TestFlag( WF_AUTO_SIZE_HEIGHT ) ) {
			cachedClientRect.w = idMath::ClampFloat( 2.0f, cachedClientRect.w, ( 2.0f * internalBorderWidth ) + columnHeight + GetTotalHeight() );
		}
		
		scrollAmountMax.SetReadOnly( false );
		scrollAmountMax = GetMaxScrollAmount();
		scrollAmountMax.SetReadOnly( true );

		pageSize.SetReadOnly( false );
		pageSize = GetVisibleHeight();
		pageSize.SetReadOnly( true );

		if( idMath::Fabs( sizeLastColumnDelta ) > 0.0f ) {
			float* columnWidths = static_cast< float* >( _alloca( columns.Num() * sizeof( float ) ) );
			CalcColumnWidths( columnWidths );

			int lastVisible = columns.Num() - 1;

			while( lastVisible >= 0 ){
				if( columnWidths[ lastVisible ] > 0.0f ) {
					break;
				}
				lastVisible--;
			}

			float totalWidth = 0.0f;

			int i;
			for( i = 0; i < lastVisible; i++ ) {
				totalWidth += columnWidths[ i ];
			}

			// resize the last visible column
			float newWidth = cachedClientRect.z - totalWidth - sizeLastColumnDelta;
			if( lastVisible >= 0 && lastVisible < columns.Num() ) {
				if( columns[ lastVisible ]->flags.widthPercent ) {
					float constantColumnWidths = 0.0f;
					int i;
					for ( i = 0; i < columns.Num(); i++ ) {
						sdUIListColumn* column = columns[ i ];
						if( !column->flags.widthPercent ) {
							constantColumnWidths += column->baseWidth;
						}
					}
					float percent = newWidth / ( cachedClientRect.z - constantColumnWidths );
					columns[ lastVisible ]->baseWidth = percent;
				} else {
					columns[ lastVisible ]->baseWidth = newWidth;
				}
			}
			sizeLastColumnDelta = 0.0f;
		}

		if( scrollToItem != -1 ) {
			ScrollToItem( scrollToItem );
			scrollToItem = -1;
		}

		if( storedScrollAmount >= 0.0f ) {
			scrollAmount = idMath::ClampFloat( 0.0f, GetMaxScrollAmount(), storedScrollAmount );
			storedScrollAmount = -1.0f;
		}

		EndLayout();
	}
	sdUIObject::ApplyLayout();
}

/*
============
sdUIList::CheckHeaderClick
============
*/
bool sdUIList::CheckHeaderClick( const sdSysEvent* event, const idVec2& point ) {
	if( !TestFlag( LF_SHOW_HEADINGS ) ) {
		return false;
	}

	idVec4 rect;
	int selectedColumn = -1;
	float* columnWidths = static_cast< float* >( _alloca( columns.Num() * sizeof( float ) ) );
	CalcColumnWidths( columnWidths );

	for( int i = 0; i < columns.Num(); i++ ) {
		GetHeaderColumnRect( i, rect, columnWidths );
		sdBounds2D bounds( rect.x, rect.y, rect.z, rect.w );
		if( bounds.ContainsPoint( point )) {
			selectedColumn = i;
			break;
		}
	}

	if( selectedColumn > -1 && TestFlag( LF_COLUMN_SORT ) ) {
		if( columns[ selectedColumn ]->flags.allowSort ) {
			int active = idMath::Ftoi( activeColumn );
			if( active == selectedColumn ) {
				columns[ active ]->flags.sortDescending ^= true;
				activeColumnSort = columns[ active ]->flags.sortDescending ? -1.0f : 1.0f;
			}
			activeColumn = selectedColumn;
			Sort();
		}		
	}
	if( selectedColumn > -1 ) {
		RunEvent( sdUIEventInfo( LE_CLICK_COLUMN, 0 ) );
	}

	return selectedColumn != -1;
}

/*
============
sdUIList::CheckItemClick
============
*/
bool sdUIList::CheckItemClick( const sdSysEvent* event, const idVec2& point ) {
	idVec4 rect;

	for( int j = 0; j < GetNumItems(); j++ ) {
		GetItemRect( j, rect );
		sdBounds2D bounds( rect.x, rect.y, rect.z, rect.w );

		if(	bounds.ContainsPoint( point )) {
			currentSelection = j;	
			return true;
		}					
	}
	return false;
}

/*
============
sdUIList::HandleKeyInput
============
*/
bool sdUIList::HandleKeyInput( const sdSysEvent* event ) {
	if ( TestFlag( LF_NO_KEY_EVENTS ) ) {
		return false;
	}

	bool retVal = false;

	if ( event->IsMouseButtonEvent() ) {
		mouseButton_t mb = event->GetMouseButton();

		if( GetNumItems() > 0 ) {
			int index;
			switch( mb ) {
				case M_MWHEELUP:
					index = idMath::ClampInt( 0, GetNumItems() - 1, GetFirstVisibleItem() );
					Scroll( -rowHeights[ index ] );
					retVal = true;
					break;
				case M_MWHEELDOWN:
					index = idMath::ClampInt( 0, GetNumItems() - 1, GetLastVisibleItem( GetFirstVisibleItem() ) );
					Scroll( rowHeights[ index ] );
					retVal = true;
					break;
			}
		}
	} else if ( event->IsKeyEvent() ) {
		keyNum_t key = event->GetKey();

		switch( key ) {
			case K_UPARROW:
			case K_KP_UPARROW:
				if( currentSelection > 0 ) {
					currentSelection = Max( 0, idMath::Ftoi( currentSelection - 1.0f ) );
				}
				retVal = true;
				break;
			case K_DOWNARROW:
			case K_KP_DOWNARROW:
				currentSelection = Min( GetNumItems() - 1, idMath::Ftoi( currentSelection + 1.0f ));
				retVal = true;
				break;
			case K_HOME:
			case K_KP_HOME:
				currentSelection = GetNumItems() ? 0 : -1;
				retVal = true;
				break;
			case K_END:
			case K_KP_END:
				currentSelection = GetNumItems() - 1;
				retVal = true;
				break;
			case K_PGUP:
			case K_KP_PGUP:
				scrollAmount = idMath::ClampFloat( 0.0f, GetMaxScrollAmount(), scrollAmount - pageSize );
				retVal = true;
				break;
			case K_PGDN:
			case K_KP_PGDN:
				scrollAmount = idMath::ClampFloat( 0.0f, GetMaxScrollAmount(), scrollAmount + pageSize );
				retVal = true;
				break;
		}
	}
	return retVal;
}

/*
============
sdUIList::PostEvent
============
*/
bool sdUIList::PostEvent( const sdSysEvent* event ) {
	if( windowState.fullyClipped || !IsVisible() || !ParentsAllowEventPosting() ) {
		return false;
	}

	idVec2 point( ui->cursorPos );
	
	bool retVal = false;
	bool hitItem = false;	

	if ( IsMouseClick( event ) && !cachedClippedRect.ContainsPoint( point ) ) {
		return false;
	}	

	if( columns.Num() > 0 ) {
		if( event->IsMouseEvent() ) {
			if( TestFlag( LF_HOT_TRACK ) ) {
				retVal |= CheckItemClick( event, point );
			}

			bool hitColumn =  false;
			// check for mouse enter/exit of column headings
			if( TestFlag( LF_SHOW_HEADINGS ) ) {
				hitColumn = CheckHeaderMouseOver( cachedClippedRect, point );
			}
			if( !hitColumn ) {
				CheckItemMouseOver( cachedClippedRect, point );
			}
		} else if ( event->IsMouseButtonEvent() ) {
			if ( event->IsButtonDown() ) {
				mouseButton_t mb = event->GetMouseButton();

				// handle item selection
				if ( mb >= M_MOUSE1 && mb <= M_MOUSE12 ) {
					bool result = CheckItemClick( event, point );
					retVal |= result;
					retVal |= CheckHeaderClick( event, point );
					if ( !result && !TestFlag( LF_NO_NULL_SELECTION )) {
						currentSelection = -1;
					}
				} else if ( GetUI()->IsFocused( this )  ) {					
					retVal |= HandleKeyInput( event );
				}
			}
		} else if ( event->IsKeyEvent() ) {
			if ( event->IsKeyDown() ) {
				if ( GetUI()->IsFocused( this )  ) {					
					retVal |= HandleKeyInput( event );
				}
			}

		}
	}
	retVal |= sdUIWindow::PostEvent( event );

	return retVal;
}

/*
============
sdUIList::CheckHeaderMouseOver
============
*/
bool sdUIList::CheckHeaderMouseOver( const sdBounds2D& windowBounds, const idVec2& point ) {
	bool retVal = false;

	if( windowBounds.ContainsPoint( point ) ) {
		float* columnWidths = static_cast< float* >( _alloca( columns.Num() * sizeof( float ) ) );
		CalcColumnWidths( columnWidths );

		idVec4 rect;
		for( int i = 0; i < columns.Num(); i++ ) {
			GetHeaderColumnRect( i, rect, columnWidths );
			sdBounds2D bounds( rect.x, rect.y, rect.z, rect.w );
			if( bounds.ContainsPoint( point ) ) {
				if( !columns[ i ]->flags.mouseFocused ) {
					GetUI()->PushScriptVar( i );
					RunEvent( sdUIEventInfo( LE_ENTER_COLUMN, 0 ) );
					columns[ i ]->flags.mouseFocused = true;
					retVal = true;
				}
			} else if( columns[ i ]->flags.mouseFocused ) {
				GetUI()->PushScriptVar( i );
				RunEvent( sdUIEventInfo( LE_EXIT_COLUMN, 0 ) );
				columns[ i ]->flags.mouseFocused = false;
			}			
		}
	} else {
		for( int i = 0; i < columns.Num(); i++ ) {
			if( columns[ i ]->flags.mouseFocused ) {
				GetUI()->PushScriptVar( i );
				RunEvent( sdUIEventInfo( LE_EXIT_COLUMN, 0 ) );
				columns[ i ]->flags.mouseFocused = false;
			}
		}
	}
	return retVal;
}

/*
============
sdUIList::CheckItemMouseOver
============
*/
bool sdUIList::CheckItemMouseOver( const sdBounds2D& windowBounds, const idVec2& point ) {
	bool retVal = false;	

	if( windowBounds.ContainsPoint( point ) ) {
		idVec4 rect;
		int firstVisible = GetFirstVisibleItem();
		int lastVisibleItem = firstVisible;
		if( firstVisible >=0 ) {
			lastVisibleItem = GetLastVisibleItem( firstVisible );
		}

		float* columnWidths = static_cast< float* >( _alloca( columns.Num() * sizeof( float ) ) );
		CalcColumnWidths( columnWidths );
		float baseX = cachedClientRect.x + ( 2.0f * ( borderWidth + internalBorderWidth ) ) - columnBorder;

		for( int i = 0; i < columns.Num(); baseX += columnWidths[ i ], i++ ) {
			sdUIListColumn* col = columns[ i ];	
			
			for( int j = firstVisible; j <= lastVisibleItem; j++ ) {
				sdUIListItem* item = col->items[ j ];
				if( point.x < baseX || point.x > ( baseX + columnWidths[ i ] ) ) {
					if( item->flags.mouseFocused ) {					
						GetUI()->PushScriptVar( i );
						GetUI()->PushScriptVar( j );
						RunEvent( sdUIEventInfo( LE_EXIT_ITEM, 0 ) );
						GetUI()->ClearScriptStack();
						item->flags.mouseFocused = false;
					}
					continue;
				}	
				
				GetItemRect( j, i, rect, columnWidths );

				sdBounds2D bounds( rect.x, rect.y, rect.z, rect.w );
				if( bounds.ContainsPoint( point ) ) {
					if( !item->flags.mouseFocused ) {						
						GetUI()->PushScriptVar( i );
						GetUI()->PushScriptVar( j );
						RunEvent( sdUIEventInfo( LE_ENTER_ITEM, 0 ) );
						GetUI()->ClearScriptStack();
						item->flags.mouseFocused = true;
						retVal = true;
					}
				} else if( item->flags.mouseFocused ) {					
					GetUI()->PushScriptVar( i );
					GetUI()->PushScriptVar( j );
					RunEvent( sdUIEventInfo( LE_EXIT_ITEM, 0 ) );
					GetUI()->ClearScriptStack();
					item->flags.mouseFocused = false;
				}
			}
		}
	} else {
		for( int i = 0; i < columns.Num(); i++ ) {
			sdUIListColumn* col = columns[ i ];
			for( int j = 0; j < col->items.Num(); j++ ) {
				sdUIListItem* item = col->items[ j ];
				if( item->flags.mouseFocused ) {					
					GetUI()->PushScriptVar( i );
					GetUI()->PushScriptVar( j );
					RunEvent( sdUIEventInfo( LE_EXIT_ITEM, 0 ) );
					GetUI()->ClearScriptStack();
					item->flags.mouseFocused = false;
				}
			}
		}
	}
	return retVal;
}

/*
============
sdUIList::InsertItem
============
*/
int sdUIList::InsertItem( sdUIWindow* list, const wchar_t* name, int item, int column ) {
	assert( list );

	sdUIFunctionStack stack;	
	stack.Push( column );				// column
	stack.Push( item );					// index
	stack.Push( name );

	if ( !list->RunNamedMethod( "insertItem", stack ) ) {
		return -1;
	}

	float retVal;
	stack.Pop( retVal );

	return idMath::Ftoi( retVal );
}

/*
============
sdUIList::DeleteItem
============
*/
void sdUIList::DeleteItem( sdUIWindow* list, int item ) {
	assert( list );

	sdUIFunctionStack stack;	
	stack.Push( item );					// index

	list->RunNamedMethod( "deleteItem", stack );
}

/*
============
sdUIList::SetItemText
============
*/
void sdUIList::SetItemText( sdUIWindow* list, const wchar_t* name, int item, int column ) {
	assert( list );

	sdUIFunctionStack stack;	
	stack.Push( column );				// column
	stack.Push( item );					// index
	stack.Push( name );

	list->RunNamedMethod( "setItemText", stack );	
}

/*
============
sdUIList::SetItemForeColor
============
*/
void sdUIList::SetItemForeColor( sdUIWindow* list, const idVec4& color, int item, int column ) {
	assert( list );

	sdUIFunctionStack stack;	
	stack.Push( column );				// column
	stack.Push( item );					// index
	stack.Push( color );

	list->RunNamedMethod( "setItemForeColor", stack );
}

/*
============
sdUIList::SetItemBackColor
============
*/
void sdUIList::SetItemBackColor( sdUIWindow* list, const idVec4& color, int item, int column ) {
	assert( list );

	sdUIFunctionStack stack;	
	stack.Push( column );				// column
	stack.Push( item );					// index
	stack.Push( color );

	list->RunNamedMethod( "setItemBackColor", stack );
}

/*
============
sdUIList::ClearItems
============
*/
void sdUIList::ClearItems( sdUIWindow* list ) {
	assert( list );

	sdUIFunctionStack s;
	list->RunNamedMethod( "clearItems", s );
}

/*
============
sdUIList::ClearColumns
============
*/
void sdUIList::ClearColumns( sdUIWindow* list ) {
	assert( list );

	sdUIFunctionStack s;
	list->RunNamedMethod( "clearColumns", s );
}

/*
============
sdUIList::SetItemIcon
============
*/
void sdUIList::SetItemIcon( sdUIWindow* list, const char* iconMaterial, int item, int column  ) {
	assert( list );

	sdUIFunctionStack stack;
	stack.Push( column );
	stack.Push( item );
	stack.Push( iconMaterial );	
	
	list->RunNamedMethod( "setItemIcon", stack );
}

/*
============
sdUIList::EnumerateEvents
============
*/
void sdUIList::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	if ( !idStr::Icmp( name, "onSelectItem" ) ) {
		events.Append( sdUIEventInfo( LE_SELECT_ITEM, 0 ) );
		return;
	}	
	if ( !idStr::Icmp( name, "onItemAdded" ) ) {
		events.Append( sdUIEventInfo( LE_ITEM_ADDED, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onItemRemoved" ) ) {
		events.Append( sdUIEventInfo( LE_ITEM_REMOVED, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onDrawSelectedBackground" ) ) {
		events.Append( sdUIEventInfo( LE_DRAW_SELECTED_BACKGROUND, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onDrawItemBackground" ) ) {
		events.Append( sdUIEventInfo( LE_DRAW_ITEM_BACKGROUND, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onDrawItem" ) ) {
		events.Append( sdUIEventInfo( LE_DRAW_ITEM, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onDrawColumn" ) ) {
		events.Append( sdUIEventInfo( LE_DRAW_COLUMN, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onEnterColumnHeader" ) ) {
		events.Append( sdUIEventInfo( LE_ENTER_COLUMN, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onExitColumnHeader" ) ) {
		events.Append( sdUIEventInfo( LE_EXIT_COLUMN, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onClickColumnHeader" ) ) {
		events.Append( sdUIEventInfo( LE_CLICK_COLUMN, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onEnterItem" ) ) {
		events.Append( sdUIEventInfo( LE_ENTER_ITEM, 0 ) );
		return;
	}
	if ( !idStr::Icmp( name, "onExitItem" ) ) {
		events.Append( sdUIEventInfo( LE_EXIT_ITEM, 0 ) );
		return;
	}
	sdUIWindow::EnumerateEvents( name, flags, events, tokenCache );
}

/*
============
sdUIList::GetVisibleItemOffset
============
*/
float sdUIList::GetVisibleItemOffset( int item ) const {
	if( item >= GetNumItems() || item < 0 ) {
		return 0.0f;
	}

	const float headerHeight = GetHeaderHeight();
	idVec4 rect;
	GetItemRect( item, rect );

	float rectBottom = rect.y + rect.w;
	float clientRectBottom = cachedClientRect.y + cachedClientRect.w - ( borderWidth + internalBorderWidth );

	// item is above the top of the list
	if( rect.y <= cachedClientRect.y + headerHeight + ( borderWidth + internalBorderWidth ) ) {
		return rect.y - ( headerHeight + cachedClientRect.y + ( borderWidth + internalBorderWidth ) );
	}

	// item is below the bottom of the list
	if( rectBottom >= clientRectBottom ) {
		return rectBottom - clientRectBottom;
	}

	return 0.0f;
}

/*
============
sdUIList::OnFixedRowHeightChanged
============
*/
void sdUIList::OnFixedRowHeightChanged( const float oldValue, const float newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIList::OnInternalBorderChanged
============
*/
void sdUIList::OnInternalBorderChanged( const float oldValue, const float newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIList::OnActiveColumnChanged
============
*/
void sdUIList::OnActiveColumnChanged( const float oldValue, const float newValue ) {
	if( TestFlag( LF_COLUMN_SORT ) ) {
		Sort();
	}
}

/*
============
sdUIList::Sort
============
*/
void sdUIList::Sort() {
	if( columns.Num() == 0 || TestFlag( LF_DIRECT_UPDATES ) ) {
		return;
	}
	
	int numRows = columns[ 0 ]->items.Num();
	
	// this needs to be setup regardless of whether we sort or not
	int index;
	indices.AssureSize( numRows );
	for( index = 0; index < numRows; index++ ) {
		indices[ index ] = index;
	}

	// nothing to do...
	if( inBatchOperation || numRows < 2 || !TestFlag( LF_COLUMN_SORT ) ) {
		return;
	}

	int active = idMath::Ftoi( activeColumn );
	sdUIListColumn* column = active == -1 ? columns[ 0 ] : columns[ active ];	

	// quick-sort the indices based on the active column, then fix up all of the columns
	if( column->flags.dataSort ) {
		Sort( sdColumnSortData( *column ) );
	} else if( column->flags.numericSort ) {
		Sort( sdColumnSortNumeric( *column ) );
	} else {
		Sort( sdColumnSort( *column ) );
	}

	if( column->flags.sortDescending ) {
		for( index = 0; index < numRows / 2; index++ ) {
			idSwap( indices[ index ], indices[ numRows - index - 1 ] );
		}
	}
}

/*
============
sdUIList::ExtractTextFormatFromString
============
*/
void sdUIList::ExtractTextFormatFromString( const wchar_t* format, const wchar_t* indicator, int& flags ) {
	flags = DTF_SINGLELINE;

	bool horizontalSet = false;
	bool verticalSet = false;

	int length = idWStr::Length( format );
	int offset = 0;
	while( offset <= length ) {
		if( format[ offset ] != L'<' ) {
			offset++;
			continue;
		}
		idWLexer src( format + offset, idWStr::Length( format + offset ), "ExtractTextFormatFromString", LEXFL_NOERRORS );
		
		while( true ) {
			if( !src.ReadToken( &token ) ) {
				break;
			}
			if( token.Cmp( L">" ) == 0 ) {
				break;
			}
			if( token.Cmp( L"<" ) != 0 ) {
				continue;
			}
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad text format string", name.GetValue().c_str() );
				break;
			}
			if( token.Icmp( indicator ) != 0 ) {
				continue;
			}
			if( !src.ExpectTokenString( L"=" )) {
				break;
			}
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad text format string", name.GetValue().c_str() );
				break;
			}
			while( src.ReadToken( &token ) ) {
				if( token.Cmp( L"," ) == 0 ) {
					continue;
				}
				if( token.Icmp( L"right" ) == 0 ) {
					flags |= DTF_RIGHT;
					horizontalSet = true;
					continue;
				}
				if( token.Icmp( L"left" ) == 0 ) {
					flags |= DTF_LEFT;
					horizontalSet = true;
					continue;
				}
				if( token.Icmp( L"center" ) == 0 ) {
					flags |= DTF_CENTER;
					horizontalSet = true;
					continue;
				}
				if( token.Icmp( L"top" ) == 0 ) {
					flags |= DTF_TOP;
					verticalSet = true;
					continue;
				}
				if( token.Icmp( L"bottom" ) == 0 ) {
					flags |= DTF_BOTTOM;
					verticalSet = true;
					continue;
				}
				if( token.Icmp( L"vcenter" ) == 0 ) {
					flags |= DTF_VCENTER;
					verticalSet = true;
					continue;
				}
				if( token.Cmp( L">" ) == 0 ) {
					break;
				}
			}			
			break;
		}
		offset += src.GetFileOffset();
	}	

	if( !horizontalSet ) {
		flags |= DTF_LEFT;
	}

	if( !verticalSet ) {
		flags |= DTF_BOTTOM;
	}
}

/*
============
sdUIList::ExtractColumnWidthFromString
============
*/
void sdUIList::ExtractColumnWidthFromString( const wchar_t* format, const wchar_t* indicator, sdUIListColumn& column ) {
	column.flags.widthPercent = false;

	int length = idWStr::Length( format );
	int offset = 0;
	while( offset <= length ) {
		if( format[ offset ] != L'<' ) {
			offset++;
			continue;
		}

		idWLexer src( format + offset, idWStr::Length( format + offset ), "ExtractColumnWidthFromString", LEXFL_NOERRORS );

		while( true ) {
			if( !src.ReadToken( &token ) ) {
				break;
			}
			if( token.Cmp( L">" ) == 0 ) {
				break;
			}
			if( token.Cmp( L"<" ) != 0 ) {
				continue;
			}
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad format string '%ls'", name.GetValue().c_str(), format );
				break;
			}
			if( token.Icmp( indicator ) != 0 ) {
				continue;
			}
			if( !src.ExpectTokenString( L"=" )) {
				break;
			}
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad format string '%ls'", name.GetValue().c_str(), format );
				break;
			}

			if( token.type != TT_NUMBER ) {
				src.Warning( "'%s' bad format string, expected number but found '%ls'", name.GetValue().c_str(), format );
				break;
			}

			column.baseWidth = token.GetFloatValue();
			if( src.ReadToken( &token ) ) {
				if( token.Cmp( L"%" ) == 0 ) {
					column.flags.widthPercent = true;
					column.baseWidth /= 100.0f;
				} else {
					src.Warning( "'%s' bad format string, unknown token '%ls'", name.GetValue().c_str(), format );
					break;
				}
			}
			if( !src.ExpectTokenString( L">" )) {
				src.Warning( "'%s' bad text format string", name.GetValue().c_str() );			
			}
			break;
		}
		offset += src.GetFileOffset();
	}
}

/*
============
sdUIList::ExtractLocalizedTextFromString
============
*/
void sdUIList::ExtractLocalizedTextFromString( const wchar_t* format, const wchar_t* indicator, int& handle ) {
	handle = -1;

	int length = idWStr::Length( format );
	int offset = 0;
	while( offset <= length ) {
		if( format[ offset ] != L'<' ) {
			offset++;
			continue;
		}

		idWLexer src( format + offset, idWStr::Length( format + offset ), "ExtractLocalizedTextFromString", LEXFL_NOERRORS );
		
		while( true ) {
			if( !src.ReadToken( &token ) ) {
				break;
			}
			if( token.Cmp( L">" ) == 0 ) {
				break;
			}
			if( token.Cmp( L"<" ) != 0 ) {
				continue;
			}
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad format string '%ls'", name.GetValue().c_str(), format );
				break;
			}
			if( token.Icmp( indicator ) != 0 ) {
				continue;
			}
			if( !src.ExpectTokenString( L"=" )) {
				break;
			}
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad format string '%ls'", name.GetValue().c_str(), format );
				break;
			}

			const sdDeclLocStr* loc = declHolder.declLocStrType.LocalFind( va( "%ls", token.c_str() ) );
			handle = loc->Index();
			
			if( !src.ExpectTokenString( L">" )) {
				src.Warning( "'%s' bad format string '%ls'", name.GetValue().c_str(), format );			
			}	
			break;
		}
		offset += src.GetFileOffset();
	}
}

/*
============
sdUIList::ExtractColorFromString
============
*/
bool sdUIList::ExtractColorFromString( const wchar_t* format, const wchar_t* indicator, idVec4& color ) {
	int length = idWStr::Length( format );
	int offset = 0;
	bool set = false;
	while( offset <= length ) {
		if( format[ offset ] != L'<' ) {
			offset++;
			continue;
		}

		idWLexer src( format + offset, idWStr::Length( format + offset ), "ExtractColorFromString", LEXFL_NOERRORS );

		while( true ) {
			if( !src.ReadToken( &token ) ) {
				break;
			}
			if( token.Cmp( L">" ) == 0 ) {
				break;
			}
			if( token.Cmp( L"<" ) != 0 ) {
				continue;
			}		
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad format string '%ls'", name.GetValue().c_str(), format );
				break;
			}
			if( token.Icmp( indicator ) != 0 ) {
				continue;
			}
			if( !src.ExpectTokenString( L"=" )) {
				break;
			}
			idVec4 temp;
			int i = 0;
			while( true ) {
				if( i >= 4 ) {
					break;
				}
				if( !src.ReadToken( &token )) {
					break;
				}
				if( !token.Cmp( L"," )) {
					continue;
				}
				if( !token.Cmp( L">" )) {
					break;
				}
				temp[ i ] = token.GetFloatValue();
				i++;
			}
			color = temp;
			set = true;
		}
		offset += src.GetFileOffset();
	}
	return set;
}

/*
============
sdUIList::ExtractMaterialFromString
============
*/
void sdUIList::ExtractMaterialFromString( const wchar_t* format, const wchar_t* indicator, uiDrawPart_t& part ) {
	part.mi.Clear();
	part.height = part.width = 0;

	int length = idWStr::Length( format );
	int offset = 0;
	while( offset <= length ) {
		if( format[ offset ] != L'<' ) {
			offset++;
			continue;
		}

		idWLexer src( format + offset, idWStr::Length( format + offset ), "ExtractMaterialFromString", LEXFL_NOERRORS );

		while( true ) {
			if( !src.ReadToken( &token ) ) {
				break;
			}
			if( token.Cmp( L">" ) == 0 ) {
				break;
			}
			if( token.Cmp( L"<" ) != 0 ) {
				continue;
			}
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad format string '%ls'", name.GetValue().c_str(), format );
				break;
			}
			if( token.Icmp( indicator ) != 0 ) {
				continue;
			}
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad format string '%ls'", name.GetValue().c_str(), format );
				break;
			}

			if ( token.Icmp( L"=" ) != 0 ) {
				break;
			}
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad format string '%ls'", name.GetValue().c_str(), format );
				break;
			}
			if( token.Length() ) {
				idStr material;
				
				bool globalLookup;
				bool literal;
				uiDrawMode_e mode;	// FIXME: support this at some point
				int offset = sdUserInterfaceLocal::ParseMaterial( va( "%ls", token.c_str() ), material, globalLookup, literal, mode );
				
				if( globalLookup ) {
					material = "::" + material;
				}

				if( literal ) {
					GetUI()->LookupMaterial( va( "literal: %ls", token.c_str() + offset ), part.mi );
				} else if( ( material.Length() && !globalLookup ) || ( material.Length() > 2 && globalLookup ) ) {
					GetUI()->LookupMaterial( material, part.mi );
				}
				
			}
			if( !src.ExpectTokenString( L">" )) {
				src.Warning( "'%s' bad material format string", name.GetValue().c_str() );
				break;
			}	
		}
		offset += src.GetFileOffset();
	}
}

/*
============
sdUIList::StripFormatting
============
*/
void sdUIList::StripFormatting( idWStr& string ) {
	int start = string.Find( L"<" );
	while( start != idWStr::INVALID_POSITION ) {
		int end = string.Find( L">", false, start + 1 );
		if( end == idWStr::INVALID_POSITION ) {
			break;
		}
		string.EraseRange( start, end - start + 1 );
		start = string.Find( L"<" );		
	}
	string.Replace( L"&lt", L"<" );
	string.Replace( L"&gt", L">" );
}

/*
============
sdUIList::OnCurrentItemChanged
============
*/
void sdUIList::OnCurrentItemChanged( const float oldValue, const float newValue ) {
	if ( newValue >= 0.0f ) {
		if( TestFlag( LF_AUTO_SCROLL_TO_SELECTION )) {
			scrollToItem = idMath::FtoiFast( newValue );
			MakeLayoutDirty();
		}
		RunEvent( sdUIEventInfo( LE_SELECT_ITEM, 0 ) );
	}
	if ( sdUserInterfaceLocal::g_debugGUIEvents.GetInteger() > 2 && ui ) {
		gameLocal.Printf( "GUI '%s', window '%s',selected item '%i'\n", ui->GetName(), name.GetValue().c_str(), idMath::Ftoi( currentSelection ));
	}
	lastClickTime = -1;
}

/*
============
sdUIList::SelectItem
============
*/
void sdUIList::SelectItem( int item ) {
	if( columns.Num() == 0 || item >= columns[ 0 ]->items.Num() ) {
		gameLocal.Warning( "SelectItem: '%s' %i out of range", name.GetValue().c_str(), item );
		return;
	}
	currentSelection = item; 
}

/*
============
sdUIList::SetItemDataInt
============
*/
void sdUIList::SetItemDataInt( int integer, int item, int column, bool direct ) {
	sdUIListItem* listItem = GetItem( item, column, "SetItemDataInt", direct );
	if( listItem != NULL ) {
		listItem->data.integer = integer;
	}	
}

/*
============
sdUIList::SetItemDataPtr
============
*/
void sdUIList::SetItemDataPtr( void* ptr, int item, int column, bool direct ) {
	sdUIListItem* listItem = GetItem( item, column, "SetItemDataPtr", direct );
	if( listItem != NULL ) {
		listItem->data.ptr = ptr;
	}	
}

/*
============
sdUIList::GetItemDataInt
============
*/
int sdUIList::GetItemDataInt( int item, int column, bool direct ) const {
	const sdUIListItem* listItem = GetItem( item, column, "GetItemDataInt", direct );
	if( listItem != NULL ) {
		return listItem->data.integer;
	}
	return 0;
}

/*
============
sdUIList::GetItemDataPtr
============
*/
void* sdUIList::GetItemDataPtr( int item, int column, bool direct ) const {
	const sdUIListItem* listItem = GetItem( item, column, "GetItemDataPtr", direct );
	if( listItem != NULL ) {
		return listItem->data.ptr;
	}
	return NULL;
}


/*
============
sdUIList::GetItemText
============
*/
const wchar_t* sdUIList::GetItemText( int item, int column, bool direct ) const {
	// return header text
	if( item == -1 ) {
		if( column < 0 || column >= columns.Num() ) {
			gameLocal.Warning( "GetItemText: %s column %i out of range", name.GetValue().c_str(), column );
			return L"";
		}
		if( columns[ column ]->textHandle != -1 ) {
			const sdDeclLocStr* locStr = declHolder.declLocStrType.LocalFindByIndex( columns[ column ]->textHandle, false );
			if( locStr != NULL ) {
				return locStr->GetText();
			}
		}
		return columns[ column ]->text.c_str();
	}
	const sdUIListItem* listItem = GetItem( item, column, "GetItemText", direct );
	if( listItem != NULL ) {
		if( listItem->textHandle != -1 ) {
			const sdDeclLocStr* locStr = declHolder.declLocStrType.LocalFindByIndex( listItem->textHandle, false );
			if( locStr != NULL ) {
				return locStr->GetText();
			}
		}

		return listItem->text.c_str();
	}
	return L"";
}


/*
============
sdUIList::EndLevelLoad
============
*/
void sdUIList::EndLevelLoad() {
	sdUIWindow::EndLevelLoad();

	for( int c = 0; c < columns.Num(); c++ ) {
		sdUIListColumn* col = columns[ c ];
		for( int r = 0; r < col->items.Num(); r++ ) {
			sdUIListItem* item = col->items[ r ];
			sdUserInterfaceLocal::LookupPartSizes( &item->part, 1 );
		}
	}
}

/*
============
sdUIList::InitVec4Transition
============
*/
void sdUIList::InitVec4Transition( const idVec4& from, const idVec4& to, const int time, const idStr& accel, vec4Evaluator_t& evaluator ) {
	evaluator.InitLerp( GetUI()->GetCurrentTime(), GetUI()->GetCurrentTime() + idMath::Ftoi( time ), from, to );

	if( !accel.IsEmpty() ) {
		idLexer src( accel.c_str(), accel.Length(), "TransitionItemVec4", LEXFL_NOERRORS );
		idToken token;

		if( !src.ReadToken( &token ) ) {
			gameLocal.Error( "TransitionItemVec4: '%s' invalid acceleration", name.GetValue().c_str() );
			return;
		}
		bool isTable = idStr::Icmpn( token, "table://", 8 ) == 0;
		if( !isTable ) {
			idVec2 accelTimes;
			accelTimes.x = src.ParseFloat();
			src.ExpectTokenString( "," );
			accelTimes.y = src.ParseFloat();

			evaluator.InitAccelDecelEvaluation( accelTimes.x, accelTimes.y );
		} else if( token.Length() > 8 ) {
			evaluator.InitTableEvaluation( token.c_str() + 8 );
		}
	}
}

/*
============
sdUIList::ClearTransition
============
*/
void sdUIList::ClearTransition( sdTransition& transition ) {
	transition.backColor.InitConstant( backColor );
	transition.foreColor.InitConstant( foreColor );
	transition.FreeEvaluators();
}

/*
============
sdUIListItem::sdUIListItem
============
*/
sdUIList::sdUIListItem::sdUIListItem() :
	textFlags( DTF_LEFT ),
	textHandle( -1 ) {
	data.ptr = NULL;
	flags.customDraw = false;
	flags.mouseFocused = false;
}

/*
============
sdUIListItem::~sdUIListItem
============
*/
sdUIList::sdUIListItem::~sdUIListItem() {
}

/*
============
sdUIListItem::GetEvaluator
============
*/
sdUIList::vec4Evaluator_t* sdUIList::sdTransition::GetEvaluator( int index, bool allowCreate ) {
	switch( index ) {
		case LTP_FORECOLOR:
			return &foreColor;
		case LTP_BACKCOLOR:
			return &backColor;
		default:
			if( evaluators == NULL ) {
				if( !allowCreate ) {
					return NULL;
				}
				evaluators = vec4EvaluatorListAllocator.Alloc();
				evaluators->SetNum( MAX_VEC4_EVALUATORS );
			}
			return &(*evaluators)[ index ];
	}
}

/*
============
sdUIListColumn::sdUIListColumn
============
*/
sdUIList::sdUIListColumn::sdUIListColumn() :
	baseWidth( 0.0f ),
	textHandle( -1 ), 
	textFlags( DTF_LEFT | DTF_BOTTOM | DTF_SINGLELINE ) {
	flags.sortDescending = false;	
	flags.widthPercent = false;	
	flags.mouseFocused = false;
	
	ResetUserFlags();
}


/*
============
sdUIList::SetItemGranularity
============
*/
void sdUIList::SetItemGranularity( int granularity ) {
	for( int i = 0; i < columns.Num(); i++ ) {
		columns[ i ]->items.SetGranularity( granularity );
		columns[ i ]->itemTransitions.SetGranularity( granularity );
	}
}

/*
============
sdUIList::FormatItemText
============
*/
void sdUIList::FormatItemText( sdUIListItem& item, sdTransition& transition, const wchar_t* text ) {

	item.text = text;

	int now = GetUI()->GetCurrentTime();
	idVec4 localForeColor = transition.foreColor.Evaluate( now );
	idVec4 localBackColor = transition.backColor.Evaluate( now );

	if( ExtractColorFromString( text,			L"fore",		localForeColor ) ) {
		transition.foreColor.InitConstant( localForeColor );
	}
	if( ExtractColorFromString( text,			L"back",		localBackColor ) ) {
		transition.backColor.InitConstant( localBackColor );
	}
	ExtractMaterialFromString( text,		L"material",	item.part );
	ExtractTextFormatFromString( text,		L"align",		item.textFlags );
	ExtractLocalizedTextFromString( text,	L"loc",			item.textHandle );
	ExtractItemFlagsFromString( text,		L"flags",		item );	

	StripFormatting( item.text );
}


/*
============
sdUIList::ExtractItemFlagsFromString
============
*/
void sdUIList::ExtractItemFlagsFromString( const wchar_t* format, const wchar_t* indicator, sdUIListItem& item ) {
	item.flags.customDraw = false;
	int length = idWStr::Length( format );
	int offset = 0;
	while( offset <= length ) {
		if( format[ offset ] != L'<' ) {
			offset++;
			continue;
		}

		idWLexer src( format + offset, idWStr::Length( format + offset ), "ExtractItemFlagsFromString", LEXFL_NOERRORS );

		while( true ) {
			if( !src.ReadToken( &token ) ) {
				break;
			}
			if( token.Cmp( L">" ) == 0 ) {
				break;
			}
			if( token.Cmp( L"<" ) != 0 ) {
				continue;
			}
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad text format string", name.GetValue().c_str() );
				break;
			}
			if( token.Icmp( indicator ) != 0 ) {
				continue;
			}

			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad text format string", name.GetValue().c_str() );
				break;
			}
			if( token.Icmp( L"customDraw" ) == 0 ) {
				item.flags.customDraw = true;
			}

			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad text format string", name.GetValue().c_str() );
				break;
			}

			if( token.Cmp( L"," ) == 0 ) {			
				continue;
			}
			if( token.Cmp( L">" ) != 0 ) {
				src.Warning( "'%s' bad text format string", name.GetValue().c_str() );			
			}
			break;
		}
		offset += src.GetFileOffset();
	}
}

/*
============
sdUIList::DrawItemMaterial
============
*/
void sdUIList::DrawItemMaterial( int row, int col, const idVec4& rect, const idVec4& color, bool directIndex ) {
	if( col < 0 || col >= columns.Num() ) {
		return;
	}

	sdUIListColumn* column = columns[ col ];

	float* columnWidths = static_cast< float* >( _alloca( columns.Num() * sizeof( float ) ) );
	CalcColumnWidths( columnWidths );

	if( row < 0 || row >= column->items.Num() ) {
		return;
	}
	sdUIListItem* item;
	if( directIndex ) {
		item = column->items[ row ];
	} else {
		item = column->items[ indices[ row ] ];
	}

	int width = item->part.width > 0 ? item->part.width : idMath::Ftoi( columnWidths[ col ] );
	int height = item->part.height > 0 ? item->part.height : rowHeights[ row ];
	idVec4 rectLocal = rect;

	if( columnWidths[ col ] > item->part.width ) {
		if( item->textFlags & DTF_CENTER ) {
			rectLocal.x += ( columnWidths[ col ] - item->part.width ) * 0.5f;
		} else if( item->textFlags & DTF_RIGHT ) {
			rectLocal.x += ( columnWidths[ col ] - item->part.width );
		}
	}

	float drawHeight = rowHeights[ row ] - rowSpacing;
	if( drawHeight > item->part.height ) {
		if( item->textFlags & DTF_VCENTER ) {
			rectLocal.y += ( drawHeight - item->part.height ) * 0.5f;
		} else if( item->textFlags & DTF_BOTTOM ) {
			rectLocal.y += ( drawHeight - item->part.height );
		}
	}

	if ( dropShadowOffsetImage != 0.0f ) {
		DrawMaterial( item->part.mi, rectLocal.x + dropShadowOffsetImage, rectLocal.y + dropShadowOffsetImage, width, height, idVec4( 0.0f, 0.0f, 0.0f, color.w ) );
	}
	DrawMaterial( item->part.mi, rectLocal.x, rectLocal.y, width, height, color );
}

/*
============
sdUIList::CleanUserInput
============
*/
void sdUIList::CleanUserInput( idWStr& input ) {
	input.Replace( L"<" , L"&lt" );
	input.Replace( L">" , L"&gt" );
}

/*
============
sdUIList::ExtractColumnFlagsFromString
============
*/
void sdUIList::ExtractColumnFlagsFromString( const wchar_t* format, const wchar_t* indicator, sdUIListColumn& column ) {
	column.ResetUserFlags();

	int length = idWStr::Length( format );
	int offset = 0;
	while( offset <= length ) {
		if( format[ offset ] != L'<' ) {
			offset++;
			continue;
		}

		idWLexer src( format + offset, idWStr::Length( format + offset ), "ExtractColumnFlagsFromString", LEXFL_NOERRORS );

		while( true ) {
			if( !src.ReadToken( &token ) ) {
				break;
			}
			if( token.Cmp( L">" ) == 0 ) {
				break;
			}
			if( token.Cmp( L"<" ) != 0 ) {
				continue;
			}
			if( !src.ReadToken( &token ) ) {
				src.Warning( "'%s' bad text format string", name.GetValue().c_str() );
				break;
			}
			if( token.Icmp( indicator ) != 0 ) {
				continue;
			}

			while( src.ReadToken( &token ) ) {
				if( token.Cmp( L">" ) == 0 ) {
					break;
				}
				if( token.Cmp( L"," ) == 0 ) {
					continue;
				}
				if( token.Icmp( L"customDraw" ) == 0 ) {
					column.flags.customDraw = true;
					continue;
				}
				if( token.Icmp( L"numeric" ) == 0 ) {
					column.flags.numericSort = true;
					continue;
				}
				if( token.Icmp( L"noSort" ) == 0 ) {
					column.flags.allowSort = false;
					continue;
				}
				if( token.Icmp( L"dataSort" ) == 0 ) {
					column.flags.dataSort = true;
					continue;
				}
				src.Warning( "'%s' bad text format string, unknown toekn '%s'", name.GetValue().c_str(), token.c_str() );
				break;
			}

			if( token.Cmp( L">" ) != 0 ) {
				src.Warning( "'%s' bad text format string", name.GetValue().c_str() );			
			}
			break;
		}
		offset += src.GetFileOffset();
	}
}

/*
============
sdUIList::CalcColumnWidths
============
*/
void sdUIList::CalcColumnWidths( float* columnWidths ) const {
	float constantColumnWidths = 0.0f;
	int i;
	for ( i = 0; i < columns.Num(); i++ ) {
		sdUIListColumn* column = columns[ i ];
		if( !column->flags.widthPercent ) {
			constantColumnWidths += column->baseWidth;
		}
	}
	for ( i = 0; i < columns.Num(); i++ ) {
		sdUIListColumn* column = columns[ i ];
		if( column->flags.widthPercent ) {
			assert( column->baseWidth >= 0.0f && column->baseWidth <= 1.0f );
			columnWidths[ i ] = ( cachedClientRect.z - constantColumnWidths ) * column->baseWidth;			
		} else {
			columnWidths[ i ] = column->baseWidth;			
		}
	}
}
