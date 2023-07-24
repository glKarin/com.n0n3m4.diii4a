// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_GUIS_USERINTERFACEEDIT_H__
#define __GAME_GUIS_USERINTERFACEEDIT_H__

#include "UserInterfaceTypes.h"
#include "UIEditHelper.h"

//===============================================================
//
//	sdUIEdit
//
//===============================================================

extern const char sdUITemplateFunctionInstance_IdentifierEdit[];

SD_UI_PUSH_CLASS_TAG( sdUIEdit )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"An edit window should be used to get text input from the player. It is used for entering a server password, player name, " \
	"clan tag, renaming the fireteam and chat text among other things. If the edit window should support non ASCII characters then " \
	"editw should be used instead.\n\n" \

	"Templates are used to decrease the amount of text needed to create an edit box, the _edit and _end_edit templates should be used."
/* ============ */
)
SD_UI_CLASS_EXAMPLE_TAG(
/* ============ */
	" _edit( createServerName, 100, 0, 100, BUTTON_HEIGHT )\n" \
		" \t_cvar_set_edit( si_name )\n" \
		" \t_draw_left_edit_label( localize( \"guis/mainmenu/servername\" ), COLOR_TEXT, 100 )\n" \
		" \tproperties {\n" \
			" \t\tfloat maxTextLength = 30;\n" \
		" \t}\n" \
	" _end_edit\n"
/* ============ */
)
SD_UI_POP_CLASS_TAG
SD_UI_PROPERTY_TAG(
alias = "edit";
)
class sdUIEdit :
	public sdUIWindow {
friend class sdUIEditHelper< sdUIEdit, idStr, char >;
public:
	enum eEditFlag {
		EF_INTEGERS_ONLY	= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 0 >::VALUE,
		EF_ALLOW_DECIMAL	= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 1 >::VALUE,
		EF_UPPERCASE		= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 2 >::VALUE,
		EF_MULTILINE		= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 3 >::VALUE,
	};

	typedef enum editEvent_e {
		EE_INPUT_FAILED = WE_NUM_EVENTS + 1,
		EE_NUM_EVENTS,
	} editEvent_t;

	SD_UI_DECLARE_CLASS( sdUIEdit )
		sdUIEdit();
		virtual									~sdUIEdit();

		virtual void							InitProperties();
		virtual const char*						GetScopeClassName() const { return "sdUIEdit"; }

		virtual bool							PostEvent( const sdSysEvent* event );

		virtual sdUIFunctionInstance*			GetFunction( const char* name );
		virtual bool							RunNamedMethod( const char* name, sdUIFunctionStack& stack );

		virtual void							OnGainFocus( void );

		void									Script_ClearText( sdUIFunctionStack& stack );
		void									Script_IsWhitespace( sdUIFunctionStack& stack );
		void									Script_InsertText( sdUIFunctionStack& stack );
		void									Script_SurroundSelection( sdUIFunctionStack& stack );
		void									Script_AnySelected( sdUIFunctionStack& stack );
		void									Script_SelectAll( sdUIFunctionStack& stack );

		sdBounds2D								GetDrawRect() const;


		static void										InitFunctions( void );
		static void										ShutdownFunctions( void ) { editFunctions.DeleteContents(); }
		static const sdUITemplateFunction< sdUIEdit >*	FindFunction( const char* name );

		virtual void							EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache );
		virtual int								GetMaxEventTypes( void ) const { return EE_NUM_EVENTS; }
		virtual const char*						GetEventNameForType( int event ) const { return ( event < ( WE_NUM_EVENTS + 1 )) ? sdUIWindow::GetEventNameForType( event ): eventNames[ event - WE_NUM_EVENTS - 1 ]; }


protected:
	virtual void							DrawLocal();
	virtual void							ApplyLayout();
	virtual void							EndLevelLoad();

private:
	void									ClearText();
	static bool								CharIsPrintable( char c ) { return idStr::CharIsPrintable( c ); }
	const wchar_t*							GetCursorText() const { return L""; }

	void									OnEditTextChanged( const idStr& oldValue, const idStr& newValue );
	void									OnReadOnlyChanged( const float oldValue, const float newValue );
	void									OnCursorNameChanged( const idStr& oldValue, const idStr& newValue );
	void									OnOverwriteCursorNameChanged( const idStr& oldValue, const idStr& newValue );
	void									OnFlagsChanged( const float oldValue, const float newValue );

protected:
	SD_UI_PROPERTY_TAG(
	title				= "Object/Edit Text";
	desc				= "Editable text displayed within the object.";
	editor				= "editText";
	datatype			= "string";
	)
	sdStringProperty	editText;

	SD_UI_PROPERTY_TAG(
	title				= "Object/Max Length";
	desc				= "Maximum allowed length for the text";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty	maxTextLength;

	SD_UI_PROPERTY_TAG(
	title				= "Object/Read Only";
	desc				= "Don't allow editing of the text";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty	readOnly;

	SD_UI_PROPERTY_TAG(
	title				= "Object/Password";
	desc				= "Don't allow editing of the text";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty	password;

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Cursor";
	desc				= "Normal editing mode cursor";
	editor				= "edit";
	datatype			= "String";
	)
	sdStringProperty	cursorName;

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Overwrite Cursor";
	desc				= "Insert mode cursor";
	editor				= "edit";
	datatype			= "String";
	)
	sdStringProperty	overwriteCursorName;

	sdVec2Property		scrollAmount;
	sdFloatProperty		scrollAmountMax;
	sdFloatProperty		lineHeight;

protected:
	static idHashMap< sdUITemplateFunction< sdUIEdit >* >	editFunctions;
	static const char* eventNames[ EE_NUM_EVENTS - WE_NUM_EVENTS ];

private:
	uiDrawPart_t cursor;
	uiDrawPart_t overwriteCursor;

	sdUIEditHelper< sdUIEdit, idStr, char >	helper;
};

/*
============
sdUIEdit::GetDrawRect
============
*/
ID_INLINE sdBounds2D sdUIEdit::GetDrawRect() const {
	sdBounds2D rect( GetWorldRect() );
	rect.TranslateSelf( textOffset.GetValue() );
	rect.GetMaxs() -= ( textOffset.GetValue() * 2.0f );
	return rect;
}

#endif // ! __GAME_GUIS_USERINTERFACEEDIT_H__
