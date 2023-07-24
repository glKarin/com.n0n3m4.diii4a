// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLATMOSPHERE_H__
#define __DECLATMOSPHERE_H__

#include "../framework/declManager.h"
#include "declAmbientCubeMap.h"

class idRenderModel;

const int NUM_CLOUD_LAYER_PARAMETERS = 12;
const int MAX_CLOUD_LAYERS = 10;

struct sdCloudLayer {
						sdCloudLayer() :
							material( NULL ),
							style( 0 ) {
							memset( parms, 0, sizeof( parms ) );
						}

	const idMaterial*	material;
	int					style;
	float				parms[ NUM_CLOUD_LAYER_PARAMETERS ];
};

// jRAD - if you modify this structure, ensure that you update the atmosphere editor as well
struct sdPrecipitationParameters {
	enum precipitationType_e {
		PT_NONE,
		PT_RAIN,
		PT_SNOW,
		PT_SPLASH,		// Ground impacts
		PT_MODELRAIN,
		PT_MODELSNOW,
	};

	precipitationType_e		preType;

	// Shared parameters
	int						maxParticles;		// Maximum number of particles
	float					heightMin;			// Size of a single particle
	float					heightMax;
	float					weightMin;			// Used to control tumbling speed with snow
	float					weightMax;
	float					windScale;			// Scales the atmosphere's wind direction
	float					gustWindScale;		// Scales the atmosphere's wind direction during gusts 
	float					fallMin;			// Z-axis velocity
	float					fallMax;
	float					timeMin;
	float					timeMax;
	float					tumbleStrength;		// Used to control tumbling strength with snow
	float					precipitationDistance;
	const idMaterial*		material;
	idRenderModel*			model;
	const rvDeclEffect*		effect;

	void					Default();
	bool					Parse( idParser& src );
	void					Save( idFile_Memory& f ) const;
};

class sdDeclAtmosphere : public idDecl {
public:
	struct postProcessParms_t {
		idVec3		tint;
		float		saturation;
		float		contrast;
		idVec4		glareParms;		// source brightness, blur brightness, brightness threshold, threshold dependency
		idVec4		glareBases;		// general glare strength, landscape glare strength
	};

	static const int					NUM_PRECIP_LAYERS = 2;

										sdDeclAtmosphere();
	virtual								~sdDeclAtmosphere() {}

	// Override from idDecl
	virtual const char*					DefaultDefinition( void ) const;
	virtual bool						Parse( const char* text, const int textLength );
	virtual size_t						Size( void ) const { return sizeof( sdDeclAtmosphere ); }
	virtual void						FreeData();

	static void							CacheFromDict( const idDict& dict );

	// Sun
	const idMaterial*					GetSunMaterial() const { return sunMaterial; }
	const idVec3&						GetSunDirection() const { return sunDir; }
	const float							GetSunAzimuth() const { return sunAzimuth; }
	const float							GetSunZenith() const { return sunZenith; }
	const idVec3&						GetSunColor() const { return sunColor; }
	const float							GetSunHaloScale() const { return sunHaloScale; }
	const float							GetSunHaloBias() const { return sunHaloBias; }

	// Sun sprite
	const idMaterial*					GetSunSpriteMaterial() const { return sunSpriteMaterial; }
	const float							GetSunSpriteSize() const { return sunSpriteSize; }

	// Sun flare
	const idMaterial*					GetSunFlareMaterial() const { return sunFlareMaterial; }
	const float							GetSunFlareSize() const { return sunFlareSize; }
	const float							GetSunFlareTime() const { return sunFlareTime; }
	const bool							EnableSunFlareAziZen() const { return enableSunFlareAziZen; }
	const float							GetSunFlareAzi() const { return sunFlareAzi; }
	const float							GetSunFlareZen() const { return sunFlareZen; }

	// Post processing
	const postProcessParms_t&			GetDefaultPostProcessParms() const { return defaultPostProcessParms; }
	postProcessParms_t&					GetPostProcessParms() const { return postProcessParms; }

