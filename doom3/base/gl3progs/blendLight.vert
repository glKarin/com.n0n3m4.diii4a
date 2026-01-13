#version 300 es
//#pragma optimize(off)

precision mediump float;

in highp vec4 attr_Vertex;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform mat4 u_fogMatrix;

out vec2 var_TexFog;
out vec2 var_TexFogEnter;

void main(void)
{
    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;

    // What will be computed:
    //
    // vec4 tc;
    // tc.x = dot( u_fogMatrix[0], attr_Vertex );
    // tc.y = dot( u_fogMatrix[1], attr_Vertex );
    // tc.z = 0.0;
    // tc.w = dot( u_fogMatrix[2], attr_Vertex );
    // var_TexFog.xy = tc.xy / tc.w;
    //
    // var_TexFogEnter.x = dot( u_fogMatrix[3], attr_Vertex );
    // var_TexFogEnter.y = 0.5;

    // Optimized version:
    //
    var_TexFog = vec2(dot( u_fogMatrix[0], attr_Vertex ), dot( u_fogMatrix[1], attr_Vertex )) / dot( u_fogMatrix[2], attr_Vertex );
    var_TexFogEnter = vec2( dot( u_fogMatrix[3], attr_Vertex ), 0.5 );
}
