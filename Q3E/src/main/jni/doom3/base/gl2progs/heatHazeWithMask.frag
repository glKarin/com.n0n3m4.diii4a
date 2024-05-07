#version 100
//#pragma optimize(off)

#define HEATHAZE_BFG 1

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform sampler2D u_fragmentMap1;
uniform sampler2D u_fragmentMap2;
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_windowCoords;
varying highp vec4 var_TexCoord0;
varying highp vec4 var_TexCoord1;
varying highp vec4 var_TexCoord2;

// # texture 0 is _currentRender
// # texture 1 is a normal map that we will use to deform texture 0
// # texture 2 is a mask texture
// #
// # env[0] is the 1.0 to _currentRender conversion
// # env[1] is the fragment.position to 0.0 - 1.0 conversion
void main(void)
{
#if HEATHAZE_BFG // BFG
    // load the distortion map
    vec4 mask = texture2D( u_fragmentMap2, var_TexCoord0.xy );
    // kill the pixel if the distortion wound up being very small
    mask.xy -= 0.01f;
     if ( any( lessThan( mask, vec4( 0.0 ) ) ) ) { discard; }
    // load the filtered normal map and convert to -1 to 1 range
    vec4 bumpMap = ( texture2D( u_fragmentMap1, var_TexCoord1.xy ) * 2.0f ) - 1.0f;
    vec2 localNormal = bumpMap.wy;
    localNormal *= mask.xy;
    // calculate the screen texcoord in the 0.0 to 1.0 range
    vec2 screenTexCoord = (gl_FragCoord.xy ) * u_windowCoords.xy;
    screenTexCoord += ( localNormal * var_TexCoord2.xy );
    screenTexCoord = clamp( screenTexCoord, vec2(0.0), vec2(1.0) );
    screenTexCoord = screenTexCoord * u_nonPowerOfTwo.xy;
    gl_FragColor = texture2D( u_fragmentMap0, screenTexCoord );

#else // 2004

    vec4 localNormal, mask, R0; // TEMP localNormal, mask, R0;

    vec4 subOne = vec4(-1.0, -1.0, -1.0, -1.0); // PARAM subOne = { -1, -1, -1, -1 };
    vec4 scaleTwo = vec4(2.0, 2.0, 2.0, 2.0); // PARAM scaleTwo = { 2, 2, 2, 2 };

    // # load the distortion map
    mask = texture2D(u_fragmentMap2, var_TexCoord0.xy); // TEX  mask, fragment.texcoord[0], texture[2], 2D;

    // # kill the pixel if the distortion wound up being very small
    mask.xy = (mask.xy) - (vec2(0.01)); // SUB  mask.xy, mask, 0.01;
    if (any(lessThan(mask, vec4(0.0)))) discard; // KIL  mask;

    //# load the filtered normal map and convert to -1 to 1 range
    localNormal = texture2D(u_fragmentMap1, var_TexCoord1.xy); // TEX  localNormal, fragment.texcoord[1], texture[1], 2D;
    localNormal.x = localNormal.a; // MOV  localNormal.x, localNormal.a;
    localNormal = (localNormal) * (scaleTwo) + (subOne); // MAD  localNormal, localNormal, scaleTwo, subOne;
    //localNormal.z = sqrt(max(0.0, 1.0-localNormal.x*localNormal.x-localNormal.y*localNormal.y));
    localNormal = (localNormal) * (mask); // MUL  localNormal, localNormal, mask;

    // # calculate the screen texcoord in the 0.0 to 1.0 range
    R0 = (gl_FragCoord) * (u_windowCoords); // MUL  R0, fragment.position, program.env[1];

    //localNormal.x /= localNormal.z;
    //localNormal.y /= localNormal.z;

    // # offset by the scaled localNormal and clamp it to 0.0 - 1.0
    R0 = clamp((localNormal) * (var_TexCoord2) + (R0), 0.0, 1.0); // MAD_SAT R0, localNormal, fragment.texcoord[2], R0;

    // # scale by the screen non-power-of-two-adjust
    R0 = (R0) * (u_nonPowerOfTwo); // MUL  R0, R0, program.env[0];

    // # load the screen render
    gl_FragColor = texture2D(u_fragmentMap0, R0.xy); // TEX  result.color.xyz, R0, texture[0], 2D;
#endif
}