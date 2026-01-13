#version 100
//#pragma optimize(off)

precision mediump float;

#define RESOLUTION_DIVISOR 4.0
#define NUM_COLORS 4

uniform sampler2D u_fragmentMap0; // currentRender

uniform highp vec4 u_windowCoords;
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_uniformParm0; // rpJitterTexScale

varying vec2 var_TexCoord;
varying lowp vec4 var_Color;

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
int ArrayDitherArray8x8[64];

int glsl100_mod( int a, int b )
{
    return a - (a / b) * b;
}

float DitherArray8x8( vec2 pos )
{
/* 8x8 Bayer ordered dithering  */
/* pattern.  Each input pixel   */
/* is scaled to the 0..63 range */
/* before looking in this table */
/* to determine the action.     */

    int stippleOffset = glsl100_mod( int( pos.y ), 8 ) * 8 + glsl100_mod( int( pos.x ), 8 );
    int byte = ArrayDitherArray8x8[stippleOffset];
    float stippleThreshold = float(byte) / 64.0f;
    return stippleThreshold;
}

void main(void)
{
    ArrayDitherArray8x8[0] = 0;
    ArrayDitherArray8x8[1] = 32;
    ArrayDitherArray8x8[2] = 8;
    ArrayDitherArray8x8[3] = 40;
    ArrayDitherArray8x8[4] = 2;
    ArrayDitherArray8x8[5] = 34;
    ArrayDitherArray8x8[6] = 10;
    ArrayDitherArray8x8[7] = 42;
    ArrayDitherArray8x8[8] = 48;
    ArrayDitherArray8x8[9] = 16;
    ArrayDitherArray8x8[10] = 56;
    ArrayDitherArray8x8[11] = 24;
    ArrayDitherArray8x8[12] = 50;
    ArrayDitherArray8x8[13] = 18;
    ArrayDitherArray8x8[14] = 58;
    ArrayDitherArray8x8[15] = 26;
    ArrayDitherArray8x8[16] = 12;
    ArrayDitherArray8x8[17] = 44;
    ArrayDitherArray8x8[18] = 4;
    ArrayDitherArray8x8[19] = 36;
    ArrayDitherArray8x8[20] = 14;
    ArrayDitherArray8x8[21] = 46;
    ArrayDitherArray8x8[22] = 6;
    ArrayDitherArray8x8[23] = 38;
    ArrayDitherArray8x8[24] = 60;
    ArrayDitherArray8x8[25] = 28;
    ArrayDitherArray8x8[26] = 52;
    ArrayDitherArray8x8[27] = 20;
    ArrayDitherArray8x8[28] = 62;
    ArrayDitherArray8x8[29] = 30;
    ArrayDitherArray8x8[30] = 54;
    ArrayDitherArray8x8[31] = 22;
    ArrayDitherArray8x8[32] = 3;
    ArrayDitherArray8x8[33] = 35;
    ArrayDitherArray8x8[34] = 11;
    ArrayDitherArray8x8[35] = 43;
    ArrayDitherArray8x8[36] = 1;
    ArrayDitherArray8x8[37] = 33;
    ArrayDitherArray8x8[38] = 9;
    ArrayDitherArray8x8[39] = 41;
    ArrayDitherArray8x8[40] = 51;
    ArrayDitherArray8x8[41] = 19;
    ArrayDitherArray8x8[42] = 59;
    ArrayDitherArray8x8[43] = 27;
    ArrayDitherArray8x8[44] = 49;
    ArrayDitherArray8x8[45] = 17;
    ArrayDitherArray8x8[46] = 57;
    ArrayDitherArray8x8[47] = 25;
    ArrayDitherArray8x8[48] = 15;
    ArrayDitherArray8x8[49] = 47;
    ArrayDitherArray8x8[50] = 7;
    ArrayDitherArray8x8[51] = 39;
    ArrayDitherArray8x8[52] = 13;
    ArrayDitherArray8x8[53] = 45;
    ArrayDitherArray8x8[54] = 5;
    ArrayDitherArray8x8[55] = 37;
    ArrayDitherArray8x8[56] = 63;
    ArrayDitherArray8x8[57] = 31;
    ArrayDitherArray8x8[58] = 55;
    ArrayDitherArray8x8[59] = 23;
    ArrayDitherArray8x8[60] = 61;
    ArrayDitherArray8x8[61] = 29;
    ArrayDitherArray8x8[62] = 53;
    ArrayDitherArray8x8[63] = 21;

    // CGA
    // https://lospec.com/palette-list/cga-mibend4
    vec3 palette[NUM_COLORS];
    palette[0] = RGB( 41, 31, 35 );
    palette[1] = RGB( 189, 80, 47 );
    palette[2] = RGB( 52, 209, 175 );
    palette[3] = RGB( 247, 236, 185 );

    vec2 uv = ( var_TexCoord );
    vec2 uvPixelated = floor( gl_FragCoord.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

    vec3 quantizationPeriod = vec3( 1.0 / float(NUM_COLORS) );
    vec3 quantDeviation = Deviation( palette );

    // get pixellated base color
    vec3 color = texture2D( u_fragmentMap0, uvPixelated * u_windowCoords.xy * u_nonPowerOfTwo.xy ).rgb;

    vec2 uvDither = uvPixelated;
    //if( u_uniformParm0.x > 1.0 )
    {
        uvDither = gl_FragCoord.xy / ( RESOLUTION_DIVISOR / u_uniformParm0.x );
    }
    float dither = DitherArray8x8( uvDither ) - 0.5;

    color.rgb += vec3( dither, dither, dither ) * quantDeviation * u_uniformParm0.y;

    // find closest color match from C64 color palette
    color = LinearSearch( color.rgb, palette );

    gl_FragColor = vec4( color, 1.0 );
}
