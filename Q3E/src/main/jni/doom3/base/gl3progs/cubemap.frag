#version 300 es
//#pragma optimize(off)

precision mediump float;

in vec3 var_TexCoord;
in lowp vec4 var_Color;

uniform samplerCube u_fragmentCubeMap0;
uniform lowp vec4 u_glColor;
out vec4 _gl_FragColor;

void main(void)
{
    _gl_FragColor = texture(u_fragmentCubeMap0, var_TexCoord) * u_glColor * var_Color;
}
