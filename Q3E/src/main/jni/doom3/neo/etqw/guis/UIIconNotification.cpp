// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UIWindow.h"
#include "UserInterfaceLocal.h"
#include "UIIconNotification.h"
#include "UserInterfaceManager.h"

#include "../../sys/sys_local.h"
#include "../../idlib/Sort.h"


const float sdUINotifyIcon::ICON_FADE_TIME = SEC2MS( 0.5f );
const float sdUINotifyIcon::ICON_SLIDE_TIME = SEC2MS( 0.5f );
const float sdUINotifyIcon::ICON_SIZE_TIME = SEC2MS( 0.5f );


/*
============
sdUINotifyIcon::AnimateLayoutOrigin
============
*/
void sdUINotifyIcon::AnimateLayoutOrigin( const originEvaluator_t& origin, eOriginAnimation anim ) {
	layoutOriginAnimation = anim;
	layoutOriginEvaluator = origin;
}

/*
============
sdUINotifyIcon::AnimateVisualOrigin
============
*/
void sdUINotifyIcon::AnimateVisualOrigin( const originEvaluator_t& origin, eOriginAnimation anim ) {
	visualOriginAnimation = anim;
	visualOriginEvaluator = origin;
}

/*
============
sdUINotifyIcon::AnimateColor
============
*/
void sdUINotifyIcon::AnimateColor( const colorEvaluator_t& color, eColorAnimation anim ) {
	colorAnimation = anim;
	colorEvaluator = color;
}

/*
============
sdUINotifyIcon::AnimateSize
============
*/
void sdUINotifyIcon::AnimateSize( const floatEvaluator_t& size, eSizeAnimation anim ) {
	sizeAnimation = anim;
	sizeEvaluator = size;
}

/*
============
sdUINotifyIcon::Draw
============
*/
void sdUINotifyIcon::Draw( const int time, int itemId, idVec2& origin ) {
	idVec4 color;
	GetAnimatedColor( time, color );
	GetAnimatedOrigin( time, layoutOriginEvaluator, layoutOriginAnimation, origin );

	sdBounds2D drawRect( origin.x, origin.y, part.width, part.height );
	GetAnimatedRect( time, drawRect );

	const sdUIEventHandle& handle = parent.GetPreDrawHandle();
	if( handle.IsValid() ) {
		parent.GetUI()->PushScriptVar( color );
		parent.GetUI()->PushScriptVar( drawRect.ToVec4() );
		parent.GetUI()->PushScriptVar( itemId );

		parent.RunEventHandle( handle );

		parent.GetUI()->ClearScriptStack();
	}	

	deviceContext->DrawMaterial( drawRect.GetMins().x, drawRect.GetMins().y, drawRect.GetWidth(), drawRect.GetHeight(), part.mi.material, color, part.mi.st0, part.mi.st1 );
}

/*
============
sdUINotifyIcon::GetDrawBounds
============
*/
void sdUINotifyIcon::GetDrawBounds( const int time, idVec2& origin, sdBounds2D& drawBounds ) {
	GetAnimatedOrigin( time, layoutOriginEvaluator, layoutOriginAnimation, origin );

	drawBounds.FromRectangle( origin.x, origin.y, part.width, part.height );
	GetAnimatedRect( time, drawBounds );
}


/*
============
sdUINotifyIcon::FinishAnimations
============
*/
void sdUINotifyIcon::FinishAnimations( const int time ) {
	if( colorAnimation != CA_NONE && colorEvaluator.IsDone( time ) ) {
		colorAnimation = CA_NONE;
	}

	if( layoutOriginAnimation != OA_NONE && layoutOriginEvaluator.IsDone( time ) ) {
		layoutOriginAnimation = OA_NONE;
	}

	if( visualOriginAnimation != OA_NONE && visualOriginEvaluator.IsDone( time ) ) {
		visualOriginAnimation = OA_NONE;
	}

	if( sizeAnimation != SA_NONE && sizeEvaluator.IsDone( time ) ) {
		sizeAnimation = SA_NONE;
	}
}

