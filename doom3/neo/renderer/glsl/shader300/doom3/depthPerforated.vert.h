
// z-fill perforated depth
GLSL_SHADER const char ES3_DEPTH_PERFORATED_VERT[] =
"#version 300 es\n"
"//#pragma optimize(off)\n"
"\n"
"precision mediump float;\n"
"\n"
"in vec4 attr_TexCoord;\n"
"in highp vec4 attr_Vertex;\n"
"\n"
"uniform mat4 u_textureMatrix;\n"
"uniform highp mat4 u_modelViewProjectionMatrix;\n"
"\n"
"out vec2 var_TexDiffuse;\n"
"\n"
"void main(void)\n"
"{\n"
"    var_TexDiffuse = (u_textureMatrix * attr_TexCoord).xy;\n"
"\n"
"    gl_Position = attr_Vertex * u_modelViewProjectionMatrix;\n"
"}\n"
;
