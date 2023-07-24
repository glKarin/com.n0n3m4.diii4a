// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACELIST_H__
#define __GAME_GUIS_USERINTERFACELIST_H__

#include "../../idlib/PropertiesImpl.h"
#include "UserInterfaceTypes.h"
#include "UIWindow.h"

extern const char sdUITemplateFunctionInstance_IdentifierList[];
extern const char sdUIListItem_Identifier[];
extern const char sdUIListColumn_Identifier[];

/*
============
sdUIList
============
*/
SD_UI_PUSH_CLASS_TAG( sdUIList )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"The list window type can be used if you want display a list in the GUI. It is the most complex " \
	"window type in the GUI system. Some of the features of the list type are column headers, drawing of text or materials " \
	"in headers and items, saving additional data in items, column sorting, variable row height, filling from enumerators, mouse and key events " \
	"on header columns and items."
/* ============ */
)
SD_UI_CLASS_EXAMPLE_TAG(
/* ============ */
	" windowDef vPlayerList {\n" \
		" \ttype list;\n" \
		" \tproperties {\n" \
			" \t\tfloat 	fontSize 		= 48;\n" \
			" \t\t// Truncate player name to the item rectangle\n" \
			" \t\tfloat	flags			= immediate( flags ) | WF_TRUNCATE_TEXT;\n" \
			" \t\trect 	rect 			= 24, 50, 590, 350;\n" \
			" \t\tcolor	backColor		= 0,0,0,0;\n" \
		" \t}\n" \
		" \tevents {\n" \
			" \t\tonCreate {\n" \
				" \t\t\tinsertColumn( gui.blankWStr, 0, 0 ); // Class icon column.\n" \
				" \t\t\tinsertColumn( gui.blankWStr, 590, 1 ) // Player name column;\n" \
			" \t\t}\n" \
		" \t}\n" \
		" \ttimeLine {\n" \
			" \t\tonTime 250 {\n" \
				" \t\t\t// Have the gamecode update the list of players in the vehicle every 250 milliseconds.\n" \
				" \t\t\tfillFromEnumerator( \"vehiclePlayerList\" );\n" \
				" \t\t\tresetTime( 0 );\n" \
			" \t\t}\n" \
		" \t}\n" \
	" }\n"
/* ============ */
)
SD_UI_POP_CLASS_TAG
SD_UI_PROPERTY_TAG(
	alias = "list";
	"2. Drawing/1. Selected Items"	= "Collapse";
	"2. Drawing/2. Columns"			= "Collapse";
)
class sdUIList :
	public sdUIWindow {
public:
	SD_UI_DECLARE_CLASS( sdUIList )

	typedef enum listEvent_e {
		LE_SELECT_ITEM = WE_NUM_EVENTS + 1,
		LE_ITEM_ADDED,
		LE_ITEM_REMOVED,
		LE_DRAW_SELECTED_BACKGROUND,
		LE_DRAW_ITEM_BACKGROUND,		
		LE_DRAW_ITEM,
		LE_DRAW_COLUMN,
		LE_ENTER_COLUMN,
		LE_EXIT_COLUMN,
		LE_CLICK_COLUMN,
		LE_ENTER_ITEM,
		LE_EXIT_ITEM,
		LE_NUM_EVENTS,
	} listEvent_t;

	enum listFlag_e {
		LF_AUTO_SCROLL_TO_SELECTION = BITT< sdUIWindow::NEXT_WINDOW_FLAG + 0 >::VALUE,
		LF_HOT_TRACK				= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 1 >::VALUE,
		LF_COLUMN_SORT				= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 2 >::VALUE,
		LF_SHOW_HEADINGS			= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 3 >::VALUE,
		LF_NO_NULL_SELECTION		= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 4 >::VALUE,	// clicking in an empty area won't deselect the current item
		LF_VARIABLE_HEIGHT_ROWS		= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 5 >::VALUE,
		LF_NO_KEY_EVENTS			= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 6 >::VALUE,
		LF_TRUNCATE_COLUMNS			= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 7 >::VALUE,	// only truncate column text
		LF_DIRECT_UPDATES			= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 8 >::VALUE,	// allow for directly updating item text without triggering sorting
	};

	enum listGetItemRect_e {
		GIR_MIN = -1,
		GIR_COLUMN,
		GIR_FULLWIDTH,
		GIR_MAX,
	};

	typedef sdUITemplateFunction< sdUIList > ListTemplateFunction;
											sdUIList();
	virtual									~sdUIList();

	virtual const char*						GetScopeClassName() const { return "sdUIList"; }

	virtual void							ApplyLayout();
	virtual bool							PostEvent( const sdSysEvent* event );

	virtual void							EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache );	

	virtual void							EndLevelLoad();

	virtual void							CacheEvents();

	int										RemapIndex( int index ) const;

	void									SelectItem( int item );
	void									SetItemGranularity( int granularity );
	void									BeginBatch() { inBatchOperation = true; }
	void									EndBatch() { inBatchOperation = false; Sort(); }
	
	void									Script_InsertItem( sdUIFunctionStack& stack );
	void									Script_InsertBlankItems( sdUIFunctionStack& stack );
	void									Script_SetItemText( sdUIFunctionStack& stack );
	void									Script_SetItemForeColor( sdUIFunctionStack& stack );
	void									Script_SetItemBackColor( sdUIFunctionStack& stack );
	void									Script_SetItemMaterialSize( sdUIFunctionStack& stack );
	void									Script_SetItemTextFlags( sdUIFunctionStack& stack );
	void									Script_DeleteItem( sdUIFunctionStack& stack );
	void									Script_InsertColumn( sdUIFunctionStack& stack );
	void									Script_DeleteColumn( sdUIFunctionStack& stack );
	void									Script_SetColumnText( sdUIFunctionStack& stack );
	void									Script_SetColumnWidth( sdUIFunctionStack& stack );
	void									Script_SetColumnTextFlags( sdUIFunctionStack& stack );
	void									Script_SetColumnFlags( sdUIFunctionStack& stack );
	void									Script_ClearItems( sdUIFunctionStack& stack );
	void									Script_ClearColumns( sdUIFunctionStack& stack );
	void									Script_GetItemText( sdUIFunctionStack& stack );
	void									Script_SetItemIcon( sdUIFunctionStack& stack );
	void									Script_FillFromEnumerator( sdUIFunctionStack& stack );
	void									Script_Sort( sdUIFunctionStack& stack );
	void									Script_GetItemAtPoint( sdUIFunctionStack& stack );
	void									Script_FindItem( sdUIFunctionStack& stack );
	void									Script_FindItemDataInt( sdUIFunctionStack& stack );
	void									Script_SetItemDataInt( sdUIFunctionStack& stack );
	void									Script_GetItemDataInt( sdUIFunctionStack& stack );
	void									Script_StoreVisualState( sdUIFunctionStack& stack );
	void									Script_RestoreVisualState( sdUIFunctionStack& stack );
	void									Script_TransitionItemVec4( sdUIFunctionStack& stack );
	void									Script_GetItemTransitionVec4Result( sdUIFunctionStack& stack );
	void									Script_TransitionColumnVec4( sdUIFunctionStack& stack );
	void									Script_GetColumnTransitionVec4Result( sdUIFunctionStack& stack );
	void									Script_ClearTransitions( sdUIFunctionStack& stack );
	void									Script_GetItemRect( sdUIFunctionStack& stack );
	void									Script_DrawItemMaterial( sdUIFunctionStack& stack );
	void									Script_SizeLastColumn( sdUIFunctionStack& stack );
	void									Script_SetItemFlags( sdUIFunctionStack& stack );
	void									Script_EnsureItemIsVisible( sdUIFunctionStack& stack );
	void									Script_FillFromFile( sdUIFunctionStack& stack );

	virtual sdUIFunctionInstance*			GetFunction( const char* name );
	virtual bool							RunNamedMethod( const char* name, sdUIFunctionStack& stack );

	static void								InitFunctions( void );
	static void								ShutdownFunctions( void ) { listFunctions.DeleteContents(); }
	static const ListTemplateFunction*		FindFunction( const char* name );

	static int								InsertItem( sdUIWindow* list, const wchar_t* name, int item, int column );
	static void								DeleteItem( sdUIWindow* list, int item );
	static void								SetItemText( sdUIWindow* list, const wchar_t* name, int item, int column );
	static void								SetItemForeColor( sdUIWindow* list, const idVec4& color, int item, int column );
	static void								SetItemBackColor( sdUIWindow* list, const idVec4& color, int item, int column );
	static void								SetItemIcon( sdUIWindow* list, const char* iconMaterial, int item, int column );
	static void								ClearItems( sdUIWindow* list );
	static void								ClearColumns( sdUIWindow* list );

	static void								CleanUserInput( idWStr& input );

	static void								StripFormatting( idWStr& string );

	template< class F >
	void									Sort( F sorter ) { sdQuickSort( indices.Begin(), indices.End(), sorter ); }

	void									SetItemDataInt( int integer, int item, int column, bool direct = false );
	void									SetItemDataPtr( void* ptr, int item, int column, bool direct = false );

	int										GetItemDataInt( int item, int column, bool direct = false ) const;
	void*									GetItemDataPtr( int item, int column, bool direct = false ) const;

	const wchar_t*							GetItemText( int item, int column, bool direct = false ) const;

	virtual int								GetMaxEventTypes( void ) const { return LE_NUM_EVENTS; }
	virtual const char*						GetEventNameForType( int event ) const { return ( event < ( WE_NUM_EVENTS + 1 )) ? sdUIWindow::GetEventNameForType( event ): eventNames[ event - WE_NUM_EVENTS - 1 ]; }

	int										GetCurrentSelection() const { return idMath::Ftoi( currentSelection ); }
	int										GetNumItems() const;

