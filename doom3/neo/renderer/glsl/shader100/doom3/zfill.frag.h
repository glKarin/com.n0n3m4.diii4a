
// z-fill
GLSL_SHADER const char ZFILL_FRAG[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"uniform lowp vec4 u_glColor;\n"
"uniform sampler2D u_fragmentMap0;\n"
"uniform lowp float u_alphaTest;\n"
"\n"
"varying vec2 var_TexDiffuse;\n"
"\n"
"void main(void)\n"
"{\n"
"    if (u_alphaTest > texture2D(u_fragmentMap0, var_TexDiffuse).a) {\n"
"        discard;\n"
"    }\n"
"\n"
"    gl_FragColor = u_glColor;\n"
"}\n"
;
