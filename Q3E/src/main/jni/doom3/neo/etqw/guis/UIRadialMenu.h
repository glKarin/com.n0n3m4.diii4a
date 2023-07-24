// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACERADIALMENU_H__
#define __GAME_GUIS_USERINTERFACERADIALMENU_H__

#include "UserInterfaceTypes.h"

extern const char sdUITemplateFunctionInstance_IdentifierRadialMenu[];

/*
============
sdUIRadialMenu
============
*/
SD_UI_PROPERTY_TAG(
alias = "radialmenu";
)
class sdUIRadialMenu :
	public sdUIWindow {

protected:
	typedef enum radialMenuEvent_e {
		RME_COMMAND = WE_NUM_EVENTS + 1,
		RME_MEASUREITEM,
		RME_DRAWITEM,
		RME_DRAWCONTEXT,
		RME_DRAWDEADZONE,
		RME_PAGE_PUSHED,
		RME_PAGE_POPPED,
		RME_NUM_EVENTS,
	} bindEvent_t;

	static const char* eventNames[ RME_NUM_EVENTS - WE_NUM_EVENTS ];

public:
	typedef sdUITemplateFunction< sdUIRadialMenu > uiTemplateFunction_t;
	SD_UI_DECLARE_CLASS( sdUIRadialMenu )

	enum eDrawStyle { DS_INVALID = -1, DS_ARC, DS_VERTICAL, DS_MAX };

	enum eRadialMenuFlag {
		RMF_USE_NUMBER_SHORTCUTS	= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 0 >::VALUE,
	};

											sdUIRadialMenu();
	virtual									~sdUIRadialMenu();

	virtual const char*						GetScopeClassName() const { return "sdUIRadialMenu"; }

	virtual bool							PostEvent( const sdSysEvent* event );

	virtual sdUIFunctionInstance*			GetFunction( const char* name );
	virtual bool							RunNamedMethod( const char* name, sdUIFunctionStack& stack );

	void									Script_InsertItem( sdUIFunctionStack& stack );
	void									Script_InsertPage( sdUIFunctionStack& stack );
	void									Script_GetItemData( sdUIFunctionStack& stack );
	void									Script_Clear( sdUIFunctionStack& stack );
	void									Script_ClearPage( sdUIFunctionStack& stack );
	void									Script_PostCommand( sdUIFunctionStack& stack );
	void									Script_PushPage( sdUIFunctionStack& stack );
	void									Script_PopPage( sdUIFunctionStack& stack );
	void									Script_ClearPageStack( sdUIFunctionStack& stack );
	void									Script_LoadFromDef( sdUIFunctionStack& stack );
	void									Script_AppendFromDef( sdUIFunctionStack& stack );
	void									Script_FillFromEnumerator( sdUIFunctionStack& stack );

	void									Script_TransitionItemVec4( sdUIFunctionStack& stack );
	void									Script_GetItemTransitionVec4Result( sdUIFunctionStack& stack );
	void									Script_ClearTransitions( sdUIFunctionStack& stack );

	static void								Clear( sdUIRadialMenu* menu );
	static int								InsertPage( sdUIRadialMenu* menu, int page, const char* text );
	static int								InsertItem( sdUIRadialMenu* menu, int page, const char* text );

	virtual void							EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache );
	virtual int								GetMaxEventTypes( void ) const { return RME_NUM_EVENTS; }

	virtual void							ApplyLayout();


	static void								InitFunctions( void );
	static void								ShutdownFunctions( void ) { radialMenuFunctions.DeleteContents(); }
	static const uiTemplateFunction_t*		FindFunction( const char* name );
	
