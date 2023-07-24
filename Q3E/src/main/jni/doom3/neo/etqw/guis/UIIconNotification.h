// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACEICONNOTIFICATION_H__
#define __GAME_GUIS_USERINTERFACEICONNOTIFICATION_H__

#include "UserInterfaceTypes.h"
#include "UIWindow.h"

extern const char sdUITemplateFunctionInstance_IdentifierSlider[];

class sdUIIconNotification;
/*
============
sdUINotifyIcon
============
*/
class sdUINotifyIcon {
public:
	enum eColorAnimation	{ CA_NONE, CA_FADE };
	enum eOriginAnimation	{ OA_NONE, OA_SLIDING };
	enum eSizeAnimation		{ SA_NONE, SA_BUMP };

	typedef idLinkList< sdUINotifyIcon > iconNode_t;

	typedef sdTransitionEvaluator< idVec4 >	colorEvaluator_t;
	typedef sdTransitionEvaluator< idVec2 >	originEvaluator_t;
	typedef sdTransitionEvaluator< float >	floatEvaluator_t;

	static const float ICON_FADE_TIME;
	static const float ICON_SLIDE_TIME;
	static const float ICON_SIZE_TIME;

	sdUINotifyIcon( sdUIIconNotification& parent_ ) :
		parent( parent_ ),
		color( colorWhite ),
		colorAnimation( CA_NONE ),
		layoutOriginAnimation( OA_NONE ),
		visualOriginAnimation( OA_NONE ),
		sizeAnimation( SA_NONE ),
		finalizeTime( 0 ),
		text( NULL ) { node.SetOwner( this ); data.ptr = NULL; }

	void					Draw( const int time, int itemId, idVec2& origin );
	void					GetDrawBounds( const int time, idVec2& origin, sdBounds2D& drawBounds );

	// other icons will be affected by changes to this origin
	void					AnimateLayoutOrigin( const originEvaluator_t& origin, eOriginAnimation anim );

	// only the icon's position will be affected by the visual origin
	void					AnimateVisualOrigin( const originEvaluator_t& origin, eOriginAnimation anim );

	void					AnimateColor( const colorEvaluator_t& color, eColorAnimation anim );
	void					AnimateSize( const floatEvaluator_t& size, eSizeAnimation anim );

	void					FinishAnimations( const int now );

	void					ScheduleDestruction( const int time )	{ finalizeTime = time; }
	bool					IsDestructionScheduled() const			{ return finalizeTime != 0; }

	bool					ShouldRemove( const int time ) const;

	void					SetColor( const idVec4& color ) { this->color = color; }
	const idVec4&			GetColor() const { return color; }

	uiDrawPart_t&			GetPart() { return part; }
	const uiDrawPart_t&		GetPart() const { return part; }

	iconNode_t&				GetNode() { return node; }
	const iconNode_t&		GetNode() const { return node; }

	void*					GetDataPtr() const { return data.ptr; }
	void					SetDataPtr( void* data ) { this->data.ptr = data; }

	int						GetDataInt() const { return data.integer; }
	void					SetDataInt( int data ) { this->data.integer = data; }

	const wchar_t*			GetText() const { return text.c_str(); }
	void					SetText( const wchar_t* text ) { this->text = text; }

private:
	void					GetAnimatedColor( const int time, idVec4& color ) const;
	void					GetAnimatedRect( const int time, sdBounds2D& rect ) const;
	void					GetAnimatedOrigin( const int time, const originEvaluator_t& evaluator, eOriginAnimation anim, idVec2& origin ) const;

private:
	colorEvaluator_t		colorEvaluator;
	eColorAnimation			colorAnimation;

	originEvaluator_t		layoutOriginEvaluator;
	eOriginAnimation		layoutOriginAnimation;

	originEvaluator_t		visualOriginEvaluator;
	eOriginAnimation		visualOriginAnimation;

	floatEvaluator_t		sizeEvaluator;
	eSizeAnimation			sizeAnimation;		

	int						finalizeTime;

	uiDrawPart_t			part;
	idVec4					color;
	iconNode_t				node;
	idWStr					text;

	union data_t {
		void*				ptr;
		int					integer;
	}						data;
	
	sdUIIconNotification&	parent;
};

