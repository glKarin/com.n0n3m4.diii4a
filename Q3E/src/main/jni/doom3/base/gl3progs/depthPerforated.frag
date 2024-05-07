/*
	macros:
		_DEBUG: output z value to red component.
*/
#version 300 es
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform lowp float u_alphaTest;
uniform lowp vec4 u_glColor;

in vec2 var_TexDiffuse;
#ifdef _DEBUG
out vec4 _gl_FragColor;
#endif

void main(void)
{
	if (u_alphaTest > texture(u_fragmentMap0, var_TexDiffuse).a) {
		discard;
	}

#ifdef _DEBUG
    _gl_FragColor = vec4((gl_FragCoord.z + 1.0) * 0.5, 0.0, 0.0, 1.0); // DEBUG
#endif
}