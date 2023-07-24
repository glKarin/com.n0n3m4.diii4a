// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACESLIDER_H__
#define __GAME_GUIS_USERINTERFACESLIDER_H__

#include "UserInterfaceTypes.h"
#include "UIWindow.h"

extern const char sdUITemplateFunctionInstance_IdentifierSlider[];

SD_UI_PROPERTY_TAG(
	alias = "slider";
)
class sdUISlider :
	public sdUIWindow {
public:
	SD_UI_DECLARE_CLASS( sdUISlider )
	typedef enum sliderEvent_e {
		SE_MOUSE_ENTER_THUMB = WE_NUM_EVENTS + 1,
		SE_MOUSE_EXIT_THUMB,
		SE_MOUSE_ENTER_UP_ARROW,
		SE_MOUSE_EXIT_UP_ARROW,
		SE_MOUSE_ENTER_DOWN_ARROW,
		SE_MOUSE_EXIT_DOWN_ARROW,
		SE_MOUSE_ENTER_GUTTER,
		SE_MOUSE_EXIT_GUTTER,
		SE_MOUSE_DOWN_UP_ARROW,
		SE_MOUSE_UP_UP_ARROW,
		SE_MOUSE_DOWN_DOWN_ARROW,
		SE_MOUSE_UP_DOWN_ARROW,
		SE_MOUSE_DOWN_THUMB,
		SE_MOUSE_UP_THUMB,
		SE_MOUSE_DOWN_GUTTER,
		SE_MOUSE_UP_GUTTER,
		SE_BEGIN_SCROLL,
		SE_END_SCROLL,
		SE_NUM_EVENTS,
	} listEvent_t;

	typedef sdUITemplateFunction< sdUISlider > SliderTemplateFunction;
											sdUISlider();
	virtual									~sdUISlider();

	virtual const char*						GetScopeClassName() const { return "sdUISlider"; }

	virtual bool							PostEvent( const sdSysEvent* event );

	virtual void							EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache );

	virtual int								GetMaxEventTypes( void ) const { return SE_NUM_EVENTS; }
	virtual const char*						GetEventNameForType( int event ) const { return ( event < ( WE_NUM_EVENTS + 1 )) ? sdUIWindow::GetEventNameForType( event ): eventNames[ event - WE_NUM_EVENTS - 1 ]; }

	virtual sdUIFunctionInstance*			GetFunction( const char* name );
	virtual bool							RunNamedMethod( const char* name, sdUIFunctionStack& stack );

	virtual void							EndLevelLoad();

	static void								InitFunctions( void );
	static void								ShutdownFunctions( void ) { sliderFunctions.DeleteContents(); }
	static const SliderTemplateFunction*	FindFunction( const char* name );

protected:
	virtual void							DrawLocal();
	virtual void							InitProperties();
	void									OnScrollbarThumbChanged( const idStr& oldValue, const idStr& newValue );
	void									OnScrollbarArrowChanged( const idStr& oldValue, const idStr& newValue );

private:
								// these buttons are sorted in descending priority for mouse events (ie, the gutter is the last to be checked)
	enum eScrollButtonType {	NO_BUTTON = -1,	
								UP_BUTTON,		// increment position
								DOWN_BUTTON,	// decrement position
								THUMB_BUTTON,	// draggable button
								GUTTER_AREA,	// area the thumb traverses
								MAX_BUTTONS };

	enum eScrollOrientation{ SO_VERTICAL, SO_HORIZONTAL };

	enum eSliderFlag {
		SF_INTEGER_SNAP = BITT< sdUIWindow::NEXT_WINDOW_FLAG + 0 >::VALUE,
	};

	eScrollButtonType						GetButtonForPoint( const idVec2& point ) const;
	
	bool									CheckScrollbarButtonMouseOver( const sdSysEvent* event, const idVec2& point );
	bool									CheckScrollbarButtonClick( const sdSysEvent* event, const idVec2& point );
	bool									UpdateScrollbarDrag( const sdSysEvent* event, const idVec2& point );

	void									GetScrollbarButtonRect( eScrollButtonType button, idVec4& rect ) const;

	float									GetScrollPercent() const;
	void									ClampPosition( const float inputPosition );

											// returns the sign for the direction that a point is relative to the rectangle
	float									SignForPoint( const idVec2& point, const idVec4& rect ) const;

	virtual void							InitPartsForBaseMaterial( const char* material, uiCachedMaterial_t& cached );

protected:
	static idHashMap< SliderTemplateFunction* >	sliderFunctions;

