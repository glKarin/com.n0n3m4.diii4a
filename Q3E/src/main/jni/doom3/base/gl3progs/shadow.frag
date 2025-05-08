#version 300 es
//#pragma optimize(off)

precision lowp float;

in lowp vec4 var_Color;
out vec4 _gl_FragColor;

void main(void)
{
    _gl_FragColor = var_Color;
}
