$if !defined(SHADOW_MAPPING_INCLUDEAD)
$define SHADOW_MAPPING_INCLUDEAD

$define SHADOW_ALPHA $u_uniformParm0.x
$define SHADOW_MAP_SIZE $u_uniformParm1.x
$define SHADOW_MAP_SIZE_MULTIPLICATOR $u_uniformParm1.y
$define JITTER_SCALE $u_uniformParm1.z
$define SCREEN_SIZE_MULTIPLICATOR $u_uniformParm2.xy

$ifdef _GLES2
varying highp vec3 var_LightToVertex;
$else
varying highp vec3 var_VertexToLight;
$endif
varying highp vec4 var_ViewPosition;
varying highp vec4 var_VertexPosition;

$define u_lightType $u_uniformIntParm7.x
$define var_ShadowCoord var_VertexPosition
$define shadowMVPMatrix $u_uniformMatrixArrayParm6

$define BIAS(x) (x)

$define LT_PARALLEL_LIGHT 1
$define LT_POINT_LIGHT 2
$define LT_SPOT_LIGHT 3
$define _POINT_LIGHT (u_lightType == LT_POINT_LIGHT)
$define _PARALLEL_LIGHT (u_lightType == LT_PARALLEL_LIGHT)
$define _SPOT_LIGHT (u_lightType == LT_SPOT_LIGHT)



highp vec2 ShadowMapping_RandomizeJitter()
{
    // highp float random = (gl_FragCoord.z + shadowPosition.z) * 0.5;
    highp float random = texture2D( $u_fragmentMap7, gl_FragCoord.xy * SCREEN_SIZE_MULTIPLICATOR ).r;
    random *= 3.141592653589793;
    highp vec2 rot;
    rot.x = cos( random );
    rot.y = sin( random );
    return rot;
}



$ifdef _GLES2

bool plane_ray_intersect(highp vec3 plane_point, highp vec3 plane_normal, highp vec3 dir, out highp vec3 ret) {
	highp float dotProduct = dot(dir, plane_normal);
	highp float dotProductFactor = 1.0 / dotProduct;
	// if(dotProduct == 0.0) return false;
	highp float l2 = dot(plane_normal, plane_point) * dotProductFactor;
	// if (l2 < 0.0) return false;
	highp float distance = dot(plane_normal, plane_point);
	highp float t = - (dot(plane_normal, dir) - distance) * dotProductFactor;
	ret = dir * t;
	return true;
}

$ifdef r_usePackColorAsDepth
vec4 pack (highp float depth)
{
    const highp vec4 bitSh = vec4(256.0 * 256.0 * 256.0, 256.0 * 256.0, 256.0, 1.0);
    const highp vec4 bitMsk = vec4(0.0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0);
    highp vec4 comp = fract(depth * bitSh);
    comp -= comp.xxyz * bitMsk;
    return depth < 1.0 ? comp : vec4(1.0, 1.0, 1.0, 1.0);
}

highp float unpack (vec4 colour)
{
    const highp vec4 bitShifts = vec4(1.0 / (256.0 * 256.0 * 256.0), 1.0 / (256.0 * 256.0), 1.0 / 256.0, 1.0);
    highp float dotc = dot(colour , bitShifts);
    return /*all(lessThan(colour, vec4(1.0, 1.0, 1.0, 1.0)))*/ colour.r < 1.0 ? dotc : 1.0;
}
$define DC(x) (unpack(x))
$else
$define DC(x) ((x).r)
$endif

int ShadowMapping_GetCubeAxis()
{
    highp vec3 toLightGlobal = normalize( -var_LightToVertex );
    int shadowIndex = 0;
    highp float axis[6];
    axis[0] = -toLightGlobal.x;
    axis[1] =  toLightGlobal.x;
    axis[2] = -toLightGlobal.y;
    axis[3] =  toLightGlobal.y;
    axis[4] = -toLightGlobal.z;
    axis[5] =  toLightGlobal.z;
    for( int i = 0; i < 6; ++i ) {
        //if( axis[i] > axis[shadowIndex] ) {        shadowIndex = i;    }
        shadowIndex = axis[i] > axis[shadowIndex] ? i : shadowIndex;
    }
	return shadowIndex;
}

