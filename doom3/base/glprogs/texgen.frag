#version 100
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform lowp vec4 u_glColor;

varying vec4 var_TexCoord;
varying lowp vec4 var_Color;

void main(void)
{
    // we always do a projective texture lookup so that we can support texgen
    // materials without a separate shader. Basic materials will have texture
    // coordinates with w = 1 which will result in a NOP projection when tex2Dproj
    // gets called.
    gl_FragColor = texture2DProj( u_fragmentMap0, var_TexCoord.xyw ) * u_glColor * var_Color;
}
