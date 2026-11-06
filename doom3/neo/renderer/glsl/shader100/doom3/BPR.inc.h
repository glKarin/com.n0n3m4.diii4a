
#define _PBR_GENERAL_FUNCTION \
"float dot2_4(vec2 a, vec4 b) {\n" \
"    return dot(vec4(a, 0.0, 0.0), b);\n" \
"}\n" \
"\n" \
"vec2 CenterScale( vec2 inTC, vec2 centerScale ) {\n" \
"    float scaleX = centerScale.x;\n" \
"    float scaleY = centerScale.y;\n" \
"    vec4 tc0 = vec4( scaleX, 0.0, 0.0, 0.5 - ( 0.5 * scaleX ) );\n" \
"    vec4 tc1 = vec4( 0.0, scaleY, 0.0, 0.5 - ( 0.5 * scaleY ) );\n" \
"    vec2 finalTC;\n" \
"    finalTC.x = dot2_4( inTC, tc0 );\n" \
"    finalTC.y = dot2_4( inTC, tc1 );\n" \
"    return finalTC;\n" \
"}\n" \
"\n" \
"vec2 Rotate2D( vec2 inTC, vec2 cs ) {\n" \
"    float sinValue = cs.y;\n" \
"    float cosValue = cs.x;\n" \
"\n" \
"    vec4 tc0 = vec4( cosValue, -sinValue, 0.0, ( -0.5 * cosValue ) + ( 0.5 * sinValue ) + 0.5 );\n" \
"    vec4 tc1 = vec4( sinValue, cosValue, 0.0, ( -0.5 * sinValue ) + ( -0.5 * cosValue ) + 0.5 );\n" \
"    vec2 finalTC;\n" \
"    finalTC.x = dot2_4( inTC, tc0 );\n" \
"    finalTC.y = dot2_4( inTC, tc1 );\n" \
"    return finalTC;\n" \
"}\n" \
"\n" \
"// better noise function available at https://github.com/ashima/webgl-noise\n" \
"float rand( vec2 co ) {\n" \
"    return fract( sin( dot( co.xy, vec2( 12.9898, 78.233 ) ) ) * 43758.5453 );\n" \
"}\n" \
"\n" \
"float DistributionGGX(vec3 N, vec3 H, float roughness)\n" \
"{\n" \
"    float a      = roughness*roughness;\n" \
"    float a2     = a*a;\n" \
"    float NdotH  = max(dot(N, H), 0.0);\n" \
"    float NdotH2 = NdotH*NdotH;\n" \
"    float PI = 3.14159265359;\n" \
"    float num   = a2;\n" \
"    float denom = (NdotH2 * (a2 - 1.0) + 1.0);\n" \
"    denom = PI * denom * denom;\n" \
"    return num / denom;\n" \
"}\n" \
"\n" \
"float GeometrySchlickGGX(float NdotV, float roughness)\n" \
"{\n" \
"    float r = (roughness + 1.0);\n" \
"    float k = (r*r) / 8.0;\n" \
"    float num   = NdotV;\n" \
"    float denom = NdotV * (1.0 - k) + k;\n" \
"    return num / denom;\n" \
"}\n" \
"float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)\n" \
"{\n" \
"    float NdotV = max(dot(N, V), 0.0);\n" \
"    float NdotL = max(dot(N, L), 0.0);\n" \
"    float ggx2  = GeometrySchlickGGX(NdotV, roughness);\n" \
"    float ggx1  = GeometrySchlickGGX(NdotL, roughness);\n" \
"    return ggx1 * ggx2;\n" \
"}\n" \
"\n" \
"vec3 fresnelSchlick(float cosTheta, vec3 F0)\n" \
"{\n" \
"    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);\n" \
"}\n" \
"\n"

