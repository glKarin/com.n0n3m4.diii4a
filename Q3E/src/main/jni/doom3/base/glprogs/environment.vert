#version 100
//#pragma optimize(off)

precision mediump float;

attribute highp vec4 attr_Vertex;
attribute lowp vec4 attr_Color;
attribute vec3 attr_TexCoord;
attribute vec3 attr_Normal;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform mat4 u_modelViewMatrix;
uniform mat4 u_textureMatrix;
uniform lowp vec4 u_colorAdd;
uniform lowp vec4 u_colorModulate;
uniform vec4 u_viewOrigin;

varying vec3 var_TexCoord;
varying lowp vec4 var_Color;
varying vec3 var_normalLocal;
varying vec3 var_toEyeLocal;

void main(void)
{
#if 1
    var_normalLocal = attr_Normal;
    var_toEyeLocal = vec3(u_viewOrigin - attr_Vertex);
#else // d3asm
    var_TexCoord = (u_textureMatrix * reflect( normalize( u_modelViewMatrix * attr_Vertex ),
    // This suppose the modelView matrix is orthogonal
    // Otherwise, we should use the inverse transpose
        u_modelViewMatrix * vec4(attr_TexCoord,0.0) )).xyz ;
#endif

    var_Color = (attr_Color / 255.0) * u_colorModulate + u_colorAdd;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
