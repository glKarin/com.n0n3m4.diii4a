#version 100
//#pragma optimize(off)

precision mediump float;

uniform highp sampler2D u_fragmentMap0; // depth

uniform bool u_uniformParm0; // not pack to RGBA

varying vec2 var_TexDiffuse;

vec4 pack (highp float depth)
{
    const highp vec4 bitSh = vec4(256.0 * 256.0 * 256.0, 256.0 * 256.0, 256.0, 1.0);
    const highp vec4 bitMsk = vec4(0.0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0);
    highp vec4 comp = fract(depth * bitSh);
    comp -= comp.xxyz * bitMsk;
    return depth < 1.0 ? comp : vec4(1.0, 1.0, 1.0, 1.0);
}

void main(void)
{
    highp vec4 depth = texture2D(u_fragmentMap0, var_TexDiffuse);
    vec4 packColor = pack(depth.r);
    vec4 color = vec4(depth.r);
    gl_FragColor = u_uniformParm0 ? color : packColor;
}
