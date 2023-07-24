// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLCAMPAIGN_H__
#define __DECLCAMPAIGN_H__

class sdDeclCampaign : public idDecl {
public:
							sdDeclCampaign( void );
	virtual					~sdDeclCampaign( void );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	int						GetNumMaps( void ) const { return maps.Num(); }
	const char*				GetMap( int index ) const { return maps[ index ]; }
	const idMaterial*		GetBackdrop( void ) const;

private:
	idStrList				maps;
	idStr					backdrop;
};

#endif // __DECLCAMPAIGN_H__
