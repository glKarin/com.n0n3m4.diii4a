#version 100
//#pragma optimize(off)

precision mediump float;

varying vec2 var_TexDiffuse;
varying vec2 var_TexClip;

uniform sampler2D u_fragmentMap0;
uniform sampler2D u_fragmentMap1;
uniform lowp float u_alphaTest;
uniform lowp vec4 u_glColor;

void main(void)
{
    if (u_alphaTest > (texture2D(u_fragmentMap0, var_TexDiffuse).a * texture2D(u_fragmentMap1, var_TexClip).a) ) {
discard;
    }

    gl_FragColor = u_glColor;
}
