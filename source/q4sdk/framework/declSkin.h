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

// RAVEN BEGIN
// jsinger: allow support for serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idDeclSkin : public idDecl, public Serializable<'DSKN'> {
public:
// jsinger: allow exporting of this decl type in a preparsed form
	virtual void			Write( SerialOutputStream &stream) const;
	virtual void			AddReferences() const;
							idDeclSkin(SerialInputStream &stream);
#else
class idDeclSkin : public idDecl {
#endif
public:
							idDeclSkin();
	virtual size_t			Size( void ) const;
	virtual bool			SetDefaultText( void );
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength, bool noCaching );
	virtual void			FreeData( void );
// RAVEN BEGIN
// mwhitlock: Xenon texture streaming
#if defined(_XENON)
	void					StreamAllSkinTargets(bool inBackground);
	void					GetSkinTargetsList(idList<const idMaterial*>& outList) const;
#endif
// RAVEN END
	const idMaterial *		RemapShaderBySkin( const idMaterial *shader ) const;

							// model associations are just for the preview dialog in the editor
// RAVEN BEGIN
// jscott: inlined for access from tools dll
	const int				GetNumModelAssociations() const { return( associatedModels.Num() ); }
// jscott: to prevent a recursive crash
	virtual	bool			RebuildTextSource( void ) { return( false ); }
// scork: validation member for more detailed error-checks
	virtual bool			Validate( const char *psText, int iTextLength, idStr &strReportTo ) const;
// RAVEN END
	const char *			GetAssociatedModel( int index ) const;

private:
	idList<skinMapping_t>	mappings;
	idStrList				associatedModels;
};

// RAVEN BEGIN
// jscott: inlined for access from tools dll
ID_INLINE const char *idDeclSkin::GetAssociatedModel( int index ) const {
	if ( index >= 0 && index < associatedModels.Num() ) {
		return associatedModels[ index ];
	}
	return "";
}
// RAVEN END

#endif /* !__DECLSKIN_H__ */
