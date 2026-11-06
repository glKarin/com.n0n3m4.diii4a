
// z-fill
GLSL_SHADER const char ES3_ZFILL_VERT[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"in highp vec4 attr_Vertex;\n"
"in vec4 attr_TexCoord;\n"
"uniform mat4 u_textureMatrix;\n"
"\n"
"uniform highp mat4 u_modelViewProjectionMatrix;\n"
"\n"
"out vec2 var_TexDiffuse;\n"
"\n"
"void main(void)\n"
"{\n"
"    var_TexDiffuse = (u_textureMatrix * attr_TexCoord).xy;\n"
"\n"
"    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;\n"
"}\n"
;