protected:
	virtual void							DrawLocal();
	virtual void							InitProperties();

	void									OnFixedRowHeightChanged( const float oldValue, const float newValue );
	void									OnCurrentItemChanged( const float oldValue, const float newValue );
	void									OnActiveColumnChanged( const float oldValue, const float newValue );
	void									OnInternalBorderChanged( const float oldValue, const float newValue );
	bool									IndexCheck( int row, int column, const char* warningText = NULL ) const;

	void									DrawItemMaterial( int row, int col, const idVec4& rect, const idVec4& color, bool directIndex );

											// columnWidths should be preallocated with columns.Num() elements
	void									CalcColumnWidths( float* columnWidths ) const;

	bool									CheckHeaderMouseOver( const sdBounds2D& windowBounds, const idVec2& point );
	bool									CheckItemMouseOver( const sdBounds2D& windowBounds, const idVec2& point );

private:
	enum listTransitionProperty_e {	LTP_FORECOLOR = -2, 
									LTP_BACKCOLOR = -1,
									LTP_PROPERTY_0 = 0,
									LTP_PROPERTY_1 = 1,
									LTP_PROPERTY_2 = 2,
									LTP_PROPERTY_MAX = 3
	};

	static const int MAX_VEC4_EVALUATORS = LTP_PROPERTY_MAX;
	typedef sdTransitionEvaluator< idVec4 > vec4Evaluator_t;
	typedef idStaticList< vec4Evaluator_t, MAX_VEC4_EVALUATORS > vec4EvaluatorList_t;

	class sdTransition {
	public:
								sdTransition() : evaluators( NULL ) {
									backColor.InitConstant( idVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );
									foreColor.InitConstant( colorWhite );
								}
								~sdTransition() {
									FreeEvaluators();
								}

		vec4Evaluator_t*		GetEvaluator( int index, bool allowCreate );
		void					FreeEvaluators() {
									vec4EvaluatorListAllocator.Free( evaluators );
									evaluators = NULL;
								}

		vec4Evaluator_t			backColor;
		vec4Evaluator_t			foreColor;
		vec4EvaluatorList_t*	evaluators;
	};
	static idBlockAlloc< vec4EvaluatorList_t, 32 > vec4EvaluatorListAllocator;

	/*
	============
	sdUIListItem
	============
	*/
	class sdUIListItem :
		public sdPoolAllocator< sdUIListItem, sdUIListItem_Identifier, 1024 > {
	public:
								sdUIListItem();
								~sdUIListItem();

		idWStr					text;
		int						textHandle;
		uiDrawPart_t			part;
		int						textFlags;

		union data_t {
			void*				ptr;
			int					integer;
		} data;

		struct flags_t {
			bool customDraw		: 1;
			bool mouseFocused	: 1;
		} flags;
	};	

	/*
	============
	sdUIListColumn
	============
	*/
	class sdUIListColumn :
		public sdPoolAllocator< sdUIListColumn, sdUIListColumn_Identifier, 1024 > {
	public:
		sdUIListColumn();
		~sdUIListColumn() {
			items.DeleteContents( true );
		}

		void ResetUserFlags() {
			flags.numericSort = false;
			flags.customDraw = false;
			flags.allowSort = true;
			flags.dataSort = false;
		}

		float					baseWidth;
		idWStr					text;
		uiDrawPart_t			part;
		int						textHandle;
		int						textFlags;
		idList< sdUIListItem* >	items;		
		idList< sdTransition >	itemTransitions;
		sdTransition			transition;

		struct flags_t {
			bool allowSort		: 1;
			bool numericSort	: 1;
			bool dataSort		: 1;
			bool sortDescending : 1;
			bool customDraw		: 1;
			bool widthPercent	: 1;	// the width field should be interpreted as a percentage of the parent window
			bool mouseFocused	: 1;
		} flags;
	};

	/*
	============
	sdColumnSort
	local functor class to sort based on the active column
	============
	*/
	class sdColumnSort {
	public:
		sdColumnSort( sdUIListColumn& c ) : column( c ) {}
		
		int operator()( const int first, const int second ) const {
			sdUIListItem* firstItem = column.items[ first ];
			sdUIListItem* secondItem = column.items[ second ];

			return idWStr::IcmpNoColor( firstItem->text.c_str(), secondItem->text.c_str() );
		}
	private:
		sdUIListColumn& column;
	};

	/*
	============
	sdColumnSortNumeric
	local functor class to sort based on the active column
	============
	*/
	class sdColumnSortNumeric {
	public:
		sdColumnSortNumeric( sdUIListColumn& c ) : column( c ) {}

		int operator()( const int first, const int second ) const {
			sdUIListItem* firstItem = column.items[ first ];
			sdUIListItem* secondItem = column.items[ second ];
			float value1 = sdTypeFromString< float >( firstItem->text.c_str() );
			float value2 = sdTypeFromString< float >( secondItem->text.c_str() );

			return value1 - value2;
		}
	private:
		sdUIListColumn& column;
	};

	/*
	============
	sdColumnSortData
	local functor class to sort based on the active column
	============
	*/
	class sdColumnSortData {
	public:
		sdColumnSortData( sdUIListColumn& c ) : column( c ) {}

		int operator()( const int first, const int second ) const {
			sdUIListItem* firstItem = column.items[ first ];
			sdUIListItem* secondItem = column.items[ second ];
			return firstItem->data.integer - secondItem->data.integer;
		}
	private:
		sdUIListColumn& column;
	};
