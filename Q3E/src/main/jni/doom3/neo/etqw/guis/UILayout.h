// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACELAYOUT_H__
#define __GAME_GUIS_USERINTERFACELAYOUT_H__

#include "UserInterfaceTypes.h"
#include "UIObject.h"


extern const char sdUITemplateFunctionInstance_IdentifierLayout_Table[];

/*
============
sdUILayout_Static
============
*/
SD_UI_PUSH_CLASS_TAG( sdUILayout_Static )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"The static layout can be used to surround other windows in a horizontal or vertical layout."
/* ============ */
)
SD_UI_POP_CLASS_TAG
SD_UI_PROPERTY_TAG(
	   alias = "layoutStatic";
)
class sdUILayout_Static :
	public sdUIObject_Drawable {
public:
	SD_UI_DECLARE_CLASS( sdUILayout_Static )

											sdUILayout_Static();
	virtual									~sdUILayout_Static();

	virtual void							Draw();
	virtual void							FinalDraw();
	virtual bool							PostEvent( const sdSysEvent* event );
	virtual bool							HandleFocus( const sdSysEvent* event );
	virtual void							ApplyLayout();
	virtual void							MakeLayoutDirty();
	virtual sdUIObject*						UpdateToolTip( const idVec2& point );

	virtual const char*						GetScopeClassName() const { return "sdUILayout_Static"; }	

protected:
	void									OnRectChanged( const idVec4& oldValue, const idVec4& newValue );	
	virtual void							DoApplyLayout() {}	

protected:
	SD_UI_PROPERTY_TAG(
	title				= "1. Layout/Rect";
	desc				= "Window's rectangle";
	editor				= "edit";
	option1				= "{editorComponents} {x,y,w,h}";
	option2				= "{editorSeparator} {,}";	
	datatype			= "vec4";
	)
	sdVec4Property							rect;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Layout/Rect";
	desc				= "Window's absolute rectangle.";
	editor				= "edit";
	option1				= "{editorComponents} {x,y,w,h}";
	option2				= "{editorSeparator} {,}";	
	datatype			= "vec4";
	readOnly			= "true";
	)
	sdVec4Property							absoluteRect;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Layout/Visible";
	desc				= "When enabled the window will be drawn and respond to events.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		visible;
	// ===========================================

	bool									recalculateLayout;

};

/*
============
sdUILayout_StaticBase
============
*/
class sdUILayout_StaticBase :
	public sdUILayout_Static {
public:
	SD_UI_DECLARE_CLASS( sdUILayout_StaticBase )
	typedef sdUITemplateFunction< sdUILayout_StaticBase > sdUILayoutFunction;

	enum layoutFlag_e {
		VLF_DRAW_REVERSED		= BITT< 0 >::VALUE,
		VLF_NOSIZE				= BITT< 1 >::VALUE,
		VLF_DYNAMIC_CHILDREN	= BITT< 2 >::VALUE,	// update layout when children's rectangles change
	};
	static const int NEXT_BIT_FLAG = 2;

											sdUILayout_StaticBase();
	virtual									~sdUILayout_StaticBase();

	virtual void							OnCreate();	
	virtual void							Draw();
	virtual void							FinalDraw();
	virtual bool							PostEvent( const sdSysEvent* event );
	virtual bool							HandleFocus( const sdSysEvent* event );

	virtual const char*						GetScopeClassName() const = 0;

	virtual sdUIFunctionInstance*			GetFunction( const char* name );

	static const sdUILayoutFunction*		FindFunction( const char* name );
	static void								InitFunctions( void );
	static void								ShutdownFunctions( void ) { layoutFunctions.DeleteContents(); }

protected:
	virtual void							DoApplyLayout() = 0;
	void									OnChildRectChanged( const idVec4& oldValue, const idVec4& newValue );

private:
	void									OnMarginsChanged( const idVec4& oldValue, const idVec4& newValue );
	void									OnSpacingChanged( const idVec2& oldValue, const idVec2& newValue );	

protected:
	SD_UI_PROPERTY_TAG(
	title				= "1. Layout/Visible";
	desc				= "Layout should keep a margin.";
	editor				= "edit";
	option1				= "{editorComponents} {l,t,r,b}";
	option2				= "{editorSeparator} {,}";	
	datatype			= "vec4";
	)
	sdVec4Property							margins;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Layout/Visible";
	desc				= "Spacing between child windows. Use x for the horizontal layout and y for the vertical layout.";
	editor				= "edit";
	option1				= "{editorComponents} {h,v}";
	option2				= "{editorSeparator} {,}";	
	datatype			= "vec2"
	)
	sdVec2Property							spacing;
	// ===========================================

private:
	static idHashMap< sdUILayoutFunction* >	layoutFunctions;

protected:
	struct rectInfo_t {
		sdUIObject* 				source;
		idVec4						rect;
		sdProperties::sdProperty*	property;
	};
	idListGranularityOne< rectInfo_t > childRects;
};

/*
============
sdUILayout_Vertical
============
*/
SD_UI_PUSH_CLASS_TAG( sdUILayout_Vertical )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"A vertical layout will automatically order child windows vertically. All child window rectangles are relative to the " \
	"the previously defined child rectangle."
