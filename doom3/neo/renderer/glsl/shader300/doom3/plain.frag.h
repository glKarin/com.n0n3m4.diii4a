
GLSL_SHADER const char ES3_PLAIN_FRAG[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"uniform sampler2D u_fragmentMap0;\n"
"uniform lowp vec4 u_glColor;\n"
"\n"
"in vec2 var_TexDiffuse;\n"
"in lowp vec4 var_Color;\n"
"out vec4 _gl_FragColor;\n"
"\n"
"void main(void)\n"
"{\n"
"    _gl_FragColor = texture(u_fragmentMap0, var_TexDiffuse) * u_glColor * var_Color;\n"
"}\n"
;