private:

	void									Scroll( float amount );
	void									ScrollToItem( int item );
	void									ScrollPercent( float percent );

											// is the item completely visible within the client rect?
											// returns how far above or below the item is, or 0 if it is visible
	float									GetVisibleItemOffset( int item ) const;					

	void									GetItemRect( const int row, idVec4& rect ) const;	// get the full (control-width) rectangle of the item
	void									GetItemRect( const int row, const int col, idVec4& rect, const float* columnWidths ) const;	// get the column width rectangle of the item
	void									GetThumbRect( idVec4& rect ) const;
	void									GetHeaderColumnRect( const int column, idVec4& rect, const float* columnWidths ) const;

	int										GetFirstVisibleItem() const;
	int										GetLastVisibleItem( int firstVisible ) const;
	bool									IsItemVisible( int item ) const;
	float									GetHeaderHeight() const;
	float									GetMaxScrollAmount() const;
	sdUIListItem*							GetItem( int row, int column, const char* warningText = "GetItem", bool direct = false );
	const sdUIListItem*						GetItem( int row, int column, const char* warningText = "GetItem", bool direct = false ) const;
	
	bool									CheckItemClick( const sdSysEvent* event, const idVec2& point );
	bool									CheckHeaderClick( const sdSysEvent* event, const idVec2& point );
	bool									HandleKeyInput( const sdSysEvent* event );

	void									Sort();
	bool									ExtractColorFromString( const wchar_t* format, const wchar_t* indicator, idVec4& color );
	void									ExtractTextFormatFromString( const wchar_t* format, const wchar_t* indicator, int& flags );
	void									ExtractMaterialFromString( const wchar_t* format, const wchar_t* indicator, uiDrawPart_t& mat );
	void									ExtractLocalizedTextFromString( const wchar_t* format, const wchar_t* indicator, int& handle );
	void									ExtractItemFlagsFromString( const wchar_t* format, const wchar_t* indicator, sdUIListItem& item );
	void									ExtractColumnFlagsFromString( const wchar_t* format, const wchar_t* indicator, sdUIListColumn& column );
	void									ExtractColumnWidthFromString( const wchar_t* format, const wchar_t* indicator, sdUIListColumn& column );

	void									InitVec4Transition( const idVec4& from, const idVec4& to, const int time, const idStr& accel, vec4Evaluator_t& evaluator );
	void									ClearTransition( sdTransition& transition );

	void									FormatItemText( sdUIListItem& item, sdTransition& transition, const wchar_t* text );

	float									GetTotalHeight( int start = 0, int end = -1 ) const;
	float									GetVisibleHeight() const;

