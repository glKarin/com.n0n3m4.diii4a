#version 300 es
//#pragma optimize(off)

precision mediump float;

#define RESOLUTION_DIVISOR                4.0
#define Dithering_QuantizationSteps        32.0 // 8.0 = 2 ^ 3 quantization bits

uniform sampler2D u_fragmentMap0; // currentRender

uniform highp vec4 u_windowCoords;
uniform highp vec4 u_nonPowerOfTwo;

in vec2 var_TexCoord;
in lowp vec4 var_Color;
out vec4 _gl_FragColor;

vec3 Quantize( vec3 color, vec3 period )
{
    return floor( color * Dithering_QuantizationSteps ) * ( 1.0 / ( Dithering_QuantizationSteps - 1.0 ) );
}

#define RGB(r, g, b) vec3(float(r)/255.0, float(g)/255.0, float(b)/255.0)

// array/table version from http://www.anisopteragames.com/how-to-fix-color-banding-with-dithering/
const int ArrayDitherArray8x8[64] = int[64] (
    0, 32,  8, 40,  2, 34, 10, 42,   /* 8x8 Bayer ordered dithering  */
    48, 16, 56, 24, 50, 18, 58, 26,  /* pattern.  Each input pixel   */
    12, 44,  4, 36, 14, 46,  6, 38,  /* is scaled to the 0..63 range */
    60, 28, 52, 20, 62, 30, 54, 22,  /* before looking in this table */
    3, 35, 11, 43,  1, 33,  9, 41,   /* to determine the action.     */
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  5, 37,
    63, 31, 55, 23, 61, 29, 53, 21
);

float DitherArray8x8( vec2 pos )
{
    int stippleOffset = ( int( pos.y ) % 8 ) * 8 + ( int( pos.x ) % 8 );
    int byte = ArrayDitherArray8x8[stippleOffset];
    float stippleThreshold = float(byte) / 64.0f;
    return stippleThreshold;
}

void main(void)
{
    vec2 uv = ( var_TexCoord );
    vec2 uvPixelated = floor( gl_FragCoord.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

    // most Sony Playstation 1 titles used 5 bit per RGB channel
    // 2^5 = 32
    // 32 * 32 * 32 = 32768 colors

    const float quantizationSteps = Dithering_QuantizationSteps;
    vec3 quantizationPeriod = vec3( 1.0 / ( quantizationSteps - 1.0 ) );

    // get pixellated base color
    vec3 color = texture( u_fragmentMap0, uvPixelated * u_windowCoords.xy * u_nonPowerOfTwo.xy ).rgb;

    // add Bayer 8x8 dithering
    vec2 uvDither = gl_FragCoord.xy / RESOLUTION_DIVISOR;

    float dither = DitherArray8x8( uvDither ) - 0.5;

    color.rgb += vec3( dither, dither, dither ) * quantizationPeriod;

    // PSX color quantization with 15-bit
    color = Quantize( color, quantizationPeriod );

    //color = texture( u_fragmentMap0, uv ).rgb;
    _gl_FragColor = vec4( color, 1.0 );
}
