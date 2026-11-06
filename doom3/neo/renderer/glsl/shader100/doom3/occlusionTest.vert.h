
// occlusion testing
GLSL_SHADER const char OCCLUSIONTEST_VERT[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"attribute highp vec4 attr_Vertex;\n"
"\n"
"uniform highp mat4 u_modelViewProjectionMatrix;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;\n"
"}\n"
;