protected:
	static idHashMap< ListTemplateFunction* >	listFunctions;

private:
	// selection

	// rows
	SD_UI_PROPERTY_TAG(
	title			= "1. Behavior/Current Selection";
	desc			= "The current selected item";
	editor			= "edit";
	datatype		= "float";
	)
	sdFloatProperty		currentSelection;
	// ===========================================
	

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Internal Border";
	desc				= "Border to uniformly shrink items by";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		internalBorderWidth;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/1. Selected Items/Foreground Color";
	desc				= "Foreground color of a selected item";
	editor				= "edit";
	option1				= "{editorComponents} {r,g,b,a}";
	option2				= "{editorSeparator} {,}";
	datatype			= "vec4";
	aliasdatatype		= "color";
	)
	sdVec4Property 		selectedItemForeColor;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/2. Columns/Text scale";
	desc				= "Column font size";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		columnFontSize;
	// ===========================================

	// rows
	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/2. Columns/Row spacing";
	desc				= "Vertical space between rows";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		rowSpacing;
	// ===========================================

	// row height
	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Row height";
	desc				= "If specified, uses a fixed row height, set to 0 for auto-sizing.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		fixedRowHeight;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Drop Shadow Image";
	desc				= "Draw a drop shadow under images. Specifies the number of pixels to offset.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		dropShadowOffsetImage;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Active Column";
	desc				= "Column used for sorting.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		activeColumn;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Active Column Sort";
	desc				= "Direction of sorting. -1 for descending, 1 for ascending.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		activeColumnSort;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Scroll Amount";
	desc				= "Offset from top ( 0 ).";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		scrollAmount;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Scroll Amount Max";
	desc				= "Maximum offset from top, inclusive.";
	editor				= "edit";
	datatype			= "float";
	readOnly			= "true";
	)
	sdFloatProperty		scrollAmountMax;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Page Size";
	desc				= "Size in pixels of one full page of items.";
	editor				= "edit";
	datatype			= "float";
	readOnly			= "true";
	)
	sdFloatProperty		pageSize;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Column Border";
	desc				= "Column border between items.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		columnBorder;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Column Height";
	desc				= "Derived as the max height of each column's title.";
	editor				= "edit";
	datatype			= "float";
	readOnly			= "true";
	)
	sdFloatProperty		columnHeight;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Num Items";
	desc				= "Number of rows in the list.";
	editor				= "edit";
	datatype			= "float";
	readOnly			= "true";
	)
	sdFloatProperty		numItems;
	// ===========================================

	static const char* eventNames[ LE_NUM_EVENTS - WE_NUM_EVENTS ];

	sdUIEventHandle		drawColumnHandle;
	sdUIEventHandle		drawItemHandle;
	sdUIEventHandle		drawItemBackHandle;
	sdUIEventHandle		drawSelectedItemBackHandle;
	
