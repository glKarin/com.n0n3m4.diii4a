
#ifndef __GAME_GUIS_USERINTERFACENEWS_H__
#define __GAME_GUIS_USERINTERFACENEWS_H__

#include "UserInterfaceTypes.h"


/*
============
sdUINews
============
*/
class sdUINews : public sdUIWindow {
public:
	SD_UI_DECLARE_CLASS( sdUINews )

	typedef enum windowEvent_e {
		NE_NOOP = sdUIWindow::WE_NUM_EVENTS + 1,
		NE_DRAW_BUTTON,
		NE_MOUSE_ENTER_PREV,
		NE_MOUSE_EXIT_PREV,
		NE_MOUSE_ENTER_NEXT,
		NE_MOUSE_EXIT_NEXT,
		NE_MOUSE_BUTTON_DOWN_PREV,
		NE_MOUSE_BUTTON_UP_PREV,
		NE_MOUSE_BUTTON_DOWN_NEXT,
		NE_MOUSE_BUTTON_UP_NEXT,
		NE_MOUSE_UP_TEXT,
		NE_MOUSE_DOWN_TEXT,
		NE_NUM_EVENTS,
	} windowEvent_t;

	enum eFadeType { FT_VSCROLL, FT_FADE };
	enum eItemState { IS_INVALID, IS_SCROLLIN, IS_FIXED, IS_SCROLLOUT };
	enum eButtonRect { BR_NONE = -1, BR_PREV, BR_NEXT, BR_TEXT };
	enum eUnderlineState { US_NONE, US_LINK, US_HIGHLIGHTED_LINK };

											sdUINews( void );
	virtual									~sdUINews( void );

	virtual const char*						GetScopeClassName() const { return "sdUINews"; }

	virtual sdUIFunctionInstance*			GetFunction( const char* name );
	static void								InitFunctions();
	static void								ShutdownFunctions( void ) { newsFunctions.DeleteContents(); }

	virtual void							ApplyLayout();

	virtual void							DrawLocal();

	virtual void							CacheEvents();
	void									EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache );
	virtual int								GetMaxEventTypes( void ) const { return NE_NUM_EVENTS; }
	virtual const char*						GetEventNameForType( int event ) const { return ( event < ( WE_NUM_EVENTS + 1 )) ? sdUIWindow::GetEventNameForType( event ): eventNames[ event - WE_NUM_EVENTS - 1 ]; }
	virtual bool							PostEvent( const sdSysEvent* event );

	void									Script_PrevNewsItem( sdUIFunctionStack& stack );
	void									Script_NextNewsItem( sdUIFunctionStack& stack );
	void									Script_GotoURL( sdUIFunctionStack& stack );
protected:

	static const sdUITemplateFunction< sdUINews >*	FindFunction( const char* name );

	void									CalcOffsets();

	bool									HasNewsItem() { return ( currentItem >= 0 && currentItem < items.Num() && items.Num() > 0 ); }
	void									SetNewsItem( int index );
	void									MakeDefaultMotD();

	void									FillNewsList();

	void									DrawButtons();

	bool									CheckButtonMouseOver( const sdSysEvent* event, const idVec2& point );
	bool									CheckButtonClick( const sdSysEvent* event, const idVec2& point );

	eButtonRect								GetButtonForPoint( const idVec2& point ) const;
private:

	static idHashMap< sdUITemplateFunction< sdUINews >* >	newsFunctions;

	static const char*			eventNames[ NE_NUM_EVENTS - WE_NUM_EVENTS ];	

	typedef sdPair< idWStr, idStr > motdItem_t;
	idList< motdItem_t >	items;

	int					scrollStartTime;
	int					scrollTargetTime;
	int					currentItem;
	int					textWidth;
	int					textHeight;
	idVec2				scrollOffset;
	int					displayStartTime;
	float				alphaMul;
	idVec4				buttonRect[ 2 ];
	eButtonRect			currentButton;
	eButtonRect			currentClickedButton;

	sdUIEventHandle		drawButtonHandle;

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Marquee/Speed";
	desc				= "How quickly the contents scroll/How quickly the contents fade.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		speed;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/News/FadeType";
	desc				= "Type of fading on change between news items. See FT_* defines.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		fadeType;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/News/DisplayTime";
	desc				= "For how long to display a news item.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		displayTime;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/News/ColorLink";
	desc				= "Color of highlighted link.";
	editor				= "edit";
	datatype			= "vec4";
	)
	sdVec4Property		colorLink;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/News/LinkState";
	desc				= "Text has link. See US_* defines.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		linkState;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/News/NewsText";
	desc				= "News text displayed.";
	editor				= "edit";
	datatype			= "wstring";
	)
	sdWStringProperty	newsText;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/News/PauseNews";
	desc				= "Pause news ticker.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty	pauseNews;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/News/ScrollState";
	desc				= "State of text. See IS_* defines.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty	itemState;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/News/Url";
	desc				= "Read only property. URL to go to if clicking link.";
	editor				= "edit";
	datatype			= "wstring";
	)
	sdWStringProperty	url;
	// ===========================================
};

#endif