#version 100
//#pragma optimize(off)

precision mediump float;

attribute highp vec4 attr_Vertex;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform lowp vec4 u_glColor;
uniform vec4 u_lightOrigin;

varying lowp vec4 var_Color;

void main(void)
{
    gl_Position = u_modelViewProjectionMatrix * (attr_Vertex.w * u_lightOrigin + attr_Vertex - u_lightOrigin);

    var_Color = u_glColor;
}
