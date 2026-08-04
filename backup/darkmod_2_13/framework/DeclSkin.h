/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

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
	virtual size_t			Size( void ) const override;
	virtual bool			SetDefaultText( void ) override;
	virtual const char *	DefaultDefinition( void ) const override;
	virtual bool			Parse( const char *text, const int textLength ) override;
	virtual void			FreeData( void ) override;

	const idMaterial *		RemapShaderBySkin( const idMaterial *shader ) const;

							// model associations are just for the preview dialog in the editor
	const int				GetNumModelAssociations() const;
	const char *			GetAssociatedModel( const int index ) const;

private:
	idList<skinMapping_t>	mappings;
	// The list of models associated with this skin is only to guide the
	// user selection in the editor. The skin will be applied for any model
	// the entity has, regardless on whether it is in this list, or not.
	idStrList				associatedModels;
};

#endif /* !__DECLSKIN_H__ */
