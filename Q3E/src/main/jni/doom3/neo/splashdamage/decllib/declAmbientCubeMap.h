// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLAMBIENTCUBEMAP_H__
#define __DECLAMBIENTCUBEMAP_H__

#include "../framework/declManager.h"
#include "../renderer/Image.h"

//===============================================================
//
//	sdDeclAmbientCubeMap
//
//===============================================================

class sdDeclAmbientCubeMap : public idDecl {
public:
									struct ambientLight_t {
										void	SetAngles( const idVec2& angles ) {
													dir.x = idMath::Cos( DEG2RAD( angles.x ) ) * idMath::Sin( DEG2RAD( angles.y ) );
													dir.y = idMath::Sin( DEG2RAD( angles.x ) ) * idMath::Sin( DEG2RAD( angles.y ) );
													dir.z = idMath::Cos( DEG2RAD( angles.y ) );
												}

										idVec2	GetAngles( ) const {
													idVec2 angles;

													angles.x = RAD2DEG( idMath::ATan( dir.y, dir.x ) );
													angles.y = RAD2DEG( idMath::ACos( dir.z ) );

													return angles;
												}

										idVec3	dir;
										idVec3	color;
										idStr	name;
										bool    specular;
										bool	ambient;

										ambientLight_t() : specular(true), ambient(true) {
										}
									};

									sdDeclAmbientCubeMap();
	virtual							~sdDeclAmbientCubeMap( void ) {}

	// Override from idDecl
	virtual const char*				DefaultDefinition( void ) const;
	virtual bool					Parse( const char* text, const int textLength );
	virtual size_t					Size( void ) const { return sizeof( sdDeclAmbientCubeMap ); }
	virtual void					FreeData();

	static void						CacheFromDict( const idDict& dict );
	
	// Getters and setters
	const idList< ambientLight_t >&	GetAmbientLights() const { return ambientLights; }
	bool							IsIndoors() const { return indoors; }
	const char *					GetEnvironmentMap() const { return envMap.c_str(); }
	idVec3							GetAmbientColor() const { return ambientColor; }
	idVec3							GetHighLightColor() const { return highLightColor; }
	idVec4							GetAvgAmbientColor() const { return avgAmbientColor; }
	float							GetBrightness() const { return brightness; }
	void							SetBrightness( float b ) { brightness = b; }

	idVec3							GetMinSpecAmbientColor() const { return minSpecAmbientColor; }
	idVec3							GetMinSpecShadowColor() const { return minSpecShadowColor; }
	void							SetMinSpecAmbientColor( const idVec3 &color ) { minSpecAmbientColor = color; }
	void							SetMinSpecShadowColor( const idVec3 &color ) { minSpecShadowColor = color; }

	idImage*						GetAmbientCubeMap() const { return const_cast< idImage* >( ambientCubeMap ); }
	idImage*						GetLightCubeMap() const { return const_cast< idImage* >( lightCubeMap ); }
	idImage*						GetSpecularCubeMap() const { return const_cast< idImage* >( specularCubeMap ); }
	idImage*						GetEnvironmentCubeMap() const { return const_cast< idImage* >( environmentCubeMap ); }
	idImage*						GetGradientMap() const { return const_cast< idImage* >( gradientMap ); }

	void							SetSunParameters( const idVec3& sunDirection, const idVec3& sunColor );

	// Needed for editor
	int								AddAmbientLight( const ambientLight_t& ambientLight ) { return ambientLights.Append( ambientLight ); }
	void							UpdateAmbientLight( int index, const ambientLight_t& value ) { ambientLights[index] = value; }
	void							RemoveAmbientLight( int index ) { ambientLights.RemoveIndex( index ); }
	void							SetIndoors( bool val ) { indoors = val; }
	void							SetEnvironmentMap( const char *name ) { envMap = name; }
	void							SetAmbientColor( const idVec3 &color ) { ambientColor = color; }
	void							SetHighLightColor( const idVec3 &color ) { highLightColor = color; }
	bool							Save();

	void							GenerateImages();

private:
	bool							ParseAmbientLight( idParser *src );
	bool							RebuildTextSource();

	static const int				BAKEDLIGHT_SIZE = 64;
	static const int				GRADIENT_SIZE = 16;

	static float					cubeMapDataFloat[ 6 * BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 ];
	static byte						cubeMapDataByte[ 6 * BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 ];
	static byte						gradientMapData[ GRADIENT_SIZE * 4 ];

	static float*					cubeMapFloat[ 6 ];
	static byte*					cubeMapByte[ 6 ];

	static void						ClearCubeMap( float* cubeMap[6], const int faceSize );
	static void						ScaleCubeMapColor( float* cubeMap[6], const int faceSize, const float scale );
	static void						CubeMapFtob( float* cubeMapFloat[6], byte* cubeMapByte[6], const int faceSize );

	static void						BakeLight( float* cubeMap[6], const int faceSize, const idVec3& lightDir, const idVec3& lightColor);
	static void						BakeLight( float* cubeMap[6], const int faceSize, const idVec3& lightDir, const idVec3& lightColor, const float power );
	static void						BakeGradientMap( byte* pic, const int size, const idVec3& ambientColor, const idVec3& highLightColor );

	static void						UploadCubeMap( idImage* image, const byte* cubeMap[6], const int faceSize );

	idImageGeneratorFunctor< sdDeclAmbientCubeMap >	ambientCubeMapImageFunctor;
	void											AmbientCubeMapImage( idImage* image );
	idImageGeneratorFunctor< sdDeclAmbientCubeMap >	lightCubeMapImageFunctor;
	void											LightCubeMapImage( idImage* image );
	idImageGeneratorFunctor< sdDeclAmbientCubeMap >	specularCubeMapImageFunctor;
	void											SpecularCubeMapImage( idImage* image );
	idImageGeneratorFunctor< sdDeclAmbientCubeMap >	gradientMapImageFunctor;
	void											GradientMapImage( idImage* image );

private:
	idList< ambientLight_t >		ambientLights;
	bool							indoors;			// Don't bake the sun into the light cubeMap
	idStr							envMap;				// Name of the environment cubeMap to use
	idVec3							ambientColor;
	idVec3							highLightColor;
	float							brightness;

	// generated data
	idVec3							sunDirection;
	idVec3							sunColor;

	idVec4							avgAmbientColor;
	idVec3							minSpecAmbientColor;
	idVec3							minSpecShadowColor;

	idImage*						ambientCubeMap;
	idImage*						lightCubeMap;		// Scaled down 4x and contains the sun as well
	idImage*						specularCubeMap;
	idImage*						environmentCubeMap;
	idImage*						gradientMap;
};

#endif /* !__DECLAMBIENTCUBEMAP_H__ */
