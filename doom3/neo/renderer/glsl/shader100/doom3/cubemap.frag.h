
// cubemap
GLSL_SHADER const char CUBEMAP_FRAG[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"varying vec3 var_TexCoord;\n"
"varying lowp vec4 var_Color;\n"
"\n"
"uniform samplerCube u_fragmentCubeMap0;\n"
"uniform lowp vec4 u_glColor;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_FragColor = textureCube(u_fragmentCubeMap0, var_TexCoord) * u_glColor * var_Color;\n"
"}\n"
;