	// Fog
	const float							GetFogDistHalf() const { return fogDistHalf; }
	const float							GetFogHeightHalf() const { return fogHeightHalf; }
	const float							GetFogHeightOffset() const { return fogHeightOffset; }
	const idVec3&						GetFogColor() const { return fogColor; }
	const float							GetFogStart() const { return fogStart; }
	const float							GetFogEnd() const { return fogEnd; }

	// Misc
	const idMaterial*					GetAtmosphereMaterial() const { return atmosphereMaterial; }
	const sdDeclAmbientCubeMap*			GetAmbientCubeMap() const { return ambientCubeMap; }
	idImage*							GetSkyGradientImage() const { return const_cast< idImage* >( skyGradientImage ); }

	const float							GetFarClip() const { return farClip; }

	const bool							IsNight() const { return isNight; }
	const bool							DrawAtmosphereLast() const { return drawAtmosphereLast; }

	const idVec3&						GetMinSpecShadowColor() const { return minSpecShadowColor; }

	// Clouds
	const idList< sdCloudLayer >&		GetCloudLayers() const { return cloudLayers; }

	// Precipitation
	const sdPrecipitationParameters&	GetPrecipitation( const int layer ) const { return precipitation[ layer ]; }

	// Wind
	float								GetWindAngle() const { return windAngle; }
	float								GetWindAngleDev() const { return windAngleDev; }
	float								GetWindStrength() const { return windStrength; }
	float								GetWindStrengthDev() const { return windStrengthDev; }

	//
	// Needed for editor
	//
	const bool							IsModified() const { return modified; }
	void								Save();
	void								Save( idFile_Memory& f ) const;

	// Sun
	void								SetSunMaterial( const idMaterial* material ) { sunMaterial = material; modified = true; }
	void								SetSunDirection( const idVec3& dir ) { sunDir = dir; modified = true; }
	void								SetSunAzimuth( const float azi ) { sunAzimuth = azi; UpdateSunDirFromAziZen(); modified = true; }
	void								SetSunZenith( const float zen ) { sunZenith = zen; UpdateSunDirFromAziZen(); modified = true; }
	void								SetSunColor( const idVec3& color ) { sunColor = color; modified = true; }
	void								SetSunHaloScale( const float scale ) { sunHaloScale = scale; modified = true; }
	void								SetSunHaloBias( const float bias ) { sunHaloBias = bias; modified = true; }

	// Sun sprite
	void								SetSunSpriteMaterial( const idMaterial* material ) { sunSpriteMaterial = material; modified = true; }
	void								SetSunSpriteSize( const float size ) { sunSpriteSize = size; modified = true; }

	// Sun flare
	void								SetSunFlareMaterial( const idMaterial* material ) { sunFlareMaterial = material; modified = true; }
	void								SetSunFlareSize( const float size ) { sunFlareSize = size; modified = true; }
	void								SetSunFlareTime( const float time ) { sunFlareTime = time; modified = true; }
	void								SetEnableSunFlareAziZen( const bool enabled ) { enableSunFlareAziZen = enabled; modified = true; }
	void								SetSunFlareAzi( const float azi ) { sunFlareAzi = azi; modified = true; }
	void								SetSunFlareZen( const float zen ) { sunFlareZen = zen; modified = true; }

	// Post processing
	void								SetPostProcessParms( const postProcessParms_t& parms ) { postProcessParms = defaultPostProcessParms = parms; modified = true; }

	// Fog
	void								SetFogDistHalf( const float distHalf ) { fogDistHalf = distHalf; modified = true; }
	void								SetFogHeightHalf( const float heightHalf ) { fogHeightHalf = heightHalf; modified = true; }
	void								SetFogHeightOffset( const float heightOffset ) { fogHeightOffset = heightOffset; modified = true; }
	void								SetFogColor( const idVec3& color ) { fogColor = color; modified = true; }
	void								SetFogStart( const float start ) { fogStart = start; modified = true; }
	void								SetFogEnd( const float end ) { fogEnd = end; modified = true; }

