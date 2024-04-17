#version 300 es
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform highp vec4 u_nonPowerOfTwo;

in highp vec4 var_TexCoord;
in lowp vec4 var_Color;
out vec4 _gl_FragColor;

void main(void)
{
    vec4 src = texture( u_fragmentMap0, var_TexCoord.xy * u_nonPowerOfTwo.xy /* scale by the screen non-power-of-two-adjust */ );
    vec4 target = var_Color * dot( vec3( 0.333, 0.333, 0.333 ), src.xyz );
    _gl_FragColor = mix( src, target, var_TexCoord.z );
}