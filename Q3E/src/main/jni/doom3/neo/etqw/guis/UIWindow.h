// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACEWINDOW_H__
#define __GAME_GUIS_USERINTERFACEWINDOW_H__

#include "UserInterfaceTypes.h"
#include "UIObject.h"
#include "../../renderer/DeviceContext.h"

// grrr, namespace cleanup
#undef TA_LEFT
#undef TA_CENTER
#undef TA_RIGHT
#undef TA_TOP
#undef TA_VCENTER
#undef TA_BOTTOM

extern const char sdUITemplateFunctionInstance_IdentifierWindow[];

/*
============
sdUIWindow
============
*/
SD_UI_PUSH_CLASS_TAG( sdUIWindow )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"The most common window type can be used to draw text or materials within its rectangle.\n\n" \
	"In many places it is also used just for managing other windows, handling events and timelines." 
/* ============ */
)
SD_UI_CLASS_EXAMPLE_TAG(
/* ============ */
	" // Define properties required to display the warmup text at the center of the screen.\n" \
	" windowDef warmupLabel {\n" \
	" \t_big_text_props // Template: Sets the font size.\n" \
		" \tproperties {\n" \
			" \t\trect rect = 0, CENTER_Y - 28, SCREEN_WIDTH, 16;\n" \
			" \t\twstring text = player.matchStatus; // The text we're displaying\n" \
			" \t\tfloat visible	= gui.respawnLabel.visible == false && player.warmup && player.commandmapstate == 0 && player.commandmapstate == 0 && player.scoreboardActive == false && globals.gameHud.hideCrosshairCounter == 0;\n" \
			" \t\tfloat flags = immediate( flags ) | WF_DROPSHADOW;\n" \
		" \t}\n" \
	" }"
/* ============ */
)
SD_UI_POP_CLASS_TAG
SD_UI_PROPERTY_TAG(
	alias = "window";
	"1. Common" = "Expand";
	"2. Drawing" = "Expand";
	"3. Behavior" = "Collapse";
)
class sdUIWindow :
	public sdUIObject_Drawable {
public:
	SD_UI_DECLARE_CLASS( sdUIWindow )

	typedef enum windowEvent_e {
		WE_NOOP = sdUIObject::OE_NUM_EVENTS + 1,
		WE_PREDRAW,
		WE_POSTDRAW,
		WE_POSTCHILDDRAW,
		WE_MOUSEMOVE,
		WE_MOUSEENTER,
		WE_MOUSEEXIT,
		WE_KEYUP,
		WE_KEYDOWN,
		WE_KEYUP_BIND,
		WE_KEYDOWN_BIND,
		WE_SHOW,
		WE_HIDE,
		WE_GAINFOCUS,
		WE_LOSEFOCUS,
		WE_DOUBLE_CLICK,
		WE_ACTIVATE,
		WE_QUERY_TOOLTIP,
		WE_NAV_FORWARD,
		WE_NAV_BACKWARD,
		WE_ACCEPT,
		WE_CANCEL,
		WE_NUM_EVENTS,
	} windowEvent_t;

	enum windowFlag_t {
		WF_CLIP_TO_RECT			= BITT< NEXT_OBJECT_FLAG + 0 >::VALUE,
		WF_COLOR_ESCAPES		= BITT< NEXT_OBJECT_FLAG + 1 >::VALUE,
		WF_CAPTURE_KEYS			= BITT< NEXT_OBJECT_FLAG + 2 >::VALUE,
		WF_CAPTURE_MOUSE		= BITT< NEXT_OBJECT_FLAG + 3 >::VALUE,
		WF_ALLOW_FOCUS			= BITT< NEXT_OBJECT_FLAG + 4 >::VALUE,
		WF_WRAP_TEXT			= BITT< NEXT_OBJECT_FLAG + 5 >::VALUE,
		WF_MULTILINE_TEXT		= BITT< NEXT_OBJECT_FLAG + 6 >::VALUE,
		WF_AUTO_SIZE_WIDTH		= BITT< NEXT_OBJECT_FLAG + 7 >::VALUE,
		WF_AUTO_SIZE_HEIGHT		= BITT< NEXT_OBJECT_FLAG + 8 >::VALUE,
		WF_TAB_STOP				= BITT< NEXT_OBJECT_FLAG + 9 >::VALUE,
		WF_TRUNCATE_TEXT		= BITT< NEXT_OBJECT_FLAG + 10 >::VALUE,
		WF_INHERIT_PARENT_COLORS = BITT< NEXT_OBJECT_FLAG + 11 >::VALUE,
		WF_DROPSHADOW			 = BITT< NEXT_OBJECT_FLAG + 12 >::VALUE,
		// Make sure to update NEXT_WINDOW_FLAG below!
	};
	static const int NEXT_WINDOW_FLAG = NEXT_OBJECT_FLAG + 13;	// derived classes should use bit fields from this bit up to avoid conflicts

protected:	
	enum textAlignmentX_e {
		TA_LEFT,
		TA_CENTER,
		TA_RIGHT
	};

	enum textAlignmentY_e {
		TA_TOP,
		TA_VCENTER,
		TA_BOTTOM
	};

	struct windowState_t {
		bool	mouseFocused			: 1;
		bool	recalculateLayout		: 1;
		bool	lookupFont				: 1;
		bool	fullyClipped			: 1;
#ifdef _DEBUG	
		bool	breakOnDraw				: 1;
		bool	breakOnLayout			: 1;
#endif // _DEBUG
	};

public:
											sdUIWindow( void );
	virtual									~sdUIWindow( void );

	virtual const char*						GetScopeClassName() const { return "sdUIWindow"; }

	virtual void							Draw();
	virtual void							FinalDraw();

	virtual void							InitProperties();
	virtual void							EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache );
	virtual void							CreateProperties( sdUserInterfaceLocal* _ui, const sdDeclGUIWindow* windowParms, const idTokenCache& tokenCache );

	virtual const char*						GetEventNameForType( int event ) const { return ( event < ( OE_NUM_EVENTS + 1 )) ? sdUIObject::GetEventNameForType( event ): eventNames[ event - OE_NUM_EVENTS - 1 ]; }

	virtual void							OnActivate( void );
	virtual void							OnGainFocus( void );
	virtual void							OnLoseFocus( void );
	
	virtual void							SetRenderCallback( uiRenderCallback_t callback, uiRenderCallbackType_t type );
	virtual void							SetInputHandler( uiInputHandler_t handler );

	virtual void							ApplyLayout();

	virtual void							CacheEvents();

	virtual sdUIObject*						UpdateToolTip( const idVec2& cursor );

	virtual int								GetFirstEventType() const		{ return 0; }
	virtual int								GetMaxEventTypes( void ) const	{ return WE_NUM_EVENTS; }

	bool									IsVisible() const;
	bool									ParentsAllowEventPosting() const;

	void									Script_AttachRenderCallback( sdUIFunctionStack& stack );
	void									Script_AttachInputHandler( sdUIFunctionStack& stack );
	void									Script_DrawMaterialArc( sdUIFunctionStack& stack );
	void									Script_DrawTiledMaterial( sdUIFunctionStack& stack );
	void									Script_DrawRenderCallback ( sdUIFunctionStack& stack );
	void									Script_DrawMaterial( sdUIFunctionStack& stack );
	void									Script_DrawTimer( sdUIFunctionStack& stack );
	void									Script_DrawMaterialInfo( sdUIFunctionStack& stack );
	void									Script_DrawCachedMaterial( sdUIFunctionStack& stack );
	void									Script_GetCachedMaterialDimensions( sdUIFunctionStack& stack );
	void									Script_DrawRect( sdUIFunctionStack& stack );
	void									Script_DrawLine( sdUIFunctionStack& stack );
	void									Script_DrawText( sdUIFunctionStack& stack );
	void									Script_DrawLocalizedText( sdUIFunctionStack& stack );
	void									Script_RequestLayout( sdUIFunctionStack& stack );
	void									Script_NextTabStop( sdUIFunctionStack& stack );
	void									Script_PrevTabStop( sdUIFunctionStack& stack );
	void									Script_SetTabStop( sdUIFunctionStack& stack );
	void									Script_ContainsPoint( sdUIFunctionStack& stack );	
	void									Script_CacheRenderCallback( sdUIFunctionStack& stack );
	void									Script_MeasureText( sdUIFunctionStack& stack );
	void									Script_MeasureLocalizedText( sdUIFunctionStack& stack );
	void									Script_ClipToRect( sdUIFunctionStack& stack );
	void									Script_UnclipRect( sdUIFunctionStack& stack );
	void									Script_IsVisible( sdUIFunctionStack& stack );
	void									Script_PushColor( sdUIFunctionStack& stack );
	void									Script_PopColor( sdUIFunctionStack& stack );
	void									Script_SetShaderParm( sdUIFunctionStack& stack );

	virtual sdUIFunctionInstance*			GetFunction( const char* name );

	virtual void							EndLevelLoad();

	static const sdUITemplateFunction< sdUIWindow >*	FindFunction( const char* name );
	static void											InitFunctions( void );
	static void											ShutdownFunctions( void ) { windowFunctions.DeleteContents(); }

	virtual bool							HandleFocus( const sdSysEvent* event );
	virtual bool							PostEvent( const sdSysEvent* event );	
	bool									PostKeyBindingEvent( const char* key );

	virtual void							MakeLayoutDirty();

	virtual	const idVec4&					GetWorldRect() const	{ return cachedClientRect; }	