	// Misc
	void								SetAtmosphereMaterial( const idMaterial* material ) { atmosphereMaterial = material; modified = true; }
	void								SetAmbientCubeMap( const sdDeclAmbientCubeMap* ambientCubeMap ) { this->ambientCubeMap = ambientCubeMap; modified = true; }
	bool								SetSkyGradientImage( const char* imageName );

	void								SetFarClip( const float farClip ) { this->farClip = farClip; modified = true; }

	void								SetIsNight( const bool isNight ) { this->isNight = isNight; modified = true; }
	void								SetDrawAtmosphereLast( const bool drawAtmosphereLast ) { this->drawAtmosphereLast = drawAtmosphereLast; modified = true; }

	void								SetMinSpecShadowColor( const idVec3& color ) { minSpecShadowColor = color; modified = true; }

	// Clouds
	const int							AddCloudLayer( const sdCloudLayer& cloudLayer ) { modified = true; return cloudLayers.Append( cloudLayer ); }
	void								UpdateCloudLayer( const int layer, const sdCloudLayer& cloudLayer ) { cloudLayers[ layer ] = cloudLayer; modified = true; }
	void								RemoveCloudLayer( const int layer ) { cloudLayers.RemoveIndex( layer ); modified = true; }

	// Precipitation
	sdPrecipitationParameters			GetPrecipitation( const int layer ) { return precipitation[ layer ]; }
	void								SetPrecipitation( const int layer, const sdPrecipitationParameters& parms ) { precipitation[ layer ] = parms;  modified = true; }

	// Wind
	void								SetWindAngle( const float angle ) { windAngle = angle; modified = true; }
	void								SetWindAngleDev( const float angleDev ) { windAngleDev = angleDev; modified = true; }
	void								SetWindStrength( const float strength ) { windStrength = strength; modified = true; }
	void								SetWindStrengthDev( const float strengthDev ) { windStrengthDev = strengthDev; modified = true; }

private:
	bool								ParsePostProcessParms( idParser& src );
	bool								ParseCloudLayer( idParser& src );
	bool								ParsePrecipitationLayer( idParser& src );

	void								RebuildTextSource( idFile_Memory& f ) const;

	void								UpdateSunDirFromAziZen();

private:
	bool								modified;

	const idMaterial*					sunMaterial;
	idVec3								sunDir;
	float								sunAzimuth;
	float								sunZenith;
	idVec3								sunColor;
	float								sunHaloScale;
	float								sunHaloBias;

	const idMaterial*					sunSpriteMaterial;
	float								sunSpriteSize;

	const idMaterial*					sunFlareMaterial;
	float								sunFlareSize;
	float								sunFlareTime;
	bool								enableSunFlareAziZen;
	float								sunFlareAzi;
	float								sunFlareZen;

	postProcessParms_t					defaultPostProcessParms;
	mutable postProcessParms_t			postProcessParms;

	float								fogDistHalf;
	float								fogHeightHalf;
	float								fogHeightOffset;
	idVec3								fogColor;
	float								fogStart;
	float								fogEnd;

	const idMaterial*					atmosphereMaterial;
	const sdDeclAmbientCubeMap*			ambientCubeMap;
	idImage*							skyGradientImage;

	float								farClip;
	bool								isNight;
	bool								drawAtmosphereLast;

	idVec3								minSpecShadowColor;

	float								windAngle;
	float								windAngleDev;
	float								windStrength;
	float								windStrengthDev;

	// cloud layers
	idList< sdCloudLayer >				cloudLayers;

	// precepitation
	int									numPrecipLayers;
	sdPrecipitationParameters			precipitation[ NUM_PRECIP_LAYERS ];
};

#endif // __DECLATMOSPHERE_H__
