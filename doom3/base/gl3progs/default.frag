#version 300 es
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform lowp vec4 u_glColor;

in vec2 var_TexDiffuse;
in lowp vec4 var_Color;
out vec4 _gl_FragColor;

void main(void)
{
    _gl_FragColor = texture(u_fragmentMap0, var_TexDiffuse) * u_glColor * var_Color;
}
