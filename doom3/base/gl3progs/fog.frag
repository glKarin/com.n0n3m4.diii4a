#version 300 es
//#pragma optimize(off)

precision mediump float;

in vec2 var_TexFog;            // input Fog TexCoord
in vec2 var_TexFogEnter;       // input FogEnter TexCoord

uniform sampler2D u_fragmentMap0;   // Fog Image
uniform sampler2D u_fragmentMap1;   // Fog Enter Image
uniform lowp vec4 u_fogColor;       // Fog Color
out vec4 _gl_FragColor;

void main(void)
{
    _gl_FragColor = texture( u_fragmentMap0, var_TexFog ) * texture( u_fragmentMap1, var_TexFogEnter ) * vec4(u_fogColor.rgb, 1.0);
}
