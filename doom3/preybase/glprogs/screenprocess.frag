#version 100
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_fragmentParm0;
uniform highp vec4 u_fragmentParm1;
uniform highp vec4 u_fragmentParm2;
uniform highp vec4 u_fragmentParm3;

varying highp vec4 var_TexCoord;

// # texture 0 is _currentRender
// # texture 1 is a normal map that we will use to deform texture 0

// # env[0] is the 1.0 to _currentRender conversion
// # env[1] is the fragment.position to 0.0 - 1.0 conversion

void main(void)
{
// # scale by the screen non-power-of-two-adjust
    vec3 color = texture2D( u_fragmentMap0, var_TexCoord.st * u_nonPowerOfTwo.xy ).rgb;
    float R0 = dot( color, vec3( 0.3, 0.5, 0.1 ) );
    color = mix( vec3( R0 ), color, u_fragmentParm0.xyz );
    color = mix( vec3( u_fragmentParm1.y ), color, u_fragmentParm1.x );
    color = color * u_fragmentParm2.xyz + u_fragmentParm3.xyz;
    gl_FragColor = vec4( color, 0.2 );
}
