// When idDrawVert changes you need to update this and the bind attribute bits in DeclRenderProg.cpp
#ifdef ID_VERTEX_SHADER
in vec4		attr_Position;
in vec4		attr_TexCoord0;
in vec3		attr_Tangent;
in vec3		attr_Bitangent;
in vec3     attr_Normal;
in vec4     attr_Color;
in vec4     attr_Color2;
#endif

#ifdef SKINNED
	uniform matrices_ubo { vec4 matrices[408]; };
#endif

#ifdef ID_VERTEX_SHADER
out vec4	vertex_texcoord0;
out vec4	vertex_texcoord1;
out vec4	vertex_texcoord2;
out vec4	vertex_texcoord3;
flat out vec4 vertex_texcoord3_flat;
out vec4	vertex_texcoord4;
out vec4	vertex_texcoord5;
flat out vec4 vertex_texcoord5_flat;
out vec4	vertex_texcoord6;
out vec4	vertex_texcoord7;
#endif

#ifdef ID_PIXEL_SHADER
in vec4		vertex_texcoord0;
in vec4		vertex_texcoord1;
in vec4		vertex_texcoord2;
flat in vec4     vertex_texcoord3_flat;
in vec4		vertex_texcoord3;
in vec4		vertex_texcoord4;
in vec4		vertex_texcoord5;
flat in vec4 vertex_texcoord5_flat;
in vec4		vertex_texcoord6;
in vec4		vertex_texcoord7;

out vec4	finalpixel_color;
#if 1 //k
out vec4	finalpixel_color2;
out vec4	finalpixel_color3;
out vec4	finalpixel_color4;
out vec4	finalpixel_color5;
out vec4	finalpixel_color6;
#endif
#endif

float dot2( vec2 a, vec2 b ) { return dot( a, b ); }
float dot3( vec3 a, vec3 b ) { return dot( a, b ); }
float dot3( vec3 a, vec4 b ) { return dot( a, b.xyz ); }
float dot3( vec4 a, vec3 b ) { return dot( a.xyz, b ); }
float dot3( vec4 a, vec4 b ) { return dot( a.xyz, b.xyz ); }
float dot4( vec4 a, vec4 b ) { return dot( a, b ); }
float dot4( vec2 a, vec4 b ) { return dot( vec4( a, 0.0, 1.0 ), b ); }

vec4 tex2D( sampler2D samp, vec2 texCoords ) { return texture(samp, texCoords); }
vec4 idtex2Dproj( sampler2D samp, vec4 texCoords ) { return textureProj( samp, texCoords.xyw ); }

float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }
vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }
vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }
vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }

vec3 absv3(vec3 a)
{
	return vec3(
		abs(a.x),
		abs(a.y),
		abs(a.z)
	);
}

// https://www.gamedev.net/forums/topic/687535-implementing-a-cube-map-lookup-function/
vec3 ComputeSampleCube( const vec3 v)
{
	vec3 vAbs = absv3(v);
	float ma;
	vec2 uv;
	float faceIndex;
	
	if(vAbs.z >= vAbs.x && vAbs.z >= vAbs.y)
	{
		faceIndex = v.z < 0.0 ? 5.0 : 4.0;
		ma = 0.5 / vAbs.z;
		uv = vec2(v.z < 0.0 ? -v.x : v.x, -v.y);
	}
	else if(vAbs.y >= vAbs.x)
	{
		faceIndex = v.y < 0.0 ? 3.0 : 2.0;
		ma = 0.5 / vAbs.y;
		uv = vec2(v.x, v.y < 0.0 ? -v.z : v.z);
	}
	else
	{
		faceIndex = v.x < 0.0 ? 1.0 : 0.0;
		ma = 0.5 / vAbs.x;
		uv = vec2(v.x < 0.0 ? v.z : -v.z, -v.y);
	}
	return vec3(uv * ma + 0.5, faceIndex);
}

float SampleShadowMapPCF(sampler2D shadowMapAtlas, vec2 uv, float scenedepth, vec4 rpShadowMapInfo) {
	float shadowMapSliceSize = 1.0 / rpShadowMapInfo.z;
	float factor = 0.0;
	float numSamples = 0.0;
    for (int y = -1 ; y <= 1 ; y++) {
        for (int x = -1 ; x <= 1 ; x++) {
			float shadowdepth = tex2D(shadowMapAtlas, uv + vec2(float(x) * shadowMapSliceSize, float(y) * shadowMapSliceSize)).r * 3000.0;
			if(scenedepth <= shadowdepth)
				factor += 1.0;
			
			numSamples++;
		}	
	}
	
	return factor / numSamples;
}

