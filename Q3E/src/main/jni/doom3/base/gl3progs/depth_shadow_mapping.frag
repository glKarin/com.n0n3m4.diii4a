/*
	macros:
		_DEBUG: output z value to red component.
*/
#version 300 es
//#pragma optimize(off)

precision mediump float;

uniform highp vec4 globalLightOrigin;

out vec4 _gl_FragColor;

void main(void)
{
#ifdef _DEBUG
    _gl_FragColor = vec4((gl_FragCoord.z + 1.0) * 0.5, 0.0, 0.0, 1.0); // DEBUG
#else
    _gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
#endif
}