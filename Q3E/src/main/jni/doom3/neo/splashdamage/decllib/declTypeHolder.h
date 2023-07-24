// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLTYPEHOLDER_H__
#define __DECLTYPEHOLDER_H__

//===============================================================
//
//	sdDeclTypeHolder
//
//===============================================================

#include "../framework/declManager.h"

class sdDeclTypeHolder {
public:
															sdDeclTypeHolder();

	sdDeclWrapperTemplate< class idDeclTable >				declTableType;
	sdDeclWrapperTemplate< class idMaterial >				declMaterialType;
	sdDeclWrapperTemplate< class idDeclSkin >				declSkinType;
	sdDeclWrapperTemplate< class idSoundShader >			declSoundShaderType;
	sdDeclWrapperTemplate< class idDeclEntityDef >			declEntityDefType;
	sdDeclWrapperTemplate< class idDeclAF >					declAFType;
	sdDeclWrapperTemplate< class rvDeclEffect >				declEffectsType;
	sdDeclWrapperTemplate< class sdDeclAtmosphere >			declAtmosphereType;
	sdDeclWrapperTemplate< class sdDeclAmbientCubeMap >		declAmbientCubeMapType;
	sdDeclWrapperTemplate< class sdDeclStuffType >			declStuffTypeType;
	sdDeclWrapperTemplate< class sdDeclDecal >				declDecalType;
	sdDeclWrapperTemplate< class sdDeclSurfaceType >		declSurfaceTypeType;
	sdDeclWrapperTemplate< class sdDeclSurfaceTypeMap >		declSurfaceTypeMapType;
	sdDeclWrapperTemplate< class sdDeclRenderProgram >		declRenderProgramType;
	sdDeclWrapperTemplate< class sdDeclRenderBinding >		declRenderBindingType;
	sdDeclWrapperTemplate< class sdDeclTemplate >			declTemplateType;
	sdDeclWrapperTemplate< class sdDeclImposter >			declImposterType;
	sdDeclWrapperTemplate< class sdDeclImposterGenerator >	declImposterGeneratorType;
	sdDeclWrapperTemplate< class sdDeclLocStr >				declLocStrType;
	sdDeclWrapperTemplate< class sdDeclModelExport >		declModelExportType;

	const class idDeclTable*								FindTable( const char* name, bool makeDefault = true ) const { return declTableType.LocalFind( name, makeDefault ); }
	const class idMaterial*									FindMaterial( const char* name, bool makeDefault = true ) const { return declMaterialType.LocalFind( name, makeDefault ); }
	const class idDeclSkin*									FindSkin( const char* name, bool makeDefault = true ) const { return declSkinType.LocalFind( name, makeDefault ); }
	const class idSoundShader*								FindSoundShader( const char* name, bool makeDefault = true ) const { return declSoundShaderType.LocalFind( name, makeDefault ); }
	const class idDeclEntityDef*							FindEntityDef( const char* name, bool makeDefault = true ) const { return declEntityDefType.LocalFind( name, makeDefault ); }
	const class idDeclAF*									FindAF( const char* name, bool makeDefault = true ) const { return declAFType.LocalFind( name, makeDefault ); }
	const class rvDeclEffect*								FindEffect( const char* name, bool makeDefault = true ) const { return declEffectsType.LocalFind( name, makeDefault ); }
	const class sdDeclAtmosphere*							FindAtmosphere( const char* name, bool makeDefault = true ) const { return declAtmosphereType.LocalFind( name, makeDefault ); }
	const class sdDeclAmbientCubeMap*						FindAmbientCubeMap( const char* name, bool makeDefault = true ) const { return declAmbientCubeMapType.LocalFind( name, makeDefault ); }
	const class sdDeclStuffType*							FindStuffType( const char* name, bool makeDefault = true ) const { return declStuffTypeType.LocalFind( name, makeDefault ); }
	const class sdDeclDecal*								FindDecal( const char* name, bool makeDefault = true ) const { return declDecalType.LocalFind( name, makeDefault ); }
	const class sdDeclSurfaceType*							FindSurfaceType( const char* name, bool makeDefault = true ) const { return declSurfaceTypeType.LocalFind( name, makeDefault ); }
	const class sdDeclSurfaceTypeMap*						FindSurfaceTypeMap( const char* name, bool makeDefault = true ) const { return declSurfaceTypeMapType.LocalFind( name, makeDefault ); }
	const class sdDeclRenderProgram*						FindRenderProgram( const char* name, bool makeDefault = true ) const { return declRenderProgramType.LocalFind( name, makeDefault ); }
	const class sdDeclRenderBinding*						FindRenderBinding( const char* name, bool makeDefault = true ) const { return declRenderBindingType.LocalFind( name, makeDefault ); }
	const class sdDeclTemplate*								FindTemplate( const char* name, bool makeDefault = true ) const { return declTemplateType.LocalFind( name, makeDefault ); }
	const class sdDeclImposter*								FindImposter( const char* name, bool makeDefault = true ) const { return declImposterType.LocalFind( name, makeDefault ); }
	const class sdDeclLocStr*								FindLocStr( const char* name, bool makeDefault = true ) const { return declLocStrType.LocalFind( name, makeDefault ); }

