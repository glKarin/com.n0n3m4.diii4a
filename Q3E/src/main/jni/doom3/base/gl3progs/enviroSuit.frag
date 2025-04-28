#version 300 es
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0; // texture 0 is _currentRender
uniform sampler2D u_fragmentMap1; // texture 1 is a normal map that we will use to deform texture 0

uniform highp vec4 u_nonPowerOfTwo; // env[0] is the 1.0 to _currentRender conversion
uniform highp vec4 u_windowCoords; // env[1] is the fragment.position to 0.0 - 1.0 conversion

in vec2 var_TexCoord0;
// in vec4 var_TexCoord1;
in vec4 var_Color;

out vec4 _gl_FragColor;

void main(void)
{
    vec4 unadjustedTC = gl_FragCoord * u_windowCoords;
    vec4 adjustedTC = unadjustedTC * u_nonPowerOfTwo;

    vec4 R0 = textureProj(u_fragmentMap1, unadjustedTC.xyw);
    R0 = R0 * var_Color;
    vec4 warpFactor = 1.0 - R0;

    R0 = unadjustedTC;

    float R1 = 1.0 / R0.w;
    R0.w = 1.0;
    R0.xy = R0.xy * R1;

    R0.xy = R0.xy - 0.5;
    R0.xy = R0.xy * warpFactor.xy;
    R0.xy = R0.xy + 0.5;

    R0 = R0 * u_nonPowerOfTwo;

    _gl_FragColor = vec4(textureProj(u_fragmentMap0, R0.xyw).xyz, 1.0);
}