private:
	idList< int >					indices;	
	idList< sdUIListColumn* >		columns;
	idList< int >					rowHeights;

	float							storedScrollAmount;
	int								scrollToItem;

	bool							inBatchOperation;
	float							sizeLastColumnDelta;

	idWToken						token;					// shared token for parsing format strings
};

/*
============
sdUIList::GetFirstVisibleItem
============
*/
ID_INLINE int sdUIList::GetFirstVisibleItem() const {	
	int num = GetNumItems();

	int index = 0;	
	float bottom = 0.0f;

	while( index < num - 1 ) {
		bottom += rowHeights[ index ];
		if( bottom >= scrollAmount ) {
			break;
		}
		index++;
	}
	return index;
}

/*
============
sdUIList::GetNumItems
============
*/
ID_INLINE int sdUIList::GetNumItems() const {
	if( !columns.Num()) {
		return 0;
	}

	return ( columns[ 0 ]->items.Num() );
}

/*
============
sdUIList::Scroll
============
*/
ID_INLINE void sdUIList::Scroll( float amount ) {
	scrollAmount = idMath::ClampFloat( 0.0f, GetMaxScrollAmount(), scrollAmount + amount );
}

/*
============
sdUIList::GetMaxScrollAmount
============
*/
ID_INLINE float sdUIList::GetMaxScrollAmount() const {
	float total = GetTotalHeight();
	float visible = GetVisibleHeight();
	if( visible >= total ) {
		return 0.0f;
	}
	return total - visible;
}