SD_UI_PROPERTY_TAG(
alias = "iconNotification";
)
class sdUIIconNotification :
	public sdUIWindow {
public:
	SD_UI_DECLARE_CLASS( sdUIIconNotification )
	typedef enum eIconNotificationEvent_e {
		INE_ICON_ADDED = WE_NUM_EVENTS + 1,
		INE_ICON_REMOVED,
		INE_ICON_PREDRAW,
		INE_NUM_EVENTS,
	} listEvent_t;

	typedef sdHandles< sdUINotifyIcon* > 	icons_t;
	typedef icons_t::handle_t				iconHandle_t;

	typedef sdUITemplateFunction< sdUIIconNotification > IconNotificationTemplateFunction;
											sdUIIconNotification();
	virtual									~sdUIIconNotification();

	virtual const char*						GetScopeClassName() const { return "sdUIIconNotification"; }

	virtual void							EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache );

	virtual int								GetMaxEventTypes( void ) const { return INE_NUM_EVENTS; }
	virtual const char*						GetEventNameForType( int event ) const { return ( event < ( INE_NUM_EVENTS + 1 )) ? sdUIWindow::GetEventNameForType( event ) : eventNames[ event - INE_NUM_EVENTS - 1 ]; }

	virtual sdUIFunctionInstance*			GetFunction( const char* name );
	virtual bool							RunNamedMethod( const char* name, sdUIFunctionStack& stack );

	virtual void							EndLevelLoad();

	virtual void							CacheEvents();

	const sdUIEventHandle&					GetPreDrawHandle() const { return iconPreDrawHandle; }

	static void								InitFunctions( void );
	static void								ShutdownFunctions( void ) { iconNotificationFunctions.DeleteContents(); }
	static const IconNotificationTemplateFunction*	FindFunction( const char* name );

	void									Script_AddIcon( sdUIFunctionStack& stack );
	void									Script_RemoveIcon( sdUIFunctionStack& stack );
	void									Script_Clear( sdUIFunctionStack& stack );
	void									Script_GetFirstItem( sdUIFunctionStack& stack );
	void									Script_BumpIcon( sdUIFunctionStack& stack );
	void									Script_FillFromEnumerator( sdUIFunctionStack& stack );
	void									Script_GetItemData( sdUIFunctionStack& stack );
	void									Script_SetItemData( sdUIFunctionStack& stack );
	void									Script_SetItemText( sdUIFunctionStack& stack );
	void									Script_GetItemText( sdUIFunctionStack& stack );
	void									Script_GetItemAtPoint( sdUIFunctionStack& stack );

	iconHandle_t							AddIcon( const char* material );
	void									RemoveIcon( iconHandle_t& handle );
	void									Clear();
	void									BumpIcon( const iconHandle_t& handle, const char* table );
	void									SetItemDataPtr( const iconHandle_t& handle, void* data );
	void*									GetItemDataPtr( const iconHandle_t& handle ) const;

	void									SetItemDataInt( const iconHandle_t& handle, int data );
	int										GetItemDataInt( const iconHandle_t& handle ) const;

	void									SetItemText( const iconHandle_t& handle, const wchar_t* text );
	const wchar_t*							GetItemText( const iconHandle_t& handle ) const;

	iconHandle_t							GetFirstItem() const;
	iconHandle_t							GetNextItem( const iconHandle_t& handle ) const;

protected:
	virtual void							DrawLocal();
	virtual void							InitProperties();
	iconHandle_t							FindIcon( sdUINotifyIcon& icon );

private:
	enum eOrientation{ IO_VERTICAL, IO_HORIZONTAL, IO_HORIZONTAL_RIGHT };	

	void									GetNextOrigin( sdUINotifyIcon* icon, idVec2& origin ) const;
	void									FinishAnimations();
	void									CalculateMaxDimensions();
	idVec2									GetBaseOrigin() const;
	void									RetireIcons();

protected:
	static idHashMap< IconNotificationTemplateFunction* >	iconNotificationFunctions;

private:
	sdFloatProperty				orientation;
	sdFloatProperty				iconSpacing;
	sdFloatProperty				iconFadeTime;
	sdFloatProperty				iconSlideTime;
	sdVec2Property				iconSize;
	sdUIEventHandle				iconPreDrawHandle;
	
private:
	static const char*			eventNames[ INE_NUM_EVENTS - WE_NUM_EVENTS ];
	icons_t						icons;
	idVec2						maxIconDimensions;
	sdUINotifyIcon::iconNode_t	drawNode;
};


#endif // ! __GAME_GUIS_USERINTERFACEICONNOTIFICATION_H__
