#version 300 es
//#pragma optimize(off)

precision mediump float;

in vec4 attr_TexCoord;
in highp vec4 attr_Vertex;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp mat4 u_modelViewMatrix;
uniform highp mat4 u_projectionMatrix;

uniform highp vec4 u_vertexParm0; // texture scrolling
uniform highp vec4 u_vertexParm1; // magnitude of the distortion

out highp vec4 var_TexCoord0;
out highp vec4 var_TexCoord1;
out highp vec4 var_TexCoord2;

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
    const float magicProjectionAdjust = 0.43;
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
}
