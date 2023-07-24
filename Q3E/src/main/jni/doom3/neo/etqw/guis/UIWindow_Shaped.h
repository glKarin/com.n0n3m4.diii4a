// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACEWINDOW_SHAPED_H__
#define __GAME_GUIS_USERINTERFACEWINDOW_SHAPED_H__

#include "UserInterfaceTypes.h"

extern const char sdUITemplateFunctionInstance_IdentifierShaped[];
/*
============
sdUIWindow_Shaped
============
*/
class sdUIWindow_Shaped :
	public sdUIWindow {
public:

	typedef sdUITemplateFunction< sdUIWindow_Shaped > ShapedTemplateFunction;

public:
											sdUIWindow_Shaped( void );
	virtual									~sdUIWindow_Shaped( void );

	virtual const char*						GetScopeClassName() const { return "sdUIWindow_Shaped"; }
	virtual const ShapedTemplateFunction*	FindFunction( const char* name );
	
	virtual sdUIFunctionInstance*			GetFunction( const char* name );
	static void								InitFunctions();
	static void								ShutdownFunctions( void ) { shapedFunctions.DeleteContents(); }

	virtual bool							RunNamedMethod( const char* name, sdUIFunctionStack& stack );

	void									Script_PushShapeVertex( sdUIFunctionStack& stack );

	void									Script_DrawMaterialShape( sdUIFunctionStack& stack );

protected:
	virtual void							DrawLocal();

private:
	idWinding2D							drawWinding;
	static idHashMap< ShapedTemplateFunction* >	shapedFunctions;
};

#endif // __GAME_GUIS_USERINTERFACEWINDOW_SHAPED_H__
