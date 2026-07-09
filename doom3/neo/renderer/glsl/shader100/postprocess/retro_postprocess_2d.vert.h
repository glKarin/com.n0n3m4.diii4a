
// retro postprocess 2D general vertex shader
GLSL_SHADER const char RETRO_POSTPROCESS_2D_VERT[] =
"#version 100\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"attribute vec4 attr_TexCoord;\n"
"attribute highp vec4 attr_Vertex;\n"
"\n"
"varying vec2 var_TexCoord;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_Position = attr_Vertex;\n"
"    gl_Position.y = -attr_Vertex.y;\n"
"\n"
"    var_TexCoord = attr_TexCoord.xy;\n"
"}\n"
;
