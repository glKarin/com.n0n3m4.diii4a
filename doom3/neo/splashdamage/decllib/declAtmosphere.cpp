// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "declAtmosphere.h"
#include "framework/DeclParseHelper.h"
#include "renderer/Image.h"

void sdPrecipitationParameters::Default() {
	preType = PT_NONE;
	maxParticles = 0;
	heightMin = 0.0f;
	heightMax = 0.0f;
	weightMin = 0.0f;
	weightMax = 0.0f;
	windScale = 0.0f;
	gustWindScale = 0.0f;
	fallMin = 0.0f;
	fallMax = 0.0f;
	timeMin = 0.0f;
	timeMax = 0.0f;
	tumbleStrength = 0.0f;
	precipitationDistance = 0.0f;
	material = NULL;
	model = NULL;
	effect = NULL;
}

bool sdPrecipitationParameters::Parse( idParser& src ) {
	return true;
}
void sdPrecipitationParameters::Save( idFile_Memory& f ) const {
}

static void postProcessParms_t_Init(sdDeclAtmosphere::postProcessParms_t &parms) {
	parms.tint.Set(1.0f, 1.0f, 1.0f);
	parms.saturation = 1.0f;
	parms.contrast = 1.0f;
	parms.glareParms.Set(1.0f, 0.0f, 1.0f, 1.0f);
	parms.glareBases.Set(1.0f, 1.0f, 1.0f, 1.0f);
}

sdDeclAtmosphere::sdDeclAtmosphere()
	: modified(false),
	  sunMaterial(NULL),
	  sunAzimuth(0.0f),
	  sunZenith(0.0f),
	  sunHaloScale(1.0f),
	  sunHaloBias(0.0f),
	  sunSpriteMaterial(NULL),
	  sunSpriteSize(0.0f),
	  sunFlareMaterial(NULL),
	  sunFlareSize(0.0f),
	  sunFlareTime(0.0f),
	  enableSunFlareAziZen(false),
	  sunFlareAzi(0.0f),
	  sunFlareZen(0.0f),
	  fogDistHalf(0.0f),
	  fogHeightHalf(0.0f),
	  fogHeightOffset(0.0f),
	  fogStart(0.0f),
	  fogEnd(0.0f),
	  atmosphereMaterial(NULL),
	  ambientCubeMap(NULL),
	  skyGradientImage(NULL),
	  farClip(0.0f),
	  isNight(false),
	  drawAtmosphereLast(false),
	  windAngle(0.0f),
	  windAngleDev(0.0f),
	  windStrength(0.0f),
	  windStrengthDev(0.0f),
	  numPrecipLayers(0)
{
	sunDir.Set(0.0f, 0.0f, 1.0f);
	sunColor.Set(1.0f, 1.0f, 1.0f);

	postProcessParms_t_Init(defaultPostProcessParms);
	postProcessParms = defaultPostProcessParms;

	fogColor.Set(1.0f, 1.0f, 1.0f);

	minSpecShadowColor.Zero();

	cloudLayers.Clear();

	for ( int i = 0; i < NUM_PRECIP_LAYERS; i++ ) {
		precipitation[i].Default();
	}
}

const char* sdDeclAtmosphere::DefaultDefinition( void ) const {
	return "{  }";
}

