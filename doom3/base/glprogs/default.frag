#version 100
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform lowp vec4 u_glColor;

varying vec2 var_TexDiffuse;
varying lowp vec4 var_Color;

void main(void)
{
    gl_FragColor = texture2D(u_fragmentMap0, var_TexDiffuse) * u_glColor * var_Color;
}
