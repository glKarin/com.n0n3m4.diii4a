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
    // COMPUTE UNADJUSTED/ADJUSTED TEXCOORDS
    vec4 unadjustedTC = gl_FragCoord * u_windowCoords;

    // compute warp factor
    vec4 R0 = texture(u_fragmentMap1, unadjustedTC.xy);
    R0 = R0 * var_Color;
    vec4 warpFactor = 1.0 - R0;

    // compute _currentRender preturbed texcoords
    R0 = unadjustedTC;

    R0.xy = R0.xy - 0.5;
    R0.xy = R0.xy * warpFactor.xy;
    R0.xy = R0.xy + 0.5;

    // scale by the screen non-power-of-two-adjust
    R0 = R0 * u_nonPowerOfTwo;

    // do the _currentRender lookup
    _gl_FragColor = vec4(texture(u_fragmentMap0, R0.xy).xyz, 1.0);
}
