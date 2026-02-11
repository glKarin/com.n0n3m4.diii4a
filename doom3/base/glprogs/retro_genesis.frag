#version 100
//#pragma optimize(off)

precision mediump float;

#define RESOLUTION_DIVISOR 4.0
#define NUM_COLORS 64 // original 61

uniform sampler2D u_fragmentMap0; // currentRender

uniform highp vec4 u_windowCoords;
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_uniformParm0; // rpJitterTexScale

varying vec2 var_TexCoord;
varying lowp vec4 var_Color;

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

    // + very good dithering variety at dark grey values
    // + does not leak too much color into grey values
    // + good saturation when colors are really needed
    // - a bit too strong visible dithering pattern
    // https://lospec.com/palette-list/famicube
    vec3 palette[NUM_COLORS];
    palette[0] = RGB( 0, 0, 0 );
    palette[1] = RGB( 21, 21, 21 );
    palette[2] = RGB( 35, 23, 18 );
    palette[3] = RGB( 23, 40, 8 );
    palette[4] = RGB( 13, 32, 48 );
    palette[5] = RGB( 33, 22, 64 );
    palette[6] = RGB( 0, 78, 0 );
    palette[7] = RGB( 79, 21, 7 );
    palette[8] = RGB( 52, 52, 52 );
    palette[9] = RGB( 92, 60, 13 );
    palette[10] = RGB( 0, 96, 75 );
    palette[11] = RGB( 55, 109, 3 );
    palette[12] = RGB( 0, 23, 125 );
    palette[13] = RGB( 0, 82, 128 );
    palette[14] = RGB( 65, 93, 102 );
    palette[15] = RGB( 135, 22, 70 );
    palette[16] = RGB( 130, 60, 61 );
    palette[17] = RGB( 19, 157, 8 );
    palette[18] = RGB( 90, 25, 145 );
    palette[19] = RGB( 61, 52, 165 );
    palette[20] = RGB( 173, 78, 26 );
    palette[21] = RGB( 32, 181, 98 );
    palette[22] = RGB( 106, 180, 23 );
    palette[23] = RGB( 147, 151, 23 );
    palette[24] = RGB( 174, 108, 55 );
    palette[25] = RGB( 123, 123, 123 );
    palette[26] = RGB( 2, 74, 202 );
    palette[27] = RGB( 10, 152, 172 );
    palette[28] = RGB( 106, 49, 202 );
    palette[29] = RGB( 88, 211, 50 );
    palette[30] = RGB( 224, 60, 40 );
    palette[31] = RGB( 207, 60, 113 );
    palette[32] = RGB( 163, 40, 179 );
    palette[33] = RGB( 204, 143, 21 );
    palette[34] = RGB( 140, 214, 18 );
    palette[35] = RGB( 113, 166, 161 );
    palette[36] = RGB( 218, 101, 94 );
    palette[37] = RGB( 98, 100, 220 );
    palette[38] = RGB( 182, 193, 33 );
    palette[39] = RGB( 197, 151, 130 );
    palette[40] = RGB( 10, 137, 255 );
    palette[41] = RGB( 246, 143, 55 );
    palette[42] = RGB( 168, 168, 168 );
    palette[43] = RGB( 225, 130, 137 );
    palette[44] = RGB( 37, 226, 205 );
    palette[45] = RGB( 91, 168, 255 );
    palette[46] = RGB( 255, 187, 49 );
    palette[47] = RGB( 190, 235, 113 );
    palette[48] = RGB( 204, 105, 228 );
    palette[49] = RGB( 166, 117, 254 );
    palette[50] = RGB( 155, 160, 239 );
    palette[51] = RGB( 245, 183, 132 );
    palette[52] = RGB( 255, 231, 55 );
    palette[53] = RGB( 255, 130, 206 );
    palette[54] = RGB( 226, 215, 181 );
    palette[55] = RGB( 213, 156, 252 );
    palette[56] = RGB( 152, 220, 255 );
    palette[57] = RGB( 215, 215, 215 );
    palette[58] = RGB( 189, 255, 202 );
    palette[59] = RGB( 238, 255, 169 );
    palette[60] = RGB( 226, 201, 255 );
    palette[61] = RGB( 255, 233, 197 );
    palette[62] = RGB( 254, 201, 237 );
    palette[63] = RGB( 255, 255, 255 );

    const vec3 medianAbsoluteDeviation = RGB( 63, 175, 2 );
    const vec3 deviation = RGB( 76, 62, 75 );

    vec2 uv = ( var_TexCoord );
    vec2 uvPixelated = floor( gl_FragCoord.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

    vec3 quantizationPeriod = vec3( 1.0 / float(NUM_COLORS) );
    vec3 quantDeviation = deviation;

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

    //color = texture2D( u_fragmentMap0, uv ).rgb;
    gl_FragColor = vec4( color, 1.0 );
}
