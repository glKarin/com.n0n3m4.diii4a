#version 100
//#pragma optimize(off)

precision lowp float;

varying lowp vec4 var_Color;

void main(void)
{
    gl_FragColor = var_Color;
}