private:
	// scrollbars
	SD_UI_PROPERTY_TAG(
	title				= "Object/Sliders/Range";
	desc				= "Lower and upper values.";
	editor				= "edit";
	datatype			= "vec2";
	)
	sdVec2Property		range;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Object/Sliders/Position";
	desc				= "Current position within the range";
	editor				= "edit";
	datatype			= "string";
	)
	sdFloatProperty		position;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Sliders/Orientation";
	desc				= "0 is vertical, 1 is horizontal";
	editor				= "edit";
	datatype			= "string";
	)
	sdFloatProperty		orientation;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Sliders/ThumbColor";
	desc				= "Thumb color";
	editor				= "edit";
	datatype			= "vec4";
	)
	sdVec4Property		thumbColor;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Sliders/ThumbOverlayColor";
	desc				= "Thumb overlay color";
	editor				= "edit";
	datatype			= "vec4";
	)
	sdVec4Property		thumbOverlayColor;
	// ===========================================
	
	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Sliders/FillColor";
	desc				= "Color of the filled portion, drawn up to the thumb's location";
	editor				= "edit";
	datatype			= "vec4";
	)
	sdVec4Property		fillColor;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Sliders/UpArrowColor";
	desc				= "Up arrow color.";
	editor				= "edit";
	datatype			= "vec4";
	)
	sdVec4Property		upArrowColor;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Sliders/DownArrowColor";
	desc				= "Down arrow color.";
	editor				= "edit";
	datatype			= "vec4";
	)
	sdVec4Property		downArrowColor;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Sliders/PageStep";
	desc				= "Amount to scroll when the gutter is clicked or pgup/pgdn is pressed.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		pageStep;
	// ===========================================
private:
	struct flags_t {
		bool			draggingThumb		: 1;
		bool			gutterHighlighted	: 1;
	}					flags;
	
	float				scrollDirection;
	int					lastScrollTime;
	
	eScrollButtonType	currentScrollButton;	
	eScrollButtonType	currentClickedScrollButton;
	
	typedef sdPair< listEvent_t, listEvent_t > thumbButtonEvent_t;
	static thumbButtonEvent_t thumbButtonEvents[ MAX_BUTTONS ];
	static thumbButtonEvent_t thumbButtonClickEvents[ MAX_BUTTONS ];

	static const char* eventNames[ SE_NUM_EVENTS - WE_NUM_EVENTS ];

	enum eScrollbarPart { SBP_BACK_TOP, SBP_BACK_CENTER, SBP_BACK_BOTTOM, SBP_FILL_TOP, SBP_FILL_CENTER, SBP_FILL_BOTTOM, SBP_THUMB, SBP_THUMB_OVERLAY, SBP_ARROW_UP, SBP_ARROW_DOWN, SBP_MAX };
	typedef idStaticList< uiDrawPart_t, SBP_MAX > uiDrawPartList_t;

	uiDrawPartList_t				scrollbarParts;

	static const char* partNames[ SBP_MAX ];

};

/*
============
sdUISlider::GetScrollPercent
============
*/
ID_INLINE float sdUISlider::GetScrollPercent() const {
	return idMath::Fabs( range.GetValue().y - range.GetValue().x ) * 0.1f;
}

/*
============
sdUISlider::ClampPosition
============
*/
ID_INLINE void sdUISlider::ClampPosition( const float inputPosition ) {
	float value = range.GetValue().x + ( idMath::ClampFloat( 0.0f, 1.0f, inputPosition ) * idMath::Fabs( range.GetValue().y - range.GetValue().x ) );;
	if( TestFlag( SF_INTEGER_SNAP ) ) {
		value = idMath::Floor( value );
	}
	position = value;
}

/*
============
sdUISlider::GetButtonForPoint
============
*/
ID_INLINE sdUISlider::eScrollButtonType sdUISlider::GetButtonForPoint( const idVec2& point ) const {
	idVec4 rect;

	for( int i = 0; i < MAX_BUTTONS; i++ ) {
		eScrollButtonType button = static_cast< eScrollButtonType >( i );

		GetScrollbarButtonRect( button, rect );
		if( rect.ContainsPoint( point )) {
			return button;
		}
	}

	return NO_BUTTON;
}


/*
============
sdUISlider::SignForPoint
============
*/
ID_INLINE float	sdUISlider::SignForPoint( const idVec2& point, const idVec4& rect ) const {
	if( idMath::Ftoi( orientation ) == SO_VERTICAL ) {
		if( point.y < rect.y ) {
			return -1.0f;
		} else if( point.y > rect.y + rect.w ) {
			return 1.0f;
		}
	} else {
		if( point.x < rect.x ) {
			return -1.0f;
		} else if( point.x > rect.x + rect.z ) {
			return 1.0f;
		}
	}
	return 0.0f;
}

#endif // ! __GAME_GUIS_USERINTERFACESLIDER_H__
