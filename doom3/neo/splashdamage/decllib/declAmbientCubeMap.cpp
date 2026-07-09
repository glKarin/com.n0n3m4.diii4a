// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "declAmbientCubeMap.h"
#include "framework/DeclParseHelper.h"

//===============================================================
//
//	sdDeclAmbientCubeMap
//
//===============================================================

static const idMat3 & CubeMap_GetAxis(int a1)
{
	static bool isInit = false;
	static idMat3 axis[6];
	if (!isInit)
	{
		// forward = east (positive x-axis in DR)
		axis[0][0][0] = 1;
		axis[0][1][1] = 1;
		axis[0][2][2] = 1;

		// left = north
		axis[1][0][1] = 1;
		axis[1][1][0] = -1;
		axis[1][2][2] = 1;

		// right = south
		axis[2][0][1] = -1;
		axis[2][1][0] = 1;
		axis[2][2][2] = 1;

		// back = west
		axis[3][0][0] = -1;
		axis[3][1][1] = -1;
		axis[3][2][2] = 1;

		// down, while facing forward
		axis[4][0][2] = -1;
		axis[4][1][1] = 1;
		axis[4][2][0] = 1;

		// up, while facing forward
		axis[5][0][2] = 1;
		axis[5][1][1] = 1;
		axis[5][2][0] = -1;
		isInit = true;
	}
	return axis[a1];
}

sdDeclAmbientCubeMap::sdDeclAmbientCubeMap()
	: indoors(false),
	brightness(1.0f),
	ambientCubeMap(NULL),
	lightCubeMap(NULL),
	specularCubeMap(NULL),
	environmentCubeMap(NULL),
	gradientMap(NULL) {
		ambientColor.Set(1.0f, 1.0, 1.0f);
		highLightColor.Set(1.0f, 1.0, 1.0f);
		sunDirection.Set(0.0f, 0.0, -1.0f);
		sunColor.Set(1.0f, 1.0, 1.0f);
		avgAmbientColor.Set(1.0f, 1.0, 1.0f, 1.0f);
		minSpecAmbientColor.Zero();
		minSpecShadowColor.Zero();

		ambientCubeMapImageFunctor.Init(this, &sdDeclAmbientCubeMap::AmbientCubeMapImage);
		lightCubeMapImageFunctor.Init(this, &sdDeclAmbientCubeMap::LightCubeMapImage);
		specularCubeMapImageFunctor.Init(this, &sdDeclAmbientCubeMap::SpecularCubeMapImage);
		gradientMapImageFunctor.Init(this, &sdDeclAmbientCubeMap::GradientMapImage);
	}

const char* sdDeclAmbientCubeMap::DefaultDefinition( void ) const {
	return "{  }";
}

bool sdDeclAmbientCubeMap::Parse( const char* text, const int textLength ) {
	idParser src;
	idToken	token;

	src.SetFlags(DECL_LEXER_FLAGS);
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclAmbientCubeMap::Parse: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if( !token.Icmp( "AmbientLight" )) {
			if(!ParseAmbientLight(&src))
			{
				src.SkipBracedSection(false);
				break;
			}
			continue;
		}

		if (!token.Icmp("ambientColor")) {
			avgAmbientColor[0] = src.ParseFloat();
			avgAmbientColor[1] = src.ParseFloat();
			avgAmbientColor[2] = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("highLightColor")) {
			highLightColor[0] = src.ParseFloat();
			highLightColor[1] = src.ParseFloat();
			highLightColor[2] = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("brightness")) {
			brightness = src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "envMap" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclAmbientCubeMap::Parse: failed to parse envMap" );
				break;
			}
			envMap = token.c_str();
			continue;
		}

		if (!token.Icmp("minSpecAmbientColor")) {
			minSpecAmbientColor[0] = src.ParseFloat();
			minSpecAmbientColor[1] = src.ParseFloat();
			minSpecAmbientColor[2] = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("minSpecShadowColor")) {
			minSpecShadowColor[0] = src.ParseFloat();
			minSpecShadowColor[1] = src.ParseFloat();
			minSpecShadowColor[2] = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("indoors")) {
			indoors = true;
			continue;
		}

		src.Warning( "sdDeclAmbientCubeMap::Parse: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}

	GenerateImages();

	return true;
}

