// AutoRenderParms.cpp
//

#include "RenderSystem_local.h"

/*
=========================
idRenderSystemLocal::InitRenderParms
=========================
*/
void idRenderSystemLocal::InitRenderParms(void)
{
	albedoTextureParam = declManager->FindRenderParam("albedoTexture");
	bumpmapTextureParam = declManager->FindRenderParam("bumpmap");
	specularTextureParam = declManager->FindRenderParam("specularmap");
	lightfalloffTextureParam = declManager->FindRenderParam("lightfallofftex");
	lightProgTextureParam = declManager->FindRenderParam("lightprogtex");
	bumpmatrixSParam = declManager->FindRenderParam("rpbumpmatrixs");
	bumpmatrixTParam = declManager->FindRenderParam("rpbumpmatrixt");
	diffuseMatrixSParam = declManager->FindRenderParam("rpdiffusematrixs");
	diffuseMatrixTParam = declManager->FindRenderParam("rpdiffusematrixt");
	specularMatrixSParam = declManager->FindRenderParam("rpspecularmatrixs");
	specularMatrixTParam = declManager->FindRenderParam("rpspecularmatrixt");
	viewOriginParam = declManager->FindRenderParam("vieworigin");
	lightColorParam = declManager->FindRenderParam("lightcolor");
	lightScaleParam = declManager->FindRenderParam("lightscale");
	shadowMapInfoParm = declManager->FindRenderParam("shadowMapInfo");
	globalLightOriginParam = declManager->FindRenderParam("globalLightOrigin");
	globalLightExtentsParam = declManager->FindRenderParam("globalLightExtents");

	numLightsParam = declManager->FindRenderParam("numLights");

	atlasLookupParam = declManager->FindRenderParam("atlasLookup");
	shadowMapAtlasParam = declManager->FindRenderParam("shadowMapAtlas");

	modelMatrixX = declManager->FindRenderParam("modelMatrixX");
	modelMatrixY = declManager->FindRenderParam("modelMatrixY");
	modelMatrixZ = declManager->FindRenderParam("modelMatrixZ");
	modelMatrixW = declManager->FindRenderParam("modelMatrixW");

	viewMatrixX = declManager->FindRenderParam("viewMatrixX");
	viewMatrixY = declManager->FindRenderParam("viewMatrixY");
	viewMatrixZ = declManager->FindRenderParam("viewMatrixZ");
	viewMatrixW = declManager->FindRenderParam("viewMatrixW");

	projectionMatrixX = declManager->FindRenderParam("projectionMatrixX");
	projectionMatrixY = declManager->FindRenderParam("projectionMatrixY");
	projectionMatrixZ = declManager->FindRenderParam("projectionMatrixZ");
	projectionMatrixW = declManager->FindRenderParam("projectionMatrixW");

	mvpMatrixX = declManager->FindRenderParam("mvpMatrixX");
	mvpMatrixY = declManager->FindRenderParam("mvpMatrixY");
	mvpMatrixZ = declManager->FindRenderParam("mvpMatrixZ");
	mvpMatrixW = declManager->FindRenderParam("mvpMatrixW");

	vertexColorParm = declManager->FindRenderParam("vertexcolor");

	vertexScaleModulateParam = declManager->FindRenderParam("vertexcolormodulate");
	vertexScaleAddParam = declManager->FindRenderParam("vertexcoloradd");
	screenInfoParam = declManager->FindRenderParam("screeninfo");
	genericShaderParam = declManager->FindRenderParam("genericinfo");
#ifdef __ANDROID__
	textureMatrixX = declManager->FindRenderParam("textureMatrixX");
	textureMatrixY = declManager->FindRenderParam("textureMatrixY");
	textureMatrixZ = declManager->FindRenderParam("textureMatrixZ");
	textureMatrixW = declManager->FindRenderParam("textureMatrixW");
#endif
}
