
#ifndef __DECLMATTYPE_H__
#define __DECLMATTYPE_H__

// Defines a material type - such as concrete, metal, glass etc
#ifdef RV_BINARYDECLS
class rvDeclMatType : public idDecl, public Serializable<'RDMT'>
{
public:
// jsinger: allow exporting of this decl type in a preparsed form
	virtual void		Write( SerialOutputStream &stream ) const;
	virtual void		AddReferences() const;
						rvDeclMatType( SerialInputStream &stream );
#else
class rvDeclMatType : public idDecl
{
public:
#endif
						rvDeclMatType( void ) { *( ulong *)mTint = 0; }
						~rvDeclMatType( void ) {}

	void				SetDescription( idStr &desc ) { mDescription = desc; }
	const idStr			&GetDescription( void ) const { return( mDescription ); }

	void				SetTint( byte tint[4] ) { *( ulong *)mTint = *( ulong *)tint; }
	int					GetTint( void ) const { return( *( int *)mTint ); }

	float				GetRed( void ) const { return( mTint[0] / 255.0f ); }
	float				GetGreen( void ) const { return( mTint[1] / 255.0f ); }
	float				GetBlue( void ) const { return( mTint[2] / 255.0f ); }

	virtual const char	*DefaultDefinition( void ) const;
	virtual bool		Parse( const char *text, const int textLength );
	virtual void		FreeData( void );
	virtual size_t		Size( void ) const;

// RAVEN BEGIN
// jscott: to prevent a recursive crash
	virtual	bool		RebuildTextSource( void ) { return( false ); }
// scork: for detailed error-reporting
	//virtual bool		Validate( const char *psText, int iTextLength, idStr &strReportTo ) const;
// RAVEN END


private:

	idStr				mDescription;
	byte				mTint[4];
};

byte *MT_GetMaterialTypeArray( idStr image, int &width, int &height );

#endif // __DECLMATTYPE_H__
