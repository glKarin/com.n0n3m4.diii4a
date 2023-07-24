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
#include "UICreditScroll.h"

#include "../../sys/sys_local.h"

SD_UI_IMPLEMENT_CLASS( sdUICreditScroll, sdUIWindow )

idHashMap< sdUITemplateFunction< sdUICreditScroll >* >	sdUICreditScroll::creditFunctions;

/*
============
sdUICreditScroll::sdUICreditScroll
============
*/
sdUICreditScroll::sdUICreditScroll( void ) :
	items( NULL ),
	totalHeight( 0.0f ),
	scrollStartTime( 0 ),
	scrollTargetTime( 0 ) {
	scriptState.GetProperties().RegisterProperty( "speed",			speed );

	speed				= 5.0f;
	scrollOffset		= 0.0f;

	SetWindowFlag( WF_CLIP_TO_RECT );
}


/*
============
sdUICreditScroll::~sdUICreditScroll
============
*/
sdUICreditScroll::~sdUICreditScroll( void ) {
	DisconnectGlobalCallbacks();
	ClearItems();
}


/*
============
sdUICreditScroll::ClearItems
============
*/
void sdUICreditScroll::ClearItems() {
	while( items != NULL ) {
		sdCreditItem* next = items->next;
		delete items;
		items = next;
	}
}

