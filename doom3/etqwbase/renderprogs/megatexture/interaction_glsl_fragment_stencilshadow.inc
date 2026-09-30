$if !defined(STENCIL_SHADOW_INCLUDEAD)
$define STENCIL_SHADOW_INCLUDEAD

$ifdef _SOFT
$define ES3_STENCIL_SHADOW_SAMPLE_POISSON_DISK
        $define SAMPLES 16
        $define SAMPLE_MULTIPLICATOR (1.0 / 16.0)
        const vec2 sampleOffsetTable[SAMPLES] = vec2[SAMPLES](
            vec2( -0.94201624, -0.39906216 ),
            vec2( 0.94558609, -0.76890725 ),
            vec2( -0.094184101, -0.92938870 ),
            vec2( 0.34495938, 0.29387760 ),
            vec2( -0.91588581, 0.45771432 ),
            vec2( -0.81544232, -0.87912464 ),
            vec2( -0.38277543, 0.27676845 ),
            vec2( 0.97484398, 0.75648379 ),
            vec2( 0.44323325, -0.97511554 ),
            vec2( 0.53742981, -0.47373420 ),
            vec2( -0.26496911, -0.41893023 ),
            vec2( 0.79197514, 0.19090188 ),
            vec2( -0.24188840, 0.99706507 ),
            vec2( -0.81409955, 0.91437590 ),
            vec2( 0.19984126, 0.78641367 ),
            vec2( 0.14383161, -0.14100790 )
            // , vec2( 0.0, 0.0 )
        );

$define u_nonPowerOfTwo $u_uniformParm3
$define u_windowCoords $u_uniformParm4

ES3_STENCIL_SHADOW_SAMPLE_POISSON_DISK

highp float StencilShadow_CalcColor()
{
    vec2 screenTexCoord = gl_FragCoord.xy * u_windowCoords.xy;
    screenTexCoord = screenTexCoord * u_nonPowerOfTwo.xy;
    vec2 factor = u_nonPowerOfTwo.zw * $u_uniformParm1.x;
    float shadow = 0.0;
    for (int i = 0; i < SAMPLES; ++i) {
/*
    vec2 texSize = vec2(textureSize($u_fragmentIntMap6, 0));
    vec2 pixSize = vec2(1.0, 1.0) / texSize;
    vec2 baseTexCoord = gl_FragCoord.xy * pixSize;
    screenTexCoord = baseTexCoord;
*/
        vec2 stc = screenTexCoord + sampleOffsetTable[i] * factor;
        float t = float(texture($u_fragmentIntMap6, stc).r);
        float shadowColor = clamp(129.0 - t, $u_uniformParm0.x, 1.0);
        shadow += shadowColor;
    }
    const highp float sampleAvg = SAMPLE_MULTIPLICATOR;
    shadow *= sampleAvg;

	return shadow;
}
$endif // _SOFT

$else // STENCIL_SHADOW_INCLUDEAD

$ifdef _SOFT
    highp float shadow = StencilShadow_CalcColor();
    gl_FragColor *= shadow;
$endif // _SOFT

$ifdef _TRANSLUCENT
    gl_FragColor *= $u_uniformParm0.x;
$endif // _TRANSLUCENT

$endif // STENCIL_SHADOW_INCLUDEAD
