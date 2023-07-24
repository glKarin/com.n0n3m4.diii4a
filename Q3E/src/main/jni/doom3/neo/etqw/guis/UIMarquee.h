// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACEMARQUEE_H__
#define __GAME_GUIS_USERINTERFACEMARQUEE_H__

#include "UserInterfaceTypes.h"


/*
============
sdUIMarquee
============
*/
SD_UI_PUSH_CLASS_TAG( sdUIMarquee )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"Marquee window type is used for scrolling text vertically or horizontally. " \
	"It is used for showing the message of the day in the main menu and the spectator list in the scoreboard and limbo menu."
/* ============ */
)
SD_UI_CLASS_EXAMPLE_TAG(
/* ============ */
	" \twindowDef spectatorList {\n" \
		" \ttype marquee;\n" \
		" \tproperties {\n" \
			" \t\trect rect = PADDING, _to_bottom_of( lblSpectating ) + 2, _fill_to_right_of( specInfoSurround ), 14;\n" \
			" \t\tfloat fontSize = 12;\n" \
			" \t\tcolor forecolor = 1,1,1,0.7;\n" \
			" \t\tvec2 textAlignment = TA_LEFT, TA_VCENTER;\n" \
			" \t\tfloat speed = 9;\n" \
		" \t}\n" \
	" \n" \
		" \ttimeline {\n" \
			" \t\tonTime 500 {\n" \
				" \t\ttext = toWStr( gui.getSpectatorList() );\n" \
				" \t\tresetTime( 0 );\n" \
			" \t\t}\n" \
		" \t}\n" \
	" }\n"
/* ============ */
)
SD_UI_POP_CLASS_TAG
SD_UI_PROPERTY_TAG(
alias = "marquee";
)
class sdUIMarquee :
	public sdUIWindow {
public:
	SD_UI_DECLARE_CLASS( sdUIMarquee )
											sdUIMarquee( void );
	virtual									~sdUIMarquee( void ) { DisconnectGlobalCallbacks(); }

	virtual const char*						GetScopeClassName() const { return "sdUIMarquee"; }
	virtual sdUIFunctionInstance*			GetFunction( const char* name );

	void									Script_ResetScroll( sdUIFunctionStack& stack );

	virtual void							ApplyLayout();

	static void								InitFunctions();
	static void								ShutdownFunctions( void ) { marqueeFunctions.DeleteContents(); }

protected:	
	virtual void							DrawText( const wchar_t* text, const idVec4& color );

	void									CalcOffsets();

	static const sdUITemplateFunction< sdUIMarquee >*	FindFunction( const char* name );	

private:
	enum eScrollOrientation{ SO_VERTICAL, SO_HORIZONTAL };

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Marquee/Speed";
	desc				= "How quickly the contents scroll.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		speed;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Marquee/Orientation";
	desc				= "0 is vertical, 1 is horizontal.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		orientation;
	// ===========================================

private:
	static idHashMap< sdUITemplateFunction< sdUIMarquee >* >	marqueeFunctions;

	int					scrollStartTime;
	int					scrollTargetTime;	

	int					textWidth;
	int					textHeight;
	idVec2				scrollOffset;
};

#endif // ! __GAME_GUIS_USERINTERFACEMARQUEE_H__
