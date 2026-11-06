
// shadow
GLSL_SHADER const char SHADOW_FRAG[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision lowp float;\n"
"\n"
"varying lowp vec4 var_Color;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_FragColor = var_Color;\n"
"}\n"
;
