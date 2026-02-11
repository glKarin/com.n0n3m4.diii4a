#version 300 es
//#pragma optimize(off)

precision mediump float;

in vec2 var_TexDiffuse;
in vec2 var_TexClip;

uniform sampler2D u_fragmentMap0;
uniform sampler2D u_fragmentMap1;
uniform lowp float u_alphaTest;
uniform lowp vec4 u_glColor;
out vec4 _gl_FragColor;

void main(void)
{
    if (u_alphaTest > (texture(u_fragmentMap0, var_TexDiffuse).a * texture(u_fragmentMap1, var_TexClip).a) ) {
        discard;
    }

    _gl_FragColor = u_glColor;
}