highp vec3 ShadowMapping_GetCubeSampleDir(int shadowIndex)
{
    highp vec3 plane_normal;
    highp vec3 plane_point;
    highp float radius = SHADOW_MAP_SIZE * 0.5;
    if(shadowIndex == 0) { // +X
        plane_point = vec3(radius, 0.0, 0.0);
        plane_normal = vec3(-1.0, 0.0, 0.0);
    } else if(shadowIndex == 1) { // -X
        plane_point = vec3(-radius, 0.0, 0.0);
        plane_normal = vec3(1.0, 0.0, 0.0);
    } else if(shadowIndex == 2) { // +Y
        plane_point = vec3(0.0, radius, 0.0);
        plane_normal = vec3(0.0, -1.0, 0.0);
    } else if(shadowIndex == 3) { // -Y
        plane_point = vec3(0.0, -radius, 0.0);
        plane_normal = vec3(0.0, 1.0, 0.0);
    } else if(shadowIndex == 4) { // +Z
        plane_point = vec3(0.0, 0.0, radius);
        plane_normal = vec3(0.0, 0.0, -1.0);
    } else { // -Z
        plane_point = vec3(0.0, 0.0, -radius);
        plane_normal = vec3(0.0, 0.0, 1.0);
    }
    highp vec3 texcoordCube;
    plane_ray_intersect(plane_point, plane_normal, normalize(var_LightToVertex), texcoordCube);
    return texcoordCube;
}

highp float ShadowMapping_RandomizeJitter2D(highp vec4 shadowPosition, highp vec2 rot)
{
    $define SAMPLES_2D 12
    $define SAMPLE_MULTIPLICATOR_2D (1.0 / 12.0)
    vec2 sampleOffsetTable[SAMPLES_2D];
    sampleOffsetTable[0] = vec2( 0.6111618, 0.1050905 );
    sampleOffsetTable[1] = vec2( 0.1088336, 0.1127091 );
    sampleOffsetTable[2] = vec2( 0.3030421, -0.6292974 );
    sampleOffsetTable[3] = vec2( 0.4090526, 0.6716492 );
    sampleOffsetTable[4] = vec2( -0.1608387, -0.3867823 );
    sampleOffsetTable[5] = vec2( 0.7685862, -0.6118501 );
    sampleOffsetTable[6] = vec2( -0.1935026, -0.856501 );
    sampleOffsetTable[7] = vec2( -0.4028573, 0.07754025 );
    sampleOffsetTable[8] = vec2( -0.6411021, -0.4748057 );
    sampleOffsetTable[9] = vec2( -0.1314865, 0.8404058 );
    sampleOffsetTable[10] = vec2( -0.7005203, 0.4596822 );
    sampleOffsetTable[11] = vec2( -0.9713828, -0.06329931 );
    // sampleOffsetTable[12] = vec2( 0.0, 0.0 );

    highp float shadow = 0.0;
    highp float distance = JITTER_SCALE * SHADOW_MAP_SIZE_MULTIPLICATOR;
    highp float currentDepth = BIAS(shadowPosition.z);
    for (int i = 0; i < SAMPLES_2D; ++i) {
        highp vec2 jitter = sampleOffsetTable[i];
        highp vec2 jitterRotated;
        jitterRotated.x = jitter.x * rot.x - jitter.y * rot.y;
        jitterRotated.y = jitter.x * rot.y + jitter.y * rot.x;
        highp float shadowDepth = DC(texture2D($u_fragmentMap5, shadowPosition.st + jitterRotated * distance));
        highp float visibility = currentDepth - shadowDepth;
        shadow += 1.0 - step(0.0, visibility) * SHADOW_ALPHA;
        //shadow += visibility < 0.0 ? 1.0 : SHADOW_ALPHA;
    }
    const highp float sampleAvg = SAMPLE_MULTIPLICATOR_2D;
    shadow *= sampleAvg;
    return shadow;
}

