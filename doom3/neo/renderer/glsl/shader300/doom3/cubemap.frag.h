
// cubemap
GLSL_SHADER const char ES3_CUBEMAP_FRAG[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"in vec3 var_TexCoord;\n"
"in lowp vec4 var_Color;\n"
"\n"
"uniform samplerCube u_fragmentCubeMap0;\n"
"uniform lowp vec4 u_glColor;\n"
"out vec4 _gl_FragColor;\n"
"\n"
"void main(void)\n"
"{\n"
"    _gl_FragColor = texture(u_fragmentCubeMap0, var_TexCoord) * u_glColor * var_Color;\n"
"}\n"
;
