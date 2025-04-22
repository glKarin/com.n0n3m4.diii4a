#version 300 es
//#pragma optimize(off)

precision mediump float;

in highp vec4 attr_Vertex;
in lowp vec4 attr_Color;
in vec3 attr_TexCoord;
in vec3 attr_Normal;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform mat4 u_modelViewMatrix;
uniform mat4 u_textureMatrix;
uniform lowp float u_colorAdd;
uniform lowp float u_colorModulate;
uniform vec4 u_viewOrigin;

out vec3 var_TexCoord;
out lowp vec4 var_Color;
out vec3 var_normalLocal;
out vec3 var_toEyeLocal;

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

    var_Color = attr_Color * u_colorModulate + u_colorAdd;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
