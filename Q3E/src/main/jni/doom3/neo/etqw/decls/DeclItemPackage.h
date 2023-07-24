// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLITEMPACKAGE_H__
#define __DECLITEMPACKAGE_H__

class sdDeclInvItem;
class sdDeclToolTip;

class sdConsumable {
public:
	virtual						~sdConsumable( void ) { ; }
	virtual bool				Give( idPlayer* player ) const = 0;
	virtual bool				Parse( idParser& src ) = 0;
};

class sdDeclItemPackageNode {
public:
													~sdDeclItemPackageNode( void ) { Clear(); }

	const sdRequirementContainer&					GetRequirements( void ) const { return requirements; }
	const idList< const sdDeclItemPackageNode* >&	GetNodes( void ) const { return nodes; }
	const sdDeclItemPackageItems&					GetItems( void ) const { return items; }
	const idList< const sdConsumable* >				GetConsumables( void ) const { return consumables; }

	void											AddRequirement( const char* text ) { requirements.Load( text ); }
	void											AddNode( const sdDeclItemPackageNode* node ) { nodes.Alloc() = node; }
	void											AddItem( const sdDeclInvItem* item ) { items.Alloc() = item; }
	void											AddConsumable( const sdConsumable* item ) { consumables.Alloc() = item; }

	void											Clear( void );

private:
	sdRequirementContainer							requirements;
	idList< const sdDeclItemPackageNode* >			nodes;
	sdDeclItemPackageItems							items;
	idList< const sdConsumable* >					consumables;
};

class sdDeclItemPackage : public idDecl {
public:
										sdDeclItemPackage( void );
	virtual								~sdDeclItemPackage( void );

	virtual const char*					DefaultDefinition( void ) const;
	virtual bool						Parse( const char *text, const int textLength );
	virtual void						FreeData( void );

	const sdDeclItemPackageNode&		GetItemRoot( void ) const { return rootNode; }

	static void							CacheFromDict( const idDict& dict );

	static void							InitConsumables( void );
	static void							ShutdownConsumables( void );

private:
	bool								ParseNode( idParser& src, sdDeclItemPackageNode& node );

	sdDeclItemPackageNode				rootNode;

	typedef sdFactory< sdConsumable >	consumableFactory_t;

	static consumableFactory_t			s_consumableFactory;
};

#endif // __DECLITEMPACKAGE_H__