/*
============
sdUINotifyIcon::GetAnimatedRect
============
*/
void sdUINotifyIcon::GetAnimatedRect( const int time, sdBounds2D& rect ) const {
	idVec2 size( rect.GetWidth(), rect.GetHeight() );

	switch( sizeAnimation ) {
		case SA_BUMP:
			size *= sizeEvaluator.Evaluate( time );
			rect.GetMins().x -= size.x * 0.5f;
			rect.GetMaxs().x += size.x * 0.5f;
			rect.GetMins().y -= size.y * 0.5f;
			rect.GetMaxs().y += size.y * 0.5f;
			break;
	}

	idVec2 origin( vec2_zero );
	GetAnimatedOrigin( time, visualOriginEvaluator, visualOriginAnimation, origin );
	rect.TranslateSelf( origin );
}

/*
============
sdUINotifyIcon::GetAnimatedColor
============
*/
void sdUINotifyIcon::GetAnimatedColor( const int time, idVec4& color ) const {
	color = this->color;

	switch( colorAnimation ) {
		case CA_NONE:
			break;
		case CA_FADE:
			color = colorEvaluator.Evaluate( time );
			break;
	}
}

/*
============
sdUINotifyIcon::AnimateOrigin
============
*/
void sdUINotifyIcon::GetAnimatedOrigin( const int time, const originEvaluator_t& evaluator, eOriginAnimation anim, idVec2& origin ) const {	
	switch( anim ) {
		case OA_NONE:
			break;
		case OA_SLIDING:
			origin += evaluator.Evaluate( time );
			break;
	}
}

/*
============
sdUINotifyIcon::ShouldRemove
============
*/
bool sdUINotifyIcon::ShouldRemove( const int time ) const {
	return IsDestructionScheduled() && ( time >= finalizeTime );
}


SD_UI_IMPLEMENT_CLASS( sdUIIconNotification, sdUIWindow )

idHashMap< sdUITemplateFunction< sdUIIconNotification >* > sdUIIconNotification::iconNotificationFunctions;

const char sdUITemplateFunctionInstance_IdentifierNotifcationFunction[] = "sdUIIconNotificationFunction";

SD_UI_PUSH_CLASS_TAG( sdUIIconNotification )
const char* sdUIIconNotification::eventNames[ INE_NUM_EVENTS - WE_NUM_EVENTS ] = {
	SD_UI_EVENT_TAG( "onIconAdded",			"", "Called when an item has been added." ),

	SD_UI_EVENT_TAG( "onIconRemoved",		"", "Called when an item has been removed." ),

	SD_UI_EVENT_PARM_TAG( "onPreDrawIcon",	"", "Called before drawing an icon with the item handle specified." ),
		SD_UI_EVENT_PARM( float, "itemHandle", "Item handle" )
		SD_UI_EVENT_PARM( rect, "itemRect", "Item rectangle" )
		SD_UI_EVENT_PARM( color, "itemColor", "Item color" )
	SD_UI_END_EVENT_TAG
};
SD_UI_POP_CLASS_TAG

/*
============
sdUIIconNotification::sdUIIconNotification
============
*/
sdUIIconNotification::sdUIIconNotification() {
	scriptState.GetProperties().RegisterProperty( "orientation",	orientation );
	scriptState.GetProperties().RegisterProperty( "iconSpacing",	iconSpacing );
	scriptState.GetProperties().RegisterProperty( "iconSize",		iconSize );
	scriptState.GetProperties().RegisterProperty( "iconFadeTime",	iconFadeTime );
	scriptState.GetProperties().RegisterProperty( "iconSlideTime",	iconSlideTime );

	orientation			= IO_HORIZONTAL;
	iconSpacing			= 4.0f;
	iconSize			= vec2_zero;
	maxIconDimensions	= vec2_zero;
	iconFadeTime		= sdUINotifyIcon::ICON_FADE_TIME;
	iconSlideTime		= sdUINotifyIcon::ICON_SLIDE_TIME;

	SetWindowFlag( WF_CLIP_TO_RECT );
}

/*
============
sdUIIconNotification::InitProperties
============
*/
void sdUIIconNotification::InitProperties() {
	sdUIWindow::InitProperties();
}

/*
============
sdUIIconNotification::~sdUIIconNotification
============
*/
sdUIIconNotification::~sdUIIconNotification() {
	DisconnectGlobalCallbacks();
	Clear();
}

