
GLSL_SHADER const char PLAIN_FRAG[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"uniform sampler2D u_fragmentMap0;\n"
"uniform lowp vec4 u_glColor;\n"
"\n"
"varying vec2 var_TexDiffuse;\n"
"varying lowp vec4 var_Color;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_FragColor = texture2D(u_fragmentMap0, var_TexDiffuse) * u_glColor * var_Color;\n"
"}\n"
;
