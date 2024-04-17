#version 100
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform highp vec4 u_nonPowerOfTwo;

varying highp vec4 var_TexCoord;
varying lowp vec4 var_Color;

void main(void)
{
    vec4 src = texture2D( u_fragmentMap0, var_TexCoord.xy * u_nonPowerOfTwo.xy /* scale by the screen non-power-of-two-adjust */ );
    vec4 target = var_Color * dot( vec3( 0.333, 0.333, 0.333 ), src.xyz );
    gl_FragColor = mix( src, target, var_TexCoord.z );
}