#ifdef ID_SPOTLIGHT
vec2 ShowSpotLightCoords(vec3 vertexPos, vec3 lightPos) {
	vec4 shadowTexCoords;
	vec4 _vertexPos = vec4(vertexPos.x, vertexPos.y, vertexPos.z, 1.0);

	shadowTexCoords.x = dot4(_vertexPos, rpShadowMatrixX);	
	shadowTexCoords.y = dot4(_vertexPos, rpShadowMatrixY);
	shadowTexCoords.z = dot4(_vertexPos, rpShadowMatrixZ);
	shadowTexCoords.w = dot4(_vertexPos, rpShadowMatrixW);

	vec2 initialuv = shadowTexCoords.xy / shadowTexCoords.w;

	return initialuv;
}
#endif

float CalculateShadows(sampler2D atlasLookup, sampler2D shadowMapAtlas, vec3 vertexPos, vec3 lightPos, vec4 ShadowMapInfo, float lightType) {
	vec3 vertToLight = vertexPos - lightPos;
	vec2 initialuv;
	float slice;

	if(lightType == 2.0)
	{
// jmarshall - I don't like the way I did this in IcedTech requires sending too much data to the GPU. Figure out another way to do this!
#if 0
		vec4 shadowTexCoords;
		vec4 _vertexPos = vec4(vertexPos.x, vertexPos.y, vertexPos.z, 1.0);

		shadowTexCoords.x = dot4(_vertexPos, shadowMatrixX);	
		shadowTexCoords.y = dot4(_vertexPos, shadowMatrixY);
		shadowTexCoords.z = dot4(_vertexPos, shadowMatrixZ);
		shadowTexCoords.w = dot4(_vertexPos, shadowMatrixW);

		vec2 initialuv = shadowTexCoords.xy / shadowTexCoords.w;
		initialuv = initialuv * 0.5 + 0.5;
		slice = 0;
#endif
		return 1.0;
// jmarshall end
	}
	else if(lightType == 1.0)
	{
		vec3 cubeInfo = ComputeSampleCube(vertToLight);
		initialuv = cubeInfo.xy;
		slice = cubeInfo.z;
	}

	initialuv *= ShadowMapInfo.y;
	
	// Get the slice information(aka were in the atlas are we going to sample from).
	vec2 sliceOffset = texelFetch(atlasLookup, ivec2(ShadowMapInfo.x + slice, 0), 0).xy;
	vec2 uv = initialuv + sliceOffset;
	
	float scenedepth = distance(vertexPos, lightPos);
	return SampleShadowMapPCF(shadowMapAtlas, uv, scenedepth, ShadowMapInfo);
}

float AttenuationSpotLight( float lightDist, float maxLightDist, float falloffPower ) {
	float attenuation = 1.0f - saturate( lightDist / maxLightDist );
	falloffPower = max( falloffPower, 0.0f );
	return pow( attenuation, falloffPower );
}

float AttenuationPointLight( vec3 fragmentInWorldSpace, vec4 curLightPosInWorldSpace, vec4 curLightExtents ) {
	float falloffPower = curLightPosInWorldSpace.w;
	vec3 perAxis = 1.0f - saturate( absv3( fragmentInWorldSpace - curLightPosInWorldSpace.xyz ) / curLightExtents.xyz );			
	float attenuation = pow( perAxis.x * perAxis.y * perAxis.z, falloffPower );
	
	return attenuation;
}

#ifdef ID_VERTEX_SHADER
vec4 TransformVertex(vec4 mvpX, vec4 mvpY, vec4 mvpZ, vec4 mvpW)
{
	vec4 position;

	position.x = dot4( attr_Position, mvpX );
	position.y = dot4( attr_Position, mvpY );
	position.z = dot4( attr_Position, mvpZ );
	position.w = dot4( attr_Position, mvpW );

	return position;
}

vec4 TransformInputVertex(vec4 xyz, vec4 mvpX, vec4 mvpY, vec4 mvpZ, vec4 mvpW)
{
	vec4 position;

	position.x = dot4( xyz, mvpX );
	position.y = dot4( xyz, mvpY );
	position.z = dot4( xyz, mvpZ );
	position.w = dot4( xyz, mvpW );

	return position;
}
#endif