protected:
	unsigned int							GetDrawTextFlags() const;
	virtual void							DrawText( const wchar_t* text, const idVec4& color );
	static void								DrawText( const wchar_t* text, const idVec4& color, const int pointSize, const sdBounds2D& rect, qhandle_t font, const unsigned int flags );
	void									DrawText();
	void									DrawBackground( const idVec4& rect );
	void									DrawFrame( const idVec4& rect, uiMaterialCache_t::Iterator& cacheEntry, const idVec4& color );
	virtual void							DrawLocal( );		

	static void								ShortenText( const wchar_t* src, const sdBounds2D& rect, qhandle_t font, int drawFlags, int fontSize, const wchar_t* truncation, int truncationWidth, sdWStringBuilder_Heap& builder );


	void									DrawMaterial( const uiMaterialInfo_t& mi, float x, float y, float w, float h, const idVec4& color );

	static bool								IsMouseClick( const sdSysEvent* event );

	void									LookupFont();
	void									ActivateFont( bool set = true );

	bool									HandleBoundKeyInput( const sdSysEvent* event );
	bool									ParseKeys( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache, const char* identifier, int type );
	
	virtual void							InitPartsForBaseMaterial( const char* material, uiCachedMaterial_t& cached );

	void									DrawThreeHorizontalParts( const idVec4& rect, const idVec4& color, const idVec2& scale, const uiDrawPart_t& left, const uiDrawPart_t& center, const uiDrawPart_t& right );
	void									DrawFiveHorizontalParts( const idVec4& rect, const idVec4& color, const idVec2& scale, const uiDrawPart_t& left, const uiDrawPart_t& leftStretch, const uiDrawPart_t& center, const uiDrawPart_t& rightStretch, const uiDrawPart_t& right );
	void									DrawThreeVerticalParts( const idVec4& rect, const idVec4& color, const idVec2& scale, const uiDrawPart_t& top, const uiDrawPart_t& center, const uiDrawPart_t& bottom );

	void									DrawHorizontalProgress( const idVec4& rect, const idVec4& color, const idVec2& scale, const uiDrawPart_t& left, const uiDrawPart_t& center, const uiDrawPart_t& right );
	void									DrawVerticalProgress(  const idVec4& rect, const idVec4& color, const idVec2& scale, const uiDrawPart_t& top, const uiDrawPart_t& center, const uiDrawPart_t& bottom );

											// returns true if the window should continue with normal drawing
	bool									PreDraw();
	void									PostDraw();

	void									BeginLayout();
	idVec2									CalcWorldOffset() const;
	void									EndLayout();
	