/* ============ */
)
SD_UI_CLASS_EXAMPLE_TAG(
/* ============ */
	" windowDef lytLeft {\n" \
		" \ttype layoutVertical;\n" \
		" \tproperties {\n" \
			" \t\trect rect = PADDING, _top( awards ), _client_dimension( awards, width ) * 0.5f, _fill_to_bottom_of( awards );\n" \
			" \t\tvec4 margins = 0, 0, 0, 0; // No layout margins on any of the edges\n" \
		" \t}\n" \
		" \n" \
		" \t// Place each button directly below the previously defined button\n" \
		" \t_rating_button( Soldier,     \"best_soldier\", 	localize( \"guis/mainmenu/bestsoldier\" ), 	PR_BEST_SOLDIER, 		0, 0 )\n" \
		" \t_rating_button( Medic,       \"best_medic\", 		localize( \"guis/mainmenu/bestmedic\" ), 		PR_BEST_MEDIC,			0, 0 )\n" \
		" \t_rating_button( Engineer,    \"best_engineer\", 	localize( \"guis/mainmenu/bestengineer\" ), 	PR_BEST_ENGINEER, 		0, 0 )\n" \
		" \t_rating_button( FieldOps,    \"best_fieldops\", 	localize( \"guis/mainmenu/bestfieldops\" ), 	PR_BEST_FIELDOPS, 		0, 0 )\n" \
		" \t_rating_button( CovertOps,   \"best_covertops\", 	localize( \"guis/mainmenu/bestcovertops\" ), 	PR_BEST_COVERTOPS, 		0, 0 )\n" \
		" \t_rating_button( BattleSense, \"best_battlesense\", localize( \"guis/mainmenu/bestbattlesense\" ),PR_BEST_BATTLESENSE, 	0, 0 )\n" \
		" \t_rating_button( Weapons,     \"best_weapons\", 	localize( \"guis/mainmenu/bestweapons\" ), 	PR_BEST_LIGHTWEAPONS, 	0, 0 )\n" \
		" \t_rating_button( Vehicles,    \"best_vehicles\", 	localize( \"guis/mainmenu/bestvehicles\" ), 	PR_BEST_VEHICLE, 		0, 0 )\n" \
	" }"
/* ============ */
)
SD_UI_POP_CLASS_TAG
SD_UI_PROPERTY_TAG(
	   alias = "layoutVertical";
)
class sdUILayout_Vertical :
	public sdUILayout_StaticBase {
public:
	SD_UI_DECLARE_CLASS( sdUILayout_Vertical )

											sdUILayout_Vertical();
	virtual									~sdUILayout_Vertical() {}
	virtual const char*						GetScopeClassName() const { return "sdUILayout_Vertical"; }

private:
	SD_UI_PROPERTY_TAG(
	title				= "1. Drawing";
	desc				= "The vertical size of the layout.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		verticalSize;
	// ===========================================
private:
	virtual void							DoApplyLayout();
};

/*
============
sdUILayout_Horizontal
============
*/
SD_UI_PUSH_CLASS_TAG( sdUILayout_Horizontal )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"A horizontal layout will automatically order child windows horizontally. All child window rectangles are relative to the " \
	"the previously defined child rectangle."
/* ============ */
)
SD_UI_CLASS_EXAMPLE_TAG(
/* ============ */
	" windowDef lytMedals {\n" \
		" \ttype layoutHorizontal;\n" \
		" \tproperties {\n" \
			" \t\trect rect = PADDING, 150, _fill_to_right_of( statsPlayerBanner_Achievements ), 210;\n" \
			" \t\tstring class;\n" \
			" \t\tfloat selectedItem = -1;\n" \
		" \t}\n" \
		" \n" \
		" \t// Place each button directly to the right of the previously defined button\n" \
		" \t_achievements_list( Soldier,      \"soldier\" )\n" \
		" \t_achievements_list( Medic,        \"medic\" )\n" \
		" \t_achievements_list( Engineer,     \"engineer\" )\n" \
		" \t_achievements_list( FieldOps,     \"fieldops\" )\n" \
		" \t_achievements_list( CovertOps,    \"covertops\" )\n" \
		" \t_achievements_list( BattleSense,  \"battlesense\" )\n" \
		" \t_achievements_list( LightWeapons, \"lightweapons\" )\n" \
		" \t_achievements_list( Vehicles,     \"vehicles\" )\n" \
	" }"
/* ============ */
)
SD_UI_POP_CLASS_TAG
SD_UI_PROPERTY_TAG(
	   alias = "layoutHorizontal";
)
class sdUILayout_Horizontal :
	public sdUILayout_StaticBase {
public:
	SD_UI_DECLARE_CLASS( sdUILayout_Horizontal )

	virtual									~sdUILayout_Horizontal() {}
	virtual const char*						GetScopeClassName() const { return "sdUILayout_Horizontal"; }

private:
	virtual void							DoApplyLayout();
};

#endif // __GAME_GUIS_USERINTERFACELAYOUT_H__
