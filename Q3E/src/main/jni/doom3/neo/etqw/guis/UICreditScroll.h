// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACECREDITS_H__
#define __GAME_GUIS_USERINTERFACECREDITS_H__

#include "UserInterfaceTypes.h"


/*
============
sdUICreditScroll
============
*/
SD_UI_PROPERTY_TAG(
alias = "creditScroll";
)
class sdUICreditScroll :
	public sdUIWindow {
public:
	SD_UI_DECLARE_CLASS( sdUICreditScroll )
											sdUICreditScroll( void );
	virtual									~sdUICreditScroll( void );

	virtual const char*						GetScopeClassName() const { return "sdUICreditScroll"; }
	virtual sdUIFunctionInstance*			GetFunction( const char* name );

	void									Script_ResetScroll( sdUIFunctionStack& stack );
	void									Script_LoadFromFile( sdUIFunctionStack& stack );

	virtual void							ApplyLayout();

	static void								InitFunctions();
	static void								ShutdownFunctions( void ) { creditFunctions.DeleteContents(); }

protected:	
	virtual void							DrawLocal();
	void									ClearItems();	

	static const sdUITemplateFunction< sdUICreditScroll >*	FindFunction( const char* name );	

private:
	enum eScrollOrientation{ SO_VERTICAL, SO_HORIZONTAL };

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Credits/Speed";
	desc				= "How quickly the contents scroll.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		speed;
	// ===========================================		

private:
	static idHashMap< sdUITemplateFunction< sdUICreditScroll >* >	creditFunctions;

	class sdCreditItem :
		public sdPoolAllocator< sdCreditItem, sdPoolAllocator_DefaultIdentifier, 64 > {
	public:
		sdCreditItem() : 
			next( NULL ),
			fontSize( 0 ),
			color( colorLtGrey ),
			rect( vec4_zero ) {}
	
		idVec4				rect;
		idVec4				color;
		idWStr				text;
		uiMaterialInfo_t	material;
		int					fontSize;
		sdCreditItem*		next;
	};

private:
	void									Append( sdCreditItem* item );

private:

	sdCreditItem*		items;
	sdCreditItem*		lastItem;	// used by Append
	float				totalHeight;
	int					scrollStartTime;
	int					scrollTargetTime;
	float				scrollOffset;
};

#endif // ! __GAME_GUIS_USERINTERFACECREDITS_H__
