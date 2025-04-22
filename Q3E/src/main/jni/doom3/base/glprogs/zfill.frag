#version 100
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform lowp float u_alphaTest;
uniform lowp vec4 u_glColor;

varying vec2 var_TexDiffuse;

void main(void)
{
    if (u_alphaTest > texture2D(u_fragmentMap0, var_TexDiffuse).a) {
        discard;
    }

    gl_FragColor = u_glColor;
}