float Min3(float x, float y, float z)
{
    return min(x, min(y, z));
}

float Max3(float x, float y, float z)
{
    return max(x, max(y, z));
}


float rcp(float v)
{
	return 1.0 / v;
}

float rand(float co) { 
	return fract(sin(co*(91.3458)) * 47453.5453); 
}

vec2 randnoise(vec2 coord, float width, float height, float noiseamount) //generating noise/pattern texture for dithering
{
  float noiseX = ((fract(1.0-coord.s*(width/2.0))*0.25)+(fract(coord.t*(height/2.0))*0.75))*2.0-1.0;
  float noiseY = ((fract(1.0-coord.s*(width/2.0))*0.75)+(fract(coord.t*(height/2.0))*0.25))*2.0-1.0;

  {
    noiseX = clamp(fract(sin(dot(coord ,vec2(12.9898,78.233))) * 43758.5453),0.0,1.0)*2.0-1.0;
    noiseY = clamp(fract(sin(dot(coord ,vec2(12.9898,78.233)*2.0)) * 43758.5453),0.0,1.0)*2.0-1.0;
  }
  return vec2(noiseX,noiseY)*noiseamount;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	float PI = 3.14159265359;

	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec2 CenterScale( vec2 inTC, vec2 centerScale ) {
	float scaleX = centerScale.x;
	float scaleY = centerScale.y;
	vec4 tc0 = vec4( scaleX, 0, 0, 0.5 - ( 0.5f * scaleX ) );
	vec4 tc1 = vec4( 0, scaleY, 0, 0.5 - ( 0.5f * scaleY ) );

	vec2 finalTC;
	finalTC.x = dot4( inTC, tc0 );
	finalTC.y = dot4( inTC, tc1 );
	return finalTC;
}

vec2 Rotate2D( vec2 inTC, vec2 cs ) {
	float sinValue = cs.y;
	float cosValue = cs.x;

	vec4 tc0 = vec4( cosValue, -sinValue, 0, ( -0.5f * cosValue ) + ( 0.5f * sinValue ) + 0.5f );
	vec4 tc1 = vec4( sinValue, cosValue, 0, ( -0.5f * sinValue ) + ( -0.5f * cosValue ) + 0.5f );

	vec2 finalTC;
	finalTC.x = dot4( inTC, tc0 );
	finalTC.y = dot4( inTC, tc1 );
	return finalTC;
}

vec3 CalcSpecPBR(vec3 cameraVector, vec3 Cd, float metallic, float roughness, vec3 H, vec3 N, vec3 L)
{
	vec3 V = normalize(cameraVector);

	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, Cd.xyz, vec3(metallic, metallic, metallic));
			
	// cook-torrance brdf
	float NDF = DistributionGGX(N, H, roughness);        
	float G   = GeometrySmith(N, V, L, roughness);      
	vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
			
	vec3 kS = F;
	vec3 kD = vec3(1.0, 1.0, 1.0) - kS;
	kD *= 1.0 - vec3(metallic, metallic, metallic);	  
			
	vec3 numerator    = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	vec3 pbr     = numerator / max(denominator, 0.001); 

	return pbr;
}

vec4 blur13(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
  vec4 color = vec4(0.0);
  vec2 off1 = vec2(1.411764705882353) * direction;
  vec2 off2 = vec2(3.2941176470588234) * direction;
  vec2 off3 = vec2(5.176470588235294) * direction;
  color += texture(image, uv) * 0.1964825501511404;
  color += texture(image, uv + (off1 / resolution)) * 0.2969069646728344;
  color += texture(image, uv - (off1 / resolution)) * 0.2969069646728344;
  color += texture(image, uv + (off2 / resolution)) * 0.09447039785044732;
  color += texture(image, uv - (off2 / resolution)) * 0.09447039785044732;
  color += texture(image, uv + (off3 / resolution)) * 0.010381362401148057;
  color += texture(image, uv - (off3 / resolution)) * 0.010381362401148057;
  return color;
}

float LinearFog(float d, float FogMin, float FogMax)
{
    if (d>=FogMax) return 1.0;
    if (d<=FogMin) return 0.0;

    return 1.0 - (FogMax - d) / (FogMax - FogMin);
}