highp float ShadowMapping_RandomizeJitterCube(highp vec4 shadowPosition, highp vec3 texcoordCube)
{
    $define SAMPLES 20
    $define SAMPLE_MULTIPLICATOR (1.0 / 20.0)
    vec3 sampleOffsetTable[SAMPLES];
    sampleOffsetTable[0] = vec3(1.0, 1.0, 1.0); sampleOffsetTable[1] = vec3(1.0, -1.0, 1.0); sampleOffsetTable[2] = vec3(-1.0, -1.0, 1.0); sampleOffsetTable[3] = vec3(-1.0, 1.0, 1.0);
    sampleOffsetTable[4] = vec3(1.0, 1.0, -1.0); sampleOffsetTable[5] = vec3(1.0, -1.0, -1.0); sampleOffsetTable[6] = vec3(-1.0, -1.0, -1.0); sampleOffsetTable[7] = vec3(-1.0, 1.0, -1.0);
    sampleOffsetTable[8] = vec3(1.0, 1.0, 0.0); sampleOffsetTable[9] = vec3(1.0, -1.0, 0.0); sampleOffsetTable[10] = vec3(-1.0, -1.0, 0.0); sampleOffsetTable[11] = vec3(-1.0, 1.0, 0.0);
    sampleOffsetTable[12] = vec3(1.0, 0.0, 1.0); sampleOffsetTable[13] = vec3(-1.0, 0.0, 1.0); sampleOffsetTable[14] = vec3(1.0, 0.0, -1.0); sampleOffsetTable[15] = vec3(-1.0, 0.0, -1.0);
    sampleOffsetTable[16] = vec3(0.0, 1.0, 1.0); sampleOffsetTable[17] = vec3(0.0, -1.0, 1.0); sampleOffsetTable[18] = vec3(0.0, -1.0, -1.0); sampleOffsetTable[19] = vec3(0.0, 1.0, -1.0);
    // sampleOffsetTable[20] = vec3(0.0, 0.0, 0.0);

    highp float shadow = 0.0;
    highp float currentDepth = shadowPosition.z;
    highp float distance = JITTER_SCALE * 0.5;
    for (int i = 0; i < SAMPLES; ++i) {
        highp vec3 jitter = sampleOffsetTable[i];
        highp float shadowDepth = DC(textureCube($u_fragmentCubeMap6, normalize(texcoordCube + jitter * distance)));
        highp float visibility = currentDepth - shadowDepth;
        shadow += 1.0 - step(0.0, visibility) * SHADOW_ALPHA;
        //shadow += visibility < 0.0 ? 1.0 : SHADOW_ALPHA;
    }
    const highp float sampleAvg = SAMPLE_MULTIPLICATOR;
    shadow *= sampleAvg;
    return shadow;
}

$else

int ShadowMapping_GetAxis()
{
    int shadowIndex = 0;
    highp vec3 toLightGlobal = normalize( var_VertexToLight );
    highp float axis[6] = float[6](
        -toLightGlobal.x,
        toLightGlobal.x,
        -toLightGlobal.y,
        toLightGlobal.y,
        -toLightGlobal.z,
        toLightGlobal.z
    );
    for( int i = 0; i < 6; ++i ) {
        //if( axis[i] > axis[shadowIndex] ) {        shadowIndex = i;    }
        shadowIndex = axis[i] > axis[shadowIndex] ? i : shadowIndex;
    }
    return shadowIndex;
}

int ShadowMapping_GetCascadeFrustum()
{
    int shadowIndex = 4;
    highp float viewZ = -var_ViewPosition.z;
    for( int i = 0; i < 4; ++i ) {
        shadowIndex = shadowIndex == 4 && viewZ < $u_uniformParm3[i] ? i : shadowIndex;
        //if( viewZ < u_uniformParm3[i] ) {            shadowIndex = i;            break;        }
    }
    return shadowIndex;
}

highp vec4 ShadowMapping_GetPosition()
{
	highp vec4 shadowPosition;
    if (_POINT_LIGHT)
    {
        int shadowIndex = ShadowMapping_GetAxis();
        shadowPosition = var_VertexPosition * shadowMVPMatrix[shadowIndex];
        shadowPosition.xyz /= shadowPosition.w;
        shadowPosition.w = float(shadowIndex);
    }
    else if (_PARALLEL_LIGHT)
    {
        int shadowIndex = ShadowMapping_GetCascadeFrustum();
        shadowPosition = var_VertexPosition * shadowMVPMatrix[shadowIndex];
        shadowPosition.xyz /= shadowPosition.w;
        shadowPosition.w = float(shadowIndex);
    }
    else if (_SPOT_LIGHT)
    {
        shadowPosition = vec4(var_ShadowCoord.xyz / var_ShadowCoord.w, 0.0);
    }
    shadowPosition.z = BIAS(shadowPosition.z);
	return shadowPosition;
}