	const class idDeclTable*								FindTableByIndex( int index, bool forceParse = true ) const { return declTableType.LocalFindByIndex( index, forceParse ); }
	const class idMaterial*									FindMaterialByIndex( int index, bool forceParse = true ) const { return declMaterialType.LocalFindByIndex( index, forceParse ); }
	const class idDeclSkin*									FindSkinByIndex( int index, bool forceParse = true ) const { return declSkinType.LocalFindByIndex( index, forceParse ); }
	const class idSoundShader*								FindSoundShaderByIndex( int index, bool forceParse = true ) const { return declSoundShaderType.LocalFindByIndex( index, forceParse ); }
	const class idDeclEntityDef*							FindEntityDefByIndex( int index, bool forceParse = true ) const { return declEntityDefType.LocalFindByIndex( index, forceParse ); }
	const class idDeclAF*									FindAFByIndex( int index, bool forceParse = true ) const { return declAFType.LocalFindByIndex( index, forceParse ); }
	const class rvDeclEffect*								FindEffectByIndex( int index, bool forceParse = true ) const { return declEffectsType.LocalFindByIndex( index, forceParse ); }
	const class sdDeclAtmosphere*							FindAtmosphereByIndex( int index, bool forceParse = true ) const { return declAtmosphereType.LocalFindByIndex( index, forceParse ); }
	const class sdDeclAmbientCubeMap*						FindAmbientCubeMapByIndex( int index, bool forceParse = true ) const { return declAmbientCubeMapType.LocalFindByIndex( index, forceParse ); }
	const class sdDeclStuffType*							FindStuffTypeByIndex( int index, bool forceParse = true ) const { return declStuffTypeType.LocalFindByIndex( index, forceParse ); }
	const class sdDeclDecal*								FindDecalByIndex( int index, bool forceParse = true ) const { return declDecalType.LocalFindByIndex( index, forceParse ); }
	const class sdDeclSurfaceType*							FindSurfaceTypeByIndex( int index, bool forceParse = true ) const { return declSurfaceTypeType.LocalFindByIndex( index, forceParse ); }
	const class sdDeclSurfaceTypeMap*						FindSurfaceTypeMapByIndex( int index, bool forceParse = true ) const { return declSurfaceTypeMapType.LocalFindByIndex( index, forceParse ); }
	const class sdDeclRenderProgram*						FindRenderProgramByIndex( int index, bool forceParse = true ) const { return declRenderProgramType.LocalFindByIndex( index, forceParse ); }
	const class sdDeclRenderBinding*						FindRenderBindingByIndex( int index, bool forceParse = true ) const { return declRenderBindingType.LocalFindByIndex( index, forceParse ); }
	const class sdDeclTemplate*								FindTemplateByIndex( int index, bool forceParse = true ) const { return declTemplateType.LocalFindByIndex( index, forceParse ); }
	const class sdDeclImposter*								FindImposterByIndex( int index, bool forceParse = true ) const { return declImposterType.LocalFindByIndex( index, forceParse ); }
	const class sdDeclLocStr*								FindLocStrByIndex( int index, bool forceParse = true ) const { return declLocStrType.LocalFindByIndex( index, forceParse ); }
};

/*
============
sdDeclTypeHolder::sdDeclTypeHolder
============
*/
ID_INLINE sdDeclTypeHolder::sdDeclTypeHolder() {
	declTableType.Init( declTableIdentifier );
	declMaterialType.Init( declMaterialIdentifier );
	declSkinType.Init( declSkinIdentifier );
	declSoundShaderType.Init( declSoundShaderIdentifier );
	declEntityDefType.Init( declEntityDefIdentifier );
	declEffectsType.Init( declEffectsIdentifier );
	declAFType.Init( declAFIdentifier );
	declAtmosphereType.Init( declAtmosphereIdentifier );
	declAmbientCubeMapType.Init( declAmbientCubeMapIdentifier );
	declStuffTypeType.Init( declStuffTypeIdentifier );
	declDecalType.Init( declDecalIdentifier );
	declSurfaceTypeType.Init( declSurfaceTypeIdentifier );
	declSurfaceTypeMapType.Init( declSurfaceTypeMapIdentifier );
	declRenderProgramType.Init( declRenderProgramIdentifier );
	declRenderBindingType.Init( declRenderBindingIdentifier );
	declTemplateType.Init( declTemplateIdentifier );
	declImposterType.Init( declImposterIdentifier );
	declImposterGeneratorType.Init( declImposterGeneratorIdentifier );
	declLocStrType.Init( declLocStrIdentifier );
	declModelExportType.Init( declModelExportIdentifier );
}

typedef sdSingleton< sdDeclTypeHolder >	declTypeHolder;

#define declHolder declTypeHolder::GetInstance()

#endif /* __DECLTYPEHOLDER_H__ */
