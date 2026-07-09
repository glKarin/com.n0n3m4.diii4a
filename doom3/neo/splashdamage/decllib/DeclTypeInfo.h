#ifndef _DECLTYPEINFO_H
#define _DECLTYPEINFO_H

#include "decllib/declAmbientCubeMap.h"
#include "decllib/declAtmosphere.h"
#include "decllib/declDecal.h"
#include "decllib/declImposter.h"
#include "decllib/declLocStr.h"
#include "decllib/declRenderBinding.h"
#include "decllib/declStuffType.h"
#include "decllib/DeclSurfaceType.h"
#include "decllib/declRenderProgram.h"
#include "decllib/DeclTemplate.h"
#include "decllib/DeclSurfaceTypeMap.h"

extern sdDeclInfo declTableInfo;
extern sdDeclInfo declMaterialInfo;
extern sdDeclInfo declSkinInfo;
extern sdDeclInfo declSoundInfo;
extern sdDeclInfo declEntityDefInfo;
extern sdDeclInfo declMapDefInfo;
extern sdDeclInfo declAFInfo;

extern sdDeclInfo declEffectInfo;
extern sdDeclInfo declAtmosphereInfo;
extern sdDeclInfo declAmbientCubeMapInfo;
extern sdDeclInfo declDecalInfo;
extern sdDeclInfo declSurfaceTypeInfo;
extern sdDeclInfo declImposterInfo;
extern sdDeclInfo declImposterGeneratorInfo;
extern sdDeclInfo declStuffTypeInfo;
extern sdDeclInfo declRenderBindingInfo;
extern sdDeclInfo declRenderProgramInfo;
extern sdDeclInfo declLocStrInfo;
extern sdDeclInfo declTemplateInfo;
extern sdDeclInfo declSurfaceTypeMapInfo;


extern idDeclTypeTemplate< idDeclTable, &declTableInfo > declTableType;
extern idDeclTypeTemplate< idMaterial, &declMaterialInfo > declMaterialType;
extern idDeclTypeTemplate< idDeclSkin, &declSkinInfo > declSkinType;
extern idDeclTypeTemplate< idSoundShader, &declSoundInfo > declSoundType;
extern idDeclTypeTemplate< idDeclEntityDef, &declEntityDefInfo > declEntityDefType;
extern idDeclTypeTemplate< idDeclEntityDef, &declMapDefInfo > declMapDefType;
extern idDeclTypeTemplate< idDeclAF, &declAFInfo > declAFType;

extern idDeclTypeTemplate< rvDeclEffect, &declEffectInfo > declEffectType;
extern idDeclTypeTemplate< sdDeclAtmosphere, &declAtmosphereInfo > declAtmosphereType;
extern idDeclTypeTemplate< sdDeclAmbientCubeMap, &declAmbientCubeMapInfo > declAmbientCubeMapType;
extern idDeclTypeTemplate< sdDeclDecal, &declDecalInfo > declDecalType;
extern idDeclTypeTemplate< sdDeclSurfaceType, &declSurfaceTypeInfo > declSurfaceTypeType;
extern idDeclTypeTemplate< sdDeclSurfaceTypeMap, &declSurfaceTypeMapInfo > declSurfaceTypeMapType;
extern idDeclTypeTemplate< sdDeclImposter, &declImposterInfo > declImposterType;
extern idDeclTypeTemplate< sdDeclImposterGenerator, &declImposterGeneratorInfo > declImposterGeneratorType;
extern idDeclTypeTemplate< sdDeclStuffType, &declStuffTypeInfo > declStuffTypeType;
extern idDeclTypeTemplate< sdDeclRenderBinding, &declRenderBindingInfo > declRenderBindingType;
extern idDeclTypeTemplate< sdDeclRenderProgram, &declRenderProgramInfo > declRenderProgramType;
extern idDeclTypeTemplate< sdDeclLocStr, &declLocStrInfo > declLocStrType;
extern idDeclTypeTemplate< sdDeclTemplate, &declTemplateInfo > declTemplateType;

#endif _DECLTYPEINFO_H
