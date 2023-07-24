// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACEPROGRESS_H__
#define __GAME_GUIS_USERINTERFACEPROGRESS_H__

#include "UserInterfaceTypes.h"
#include "UIWindow.h"

SD_UI_PROPERTY_TAG(
alias = "progress";
)
class sdUIProgress :
	public sdUIWindow {

private:
	enum eSliderPart { SP_BEGIN, SP_CENTER, SP_END, SP_HIGHLIGHT_BEGIN, SP_HIGHLIGHT_CENTER, SP_HIGHLIGHT_END, SP_MAX };

public:
	SD_UI_DECLARE_CLASS( sdUIProgress )

	enum progressFlag_e {
		PF_DRAW_FROM_LOWER_END = BITT< sdUIWindow::NEXT_WINDOW_FLAG + 0 >::VALUE,
	};
											sdUIProgress();
	virtual									~sdUIProgress();

	virtual const char*						GetScopeClassName() const { return "sdUIProgress"; }
	virtual void							EndLevelLoad();

	static void								InitFunctions( void );
	static void								ShutdownFunctions( void );

protected:
	virtual void							DrawLocal();
	virtual void							InitProperties();
	float									GetPercent() const { return ( position - range.GetValue().x ) / ( range.GetValue().y - range.GetValue().x ); }

private:
	void									OnFillMaterialChanged( const idStr& oldValue, const idStr& newValue );
	void									OnHighlightFillMaterialChanged( const idStr& oldValue, const idStr& newValue );

	void									DrawSegment( const idVec4& color, eSliderPart begin, eSliderPart center, eSliderPart end, int xDim, int yDim, int offset, float totalDim );

private:
	enum eScrollOrientation{ SO_VERTICAL, SO_HORIZONTAL };

	SD_UI_PROPERTY_TAG(
	title				= "1. Drawing/Progress/Fill Material";
	desc				= "Base material set of the progress bar.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty 	fillMaterialName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Drawing/Progress/Highlihgt Fill Material";
	desc				= "Highlight material set of the progress bar.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty 	highlightFillMaterialName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Drawing/Progress/Segments";
	desc				= "Number of segments.";
	editor				= "edit";
	datatype			= "string";
	)
	sdFloatProperty		numSegments;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Drawing/Progress/Orientation";
	desc				= "0 is vertical, 1 is horizontal";
	editor				= "edit";
	datatype			= "string";
	)
	sdFloatProperty		orientation;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Object/Progress/Range";
	desc				= "Lower and upper values.";
	editor				= "edit";
	datatype			= "vec2";
	)
	sdVec2Property		range;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Object/Progress/Highlight Range";
	desc				= "Lower and upper highlight values.";
	editor				= "edit";
	datatype			= "vec2";
	)
	sdVec2Property		highlightRange;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Object/Progress/Position";
	desc				= "Current position within the range";
	editor				= "edit";
	datatype			= "string";
	)
	sdFloatProperty		position;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "2. Object/Progress/Highlight color";
	desc				= "Color of highlight segment";
	editor				= "edit";
	datatype			= "vec4";
	)
	sdVec4Property		highlightColor;
	// ===========================================
	
private:	
	idStaticList< uiDrawPart_t, SP_MAX > sliderParts;
};


#endif // ! __GAME_GUIS_USERINTERFACEPROGRESS_H__