void sdDeclAmbientCubeMap::FreeData() {
	indoors = false;
	brightness = 1.0f;
	ambientCubeMap = NULL;
	lightCubeMap = NULL;
	specularCubeMap = NULL;
	environmentCubeMap = NULL;
	gradientMap = NULL;
	ambientColor.Set(1.0f, 1.0, 1.0f);
	highLightColor.Set(1.0f, 1.0, 1.0f);
	sunDirection.Set(0.0f, 0.0, -1.0f);
	sunColor.Set(1.0f, 1.0, 1.0f);
	avgAmbientColor.Set(1.0f, 1.0, 1.0f, 1.0f);
	minSpecAmbientColor.Zero();
	minSpecShadowColor.Zero();
}

void sdDeclAmbientCubeMap::CacheFromDict( const idDict& dict ) {
	const idKeyValue* kv = NULL;

	while( kv = dict.MatchPrefix( "ambientCubeMap", kv ) ) {
		if ( kv->GetValue().Length() ) {
			declAmbientCubeMapType[ kv->GetValue() ];
		}
	}
}

bool sdDeclAmbientCubeMap::Save() {
	return true;
}

void sdDeclAmbientCubeMap::GenerateImages() {
	idStr path = GetFileName();
	path.StripFilename();

	ambientCubeMap = globalImages->ImageFromFunctor(va("%s/%s_ambientCubeMap", path.c_str(), GetName()), &ambientCubeMapImageFunctor);

	specularCubeMap = globalImages->ImageFromFunctor(va("%s/%s_specularCubeMap", path.c_str(), GetName()), &specularCubeMapImageFunctor);

	lightCubeMap = globalImages->ImageFromFunctor(va("%s/%s_lightCubeMap", path.c_str(), GetName()), &lightCubeMapImageFunctor);

	gradientMap = globalImages->ImageFromFunctor(va("%s/%s_gradientMap", path.c_str(), GetName()), &gradientMapImageFunctor);

	if (!envMap.IsEmpty())
	{
		environmentCubeMap = globalImages->ImageFromFile(envMap.c_str(),
			TF_LINEAR, // 0
			false, // ???
			TR_CLAMP, // 1
			TD_DEFAULT, // 2
			CF_NATIVE // 1
		);
	}
}

