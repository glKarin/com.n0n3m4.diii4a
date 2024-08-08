#version 300 es
//#pragma optimize(off)

precision mediump float;

in vec3 var_TexCoord;
in lowp vec4 var_Color;
in vec3 var_normalLocal;
in vec3 var_toEyeLocal;

uniform samplerCube u_fragmentCubeMap0;
uniform lowp vec4 u_glColor;
out vec4 _gl_FragColor;

void main(void)
{
#if 1
    vec3 normal = normalize(var_normalLocal);
    vec3 toEye = normalize(var_toEyeLocal);

    // calculate reflection vector
    float dotEN = dot(toEye, normal);
    vec3 reflectionVector = vec3(2.0) * normal * vec3(dotEN) - toEye;

    _gl_FragColor = texture(u_fragmentCubeMap0, reflectionVector) * u_glColor * var_Color;
#else // d3asm
    _gl_FragColor = texture(u_fragmentCubeMap0, var_TexCoord) * u_glColor * var_Color;
#endif
}