/*
============
sdUIList::ScrollPercent
============
*/
ID_INLINE void sdUIList::ScrollPercent( float percent ) {
	scrollAmount = idMath::ClampFloat( GetMaxScrollAmount(), 0.0f, percent * GetMaxScrollAmount() );
}

/*
============
sdUIList::ScrollToItem
============
*/
ID_INLINE void sdUIList::ScrollToItem( int item ) {
	Scroll( GetVisibleItemOffset( item ));
}

/*
============
sdUIList::GetItemRect
============
*/
ID_INLINE void sdUIList::GetItemRect( const int row, idVec4& rect ) const {
	rect.x = cachedClientRect.x;
	rect.x += borderWidth + internalBorderWidth;
	//rect.y = GetHeaderHeight() + cachedClientRect.y + ( rowHeight * row );
	rect.y = GetHeaderHeight() + cachedClientRect.y + GetTotalHeight( 0, row );
	rect.y += borderWidth + internalBorderWidth;
	rect.y -= scrollAmount;
	rect.z = cachedClientRect.z;
	//rect.w = rowHeight - rowSpacing;
	rect.w = rowHeights[ row ] - rowSpacing;
}

/*
============
sdUIList::GetItemRect
============
*/
ID_INLINE void sdUIList::GetItemRect( const int row, int col, idVec4& rect, const float* columnWidths ) const {
	GetItemRect( row, rect );
	
	int i = 0;
	while( i <= col ) {
		if( i == col ) {
			rect.z = columnWidths[ i ] - ( 2.0f * ( borderWidth + internalBorderWidth ) ) - columnBorder;
			break;
		}
		rect.x += columnWidths[ i ];
		i++;
	}	
}

