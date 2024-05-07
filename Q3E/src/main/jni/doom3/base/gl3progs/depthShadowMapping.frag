/*
	macros:
		_DEBUG: output z value to red component.
*/
#version 300 es
//#pragma optimize(off)

precision highp float;

#ifdef _DEBUG
out vec4 _gl_FragColor;
#endif

void main(void)
{
#ifdef _DEBUG
    _gl_FragColor = vec4((gl_FragCoord.z + 1.0) * 0.5, 0.0, 0.0, 1.0); // DEBUG
#endif
}