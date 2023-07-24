// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclInvItem.h"
#include "../Game_local.h"
#include "../../renderer/ModelManager.h"
#include "../../decllib/declTypeHolder.h"
#include "../../framework/DeclParseHelper.h"
/*
===============================================================================

	sdDeclInvItem

===============================================================================
*/

/*
================
sdDeclInvItem::sdDeclInvItem
================
*/
sdDeclInvItem::sdDeclInvItem( void ) {
	FreeData();
}

/*
================
sdDeclInvItem::~sdDeclInvItem
================
*/
sdDeclInvItem::~sdDeclInvItem( void ) {
	FreeData();
}

/*
================
sdDeclInvItem::DefaultDefinition
================
*/
const char* sdDeclInvItem::DefaultDefinition( void ) const {
	return					\
		"{\n"				\
		"\ttype \"item\"\n"	\
		"}\n";
}

/*
================
sdDeclInvItem::ParseClip
================
*/
bool sdDeclInvItem::ParseClip( idParser& src ) {

	idDict temp;
	if ( !temp.Parse( src ) ) {
		src.Error( "sdDeclInvItem::ParseClip Error Parsing Generic Clip Data" );
	}

	itemClip_t& clip	= clips.Alloc();
	clip.ammoPerShot		= 0;
	clip.ammoType			= ( ammoType_t )( -1 );
	clip.clientProjectile	= "";
	clip.lowAmmo			= 0;
	clip.maxAmmo			= 0;
	clip.projectile			= NULL;

	const char* typeText = temp.GetString( "type" );
	const sdDeclAmmoType* type = gameLocal.declAmmoTypeType[ typeText ];
	if ( !type ) {
		src.Error( "sdDeclInvItem::ParseAmmoTypes Invalid Ammo Type '%s'", typeText );
		return false;
	}

	clip.ammoPerShot		= temp.GetInt( "ammo_per_shot" );
	clip.ammoType			= type->GetAmmoType();
	clip.clientProjectile	= temp.GetString( "client_projectile" );
	clip.lowAmmo			= temp.GetInt( "low_ammo" );
	clip.maxAmmo			= temp.GetInt( "max_ammo" );
	clip.projectile			= gameLocal.declEntityDefType[ temp.GetString( "projectile" ) ];

	return true;
}

/*
================
sdDeclInvItem::Parse
================
*/
bool sdDeclInvItem::Parse( const char *text, const int textLength ) {
	slot = -1;

	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	while( true ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}

		if( !token.Icmp( "slot" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			const sdDeclInvSlot* slot = gameLocal.declInvSlotType.LocalFind( token.c_str(), false );
			if( !slot ) {
				src.Error( "sdDeclInvItem::Parse Invalid Slot '%s'", token.c_str() );
				return false;
			}

			this->slot = slot->Index();

		} else if( !token.Icmp( "type" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			type = gameLocal.declInvItemTypeType.LocalFind( token, false );
			if( !type ) {
				src.Error( "sdDeclInvItem::Parse Invalid Item Type '%s'", token.c_str() );
				return false;
			}

		} else if( !token.Icmp( "model" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			model = renderModelManager->FindModel( token );
			if( !model ) {
				src.Warning( "sdDeclInvItem::Parse Could Not Find Model '%s'", token.c_str() );
			}

		} else if( !token.Icmp( "name" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Warning( "sdDeclInvItem::Parse Expected String, Found '%s' While Parsing Name", token.c_str() );
				return false;
			}

			name = declHolder.FindLocStr( token.c_str() );

		} else if( !token.Icmp( "joint" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Warning( "sdDeclInvItem::Parse Expected String, Found '%s' While Parsing Joint", token.c_str() );
				return false;
			}

			joint = token;

		} else if( !token.Icmp( "tooltip_select" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Warning( "sdDeclInvItem::Parse Expected String, Found '%s' While Parsing Tooltip_Select", token.c_str() );
				return false;
			}
			
			tooltipSelect = gameLocal.declToolTipType.LocalFind( token, false );

		} else if( !token.Icmp( "clip" ) ) {

			if ( !ParseClip( src ) ) {
				src.Error( "sdDeclInvItem::Parse Error Parsing Ammo Types" );
				return false;
			}

		} else if ( !token.Icmp( "data" ) ) {
			idDict temp;

			if ( !temp.Parse( src ) ) {
				src.Error( "sdDeclInvItem::Parse Error Generic Item Data" );
				return false;
			}

			// "inherit" keys will cause all values from another entityDef to be copied into this one
			// if they don't conflict.  We can't have circular recursions, because each entityDef will
			// never be parsed mroe than once

			// find all of the dicts first, because copying inherited values will modify the dict
			idList<const sdDeclInvItem *> defList;

			while ( true ) {
				const idKeyValue *kv;
				kv = temp.MatchPrefix( "inherit", NULL );
				if ( !kv ) {
					break;
				}

				const sdDeclInvItem* copy = gameLocal.declInvItemType[ kv->GetValue().c_str() ];
				if ( !copy ) {
					src.Warning( "Unknown invItemDef '%s' inherited by '%s'", kv->GetValue().c_str(), GetName() );
				} else {
					defList.Append( copy );
					declManager->AddDependency( this, copy );
				}

				// delete this key/value pair
				temp.Delete( kv->GetKey() );
			}

			// now copy over the inherited key / value pairs
			for ( int i = 0 ; i < defList.Num() ; i++ ) {
				temp.SetDefaults( &defList[ i ]->GetData() );
			}

			// precache all referenced media
			// do this as long as we aren't in modview
			game->CacheDictionaryMedia( temp );

			data.Copy( temp );

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclInvItem::Parse Invalid Token '%s'", token.c_str() );
			return false;

		}
	}

	usageRequirements.Load( data.GetString( "require_use" ) );
	autoSwitchPriority	= data.GetInt( "autoswitch_priority" );
	autoSwitchExplosive	= data.GetBool( "autoswitch_isexplosive" );
	weaponMenuIgnore	= data.GetBool( "weapon_menu_ignore" );
	selectWhenEmpty		= data.GetBool( "select_when_empty" );
	weaponChangeAllowed = data.GetBool( "weapon_change_allowed", "1" );
	ignoreViewPitch		= data.GetBool( "ignore_view_pitch" );
	allowProne			= data.GetBool( "allow_prone", "1" );

	if( !type ) {
		src.Error( "sdDeclInvItem::Parse No Type Supplied" );
		return false;
	}

	return true;
}

/*
================
sdDeclInvItem::FreeData
================
*/
void sdDeclInvItem::FreeData( void ) {
	type				= NULL;

	data.Clear();
	clips.Clear();

	name				= NULL;
	tooltipSelect		= NULL;
	joint				= "";
	model				= NULL;

	usageRequirements.Clear();
}