private:
											sdUIWindow( const sdUIWindow& );
	sdUIWindow&								operator=( const sdUIWindow& );

	void									OnAlignmentChanged( const idVec2& oldValue, const idVec2& newValue );
	void									OnTextOffsetChanged( const idVec2& oldValue, const idVec2& newValue );
	void									OnVisibleChanged( const float oldValue, const float newValue );
	void									OnFontSizeChanged( const float oldValue, const float newValue );
	void									OnTextChanged( const idWStr& oldValue, const idWStr& newValue );
	void									OnLocalizedTextChanged( const int& oldValue, const int& newValue );
	void									OnClientRectChanged( const idVec4& oldValue, const idVec4& newValue );
	void									OnMaterialChanged( const idStr& oldValue, const idStr& newValue );
	void									OnFontChanged( const idStr& oldValue, const idStr& newValue );
	void									OnFlagsChanged( const float oldValue, const float newValue );	

	void									Show_r( bool show );
	void									ClearCapture_r( sdUIWindow* window );
	
											// returns an object that is a tab stop (or NULL if none exist), starting with object and searching its children recursively
	const sdUIObject*						IsTabStop( const sdUIObject* object ) const;
	int										NumTabStops_r( const sdUIObject* object ) const;
	void									ListTabStops_r( const sdUIObject* object, const sdUIObject** objects, int& index ) const;
	bool									ClipBounds( sdBounds2D& bounds );
	
