#version 100
//#pragma optimize(off)

#define HEATHAZE_BFG 1

precision mediump float;

attribute vec4 attr_TexCoord;
attribute highp vec4 attr_Vertex;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp mat4 u_modelViewMatrix;
uniform highp mat4 u_projectionMatrix;

uniform highp vec4 u_vertexParm0; // texture scrolling
uniform highp vec4 u_vertexParm1; // magnitude of the distortion

varying highp vec4 var_TexCoord0;
varying highp vec4 var_TexCoord1;
varying highp vec4 var_TexCoord2;

// # input:
// #
// # texcoord[0] TEX0 texcoords
// #
// # local[0] scroll
// # local[1] deform magnitude (1.0 is reasonable, 2.0 is twice as wavy, 0.5 is half as wavy, etc)
// #
// # output:
// #
// # texture 0 is _currentRender
// # texture 1 is a normal map that we will use to deform texture 0
// # texture 2 is a mask texture
// #
// # texCoord[0] is the model surface texture coords unmodified for the mask
// # texCoord[1] is the model surface texture coords with a scroll
// # texCoord[2] is the copied deform magnitude
void main(void)
{
#if HEATHAZE_BFG // BFG
    // texture 0 takes the texture coordinates unmodified
    var_TexCoord0 = vec4( attr_TexCoord.xy, 0.0, 0.0 );
    // texture 1 takes the texture coordinates and adds a scroll
    vec4 textureScroll = u_vertexParm0;
    var_TexCoord1 = vec4( attr_TexCoord.xy, 0.0, 0.0 ) + textureScroll;
    // texture 2 takes the deform magnitude and scales it by the projection distance
    vec4 vec = vec4( 0.0, 1.0, 0.0, 1.0 );
    vec.z  = dot( attr_Vertex, u_modelViewMatrix[2] );
    // magicProjectionAdjust is a magic scalar that scales the projection since we changed from
    // using the X axis to the Y axis to calculate R1.  It is an approximation to closely match
    // what the original game did
    const float magicProjectionAdjust = 0.43f;
    float x = dot ( vec, u_projectionMatrix[1] ) * magicProjectionAdjust;
    float w = dot ( vec, u_projectionMatrix[3] );
    // don't let the recip get near zero for polygons that cross the view plane
    w = max( w, 1.0 );
    x /= w;
    // clamp the distance so the the deformations don't get too wacky near the view
    x = min( x, 0.02 );
    vec4 deformMagnitude = u_vertexParm1;
    var_TexCoord2 = vec4(x) * deformMagnitude;
    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;

#else // 2004

    vec4 R0, R1, R2; // TEMP R0, R1, R2;

    // # texture 0 takes the texture coordinates unmodified
    var_TexCoord0 = attr_TexCoord; // MOV  result.texcoord[0], vertex.texcoord[0];

    // # texture 1 takes the texture coordinates and adds a scroll
    var_TexCoord1 = (attr_TexCoord) + (u_vertexParm0); // ADD  result.texcoord[1], vertex.texcoord[0], program.local[0];

    // # texture 2 takes the deform magnitude and scales it by the projection distance
    vec4 vec = vec4(1.0, 0.0, 0.0, 1.0); // PARAM vec = { 1, 0, 0, 1 };

    R0 = vec; // MOV  R0, vec;
    R0.z = dot(attr_Vertex, (u_modelViewMatrix)[2]); // DP4  R0.z, vertex.position, state.matrix.modelview.row[2];

    R1 = vec4(dot(R0, (u_projectionMatrix)[0])); // DP4  R1, R0, state.matrix.projection.row[0];
    R2 = vec4(dot(R0, (u_projectionMatrix)[3])); // DP4  R2, R0, state.matrix.projection.row[3];

    // # don't let the recip get near zero for polygons that cross the view plane
    R2 = max(R2, vec4(1.0)); // MAX  R2, R2, 1;

    R2 = vec4(1.0 / R2.w); // RCP  R2, R2.w;
    R1 = (R1) * (R2); // MUL  R1, R1, R2;

    // # clamp the distance so the the deformations don't get too wacky near the view
    R1 = min(R1, vec4(0.02)); // MIN  R1, R1, 0.02;

    var_TexCoord2 = (R1) * (u_vertexParm1); // MUL  result.texcoord[2], R1, program.local[1];

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
#endif
}