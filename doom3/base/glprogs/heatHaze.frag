#version 100
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform sampler2D u_fragmentMap1;
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_windowCoords;
varying highp vec4 var_TexCoord1;
varying highp vec4 var_TexCoord2;

// # texture 0 is _currentRender
// # texture 1 is a normal map that we will use to deform texture 0
// #
// # env[0] is the 1.0 to _currentRender conversion
// # env[1] is the fragment.position to 0.0 - 1.0 conversion
void main(void)
{
    // load the filtered normal map and convert to -1 to 1 range
    vec4 bumpMap = ( texture2D( u_fragmentMap1, var_TexCoord1.xy ) * 2.0 ) - 1.0;
    vec2 localNormal = bumpMap.wy;
    // calculate the screen texcoord in the 0.0 to 1.0 range
    vec2 screenTexCoord = (gl_FragCoord.xy ) * u_windowCoords.xy;
    screenTexCoord += ( localNormal * var_TexCoord2.xy );
    screenTexCoord = clamp( screenTexCoord, vec2(0.0), vec2(1.0) );
    screenTexCoord = screenTexCoord * u_nonPowerOfTwo.xy;
    gl_FragColor = texture2D( u_fragmentMap0, screenTexCoord );
}
