
// occlusion testing
GLSL_SHADER const char ES3_OCCLUSIONTEST_FRAG[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"//#define _DEBUG\n"
"#ifdef _DEBUG\n"
"uniform lowp vec4 u_glColor;\n"
"out vec4 _gl_FragColor;\n"
"#endif\n"
"\n"
"void main(void)\n"
"{\n"
"#ifdef _DEBUG\n"
"    _gl_FragColor = u_glColor;\n"
"#endif\n"
"}\n"
;

