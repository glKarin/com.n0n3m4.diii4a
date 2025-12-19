#version 100

//#pragma optimize(off)

precision mediump float;

attribute vec4 attr_TexCoord;
attribute vec3 attr_Tangent;
attribute vec3 attr_Bitangent;
attribute vec3 attr_Normal;
attribute highp vec4 attr_Vertex;

uniform vec4 u_viewOrigin;
uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp mat4 u_modelMatrix;
uniform highp mat4 u_modelViewMatrix;
uniform highp mat4 u_projectionMatrix;
uniform vec4 u_vertexParm1; // deform magnitude (1.0 is reasonable, 2.0 is twice as wavy, 0.5 is half as wavy, etc)

varying vec4 var_TexCoord0; // texCoord[0] is the normal map texcoord
varying highp vec4 var_TexCoord1; // texCoord[1] is the vector to the eye in global space //karin: must high precison on GLSL
varying vec4 var_TexCoord2; // texCoord[2] is the surface tangent to global coordiantes
varying vec4 var_TexCoord3; // texCoord[3] is the surface bitangent to global coordiantes
varying vec4 var_TexCoord4; // texCoord[4] is the surface normal to global coordiantes
varying vec4 var_TexCoord6; // texCoord[6] is the copied deform magnitude
//varying vec4 var_TexCoord7; // texCoord[7] is the copied scroll

void main(void)
{
    vec4 scale = vec4( 2.0, 2.0, 2.0, 2.0 );
    vec4 R0 = u_viewOrigin - attr_Vertex;

    // texture 1 is the vector to the eye in global coordinates
    var_TexCoord1.x = dot(R0.xyz, u_modelMatrix[0].xyz);
    var_TexCoord1.y = dot(R0.xyz, u_modelMatrix[1].xyz);
    var_TexCoord1.z = dot(R0.xyz, u_modelMatrix[2].xyz);

    // texture 2 gets the transformed tangent
    var_TexCoord2.x = dot(attr_Tangent, u_modelMatrix[0].xyz);
    var_TexCoord3.x = dot(attr_Tangent, u_modelMatrix[1].xyz);
    var_TexCoord4.x = dot(attr_Tangent, u_modelMatrix[2].xyz);

    // texture 3 gets the transformed bitangent
    var_TexCoord2.y = dot(attr_Bitangent, u_modelMatrix[0].xyz);
    var_TexCoord3.y = dot(attr_Bitangent, u_modelMatrix[1].xyz);
    var_TexCoord4.y = dot(attr_Bitangent, u_modelMatrix[2].xyz);

    // texture 4 gets the transformed normal
    var_TexCoord2.z = dot(attr_Normal, u_modelMatrix[0].xyz);
    var_TexCoord3.z = dot(attr_Normal, u_modelMatrix[1].xyz);
    var_TexCoord4.z = dot(attr_Normal, u_modelMatrix[2].xyz);

    // take the texture coordinates and add a scroll
    var_TexCoord0 = attr_TexCoord;

    // texture 6 takes the deform magnitude and scales it by the projection distance
    R0 = vec4( 1.0, 0.0, 0.0, 1.0 );
    R0.z = dot(attr_Vertex, u_modelViewMatrix[2]);

    float R1 = dot(R0, u_projectionMatrix[0]);
    float R2 = dot(R0, u_projectionMatrix[3]);

    // don't let the recip get near zero for polygons that cross the view plane
    R2 = max(R2, 1.0);

    R2 = 1.0 / R2;
    R1 = R1 * R2;

    // clamp the distance so the the deformations don't get too wacky near the view
    R1 = min(R1, 0.02);
    var_TexCoord6 = R1 * u_vertexParm1;

    //var_TexCoord7 = scroll;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
