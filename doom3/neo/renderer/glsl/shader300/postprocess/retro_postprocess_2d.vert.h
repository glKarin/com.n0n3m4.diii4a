
// retro postprocess 2D general vertex shader
GLSL_SHADER const char ES3_RETRO_POSTPROCESS_2D_VERT[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"in vec4 attr_TexCoord;\n"
"in highp vec4 attr_Vertex;\n"
"\n"
"out vec2 var_TexCoord;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_Position = attr_Vertex;\n"
"    gl_Position.y = -attr_Vertex.y;\n"
"\n"
"    var_TexCoord = attr_TexCoord.xy;\n"
"}\n"
;