private:

	enum radialItemStyle {	RIS_LEFT		= BITT< 0 >::VALUE,
							RIS_RIGHT		= BITT< 1 >::VALUE,
							RIS_CENTER		= BITT< 2 >::VALUE,
							RIS_TOP			= BITT< 3 >::VALUE,
							RIS_BOTTOM		= BITT< 4 >::VALUE,							
	};

	enum radialTransitionProperty_e{	RTP_FORECOLOR = -2,
										RTP_BACKCOLOR = -1,
										RTP_PROPERTY_0 = 0,
										RTP_PROPERTY_1 = 1,
										RTP_PROPERTY_2 = 2,
										RTP_PROPERTY_3 = 3,
										RTP_PROPERTY_MAX = 4
	};
	static const int MAX_VEC4_EVALUATORS = RTP_PROPERTY_MAX;
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

	struct radialItem_t {
		radialItem_t() :
			title( NULL ),
			owner( NULL ) {
			commandID.SetGranularity( 1 );
			commandData.SetGranularity( 1 );
			flags.enabled = true;
			flags.drawChevron = false;
			size.Zero();
			lastDrawAngle = 0.0f;
		}

		bool				Update();

		const sdDeclLocStr*	title;
		idStrList			commandID;
		idStrList			commandData;
		idStr				commandKey;
		idStr				commandNumberKey;
		idStr				drawCallback;
		uiDrawPart_t				part;
		idVec2				size;

		vec4EvaluatorList_t evaluators;

		idStr				scriptUpdateCallback;
		idStr				scriptUpdateParm;
		float				lastDrawAngle;

		struct flags_t {
			bool			enabled			: 1;
			bool			drawChevron		: 1;
		} flags;

		sdTransition		transition;
		sdUIWindow*			owner;
	};

	struct radialPage_t {
		radialPage_t() : currentItem( 0 ), popFactor( 1.5f ), title( NULL ) {}

		const sdDeclLocStr*		title;
		idList< radialItem_t >	items;
		int						currentItem;
		float					popFactor;
		idVec2					maxSize;	// dimensions of the largest items
	};

protected:
	bool									OnValidateDrawStyle( const float newValue );
	void									LoadFromDef( const sdDeclRadialMenu& def, radialPage_t& page );
	virtual void							DrawLocal();

	void									DrawItem( radialPage_t& page, radialItem_t& item, int index, const idVec2& center, const idVec2& offset, int radialItemStyle );
	void									DrawTitle( radialPage_t& page, const idVec2& center );
	
	void									OnCurrentPageChanged( const float oldValue, const float newValue );
	void									OnDrawStyleChanged( const float oldValue, const float newValue );
	bool									OnValidateCurrentItem( const float newValue );
	radialPage_t*							GetSafePage( int index ) {
												if( index < 0 || index >= pages.Num() ) {
													return NULL;
												}
												return &pages[ index ];
											}

	radialItem_t*								GetSafeItem( int pageIndex, int index ) {
												if( radialPage_t* page = GetSafePage( pageIndex )) {
													if( index >= 0 && index < page->items.Num() ) {
														return &page->items[ index ];
													}
												}
												return NULL;
											}

	void									DrawItemCircle( radialPage_t& page, const idVec2& center );
	void									DrawItemLine( radialPage_t& page, const idVec2& center );

	void									HandleArcMouseMove( const sdSysEvent* event, const idVec2 delta, radialPage_t& page, int& currentItem );

	void									ClearTransition( sdTransition& transition );
	void									InitVec4Transition( const int property, const idVec4& from, const idVec4& to, const int time, const idStr& accel, int item, int page );
	void									MoveToFirstEnabledItem( int index );


protected:
	SD_UI_PROPERTY_TAG(
	title				= "1. Object/Radius";
	desc				= "Radius of the menu";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		radius;

	SD_UI_PROPERTY_TAG(
	title				= "1. Object/CurrentPage";
	desc				= "Current page";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		currentPage;
	
	SD_UI_PROPERTY_TAG(
	title				= "1. Object/CurrentItem";
	desc				= "CurrentItem";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		currentItem;

	SD_UI_PROPERTY_TAG(
	title				= "1. Object/VerticalPaddin";
	desc				= "Vertical padding";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		verticalPadding;
	
	SD_UI_PROPERTY_TAG(
	title				= "1. Object/DrawStyle";
	desc				= "Radial menu draw style";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		drawStyle;

	idVec2				lastArcMoveInfo;
	idVec2				imaginaryCursorPos;

protected:
	static idHashMap< sdUITemplateFunction< sdUIRadialMenu >* >	radialMenuFunctions;

private:
	idList< radialPage_t >	pages;
	sdStack< int >			pageStack;

	static const int MAX_ITEMS_PER_PAGE = 10;

	int						lastActiveEventFrame;
};

#endif // ! __GAME_GUIS_USERINTERFACERADIALMENU_H__


