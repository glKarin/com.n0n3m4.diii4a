#version 100
//#pragma optimize(off)

precision mediump float;

varying vec2 var_TexFog;            // input Fog TexCoord
varying vec2 var_TexFogEnter;       // input FogEnter TexCoord

uniform sampler2D u_fragmentMap0;   // Fog Image
uniform sampler2D u_fragmentMap1;   // Fog Enter Image
uniform lowp vec4 u_fogColor;       // Fog Color

void main(void)
{
    gl_FragColor = texture2D( u_fragmentMap0, var_TexFog ) * texture2D( u_fragmentMap1, var_TexFogEnter ) * vec4(u_fogColor.rgb, 1.0);
}