highp float ShadowMapping_RandomizeJitter(highp vec4 shadowPosition, highp vec2 rot)
{
    $define SAMPLES 12
    $define SAMPLE_MULTIPLICATOR (1.0 / 12.0)
    const vec2 sampleOffsetTable[SAMPLES] = vec2[SAMPLES](
        vec2( 0.6111618, 0.1050905 ),
        vec2( 0.1088336, 0.1127091 ),
        vec2( 0.3030421, -0.6292974 ),
        vec2( 0.4090526, 0.6716492 ),
        vec2( -0.1608387, -0.3867823 ),
        vec2( 0.7685862, -0.6118501 ),
        vec2( -0.1935026, -0.856501 ),
        vec2( -0.4028573, 0.07754025 ),
        vec2( -0.6411021, -0.4748057 ),
        vec2( -0.1314865, 0.8404058 ),
        vec2( -0.7005203, 0.4596822 ),
        vec2( -0.9713828, -0.06329931 )
        // , vec2( 0.0, 0.0 )
    );

    highp float distance = JITTER_SCALE * SHADOW_MAP_SIZE_MULTIPLICATOR;
    highp float shadow = 0.0;
    for (int i = 0; i < SAMPLES; ++i) {
        highp vec2 jitter = sampleOffsetTable[i];
        highp vec2 jitterRotated;
        jitterRotated.x = jitter.x * rot.x - jitter.y * rot.y;
        jitterRotated.y = jitter.x * rot.y + jitter.y * rot.x;
        highp float shadowDepth = texture2D($u_fragment2DArrayShadowMap6, vec4(shadowPosition.st + jitterRotated * distance, shadowPosition.wz));
        shadow += 1.0 - (1.0 - shadowDepth) * SHADOW_ALPHA; //shadow += shadowDepth > 0.0 ? 1.0 : SHADOW_ALPHA;
    }
    const highp float sampleAvg = SAMPLE_MULTIPLICATOR;
    shadow *= sampleAvg;
    return shadow;
}

$endif

$else // SHADOW_MAPPING_INCLUDEAD

$ifdef _GLES2

    highp float shadow;
	highp vec4 shadowPosition;
    
    if (_POINT_LIGHT)
    {
        int shadowIndex = ShadowMapping_GetCubeAxis();
        shadowPosition = var_VertexPosition * shadowMVPMatrix[shadowIndex];
        shadowPosition.xyz /= shadowPosition.w;
        shadowPosition.z = BIAS(shadowPosition.z);
        highp vec3 texcoordCube = ShadowMapping_GetCubeSampleDir(shadowIndex);
        shadow = ShadowMapping_RandomizeJitterCube(shadowPosition, texcoordCube);
    }
    else if (_PARALLEL_LIGHT)
    {
        shadowPosition = vec4(var_ShadowCoord.xyz / var_ShadowCoord.w, 0.0);
        shadowPosition.z = BIAS(shadowPosition.z);
        highp vec2 rot = ShadowMapping_RandomizeJitter();
        shadow = ShadowMapping_RandomizeJitter2D(shadowPosition, rot);
    }
    else if (_SPOT_LIGHT)
    {
        shadowPosition = vec4(var_ShadowCoord.xyz / var_ShadowCoord.w, 0.0);
        shadowPosition.z = BIAS(shadowPosition.z);
        highp vec2 rot = ShadowMapping_RandomizeJitter();
        shadow = ShadowMapping_RandomizeJitter2D(shadowPosition, rot);
    }

    gl_FragColor *= shadow;

$else

    highp vec4 shadowPosition = ShadowMapping_GetPosition();

    highp vec2 rot = ShadowMapping_RandomizeJitter();
    highp float shadow = ShadowMapping_RandomizeJitter(shadowPosition, rot);

    gl_FragColor *= shadow;

$endif

$endif // SHADOW_MAPPING_INCLUDEAD
