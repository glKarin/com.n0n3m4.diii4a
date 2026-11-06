
// shadow map(depth)
GLSL_SHADER const char ES3_DEPTH_FRAG[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision highp float;\n"
"\n"
"#ifdef _DEBUG\n"
"out vec4 _gl_FragColor;\n"
"#endif\n"
"\n"
"void main(void)\n"
"{\n"
"#ifdef _DEBUG\n"
"    _gl_FragColor = vec4((gl_FragCoord.z + 1.0) * 0.5, 0.0, 0.0, 1.0); // DEBUG\n"
"#endif\n"
"}\n"
;
