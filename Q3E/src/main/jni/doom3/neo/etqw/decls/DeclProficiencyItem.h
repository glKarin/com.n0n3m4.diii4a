// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLPROFICIENCYITEM_H__
#define __DECLPROFICIENCYITEM_H__

class sdDeclProficiencyType;

class sdDeclProficiencyItem : public idDecl {
public:
								sdDeclProficiencyItem( void );
	virtual						~sdDeclProficiencyItem( void );

	virtual const char*			DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength );
	virtual void				FreeData( void );

	const sdDeclProficiencyType*	GetProficiencyType( void ) const { return type; }
	float							GetProficiencyCount( void ) const { return count; }
	sdPlayerStatEntry*				GetStat( void ) const { return stat; }

protected:
	const sdDeclProficiencyType*	type;
	float							count;
	sdPlayerStatEntry*				stat;
};

#endif // __DECLPROFICIENCYITEM_H__

