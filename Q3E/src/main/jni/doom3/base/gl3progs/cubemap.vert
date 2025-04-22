#version 300 es
//#pragma optimize(off)

precision mediump float;

in lowp vec4 attr_Color;
in vec4 attr_TexCoord;
in highp vec4 attr_Vertex;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;
uniform lowp vec4 u_colorAdd;
uniform lowp vec4 u_colorModulate;
uniform vec4 u_viewOrigin;

out vec3 var_TexCoord;
out lowp vec4 var_Color;

void main(void)
{
    var_TexCoord = (u_textureMatrix * attr_TexCoord).xyz;

    var_Color = (attr_Color / 255.0) * u_colorModulate + u_colorAdd;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