/*
============
sdUIIconNotification::FindFunction
============
*/
const sdUIIconNotification::IconNotificationTemplateFunction* sdUIIconNotification::FindFunction( const char* name ) {
	sdUIIconNotification::IconNotificationTemplateFunction** ptr;
	return iconNotificationFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUIIconNotification::GetFunction
============
*/
sdUIFunctionInstance* sdUIIconNotification::GetFunction( const char* name ) {
	const IconNotificationTemplateFunction* function = sdUIIconNotification::FindFunction( name );
	if ( !function ) {		
		return sdUIWindow::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUIIconNotification, sdUITemplateFunctionInstance_IdentifierSlider >( this, function );
}

/*
============
sdUIIconNotification::RunNamedMethod
============
*/
bool sdUIIconNotification::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const sdUIIconNotification::IconNotificationTemplateFunction* func = sdUIIconNotification::FindFunction( name );
	if ( !func ) {
		return sdUIWindow::RunNamedMethod( name, stack );
	}

	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}

/*
============
sdUIIconNotification::DrawLocal
============
*/
void sdUIIconNotification::DrawLocal() {
	sdUIWindow::DrawLocal();

	RetireIcons();
	FinishAnimations();

	idVec2 origin = GetBaseOrigin();
	const int now = GetUI()->GetCurrentTime();

	sdUINotifyIcon* icon = drawNode.Next();
	while( icon != NULL ) { 
		iconHandle_t handle = FindIcon( *icon );
		icon->Draw( now, handle, origin );	
		GetNextOrigin( icon, origin );
		icon = icon->GetNode().Next();
	}
}

/*
============
sdUIIconNotification::InitFunctions
============
*/
#pragma inline_depth( 0 )
#pragma optimize( "", off )
SD_UI_PUSH_CLASS_TAG( sdUIIconNotification )
void sdUIIconNotification::InitFunctions() {
	SD_UI_FUNC_TAG( addIcon, "Add an icon to the list of notify icons." )
		SD_UI_FUNC_PARM( string, "material", "Material name." )
		SD_UI_FUNC_RETURN_PARM( handle, "Icon handle." )
	SD_UI_END_FUNC_TAG
	iconNotificationFunctions.Set( "addIcon",				new sdUITemplateFunction< sdUIIconNotification >( 'i', "s",	&sdUIIconNotification::Script_AddIcon ) );
	
	SD_UI_FUNC_TAG( removeIcon, "Remove an icon from the list of notify icons." )
		SD_UI_FUNC_PARM( handle, "itemHandle", "Handle of icon to remove." )
	SD_UI_END_FUNC_TAG
	iconNotificationFunctions.Set( "removeIcon",			new sdUITemplateFunction< sdUIIconNotification >( 'v', "i",	&sdUIIconNotification::Script_RemoveIcon ) );

	SD_UI_FUNC_TAG( clear, "Clear all notification icons." )
	SD_UI_END_FUNC_TAG
	iconNotificationFunctions.Set( "clear",					new sdUITemplateFunction< sdUIIconNotification >( 'v', "",	&sdUIIconNotification::Script_Clear ) );

	SD_UI_FUNC_TAG( getFirstItem, "Get the first notification icon." )
		SD_UI_FUNC_RETURN_PARM( handle, "Handle to the first icon." )
	SD_UI_END_FUNC_TAG
	iconNotificationFunctions.Set( "getFirstItem",			new sdUITemplateFunction< sdUIIconNotification >( 'i', "",	&sdUIIconNotification::Script_GetFirstItem ) );

	SD_UI_FUNC_TAG( bumpIcon, "Bump a notify icon." )
		SD_UI_FUNC_PARM( handle, "itemHandle", "Item handle." )
		SD_UI_FUNC_PARM( string, "strTable", "Table to use for bump motion." )
	SD_UI_END_FUNC_TAG
	iconNotificationFunctions.Set( "bumpIcon",				new sdUITemplateFunction< sdUIIconNotification >( 'v', "is",	&sdUIIconNotification::Script_BumpIcon ) );

	SD_UI_FUNC_TAG( fillFromEnumerator, "Fill list of notify icons from an enumerator." )
		SD_UI_FUNC_PARM( string, "name", "Enumerator name." )
	SD_UI_END_FUNC_TAG
	iconNotificationFunctions.Set( "fillFromEnumerator",	new sdUITemplateFunction< sdUIIconNotification >( 'v', "s",	&sdUIIconNotification::Script_FillFromEnumerator ) );

	SD_UI_FUNC_TAG( setItemData, "Set item data." )
		SD_UI_FUNC_PARM( handle, "itemHandle", "Item handle." )
		SD_UI_FUNC_PARM( float, "data", "Set data for an item." )
	SD_UI_END_FUNC_TAG
	iconNotificationFunctions.Set( "setItemData",			new sdUITemplateFunction< sdUIIconNotification >( 'v', "if",	&sdUIIconNotification::Script_SetItemData ) );

	SD_UI_FUNC_TAG( getItemData, "Get item data." )
		SD_UI_FUNC_PARM( handle, "itemHandle", "Item handle." )
		SD_UI_FUNC_RETURN_PARM( float, "Item data." )
	SD_UI_END_FUNC_TAG
	iconNotificationFunctions.Set( "getItemData",			new sdUITemplateFunction< sdUIIconNotification >( 'f', "i",		&sdUIIconNotification::Script_GetItemData ) );

	SD_UI_FUNC_TAG( getItemText, "Get item text." )
		SD_UI_FUNC_PARM( handle, "itemHandle", "Item handle." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Get item text." )
	SD_UI_END_FUNC_TAG
	iconNotificationFunctions.Set( "getItemText",			new sdUITemplateFunction< sdUIIconNotification >( 'w', "i",		&sdUIIconNotification::Script_GetItemText ) );

	SD_UI_FUNC_TAG( setItemText, "Set item text." )
		SD_UI_FUNC_PARM( handle, "itemHandle", "Item handle." )
		SD_UI_FUNC_PARM( wstring, "text", "Set item text." )
	SD_UI_END_FUNC_TAG
	iconNotificationFunctions.Set( "setItemText",			new sdUITemplateFunction< sdUIIconNotification >( 'v', "iw",	&sdUIIconNotification::Script_SetItemText ) );

	SD_UI_FUNC_TAG( getItemAtPoint, "Get item at position. Often used to get the list item under the cursor." )
		SD_UI_FUNC_PARM( float, "posX", "X position." )
		SD_UI_FUNC_PARM( float, "posY", "Y position." )
		SD_UI_FUNC_RETURN_PARM( handle, "Item handle at point." )
	SD_UI_END_FUNC_TAG
	iconNotificationFunctions.Set( "getItemAtPoint",		new sdUITemplateFunction< sdUIIconNotification >( 'i', "ff",	&sdUIIconNotification::Script_GetItemAtPoint ) );

	// Init code defines
	SD_UI_ENUM_TAG( IO_VERTICAL, "Vertical orientation of icons." )
	sdDeclGUI::AddDefine( va( "IO_VERTICAL %i", IO_VERTICAL ) );
	SD_UI_ENUM_TAG( IO_HORIZONTAL, "Horizontal orientation of icons." )
	sdDeclGUI::AddDefine( va( "IO_HORIZONTAL %i", IO_HORIZONTAL ) );
	SD_UI_ENUM_TAG( IO_HORIZONTAL_RIGHT, "Horizontal orientation of icons. Aligned to the right of the window rectangle." )
	sdDeclGUI::AddDefine( va( "IO_HORIZONTAL_RIGHT %i", IO_HORIZONTAL_RIGHT ) );
}
SD_UI_POP_CLASS_TAG
#pragma optimize( "", on )
#pragma inline_depth()

/*
============
sdUIIconNotification::EnumerateEvents
============
*/
void sdUIIconNotification::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	if ( !idStr::Icmp( name, "onIconAdded" ) ) {
		events.Append( sdUIEventInfo( INE_ICON_ADDED, 0 ) );
		return;
	}

	if ( !idStr::Icmp( name, "onIconRemoved" ) ) {
		events.Append( sdUIEventInfo( INE_ICON_REMOVED, 0 ) );
		return;
	}

	if ( !idStr::Icmp( name, "onPreDrawIcon" ) ) {
		events.Append( sdUIEventInfo( INE_ICON_PREDRAW, 0 ) );
		return;
	}

	sdUIWindow::EnumerateEvents( name, flags, events, tokenCache );
}

/*
============
sdUIIconNotification::EndLevelLoad
============
*/
void sdUIIconNotification::EndLevelLoad() {
	sdUIWindow::EndLevelLoad();
}


/*
============
sdUIIconNotification::CalculateMaxDimensions
============
*/
void sdUIIconNotification::CalculateMaxDimensions() {
	maxIconDimensions.Zero();
	sdUINotifyIcon* icon = drawNode.Next();
	while( icon != NULL ) {
		maxIconDimensions.x = Max< float > ( maxIconDimensions.x, icon->GetPart().width );
		maxIconDimensions.y = Max< float > ( maxIconDimensions.y, icon->GetPart().height );
		icon = icon->GetNode().Next();
	}
}

/*
============
sdUIIconNotification::Script_AddIcon
============
*/
void sdUIIconNotification::Script_AddIcon( sdUIFunctionStack& stack ) {
	idStr material;
	stack.Pop( material );
	
	iconHandle_t handle = AddIcon( material.c_str() );
	stack.Push( handle );
}

/*
============
sdUIIconNotification::Script_RemoveIcon
============
*/
void sdUIIconNotification::Script_RemoveIcon( sdUIFunctionStack& stack ) {
	int handleRaw;
	stack.Pop( handleRaw );

	iconHandle_t handle( handleRaw );
	RemoveIcon( handle );
}

/*
============
sdUIIconNotification::Script_Clear
============
*/
void sdUIIconNotification::Script_Clear( sdUIFunctionStack& stack ) {
	Clear();
}

/*
============
sdUIIconNotification::Script_BumpIcon
============
*/
void sdUIIconNotification::Script_BumpIcon( sdUIFunctionStack& stack ) {
	int handleRaw;
	stack.Pop( handleRaw );

	idStr table;
	stack.Pop( table );

	iconHandle_t handle( handleRaw );
	BumpIcon( handle, table.c_str() );
}

/*
============
sdUIIconNotification::Script_FillFromEnumerator
============
*/
void sdUIIconNotification::Script_FillFromEnumerator( sdUIFunctionStack& stack ) {
	idStr enumerator;
	stack.Pop( enumerator );

	uiIconEnumerationCallback_t callback = uiManager->GetIconEnumerationCallback( enumerator );
	if ( callback != NULL ) {
		callback( this );
	} else {
		gameLocal.Warning( "sdUIIconNotification::Script_FillFromEnumerator: '%s' Unknown enumerator '%s'", this->name.GetValue().c_str(), enumerator.c_str() );
	}
}

/*
============
sdUIIconNotification::Script_GetFirstItem
============
*/
void sdUIIconNotification::Script_GetFirstItem( sdUIFunctionStack& stack ) {
	sdUINotifyIcon* icon = drawNode.Next();
	while( icon != NULL && icon->IsDestructionScheduled() ) {
		icon = icon->GetNode().Next();
	}

	if( icon == NULL ) {
		stack.Push( iconHandle_t() );
		return;
	}

	iconHandle_t iter = icons.GetFirst();
	while( iter.IsValid() ) {
		if( icons[ iter ] == icon ) {
			break;
		}
		iter = icons.GetNext( iter );
	}
	stack.Push( iter );
}

/*
============
sdUIIconNotification::GetNextOrigin
============
*/
void sdUIIconNotification::GetNextOrigin( sdUINotifyIcon* icon, idVec2& origin ) const {
	if( icon == NULL ) {
		return;
	}

	switch( idMath::Ftoi( orientation ) ) {
		case IO_HORIZONTAL:
			origin.x += iconSpacing + icon->GetPart().width;		
			
			// wrap to next line
			if( ( origin.x + icon->GetPart().width ) > ( cachedClientRect.x + cachedClientRect.z - iconSpacing ) ) {
				origin.x = GetBaseOrigin().x;
				origin.y += iconSpacing + maxIconDimensions.y;
			}
		break;

		case IO_HORIZONTAL_RIGHT:
			origin.x -= iconSpacing + icon->GetPart().width;		

			// wrap to next line
			if( ( origin.x ) < ( cachedClientRect.x ) ) {
				origin.x = GetBaseOrigin().x;
				origin.y += iconSpacing + maxIconDimensions.y;
			}
			break;
		case IO_VERTICAL:
			origin.y += iconSpacing + icon->GetPart().height;

			// wrap to next line
			if( ( origin.y + icon->GetPart().height ) > ( cachedClientRect.y + cachedClientRect.w - iconSpacing ) ) {
				origin.x += iconSpacing + maxIconDimensions.x;
				origin.y = GetBaseOrigin().y;			
			}
		break;
	}
}

/*
============
sdUIIconNotification::GetBaseOrigin
============
*/
idVec2 sdUIIconNotification::GetBaseOrigin() const {
	switch( idMath::Ftoi( orientation ) ) {
		case IO_HORIZONTAL_RIGHT:
			return idVec2( cachedClientRect.x + cachedClientRect.z - iconSpacing - iconSize.GetValue().x, cachedClientRect.y + iconSpacing );
	}
	return idVec2( cachedClientRect.x + iconSpacing, cachedClientRect.y + iconSpacing );
}

/*
============
sdUIIconNotification::RetireIcons
============
*/
void sdUIIconNotification::RetireIcons() {
	const int now = GetUI()->GetCurrentTime();

	iconHandle_t handle = icons.GetFirst();
	while( handle.IsValid() ) {
		const sdUINotifyIcon* icon = icons[ handle ];
		if( icon->ShouldRemove( now) ) {			
			iconHandle_t current = handle;
			handle = icons.GetNext( handle );

			delete icon;
			icons.Release( current );
		} else {
			handle = icons.GetNext( handle );
		}		
	}
	CalculateMaxDimensions();
}

/*
============
sdUIIconNotification::FinishAnimations
============
*/
void sdUIIconNotification::FinishAnimations() {
	const int now = GetUI()->GetCurrentTime();

	iconHandle_t iter = icons.GetFirst();
	while( iter.IsValid() ) {
		sdUINotifyIcon* icon = icons[ iter ];
		icon->FinishAnimations( now );
		iter = icons.GetNext( iter );
	}
}

/*
============
sdUIIconNotification::AddIcon
============
*/
sdUIIconNotification::iconHandle_t sdUIIconNotification::AddIcon( const char* material ) {
	iconHandle_t handle = icons.Acquire();
	sdUINotifyIcon*& icon = icons[ handle ];
	icon = new sdUINotifyIcon( *this );

	GetUI()->LookupMaterial( material, icon->GetPart().mi, &icon->GetPart().width, &icon->GetPart().height );
	if( iconSize.GetValue().x > 0.0f ) {
		icon->GetPart().width = iconSize.GetValue().x;
	}
	if( iconSize.GetValue().y > 0.0f ) {
		icon->GetPart().height = iconSize.GetValue().y;
	}

	sdUINotifyIcon::colorEvaluator_t evaluator;
	int now = GetUI()->GetCurrentTime();
	idVec4 faded = icon->GetColor();
	faded.w = 0.0f;

	evaluator.SetParms( now, now + idMath::Ftoi( iconFadeTime ), faded, icon->GetColor() );

	icon->AnimateColor( evaluator, sdUINotifyIcon::CA_FADE );

	icon->GetNode().AddToEnd( drawNode );

	CalculateMaxDimensions();
	RunEvent( sdUIEventInfo( INE_ICON_ADDED, 0 ) );
	return handle;
}

/*
============
sdUIIconNotification::RemoveIcon
============
*/
void sdUIIconNotification::RemoveIcon( iconHandle_t& handle ) {
	if( !icons.IsValid( handle )) {
		gameLocal.Warning( "sdUIIconNotification::Script_RemoveIcon: invalid handle '%i'", (int)handle );
		return;
	}

	sdUINotifyIcon* icon = icons[ handle ];
	if( icon->IsDestructionScheduled() ) {
		return;
	}

	// start fading the icon out
	int now = GetUI()->GetCurrentTime();
	
	sdUINotifyIcon::colorEvaluator_t evaluator;

	idVec4 faded = icon->GetColor();
	faded.w = 0.0f;

	evaluator.SetParms( now, now + idMath::Ftoi( iconFadeTime ), icon->GetColor(), faded );
	icon->AnimateColor( evaluator, sdUINotifyIcon::CA_FADE );

	icon->ScheduleDestruction( now + idMath::Ftoi( iconFadeTime ) );

	// animate its neighbor to fill the icon's gap
	sdUINotifyIcon* next = icon->GetNode().Next();
	if( next != NULL ) {
		sdUINotifyIcon::originEvaluator_t evaluator;
		switch( static_cast< eOrientation >( idMath::Ftoi( orientation ) ) ) {			
			case IO_HORIZONTAL:				
				evaluator.SetParms( now, now + idMath::Ftoi( iconSlideTime ), vec2_zero, idVec2( -( icon->GetPart().width + iconSpacing ), 0.0f ) );
				break;
			case IO_HORIZONTAL_RIGHT:
				evaluator.SetParms( now, now + idMath::Ftoi( iconSlideTime ), vec2_zero, idVec2( ( icon->GetPart().width + iconSpacing ), 0.0f ) );
				break;
			case IO_VERTICAL:
				evaluator.SetParms( now, now + idMath::Ftoi( iconSlideTime ), vec2_zero, idVec2( 0.0f, -( icon->GetPart().height + iconSpacing ) ) );
		}

		next->AnimateLayoutOrigin( evaluator, sdUINotifyIcon::OA_SLIDING );
	}
	RunEvent( sdUIEventInfo( INE_ICON_REMOVED, 0 ) );
	handle.Release();
}

/*
============
sdUIIconNotification::Clear
============
*/
void sdUIIconNotification::Clear() {
	icons.DeleteContents();
}


/*
============
sdUIIconNotification::BumpIcon
============
*/
void sdUIIconNotification::BumpIcon( const iconHandle_t& handle, const char* table ) {
	if( !icons.IsValid( handle )) {
		gameLocal.Warning( "sdUIIconNotification::Script_BumpIcon: invalid handle '%i'", (int)handle );
		return;
	}

	int now = GetUI()->GetCurrentTime();

	sdUINotifyIcon* icon = icons[ handle ];

	sdUINotifyIcon::originEvaluator_t evaluator;
	evaluator.InitTableEvaluation( table );
	switch( static_cast< eOrientation >( idMath::Ftoi( orientation ) ) ) {			
		case IO_HORIZONTAL:			// FALL THROUGH		
		case IO_HORIZONTAL_RIGHT:
			evaluator.SetParms( now, now + iconSlideTime, vec2_zero, idVec2( 0.0f, iconSpacing ) );
			break;
		case IO_VERTICAL:
			evaluator.SetParms( now, now + iconSlideTime, vec2_zero, idVec2( iconSpacing, 0.0f ) );
	}

	icon->AnimateVisualOrigin( evaluator, sdUINotifyIcon::OA_SLIDING );	
}


/*
============
sdUIIconNotification::SetItemDataPtr
============
*/
void sdUIIconNotification::SetItemDataPtr( const iconHandle_t& handle, void* data ) {
	if( !icons.IsValid( handle )) {
		gameLocal.Warning( "sdUIIconNotification::SetItemDataPtr: invalid handle '%i'", (int)handle );
		return;
	}

	sdUINotifyIcon* icon = icons[ handle ];
	assert( icon != NULL );
	icon->SetDataPtr( data );
}


/*
============
sdUIIconNotification::GetItemDataPtr
============
*/
void* sdUIIconNotification::GetItemDataPtr( const iconHandle_t& handle ) const {
	if( !icons.IsValid( handle )) {
		gameLocal.Warning( "sdUIIconNotification::GetItemDataPtr: invalid handle '%i'", (int)handle );
		return NULL;
	}

	sdUINotifyIcon* icon = icons[ handle ];
	assert( icon != NULL );
	return icon->GetDataPtr();
}

/*
============
sdUIIconNotification::SetItemDataInt
============
*/
void sdUIIconNotification::SetItemDataInt( const iconHandle_t& handle, int data ) {
	if( !icons.IsValid( handle )) {
		gameLocal.Warning( "sdUIIconNotification::SetItemDataInt: invalid handle '%i'", (int)handle );
		return;
	}

	sdUINotifyIcon* icon = icons[ handle ];
	assert( icon != NULL );
	icon->SetDataInt( data );
}


/*
============
sdUIIconNotification::GetItemDataInt
============
*/
int sdUIIconNotification::GetItemDataInt( const iconHandle_t& handle ) const {
	if( !icons.IsValid( handle )) {
		gameLocal.Warning( "sdUIIconNotification::GetItemDataInt: invalid handle '%i'", (int)handle );
		return 0;
	}

	sdUINotifyIcon* icon = icons[ handle ];
	assert( icon != NULL );
	return icon->GetDataInt();
}

/*
============
sdUIIconNotification::GetFirstItem
============
*/
sdUIIconNotification::iconHandle_t sdUIIconNotification::GetFirstItem() const {
	return icons.GetFirst();
}


/*
============
sdUIIconNotification::GetNextItem
============
*/
sdUIIconNotification::iconHandle_t sdUIIconNotification::GetNextItem( const iconHandle_t& handle ) const {
	return icons.GetNext( handle );
}

/*
============
sdUIIconNotification::FindIcon
============
*/
sdUIIconNotification::iconHandle_t sdUIIconNotification::FindIcon( sdUINotifyIcon& icon ) {
	for( int i = 0; i < icons.Num(); i++ ) {
		if( icons[ i ] == &icon ) {
			return i;
		}
	}
	return iconHandle_t();
}

/*
============
sdUIIconNotification::Script_GetItemData
============
*/
void sdUIIconNotification::Script_GetItemData( sdUIFunctionStack& stack ) {
	int handleRaw;
	stack.Pop( handleRaw );

	iconHandle_t handle( handleRaw );
	stack.Push( GetItemDataInt( handle ) );
}

/*
============
sdUIIconNotification::Script_SetItemData
============
*/
void sdUIIconNotification::Script_SetItemData( sdUIFunctionStack& stack ) {
	int handleRaw;
	stack.Pop( handleRaw );

	int data;
	stack.Pop( data );

	iconHandle_t handle( handleRaw );
	SetItemDataInt( handle, data );
}

/*
============
sdUIIconNotification::Script_SetItemText
============
*/
void sdUIIconNotification::Script_SetItemText( sdUIFunctionStack& stack ) {
	int handleRaw;
	stack.Pop( handleRaw );

	idWStr text;
	stack.Pop( text );
	
	iconHandle_t handle( handleRaw );
	SetItemText( handle, text.c_str() );
}

/*
============
sdUIIconNotification::Script_GetItemText
============
*/
void sdUIIconNotification::Script_GetItemText( sdUIFunctionStack& stack ) {
	int handleRaw;
	stack.Pop( handleRaw );
	
	iconHandle_t handle( handleRaw );

	const wchar_t* text = GetItemText( handle );
	stack.Push( text );
}

/*
============
sdUIIconNotification::SetItemText
============
*/
void sdUIIconNotification::SetItemText( const iconHandle_t& handle, const wchar_t* text ) {
	if( !icons.IsValid( handle )) {
		gameLocal.Warning( "sdUIIconNotification::SetItemText: invalid handle '%i'", (int)handle );
		return;
	}

	sdUINotifyIcon* icon = icons[ handle ];
	icon->SetText( text );	
}

/*
============
sdUIIconNotification::GetItemText
============
*/
const wchar_t* sdUIIconNotification::GetItemText( const iconHandle_t& handle ) const {
	if( !icons.IsValid( handle )) {
		gameLocal.Warning( "sdUIIconNotification::GetItemText: invalid handle '%i'", (int)handle );
		return NULL;
	}

	sdUINotifyIcon* icon = icons[ handle ];
	return icon->GetText();
}

/*
============
sdUIIconNotification::Script_GetItemAtPoint
============
*/
void sdUIIconNotification::Script_GetItemAtPoint( sdUIFunctionStack& stack ) {
	idVec2 point;
	stack.Pop( point.x );
	stack.Pop( point.y );

	idVec2 origin = GetBaseOrigin();
	const int now = GetUI()->GetCurrentTime();
	sdBounds2D drawBounds;

	sdUINotifyIcon* icon = drawNode.Next();
	while( icon != NULL ) {		
		icon->GetDrawBounds( now, origin, drawBounds );	
		if( drawBounds.ContainsPoint( point ) ) {
			stack.Push( FindIcon( *icon ) );
			return;
		}
		GetNextOrigin( icon, origin );		
		icon = icon->GetNode().Next();
	}

	stack.Push( iconHandle_t() );
}

/*
============
sdUIIconNotification::CacheEvents
============
*/
void sdUIIconNotification::CacheEvents() {
	sdUIWindow::CacheEvents();
	iconPreDrawHandle = events.GetEvent( sdUIEventInfo( INE_ICON_PREDRAW, 0 ) );
}
