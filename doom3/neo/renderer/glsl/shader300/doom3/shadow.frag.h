
// shadow
GLSL_SHADER const char ES3_SHADOW_FRAG[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision lowp float;\n"
"\n"
"in lowp vec4 var_Color;\n"
"out vec4 _gl_FragColor;\n"
"\n"
"void main(void)\n"
"{\n"
"    _gl_FragColor = var_Color;\n"
"}\n"
;
