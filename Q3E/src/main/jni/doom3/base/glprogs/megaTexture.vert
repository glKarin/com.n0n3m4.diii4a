#version 100
//#pragma optimize(off)

precision mediump float;

attribute highp vec4 attr_Vertex;
attribute vec4 attr_TexCoord;
attribute lowp vec4 attr_Color;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform vec4 u_megaTextureLevel0;
uniform vec4 u_megaTextureLevel1;
uniform vec4 u_megaTextureLevel2;
uniform vec4 u_megaTextureLevel3;
uniform vec4 u_megaTextureLevel4;
uniform vec4 u_megaTextureLevel5;
uniform vec4 u_megaTextureLevel6;
uniform vec4 u_megaTextureLevel7;
uniform vec4 u_megaTextureLevel8;

varying vec4 var_TexCoord0;
varying vec4 var_TexCoord1;
varying vec4 var_TexCoord2;
varying vec4 var_TexCoord3;
varying vec4 var_TexCoord4;
varying vec4 var_TexCoord5;
varying vec4 var_TexCoord6;
varying vec4 var_TexCoord7;

void main(void)
{
    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;

    // Megatexture masks, (texCoord * scale) + offset.
    var_TexCoord0.xy = ( attr_TexCoord.xy * u_megaTextureLevel0.w ) + u_megaTextureLevel0.xy;
    var_TexCoord1.xy = ( attr_TexCoord.xy * u_megaTextureLevel1.w ) + u_megaTextureLevel1.xy;
    var_TexCoord2.xy = ( attr_TexCoord.xy * u_megaTextureLevel2.w ) + u_megaTextureLevel2.xy;
    var_TexCoord3.xy = ( attr_TexCoord.xy * u_megaTextureLevel3.w ) + u_megaTextureLevel3.xy;
    var_TexCoord4.xy = ( attr_TexCoord.xy * u_megaTextureLevel4.w ) + u_megaTextureLevel4.xy;
    var_TexCoord5.xy = ( attr_TexCoord.xy * u_megaTextureLevel5.w ) + u_megaTextureLevel5.xy;
    var_TexCoord6.xy = ( attr_TexCoord.xy * u_megaTextureLevel6.w ) + u_megaTextureLevel6.xy;

    // Images only have scale.
    var_TexCoord7.xy = ( attr_TexCoord.xy * u_megaTextureLevel6.w );
    var_TexCoord7.zw = attr_TexCoord.xy;
}
