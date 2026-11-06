
// occlusion testing
GLSL_SHADER const char OCCLUSIONTEST_FRAG[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"//#define _DEBUG\n"
"#ifdef _DEBUG\n"
"uniform lowp vec4 u_glColor;\n"
"#endif\n"
"\n"
"void main(void)\n"
"{\n"
"#ifdef _DEBUG\n"
"    gl_FragColor = u_glColor;\n"
"#endif\n"
"}\n"
;