bool sdDeclAmbientCubeMap::ParseAmbientLight( idParser *src ) {
	idToken token;
	if( !src->ExpectTokenString( "{" )) {
		src->Error( "sdDeclAmbientCubeMap::ParseAmbientLight: expected {." );
		return false;
	}

	ambientLight_t item;
	while (1) {
		if( !src->ReadToken( &token )) {
			src->Error( "sdDeclAmbientCubeMap::ParseAmbientLight: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("Color")) {
			item.color[0] = src->ParseFloat();
			item.color[1] = src->ParseFloat();
			item.color[2] = src->ParseFloat();
			continue;
		}

		if (!token.Icmp("Direction")) {
			item.dir[0] = src->ParseFloat();
			item.dir[1] = src->ParseFloat();
			item.dir[2] = src->ParseFloat();
			continue;
		}

		if (!token.Icmp("name")) {
			if( !src->ReadToken(&token)) {
				src->Error( "sdDeclAmbientCubeMap::ParseAmbientLight: failed to parse name" );
				break;
			}
			item.name = token.c_str();
			continue;
		}

		if (!token.Icmp("specular")) {
			item.specular = src->ParseBool();
			continue;
		}

		if (!token.Icmp("ambient")) {
			item.ambient = src->ParseBool();
			continue;
		}

		src->Warning( "sdDeclAmbientCubeMap::ParseAmbientLight: unexpected token '%s'.", token.c_str() );
		src->SkipBracedSection(false);
		break;
	}
	ambientLights.Append(item);

	return true;
}

bool sdDeclAmbientCubeMap::RebuildTextSource() {
	return true;
}

float sdDeclAmbientCubeMap::cubeMapDataFloat[ 6 * BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 ];
byte sdDeclAmbientCubeMap::cubeMapDataByte[ 6 * BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 ];
byte sdDeclAmbientCubeMap::gradientMapData[ GRADIENT_SIZE * 4 ];

float* sdDeclAmbientCubeMap::cubeMapFloat[ 6 ];
byte* sdDeclAmbientCubeMap::cubeMapByte[ 6 ];

void sdDeclAmbientCubeMap::ClearCubeMap( float* cubeMap[6], const int faceSize ) {
}

void sdDeclAmbientCubeMap::ScaleCubeMapColor( float* cubeMap[6], const int faceSize, const float scale ) {
	double v3; // st7
	int v4; // edi
	int v5; // esi
	int v6; // edx
	float *v7; // eax
	unsigned int v8; // ecx
	double v9; // st6
	float *v10; // eax
	int v11; // ecx

	v3 = scale;
	v4 = 0;
	v5 = 4 * faceSize * faceSize;
	do
	{
		v6 = 0;
		if ( v5 >= 4 )
		{
			v7 = cubeMap[v4] + 2;
			v8 = ((unsigned int)(v5 - 4) >> 2) + 1;
			v6 = 4 * v8;
			do
			{
				v9 = *(v7 - 2);
				v7 += 4;
				--v8;
				*(v7 - 6) = v9 * v3;
				*(v7 - 5) = v3 * *(v7 - 5);
				*(v7 - 4) = *(v7 - 4) * v3;
				*(v7 - 3) = *(v7 - 3) * v3;
			}
			while ( v8 );
		}
		if ( v6 < v5 )
		{
			v10 = &cubeMap[v4][v6];
			v11 = v5 - v6;
			do
			{
				++v10;
				--v11;
				*(v10 - 1) = v3 * *(v10 - 1);
			}
			while ( v11 );
		}
		++v4;
	}
	while ( v4 < 6 );
}

void sdDeclAmbientCubeMap::CubeMapFtob( float* cubeMapFloat[6], byte* cubeMapByte[6], const int faceSize ) {
	int length;
	int m;
	int i;
	float comp;

	length = 4 * faceSize * faceSize;
	for( m = 0; m < 6; m++)
	{
		for (i = 0; i < length; i++)
		{
			comp = cubeMapFloat[m][i] * 255.0f;
			cubeMapByte[m][i] = (byte)fminf(fmaxf(comp, 0.0f), 255.0f);
		}
	}
}

void sdDeclAmbientCubeMap::BakeLight( float* cubeMap[6], const int faceSize, const idVec3& lightDir, const idVec3& lightColor) {
	int i; // edi
	int v5; // ecx
	int v6; // edx
	double v7; // st7
	double v8; // st6
	int v9; // esi
	double v10; // st2
	float *v11; // edx
	double v12; // st7
	double v13; // st6
	double v14; // st7
	int v15; // [esp+10h] [ebp-78h]
	float v16; // [esp+14h] [ebp-74h]
	float v17; // [esp+14h] [ebp-74h]
	float v18; // [esp+14h] [ebp-74h]
	float v19; // [esp+14h] [ebp-74h]
	float v20; // [esp+14h] [ebp-74h]
	float v21; // [esp+14h] [ebp-74h]
	float v22; // [esp+14h] [ebp-74h]
	float v23; // [esp+14h] [ebp-74h]
	float v24; // [esp+14h] [ebp-74h]
	float v25; // [esp+14h] [ebp-74h]
	int v26; // [esp+18h] [ebp-70h]
	double v27; // [esp+28h] [ebp-60h]
	idVec3 v28; // [esp+34h] [ebp-54h]
	idVec3 v29; // [esp+40h] [ebp-48h]
	idVec3 v30; // v30 v31 v32
	idVec3 v33; // [esp+58h] [ebp-30h]
	idVec3 v34; // v34 v35 v36
	idVec3 v37; // v37 v38 v39
	idVec3 v40; // v40 v41 v42

	for ( i = 0; i < 6; ++i )
	{
		v29 = CubeMap_GetAxis(i)[0];
		v28 = CubeMap_GetAxis(i)[1];
		v33 = CubeMap_GetAxis(i)[2];
		v5 = faceSize;
		v15 = 0;
		if ( faceSize > 0 )
		{
			v6 = 4 * faceSize;
			v7 = (double)faceSize - 1.0;
			v27 = v7;
			v8 = 2.0f;
			do
			{
				v26 = 0;
				v9 = 4 * v15;
				v16 = -((double)v15 * v8 / v7 - 1.0f);
				v30 = v28 * v16;
				do
				{
					v17 = -((double)v26 * v8 / v7 - 1.0f);
					v37 = v33 * v17;
					v34 = v29 + v30;
					v40 = v34 + v37;
					v18 = lightDir * v40;
					v10 = v18;
					if ( v18 >= 0.0f )
					{
						v19 = v40.LengthSqr();
						v20 = sqrt(v19);
						v11 = cubeMap[i];
						v21 = v10 / v20;
						v12 = v21;
						v22 = lightColor.x * v21 + v11[v9];
						if ( v22 >= 1.0f )
							v22 = 1.0f;
						v11[v9] = v22;
						v23 = lightColor.y * v12 + v11[v9 + 1];
						if ( v23 >= 1.0f )
							v23 = 1.0f;
						v11[v9 + 1] = v23;
						v13 = v12 * lightColor.z + v11[v9 + 2];
						v14 = 1.0f;
						v24 = v13;
						if ( v24 < 1.0f )
							v14 = v24;
						v5 = faceSize;
						v25 = v14;
						v11[v9 + 2] = v25;
						v6 = 4 * faceSize;
						v8 = 2.0f;
						v7 = v27;
					}
					v9 += v6;
					++v26;
				}
				while ( v26 < v5 );
				++v15;
			}
			while ( v15 < v5 );
		}
	}
}

void sdDeclAmbientCubeMap::BakeLight( float* cubeMap[6], const int faceSize, const idVec3& lightDir, const idVec3& lightColor, const float power ) {
	int i; // edi
  int v6; // esi
  double v7; // st7
  double v8; // st6
  float *v9; // edx
  double v10; // st6
  float v11; // [esp+10h] [ebp-70h]
  float v12; // [esp+10h] [ebp-70h]
  float v13; // [esp+10h] [ebp-70h]
  float v14; // [esp+10h] [ebp-70h]
  float v15; // [esp+10h] [ebp-70h]
  float v16; // [esp+10h] [ebp-70h]
  float v17; // [esp+10h] [ebp-70h]
  float v18; // [esp+10h] [ebp-70h]
  float v19; // [esp+10h] [ebp-70h]
  float v20; // [esp+10h] [ebp-70h]
  float v21; // [esp+10h] [ebp-70h]
  int v22; // [esp+14h] [ebp-6Ch]
  int v23; // [esp+18h] [ebp-68h]
  double v24; // [esp+20h] [ebp-60h]
  idVec3 v25; // v25 v26 v27
  idVec3 v28; // [esp+38h] [ebp-48h]
  idVec3 v29; // [esp+44h] [ebp-3Ch]
  idVec3 v30; // v30 v31 v32
  idVec3 v33; // [esp+5Ch] [ebp-24h]
  idVec3 v34; // v34 v35 v36
  idVec3 v37; // v37 v38 v39

  for ( i = 0; i < 6; ++i )
  {
    v29 = CubeMap_GetAxis(i)[0];
    v28 = CubeMap_GetAxis(i)[1];
    v33 = CubeMap_GetAxis(i)[2];
    v22 = 0;
    if ( faceSize > 0 )
    {
      v24 = (double)faceSize - 1.0;
      do
      {
        v23 = 0;
        v6 = 4 * v22;
        v11 = -(((double)v22 + (double)v22) / v24 - 1.0);
        v30 = v28 * v11;
        do
        {
          v12 = -(((double)v23 + (double)v23) / v24 - 1.0);
          v37 = v33 * v12;
          v34 = v29 + v30;
          v25 = v34 + v37;
          v13 = v25.LengthSqr();
          v14 = sqrt(v13);
          if ( v14 >= 0.00000011920929 )
          {
            v15 = 1.0f / v14;
            v25 = v25 * v15;
          }
          v16 = lightDir * v25;
          if ( v16 >= 0.0 )
          {
            v17 = pow(v16, power);
            if ( v17 >= 0.0 )
            {
              v8 = 1.0;
              if ( v17 <= 1.0 )
                v8 = v17;
              v7 = 1.0;
              v18 = v8;
            }
            else
            {
              v18 = 0.0;
              v7 = 1.0;
            }
            v9 = cubeMap[i];
            v10 = v18;
            v19 = lightColor.x * v18 + v9[v6];
            if ( v19 >= v7 )
              v19 = v7;
            v9[v6] = v19;
            v20 = lightColor.y * v10 + v9[v6 + 1];
            if ( v20 >= v7 )
              v20 = v7;
            v9[v6 + 1] = v20;
            v21 = v10 * lightColor.z + v9[v6 + 2];
            if ( v21 >= v7 )
              v21 = v7;
            v9[v6 + 2] = v21;
          }
          v6 += 4 * faceSize;
          ++v23;
        }
        while ( v23 < faceSize );
        ++v22;
      }
      while ( v22 < faceSize );
    }
  }
}

void sdDeclAmbientCubeMap::BakeGradientMap( byte* pic, const int size, const idVec3& ambientColor, const idVec3& highLightColor ) {
	byte *v6; // ecx
	float v7; // st4
	float v9; // [esp+Ch] [ebp-58h]
	float v10; // [esp+Ch] [ebp-58h]
	idVec3 v11; // v11 v12 v13
	idVec3 v14; // v14 v15 v16
	idVec3 v17; // v17 v18 v19
	idVec3 v20; // v20 v21 v22
	float v23; // [esp+6Ch] [ebp+8h]
	int i;

	if ( size > 0 )
	{
		v23 = (float)(size - 1);
		v6 = pic;
		for(i = 0; i < size; i++)
		{
			v9 = (float)i / v23;
			v7 = v9;
			v10 = 1.0f - v9;
			v14 = highLightColor * v7;
			v11 = ambientColor * v10;
			v17 = v11 + v14;
			v20 = v17 * 255.0f;
			v6[0] = (byte)fminf(fmaxf(v20.x, 0.0f), 255.0f);
			v6[1] = (byte)fminf(fmaxf(v20.y, 0.0f), 255.0f);
			v6[2] = (byte)fminf(fmaxf(v20.z, 0.0f), 255.0f);
			v6[3] = 255;
			v6 += 4;
		}
	}
}

void sdDeclAmbientCubeMap::UploadCubeMap( idImage* image, const byte* cubeMap[6], const int faceSize ) {
#if 1
	image->GenerateCubeImage(cubeMap, faceSize, TF_LINEAR, false, TD_HIGH_QUALITY);
#else
	int i; // esi

	if ( !image->IsLoaded()) // a1->vtbl + 4
		image->Reload(false, true, true); // a1->vtbl + 11
	image->Bind(); // a1->vtbl + 2
	for ( i = 0; i < 6; ++i )
	{
		qglTexImage2D(i + GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA/*GL_RGBA8*/, faceSize, faceSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, cubeMap[i]);
	}
#endif
#if 0
	for ( int i = 0; i < 6; ++i )
	{
		//fileSystem->WriteTGA(va("textures/%s_%d.tga", image->imgName.c_str(), i), cubeMap[i], faceSize, faceSize);
		extern void R_WriteJPG(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical = false, int compression = 0, const char *basePath = NULL);
		R_WriteJPG(va("texturesxxx/%s_%d.jpg", image->imgName.c_str(), i), cubeMap[i], faceSize, faceSize,4);
	}
#endif
}

void sdDeclAmbientCubeMap::AmbientCubeMapImage( idImage* image ) {
	float **v8; // ecx
	int v9; // edx
	int v10; // eax
	int v11; // edi
	double v12; // st6
	float *v13; // edi
	float *v14; // ebx
	double v15; // st6
	float v16; // [esp+10h] [ebp-Ch]
	float *v17; // [esp+14h] [ebp-8h]
	float *v18; // [esp+18h] [ebp-4h]
	float v19; // [esp+18h] [ebp-4h]
	float v20; // [esp+18h] [ebp-4h]
	float v21; // [esp+18h] [ebp-4h]
	float v22; // [esp+18h] [ebp-4h]
	float v23; // [esp+18h] [ebp-4h]
	float v24; // [esp+18h] [ebp-4h]
	float v25; // [esp+18h] [ebp-4h]
	float v26; // [esp+18h] [ebp-4h]
	float v27; // [esp+18h] [ebp-4h]
	int i;

	for(i = 0; i < 6; i++)
	{
		cubeMapFloat[i] = &cubeMapDataFloat[BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 * i];
		cubeMapByte[i] = &cubeMapDataByte[BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 * i];
	}

	//float **off_81DA44 = &cubeMapFloat[0];
	//byte **off_81DA5C = &cubeMapByte[0];

	//image->generatorFunctor = NULL;
	//image->Reload(false, true, true); // (_DWORD *)image->vtbl + 11
	for ( i = 0; i < 6; i++ )
	{
		memset(cubeMapFloat[i]/**off_81DA44++*/, 0, BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 * sizeof(float)/*0x10000u*/);
		memset(cubeMapByte[i]/**off_81DA5C++*/, 0, BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4/*0x4000u*/);
	}

	for ( i = 0; i < this->ambientLights.Num(); i++ )
	{
		const sdDeclAmbientCubeMap::ambientLight_t &light = this->ambientLights[i];
		if ( light.ambient )
			sdDeclAmbientCubeMap::BakeLight(cubeMapFloat, BAKEDLIGHT_SIZE, light.dir, light.color);
	}

	this->avgAmbientColor.Zero();
	v16 = 0.0f;
	for ( i = 0; i < 6; i++ )
	{
		v8 = &cubeMapFloat[i];
		v9 = -2;
		v10 = 3;
		do
		{
			v11 = (v9 - 2) & 3;
			v12 = (*v8)[v10 - 3] + *(&this->avgAmbientColor.x + v11);
			v13 = &this->avgAmbientColor.x + v11;
			*v13 = v12;
			v14 = &this->avgAmbientColor.x + ((v9 - 1) & 3);
			*v14 = (*v8)[v10 - 2] + *v14;
			v17 = &this->avgAmbientColor.x + (v9 & 3);
			v15 = (*v8)[v10 - 1] + *v17;
			v10 += 8;
			*v17 = v15;
			v18 = &this->avgAmbientColor.x + ((v9 + 1) & 3);
			v9 += 8;
			*v18 = (*v8)[v10 - 8] + *v18;
			*v13 = (*v8)[v10 - 7] + *v13;
			*v14 = (*v8)[v10 - 6] + *v14;
			*v17 = (*v8)[v10 - 5] + *v17;
			*v18 = (*v8)[v10 - 4] + *v18;
			v19 = v16 + 1.0f;
			v20 = v19 + 1.0f;
			v21 = v20 + 1.0f;
			v22 = v21 + 1.0f;
			v23 = v22 + 1.0f;
			v24 = v23 + 1.0f;
			v25 = v24 + 1.0f;
			v16 = v25 + 1.0f;
		}
		while ( v9 + 2 < 0x4000 );
	}
	v26 = v16 * 0.25f;
	v27 = 1.0f / v26;
	this->avgAmbientColor *= v27;
	sdDeclAmbientCubeMap::CubeMapFtob(cubeMapFloat/*off_81DA44*/, cubeMapByte/*off_81DA5C*/, BAKEDLIGHT_SIZE);
	sdDeclAmbientCubeMap::UploadCubeMap(image, (const byte **)cubeMapByte/*off_81DA5C*/, BAKEDLIGHT_SIZE);
	//image->generatorFunctor = &this->ambientCubeMapImageFunctor;
}

void sdDeclAmbientCubeMap::LightCubeMapImage( idImage* image ) {
	int i;

	for(i = 0; i < 6; i++)
	{
		cubeMapFloat[i] = &cubeMapDataFloat[BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 * i];
		cubeMapByte[i] = &cubeMapDataByte[BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 * i];
	}

	//float **off_81DA44 = &cubeMapFloat[0];
	//byte **off_81DA5C = &cubeMapByte[0];

	//image->generatorFunctor = NULL;
	//image->Reload(false, true, true); // (_DWORD *)image->vtbl + 11
	for ( i = 0; i < 6; i++ )
	{
		memset(cubeMapFloat[i]/**off_81DA44++*/, 0, BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 * sizeof(float)/*0x10000u*/);
		memset(cubeMapByte[i]/**off_81DA5C++*/, 0, BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4/*0x4000u*/);
	}

	for ( i = 0; i < this->ambientLights.Num(); i++ )
	{
		const sdDeclAmbientCubeMap::ambientLight_t &light = this->ambientLights[i];
		if ( light.ambient )
			sdDeclAmbientCubeMap::BakeLight(cubeMapFloat/*off_81DA44*/, BAKEDLIGHT_SIZE, light.dir, light.color);
	}
	sdDeclAmbientCubeMap::ScaleCubeMapColor(cubeMapFloat/*off_81DA44*/, BAKEDLIGHT_SIZE, 0.25f);
	if ( !this->indoors
			&& !this->sunColor.IsZero() // (LODWORD(this->sunColor.x) | LODWORD(this->sunColor.y) | LODWORD(this->sunColor.z)) & 0x7FFFFFFF) != 0
		)
		{
			sdDeclAmbientCubeMap::BakeLight(cubeMapFloat/*off_81DA44*/, BAKEDLIGHT_SIZE, this->sunDirection, this->sunColor);
		}
	sdDeclAmbientCubeMap::CubeMapFtob(cubeMapFloat/*off_81DA44*/, cubeMapByte/*off_81DA5C*/, BAKEDLIGHT_SIZE);
	sdDeclAmbientCubeMap::UploadCubeMap(image, (const byte **)cubeMapByte/*off_81DA5C*/, BAKEDLIGHT_SIZE);
	//image->generatorFunctor = &this->lightCubeMapImageFunctor;
}

void sdDeclAmbientCubeMap::SpecularCubeMapImage( idImage* image ) {
	int i;

	for(i = 0; i < 6; i++)
	{
		cubeMapFloat[i] = &cubeMapDataFloat[BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 * i];
		cubeMapByte[i] = &cubeMapDataByte[BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 * i];
	}

	//float **off_81DA44 = &cubeMapFloat[0];
	//byte **off_81DA5C = &cubeMapByte[0];

	//image->generatorFunctor = NULL;
	//image->Reload(false, true, true); // (_DWORD *)image->vtbl + 11
	for ( i = 0; i < 6; i++ )
	{
		memset(cubeMapFloat[i]/**off_81DA44++*/, 0, BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4 * sizeof(float)/*0x10000u*/);
		memset(cubeMapByte[i]/**off_81DA5C++*/, 0, BAKEDLIGHT_SIZE * BAKEDLIGHT_SIZE * 4/*0x4000u*/);
	}

	for ( i = 0; i < this->ambientLights.Num(); i++ )
	{
		const sdDeclAmbientCubeMap::ambientLight_t &light = this->ambientLights[i];
		if ( light.specular )
			sdDeclAmbientCubeMap::BakeLight(cubeMapFloat/*off_81DA44*/, BAKEDLIGHT_SIZE, light.dir, light.color, 16.0f);
	}
	sdDeclAmbientCubeMap::CubeMapFtob(cubeMapFloat/*off_81DA44*/, cubeMapByte/*off_81DA5C*/, BAKEDLIGHT_SIZE);
	sdDeclAmbientCubeMap::UploadCubeMap(image, (const byte **)cubeMapByte/*off_81DA5C*/, BAKEDLIGHT_SIZE);
	//image->generatorFunctor = &this->specularCubeMapImageFunctor;
}

void sdDeclAmbientCubeMap::GradientMapImage( idImage* image ) {
	sdDeclAmbientCubeMap::BakeGradientMap(gradientMapData, GRADIENT_SIZE, ambientColor, highLightColor);
	// vtbl = a2->vtbl;	// vtbl + 6)

	image->GenerateImage(gradientMapData, GRADIENT_SIZE, 1,
	  TF_LINEAR, // 0
	  false,
	  TR_CLAMP, // 1
	  TD_HIGH_QUALITY // 4
	  );
}

void sdDeclAmbientCubeMap::SetSunParameters( const idVec3& sunDirection, const idVec3& sunColor ) {
	this->sunDirection = sunDirection;
	this->sunColor = sunColor;
}

