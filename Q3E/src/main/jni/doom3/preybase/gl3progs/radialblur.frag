#version 300 es
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_fragmentParm0;
uniform highp vec4 u_fragmentParm1;
uniform highp vec4 u_fragmentParm2;
uniform highp vec4 u_fragmentParm3;
uniform highp vec4 u_fragmentParm4;
uniform highp vec4 u_fragmentParm5;

in highp vec4 var_TexCoord;
in lowp vec4 var_Color;
out vec4 _gl_FragColor;

// # texture 0 is _currentRender
// # texture 1 is a normal map that we will use to deform texture 0

// # env[0] is the 1.0 to _currentRender conversion
// # env[1] is the fragment.position to 0.0 - 1.0 conversion

void main(void)
{
    vec2 size = u_fragmentParm0.xy;
    vec2 strength = u_fragmentParm0.xy;

// # calculate the screen texcoord in the 0.0 to 1.0 range
// # scale by the screen non-power-of-two-adjust
    vec2 pos = var_TexCoord.st * u_nonPowerOfTwo.xy;

    vec3 color = vec3( 0.0 );

    vec2 halfTemp = u_nonPowerOfTwo.xy * 0.5;
    vec2 R1 = pos.xy - halfTemp;

    float displace = 1.0;
    vec2 tc = R1 * displace + halfTemp;
    vec3 c = texture( u_fragmentMap0, tc ).rgb;
    color = c * strength.y + color;

    displace = displace - size.x;
    tc = R1 * displace + halfTemp;
    c = texture( u_fragmentMap0, tc ).rgb;
    color = c * strength.y + color;

    displace = displace - size.x;
    tc = R1 * displace + halfTemp;
    c = texture( u_fragmentMap0, tc ).rgb;
    color = c * strength.y + color;

    displace = displace - size.x;
    tc = R1 * displace + halfTemp;
    c = texture( u_fragmentMap0, tc ).rgb;
    color = c * strength.y + color;

    displace = displace - size.x;
    tc = R1 * displace + halfTemp;
    c = texture( u_fragmentMap0, tc ).rgb;
    color = c * strength.y + color;

    displace = displace - size.x;
    tc = R1 * displace + halfTemp;
    c = texture( u_fragmentMap0, tc ).rgb;
    color = c * strength.y + color;

    displace = displace - size.x;
    tc = R1 * displace + halfTemp;
    c = texture( u_fragmentMap0, tc ).rgb;
    color = c * strength.y + color;

    displace = displace - size.x;
    tc = R1 * displace + halfTemp;
    c = texture( u_fragmentMap0, tc ).rgb;
    color = c * strength.y + color;

    vec3 R0 = texture( u_fragmentMap0, pos ).rgb;

    color = clamp( u_fragmentParm1.x * color + u_fragmentParm1.y, 0.0, 1.0 );

    color = mix( color, R0, u_fragmentParm0.z );

    float R1F = dot( color, vec3( 0.3, 0.5, 0.1 ) );
    color = mix( vec3( R1F ), color, u_fragmentParm2.x );
    color = mix( vec3( u_fragmentParm3.y ), color, u_fragmentParm3.x );
    R1F = R1F * u_fragmentParm4.a;
    color = mix( color, u_fragmentParm4.xyz, R1F );
    color = color * 0.95 + u_fragmentParm5.xyz;

    _gl_FragColor = vec4( color, 1.0 );
}
