
// simple vertex/texcoord vertex shader
GLSL_SHADER const char ES3_SIMPLE_VERTEX_TEXCOORD_VERT[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"in vec4 attr_TexCoord;\n"
"in highp vec4 attr_Vertex;\n"
"\n"
"uniform highp mat4 u_modelViewProjectionMatrix;\n"
"\n"
"out vec2 var_TexDiffuse;\n"
"\n"
"void main(void)\n"
"{\n"
"    var_TexDiffuse = attr_TexCoord.xy;\n"
"\n"
"    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;\n"
"}\n"
;
