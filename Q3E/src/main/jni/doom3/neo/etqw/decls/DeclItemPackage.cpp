// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclItemPackage.h"
#include "../Game_local.h"
#include "../Player.h"

#include "../../framework/DeclParseHelper.h"

sdFactory< sdConsumable >	sdDeclItemPackage::s_consumableFactory;

/*
===============================================================================

	sdConsumable_Ammo

===============================================================================
*/

class sdConsumable_Ammo : public sdConsumable {
public:
								sdConsumable_Ammo( void );
	virtual						~sdConsumable_Ammo( void );
	virtual bool				Give( idPlayer* player ) const;
	virtual bool				Parse( idParser& src );

private:
	ammoType_t					type;
	int							amount;
};

/*
================
sdConsumable_Ammo::sdConsumable_Ammo
================
*/
sdConsumable_Ammo::sdConsumable_Ammo( void ) {
	type	= -1;
	amount	= 0;
}

/*
================
sdConsumable_Ammo::~sdConsumable_Ammo
================
*/
sdConsumable_Ammo::~sdConsumable_Ammo( void ) {
}

/*
================
sdConsumable_Ammo::Give
================
*/
bool sdConsumable_Ammo::Give( idPlayer* player ) const {
	sdInventory& inv = player->GetInventory();

	int maxAmmo		= inv.GetMaxAmmo( type );
	int currentAmmo	= inv.GetAmmo( type );

	if ( currentAmmo >= maxAmmo ) {
		return false;
	}

	currentAmmo = Min( currentAmmo + amount, maxAmmo );

	inv.SetAmmo( type, currentAmmo );

	return true;
}

/*
================
sdConsumable_Ammo::Parse
================
*/
bool sdConsumable_Ammo::Parse( idParser& src ) {
	idToken typeName;
	if ( !src.ReadToken( &typeName ) ) {
		return false;
	}

	const sdDeclAmmoType* ammoDecl = gameLocal.declAmmoTypeType[ typeName ];
	if( !ammoDecl ) {
		src.Error( "sdConsumable_Ammo::Parse Invalid Ammo Type %s", typeName.c_str() );
		return false;
	}

	type = ammoDecl->GetAmmoType();

	idToken value;
	if ( !src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &value ) ) {
		return false;
	}

	amount = value.GetIntValue();
	if ( amount <= 0 ) {
		return false;
	}

	return true;
}

/*
===============================================================================

	sdConsumable_Health

===============================================================================
*/

class sdConsumable_Health : public sdConsumable {
public:
								sdConsumable_Health( void );
	virtual						~sdConsumable_Health( void );
	virtual bool				Give( idPlayer* player ) const;
	virtual bool				Parse( idParser& src );

private:
	int							amount;
};

/*
================
sdConsumable_Health::sdConsumable_Health
================
*/
sdConsumable_Health::sdConsumable_Health( void ) {
	amount = 0;
}

/*
================
sdConsumable_Health::~sdConsumable_Health
================
*/
sdConsumable_Health::~sdConsumable_Health( void ) {
}

/*
================
sdConsumable_Health::Give
================
*/
bool sdConsumable_Health::Give( idPlayer* player ) const {
	return player->Heal( amount ) > 0;
}

/*
================
sdConsumable_Health::Parse
================
*/
bool sdConsumable_Health::Parse( idParser& src ) {
	idToken value;
	if ( !src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &value ) ) {
		return false;
	}

	amount = value.GetIntValue();
	if ( amount <= 0 ) {
		return false;
	}

	return true;
}

/*
===============================================================================

	sdDeclItemPackage

===============================================================================
*/

/*
================
sdDeclItemPackage::sdDeclItemPackage
================
*/
sdDeclItemPackage::sdDeclItemPackage( void ) {
}

/*
================
sdDeclItemPackage::~sdDeclItemPackage
================
*/
sdDeclItemPackage::~sdDeclItemPackage( void ) {
	FreeData();
}

/*
================
sdDeclItemPackage::DefaultDefinition
================
*/
const char* sdDeclItemPackage::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclItemPackage::InitConsumables
================
*/
void sdDeclItemPackage::InitConsumables( void ) {
	s_consumableFactory.RegisterType( "ammo",	consumableFactory_t::Allocator< sdConsumable_Ammo > );
	s_consumableFactory.RegisterType( "health", consumableFactory_t::Allocator< sdConsumable_Health > );
}

/*
================
sdDeclItemPackage::ShutdownConsumables
================
*/
void sdDeclItemPackage::ShutdownConsumables( void ) {
	s_consumableFactory.Shutdown();
}

/*
================
sdDeclItemPackage::ParseNode
================
*/
bool sdDeclItemPackage::ParseNode( idParser& src, sdDeclItemPackageNode& node ) {
	idToken token;
	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Cmp( "{" ) ) {
			sdDeclItemPackageNode* newNode = new sdDeclItemPackageNode();
			if ( !ParseNode( src, *newNode ) ) {
				delete newNode;
				return false;
			}

			node.AddNode( newNode );

		} else if ( !token.Icmp( "item" ) ) {

			if ( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			const sdDeclInvItem* item = gameLocal.declInvItemType[ token ];
			if ( !item ) {
				src.Error( "sdDeclItemPackage::Parse Invalid Item '%s'", token.c_str() );
				return false;
			}

			node.AddItem( item );

		} else if ( !token.Icmp( "require" ) ) {

			if ( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			node.AddRequirement( token.c_str() );

		} else if ( !token.Icmp( "consumable" ) ) {

			idToken type;
			if ( !src.ReadToken( &type ) ) {
				return false;
			}

			sdConsumable* consumable = s_consumableFactory.CreateType( type );
			if ( !consumable ) {
				src.Error( "sdDeclItemPackage::Parse Invalid consumable type %s", type.c_str() );
				return false;
			}

			if ( !consumable->Parse( src ) ) {
				delete consumable;
				src.Error( "sdDeclItemPackage::Parse Failed to Parse Consumable" );
				return false;
			}

			node.AddConsumable( consumable );

		} else if ( !token.Cmp( "}" ) ) {

			break;
		
		}
	}

	return true;
}


/*
================
sdDeclItemPackage::Parse
================
*/
bool sdDeclItemPackage::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	ParseNode( src, rootNode );

	return true;
}

/*
================
sdDeclItemPackage::FreeData
================
*/
void sdDeclItemPackage::FreeData( void ) {
	rootNode.Clear();
}

/*
================
sdDeclItemPackage::CacheFromDict
================
*/
void sdDeclItemPackage::CacheFromDict( const idDict& dict ) {
	const idKeyValue* kv = NULL;

	while( kv = dict.MatchPrefix( "pck_", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declItemPackageType[ kv->GetValue() ];
		}
	}
}







/*
================
sdDeclItemPackageNode::Clear
================
*/
void sdDeclItemPackageNode::Clear( void ) {
	requirements.Clear();
	nodes.DeleteContents( true );
	items.Clear();
	consumables.DeleteContents( true );
}
