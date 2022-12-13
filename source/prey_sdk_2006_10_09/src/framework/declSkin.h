// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __DECLSKIN_H__
#define __DECLSKIN_H__

/*
===============================================================================

	idDeclSkin

===============================================================================
*/

typedef struct {
	const idMaterial *		from;			// 0 == any unmatched shader
	const idMaterial *		to;
} skinMapping_t;

class idDeclSkin : public idDecl {
public:
	virtual size_t			Size( void ) const;
	virtual bool			SetDefaultText( void );
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	virtual //HUMANHEAD: aob - needed so this is added to vtable and callable from DLL
	const idMaterial *		RemapShaderBySkin( const idMaterial *shader ) const;

							// model associations are just for the preview dialog in the editor
	const int				GetNumModelAssociations() const;
	const char *			GetAssociatedModel( int index ) const;

private:
	idList<skinMapping_t>	mappings;
	idStrList				associatedModels;
};

#endif /* !__DECLSKIN_H__ */