/*
============
sdUIList::GetHeaderColumnRect
============
*/
ID_INLINE void sdUIList::GetHeaderColumnRect( const int column, idVec4& rect, const float* columnWidths ) const {
	assert( columnWidths != NULL );
	rect.Zero();

	rect.x = cachedClientRect.x + borderWidth;
	rect.y = cachedClientRect.y + borderWidth;
	rect.w = GetHeaderHeight();

	for( int i = 0; i < columns.Num(); i++ ) {
		if( column == i ) {
			rect.z = columnWidths[ i ];
			break;
		}
		rect.x += columnWidths[ i ];
	}
}

/*
============
sdUIList::IsItemVisible
============
*/
ID_INLINE bool sdUIList::IsItemVisible( int item ) const {
	int firstVisible = GetFirstVisibleItem();
	int lastVisible = GetLastVisibleItem( firstVisible );
	return ( item >= firstVisible ) && ( item <= lastVisible );
}

/*
============
sdUIList::GetHeaderHeight
============
*/
ID_INLINE float sdUIList::GetHeaderHeight() const {
	return TestFlag( LF_SHOW_HEADINGS ) ? columnHeight.GetValue() : 0.0f;
}

/*
============
sdUIList::IndexCheck
============
*/
ID_INLINE bool sdUIList::IndexCheck( int row, int column, const char* warningText ) const {
	if( column < 0 || 
		column >= columns.Num() ||
		row < 0 || 
		row >= columns[ 0 ]->items.Num() ) {
		if( warningText != NULL ) {
			gameLocal.Warning( "%s: %s: index out of range (row %i, column %i)", name.GetValue().c_str(), warningText, row, column );
		}
		return false;
	}
	return true;
}


/*
============
sdUIList::GetItem
============	
*/
ID_INLINE sdUIList::sdUIListItem* sdUIList::GetItem( int row, int column, const char* warningText, bool direct ) {
	if( !IndexCheck( row, column, warningText )) {
		return NULL;
	}
	if( direct ) {
		return columns[ column ]->items[ row ];
	}
	return columns[ column ]->items[ indices[ row ] ];
}

/*
============
sdUIList::GetItem
============	
*/
ID_INLINE const sdUIList::sdUIListItem* sdUIList::GetItem( int row, int column, const char* warningText, bool direct ) const {
	if( !IndexCheck( row, column, warningText )) {
		return NULL;
	}
	if( direct ) {
		return columns[ column ]->items[ row ];
	}
	return columns[ column ]->items[ indices[ row ] ];
}

/*
============
sdUIList::RemapIndex
============
*/
ID_INLINE int sdUIList::RemapIndex( int index ) const {
	if( columns.Num() == 0 || index < 0 || index >= columns[ 0 ]->items.Num() ) {
		return -1;
	}
	return indices[ index ];
}

/*
============
sdUIList::GetTotalHeight
============
*/
ID_INLINE float sdUIList::GetTotalHeight( int start, int end ) const {
	if( end == -1 ) {
		end = GetNumItems();
	}

	float total = 0.0f;
	for( int i = start; i < end; i++ ) {
		total += rowHeights[ i ];
	}
	return total;
}

/*
============
sdUIList::GetVisibleHeight
============
*/
ID_INLINE float sdUIList::GetVisibleHeight() const {
	return cachedClientRect.w - ( GetHeaderHeight() + 2.0f * ( internalBorderWidth + borderWidth ) );
}

/*
============
sdUIList::GetLastVisibleItem
============
*/
ID_INLINE int sdUIList::GetLastVisibleItem( int firstVisible ) const {
	float y = GetTotalHeight( 0, firstVisible );
	float maxY = y + GetVisibleHeight();

	int index = firstVisible;
	int num = GetNumItems();
	if( num == 0 ) {
		return -1;
	}

	while( index < num - 1 && y <= maxY ) {
		y += rowHeights[ index ];
		index++;
	}
	return index;
}


#endif // ! __GAME_GUIS_USERINTERFACELIST_H__