/*
============
sdUICreditScroll::GetFunction
============
*/
sdUIFunctionInstance* sdUICreditScroll::GetFunction( const char* name ) {
	const sdUITemplateFunction< sdUICreditScroll >* function = sdUICreditScroll::FindFunction( name );
	if( !function ) {		
		return sdUIWindow::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUICreditScroll, sdUITemplateFunctionInstance_IdentifierTimeline >( this, function );
}

/*
============
sdUICreditScroll::FindFunction
============
*/
const sdUITemplateFunction< sdUICreditScroll >* sdUICreditScroll::FindFunction( const char* name ) {
	sdUITemplateFunction< sdUICreditScroll >** ptr;
	return creditFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUICreditScroll::InitFunctions
============
*/
SD_UI_PUSH_CLASS_TAG( sdUICreditScroll )
void sdUICreditScroll::InitFunctions() {
	SD_UI_FUNC_TAG( resetScroll, "Reset scroll amount." )
	SD_UI_END_FUNC_TAG
	creditFunctions.Set( "resetScroll",	new sdUITemplateFunction< sdUICreditScroll >( 'v', "",	&sdUICreditScroll::Script_ResetScroll ) );	

	SD_UI_FUNC_TAG( loadFromFile, "Load the text to scroll from a file. Supports UTF8." )
		SD_UI_FUNC_PARM( string, "filePath", "File to load data from." )
	SD_UI_END_FUNC_TAG
	creditFunctions.Set( "loadFromFile",new sdUITemplateFunction< sdUICreditScroll >( 'v', "s",	&sdUICreditScroll::Script_LoadFromFile ) );	
}
SD_UI_POP_CLASS_TAG

/*
============
sdUICreditScroll::Script_ResetScroll
============
*/
void sdUICreditScroll::Script_ResetScroll( sdUIFunctionStack& stack ) {
	scrollOffset = 0.0f;
}

/*
============
sdUICreditScroll::DrawLocal
============
*/
void sdUICreditScroll::DrawLocal() {
	const sdCreditItem* item = items;
	
	sdBounds2D bounds( cachedClientRect );
	while( item != NULL ) {
		sdBounds2D b( item->rect );
		b.TranslateSelf( 0.0f, scrollOffset );

		if( b.IntersectsBounds( bounds ) ) {
			if( item->material.material != NULL ) {
				deviceContext->DrawMaterial( item->rect.x, item->rect.y + scrollOffset, item->rect.z, item->rect.w, item->material.material, item->color, item->material.st0, item->material.st1 );
			} else {							
				deviceContext->SetColor( item->color );
				
				deviceContext->SetFontSize( fontSize + item->fontSize );
				deviceContext->DrawText( item->text.c_str(), b, DTF_TOP | DTF_SINGLELINE |  DTF_CENTER );
			}
		}
		item = item->next;
	}
}

/*
============
sdUICreditScroll::Script_LoadFromFile
============
*/
void sdUICreditScroll::Script_LoadFromFile( sdUIFunctionStack& stack ) {
	ClearItems();

	idStr fileName;
	stack.Pop( fileName );
	
	sdFilePtr file( fileSystem->OpenFileRead( fileName.c_str() ) );
	if( !file.IsValid() ) {
		gameLocal.Warning( "%s: Script_LoadFromFile: could not find '%s'", name.GetValue().c_str(), fileName.c_str() );
		return;
	}

	sdUTF8 utf8( file.Get() );
	file.Release();

	int numChars = utf8.DecodeLength();
	wchar_t* ucs2 = static_cast< wchar_t* >( Mem_Alloc( ( numChars + 1 ) * sizeof( wchar_t ) ) );

	utf8.Decode( ucs2 );

	idWLexer src( ucs2, idWStr::Length( ucs2 ), fileName );
	if ( !src.IsLoaded() ) {
		Mem_Free( ucs2 );
		return;
	}

	declManager->AddDependency( GetUI()->GetDecl(), fileName.c_str() );

	lastItem = NULL;

	idWToken token;
	while( src.ReadToken( &token ) ) {

		int i = token.linesCrossed; 
		while( i > 1 ) {
			sdCreditItem* item = new sdCreditItem;
			item->rect.w = 8.0f;
			Append( item );
			i--;
		}
		sdCreditItem* item = new sdCreditItem;

		bool needText = false;
		bool useText = true;
		if( token.Cmp( L"material" ) == 0 ) {
			idWToken w;
			idWToken h;
			src.ReadToken( &w );
			src.ReadToken( &h );

			src.ReadToken( &token );

			GetUI()->LookupMaterial( va( "%ls", token.c_str() ), item->material );

			assert( w.type == TT_NUMBER );
			assert( h.type == TT_NUMBER );
			item->rect.z = w.GetFloatValue();
			item->rect.w = h.GetFloatValue();			
			useText = false;
			item->color = colorWhite;
		} else if( token.Icmp( L"small" ) == 0 )  {
			item->fontSize = -2;
			needText = true;
			item->color = colorWhite;
		} else if( token.Icmp( L"big" ) == 0 )  {
			item->fontSize = 2;
			item->color = colorWhite;
			needText = true;
		} else if( token.Icmp( L"huge" ) == 0 )  {
			item->fontSize = 4;
			item->color = colorWhite;
			needText = true;
		}

		if( needText ) {
			src.ReadToken( &token );		
		}

		if( useText ) {
			assert( token.type == TT_STRING );
			item->text = token.c_str();
		}
		
		Append( item );
	}
	MakeLayoutDirty();
}


/*
============
sdUICreditScroll::Append
============
*/
void sdUICreditScroll::Append( sdCreditItem* item ) {
	if( items == NULL ) {
		items = item;			
	} else if( lastItem != NULL ) {
		lastItem->next = item;
	}
	lastItem = item;
}

/*
============
sdUICreditScroll::ApplyLayout
============
*/
void sdUICreditScroll::ApplyLayout() {
	bool apply = windowState.recalculateLayout;
	sdUIWindow::ApplyLayout();

	if( apply ) {
		float yOffset = 0.0f;
		sdBounds2D measureRect( 0.0f, 0.0f,  cachedClientRect.z * 0.33f, SCREEN_HEIGHT );
		int w;
		int h;

		ActivateFont( true );

		sdCreditItem* item = items;
		while( item != NULL ) {
			if( item->material.material != NULL ) {
				item->rect.x = cachedClientRect.x + ( cachedClientRect.z * 0.5f ) - ( item->rect.z * 0.5f );
				item->rect.y = cachedClientRect.y + yOffset;				
			} else if( !item->text.IsEmpty() ) {
				deviceContext->SetFontSize( fontSize + item->fontSize );
				deviceContext->GetTextDimensions( item->text.c_str(), measureRect, DTF_SINGLELINE | DTF_CENTER | DTF_TOP, cachedFontHandle, fontSize + item->fontSize, w, h );
				item->rect.x = cachedClientRect.x + ( cachedClientRect.z * 0.5f ) - ( w * 0.5f );
				item->rect.y = cachedClientRect.y +  yOffset;
				item->rect.z = w;
				item->rect.w = h;
			}
			yOffset += item->rect.w;
			item = item->next;			
		}
		totalHeight = yOffset;
		scrollOffset = cachedClientRect.w;
		scrollStartTime = 0;
		scrollTargetTime = 0;
	}

	const int now = GetUI()->GetCurrentTime();

	float ratio = totalHeight / cachedClientRect.w;
	if ( ratio < 1.0f ) {
		ratio = 1.0f;
	}

	if ( now >= scrollTargetTime || scrollStartTime == scrollTargetTime || now < scrollStartTime ) {
		scrollStartTime = now;
	}
	scrollTargetTime = scrollStartTime + SEC2MS( speed * ratio );

	float totalTime = static_cast< float >( scrollTargetTime - scrollStartTime );
	float percent = static_cast< float >( now - scrollStartTime ) / totalTime;	

	scrollOffset = cachedClientRect.w - percent * ( cachedClientRect.w + totalHeight );	

	if( idMath::Fabs( scrollOffset ) >= totalHeight ) {
		scrollOffset = cachedClientRect.w;
	}
}
