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

#define INIT_SCRIPT_FUNCTION( SCRIPTNAME, RETURN, PARMS, FUNCTION ) listFunctions.Set( SCRIPTNAME, new sdUITemplateFunction< sdUIList >( RETURN, PARMS, &sdUIList::FUNCTION ) );

/*
===============================================================================

	sdUIList

===============================================================================
*/

/*
============
sdUIList::InitFunctions
============
*/
#pragma inline_depth( 0 )
#pragma optimize( "", off )
SD_UI_PUSH_CLASS_TAG( sdUIList )
void sdUIList::InitFunctions() {
	SD_UI_FUNC_TAG( insertItem, "Insert list item. Calls onItemAdded event." )
		SD_UI_FUNC_PARM( wstring, "text", "Text to insert." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
		SD_UI_FUNC_RETURN_PARM( float, "The row index or -1 on error." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "insertItem", 'f', "wff", Script_InsertItem );

	SD_UI_FUNC_TAG( insertBlankItems, "Insert multiple blank items." )
		SD_UI_FUNC_PARM( float, "numItems", "Number of items to add." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "insertBlankItems", 'v', "f", Script_InsertBlankItems );

	SD_UI_FUNC_TAG( setItemText, "Set item text." )
		SD_UI_FUNC_PARM( wstring, "text", "Text." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setItemText", 'v', "wff", Script_SetItemText );

	SD_UI_FUNC_TAG( setItemTextFlags, "Set item text flags." )
		SD_UI_FUNC_PARM( float, "flags", "Item flags." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setItemTextFlags", 'v', "fff", Script_SetItemTextFlags );

	SD_UI_FUNC_TAG( setItemForeColor, "Set item forecolor." )
		SD_UI_FUNC_PARM( color, "forecolor", "Item forecolor." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setItemForeColor", 'v', "4ff", Script_SetItemForeColor );

	SD_UI_FUNC_TAG( setItemBackColor, "Set item backcolor." )
		SD_UI_FUNC_PARM( color, "backcolor", "Item backcolor." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setItemBackColor", 'v', "4ff", Script_SetItemBackColor );

	SD_UI_FUNC_TAG( setItemMaterialSize, "Set iteam material size." )
		SD_UI_FUNC_PARM( vec2, "size", "Material size." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setItemMaterialSize", 'v', "2ff", Script_SetItemMaterialSize );

	SD_UI_FUNC_TAG( setItemIcon, "Set icon for item." )
		SD_UI_FUNC_PARM( string, "materialName", "Material name." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setItemIcon", 'v', "sff", Script_SetItemIcon );

	SD_UI_FUNC_TAG( transitionItemVec4, "Transition a property for an item." )
		SD_UI_FUNC_PARM( float, "propertyType", "A list transition property define." )
		SD_UI_FUNC_PARM( vec4, "from", "Start state." )
		SD_UI_FUNC_PARM( vec4, "to", "End state." )
		SD_UI_FUNC_PARM( float, "time", "Transition time." )
		SD_UI_FUNC_PARM( string, "acceleration", "Optionally specify a non-linear transition." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "transitionItemVec4", 'v', "f44fsff", Script_TransitionItemVec4 );

	SD_UI_FUNC_TAG( getItemTransitionVec4Result, "Get the items immediate transition value." )
		SD_UI_FUNC_PARM( float, "propertyType", "A list transition property define." )
		SD_UI_FUNC_PARM( vec4, "defaultValue", "Default value." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
		SD_UI_FUNC_RETURN_PARM( vec4, "Transition result or default value on error." );
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getItemTransitionVec4Result", '4', "f4ff", Script_GetItemTransitionVec4Result );

	SD_UI_FUNC_TAG( transitionColumnVec4, "Transition a property for a column." )
		SD_UI_FUNC_PARM( float, "propertyType", "A list transition property define." )
		SD_UI_FUNC_PARM( vec4, "from", "Start state." )
		SD_UI_FUNC_PARM( vec4, "to", "End state." )
		SD_UI_FUNC_PARM( float, "time", "Transition time." )
		SD_UI_FUNC_PARM( string, "acceleration", "Optionally specify a non-linear transition." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "transitionColumnVec4", 'v', "f44fsf", Script_TransitionColumnVec4 );

	SD_UI_FUNC_TAG( getColumnTransitionVec4Result, "Get the columns immediate transition value." )
		SD_UI_FUNC_PARM( float, "propertyType", "A list transition property define." )
		SD_UI_FUNC_PARM( vec4, "defaultValue", "Default value." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
		SD_UI_FUNC_RETURN_PARM( vec4, "Transition result or default value on error." );
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getColumnTransitionVec4Result", '4', "f4f", Script_GetColumnTransitionVec4Result );

	SD_UI_FUNC_TAG( clearTransitions, "Clear transitions for an item." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "clearTransitions", 'v', "ff", Script_ClearTransitions );

	SD_UI_FUNC_TAG( setColumnText, "Set column header text." )
		SD_UI_FUNC_PARM( wstring, "text", "Text." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setColumnText", 'v', "wf", Script_SetColumnText );

	SD_UI_FUNC_TAG( setColumnWidth, "Set column width." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
		SD_UI_FUNC_PARM( float, "width", "Width." )
		SD_UI_FUNC_PARM( float, "isPercent", "width is in percent." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setColumnWidth", 'v', "fff", Script_SetColumnWidth );

	SD_UI_FUNC_TAG( setColumnTextFlags, "Set column header text flags." )
		SD_UI_FUNC_PARM( float, "flags", "Text flags." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setColumnTextFlags", 'v', "ff", Script_SetColumnTextFlags );

	SD_UI_FUNC_TAG( deleteItem, "Delete an item from the list. Calls onItemRemoved event." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "deleteItem", 'v', "f", Script_DeleteItem );

	SD_UI_FUNC_TAG( insertColumn, "Insert a column." )
		SD_UI_FUNC_PARM( wstring, "text", "Column header text." )
		SD_UI_FUNC_PARM( float, "baseWidth", "Column width." )
		SD_UI_FUNC_PARM( float, "index", "Index of the new column." )
		SD_UI_FUNC_RETURN_PARM( float, "Index of the new column." );
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "insertColumn", 'f', "wff", Script_InsertColumn );

	SD_UI_FUNC_TAG( deleteColumn, "Delete a column. NOT IMPLEMENTED." )
		SD_UI_FUNC_PARM( float, "column", "Column Index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "deleteColumn", 'v', "f", Script_DeleteColumn );

	SD_UI_FUNC_TAG( clearItems, "Delete all items." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "clearItems", 'v', "", Script_ClearItems );

	SD_UI_FUNC_TAG( clearColumns, "Delete all columns." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "clearColumns", 'v', "", Script_ClearColumns );

	SD_UI_FUNC_TAG( getItemText, "Get item text. Returns header text if row is -1." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Item text." );
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getItemText", 'w', "ff", Script_GetItemText );

	SD_UI_FUNC_TAG( getItemRect, "Get item rectangle." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
		SD_UI_FUNC_PARM( float, "type", "Type of rectangle to get. Either GIR_COLUMN or GIR_FULLWIDTH." )
		SD_UI_FUNC_RETURN_PARM( rect, "Item rectangle." );
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getItemRect", '4', "fff", Script_GetItemRect );

	SD_UI_FUNC_TAG( fillFromEnumerator, "Fill list using the specified enumerator. Some enumerators expect additional parameters to be pushed on the stack (like the fireteam menu which expects you to push the current menu page before calling this function)." )
		SD_UI_FUNC_PARM( string, "enumerator", "Enumerator name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "fillFromEnumerator", 'v', "s", Script_FillFromEnumerator );

	SD_UI_FUNC_TAG( sort, "Explicitly sort list." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "sort", 'v', "", Script_Sort );

	SD_UI_FUNC_TAG( getItemAtPoint, "Get item at position. Often used to get the list item under the cursor." )
		SD_UI_FUNC_PARM( float, "posX", "X position." )
		SD_UI_FUNC_PARM( float, "posY", "Y position." )
		SD_UI_FUNC_RETURN_PARM( vec2, "Row, column of item or -1 if the cursor isn't over an item." );
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getItemAtPoint", '2', "ff", Script_GetItemAtPoint );

	SD_UI_FUNC_TAG( findItem, "Finds first item in column which matches the search text." )
		SD_UI_FUNC_PARM( wstring, "searcText", "Search text." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns the row index or -1 if not found." );
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "findItem", 'f', "wf", Script_FindItem );

	SD_UI_FUNC_TAG( findItemDataInt, "Find the item with the item data." )
		SD_UI_FUNC_PARM( float, "data", "Value to search for." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns the row index or -1 if not found." );
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "findItemDataInt", 'f', "ff", Script_FindItemDataInt );

	SD_UI_FUNC_TAG( setItemDataInt, "Set item data." )
		SD_UI_FUNC_PARM( float, "value", "Integer value." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setItemDataInt", 'v', "fff", Script_SetItemDataInt );

	SD_UI_FUNC_TAG( getItemDataInt, "Get item data." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
		SD_UI_FUNC_PARM( float, "value", "Default value." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns the data or default value if item was not found." );
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getItemDataInt", 'f', "fff", Script_GetItemDataInt );

	SD_UI_FUNC_TAG( storeVisualState, "Store the visual state, in essence the scroll amount." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "storeVisualState", 'v', "", Script_StoreVisualState );

	SD_UI_FUNC_TAG( restoreVisualState, "Restore the visual state. Makes the layout dirty." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "restoreVisualState", 'v', "", Script_RestoreVisualState );
	
	SD_UI_FUNC_TAG( drawItemMaterial, "Draw a material in the given item." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
		SD_UI_FUNC_PARM( rect, "drawRect", "Draw rectangle." )
		SD_UI_FUNC_PARM( color, "materialColor", "Material color." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "drawItemMaterial", 'v', "ff44", Script_DrawItemMaterial );

	SD_UI_FUNC_TAG( sizeLastColumn, "Stretch the last column to fill any leftover space, minus the amount passed in." )
		SD_UI_FUNC_PARM( float, "reservedWidth", "Reserved width." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "sizeLastColumn", 'v', "f", Script_SizeLastColumn );

	SD_UI_FUNC_TAG( setItemFlags, "Set item flags." )
		SD_UI_FUNC_PARM( wstring, "strFlags", "String based flags." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setItemFlags", 'v', "wff", Script_SetItemFlags );

	SD_UI_FUNC_TAG( setColumnFlags, "Set column header flags." )
		SD_UI_FUNC_PARM( wstring, "strFlags", "String based flags." )
		SD_UI_FUNC_PARM( float, "column", "Column index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setColumnFlags", 'v', "wf", Script_SetColumnFlags );

	SD_UI_FUNC_TAG( ensureItemIsVisible, "Potentially scroll the list to make sure the item is visible." )
		SD_UI_FUNC_PARM( float, "row", "Row index." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "ensureItemIsVisible", 'v', "f", Script_EnsureItemIsVisible );

	SD_UI_FUNC_TAG( fillFromFile, "Fill list from file. Supports UTF8 encoding." )
		SD_UI_FUNC_PARM( string, "filename", "Relative file name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "fillFromFile", 'v', "s", Script_FillFromFile );


	SD_UI_PUSH_GROUP_TAG( "List Flags" )

	SD_UI_ENUM_TAG( LF_AUTO_SCROLL_TO_SELECTION, "Automatically scroll to selected item." )
	sdDeclGUI::AddDefine( va( "LF_AUTO_SCROLL_TO_SELECTION %i", LF_AUTO_SCROLL_TO_SELECTION ) );

	SD_UI_ENUM_TAG( LF_SHOW_HEADINGS, "Show Column headers." )
	sdDeclGUI::AddDefine( va( "LF_SHOW_HEADINGS %i",			LF_SHOW_HEADINGS ) );

	SD_UI_ENUM_TAG( LF_HOT_TRACK, "Automatically select item under the mouse." )
	sdDeclGUI::AddDefine( va( "LF_HOT_TRACK %i",				LF_HOT_TRACK ) );

	SD_UI_ENUM_TAG( LF_COLUMN_SORT, "Sort items by column. Items are sorted by the first column by default." )
	sdDeclGUI::AddDefine( va( "LF_COLUMN_SORT %i",				LF_COLUMN_SORT ) );	

	SD_UI_ENUM_TAG( LF_NO_NULL_SELECTION, "Clicking in an empty area won't deselect the current item." )
	sdDeclGUI::AddDefine( va( "LF_NO_NULL_SELECTION %i",		LF_NO_NULL_SELECTION ) );

	SD_UI_ENUM_TAG( LF_VARIABLE_HEIGHT_ROWS, "Rows will always fit the text inside them. Text will automatically wrap." )
	sdDeclGUI::AddDefine( va( "LF_VARIABLE_HEIGHT_ROWS %i",		LF_VARIABLE_HEIGHT_ROWS ) );

	SD_UI_ENUM_TAG( LF_NO_KEY_EVENTS, "No key events are handled by the list." )
	sdDeclGUI::AddDefine( va( "LF_NO_KEY_EVENTS %i",			LF_NO_KEY_EVENTS ) );

	SD_UI_ENUM_TAG( LF_TRUNCATE_COLUMNS, "Only truncate column header text." )
	sdDeclGUI::AddDefine( va( "LF_TRUNCATE_COLUMNS %i",			LF_TRUNCATE_COLUMNS ) );

	SD_UI_ENUM_TAG( LF_DIRECT_UPDATES, "Allow for directly updating item text without triggering sorting." )
	sdDeclGUI::AddDefine( va( "LF_DIRECT_UPDATES %i",			LF_DIRECT_UPDATES ) );

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "List Transition Flags" )

	SD_UI_ENUM_TAG( LTP_FORECOLOR, "Forecolor transition property. Directly tied to foreColor" )
	sdDeclGUI::AddDefine( va( "LTP_FORECOLOR %i", 				LTP_FORECOLOR ) );

	SD_UI_ENUM_TAG( LTP_BACKCOLOR, "Backcolor transition property. Directly tied to backColor" )
	sdDeclGUI::AddDefine( va( "LTP_BACKCOLOR %i", 				LTP_BACKCOLOR ) );

	SD_UI_ENUM_TAG( LTP_PROPERTY_0, "Transition transition property 0." )
	sdDeclGUI::AddDefine( va( "LTP_PROPERTY_0 %i", 				LTP_PROPERTY_0 ) );

	SD_UI_ENUM_TAG( LTP_PROPERTY_1, "Transition transition property 1." )
	sdDeclGUI::AddDefine( va( "LTP_PROPERTY_1 %i", 				LTP_PROPERTY_1 ) );

	SD_UI_ENUM_TAG( LTP_PROPERTY_2, "Transition transition property 2." )
	sdDeclGUI::AddDefine( va( "LTP_PROPERTY_2 %i", 				LTP_PROPERTY_2 ) );

	SD_UI_POP_GROUP_TAG

	SD_UI_ENUM_TAG( GIR_COLUMN, "Get item rectangle with column width." )
	sdDeclGUI::AddDefine( va( "GIR_COLUMN %i",					GIR_COLUMN ) );

	SD_UI_ENUM_TAG( GIR_FULLWIDTH, "Get item rectangle with row width." )
	sdDeclGUI::AddDefine( va( "GIR_FULLWIDTH %i",				GIR_FULLWIDTH ) );	
}
SD_UI_POP_CLASS_TAG
#pragma optimize( "", on )
#pragma inline_depth()

/*
============
sdUIList::Script_GetItemDataInt
============
*/
void sdUIList::Script_GetItemDataInt( sdUIFunctionStack& stack ) {
	int row;
	int column;
	int defaultValue;

	stack.Pop( row );
	stack.Pop( column );
	stack.Pop( defaultValue );
	
	if( !IndexCheck( row, column, "GetItemDataInt" )) {
		stack.Push( defaultValue );
		return;
	}
	stack.Push( static_cast< float >( GetItemDataInt( row, column ) ) );
}

/*
============
sdUIList::Script_SetItemDataInt
============
*/
void sdUIList::Script_SetItemDataInt( sdUIFunctionStack& stack ) {
	int row;
	int column;
	int value;

	stack.Pop( value );
	stack.Pop( row );
	stack.Pop( column );

	if( !IndexCheck( row, column, "SetItemDataInt" )) {
		return;
	}
	SetItemDataInt( value, row, column );
}

/*
============
sdUIList::Script_FillFromEnumerator
============
*/
void sdUIList::Script_FillFromEnumerator( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );

	uiListEnumerationCallback_t enumerator = uiManager->GetListEnumerationCallback( name );
	if ( enumerator ) {
		BeginBatch();
		enumerator( this );
		EndBatch();

/*		if ( name.Icmp( "scoreboardList" ) != 0 ) {
			assert( GetUI()->IsScriptStackEmpty() );
		}*/
	} else {
		if( sdUserInterfaceLocal::g_debugGUIEvents.GetInteger() > 1 ) {
			gameLocal.Warning( "Script_FillFromEnumerator: '%s' Unknown enumerator '%s'", this->name.GetValue().c_str(), name.c_str() );
		}
		GetUI()->ClearScriptStack();
	}
}


/*
============
sdUIList::Script_GetItemText
============
*/
void sdUIList::Script_GetItemText( sdUIFunctionStack& stack ) {
	int row;
	int column;
	stack.Pop( row );
	stack.Pop( column );

	stack.Push( GetItemText( row, column, false ) );
}

/*
============
sdUIList::Script_InsertItem
============
*/
void sdUIList::Script_InsertItem( sdUIFunctionStack& stack ) {
	int index;
	int column;
	idWStr text;

	stack.Pop( text );
	stack.Pop( index );
	stack.Pop( column );

	if( columns.Num() == 0 ) {
		stack.Push( -1 );
		return;
	}

	idWStrList items;
	idSplitStringIntoList( items, text.c_str(), L"\t" );

	if ( items.Num() == 0 ) {
		items.Append( L"" );
	}

	sdUIListItem* newItem = new sdUIListItem;
	sdTransition transition;
	transition.foreColor.InitConstant( foreColor );
	transition.backColor.InitConstant( backColor );
	FormatItemText( *newItem, transition, items[ 0 ].c_str() );

	if( index < 0 ) {
		index = GetNumItems();
	}

	int itemIndex = -1;

	for( int i = 0; i < columns.Num(); i++ ) {
		if( i == column ) {
			itemIndex = columns[ i ]->items.Insert( newItem, index );
			columns[ i ]->itemTransitions.Insert( transition, index );
		} else {
			sdUIListItem* blankItem = new sdUIListItem( *newItem );
			blankItem->textHandle = -1;
			blankItem->part.mi.Clear();
			
			sdTransition transition;
			transition.foreColor.InitConstant( foreColor );
			transition.backColor.InitConstant( backColor );
			if( i < items.Num() && column == 0 ) {
				FormatItemText( *blankItem, transition, items[ i ].c_str() );
			} else {
				blankItem->text = L"";				
			}			
			columns[ i ]->items.Insert( blankItem, index );
			columns[ i ]->itemTransitions.Insert( transition, index );
		}
	}

	rowHeights.Insert( fixedRowHeight, index );

	Sort();
	MakeLayoutDirty();
	
	stack.Push( itemIndex );

	numItems.SetReadOnly( false );
	numItems = columns[ 0 ]->items.Num();
	numItems.SetReadOnly( true );

	RunEvent( sdUIEventInfo( LE_ITEM_ADDED, 0 ) );
}

/*
============
sdUIList::Script_InsertBlankItems
============
*/
void sdUIList::Script_InsertBlankItems( sdUIFunctionStack& stack ) {
	int numItems;

	stack.Pop( numItems );
	for( int i = 0; i < numItems; i++ ) {
		sdUIFunctionStack tempStack;
		tempStack.Push( -1 );
		tempStack.Push( -1 );
		tempStack.Push( L"" );
		Script_InsertItem( tempStack );
	}
}

/*
============
sdUIList::Script_SetItemText
============
*/
void sdUIList::Script_SetItemText( sdUIFunctionStack& stack ) {
	int column;
	int item;
	idWStr text;
	stack.Pop( text );
	stack.Pop( item );	
	stack.Pop( column );	

	if( column < 0 ) {
		// take care of all columns if < 0
		for( int i = 0; i < columns.Num(); i++ ) {
			sdUIListColumn* c = columns[ i ];

			if( item < 0 || item >= c->items.Num() ) {
				gameLocal.Warning( "SetItemText: '%s' item '%i' out of range in column '%i'", name.GetValue().c_str(), (int)item, i );
				return;
			}
			sdTransition transition;
			transition.foreColor.InitConstant( foreColor );
			transition.backColor.InitConstant( backColor );

			FormatItemText( *c->items[ item ], transition, text.c_str() );
			c->itemTransitions[ item ] = transition;
		}
		Sort();
	} else {
		if( column >= columns.Num() ) {
			gameLocal.Warning( "SetItemText: '%s' column '%i' out of range", name.GetValue().c_str(), (int)column );
			return;
		}

		sdUIListColumn* c = columns[ column ];

		if( item < 0 || item >= c->items.Num() ) {
			sdTransition transition;
			transition.foreColor.InitConstant( foreColor );
			transition.backColor.InitConstant( backColor );

			// take care of all items in the column
			for( int i = 0; i < c->items.Num(); i++ ) {
				FormatItemText( *c->items[ i ], transition, text.c_str() );

				c->itemTransitions[ i ] = transition;
			}
			return;
		} else {
			sdTransition transition;
			transition.foreColor.InitConstant( foreColor );
			transition.backColor.InitConstant( backColor );

			FormatItemText( *c->items[ item ], transition, text.c_str() );
			c->itemTransitions[ item ] = transition;
		}

		if( column == idMath::Ftoi( activeColumn ) ) {
			Sort();
		}
	}
}

/*
============
sdUIList::Script_SetItemIcon
============
*/
void sdUIList::Script_SetItemIcon( sdUIFunctionStack& stack ) {
	int column;
	int item;
	idStr materialName;
	stack.Pop( materialName );
	stack.Pop( item );	
	stack.Pop( column );	

	uiMaterialInfo_t mi;
	GetUI()->LookupMaterial( materialName, mi );

	if( column < 0 ) {
		// take care of all columns if < 0
		for( int i = 0; i < columns.Num(); i++ ) {
			sdUIListColumn* c = columns[ i ];

			if( item < 0 || item >= c->items.Num() ) {
				gameLocal.Warning( "SetItemIcon:  '%s' item '%i' out of range in column '%i'", name.GetValue().c_str(), item, i );
				return;
			}

			c->items[ item ]->part.mi = mi;
		}		
	} else {
		if( column >= columns.Num() ) {
			gameLocal.Warning( "SetItemIcon:  '%s' column '%i' out of range", name.GetValue().c_str(), column );
			return;
		}

		sdUIListColumn* c = columns[ column ];

		if( item < 0 || item >= c->items.Num() ) {
			gameLocal.Warning( "SetItemIcon:  '%s' item '%i' out of range in column '%i'", name.GetValue().c_str(), item, column );
			return;
		}

		c->items[ item ]->part.mi = mi;
	}
}

/*
============
sdUIList::Script_SetColumnWidth
============
*/
void sdUIList::Script_SetColumnWidth( sdUIFunctionStack& stack ) {
	float width;
	int column;
	bool percent;

	stack.Pop( width );	
	stack.Pop( column );
	stack.Pop( percent );

	if( column < 0 || column >= columns.Num() ) {
		gameLocal.Warning( "SetColumnWidth: '%s' column '%i' out of range", name.GetValue().c_str(), (int)column );
		return;
	}

	if( percent ) {
		width /= 100.0f;
	}

	sdUIListColumn* c = columns[ column ];
	c->baseWidth = width;
	c->flags.widthPercent = percent;
	//MakeLayoutDirty();
}

/*
============
sdUIList::Script_SetItemForeColor
============
*/
void sdUIList::Script_SetItemForeColor( sdUIFunctionStack& stack ) {
	int column;
	int item;
	idVec4 color;
	stack.Pop( color );
	stack.Pop( item );
	stack.Pop( column );	

	if( column < 0 ) {
		// take care of all columns if < 0
		for( int i = 0; i < columns.Num(); i++ ) {
			sdUIListColumn* c = columns[ i ];
			
			if( item < 0 || item >= c->items.Num() ) {
				gameLocal.Warning( "SetItemForeColor:  '%s' item '%i' out of range in column '%i'", name.GetValue().c_str(), item, i );
				return;
			}

			c->itemTransitions[ item ].foreColor.InitConstant( color );
		}		
	} else {
		if( column >= columns.Num() ) {
			gameLocal.Warning( "SetItemForeColor: '%s' column '%i' out of range", name.GetValue().c_str(), column );
			return;
		}

		sdUIListColumn* c = columns[ column ];

		if( item < 0 || item >= c->items.Num() ) {
			// take care of all items in the column
			for( int i = 0; i < c->items.Num(); i++ ) {
				c->itemTransitions[ i ].foreColor.InitConstant( color );
			}
			return;
		}

		c->itemTransitions[ item ].foreColor.InitConstant( color );
	}
}

/*
============
sdUIList::Script_SetItemBackColor
============
*/
void sdUIList::Script_SetItemBackColor( sdUIFunctionStack& stack ) {
	int column;
	int item;
	idVec4 color;
	stack.Pop( color );
	stack.Pop( item );
	stack.Pop( column );	
	
	if( column < 0 ) {
		// take care of all columns if < 0
		for( int i = 0; i < columns.Num(); i++ ) {
			sdUIListColumn* c = columns[ i ];
			if( item < 0 || item >= c->items.Num() ) {
				gameLocal.Warning( "SetItemBackColor: '%s' item '%i' out of range in column '%i'", name.GetValue().c_str(), item, i );
				return;
			}

			c->itemTransitions[ item ].backColor.InitConstant( color );
		}		
	} else {
		if( column >= columns.Num() ) {
			gameLocal.Warning( "SetItemBackColor: '%s' column '%i' out of range", name.GetValue().c_str(), (int)column );
			return;
		}

		sdUIListColumn* c = columns[ column ];

		if( item < 0 || item >= c->items.Num() ) {
			gameLocal.Warning( "SetItemBackColor: '%s' item '%i' out of range in column '%i'", name.GetValue().c_str(), item, column );
			return;
		}

		c->itemTransitions[ item ].backColor.InitConstant( color );
	}
}

/*
============
sdUIList::Script_DeleteItem
============
*/
void sdUIList::Script_DeleteItem( sdUIFunctionStack& stack ) {
	int item;
	stack.Pop( item );	

	if( columns.Num() == 0 ) {		
		return;
	}

	if( item < 0 || item >= GetNumItems() ) {
		gameLocal.Warning( "DeleteItem: '%s' index '%i' out of range", name.GetValue().c_str(), item );
		return;
	}

	for( int i = 0; i < columns.Num(); i++ ) {
		delete columns[ i ]->items[ item ];
		columns[ i ]->items.RemoveIndex( item );
	}
	Sort();
	
	numItems.SetReadOnly( false );
	numItems = columns[ 0 ]->items.Num();
	numItems.SetReadOnly( true );

	RunEvent( sdUIEventInfo( LE_ITEM_REMOVED, 0 ) );
}

/*
============
sdUIList::Script_InsertColumn
============
*/
void sdUIList::Script_InsertColumn( sdUIFunctionStack& stack ) {
	
	sdUIListColumn* column = new sdUIListColumn;
	idVec4 localForeColor = foreColor;
	idVec4 localBackColor = backColor;

	int index;
	stack.Pop( column->text );
	stack.Pop( column->baseWidth );
	stack.Pop( index );

	ExtractColorFromString( column->text.c_str(),			L"fore",		localForeColor );
	ExtractColorFromString( column->text.c_str(),			L"back",		localBackColor );
	ExtractTextFormatFromString( column->text.c_str(),		L"align",		column->textFlags );
	ExtractLocalizedTextFromString( column->text.c_str(),	L"loc",			column->textHandle );
	ExtractColumnFlagsFromString( column->text.c_str(),		L"flags",		*column );
	ExtractColumnWidthFromString( column->text.c_str(),		L"width",		*column );

	column->transition.foreColor.InitConstant( localForeColor );
	column->transition.backColor.InitConstant( localBackColor );

	StripFormatting( column->text );

	if( index < 0 ) {
		index = columns.Num();
	}

	stack.Push( columns.Insert( column, index ));
	
	// sort by the first column by default
	if( TestFlag( LF_COLUMN_SORT ) && index == 0 && idMath::Ftoi( activeColumn ) == -1 ) {
		activeColumn = 0.0f;
	}

	MakeLayoutDirty();
}

/*
============
sdUIList::Script_DeleteColumn
============
*/
void sdUIList::Script_DeleteColumn( sdUIFunctionStack& stack ) {
	// TODO:
}

/*
============
sdUIList::Script_ClearItems
============
*/
void sdUIList::Script_ClearItems( sdUIFunctionStack& stack ) {
	for( int i = 0; i < columns.Num(); i++ ) {
		columns[ i ]->items.DeleteContents( false );
		columns[ i ]->items.SetNum( 0, false );
		columns[ i ]->itemTransitions.SetNum( 0, false );
	}
	numItems.SetReadOnly( false );
	numItems = 0.0f;
	numItems.SetReadOnly( true );

	indices.SetNum( 0, false );
	rowHeights.SetNum( 0, false );
	currentSelection = -1.0f;
	scrollAmount = 0.0f;
}

/*
============
sdUIList::Script_ClearColumns
============
*/
void sdUIList::Script_ClearColumns( sdUIFunctionStack& stack ) {
	columns.DeleteContents( true );
	indices.Clear();

	numItems.SetReadOnly( false );
	numItems = 0.0f;
	numItems.SetReadOnly( true );

	currentSelection = -1.0f;
	scrollAmount = 0.0f;
}

/*
============
sdUIList::Script_GetItemAtPoint
============
*/
void sdUIList::Script_GetItemAtPoint( sdUIFunctionStack& stack ) {
	idVec2 point;
	stack.Pop( point.x );
	stack.Pop( point.y );

	sdBounds2D listBounds( GetWorldRect() );
	if( listBounds.ContainsPoint( point )) {
		float* columnWidths = static_cast< float* >( _alloca( columns.Num() * sizeof( float ) ) );
		CalcColumnWidths( columnWidths );

		idVec4 rect;

		// check items
		for( int i = 0; i < columns.Num(); i++ ) {
			GetHeaderColumnRect( i, rect, columnWidths );
			sdBounds2D bounds( rect );
			if( bounds.ContainsPoint( point )) {
				stack.Push( idVec2( -1.0f, static_cast< float >( i ) ) );
				return;
			}


			for( int j = 0; j < GetNumItems(); j++ ) {
				GetItemRect( j, i, rect, columnWidths );
				sdBounds2D bounds( rect );

				if(	bounds.ContainsPoint( point )) {
					stack.Push( idVec2( j, i ) );
					return;
				}
			}
		}
	}

	stack.Push( idVec2( -1.0f, -1.0f ) );
}

/*
============
sdUIList::Script_StoreVisualState
============
*/
void sdUIList::Script_StoreVisualState( sdUIFunctionStack& stack ) {
	storedScrollAmount = idMath::Fabs( scrollAmount );
}

/*
============
sdUIList::Script_RestoreVisualState
============
*/
void sdUIList::Script_RestoreVisualState( sdUIFunctionStack& stack ) {
	MakeLayoutDirty();	
}

/*
============
sdUIList::Script_Sort
============
*/
void sdUIList::Script_Sort( sdUIFunctionStack& stack ) {
	Sort();
}

/*
============
sdUIList::Script_FindItem
============
*/
void sdUIList::Script_FindItem( sdUIFunctionStack& stack ) {
	idWStr searchText;
	stack.Pop( searchText );
	
	int column;
	stack.Pop( column );
	
	if ( column < 0 || column >= columns.Num() ) {
		gameLocal.Warning( "Script_FindItem: '%s' column %i out of range", name.GetValue().c_str(), idMath::Ftoi( column ));
		stack.Push( -1 );
		return;
	}

	sdUIListColumn* col = columns[ column ];
	for ( int i = 0; i < col->items.Num(); i++ ) {
		const sdUIListItem* item = col->items[ indices[ i ] ];
		if( item->textHandle != -1 ) {
			const sdDeclLocStr* loc = declHolder.declLocStrType.LocalFindByIndex( item->textHandle );
			if( searchText.IcmpNoColor( loc->GetText() ) == 0 ) {
				stack.Push( i );
				return;
			}
		}
		if ( searchText.IcmpNoColor( item->text.c_str() ) == 0 ) {
			stack.Push( i );
			return;
		}
	}
	stack.Push( -1 );
}

/*
============
sdUIList::Script_FindItemDataInt
============
*/
void sdUIList::Script_FindItemDataInt( sdUIFunctionStack& stack ) {
	int searchData;
	stack.Pop( searchData );

	int column;
	stack.Pop( column );

	if ( column < 0 || column >= columns.Num() ) {
		gameLocal.Warning( "Script_FindItemDataInt: '%s' column %i out of range", name.GetValue().c_str(), idMath::Ftoi( column ));
		stack.Push( -1 );
		return;
	}

	sdUIListColumn* col = columns[ column ];
	for ( int i = 0; i < col->items.Num(); i++ ) {
		const sdUIListItem* item = col->items[ indices[ i ] ];
		if ( searchData == item->data.integer ) {
			stack.Push( i );
			return;
		}
	}
	stack.Push( -1 );
}

/*
============
sdUIList::Script_SetItemMaterialSize
============
*/
void sdUIList::Script_SetItemMaterialSize( sdUIFunctionStack& stack ) {
	int column;
	int item;
	idVec2 size;

	stack.Pop( size );
	stack.Pop( item );
	stack.Pop( column );	

	if( column < 0 ) {
		// take care of all columns if < 0
		for( int i = 0; i < columns.Num(); i++ ) {
			sdUIListColumn* c = columns[ i ];

			if( item < 0 || item >= c->items.Num() ) {
				// take care of all items in the column
				for( int i = 0; i < c->items.Num(); i++ ) {
					c->items[ i ]->part.width = size.x;
					c->items[ i ]->part.height = size.y;
				}
				return;
			}

			c->items[ item ]->part.width = size.x;
			c->items[ item ]->part.height = size.y;
		}		
	} else {
		if( column >= columns.Num() ) {
			gameLocal.Warning( "SetItemMaterialSize: '%s' column '%i' out of range", name.GetValue().c_str(), column );
			return;
		}

		sdUIListColumn* c = columns[ column ];

		if( item < 0 || item >= c->items.Num() ) {
			// take care of all items in the column
			for( int i = 0; i < c->items.Num(); i++ ) {
				c->items[ i ]->part.width = size.x;
				c->items[ i ]->part.height = size.y;
			}
			return;
		}

		c->items[ item ]->part.width = size.x;
		c->items[ item ]->part.height = size.y;
	}	
}

/*
============
sdUIList::Script_SetItemTextFlags
============
*/
void sdUIList::Script_SetItemTextFlags( sdUIFunctionStack& stack ) {
	int column;
	int item;
	int flags;

	stack.Pop( flags );
	stack.Pop( item );
	stack.Pop( column );	

	if( column < 0 ) {
		// take care of all columns if < 0
		for( int i = 0; i < columns.Num(); i++ ) {
			sdUIListColumn* c = columns[ i ];

			if( item < 0 || item >= c->items.Num() ) {
				gameLocal.Warning( "SetItemTextFlags:  '%s' item '%i' out of range in column '%i'", name.GetValue().c_str(), item, i );
				return;
			}

			c->items[ item ]->textFlags = flags;
		}		
	} else {
		if( column >= columns.Num() ) {
			gameLocal.Warning( "SetItemTextFlags: '%s' column '%i' out of range", name.GetValue().c_str(), column );
			return;
		}

		sdUIListColumn* c = columns[ column ];

		if( item < 0 || item >= c->items.Num() ) {
			// take care of all items in the column
			for( int i = 0; i < c->items.Num(); i++ ) {
				c->items[ i ]->textFlags = idMath::Ftoi( flags );
			}
			return;
		}

		c->items[ item ]->textFlags = flags;
	}	
}

/*
============
sdUIList::Script_SetColumnText
============
*/
void sdUIList::Script_SetColumnText( sdUIFunctionStack& stack ) {
	int index;
	idWStr text;

	stack.Pop( text );
	stack.Pop( index );

	if( index < 0 || index >= columns.Num() ) {
		gameLocal.Warning( "'%s': '%s' SetColumnText: index '%i' out of range", GetUI()->GetName(), name.GetValue().c_str(), index );
		return;
	}

	sdUIListColumn* column = columns[ index ];

	int now = GetUI()->GetCurrentTime();
	idVec4 localForeColor = column->transition.foreColor.Evaluate( now );
	idVec4 localBackColor = column->transition.backColor.Evaluate( now );

	if( ExtractColorFromString( text.c_str(),			L"fore",	localForeColor ) ) {
		column->transition.foreColor.InitConstant( localForeColor );
	}
	if( ExtractColorFromString( text.c_str(),			L"back",		localBackColor ) ) {
		column->transition.backColor.InitConstant( localBackColor );
	}
	ExtractTextFormatFromString( text.c_str(),		L"align",		column->textFlags );
	ExtractLocalizedTextFromString( text.c_str(),	L"loc",			column->textHandle );
	ExtractColumnFlagsFromString( text.c_str(),		L"flags",		*column );
	ExtractColumnWidthFromString( text.c_str(),		L"width",		*column );	

	StripFormatting( text );
	column->text = text;
}

/*
============
sdUIList::Script_SetColumnTextFlags
============
*/
void sdUIList::Script_SetColumnTextFlags( sdUIFunctionStack& stack ) {
	int index;
	int flags;

	stack.Pop( flags );
	stack.Pop( index );

	if( index < 0 || index >= columns.Num() ) {
		gameLocal.Warning( "'%s': '%s' SetColumnTextFlags: index '%i' out of range", GetUI()->GetName(), name.GetValue().c_str(), index );
		return;
	}
	sdUIListColumn* column = columns[ index ];
	column->textFlags = flags;
}

/*
============
sdUIList::Script_SetColumnFlags
============
*/
void sdUIList::Script_SetColumnFlags( sdUIFunctionStack& stack ) {
	int index;
	idWStr flags;

	stack.Pop( flags );
	stack.Pop( index );

	if( index >= columns.Num() ) {
		gameLocal.Warning( "'%s': '%s' SetColumnFlags: index '%i' out of range", GetUI()->GetName(), name.GetValue().c_str(), index );
		return;
	}
	if( index < 0 ) {
		for( int i = 0; i < columns.Num(); i++ ) {
			ExtractColumnFlagsFromString( flags.c_str(),		L"flags",		*columns[ i ] );
		}
	} else {
		sdUIListColumn* column = columns[ index ];
		ExtractColumnFlagsFromString( flags.c_str(),		L"flags",		*column );
	}
}

/*
============
sdUIList::Script_TransitionItemVec4
============
*/
void sdUIList::Script_TransitionItemVec4( sdUIFunctionStack& stack ) {
	int property;
	int column;
	int row;
	idVec4 from;
	idVec4 to;
	int time;
	idStr accel;

	stack.Pop( property );
	stack.Pop( from );
	stack.Pop( to );
	stack.Pop( time );
	stack.Pop( accel );
	stack.Pop( row );	
	stack.Pop( column );

	if( time < 0.0f ) {
		gameLocal.Error( "TransitionItemVec4: '%s' duration '%i' out of bounds", name.GetValue().c_str(), time );
		return;
	}

	// handle all columns
	if( column < 0.0f ) {
		for( int i = 0; i < columns.Num(); i++ ) {
			sdUIListColumn* col = columns[ i ];

			vec4Evaluator_t* evaluator = col->itemTransitions[ indices[ row ] ].GetEvaluator( property, true );
			assert( evaluator != NULL );

			InitVec4Transition( from, to, time, accel, *evaluator );
		}
	} else if( column < columns.Num() && row >= 0 && row < GetNumItems() ) {
		sdUIListColumn* col = columns[ column ];

		vec4Evaluator_t* evaluator = col->itemTransitions[ indices[ row ] ].GetEvaluator( property, true );
		assert( evaluator != NULL );

		InitVec4Transition( from, to, idMath::Ftoi( time ), accel, *evaluator );
	} else {
		gameLocal.Warning( "TransitionItemVec4: '%s' index (%i, %i) out of bounds (%i, %i)", name.GetValue() .c_str(), row, column, GetNumItems(), columns.Num() );
	}
}

/*
============
sdUIList::Script_GetItemTransitionVec4Result
============
*/
void sdUIList::Script_GetItemTransitionVec4Result( sdUIFunctionStack& stack ) {
	int	property;
	idVec4	defaultValue;
	int	column;
	int	row;

	stack.Pop( property );
	stack.Pop( defaultValue );
	stack.Pop( row );	
	stack.Pop( column );

	if( property < LTP_FORECOLOR || property >= LTP_PROPERTY_MAX ) {
		gameLocal.Error( "GetItemTransitionVec4Result: '%s' property index '%i' out of bounds", name.GetValue().c_str(), property );
		stack.Push( defaultValue );
		return;
	}

	if( columns.Empty() ) {
		stack.Push( defaultValue );
		return;
	}

	if( column < 0.0f ) {
		column = 0.0f;
	}

	sdUIListColumn* col = columns[ column ];

	if( row < 0.0f || row >= col->items.Num() ) {
		stack.Push( defaultValue );
		return;
	}

	vec4Evaluator_t* evaluator = col->itemTransitions[ row ].GetEvaluator( property, false );

	if( evaluator == NULL || !evaluator->IsInitialized() ) {
		stack.Push( defaultValue );
		return;
	}
	stack.Push( evaluator->Evaluate( GetUI()->GetCurrentTime() ) );
}

/*
============
sdUIList::Script_TransitionColumnVec4
============
*/
void sdUIList::Script_TransitionColumnVec4( sdUIFunctionStack& stack ) {
	int property;
	int column;
	idVec4 from;
	idVec4 to;
	int time;
	idStr accel;

	stack.Pop( property );
	stack.Pop( from );
	stack.Pop( to );
	stack.Pop( time );
	stack.Pop( accel );
	stack.Pop( column );	

	if( time < 0.0f ) {
		gameLocal.Error( "TransitionColumnVec4: '%s' duration '%i' out of bounds", name.GetValue().c_str(), time );
		return;
	}

	sdUIListColumn* col = columns[ column ];

	vec4Evaluator_t* evaluator = col->transition.GetEvaluator( property, true );
	assert( evaluator != NULL );

	InitVec4Transition( from, to, time, accel, *evaluator );
}

/*
============
sdUIList::Script_GetColumnTransitionVec4Result
============
*/
void sdUIList::Script_GetColumnTransitionVec4Result( sdUIFunctionStack& stack ) {
	int	property;
	idVec4	defaultValue;
	int	column;

	stack.Pop( property );
	stack.Pop( defaultValue );
	stack.Pop( column );

	if( property < LTP_FORECOLOR || property >= LTP_PROPERTY_MAX ) {
		gameLocal.Error( "GetItemTransitionVec4Result: '%s' property index '%i' out of bounds", name.GetValue().c_str(), property );
		stack.Push( defaultValue );
		return;
	}

	if( columns.Empty() ) {
		stack.Push( defaultValue );
		return;
	}

	if( column < 0.0f ) {
		column = 0.0f;
	}

	sdUIListColumn* col = columns[ column ];

	vec4Evaluator_t* evaluator = col->transition.GetEvaluator( property, false );

	if( evaluator == NULL || !evaluator->IsInitialized() ) {
		stack.Push( defaultValue );
		return;
	}
	stack.Push( evaluator->Evaluate( GetUI()->GetCurrentTime() ) );
}

/*
============
sdUIList::Script_ClearTransitions
============
*/
void sdUIList::Script_ClearTransitions( sdUIFunctionStack& stack ) {
	int row;
	int column;
	
	stack.Pop( row );
	stack.Pop( column );

	if( column >= columns.Num() ) {
		gameLocal.Warning( "ClearTransitions: '%s' column '%i' out of bounds", name.GetValue().c_str(), column );
		return;
	}

	if( row >= GetNumItems() ) {
		return;
	}

	if( column < 0 ) {
		// clear all columns
		for( int i = 0; i < columns.Num(); i++ ) {
			sdUIListColumn* col = columns[ i ];

			if( row < 0 ) {
				for( int trans = 0; trans < col->itemTransitions.Num(); trans++ ) {
					ClearTransition( col->itemTransitions[ trans ] );
				}				
			} else {
				ClearTransition( col->itemTransitions[ row ] );
			}
		}
	} else {
		sdUIListColumn* col = columns[ column ];
		if( row < 0 ) {
			for( int trans = 0; trans < col->itemTransitions.Num(); trans++ ) {
				ClearTransition( col->itemTransitions[ trans ] );				
			}			
		} else if( row < col->itemTransitions.Num() ) {
			ClearTransition( col->itemTransitions[ row ] );
		}
	}
}

/*
============
sdUIList::Script_GetItemRect
============
*/
void sdUIList::Script_GetItemRect( sdUIFunctionStack& stack ) {
	idVec4 rect( vec4_zero );

	int row;
	int column;
	int iType;

	stack.Pop( row );
	stack.Pop( column );
	stack.Pop( iType );

	listGetItemRect_e type = GIR_MIN;
	sdIntToContinuousEnum< listGetItemRect_e >( iType, GIR_MIN, GIR_MAX, type );
	switch( type ) {
		case GIR_COLUMN: {
			float* columnWidths = static_cast< float* >( _alloca( columns.Num() * sizeof( float ) ) );
			CalcColumnWidths( columnWidths );
			GetItemRect( row, column, rect, columnWidths );
			}
			break;
		case GIR_FULLWIDTH:
			GetItemRect( row, rect );
			break;
	}
	stack.Push( rect );
}

/*
============
sdUIList::Script_DrawItemMaterial
============
*/
void sdUIList::Script_DrawItemMaterial( sdUIFunctionStack& stack ) {
	float row;
	float column;
	idVec4 rect;
	idVec4 color;

	stack.Pop( row );
	stack.Pop( column );
	stack.Pop( rect );
	stack.Pop( color );

	DrawItemMaterial( idMath::Ftoi( row ), idMath::Ftoi( column ), rect, color, true );
}

/*
============
sdUIList::Script_SizeLastColumn
============
*/
void sdUIList::Script_SizeLastColumn( sdUIFunctionStack& stack ) {
	float delta;
	stack.Pop( delta );
	if( columns.Empty() ) {
		return;
	}
	MakeLayoutDirty();
	sizeLastColumnDelta = delta;		
}

/*
============
sdUIList::Script_SetItemFlags
============
*/
void sdUIList::Script_SetItemFlags( sdUIFunctionStack& stack ) {
	idWStr text;
	int row;
	int column;	

	stack.Pop( text );
	stack.Pop( row );
	stack.Pop( column );	
	

	if( column < 0 ) {
		// take care of all columns if < 0
		for( int i = 0; i < columns.Num(); i++ ) {
			sdUIListColumn* c = columns[ i ];

			if( row < 0 || row >= c->items.Num() ) {
				// take care of all items in the column
				for( int j = 0; j < c->items.Num(); j++ ) {
					ExtractItemFlagsFromString( text.c_str(), L"flags", *c->items[ j ] );
				}
			} else {
				ExtractItemFlagsFromString( text.c_str(), L"flags", *c->items[ row ] );
			}			
		}		
	} else {
		if( column >= columns.Num() ) {
			gameLocal.Warning( "SetItemFlags: '%s' column '%i' out of range", name.GetValue().c_str(), column );
			return;
		}

		sdUIListColumn* c = columns[ column ];

		if( row < 0 || row >= c->items.Num() ) {
			// take care of all items in the column
			for( int i = 0; i < c->items.Num(); i++ ) {
				ExtractItemFlagsFromString( text.c_str(), L"flags", *c->items[ i ] );
			}
			return;
		}

		ExtractItemFlagsFromString( text.c_str(), L"flags", *c->items[ row ] );
	}
}

/*
============
sdUIList::Script_EnsureItemIsVisible
============
*/
void sdUIList::Script_EnsureItemIsVisible( sdUIFunctionStack& stack ) {
	// Defer scrolling until right before drawing
	stack.Pop( scrollToItem );
}

/*
============
sdUIList::Script_FillFromFile
============
*/
void sdUIList::Script_FillFromFile( sdUIFunctionStack& stack ) {
	idStr fileName;
	stack.Pop( fileName );
	
	sdFilePtr file( fileSystem->OpenFileRead( fileName.c_str() ) );
	if( !file.IsValid() ) {
		gameLocal.Warning( "%s: Script_FillFromFile: could not find '%s'", name.GetValue().c_str(), fileName.c_str() );
		return;
	}

	sdUTF8 utf8( file.Get() );
	file.Release();

	int numChars = utf8.DecodeLength();
	wchar_t* ucs2 = static_cast< wchar_t* >( Mem_Alloc( ( numChars + 1 ) * sizeof( wchar_t ) ) );

	utf8.Decode( ucs2 );

	idWLexer src( ucs2, idWStr::Length( ucs2 ), fileName );
	if ( !src.IsLoaded() ) {
		Mem_Free( ucs2 );
		return;
	}

	declManager->AddDependency( GetUI()->GetDecl(), fileName.c_str() );

	idWToken tok;
	src.ExpectTokenString( L"{" );
	while ( src.ReadToken( &tok ) ) {
		if ( tok == L"}" ) {
			break;
		}
		InsertItem( this, tok.c_str(), -1, 0 );
	}
}
