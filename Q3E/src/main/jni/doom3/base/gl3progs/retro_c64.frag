#version 300 es
//#pragma optimize(off)

precision mediump float;

#define RESOLUTION_DIVISOR 4.0
#define NUM_COLORS 16

uniform sampler2D u_fragmentMap0; // currentRender

uniform highp vec4 u_windowCoords;
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_uniformParm0; // rpJitterTexScale

in vec2 var_TexCoord;
in lowp vec4 var_Color;
out vec4 _gl_FragColor;

vec3 Average( vec3 pal[NUM_COLORS] )
{
    vec3 sum = vec3( 0.0 );

    for( int i = 0; i < NUM_COLORS; i++ )
    {
        sum += pal[i];
    }

    return sum / float( NUM_COLORS );
}

vec3 Deviation( vec3 pal[NUM_COLORS] )
{
    vec3 sum = vec3( 0.0 );
    vec3 avg = Average( pal );

    for( int i = 0; i < NUM_COLORS; i++ )
    {
        sum += abs( pal[i] - avg );
    }

    return sum / float( NUM_COLORS );
}

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
    // gamma corrected version
    const vec3 palette[NUM_COLORS] = vec3[NUM_COLORS] (
        RGB( 0, 0, 0 ),            // black
        RGB( 255, 255, 255 ),    // white
        RGB( 104, 55,  43 ),    // red
        RGB( 112, 164, 178 ),    // cyan
        RGB( 111, 61,  134 ),    // purple
        RGB( 88,  141, 67 ),    // green
        RGB( 53,  40,  121 ),    // blue
        RGB( 184, 199, 111 ),    // yellow
        RGB( 111, 79,  37 ),    // orange
        RGB( 67,  57,  0 ),        // brown
        RGB( 154, 103, 89 ),    // light red
        RGB( 68,  68,  68 ),    // dark grey
        RGB( 108, 108, 108 ),    // grey
        RGB( 154, 210, 132 ),    // light green
        RGB( 108, 94,  181 ),    // light blue
        RGB( 149, 149, 149 )    // light grey
    );
    vec2 uv = ( var_TexCoord );
    vec2 uvPixelated = floor( gl_FragCoord.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

    vec3 quantizationPeriod = vec3( 1.0 / float(NUM_COLORS) );
    vec3 quantDeviation = Deviation( palette );

    // get pixellated base color
    vec3 color = texture( u_fragmentMap0, uvPixelated * u_windowCoords.xy * u_nonPowerOfTwo.xy ).rgb;

    vec2 uvDither = uvPixelated;
    //if( u_uniformParm0.x > 1.0 )
    {
        uvDither = gl_FragCoord.xy / ( RESOLUTION_DIVISOR / u_uniformParm0.x );
    }
    float dither = DitherArray8x8( uvDither ) - 0.5;

    //color.rgb += vec3( dither, dither, dither ) * quantizationPeriod;
    color.rgb += vec3( dither, dither, dither ) * quantDeviation * u_uniformParm0.y;

    // find closest color match from C64 color palette
    color = LinearSearch( color.rgb, palette );

    //color = texture( u_fragmentMap0, uv ).rgb;
    _gl_FragColor = vec4( color, 1.0 );
}