bool sdDeclAtmosphere::Parse( const char* text, const int textLength ) {
	idParser src;
	idToken	token;

	src.SetFlags(DECL_LEXER_FLAGS);
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclAtmosphere::Parse: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if( !token.Icmp( "sunMaterial" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclAtmosphere::Parse: failed to parse sunMaterial" );
				break;
			}
			sunMaterial = declManager->FindMaterial(token);
			continue;
		}

		if (!token.Icmp("sunDir")) {
			if( !src.Parse1DMatrix(3, sunDir.ToFloatPtr())) {
				src.Error( "sdDeclAtmosphere::Parse: failed to parse sunDir" );
				break;
			}
			continue;
		}

		if (!token.Icmp("sunColor")) {
			if( !src.Parse1DMatrix(3, sunColor.ToFloatPtr())) {
				src.Error( "sdDeclAtmosphere::Parse: failed to parse sunColor" );
				break;
			}
			continue;
		}

		if (!token.Icmp("sunHaloScale")) {
			sunHaloScale = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("sunHaloBias")) {
			sunHaloBias = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "sunSpriteMaterial" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclAtmosphere::Parse: failed to parse sunSpriteMaterial" );
				break;
			}
			sunSpriteMaterial = declManager->FindMaterial(token);
			continue;
		}

		if (!token.Icmp("sunSpriteSize")) {
			sunSpriteSize = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "sunFlareMaterial" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclAtmosphere::Parse: failed to parse sunFlareMaterial" );
				break;
			}
			sunFlareMaterial = declManager->FindMaterial(token);
			continue;
		}

		if (!token.Icmp("sunFlareSize")) {
			sunFlareSize = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("sunFlareTime")) {
			sunFlareTime = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("enableSunFlareAziZen")) {
			enableSunFlareAziZen = src.ParseBool();
			continue;
		}

		if (!token.Icmp("sunFlareAzi")) {
			sunFlareAzi = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("sunFlareZen")) {
			sunFlareZen = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "postProcessParms" )) {
			if(!ParsePostProcessParms(src))
			{
				src.SkipBracedSection(false);
				break;
			}
			postProcessParms = defaultPostProcessParms;
			continue;
		}

		if (!token.Icmp("fogDistHalf")) {
			fogDistHalf = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("fogHeightHalf")) {
			fogHeightHalf = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("fogHeightOffset")) {
			fogHeightOffset = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("fogColor")) {
			if( !src.Parse1DMatrix(3, fogColor.ToFloatPtr())) {
				src.Error( "sdDeclAtmosphere::Parse: failed to parse fogColor" );
				break;
			}
			continue;
		}

		if (!token.Icmp("fogStart")) {
			fogStart = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("fogEnd")) {
			fogEnd = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "atmosphereMaterial" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclAtmosphere::Parse: failed to parse atmosphereMaterial" );
				break;
			}
			atmosphereMaterial = declManager->FindMaterial(token);
			continue;
		}

		if( !token.Icmp( "ambientCubeMap" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclAtmosphere::Parse: failed to parse ambientCubeMap" );
				break;
			}
			ambientCubeMap = static_cast<const sdDeclAmbientCubeMap *>(declManager->GetDeclType("ambientCubeMap")->Find(token));
			continue;
		}

		if( !token.Icmp( "skyGradientImage" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclAtmosphere::Parse: failed to parse skyGradientImage" );
				break;
			}
			skyGradientImage = globalImages->ImageFromFile(token, TF_DEFAULT, true, TR_CLAMP, TD_DEFAULT, CF_HALFSPHERE);
			continue;
		}

		if (!token.Icmp("farClip")) {
			farClip = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("isNight")) {
			isNight = src.ParseBool();
			continue;
		}

		if (!token.Icmp("drawAtmosphereLast")) {
			drawAtmosphereLast = src.ParseBool();
			continue;
		}

		if (!token.Icmp("windAngle")) {
			windAngle = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("windAngleDev")) {
			windAngleDev = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("windStrength")) {
			windStrength = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("windStrengthDev")) {
			windStrengthDev = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "cloudLayer" )) {
			if(!ParseCloudLayer(src))
			{
				src.SkipBracedSection(false);
				break;
			}
			continue;
		}

		if( !token.Icmp( "precipitationLayer" )) {
			if(!ParsePrecipitationLayer(src))
			{
				src.SkipBracedSection(false);
				break;
			}
			continue;
		}

		src.Warning( "sdDeclAtmosphere::Parse: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}

	return true;
}

void sdDeclAtmosphere::FreeData() {
	modified = false;
	sunMaterial = NULL;
	sunDir.Set(0.0f, 0.0f, 1.0f);
	sunAzimuth = 0.0f;
	sunZenith = 0.0f;
	sunColor.Set(1.0f, 1.0f, 1.0f);
	sunHaloScale = 1.0f;
	sunHaloBias = 0.0f;

	sunSpriteMaterial = NULL;
	sunSpriteSize = 0.0f;

	sunFlareMaterial = NULL;
	sunFlareSize = 0.0f;
	sunFlareTime = 0.0f;
	enableSunFlareAziZen = false;
	sunFlareAzi = 0.0f;
	sunFlareZen = 0.0f;

	postProcessParms_t_Init(defaultPostProcessParms);
	postProcessParms = defaultPostProcessParms;

	fogDistHalf = 0.0f;
	fogHeightHalf = 0.0f;
	fogHeightOffset = 0.0f;
	fogColor.Set(1.0f, 1.0f, 1.0f);
	fogStart = 0.0f;
	fogEnd = 0.0f;

	atmosphereMaterial = NULL;
	ambientCubeMap = NULL;
	skyGradientImage = NULL;

	farClip = 0.0f;
	isNight = false;
	drawAtmosphereLast = false;

	minSpecShadowColor.Zero();

	windAngle = 0.0f;
	windAngleDev = 0.0f;
	windStrength = 0.0f;
	windStrengthDev = 0.0f;

	cloudLayers.Clear();

	numPrecipLayers = 0;

	for ( int i = 0; i < NUM_PRECIP_LAYERS; i++ ) {
		precipitation[i].Default();
	}
}

void sdDeclAtmosphere::CacheFromDict( const idDict& dict ) {
}

void sdDeclAtmosphere::Save() {
}

void sdDeclAtmosphere::Save( idFile_Memory& f ) const {
}

bool sdDeclAtmosphere::SetSkyGradientImage( const char* imageName ) {
	skyGradientImage = globalImages->GetImage(imageName);
	return NULL != skyGradientImage;
}

