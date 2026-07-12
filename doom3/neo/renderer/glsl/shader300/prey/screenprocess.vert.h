
// screenprocess
GLSL_SHADER const char ES3_SCREENPROCESS_VERT[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"in highp vec4 attr_Vertex;\n"
"in highp vec4 attr_TexCoord;\n"
"\n"
"uniform highp mat4 u_modelViewProjectionMatrix;\n"
"\n"
"out highp vec4 var_TexCoord;\n"
"\n"
"// # texture 0 takes the texture coordinates unmodified\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;\n"
"\n"
"    var_TexCoord = attr_TexCoord;\n"
"}\n"
;
