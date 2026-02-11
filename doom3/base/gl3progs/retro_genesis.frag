#version 300 es
//#pragma optimize(off)

precision mediump float;

#define RESOLUTION_DIVISOR 4.0
#define NUM_COLORS 64 // original 61

uniform sampler2D u_fragmentMap0; // currentRender

uniform highp vec4 u_windowCoords;
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_uniformParm0; // rpJitterTexScale

in vec2 var_TexCoord;
in lowp vec4 var_Color;
out vec4 _gl_FragColor;

// squared distance to avoid the sqrt of distance function
float ColorCompare( vec3 a, vec3 b )
{
    vec3 diff = b - a;
    return dot( diff, diff );
}

// find nearest palette color using Euclidean distance
vec3 LinearSearch( vec3 c, vec3 pal[NUM_COLORS] )
{
    int index = 0;
    float minDist = ColorCompare( c, pal[0] );

    for( int i = 1; i < NUM_COLORS; i++ )
    {
        float dist = ColorCompare( c, pal[i] );

        if( dist < minDist )
        {
            minDist = dist;
            index = i;
        }
    }

    return pal[index];
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
    // + very good dithering variety at dark grey values
    // + does not leak too much color into grey values
    // + good saturation when colors are really needed
    // - a bit too strong visible dithering pattern
    // https://lospec.com/palette-list/famicube
    /*const*/ vec3 palette[NUM_COLORS] = vec3[NUM_COLORS] (// 64
        RGB( 0, 0, 0 ),
        RGB( 21, 21, 21 ),
        RGB( 35, 23, 18 ),
        RGB( 23, 40, 8 ),
        RGB( 13, 32, 48 ),
        RGB( 33, 22, 64 ),
        RGB( 0, 78, 0 ),
        RGB( 79, 21, 7 ),
        RGB( 52, 52, 52 ),
        RGB( 92, 60, 13 ),
        RGB( 0, 96, 75 ),
        RGB( 55, 109, 3 ),
        RGB( 0, 23, 125 ),
        RGB( 0, 82, 128 ),
        RGB( 65, 93, 102 ),
        RGB( 135, 22, 70 ),
        RGB( 130, 60, 61 ),
        RGB( 19, 157, 8 ),
        RGB( 90, 25, 145 ),
        RGB( 61, 52, 165 ),
        RGB( 173, 78, 26 ),
        RGB( 32, 181, 98 ),
        RGB( 106, 180, 23 ),
        RGB( 147, 151, 23 ),
        RGB( 174, 108, 55 ),
        RGB( 123, 123, 123 ),
        RGB( 2, 74, 202 ),
        RGB( 10, 152, 172 ),
        RGB( 106, 49, 202 ),
        RGB( 88, 211, 50 ),
        RGB( 224, 60, 40 ),
        RGB( 207, 60, 113 ),
        RGB( 163, 40, 179 ),
        RGB( 204, 143, 21 ),
        RGB( 140, 214, 18 ),
        RGB( 113, 166, 161 ),
        RGB( 218, 101, 94 ),
        RGB( 98, 100, 220 ),
        RGB( 182, 193, 33 ),
        RGB( 197, 151, 130 ),
        RGB( 10, 137, 255 ),
        RGB( 246, 143, 55 ),
        RGB( 168, 168, 168 ),
        RGB( 225, 130, 137 ),
        RGB( 37, 226, 205 ),
        RGB( 91, 168, 255 ),
        RGB( 255, 187, 49 ),
        RGB( 190, 235, 113 ),
        RGB( 204, 105, 228 ),
        RGB( 166, 117, 254 ),
        RGB( 155, 160, 239 ),
        RGB( 245, 183, 132 ),
        RGB( 255, 231, 55 ),
        RGB( 255, 130, 206 ),
        RGB( 226, 215, 181 ),
        RGB( 213, 156, 252 ),
        RGB( 152, 220, 255 ),
        RGB( 215, 215, 215 ),
        RGB( 189, 255, 202 ),
        RGB( 238, 255, 169 ),
        RGB( 226, 201, 255 ),
        RGB( 255, 233, 197 ),
        RGB( 254, 201, 237 ),
        RGB( 255, 255, 255 )
    );

    const vec3 medianAbsoluteDeviation = RGB( 63, 175, 2 );
    const vec3 deviation = RGB( 76, 62, 75 );

    vec2 uv = ( var_TexCoord );
    vec2 uvPixelated = floor( gl_FragCoord.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

    vec3 quantizationPeriod = vec3( 1.0 / float(NUM_COLORS) );
    vec3 quantDeviation = deviation;

    // get pixellated base color
    vec3 color = texture( u_fragmentMap0, uvPixelated * u_windowCoords.xy * u_nonPowerOfTwo.xy ).rgb;

    vec2 uvDither = uvPixelated;
    //if( u_uniformParm0.x > 1.0 )
    {
        uvDither = gl_FragCoord.xy / ( RESOLUTION_DIVISOR / u_uniformParm0.x );
    }
    float dither = DitherArray8x8( uvDither ) - 0.5;

    color.rgb += vec3( dither, dither, dither ) * quantDeviation * u_uniformParm0.y;

    // find closest color match from C64 color palette
    color = LinearSearch( color.rgb, palette );

    //color = texture( u_fragmentMap0, uv ).rgb;
    _gl_FragColor = vec4( color, 1.0 );
}
