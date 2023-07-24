// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __DECLGUITHEME_H__
#define __DECLGUITHEME_H__

class sdDeclGUITheme : public idDecl {
public:
										sdDeclGUITheme( void );
	virtual								~sdDeclGUITheme( void );

	virtual const char*					DefaultDefinition( void ) const;
	virtual bool						Parse( const char *text, const int textLength );
	virtual void						FreeData( void );

	static void							CacheFromDict( const idDict& dict );

	virtual idDict&						GetMaterials() { return globalMaterialTable; }
	virtual const idDict&				GetMaterials() const { return globalMaterialTable; }
	virtual const char*					GetMaterial( const char* material ) const;

	virtual idDict&						GetSounds() { return globalSoundTable; }
	virtual const idDict&				GetSounds() const { return globalSoundTable; }
	virtual const char*					GetSound( const char* sound ) const;

	virtual idDict&						GetColors() { return globalColorTable; }
	virtual const idDict&				GetColors() const { return globalColorTable; }
	virtual const idVec4				GetColor( const char* color ) const;

	virtual idDict&						GetFonts() { return globalFontsTable; }
	virtual const idDict&				GetFonts() const { return globalFontsTable; }
	virtual const char*					GetFont( const char* font ) const;

	static void							OnReloadGUITheme( idDecl* decl );

private:
	idDict								globalMaterialTable;
	idDict								globalSoundTable;
	idDict								globalColorTable;
	idDict								globalFontsTable;

	idList< qhandle_t >					fontHandles;
};

#endif // __DECLGUITHEME_H__

