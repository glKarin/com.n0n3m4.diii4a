// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __DECLPROFICIENCYTYPE_H__
#define __DECLPROFICIENCYTYPE_H__

class sdDeclItemPackage;
class sdPlayerStatEntry;

class sdDeclProficiencyType : public idDecl {
public:
	struct stats_t {
		sdPlayerStatEntry*		xp;
		sdPlayerStatEntry*		totalXP;
		idStr					name;
	};

								sdDeclProficiencyType( void );
	virtual						~sdDeclProficiencyType( void );

	virtual const char*			DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength );
	virtual void				FreeData( void );

	int							GetNumLevels( void ) const { return levels.Num(); }
	int							GetLevel( int index ) const { return levels[ index ]; }
	const char*					GetLookupTitle( void ) const { return title; }
	const stats_t&				GetStats( void ) const { return stats; }
	const sdDeclLocStr*			GetProficiencyText( void ) const { return text; }

protected:
	stats_t stats;

	const sdDeclLocStr*			text;
	idStr						title;
	idList< int >				levels;
};

#endif // __DECLPROFICIENCYTYPE_H__

