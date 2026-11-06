
// screenprocess
GLSL_SHADER const char SCREENPROCESS_VERT[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"attribute highp vec4 attr_Vertex;\n"
"attribute highp vec4 attr_TexCoord;\n"
"\n"
"uniform highp mat4 u_modelViewProjectionMatrix;\n"
"\n"
"varying highp vec4 var_TexCoord;\n"
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