bool sdDeclAtmosphere::ParsePostProcessParms( idParser& src ) {
	idToken token;
	if( !src.ExpectTokenString( "{" )) {
		src.Error( "sdDeclAmbientCubeMap::ParsePostProcessParms: expected {." );
		return false;
	}

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclAmbientCubeMap::ParsePostProcessParms: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("tint")) {
			if( !src.Parse1DMatrix(3, defaultPostProcessParms.tint.ToFloatPtr())) {
				src.Error( "sdDeclAmbientCubeMap::ParsePostProcessParms: failed to parse tint" );
				break;
			}
			continue;
		}

		if (!token.Icmp("saturation")) {
			defaultPostProcessParms.saturation = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("contrast")) {
			defaultPostProcessParms.contrast = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("glareParms")) {
			if( !src.Parse1DMatrix(4, defaultPostProcessParms.glareParms.ToFloatPtr())) {
				src.Error( "sdDeclAmbientCubeMap::ParsePostProcessParms: failed to parse glareParms" );
				break;
			}
			continue;
		}

		if (!token.Icmp("glareBases")) {
			if( !src.Parse1DMatrix(4, defaultPostProcessParms.glareBases.ToFloatPtr())) {
				src.Error( "sdDeclAmbientCubeMap::ParsePostProcessParms: failed to parse glareBases" );
				break;
			}
			continue;
		}

		src.Warning( "sdDeclAmbientCubeMap::ParsePostProcessParms: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}

	return true;
}

bool sdDeclAtmosphere::ParseCloudLayer( idParser& src ) {
	idToken token;

	if (!src.ReadToken(&token)) {
		src.Error( "sdDeclAtmosphere::ParseCloudLayer: expected name." );
		return false;
	}

	if( !src.ExpectTokenString( "{" )) {
		src.Error( "sdDeclAtmosphere::ParseCloudLayer: expected {." );
		return false;
	}

	sdCloudLayer item;
	item.material = declManager->FindMaterial(token);
	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclAtmosphere::ParseCloudLayer: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("style")) {
			if (src.ReadToken(&token)) {
				if (token.type == TT_INTEGER)
					item.style = token.GetIntValue();
				else
				{
					if(!token.Icmp("skybox"))
						item.style = 1;
					else if(!token.Icmp("old"))
						item.style = 0;
					else
						item.style = 0; // old
				}
			}
			continue;
		}

		if (!token.Icmp("parms")) {
			int num = src.ParseInt();
			if (num > NUM_CLOUD_LAYER_PARAMETERS) {
				src.Error( "sdDeclAtmosphere::ParseCloudLayer: cloudLayer parms %d > %d", num, NUM_CLOUD_LAYER_PARAMETERS );
				return false;
			}
			if( !src.Parse1DMatrix(num, item.parms)) {
				src.Error( "sdDeclAtmosphere::ParseCloudLayer: failed to parse parms" );
				return false;
			}
			continue;
		}

		src.Warning( "sdDeclAtmosphere::ParseCloudLayer: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}
	cloudLayers.Append(item);

	return true;
}

bool sdDeclAtmosphere::ParsePrecipitationLayer( idParser& src ) {
	idToken token;
	if( !src.ExpectTokenString( "{" )) {
		src.Error( "sdDeclAmbientCubeMap::ParsePrecipitationLayer: expected {." );
		return false;
	}

	if (numPrecipLayers >= NUM_CLOUD_LAYER_PARAMETERS) {
		src.Error( "sdDeclAtmosphere::ParseCloudLayer: precipitationLayer num over %d", NUM_CLOUD_LAYER_PARAMETERS );
		return false;
	}

	sdPrecipitationParameters item;
	item.Default();
	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclAmbientCubeMap::ParsePrecipitationLayer: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("type")) {
			item.preType = static_cast<sdPrecipitationParameters::precipitationType_e>(src.ParseInt());
			continue;
		}

		if (!token.Icmp("maxParticles")) {
			item.maxParticles = src.ParseInt();
			continue;
		}

		if (!token.Icmp("heightMin")) {
			item.heightMin = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("heightMax")) {
			item.heightMax = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("weightMin")) {
			item.weightMin = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("weightMax")) {
			item.weightMax = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("timeMin")) {
			item.timeMin = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("timeMax")) {
			item.timeMax = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("windScale")) {
			item.windScale = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("gustWindScale")) {
			item.gustWindScale = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("fallMin")) {
			item.fallMin = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("fallMax")) {
			item.fallMax = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("tumbleStrength")) {
			item.tumbleStrength = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("precipitationDistance")) {
			item.precipitationDistance = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "material" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclAtmosphere::ParsePrecipitationLayer: failed to parse material" );
				break;
			}
			item.material = declManager->FindMaterial(token);
			continue;
		}

		if( !token.Icmp( "model" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclAtmosphere::ParsePrecipitationLayer: failed to parse model" );
				break;
			}
			item.model = renderModelManager->FindModel(token);
			continue;
		}

		if( !token.Icmp( "effect" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclAtmosphere::ParsePrecipitationLayer: failed to parse effect" );
				break;
			}
			item.effect = declManager->FindEffect(token);
			continue;
		}

		src.Warning( "sdDeclAmbientCubeMap::ParsePrecipitationLayer: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}
	precipitation[numPrecipLayers++] = item;

	return true;
}

void sdDeclAtmosphere::RebuildTextSource( idFile_Memory& f ) const {
}

void sdDeclAtmosphere::UpdateSunDirFromAziZen() {
}