protected:	
	uiRenderCallback_t						renderCallback[ UIRC_MAX ];
	uiInputHandler_t						inputHandler;
	
	SD_UI_PROPERTY_TAG(
	title				= "1. Common/Background Color";
	desc				= "RGBA of the window rectangle. If a material is specified, the material will be modulated by this color.";
	editor				= "edit";
	option1				= "{editorComponents} {r,g,b,a}";
	option2				= "{editorSeparator} {,}";
	datatype			= "vec4";
	aliasdatatype		= "color";
	)
	sdVec4Property		backColor;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Common/Foreground Color";
	desc				= "RGBA of the window text.";
	editor				= "edit";
	option1				= "{editorComponents} {r,g,b,a}";
	option2				= "{editorSeparator} {,}";
	datatype			= "vec4";
	aliasdatatype		= "color";
	)
	sdVec4Property		foreColor;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Common/Text";
	desc				= "Text displayed within the window.";
	editor				= "edit";
	datatype			= "wstring";
	)
	sdWStringProperty	text;

	// ===========================================
	SD_UI_PROPERTY_TAG(
	title				= "1. Common/LocalizedText";
	desc				= "Text displayed within the window.";
	editor				= "edit";
	datatype			= "int";
	)
	sdIntProperty		localizedText;

	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Alignment";
	desc				= "Controls how the text is drawn within the window. '0' is left-aligned, '1' is centered, '2' is right-aligned for x, '0' is top-aligned, '1' is centered, '2' is bottom-aligned for y.";
	option2				= "{editorComponents} {x,y}";
	option2				= "{editorSeparator} {,}";
	editor				= "edit";
	datatype			= "vec2";
	)
	sdVec2Property		textAlignment;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Common/Font Size";
	desc				= "Size of the window's text in points.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		fontSize;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Common/Text Offset";
	desc				= "Offset added to the auto-calculated text layout";
	editor				= "edit";
	datatype			= "float";
	)
	sdVec2Property		textOffset;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Text Font";
	desc				= "The font to be used to draw the window text.";
	editor				= "edit";
	datatype			= "string";
	alias				= "font";
	)
	sdStringProperty	fontName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Common/Rect";
	desc				= "Window's rectangle";
	editor				= "edit";
	option1				= "{editorComponents} {x,y,w,h}";
	option2				= "{editorSeparator} {,}";	
	datatype			= "vec4";
	aliasdatatype		= "rect";
	alias				= "rect";
	)
	sdVec4Property		clientRect;
	// ===========================================
	
	SD_UI_PROPERTY_TAG(
	title				= "1. Common/Material";
	desc				= "Window's material.";
	editor				= "edit";
	datatype			= "string";
	alias				= "material";
	option1				= "{browseTable} {materials}";
	)
	sdStringProperty	materialName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "3. Behavior/Tooltip";
	desc				= "Text displayed in the tooltip window";
	editor				= "edit";
	datatype			= "wstring";
	)
	sdWStringProperty	toolTipText;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Common/Visible";
	desc				= "When enabled the window will be drawn and respond to events.";
	editor				= "edit";
	datatype			= "float"
	)
	sdFloatProperty		visible;
	// ===========================================
	
	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Material Scale";
	desc				= "Number of repetitions for the material. Negative values will flip the material for that axis.";
	option1				= "{editorComponents} {x,y}";
	option2				= "{editorSeparator} {,}";
	editor				= "edit";
	datatype			= "vec2";
	)
	sdVec2Property		materialScale;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Material Shift";
	desc				= "Number of pixels to shift the material.";
	option1				= "{editorComponents} {x,y}";
	option2				= "{editorSeparator} {,}";
	editor				= "edit";
	datatype			= "vec2";
	)
	sdVec2Property		materialShift;
	// ===========================================
	
	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Border Width";
	desc				= "Width of the window's border.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		borderWidth;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Border Color";
	desc				= "Color of the border. Requires borderWidth to be greater than 0.";
	editor				= "edit";
	option1				= "{editorComponents} {r,g,b,a}";
	option2				= "{editorSeparator} {,}";
	datatype			= "vec4";
	aliasdatatype		= "color";
	)
	sdVec4Property		borderColor;
	// ===========================================
	
	SD_UI_PROPERTY_TAG(
	title				= "3. Behavior/Allow Events";
	desc				= "When disabled, the window (and its children) won't respond to any events.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		allowEvents;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "3. Behavior/Allow Child Events";
	desc				= "Allow children to receive events.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		allowChildEvents;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Rotation";
	desc				= "Rotation (in degrees) that the window's text and material will be drawn at.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		rotation;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Drawing/Color Multiplier";
	desc				= "Color value that is applied to all window drawing";
	editor				= "edit";
	datatype			= "float";
	)
	sdVec4Property		colorMultiplier;
	// ===========================================

	// convenience properties
	SD_UI_PROPERTY_TAG(
	title				= "1. Common/Rect";
	desc				= "Window's absolute rectangle.";
	editor				= "edit";
	option1				= "{editorComponents} {x,y,w,h}";
	option2				= "{editorSeparator} {,}";	
	datatype			= "vec4";
	readOnly			= "true";
	)
	sdVec4Property		absoluteRect;
	// ===========================================

	windowState_t		windowState;

	sdUIEventHandle		preDrawHandle;
	sdUIEventHandle		postDrawHandle;
	sdUIEventHandle		postDrawChildHandle;
	
protected:
	// cache the text origin until certain properties are flagged dirty (text, window size, etc)
	idVec4						cachedClientRect;
	sdBounds2D					cachedClippedRect;
	qhandle_t					cachedFontHandle;

	static idHashMap< sdUITemplateFunction< sdUIWindow >* >	windowFunctions;

	static const char*			eventNames[ WE_NUM_EVENTS - OE_NUM_EVENTS ];	
};

/*
============
sdUIWindow::DrawMaterial
============
*/
ID_INLINE void sdUIWindow::DrawMaterial( const uiMaterialInfo_t& mi, float x, float y, float w, float h, const idVec4& color ) {
	deviceContext->DrawMaterial( x, y, w, h, mi.material, color, mi.st0, mi.st1 );
}

#endif // __GAME_GUIS_USERINTERFACEWINDOW_H__
