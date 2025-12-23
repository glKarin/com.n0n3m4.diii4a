#version 300 es
//#pragma optimize(off)

precision mediump float;

uniform mediump usampler2D u_fragmentMap0; // stencil index

uniform int u_uniformParm0; // 0=RGB, 1=R, 2=G, 4=B

in vec2 var_TexDiffuse;
out vec4 _gl_FragColor;

void main(void)
{
    float index = float(texture(u_fragmentMap0, var_TexDiffuse).r) / 255.0;
    float r = bool(u_uniformParm0 & 1) || u_uniformParm0 == 0 ? index : 0.0;
    float g = bool(u_uniformParm0 & 2) || u_uniformParm0 == 0 ? index : 0.0;
    float b = bool(u_uniformParm0 & 4) || u_uniformParm0 == 0 ? index : 0.0;
    _gl_FragColor = vec4(r, g, b, 1.0);
}
